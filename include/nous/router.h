/**
 * CONVERGIO CONDITIONAL ROUTER
 *
 * LangGraph-inspired conditional routing
 * Evaluates conditions to determine next workflow node
 */

#ifndef CONVERGIO_ROUTER_H
#define CONVERGIO_ROUTER_H

#include "nous/workflow.h"
#include "nous/nous.h"
#include <stdbool.h>

// ============================================================================
// CONDITION EVALUATION
// ============================================================================

// Evaluate condition expression against workflow state
// Returns true if condition is met, false otherwise
// Supports: ==, !=, <, >, <=, >=, &&, ||, !
bool router_evaluate_condition(
    const char* condition_expr,
    const WorkflowState* state
);

// Get next node based on condition evaluation
WorkflowNode* router_get_next_node(
    Workflow* wf,
    WorkflowNode* current,
    const WorkflowState* state
);

#endif // CONVERGIO_ROUTER_H

