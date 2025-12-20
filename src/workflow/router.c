/**
 * CONVERGIO CONDITIONAL ROUTER
 *
 * Secure condition evaluation for workflow routing
 */

#include "nous/router.h"
#include "nous/workflow.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// CONDITION PARSING (Simplified, secure)
// ============================================================================

// Simple condition evaluator (supports: key == value, key != value)
// Full implementation would use proper expression parser
static bool evaluate_simple_condition(const char* expr, const WorkflowState* state) {
    if (!expr || !state) {
        return false;
    }
    
    // Parse: "key == value" or "key != value"
    char* expr_copy = strdup(expr);
    if (!expr_copy) {
        return false;
    }
    
    // Find operator
    char* op_pos = strstr(expr_copy, "==");
    bool is_equal = true;
    
    if (!op_pos) {
        op_pos = strstr(expr_copy, "!=");
        is_equal = false;
    }
    
    if (!op_pos) {
        free(expr_copy);
        return false; // Invalid expression
    }
    
    // Split into key and value
    *op_pos = '\0';
    char* key = expr_copy;
    char* value = op_pos + (is_equal ? 2 : 2);
    
    // Trim whitespace
    while (isspace((unsigned char)*key)) key++;
    while (isspace((unsigned char)*value)) value++;
    
    char* key_end = key + strlen(key) - 1;
    while (key_end > key && isspace((unsigned char)*key_end)) {
        *key_end = '\0';
        key_end--;
    }
    
    // Get state value
    const char* state_value = workflow_state_get(state, key);
    
    bool result = false;
    if (state_value) {
        if (is_equal) {
            result = (strcmp(state_value, value) == 0);
        } else {
            result = (strcmp(state_value, value) != 0);
        }
    } else {
        result = !is_equal; // Key not found, != returns true
    }
    
    free(expr_copy);
    return result;
}

// ============================================================================
// CONDITION EVALUATION
// ============================================================================

bool router_evaluate_condition(
    const char* condition_expr,
    const WorkflowState* state
) {
    if (!condition_expr || !state) {
        return false;
    }
    
    // Empty condition is always true
    if (strlen(condition_expr) == 0) {
        return true;
    }
    
    // For now, support simple conditions
    // Full implementation would parse complex expressions
    return evaluate_simple_condition(condition_expr, state);
}

// ============================================================================
// ROUTING
// ============================================================================

WorkflowNode* router_get_next_node(
    Workflow* wf,
    WorkflowNode* current,
    const WorkflowState* state
) {
    if (!wf || !current || !state) {
        return NULL;
    }
    
    // If no next nodes, return NULL
    if (current->next_node_count == 0) {
        return NULL;
    }
    
    // Evaluate conditions for each next node
    for (size_t i = 0; i < current->next_node_count; i++) {
        WorkflowNode* next = current->next_nodes[i];
        
        if (!next) {
            continue;
        }
        
        // If node has condition, evaluate it
        if (current->condition_expr) {
            if (router_evaluate_condition(current->condition_expr, state)) {
                return next;
            }
        } else {
            // No condition, use as default
            return next;
        }
    }
    
    // No condition matched, use fallback
    if (current->fallback_node) {
        return current->fallback_node;
    }
    
    // No fallback, return first next node
    if (current->next_node_count > 0) {
        return current->next_nodes[0];
    }
    
    return NULL;
}

