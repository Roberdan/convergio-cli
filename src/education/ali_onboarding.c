/**
 * CONVERGIO EDUCATION - ALI ONBOARDING (EDU-01, EDU-02)
 *
 * Two-phase onboarding system:
 * 1. API Setup (for parents) - Minimal, technical, one-time
 * 2. Ali Preside Onboarding - Conversational AI interview with student
 *
 * This replaces the old form-based wizard with a natural conversation
 * where Ali asks questions and gets to know the student.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under MIT License
 */

#include "nous/education.h"
#include "nous/nous.h"
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

#define MAX_INPUT_LENGTH 512
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_DIM     "\033[2m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_ALI     "\033[1;38;5;135m"  // Ali's signature color

// ============================================================================
// INTERNAL TYPES
// ============================================================================

typedef struct {
    char name[64];
    int age;
    char grade_info[128];
    char favorite_subjects[256];
    char difficult_subjects[256];
    char learning_preferences[256];
    char goals[512];
    char accessibility_notes[256];
    bool needs_accessibility;
} AliOnboardingData;

// ============================================================================
// PHASE 1: API SETUP (FOR PARENTS)
// ============================================================================

static void print_parent_header(void) {
    printf("\n");
    printf("  " ANSI_CYAN "┌─────────────────────────────────────────────────────────────┐" ANSI_RESET "\n");
    printf("  " ANSI_CYAN "│" ANSI_RESET "  " ANSI_BOLD "Convergio Education - Configurazione Iniziale" ANSI_RESET "              " ANSI_CYAN "│" ANSI_RESET "\n");
    printf("  " ANSI_CYAN "│" ANSI_RESET "  " ANSI_DIM "Per genitori/tutori" ANSI_RESET "                                         " ANSI_CYAN "│" ANSI_RESET "\n");
    printf("  " ANSI_CYAN "└─────────────────────────────────────────────────────────────┘" ANSI_RESET "\n");
    printf("\n");
}

static bool check_api_key_configured(void) {
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    return (api_key && strlen(api_key) > 10);
}

static void show_api_key_instructions(void) {
    printf("  Per utilizzare Convergio Education, è necessaria una chiave API.\n\n");

    printf("  " ANSI_BOLD "Come ottenere una chiave API:" ANSI_RESET "\n");
    printf("  1. Visita " ANSI_CYAN "https://console.anthropic.com" ANSI_RESET "\n");
    printf("  2. Crea un account o accedi\n");
    printf("  3. Vai su 'API Keys' e crea una nuova chiave\n");
    printf("  4. Copia la chiave (inizia con 'sk-ant-')\n\n");

    printf("  " ANSI_BOLD "Per configurarla:" ANSI_RESET "\n");
    printf("  Aggiungi questa riga al file ~/.zshrc:\n");
    printf("  " ANSI_YELLOW "export ANTHROPIC_API_KEY=\"la-tua-chiave-qui\"" ANSI_RESET "\n\n");

    printf("  Poi esegui: " ANSI_YELLOW "source ~/.zshrc" ANSI_RESET "\n\n");

    printf("  " ANSI_DIM "La chiave è sicura e usata solo per le conversazioni educative." ANSI_RESET "\n");
}

/**
 * Phase 1: Check/setup API key (for parents)
 * Returns true if API is ready, false if needs setup
 */
bool ali_check_api_setup(void) {
    if (check_api_key_configured()) {
        return true;
    }

    print_parent_header();

    printf("  " ANSI_YELLOW "⚠ Configurazione richiesta" ANSI_RESET "\n\n");
    show_api_key_instructions();

    printf("  Premi Invio dopo aver configurato la chiave API...\n");
    printf("  " ANSI_DIM "(oppure digita 'esci' per uscire)" ANSI_RESET "\n\n");
    printf("  > ");
    fflush(stdout);

    char input[64];
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strstr(input, "esci") || strstr(input, "exit")) {
            return false;
        }
    }

    // Re-check
    if (check_api_key_configured()) {
        printf("\n  " ANSI_GREEN "✓ Chiave API configurata correttamente!" ANSI_RESET "\n\n");
        return true;
    }

    printf("\n  " ANSI_YELLOW "La chiave API non è ancora configurata." ANSI_RESET "\n");
    printf("  Riavvia Convergio dopo aver completato la configurazione.\n\n");
    return false;
}

// ============================================================================
// PHASE 2: ALI PRESIDE ONBOARDING (CONVERSATIONAL)
// ============================================================================

static void print_ali_message(const char* message) {
    printf("\n  " ANSI_ALI "🎓 Ali:" ANSI_RESET " %s\n", message);
}

static void print_ali_prompt(void) {
    printf("\n  " ANSI_DIM "Tu:" ANSI_RESET " ");
    fflush(stdout);
}

static void read_student_input(char* buffer, size_t max_len) {
    if (fgets(buffer, max_len, stdin)) {
        buffer[strcspn(buffer, "\n")] = 0;
        // Trim whitespace
        char* start = buffer;
        while (*start && isspace(*start)) start++;
        if (start != buffer) memmove(buffer, start, strlen(start) + 1);
        char* end = buffer + strlen(buffer) - 1;
        while (end > buffer && isspace(*end)) *end-- = '\0';
    }
}

static int extract_age_from_response(const char* response) {
    // Try to extract a number from the response
    int age = 0;
    for (size_t i = 0; i < strlen(response); i++) {
        if (isdigit(response[i])) {
            age = atoi(response + i);
            if (age >= 5 && age <= 25) return age;
        }
    }
    return 0;
}

/**
 * Phase 2: Ali's conversational onboarding
 * Ali asks questions naturally and builds the student profile
 */
bool ali_conversational_onboarding(void) {
    AliOnboardingData data = {0};
    char input[MAX_INPUT_LENGTH];

    // Clear screen and show Ali's welcome
    printf("\033[2J\033[H");

    printf("\n");
    printf("  " ANSI_ALI "┌─────────────────────────────────────────────────────────────┐" ANSI_RESET "\n");
    printf("  " ANSI_ALI "│" ANSI_RESET "  " ANSI_BOLD "🎓 Ali, il Preside" ANSI_RESET "                                          " ANSI_ALI "│" ANSI_RESET "\n");
    printf("  " ANSI_ALI "│" ANSI_RESET "  " ANSI_DIM "Benvenuto a Convergio Education" ANSI_RESET "                              " ANSI_ALI "│" ANSI_RESET "\n");
    printf("  " ANSI_ALI "└─────────────────────────────────────────────────────────────┘" ANSI_RESET "\n");

    // ========================================
    // Question 1: Name
    // ========================================
    print_ali_message("Ciao! Sono Ali, il Preside di questa scuola virtuale.");
    print_ali_message("Sono felice di conoscerti! Come ti chiami?");

    print_ali_prompt();
    read_student_input(input, sizeof(input));

    if (strlen(input) == 0) {
        strncpy(data.name, "Studente", sizeof(data.name) - 1);
    } else {
        // Extract first word as name
        char* space = strchr(input, ' ');
        if (space) *space = '\0';
        // Capitalize first letter
        input[0] = toupper(input[0]);
        strncpy(data.name, input, sizeof(data.name) - 1);
    }

    // ========================================
    // Question 2: Age/Grade
    // ========================================
    char age_msg[256];
    snprintf(age_msg, sizeof(age_msg),
        "Piacere di conoscerti, %s! Quanti anni hai e che classe frequenti?",
        data.name);
    print_ali_message(age_msg);

    print_ali_prompt();
    read_student_input(input, sizeof(input));

    data.age = extract_age_from_response(input);
    if (data.age == 0) data.age = 12; // Default
    strncpy(data.grade_info, input, sizeof(data.grade_info) - 1);

    // ========================================
    // Question 3: Favorite subjects
    // ========================================
    print_ali_message("Perfetto! Adesso dimmi: quali sono le materie che ti piacciono di più?");
    print_ali_message(ANSI_DIM "Puoi dirmi tutto quello che vuoi, anche più materie!" ANSI_RESET);

    print_ali_prompt();
    read_student_input(input, sizeof(input));
    strncpy(data.favorite_subjects, input, sizeof(data.favorite_subjects) - 1);

    // ========================================
    // Question 4: Difficult subjects
    // ========================================
    print_ali_message("Capisco! E invece, ci sono materie in cui fai più fatica o che non ti piacciono molto?");
    print_ali_message(ANSI_DIM "Non preoccuparti, i nostri maestri sono bravissimi ad aiutare!" ANSI_RESET);

    print_ali_prompt();
    read_student_input(input, sizeof(input));
    strncpy(data.difficult_subjects, input, sizeof(data.difficult_subjects) - 1);

    // ========================================
    // Question 5: Learning style
    // ========================================
    print_ali_message("Interessante! Come preferisci imparare? Per esempio:");
    printf("     • Leggendo e prendendo appunti?\n");
    printf("     • Guardando video e immagini?\n");
    printf("     • Ascoltando spiegazioni?\n");
    printf("     • Facendo esercizi pratici?\n");

    print_ali_prompt();
    read_student_input(input, sizeof(input));
    strncpy(data.learning_preferences, input, sizeof(data.learning_preferences) - 1);

    // ========================================
    // Question 6: Goals
    // ========================================
    print_ali_message("Ottimo! Un'ultima cosa: cosa vorresti ottenere quest'anno? Hai qualche obiettivo?");
    print_ali_message(ANSI_DIM "Può essere migliorare in una materia, preparare un esame, o qualsiasi cosa!" ANSI_RESET);

    print_ali_prompt();
    read_student_input(input, sizeof(input));
    strncpy(data.goals, input, sizeof(data.goals) - 1);

    // ========================================
    // Question 7: Accessibility (gentle)
    // ========================================
    print_ali_message("Ancora una cosa, e poi siamo pronti!");
    print_ali_message("C'è qualcosa che dovrei sapere per aiutarti meglio?");
    print_ali_message(ANSI_DIM "Per esempio: hai bisogno di caratteri più grandi, leggi più lentamente," ANSI_RESET);
    printf("     " ANSI_DIM "o preferisci che ti legga le cose ad alta voce?" ANSI_RESET "\n");
    printf("     " ANSI_DIM "(Scrivi 'no' se va tutto bene così)" ANSI_RESET "\n");

    print_ali_prompt();
    read_student_input(input, sizeof(input));

    if (strlen(input) > 0 &&
        strcasecmp(input, "no") != 0 &&
        strcasecmp(input, "niente") != 0 &&
        strcasecmp(input, "nulla") != 0) {
        data.needs_accessibility = true;
        strncpy(data.accessibility_notes, input, sizeof(data.accessibility_notes) - 1);
    }

    // ========================================
    // Summary and Confirmation
    // ========================================
    printf("\n\n");
    printf("  " ANSI_ALI "┌─────────────────────────────────────────────────────────────┐" ANSI_RESET "\n");
    printf("  " ANSI_ALI "│" ANSI_RESET "  " ANSI_BOLD "📋 Riepilogo" ANSI_RESET "                                                  " ANSI_ALI "│" ANSI_RESET "\n");
    printf("  " ANSI_ALI "└─────────────────────────────────────────────────────────────┘" ANSI_RESET "\n");

    print_ali_message("Perfetto! Ecco cosa ho capito di te:");
    printf("\n");
    printf("     " ANSI_BOLD "Nome:" ANSI_RESET " %s\n", data.name);
    printf("     " ANSI_BOLD "Età:" ANSI_RESET " %d anni\n", data.age);
    printf("     " ANSI_BOLD "Classe:" ANSI_RESET " %s\n", data.grade_info);
    printf("     " ANSI_BOLD "Materie preferite:" ANSI_RESET " %s\n", data.favorite_subjects);
    printf("     " ANSI_BOLD "Aree da migliorare:" ANSI_RESET " %s\n", data.difficult_subjects);
    printf("     " ANSI_BOLD "Stile di apprendimento:" ANSI_RESET " %s\n", data.learning_preferences);
    printf("     " ANSI_BOLD "Obiettivi:" ANSI_RESET " %s\n", data.goals);
    if (data.needs_accessibility) {
        printf("     " ANSI_BOLD "Note speciali:" ANSI_RESET " %s\n", data.accessibility_notes);
    }

    print_ali_message("Va tutto bene? (sì/no)");
    print_ali_prompt();
    read_student_input(input, sizeof(input));

    if (tolower(input[0]) == 'n') {
        print_ali_message("Nessun problema! Possiamo ricominciare quando vuoi.");
        print_ali_message("Usa il comando /education setup per riprovare.");
        return false;
    }

    // ========================================
    // Save Profile
    // ========================================

    // Initialize education system
    if (education_init() != 0) {
        print_ali_message(ANSI_YELLOW "Ops! C'è stato un problema nel salvare il profilo." ANSI_RESET);
        return false;
    }

    // Create profile
    EducationCreateOptions options = {0};
    options.name = data.name;
    options.age = data.age;
    options.grade_level = data.age - 5; // Rough estimate
    options.curriculum_id = "generale";
    options.parent_name = NULL;
    options.parent_email = NULL;
    options.accessibility = NULL;

    int64_t profile_id = education_profile_create(&options);
    if (profile_id < 0) {
        print_ali_message(ANSI_YELLOW "Ops! C'è stato un problema nel creare il profilo." ANSI_RESET);
        return false;
    }

    // Set as active
    education_profile_set_active(profile_id);

    // Add goals if provided
    if (strlen(data.goals) > 0) {
        education_goal_add(profile_id, GOAL_MEDIUM_TERM, data.goals, 0);
    }

    // Broadcast to maestri
    education_maestro_broadcast_profile(profile_id);

    // ========================================
    // Welcome Complete
    // ========================================
    printf("\n\n");
    printf("  " ANSI_GREEN "┌─────────────────────────────────────────────────────────────┐" ANSI_RESET "\n");
    printf("  " ANSI_GREEN "│" ANSI_RESET "  " ANSI_BOLD "🎉 Benvenuto a Convergio Education!" ANSI_RESET "                        " ANSI_GREEN "│" ANSI_RESET "\n");
    printf("  " ANSI_GREEN "└─────────────────────────────────────────────────────────────┘" ANSI_RESET "\n");

    print_ali_message("Fantastico! Il tuo profilo è stato creato.");
    print_ali_message("Ho già parlato con tutti i 15 maestri di te.");
    print_ali_message("Sono pronti ad aiutarti nel tuo percorso di apprendimento!\n");

    printf("  I tuoi maestri:\n");
    printf("     • " ANSI_CYAN "Socrate" ANSI_RESET " (Filosofia)    • " ANSI_CYAN "Euclide" ANSI_RESET " (Matematica)\n");
    printf("     • " ANSI_CYAN "Feynman" ANSI_RESET " (Fisica)       • " ANSI_CYAN "Darwin" ANSI_RESET " (Scienze)\n");
    printf("     • " ANSI_CYAN "Manzoni" ANSI_RESET " (Italiano)     • " ANSI_CYAN "Shakespeare" ANSI_RESET " (Inglese)\n");
    printf("     • " ANSI_CYAN "Erodoto" ANSI_RESET " (Storia)       • " ANSI_CYAN "Leonardo" ANSI_RESET " (Arte)\n");
    printf("     • " ANSI_CYAN "Mozart" ANSI_RESET " (Musica)        • " ANSI_CYAN "Lovelace" ANSI_RESET " (Informatica)\n");
    printf("     • " ANSI_CYAN "Humboldt" ANSI_RESET " (Geografia)   • " ANSI_CYAN "Smith" ANSI_RESET " (Economia)\n");
    printf("     • " ANSI_CYAN "Cicerone" ANSI_RESET " (Educazione Civica)\n");
    printf("     • " ANSI_CYAN "Ippocrate" ANSI_RESET " (Salute)     • " ANSI_CYAN "Chris" ANSI_RESET " (Storytelling)\n");

    printf("\n");
    print_ali_message("Cosa vorresti fare ora? Ecco alcuni comandi utili:\n");
    printf("     • " ANSI_YELLOW "/study <materia>" ANSI_RESET " - Inizia una sessione di studio\n");
    printf("     • " ANSI_YELLOW "/homework <compito>" ANSI_RESET " - Aiuto con i compiti\n");
    printf("     • " ANSI_YELLOW "/quiz <argomento>" ANSI_RESET " - Fai un quiz\n");
    printf("     • " ANSI_YELLOW "/mindmap <concetto>" ANSI_RESET " - Crea una mappa mentale\n");
    printf("     • " ANSI_YELLOW "/libretto" ANSI_RESET " - Guarda i tuoi progressi\n");
    printf("\n");
    print_ali_message("In bocca al lupo per i tuoi studi! 🍀\n");

    return true;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

/**
 * Main entry point for Education onboarding
 * Phase 1: Check API setup (for parents)
 * Phase 2: Ali conversational onboarding (for student)
 */
bool ali_education_onboarding(void) {
    // Phase 1: Check API setup
    if (!ali_check_api_setup()) {
        return false;
    }

    // Phase 2: Ali conversational onboarding
    return ali_conversational_onboarding();
}

/**
 * Check if student needs onboarding
 * Returns true if no active profile exists
 */
bool ali_needs_onboarding(void) {
    if (education_init() != 0) {
        return true;  // If can't init, needs onboarding
    }

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        return true;
    }

    // Profile exists
    return false;
}
