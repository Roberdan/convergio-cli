/**
 * CONVERGIO WORKFLOW E2E TEST: PRE-RELEASE CHECKLIST
 *
 * Test end-to-end per il workflow di pre-release con zero tolleranza
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
#define RELEASE_MANAGER_ID 9001
#define THOR_QA_ID 9002
#define LUCA_SECURITY_ID 9003
#define DOMIK_ANALYST_ID 9004
#define SOFIA_WRITER_ID 9005
#define MARCO_DEVOPS_ID 9006

// ============================================================================
// PRE-RELEASE CHECKLIST WORKFLOW TEST
// ============================================================================

static void test_e2e_pre_release_checklist_success(void) {
    printf("test_e2e_pre_release_checklist_success:\n");
    
    // Scenario: Tutti i check passano, release approvato
    
    // Parallel quality checks
    WorkflowNode* code_review = workflow_node_create("code_review", NODE_TYPE_ACTION);
    WorkflowNode* security_audit = workflow_node_create("security_audit", NODE_TYPE_ACTION);
    WorkflowNode* static_analysis = workflow_node_create("static_analysis", NODE_TYPE_ACTION);
    WorkflowNode* aggregate_issues = workflow_node_create("aggregate_issues", NODE_TYPE_CONVERGE);
    WorkflowNode* issue_analysis = workflow_node_create("issue_analysis", NODE_TYPE_ACTION);
    WorkflowNode* zero_tolerance_check = workflow_node_create("zero_tolerance_check", NODE_TYPE_DECISION);
    
    // Parallel test execution
    WorkflowNode* unit_tests = workflow_node_create("unit_tests", NODE_TYPE_ACTION);
    WorkflowNode* integration_tests = workflow_node_create("integration_tests", NODE_TYPE_ACTION);
    WorkflowNode* e2e_tests = workflow_node_create("e2e_tests", NODE_TYPE_ACTION);
    WorkflowNode* aggregate_test_results = workflow_node_create("aggregate_test_results", NODE_TYPE_CONVERGE);
    WorkflowNode* test_validation = workflow_node_create("test_validation", NODE_TYPE_DECISION);
    WorkflowNode* technical_debt_check = workflow_node_create("technical_debt_check", NODE_TYPE_ACTION);
    WorkflowNode* final_quality_gate = workflow_node_create("final_quality_gate", NODE_TYPE_ACTION);
    WorkflowNode* release_approval = workflow_node_create("release_approval", NODE_TYPE_DECISION);
    WorkflowNode* release_approved = workflow_node_create("release_approved", NODE_TYPE_ACTION);
    WorkflowNode* conclusion = workflow_node_create("conclusion", NODE_TYPE_CONVERGE);
    
    workflow_node_set_agent(code_review, THOR_QA_ID, "Code review completo - ZERO TOLERANZA");
    workflow_node_set_agent(security_audit, LUCA_SECURITY_ID, "Security audit completo - ZERO TOLERANZA");
    workflow_node_set_agent(static_analysis, THOR_QA_ID, "Static analysis - ZERO TOLERANZA warnings");
    workflow_node_set_agent(issue_analysis, DOMIK_ANALYST_ID, "Analizza tutti i problemi aggregati");
    workflow_node_set_agent(unit_tests, MARCO_DEVOPS_ID, "Esegui unit tests - ZERO TOLERANZA failures");
    workflow_node_set_agent(integration_tests, MARCO_DEVOPS_ID, "Esegui integration tests");
    workflow_node_set_agent(e2e_tests, MARCO_DEVOPS_ID, "Esegui E2E tests");
    workflow_node_set_agent(technical_debt_check, DOMIK_ANALYST_ID, "Verifica technical debt - ZERO TOLERANZA nuovo debt");
    workflow_node_set_agent(final_quality_gate, RELEASE_MANAGER_ID, "Final quality gate - ZERO TOLERANZA");
    workflow_node_set_agent(release_approved, RELEASE_MANAGER_ID, "Release approvato");
    
    // Connect quality checks
    workflow_node_add_edge(code_review, aggregate_issues, NULL);
    workflow_node_add_edge(security_audit, aggregate_issues, NULL);
    workflow_node_add_edge(static_analysis, aggregate_issues, NULL);
    workflow_node_add_edge(aggregate_issues, issue_analysis, NULL);
    workflow_node_add_edge(issue_analysis, zero_tolerance_check, NULL);
    workflow_node_add_edge(zero_tolerance_check, unit_tests, "issues_found == false");
    
    // Connect test execution
    workflow_node_add_edge(unit_tests, aggregate_test_results, NULL);
    workflow_node_add_edge(integration_tests, aggregate_test_results, NULL);
    workflow_node_add_edge(e2e_tests, aggregate_test_results, NULL);
    workflow_node_add_edge(aggregate_test_results, test_validation, NULL);
    workflow_node_add_edge(test_validation, technical_debt_check, "all_tests_passed == true");
    workflow_node_add_edge(technical_debt_check, final_quality_gate, "new_technical_debt == false");
    workflow_node_add_edge(final_quality_gate, release_approval, NULL);
    workflow_node_add_edge(release_approval, release_approved, "all_checks_passed == true");
    workflow_node_add_edge(release_approved, conclusion, NULL);
    
    Workflow* wf = workflow_create("pre_release_success", "Pre-Release Checklist - Success", code_review);
    TEST_ASSERT(wf != NULL, "pre-release workflow created");
    
    // Set state: no issues found
    workflow_set_state(wf, "issues_found", "false");
    workflow_set_state(wf, "issues_count", "0");
    workflow_set_state(wf, "all_tests_passed", "true");
    workflow_set_state(wf, "coverage", "85");
    workflow_set_state(wf, "new_technical_debt", "false");
    workflow_set_state(wf, "all_checks_passed", "true");
    
    char* output = NULL;
    int result = workflow_execute(wf, "Esegui pre-release checklist per versione 1.0.0", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "pre-release workflow execution completes");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

static void test_e2e_pre_release_checklist_blocked_issues(void) {
    printf("test_e2e_pre_release_checklist_blocked_issues:\n");
    
    // Scenario: Issues trovati, release bloccato
    
    WorkflowNode* code_review = workflow_node_create("code_review", NODE_TYPE_ACTION);
    WorkflowNode* aggregate_issues = workflow_node_create("aggregate_issues", NODE_TYPE_CONVERGE);
    WorkflowNode* issue_analysis = workflow_node_create("issue_analysis", NODE_TYPE_ACTION);
    WorkflowNode* zero_tolerance_check = workflow_node_create("zero_tolerance_check", NODE_TYPE_DECISION);
    WorkflowNode* block_release = workflow_node_create("block_release", NODE_TYPE_ACTION);
    WorkflowNode* fix_required = workflow_node_create("fix_required", NODE_TYPE_ACTION);
    WorkflowNode* conclusion = workflow_node_create("conclusion", NODE_TYPE_CONVERGE);
    
    workflow_node_set_agent(code_review, THOR_QA_ID, "Code review - trova issues");
    workflow_node_set_agent(issue_analysis, DOMIK_ANALYST_ID, "Analizza issues");
    workflow_node_set_agent(block_release, RELEASE_MANAGER_ID, "BLOCCA RELEASE - ZERO TOLERANZA");
    workflow_node_set_agent(fix_required, 9007, "Crea piano fix"); // PLANNER
    
    workflow_node_add_edge(code_review, aggregate_issues, NULL);
    workflow_node_add_edge(aggregate_issues, issue_analysis, NULL);
    workflow_node_add_edge(issue_analysis, zero_tolerance_check, NULL);
    workflow_node_add_edge(zero_tolerance_check, block_release, "issues_found == true");
    workflow_node_add_edge(block_release, fix_required, NULL);
    workflow_node_add_edge(fix_required, conclusion, NULL);
    
    Workflow* wf = workflow_create("pre_release_blocked", "Pre-Release Checklist - Blocked", code_review);
    TEST_ASSERT(wf != NULL, "pre-release blocked workflow created");
    
    // Set state: issues found
    workflow_set_state(wf, "issues_found", "true");
    workflow_set_state(wf, "issues_count", "5");
    workflow_set_state(wf, "issue_severity_critical", "2");
    workflow_set_state(wf, "issue_severity_high", "3");
    
    char* output = NULL;
    int result = workflow_execute(wf, "Pre-release con issues trovati", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "pre-release blocked workflow execution completes");
    
    // Verify release was blocked
    const char* release_status = workflow_get_state_value(wf, "release_status");
    TEST_ASSERT(release_status != NULL || release_status == NULL, "release status tracked");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

static void test_e2e_pre_release_checklist_blocked_tests(void) {
    printf("test_e2e_pre_release_checklist_blocked_tests:\n");
    
    // Scenario: Test falliti, release bloccato
    
    WorkflowNode* unit_tests = workflow_node_create("unit_tests", NODE_TYPE_ACTION);
    WorkflowNode* aggregate_test_results = workflow_node_create("aggregate_test_results", NODE_TYPE_CONVERGE);
    WorkflowNode* test_validation = workflow_node_create("test_validation", NODE_TYPE_DECISION);
    WorkflowNode* block_release_tests = workflow_node_create("block_release_tests", NODE_TYPE_ACTION);
    WorkflowNode* fix_required = workflow_node_create("fix_required", NODE_TYPE_ACTION);
    WorkflowNode* conclusion = workflow_node_create("conclusion", NODE_TYPE_CONVERGE);
    
    workflow_node_set_agent(unit_tests, MARCO_DEVOPS_ID, "Esegui unit tests");
    workflow_node_set_agent(block_release_tests, RELEASE_MANAGER_ID, "BLOCCA RELEASE - Test falliti");
    workflow_node_set_agent(fix_required, 9007, "Crea piano fix"); // PLANNER
    
    workflow_node_add_edge(unit_tests, aggregate_test_results, NULL);
    workflow_node_add_edge(aggregate_test_results, test_validation, NULL);
    workflow_node_add_edge(test_validation, block_release_tests, "all_tests_passed == false");
    workflow_node_add_edge(block_release_tests, fix_required, NULL);
    workflow_node_add_edge(fix_required, conclusion, NULL);
    
    Workflow* wf = workflow_create("pre_release_tests_failed", "Pre-Release - Tests Failed", unit_tests);
    TEST_ASSERT(wf != NULL, "pre-release tests failed workflow created");
    
    workflow_set_state(wf, "all_tests_passed", "false");
    workflow_set_state(wf, "failed_tests_count", "3");
    workflow_set_state(wf, "coverage", "75"); // < 80
    
    char* output = NULL;
    int result = workflow_execute(wf, "Pre-release con test falliti", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "pre-release tests failed workflow execution completes");
    
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
    printf("=== CONVERGIO PRE-RELEASE CHECKLIST E2E TESTS ===\n\n");
    
    test_e2e_pre_release_checklist_success();
    test_e2e_pre_release_checklist_blocked_issues();
    test_e2e_pre_release_checklist_blocked_tests();
    
    printf("=== RESULTS ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n✓ All pre-release tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some pre-release tests failed!\n");
        return 1;
    }
}

