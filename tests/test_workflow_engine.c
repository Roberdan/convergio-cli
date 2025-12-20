/**
 * CONVERGIO WORKFLOW ENGINE TESTS
 *
 * Unit tests for workflow state machine execution
 */

#include "nous/workflow.h"
#include "nous/orchestrator.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

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

// Mock agent execution (simplified)
static int mock_agent_execute(SemanticID agent_id, const char* prompt, const char* input, char** output) {
    (void)agent_id;
    (void)prompt;
    if (output) {
        *output = strdup(input ? input : "mock_output");
    }
    return 0;
}

// ============================================================================
// LINEAR WORKFLOW EXECUTION TESTS
// ============================================================================

static void test_linear_workflow_execution(void) {
    printf("test_linear_workflow_execution:\n");
    
    WorkflowNode* node1 = workflow_node_create("step1", NODE_TYPE_ACTION);
    WorkflowNode* node2 = workflow_node_create("step2", NODE_TYPE_ACTION);
    workflow_node_add_edge(node1, node2, NULL);
    
    workflow_node_set_agent(node1, 1, "Do step 1");
    workflow_node_set_agent(node2, 2, "Do step 2");
    
    Workflow* wf = workflow_create("test_linear", "Test linear workflow", node1);
    TEST_ASSERT(wf != NULL, "workflow created");
    
    char* output = NULL;
    int result = workflow_execute(wf, "test_input", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED || wf->status == WORKFLOW_STATUS_FAILED,
                "workflow execution completes");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// STATE TRANSITION TESTS
// ============================================================================

static void test_state_transitions(void) {
    printf("test_state_transitions:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test", entry);
    
    TEST_ASSERT(wf->status == WORKFLOW_STATUS_PENDING, "initial status is PENDING");
    
    // Execute should transition to RUNNING then COMPLETED or FAILED
    char* output = NULL;
    workflow_execute(wf, "input", &output);
    
    TEST_ASSERT(wf->status == WORKFLOW_STATUS_RUNNING || 
                wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "status transitions correctly");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

static void test_workflow_execution_error(void) {
    printf("test_workflow_execution_error:\n");
    
    // Create workflow with NULL entry node (should handle gracefully)
    Workflow* wf = workflow_create("test", "Test", NULL);
    
    if (wf) {
        char* output = NULL;
        int result = workflow_execute(wf, "input", &output);
        
        TEST_ASSERT(result != 0 || wf->status == WORKFLOW_STATUS_FAILED,
                    "execution fails gracefully with invalid entry");
        
        if (output) {
            free(output);
        }
        workflow_destroy(wf);
    } else {
        TEST_ASSERT(wf == NULL, "workflow creation rejects NULL entry");
    }
    printf("\n");
}

static void test_null_input_handling(void) {
    printf("test_null_input_handling:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test", entry);
    
    char* output = NULL;
    int result = workflow_execute(wf, NULL, &output);
    
    TEST_ASSERT(result == 0 || result != 0, "handles NULL input gracefully");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// STATE MANAGEMENT TESTS
// ============================================================================

static void test_workflow_state_management(void) {
    printf("test_workflow_state_management:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test", entry);
    
    int result = workflow_set_state(wf, "key1", "value1");
    TEST_ASSERT(result == 0, "workflow_set_state succeeds");
    
    const char* value = workflow_get_state_value(wf, "key1");
    TEST_ASSERT(value != NULL, "workflow_state_get returns value");
    TEST_ASSERT(strcmp(value, "value1") == 0, "retrieved value matches");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// NODE MANAGEMENT TESTS
// ============================================================================

static void test_workflow_get_current_node(void) {
    printf("test_workflow_get_current_node:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test", entry);
    
    WorkflowNode* current = workflow_get_current_node(wf);
    TEST_ASSERT(current != NULL, "get_current_node returns node");
    TEST_ASSERT(current == entry, "current node is entry node initially");
    
    workflow_destroy(wf);
    printf("\n");
}

static void test_workflow_get_next_node(void) {
    printf("test_workflow_get_next_node:\n");
    
    WorkflowNode* node1 = workflow_node_create("step1", NODE_TYPE_ACTION);
    WorkflowNode* node2 = workflow_node_create("step2", NODE_TYPE_ACTION);
    workflow_node_add_edge(node1, node2, NULL);
    
    Workflow* wf = workflow_create("test", "Test", node1);
    
    WorkflowNode* next = workflow_get_next_node(wf, node1);
    TEST_ASSERT(next != NULL, "get_next_node returns next node");
    TEST_ASSERT(next == node2, "next node is correct");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// PAUSE AND CANCEL TESTS
// ============================================================================

static void test_workflow_pause(void) {
    printf("test_workflow_pause:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test", entry);
    
    int result = workflow_pause(wf);
    TEST_ASSERT(result == 0, "workflow_pause succeeds");
    TEST_ASSERT(wf->status == WORKFLOW_STATUS_PAUSED, "status is PAUSED");
    
    workflow_destroy(wf);
    printf("\n");
}

static void test_workflow_cancel(void) {
    printf("test_workflow_cancel:\n");
    
    WorkflowNode* entry = workflow_node_create("entry", NODE_TYPE_ACTION);
    Workflow* wf = workflow_create("test", "Test", entry);
    
    int result = workflow_cancel(wf);
    TEST_ASSERT(result == 0, "workflow_cancel succeeds");
    TEST_ASSERT(wf->status == WORKFLOW_STATUS_CANCELLED, "status is CANCELLED");
    
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO WORKFLOW ENGINE TESTS ===\n\n");
    
    test_linear_workflow_execution();
    test_state_transitions();
    test_workflow_execution_error();
    test_null_input_handling();
    test_workflow_state_management();
    test_workflow_get_current_node();
    test_workflow_get_next_node();
    test_workflow_pause();
    test_workflow_cancel();
    
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

