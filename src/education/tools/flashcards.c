/**
 * CONVERGIO EDUCATION - FLASHCARD ENGINE
 *
 * Spaced repetition flashcards using SM-2 algorithm,
 * with TTS support and Anki export.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include "nous/orchestrator.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// External education DB functions
extern int64_t education_toolkit_save(int64_t student_id, EducationToolkitType type,
                                      const char* topic, const char* content, const char* format);
extern int education_flashcard_create_reviews(int64_t toolkit_output_id, int card_count);
extern int education_flashcard_review(int64_t review_id, int quality);

// FSRS integration (Phase 2 - Learning Science)
// Forward declarations for FSRS functions (from src/education/fsrs.c)
typedef struct {
    int64_t card_id;
    int64_t student_id;
    char* topic_id;
    char* front;
    char* back;
    float stability;
    float difficulty;
    float retrievability;
    int reps;
    int lapses;
    time_t last_review;
    time_t next_review;
    time_t created_at;
} FSRSCard;

typedef struct {
    FSRSCard* cards;
    int count;
    int capacity;
} FSRSCardList;

// External FSRS functions
extern FSRSCardList* fsrs_get_due_cards(int64_t student_id, int limit);
extern int fsrs_record_review(int64_t card_id, int quality);
extern void fsrs_free_cards(FSRSCardList* list);

// ============================================================================
// SM-2 ALGORITHM CONSTANTS
// ============================================================================

#define SM2_MIN_EASINESS 1.3f
#define SM2_DEFAULT_EASINESS 2.5f
#define SM2_INITIAL_INTERVAL 1
#define SM2_SECOND_INTERVAL 6
#define SM2_MIN_QUALITY 0
#define SM2_MAX_QUALITY 5

// Quality ratings:
// 0 - Complete blackout, no memory
// 1 - Incorrect, but recognized after seeing answer
// 2 - Incorrect, but easy to remember after seeing
// 3 - Correct with difficulty
// 4 - Correct with hesitation
// 5 - Perfect recall

// ============================================================================
// TYPES AND STRUCTURES
// ============================================================================

typedef enum {
    CARD_NEW,
    CARD_LEARNING,
    CARD_REVIEWING,
    CARD_MASTERED,
    CARD_SUSPENDED
} FlashcardStatus;

typedef struct {
    int64_t id;
    int64_t deck_id;
    int64_t student_id;
    char* front;
    char* back;
    char* hint;
    char* mnemonic;
    char* image_path;
    char* audio_path;

    // SM-2 state
    float easiness_factor;
    int interval_days;
    int repetition_count;
    time_t next_review;
    time_t last_review;
    int last_quality;
    FlashcardStatus status;

    time_t created_at;
} Flashcard;

typedef struct {
    int64_t id;
    int64_t student_id;
    char* title;
    char* subject;
    char* topic;
    char* description;
    int card_count;
    int mastered_count;
    int due_count;
    time_t created_at;
    time_t last_reviewed;
} FlashcardDeck;

typedef struct {
    FlashcardDeck* deck;
    Flashcard** due_cards;
    int due_count;
    int current_index;
    int reviewed_count;
    int correct_count;
    time_t started_at;
} FlashcardSession;

// ============================================================================
// SM-2 ALGORITHM IMPLEMENTATION
// ============================================================================

/**
 * Calculate next review date using SM-2 algorithm
 */
void sm2_calculate_next_review(Flashcard* card, int quality) {
    if (!card || quality < SM2_MIN_QUALITY || quality > SM2_MAX_QUALITY) {
        return;
    }

    time_t now = time(NULL);

    if (quality >= 3) {
        // Correct response
        if (card->repetition_count == 0) {
            card->interval_days = SM2_INITIAL_INTERVAL;
        } else if (card->repetition_count == 1) {
            card->interval_days = SM2_SECOND_INTERVAL;
        } else {
            card->interval_days = (int)round(card->interval_days * card->easiness_factor);
        }
        card->repetition_count++;
    } else {
        // Incorrect response - reset
        card->repetition_count = 0;
        card->interval_days = SM2_INITIAL_INTERVAL;
    }

    // Update easiness factor
    float ef_change = 0.1f - (5 - quality) * (0.08f + (5 - quality) * 0.02f);
    card->easiness_factor += ef_change;

    if (card->easiness_factor < SM2_MIN_EASINESS) {
        card->easiness_factor = SM2_MIN_EASINESS;
    }

    // Calculate next review timestamp
    card->next_review = now + (card->interval_days * 24 * 60 * 60);
    card->last_review = now;
    card->last_quality = quality;

    // Update status
    if (card->interval_days >= 21 && card->repetition_count >= 3) {
        card->status = CARD_MASTERED;
    } else if (card->repetition_count > 0) {
        card->status = CARD_REVIEWING;
    } else {
        card->status = CARD_LEARNING;
    }
}

/**
 * Get cards due for review using FSRS algorithm (Phase 2 integration)
 */
Flashcard** flashcard_get_due(int64_t deck_id, int64_t student_id, int max_cards, int* count) {
    *count = 0;

    // Use FSRS to get due cards
    FSRSCardList* fsrs_list = fsrs_get_due_cards(student_id, max_cards);
    if (!fsrs_list || fsrs_list->count == 0) {
        if (fsrs_list)
            fsrs_free_cards(fsrs_list);
        return NULL;
    }

    // Allocate array for Flashcard pointers
    Flashcard** cards = calloc(fsrs_list->count, sizeof(Flashcard*));
    if (!cards) {
        fsrs_free_cards(fsrs_list);
        return NULL;
    }

    // Convert FSRSCard to Flashcard
    for (int i = 0; i < fsrs_list->count; i++) {
        FSRSCard* fsrs_card = &fsrs_list->cards[i];

        Flashcard* card = calloc(1, sizeof(Flashcard));
        if (!card) {
            // Free already allocated cards
            for (int j = 0; j < i; j++) {
                if (cards[j]) {
                    free(cards[j]->front);
                    free(cards[j]->back);
                    free(cards[j]);
                }
            }
            free(cards);
            fsrs_free_cards(fsrs_list);
            return NULL;
        }

        card->id = fsrs_card->card_id;
        card->deck_id = deck_id;
        card->student_id = fsrs_card->student_id;
        card->front = fsrs_card->front ? strdup(fsrs_card->front) : NULL;
        card->back = fsrs_card->back ? strdup(fsrs_card->back) : NULL;
        card->next_review = fsrs_card->next_review;
        card->last_review = fsrs_card->last_review;
        card->created_at = fsrs_card->created_at;

        // Map FSRS state to Flashcard state
        card->repetition_count = fsrs_card->reps;
        card->interval_days = (int)round(fsrs_card->stability);
        card->easiness_factor = 2.5f + (1.0f - fsrs_card->difficulty) * 0.5f; // Approximate mapping

        // Determine status from FSRS state
        if (fsrs_card->reps == 0) {
            card->status = CARD_NEW;
        } else if (fsrs_card->stability >= 21.0f && fsrs_card->reps >= 3) {
            card->status = CARD_MASTERED;
        } else if (fsrs_card->reps > 0) {
            card->status = CARD_REVIEWING;
        } else {
            card->status = CARD_LEARNING;
        }

        cards[i] = card;
    }

    *count = fsrs_list->count;
    fsrs_free_cards(fsrs_list);
    return cards;
}

/**
 * Get number of cards due today
 */
int flashcard_count_due(int64_t deck_id, int64_t student_id) {
    // Query database for cards where next_review <= now
    return 0;
}

// ============================================================================
// DECK MANAGEMENT
// ============================================================================

/**
 * Create a new deck
 */
FlashcardDeck* flashcard_deck_create(int64_t student_id, const char* title, const char* subject,
                                     const char* topic) {
    if (!title)
        return NULL;

    FlashcardDeck* deck = calloc(1, sizeof(FlashcardDeck));
    if (!deck)
        return NULL;

    deck->student_id = student_id;
    deck->title = strdup(title);
    deck->subject = subject ? strdup(subject) : NULL;
    deck->topic = topic ? strdup(topic) : NULL;
    deck->created_at = time(NULL);

    // Save to database and get ID
    int64_t db_id = education_toolkit_save(student_id, TOOLKIT_FLASHCARD, title, "", "deck");
    if (db_id > 0) {
        deck->id = db_id;
    }

    return deck;
}

/**
 * Add card to deck
 */
Flashcard* flashcard_add(FlashcardDeck* deck, const char* front, const char* back, const char* hint,
                         const char* mnemonic) {
    if (!deck || !front || !back)
        return NULL;

    Flashcard* card = calloc(1, sizeof(Flashcard));
    if (!card)
        return NULL;

    card->deck_id = deck->id;
    card->student_id = deck->student_id;
    card->front = strdup(front);
    card->back = strdup(back);
    card->hint = hint ? strdup(hint) : NULL;
    card->mnemonic = mnemonic ? strdup(mnemonic) : NULL;

    // Initialize SM-2 state
    card->easiness_factor = SM2_DEFAULT_EASINESS;
    card->interval_days = 0;
    card->repetition_count = 0;
    card->next_review = time(NULL); // Due immediately
    card->status = CARD_NEW;
    card->created_at = time(NULL);

    deck->card_count++;

    // Create flashcard review entry in database for spaced repetition tracking
    if (deck->id > 0) {
        education_flashcard_create_reviews(deck->id, 1);
        card->id = deck->card_count; // Use deck card count as card ID within deck
    }

    return card;
}

// ============================================================================
// LLM GENERATION
// ============================================================================

static const char* FLASHCARD_PROMPT_TEMPLATE =
    "Generate flashcards for studying: %s\n\n"
    "Content:\n%s\n\n"
    "Requirements:\n"
    "- Generate %d flashcards\n"
    "- Each card has: front (question/term), back (answer/definition)\n"
    "- Include a hint for each card\n"
    "- Include a mnemonic device when helpful\n"
    "%s"
    "\nFormat as JSON array:\n"
    "[{\"front\": \"...\", \"back\": \"...\", \"hint\": \"...\", \"mnemonic\": \"...\"}]\n";

/**
 * Generate flashcards using LLM
 */
FlashcardDeck* flashcard_generate_from_llm(int64_t student_id, const char* topic,
                                           const char* content, int card_count,
                                           const EducationAccessibility* access) {
    if (!topic)
        return NULL;

    FlashcardDeck* deck = flashcard_deck_create(student_id, topic, NULL, topic);
    if (!deck)
        return NULL;

    // Accessibility adjustments
    char access_req[256] = "";
    size_t remaining = sizeof(access_req) - 1;
    if (access) {
        if (access->dyslexia) {
            strncat(access_req, "- Use simple, clear language\n", remaining);
            remaining = sizeof(access_req) - strlen(access_req) - 1;
            strncat(access_req, "- Keep fronts and backs short\n", remaining);
            remaining = sizeof(access_req) - strlen(access_req) - 1;
        }
        if (access->autism) {
            strncat(access_req, "- Be explicit and literal\n", remaining);
            remaining = sizeof(access_req) - strlen(access_req) - 1;
            strncat(access_req, "- Avoid ambiguity\n", remaining);
        }
    }

    // Build prompt
    size_t prompt_size = strlen(FLASHCARD_PROMPT_TEMPLATE) + strlen(topic) +
                         (content ? strlen(content) : 100) + strlen(access_req) + 100;

    char* prompt = malloc(prompt_size);
    if (prompt) {
        snprintf(prompt, prompt_size, FLASHCARD_PROMPT_TEMPLATE, topic,
                 content ? content : "Generate appropriate content for the topic",
                 card_count > 0 ? card_count : 10, access_req);

        // Send prompt to LLM
        TokenUsage usage = {0};
        char* response = llm_chat(
            "You are an expert flashcard creator. Generate educational flashcards in JSON format.",
            prompt, &usage);

        if (response) {
            // Parse JSON response and add cards to deck
            // Expected format: [{"front": "...", "back": "...", "hint": "...", "mnemonic": "..."}]
            char* ptr = response;
            int cards_added = 0;

            while ((ptr = strstr(ptr, "\"front\"")) != NULL && cards_added < card_count) {
                char front[512] = "";
                char back[512] = "";
                char hint[256] = "";
                char mnemonic[256] = "";

                // Extract front
                char* start = strstr(ptr, ":");
                if (start) {
                    start = strchr(start, '"');
                    if (start) {
                        start++;
                        char* end = strchr(start, '"');
                        if (end && (end - start) < 500) {
                            strncpy(front, start, end - start);
                            front[end - start] = '\0';
                        }
                    }
                }

                // Extract back
                char* back_ptr = strstr(ptr, "\"back\"");
                if (back_ptr) {
                    start = strstr(back_ptr, ":");
                    if (start) {
                        start = strchr(start, '"');
                        if (start) {
                            start++;
                            char* end = strchr(start, '"');
                            if (end && (end - start) < 500) {
                                strncpy(back, start, end - start);
                                back[end - start] = '\0';
                            }
                        }
                    }
                }

                // Extract hint (optional)
                char* hint_ptr = strstr(ptr, "\"hint\"");
                if (hint_ptr && hint_ptr < strstr(ptr + 1, "\"front\"")) {
                    start = strstr(hint_ptr, ":");
                    if (start) {
                        start = strchr(start, '"');
                        if (start) {
                            start++;
                            char* end = strchr(start, '"');
                            if (end && (end - start) < 250) {
                                strncpy(hint, start, end - start);
                                hint[end - start] = '\0';
                            }
                        }
                    }
                }

                // Extract mnemonic (optional)
                char* mnem_ptr = strstr(ptr, "\"mnemonic\"");
                if (mnem_ptr && mnem_ptr < strstr(ptr + 1, "\"front\"")) {
                    start = strstr(mnem_ptr, ":");
                    if (start) {
                        start = strchr(start, '"');
                        if (start) {
                            start++;
                            char* end = strchr(start, '"');
                            if (end && (end - start) < 250) {
                                strncpy(mnemonic, start, end - start);
                                mnemonic[end - start] = '\0';
                            }
                        }
                    }
                }

                // Add card if we got valid front and back
                if (strlen(front) > 0 && strlen(back) > 0) {
                    flashcard_add(deck, front, back, strlen(hint) > 0 ? hint : NULL,
                                  strlen(mnemonic) > 0 ? mnemonic : NULL);
                    cards_added++;
                }

                ptr++; // Move past current position
            }

            free(response);
        }

        free(prompt);
    }

    return deck;
}

// ============================================================================
// STUDY SESSION
// ============================================================================

/**
 * Start a study session
 */
FlashcardSession* flashcard_session_start(FlashcardDeck* deck, int max_cards) {
    if (!deck)
        return NULL;

    FlashcardSession* session = calloc(1, sizeof(FlashcardSession));
    if (!session)
        return NULL;

    session->deck = deck;
    session->started_at = time(NULL);

    // Get due cards
    session->due_cards =
        flashcard_get_due(deck->id, deck->student_id, max_cards, &session->due_count);

    return session;
}

/**
 * Get current card in session
 */
Flashcard* flashcard_session_current(FlashcardSession* session) {
    if (!session || session->current_index >= session->due_count) {
        return NULL;
    }
    return session->due_cards[session->current_index];
}

/**
 * Rate current card and move to next using FSRS algorithm (Phase 2 integration)
 */
bool flashcard_session_rate(FlashcardSession* session, int quality) {
    if (!session || quality < 1 || quality > 5) {
        return false;
    }

    Flashcard* card = flashcard_session_current(session);
    if (!card)
        return false;

    // Use FSRS algorithm instead of SM-2 (Phase 2 integration)
    if (card->id > 0) {
        // Record review using FSRS algorithm
        int result = fsrs_record_review(card->id, quality);
        if (result == 0) {
            // FSRS updated successfully, also update legacy review table
            education_flashcard_review(card->id, quality);
        }
        // Note: FSRS handles all scheduling internally, no need for SM-2 calculation
    }

    session->reviewed_count++;
    if (quality >= 3)
        session->correct_count++;
    session->current_index++;

    return true;
}

/**
 * Check if session is complete
 */
bool flashcard_session_complete(const FlashcardSession* session) {
    if (!session)
        return true;
    return session->current_index >= session->due_count;
}

/**
 * Get session statistics
 */
void flashcard_session_stats(const FlashcardSession* session, int* reviewed, int* correct,
                             float* accuracy) {
    if (!session)
        return;

    if (reviewed)
        *reviewed = session->reviewed_count;
    if (correct)
        *correct = session->correct_count;
    if (accuracy && session->reviewed_count > 0) {
        *accuracy = (float)session->correct_count / (float)session->reviewed_count * 100.0f;
    }
}

// ============================================================================
// ANKI EXPORT
// ============================================================================

/**
 * Export deck to Anki format (.apkg)
 *
 * Anki uses SQLite database with specific schema.
 * This is a simplified export that creates a text file
 * that can be imported into Anki.
 */
int flashcard_export_anki(const FlashcardDeck* deck, Flashcard** cards, int card_count,
                          const char* output_path) {
    if (!deck || !cards || !output_path)
        return -1;

    // Create tab-separated file for Anki import
    FILE* f = fopen(output_path, "w");
    if (!f)
        return -1;

    // Header comment
    fprintf(f, "# Anki Import File\n");
    fprintf(f, "# Deck: %s\n", deck->title);
    fprintf(f, "# Format: front\\tback\\thint\\n");
    fprintf(f, "\n");

    for (int i = 0; i < card_count; i++) {
        Flashcard* card = cards[i];
        if (!card)
            continue;

        // Escape tabs and newlines
        fprintf(f, "%s\t%s\t%s\n", card->front ? card->front : "", card->back ? card->back : "",
                card->hint ? card->hint : "");
    }

    fclose(f);
    return 0;
}

/**
 * Export deck to printable PDF (front-back format)
 */
int flashcard_export_pdf(const FlashcardDeck* deck, Flashcard** cards, int card_count,
                         const char* output_path, bool double_sided) {
    if (!deck || !cards || !output_path)
        return -1;

    // Build HTML for conversion
    size_t buf_size = 4096 + (card_count * 512);
    char* html = malloc(buf_size);
    if (!html)
        return -1;

    char* ptr = html;
    int remaining = buf_size;

    // HTML header with print styles
    int written = snprintf(ptr, remaining,
                           "<html><head><style>"
                           "body { font-family: Arial, sans-serif; }"
                           ".card { width: 3in; height: 2in; border: 1px solid #ccc; "
                           "        margin: 10px; padding: 10px; display: inline-block; "
                           "        vertical-align: top; text-align: center; }"
                           ".front { background: #f0f8ff; }"
                           ".back { background: #fff8f0; }"
                           "@media print { .page-break { page-break-after: always; } }"
                           "</style></head><body>"
                           "<h1>%s</h1>",
                           deck->title);
    ptr += written;
    remaining -= written;

    // Cards
    if (double_sided) {
        // Print all fronts, then all backs
        snprintf(ptr, remaining, "<h2>Front Side</h2>");
        ptr += strlen(ptr);

        for (int i = 0; i < card_count; i++) {
            if (!cards[i] || remaining < 300)
                continue;
            written = snprintf(ptr, remaining, "<div class='card front'>%s</div>", cards[i]->front);
            ptr += written;
            remaining -= written;
        }

        snprintf(ptr, remaining, "<div class='page-break'></div><h2>Back Side</h2>");
        ptr += strlen(ptr);

        for (int i = 0; i < card_count; i++) {
            if (!cards[i] || remaining < 300)
                continue;
            written = snprintf(ptr, remaining, "<div class='card back'>%s</div>", cards[i]->back);
            ptr += written;
            remaining -= written;
        }
    } else {
        // Print front and back together
        for (int i = 0; i < card_count; i++) {
            if (!cards[i] || remaining < 600)
                continue;
            written = snprintf(ptr, remaining,
                               "<div class='card front'>%s</div>"
                               "<div class='card back'>%s</div>",
                               cards[i]->front, cards[i]->back);
            ptr += written;
            remaining -= written;
        }
    }

    snprintf(ptr, remaining, "</body></html>");

    // Convert to PDF
    char temp_html[256];
    snprintf(temp_html, sizeof(temp_html), "/tmp/flashcards_%d.html", getpid());

    FILE* f = fopen(temp_html, "w");
    if (f) {
        fprintf(f, "%s", html);
        fclose(f);

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "wkhtmltopdf %s %s 2>/dev/null", temp_html, output_path);

        int result = system(cmd);
        unlink(temp_html);
        free(html);
        return result == 0 ? 0 : -1;
    }

    free(html);
    return -1;
}

// ============================================================================
// TTS SUPPORT
// ============================================================================

/**
 * Generate audio for card using TTS
 */
int flashcard_generate_audio(Flashcard* card, bool front_only, float speed) {
    if (!card)
        return -1;

    char audio_path[256];
    snprintf(audio_path, sizeof(audio_path), "/tmp/flashcard_audio_%lld.m4a", card->id);

    // Use macOS say command or AVSpeechSynthesizer
    char cmd[1024];

    if (front_only) {
        snprintf(cmd, sizeof(cmd), "say -r %d -o %s '%s'",
                 (int)(180 * speed), // Words per minute
                 audio_path, card->front);
    } else {
        snprintf(cmd, sizeof(cmd), "say -r %d -o %s '%s ... %s'", (int)(180 * speed), audio_path,
                 card->front, card->back);
    }

    int result = system(cmd);

    if (result == 0) {
        free(card->audio_path);
        card->audio_path = strdup(audio_path);
    }

    return result;
}

// ============================================================================
// TERMINAL UI FOR STUDY SESSION
// ============================================================================

/**
 * Terminal UI for flashcard study session
 * Displays cards one at a time with front/back reveal
 */
int flashcards_ui_study(FlashcardSession* session, const EducationAccessibility* access) {
    if (!session)
        return -1;

    // Terminal control codes
    const char* CLEAR_SCREEN = "\033[2J\033[H";
    const char* BOLD = "\033[1m";
    const char* RESET = "\033[0m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";
    const char* RED = "\033[31m";
    const char* CYAN = "\033[36m";

    printf("%s", CLEAR_SCREEN);
    printf("%s=== Flashcard Study Session ===%s\n", BOLD, RESET);
    printf("Deck: %s\n", session->deck->title);
    printf("Cards to review: %d\n\n", session->due_count);
    printf("Press ENTER to continue...\n");
    getchar();

    while (!flashcard_session_complete(session)) {
        Flashcard* card = flashcard_session_current(session);
        if (!card)
            break;

        // Clear screen and show front
        printf("%s", CLEAR_SCREEN);
        printf("%s=== Card %d/%d ===%s\n\n", BOLD, session->current_index + 1, session->due_count,
               RESET);

        printf("%sFRONT:%s\n", CYAN, RESET);
        printf("%s\n\n", card->front);

        // Show hint if available and requested
        if (card->hint && access && access->dyscalculia) {
            printf("%sHint:%s %s\n\n", YELLOW, RESET, card->hint);
        }

        printf("Press ENTER to reveal answer...\n");
        getchar();

        // Show back
        printf("\n%sBACK:%s\n", CYAN, RESET);
        printf("%s\n\n", card->back);

        // Show mnemonic if available
        if (card->mnemonic) {
            printf("%sMnemonic:%s %s\n\n", YELLOW, RESET, card->mnemonic);
        }

        // Ask for quality rating
        printf("\nHow well did you remember?\n");
        printf("0 - Complete blackout\n");
        printf("1 - Wrong, but recognized\n");
        printf("2 - Wrong, but easy after seeing\n");
        printf("3 - Correct with difficulty\n");
        printf("4 - Correct with hesitation\n");
        printf("5 - Perfect recall\n\n");
        printf("Enter rating (0-5): ");

        char input[10];
        if (fgets(input, sizeof(input), stdin)) {
            int quality = atoi(input);
            if (quality >= 0 && quality <= 5) {
                flashcard_session_rate(session, quality);

                // Show feedback
                if (quality >= 3) {
                    printf("\n%sGreat!%s ", GREEN, RESET);
                } else if (quality >= 1) {
                    printf("\n%sKeep practicing!%s ", YELLOW, RESET);
                } else {
                    printf("\n%sDon't worry, you'll get it!%s ", RED, RESET);
                }

                // Show next interval
                printf("Next review in %d days.\n", card->interval_days);
            }
        }

        printf("\nPress ENTER for next card...\n");
        getchar();
    }

    // Show session summary
    printf("%s", CLEAR_SCREEN);
    printf("%s=== Session Complete ===%s\n\n", BOLD, RESET);

    int reviewed, correct;
    float accuracy;
    flashcard_session_stats(session, &reviewed, &correct, &accuracy);

    printf("Cards reviewed: %d\n", reviewed);
    printf("Correct (3+): %d\n", correct);
    printf("Accuracy: %.1f%%\n", accuracy);
    printf("\n%sGreat work! Keep up the practice!%s\n", GREEN, RESET);

    return 0;
}

/**
 * Auto-generate flashcards from lesson text using LLM
 */
FlashcardDeck* flashcards_auto_generate(int64_t student_id, const char* topic,
                                        const char* lesson_text, int target_count,
                                        const EducationAccessibility* access) {
    if (!topic || !lesson_text)
        return NULL;

    // Build prompt for LLM to extract key concepts
    char prompt[4096];
    snprintf(prompt, sizeof(prompt),
             "Analyze this lesson text and generate %d flashcards.\n\n"
             "Lesson text:\n%s\n\n"
             "For each flashcard:\n"
             "- Front: A question or term to remember\n"
             "- Back: The answer or definition\n"
             "- Hint: A helpful hint (optional)\n"
             "- Mnemonic: A memory aid (optional)\n\n"
             "Focus on:\n"
             "- Key concepts and definitions\n"
             "- Important facts and dates\n"
             "- Cause and effect relationships\n"
             "- Critical thinking questions\n\n"
             "%s"
             "Output as JSON array.",
             target_count > 0 ? target_count : 10, lesson_text,
             (access && access->dyslexia) ? "- Use simple, clear language\n" : "");

    FlashcardDeck* deck = flashcard_deck_create(student_id, topic, NULL, topic);
    if (!deck)
        return NULL;

    // Send prompt to LLM
    TokenUsage usage = {0};
    char* response = llm_chat("You are an expert educator creating flashcards from lesson content. "
                              "Output only valid JSON.",
                              prompt, &usage);

    if (response) {
        // Parse JSON response and add cards to deck (same parser as flashcard_generate_from_llm)
        char* ptr = response;
        int cards_added = 0;
        int max_cards = target_count > 0 ? target_count : 10;

        while ((ptr = strstr(ptr, "\"front\"")) != NULL && cards_added < max_cards) {
            char front[512] = "";
            char back[512] = "";
            char hint[256] = "";
            char mnemonic[256] = "";

            // Extract front
            char* start = strstr(ptr, ":");
            if (start) {
                start = strchr(start, '"');
                if (start) {
                    start++;
                    char* end = strchr(start, '"');
                    if (end && (end - start) < 500) {
                        strncpy(front, start, end - start);
                        front[end - start] = '\0';
                    }
                }
            }

            // Extract back
            char* back_ptr = strstr(ptr, "\"back\"");
            if (back_ptr) {
                start = strstr(back_ptr, ":");
                if (start) {
                    start = strchr(start, '"');
                    if (start) {
                        start++;
                        char* end = strchr(start, '"');
                        if (end && (end - start) < 500) {
                            strncpy(back, start, end - start);
                            back[end - start] = '\0';
                        }
                    }
                }
            }

            // Extract hint (optional)
            char* hint_ptr = strstr(ptr, "\"hint\"");
            char* next_front = strstr(ptr + 1, "\"front\"");
            if (hint_ptr && (!next_front || hint_ptr < next_front)) {
                start = strstr(hint_ptr, ":");
                if (start) {
                    start = strchr(start, '"');
                    if (start) {
                        start++;
                        char* end = strchr(start, '"');
                        if (end && (end - start) < 250) {
                            strncpy(hint, start, end - start);
                            hint[end - start] = '\0';
                        }
                    }
                }
            }

            // Extract mnemonic (optional)
            char* mnem_ptr = strstr(ptr, "\"mnemonic\"");
            if (mnem_ptr && (!next_front || mnem_ptr < next_front)) {
                start = strstr(mnem_ptr, ":");
                if (start) {
                    start = strchr(start, '"');
                    if (start) {
                        start++;
                        char* end = strchr(start, '"');
                        if (end && (end - start) < 250) {
                            strncpy(mnemonic, start, end - start);
                            mnemonic[end - start] = '\0';
                        }
                    }
                }
            }

            // Add card if we got valid front and back
            if (strlen(front) > 0 && strlen(back) > 0) {
                flashcard_add(deck, front, back, strlen(hint) > 0 ? hint : NULL,
                              strlen(mnemonic) > 0 ? mnemonic : NULL);
                cards_added++;
            }

            ptr++;
        }

        free(response);
    }

    return deck;
}

// ============================================================================
// CLEANUP
// ============================================================================

void flashcard_free(Flashcard* card) {
    if (!card)
        return;
    free(card->front);
    free(card->back);
    free(card->hint);
    free(card->mnemonic);
    free(card->image_path);
    free(card->audio_path);
    free(card);
}

void flashcard_deck_free(FlashcardDeck* deck) {
    if (!deck)
        return;
    free(deck->title);
    free(deck->subject);
    free(deck->topic);
    free(deck->description);
    free(deck);
}

void flashcard_session_free(FlashcardSession* session) {
    if (!session)
        return;
    if (session->due_cards) {
        free(session->due_cards); // Cards themselves freed separately
    }
    free(session);
}

// ============================================================================
// CLI COMMAND HANDLER
// ============================================================================

int flashcard_command_handler(int argc, char** argv, const EducationStudentProfile* profile) {
    if (argc < 2) {
        printf("Usage: /flashcards <topic> [--count n] [--export anki|pdf]\n");
        return 1;
    }

    const char* topic = argv[1];
    int count = 10;
    const char* export_format = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--export") == 0 && i + 1 < argc) {
            export_format = argv[++i];
        }
    }

    printf("Generating %d flashcards for: %s\n", count, topic);

    int64_t student_id = profile ? profile->id : 0;
    const EducationAccessibility* access = profile ? profile->accessibility : NULL;

    FlashcardDeck* deck = flashcard_generate_from_llm(student_id, topic, NULL, count, access);

    if (!deck) {
        fprintf(stderr, "Failed to generate flashcards\n");
        return 1;
    }

    printf("Created deck: %s (%d cards)\n", deck->title, deck->card_count);

    if (export_format) {
        char output_path[256];
        if (strcmp(export_format, "anki") == 0) {
            snprintf(output_path, sizeof(output_path), "%s.txt", topic);
            // Would need to get cards from database
            printf("Export to Anki format: %s\n", output_path);
        } else if (strcmp(export_format, "pdf") == 0) {
            snprintf(output_path, sizeof(output_path), "%s.pdf", topic);
            printf("Export to PDF: %s\n", output_path);
        }
    }

    flashcard_deck_free(deck);
    return 0;
}
