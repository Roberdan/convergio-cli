/**
 * CONVERGIO TEST UTILITIES (REF-06)
 *
 * Unified test framework with:
 * - Standardized assertions
 * - Test discovery
 * - Setup/teardown hooks
 * - Output formatting
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ============================================================================
// TEST FRAMEWORK GLOBALS
// ============================================================================

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;
static const char* g_current_test = NULL;
static const char* g_current_file = NULL;

// ============================================================================
// ANSI COLORS
// ============================================================================

#define TEST_COLOR_GREEN  "\033[32m"
#define TEST_COLOR_RED    "\033[31m"
#define TEST_COLOR_YELLOW "\033[33m"
#define TEST_COLOR_RESET  "\033[0m"
#define TEST_COLOR_BOLD   "\033[1m"

// ============================================================================
// TEST MACROS
// ============================================================================

/**
 * Define a test function
 */
#define TEST(name) \
    static void test_##name(void); \
    static void __attribute__((constructor)) register_test_##name(void) { \
        test_registry_add(#name, test_##name, __FILE__); \
    } \
    static void test_##name(void)

/**
 * Define a test with setup/teardown
 */
#define TEST_F(fixture, name) \
    static void test_##fixture##_##name(void); \
    static void __attribute__((constructor)) register_test_##fixture##_##name(void) { \
        test_registry_add_fixture(#fixture, #name, \
            fixture##_setup, fixture##_teardown, \
            test_##fixture##_##name, __FILE__); \
    } \
    static void test_##fixture##_##name(void)

/**
 * Skip a test
 */
#define SKIP_TEST(reason) do { \
    printf(TEST_COLOR_YELLOW "  SKIP: %s" TEST_COLOR_RESET "\n", reason); \
    return; \
} while(0)

// ============================================================================
// ASSERTIONS
// ============================================================================

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_TRUE(%s)" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #expr); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(expr) do { \
    if (expr) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_FALSE(%s)" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #expr); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_EQ(%s, %s) - expected %ld, got %ld" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #expected, #actual, (long)(expected), (long)(actual)); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NE(expected, actual) do { \
    if ((expected) == (actual)) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_NE(%s, %s) - both equal %ld" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #expected, #actual, (long)(expected)); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_NULL(%s)" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #ptr); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_NOT_NULL(%s)" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #ptr); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_STR_EQ - expected \"%s\", got \"%s\"" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_CONTAINS(haystack, needle) do { \
    if (strstr((haystack), (needle)) == NULL) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_STR_CONTAINS - \"%s\" not in \"%s\"" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, (needle), (haystack)); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_GT(a, b) do { \
    if (!((a) > (b))) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_GT(%s > %s)" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #a, #b); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_GE(a, b) do { \
    if (!((a) >= (b))) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_GE(%s >= %s)" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #a, #b); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_LT(a, b) do { \
    if (!((a) < (b))) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_LT(%s < %s)" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #a, #b); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_LE(a, b) do { \
    if (!((a) <= (b))) { \
        printf(TEST_COLOR_RED "  FAIL: %s:%d: ASSERT_LE(%s <= %s)" TEST_COLOR_RESET "\n", \
               __FILE__, __LINE__, #a, #b); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// TEST REGISTRY
// ============================================================================

typedef void (*TestFunc)(void);
typedef void (*SetupFunc)(void);
typedef void (*TeardownFunc)(void);

typedef struct {
    const char* name;
    const char* fixture;
    const char* file;
    TestFunc test;
    SetupFunc setup;
    TeardownFunc teardown;
} TestEntry;

#define MAX_TESTS 1024
static TestEntry g_test_registry[MAX_TESTS];
static int g_test_count = 0;

static inline void test_registry_add(const char* name, TestFunc test, const char* file) {
    if (g_test_count < MAX_TESTS) {
        g_test_registry[g_test_count].name = name;
        g_test_registry[g_test_count].fixture = NULL;
        g_test_registry[g_test_count].file = file;
        g_test_registry[g_test_count].test = test;
        g_test_registry[g_test_count].setup = NULL;
        g_test_registry[g_test_count].teardown = NULL;
        g_test_count++;
    }
}

static inline void test_registry_add_fixture(const char* fixture, const char* name,
                                              SetupFunc setup, TeardownFunc teardown,
                                              TestFunc test, const char* file) {
    if (g_test_count < MAX_TESTS) {
        g_test_registry[g_test_count].name = name;
        g_test_registry[g_test_count].fixture = fixture;
        g_test_registry[g_test_count].file = file;
        g_test_registry[g_test_count].test = test;
        g_test_registry[g_test_count].setup = setup;
        g_test_registry[g_test_count].teardown = teardown;
        g_test_count++;
    }
}

// ============================================================================
// TEST RUNNER
// ============================================================================

static inline void test_run_all(void) {
    printf(TEST_COLOR_BOLD "\n=== Running %d tests ===" TEST_COLOR_RESET "\n\n", g_test_count);

    for (int i = 0; i < g_test_count; i++) {
        TestEntry* entry = &g_test_registry[i];
        g_current_test = entry->name;
        g_current_file = entry->file;
        g_tests_run++;

        int failed_before = g_tests_failed;

        // Run setup if fixture
        if (entry->setup) entry->setup();

        // Run test
        printf("  [%d/%d] %s%s%s ... ", i + 1, g_test_count,
               entry->fixture ? entry->fixture : "",
               entry->fixture ? "." : "",
               entry->name);
        fflush(stdout);

        entry->test();

        // Run teardown if fixture
        if (entry->teardown) entry->teardown();

        if (g_tests_failed == failed_before) {
            printf(TEST_COLOR_GREEN "PASS" TEST_COLOR_RESET "\n");
            g_tests_passed++;
        }
    }

    // Summary
    printf(TEST_COLOR_BOLD "\n=== Results ===" TEST_COLOR_RESET "\n");
    printf("  Total:  %d\n", g_tests_run);
    printf(TEST_COLOR_GREEN "  Passed: %d" TEST_COLOR_RESET "\n", g_tests_passed);
    if (g_tests_failed > 0) {
        printf(TEST_COLOR_RED "  Failed: %d" TEST_COLOR_RESET "\n", g_tests_failed);
    }
    printf("\n");
}

/**
 * Main entry point macro for test executables
 */
#define TEST_MAIN() \
    int main(int argc, char** argv) { \
        (void)argc; (void)argv; \
        test_run_all(); \
        return g_tests_failed > 0 ? 1 : 0; \
    }

#endif // TEST_UTILS_H
