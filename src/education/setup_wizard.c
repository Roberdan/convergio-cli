/**
 * CONVERGIO EDUCATION SETUP WIZARD
 *
 * Interactive wizard for setting up student profiles with curriculum selection,
 * accessibility assessment, and personalized learning preferences.
 *
 * Implements FASE 1 tasks S01-S09 from EducationPackPlan.md
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_INPUT_LENGTH 256
#define MAX_NAME_LENGTH 100
#define MAX_GOALS 5
#define MAX_GOAL_LEN 256

// ANSI Color codes for terminal output
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_DIM     "\033[2m"

// ============================================================================
// WIZARD STATE STRUCT (local to this file)
// ============================================================================

/**
 * Local struct to collect wizard data before converting to API calls
 */
typedef struct {
    // Basic info
    char name[MAX_NAME_LENGTH];
    int age;
    char parent_name[MAX_NAME_LENGTH];
    char parent_email[MAX_NAME_LENGTH];

    // Curriculum
    char curriculum_id[128];
    int grade_level;

    // Accessibility
    EducationAccessibility accessibility;

    // Preferences (stored in accessibility or separate)
    int session_duration;
    int break_duration;
    char learning_style[32];
    char study_method[512];

    // Goals
    char goals[MAX_GOALS][MAX_GOAL_LEN];
    int goals_count;
} WizardState;

// ============================================================================
// AVAILABLE CURRICULA
// ============================================================================

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    int min_grade;
    int max_grade;
} CurriculumInfo;

static const CurriculumInfo AVAILABLE_CURRICULA[] = {
    // Primary/Secondary School (Italian system)
    {"elementari", "Elementary School", "Grades 1-5 (Primary)", 1, 5},
    {"scuola_media", "Middle School", "Grades 6-8 (Lower Secondary)", 6, 8},

    // High Schools (Licei)
    {"liceo_scientifico", "Scientific High School", "5 years - focus on mathematics and sciences", 9, 13},
    {"liceo_classico", "Classical High School", "5 years - Latin, Greek and philosophy", 9, 13},
    {"liceo_linguistico", "Language High School", "5 years - 3 foreign languages", 9, 13},
    {"liceo_artistico", "Art High School", "5 years - visual arts and design", 9, 13},

    // Technical Institutes
    {"iti_informatica", "Technical - Computer Science", "Technical Institute - Computing and Telecommunications", 9, 13},
    {"iti_commerciale", "Technical - Business", "Technical Institute - Business and Economics", 9, 13},
    {NULL, NULL, NULL, 0, 0}
};

// ============================================================================
// ACCESSIBILITY CONDITIONS
// ============================================================================

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    const char* support_info;
} AccessibilityCondition;

static const AccessibilityCondition ACCESSIBILITY_CONDITIONS[] = {
    {"dyslexia", "Dyslexia",
     "Difficulty with reading and word recognition",
     "Support: OpenDyslexic font, TTS, syllable breaking, cream background"},
    {"dyscalculia", "Dyscalculia",
     "Difficulty with numbers and mathematical calculations",
     "Support: Visualizations, step-by-step, no timers, colors"},
    {"adhd", "ADHD",
     "Attention and/or hyperactivity difficulties",
     "Support: Short responses, celebrations, gamification, breaks"},
    {"autism", "Autism",
     "Different social and sensory processing modes",
     "Support: Literal language, predictable structure, details"},
    {"cerebral_palsy", "Cerebral Palsy",
     "Motor difficulties of varying degrees",
     "Support: Voice input, extended timeouts, frequent breaks"},
    {"visual", "Visual Impairment",
     "Reduced visual ability",
     "Support: TTS, high contrast, large fonts"},
    {"hearing", "Hearing Impairment",
     "Reduced hearing ability",
     "Support: Text content, subtitles"},
    {NULL, NULL, NULL, NULL}
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void print_header(const char* title, int step, int total_steps) {
    printf("\n");
    printf(ANSI_CYAN "══════════════════════════════════════════════════════════════\n" ANSI_RESET);
    printf(ANSI_BOLD "   CONVERGIO EDUCATION - Student Setup\n" ANSI_RESET);
    if (step > 0) {
        printf(ANSI_DIM "   Step %d of %d: %s\n" ANSI_RESET, step, total_steps, title);
    } else {
        printf(ANSI_DIM "   %s\n" ANSI_RESET, title);
    }
    printf(ANSI_CYAN "══════════════════════════════════════════════════════════════\n" ANSI_RESET);
    printf("\n");
}

static void print_success(const char* message) {
    printf(ANSI_GREEN "✓ %s\n" ANSI_RESET, message);
}

static void print_info(const char* message) {
    printf(ANSI_BLUE "ℹ %s\n" ANSI_RESET, message);
}

static void print_warning(const char* message) {
    printf(ANSI_YELLOW "⚠ %s\n" ANSI_RESET, message);
}

static void print_option(int num, const char* text, const char* description) {
    printf("  " ANSI_BOLD "%d." ANSI_RESET " %s\n", num, text);
    if (description && strlen(description) > 0) {
        printf("     " ANSI_DIM "%s" ANSI_RESET "\n", description);
    }
}

static int read_int_choice(int min, int max) {
    char buffer[MAX_INPUT_LENGTH];
    int choice;

    while (1) {
        printf("\n  > Scelta [%d-%d]: ", min, max);
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            return -1;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        if (strlen(buffer) == 0) {
            continue;
        }

        if (sscanf(buffer, "%d", &choice) == 1) {
            if (choice >= min && choice <= max) {
                return choice;
            }
        }

        print_warning("Scelta non valida. Riprova.");
    }
}

static void read_string(const char* prompt, char* buffer, size_t max_len) {
    if (prompt && strlen(prompt) > 0) {
        printf("  > %s: ", prompt);
    }
    fflush(stdout);

    if (fgets(buffer, max_len, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\n")] = 0;

    // Trim whitespace
    char* start = buffer;
    while (*start && isspace(*start)) start++;
    if (start != buffer) {
        memmove(buffer, start, strlen(start) + 1);
    }

    char* end = buffer + strlen(buffer) - 1;
    while (end > buffer && isspace(*end)) {
        *end = '\0';
        end--;
    }
}

static bool read_yes_no(const char* prompt, bool default_value) {
    char buffer[MAX_INPUT_LENGTH];

    printf("  > %s [%s]: ", prompt, default_value ? "S/n" : "s/N");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return default_value;
    }

    buffer[strcspn(buffer, "\n")] = 0;

    if (strlen(buffer) == 0) {
        return default_value;
    }

    char first = tolower(buffer[0]);
    if (first == 's' || first == 'y') return true;
    if (first == 'n') return false;

    return default_value;
}

// ============================================================================
// WIZARD STEPS
// ============================================================================

/**
 * S02: Step 1 - Nome e info base studente
 */
static bool wizard_step1_basic_info(WizardState* state) {
    print_header("Basic Information", 1, 6);

    printf("  Welcome to the student profile setup!\n");
    printf("  Let's start with some basic information.\n\n");

    // Student name
    read_string("What is your name?", state->name, sizeof(state->name));
    if (strlen(state->name) == 0) {
        print_warning("Name is required.");
        return false;
    }

    // Age
    printf("\n");
    char age_buf[16];
    read_string("How old are you?", age_buf, sizeof(age_buf));
    state->age = atoi(age_buf);
    if (state->age < 5 || state->age > 99) {
        state->age = 0;
    }

    // Parent contact (optional)
    printf("\n");
    if (read_yes_no("Add a parent/guardian contact?", false)) {
        read_string("Parent/guardian name", state->parent_name, sizeof(state->parent_name));
        read_string("Parent/guardian email", state->parent_email, sizeof(state->parent_email));
    }

    printf("\n");
    print_success("Basic info saved!");
    return true;
}

/**
 * S03: Step 2 - Selezione curriculum
 */
static bool wizard_step2_curriculum(WizardState* state) {
    print_header("Curriculum Selection", 2, 6);

    printf("  What type of school do you attend?\n\n");

    int count = 0;
    for (int i = 0; AVAILABLE_CURRICULA[i].id != NULL; i++) {
        print_option(i + 1, AVAILABLE_CURRICULA[i].name, AVAILABLE_CURRICULA[i].description);
        count++;
    }

    int choice = read_int_choice(1, count);
    if (choice < 1) return false;

    const CurriculumInfo* selected = &AVAILABLE_CURRICULA[choice - 1];
    strncpy(state->curriculum_id, selected->id, sizeof(state->curriculum_id) - 1);

    // Anno specifico
    if (selected->max_grade > selected->min_grade) {
        printf("\n  Che anno stai frequentando? (%d-%d)\n",
               selected->min_grade, selected->max_grade);
        state->grade_level = read_int_choice(selected->min_grade, selected->max_grade);
    } else {
        state->grade_level = selected->min_grade;
    }

    printf("\n");
    print_success("Curriculum selezionato!");
    printf("  %s - Anno %d\n", selected->name, state->grade_level);

    return true;
}

/**
 * S04: Step 3 - Assessment accessibilità
 */
static bool wizard_step3_accessibility(WizardState* state) {
    print_header("Accessibility", 3, 6);

    printf("  Let's talk about your learning needs.\n");
    printf("  Everything you share will ONLY be used to help you better.\n");
    printf("  No judgment, just support.\n\n");

    // Initialize accessibility
    memset(&state->accessibility, 0, sizeof(state->accessibility));

    for (int i = 0; ACCESSIBILITY_CONDITIONS[i].id != NULL; i++) {
        const AccessibilityCondition* cond = &ACCESSIBILITY_CONDITIONS[i];

        printf("  " ANSI_BOLD "%s" ANSI_RESET "\n", cond->name);
        printf("  " ANSI_DIM "%s" ANSI_RESET "\n", cond->description);

        bool has_condition = read_yes_no("Hai questa condizione?", false);

        if (has_condition) {
            print_info(cond->support_info);

            // Set specific flags based on condition
            if (strcmp(cond->id, "dyslexia") == 0) {
                state->accessibility.dyslexia = true;
                printf("  Quanto è severa? (1=lieve, 2=moderata, 3=severa)\n");
                state->accessibility.dyslexia_severity = read_int_choice(1, 3);
            } else if (strcmp(cond->id, "dyscalculia") == 0) {
                state->accessibility.dyscalculia = true;
                state->accessibility.dyscalculia_severity = SEVERITY_MODERATE;
            } else if (strcmp(cond->id, "adhd") == 0) {
                state->accessibility.adhd = true;
                state->accessibility.adhd_type = ADHD_COMBINED;
            } else if (strcmp(cond->id, "autism") == 0) {
                state->accessibility.autism = true;
            } else if (strcmp(cond->id, "cerebral_palsy") == 0) {
                state->accessibility.cerebral_palsy = true;
            } else if (strcmp(cond->id, "visual") == 0) {
                state->accessibility.visual_impairment = true;
            } else if (strcmp(cond->id, "hearing") == 0) {
                state->accessibility.hearing_impairment = true;
            }
        }

        printf("\n");
    }

    print_success("Profilo accessibilità configurato!");
    return true;
}

/**
 * S05: Step 4 - Preferenze input/output
 */
static bool wizard_step4_preferences(WizardState* state) {
    print_header("Preferences", 4, 6);

    printf("  How do you prefer to interact with the maestri?\n\n");

    // Input preference
    printf("  " ANSI_BOLD "Preferred input:" ANSI_RESET "\n");
    print_option(1, "Keyboard", "I type my questions");
    print_option(2, "Voice", "I speak into the microphone");
    print_option(3, "Both", "I use both depending on the situation");

    int input_choice = read_int_choice(1, 3);
    switch (input_choice) {
        case 1: state->accessibility.preferred_input = INPUT_KEYBOARD; break;
        case 2: state->accessibility.preferred_input = INPUT_VOICE; break;
        case 3: state->accessibility.preferred_input = INPUT_BOTH; break;
    }

    // Output preference
    printf("\n  " ANSI_BOLD "Preferred output:" ANSI_RESET "\n");
    print_option(1, "Text only", "I read the responses");
    print_option(2, "Audio only", "I listen to responses (TTS)");
    print_option(3, "Both", "Text + audio together");

    int output_choice = read_int_choice(1, 3);
    switch (output_choice) {
        case 1:
            state->accessibility.preferred_output = OUTPUT_TEXT;
            state->accessibility.tts_enabled = false;
            break;
        case 2:
            state->accessibility.preferred_output = OUTPUT_TTS;
            state->accessibility.tts_enabled = true;
            break;
        case 3:
            state->accessibility.preferred_output = OUTPUT_BOTH;
            state->accessibility.tts_enabled = true;
            break;
    }

    // TTS Speed
    if (output_choice >= 2) {
        printf("\n  Audio reading speed (0.5 = slow, 1.0 = normal, 1.5 = fast)?\n");
        char speed_buf[16];
        read_string("Speed [0.5-2.0]", speed_buf, sizeof(speed_buf));
        state->accessibility.tts_speed = atof(speed_buf);
        if (state->accessibility.tts_speed < 0.5) state->accessibility.tts_speed = 0.5f;
        if (state->accessibility.tts_speed > 2.0) state->accessibility.tts_speed = 2.0f;
    } else {
        state->accessibility.tts_speed = 1.0f;
    }

    // Session duration (Pomodoro)
    printf("\n  How long should study sessions be? (in minutes)\n");
    printf("  " ANSI_DIM "Recommended: 25 minutes (Pomodoro technique)" ANSI_RESET "\n");
    char dur_buf[16];
    read_string("Duration [10-60]", dur_buf, sizeof(dur_buf));
    state->session_duration = atoi(dur_buf);
    if (state->session_duration < 10) state->session_duration = 25;
    if (state->session_duration > 60) state->session_duration = 60;

    // Break duration
    printf("\n  How long should breaks be?\n");
    char break_buf[16];
    read_string("Break [5-15]", break_buf, sizeof(break_buf));
    state->break_duration = atoi(break_buf);
    if (state->break_duration < 5) state->break_duration = 5;
    if (state->break_duration > 15) state->break_duration = 15;

    printf("\n");
    print_success("Preferences saved!");
    return true;
}

/**
 * S06: Step 5 - Study method
 */
static bool wizard_step5_study_method(WizardState* state) {
    print_header("Study Method", 5, 6);

    printf("  Tell us about how you usually study.\n");
    printf("  This helps us personalize your experience.\n\n");

    // Learning style
    printf("  " ANSI_BOLD "How do you learn best?" ANSI_RESET "\n");
    print_option(1, "Visual", "Images, graphs, videos, mind maps");
    print_option(2, "Auditory", "Listening, discussions, oral explanations");
    print_option(3, "Kinesthetic", "Doing, trying, hands-on experiments");
    print_option(4, "Reading/Writing", "Reading, taking notes, summaries");
    print_option(5, "Mixed", "A bit of everything");

    int style_choice = read_int_choice(1, 5);
    switch (style_choice) {
        case 1: strncpy(state->learning_style, "visual", sizeof(state->learning_style) - 1); break;
        case 2: strncpy(state->learning_style, "auditory", sizeof(state->learning_style) - 1); break;
        case 3: strncpy(state->learning_style, "kinesthetic", sizeof(state->learning_style) - 1); break;
        case 4: strncpy(state->learning_style, "reading", sizeof(state->learning_style) - 1); break;
        case 5: strncpy(state->learning_style, "mixed", sizeof(state->learning_style) - 1); break;
    }
    state->learning_style[sizeof(state->learning_style) - 1] = '\0';

    // Current study method
    printf("\n  " ANSI_BOLD "How do you usually study?" ANSI_RESET "\n");
    printf("  (Write freely, press Enter when done)\n");
    read_string("", state->study_method, sizeof(state->study_method));

    // Challenges - just for UX, not stored
    printf("\n  " ANSI_BOLD "What do you find most difficult about studying?" ANSI_RESET "\n");
    print_option(1, "Focus", "I get distracted easily");
    print_option(2, "Memory", "I struggle to remember things");
    print_option(3, "Comprehension", "I don't understand right away");
    print_option(4, "Organization", "I don't know where to start");
    print_option(5, "Motivation", "I don't feel like studying");
    print_option(6, "Nothing specific", "I study well enough");
    read_int_choice(1, 6);

    printf("\n");
    print_success("Study profile acquired!");
    return true;
}

/**
 * S07: Step 6 - Personal goals
 */
static bool wizard_step6_goals(WizardState* state) {
    print_header("Goals", 6, 6);

    printf("  What do you want to achieve with your studies?\n");
    printf("  You can add up to %d goals.\n\n", MAX_GOALS);

    state->goals_count = 0;

    for (int i = 0; i < MAX_GOALS; i++) {
        printf("  " ANSI_BOLD "Goal %d:" ANSI_RESET "\n", i + 1);
        print_option(1, "Improve in a specific subject", "");
        print_option(2, "Prepare for an exam", "");
        print_option(3, "Catch up on a subject", "");
        print_option(4, "Deep dive into a topic", "");
        print_option(5, "Other personal goal", "");
        print_option(6, "Done with goals", "Finished");

        int goal_choice = read_int_choice(1, 6);
        if (goal_choice == 6) break;

        char description[256] = {0};

        switch (goal_choice) {
            case 1:
                printf("  Which subject do you want to improve?\n");
                read_string("", description, sizeof(description));
                snprintf(state->goals[i], MAX_GOAL_LEN,
                         "Improve in %s", description);
                break;
            case 2:
                printf("  What exam are you preparing for?\n");
                read_string("", description, sizeof(description));
                snprintf(state->goals[i], MAX_GOAL_LEN,
                         "Prepare for exam: %s", description);
                break;
            case 3:
                printf("  Which subject do you need to catch up on?\n");
                read_string("", description, sizeof(description));
                snprintf(state->goals[i], MAX_GOAL_LEN,
                         "Catch up in %s", description);
                break;
            case 4:
                printf("  What topic do you want to explore?\n");
                read_string("", description, sizeof(description));
                snprintf(state->goals[i], MAX_GOAL_LEN,
                         "Deep dive: %s", description);
                break;
            case 5:
                printf("  Describe your goal:\n");
                read_string("", state->goals[i], MAX_GOAL_LEN);
                break;
        }

        state->goals_count++;
        print_success("Goal added!");
        printf("\n");
    }

    if (state->goals_count == 0) {
        strncpy(state->goals[0], "Study and learn", MAX_GOAL_LEN - 1);
        state->goals[0][MAX_GOAL_LEN - 1] = '\0';
        state->goals_count = 1;
    }

    print_success("Goals registered!");
    return true;
}

/**
 * S08: Show summary and confirm
 */
static bool wizard_show_summary(WizardState* state) {
    print_header("Profile Summary", 0, 0);

    printf("  Here is your complete profile:\n\n");

    printf("  " ANSI_BOLD "👤 Name:" ANSI_RESET " %s", state->name);
    if (state->age > 0) {
        printf(" (%d years old)", state->age);
    }
    printf("\n");

    printf("  " ANSI_BOLD "📚 Curriculum:" ANSI_RESET " %s (Year %d)\n",
           state->curriculum_id, state->grade_level);

    printf("  " ANSI_BOLD "🎯 Learning style:" ANSI_RESET " %s\n", state->learning_style);

    printf("  " ANSI_BOLD "⏱️ Sessions:" ANSI_RESET " %d min study, %d min break\n",
           state->session_duration, state->break_duration);

    // Accessibility summary
    printf("  " ANSI_BOLD "♿ Accessibility:" ANSI_RESET " ");
    bool has_any = false;
    if (state->accessibility.dyslexia) { printf("Dyslexia "); has_any = true; }
    if (state->accessibility.dyscalculia) { printf("Dyscalculia "); has_any = true; }
    if (state->accessibility.adhd) { printf("ADHD "); has_any = true; }
    if (state->accessibility.autism) { printf("Autism "); has_any = true; }
    if (state->accessibility.cerebral_palsy) { printf("Cerebral Palsy "); has_any = true; }
    if (!has_any) { printf("No special needs"); }
    printf("\n");

    // Goals
    printf("  " ANSI_BOLD "🎯 Goals:" ANSI_RESET "\n");
    for (int i = 0; i < state->goals_count; i++) {
        printf("     • %s\n", state->goals[i]);
    }

    printf("\n");
    return read_yes_no("Confirm this data?", true);
}

/**
 * S08: Save profile to database
 */
static int64_t wizard_save_profile(WizardState* state) {
    // Build create options
    EducationCreateOptions options = {0};
    options.name = state->name;
    options.age = state->age;
    options.grade_level = state->grade_level;
    options.curriculum_id = state->curriculum_id;
    options.parent_name = strlen(state->parent_name) > 0 ? state->parent_name : NULL;
    options.parent_email = strlen(state->parent_email) > 0 ? state->parent_email : NULL;
    options.accessibility = &state->accessibility;

    // Create profile
    int64_t profile_id = education_profile_create(&options);
    if (profile_id < 0) {
        return -1;
    }

    // Save accessibility settings
    education_accessibility_update(profile_id, &state->accessibility);

    // Add goals
    for (int i = 0; i < state->goals_count; i++) {
        education_goal_add(profile_id, GOAL_MEDIUM_TERM, state->goals[i], 0);
    }

    return profile_id;
}

/**
 * S09: Activate and broadcast profile
 */
static bool wizard_activate_profile(int64_t profile_id) {
    if (education_profile_set_active(profile_id) != 0) {
        print_warning("Errore nell'attivazione del profilo.");
        return false;
    }

    // Broadcast to maestri
    education_maestro_broadcast_profile(profile_id);

    print_info("Profile shared with all 15 maestri.");
    print_info("Each maestro will adapt their style to your needs.");

    return true;
}

// ============================================================================
// MAIN ENTRY POINTS
// ============================================================================

/**
 * S01: Comando /education setup - Entry point del wizard
 */
bool education_setup_wizard(void) {
    // Initialize education database if needed
    if (education_init() != 0) {
        print_warning("Errore nell'inizializzazione del sistema educativo.");
        return false;
    }

    // Check for existing active profile
    EducationStudentProfile* existing = education_profile_get_active();
    if (existing != NULL) {
        clear_screen();
        print_header("Existing Profile", 0, 0);
        printf("  A profile already exists for %s.\n\n", existing->name);

        print_option(1, "Continue with this profile", "");
        print_option(2, "Create a new profile", "");
        print_option(3, "Edit existing profile", "");

        int choice = read_int_choice(1, 3);

        if (choice == 1) {
            print_success("Existing profile reactivated!");
            return true;
        } else if (choice == 3) {
            print_warning("Profile editing not yet implemented. Proceeding with new profile.");
        }
        // choice == 2 or fallthrough: create new
    }

    // Create wizard state
    WizardState state = {0};
    state.session_duration = 25;
    state.break_duration = 5;
    state.accessibility.tts_speed = 1.0f;

    clear_screen();
    print_header("Welcome!", 0, 0);
    printf("  Hello! I'm here to configure your student profile.\n");
    printf("  I'll ask you some questions to get to know you better\n");
    printf("  and help you in the best way possible.\n\n");
    printf("  Press Enter to begin...\n");
    getchar();

    // Run wizard steps
    clear_screen();
    if (!wizard_step1_basic_info(&state)) return false;

    clear_screen();
    if (!wizard_step2_curriculum(&state)) return false;

    clear_screen();
    if (!wizard_step3_accessibility(&state)) return false;

    clear_screen();
    if (!wizard_step4_preferences(&state)) return false;

    clear_screen();
    if (!wizard_step5_study_method(&state)) return false;

    clear_screen();
    if (!wizard_step6_goals(&state)) return false;

    clear_screen();
    if (!wizard_show_summary(&state)) {
        print_warning("Setup cancelled. Try again with /education setup");
        return false;
    }

    // Save profile
    int64_t profile_id = wizard_save_profile(&state);
    if (profile_id < 0) {
        print_warning("Errore nel salvataggio del profilo. Riprova.");
        return false;
    }

    // Activate profile
    if (!wizard_activate_profile(profile_id)) {
        return false;
    }

    printf("\n");
    print_success("Profile saved successfully!");
    printf("\n");
    printf("  " ANSI_GREEN "🎉 Welcome, %s!" ANSI_RESET "\n", state.name);
    printf("  The 15 historical maestri are ready to help you.\n\n");
    printf("  Try these commands:\n");
    printf("  • " ANSI_CYAN "/study <subject>" ANSI_RESET " - Start a study session\n");
    printf("  • " ANSI_CYAN "/homework <task>" ANSI_RESET " - Get help with homework\n");
    printf("  • " ANSI_CYAN "/quiz <topic>" ANSI_RESET " - Take a quiz\n");
    printf("  • " ANSI_CYAN "/mindmap <concept>" ANSI_RESET " - Create a mind map\n");
    printf("\n");

    return true;
}

/**
 * Quick setup for testing - creates a basic profile
 */
bool education_quick_setup(const char* name, const char* curriculum, int grade) {
    if (education_init() != 0) {
        return false;
    }

    EducationCreateOptions options = {0};
    options.name = name;
    options.age = 0;
    options.grade_level = grade;
    options.curriculum_id = curriculum;
    options.parent_name = NULL;
    options.parent_email = NULL;
    options.accessibility = NULL;

    int64_t profile_id = education_profile_create(&options);
    if (profile_id < 0) {
        return false;
    }

    return education_profile_set_active(profile_id) == 0;
}
