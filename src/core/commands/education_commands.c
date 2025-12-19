/**
 * CONVERGIO KERNEL - Education Pack Commands
 *
 * CLI commands for the Education Pack: /education, /study, /homework, etc.
 *
 * Copyright (c) 2025 Convergio.io
 */

#include "nous/commands.h"
#include "nous/education.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// EXTERNAL FUNCTIONS FROM EDUCATION MODULE
// ============================================================================

// From setup_wizard.c
extern bool education_setup_wizard(void);
extern bool education_quick_setup(const char* name, const char* curriculum, int grade);

// From features/study_session.c (stub declarations)
bool study_session_start(const char* subject, int duration_minutes);
bool study_session_pause(void);
bool study_session_resume(void);
bool study_session_end(void);

// From features/homework.c (stub declarations)
bool homework_start(const char* description);
bool homework_add_hint(void);

// From tools/quiz.c (stub declarations)
char* quiz_generate(const char* topic, int num_questions);

// From tools/flashcards.c (stub declarations)
bool flashcards_create_deck(const char* name, const char* topic);
bool flashcards_study(const char* deck_name);

// From tools/mindmap.c (stub declarations)
char* mindmap_generate_mermaid(const char* topic, int max_depth);

// ============================================================================
// STUB IMPLEMENTATIONS (until feature modules are linked)
// ============================================================================

__attribute__((weak))
bool study_session_start(const char* subject, int duration_minutes) {
    (void)subject; (void)duration_minutes;
    printf("[Study session would start here - feature module not linked]\n");
    return true;
}

__attribute__((weak))
bool study_session_pause(void) { return true; }

__attribute__((weak))
bool study_session_resume(void) { return true; }

__attribute__((weak))
bool study_session_end(void) { return true; }

__attribute__((weak))
bool homework_start(const char* description) {
    (void)description;
    printf("[Homework helper would start here - feature module not linked]\n");
    return true;
}

__attribute__((weak))
bool homework_add_hint(void) { return true; }

__attribute__((weak))
char* quiz_generate(const char* topic, int num_questions) {
    (void)topic; (void)num_questions;
    return strdup("[Quiz generator not yet linked]");
}

__attribute__((weak))
bool flashcards_create_deck(const char* name, const char* topic) {
    (void)name; (void)topic;
    printf("[Flashcard deck would be created here - feature module not linked]\n");
    return true;
}

__attribute__((weak))
bool flashcards_study(const char* deck_name) {
    (void)deck_name;
    printf("[Flashcard study would start here - feature module not linked]\n");
    return true;
}

__attribute__((weak))
char* mindmap_generate_mermaid(const char* topic, int max_depth) {
    (void)topic; (void)max_depth;
    return strdup("mindmap\n  root((Topic))\n    Branch1\n    Branch2\n");
}

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
        // Run full setup wizard
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
            fprintf(stderr, "  elementari, scuola_media, liceo_scientifico, liceo_classico,\n");
            fprintf(stderr, "  liceo_linguistico, liceo_artistico, iti_informatica\n");
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
 * Usage: /study <subject> [duration]
 * Example: /study matematica 30
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
        printf("\n📖 Study Session\n\n");
        printf("Usage: /study <subject> [duration_minutes]\n");
        printf("Example: /study matematica 25\n\n");
        printf("Available subjects based on your curriculum:\n");
        // TODO: List subjects from curriculum
        printf("  matematica, fisica, italiano, storia, inglese...\n\n");
        return 0;
    }

    const char* subject = argv[1];
    int duration = 25; // Default Pomodoro duration

    if (argc >= 3) {
        duration = atoi(argv[2]);
        if (duration < 5) duration = 5;
        if (duration > 120) duration = 120;
    }

    printf("\n📖 Starting study session...\n");
    printf("   Subject: %s\n", subject);
    printf("   Duration: %d minutes\n", duration);
    printf("   Student: %s\n\n", profile->name);

    if (study_session_start(subject, duration)) {
        printf("✓ Session started! The appropriate maestro will guide you.\n\n");
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
        printf("Usage: /homework <describe your homework>\n");
        printf("Example: /homework risolvere l'equazione 3x + 5 = 14\n\n");
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
    printf("   Mode: Anti-cheating (guided learning)\n\n");

    if (homework_start(description)) {
        printf("The appropriate maestro will guide you step by step.\n\n");
        return 0;
    } else {
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
 * Usage: /quiz <topic> [num_questions]
 */
int cmd_quiz(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("\n🧠 Quiz Generator\n\n");
        printf("Usage: /quiz <topic> [num_questions]\n");
        printf("Example: /quiz \"equazioni di primo grado\" 5\n\n");
        return 0;
    }

    const char* topic = argv[1];
    int num_questions = 5; // Default

    if (argc >= 3) {
        num_questions = atoi(argv[2]);
        if (num_questions < 1) num_questions = 1;
        if (num_questions > 20) num_questions = 20;
    }

    printf("\n🧠 Generating %d questions on: %s\n\n", num_questions, topic);

    char* quiz = quiz_generate(topic, num_questions);
    if (quiz != NULL) {
        printf("%s\n", quiz);
        free(quiz);
        return 0;
    } else {
        fprintf(stderr, "Failed to generate quiz.\n");
        return 1;
    }
}

// ============================================================================
// COMMAND: /flashcards
// ============================================================================

/**
 * /flashcards - Create and review flashcards
 *
 * Subcommands:
 *   create <name> <topic> - Create a new deck
 *   study <name>          - Study a deck
 *   list                  - List all decks
 */
int cmd_flashcards(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("\n📚 Flashcards (Spaced Repetition)\n\n");
        printf("Usage:\n");
        printf("  /flashcards create <name> <topic>  - Create new deck\n");
        printf("  /flashcards study <name>           - Study a deck\n");
        printf("  /flashcards list                   - List all decks\n\n");
        printf("Example: /flashcards create verbi_latini \"coniugazioni latine\"\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "create") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: /flashcards create <name> <topic>\n");
            return 1;
        }
        const char* name = argv[2];
        const char* topic = argv[3];

        if (flashcards_create_deck(name, topic)) {
            printf("✓ Deck '%s' created!\n", name);
            return 0;
        } else {
            fprintf(stderr, "Failed to create deck.\n");
            return 1;
        }
    }

    if (strcmp(subcommand, "study") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: /flashcards study <name>\n");
            return 1;
        }
        const char* name = argv[2];

        if (flashcards_study(name)) {
            return 0;
        } else {
            fprintf(stderr, "Failed to start study session.\n");
            return 1;
        }
    }

    if (strcmp(subcommand, "list") == 0) {
        // TODO: List decks from database
        printf("\n📚 Your flashcard decks:\n\n");
        printf("(No decks yet. Create one with /flashcards create)\n\n");
        return 0;
    }

    fprintf(stderr, "Unknown subcommand: %s\n", subcommand);
    return 1;
}

// ============================================================================
// COMMAND: /mindmap
// ============================================================================

/**
 * /mindmap - Generate visual mind maps
 *
 * Usage: /mindmap <concept> [depth]
 */
int cmd_mindmap(int argc, char** argv) {
    if (education_init() != 0) {
        fprintf(stderr, "Error: Education system not initialized\n");
        return 1;
    }

    if (argc < 2) {
        printf("\n🗺️ Mind Map Generator\n\n");
        printf("Usage: /mindmap <concept> [depth]\n");
        printf("Example: /mindmap \"Rivoluzione Francese\" 3\n\n");
        return 0;
    }

    const char* topic = argv[1];
    int depth = 3; // Default

    if (argc >= 3) {
        depth = atoi(argv[2]);
        if (depth < 1) depth = 1;
        if (depth > 5) depth = 5;
    }

    printf("\n🗺️ Generating mind map for: %s (depth: %d)\n\n", topic, depth);

    char* mermaid = mindmap_generate_mermaid(topic, depth);
    if (mermaid != NULL) {
        printf("```mermaid\n%s\n```\n\n", mermaid);
        free(mermaid);
        printf("Copy this Mermaid code to visualize the mind map.\n\n");
        return 0;
    } else {
        fprintf(stderr, "Failed to generate mind map.\n");
        return 1;
    }
}

// ============================================================================
// COMMAND: /libretto
// ============================================================================

/**
 * /libretto - Student gradebook and activity log
 *
 * Subcommands:
 *   (none)      - Show dashboard summary
 *   voti        - Show grade history
 *   diario      - Show daily activity log
 *   progressi   - Show progress graphs
 *   media       - Show grade averages
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
        printf("║               📚 LIBRETTO DELLO STUDENTE                     ║\n");
        printf("╠═══════════════════════════════════════════════════════════════╣\n");
        printf("║  👤 Studente: %-46s ║\n", profile->name);
        printf("╠═══════════════════════════════════════════════════════════════╣\n");

        if (report) {
            printf("║  📊 Ultimi 30 giorni:                                         ║\n");
            printf("║     • Media voti: %.1f/10                                     ║\n", report->overall_average > 0 ? report->overall_average : 0.0f);
            printf("║     • Ore di studio: %d                                        ║\n", report->total_study_hours);
            printf("║     • Quiz completati: %d                                      ║\n", report->quizzes_taken);
            printf("║     • Obiettivi raggiunti: %d                                  ║\n", report->goals_achieved);
            printf("║     • Streak attuale: %d giorni                                ║\n", report->current_streak);
        } else {
            printf("║  📊 Nessun dato disponibile ancora                            ║\n");
        }

        printf("╠═══════════════════════════════════════════════════════════════╣\n");
        printf("║  Comandi:                                                     ║\n");
        printf("║    /libretto voti     - Storico voti per materia              ║\n");
        printf("║    /libretto diario   - Log attivita giornaliere              ║\n");
        printf("║    /libretto progressi - Grafici aree migliorate              ║\n");
        printf("║    /libretto media    - Medie per materia                     ║\n");
        printf("╚═══════════════════════════════════════════════════════════════╝\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    const char* subcommand = argv[1];

    // -------------------------------------------------------------------------
    // /libretto voti [materia]
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "voti") == 0) {
        const char* subject_filter = (argc >= 3) ? argv[2] : NULL;

        int count = 0;
        EducationGrade** grades = libretto_get_grades(profile->id, subject_filter, 0, 0, &count);

        printf("\n📝 STORICO VOTI");
        if (subject_filter) printf(" - %s", subject_filter);
        printf("\n");
        printf("─────────────────────────────────────────────────────────────────\n");

        if (grades && count > 0) {
            printf("%-12s %-20s %-8s %-10s %s\n", "Data", "Materia", "Tipo", "Voto", "Commento");
            printf("─────────────────────────────────────────────────────────────────\n");

            for (int i = 0; i < count && i < 20; i++) {  // Limit to 20 most recent
                EducationGrade* g = grades[i];

                // Format date
                char date_str[12];
                struct tm* tm_info = localtime(&g->recorded_at);
                strftime(date_str, sizeof(date_str), "%d/%m/%Y", tm_info);

                // Grade type abbreviation
                const char* type_str = "?";
                switch (g->grade_type) {
                    case GRADE_TYPE_QUIZ: type_str = "Quiz"; break;
                    case GRADE_TYPE_HOMEWORK: type_str = "Compito"; break;
                    case GRADE_TYPE_ORAL: type_str = "Orale"; break;
                    case GRADE_TYPE_PROJECT: type_str = "Progetto"; break;
                    case GRADE_TYPE_PARTICIPATION: type_str = "Partecip."; break;
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
                printf("\n... e altri %d voti\n", count - 20);
            }

            libretto_grades_free(grades, count);
        } else {
            printf("Nessun voto registrato ancora.\n");
        }

        printf("─────────────────────────────────────────────────────────────────\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    // -------------------------------------------------------------------------
    // /libretto diario [giorni]
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "diario") == 0) {
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

        printf("\n📖 DIARIO ATTIVITA - Ultimi %d giorni\n", days);
        printf("─────────────────────────────────────────────────────────────────\n");

        if (logs && count > 0) {
            printf("%-12s %-12s %-15s %-8s %s\n", "Data", "Attivita", "Materia", "Durata", "Note");
            printf("─────────────────────────────────────────────────────────────────\n");

            for (int i = 0; i < count && i < 30; i++) {
                EducationDailyLogEntry* e = logs[i];

                char date_str[12];
                struct tm* tm_info = localtime(&e->started_at);
                strftime(date_str, sizeof(date_str), "%d/%m/%Y", tm_info);

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
            printf("Nessuna attivita registrata in questo periodo.\n");
        }

        printf("─────────────────────────────────────────────────────────────────\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    // -------------------------------------------------------------------------
    // /libretto progressi
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "progressi") == 0) {
        printf("\n📈 PROGRESSI E TREND\n");
        printf("─────────────────────────────────────────────────────────────────\n");

        if (report && report->subject_count > 0) {
            printf("%-20s %-10s %-10s %s\n", "Materia", "Media", "N.Voti", "Grafico");
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
            printf("Non ci sono ancora abbastanza dati per i progressi.\n");
            printf("Continua a studiare e fare quiz per vedere i tuoi trend!\n");
        }

        printf("─────────────────────────────────────────────────────────────────\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    // -------------------------------------------------------------------------
    // /libretto media [materia]
    // -------------------------------------------------------------------------
    if (strcmp(subcommand, "media") == 0) {
        const char* subject_filter = (argc >= 3) ? argv[2] : NULL;

        printf("\n📊 MEDIE VOTI\n");
        printf("─────────────────────────────────────────────────────────────────\n");

        if (subject_filter) {
            float avg = libretto_get_average(profile->id, subject_filter, 0, 0);
            if (avg >= 0) {
                printf("Media in %s: %.2f/10\n", subject_filter, avg);
            } else {
                printf("Nessun voto in %s\n", subject_filter);
            }
        } else {
            // Show all subject averages
            if (report && report->subject_count > 0) {
                printf("%-25s %s\n", "Materia", "Media");
                printf("─────────────────────────────────────────────────────────────────\n");

                for (int i = 0; i < report->subject_count; i++) {
                    printf("%-25s %.2f/10\n",
                           report->subjects[i].subject,
                           report->subjects[i].average_grade);
                }

                printf("─────────────────────────────────────────────────────────────────\n");
                printf("%-25s %.2f/10\n", "MEDIA GENERALE", report->overall_average);
            } else {
                printf("Nessun voto registrato ancora.\n");
            }
        }

        printf("─────────────────────────────────────────────────────────────────\n\n");

        if (report) libretto_report_free(report);
        return 0;
    }

    fprintf(stderr, "Sottocomando sconosciuto: %s\n", subcommand);
    fprintf(stderr, "Uso: /libretto [voti|diario|progressi|media]\n");

    if (report) libretto_report_free(report);
    return 1;
}
