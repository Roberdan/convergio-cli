/**
 * CONVERGIO ORCHESTRATOR WORKFLOW INTEGRATION
 *
 * Wrapper functions that use workflow patterns for orchestrator operations
 * Maintains backward compatibility while enabling workflow-based execution
 */

#include "nous/orchestrator.h"
#include "nous/workflow.h"
#include "nous/patterns.h"
#include "nous/nous.h"
#include "nous/planning.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// WORKFLOW-BASED PARALLEL ANALYSIS (v2)
// ============================================================================

/**
 * @brief Parallel analysis using workflow pattern (v2 - workflow-based)
 * @param input Input to analyze
 * @param agent_names Array of agent names
 * @param agent_count Number of agents
 * @return Synthesized analysis result, or NULL on failure
 * @note Caller must free() the returned string
 * 
 * This is the workflow-based version of orchestrator_parallel_analyze.
 * It uses the parallel_analysis pattern for better state management and checkpointing.
 */
char* orchestrator_parallel_analyze_v2(const char* input, const char** agent_names, size_t agent_count) {
    if (!input || !agent_names || agent_count == 0) {
        return NULL;
    }
    
    // Get agent IDs from names
    SemanticID* agent_ids = malloc(sizeof(SemanticID) * agent_count);
    if (!agent_ids) {
        return NULL;
    }
    
    // Find agents by name
    extern ManagedAgent* agent_find_by_name(const char* name);
    
    for (size_t i = 0; i < agent_count; i++) {
        ManagedAgent* agent = agent_find_by_name(agent_names[i]);
        if (agent) {
            agent_ids[i] = agent->id;
        } else {
            agent_ids[i] = 0;
            LOG_WARN(LOG_CAT_AGENT, "Agent '%s' not found, skipping", agent_names[i]);
        }
    }
    
    // Find converger agent (use first agent or a synthesizer agent)
    SemanticID converger_id = agent_ids[0]; // Use first agent as converger
    
    // Create parallel analysis workflow
    Workflow* wf = pattern_create_parallel_analysis(
        agent_ids,
        agent_count,
        converger_id
    );
    
    if (!wf) {
        free(agent_ids);
        return NULL;
    }
    
    // Execute workflow
    char* output = NULL;
    int result = workflow_execute(wf, input, &output);
    
    // Cleanup
    free(agent_ids);
    workflow_destroy(wf);
    
    if (result != 0) {
        if (output) {
            free(output);
        }
        return NULL;
    }
    
    return output; // Caller must free
}

// ============================================================================
// WORKFLOW-BASED SEQUENTIAL PLANNING (v2)
// ============================================================================

/**
 * @brief Sequential planning using workflow pattern (v2 - workflow-based)
 * @param goal Overall goal to plan for
 * @param planner_names Array of planner agent names
 * @param planner_count Number of planners
 * @return Execution plan, or NULL on failure
 * 
 * This is the workflow-based version of sequential planning.
 * It uses the sequential_planning pattern for better state management.
 */
ExecutionPlan* orchestrator_sequential_plan_v2(const char* goal, const char** planner_names, size_t planner_count) {
    if (!goal || !planner_names || planner_count == 0) {
        return NULL;
    }
    
    // Get planner IDs from names
    SemanticID* planner_ids = malloc(sizeof(SemanticID) * planner_count);
    if (!planner_ids) {
        return NULL;
    }
    
    // Find planners by name
    extern ManagedAgent* agent_find_by_name(const char* name);
    
    for (size_t i = 0; i < planner_count; i++) {
        ManagedAgent* agent = agent_find_by_name(planner_names[i]);
        if (agent) {
            planner_ids[i] = agent->id;
        } else {
            planner_ids[i] = 0;
            LOG_WARN(LOG_CAT_AGENT, "Planner '%s' not found, skipping", planner_names[i]);
        }
    }
    
    // Create sequential planning workflow
    Workflow* wf = pattern_create_sequential_planning(
        planner_ids,
        planner_count
    );
    
    if (!wf) {
        free(planner_ids);
        return NULL;
    }
    
    // Execute workflow to generate plan
    char* plan_output = NULL;
    int result = workflow_execute(wf, goal, &plan_output);
    
    // Convert workflow output to ExecutionPlan (simplified)
    ExecutionPlan* plan = NULL;
    if (result == 0 && plan_output) {
        // TODO: Parse plan_output and create ExecutionPlan
        // For now, create a basic plan
        plan = orch_plan_create(goal);
        if (plan) {
            // Add plan steps from workflow state
            // This is a simplified version - real implementation would parse workflow state
        }
    }
    
    // Cleanup
    free(planner_ids);
    if (plan_output) {
        free(plan_output);
    }
    workflow_destroy(wf);
    
    return plan;
}

// ============================================================================
// WORKFLOW-BASED REVIEW-REFINE (v2)
// ============================================================================

/**
 * @brief Review-refine loop using workflow pattern (v2 - workflow-based)
 * @param input Initial input to refine
 * @param generator_name Generator agent name
 * @param reviewer_name Reviewer agent name
 * @param refiner_name Refiner agent name (optional, uses generator if NULL)
 * @param max_iterations Maximum refinement iterations
 * @return Refined output, or NULL on failure
 * @note Caller must free() the returned string
 */
char* orchestrator_review_refine_v2(
    const char* input,
    const char* generator_name,
    const char* reviewer_name,
    const char* refiner_name,
    int max_iterations
) {
    if (!input || !generator_name || !reviewer_name) {
        return NULL;
    }
    
    // Get agent IDs from names
    extern ManagedAgent* agent_find_by_name(const char* name);
    
    ManagedAgent* generator = agent_find_by_name(generator_name);
    ManagedAgent* reviewer = agent_find_by_name(reviewer_name);
    ManagedAgent* refiner = refiner_name ? agent_find_by_name(refiner_name) : NULL;
    
    if (!generator || !reviewer) {
        LOG_ERROR(LOG_CAT_AGENT, "Required agents not found: generator=%s, reviewer=%s",
                  generator_name, reviewer_name);
        return NULL;
    }
    
    SemanticID generator_id = generator->id;
    SemanticID reviewer_id = reviewer->id;
    SemanticID refiner_id = refiner ? refiner->id : generator_id;
    
    // Create review-refine workflow
    Workflow* wf = pattern_create_review_refine_loop(
        generator_id,
        reviewer_id,
        refiner_id,
        max_iterations > 0 ? max_iterations : 5
    );
    
    if (!wf) {
        return NULL;
    }
    
    // Execute workflow
    char* output = NULL;
    int result = workflow_execute(wf, input, &output);
    
    // Cleanup
    workflow_destroy(wf);
    
    if (result != 0) {
        if (output) {
            free(output);
        }
        return NULL;
    }
    
    return output; // Caller must free
}

// ============================================================================
// BACKWARD COMPATIBILITY HELPERS
// ============================================================================

/**
 * @brief Check if workflow-based orchestrator is available
 * @return true if workflow system is available, false otherwise
 */
bool orchestrator_workflow_available(void) {
    // Check if workflow system is initialized
    // For now, always return true (workflow system is always available)
    return true;
}

/**
 * @brief Get recommended orchestrator function (workflow vs legacy)
 * @param function_name Name of function (e.g., "parallel_analyze")
 * @return Recommended function name (e.g., "parallel_analyze_v2" or "parallel_analyze")
 */
const char* orchestrator_get_recommended_function(const char* function_name) {
    if (!function_name) {
        return NULL;
    }
    
    // If workflow system is available, recommend v2 functions
    if (orchestrator_workflow_available()) {
        if (strcmp(function_name, "parallel_analyze") == 0) {
            return "parallel_analyze_v2";
        }
        if (strcmp(function_name, "sequential_plan") == 0) {
            return "sequential_plan_v2";
        }
        if (strcmp(function_name, "review_refine") == 0) {
            return "review_refine_v2";
        }
    }
    
    // Default to legacy function
    return function_name;
}

