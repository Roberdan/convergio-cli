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

// ANSI Color codes for terminal output
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_DIM     "\033[2m"

// ============================================================================
// AVAILABLE CURRICULA
// ============================================================================

typedef struct {
    const char* id;
    const char* name;
    const char* description;
    int min_grade;
    int max_grade;
} EducationCurriculum;

static const EducationCurriculum AVAILABLE_CURRICULA[] = {
    {"elementari", "Scuola Elementare", "Classi 1-5 elementare", 1, 5},
    {"scuola_media", "Scuola Media", "Classi 1-3 media", 6, 8},
    {"liceo_scientifico", "Liceo Scientifico", "5 anni di liceo scientifico", 9, 13},
    {"liceo_classico", "Liceo Classico", "5 anni di liceo classico", 9, 13},
    {"liceo_linguistico", "Liceo Linguistico", "5 anni di liceo linguistico", 9, 13},
    {"liceo_artistico", "Liceo Artistico", "5 anni di liceo artistico", 9, 13},
    {"iti_informatica", "ITI Informatica", "Istituto Tecnico Informatico", 9, 13},
    {"custom", "Percorso Personalizzato", "Scegli tu le materie", 1, 13},
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
    {"dyslexia", "Dislessia",
     "Difficoltà nella lettura e nel riconoscimento delle parole",
     "Supporto: Font OpenDyslexic, TTS, sillabazione, sfondo crema"},
    {"dyscalculia", "Discalculia",
     "Difficoltà con numeri e calcoli matematici",
     "Supporto: Visualizzazioni, step-by-step, niente timer, colori"},
    {"adhd", "ADHD",
     "Difficoltà di attenzione e/o iperattività",
     "Supporto: Risposte brevi, celebrazioni, gamification, pause"},
    {"autism", "Autismo",
     "Diverse modalità di elaborazione sociale e sensoriale",
     "Supporto: Linguaggio letterale, struttura prevedibile, dettagli"},
    {"cerebral_palsy", "Paralisi Cerebrale",
     "Difficoltà motorie di vario grado",
     "Supporto: Input vocale, timeout estesi, pause frequenti"},
    {"visual", "Disabilità Visiva",
     "Ridotta capacità visiva",
     "Supporto: TTS, alto contrasto, font grandi"},
    {"hearing", "Disabilità Uditiva",
     "Ridotta capacità uditiva",
     "Supporto: Contenuti testuali, sottotitoli"},
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
    printf(ANSI_BOLD "   🎓 CONVERGIO EDUCATION - Setup Studente\n" ANSI_RESET);
    if (step > 0) {
        printf(ANSI_DIM "   Passo %d di %d: %s\n" ANSI_RESET, step, total_steps, title);
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

        // Remove newline
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
    printf("  > %s: ", prompt);
    fflush(stdout);

    if (fgets(buffer, max_len, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    // Remove newline
    buffer[strcspn(buffer, "\n")] = 0;

    // Trim leading/trailing whitespace
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
static bool wizard_step1_basic_info(EducationStudentProfile* profile) {
    print_header("Informazioni Base", 1, 6);

    printf("  Benvenuto nel setup del profilo studente!\n");
    printf("  Iniziamo con alcune informazioni di base.\n\n");

    // Nome studente
    read_string("Come ti chiami?", profile->name, sizeof(profile->name));
    if (strlen(profile->name) == 0) {
        print_warning("Il nome è obbligatorio.");
        return false;
    }

    // Età
    printf("\n");
    printf("  > Quanti anni hai? ");
    char age_buf[16];
    read_string("", age_buf, sizeof(age_buf));
    profile->age = atoi(age_buf);
    if (profile->age < 5 || profile->age > 99) {
        profile->age = 0; // Unknown
    }

    // Contatto genitore (opzionale)
    printf("\n");
    if (read_yes_no("Vuoi aggiungere un contatto genitore/tutore?", false)) {
        read_string("Nome genitore/tutore", profile->parent_name, sizeof(profile->parent_name));
        read_string("Email genitore/tutore", profile->parent_email, sizeof(profile->parent_email));
    }

    printf("\n");
    print_success("Informazioni base salvate!");
    return true;
}

/**
 * S03: Step 2 - Selezione curriculum
 */
static bool wizard_step2_curriculum(EducationStudentProfile* profile) {
    print_header("Selezione Curriculum", 2, 6);

    printf("  Che tipo di scuola frequenti?\n\n");

    int count = 0;
    for (int i = 0; AVAILABLE_CURRICULA[i].id != NULL; i++) {
        print_option(i + 1, AVAILABLE_CURRICULA[i].name, AVAILABLE_CURRICULA[i].description);
        count++;
    }

    int choice = read_int_choice(1, count);
    if (choice < 1) return false;

    const EducationCurriculum* selected = &AVAILABLE_CURRICULA[choice - 1];
    strncpy(profile->curriculum_id, selected->id, sizeof(profile->curriculum_id) - 1);

    // Anno specifico
    if (selected->max_grade > selected->min_grade) {
        printf("\n  Che anno stai frequentando? (%d-%d)\n",
               selected->min_grade, selected->max_grade);
        profile->grade_level = read_int_choice(selected->min_grade, selected->max_grade);
    } else {
        profile->grade_level = selected->min_grade;
    }

    printf("\n");
    print_success("Curriculum selezionato!");
    printf("  %s - Anno %d\n", selected->name, profile->grade_level);

    return true;
}

/**
 * S04: Step 3 - Assessment accessibilità
 */
static bool wizard_step3_accessibility(EducationStudentProfile* profile) {
    print_header("Accessibilità", 3, 6);

    printf("  Parliamo delle tue esigenze di apprendimento.\n");
    printf("  Tutto quello che indichi sarà usato SOLO per aiutarti meglio.\n");
    printf("  Nessun giudizio, solo supporto.\n\n");

    // Initialize accessibility
    memset(&profile->accessibility, 0, sizeof(profile->accessibility));

    for (int i = 0; ACCESSIBILITY_CONDITIONS[i].id != NULL; i++) {
        const AccessibilityCondition* cond = &ACCESSIBILITY_CONDITIONS[i];

        printf("  " ANSI_BOLD "%s" ANSI_RESET "\n", cond->name);
        printf("  " ANSI_DIM "%s" ANSI_RESET "\n", cond->description);

        bool has_condition = read_yes_no("Hai questa condizione?", false);

        if (has_condition) {
            print_info(cond->support_info);

            // Set specific flags based on condition
            if (strcmp(cond->id, "dyslexia") == 0) {
                profile->accessibility.dyslexia = true;
                printf("  Quanto è severa? (1=lieve, 2=moderata, 3=severa)\n");
                profile->accessibility.dyslexia_severity = read_int_choice(1, 3);
            } else if (strcmp(cond->id, "dyscalculia") == 0) {
                profile->accessibility.dyscalculia = true;
                profile->accessibility.dyscalculia_severity = 2; // Default moderate
            } else if (strcmp(cond->id, "adhd") == 0) {
                profile->accessibility.adhd = true;
            } else if (strcmp(cond->id, "autism") == 0) {
                profile->accessibility.autism = true;
            } else if (strcmp(cond->id, "cerebral_palsy") == 0) {
                profile->accessibility.cerebral_palsy = true;
            } else if (strcmp(cond->id, "visual") == 0) {
                profile->accessibility.visual_impairment = true;
            } else if (strcmp(cond->id, "hearing") == 0) {
                profile->accessibility.hearing_impairment = true;
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
static bool wizard_step4_preferences(EducationStudentProfile* profile) {
    print_header("Preferenze", 4, 6);

    printf("  Come preferisci interagire con i maestri?\n\n");

    // Input preference
    printf("  " ANSI_BOLD "Input preferito:" ANSI_RESET "\n");
    print_option(1, "Tastiera", "Scrivo le domande");
    print_option(2, "Voce", "Parlo al microfono");
    print_option(3, "Entrambi", "Uso entrambi a seconda del momento");

    int input_choice = read_int_choice(1, 3);
    switch (input_choice) {
        case 1: strcpy(profile->preferences.preferred_input, "keyboard"); break;
        case 2: strcpy(profile->preferences.preferred_input, "voice"); break;
        case 3: strcpy(profile->preferences.preferred_input, "both"); break;
    }

    // Output preference
    printf("\n  " ANSI_BOLD "Output preferito:" ANSI_RESET "\n");
    print_option(1, "Solo testo", "Leggo le risposte");
    print_option(2, "Solo audio", "Ascolto le risposte (TTS)");
    print_option(3, "Entrambi", "Testo + audio insieme");

    int output_choice = read_int_choice(1, 3);
    switch (output_choice) {
        case 1: strcpy(profile->preferences.preferred_output, "text"); break;
        case 2: strcpy(profile->preferences.preferred_output, "tts"); break;
        case 3: strcpy(profile->preferences.preferred_output, "both"); break;
    }

    // TTS Speed
    if (output_choice >= 2) {
        printf("\n  Velocità lettura audio (0.5 = lento, 1.0 = normale, 1.5 = veloce)?\n");
        printf("  > Velocità [0.5-2.0]: ");
        char speed_buf[16];
        read_string("", speed_buf, sizeof(speed_buf));
        profile->preferences.tts_speed = atof(speed_buf);
        if (profile->preferences.tts_speed < 0.5) profile->preferences.tts_speed = 0.5;
        if (profile->preferences.tts_speed > 2.0) profile->preferences.tts_speed = 2.0;
    } else {
        profile->preferences.tts_speed = 1.0;
    }

    // Session duration (Pomodoro)
    printf("\n  Quanto vuoi che durino le sessioni di studio? (in minuti)\n");
    printf("  " ANSI_DIM "Consigliato: 25 minuti (tecnica Pomodoro)" ANSI_RESET "\n");
    printf("  > Durata [10-60]: ");
    char dur_buf[16];
    read_string("", dur_buf, sizeof(dur_buf));
    profile->preferences.session_duration = atoi(dur_buf);
    if (profile->preferences.session_duration < 10) profile->preferences.session_duration = 25;
    if (profile->preferences.session_duration > 60) profile->preferences.session_duration = 60;

    // Break duration
    printf("\n  Quanto vuoi che durino le pause?\n");
    printf("  > Pausa [5-15]: ");
    char break_buf[16];
    read_string("", break_buf, sizeof(break_buf));
    profile->preferences.break_duration = atoi(break_buf);
    if (profile->preferences.break_duration < 5) profile->preferences.break_duration = 5;
    if (profile->preferences.break_duration > 15) profile->preferences.break_duration = 15;

    printf("\n");
    print_success("Preferenze salvate!");
    return true;
}

/**
 * S06: Step 5 - Metodo di studio attuale
 */
static bool wizard_step5_study_method(EducationStudentProfile* profile) {
    print_header("Metodo di Studio", 5, 6);

    printf("  Raccontaci come studi di solito.\n");
    printf("  Questo ci aiuta a personalizzare l'esperienza.\n\n");

    // Learning style
    printf("  " ANSI_BOLD "Come impari meglio?" ANSI_RESET "\n");
    print_option(1, "Visivo", "Immagini, grafici, video, mappe mentali");
    print_option(2, "Uditivo", "Ascolto, discussioni, spiegazioni orali");
    print_option(3, "Cinestetico", "Fare, provare, esperimenti pratici");
    print_option(4, "Lettura/Scrittura", "Leggere, prendere appunti, riassunti");
    print_option(5, "Misto", "Un po' di tutto");

    int style_choice = read_int_choice(1, 5);
    switch (style_choice) {
        case 1: strcpy(profile->learning_style, "visual"); break;
        case 2: strcpy(profile->learning_style, "auditory"); break;
        case 3: strcpy(profile->learning_style, "kinesthetic"); break;
        case 4: strcpy(profile->learning_style, "reading"); break;
        case 5: strcpy(profile->learning_style, "mixed"); break;
    }

    // Current study method
    printf("\n  " ANSI_BOLD "Come studi di solito?" ANSI_RESET "\n");
    printf("  (Scrivi liberamente, premi Invio quando hai finito)\n");
    read_string("", profile->study_method, sizeof(profile->study_method));

    // Challenges
    printf("\n  " ANSI_BOLD "Cosa trovi più difficile nello studio?" ANSI_RESET "\n");
    print_option(1, "Concentrazione", "Mi distraggo facilmente");
    print_option(2, "Memoria", "Faccio fatica a ricordare");
    print_option(3, "Comprensione", "Non capisco al primo colpo");
    print_option(4, "Organizzazione", "Non so da dove iniziare");
    print_option(5, "Motivazione", "Non ho voglia di studiare");
    print_option(6, "Nessuna in particolare", "Studio abbastanza bene");

    // Just for personalization, not stored
    read_int_choice(1, 6);

    printf("\n");
    print_success("Profilo di studio acquisito!");
    return true;
}

/**
 * S07: Step 6 - Obiettivi personali
 */
static bool wizard_step6_goals(EducationStudentProfile* profile) {
    print_header("Obiettivi", 6, 6);

    printf("  Cosa vuoi ottenere con il tuo studio?\n");
    printf("  Puoi aggiungere fino a %d obiettivi.\n\n", MAX_GOALS);

    profile->goals_count = 0;

    for (int i = 0; i < MAX_GOALS; i++) {
        printf("  " ANSI_BOLD "Obiettivo %d:" ANSI_RESET "\n", i + 1);
        print_option(1, "Migliorare in una materia specifica", "");
        print_option(2, "Preparare un esame", "");
        print_option(3, "Recuperare un debito", "");
        print_option(4, "Approfondire un argomento", "");
        print_option(5, "Altro obiettivo personale", "");
        print_option(6, "Basta obiettivi", "Ho finito");

        int goal_choice = read_int_choice(1, 6);
        if (goal_choice == 6) break;

        char description[256] = {0};

        switch (goal_choice) {
            case 1:
                printf("  Quale materia vuoi migliorare?\n");
                read_string("", description, sizeof(description));
                snprintf(profile->goals[i], sizeof(profile->goals[i]),
                         "Migliorare in %s", description);
                break;
            case 2:
                printf("  Che esame devi preparare?\n");
                read_string("", description, sizeof(description));
                snprintf(profile->goals[i], sizeof(profile->goals[i]),
                         "Preparare esame: %s", description);
                break;
            case 3:
                printf("  Quale debito devi recuperare?\n");
                read_string("", description, sizeof(description));
                snprintf(profile->goals[i], sizeof(profile->goals[i]),
                         "Recuperare debito in %s", description);
                break;
            case 4:
                printf("  Cosa vuoi approfondire?\n");
                read_string("", description, sizeof(description));
                snprintf(profile->goals[i], sizeof(profile->goals[i]),
                         "Approfondire: %s", description);
                break;
            case 5:
                printf("  Descrivi il tuo obiettivo:\n");
                read_string("", profile->goals[i], sizeof(profile->goals[i]));
                break;
        }

        profile->goals_count++;
        print_success("Obiettivo aggiunto!");
        printf("\n");
    }

    if (profile->goals_count == 0) {
        strcpy(profile->goals[0], "Studiare e imparare");
        profile->goals_count = 1;
    }

    print_success("Obiettivi registrati!");
    return true;
}

/**
 * S08: Generazione profilo completo
 */
static bool wizard_finalize_profile(EducationStudentProfile* profile) {
    print_header("Riepilogo Profilo", 0, 0);

    printf("  Ecco il tuo profilo completo:\n\n");

    printf("  " ANSI_BOLD "👤 Nome:" ANSI_RESET " %s", profile->name);
    if (profile->age > 0) {
        printf(" (%d anni)", profile->age);
    }
    printf("\n");

    printf("  " ANSI_BOLD "📚 Curriculum:" ANSI_RESET " %s (Anno %d)\n",
           profile->curriculum_id, profile->grade_level);

    printf("  " ANSI_BOLD "🎯 Stile apprendimento:" ANSI_RESET " %s\n", profile->learning_style);

    printf("  " ANSI_BOLD "⏱️ Sessioni:" ANSI_RESET " %d min studio, %d min pausa\n",
           profile->preferences.session_duration, profile->preferences.break_duration);

    // Accessibility summary
    printf("  " ANSI_BOLD "♿ Accessibilità:" ANSI_RESET " ");
    bool has_any = false;
    if (profile->accessibility.dyslexia) {
        printf("Dislessia ");
        has_any = true;
    }
    if (profile->accessibility.dyscalculia) {
        printf("Discalculia ");
        has_any = true;
    }
    if (profile->accessibility.adhd) {
        printf("ADHD ");
        has_any = true;
    }
    if (profile->accessibility.autism) {
        printf("Autismo ");
        has_any = true;
    }
    if (profile->accessibility.cerebral_palsy) {
        printf("Paralisi Cerebrale ");
        has_any = true;
    }
    if (!has_any) {
        printf("Nessuna esigenza speciale");
    }
    printf("\n");

    // Goals
    printf("  " ANSI_BOLD "🎯 Obiettivi:" ANSI_RESET "\n");
    for (int i = 0; i < profile->goals_count; i++) {
        printf("     • %s\n", profile->goals[i]);
    }

    printf("\n");

    if (!read_yes_no("Confermi questi dati?", true)) {
        print_warning("Setup annullato. Riprova con /education setup");
        return false;
    }

    // Save to database
    if (!education_save_profile(profile)) {
        print_warning("Errore nel salvataggio del profilo. Riprova.");
        return false;
    }

    printf("\n");
    print_success("Profilo salvato con successo!");
    printf("\n");
    printf("  " ANSI_GREEN "🎉 Benvenuto, %s!" ANSI_RESET "\n", profile->name);
    printf("  I 14 maestri storici sono pronti ad aiutarti.\n\n");
    printf("  Prova questi comandi:\n");
    printf("  • " ANSI_CYAN "/study <materia>" ANSI_RESET " - Inizia una sessione di studio\n");
    printf("  • " ANSI_CYAN "/homework <compito>" ANSI_RESET " - Chiedi aiuto con i compiti\n");
    printf("  • " ANSI_CYAN "/quiz <argomento>" ANSI_RESET " - Fai un quiz\n");
    printf("  • " ANSI_CYAN "/mindmap <concetto>" ANSI_RESET " - Crea una mappa mentale\n");
    printf("\n");

    return true;
}

/**
 * S09: Broadcast profilo a tutti i maestri
 */
static bool wizard_broadcast_profile(EducationStudentProfile* profile) {
    // Set as active profile for this session
    if (!education_set_active_profile(profile->id)) {
        print_warning("Errore nell'attivazione del profilo.");
        return false;
    }

    print_info("Profilo condiviso con tutti i 14 maestri.");
    print_info("Ogni maestro adatterà il suo stile alle tue esigenze.");

    return true;
}

// ============================================================================
// MAIN ENTRY POINT (S01)
// ============================================================================

/**
 * S01: Comando /education setup - Entry point del wizard
 *
 * Run the complete education setup wizard for a new student.
 * This implements the full FASE 1 wizard flow from EducationPackPlan.md.
 *
 * @return true if setup completed successfully
 */
bool education_setup_wizard(void) {
    // Initialize education database if needed
    if (!education_init(NULL)) {
        print_warning("Errore nell'inizializzazione del sistema educativo.");
        return false;
    }

    // Check for existing active profile
    EducationStudentProfile* existing = education_get_active_profile();
    if (existing != NULL) {
        clear_screen();
        print_header("Profilo Esistente", 0, 0);
        printf("  Esiste già un profilo per %s.\n\n", existing->name);

        print_option(1, "Continua con questo profilo", "");
        print_option(2, "Crea un nuovo profilo", "");
        print_option(3, "Modifica il profilo esistente", "");

        int choice = read_int_choice(1, 3);

        if (choice == 1) {
            print_success("Profilo esistente riattivato!");
            return true;
        } else if (choice == 3) {
            // TODO: Edit mode
            print_warning("Modifica profilo non ancora implementata. Procedo con nuovo profilo.");
        }
        // choice == 2 or fallthrough: create new
    }

    // Create new profile structure
    EducationStudentProfile profile;
    memset(&profile, 0, sizeof(profile));

    clear_screen();
    print_header("Benvenuto!", 0, 0);
    printf("  Ciao! Sono qui per configurare il tuo profilo studente.\n");
    printf("  Ti farò alcune domande per conoscerti meglio e aiutarti\n");
    printf("  nel modo migliore possibile.\n\n");
    printf("  Premi Invio per iniziare...\n");
    getchar();

    // Run wizard steps
    clear_screen();
    if (!wizard_step1_basic_info(&profile)) return false;

    clear_screen();
    if (!wizard_step2_curriculum(&profile)) return false;

    clear_screen();
    if (!wizard_step3_accessibility(&profile)) return false;

    clear_screen();
    if (!wizard_step4_preferences(&profile)) return false;

    clear_screen();
    if (!wizard_step5_study_method(&profile)) return false;

    clear_screen();
    if (!wizard_step6_goals(&profile)) return false;

    clear_screen();
    if (!wizard_finalize_profile(&profile)) return false;

    if (!wizard_broadcast_profile(&profile)) return false;

    return true;
}

/**
 * Quick setup for testing - creates a basic profile
 */
bool education_quick_setup(const char* name, const char* curriculum, int grade) {
    if (!education_init(NULL)) {
        return false;
    }

    EducationStudentProfile profile;
    memset(&profile, 0, sizeof(profile));

    strncpy(profile.name, name, sizeof(profile.name) - 1);
    strncpy(profile.curriculum_id, curriculum, sizeof(profile.curriculum_id) - 1);
    profile.grade_level = grade;
    profile.preferences.session_duration = 25;
    profile.preferences.break_duration = 5;
    profile.preferences.tts_speed = 1.0;
    strcpy(profile.preferences.preferred_input, "keyboard");
    strcpy(profile.preferences.preferred_output, "text");
    strcpy(profile.learning_style, "mixed");
    strcpy(profile.goals[0], "Studiare e imparare");
    profile.goals_count = 1;

    if (!education_save_profile(&profile)) {
        return false;
    }

    return education_set_active_profile(profile.id);
}
