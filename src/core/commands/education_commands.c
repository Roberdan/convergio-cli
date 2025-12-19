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
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(description, " ");
        strncat(description, argv[i], sizeof(description) - strlen(description) - 1);
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
