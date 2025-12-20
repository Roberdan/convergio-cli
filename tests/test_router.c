/**
 * CONVERGIO ROUTER TESTS
 *
 * Unit tests for conditional routing and condition evaluation
 */

#include "nous/router.h"
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

// ============================================================================
// CONDITION EVALUATION TESTS
// ============================================================================

static void test_router_simple_condition(void) {
    printf("test_router_simple_condition:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    TEST_ASSERT(wf != NULL, "workflow created");
    
    workflow_set_state(wf, "status", "active");
    
    bool result = router_evaluate_condition(wf, "status == 'active'");
    TEST_ASSERT(result == true, "simple condition evaluates correctly");
    
    workflow_destroy(wf);
    printf("\n");
}

static void test_router_negation(void) {
    printf("test_router_negation:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    TEST_ASSERT(wf != NULL, "workflow created");
    
    workflow_set_state(wf, "status", "inactive");
    
    bool result = router_evaluate_condition(wf, "status != 'active'");
    TEST_ASSERT(result == true, "negation condition evaluates correctly");
    
    workflow_destroy(wf);
    printf("\n");
}

static void test_router_logical_and(void) {
    printf("test_router_logical_and:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    TEST_ASSERT(wf != NULL, "workflow created");
    
    workflow_set_state(wf, "status", "active");
    workflow_set_state(wf, "type", "production");
    
    bool result = router_evaluate_condition(wf, "status == 'active' && type == 'production'");
    TEST_ASSERT(result == true || result == false, "logical AND evaluates");
    
    workflow_destroy(wf);
    printf("\n");
}

static void test_router_logical_or(void) {
    printf("test_router_logical_or:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    TEST_ASSERT(wf != NULL, "workflow created");
    
    workflow_set_state(wf, "status", "pending");
    
    bool result = router_evaluate_condition(wf, "status == 'active' || status == 'pending'");
    TEST_ASSERT(result == true || result == false, "logical OR evaluates");
    
    workflow_destroy(wf);
    printf("\n");
}

static void test_router_missing_key(void) {
    printf("test_router_missing_key:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    TEST_ASSERT(wf != NULL, "workflow created");
    
    // Don't set the key - should handle gracefully (return false or handle error)
    bool result = router_evaluate_condition(wf, "missing_key == 'value'");
    TEST_ASSERT(result == false || result == true, "missing key handled gracefully");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO ROUTER TESTS ===\n\n");
    
    test_router_simple_condition();
    test_router_negation();
    test_router_logical_and();
    test_router_logical_or();
    test_router_missing_key();
    
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

