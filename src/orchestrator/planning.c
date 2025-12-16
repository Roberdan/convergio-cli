/**
 * CONVERGIO PLANNING
 *
 * Task planning and execution plan management
 * Now with optional SQLite persistence via plan_db
 */

#include "nous/planning.h"
#include "nous/orchestrator.h"
#include "nous/plan_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// TASK PLANNING
// ============================================================================

static uint64_t g_next_task_id = 1;
static uint64_t g_next_plan_id = 1;

// Helper: Get agent name from SemanticID for persistence
static const char* get_agent_name(SemanticID id) {
    // SemanticID is a uint64_t hash - we'd need registry lookup
    // For now, return NULL (agent not tracked in DB)
    (void)id;
    return NULL;
}

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
    plan->db_id[0] = '\0';  // Initialize

    // Persist to SQLite if plan_db is ready
    if (plan_db_is_ready()) {
        char db_id[64];
        if (plan_db_create_plan(goal, NULL, db_id) == PLAN_DB_OK) {
            strncpy(plan->db_id, db_id, sizeof(plan->db_id) - 1);
            plan->db_id[sizeof(plan->db_id) - 1] = '\0';
        }
    }

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
    task->db_id[0] = '\0';  // Initialize

    return task;
}

void orch_plan_add_task(ExecutionPlan* plan, Task* task) {
    if (!plan || !task) return;

    // Add to linked list
    task->next = plan->tasks;
    plan->tasks = task;

    // Persist to SQLite if plan has db_id
    if (plan->db_id[0] && plan_db_is_ready()) {
        char task_db_id[64];
        const char* agent_name = get_agent_name(task->assigned_to);
        if (plan_db_add_task(plan->db_id, task->description, agent_name, 50, NULL, task_db_id) == PLAN_DB_OK) {
            strncpy(task->db_id, task_db_id, sizeof(task->db_id) - 1);
            task->db_id[sizeof(task->db_id) - 1] = '\0';
        }
    }
}

void orch_task_complete(Task* task, const char* result) {
    if (!task) return;

    task->status = TASK_STATUS_COMPLETED;
    task->result = result ? strdup(result) : NULL;
    task->completed_at = time(NULL);

    // Update SQLite if task has db_id
    if (task->db_id[0] && plan_db_is_ready()) {
        plan_db_complete_task(task->db_id, result);
    }
}

// ============================================================================
// PLAN PROGRESS (uses SQLite if available)
// ============================================================================

bool orch_plan_get_progress(ExecutionPlan* plan, int* total, int* completed, float* percent) {
    if (!plan) return false;

    // If plan has db_id, get from SQLite for accurate progress
    if (plan->db_id[0] && plan_db_is_ready()) {
        PlanProgress progress;
        if (plan_db_get_progress(plan->db_id, &progress) == PLAN_DB_OK) {
            if (total) *total = progress.total;
            if (completed) *completed = progress.completed;
            if (percent) *percent = progress.percent_complete;
            return true;
        }
    }

    // Fallback: count in-memory tasks
    int t = 0, c = 0;
    for (Task* task = plan->tasks; task; task = task->next) {
        t++;
        if (task->status == TASK_STATUS_COMPLETED) c++;
    }
    if (total) *total = t;
    if (completed) *completed = c;
    if (percent) *percent = t > 0 ? (float)c / (float)t * 100.0f : 0.0f;
    return true;
}
