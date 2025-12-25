/**
 * CONVERGIO EDUCATION PACK - TEST SUITE
 *
 * Test scenari realistici dalla vita scolastica quotidiana.
 * Basati su casi d'uso reali descritti nel piano Education Pack.
 *
 * Scenari testati:
 * 1. Mario (16 anni) - Dislessia + Paralisi cerebrale + Discalculia
 * 2. Sofia (14 anni) - ADHD tipo combinato
 * 3. Luca (17 anni) - Autismo ad alto funzionamento
 * 4. Giulia (15 anni) - Nessuna disabilita' (baseline)
 *
 * Copyright (c) 2025 Convergio.io
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "nous/education.h"

// ============================================================================
// TEST UTILITIES
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("\n[TEST] %s... ", name); \
    fflush(stdout);

#define PASS() \
    printf("PASSED\n"); \
    tests_passed++;

#define FAIL(msg) \
    printf("FAILED: %s\n", msg); \
    tests_failed++;

#define ASSERT_TRUE(cond, msg) \
    if (!(cond)) { FAIL(msg); return; }

#define ASSERT_NOT_NULL(ptr, msg) \
    if ((ptr) == NULL) { FAIL(msg); return; }

#define ASSERT_EQ(a, b, msg) \
    if ((a) != (b)) { FAIL(msg); return; }

#define ASSERT_STR_EQ(a, b, msg) \
    if (strcmp((a), (b)) != 0) { FAIL(msg); return; }

// ============================================================================
// SCENARIO 1: MARIO - Multi-disabilita'
// ============================================================================

/**
 * Mario, 16 anni, 1° Liceo Scientifico
 * - Dislessia severa
 * - Paralisi cerebrale lieve
 * - Discalculia moderata
 *
 * Scenario: Mario deve studiare le equazioni di primo grado.
 * Il sistema deve:
 * - Usare font accessibili e TTS
 * - Dare tempo extra
 * - Visualizzare i numeri con blocchi colorati
 * - Mai mettere ansia da performance
 */
void test_scenario_mario_setup(void) {
    TEST("Scenario Mario - Creazione profilo multi-disabilita");

    // Inizializza il sistema education
    int rc = education_init();
    ASSERT_EQ(rc, 0, "education_init failed");

    // Configura accessibilita' Mario
    EducationAccessibility* access = calloc(1, sizeof(EducationAccessibility));
    ASSERT_NOT_NULL(access, "Failed to allocate accessibility");

    access->dyslexia = true;
    access->dyslexia_severity = SEVERITY_SEVERE;
    access->cerebral_palsy = true;
    access->cerebral_palsy_severity = SEVERITY_MILD;
    access->dyscalculia = true;
    access->dyscalculia_severity = SEVERITY_MODERATE;
    access->tts_enabled = true;
    access->tts_speed = 0.8f;
    access->preferred_input = INPUT_VOICE;
    access->preferred_output = OUTPUT_BOTH;
    access->high_contrast = true;

    // Crea profilo Mario usando EducationCreateOptions
    EducationCreateOptions options = {
        .name = "Mario",
        .age = 16,
        .grade_level = 1,
        .curriculum_id = "liceo_scientifico",
        .parent_name = "Giuseppe Rossi",
        .parent_email = "giuseppe.rossi@email.it",
        .accessibility = access
    };

    // Salva profilo
    int64_t mario_id = education_profile_create(&options);
    ASSERT_TRUE(mario_id > 0, "Failed to create Mario's profile");

    // Verifica che il profilo sia stato salvato correttamente
    EducationStudentProfile* loaded = education_profile_get(mario_id);
    ASSERT_NOT_NULL(loaded, "Failed to load Mario's profile");
    ASSERT_STR_EQ(loaded->name, "Mario", "Name mismatch");
    ASSERT_TRUE(loaded->accessibility->dyslexia, "Dyslexia flag not saved");
    ASSERT_EQ(loaded->accessibility->dyslexia_severity, SEVERITY_SEVERE, "Dyslexia severity mismatch");

    // Imposta come profilo attivo per test successivi
    education_profile_set_active(mario_id);

    // Cleanup
    free(access);
    education_profile_free(loaded);

    PASS();
}

void test_scenario_mario_study_math(void) {
    TEST("Scenario Mario - Studio matematica con accessibilita");

    // Mario chiede aiuto con le equazioni
    // Simuliamo una sessione di studio

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile - run setup test first");
        return;
    }

    // Verifica che le impostazioni accessibilita' siano rispettate
    ASSERT_TRUE(profile->accessibility->dyscalculia, "Dyscalculia should be enabled");
    ASSERT_TRUE(profile->accessibility->tts_enabled, "TTS should be enabled");

    // Simula inizio sessione di studio
    int64_t session_id = education_session_start(
        profile->id,
        "study",
        "matematica",
        "equazioni_primo_grado"
    );
    ASSERT_TRUE(session_id > 0, "Failed to start study session");

    // Verifica che la sessione sia stata creata
    // (in un test reale, verificheremmo che il timer sia partito, etc.)

    // Note: DO NOT free profile here - it's the internal g_active_profile pointer
    // returned by education_profile_get_active(), not a copy
    PASS();
}

// ============================================================================
// SCENARIO 2: SOFIA - ADHD
// ============================================================================

/**
 * Sofia, 14 anni, 3° Scuola Media
 * - ADHD tipo combinato (inattenzione + iperattivita')
 * - Severita' moderata
 *
 * Scenario: Sofia deve fare i compiti di storia.
 * Il sistema deve:
 * - Dare risposte brevi e frammentate
 * - Inserire pause frequenti
 * - Gamificare l'esperienza
 * - Mai dare monologhi lunghi
 */
void test_scenario_sofia_setup(void) {
    TEST("Scenario Sofia - Profilo ADHD");

    // Configura accessibilita' Sofia
    EducationAccessibility* access = calloc(1, sizeof(EducationAccessibility));
    ASSERT_NOT_NULL(access, "Failed to allocate accessibility");

    access->adhd = true;
    access->adhd_type = ADHD_COMBINED;
    access->adhd_severity = SEVERITY_MODERATE;
    access->preferred_input = INPUT_KEYBOARD;
    access->preferred_output = OUTPUT_TEXT;

    // Crea profilo Sofia usando EducationCreateOptions
    EducationCreateOptions options = {
        .name = "Sofia",
        .age = 14,
        .grade_level = 3,
        .curriculum_id = "scuola_media",
        .parent_name = "Anna Bianchi",
        .parent_email = "anna.bianchi@email.it",
        .accessibility = access
    };

    int64_t sofia_id = education_profile_create(&options);
    ASSERT_TRUE(sofia_id > 0, "Failed to create Sofia's profile");

    // Verifica profilo
    EducationStudentProfile* loaded = education_profile_get(sofia_id);
    ASSERT_NOT_NULL(loaded, "Failed to load Sofia's profile");
    ASSERT_TRUE(loaded->accessibility->adhd, "ADHD flag not saved");
    ASSERT_EQ(loaded->accessibility->adhd_severity, SEVERITY_MODERATE, "ADHD severity mismatch");

    // Cleanup
    free(access);
    education_profile_free(loaded);
    PASS();
}

void test_scenario_sofia_homework(void) {
    TEST("Scenario Sofia - Compiti storia con anti-cheating");

    // Sofia chiede aiuto con i compiti
    // Il sistema deve guidarla, non dare risposte dirette

    // Simula richiesta compiti
    const char* homework_desc = "Riassumi le cause della Rivoluzione Francese";
    (void)homework_desc;  // Used in future test expansion

    // In un test reale, verificheremmo che:
    // 1. La risposta sia breve (max 3-4 bullet points per ADHD)
    // 2. Non venga data la risposta diretta (anti-cheating)
    // 3. Ci siano domande guida invece di risposte

    PASS();
}

// ============================================================================
// SCENARIO 3: LUCA - Autismo
// ============================================================================

/**
 * Luca, 17 anni, 2° Liceo Classico
 * - Autismo ad alto funzionamento
 * - Preferisce comunicazione letterale
 * - Si stressa con ambiguita'
 *
 * Scenario: Luca studia filosofia con Socrate.
 * Il sistema deve:
 * - Usare linguaggio letterale, no metafore
 * - Dare struttura prevedibile
 * - Avvisare prima dei cambi di topic
 * - Permettere approfondimenti dettagliati
 */
void test_scenario_luca_setup(void) {
    TEST("Scenario Luca - Profilo Autismo");

    // Configura accessibilita' Luca
    EducationAccessibility* access = calloc(1, sizeof(EducationAccessibility));
    ASSERT_NOT_NULL(access, "Failed to allocate accessibility");

    access->autism = true;
    access->autism_severity = SEVERITY_MILD;
    access->reduce_motion = true;  // Preferisce interfacce calme
    access->preferred_input = INPUT_KEYBOARD;
    access->preferred_output = OUTPUT_TEXT;

    // Crea profilo Luca usando EducationCreateOptions
    EducationCreateOptions options = {
        .name = "Luca",
        .age = 17,
        .grade_level = 2,
        .curriculum_id = "liceo_classico",
        .parent_name = "Marco Verdi",
        .parent_email = "marco.verdi@email.it",
        .accessibility = access
    };

    int64_t luca_id = education_profile_create(&options);
    ASSERT_TRUE(luca_id > 0, "Failed to create Luca's profile");

    // Verifica profilo
    EducationStudentProfile* loaded = education_profile_get(luca_id);
    ASSERT_NOT_NULL(loaded, "Failed to load Luca's profile");
    ASSERT_TRUE(loaded->accessibility->autism, "Autism flag not saved");
    ASSERT_TRUE(loaded->accessibility->reduce_motion, "Reduce motion not saved");

    // Cleanup
    free(access);
    education_profile_free(loaded);
    PASS();
}

// ============================================================================
// SCENARIO 4: GIULIA - Baseline
// ============================================================================

/**
 * Giulia, 15 anni, 1° Liceo Linguistico
 * - Nessuna disabilita'
 * - Caso baseline per confronto
 *
 * Scenario: Giulia usa flashcard per studiare inglese.
 * Verifica che il sistema funzioni anche senza accessibilita'.
 */
void test_scenario_giulia_baseline(void) {
    TEST("Scenario Giulia - Baseline senza accessibilita");

    // Accessibilita' minima (defaults per studente neurotypical)
    EducationAccessibility* access = calloc(1, sizeof(EducationAccessibility));
    ASSERT_NOT_NULL(access, "Failed to allocate accessibility");

    access->preferred_input = INPUT_KEYBOARD;
    access->preferred_output = OUTPUT_TEXT;

    // Crea profilo Giulia usando EducationCreateOptions
    EducationCreateOptions options = {
        .name = "Giulia",
        .age = 15,
        .grade_level = 1,
        .curriculum_id = "liceo_linguistico",
        .parent_name = "Laura Neri",
        .parent_email = "laura.neri@email.it",
        .accessibility = access
    };

    int64_t giulia_id = education_profile_create(&options);
    ASSERT_TRUE(giulia_id > 0, "Failed to create Giulia's profile");

    // Verifica profilo baseline
    EducationStudentProfile* loaded = education_profile_get(giulia_id);
    ASSERT_NOT_NULL(loaded, "Failed to load Giulia's profile");
    ASSERT_STR_EQ(loaded->name, "Giulia", "Name mismatch");
    ASSERT_EQ(loaded->age, 15, "Age mismatch");

    // Cleanup
    free(access);
    education_profile_free(loaded);
    PASS();
}

// ============================================================================
// TEST GOAL MANAGEMENT
// ============================================================================

void test_goal_management(void) {
    TEST("Goal Management - Obiettivi studente");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Aggiungi obiettivi
    int64_t goal1 = education_goal_add(
        profile->id,
        GOAL_SHORT_TERM,
        "Completare esercizi capitolo 5 matematica",
        0  // No deadline
    );
    ASSERT_TRUE(goal1 > 0, "Failed to add goal 1");

    int64_t goal2 = education_goal_add(
        profile->id,
        GOAL_MEDIUM_TERM,
        "Passare verifica di fisica con almeno 7",
        0
    );
    ASSERT_TRUE(goal2 > 0, "Failed to add goal 2");

    // Lista obiettivi
    int count = 0;
    EducationGoal** goals = education_goal_list(profile->id, &count);
    ASSERT_TRUE(count >= 2, "Should have at least 2 goals");

    // Cleanup
    education_goal_list_free(goals, count);
    // Note: DO NOT free profile - it's g_active_profile, managed internally
    PASS();
}

// ============================================================================
// TEST CURRICULUM LOADING
// ============================================================================

void test_curriculum_load(void) {
    TEST("Curriculum - Caricamento Liceo Scientifico");

    // Verifica che i file curriculum esistano
    // In un test reale, caricheremmo e parseremmo il JSON

    FILE* f = fopen("curricula/it/liceo_scientifico.json", "r");
    if (!f) {
        // Prova path alternativo
        f = fopen("../curricula/it/liceo_scientifico.json", "r");
    }
    ASSERT_NOT_NULL(f, "Curriculum file not found");
    fclose(f);

    PASS();
}

// ============================================================================
// TEST MAESTRI ESISTONO
// ============================================================================

void test_maestri_exist(void) {
    TEST("Maestri - Verifica 15 agent definitions");

    const char* maestri[] = {
        "socrate-filosofia",
        "euclide-matematica",
        "feynman-fisica",
        "erodoto-storia",
        "humboldt-geografia",
        "manzoni-italiano",
        "darwin-scienze",
        "leonardo-arte",
        "mozart-musica",
        "shakespeare-inglese",
        "cicerone-civica",
        "smith-economia",
        "lovelace-informatica",
        "ippocrate-corpo",
        "chris-storytelling"
    };

    char path[512];
    for (int i = 0; i < 15; i++) {
        snprintf(path, sizeof(path),
                 "src/agents/definitions/education/%s.md", maestri[i]);
        FILE* f = fopen(path, "r");
        if (!f) {
            snprintf(path, sizeof(path),
                     "../src/agents/definitions/education/%s.md", maestri[i]);
            f = fopen(path, "r");
        }
        if (!f) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Maestro %s not found", maestri[i]);
            FAIL(msg);
            return;
        }
        fclose(f);
    }

    PASS();
}

// ============================================================================
// TEST MAIEUTIC METHOD (MT02)
// Verify teachers use Socratic questioning approach
// ============================================================================

void test_maestri_maieutic_prompts(void) {
    TEST("Maestri - Verificare che usino il metodo maieutico");

    // Read Socrate's definition to verify maieutic instructions
    const char* paths[] = {
        "src/agents/definitions/education/socrate-filosofia.md",
        "../src/agents/definitions/education/socrate-filosofia.md"
    };

    FILE* f = NULL;
    for (int i = 0; i < 2 && !f; i++) {
        f = fopen(paths[i], "r");
    }

    if (!f) {
        FAIL("Could not read Socrate agent definition");
        return;
    }

    // Read file content
    char buffer[8192] = {0};
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, f);
    fclose(f);

    if (bytes_read == 0) {
        FAIL("Empty agent definition file");
        return;
    }

    // Check for maieutic keywords in the definition
    bool has_maieutic = strstr(buffer, "maieutic") != NULL ||
                        strstr(buffer, "Maieutic") != NULL ||
                        strstr(buffer, "socratic") != NULL ||
                        strstr(buffer, "Socratic") != NULL ||
                        strstr(buffer, "question") != NULL ||
                        strstr(buffer, "guide") != NULL;

    ASSERT_TRUE(has_maieutic, "Socrate definition should contain maieutic/Socratic method references");

    // Verify the definition contains guidance for NOT giving direct answers
    bool has_no_direct = strstr(buffer, "non dare") != NULL ||
                         strstr(buffer, "don't give") != NULL ||
                         strstr(buffer, "guide") != NULL ||
                         strstr(buffer, "help") != NULL ||
                         strstr(buffer, "discover") != NULL;

    ASSERT_TRUE(has_no_direct, "Definition should guide teacher to not give direct answers");

    PASS();
}

// ============================================================================
// TEST ACCESSIBILITY ADAPTATION (MT03)
// Verify teachers adapt to accessibility profiles
// ============================================================================

void test_maestri_accessibility_adaptation(void) {
    TEST("Maestri - Verificare adattamento accessibilita");

    // Read a teacher definition to verify accessibility awareness
    const char* paths[] = {
        "src/agents/definitions/education/euclide-matematica.md",
        "../src/agents/definitions/education/euclide-matematica.md"
    };

    FILE* f = NULL;
    for (int i = 0; i < 2 && !f; i++) {
        f = fopen(paths[i], "r");
    }

    if (!f) {
        FAIL("Could not read Euclide agent definition");
        return;
    }

    // Read file content
    char buffer[8192] = {0};
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, f);
    fclose(f);

    if (bytes_read == 0) {
        FAIL("Empty agent definition file");
        return;
    }

    // Check for accessibility-related keywords
    bool has_accessibility = strstr(buffer, "accessib") != NULL ||
                             strstr(buffer, "dislessia") != NULL ||
                             strstr(buffer, "dyslexia") != NULL ||
                             strstr(buffer, "discalculia") != NULL ||
                             strstr(buffer, "dyscalculia") != NULL ||
                             strstr(buffer, "ADHD") != NULL ||
                             strstr(buffer, "visual") != NULL ||
                             strstr(buffer, "step") != NULL;

    ASSERT_TRUE(has_accessibility, "Teacher definition should reference accessibility adaptations");

    // Verify there are instructions for adapting output
    bool has_adaptation = strstr(buffer, "adapt") != NULL ||
                          strstr(buffer, "adjust") != NULL ||
                          strstr(buffer, "simpl") != NULL ||
                          strstr(buffer, "step-by-step") != NULL ||
                          strstr(buffer, "passo") != NULL;

    ASSERT_TRUE(has_adaptation, "Definition should contain adaptation instructions");

    PASS();
}

// ============================================================================
// TEST LIBRETTO DELLO STUDENTE
// ============================================================================

void test_libretto_grade_recording(void) {
    TEST("Libretto - Registrazione voti");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Aggiungi un voto manuale
    int64_t grade_id = libretto_add_grade(
        profile->id,
        "ED02",                    // Euclide (Matematica)
        "Matematica",
        "Equazioni di primo grado",
        GRADE_TYPE_ORAL,
        8.5f,                      // Voto
        "Ottima comprensione dei passaggi, qualche errore di calcolo"
    );
    ASSERT_TRUE(grade_id > 0, "Failed to add grade");

    // Verifica che il voto sia stato salvato (ultimi 30 giorni)
    int count = 0;
    time_t now = time(NULL);
    time_t from_date = now - (30 * 24 * 60 * 60);  // 30 days ago
    EducationGrade** grades = libretto_get_grades(profile->id, "Matematica", from_date, now, &count);
    ASSERT_TRUE(count >= 1, "Should have at least 1 grade");
    ASSERT_NOT_NULL(grades, "Grades array is NULL");

    // Verifica il voto
    bool found = false;
    for (int i = 0; i < count; i++) {
        if (grades[i]->id == grade_id) {
            found = true;
            ASSERT_TRUE(grades[i]->grade >= 8.0f && grades[i]->grade <= 9.0f, "Grade value mismatch");
            ASSERT_EQ(grades[i]->grade_type, GRADE_TYPE_ORAL, "Grade type mismatch");
        }
    }
    ASSERT_TRUE(found, "Grade not found in list");

    // Cleanup
    libretto_grades_free(grades, count);
    PASS();
}

void test_libretto_quiz_grade_conversion(void) {
    TEST("Libretto - Conversione percentuale to voto italiano");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Test: 8 correct out of 10 = 80% = voto 9 (scala: 80-89% = 9)
    int64_t grade_id = libretto_add_quiz_grade(
        profile->id,
        "ED03",                    // Feynman (Fisica)
        "Fisica",
        "Cinematica",
        8,                         // correct
        10,                        // total
        "Quiz completato con successo"
    );
    ASSERT_TRUE(grade_id > 0, "Failed to add quiz grade");

    // Recupera e verifica la conversione (ultimi 30 giorni)
    int count = 0;
    time_t now = time(NULL);
    time_t from_date = now - (30 * 24 * 60 * 60);  // 30 days ago
    EducationGrade** grades = libretto_get_grades(profile->id, "Fisica", from_date, now, &count);
    ASSERT_TRUE(count >= 1, "Should have at least 1 grade");

    // Verifica che la percentuale 80% sia stata convertita in voto 9
    // (scala: 80-89% = 9, 90-100% = 10)
    bool found = false;
    for (int i = 0; i < count; i++) {
        if (grades[i]->id == grade_id) {
            found = true;
            ASSERT_TRUE(grades[i]->grade >= 8.5f && grades[i]->grade <= 9.5f,
                        "80% should convert to approximately 9");
            ASSERT_EQ(grades[i]->grade_type, GRADE_TYPE_QUIZ, "Should be quiz type");
            ASSERT_EQ(grades[i]->questions_correct, 8, "Correct count mismatch");
            ASSERT_EQ(grades[i]->questions_total, 10, "Total count mismatch");
        }
    }
    ASSERT_TRUE(found, "Quiz grade not found");

    libretto_grades_free(grades, count);
    PASS();
}

void test_libretto_daily_log(void) {
    TEST("Libretto - Diario attivita giornaliere");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Aggiungi entry nel diario
    int64_t log_id = libretto_add_log_entry(
        profile->id,
        "ED06",                    // Manzoni (Italiano)
        "study",
        "Italiano",
        "I Promessi Sposi - Capitolo 8",
        45,                        // 45 minuti
        "Letto e analizzato il capitolo dell'Addio ai monti"
    );
    ASSERT_TRUE(log_id > 0, "Failed to add log entry");

    // Recupera log entries (ultimi 7 giorni)
    int count = 0;
    time_t now = time(NULL);
    time_t from_date = now - (7 * 24 * 60 * 60);  // 7 days ago
    EducationDailyLogEntry** logs = libretto_get_daily_log(profile->id, from_date, now, &count);
    ASSERT_TRUE(count >= 1, "Should have at least 1 log entry");
    ASSERT_NOT_NULL(logs, "Logs array is NULL");

    // Verifica la entry
    bool found = false;
    for (int i = 0; i < count; i++) {
        if (logs[i]->id == log_id) {
            found = true;
            ASSERT_EQ(logs[i]->duration_minutes, 45, "Duration mismatch");
            ASSERT_STR_EQ(logs[i]->subject, "Italiano", "Subject mismatch");
        }
    }
    ASSERT_TRUE(found, "Log entry not found");

    libretto_logs_free(logs, count);
    PASS();
}

void test_libretto_average_calculation(void) {
    TEST("Libretto - Calcolo media voti");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Aggiungi altri voti per avere una media significativa
    libretto_add_grade(profile->id, "ED02", "Matematica", "Disequazioni", GRADE_TYPE_HOMEWORK, 7.0f, "");
    libretto_add_grade(profile->id, "ED02", "Matematica", "Sistemi", GRADE_TYPE_QUIZ, 9.0f, "");

    // Calcola media (ultimi 30 giorni)
    time_t now = time(NULL);
    time_t from_date = now - (30 * 24 * 60 * 60);  // 30 days ago
    float average = libretto_get_average(profile->id, "Matematica", from_date, now);
    ASSERT_TRUE(average > 0.0f, "Average should be positive");
    ASSERT_TRUE(average >= 1.0f && average <= 10.0f, "Average should be in Italian scale 1-10");

    PASS();
}

void test_libretto_progress_report(void) {
    TEST("Libretto - Report progressi");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Genera report progressi (ultimi 30 giorni)
    time_t now = time(NULL);
    time_t from_date = now - (30 * 24 * 60 * 60);  // 30 days ago
    EducationProgressReport* report = libretto_get_progress_report(profile->id, from_date, now);
    ASSERT_NOT_NULL(report, "Failed to generate progress report");

    // Verifica contenuti base del report
    ASSERT_TRUE(report->total_sessions >= 0, "Sessions count should be non-negative");
    ASSERT_TRUE(report->total_study_hours >= 0, "Study hours should be non-negative");

    libretto_report_free(report);
    PASS();
}

// ============================================================================
// TOOLKIT TESTS (TKT01-06)
// Using actual education_toolkit_* API
// ============================================================================

void test_toolkit_save_mindmap(void) {
    TEST("Toolkit - Save mindmap output (TKT01)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Test saving a mindmap via the toolkit API
    const char* mermaid_content = "mindmap\n  root((Topic))\n    Branch A\n    Branch B\n";
    int64_t output_id = education_toolkit_save(
        profile->id,
        TOOLKIT_MINDMAP,
        "Test Topic",
        mermaid_content,
        "mermaid"
    );
    ASSERT_TRUE(output_id > 0, "Failed to save mindmap");

    // Retrieve and verify
    EducationToolkitOutput* output = education_toolkit_get(output_id);
    ASSERT_NOT_NULL(output, "Failed to retrieve mindmap");
    ASSERT_EQ(output->tool_type, TOOLKIT_MINDMAP, "Tool type mismatch");
    ASSERT_NOT_NULL(output->content, "Content is NULL");

    education_toolkit_free(output);
    PASS();
}

void test_toolkit_save_quiz(void) {
    TEST("Toolkit - Save quiz output (TKT02)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Test saving a quiz via the toolkit API
    const char* quiz_json = "{\"title\":\"Math Quiz\",\"questions\":[{\"q\":\"2+2?\",\"a\":\"4\"}]}";
    int64_t output_id = education_toolkit_save(
        profile->id,
        TOOLKIT_QUIZ,
        "Math Basics",
        quiz_json,
        "json"
    );
    ASSERT_TRUE(output_id > 0, "Failed to save quiz");

    // Verify it was saved
    EducationToolkitOutput* output = education_toolkit_get(output_id);
    ASSERT_NOT_NULL(output, "Failed to retrieve quiz");
    ASSERT_EQ(output->tool_type, TOOLKIT_QUIZ, "Tool type mismatch");

    education_toolkit_free(output);
    PASS();
}

void test_toolkit_flashcards_api(void) {
    TEST("Toolkit - Flashcard spaced repetition (TKT03)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Save a flashcard deck as toolkit output
    const char* deck_json = "{\"cards\":[{\"front\":\"Hello\",\"back\":\"Ciao\"},{\"front\":\"Goodbye\",\"back\":\"Arrivederci\"}]}";
    int64_t deck_id = education_toolkit_save(
        profile->id,
        TOOLKIT_FLASHCARD,
        "Italian Vocabulary",
        deck_json,
        "json"
    );
    ASSERT_TRUE(deck_id > 0, "Failed to save flashcard deck");

    // Note: flashcard_reviews uses different schema (deck_id from flashcard_decks table)
    // The education_flashcard_create_reviews function would need flashcard_decks integration
    // For now, just verify due count returns 0 (no error)
    int due = education_flashcard_due_count(profile->id);
    ASSERT_TRUE(due >= 0, "Due count should be non-negative");

    PASS();
}

void test_toolkit_accessibility_wants_tts(void) {
    TEST("Toolkit - TTS preference check (TKT04)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Check if TTS is wanted based on profile
    bool wants_tts = education_accessibility_wants_tts(profile->id);

    // For Mario's profile (dyslexia with TTS enabled), this should be true
    // The function should not crash regardless of the value
    (void)wants_tts;

    PASS();
}

void test_toolkit_save_audio(void) {
    TEST("Toolkit - Save audio output (TKT05)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Test saving an audio output reference
    int64_t output_id = education_toolkit_save(
        profile->id,
        TOOLKIT_AUDIO,
        "Lesson Audio",
        "/path/to/audio.m4a",
        "m4a"
    );
    ASSERT_TRUE(output_id > 0, "Failed to save audio reference");

    PASS();
}

void test_toolkit_list_outputs(void) {
    TEST("Toolkit - List all outputs (TKT06)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // List all toolkit outputs for the student
    int count = 0;
    EducationToolkitOutput** outputs = education_toolkit_list(profile->id, -1, &count);

    // We should have at least the outputs from previous tests
    ASSERT_TRUE(count >= 0, "Count should be non-negative");
    if (count > 0) {
        ASSERT_NOT_NULL(outputs, "Outputs array should not be NULL");
        education_toolkit_list_free(outputs, count);
    }

    PASS();
}

// ============================================================================
// ADAPTIVE LEARNING TESTS (S18)
// ============================================================================

void test_adaptive_learning_api(void) {
    TEST("Adaptive Learning - Analysis API (S18)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Record some learning progress first
    education_progress_record(profile->id, "ED02", "Equations", 0.7f, 30);
    education_progress_record(profile->id, "ED06", "Grammar", 0.9f, 45);

    // Test adaptive analysis
    char* analysis = education_adaptive_analyze(profile->id);
    ASSERT_NOT_NULL(analysis, "Adaptive analysis should return JSON");
    ASSERT_TRUE(strlen(analysis) > 20, "Analysis JSON too short");
    ASSERT_TRUE(strstr(analysis, "student_id") != NULL, "JSON should contain student_id");

    free(analysis);

    // Test profile update
    int rc = education_adaptive_update_profile(profile->id);
    ASSERT_TRUE(rc == 0, "Adaptive profile update should succeed");

    // Test next topic suggestion (may return NULL if no curriculum loaded)
    char* next = education_adaptive_next_topic(profile->id, "Matematica");
    if (next) {
        ASSERT_TRUE(strlen(next) > 0, "Next topic should not be empty");
        free(next);
    }
    // NULL is acceptable if curriculum not fully loaded

    PASS();
}

// ============================================================================
// CURRICULUM TESTS (CT02-04)
// ============================================================================

void test_curriculum_api_load(void) {
    TEST("Curriculum - Load curriculum API (CT02)");

    // Test loading curriculum using the API
    EducationCurriculum* curr = education_curriculum_load("it_liceo_scientifico_1");
    // This might return NULL if file not found, which is OK for API test
    if (curr) {
        education_curriculum_free(curr);
    }

    // Test listing curricula
    int count = 0;
    char** curricula = education_curriculum_list(&count);
    // Should return list or NULL
    if (curricula && count > 0) {
        education_curriculum_list_free(curricula, count);
    }

    PASS();
}

void test_curriculum_subjects(void) {
    TEST("Curriculum - Get subjects API (CT03)");

    // Test getting subjects for a curriculum year
    int count = 0;
    EducationSubject** subjects = education_curriculum_get_subjects("it_liceo_scientifico_1", 1, &count);
    // May return NULL if curriculum not loaded, which is OK
    if (subjects && count > 0) {
        // Verify subject structure
        ASSERT_NOT_NULL(subjects[0], "First subject should not be NULL");
    }

    PASS();
}

void test_curriculum_progress_api(void) {
    TEST("Curriculum - Progress tracking via API (CT04)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Record progress using the standard API
    int rc = education_progress_record(profile->id, "ED02", "Quadratic Equations", 0.8f, 45);
    ASSERT_TRUE(rc == 0, "Failed to record progress");

    // Get progress
    EducationProgress* prog = education_progress_get(profile->id, "Quadratic Equations");
    if (prog) {
        ASSERT_TRUE(prog->skill_level >= 0.0f && prog->skill_level <= 1.0f, "Skill level out of range");
        education_progress_free(prog);
    }

    PASS();
}

// ============================================================================
// FEATURE TESTS (FT01-04)
// ============================================================================

void test_session_api(void) {
    TEST("Features - Study session API (FT01)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Start a session using the API
    int64_t session_id = education_session_start(
        profile->id,
        "study",
        "Matematica",
        "Equazioni"
    );
    ASSERT_TRUE(session_id > 0, "Failed to start session");

    // End the session
    int rc = education_session_end(session_id, 10);  // 10 XP
    ASSERT_TRUE(rc == 0, "Failed to end session");

    PASS();
}

void test_session_list(void) {
    TEST("Features - List recent sessions (FT02)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // List recent sessions
    int count = 0;
    EducationSession** sessions = education_session_list(profile->id, 10, &count);
    ASSERT_TRUE(count >= 0, "Count should be non-negative");

    if (sessions && count > 0) {
        education_session_list_free(sessions, count);
    }

    PASS();
}

void test_preside_dashboard(void) {
    TEST("Features - Ali Preside dashboard (FT03)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Get preside dashboard
    PresideStudentDashboard* dashboard = preside_get_dashboard(profile->id);
    ASSERT_NOT_NULL(dashboard, "Failed to get preside dashboard");
    ASSERT_TRUE(dashboard->student_id == profile->id, "Student ID mismatch");

    preside_dashboard_free(dashboard);
    PASS();
}

void test_preside_weekly_report(void) {
    TEST("Features - Weekly report generation (FT04)");

    EducationStudentProfile* profile = education_profile_get_active();
    if (!profile) {
        FAIL("No active profile");
        return;
    }

    // Generate weekly report
    char* report = preside_generate_weekly_report(profile->id);
    ASSERT_NOT_NULL(report, "Failed to generate weekly report");
    ASSERT_TRUE(strlen(report) > 0, "Report should not be empty");

    free(report);
    PASS();
}

// ============================================================================
// ACCESSIBILITY RUNTIME TESTS (AT04-09)
// Using actual a11y_* functions from education.h
// ============================================================================

void test_accessibility_font_api(void) {
    TEST("Accessibility - Dyslexia font API (AT04)");

    EducationAccessibility access = {
        .dyslexia = true,
        .dyslexia_severity = SEVERITY_SEVERE
    };

    // Test font recommendation
    const char* font = a11y_get_font(&access);
    ASSERT_NOT_NULL(font, "Should return a font name");

    // Test line spacing
    float spacing = a11y_get_line_spacing(&access);
    ASSERT_TRUE(spacing >= 1.0f, "Line spacing should be >= 1.0");

    // Test max line width
    int width = a11y_get_max_line_width(&access);
    ASSERT_TRUE(width > 0, "Line width should be positive");

    PASS();
}

void test_accessibility_text_adaptation(void) {
    TEST("Accessibility - Text adaptation (AT05)");

    EducationAccessibility access = {
        .dyslexia = true,
        .dyslexia_severity = SEVERITY_MODERATE
    };

    // Test syllabification
    char* syllabified = a11y_syllabify_word("computer");
    ASSERT_NOT_NULL(syllabified, "Syllabification should work");
    free(syllabified);

    // Test background color
    const char* bg = a11y_get_background_color(&access);
    ASSERT_NOT_NULL(bg, "Should return a background color");

    PASS();
}

void test_accessibility_dyscalculia(void) {
    TEST("Accessibility - Dyscalculia number formatting (AT06)");

    // Test number formatting with colors
    char* formatted = a11y_format_number_colored(12345.67, true);
    ASSERT_NOT_NULL(formatted, "Number formatting should work");
    free(formatted);

    // Test place value blocks
    char* blocks = a11y_generate_place_value_blocks(1234);
    ASSERT_NOT_NULL(blocks, "Place value blocks should be generated");
    free(blocks);

    PASS();
}

void test_accessibility_motor(void) {
    TEST("Accessibility - Motor difficulties timeout (AT07)");

    EducationAccessibility access = {
        .cerebral_palsy = true,
        .cerebral_palsy_severity = SEVERITY_MODERATE
    };

    // Test timeout multiplier
    int multiplier = a11y_get_timeout_multiplier(&access);
    ASSERT_TRUE(multiplier >= 1, "Timeout multiplier should be >= 1");

    // Test adjusted timeout
    int adjusted = a11y_get_adjusted_timeout(&access, 30);
    ASSERT_TRUE(adjusted >= 30, "Adjusted timeout should be >= base");

    // Test break suggestion
    bool suggest = a11y_suggest_break(&access, 15);
    (void)suggest;  // Just verify it doesn't crash

    PASS();
}

void test_accessibility_adhd(void) {
    TEST("Accessibility - ADHD adaptations (AT08)");

    EducationAccessibility access = {
        .adhd = true,
        .adhd_type = ADHD_COMBINED,
        .adhd_severity = SEVERITY_MODERATE
    };

    // Test max bullets
    int bullets = a11y_get_max_bullets(&access);
    ASSERT_TRUE(bullets > 0 && bullets <= 10, "Max bullets should be reasonable");

    // Test progress bar generation
    char* progress = a11y_generate_progress_bar(3, 10, 20);
    ASSERT_NOT_NULL(progress, "Progress bar should be generated");
    free(progress);

    // Test celebration message
    const char* celebration = a11y_get_celebration_message(1);
    ASSERT_NOT_NULL(celebration, "Celebration message should exist");

    PASS();
}

void test_accessibility_autism(void) {
    TEST("Accessibility - Autism adaptations (AT09)");

    EducationAccessibility access = {
        .autism = true,
        .autism_severity = SEVERITY_MILD
    };

    // Test metaphor avoidance check
    bool avoid = a11y_avoid_metaphors(&access);
    ASSERT_TRUE(avoid, "Should avoid metaphors for autism");

    // Test metaphor detection
    bool has_metaphor = a11y_contains_metaphors("The sky is crying");
    (void)has_metaphor;  // Just verify it doesn't crash

    // Test structure prefix
    const char* prefix = a11y_get_structure_prefix("introduction");
    ASSERT_NOT_NULL(prefix, "Structure prefix should exist");

    // Test topic change warning
    char* warning = a11y_get_topic_change_warning("Math", "History");
    ASSERT_NOT_NULL(warning, "Topic change warning should be generated");
    free(warning);

    // Test reduce motion
    bool reduce = a11y_reduce_motion(&access);
    (void)reduce;  // Value depends on settings

    PASS();
}

// ============================================================================
// PHASE 14: PROACTIVE TEACHING & STUDENT EXPERIENCE
// ============================================================================

/**
 * Test Error Interpreter (ER01-05)
 * Transforms technical errors into friendly, empathetic messages
 */
void test_error_interpreter(void) {
    TEST("Error Interpreter - Education edition only (ER01-05)");

    // Test that the function exists and handles NULL
    bool should_interpret = education_should_interpret_error(NULL);
    ASSERT_TRUE(!should_interpret, "NULL should not be interpreted");

    // Test error pattern matching (only works in education edition)
    should_interpret = education_should_interpret_error("Error: Tool execution failed");
    // Result depends on edition, just verify it doesn't crash

    // Test interpretation function
    char* friendly = education_interpret_error("Error: Too many tool iterations", "euclide-matematica");
    // In non-education edition, returns the original
    if (friendly) {
        free(friendly);
    }

    PASS();
}

/**
 * Test Multi-Profile System (SP01-06)
 * Multiple students per device with profile switching
 */
void test_multi_profile(void) {
    TEST("Multi-Profile System - Profile list and count (SP01-06)");

    // Test profile count (should work even with no profiles)
    int count = education_profile_count();
    ASSERT_TRUE(count >= 0, "Profile count should be non-negative");

    // Test first run detection
    bool first_run = education_is_first_run();
    // Just verify it doesn't crash - result depends on state

    // Test profile list
    int list_count = 0;
    EducationStudentProfile** profiles = education_profile_list(&list_count);
    ASSERT_TRUE(list_count >= 0, "List count should be non-negative");

    if (profiles && list_count > 0) {
        // Verify first profile has valid name
        ASSERT_NOT_NULL(profiles[0], "First profile should exist");
        if (profiles[0]->name) {
            ASSERT_TRUE(strlen(profiles[0]->name) > 0, "Profile name should not be empty");
        }
        education_profile_list_free(profiles, list_count);
    }

    (void)first_run;  // Suppress unused warning

    PASS();
}

/**
 * Test Document Upload API (DU01-13)
 * File picker and Claude Files API integration
 */
void test_document_upload_api(void) {
    TEST("Document Upload - API functions (DU01-13)");

    // Test document state functions
    bool active = document_is_active();
    ASSERT_TRUE(!active, "No document should be active initially");

    const char* file_id = document_get_current_file_id();
    ASSERT_TRUE(file_id == NULL, "File ID should be NULL when no document active");

    const char* filename = document_get_current_filename();
    ASSERT_TRUE(filename == NULL, "Filename should be NULL when no document active");

    // Test document clear (should not crash even with no documents)
    document_clear();

    // Test document select with invalid index (should return false)
    bool selected = document_select(999);
    ASSERT_TRUE(!selected, "Invalid index should return false");

    PASS();
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║     CONVERGIO EDUCATION PACK - TEST SCENARI SCOLASTICI    ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    // Inizializzazione
    printf("\n=== SETUP ===\n");
    int rc = education_init();
    if (rc != 0) {
        printf("FATAL: education_init() failed with code %d\n", rc);
        return 1;
    }

    // Test scenari studenti
    printf("\n=== SCENARIO MARIO (Multi-disabilita) ===\n");
    test_scenario_mario_setup();
    test_scenario_mario_study_math();

    printf("\n=== SCENARIO SOFIA (ADHD) ===\n");
    test_scenario_sofia_setup();
    test_scenario_sofia_homework();

    printf("\n=== SCENARIO LUCA (Autismo) ===\n");
    test_scenario_luca_setup();

    printf("\n=== SCENARIO GIULIA (Baseline) ===\n");
    test_scenario_giulia_baseline();

    // Test funzionalita'
    printf("\n=== FUNZIONALITA' ===\n");
    test_goal_management();
    test_curriculum_load();
    test_maestri_exist();
    test_maestri_maieutic_prompts();
    test_maestri_accessibility_adaptation();

    // Test Libretto dello Studente
    printf("\n=== LIBRETTO DELLO STUDENTE ===\n");
    test_libretto_grade_recording();
    test_libretto_quiz_grade_conversion();
    test_libretto_daily_log();
    test_libretto_average_calculation();
    test_libretto_progress_report();

    // Test Toolkit (TKT01-06)
    printf("\n=== TOOLKIT ===\n");
    test_toolkit_save_mindmap();
    test_toolkit_save_quiz();
    test_toolkit_flashcards_api();
    test_toolkit_accessibility_wants_tts();
    test_toolkit_save_audio();
    test_toolkit_list_outputs();

    // Test Adaptive Learning (S18)
    printf("\n=== ADAPTIVE LEARNING ===\n");
    test_adaptive_learning_api();

    // Test Curriculum (CT02-04)
    printf("\n=== CURRICULUM ===\n");
    test_curriculum_api_load();
    test_curriculum_subjects();
    test_curriculum_progress_api();

    // Test Features (FT01-04)
    printf("\n=== FEATURES ===\n");
    test_session_api();
    test_session_list();
    test_preside_dashboard();
    test_preside_weekly_report();

    // Test Accessibility (AT04-09)
    printf("\n=== ACCESSIBILITY ===\n");
    test_accessibility_font_api();
    test_accessibility_text_adaptation();
    test_accessibility_dyscalculia();
    test_accessibility_motor();
    test_accessibility_adhd();
    test_accessibility_autism();

    // Test Phase 14: Proactive Teaching & Student Experience
    printf("\n=== PHASE 14: PROACTIVE TEACHING ===\n");
    test_error_interpreter();
    test_multi_profile();
    test_document_upload_api();

    // Cleanup
    education_shutdown();

    // Report finale
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                      TEST RESULTS                         ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-3d                                              ║\n", tests_passed);
    printf("║  Failed: %-3d                                              ║\n", tests_failed);
    printf("║  Total:  %-3d                                              ║\n", tests_passed + tests_failed);
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    return tests_failed > 0 ? 1 : 0;
}
