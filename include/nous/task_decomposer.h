/**
 * CONVERGIO TASK DECOMPOSER
 *
 * CrewAI-inspired hierarchical task decomposition
 * Breaks down complex goals into subtasks with dependency resolution
 */

#ifndef CONVERGIO_TASK_DECOMPOSER_H
#define CONVERGIO_TASK_DECOMPOSER_H

#include "nous/orchestrator.h"
#include "nous/nous.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <dispatch/dispatch.h>

// TaskStatus is defined in orchestrator.h

// ============================================================================
// DECOMPOSED TASK
// ============================================================================

typedef struct DecomposedTask {
    uint64_t task_id;
    char* description;
    AgentRole required_role;
    TaskStatus status;
    uint64_t* prerequisite_ids;      // Array of task IDs that must complete first
    size_t prerequisite_count;
    size_t prerequisite_capacity;
    char* validation_criteria;        // How to validate task completion
    int max_retries;
    int current_retry;
    char* result;                     // Task output/result
    time_t created_at;
    time_t completed_at;
    SemanticID assigned_agent_id;    // Agent assigned to this task
} DecomposedTask;

// ============================================================================
// TASK DECOMPOSITION
// ============================================================================

// Decompose a goal into subtasks
// Uses LLM to break down goal into actionable tasks
DecomposedTask* task_decompose(
    const char* goal,
    AgentRole* roles,
    size_t role_count,
    size_t* out_count
);

// Resolve task dependencies and detect cycles
// Returns 0 on success, -1 on error (e.g., circular dependency)
int task_resolve_dependencies(DecomposedTask* tasks, size_t task_count);

// Create execution plan from decomposed tasks
// Returns topological sort of tasks ready for execution
ExecutionPlan* task_create_execution_plan(DecomposedTask* tasks, size_t task_count);

// Execute tasks in parallel using GCD
// Tasks with resolved dependencies can run in parallel
int task_execute_parallel(
    DecomposedTask* tasks,
    size_t task_count,
    dispatch_group_t group
);

// ============================================================================
// TASK MANAGEMENT
// ============================================================================

// Free decomposed tasks array
void task_free_decomposed(DecomposedTask* tasks, size_t count);

// Get tasks ready for execution (all prerequisites completed)
DecomposedTask* task_get_ready(DecomposedTask* tasks, size_t task_count, size_t* out_count);

// Mark task as completed
int task_mark_completed(DecomposedTask* task, const char* result);

// Mark task as failed
int task_mark_failed(DecomposedTask* task, const char* error);

// Check if all prerequisites are completed
bool task_prerequisites_met(const DecomposedTask* task, const DecomposedTask* all_tasks, size_t task_count);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Detect circular dependencies in task graph
bool task_detect_cycles(DecomposedTask* tasks, size_t task_count);

// Topological sort of tasks (Kahn's algorithm)
int task_topological_sort(DecomposedTask* tasks, size_t task_count, uint64_t* sorted_ids, size_t* out_count);

#endif // CONVERGIO_TASK_DECOMPOSER_H

