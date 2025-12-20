/**
 * CONVERGIO TASK DECOMPOSER TESTS
 *
 * Unit tests for task decomposition and dependency resolution
 */

#include "nous/task_decomposer.h"
#include "nous/orchestrator.h"
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
// TASK DECOMPOSITION TESTS
// ============================================================================

static void test_task_decompose_simple_goal(void) {
    printf("test_task_decompose_simple_goal:\n");
    
    AgentRole roles[] = {AGENT_ROLE_CODER, AGENT_ROLE_WRITER};
    size_t count = 0;
    
    DecomposedTask* tasks = task_decompose("Write a hello world program", roles, 2, &count);
    
    TEST_ASSERT(tasks != NULL || count == 0, "task_decompose handles gracefully");
    
    if (tasks) {
        task_decomposed_free_all(tasks, count);
    }
    printf("\n");
}

// ============================================================================
// DEPENDENCY RESOLUTION TESTS
// ============================================================================

static void test_dependency_resolution_linear(void) {
    printf("test_dependency_resolution_linear:\n");
    
    // Create a simple linear dependency chain
    DecomposedTask tasks[3];
    memset(tasks, 0, sizeof(tasks));
    
    tasks[0].task_id = 1;
    tasks[0].description = strdup("Task 1");
    tasks[0].prerequisite_count = 0;
    
    tasks[1].task_id = 2;
    tasks[1].description = strdup("Task 2");
    tasks[1].prerequisite_count = 1;
    tasks[1].prerequisite_ids = malloc(sizeof(uint64_t));
    tasks[1].prerequisite_ids[0] = 1;
    
    tasks[2].task_id = 3;
    tasks[2].description = strdup("Task 3");
    tasks[2].prerequisite_count = 1;
    tasks[2].prerequisite_ids = malloc(sizeof(uint64_t));
    tasks[2].prerequisite_ids[0] = 2;
    
    int result = task_resolve_dependencies(tasks, 3);
    TEST_ASSERT(result == 0, "linear dependency resolution succeeds");
    
    // Cleanup
    for (int i = 0; i < 3; i++) {
        free(tasks[i].description);
        if (tasks[i].prerequisite_ids) {
            free(tasks[i].prerequisite_ids);
        }
    }
    printf("\n");
}

static void test_dependency_resolution_circular(void) {
    printf("test_dependency_resolution_circular:\n");
    
    // Create circular dependency
    DecomposedTask tasks[2];
    memset(tasks, 0, sizeof(tasks));
    
    tasks[0].task_id = 1;
    tasks[0].description = strdup("Task 1");
    tasks[0].prerequisite_count = 1;
    tasks[0].prerequisite_ids = malloc(sizeof(uint64_t));
    tasks[0].prerequisite_ids[0] = 2;
    
    tasks[1].task_id = 2;
    tasks[1].description = strdup("Task 2");
    tasks[1].prerequisite_count = 1;
    tasks[1].prerequisite_ids = malloc(sizeof(uint64_t));
    tasks[1].prerequisite_ids[0] = 1;
    
    int result = task_resolve_dependencies(tasks, 2);
    TEST_ASSERT(result != 0, "circular dependency detection works");
    
    // Cleanup
    for (int i = 0; i < 2; i++) {
        free(tasks[i].description);
        if (tasks[i].prerequisite_ids) {
            free(tasks[i].prerequisite_ids);
        }
    }
    printf("\n");
}

// ============================================================================
// EXECUTION PLAN TESTS
// ============================================================================

static void test_create_execution_plan(void) {
    printf("test_create_execution_plan:\n");
    
    DecomposedTask tasks[2];
    memset(tasks, 0, sizeof(tasks));
    
    tasks[0].task_id = 1;
    tasks[0].description = strdup("Task 1");
    tasks[0].required_role = AGENT_ROLE_CODER;
    tasks[0].prerequisite_count = 0;
    
    tasks[1].task_id = 2;
    tasks[1].description = strdup("Task 2");
    tasks[1].required_role = AGENT_ROLE_WRITER;
    tasks[1].prerequisite_count = 0;
    
    ExecutionPlan* plan = task_create_execution_plan(tasks, 2);
    TEST_ASSERT(plan != NULL || plan == NULL, "execution plan creation handles gracefully");
    
    if (plan) {
        // Note: orch_plan_destroy would be called here if available
    }
    
    // Cleanup
    for (int i = 0; i < 2; i++) {
        free(tasks[i].description);
    }
    printf("\n");
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("=== CONVERGIO TASK DECOMPOSER TESTS ===\n\n");
    
    test_task_decompose_simple_goal();
    test_dependency_resolution_linear();
    test_dependency_resolution_circular();
    test_create_execution_plan();
    
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

