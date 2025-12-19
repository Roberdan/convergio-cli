/**
 * CONVERGIO EDUCATION - QUIZ ENGINE
 *
 * Generates adaptive quizzes with multiple question types,
 * accessibility support, and LLM-powered content generation.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

// ============================================================================
// TYPES AND STRUCTURES
// ============================================================================

typedef enum {
    QUIZ_MULTIPLE_CHOICE,
    QUIZ_TRUE_FALSE,
    QUIZ_OPEN_ANSWER,
    QUIZ_SEQUENCE,
    QUIZ_MATCHING,
    QUIZ_CLOZE,
    QUIZ_IMAGE_IDENTIFY
} QuizQuestionType;

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD,
    DIFFICULTY_ADAPTIVE
} QuizDifficulty;

typedef struct {
    char* text;
    bool is_correct;
} QuizOption;

typedef struct {
    int64_t id;
    QuizQuestionType type;
    char* question_text;
    char* hint;
    char* explanation;
    QuizOption* options;
    int option_count;
    char* correct_answer;  // For open/cloze
    QuizDifficulty difficulty;
    int points;
    bool timed;
    int time_limit_seconds;
} QuizQuestion;

typedef struct {
    int64_t id;
    char* title;
    char* subject;
    char* topic;
    QuizQuestion* questions;
    int question_count;
    QuizDifficulty difficulty;
    bool adaptive;
    int total_points;
} Quiz;

typedef struct {
    int question_index;
    char* user_answer;
    bool is_correct;
    int points_earned;
    int time_taken_seconds;
} QuizAnswer;

typedef struct {
    Quiz* quiz;
    QuizAnswer* answers;
    int answered_count;
    int correct_count;
    int total_score;
    time_t started_at;
    time_t completed_at;
    float percentage;
} QuizSession;

// ============================================================================
// ACCESSIBILITY ADAPTATIONS
// ============================================================================

typedef struct {
    bool use_tts;
    float tts_speed;
    bool extended_time;
    float time_multiplier;
    bool simplified_options;
    int max_options;
    bool no_timed_questions;
    bool show_hints_always;
    bool larger_text;
    bool high_contrast;
} QuizAccessibility;

static QuizAccessibility get_quiz_accessibility(const EducationAccessibility* a) {
    QuizAccessibility qa = {
        .use_tts = false,
        .tts_speed = 1.0f,
        .extended_time = false,
        .time_multiplier = 1.0f,
        .simplified_options = false,
        .max_options = 4,
        .no_timed_questions = false,
        .show_hints_always = false,
        .larger_text = false,
        .high_contrast = false
    };

    if (!a) return qa;

    // Dyslexia adaptations
    if (a->dyslexia) {
        qa.use_tts = true;
        qa.tts_speed = a->tts_speed > 0 ? a->tts_speed : 0.9f;
        qa.extended_time = true;
        qa.time_multiplier = 1.5f;
        qa.larger_text = true;
    }

    // Dyscalculia adaptations
    if (a->dyscalculia) {
        qa.no_timed_questions = true;
        qa.show_hints_always = true;
    }

    // ADHD adaptations
    if (a->adhd) {
        qa.simplified_options = true;
        qa.max_options = 3;  // Fewer options to reduce overwhelm
    }

    // Cerebral palsy adaptations
    if (a->cerebral_palsy) {
        qa.extended_time = true;
        qa.time_multiplier = 2.0f;
        qa.no_timed_questions = a->cerebral_palsy_severity >= SEVERITY_MODERATE;
    }

    // Autism adaptations
    if (a->autism) {
        qa.show_hints_always = false;  // May prefer to try without
    }

    // General preferences
    if (a->high_contrast) {
        qa.high_contrast = true;
    }

    return qa;
}

// ============================================================================
// QUESTION GENERATION
// ============================================================================

/**
 * Create a multiple choice question
 */
QuizQuestion* quiz_create_multiple_choice(const char* question,
                                           const char** options,
                                           int option_count,
                                           int correct_index,
                                           const char* explanation) {
    if (!question || !options || option_count < 2 || correct_index >= option_count) {
        return NULL;
    }

    QuizQuestion* q = calloc(1, sizeof(QuizQuestion));
    if (!q) return NULL;

    q->type = QUIZ_MULTIPLE_CHOICE;
    q->question_text = strdup(question);
    q->explanation = explanation ? strdup(explanation) : NULL;
    q->option_count = option_count;
    q->options = calloc(option_count, sizeof(QuizOption));
    q->difficulty = DIFFICULTY_MEDIUM;
    q->points = 1;

    for (int i = 0; i < option_count; i++) {
        q->options[i].text = strdup(options[i]);
        q->options[i].is_correct = (i == correct_index);
    }

    return q;
}

/**
 * Create a true/false question
 */
QuizQuestion* quiz_create_true_false(const char* statement,
                                      bool correct_answer,
                                      const char* explanation) {
    if (!statement) return NULL;

    QuizQuestion* q = calloc(1, sizeof(QuizQuestion));
    if (!q) return NULL;

    q->type = QUIZ_TRUE_FALSE;
    q->question_text = strdup(statement);
    q->explanation = explanation ? strdup(explanation) : NULL;
    q->option_count = 2;
    q->options = calloc(2, sizeof(QuizOption));
    q->difficulty = DIFFICULTY_EASY;
    q->points = 1;

    q->options[0].text = strdup("Vero");
    q->options[0].is_correct = correct_answer;
    q->options[1].text = strdup("Falso");
    q->options[1].is_correct = !correct_answer;

    return q;
}

/**
 * Create a cloze (fill in the blank) question
 */
QuizQuestion* quiz_create_cloze(const char* text_with_blank,
                                 const char* correct_answer,
                                 const char* hint) {
    if (!text_with_blank || !correct_answer) return NULL;

    QuizQuestion* q = calloc(1, sizeof(QuizQuestion));
    if (!q) return NULL;

    q->type = QUIZ_CLOZE;
    q->question_text = strdup(text_with_blank);
    q->correct_answer = strdup(correct_answer);
    q->hint = hint ? strdup(hint) : NULL;
    q->difficulty = DIFFICULTY_MEDIUM;
    q->points = 1;

    return q;
}

/**
 * Create a sequence ordering question
 */
QuizQuestion* quiz_create_sequence(const char* question,
                                    const char** items,
                                    int item_count) {
    if (!question || !items || item_count < 2) return NULL;

    QuizQuestion* q = calloc(1, sizeof(QuizQuestion));
    if (!q) return NULL;

    q->type = QUIZ_SEQUENCE;
    q->question_text = strdup(question);
    q->option_count = item_count;
    q->options = calloc(item_count, sizeof(QuizOption));
    q->difficulty = DIFFICULTY_HARD;
    q->points = item_count; // More points for complex ordering

    // Store correct order (options are already in correct order)
    for (int i = 0; i < item_count; i++) {
        q->options[i].text = strdup(items[i]);
    }

    return q;
}

// ============================================================================
// QUIZ GENERATION FROM LLM
// ============================================================================

static const char* QUIZ_PROMPT_TEMPLATE =
    "Generate a quiz about: %s\n\n"
    "Topic content:\n%s\n\n"
    "Requirements:\n"
    "- Generate %d questions\n"
    "- Difficulty: %s\n"
    "- Question types: %s\n"
    "%s"
    "\nFormat each question as JSON with:\n"
    "{\n"
    "  \"type\": \"multiple_choice|true_false|cloze|sequence\",\n"
    "  \"question\": \"...\",\n"
    "  \"options\": [\"A\", \"B\", \"C\", \"D\"],\n"
    "  \"correct\": 0,\n"
    "  \"explanation\": \"...\"\n"
    "}\n";

/**
 * Generate quiz using LLM
 */
Quiz* quiz_generate_from_llm(const char* topic, const char* content,
                              int question_count, QuizDifficulty difficulty,
                              const EducationAccessibility* access) {
    if (!topic) return NULL;

    QuizAccessibility qa = get_quiz_accessibility(access);

    // Determine difficulty string
    const char* diff_str = "medium";
    switch (difficulty) {
        case DIFFICULTY_EASY: diff_str = "easy"; break;
        case DIFFICULTY_HARD: diff_str = "hard"; break;
        case DIFFICULTY_ADAPTIVE: diff_str = "mixed, starting easy"; break;
        default: break;
    }

    // Determine question types based on accessibility
    const char* types = "multiple_choice, true_false";
    if (!qa.simplified_options) {
        types = "multiple_choice, true_false, cloze, sequence";
    }

    // Accessibility requirements for prompt
    char access_req[512] = "";
    if (qa.simplified_options) {
        strcat(access_req, "- Use maximum 3 options per question\n");
    }
    if (qa.show_hints_always) {
        strcat(access_req, "- Include a helpful hint for each question\n");
    }
    if (access && access->dyslexia) {
        strcat(access_req, "- Use simple, clear language\n");
        strcat(access_req, "- Keep questions short (max 2 sentences)\n");
    }

    // Build prompt (in real implementation, send to LLM)
    // For now, create a sample quiz

    Quiz* quiz = calloc(1, sizeof(Quiz));
    if (!quiz) return NULL;

    quiz->title = malloc(strlen(topic) + 20);
    sprintf(quiz->title, "Quiz: %s", topic);
    quiz->topic = strdup(topic);
    quiz->difficulty = difficulty;
    quiz->adaptive = (difficulty == DIFFICULTY_ADAPTIVE);
    quiz->question_count = question_count > 0 ? question_count : 5;
    quiz->questions = calloc(quiz->question_count, sizeof(QuizQuestion));

    // Generate sample questions (placeholder)
    // In real implementation, parse LLM response

    return quiz;
}

// ============================================================================
// QUIZ SESSION MANAGEMENT
// ============================================================================

/**
 * Start a new quiz session
 */
QuizSession* quiz_session_start(Quiz* quiz) {
    if (!quiz) return NULL;

    QuizSession* session = calloc(1, sizeof(QuizSession));
    if (!session) return NULL;

    session->quiz = quiz;
    session->answers = calloc(quiz->question_count, sizeof(QuizAnswer));
    session->answered_count = 0;
    session->correct_count = 0;
    session->total_score = 0;
    session->started_at = time(NULL);

    return session;
}

/**
 * Submit an answer
 */
bool quiz_submit_answer(QuizSession* session, int question_index,
                        const char* answer, int time_taken) {
    if (!session || !answer || question_index >= session->quiz->question_count) {
        return false;
    }

    QuizQuestion* q = &session->quiz->questions[question_index];
    QuizAnswer* a = &session->answers[question_index];

    a->question_index = question_index;
    a->user_answer = strdup(answer);
    a->time_taken_seconds = time_taken;

    // Check correctness based on question type
    bool correct = false;
    switch (q->type) {
        case QUIZ_MULTIPLE_CHOICE:
        case QUIZ_TRUE_FALSE: {
            int selected = atoi(answer);
            if (selected >= 0 && selected < q->option_count) {
                correct = q->options[selected].is_correct;
            }
            break;
        }
        case QUIZ_CLOZE: {
            // Case-insensitive comparison
            correct = (strcasecmp(answer, q->correct_answer) == 0);
            break;
        }
        default:
            break;
    }

    a->is_correct = correct;
    a->points_earned = correct ? q->points : 0;

    session->answered_count++;
    if (correct) session->correct_count++;
    session->total_score += a->points_earned;

    return correct;
}

/**
 * Complete the quiz session and save grade to libretto
 */
void quiz_session_complete(QuizSession* session) {
    if (!session) return;

    session->completed_at = time(NULL);
    session->percentage = (float)session->correct_count /
                          (float)session->quiz->question_count * 100.0f;
}

/**
 * Complete quiz and save to student's libretto
 */
void quiz_session_complete_with_grade(QuizSession* session, int64_t student_id,
                                       const char* maestro_id) {
    if (!session) return;

    // Complete the session first
    quiz_session_complete(session);

    // Generate grade comment based on performance
    char comment[256] = "";
    if (session->percentage >= 90.0f) {
        snprintf(comment, sizeof(comment), "Eccellente! %d/%d corrette.",
                 session->correct_count, session->quiz->question_count);
    } else if (session->percentage >= 70.0f) {
        snprintf(comment, sizeof(comment), "Buon lavoro! %d/%d corrette.",
                 session->correct_count, session->quiz->question_count);
    } else if (session->percentage >= 50.0f) {
        snprintf(comment, sizeof(comment), "Sufficiente. %d/%d corrette. Ripassare il materiale.",
                 session->correct_count, session->quiz->question_count);
    } else {
        snprintf(comment, sizeof(comment), "Da rivedere. %d/%d corrette. Consiglio ripasso approfondito.",
                 session->correct_count, session->quiz->question_count);
    }

    // Save to libretto using the API
    int64_t grade_id = libretto_add_quiz_grade(
        student_id,
        maestro_id ? maestro_id : "ED00",
        session->quiz->subject ? session->quiz->subject : "Generale",
        session->quiz->topic,
        session->correct_count,
        session->quiz->question_count,
        comment
    );

    if (grade_id > 0) {
        printf("\n✅ Voto salvato nel libretto (ID: %lld)\n", (long long)grade_id);
    }

    // Also log the activity
    libretto_add_log_entry(
        student_id,
        maestro_id,
        "quiz",
        session->quiz->subject,
        session->quiz->topic,
        (int)(session->completed_at - session->started_at) / 60,  // duration in minutes
        comment
    );
}

/**
 * Get feedback for a question
 */
const char* quiz_get_feedback(const QuizSession* session, int question_index) {
    if (!session || question_index >= session->quiz->question_count) {
        return NULL;
    }

    QuizQuestion* q = &session->quiz->questions[question_index];
    QuizAnswer* a = &session->answers[question_index];

    if (a->is_correct) {
        return q->explanation ? q->explanation : "Correct!";
    } else {
        return q->explanation ? q->explanation : "Incorrect. Try reviewing the material.";
    }
}

// ============================================================================
// ADAPTIVE DIFFICULTY
// ============================================================================

/**
 * Adjust difficulty based on recent performance
 */
QuizDifficulty quiz_adjust_difficulty(const QuizSession* session) {
    if (!session || session->answered_count < 3) {
        return DIFFICULTY_MEDIUM;
    }

    // Calculate recent accuracy (last 5 questions)
    int recent_start = session->answered_count > 5 ? session->answered_count - 5 : 0;
    int recent_correct = 0;

    for (int i = recent_start; i < session->answered_count; i++) {
        if (session->answers[i].is_correct) recent_correct++;
    }

    float recent_accuracy = (float)recent_correct /
                            (float)(session->answered_count - recent_start);

    if (recent_accuracy >= 0.8f) {
        return DIFFICULTY_HARD;
    } else if (recent_accuracy <= 0.4f) {
        return DIFFICULTY_EASY;
    }

    return DIFFICULTY_MEDIUM;
}

// ============================================================================
// EXPORT FUNCTIONS
// ============================================================================

/**
 * Export quiz to PDF for printing
 */
int quiz_export_pdf(const Quiz* quiz, const char* output_path,
                    const QuizAccessibility* access) {
    if (!quiz || !output_path) return -1;

    // Build HTML content first
    size_t buf_size = 8192;
    char* html = malloc(buf_size);
    if (!html) return -1;

    char* ptr = html;
    int remaining = buf_size;

    // HTML header with accessibility styles
    int written = snprintf(ptr, remaining,
        "<html><head><style>"
        "body { font-family: %s; font-size: %s; line-height: 1.6; }"
        ".question { margin: 20px 0; padding: 15px; border: 1px solid #ccc; }"
        ".options { margin-left: 20px; }"
        ".option { margin: 8px 0; }"
        "</style></head><body>"
        "<h1>%s</h1>",
        access && access->larger_text ? "OpenDyslexic, Arial" : "Arial",
        access && access->larger_text ? "16pt" : "12pt",
        quiz->title
    );
    ptr += written;
    remaining -= written;

    // Questions
    for (int i = 0; i < quiz->question_count && remaining > 500; i++) {
        QuizQuestion* q = &quiz->questions[i];

        written = snprintf(ptr, remaining,
            "<div class='question'>"
            "<p><strong>%d.</strong> %s</p>"
            "<div class='options'>",
            i + 1, q->question_text
        );
        ptr += written;
        remaining -= written;

        for (int j = 0; j < q->option_count && remaining > 100; j++) {
            written = snprintf(ptr, remaining,
                "<div class='option'>%c) %s</div>",
                'A' + j, q->options[j].text
            );
            ptr += written;
            remaining -= written;
        }

        written = snprintf(ptr, remaining, "</div></div>");
        ptr += written;
        remaining -= written;
    }

    snprintf(ptr, remaining, "</body></html>");

    // Convert HTML to PDF
    char temp_html[256];
    snprintf(temp_html, sizeof(temp_html), "/tmp/quiz_%d.html", getpid());

    FILE* f = fopen(temp_html, "w");
    if (f) {
        fprintf(f, "%s", html);
        fclose(f);

        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "wkhtmltopdf %s %s 2>/dev/null || "
                 "pandoc %s -o %s 2>/dev/null",
                 temp_html, output_path,
                 temp_html, output_path);

        int result = system(cmd);
        unlink(temp_html);
        free(html);
        return result == 0 ? 0 : -1;
    }

    free(html);
    return -1;
}

// ============================================================================
// CLEANUP
// ============================================================================

void quiz_question_free(QuizQuestion* q) {
    if (!q) return;
    free(q->question_text);
    free(q->hint);
    free(q->explanation);
    free(q->correct_answer);
    if (q->options) {
        for (int i = 0; i < q->option_count; i++) {
            free(q->options[i].text);
        }
        free(q->options);
    }
}

void quiz_free(Quiz* quiz) {
    if (!quiz) return;
    free(quiz->title);
    free(quiz->subject);
    free(quiz->topic);
    if (quiz->questions) {
        for (int i = 0; i < quiz->question_count; i++) {
            quiz_question_free(&quiz->questions[i]);
        }
        free(quiz->questions);
    }
    free(quiz);
}

void quiz_session_free(QuizSession* session) {
    if (!session) return;
    if (session->answers) {
        for (int i = 0; i < session->answered_count; i++) {
            free(session->answers[i].user_answer);
        }
        free(session->answers);
    }
    free(session);
}

// ============================================================================
// CLI COMMAND HANDLER
// ============================================================================

/**
 * Handle /quiz command
 */
int quiz_command_handler(int argc, char** argv,
                         const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /quiz <topic> [--count n] [--difficulty easy|medium|hard]\n");
        return 1;
    }

    const char* topic = argv[1];
    int count = 5;
    QuizDifficulty difficulty = DIFFICULTY_ADAPTIVE;

    // Parse options
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            count = atoi(argv[++i]);
            if (count < 1) count = 5;
            if (count > 20) count = 20;
        } else if (strcmp(argv[i], "--difficulty") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "easy") == 0) difficulty = DIFFICULTY_EASY;
            else if (strcmp(argv[i], "hard") == 0) difficulty = DIFFICULTY_HARD;
            else difficulty = DIFFICULTY_MEDIUM;
        }
    }

    printf("Generating %d-question quiz on: %s\n", count, topic);

    const EducationAccessibility* access = profile ? profile->accessibility : NULL;

    Quiz* quiz = quiz_generate_from_llm(topic, NULL, count, difficulty, access);
    if (!quiz) {
        fprintf(stderr, "Failed to generate quiz\n");
        return 1;
    }

    printf("\n=== %s ===\n\n", quiz->title);

    // Start session
    QuizSession* session = quiz_session_start(quiz);
    if (!session) {
        quiz_free(quiz);
        return 1;
    }

    // Interactive quiz would happen here
    // For now, just display questions

    for (int i = 0; i < quiz->question_count; i++) {
        QuizQuestion* q = &quiz->questions[i];
        printf("%d. %s\n", i + 1, q->question_text);
        for (int j = 0; j < q->option_count; j++) {
            printf("   %c) %s\n", 'A' + j, q->options[j].text);
        }
        printf("\n");
    }

    quiz_session_free(session);
    quiz_free(quiz);

    return 0;
}
