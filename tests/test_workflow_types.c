/**
 * CONVERGIO WORKFLOW TYPES TESTS
 *
 * Unit tests for workflow data structures and memory management
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
// WORKFLOW STATE TESTS
// ============================================================================

static void test_workflow_state_create_destroy(void) {
    printf("test_workflow_state_create_destroy:\n");
    
    WorkflowState* state = workflow_state_create();
    TEST_ASSERT(state != NULL, "workflow_state_create returns non-NULL");
    TEST_ASSERT(state->entry_count == 0, "initial entry_count is 0");
    TEST_ASSERT(state->entry_capacity > 0, "initial capacity > 0");
    
    workflow_state_destroy(state);
    printf("\n");
}

static void test_workflow_state_set_get(void) {
    printf("test_workflow_state_set_get:\n");
    
    WorkflowState* state = workflow_state_create();
    TEST_ASSERT(state != NULL, "state created");
    
    int result = workflow_state_set(state, "key1", "value1");
    TEST_ASSERT(result == 0, "workflow_state_set succeeds");
    TEST_ASSERT(state->entry_count == 1, "entry_count is 1");
    
    const char* value = workflow_state_get(state, "key1");
    TEST_ASSERT(value != NULL, "workflow_state_get returns non-NULL");
    TEST_ASSERT(strcmp(value, "value1") == 0, "retrieved value matches");
    
    workflow_state_destroy(state);
    printf("\n");
}

static void test_workflow_state_clear(void) {
    printf("test_workflow_state_clear:\n");
    
    WorkflowState* state = workflow_state_create();
    workflow_state_set(state, "key1", "value1");
    workflow_state_set(state, "key2", "value2");
    TEST_ASSERT(state->entry_count == 2, "two entries added");
    
    int result = workflow_state_clear(state);
    TEST_ASSERT(result == 0, "workflow_state_clear succeeds");
    TEST_ASSERT(state->entry_count == 0, "entry_count is 0 after clear");
    
    workflow_state_destroy(state);
    printf("\n");
}

// ============================================================================
// WORKFLOW NODE TESTS
// ============================================================================

static void test_workflow_node_create_destroy(void) {
    printf("test_workflow_node_create_destroy:\n");
    
    WorkflowNode* node = workflow_node_create("test_node", NODE_TYPE_ACTION);
    TEST_ASSERT(node != NULL, "workflow_node_create returns non-NULL");
    TEST_ASSERT(node->type == NODE_TYPE_ACTION, "node type is ACTION");
    TEST_ASSERT(strcmp(node->name, "test_node") == 0, "node name matches");
    
    workflow_node_destroy(node);
    printf("\n");
}

static void test_workflow_node_set_agent(void) {
    printf("test_workflow_node_set_agent:\n");
    
    WorkflowNode* node = workflow_node_create("test", NODE_TYPE_ACTION);
    TEST_ASSERT(node != NULL, "node created");
    
    int result = workflow_node_set_agent(node, 12345, "Do something");
    TEST_ASSERT(result == 0, "workflow_node_set_agent succeeds");
    TEST_ASSERT(node->agent_id == 12345, "agent_id set correctly");
    TEST_ASSERT(node->action_prompt != NULL, "action_prompt set");
    
    workflow_node_destroy(node);
    printf("\n");
}

// ============================================================================
// WORKFLOW TESTS
// ============================================================================

static void test_workflow_create_destroy(void) {
    printf("test_workflow_create_destroy:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    TEST_ASSERT(entry != NULL, "entry node created");
    
    Workflow* wf = workflow_create("test_workflow", "Test description", entry);
    TEST_ASSERT(wf != NULL, "workflow_create returns non-NULL");
    TEST_ASSERT(strcmp(wf->name, "test_workflow") == 0, "workflow name matches");
    TEST_ASSERT(wf->status == WORKFLOW_STATUS_PENDING, "initial status is PENDING");
    TEST_ASSERT(wf->state != NULL, "state is created");
    
    workflow_destroy(wf);
    // Note: entry node is not destroyed here (managed separately)
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO WORKFLOW TYPES TESTS ===\n\n");
    
    test_workflow_state_create_destroy();
    test_workflow_state_set_get();
    test_workflow_state_clear();
    test_workflow_node_create_destroy();
    test_workflow_node_set_agent();
    test_workflow_create_destroy();
    
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

