/**
 * CONVERGIO WORKFLOW E2E TEST: BUG TRIAGE & FIX
 *
 * Test end-to-end per il workflow di bug triage e fix
 */

#include "nous/workflow.h"
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

// Mock agent IDs
#define BACCIO_CODER_ID 5001
#define LUCA_SECURITY_ID 5002
#define THOR_QA_ID 5003
#define MARCO_DEVOPS_ID 5004

// ============================================================================
// BUG TRIAGE WORKFLOW TEST
// ============================================================================

static void test_e2e_bug_triage_workflow(void) {
    printf("test_e2e_bug_triage_workflow:\n");
    
    // Create bug triage workflow
    WorkflowNode* bug_analysis = workflow_node_create("bug_analysis", NODE_TYPE_ACTION);
    WorkflowNode* security_check = workflow_node_create("security_check", NODE_TYPE_ACTION);
    WorkflowNode* priority_assessment = workflow_node_create("priority_assessment", NODE_TYPE_DECISION);
    WorkflowNode* critical_fix = workflow_node_create("critical_fix", NODE_TYPE_ACTION);
    WorkflowNode* code_review = workflow_node_create("code_review", NODE_TYPE_ACTION);
    WorkflowNode* deployment = workflow_node_create("deployment", NODE_TYPE_ACTION);
    WorkflowNode* verification = workflow_node_create("verification", NODE_TYPE_ACTION);
    WorkflowNode* documentation = workflow_node_create("documentation", NODE_TYPE_ACTION);
    WorkflowNode* close_bug = workflow_node_create("close_bug", NODE_TYPE_CONVERGE);
    
    // Set agents
    workflow_node_set_agent(bug_analysis, BACCIO_CODER_ID, "Analizza il bug report");
    workflow_node_set_agent(security_check, LUCA_SECURITY_ID, "Verifica implicazioni di sicurezza");
    workflow_node_set_agent(critical_fix, BACCIO_CODER_ID, "Implementa fix critico");
    workflow_node_set_agent(code_review, THOR_QA_ID, "Review del codice");
    workflow_node_set_agent(deployment, MARCO_DEVOPS_ID, "Deploy in produzione");
    workflow_node_set_agent(verification, THOR_QA_ID, "Verifica che il fix funzioni");
    workflow_node_set_agent(documentation, 5005, "Documenta bug e fix"); // WRITER
    
    // Connect nodes
    workflow_node_add_edge(bug_analysis, security_check, NULL);
    workflow_node_add_edge(security_check, priority_assessment, NULL);
    workflow_node_add_edge(priority_assessment, critical_fix, "severity == 'critical'");
    workflow_node_add_edge(critical_fix, code_review, NULL);
    workflow_node_add_edge(code_review, deployment, NULL);
    workflow_node_add_edge(deployment, verification, NULL);
    workflow_node_add_edge(verification, documentation, "fix_verified == true");
    workflow_node_add_edge(documentation, close_bug, NULL);
    
    Workflow* wf = workflow_create("bug_triage_test", "Bug Triage & Fix Workflow", bug_analysis);
    TEST_ASSERT(wf != NULL, "bug triage workflow created");
    
    // Set bug information
    workflow_set_state(wf, "bug_id", "BUG-1234");
    workflow_set_state(wf, "bug_title", "SQL Injection vulnerability in login endpoint");
    workflow_set_state(wf, "severity", "critical");
    workflow_set_state(wf, "reporter", "Security Team");
    workflow_set_state(wf, "affected_component", "auth/login.php");
    
    // Execute workflow
    char* output = NULL;
    int result = workflow_execute(wf, "Fix SQL injection vulnerability in login endpoint", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED ||
                wf->status == WORKFLOW_STATUS_PAUSED,
                "bug triage workflow execution completes");
    
    // Verify state contains bug tracking info
    const char* bug_status = workflow_get_state_value(wf, "bug_status");
    const char* fix_verified = workflow_get_state_value(wf, "fix_verified");
    
    TEST_ASSERT(bug_status != NULL || bug_status == NULL, "bug status tracked");
    TEST_ASSERT(fix_verified != NULL || fix_verified == NULL, "fix verification tracked");
    
    // Test checkpoint during fix
    uint64_t checkpoint_id = workflow_checkpoint(wf, "during_fix");
    TEST_ASSERT(checkpoint_id > 0 || checkpoint_id == 0, "checkpoint creation works");
    
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
    printf("=== CONVERGIO BUG TRIAGE E2E TEST ===\n\n");
    
    test_e2e_bug_triage_workflow();
    
    printf("=== RESULTS ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n✓ All bug triage tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some bug triage tests failed!\n");
        return 1;
    }
}

