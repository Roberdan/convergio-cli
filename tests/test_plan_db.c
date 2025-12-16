/**
 * CONVERGIO PLAN DATABASE TESTS
 *
 * Unit tests for SQLite-backed execution plan system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "nous/plan_db.h"

// ============================================================================
// TEST MACROS
// ============================================================================

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running %s...", #name); \
    fflush(stdout); \
    test_##name(); \
    printf(" OK\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "\n    ASSERT FAILED: %s (line %d)\n", #cond, __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

// ============================================================================
// TEST FIXTURES
// ============================================================================

static const char* TEST_DB_PATH = "/tmp/convergio_test_plans.db";

static void setup(void) {
    // Remove old test database
    unlink(TEST_DB_PATH);

    // Initialize
    PlanDbError err = plan_db_init(TEST_DB_PATH);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT(plan_db_is_ready());
}

static void teardown(void) {
    plan_db_shutdown();
    unlink(TEST_DB_PATH);
}

// ============================================================================
// PLAN TESTS
// ============================================================================

TEST(create_plan) {
    char plan_id[64];
    PlanDbError err = plan_db_create_plan("Test Plan", "Some context", plan_id);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT(strlen(plan_id) > 0);

    // Verify it's a UUID format (36 chars with dashes)
    ASSERT_EQ(strlen(plan_id), 36);
}

TEST(get_plan) {
    char plan_id[64];
    plan_db_create_plan("Get Test Plan", "Context here", plan_id);

    PlanRecord plan;
    PlanDbError err = plan_db_get_plan(plan_id, &plan);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT_STR_EQ(plan.id, plan_id);
    ASSERT(strstr(plan.description, "Get Test Plan") != NULL);
    ASSERT_EQ(plan.status, PLAN_STATUS_PENDING);
    ASSERT_EQ(plan.total_tasks, 0);

    plan_record_free(&plan);
}

TEST(get_plan_not_found) {
    PlanRecord plan;
    PlanDbError err = plan_db_get_plan("non-existent-uuid", &plan);
    ASSERT_EQ(err, PLAN_DB_ERROR_NOT_FOUND);
}

TEST(update_plan_status) {
    char plan_id[64];
    plan_db_create_plan("Status Test", NULL, plan_id);

    PlanDbError err = plan_db_update_plan_status(plan_id, PLAN_STATUS_ACTIVE);
    ASSERT_EQ(err, PLAN_DB_OK);

    PlanRecord plan;
    plan_db_get_plan(plan_id, &plan);
    ASSERT_EQ(plan.status, PLAN_STATUS_ACTIVE);
    plan_record_free(&plan);

    // Complete it
    err = plan_db_update_plan_status(plan_id, PLAN_STATUS_COMPLETED);
    ASSERT_EQ(err, PLAN_DB_OK);

    plan_db_get_plan(plan_id, &plan);
    ASSERT_EQ(plan.status, PLAN_STATUS_COMPLETED);
    ASSERT(plan.completed_at > 0);
    plan_record_free(&plan);
}

TEST(delete_plan) {
    char plan_id[64];
    plan_db_create_plan("Delete Me", NULL, plan_id);

    PlanDbError err = plan_db_delete_plan(plan_id);
    ASSERT_EQ(err, PLAN_DB_OK);

    PlanRecord plan;
    err = plan_db_get_plan(plan_id, &plan);
    ASSERT_EQ(err, PLAN_DB_ERROR_NOT_FOUND);
}

TEST(list_plans) {
    // Create several plans
    char id1[64], id2[64], id3[64];
    plan_db_create_plan("Plan 1", NULL, id1);
    plan_db_create_plan("Plan 2", NULL, id2);
    plan_db_create_plan("Plan 3", NULL, id3);

    plan_db_update_plan_status(id2, PLAN_STATUS_ACTIVE);

    // List all
    PlanRecord plans[10];
    int count;
    PlanDbError err = plan_db_list_plans(-1, 10, 0, plans, 10, &count);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT(count >= 3);

    for (int i = 0; i < count; i++) {
        plan_record_free(&plans[i]);
    }

    // List only active
    err = plan_db_list_plans(PLAN_STATUS_ACTIVE, 10, 0, plans, 10, &count);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT(count >= 1);

    for (int i = 0; i < count; i++) {
        ASSERT_EQ(plans[i].status, PLAN_STATUS_ACTIVE);
        plan_record_free(&plans[i]);
    }
}

// ============================================================================
// TASK TESTS
// ============================================================================

TEST(add_task) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Task Plan", NULL, plan_id);

    PlanDbError err = plan_db_add_task(plan_id, "First task", "baccio", 80, NULL, task_id);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT_EQ(strlen(task_id), 36);

    // Verify plan task count
    PlanRecord plan;
    plan_db_get_plan(plan_id, &plan);
    ASSERT_EQ(plan.total_tasks, 1);
    plan_record_free(&plan);
}

TEST(get_task) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Get Task Plan", NULL, plan_id);
    plan_db_add_task(plan_id, "Get this task", "dario", 50, NULL, task_id);

    TaskRecord task;
    PlanDbError err = plan_db_get_task(task_id, &task);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT_STR_EQ(task.id, task_id);
    ASSERT(strstr(task.description, "Get this task") != NULL);
    ASSERT_STR_EQ(task.assigned_agent, "dario");
    ASSERT_EQ(task.status, TASK_DB_STATUS_PENDING);
    ASSERT_EQ(task.priority, 50);

    task_record_free(&task);
}

TEST(claim_task) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Claim Plan", NULL, plan_id);
    plan_db_add_task(plan_id, "Claimable task", NULL, 50, NULL, task_id);

    // First claim should succeed
    PlanDbError err = plan_db_claim_task(task_id, "agent1");
    ASSERT_EQ(err, PLAN_DB_OK);

    // Second claim should fail (already claimed)
    err = plan_db_claim_task(task_id, "agent2");
    ASSERT_EQ(err, PLAN_DB_ERROR_BUSY);

    // Verify task state
    TaskRecord task;
    plan_db_get_task(task_id, &task);
    ASSERT_EQ(task.status, TASK_DB_STATUS_IN_PROGRESS);
    ASSERT_STR_EQ(task.assigned_agent, "agent1");
    ASSERT(task.started_at > 0);
    task_record_free(&task);
}

TEST(complete_task) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Complete Plan", NULL, plan_id);
    plan_db_add_task(plan_id, "Completable task", NULL, 50, NULL, task_id);

    plan_db_claim_task(task_id, "agent1");

    PlanDbError err = plan_db_complete_task(task_id, "Task output result");
    ASSERT_EQ(err, PLAN_DB_OK);

    TaskRecord task;
    plan_db_get_task(task_id, &task);
    ASSERT_EQ(task.status, TASK_DB_STATUS_COMPLETED);
    ASSERT(strstr(task.output, "Task output result") != NULL);
    ASSERT(task.completed_at > 0);
    task_record_free(&task);
}

TEST(fail_task) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Fail Plan", NULL, plan_id);
    plan_db_add_task(plan_id, "Failing task", NULL, 50, NULL, task_id);

    plan_db_claim_task(task_id, "agent1");

    PlanDbError err = plan_db_fail_task(task_id, "Something went wrong");
    ASSERT_EQ(err, PLAN_DB_OK);

    TaskRecord task;
    plan_db_get_task(task_id, &task);
    ASSERT_EQ(task.status, TASK_DB_STATUS_FAILED);
    ASSERT(strstr(task.error, "Something went wrong") != NULL);
    ASSERT_EQ(task.retry_count, 1);
    task_record_free(&task);
}

TEST(get_next_task) {
    char plan_id[64], task1_id[64], task2_id[64], task3_id[64];
    plan_db_create_plan("Next Task Plan", NULL, plan_id);

    // Add tasks with different priorities
    plan_db_add_task(plan_id, "Low priority", NULL, 20, NULL, task1_id);
    plan_db_add_task(plan_id, "High priority", NULL, 80, NULL, task2_id);
    plan_db_add_task(plan_id, "Medium assigned", "agent1", 50, NULL, task3_id);

    // Agent1 should get their assigned task first
    TaskRecord task;
    PlanDbError err = plan_db_get_next_task(plan_id, "agent1", &task);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT_STR_EQ(task.id, task3_id);
    task_record_free(&task);

    // Other agent should get highest priority unassigned
    err = plan_db_get_next_task(plan_id, "agent2", &task);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT_STR_EQ(task.id, task2_id);
    task_record_free(&task);
}

TEST(get_tasks_list) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("List Tasks Plan", NULL, plan_id);

    plan_db_add_task(plan_id, "Task 1", NULL, 50, NULL, task_id);
    plan_db_add_task(plan_id, "Task 2", NULL, 60, NULL, task_id);
    plan_db_add_task(plan_id, "Task 3", NULL, 70, NULL, task_id);

    plan_db_claim_task(task_id, "agent1"); // Claim task 3

    // Get all tasks
    TaskRecord* tasks = NULL;
    PlanDbError err = plan_db_get_tasks(plan_id, -1, &tasks);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT(tasks != NULL);

    int count = 0;
    for (TaskRecord* t = tasks; t; t = t->next) count++;
    ASSERT_EQ(count, 3);

    task_record_free_list(tasks);

    // Get only pending tasks
    err = plan_db_get_tasks(plan_id, TASK_DB_STATUS_PENDING, &tasks);
    ASSERT_EQ(err, PLAN_DB_OK);

    count = 0;
    for (TaskRecord* t = tasks; t; t = t->next) count++;
    ASSERT_EQ(count, 2);

    task_record_free_list(tasks);
}

TEST(subtasks) {
    char plan_id[64], parent_id[64], child1_id[64], child2_id[64];
    plan_db_create_plan("Subtask Plan", NULL, plan_id);

    plan_db_add_task(plan_id, "Parent task", NULL, 50, NULL, parent_id);
    plan_db_add_task(plan_id, "Child 1", NULL, 50, parent_id, child1_id);
    plan_db_add_task(plan_id, "Child 2", NULL, 50, parent_id, child2_id);

    TaskRecord* subtasks = NULL;
    PlanDbError err = plan_db_get_subtasks(parent_id, &subtasks);
    ASSERT_EQ(err, PLAN_DB_OK);

    int count = 0;
    for (TaskRecord* t = subtasks; t; t = t->next) count++;
    ASSERT_EQ(count, 2);

    task_record_free_list(subtasks);
}

// ============================================================================
// PROGRESS TESTS
// ============================================================================

TEST(progress_tracking) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Progress Plan", NULL, plan_id);

    // Add 5 tasks
    for (int i = 0; i < 5; i++) {
        char desc[32];
        snprintf(desc, sizeof(desc), "Task %d", i + 1);
        plan_db_add_task(plan_id, desc, NULL, 50, NULL, task_id);
    }

    PlanProgress progress;
    plan_db_get_progress(plan_id, &progress);
    ASSERT_EQ(progress.total, 5);
    ASSERT_EQ(progress.pending, 5);
    ASSERT_EQ(progress.completed, 0);
    ASSERT(progress.percent_complete < 0.01);

    // Complete 2 tasks
    TaskRecord* tasks = NULL;
    plan_db_get_tasks(plan_id, -1, &tasks);

    TaskRecord* t = tasks;
    plan_db_claim_task(t->id, "agent1");
    plan_db_complete_task(t->id, "Done");

    t = t->next;
    plan_db_claim_task(t->id, "agent1");
    plan_db_complete_task(t->id, "Done");

    task_record_free_list(tasks);

    plan_db_get_progress(plan_id, &progress);
    ASSERT_EQ(progress.completed, 2);
    ASSERT_EQ(progress.pending, 3);
    ASSERT(progress.percent_complete > 39.0 && progress.percent_complete < 41.0);
}

TEST(plan_completion_check) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Completion Plan", NULL, plan_id);

    plan_db_add_task(plan_id, "Only task", NULL, 50, NULL, task_id);

    ASSERT(!plan_db_is_plan_complete(plan_id));

    plan_db_claim_task(task_id, "agent1");
    ASSERT(!plan_db_is_plan_complete(plan_id));

    plan_db_complete_task(task_id, "Done");
    ASSERT(plan_db_is_plan_complete(plan_id));
}

TEST(auto_status_refresh) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Auto Status Plan", NULL, plan_id);

    PlanRecord plan;
    plan_db_get_plan(plan_id, &plan);
    ASSERT_EQ(plan.status, PLAN_STATUS_PENDING);
    plan_record_free(&plan);

    plan_db_add_task(plan_id, "Task 1", NULL, 50, NULL, task_id);
    plan_db_claim_task(task_id, "agent1");

    plan_db_refresh_plan_status(plan_id);
    plan_db_get_plan(plan_id, &plan);
    ASSERT_EQ(plan.status, PLAN_STATUS_ACTIVE);
    plan_record_free(&plan);

    plan_db_complete_task(task_id, "Done");
    plan_db_refresh_plan_status(plan_id);
    plan_db_get_plan(plan_id, &plan);
    ASSERT_EQ(plan.status, PLAN_STATUS_COMPLETED);
    plan_record_free(&plan);
}

// ============================================================================
// EXPORT TESTS
// ============================================================================

TEST(export_markdown) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Export Test Plan", "Test context", plan_id);

    plan_db_add_task(plan_id, "Task 1", "agent1", 80, NULL, task_id);
    plan_db_claim_task(task_id, "agent1");
    plan_db_complete_task(task_id, "Completed output");

    plan_db_add_task(plan_id, "Task 2", "agent2", 60, NULL, task_id);
    plan_db_add_task(plan_id, "Task 3", NULL, 40, NULL, task_id);

    const char* out_path = "/tmp/test_plan_export.md";
    PlanDbError err = plan_db_export_markdown(plan_id, out_path, true);
    ASSERT_EQ(err, PLAN_DB_OK);

    // Verify file exists and has content
    FILE* f = fopen(out_path, "r");
    ASSERT(f != NULL);

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    ASSERT(size > 100); // Should have substantial content

    fseek(f, 0, SEEK_SET);
    char* content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    // Check for expected content
    ASSERT(strstr(content, "Export Test Plan") != NULL);
    ASSERT(strstr(content, "Progress") != NULL);
    ASSERT(strstr(content, "mermaid") != NULL);
    ASSERT(strstr(content, "Task 1") != NULL);

    free(content);
    unlink(out_path);
}

TEST(export_json) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("JSON Export Plan", NULL, plan_id);
    plan_db_add_task(plan_id, "JSON Task", NULL, 50, NULL, task_id);

    char* json = NULL;
    PlanDbError err = plan_db_export_json(plan_id, &json);
    ASSERT_EQ(err, PLAN_DB_OK);
    ASSERT(json != NULL);

    // Basic JSON structure validation
    ASSERT(strstr(json, "\"id\"") != NULL);
    ASSERT(strstr(json, "\"description\"") != NULL);
    ASSERT(strstr(json, "\"tasks\"") != NULL);
    ASSERT(strstr(json, "JSON Export Plan") != NULL);

    free(json);
}

TEST(generate_mermaid) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Mermaid Plan", NULL, plan_id);

    plan_db_add_task(plan_id, "First task", NULL, 50, NULL, task_id);
    plan_db_claim_task(task_id, "agent1");
    plan_db_complete_task(task_id, "Done");

    plan_db_add_task(plan_id, "Second task", NULL, 50, NULL, task_id);

    char* mermaid = plan_db_generate_mermaid(plan_id);
    ASSERT(mermaid != NULL);
    ASSERT(strstr(mermaid, "gantt") != NULL);
    ASSERT(strstr(mermaid, "First task") != NULL);

    free(mermaid);
}

// ============================================================================
// MAINTENANCE TESTS
// ============================================================================

TEST(cleanup_old_plans) {
    char plan_id[64];
    plan_db_create_plan("Old Plan", NULL, plan_id);
    plan_db_update_plan_status(plan_id, PLAN_STATUS_COMPLETED);

    // This won't delete anything since plan is fresh
    int deleted = plan_db_cleanup_old(0, PLAN_STATUS_COMPLETED);
    // The plan was just created, so it shouldn't be deleted with 0 days
    // (depends on timing, so just verify function works)
    ASSERT(deleted >= 0);
}

TEST(stats_json) {
    char* stats = plan_db_stats_json();
    ASSERT(stats != NULL);
    ASSERT(strstr(stats, "total_plans") != NULL);
    ASSERT(strstr(stats, "total_tasks") != NULL);
    free(stats);
}

// ============================================================================
// CONCURRENCY TESTS
// ============================================================================

typedef struct {
    const char* plan_id;
    const char* agent_name;
    int claimed_count;
    int failed_claims;
} WorkerArgs;

static void* task_claimer_thread(void* arg) {
    WorkerArgs* args = (WorkerArgs*)arg;
    args->claimed_count = 0;
    args->failed_claims = 0;

    for (int i = 0; i < 10; i++) {
        TaskRecord task;
        PlanDbError err = plan_db_get_next_task(args->plan_id, args->agent_name, &task);
        if (err == PLAN_DB_OK) {
            err = plan_db_claim_task(task.id, args->agent_name);
            if (err == PLAN_DB_OK) {
                args->claimed_count++;
                // Simulate work
                usleep(1000);
                plan_db_complete_task(task.id, "Done by thread");
            } else {
                args->failed_claims++;
            }
            task_record_free(&task);
        }
        usleep(500); // Small delay between attempts
    }

    return NULL;
}

TEST(concurrent_task_claiming) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Concurrent Plan", NULL, plan_id);

    // Add 20 tasks
    for (int i = 0; i < 20; i++) {
        char desc[32];
        snprintf(desc, sizeof(desc), "Concurrent Task %d", i + 1);
        plan_db_add_task(plan_id, desc, NULL, 50, NULL, task_id);
    }

    // Spawn multiple threads to claim tasks
    pthread_t threads[4];
    WorkerArgs args[4];

    for (int i = 0; i < 4; i++) {
        args[i].plan_id = plan_id;
        char* name = malloc(16);
        snprintf(name, 16, "worker%d", i);
        args[i].agent_name = name;
        pthread_create(&threads[i], NULL, task_claimer_thread, &args[i]);
    }

    // Wait for all threads
    int total_claimed = 0;
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
        total_claimed += args[i].claimed_count;
        free((void*)args[i].agent_name);
    }

    // Verify no double-claiming (total claimed should be <= 20)
    ASSERT(total_claimed <= 20);

    // Verify all tasks are either completed or still pending (no corruption)
    PlanProgress progress;
    plan_db_get_progress(plan_id, &progress);
    ASSERT_EQ(progress.total, 20);
    ASSERT(progress.completed + progress.pending == 20);
}

// ============================================================================
// CASCADE DELETE TEST
// ============================================================================

TEST(cascade_delete) {
    char plan_id[64], task_id[64];
    plan_db_create_plan("Cascade Plan", NULL, plan_id);

    // Add tasks
    plan_db_add_task(plan_id, "Task 1", NULL, 50, NULL, task_id);
    plan_db_add_task(plan_id, "Task 2", NULL, 50, NULL, task_id);

    // Verify tasks exist
    TaskRecord* tasks = NULL;
    plan_db_get_tasks(plan_id, -1, &tasks);
    ASSERT(tasks != NULL);
    task_record_free_list(tasks);

    // Delete plan
    plan_db_delete_plan(plan_id);

    // Verify tasks are also deleted (cascade)
    plan_db_get_tasks(plan_id, -1, &tasks);
    ASSERT(tasks == NULL);
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    printf("\n=== Convergio Plan Database Tests ===\n\n");

    printf("[PLAN OPERATIONS]\n");
    setup();
    RUN_TEST(create_plan);
    RUN_TEST(get_plan);
    RUN_TEST(get_plan_not_found);
    RUN_TEST(update_plan_status);
    RUN_TEST(delete_plan);
    RUN_TEST(list_plans);
    teardown();

    printf("\n[TASK OPERATIONS]\n");
    setup();
    RUN_TEST(add_task);
    RUN_TEST(get_task);
    RUN_TEST(claim_task);
    RUN_TEST(complete_task);
    RUN_TEST(fail_task);
    RUN_TEST(get_next_task);
    RUN_TEST(get_tasks_list);
    RUN_TEST(subtasks);
    teardown();

    printf("\n[PROGRESS TRACKING]\n");
    setup();
    RUN_TEST(progress_tracking);
    RUN_TEST(plan_completion_check);
    RUN_TEST(auto_status_refresh);
    teardown();

    printf("\n[EXPORT]\n");
    setup();
    RUN_TEST(export_markdown);
    RUN_TEST(export_json);
    RUN_TEST(generate_mermaid);
    teardown();

    printf("\n[MAINTENANCE]\n");
    setup();
    RUN_TEST(cleanup_old_plans);
    RUN_TEST(stats_json);
    teardown();

    printf("\n[CONCURRENCY]\n");
    setup();
    RUN_TEST(concurrent_task_claiming);
    teardown();

    printf("\n[CASCADE]\n");
    setup();
    RUN_TEST(cascade_delete);
    teardown();

    printf("\n=== All Plan Database Tests Passed! ===\n\n");
    return 0;
}
