/**
 * CONVERGIO EDUCATION - ALI ONBOARDING (EDU-01, EDU-02)
 *
 * TRUE CONVERSATIONAL AI ONBOARDING
 *
 * This is NOT a form wizard with colors. This is a real AI conversation
 * where Ali uses LLM to understand the student naturally and extract
 * structured profile data from the conversation context.
 *
 * How it works:
 * 1. Ali starts a natural conversation
 * 2. Student responds freely - can digress, ask questions, correct themselves
 * 3. Ali uses LLM to understand and guide the conversation
 * 4. After enough info gathered, LLM extracts structured JSON profile
 * 5. Student can make corrections in natural language
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/education.h"
#include "nous/nous.h"
#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <cjson/cJSON.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_INPUT_LENGTH 1024
#define MAX_CONVERSATION_LENGTH 32768
#define MAX_TURNS 20

// ANSI colors
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_DIM     "\033[2m"
#define ANSI_ALI     "\033[1;38;5;135m"

// ============================================================================
// ALI'S SYSTEM PROMPT FOR ONBOARDING
// ============================================================================

static const char* ALI_ONBOARDING_SYSTEM =
    "Sei Ali, il Preside di Convergio Education, una scuola virtuale con 15 maestri storici.\n\n"

    "STAI FACENDO L'ONBOARDING DI UN NUOVO STUDENTE.\n\n"

    "Il tuo obiettivo è conoscere lo studente attraverso una conversazione NATURALE e AMICHEVOLE.\n"
    "NON fare un interrogatorio! Parla come un preside gentile che accoglie un nuovo studente.\n\n"

    "INFORMAZIONI DA RACCOGLIERE (in modo naturale, non come checklist):\n"
    "- Nome (e soprannome se lo usa)\n"
    "- Età e classe/scuola che frequenta\n"
    "- Materie preferite e materie dove fa fatica\n"
    "- Come preferisce imparare (video, lettura, pratica, ascolto)\n"
    "- Obiettivi o cosa vuole migliorare\n"
    "- Eventuali bisogni speciali (dislessia, ADHD, etc) - chiedi con delicatezza\n\n"

    "REGOLE:\n"
    "- Rispondi SEMPRE in italiano\n"
    "- Sii caloroso, incoraggiante, mai giudicante\n"
    "- Fai UNA domanda alla volta, non elenchi\n"
    "- Se lo studente divaga, va bene! Segui la conversazione, poi riporta delicatamente sul tema\n"
    "- Se lo studente non vuole rispondere a qualcosa, rispetta la sua scelta\n"
    "- Usa un linguaggio adatto all'età (semplice per i piccoli, più maturo per i grandi)\n"
    "- Quando hai raccolto abbastanza informazioni, dì che sei pronto a creare il profilo\n\n"

    "IMPORTANTE: Non usare formattazioni markdown, emoji eccessivi o liste. "
    "Parla come parleresti a voce.";

// ============================================================================
// EXTRACTION PROMPT FOR JSON OUTPUT
// ============================================================================

static const char* EXTRACTION_SYSTEM =
    "Sei un assistente che estrae dati strutturati da una conversazione.\n\n"

    "Data la conversazione tra Ali (preside) e uno studente, estrai le seguenti informazioni "
    "e restituiscile SOLO come JSON valido, senza testo aggiuntivo:\n\n"

    "{\n"
    "  \"name\": \"nome dello studente (stringa)\",\n"
    "  \"nickname\": \"soprannome se menzionato, altrimenti null\",\n"
    "  \"age\": numero età (0 se non specificato),\n"
    "  \"grade\": \"classe/anno scolastico (stringa)\",\n"
    "  \"school_type\": \"tipo di scuola: elementari|medie|liceo|tecnico|altro\",\n"
    "  \"favorite_subjects\": [\"lista\", \"materie\", \"preferite\"],\n"
    "  \"difficult_subjects\": [\"lista\", \"materie\", \"difficili\"],\n"
    "  \"learning_style\": \"visual|auditory|kinesthetic|reading|mixed\",\n"
    "  \"goals\": \"obiettivi dello studente in una frase\",\n"
    "  \"accessibility\": {\n"
    "    \"dyslexia\": true/false,\n"
    "    \"dyscalculia\": true/false,\n"
    "    \"adhd\": true/false,\n"
    "    \"autism\": true/false,\n"
    "    \"visual_impairment\": true/false,\n"
    "    \"hearing_impairment\": true/false,\n"
    "    \"other\": \"altre note di accessibilità\"\n"
    "  },\n"
    "  \"confidence\": numero da 0 a 1 che indica quanto sei sicuro dei dati\n"
    "}\n\n"

    "REGOLE:\n"
    "- Restituisci SOLO il JSON, niente altro testo prima o dopo\n"
    "- Se un'informazione non è stata menzionata, usa null, stringa vuota, o false\n"
    "- Interpreta il linguaggio naturale (es. 'faccio terza media' -> grade: '3 media')\n"
    "- Per accessibility, cerca menzioni di DSA, BES, dislessia, difficoltà di attenzione, etc.\n"
    "- Il campo confidence indica quanto sei sicuro dei dati estratti\n";

// ============================================================================
// CONVERSATION STATE
// ============================================================================

typedef struct {
    char history[MAX_CONVERSATION_LENGTH];
    int turn_count;
    bool has_name;
    bool has_age;
    bool has_school;
    bool has_preferences;
    bool ready_to_extract;
} ConversationState;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static void print_ali(const char* message) {
    printf("\n  " ANSI_ALI "Ali:" ANSI_RESET " %s\n", message);
}

static void print_prompt(void) {
    printf("\n  " ANSI_DIM "Tu:" ANSI_RESET " ");
    fflush(stdout);
}

static void append_to_history(ConversationState* state, const char* role, const char* message) {
    size_t current_len = strlen(state->history);
    size_t remaining = MAX_CONVERSATION_LENGTH - current_len - 1;

    if (remaining < 100) return; // Not enough space

    char entry[2048];
    snprintf(entry, sizeof(entry), "[%s]: %s\n", role, message);

    strncat(state->history, entry, remaining);
}

static bool check_info_gathered(ConversationState* state, const char* response) {
    // Check for keywords that indicate enough info
    const char* lower = response;

    if (strstr(lower, "nome") || strstr(lower, "chiami")) state->has_name = true;
    if (strstr(lower, "anni") || strstr(lower, "età")) state->has_age = true;
    if (strstr(lower, "scuola") || strstr(lower, "classe") || strstr(lower, "media") ||
        strstr(lower, "liceo") || strstr(lower, "elementare")) state->has_school = true;
    if (strstr(lower, "piace") || strstr(lower, "preferisci") || strstr(lower, "impari"))
        state->has_preferences = true;

    // Ready after 4+ turns with basic info
    return (state->turn_count >= 4 && state->has_name && state->has_age);
}

// ============================================================================
// API KEY CHECK
// ============================================================================

static bool check_api_configured(void) {
    // Education Edition uses Azure OpenAI, so check for Azure credentials first
    const char* azure_key = getenv("AZURE_OPENAI_API_KEY");
    const char* azure_endpoint = getenv("AZURE_OPENAI_ENDPOINT");
    if (azure_key && azure_endpoint && strlen(azure_key) > 10 && strlen(azure_endpoint) > 10) {
        return true;
    }

    // Fallback to Anthropic for development/testing
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    return (api_key && strlen(api_key) > 10);
}

bool ali_check_api_setup(void) {
    if (check_api_configured()) {
        return true;
    }

    printf("\n");
    printf("  " ANSI_CYAN "┌─────────────────────────────────────────────────────────────┐" ANSI_RESET "\n");
    printf("  " ANSI_CYAN "│" ANSI_RESET "  " ANSI_BOLD "Convergio Education - Configurazione" ANSI_RESET "                       " ANSI_CYAN "│" ANSI_RESET "\n");
    printf("  " ANSI_CYAN "└─────────────────────────────────────────────────────────────┘" ANSI_RESET "\n");
    printf("\n");

    printf("  " ANSI_YELLOW "Per iniziare serve una configurazione Azure OpenAI." ANSI_RESET "\n\n");
    printf("  1. Vai su " ANSI_CYAN "portal.azure.com" ANSI_RESET "\n");
    printf("  2. Crea una risorsa Azure OpenAI\n");
    printf("  3. Aggiungi al ~/.zshrc:\n");
    printf("     " ANSI_YELLOW "export AZURE_OPENAI_API_KEY=\"your-key\"" ANSI_RESET "\n");
    printf("     " ANSI_YELLOW "export AZURE_OPENAI_ENDPOINT=\"https://...\"" ANSI_RESET "\n");
    printf("  4. Esegui: source ~/.zshrc\n\n");

    printf("  Premi Invio dopo aver configurato, o 'esci' per uscire: ");
    fflush(stdout);

    char input[64];
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strstr(input, "esci") || strstr(input, "exit")) {
            return false;
        }
    }

    if (check_api_configured()) {
        printf("\n  " ANSI_GREEN "✓ Chiave API configurata!" ANSI_RESET "\n");
        return true;
    }

    printf("\n  " ANSI_YELLOW "Chiave non trovata. Riavvia dopo la configurazione." ANSI_RESET "\n");
    return false;
}

// ============================================================================
// PROFILE EXTRACTION FROM JSON
// ============================================================================

static int64_t create_profile_from_json(const char* json_str) {
    cJSON* root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "  " ANSI_YELLOW "Errore parsing JSON profilo" ANSI_RESET "\n");
        return -1;
    }

    // Extract fields
    cJSON* name_obj = cJSON_GetObjectItem(root, "name");
    cJSON* age_obj = cJSON_GetObjectItem(root, "age");
    cJSON* grade_obj = cJSON_GetObjectItem(root, "grade");
    cJSON* school_type_obj = cJSON_GetObjectItem(root, "school_type");
    cJSON* goals_obj = cJSON_GetObjectItem(root, "goals");
    cJSON* accessibility_obj = cJSON_GetObjectItem(root, "accessibility");

    const char* name = (name_obj && cJSON_IsString(name_obj)) ? name_obj->valuestring : "Studente";
    int age = (age_obj && cJSON_IsNumber(age_obj)) ? age_obj->valueint : 12;
    const char* grade = (grade_obj && cJSON_IsString(grade_obj)) ? grade_obj->valuestring : "";
    const char* school_type = (school_type_obj && cJSON_IsString(school_type_obj)) ? school_type_obj->valuestring : "medie";
    const char* goals = (goals_obj && cJSON_IsString(goals_obj)) ? goals_obj->valuestring : "";

    // Determine grade level from school type
    int grade_level = 6; // default middle school
    if (school_type) {
        if (strstr(school_type, "element")) grade_level = 3;
        else if (strstr(school_type, "medie") || strstr(school_type, "media")) grade_level = 7;
        else if (strstr(school_type, "liceo") || strstr(school_type, "tecnico")) grade_level = 10;
    }

    // Build accessibility
    EducationAccessibility access = {0};
    access.tts_speed = 1.0f;

    if (accessibility_obj && cJSON_IsObject(accessibility_obj)) {
        cJSON* dys = cJSON_GetObjectItem(accessibility_obj, "dyslexia");
        cJSON* disc = cJSON_GetObjectItem(accessibility_obj, "dyscalculia");
        cJSON* adhd = cJSON_GetObjectItem(accessibility_obj, "adhd");
        cJSON* autism = cJSON_GetObjectItem(accessibility_obj, "autism");
        cJSON* visual = cJSON_GetObjectItem(accessibility_obj, "visual_impairment");
        cJSON* hearing = cJSON_GetObjectItem(accessibility_obj, "hearing_impairment");

        if (dys && cJSON_IsBool(dys)) access.dyslexia = cJSON_IsTrue(dys);
        if (disc && cJSON_IsBool(disc)) access.dyscalculia = cJSON_IsTrue(disc);
        if (adhd && cJSON_IsBool(adhd)) access.adhd = cJSON_IsTrue(adhd);
        if (autism && cJSON_IsBool(autism)) access.autism = cJSON_IsTrue(autism);
        if (visual && cJSON_IsBool(visual)) access.visual_impairment = cJSON_IsTrue(visual);
        if (hearing && cJSON_IsBool(hearing)) access.hearing_impairment = cJSON_IsTrue(hearing);
    }

    // Create profile
    EducationCreateOptions options = {0};
    options.name = name;
    options.age = age;
    options.grade_level = grade_level;
    options.curriculum_id = school_type;
    options.accessibility = &access;

    int64_t profile_id = education_profile_create(&options);

    if (profile_id > 0 && goals && strlen(goals) > 0) {
        education_goal_add(profile_id, GOAL_MEDIUM_TERM, goals, 0);
    }

    cJSON_Delete(root);
    return profile_id;
}

// ============================================================================
// MAIN CONVERSATIONAL ONBOARDING
// ============================================================================

bool ali_conversational_onboarding(void) {
    ConversationState state = {0};
    char input[MAX_INPUT_LENGTH];
    char ali_response[4096];

    // Initialize education system
    if (education_init() != 0) {
        fprintf(stderr, "  " ANSI_YELLOW "Errore inizializzazione sistema educativo" ANSI_RESET "\n");
        return false;
    }

    // Check if LLM is available
    if (!llm_is_available()) {
        fprintf(stderr, "  " ANSI_YELLOW "LLM non disponibile. Verifica la chiave API." ANSI_RESET "\n");
        return false;
    }

    // Clear screen and welcome
    printf("\033[2J\033[H");
    printf("\n");
    printf("  " ANSI_ALI "┌─────────────────────────────────────────────────────────────┐" ANSI_RESET "\n");
    printf("  " ANSI_ALI "│" ANSI_RESET "            " ANSI_BOLD "Convergio Education" ANSI_RESET "                               " ANSI_ALI "│" ANSI_RESET "\n");
    printf("  " ANSI_ALI "│" ANSI_RESET "  " ANSI_DIM "Benvenuto! Ali, il Preside, ti accoglierà personalmente." ANSI_RESET "   " ANSI_ALI "│" ANSI_RESET "\n");
    printf("  " ANSI_ALI "│" ANSI_RESET "  " ANSI_DIM "Digita 'esci' in qualsiasi momento per interrompere." ANSI_RESET "       " ANSI_ALI "│" ANSI_RESET "\n");
    printf("  " ANSI_ALI "└─────────────────────────────────────────────────────────────┘" ANSI_RESET "\n");

    // Start conversation with Ali's greeting
    const char* greeting = "Ciao! Sono Ali, il Preside di questa scuola virtuale. "
                          "È un piacere conoscerti! Come ti chiami?";
    print_ali(greeting);
    append_to_history(&state, "Ali", greeting);

    // Main conversation loop
    while (state.turn_count < MAX_TURNS) {
        print_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = 0;

        // Trim whitespace
        char* start = input;
        while (*start && isspace(*start)) start++;
        if (start != input) memmove(input, start, strlen(start) + 1);

        // Check for exit
        if (strlen(input) == 0) continue;
        if (strcasecmp(input, "esci") == 0 || strcasecmp(input, "exit") == 0) {
            print_ali("Va bene, ci vediamo la prossima volta! Torna quando vuoi.");
            return false;
        }

        // Add student response to history
        append_to_history(&state, "Studente", input);
        state.turn_count++;

        // Check if we have enough info
        check_info_gathered(&state, state.history);

        // Build prompt for Ali
        char prompt[MAX_CONVERSATION_LENGTH + 1024];
        snprintf(prompt, sizeof(prompt),
            "CONVERSAZIONE FINORA:\n%s\n\n"
            "TURNO: %d/%d\n"
            "INFO RACCOLTE: nome=%s, età=%s, scuola=%s, preferenze=%s\n\n"
            "Rispondi allo studente. %s",
            state.history,
            state.turn_count, MAX_TURNS,
            state.has_name ? "sì" : "no",
            state.has_age ? "sì" : "no",
            state.has_school ? "sì" : "no",
            state.has_preferences ? "sì" : "no",
            (state.turn_count >= 5 && state.has_name && state.has_age)
                ? "Se hai abbastanza informazioni, proponi di creare il profilo."
                : "Continua a conoscere lo studente in modo naturale.");

        // Get Ali's response from LLM
        TokenUsage usage = {0};
        char* response = llm_chat(ALI_ONBOARDING_SYSTEM, prompt, &usage);

        if (!response || strlen(response) == 0) {
            print_ali("Scusa, non ho capito. Puoi ripetere?");
            if (response) free(response);
            continue;
        }

        // Print Ali's response
        strncpy(ali_response, response, sizeof(ali_response) - 1);
        ali_response[sizeof(ali_response) - 1] = '\0';
        free(response);

        print_ali(ali_response);
        append_to_history(&state, "Ali", ali_response);

        // Check if Ali proposed to create profile
        if (strstr(ali_response, "profilo") &&
            (strstr(ali_response, "creare") || strstr(ali_response, "pronto") ||
             strstr(ali_response, "salvare") || strstr(ali_response, "confermi"))) {
            state.ready_to_extract = true;
        }

        // If ready to extract, do it
        if (state.ready_to_extract && state.turn_count >= 4) {
            break;
        }
    }

    // ========================================
    // EXTRACTION PHASE: Get structured JSON
    // ========================================

    printf("\n  " ANSI_DIM "Elaborazione del profilo..." ANSI_RESET "\n");

    char extraction_prompt[MAX_CONVERSATION_LENGTH + 512];
    snprintf(extraction_prompt, sizeof(extraction_prompt),
        "Estrai i dati dal seguente dialogo:\n\n%s", state.history);

    TokenUsage extract_usage = {0};
    char* json_response = llm_chat(EXTRACTION_SYSTEM, extraction_prompt, &extract_usage);

    if (!json_response) {
        print_ali("Ops, c'è stato un problema nel creare il profilo. Riproviamo?");
        return false;
    }

    // Find JSON in response (may have extra text)
    char* json_start = strchr(json_response, '{');
    char* json_end = strrchr(json_response, '}');

    if (!json_start || !json_end || json_end <= json_start) {
        print_ali("Ops, non sono riuscito a estrarre le informazioni. Riproviamo?");
        free(json_response);
        return false;
    }

    // Isolate JSON
    *(json_end + 1) = '\0';

    // Create profile from JSON
    int64_t profile_id = create_profile_from_json(json_start);
    free(json_response);

    if (profile_id < 0) {
        print_ali("C'è stato un errore nel salvare il profilo. Mi dispiace!");
        return false;
    }

    // Activate profile
    education_profile_set_active(profile_id);

    // Broadcast to maestri
    education_maestro_broadcast_profile(profile_id);

    // Get the profile to show summary
    EducationStudentProfile* profile = education_profile_get_active();

    // ========================================
    // CONFIRMATION PHASE
    // ========================================

    printf("\n");
    printf("  " ANSI_GREEN "┌─────────────────────────────────────────────────────────────┐" ANSI_RESET "\n");
    printf("  " ANSI_GREEN "│" ANSI_RESET "  " ANSI_BOLD "Profilo Creato!" ANSI_RESET "                                             " ANSI_GREEN "│" ANSI_RESET "\n");
    printf("  " ANSI_GREEN "└─────────────────────────────────────────────────────────────┘" ANSI_RESET "\n");

    if (profile) {
        printf("\n     " ANSI_BOLD "Nome:" ANSI_RESET " %s\n", profile->name);
        printf("     " ANSI_BOLD "Età:" ANSI_RESET " %d anni\n", profile->age);
        printf("     " ANSI_BOLD "Anno scolastico:" ANSI_RESET " %d\n", profile->grade_level);
    }

    print_ali("Ecco il tuo profilo! Ho già informato tutti i 15 maestri di te.");
    print_ali("Vuoi modificare qualcosa? Dimmelo, oppure scrivi 'ok' per continuare.");

    print_prompt();
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;

        // If not just "ok", let them make changes
        if (strlen(input) > 0 && strcasecmp(input, "ok") != 0 && strcasecmp(input, "sì") != 0) {
            // For now, just acknowledge - full edit would need another LLM call
            print_ali("Ho capito! Per ora salvo così, potrai modificare il profilo "
                     "in qualsiasi momento con /education profile edit.");
        }
    }

    // ========================================
    // WELCOME COMPLETE
    // ========================================

    printf("\n");
    print_ali("Perfetto! Sei pronto per iniziare a imparare con i nostri maestri:");
    printf("\n");
    printf("     " ANSI_CYAN "Socrate" ANSI_RESET " (Filosofia)    " ANSI_CYAN "Euclide" ANSI_RESET " (Matematica)\n");
    printf("     " ANSI_CYAN "Feynman" ANSI_RESET " (Fisica)       " ANSI_CYAN "Darwin" ANSI_RESET " (Scienze)\n");
    printf("     " ANSI_CYAN "Manzoni" ANSI_RESET " (Italiano)     " ANSI_CYAN "Shakespeare" ANSI_RESET " (Inglese)\n");
    printf("     " ANSI_CYAN "Erodoto" ANSI_RESET " (Storia)       " ANSI_CYAN "Leonardo" ANSI_RESET " (Arte)\n");
    printf("     " ANSI_CYAN "Mozart" ANSI_RESET " (Musica)        " ANSI_CYAN "Lovelace" ANSI_RESET " (Informatica)\n");
    printf("     " ANSI_CYAN "Humboldt" ANSI_RESET " (Geografia)   " ANSI_CYAN "Smith" ANSI_RESET " (Economia)\n");
    printf("     " ANSI_CYAN "Cicerone" ANSI_RESET " (Ed. Civica)  " ANSI_CYAN "Ippocrate" ANSI_RESET " (Salute)\n");
    printf("     " ANSI_CYAN "Chris" ANSI_RESET " (Storytelling)\n");

    printf("\n");
    print_ali("Cosa vorresti fare? Ecco alcuni comandi utili:");
    printf("\n");
    printf("     " ANSI_YELLOW "/study <materia>" ANSI_RESET " - Inizia una sessione di studio\n");
    printf("     " ANSI_YELLOW "/homework <compito>" ANSI_RESET " - Aiuto con i compiti\n");
    printf("     " ANSI_YELLOW "/quiz <argomento>" ANSI_RESET " - Fai un quiz\n");
    printf("     " ANSI_YELLOW "/mindmap <concetto>" ANSI_RESET " - Crea una mappa mentale\n");
    printf("     " ANSI_YELLOW "/libretto" ANSI_RESET " - Guarda i tuoi progressi\n");
    printf("\n");
    print_ali("In bocca al lupo per i tuoi studi!\n");

    return true;
}

// ============================================================================
// ENTRY POINTS
// ============================================================================

/**
 * Main entry point for Education onboarding
 */
bool ali_education_onboarding(void) {
    // Phase 1: Check API setup
    if (!ali_check_api_setup()) {
        return false;
    }

    // Phase 2: Conversational onboarding
    return ali_conversational_onboarding();
}

/**
 * Check if student needs onboarding
 */
bool ali_needs_onboarding(void) {
    if (education_init() != 0) {
        return true;
    }

    EducationStudentProfile* profile = education_profile_get_active();
    return (profile == NULL);
}
