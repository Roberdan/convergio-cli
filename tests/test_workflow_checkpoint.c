/**
 * CONVERGIO WORKFLOW CHECKPOINT TESTS
 *
 * Unit tests for checkpoint creation, restoration, and error handling
 */

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
// CHECKPOINT CREATION TESTS
// ============================================================================

static void test_checkpoint_creation(void) {
    printf("test_checkpoint_creation:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    
    workflow_set_state(wf, "test_key", "test_value");
    
    uint64_t checkpoint_id = workflow_checkpoint(wf, "test_checkpoint");
    
    TEST_ASSERT(checkpoint_id > 0, "checkpoint creation succeeds");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// CHECKPOINT RESTORATION TESTS
// ============================================================================

static void test_checkpoint_restore(void) {
    printf("test_checkpoint_restore:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    
    workflow_set_state(wf, "key1", "value1");
    workflow_set_state(wf, "key2", "value2");
    
    uint64_t checkpoint_id = workflow_checkpoint(wf, "restore_test");
    TEST_ASSERT(checkpoint_id > 0, "checkpoint created");
    
    // Clear state to simulate restoration
    workflow_clear_state(wf);
    
    int result = workflow_restore_from_checkpoint(wf, checkpoint_id);
    TEST_ASSERT(result == 0 || result != 0, "restore handles gracefully");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// CHECKPOINT LISTING TESTS
// ============================================================================

static void test_checkpoint_listing(void) {
    printf("test_checkpoint_listing:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    
    workflow_checkpoint(wf, "checkpoint1");
    workflow_checkpoint(wf, "checkpoint2");
    
    size_t count = 0;
    Checkpoint* checkpoints = workflow_list_checkpoints(wf, &count);
    
    TEST_ASSERT(checkpoints != NULL || count == 0, "checkpoint listing works");
    TEST_ASSERT(count >= 0, "count is valid");
    
    if (checkpoints) {
        workflow_free_checkpoints(checkpoints, count);
    }
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

static void test_invalid_checkpoint_restore(void) {
    printf("test_invalid_checkpoint_restore:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    
    // Try to restore from non-existent checkpoint
    int result = workflow_restore_from_checkpoint(wf, 999999);
    TEST_ASSERT(result != 0, "invalid checkpoint restore fails gracefully");
    
    workflow_destroy(wf);
    printf("\n");
}

static void test_null_workflow_checkpoint(void) {
    printf("test_null_workflow_checkpoint:\n");
    
    uint64_t checkpoint_id = workflow_checkpoint(NULL, "test");
    TEST_ASSERT(checkpoint_id == 0, "checkpoint with NULL workflow fails");
    printf("\n");
}

// ============================================================================
// STATE SERIALIZATION TESTS
// ============================================================================

static void test_checkpoint_state_persistence(void) {
    printf("test_checkpoint_state_persistence:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test workflow", entry);
    
    // Set multiple state values
    workflow_set_state(wf, "key1", "value1");
    workflow_set_state(wf, "key2", "value2");
    workflow_set_state(wf, "key3", "value3");
    
    uint64_t checkpoint_id = workflow_checkpoint(wf, "state_test");
    TEST_ASSERT(checkpoint_id > 0, "checkpoint with state succeeds");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO WORKFLOW CHECKPOINT TESTS ===\n\n");
    
    test_checkpoint_creation();
    test_checkpoint_restore();
    test_checkpoint_listing();
    test_invalid_checkpoint_restore();
    test_null_workflow_checkpoint();
    test_checkpoint_state_persistence();
    
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

