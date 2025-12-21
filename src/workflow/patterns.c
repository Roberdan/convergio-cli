/**
 * CONVERGIO WORKFLOW PATTERNS
 *
 * Reusable workflow pattern implementations
 */

#include "nous/patterns.h"
#include "nous/workflow.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// REVIEW-REFINE LOOP
// ============================================================================

Workflow* pattern_create_review_refine_loop(
    SemanticID generator_id,
    SemanticID critic_id,
    SemanticID refiner_id,
    int max_iterations
) {
    if (max_iterations <= 0) {
        max_iterations = 3;
    }
    
    // Create workflow
    Workflow* wf = workflow_create(
        "Review-Refine Loop",
        "Iterative refinement pattern with generator, critic, and refiner",
        NULL
    );
    
    if (!wf) {
        return NULL;
    }
    
    // Create nodes
    WorkflowNode* generate = workflow_node_create("Generate", NODE_TYPE_ACTION);
    WorkflowNode* review = workflow_node_create("Review", NODE_TYPE_ACTION);
    WorkflowNode* refine = workflow_node_create("Refine", NODE_TYPE_ACTION);
    WorkflowNode* decision = workflow_node_create("Decision", NODE_TYPE_DECISION);
    
    if (!generate || !review || !refine || !decision) {
        workflow_destroy(wf);
        return NULL;
    }
    
    // Set agents
    workflow_node_set_agent(generate, generator_id, "Generate initial output");
    workflow_node_set_agent(review, critic_id, "Review and provide feedback");
    workflow_node_set_agent(refine, refiner_id, "Refine based on feedback");
    
    // Set entry node
    wf->entry_node = generate;
    
    // Create edges
    workflow_node_add_edge(generate, review, NULL);
    workflow_node_add_edge(review, decision, NULL);
    workflow_node_add_edge(decision, refine, "iteration_count < max_iterations");
    workflow_node_add_edge(refine, review, NULL);
    workflow_node_set_fallback(decision, NULL); // Exit on max iterations
    
    return wf;
}

// ============================================================================
// PARALLEL ANALYSIS
// ============================================================================

Workflow* pattern_create_parallel_analysis(
    SemanticID* analyst_ids,
    size_t analyst_count,
    SemanticID converger_id
) {
    if (!analyst_ids || analyst_count == 0) {
        return NULL;
    }
    
    Workflow* wf = workflow_create(
        "Parallel Analysis",
        "Multiple analysts work in parallel, then converge",
        NULL
    );
    
    if (!wf) {
        return NULL;
    }
    
    // Create parallel node
    WorkflowNode* parallel = workflow_node_create("Parallel Analysis", NODE_TYPE_PARALLEL);
    WorkflowNode* converge = workflow_node_create("Converge", NODE_TYPE_CONVERGE);
    
    if (!parallel || !converge) {
        workflow_destroy(wf);
        return NULL;
    }
    
    // Create analyst nodes
    for (size_t i = 0; i < analyst_count; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Analyst %zu", i + 1);
        WorkflowNode* analyst = workflow_node_create(name, NODE_TYPE_ACTION);
        if (analyst) {
            workflow_node_set_agent(analyst, analyst_ids[i], "Analyze from your perspective");
            workflow_node_add_edge(parallel, analyst, NULL);
            workflow_node_add_edge(analyst, converge, NULL);
        }
    }
    
    workflow_node_set_agent(converge, converger_id, "Synthesize all analyses");
    
    wf->entry_node = parallel;
    
    return wf;
}

// ============================================================================
// SEQUENTIAL PLANNING
// ============================================================================

Workflow* pattern_create_sequential_planning(
    SemanticID* planner_ids,
    size_t planner_count
) {
    if (!planner_ids || planner_count == 0) {
        return NULL;
    }
    
    Workflow* wf = workflow_create(
        "Sequential Planning",
        "Chain of planners building on each other",
        NULL
    );
    
    if (!wf) {
        return NULL;
    }
    
    WorkflowNode* prev = NULL;
    
    for (size_t i = 0; i < planner_count; i++) {
        char name[64];
        snprintf(name, sizeof(name), "Planner %zu", i + 1);
        WorkflowNode* planner = workflow_node_create(name, NODE_TYPE_ACTION);
        
        if (!planner) {
            workflow_destroy(wf);
            return NULL;
        }
        
        workflow_node_set_agent(planner, planner_ids[i], "Plan next phase");
        
        if (prev) {
            workflow_node_add_edge(prev, planner, NULL);
        } else {
            wf->entry_node = planner;
        }
        
        prev = planner;
    }
    
    return wf;
}

// ============================================================================
// CONSENSUS BUILDING
// ============================================================================

Workflow* pattern_create_consensus_building(
    SemanticID* participant_ids,
    size_t participant_count,
    double consensus_threshold
) {
    if (!participant_ids || participant_count == 0) {
        return NULL;
    }
    
    Workflow* wf = workflow_create(
        "Consensus Building",
        "Multi-agent discussion to reach consensus",
        NULL
    );
    
    if (!wf) {
        return NULL;
    }
    
    // Create group chat node (simplified as action node)
    WorkflowNode* discuss = workflow_node_create("Discuss", NODE_TYPE_ACTION);
    WorkflowNode* check = workflow_node_create("Check Consensus", NODE_TYPE_DECISION);
    
    if (!discuss || !check) {
        workflow_destroy(wf);
        return NULL;
    }
    
    // Store consensus threshold in state
    char threshold_str[32];
    snprintf(threshold_str, sizeof(threshold_str), "%.2f", consensus_threshold);
    workflow_set_state(wf, "consensus_threshold", threshold_str);
    
    workflow_node_add_edge(discuss, check, NULL);
    workflow_node_add_edge(check, discuss, "consensus < threshold");
    workflow_node_set_fallback(check, NULL); // Exit when consensus reached
    
    wf->entry_node = discuss;
    
    return wf;
}

// ============================================================================
// PATTERN COMPOSITION
// ============================================================================

Workflow* pattern_compose(Workflow* wf1, Workflow* wf2, const char* join_condition) {
    if (!wf1 || !wf2) {
        return NULL;
    }
    
    // Find exit nodes of wf1
    // Connect to entry node of wf2
    // This is simplified - full implementation would traverse graph
    
    if (wf1->entry_node && wf2->entry_node) {
        // Add edge from last node of wf1 to entry of wf2
        // Simplified: assume wf1 has a single exit path
        WorkflowNode* last = wf1->entry_node;
        while (last && last->next_node_count > 0) {
            last = last->next_nodes[0];
        }
        
        if (last) {
            workflow_node_add_edge(last, wf2->entry_node, join_condition);
        }
    }
    
    return wf1;
}

