/**
 * CONVERGIO KERNEL - Education Pack Commands
 *
 * CLI commands for the Education Pack: /education, /study, /homework, etc.
 * Properly linked to feature modules in src/education/
 *
 * Copyright (c) 2025 Convergio.io
 */

#include "nous/commands.h"
#include "nous/education.h"
#include "nous/nous.h"
#include "nous/tools.h"  // For tool_web_search

// Include education features header from src/education/features/
// This provides HomeworkRequest, HomeworkResponse, StudySession, etc.
#include "../../education/features/education_features.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// EXTERNAL FUNCTIONS FROM EDUCATION MODULE
// ============================================================================

// From setup_wizard.c (legacy)
extern bool education_setup_wizard(void);
extern bool education_quick_setup(const char* name, const char* curriculum, int grade);

// From ali_onboarding.c (NEW - EDU-01, EDU-02)
extern bool ali_education_onboarding(void);

// From education_db.c - Libretto export (Phase 3 Task 3.5)
extern char* libretto_export_pdf_report(int64_t student_id, const char* report_type);
extern bool ali_needs_onboarding(void);
extern bool ali_check_api_setup(void);

// From tools/mindmap.c
extern char* mindmap_generate_from_llm(const char* topic, const char* content,
                                        const EducationAccessibility* access);
extern int mindmap_command_handler(int argc, char** argv,
                                    const EducationStudentProfile* profile);

// From tools/quiz.c
extern int quiz_command_handler(int argc, char** argv,
                                 const EducationStudentProfile* profile);

// From tools/flashcards.c
extern int flashcard_command_handler(int argc, char** argv,
                                      const EducationStudentProfile* profile);

// From tools/html_generator.c
extern char* html_save_and_open(const char* html_content, const char* topic);

// ============================================================================
// COMMAND: /education
// ============================================================================

/**
 * /education - Education pack management
 *
 * Subcommands:
 *   setup    - Run the setup wizard for a new student
 *   profile  - Show current student profile
 *   switch   - Switch to a different profile
 *   progress - Show learning progress
 */
int cmd_education(int argc, char** argv) {
    // Initialize education system
    if (education_init() != 0) {
        fprintf(stderr, "Error: Failed to initialize education system\n");
        return 1;
    }

    if (argc < 2) {
        // Show current profile or prompt for setup
        EducationStudentProfile* profile = education_profile_get_active();
        if (profile == NULL) {
            printf("\n🎓 Welcome to Convergio Education Pack!\n\n");
            printf("No student profile found. Let's set one up!\n");
            printf("Run: /education setup\n\n");
            printf("Or quick setup: /education quick <name> <curriculum> <grade>\n");
            printf("Example: /education quick Mario liceo_scientifico 1\n\n");
            return 0;
        }

        // Show current profile summary
        printf("\n🎓 Current Profile: %s\n", profile->name);
        printf("   Curriculum: %s (Year %d)\n", profile->curriculum_id, profile->grade_level);
        printf("\nCommands:\n");
        printf("   /study <subject>  - Start a study session\n");
        printf("   /homework <desc>  - Get help with homework\n");
        printf("   /quiz <topic>     - Take a quiz\n");
        printf("   /flashcards <topic> - Study with flashcards\n");
        printf("   /mindmap <concept>  - Generate a mind map\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "setup") == 0) {
        // Run Ali's conversational onboarding (EDU-01, EDU-02)
        // This separates API setup (for parents) from student onboarding (with Ali)
        if (ali_education_onboarding()) {
            return 0;
        } else {
            fprintf(stderr, "Setup cancelled or failed.\n");
            return 1;
        }
    }

    if (strcmp(subcommand, "setup-legacy") == 0) {
        // Legacy form-based wizard (kept for compatibility)
        if (education_setup_wizard()) {
            printf("\n✓ Setup completed successfully!\n");
            return 0;
        } else {
            fprintf(stderr, "Setup cancelled or failed.\n");
            return 1;
        }
    }

    if (strcmp(subcommand, "quick") == 0) {
        // Quick setup: /education quick <name> <curriculum> <grade>
        if (argc < 5) {
            fprintf(stderr, "Usage: /education quick <name> <curriculum> <grade>\n");
            fprintf(stderr, "Example: /education quick Mario liceo_scientifico 1\n\n");
            fprintf(stderr, "Available curricula:\n");
            fprintf(stderr, "  elementary, middle_school, science_high_school, classical_high_school,\n");
            fprintf(stderr, "  language_high_school, art_high_school, tech_high_school\n");
            return 1;
        }

        const char* name = argv[2];
        const char* curriculum = argv[3];
        int grade = atoi(argv[4]);

        if (education_quick_setup(name, curriculum, grade)) {
            printf("\n✓ Profile created for %s!\n", name);
            printf("  Curriculum: %s (Year %d)\n\n", curriculum, grade);
            return 0;
        } else {
            fprintf(stderr, "Failed to create profile.\n");
            return 1;
        }
    }

    if (strcmp(subcommand, "profile") == 0) {
        EducationStudentProfile* profile = education_profile_get_active();
        if (profile == NULL) {
            printf("No active profile. Run /education setup first.\n");
            return 1;
        }

        printf("\n═══════════════════════════════════════════\n");
        printf("           STUDENT PROFILE\n");
        printf("═══════════════════════════════════════════\n\n");
        printf("👤 Name: %s", profile->name);
        if (profile->age > 0) printf(" (%d years old)", profile->age);
        printf("\n");
        printf("📚 Curriculum: %s\n", profile->curriculum_id ? profile->curriculum_id : "Not set");
        printf("📅 Year: %d\n", profile->grade_level);

        // Accessibility (check if pointer is valid)
        if (profile->accessibility) {
            printf("\n♿ Accessibility:\n");
            if (profile->accessibility->dyslexia) printf("   • Dyslexia support enabled\n");
            if (profile->accessibility->dyscalculia) printf("   • Dyscalculia support enabled\n");
            if (profile->accessibility->adhd) printf("   • ADHD support enabled\n");
            if (profile->accessibility->autism) printf("   • Autism support enabled\n");
            if (profile->accessibility->cerebral_palsy) printf("   • Cerebral palsy support enabled\n");
        }

        // Goals (fetch from API)
        int goals_count = 0;
        EducationGoal** goals = education_goal_list(profile->id, &goals_count);
        if (goals_count > 0 && goals != NULL) {
            printf("\n🎯 Goals:\n");
            for (int i = 0; i < goals_count; i++) {
                printf("   %d. %s\n", i + 1, goals[i]->description);
            }
            education_goal_list_free(goals, goals_count);
        }

        printf("\n═══════════════════════════════════════════\n\n");
        return 0;
    }

    if (strcmp(subcommand, "progress") == 0) {
        // TODO: Implement progress view
        printf("\n📊 Progress tracking coming soon!\n\n");
        return 0;
    }

    fprintf(stderr, "Unknown subcommand: %s\n", subcommand);
    fprintf(stderr, "Usage: /education [setup|quick|profile|progress]\n");
    return 1;
}

// ============================================================================
// COMMAND: /study
// ============================================================================

/**
 * /study - Start a study session
 *
 * Usage: /study <subject> [topic]
 * Example: /study mathematics "linear equations"
 */
int cmd_study(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();
    if (profile == NULL) {
        printf("No student profile found. Run /education setup first.\n");
        return 1;
    }

    if (argc < 2) {
        printf("\n📖 Study Session (Pomodoro Timer)\n\n");
        printf("Usage: /study <subject> [topic]\n");
        printf("Example: /study mathematics \"linear equations\"\n\n");
        printf("Features:\n");
        printf("  • 25-minute focused work sessions\n");
        printf("  • 5-minute breaks (15 min after 4 pomodoros)\n");
        printf("  • Native macOS notifications\n");
        printf("  • End-of-session review quiz\n");
        printf("  • Automatic time tracking\n\n");
        printf("Available subjects based on your curriculum:\n");
        printf("  mathematics, physics, language arts, history, english...\n\n");
        return 0;
    }

    const char* subject = argv[1];
    const char* topic = (argc >= 3) ? argv[2] : subject;

    printf("\n📖 Starting study session...\n");
    printf("   Subject: %s\n", subject);
    printf("   Topic: %s\n", topic);
    printf("   Student: %s\n\n", profile->name);

    // Use the actual study_command_handler from study_session.c
    int64_t session_id = study_command_handler(profile->id, subject, topic);
    if (session_id >= 0) {
        return 0;
    } else {
        fprintf(stderr, "Failed to start study session.\n");
        return 1;
    }
}

// ============================================================================
// COMMAND: /homework
// ============================================================================

/**
 * /homework - Get help with homework (anti-cheating mode)
 *
 * Uses Socratic method - guides understanding without providing answers.
 * Features progressive hints and parental transparency logging.
 *
 * Usage: /homework <description>
 */
int cmd_homework(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();
    if (profile == NULL) {
        printf("No student profile found. Run /education setup first.\n");
        return 1;
    }

    if (argc < 2) {
        printf("\n📝 Homework Helper (Anti-Cheating Mode)\n\n");
        printf("I'll help you UNDERSTAND your homework, not do it for you!\n\n");
        printf("Features:\n");
        printf("  • Socratic method - guiding questions only\n");
        printf("  • 5-level progressive hint system\n");
        printf("  • Understanding verification quiz\n");
        printf("  • Parental transparency log\n\n");
        printf("Usage: /homework <describe your homework>\n");
        printf("Example: /homework Mathematics: solve the equation 3x + 5 = 14\n\n");
        return 0;
    }

    // Concatenate all arguments as the homework description
    char description[1024] = {0};
    size_t remaining = sizeof(description) - 1;
    for (int i = 1; i < argc; i++) {
        if (i > 1 && remaining > 0) {
            strncat(description, " ", remaining);
            remaining = sizeof(description) - strlen(description) - 1;
        }
        if (remaining > 0) {
            strncat(description, argv[i], remaining);
            remaining = sizeof(description) - strlen(description) - 1;
        }
    }

    printf("\n📝 Homework Helper\n");
    printf("   Task: %s\n", description);
    printf("   Mode: Anti-cheating (Socratic guidance)\n\n");

    // Use the actual homework command handler
    HomeworkRequest* request = homework_parse_request(profile->id, description);
    if (!request) {
        fprintf(stderr, "Failed to parse homework request.\n");
        return 1;
    }

    HomeworkResponse* response = homework_command_handler(request);
    if (response) {
        // Display Socratic guidance
        if (response->guidance) {
            printf("🧠 Guidance:\n%s\n\n", response->guidance);
        }

        printf("💡 Need more help? Hints are available (0=subtle to 4=detailed).\n");
        printf("   Use /homework-hint <level> to get a progressive hint.\n\n");

        homework_response_free(response);
        homework_request_free(request);
        return 0;
    } else {
        homework_request_free(request);
        fprintf(stderr, "Failed to start homework session.\n");
        return 1;
    }
}

// ============================================================================
// COMMAND: /quiz
// ============================================================================

/**
 * /quiz - Generate adaptive quizzes
 *
 * Features accessibility support, multiple question types,
 * and automatic grade saving to libretto.
 *
 * Usage: /quiz <topic> [--count n] [--difficulty easy|medium|hard]
 */
int cmd_quiz(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();

    if (argc < 2) {
        printf("\n🧠 Quiz Generator\n\n");
        printf("Features:\n");
        printf("  • Multiple question types (MC, T/F, cloze, sequence)\n");
        printf("  • Adaptive difficulty\n");
        printf("  • Accessibility support\n");
        printf("  • Automatic grade saving to libretto\n\n");
        printf("Usage: /quiz <topic> [--count n] [--difficulty easy|medium|hard]\n");
        printf("Example: /quiz \"linear equations\" --count 5\n\n");
        return 0;
    }

    // Use the actual quiz command handler from quiz.c
    return quiz_command_handler(argc, argv, profile);
}

// ============================================================================
// COMMAND: /flashcards
// ============================================================================

/**
 * /flashcards - Create and review flashcards
 *
 * Uses SM-2 spaced repetition algorithm for optimal learning.
 * Supports TTS, Anki export, and PDF export.
 *
 * Usage: /flashcards <topic> [--count n] [--export anki|pdf]
 */
int cmd_flashcards(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();

    if (argc < 2) {
        printf("\n📚 Flashcards (SM-2 Spaced Repetition)\n\n");
        printf("Features:\n");
        printf("  • SM-2 algorithm for optimal spacing\n");
        printf("  • Text-to-speech support\n");
        printf("  • Terminal UI for study sessions\n");
        printf("  • Export to Anki or PDF\n\n");
        printf("Usage: /flashcards <topic> [--count n] [--export anki|pdf]\n");
        printf("Example: /flashcards \"latin verbs\" --count 20\n\n");
        return 0;
    }

    // Use the actual flashcard command handler from flashcards.c
    return flashcard_command_handler(argc, argv, profile);
}

// ============================================================================
// COMMAND: /mindmap
// ============================================================================

/**
 * /mindmap - Generate visual mind maps
 *
 * Generates Mermaid.js mind maps with export to SVG, PNG, and PDF.
 * Supports accessibility adaptations (simplified, high contrast).
 *
 * Usage: /mindmap <concept> [--format svg|png|pdf] [--output path]
 */
int cmd_mindmap(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();

    if (argc < 2) {
        printf("\n🗺️ Mind Map Generator\n\n");
        printf("Features:\n");
        printf("  • Mermaid.js diagram generation\n");
        printf("  • Export to SVG, PNG, or PDF\n");
        printf("  • Accessibility adaptations\n");
        printf("  • LLM-powered content generation\n\n");
        printf("Usage: /mindmap <concept> [--format svg|png|pdf] [--output path]\n");
        printf("Example: /mindmap \"French Revolution\" --format svg\n\n");
        return 0;
    }

    // Use the actual mindmap command handler from mindmap.c
    return mindmap_command_handler(argc, argv, profile);
}

// ============================================================================
// COMMAND: /libretto
// ============================================================================

/**
 * /libretto - Student gradebook and activity log
 *
 * Subcommands:
 *   (none)      - Show dashboard summary
 *   grades      - Show grade history
 *   diary       - Show daily activity log
 *   progress    - Show progress graphs
 *   average     - Show grade averages
 */
int cmd_libretto(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();
    if (profile == NULL) {
        printf("No student profile found. Run /education setup first.\n");
        return 1;
    }

    // Get progress report for dashboard
    EducationProgressReport* report = libretto_get_progress_report(profile->id, 0, 0);

    if (argc < 2) {
        // Dashboard view
        printf("\n");
        printf("╔═══════════════════════════════════════════════════════════════╗\n");
        printf("║                  📚 STUDENT GRADEBOOK                         ║\n");
        printf("╠═══════════════════════════════════════════════════════════════╣\n");
        printf("║  👤 Student: %-47s ║\n", profile->name);
        printf("╠═══════════════════════════════════════════════════════════════╣\n");

        if (report) {
            printf("║  📊 Last 30 days:                                             ║\n");
            printf("║     • Grade average: %.1f/10                                  ║\n", report->overall_average > 0 ? report->overall_average : 0.0f);
            printf("║     • Study hours: %d                                          ║\n", report->total_study_hours);
            printf("║     • Quizzes completed: %d                                    ║\n", report->quizzes_taken);
            printf("║     • Goals achieved: %d                                       ║\n", report->goals_achieved);
            printf("║     • Current streak: %d days                                  ║\n", report->current_streak);
        } else {
            printf("║  📊 No data available yet                                     ║\n");
        }

        printf("╠═══════════════════════════════════════════════════════════════╣\n");
        printf("║  Commands:                                                    ║\n");
        printf("║    /libretto grades   - Grade history by subject              ║\n");
        printf("║    /libretto diary    - Daily activity log                    ║\n");
        printf("║    /libretto progress - Improvement graphs                    ║\n");
        printf("║    /libretto average  - Averages by subject                   ║\n");
        printf("║    /libretto export  - Export PDF report for parents         ║\n");
        printf("╚═══════════════════════════════════════════════════════════════╝\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    const char* subcommand = argv[1];

    // -------------------------------------------------------------------------
    // /libretto grades [subject]
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "grades") == 0 || strcmp(subcommand, "voti") == 0) {
        const char* subject_filter = (argc >= 3) ? argv[2] : NULL;

        int count = 0;
        EducationGrade** grades = libretto_get_grades(profile->id, subject_filter, 0, 0, &count);

        printf("\n📝 GRADE HISTORY");
        if (subject_filter) printf(" - %s", subject_filter);
        printf("\n");
        printf("─────────────────────────────────────────────────────────────────\n");

        if (grades && count > 0) {
            printf("%-12s %-20s %-8s %-10s %s\n", "Date", "Subject", "Type", "Grade", "Comment");
            printf("─────────────────────────────────────────────────────────────────\n");

            for (int i = 0; i < count && i < 20; i++) {  // Limit to 20 most recent
                EducationGrade* g = grades[i];

                // Format date
                char date_str[12];
                struct tm* tm_info = localtime(&g->recorded_at);
                strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);

                // Grade type abbreviation
                const char* type_str = "?";
                switch (g->grade_type) {
                    case GRADE_TYPE_QUIZ: type_str = "Quiz"; break;
                    case GRADE_TYPE_HOMEWORK: type_str = "Homework"; break;
                    case GRADE_TYPE_ORAL: type_str = "Oral"; break;
                    case GRADE_TYPE_PROJECT: type_str = "Project"; break;
                    case GRADE_TYPE_PARTICIPATION: type_str = "Particip."; break;
                }

                // Truncate comment
                char comment_short[30] = "";
                if (g->comment[0]) {
                    strncpy(comment_short, g->comment, 25);
                    comment_short[25] = '\0';
                    if (strlen(g->comment) > 25) strncat(comment_short, "...", sizeof(comment_short) - strlen(comment_short) - 1);
                }

                printf("%-12s %-20s %-8s %5.1f     %s\n",
                       date_str, g->subject, type_str, g->grade, comment_short);
            }

            if (count > 20) {
                printf("\n... and %d more grades\n", count - 20);
            }

            libretto_grades_free(grades, count);
        } else {
            printf("No grades recorded yet.\n");
        }

        printf("─────────────────────────────────────────────────────────────────\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    // -------------------------------------------------------------------------
    // /libretto diary [days]
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "diary") == 0 || strcmp(subcommand, "diario") == 0) {
        int days = 7;  // Default to last 7 days
        if (argc >= 3) {
            days = atoi(argv[2]);
            if (days < 1) days = 1;
            if (days > 30) days = 30;
        }

        time_t now = time(NULL);
        time_t from = now - (days * 24 * 60 * 60);

        int count = 0;
        EducationDailyLogEntry** logs = libretto_get_daily_log(profile->id, from, now, &count);

        printf("\n📖 ACTIVITY DIARY - Last %d days\n", days);
        printf("─────────────────────────────────────────────────────────────────\n");

        if (logs && count > 0) {
            printf("%-12s %-12s %-15s %-8s %s\n", "Date", "Activity", "Subject", "Duration", "Notes");
            printf("─────────────────────────────────────────────────────────────────\n");

            for (int i = 0; i < count && i < 30; i++) {
                EducationDailyLogEntry* e = logs[i];

                char date_str[12];
                struct tm* tm_info = localtime(&e->started_at);
                strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);

                char duration_str[10];
                snprintf(duration_str, sizeof(duration_str), "%dmin", e->duration_minutes);

                char notes_short[25] = "";
                if (e->notes[0]) {
                    strncpy(notes_short, e->notes, 20);
                    notes_short[20] = '\0';
                    if (strlen(e->notes) > 20) strncat(notes_short, "...", sizeof(notes_short) - strlen(notes_short) - 1);
                }

                printf("%-12s %-12s %-15s %-8s %s\n",
                       date_str, e->activity_type,
                       e->subject[0] ? e->subject : "-",
                       duration_str, notes_short);
            }

            libretto_logs_free(logs, count);
        } else {
            printf("No activity recorded in this period.\n");
        }

        printf("─────────────────────────────────────────────────────────────────\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    // -------------------------------------------------------------------------
    // /libretto progress
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "progress") == 0 || strcmp(subcommand, "progressi") == 0) {
        printf("\n📈 PROGRESS AND TRENDS\n");
        printf("─────────────────────────────────────────────────────────────────\n");

        if (report && report->subject_count > 0) {
            printf("%-20s %-10s %-10s %s\n", "Subject", "Average", "Grades", "Chart");
            printf("─────────────────────────────────────────────────────────────────\n");

            for (int i = 0; i < report->subject_count; i++) {
                EducationSubjectStats* s = &report->subjects[i];

                // Simple ASCII bar chart
                int bar_len = (int)(s->average_grade * 2);
                if (bar_len > 20) bar_len = 20;
                char bar[25] = "";
                for (int j = 0; j < bar_len; j++) bar[j] = '#';
                bar[bar_len] = '\0';

                printf("%-20s %5.1f     %-10d %s\n",
                       s->subject, s->average_grade, s->grade_count, bar);
            }
        } else {
            printf("Not enough data for progress tracking yet.\n");
            printf("Keep studying and taking quizzes to see your trends!\n");
        }

        printf("─────────────────────────────────────────────────────────────────\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    // -------------------------------------------------------------------------
    // /libretto average [subject]
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "average") == 0 || strcmp(subcommand, "media") == 0) {
        const char* subject_filter = (argc >= 3) ? argv[2] : NULL;

        printf("\n📊 GRADE AVERAGES\n");
        printf("─────────────────────────────────────────────────────────────────\n");

        if (subject_filter) {
            float avg = libretto_get_average(profile->id, subject_filter, 0, 0);
            if (avg >= 0) {
                printf("Average in %s: %.2f/10\n", subject_filter, avg);
            } else {
                printf("No grades in %s\n", subject_filter);
            }
        } else {
            // Show all subject averages
            if (report && report->subject_count > 0) {
                printf("%-25s %s\n", "Subject", "Average");
                printf("─────────────────────────────────────────────────────────────────\n");

                for (int i = 0; i < report->subject_count; i++) {
                    printf("%-25s %.2f/10\n",
                           report->subjects[i].subject,
                           report->subjects[i].average_grade);
                }

                printf("─────────────────────────────────────────────────────────────────\n");
                printf("%-25s %.2f/10\n", "OVERALL AVERAGE", report->overall_average);
            } else {
                printf("No grades recorded yet.\n");
            }
        }

        printf("─────────────────────────────────────────────────────────────────\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    // -------------------------------------------------------------------------
    // /libretto export [type] (Phase 3 Task 3.5 - PDF export)
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "export") == 0 || strcmp(subcommand, "esporta") == 0) {
        const char* report_type = (argc >= 3) ? argv[2] : "complete";
        
        printf("\n📄 Exporting Report...\n");
        printf("─────────────────────────────────────────────────────────────────\n");
        
        // Generate PDF report (returns HTML path, can be converted to PDF)
        char* html_path = libretto_export_pdf_report(profile->id, report_type);
        
        if (html_path) {
            printf("✅ Report generated successfully!\n\n");
            printf("File: %s\n", html_path);
            printf("\n");
            printf("To convert to PDF:\n");
            printf("  • Open in browser and print to PDF\n");
            printf("  • Or use: wkhtmltopdf %s report.pdf\n\n", html_path);
            free(html_path);
        } else {
            printf("❌ Failed to generate report.\n");
            printf("Make sure you have grades and activity data.\n\n");
        }
        
        if (report) libretto_report_free(report);
        return 0;
    }

    fprintf(stderr, "Unknown subcommand: %s\n", subcommand);
    fprintf(stderr, "Usage: /libretto [grades|diary|progress|average|export]\n");

    if (report) libretto_report_free(report);
    return 1;
}

// ============================================================================
// COMMAND: /html
// ============================================================================

/**
 * /html - Save and open LLM-generated HTML interactive visualizations
 *
 * This command is used by maestri to create custom interactive
 * HTML pages to support lessons. The HTML content is generated
 * by the LLM and saved/opened with this wrapper.
 *
 * Usage: /html <topic>
 *
 * Note: In practice, the maestri generate the HTML content via LLM
 * and call html_save_and_open() directly from their agent code.
 * This command is for manual testing and listing saved lessons.
 */
int cmd_html(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("\n🌐 HTML Interactive Visualizations\n\n");
        printf("This feature allows maestri to create custom interactive\n");
        printf("HTML pages (visualizations, simulations, diagrams) to\n");
        printf("support their lessons.\n\n");
        printf("How it works:\n");
        printf("  1. The maestro generates HTML via LLM prompt\n");
        printf("  2. HTML is saved to ~/Documents/ConvergioEducation/\n");
        printf("  3. Browser opens automatically with the visualization\n\n");
        printf("Usage: /html list                - List saved lessons\n");
        printf("       /html open <filename>     - Open a saved lesson\n");
        printf("       /html test <topic>        - Test with sample HTML\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "list") == 0) {
        // List saved HTML lessons
        const char* home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "Error: HOME not set\n");
            return 1;
        }

        char lessons_dir[512];
        snprintf(lessons_dir, sizeof(lessons_dir),
                 "%s/Documents/ConvergioEducation", home);

        printf("\n📂 Saved lessons in %s:\n\n", lessons_dir);

        // Use system command to list files
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "ls -la \"%s\" 2>/dev/null || echo '  (No lessons yet)'", lessons_dir);
        system(cmd);
        printf("\n");
        return 0;
    }

    if (strcmp(subcommand, "open") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: /html open <filename>\n");
            return 1;
        }

        const char* home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "Error: HOME not set\n");
            return 1;
        }

        char filepath[512];
        snprintf(filepath, sizeof(filepath),
                 "%s/Documents/ConvergioEducation/%s", home, argv[2]);

        printf("Opening: %s\n", filepath);
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "open \"%s\"", filepath);
        return system(cmd) == 0 ? 0 : 1;
    }

    if (strcmp(subcommand, "test") == 0) {
        const char* topic = (argc >= 3) ? argv[2] : "Convergio Test";

        // Generate sample HTML to test the system
        const char* test_html =
            "<!DOCTYPE html>\n"
            "<html lang=\"it\">\n"
            "<head>\n"
            "  <meta charset=\"UTF-8\">\n"
            "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
            "  <title>Test - Convergio Education</title>\n"
            "  <style>\n"
            "    body { font-family: -apple-system, sans-serif;\n"
            "           background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"
            "           min-height: 100vh; display: flex; align-items: center;\n"
            "           justify-content: center; margin: 0; }\n"
            "    .card { background: white; padding: 3rem; border-radius: 1rem;\n"
            "            box-shadow: 0 20px 40px rgba(0,0,0,0.2); text-align: center; }\n"
            "    h1 { color: #667eea; margin-bottom: 1rem; }\n"
            "    p { color: #666; }\n"
            "    .check { font-size: 4rem; color: #22c55e; }\n"
            "  </style>\n"
            "</head>\n"
            "<body>\n"
            "  <div class=\"card\">\n"
            "    <div class=\"check\">✓</div>\n"
            "    <h1>HTML Generator Works!</h1>\n"
            "    <p>The maestri can now create interactive visualizations.</p>\n"
            "  </div>\n"
            "</body>\n"
            "</html>\n";

        char* path = html_save_and_open(test_html, topic);
        if (path) {
            printf("✓ Test HTML saved and opened: %s\n", path);
            free(path);
            return 0;
        } else {
            fprintf(stderr, "Failed to save test HTML.\n");
            return 1;
        }
    }

    fprintf(stderr, "Unknown subcommand: %s\n", subcommand);
    fprintf(stderr, "Usage: /html [list|open|test]\n");
    return 1;
}

// ============================================================================
// COMMAND: /onboarding
// ============================================================================

/**
 * /onboarding - Shortcut to start Ali's conversational onboarding
 *
 * This is a direct alias for /education setup that provides quick access
 * to the initial student onboarding experience.
 */
int cmd_onboarding(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Initialize education system
    if (education_init() != 0) {
        fprintf(stderr, "Error: Failed to initialize education system\n");
        return 1;
    }

    // Run Ali's conversational onboarding directly
    if (ali_education_onboarding()) {
        return 0;
    } else {
        fprintf(stderr, "Onboarding cancelled or failed.\n");
        return 1;
    }
}

// ============================================================================
// COMMAND: /calc
// ============================================================================

// External declarations for calculator
extern int calc_solve_equation(const char* equation, const void* access);
extern void calc_print_fraction_visual(int numerator, int denominator);

/**
 * /calc - Visual calculator with step-by-step explanations
 *
 * Supports dyscalculia with color-coded numbers and visual fractions.
 */
int cmd_calc(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();

    if (argc < 2) {
        printf("\n🧮 Visual Calculator\n\n");
        printf("Features:\n");
        printf("  • Color-coded place values\n");
        printf("  • Step-by-step explanations\n");
        printf("  • Visual fractions (pizza slices)\n");
        printf("  • Equation solver\n\n");
        printf("Usage:\n");
        printf("  /calc solve \"2x + 3 = 7\"      - Solve equation\n");
        printf("  /calc fraction 3/4             - Visualize fraction\n");
        printf("  /calc add 123 + 456            - Step-by-step addition\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "solve") == 0 && argc >= 3) {
        printf("Solving: %s\n\n", argv[2]);
        return calc_solve_equation(argv[2], profile ? profile->accessibility : NULL);
    }

    if (strcmp(subcommand, "fraction") == 0 && argc >= 3) {
        int num = 0, den = 1;
        if (sscanf(argv[2], "%d/%d", &num, &den) == 2) {
            calc_print_fraction_visual(num, den);
            return 0;
        }
    }

    printf("Usage: /calc [solve|fraction|add|subtract|multiply|divide]\n");
    return 1;
}

// ============================================================================
// COMMAND: /define
// ============================================================================

extern int linguistic_define_handler(int argc, char** argv, const void* profile);

/**
 * /define - Dictionary lookup with accessibility support
 */
int cmd_define(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();

    if (argc < 2) {
        printf("\n📖 Dictionary\n\n");
        printf("Usage: /define <word> [--lang en|it|es|fr|de|la]\n");
        printf("Example: /define serendipity\n");
        printf("         /define amore --lang it\n\n");
        return 0;
    }

    return linguistic_define_handler(argc, argv, profile);
}

// ============================================================================
// COMMAND: /conjugate
// ============================================================================

extern int linguistic_conjugate_handler(int argc, char** argv, const void* profile);

/**
 * /conjugate - Verb conjugation for multiple languages
 */
int cmd_conjugate(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();

    if (argc < 2) {
        printf("\n📝 Verb Conjugator\n\n");
        printf("Usage: /conjugate <verb> [--lang en|it|es|fr|de|la]\n");
        printf("Example: /conjugate amare --lang it\n");
        printf("         /conjugate to be --lang en\n\n");
        return 0;
    }

    return linguistic_conjugate_handler(argc, argv, profile);
}

// ============================================================================
// COMMAND: /pronounce
// ============================================================================

extern int linguistic_pronounce_handler(int argc, char** argv, const void* profile);

/**
 * /pronounce - Word pronunciation with IPA and audio
 */
int cmd_pronounce(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();

    if (argc < 2) {
        printf("\n🔊 Pronunciation Guide\n\n");
        printf("Usage: /pronounce <word> [--lang en|it|es|fr|de]\n");
        printf("Example: /pronounce beautiful\n\n");
        printf("Shows IPA transcription and plays audio (if TTS enabled).\n\n");
        return 0;
    }

    return linguistic_pronounce_handler(argc, argv, profile);
}

// ============================================================================
// COMMAND: /grammar
// ============================================================================

extern int linguistic_grammar_handler(int argc, char** argv, const void* profile);

/**
 * /grammar - Grammatical analysis of sentences
 */
int cmd_grammar(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();

    if (argc < 2) {
        printf("\n📊 Grammar Analyzer\n\n");
        printf("Usage: /grammar \"<sentence>\" [--lang en|it|es|fr|de]\n");
        printf("Example: /grammar \"The quick brown fox jumps.\"\n\n");
        printf("Analyzes: subject, predicate, objects, parts of speech.\n\n");
        return 0;
    }

    return linguistic_grammar_handler(argc, argv, profile);
}

// ============================================================================
// COMMAND: /xp
// ============================================================================

/**
 * /xp - Gamification: XP, levels, badges, streaks
 */
int cmd_xp(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        printf("No student profile. Run /education setup first.\n");
        return 1;
    }

    // Get real engagement stats from database
    EducationEngagementStats* stats = education_engagement_get_stats(profile->id);

    if (argc < 2 || strcmp(argv[1], "status") == 0) {
        printf("\n🎮 Gamification Status for %s\n\n", profile->name);
        printf("┌─────────────────────────────────┐\n");

        if (stats) {
            int xp_in_level = stats->total_xp - ((stats->level - 1) * 100);

            // Level titles based on level
            const char* level_title = "Beginner";
            if (stats->level >= 10) level_title = "Master";
            else if (stats->level >= 7) level_title = "Expert";
            else if (stats->level >= 5) level_title = "Apprentice";
            else if (stats->level >= 3) level_title = "Student";

            printf("│ ⭐ XP:     %-5d / %-5d         │\n", xp_in_level, 100);
            printf("│ 📊 Level:  %-2d (%-12s)   │\n", stats->level, level_title);
            printf("│ 🔥 Streak: %-3d days            │\n", stats->current_streak);
            printf("└─────────────────────────────────┘\n\n");

            printf("📊 Stats:\n");
            printf("   Total XP: %d\n", stats->total_xp);
            printf("   Longest streak: %d days\n", stats->longest_streak);
            if (stats->streak_freezes_available > 0) {
                printf("   Streak freezes: %d available\n", stats->streak_freezes_available);
            }
            if (stats->has_weekend_amulet) {
                printf("   🛡️ Weekend Amulet active!\n");
            }
            printf("\n");

            int xp_to_next = 100 - xp_in_level;
            printf("📈 Next goal: %d XP to Level %d\n\n", xp_to_next, stats->level + 1);

            education_engagement_free(stats);
        } else {
            printf("│ ⭐ XP:     0 / 100              │\n");
            printf("│ 📊 Level:  1 (Beginner)        │\n");
            printf("│ 🔥 Streak: 0 days              │\n");
            printf("└─────────────────────────────────┘\n\n");
            printf("Start studying to earn XP!\n\n");
        }
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "leaderboard") == 0) {
        // Note: Real leaderboard would require multi-user database
        // For now, show student's own ranking
        printf("\n🏅 Leaderboard (this week)\n\n");
        if (stats) {
            printf("  Your stats:\n");
            printf("  📊 XP this week: %d\n", stats->total_xp);
            printf("  🔥 Current streak: %d days\n", stats->current_streak);
            printf("\n");
            printf("  (Leaderboard requires multi-user mode)\n\n");
            education_engagement_free(stats);
        } else {
            printf("  No stats available yet. Start studying!\n\n");
        }
        return 0;
    }

    if (strcmp(subcommand, "badges") == 0) {
        printf("\n🏆 All Badges\n\n");

        if (stats) {
            printf("Earned:\n");
            if (stats->current_streak >= 7) {
                printf("   ✅ 🌟 First week complete\n");
            }
            if (stats->total_xp >= 500) {
                printf("   ✅ 📚 Rising star (500 XP)\n");
            }
            if (stats->longest_streak >= 30) {
                printf("   ✅ 🔥 30-day streak\n");
            }
            if (stats->level >= 5) {
                printf("   ✅ 🎓 Apprentice level\n");
            }

            printf("\nTo unlock:\n");
            if (stats->current_streak < 7) {
                printf("   ⬜ 🌟 First week (7-day streak)\n");
            }
            if (stats->total_xp < 500) {
                printf("   ⬜ 📚 Rising star (500 XP)\n");
            }
            if (stats->total_xp < 1000) {
                printf("   ⬜ 🏆 Scholar (1000 XP)\n");
            }
            if (stats->longest_streak < 30) {
                printf("   ⬜ 🔥 30-day streak\n");
            }
            if (stats->level < 10) {
                printf("   ⬜ 👑 Master level (Level 10)\n");
            }
            printf("\n");
            education_engagement_free(stats);
        } else {
            printf("No badges yet. Start studying to earn badges!\n\n");
        }
        return 0;
    }

    if (stats) education_engagement_free(stats);
    printf("Usage: /xp [status|leaderboard|badges]\n");
    return 1;
}

// ============================================================================
// COMMAND: /video
// ============================================================================

#include "nous/tools.h"  // For tool_web_search

/**
 * /video - Search educational YouTube videos (Phase 3 Task 3.2 - Real implementation)
 * Uses web_search tool to find educational content
 */
int cmd_video(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    printf("\n🎬 Educational Video Search\n\n");

    if (argc < 2) {
        printf("Usage: /video <topic>\n");
        printf("Example: /video \"pythagorean theorem\"\n");
        printf("         /video \"photosynthesis\"\n\n");
        printf("📺 Trusted Educational Channels:\n\n");
        printf("  • Khan Academy - https://www.khanacademy.org\n");
        printf("  • 3Blue1Brown - https://www.3blue1brown.com\n");
        printf("  • CrashCourse - https://www.youtube.com/crashcourse\n");
        printf("  • Kurzgesagt - https://www.youtube.com/kurzgesagt\n");
        printf("  • Veritasium - https://www.youtube.com/veritasium\n\n");
        return 0;
    }

    const char* topic = argv[1];
    
    // Build educational search query
    char query[512];
    snprintf(query, sizeof(query), "educational video %s site:youtube.com OR site:khanacademy.org", topic);
    
    printf("🔍 Searching for educational videos about: %s\n\n", topic);
    
    // Use web_search tool to find educational content
    ToolResult* result = tool_web_search(query, 5);
    
    if (result && result->success && result->output) {
        printf("📹 Found Educational Videos:\n\n");
        printf("%s\n", result->output);
        
        // Free result
        if (result->output) free(result->output);
        if (result->error) free(result->error);
        free(result);
    } else {
        printf("⚠️  Could not search online. Here are trusted educational channels:\n\n");
        printf("  • Khan Academy - Comprehensive courses\n");
        printf("    https://www.khanacademy.org\n\n");
        printf("  • 3Blue1Brown - Visual math explanations\n");
        printf("    https://www.3blue1brown.com\n\n");
        printf("  • CrashCourse - History, science, literature\n");
        printf("    https://www.youtube.com/crashcourse\n\n");
        
        if (result) {
            if (result->error) {
                printf("Error: %s\n\n", result->error);
                free(result->error);
            }
            free(result);
        }
    }

    return 0;
}

// ============================================================================
// COMMAND: /periodic
// ============================================================================

// Forward declarations for periodic table functions (from src/education/periodic_table.c)
typedef struct {
    int atomic_number;
    const char* symbol;
    const char* name;
    const char* name_it;
    double atomic_mass;
    const char* category;
    const char* electron_config;
    double melting_point_c;
    double boiling_point_c;
    double density;
    const char* discovered;
    const char* fun_fact;
} PeriodicElement;

extern const PeriodicElement* periodic_find_element(const char* query);
extern void periodic_print_element(const PeriodicElement* el);

/**
 * /periodic - Interactive periodic table (Phase 3 Task 3.3 - Real database)
 */
int cmd_periodic(int argc, char** argv) {
    if (argc < 2) {
        printf("\n⚗️  Interactive Periodic Table\n\n");
        printf("Usage: /periodic <element>\n");
        printf("Example: /periodic Fe\n");
        printf("         /periodic iron\n");
        printf("         /periodic ferro\n");
        printf("         /periodic H\n");
        printf("         /periodic carbon\n\n");
        printf("Supports: symbol (Fe), English name (iron), Italian name (ferro)\n\n");
        return 0;
    }

    const char* query = argv[1];
    const PeriodicElement* el = periodic_find_element(query);
    
    if (el) {
        periodic_print_element(el);
    } else {
        printf("\n❌ Element not found: %s\n\n", query);
        printf("Try with:\n");
        printf("  • Symbol: Fe, H, O, C, Na, etc.\n");
        printf("  • English name: iron, hydrogen, oxygen, carbon\n");
        printf("  • Italian name: ferro, idrogeno, ossigeno, carbonio\n\n");
    }

    return 0;
}

// ============================================================================
// COMMAND: /convert
// ============================================================================

/**
 * /convert - Unit converter
 */
int cmd_convert(int argc, char** argv) {
    if (argc < 4) {
        printf("\n📐 Unit Converter\n\n");
        printf("Usage: /convert <value> <from> <to>\n");
        printf("Example: /convert 100 cm m\n");
        printf("         /convert 5 km mi\n");
        printf("         /convert 20 C F\n\n");
        printf("Supports: length, mass, temperature, area, volume.\n\n");
        return 0;
    }

    double value = atof(argv[1]);
    const char* from = argv[2];
    const char* to = argv[3];

    double result = 0;
    bool converted = false;

    // Length conversions
    if ((strcmp(from, "km") == 0 && strcmp(to, "m") == 0)) {
        result = value * 1000; converted = true;
    } else if ((strcmp(from, "m") == 0 && strcmp(to, "km") == 0)) {
        result = value / 1000; converted = true;
    } else if ((strcmp(from, "cm") == 0 && strcmp(to, "m") == 0)) {
        result = value / 100; converted = true;
    } else if ((strcmp(from, "m") == 0 && strcmp(to, "cm") == 0)) {
        result = value * 100; converted = true;
    } else if ((strcmp(from, "km") == 0 && strcmp(to, "mi") == 0)) {
        result = value * 0.621371; converted = true;
    } else if ((strcmp(from, "mi") == 0 && strcmp(to, "km") == 0)) {
        result = value * 1.60934; converted = true;
    }
    // Temperature
    else if ((strcmp(from, "C") == 0 && strcmp(to, "F") == 0)) {
        result = value * 9/5 + 32; converted = true;
    } else if ((strcmp(from, "F") == 0 && strcmp(to, "C") == 0)) {
        result = (value - 32) * 5/9; converted = true;
    }
    // Mass
    else if ((strcmp(from, "kg") == 0 && strcmp(to, "g") == 0)) {
        result = value * 1000; converted = true;
    } else if ((strcmp(from, "g") == 0 && strcmp(to, "kg") == 0)) {
        result = value / 1000; converted = true;
    } else if ((strcmp(from, "kg") == 0 && strcmp(to, "lb") == 0)) {
        result = value * 2.20462; converted = true;
    } else if ((strcmp(from, "lb") == 0 && strcmp(to, "kg") == 0)) {
        result = value / 2.20462; converted = true;
    }

    if (converted) {
        printf("\n%.4g %s = %.4g %s\n\n", value, from, result, to);
    } else {
        printf("Conversione non supportata: %s → %s\n", from, to);
    }

    return 0;
}

// ============================================================================
// COMMAND: /voice
// ============================================================================

// External from voice_mode.c
extern int voice_mode_start(const char *maestro_id, const char *topic);

/**
 * /voice [maestro] [topic] - Start conversational voice mode
 *
 * Examples:
 *   /voice                     - Start with default maestro (Euclide)
 *   /voice feynman             - Start with Feynman
 *   /voice euclide geometria   - Start with Euclide on geometry topic
 *
 * In voice mode:
 *   ESC - Exit voice mode
 *   M   - Toggle mute microphone
 *   T   - Toggle transcript display
 *   S   - Save conversation
 */
int cmd_voice(int argc, char** argv) {
    const char *maestro_id = NULL;
    const char *topic = NULL;

    if (argc >= 2) {
        maestro_id = argv[1];
    }
    if (argc >= 3) {
        topic = argv[2];
    }

    printf("\n🎤 Starting voice mode...\n");
    printf("   Speak naturally with your maestro.\n");
    printf("   Press ESC to exit.\n\n");

    return voice_mode_start(maestro_id, topic);
}

// ============================================================================
// COMMAND: /upload
// ============================================================================

/**
 * /upload - Upload a document for study help
 *
 * Opens an interactive file picker restricted to Desktop, Documents, Downloads
 * folders. Only shows supported file types (PDF, DOCX, PPTX, images, etc.)
 *
 * Usage:
 *   /upload           - Open file picker
 *   /upload <path>    - Upload specific file
 */
int cmd_upload(int argc, char** argv) {
    extern int document_command_handler(int argc, char** argv);

    // Pass to document handler
    char* upload_args[] = {"upload", NULL, NULL};
    int upload_argc = 1;

    if (argc >= 2) {
        upload_args[1] = argv[1];
        upload_argc = 2;
    }

    return document_command_handler(upload_argc, upload_args);
}

// ============================================================================
// COMMAND: /doc
// ============================================================================

/**
 * /doc - Manage uploaded documents
 *
 * Lists, selects, and manages uploaded school materials.
 *
 * Usage:
 *   /doc              - List uploaded documents
 *   /doc list         - List all uploaded documents
 *   /doc <n>          - Select document n
 *   /doc clear        - Clear all uploaded documents
 */
int cmd_doc(int argc, char** argv) {
    extern int document_command_handler(int argc, char** argv);

    // Pass to document handler
    char* doc_args[4] = {"doc", NULL, NULL, NULL};
    int doc_argc = 1;

    for (int i = 1; i < argc && i < 3; i++) {
        doc_args[i] = argv[i];
        doc_argc++;
    }

    return document_command_handler(doc_argc, doc_args);
}
