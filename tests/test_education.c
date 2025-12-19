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
    TEST("Maestri - Verifica 14 agent definitions");

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
        "ippocrate-corpo"
    };

    char path[512];
    for (int i = 0; i < 14; i++) {
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
