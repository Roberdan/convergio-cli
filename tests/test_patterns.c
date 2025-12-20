/**
 * CONVERGIO PATTERNS TESTS
 *
 * Unit tests for workflow pattern library
 */

#include "nous/patterns.h"
#include "nous/workflow.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// TEST HELPERS
// ============================================================================

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  ✓ %s\n", message); \
        } else { \
            printf("  ✗ %s\n", message); \
        } \
    } while (0)

// Mock agent IDs
#define MOCK_GENERATOR_ID 3001
#define MOCK_CRITIC_ID 3002
#define MOCK_REFINER_ID 3003
#define MOCK_ANALYST_ID 3004
#define MOCK_CONVERGER_ID 3005
#define MOCK_PLANNER_ID 3006

// ============================================================================
// REVIEW-REFINE LOOP TESTS
// ============================================================================

static void test_pattern_review_refine_loop(void) {
    printf("test_pattern_review_refine_loop:\n");
    
    Workflow* wf = pattern_create_review_refine_loop(
        MOCK_GENERATOR_ID,
        MOCK_CRITIC_ID,
        MOCK_REFINER_ID,
        3  // max iterations
    );
    
    TEST_ASSERT(wf != NULL, "review-refine loop pattern created");
    TEST_ASSERT(wf->entry_node != NULL, "entry node exists");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// PARALLEL ANALYSIS TESTS
// ============================================================================

static void test_pattern_parallel_analysis(void) {
    printf("test_pattern_parallel_analysis:\n");
    
    SemanticID analysts[] = {MOCK_ANALYST_ID, MOCK_ANALYST_ID, MOCK_ANALYST_ID};
    Workflow* wf = pattern_create_parallel_analysis(
        analysts,
        3,
        MOCK_CONVERGER_ID
    );
    
    TEST_ASSERT(wf != NULL, "parallel analysis pattern created");
    TEST_ASSERT(wf->entry_node != NULL, "entry node exists");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// SEQUENTIAL PLANNING TESTS
// ============================================================================

static void test_pattern_sequential_planning(void) {
    printf("test_pattern_sequential_planning:\n");
    
    SemanticID planners[] = {MOCK_PLANNER_ID, MOCK_PLANNER_ID};
    Workflow* wf = pattern_create_sequential_planning(planners, 2);
    
    TEST_ASSERT(wf != NULL, "sequential planning pattern created");
    TEST_ASSERT(wf->entry_node != NULL, "entry node exists");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// CONSENSUS BUILDING TESTS
// ============================================================================

static void test_pattern_consensus_building(void) {
    printf("test_pattern_consensus_building:\n");
    
    SemanticID participants[] = {MOCK_ANALYST_ID, MOCK_PLANNER_ID, MOCK_CRITIC_ID};
    Workflow* wf = pattern_create_consensus_building(
        participants,
        3,
        0.75  // 75% consensus threshold
    );
    
    TEST_ASSERT(wf != NULL, "consensus building pattern created");
    TEST_ASSERT(wf->entry_node != NULL, "entry node exists");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO PATTERNS TESTS ===\n\n");
    
    test_pattern_review_refine_loop();
    test_pattern_parallel_analysis();
    test_pattern_sequential_planning();
    test_pattern_consensus_building();
    
    printf("=== RESULTS ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n");
        return 1;
    }
}

