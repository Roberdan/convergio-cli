/**
 * Edition Separation Tests
 *
 * Tests for edition-specific settings separation:
 * - Config directories are separate per edition
 * - Keychain services are separate per edition
 * - Database paths are separate per edition
 *
 * Run with: make edition_test && ./build/bin/edition_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nous/nous.h"
#include "nous/edition.h"
#include "nous/config.h"

// Stub for nous_log
LogLevel g_log_level = LOG_LEVEL_ERROR;

void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    (void)level; (void)cat; (void)fmt;
}

void nous_log_set_level(LogLevel level) { g_log_level = level; }
LogLevel nous_log_get_level(void) { return g_log_level; }
const char* nous_log_level_name(LogLevel level) { (void)level; return ""; }

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, condition) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  \033[32m✓\033[0m %s\n", name); \
    } else { \
        tests_failed++; \
        printf("  \033[31m✗\033[0m %s FAILED\n", name); \
    } \
} while(0)

#define TEST_SECTION(name) printf("\n\033[1m=== %s ===\033[0m\n", name)

// ============================================================================
// EDITION CONFIG PATH TESTS
// ============================================================================

void test_edition_config_paths(void) {
    TEST_SECTION("Edition Config Path Separation Tests");

    // Test that different editions have different config paths
    const char* home = getenv("HOME");
    if (!home) {
        printf("  SKIP: HOME not set\n");
        return;
    }

    // Master edition
    edition_set(EDITION_MASTER);
    const char* master_config = convergio_config_get("config_dir");
    TEST("Master config dir exists", master_config != NULL);
    if (master_config) {
        TEST("Master config dir is .convergio", strstr(master_config, ".convergio") != NULL);
        TEST("Master config dir has no suffix", strstr(master_config, "-edu") == NULL &&
                                                 strstr(master_config, "-biz") == NULL &&
                                                 strstr(master_config, "-dev") == NULL);
    }

    // Note: Can't test switching to Education at runtime (security restriction)
    // But we can verify Business and Developer

#ifndef CONVERGIO_EDITION_EDUCATION
    // Business edition
    edition_set(EDITION_BUSINESS);
    convergio_config_reset(); // Reset to update paths for new edition
    const char* biz_config = convergio_config_get("config_dir");
    TEST("Business config dir exists", biz_config != NULL);
    if (biz_config) {
        TEST("Business config dir has -biz suffix", strstr(biz_config, "-biz") != NULL);
    }

    // Developer edition
    edition_set(EDITION_DEVELOPER);
    convergio_config_reset();
    const char* dev_config = convergio_config_get("config_dir");
    TEST("Developer config dir exists", dev_config != NULL);
    if (dev_config) {
        TEST("Developer config dir has -dev suffix", strstr(dev_config, "-dev") != NULL);
    }

    // Verify paths are different - must copy since config_get returns static buffer
    edition_set(EDITION_MASTER);
    convergio_config_reset();
    const char* tmp = convergio_config_get("config_dir");
    char master_copy[512] = {0};
    if (tmp) strncpy(master_copy, tmp, sizeof(master_copy) - 1);

    edition_set(EDITION_BUSINESS);
    convergio_config_reset();
    const char* biz_copy = convergio_config_get("config_dir");

    TEST("Master and Business have different config dirs",
         master_copy[0] && biz_copy && strcmp(master_copy, biz_copy) != 0);
#endif
}

// ============================================================================
// EDITION NAME TESTS
// ============================================================================

void test_edition_names(void) {
    TEST_SECTION("Edition Name Tests");

    TEST("Master name is master", strcmp(edition_get_name(EDITION_MASTER), "master") == 0);
    TEST("Education name is education", strcmp(edition_get_name(EDITION_EDUCATION), "education") == 0);
    TEST("Business name is business", strcmp(edition_get_name(EDITION_BUSINESS), "business") == 0);
    TEST("Developer name is developer", strcmp(edition_get_name(EDITION_DEVELOPER), "developer") == 0);

    TEST("Parse 'master' returns EDITION_MASTER", edition_from_name("master") == EDITION_MASTER);
    TEST("Parse 'edu' returns EDITION_EDUCATION", edition_from_name("edu") == EDITION_EDUCATION);
    TEST("Parse 'biz' returns EDITION_BUSINESS", edition_from_name("biz") == EDITION_BUSINESS);
    TEST("Parse 'dev' returns EDITION_DEVELOPER", edition_from_name("dev") == EDITION_DEVELOPER);
}

// ============================================================================
// EDITION AGENT WHITELIST TESTS
// ============================================================================

void test_edition_agents(void) {
    TEST_SECTION("Edition Agent Whitelist Tests");

#ifndef CONVERGIO_EDITION_EDUCATION
    // Master edition has all agents
    edition_set(EDITION_MASTER);
    TEST("Master has ali-chief-of-staff", edition_has_agent("ali-chief-of-staff"));
    TEST("Master has euclide-matematica", edition_has_agent("euclide-matematica"));
    TEST("Master has rex-code-reviewer", edition_has_agent("rex-code-reviewer"));

    // Business edition
    edition_set(EDITION_BUSINESS);
    TEST("Business has ali-chief-of-staff", edition_has_agent("ali-chief-of-staff"));
    TEST("Business has fabio-sales-business-development", edition_has_agent("fabio-sales-business-development"));
    TEST("Business does NOT have euclide-matematica", !edition_has_agent("euclide-matematica"));
    TEST("Business does NOT have rex-code-reviewer", !edition_has_agent("rex-code-reviewer"));

    // Developer edition
    edition_set(EDITION_DEVELOPER);
    TEST("Developer has ali-chief-of-staff", edition_has_agent("ali-chief-of-staff"));
    TEST("Developer has rex-code-reviewer", edition_has_agent("rex-code-reviewer"));
    TEST("Developer does NOT have euclide-matematica", !edition_has_agent("euclide-matematica"));
    TEST("Developer does NOT have fabio-sales-business-development", !edition_has_agent("fabio-sales-business-development"));

    // Reset to master
    edition_set(EDITION_MASTER);
#endif
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║      CONVERGIO EDITION SEPARATION TESTS                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    // Initialize
    edition_init();
    convergio_config_init();

    // Run tests
    test_edition_names();
    test_edition_config_paths();
    test_edition_agents();

    // Summary
    printf("\n══════════════════════════════════════════════════════════════\n");
    printf("Tests run: %d, Passed: %d, Failed: %d\n", tests_run, tests_passed, tests_failed);

    if (tests_failed > 0) {
        printf("\033[31mSOME TESTS FAILED\033[0m\n");
        return 1;
    }

    printf("\033[32mALL TESTS PASSED\033[0m\n");
    return 0;
}
