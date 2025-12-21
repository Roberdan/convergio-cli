/**
 * CONVERGIO WORKFLOW PATTERNS
 *
 * Reusable workflow patterns library
 * LangGraph-inspired pattern composition
 */

#ifndef CONVERGIO_PATTERNS_H
#define CONVERGIO_PATTERNS_H

#include "nous/workflow.h"
#include "nous/nous.h"
#include <stdint.h>

// ============================================================================
// PATTERN CREATION
// ============================================================================

// Review-Refine Loop: Generator -> Critic -> Refiner (iterative)
Workflow* pattern_create_review_refine_loop(
    SemanticID generator_id,
    SemanticID critic_id,
    SemanticID refiner_id,
    int max_iterations
);

// Parallel Analysis: Multiple analysts -> Converger
Workflow* pattern_create_parallel_analysis(
    SemanticID* analyst_ids,
    size_t analyst_count,
    SemanticID converger_id
);

// Sequential Planning: Chain of planners
Workflow* pattern_create_sequential_planning(
    SemanticID* planner_ids,
    size_t planner_count
);

// Consensus Building: Group chat -> Consensus check
Workflow* pattern_create_consensus_building(
    SemanticID* participant_ids,
    size_t participant_count,
    double consensus_threshold
);

// Pattern composition: Combine two workflows
Workflow* pattern_compose(Workflow* wf1, Workflow* wf2, const char* join_condition);

#endif // CONVERGIO_PATTERNS_H

