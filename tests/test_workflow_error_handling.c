/**
 * CONVERGIO WORKFLOW ERROR HANDLING TESTS
 *
 * Comprehensive tests for all error scenarios:
 * - Timeout handling
 * - Network errors
 * - File I/O errors
 * - Credit/budget exhaustion
 * - LLM service downtime
 * - Tool execution errors
 * - Agent/provider errors
 */

#include "nous/workflow.h"
#include "nous/provider.h"
#include "test_stubs.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

// ============================================================================
// TEST HELPERS
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            tests_passed++; \
            printf("  ✓ %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  ✗ %s\n", message); \
        } \
    } while (0)

// ============================================================================
// TIMEOUT TESTS
// ============================================================================

void test_timeout_check(void) {
    printf("\n=== Testing Timeout Handling ===\n");
    
    time_t start = time(NULL);
    
    // Test: No timeout yet
    bool timed_out = workflow_check_timeout(start, 60);
    TEST_ASSERT(!timed_out, "Timeout check: not timed out immediately");
    
    // Test: Timeout after delay (simulate by using past time)
    time_t past_time = time(NULL) - 120; // 2 minutes ago
    timed_out = workflow_check_timeout(past_time, 60);
    TEST_ASSERT(timed_out, "Timeout check: correctly detects timeout");
    
    // Test: Zero timeout (should use default)
    timed_out = workflow_check_timeout(start, 0);
    TEST_ASSERT(!timed_out, "Timeout check: zero timeout uses default");
}

// ============================================================================
// NETWORK ERROR TESTS
// ============================================================================

void test_network_check(void) {
    printf("\n=== Testing Network Error Handling ===\n");
    
    // Test: Network check (might pass or fail depending on actual network)
    bool network_available = workflow_check_network(5);
    printf("  ℹ Network available: %s\n", network_available ? "yes" : "no");
    
    // Test: Network error handling
    Workflow* wf = workflow_create("test", "Test workflow", NULL);
    TEST_ASSERT(wf != NULL, "Create workflow for network error test");
    
    if (wf) {
        WorkflowErrorType error = workflow_handle_network_error(wf, "Test network error");
        TEST_ASSERT(error == WORKFLOW_ERROR_NETWORK, "Network error handling returns correct type");
        TEST_ASSERT(wf->status == WORKFLOW_STATUS_FAILED, "Network error sets workflow to failed");
        TEST_ASSERT(wf->error_message != NULL, "Network error sets error message");
        
        const char* error_type = workflow_get_state_value(wf, "last_error_type");
        TEST_ASSERT(error_type != NULL && strcmp(error_type, "network") == 0, "Network error recorded in state");
        
        workflow_destroy(wf);
    }
}

// ============================================================================
// FILE I/O ERROR TESTS
// ============================================================================

void test_file_io_errors(void) {
    printf("\n=== Testing File I/O Error Handling ===\n");
    
    // Test: Check readable file (should exist: this test file)
    bool readable = workflow_check_file_readable(__FILE__);
    TEST_ASSERT(readable, "File readable check: test file is readable");
    
    // Test: Check non-existent file
    readable = workflow_check_file_readable("/nonexistent/file/path/that/does/not/exist");
    TEST_ASSERT(!readable, "File readable check: non-existent file is not readable");
    
    // Test: Check writable file (should be writable: /tmp)
    bool writable = workflow_check_file_writable("/tmp/test_workflow_write");
    TEST_ASSERT(writable, "File writable check: /tmp is writable");
    
    // Test: File I/O error handling
    Workflow* wf = workflow_create("test", "Test workflow", NULL);
    TEST_ASSERT(wf != NULL, "Create workflow for file I/O error test");
    
    if (wf) {
        WorkflowErrorType error = workflow_handle_file_io_error(wf, "/nonexistent/file", "read");
        TEST_ASSERT(error == WORKFLOW_ERROR_FILE_IO, "File I/O error handling returns correct type");
        TEST_ASSERT(wf->status == WORKFLOW_STATUS_FAILED, "File I/O error sets workflow to failed");
        TEST_ASSERT(wf->error_message != NULL, "File I/O error sets error message");
        TEST_ASSERT(strstr(wf->error_message, "read") != NULL, "File I/O error message includes operation");
        
        const char* error_type = workflow_get_state_value(wf, "last_error_type");
        TEST_ASSERT(error_type != NULL && strcmp(error_type, "file_io") == 0, "File I/O error recorded in state");
        
        workflow_destroy(wf);
    }
}

// ============================================================================
// CREDIT/BUDGET ERROR TESTS
// ============================================================================

void test_credit_exhausted(void) {
    printf("\n=== Testing Credit/Budget Error Handling ===\n");
    
    Workflow* wf = workflow_create("test", "Test workflow", NULL);
    TEST_ASSERT(wf != NULL, "Create workflow for credit error test");
    
    if (wf) {
        // Test: Check budget (should pass if no budget exceeded flag)
        bool budget_ok = workflow_check_budget(wf);
        TEST_ASSERT(budget_ok, "Budget check: budget available by default");
        
        // Test: Set budget exceeded
        workflow_set_state(wf, "budget_exceeded", "true");
        budget_ok = workflow_check_budget(wf);
        TEST_ASSERT(!budget_ok, "Budget check: correctly detects budget exceeded");
        
        // Test: Credit exhausted error handling
        WorkflowErrorType error = workflow_handle_credit_exhausted(wf);
        TEST_ASSERT(error == WORKFLOW_ERROR_CREDIT_EXHAUSTED, "Credit exhausted error handling returns correct type");
        TEST_ASSERT(wf->status == WORKFLOW_STATUS_FAILED, "Credit exhausted sets workflow to failed");
        TEST_ASSERT(wf->error_message != NULL, "Credit exhausted sets error message");
        TEST_ASSERT(strstr(wf->error_message, "Credit exhausted") != NULL, "Credit exhausted message is descriptive");
        
        const char* error_type = workflow_get_state_value(wf, "last_error_type");
        TEST_ASSERT(error_type != NULL && strcmp(error_type, "credit_exhausted") == 0, "Credit exhausted recorded in state");
        
        workflow_destroy(wf);
    }
}

// ============================================================================
// LLM SERVICE DOWNTIME TESTS
// ============================================================================

void test_llm_down(void) {
    printf("\n=== Testing LLM Service Downtime Handling ===\n");
    
    Workflow* wf = workflow_create("test", "Test workflow", NULL);
    TEST_ASSERT(wf != NULL, "Create workflow for LLM down test");
    
    if (wf) {
        // Test: LLM availability check (depends on network)
        bool llm_available = workflow_check_llm_available(PROVIDER_ANTHROPIC);
        printf("  ℹ LLM available: %s\n", llm_available ? "yes" : "no");
        
        // Test: LLM down error handling
        WorkflowErrorType error = workflow_handle_llm_down(wf, PROVIDER_ANTHROPIC);
        TEST_ASSERT(error == WORKFLOW_ERROR_LLM_DOWN, "LLM down error handling returns correct type");
        TEST_ASSERT(wf->status == WORKFLOW_STATUS_PAUSED, "LLM down sets workflow to paused (recoverable)");
        TEST_ASSERT(wf->error_message != NULL, "LLM down sets error message");
        
        const char* error_type = workflow_get_state_value(wf, "last_error_type");
        TEST_ASSERT(error_type != NULL && strcmp(error_type, "llm_down") == 0, "LLM down recorded in state");
        
        const char* provider_unavailable = workflow_get_state_value(wf, "provider_unavailable");
        TEST_ASSERT(provider_unavailable != NULL && strcmp(provider_unavailable, "true") == 0, "Provider unavailable flag set");
        
        workflow_destroy(wf);
    }
}

// ============================================================================
// TOOL EXECUTION ERROR TESTS
// ============================================================================

void test_tool_errors(void) {
    printf("\n=== Testing Tool Execution Error Handling ===\n");
    
    Workflow* wf = workflow_create("test", "Test workflow", NULL);
    TEST_ASSERT(wf != NULL, "Create workflow for tool error test");
    
    if (wf) {
        // Test: Tool error handling
        WorkflowErrorType error = workflow_handle_tool_error(wf, "test_tool", "Tool execution failed: permission denied");
        TEST_ASSERT(error == WORKFLOW_ERROR_TOOL_FAILED, "Tool error handling returns correct type");
        TEST_ASSERT(wf->status == WORKFLOW_STATUS_FAILED, "Tool error sets workflow to failed");
        TEST_ASSERT(wf->error_message != NULL, "Tool error sets error message");
        TEST_ASSERT(strstr(wf->error_message, "test_tool") != NULL, "Tool error message includes tool name");
        
        const char* error_type = workflow_get_state_value(wf, "last_error_type");
        TEST_ASSERT(error_type != NULL && strcmp(error_type, "tool_failed") == 0, "Tool error recorded in state");
        
        const char* failed_tool = workflow_get_state_value(wf, "failed_tool");
        TEST_ASSERT(failed_tool != NULL && strcmp(failed_tool, "test_tool") == 0, "Failed tool name recorded in state");
        
        workflow_destroy(wf);
    }
}

// ============================================================================
// COMPREHENSIVE ERROR HANDLING TESTS
// ============================================================================

void test_comprehensive_error_handling(void) {
    printf("\n=== Testing Comprehensive Error Handling ===\n");
    
    Workflow* wf = workflow_create("test", "Test workflow", NULL);
    TEST_ASSERT(wf != NULL, "Create workflow for comprehensive error test");
    
    if (wf) {
        WorkflowNode* node = workflow_node_create("test_node", NODE_TYPE_ACTION);
        TEST_ASSERT(node != NULL, "Create test node");
        
        if (node) {
            // Test: Timeout error (recoverable)
            bool recoverable = workflow_handle_error(wf, node, WORKFLOW_ERROR_TIMEOUT, "Node execution timeout");
            TEST_ASSERT(recoverable == true, "Timeout error is recoverable");
            TEST_ASSERT(wf->status == WORKFLOW_STATUS_FAILED, "Timeout sets workflow to failed");
            
            // Reset
            wf->status = WORKFLOW_STATUS_RUNNING;
            if (wf->error_message) {
                free(wf->error_message);
                wf->error_message = NULL;
            }
            
            // Test: Network error (recoverable, pauses)
            recoverable = workflow_handle_error(wf, node, WORKFLOW_ERROR_NETWORK, "Network error");
            TEST_ASSERT(recoverable == true, "Network error is recoverable");
            TEST_ASSERT(wf->status == WORKFLOW_STATUS_PAUSED, "Network error pauses workflow");
            
            // Reset
            wf->status = WORKFLOW_STATUS_RUNNING;
            if (wf->error_message) {
                free(wf->error_message);
                wf->error_message = NULL;
            }
            
            // Test: File I/O error (not recoverable)
            recoverable = workflow_handle_error(wf, node, WORKFLOW_ERROR_FILE_IO, "File I/O error");
            TEST_ASSERT(recoverable == false, "File I/O error is not recoverable");
            TEST_ASSERT(wf->status == WORKFLOW_STATUS_FAILED, "File I/O error sets workflow to failed");
            
            // Reset
            wf->status = WORKFLOW_STATUS_RUNNING;
            if (wf->error_message) {
                free(wf->error_message);
                wf->error_message = NULL;
            }
            
            // Test: Credit exhausted (not recoverable)
            recoverable = workflow_handle_error(wf, node, WORKFLOW_ERROR_CREDIT_EXHAUSTED, "Credit exhausted");
            TEST_ASSERT(recoverable == false, "Credit exhausted is not recoverable");
            TEST_ASSERT(wf->status == WORKFLOW_STATUS_FAILED, "Credit exhausted sets workflow to failed");
            
            // Reset
            wf->status = WORKFLOW_STATUS_RUNNING;
            if (wf->error_message) {
                free(wf->error_message);
                wf->error_message = NULL;
            }
            
            // Test: Rate limit (recoverable, pauses)
            recoverable = workflow_handle_error(wf, node, WORKFLOW_ERROR_RATE_LIMIT, "Rate limit exceeded");
            TEST_ASSERT(recoverable == true, "Rate limit error is recoverable");
            TEST_ASSERT(wf->status == WORKFLOW_STATUS_PAUSED, "Rate limit pauses workflow");
            
            // Test: Error timestamp recorded
            const char* error_time = workflow_get_state_value(wf, "last_error_time");
            TEST_ASSERT(error_time != NULL, "Error timestamp recorded");
            
            workflow_node_destroy(node);
        }
        
        workflow_destroy(wf);
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  WORKFLOW ERROR HANDLING TESTS                           ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    test_timeout_check();
    test_network_check();
    test_file_io_errors();
    test_credit_exhausted();
    test_llm_down();
    test_tool_errors();
    test_comprehensive_error_handling();
    
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  TEST RESULTS                                             ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-3d                                            ║\n", tests_passed);
    printf("║  Failed: %-3d                                            ║\n", tests_failed);
    printf("║  Total:  %-3d                                            ║\n", tests_passed + tests_failed);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    return (tests_failed == 0) ? 0 : 1;
}

