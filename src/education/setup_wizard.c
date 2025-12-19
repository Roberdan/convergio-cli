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
    // Scuola dell'Obbligo
    {"elementari", "Scuola Elementare", "Classi 1-5 elementare (Primaria)", 1, 5},
    {"scuola_media", "Scuola Media", "Classi 1-3 media (Secondaria I grado)", 6, 8},

    // Licei
    {"liceo_scientifico", "Liceo Scientifico", "5 anni con focus su matematica e scienze", 9, 13},
    {"liceo_classico", "Liceo Classico", "5 anni con latino, greco e filosofia", 9, 13},
    {"liceo_linguistico", "Liceo Linguistico", "5 anni con 3 lingue straniere", 9, 13},
    {"liceo_artistico", "Liceo Artistico", "5 anni con focus su arti visive e design", 9, 13},
    {"liceo_scienze_umane", "Liceo Scienze Umane", "5 anni con pedagogia, psicologia e sociologia", 9, 13},
    {"liceo_musicale", "Liceo Musicale e Coreutico", "5 anni con musica o danza", 9, 13},

    // Istituti Tecnici
    {"iti_informatica", "ITI Informatica", "Istituto Tecnico - Informatica e Telecomunicazioni", 9, 13},
    {"iti_elettronica", "ITI Elettronica", "Istituto Tecnico - Elettronica ed Elettrotecnica", 9, 13},
    {"iti_meccanica", "ITI Meccanica", "Istituto Tecnico - Meccanica, Meccatronica ed Energia", 9, 13},
    {"iti_chimica", "ITI Chimica", "Istituto Tecnico - Chimica, Materiali e Biotecnologie", 9, 13},

    // Istituti Professionali
    {"ipsia", "IPSIA", "Istituto Professionale - Industria e Artigianato", 9, 13},
    {"alberghiero", "Istituto Alberghiero", "Istituto Professionale - Enogastronomia e Ospitalità", 9, 13},

    // Personalizzato
    {"custom", "Percorso Personalizzato", "Scegli tu le materie (homeschooling, esigenze speciali)", 1, 13},
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
    print_header("Informazioni Base", 1, 6);

    printf("  Benvenuto nel setup del profilo studente!\n");
    printf("  Iniziamo con alcune informazioni di base.\n\n");

    // Nome studente
    read_string("Come ti chiami?", state->name, sizeof(state->name));
    if (strlen(state->name) == 0) {
        print_warning("Il nome è obbligatorio.");
        return false;
    }

    // Età
    printf("\n");
    char age_buf[16];
    read_string("Quanti anni hai?", age_buf, sizeof(age_buf));
    state->age = atoi(age_buf);
    if (state->age < 5 || state->age > 99) {
        state->age = 0;
    }

    // Contatto genitore (opzionale)
    printf("\n");
    if (read_yes_no("Vuoi aggiungere un contatto genitore/tutore?", false)) {
        read_string("Nome genitore/tutore", state->parent_name, sizeof(state->parent_name));
        read_string("Email genitore/tutore", state->parent_email, sizeof(state->parent_email));
    }

    printf("\n");
    print_success("Informazioni base salvate!");
    return true;
}

/**
 * S03: Step 2 - Selezione curriculum
 */
static bool wizard_step2_curriculum(WizardState* state) {
    print_header("Selezione Curriculum", 2, 6);

    printf("  Che tipo di scuola frequenti?\n\n");

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
    print_header("Accessibilità", 3, 6);

    printf("  Parliamo delle tue esigenze di apprendimento.\n");
    printf("  Tutto quello che indichi sarà usato SOLO per aiutarti meglio.\n");
    printf("  Nessun giudizio, solo supporto.\n\n");

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
    print_header("Preferenze", 4, 6);

    printf("  Come preferisci interagire con i maestri?\n\n");

    // Input preference
    printf("  " ANSI_BOLD "Input preferito:" ANSI_RESET "\n");
    print_option(1, "Tastiera", "Scrivo le domande");
    print_option(2, "Voce", "Parlo al microfono");
    print_option(3, "Entrambi", "Uso entrambi a seconda del momento");

    int input_choice = read_int_choice(1, 3);
    switch (input_choice) {
        case 1: state->accessibility.preferred_input = INPUT_KEYBOARD; break;
        case 2: state->accessibility.preferred_input = INPUT_VOICE; break;
        case 3: state->accessibility.preferred_input = INPUT_BOTH; break;
    }

    // Output preference
    printf("\n  " ANSI_BOLD "Output preferito:" ANSI_RESET "\n");
    print_option(1, "Solo testo", "Leggo le risposte");
    print_option(2, "Solo audio", "Ascolto le risposte (TTS)");
    print_option(3, "Entrambi", "Testo + audio insieme");

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
        printf("\n  Velocità lettura audio (0.5 = lento, 1.0 = normale, 1.5 = veloce)?\n");
        char speed_buf[16];
        read_string("Velocità [0.5-2.0]", speed_buf, sizeof(speed_buf));
        state->accessibility.tts_speed = atof(speed_buf);
        if (state->accessibility.tts_speed < 0.5) state->accessibility.tts_speed = 0.5f;
        if (state->accessibility.tts_speed > 2.0) state->accessibility.tts_speed = 2.0f;
    } else {
        state->accessibility.tts_speed = 1.0f;
    }

    // Session duration (Pomodoro)
    printf("\n  Quanto vuoi che durino le sessioni di studio? (in minuti)\n");
    printf("  " ANSI_DIM "Consigliato: 25 minuti (tecnica Pomodoro)" ANSI_RESET "\n");
    char dur_buf[16];
    read_string("Durata [10-60]", dur_buf, sizeof(dur_buf));
    state->session_duration = atoi(dur_buf);
    if (state->session_duration < 10) state->session_duration = 25;
    if (state->session_duration > 60) state->session_duration = 60;

    // Break duration
    printf("\n  Quanto vuoi che durino le pause?\n");
    char break_buf[16];
    read_string("Pausa [5-15]", break_buf, sizeof(break_buf));
    state->break_duration = atoi(break_buf);
    if (state->break_duration < 5) state->break_duration = 5;
    if (state->break_duration > 15) state->break_duration = 15;

    printf("\n");
    print_success("Preferenze salvate!");
    return true;
}

/**
 * S06: Step 5 - Metodo di studio attuale
 */
static bool wizard_step5_study_method(WizardState* state) {
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
        case 1: strncpy(state->learning_style, "visual", sizeof(state->learning_style) - 1); break;
        case 2: strncpy(state->learning_style, "auditory", sizeof(state->learning_style) - 1); break;
        case 3: strncpy(state->learning_style, "kinesthetic", sizeof(state->learning_style) - 1); break;
        case 4: strncpy(state->learning_style, "reading", sizeof(state->learning_style) - 1); break;
        case 5: strncpy(state->learning_style, "mixed", sizeof(state->learning_style) - 1); break;
    }
    state->learning_style[sizeof(state->learning_style) - 1] = '\0';

    // Current study method
    printf("\n  " ANSI_BOLD "Come studi di solito?" ANSI_RESET "\n");
    printf("  (Scrivi liberamente, premi Invio quando hai finito)\n");
    read_string("", state->study_method, sizeof(state->study_method));

    // Challenges - just for UX, not stored
    printf("\n  " ANSI_BOLD "Cosa trovi più difficile nello studio?" ANSI_RESET "\n");
    print_option(1, "Concentrazione", "Mi distraggo facilmente");
    print_option(2, "Memoria", "Faccio fatica a ricordare");
    print_option(3, "Comprensione", "Non capisco al primo colpo");
    print_option(4, "Organizzazione", "Non so da dove iniziare");
    print_option(5, "Motivazione", "Non ho voglia di studiare");
    print_option(6, "Nessuna in particolare", "Studio abbastanza bene");
    read_int_choice(1, 6);

    printf("\n");
    print_success("Profilo di studio acquisito!");
    return true;
}

/**
 * S07: Step 6 - Obiettivi personali
 */
static bool wizard_step6_goals(WizardState* state) {
    print_header("Obiettivi", 6, 6);

    printf("  Cosa vuoi ottenere con il tuo studio?\n");
    printf("  Puoi aggiungere fino a %d obiettivi.\n\n", MAX_GOALS);

    state->goals_count = 0;

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
                snprintf(state->goals[i], MAX_GOAL_LEN,
                         "Migliorare in %s", description);
                break;
            case 2:
                printf("  Che esame devi preparare?\n");
                read_string("", description, sizeof(description));
                snprintf(state->goals[i], MAX_GOAL_LEN,
                         "Preparare esame: %s", description);
                break;
            case 3:
                printf("  Quale debito devi recuperare?\n");
                read_string("", description, sizeof(description));
                snprintf(state->goals[i], MAX_GOAL_LEN,
                         "Recuperare debito in %s", description);
                break;
            case 4:
                printf("  Cosa vuoi approfondire?\n");
                read_string("", description, sizeof(description));
                snprintf(state->goals[i], MAX_GOAL_LEN,
                         "Approfondire: %s", description);
                break;
            case 5:
                printf("  Descrivi il tuo obiettivo:\n");
                read_string("", state->goals[i], MAX_GOAL_LEN);
                break;
        }

        state->goals_count++;
        print_success("Obiettivo aggiunto!");
        printf("\n");
    }

    if (state->goals_count == 0) {
        strncpy(state->goals[0], "Studiare e imparare", MAX_GOAL_LEN - 1);
        state->goals[0][MAX_GOAL_LEN - 1] = '\0';
        state->goals_count = 1;
    }

    print_success("Obiettivi registrati!");
    return true;
}

/**
 * S08: Show summary and confirm
 */
static bool wizard_show_summary(WizardState* state) {
    print_header("Riepilogo Profilo", 0, 0);

    printf("  Ecco il tuo profilo completo:\n\n");

    printf("  " ANSI_BOLD "👤 Nome:" ANSI_RESET " %s", state->name);
    if (state->age > 0) {
        printf(" (%d anni)", state->age);
    }
    printf("\n");

    printf("  " ANSI_BOLD "📚 Curriculum:" ANSI_RESET " %s (Anno %d)\n",
           state->curriculum_id, state->grade_level);

    printf("  " ANSI_BOLD "🎯 Stile apprendimento:" ANSI_RESET " %s\n", state->learning_style);

    printf("  " ANSI_BOLD "⏱️ Sessioni:" ANSI_RESET " %d min studio, %d min pausa\n",
           state->session_duration, state->break_duration);

    // Accessibility summary
    printf("  " ANSI_BOLD "♿ Accessibilità:" ANSI_RESET " ");
    bool has_any = false;
    if (state->accessibility.dyslexia) { printf("Dislessia "); has_any = true; }
    if (state->accessibility.dyscalculia) { printf("Discalculia "); has_any = true; }
    if (state->accessibility.adhd) { printf("ADHD "); has_any = true; }
    if (state->accessibility.autism) { printf("Autismo "); has_any = true; }
    if (state->accessibility.cerebral_palsy) { printf("Paralisi Cerebrale "); has_any = true; }
    if (!has_any) { printf("Nessuna esigenza speciale"); }
    printf("\n");

    // Goals
    printf("  " ANSI_BOLD "🎯 Obiettivi:" ANSI_RESET "\n");
    for (int i = 0; i < state->goals_count; i++) {
        printf("     • %s\n", state->goals[i]);
    }

    printf("\n");
    return read_yes_no("Confermi questi dati?", true);
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

    print_info("Profilo condiviso con tutti i 14 maestri.");
    print_info("Ogni maestro adatterà il suo stile alle tue esigenze.");

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
            print_warning("Modifica profilo non ancora implementata. Procedo con nuovo profilo.");
        }
        // choice == 2 or fallthrough: create new
    }

    // Create wizard state
    WizardState state = {0};
    state.session_duration = 25;
    state.break_duration = 5;
    state.accessibility.tts_speed = 1.0f;

    clear_screen();
    print_header("Benvenuto!", 0, 0);
    printf("  Ciao! Sono qui per configurare il tuo profilo studente.\n");
    printf("  Ti farò alcune domande per conoscerti meglio e aiutarti\n");
    printf("  nel modo migliore possibile.\n\n");
    printf("  Premi Invio per iniziare...\n");
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
        print_warning("Setup annullato. Riprova con /education setup");
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
    print_success("Profilo salvato con successo!");
    printf("\n");
    printf("  " ANSI_GREEN "🎉 Benvenuto, %s!" ANSI_RESET "\n", state.name);
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
