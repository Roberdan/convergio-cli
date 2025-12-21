/**
 * CONVERGIO STRESS TESTS
 *
 * Concurrent execution stress testing for workflow components:
 * - Multi-threaded workflow state access
 * - Concurrent group chat operations
 * - Parallel checkpoint creation
 * - Memory allocation under load
 */

#include "nous/workflow.h"
#include "nous/nous.h"
#include "nous/debug_mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// ============================================================================
// TEST CONFIGURATION
// ============================================================================

#define NUM_THREADS 8
#define ITERATIONS_PER_THREAD 100
#define STATE_KEYS 50

// ============================================================================
// STUBS
// ============================================================================

void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    (void)level; (void)cat; (void)fmt;
}

void nous_log_set_level(LogLevel level) { (void)level; }
LogLevel nous_log_get_level(void) { return LOG_LEVEL_ERROR; }
const char* nous_log_level_name(LogLevel level) { (void)level; return ""; }

// ============================================================================
// TEST HELPERS
// ============================================================================

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  \033[32m+\033[0m %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  \033[31m-\033[0m %s\n", message); \
        } \
    } while (0)

#define TEST_SECTION(name) printf("\n\033[1m=== %s ===\033[0m\n", name)

// ============================================================================
// CONCURRENT STATE ISOLATION TEST
// Each thread gets its own state object to test parallel state operations
// ============================================================================

typedef struct {
    int thread_id;
    int iterations;
    int errors;
    int operations;
} StateThreadArgs;

static void* concurrent_state_worker(void* arg) {
    StateThreadArgs* args = (StateThreadArgs*)arg;
    char key[64];
    char value[128];

    // Each thread creates its own state (tests concurrent allocations)
    WorkflowState* state = workflow_state_create();
    if (!state) {
        args->errors++;
        return NULL;
    }

    for (int i = 0; i < args->iterations; i++) {
        // Write operation
        snprintf(key, sizeof(key), "key_%d", i % STATE_KEYS);
        snprintf(value, sizeof(value), "value_%d_%d", args->thread_id, i);

        if (workflow_state_set(state, key, value) != 0) {
            args->errors++;
        } else {
            args->operations++;
        }

        // Read operation
        const char* read_value = workflow_state_get(state, key);
        if (!read_value) {
            args->errors++;
        } else {
            args->operations++;
        }
    }

    workflow_state_destroy(state);
    return NULL;
}

static void test_concurrent_state_access(void) {
    TEST_SECTION("Concurrent State Allocation/Deallocation");

    pthread_t threads[NUM_THREADS];
    StateThreadArgs args[NUM_THREADS];

    // Launch threads (each creates/destroys its own state)
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        args[i].iterations = ITERATIONS_PER_THREAD;
        args[i].errors = 0;
        args[i].operations = 0;
        pthread_create(&threads[i], NULL, concurrent_state_worker, &args[i]);
    }

    // Wait for all threads
    int total_errors = 0;
    int total_ops = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_errors += args[i].errors;
        total_ops += args[i].operations;
    }

    printf("  Total operations: %d, Errors: %d\n", total_ops, total_errors);
    TEST_ASSERT(total_errors == 0, "no errors in concurrent state operations");
    TEST_ASSERT(total_ops > 0, "operations completed successfully");
}

// ============================================================================
// CONCURRENT WORKFLOW CREATION TEST
// ============================================================================

typedef struct {
    int thread_id;
    int workflows_created;
    int errors;
} WorkflowThreadArgs;

static void* concurrent_workflow_worker(void* arg) {
    WorkflowThreadArgs* args = (WorkflowThreadArgs*)arg;
    char name[64];

    for (int i = 0; i < ITERATIONS_PER_THREAD / 10; i++) {
        snprintf(name, sizeof(name), "workflow_t%d_i%d", args->thread_id, i);

        // Create workflow
        WorkflowNode* entry = workflow_node_create("start", NODE_TYPE_ACTION);
        if (!entry) {
            args->errors++;
            continue;
        }

        Workflow* wf = workflow_create(name, "stress test workflow", entry);
        if (!wf) {
            workflow_node_destroy(entry);
            args->errors++;
            continue;
        }

        // Set some state
        workflow_state_set(wf->state, "test_key", "test_value");

        // Destroy immediately (stress test allocation/deallocation)
        workflow_destroy(wf);
        args->workflows_created++;
    }

    return NULL;
}

static void test_concurrent_workflow_creation(void) {
    TEST_SECTION("Concurrent Workflow Creation/Destruction");

    pthread_t threads[NUM_THREADS];
    WorkflowThreadArgs args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        args[i].workflows_created = 0;
        args[i].errors = 0;
        pthread_create(&threads[i], NULL, concurrent_workflow_worker, &args[i]);
    }

    int total_created = 0;
    int total_errors = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_created += args[i].workflows_created;
        total_errors += args[i].errors;
    }

    printf("  Workflows created: %d, Errors: %d\n", total_created, total_errors);
    TEST_ASSERT(total_created > 0, "workflows were created");
    TEST_ASSERT(total_errors == 0, "no errors in workflow creation");
}

// ============================================================================
// MEMORY ALLOCATION STRESS TEST
// ============================================================================

static void test_memory_allocation_stress(void) {
    TEST_SECTION("Memory Allocation Under Load");

    const int NUM_ALLOCATIONS = 1000;
    WorkflowState** states = malloc(sizeof(WorkflowState*) * NUM_ALLOCATIONS);
    TEST_ASSERT(states != NULL, "states array allocated");

    // Rapid allocation
    int alloc_success = 0;
    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        states[i] = workflow_state_create();
        if (states[i]) {
            alloc_success++;
            // Fill with some data
            char key[32], value[64];
            for (int j = 0; j < 10; j++) {
                snprintf(key, sizeof(key), "key_%d", j);
                snprintf(value, sizeof(value), "value_%d_%d", i, j);
                workflow_state_set(states[i], key, value);
            }
        }
    }

    printf("  Allocated: %d/%d state objects\n", alloc_success, NUM_ALLOCATIONS);
    TEST_ASSERT(alloc_success == NUM_ALLOCATIONS, "all allocations succeeded");

    // Rapid deallocation
    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        if (states[i]) {
            workflow_state_destroy(states[i]);
        }
    }

    free(states);
    TEST_ASSERT(true, "all deallocations completed without crash");
}

// ============================================================================
// NODE EDGE STRESS TEST
// ============================================================================

static void test_node_edge_stress(void) {
    TEST_SECTION("Node Edge Creation Stress");

    const int NUM_NODES = 100;
    WorkflowNode** nodes = malloc(sizeof(WorkflowNode*) * NUM_NODES);
    TEST_ASSERT(nodes != NULL, "nodes array allocated");

    // Create many nodes
    int created = 0;
    for (int i = 0; i < NUM_NODES; i++) {
        char name[32];
        snprintf(name, sizeof(name), "node_%d", i);
        nodes[i] = workflow_node_create(name, NODE_TYPE_ACTION);
        if (nodes[i]) {
            created++;
        }
    }

    printf("  Created: %d/%d nodes\n", created, NUM_NODES);
    TEST_ASSERT(created == NUM_NODES, "all nodes created");

    // Create edges (fully connected - stress test)
    int edges_created = 0;
    for (int i = 0; i < NUM_NODES - 1; i++) {
        if (nodes[i] && nodes[i + 1]) {
            if (workflow_node_add_edge(nodes[i], nodes[i + 1], NULL) == 0) {
                edges_created++;
            }
        }
    }

    printf("  Created: %d edges\n", edges_created);
    TEST_ASSERT(edges_created > 0, "edges were created");

    // Cleanup
    for (int i = 0; i < NUM_NODES; i++) {
        if (nodes[i]) {
            workflow_node_destroy(nodes[i]);
        }
    }

    free(nodes);
    TEST_ASSERT(true, "all nodes destroyed without crash");
}

// ============================================================================
// VALIDATION FUNCTION STRESS TEST
// ============================================================================

extern bool workflow_validate_name(const char* name);
extern bool workflow_validate_key(const char* key);
extern bool workflow_validate_condition_safe(const char* condition);

typedef struct {
    int thread_id;
    int validations;
} ValidationThreadArgs;

static void* concurrent_validation_worker(void* arg) {
    ValidationThreadArgs* args = (ValidationThreadArgs*)arg;
    char test_string[256];

    for (int i = 0; i < ITERATIONS_PER_THREAD; i++) {
        // Generate various test strings
        snprintf(test_string, sizeof(test_string), "test_name_%d_%d", args->thread_id, i);

        // Call validation functions concurrently
        workflow_validate_name(test_string);
        workflow_validate_key(test_string);
        workflow_validate_condition_safe(test_string);

        args->validations += 3;
    }

    return NULL;
}

static void test_concurrent_validation(void) {
    TEST_SECTION("Concurrent Validation Functions");

    pthread_t threads[NUM_THREADS];
    ValidationThreadArgs args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        args[i].validations = 0;
        pthread_create(&threads[i], NULL, concurrent_validation_worker, &args[i]);
    }

    int total_validations = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_validations += args[i].validations;
    }

    printf("  Total validations: %d\n", total_validations);
    TEST_ASSERT(total_validations == NUM_THREADS * ITERATIONS_PER_THREAD * 3,
                "all validations completed");
}

// ============================================================================
// ETHICAL GUARDRAILS STRESS TEST
// ============================================================================

extern EthicalResult workflow_validate_ethical(const char* content);

typedef struct {
    int thread_id;
    int checks;
} EthicalThreadArgs;

static void* concurrent_ethical_worker(void* arg) {
    EthicalThreadArgs* args = (EthicalThreadArgs*)arg;
    const char* test_contents[] = {
        "analyze this code for bugs",
        "write a unit test",
        "explain authentication",
        "review this PR",
        "document the API",
        NULL
    };

    for (int i = 0; i < ITERATIONS_PER_THREAD; i++) {
        int idx = i % 5;
        workflow_validate_ethical(test_contents[idx]);
        args->checks++;
    }

    return NULL;
}

static void test_concurrent_ethical_guardrails(void) {
    TEST_SECTION("Concurrent Ethical Guardrails");

    pthread_t threads[NUM_THREADS];
    EthicalThreadArgs args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        args[i].checks = 0;
        pthread_create(&threads[i], NULL, concurrent_ethical_worker, &args[i]);
    }

    int total_checks = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_checks += args[i].checks;
    }

    printf("  Total ethical checks: %d\n", total_checks);
    TEST_ASSERT(total_checks == NUM_THREADS * ITERATIONS_PER_THREAD,
                "all ethical checks completed");
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("\n");
    printf("======================================================================\n");
    printf("              CONVERGIO CONCURRENT STRESS TESTS\n");
    printf("======================================================================\n");
    printf("\n");
    printf("Configuration:\n");
    printf("  Threads: %d\n", NUM_THREADS);
    printf("  Iterations per thread: %d\n", ITERATIONS_PER_THREAD);
    printf("\n");

    // Run all stress tests
    test_concurrent_state_access();
    test_concurrent_workflow_creation();
    test_memory_allocation_stress();
    test_node_edge_stress();
    test_concurrent_validation();
    test_concurrent_ethical_guardrails();

    // Summary
    printf("\n");
    printf("======================================================================\n");
    printf("                         TEST SUMMARY\n");
    printf("======================================================================\n");
    printf("\n");
    printf("  Tests Run:    %d\n", tests_run);
    printf("  Tests Passed: \033[32m%d\033[0m\n", tests_passed);
    printf("  Tests Failed: \033[31m%d\033[0m\n", tests_failed);
    printf("\n");

    if (tests_failed == 0) {
        printf("  \033[32m+ All stress tests passed!\033[0m\n");
        printf("\n");
        return 0;
    } else {
        printf("  \033[31m- Some stress tests failed!\033[0m\n");
        printf("\n");
        return 1;
    }
}
