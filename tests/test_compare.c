/**
 * CONVERGIO COMPARE MODULE TESTS
 *
 * Unit tests for model comparison functionality
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/nous/compare.h"

// ============================================================================
// TEST HELPERS
// ============================================================================

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "TEST FAILED: %s\n", msg); \
            exit(1); \
        } \
    } while(0)

// ============================================================================
// TEST RENDER FUNCTIONS
// ============================================================================

static void test_render_json(void) {
    // Create test results
    CompareResult results[2] = {0};
    
    results[0].model_id = strdup("test-model-1");
    results[0].response = strdup("Hello \"world\"");
    results[0].success = true;
    results[0].time_ms = 123.45;
    results[0].tokens_in = 10;
    results[0].tokens_out = 20;
    results[0].cost = 0.001;
    
    results[1].model_id = strdup("test-model-2");
    results[1].response = strdup("Test\nresponse");
    results[1].success = true;
    results[1].time_ms = 234.56;
    results[1].tokens_in = 15;
    results[1].tokens_out = 25;
    results[1].cost = 0.002;
    
    // Test JSON rendering
    char* json = render_comparison_json(results, 2);
    TEST_ASSERT(json != NULL, "JSON should not be NULL");
    TEST_ASSERT(strstr(json, "test-model-1") != NULL, "JSON should contain model ID");
    TEST_ASSERT(strstr(json, "123.45") != NULL, "JSON should contain time");
    TEST_ASSERT(strstr(json, "0.001") != NULL, "JSON should contain cost");
    
    free(json);
    free(results[0].model_id);
    free(results[0].response);
    free(results[1].model_id);
    free(results[1].response);
    
    printf("✓ test_render_json passed\n");
}

// ============================================================================
// TEST COMPARISON RESULTS
// ============================================================================

static void test_compare_result_initialization(void) {
    CompareResult result = {0};
    
    TEST_ASSERT(result.success == false, "Default success should be false");
    TEST_ASSERT(result.model_id == NULL, "Default model_id should be NULL");
    TEST_ASSERT(result.response == NULL, "Default response should be NULL");
    TEST_ASSERT(result.error == NULL, "Default error should be NULL");
    TEST_ASSERT(result.time_ms == 0.0, "Default time_ms should be 0.0");
    TEST_ASSERT(result.tokens_in == 0, "Default tokens_in should be 0");
    TEST_ASSERT(result.tokens_out == 0, "Default tokens_out should be 0");
    TEST_ASSERT(result.cost == 0.0, "Default cost should be 0.0");
    
    printf("✓ test_compare_result_initialization passed\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║       CONVERGIO COMPARE MODULE TESTS              ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    
    test_render_json();
    test_compare_result_initialization();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║       ALL TESTS PASSED                            ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}

