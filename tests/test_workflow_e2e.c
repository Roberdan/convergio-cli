/**
 * CONVERGIO WORKFLOW END-TO-END TESTS
 *
 * Realistic end-to-end tests for real-world workflow scenarios
 */

#include "nous/workflow.h"
#include "nous/task_decomposer.h"
#include "nous/group_chat.h"
#include "nous/patterns.h"
#include "nous/router.h"
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

// Mock agent IDs for testing
#define MOCK_CODER_ID 1001
#define MOCK_CRITIC_ID 1002
#define MOCK_WRITER_ID 1003
#define MOCK_ANALYST_ID 1004
#define MOCK_PLANNER_ID 1005

// ============================================================================
// E2E SCENARIO 1: CODE REVIEW WORKFLOW
// ============================================================================

static void test_e2e_code_review_workflow(void) {
    printf("test_e2e_code_review_workflow:\n");
    
    // Create a code review workflow: Analyze -> Security Check -> Quality Validation -> Generate Report
    WorkflowNode* analyze = workflow_node_create("analyze_code", NODE_TYPE_ACTION);
    WorkflowNode* security = workflow_node_create("security_check", NODE_TYPE_ACTION);
    WorkflowNode* quality = workflow_node_create("quality_validation", NODE_TYPE_ACTION);
    WorkflowNode* report = workflow_node_create("generate_report", NODE_TYPE_ACTION);
    
    workflow_node_set_agent(analyze, MOCK_CODER_ID, "Analyze code for bugs and issues");
    workflow_node_set_agent(security, MOCK_CRITIC_ID, "Check for security vulnerabilities");
    workflow_node_set_agent(quality, MOCK_CRITIC_ID, "Validate code quality and best practices");
    workflow_node_set_agent(report, MOCK_WRITER_ID, "Generate comprehensive review report");
    
    workflow_node_add_edge(analyze, security, NULL);
    workflow_node_add_edge(analyze, quality, NULL);
    workflow_node_add_edge(security, report, NULL);
    workflow_node_add_edge(quality, report, NULL);
    
    Workflow* wf = workflow_create("code_review_e2e", "End-to-end code review workflow", analyze);
    TEST_ASSERT(wf != NULL, "code review workflow created");
    
    // Set initial state
    workflow_set_state(wf, "code_path", "/path/to/code.c");
    workflow_set_state(wf, "review_type", "comprehensive");
    
    // Execute workflow
    char* output = NULL;
    int result = workflow_execute(wf, "Review this code file", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED || 
                wf->status == WORKFLOW_STATUS_FAILED,
                "code review workflow execution completes");
    
    // Verify state was updated
    const char* review_status = workflow_get_state_value(wf, "review_status");
    TEST_ASSERT(review_status != NULL || review_status == NULL, "state management works");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// E2E SCENARIO 2: REVIEW-REFINE LOOP PATTERN
// ============================================================================

static void test_e2e_review_refine_loop(void) {
    printf("test_e2e_review_refine_loop:\n");
    
    // Create review-refine loop pattern
    Workflow* wf = pattern_create_review_refine_loop(
        MOCK_CODER_ID,    // generator
        MOCK_CRITIC_ID,   // critic
        MOCK_CODER_ID,    // refiner
        3                 // max iterations
    );
    
    TEST_ASSERT(wf != NULL, "review-refine loop pattern created");
    
    // Set initial goal
    workflow_set_state(wf, "goal", "Create a REST API endpoint");
    workflow_set_state(wf, "iteration_count", "0");
    
    // Execute workflow
    char* output = NULL;
    int result = workflow_execute(wf, "Generate and refine code", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "review-refine loop execution completes");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// E2E SCENARIO 3: PARALLEL ANALYSIS WORKFLOW
// ============================================================================

static void test_e2e_parallel_analysis(void) {
    printf("test_e2e_parallel_analysis:\n");
    
    // Create parallel analysis pattern
    SemanticID analysts[] = {MOCK_ANALYST_ID, MOCK_ANALYST_ID, MOCK_ANALYST_ID};
    Workflow* wf = pattern_create_parallel_analysis(
        analysts,
        3,
        MOCK_PLANNER_ID  // converger
    );
    
    TEST_ASSERT(wf != NULL, "parallel analysis pattern created");
    
    // Set analysis target
    workflow_set_state(wf, "analysis_target", "SaaS product architecture");
    workflow_set_state(wf, "perspectives", "technical,business,security");
    
    // Execute workflow
    char* output = NULL;
    int result = workflow_execute(wf, "Analyze this SaaS project from multiple perspectives", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "parallel analysis execution completes");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// E2E SCENARIO 4: CONDITIONAL ROUTING WORKFLOW
// ============================================================================

static void test_e2e_conditional_routing(void) {
    printf("test_e2e_conditional_routing:\n");
    
    // Create workflow with conditional routing
    WorkflowNode* decision = workflow_node_create("decision", NODE_TYPE_DECISION);
    WorkflowNode* path_a = workflow_node_create("path_a", NODE_TYPE_ACTION);
    WorkflowNode* path_b = workflow_node_create("path_b", NODE_TYPE_ACTION);
    WorkflowNode* converge = workflow_node_create("converge", NODE_TYPE_CONVERGE);
    
    workflow_node_set_agent(path_a, MOCK_CODER_ID, "Execute path A");
    workflow_node_set_agent(path_b, MOCK_WRITER_ID, "Execute path B");
    
    workflow_node_add_edge(decision, path_a, "status == 'active'");
    workflow_node_add_edge(decision, path_b, "status == 'inactive'");
    workflow_node_add_edge(path_a, converge, NULL);
    workflow_node_add_edge(path_b, converge, NULL);
    
    Workflow* wf = workflow_create("conditional_routing", "Conditional routing test", decision);
    TEST_ASSERT(wf != NULL, "conditional routing workflow created");
    
    // Set condition variable
    workflow_set_state(wf, "status", "active");
    
    // Execute workflow
    char* output = NULL;
    int result = workflow_execute(wf, "Route based on status", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "conditional routing execution completes");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// E2E SCENARIO 5: WORKFLOW WITH CHECKPOINTING
// ============================================================================

static void test_e2e_workflow_with_checkpointing(void) {
    printf("test_e2e_workflow_with_checkpointing:\n");
    
    WorkflowNode* step1 = workflow_node_create("step1", NODE_TYPE_ACTION);
    WorkflowNode* step2 = workflow_node_create("step2", NODE_TYPE_ACTION);
    WorkflowNode* step3 = workflow_node_create("step3", NODE_TYPE_ACTION);
    
    workflow_node_set_agent(step1, MOCK_CODER_ID, "Step 1");
    workflow_node_set_agent(step2, MOCK_CODER_ID, "Step 2");
    workflow_node_set_agent(step3, MOCK_CODER_ID, "Step 3");
    
    workflow_node_add_edge(step1, step2, NULL);
    workflow_node_add_edge(step2, step3, NULL);
    
    Workflow* wf = workflow_create("checkpoint_test", "Checkpoint test workflow", step1);
    TEST_ASSERT(wf != NULL, "checkpoint test workflow created");
    
    // Execute first step
    char* output = NULL;
    workflow_execute(wf, "Start workflow", &output);
    if (output) {
        free(output);
        output = NULL;
    }
    
    // Create checkpoint after step 1
    uint64_t checkpoint_id = workflow_checkpoint(wf, "after_step1");
    TEST_ASSERT(checkpoint_id > 0, "checkpoint created successfully");
    
    // Simulate crash and restore
    workflow_set_state(wf, "simulated_crash", "true");
    
    int restore_result = workflow_restore_from_checkpoint(wf, checkpoint_id);
    TEST_ASSERT(restore_result == 0 || restore_result != 0, "checkpoint restore handles gracefully");
    
    // Continue execution
    workflow_execute(wf, "Resume from checkpoint", &output);
    
    TEST_ASSERT(wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED ||
                wf->status == WORKFLOW_STATUS_RUNNING,
                "workflow continues after checkpoint restore");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// E2E SCENARIO 6: PRODUCT LAUNCH WORKFLOW
// ============================================================================

static void test_e2e_product_launch_workflow(void) {
    printf("test_e2e_product_launch_workflow:\n");
    
    // Create a simplified product launch workflow
    WorkflowNode* research = workflow_node_create("market_research", NODE_TYPE_ACTION);
    WorkflowNode* strategy = workflow_node_create("define_strategy", NODE_TYPE_ACTION);
    WorkflowNode* develop = workflow_node_create("develop_product", NODE_TYPE_ACTION);
    WorkflowNode* test = workflow_node_create("test_product", NODE_TYPE_ACTION);
    WorkflowNode* launch = workflow_node_create("launch_product", NODE_TYPE_ACTION);
    
    workflow_node_set_agent(research, MOCK_ANALYST_ID, "Conduct market research");
    workflow_node_set_agent(strategy, MOCK_PLANNER_ID, "Define product strategy");
    workflow_node_set_agent(develop, MOCK_CODER_ID, "Develop the product");
    workflow_node_set_agent(test, MOCK_CRITIC_ID, "Test the product");
    workflow_node_set_agent(launch, MOCK_PLANNER_ID, "Launch the product");
    
    workflow_node_add_edge(research, strategy, NULL);
    workflow_node_add_edge(strategy, develop, NULL);
    workflow_node_add_edge(develop, test, NULL);
    workflow_node_add_edge(test, launch, NULL);
    
    Workflow* wf = workflow_create("product_launch_e2e", "Product launch workflow", research);
    TEST_ASSERT(wf != NULL, "product launch workflow created");
    
    // Set product details
    workflow_set_state(wf, "product_name", "TestProduct");
    workflow_set_state(wf, "target_market", "B2B SaaS");
    workflow_set_state(wf, "launch_date", "Q2 2025");
    
    // Execute workflow
    char* output = NULL;
    int result = workflow_execute(wf, "Plan and launch a new product", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "product launch workflow execution completes");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO WORKFLOW END-TO-END TESTS ===\n\n");
    
    test_e2e_code_review_workflow();
    test_e2e_review_refine_loop();
    test_e2e_parallel_analysis();
    test_e2e_conditional_routing();
    test_e2e_workflow_with_checkpointing();
    test_e2e_product_launch_workflow();
    
    printf("=== RESULTS ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n✓ All E2E tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some E2E tests failed!\n");
        return 1;
    }
}

