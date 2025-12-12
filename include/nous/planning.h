/**
 * CONVERGIO PLANNING
 *
 * Task planning and execution plan management
 */

#ifndef CONVERGIO_PLANNING_H
#define CONVERGIO_PLANNING_H

#include "nous/orchestrator.h"
#include <stdint.h>
#include <time.h>

// ============================================================================
// PLANNING API
// ============================================================================

// Create a new execution plan
ExecutionPlan* orch_plan_create(const char* goal);

// Create a new task
Task* orch_task_create(const char* description, SemanticID assignee);

// Add task to execution plan
void orch_plan_add_task(ExecutionPlan* plan, Task* task);

// Mark task as completed with result
void orch_task_complete(Task* task, const char* result);

#endif // CONVERGIO_PLANNING_H
