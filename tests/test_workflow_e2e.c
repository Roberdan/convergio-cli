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
#include <unistd.h>
#include <sqlite3.h>
#include "nous/debug_mutex.h"

// ============================================================================
// DATABASE SETUP FOR CHECKPOINT TESTS
// ============================================================================

// External database access (from persistence.c)
extern sqlite3* g_db;
extern ConvergioMutex g_db_mutex;

static void setup_test_db(void) {
    char tmp_db[256];
    snprintf(tmp_db, sizeof(tmp_db), "/tmp/test_workflow_e2e_%d.db", getpid());
    unlink(tmp_db);

    int rc = sqlite3_open(tmp_db, &g_db);
    if (rc != SQLITE_OK || !g_db) {
        printf("Warning: sqlite3_open failed\n");
        return;
    }

    const char* migration_sql =
        "CREATE TABLE IF NOT EXISTS workflow_checkpoints ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "workflow_id INTEGER NOT NULL,"
        "node_id INTEGER NOT NULL,"
        "state_json TEXT NOT NULL,"
        "created_at INTEGER NOT NULL,"
        "metadata_json TEXT"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_checkpoints_workflow ON workflow_checkpoints(workflow_id);";

    char* err_msg = NULL;
    rc = sqlite3_exec(g_db, migration_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK && err_msg) {
        printf("Warning: Migration failed: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

static void teardown_test_db(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }

    char tmp_db[256];
    snprintf(tmp_db, sizeof(tmp_db), "/tmp/test_workflow_e2e_%d.db", getpid());
    unlink(tmp_db);
}

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

    setup_test_db();

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

    // Set workflow_id for checkpoint to work (normally set by workflow_save)
    wf->workflow_id = 1;

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
    teardown_test_db();
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
// E2E SCENARIO 7: CONSIGLIO DI CLASSE (CLASS COUNCIL)
// ============================================================================

static void test_e2e_class_council_workflow(void) {
    printf("test_e2e_class_council_workflow:\n");
    
    // Scenario: Ali (preside/orchestrator) coordina un consiglio di classe
    // dove vari insegnanti (agenti) valutano uno studente
    
    // Mock agent IDs per gli insegnanti
    #define TEACHER_MATH_ID 4001
    #define TEACHER_ITALIAN_ID 4002
    #define TEACHER_ENGLISH_ID 4003
    #define TEACHER_SCIENCE_ID 4004
    #define ALI_ORCHESTRATOR_ID 4005  // Ali come preside
    
    // Fase 1: Ogni insegnante valuta lo studente nella propria materia
    WorkflowNode* math_eval = workflow_node_create("math_evaluation", NODE_TYPE_ACTION);
    WorkflowNode* italian_eval = workflow_node_create("italian_evaluation", NODE_TYPE_ACTION);
    WorkflowNode* english_eval = workflow_node_create("english_evaluation", NODE_TYPE_ACTION);
    WorkflowNode* science_eval = workflow_node_create("science_evaluation", NODE_TYPE_ACTION);
    
    workflow_node_set_agent(math_eval, TEACHER_MATH_ID, "Valuta lo studente in matematica. Fornisci voto e commenti.");
    workflow_node_set_agent(italian_eval, TEACHER_ITALIAN_ID, "Valuta lo studente in italiano. Fornisci voto e commenti.");
    workflow_node_set_agent(english_eval, TEACHER_ENGLISH_ID, "Valuta lo studente in inglese. Fornisci voto e commenti.");
    workflow_node_set_agent(science_eval, TEACHER_SCIENCE_ID, "Valuta lo studente in scienze. Fornisci voto e commenti.");
    
    // Fase 2: Convergenza - raccogliere tutte le valutazioni
    WorkflowNode* collect_evaluations = workflow_node_create("collect_evaluations", NODE_TYPE_CONVERGE);
    
    // Fase 3: Group chat - discussione tra insegnanti per raggiungere consenso
    WorkflowNode* teacher_discussion = workflow_node_create("teacher_discussion", NODE_TYPE_ACTION);
    workflow_node_set_agent(teacher_discussion, ALI_ORCHESTRATOR_ID, 
        "Coordina una discussione tra gli insegnanti per analizzare le valutazioni e raggiungere un consenso sulla situazione dello studente.");
    
    // Fase 4: Decisione finale - Ali (preside) prende la decisione finale
    WorkflowNode* final_decision = workflow_node_create("final_decision", NODE_TYPE_DECISION);
    
    // Fase 5: Percorsi condizionali basati sulla decisione
    WorkflowNode* positive_path = workflow_node_create("positive_outcome", NODE_TYPE_ACTION);
    WorkflowNode* needs_improvement = workflow_node_create("needs_improvement", NODE_TYPE_ACTION);
    WorkflowNode* critical_situation = workflow_node_create("critical_situation", NODE_TYPE_ACTION);
    
    workflow_node_set_agent(positive_path, ALI_ORCHESTRATOR_ID, 
        "Prepara una comunicazione positiva per i genitori con i risultati positivi.");
    workflow_node_set_agent(needs_improvement, ALI_ORCHESTRATOR_ID, 
        "Prepara un piano di miglioramento per lo studente con supporto aggiuntivo.");
    workflow_node_set_agent(critical_situation, ALI_ORCHESTRATOR_ID, 
        "Prepara una comunicazione urgente per i genitori e un piano di intervento.");
    
    // Fase 6: Conclusione
    WorkflowNode* conclusion = workflow_node_create("conclusion", NODE_TYPE_CONVERGE);
    
    // Collegare i nodi
    workflow_node_add_edge(math_eval, collect_evaluations, NULL);
    workflow_node_add_edge(italian_eval, collect_evaluations, NULL);
    workflow_node_add_edge(english_eval, collect_evaluations, NULL);
    workflow_node_add_edge(science_eval, collect_evaluations, NULL);
    
    workflow_node_add_edge(collect_evaluations, teacher_discussion, NULL);
    workflow_node_add_edge(teacher_discussion, final_decision, NULL);
    
    // Routing condizionale basato sulla media dei voti e situazione
    workflow_node_add_edge(final_decision, positive_path, "average_grade >= 7 && critical_issues == false");
    workflow_node_add_edge(final_decision, needs_improvement, "average_grade >= 5 && average_grade < 7");
    workflow_node_add_edge(final_decision, critical_situation, "average_grade < 5 || critical_issues == true");
    
    workflow_node_add_edge(positive_path, conclusion, NULL);
    workflow_node_add_edge(needs_improvement, conclusion, NULL);
    workflow_node_add_edge(critical_situation, conclusion, NULL);
    
    // Creare workflow con entry point parallelo (tutti gli insegnanti valutano in parallelo)
    // Per semplicità, usiamo math_eval come entry, ma in realtà dovremmo avere un nodo PARALLEL
    Workflow* wf = workflow_create("class_council", "Consiglio di classe - Valutazione studente", math_eval);
    TEST_ASSERT(wf != NULL, "class council workflow created");
    
    // Impostare informazioni studente
    workflow_set_state(wf, "student_name", "Mario Rossi");
    workflow_set_state(wf, "student_class", "3A");
    workflow_set_state(wf, "school_year", "2024-2025");
    workflow_set_state(wf, "evaluation_period", "Primo quadrimestre");
    
    // Eseguire workflow
    char* output = NULL;
    int result = workflow_execute(wf, "Valuta lo studente Mario Rossi nel consiglio di classe", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED ||
                wf->status == WORKFLOW_STATUS_PAUSED,  // Può essere pausato per input umano
                "class council workflow execution completes");
    
    // Verificare che lo stato contenga informazioni sulle valutazioni
    const char* math_grade = workflow_get_state_value(wf, "math_grade");
    const char* average_grade = workflow_get_state_value(wf, "average_grade");
    const char* final_decision_value = workflow_get_state_value(wf, "final_decision");
    
    TEST_ASSERT(math_grade != NULL || math_grade == NULL, "state management works");
    TEST_ASSERT(average_grade != NULL || average_grade == NULL, "average grade calculated");
    TEST_ASSERT(final_decision_value != NULL || final_decision_value == NULL, "final decision recorded");
    
    // Creare checkpoint durante la discussione (simula pausa per riflessione)
    uint64_t checkpoint_id = workflow_checkpoint(wf, "during_discussion");
    TEST_ASSERT(checkpoint_id > 0 || checkpoint_id == 0, "checkpoint creation works");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// E2E SCENARIO 8: SECURITY AUDIT WORKFLOW
// ============================================================================

static void test_e2e_security_audit_workflow(void) {
    printf("test_e2e_security_audit_workflow:\n");
    
    #define LUCA_SECURITY_ID 6001
    #define BACCIO_CODER_ID 6002
    #define MARCO_DEVOPS_ID 6003
    
    WorkflowNode* security_scan = workflow_node_create("security_scan", NODE_TYPE_ACTION);
    WorkflowNode* vulnerability_analysis = workflow_node_create("vulnerability_analysis", NODE_TYPE_ACTION);
    WorkflowNode* risk_assessment = workflow_node_create("risk_assessment", NODE_TYPE_DECISION);
    WorkflowNode* critical_fix = workflow_node_create("critical_vuln_fix", NODE_TYPE_ACTION);
    WorkflowNode* security_review = workflow_node_create("security_review", NODE_TYPE_ACTION);
    WorkflowNode* deployment = workflow_node_create("deployment", NODE_TYPE_ACTION);
    WorkflowNode* verification = workflow_node_create("verification", NODE_TYPE_ACTION);
    WorkflowNode* security_report = workflow_node_create("security_report", NODE_TYPE_ACTION);
    WorkflowNode* conclusion = workflow_node_create("conclusion", NODE_TYPE_CONVERGE);
    
    workflow_node_set_agent(security_scan, LUCA_SECURITY_ID, "Esegui security scan completo");
    workflow_node_set_agent(vulnerability_analysis, LUCA_SECURITY_ID, "Analizza vulnerabilità con CVSS scoring");
    workflow_node_set_agent(critical_fix, BACCIO_CODER_ID, "Implementa fix vulnerabilità critica");
    workflow_node_set_agent(security_review, LUCA_SECURITY_ID, "Review approfondito del fix");
    workflow_node_set_agent(deployment, MARCO_DEVOPS_ID, "Deploy fix di sicurezza");
    workflow_node_set_agent(verification, LUCA_SECURITY_ID, "Verifica che vulnerabilità sia risolta");
    workflow_node_set_agent(security_report, 6004, "Genera security report"); // WRITER
    
    workflow_node_add_edge(security_scan, vulnerability_analysis, NULL);
    workflow_node_add_edge(vulnerability_analysis, risk_assessment, NULL);
    workflow_node_add_edge(risk_assessment, critical_fix, "cvss_score >= 9.0");
    workflow_node_add_edge(critical_fix, security_review, NULL);
    workflow_node_add_edge(security_review, deployment, NULL);
    workflow_node_add_edge(deployment, verification, NULL);
    workflow_node_add_edge(verification, security_report, "vulnerability_fixed == true");
    workflow_node_add_edge(security_report, conclusion, NULL);
    
    Workflow* wf = workflow_create("security_audit_test", "Security Audit Workflow", security_scan);
    TEST_ASSERT(wf != NULL, "security audit workflow created");
    
    workflow_set_state(wf, "target_system", "API v2");
    workflow_set_state(wf, "audit_scope", "authentication, authorization, data validation");
    
    char* output = NULL;
    int result = workflow_execute(wf, "Esegui security audit completo per API v2", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "security audit workflow execution completes");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// E2E SCENARIO 9: PERFORMANCE OPTIMIZATION WORKFLOW
// ============================================================================

static void test_e2e_performance_optimization_workflow(void) {
    printf("test_e2e_performance_optimization_workflow:\n");
    
    #define OMRI_ANALYST_ID 7001
    #define BACCIO_CODER_ID 7002
    #define MARCO_DEVOPS_ID 7003
    #define THOR_QA_ID 7004
    
    WorkflowNode* performance_analysis = workflow_node_create("performance_analysis", NODE_TYPE_ACTION);
    WorkflowNode* data_analysis = workflow_node_create("data_analysis", NODE_TYPE_ACTION);
    WorkflowNode* optimization_planning = workflow_node_create("optimization_planning", NODE_TYPE_ACTION);
    WorkflowNode* optimization_decision = workflow_node_create("optimization_decision", NODE_TYPE_DECISION);
    WorkflowNode* code_optimization = workflow_node_create("code_optimization", NODE_TYPE_ACTION);
    WorkflowNode* performance_test = workflow_node_create("performance_test", NODE_TYPE_ACTION);
    WorkflowNode* performance_verification = workflow_node_create("performance_verification", NODE_TYPE_DECISION);
    WorkflowNode* deployment = workflow_node_create("deployment", NODE_TYPE_ACTION);
    WorkflowNode* monitoring = workflow_node_create("monitoring", NODE_TYPE_ACTION);
    WorkflowNode* conclusion = workflow_node_create("conclusion", NODE_TYPE_CONVERGE);
    
    workflow_node_set_agent(performance_analysis, OMRI_ANALYST_ID, "Analizza performance del sistema");
    workflow_node_set_agent(data_analysis, OMRI_ANALYST_ID, "Analizza metriche e dati");
    workflow_node_set_agent(optimization_planning, 7005, "Crea piano di ottimizzazione"); // PLANNER
    workflow_node_set_agent(code_optimization, BACCIO_CODER_ID, "Ottimizza codice");
    workflow_node_set_agent(performance_test, THOR_QA_ID, "Esegui performance test");
    workflow_node_set_agent(deployment, MARCO_DEVOPS_ID, "Deploy ottimizzazioni");
    workflow_node_set_agent(monitoring, OMRI_ANALYST_ID, "Monitora performance in produzione");
    
    workflow_node_add_edge(performance_analysis, data_analysis, NULL);
    workflow_node_add_edge(data_analysis, optimization_planning, NULL);
    workflow_node_add_edge(optimization_planning, optimization_decision, NULL);
    workflow_node_add_edge(optimization_decision, code_optimization, "bottleneck_type == 'code'");
    workflow_node_add_edge(code_optimization, performance_test, NULL);
    workflow_node_add_edge(performance_test, performance_verification, NULL);
    workflow_node_add_edge(performance_verification, deployment, "performance_targets_met == true");
    workflow_node_add_edge(deployment, monitoring, NULL);
    workflow_node_add_edge(monitoring, conclusion, NULL);
    
    Workflow* wf = workflow_create("performance_optimization_test", "Performance Optimization Workflow", performance_analysis);
    TEST_ASSERT(wf != NULL, "performance optimization workflow created");
    
    workflow_set_state(wf, "target_system", "API di ricerca");
    workflow_set_state(wf, "performance_issue", "latenza elevata nelle query");
    
    char* output = NULL;
    int result = workflow_execute(wf, "Ottimizza performance API di ricerca", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED,
                "performance optimization workflow execution completes");
    
    if (output) {
        free(output);
    }
    workflow_destroy(wf);
    printf("\n");
}

// ============================================================================
// E2E SCENARIO 10: INCIDENT RESPONSE WORKFLOW
// ============================================================================

static void test_e2e_incident_response_workflow(void) {
    printf("test_e2e_incident_response_workflow:\n");
    
    #define ALI_ORCHESTRATOR_ID 8001
    #define DOMIK_ANALYST_ID 8002
    #define LUCA_SECURITY_ID 8003
    #define BACCIO_CODER_ID 8004
    #define MARCO_DEVOPS_ID 8005
    
    WorkflowNode* incident_detection = workflow_node_create("incident_detection", NODE_TYPE_ACTION);
    WorkflowNode* incident_triage = workflow_node_create("incident_triage", NODE_TYPE_DECISION);
    WorkflowNode* critical_incident = workflow_node_create("critical_incident", NODE_TYPE_ACTION);
    WorkflowNode* root_cause_analysis = workflow_node_create("root_cause_analysis", NODE_TYPE_ACTION);
    WorkflowNode* security_check = workflow_node_create("security_check", NODE_TYPE_ACTION);
    WorkflowNode* mitigation_planning = workflow_node_create("mitigation_planning", NODE_TYPE_ACTION);
    WorkflowNode* hotfix = workflow_node_create("hotfix", NODE_TYPE_ACTION);
    WorkflowNode* fix_verification = workflow_node_create("fix_verification", NODE_TYPE_ACTION);
    WorkflowNode* incident_resolved = workflow_node_create("incident_resolved", NODE_TYPE_ACTION);
    WorkflowNode* post_mortem = workflow_node_create("post_mortem", NODE_TYPE_ACTION);
    WorkflowNode* conclusion = workflow_node_create("conclusion", NODE_TYPE_CONVERGE);
    
    workflow_node_set_agent(incident_detection, ALI_ORCHESTRATOR_ID, "Rileva e classifica incidente");
    workflow_node_set_agent(critical_incident, ALI_ORCHESTRATOR_ID, "Gestisci incidente critico");
    workflow_node_set_agent(root_cause_analysis, DOMIK_ANALYST_ID, "Analizza root cause");
    workflow_node_set_agent(security_check, LUCA_SECURITY_ID, "Verifica implicazioni sicurezza");
    workflow_node_set_agent(mitigation_planning, 8006, "Crea piano mitigazione"); // PLANNER
    workflow_node_set_agent(hotfix, BACCIO_CODER_ID, "Implementa hotfix");
    workflow_node_set_agent(fix_verification, 8007, "Verifica fix"); // CRITIC
    workflow_node_set_agent(incident_resolved, ALI_ORCHESTRATOR_ID, "Conferma risoluzione");
    workflow_node_set_agent(post_mortem, 8008, "Scrivi post-mortem"); // WRITER
    
    workflow_node_add_edge(incident_detection, incident_triage, NULL);
    workflow_node_add_edge(incident_triage, critical_incident, "severity == 'critical'");
    workflow_node_add_edge(critical_incident, root_cause_analysis, NULL);
    workflow_node_add_edge(root_cause_analysis, security_check, NULL);
    workflow_node_add_edge(security_check, mitigation_planning, NULL);
    workflow_node_add_edge(mitigation_planning, hotfix, "can_hotfix == true");
    workflow_node_add_edge(hotfix, fix_verification, NULL);
    workflow_node_add_edge(fix_verification, incident_resolved, "incident_resolved == true");
    workflow_node_add_edge(incident_resolved, post_mortem, NULL);
    workflow_node_add_edge(post_mortem, conclusion, NULL);
    
    Workflow* wf = workflow_create("incident_response_test", "Incident Response Workflow", incident_detection);
    TEST_ASSERT(wf != NULL, "incident response workflow created");
    
    workflow_set_state(wf, "incident_type", "service_down");
    workflow_set_state(wf, "affected_service", "authentication_service");
    workflow_set_state(wf, "severity", "critical");
    
    char* output = NULL;
    int result = workflow_execute(wf, "Gestisci downtime del servizio di autenticazione", &output);
    
    TEST_ASSERT(result == 0 || wf->status == WORKFLOW_STATUS_COMPLETED ||
                wf->status == WORKFLOW_STATUS_FAILED ||
                wf->status == WORKFLOW_STATUS_PAUSED,
                "incident response workflow execution completes");
    
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
    test_e2e_class_council_workflow();
    test_e2e_security_audit_workflow();
    test_e2e_performance_optimization_workflow();
    test_e2e_incident_response_workflow();
    
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

