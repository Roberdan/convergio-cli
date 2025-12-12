/**
 * CONVERGIO PLANNING
 *
 * Task planning and execution plan management
 */

#include "nous/planning.h"
#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// TASK PLANNING
// ============================================================================

static uint64_t g_next_task_id = 1;
static uint64_t g_next_plan_id = 1;

ExecutionPlan* orch_plan_create(const char* goal) {
    ExecutionPlan* plan = calloc(1, sizeof(ExecutionPlan));
    if (!plan) return NULL;

    plan->id = __sync_fetch_and_add(&g_next_plan_id, 1);
    plan->goal = strdup(goal);
    if (!plan->goal) {
        free(plan);
        return NULL;
    }
    plan->created_at = time(NULL);

    return plan;
}

Task* orch_task_create(const char* description, SemanticID assignee) {
    Task* task = calloc(1, sizeof(Task));
    if (!task) return NULL;

    task->id = __sync_fetch_and_add(&g_next_task_id, 1);
    task->description = strdup(description);
    if (!task->description) {
        free(task);
        return NULL;
    }
    task->assigned_to = assignee;
    task->status = TASK_STATUS_PENDING;
    task->created_at = time(NULL);

    return task;
}

void orch_plan_add_task(ExecutionPlan* plan, Task* task) {
    if (!plan || !task) return;

    // Add to linked list
    task->next = plan->tasks;
    plan->tasks = task;
}

void orch_task_complete(Task* task, const char* result) {
    if (!task) return;

    task->status = TASK_STATUS_COMPLETED;
    task->result = result ? strdup(result) : NULL;
    task->completed_at = time(NULL);
}
