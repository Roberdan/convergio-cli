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

// Security validation functions
extern bool workflow_validate_condition_safe(const char* condition);
extern bool workflow_validate_key_safe(const char* key);
extern void workflow_security_log(const Workflow* wf, const char* security_event, const char* details);

// ============================================================================
// CONDITION PARSING (Simplified, secure)
// ============================================================================

// Simple condition evaluator (supports: key == value, key != value, key < value, key > value, key <= value, key >= value)
// Full implementation would use proper expression parser
static bool evaluate_simple_condition(const char* expr, const WorkflowState* state) {
    if (!expr || !state) {
        return false;
    }
    
    // Parse: "key == value", "key != value", "key < value", "key > value", "key <= value", "key >= value"
    char* expr_copy = workflow_strdup(expr);
    if (!expr_copy) {
        return false;
    }
    
    // Find operator (check longer operators first to avoid partial matches)
    char* op_pos = NULL;
    int op_len = 0;
    int op_type = 0; // 0: ==, 1: !=, 2: <=, 3: >=, 4: <, 5: >
    
    if ((op_pos = strstr(expr_copy, "<="))) {
        op_type = 2;
        op_len = 2;
    } else if ((op_pos = strstr(expr_copy, ">="))) {
        op_type = 3;
        op_len = 2;
    } else if ((op_pos = strstr(expr_copy, "=="))) {
        op_type = 0;
        op_len = 2;
    } else if ((op_pos = strstr(expr_copy, "!="))) {
        op_type = 1;
        op_len = 2;
    } else if ((op_pos = strchr(expr_copy, '<'))) {
        op_type = 4;
        op_len = 1;
    } else if ((op_pos = strchr(expr_copy, '>'))) {
        op_type = 5;
        op_len = 1;
    }
    
    if (!op_pos) {
        free(expr_copy);
        return false; // Invalid expression
    }
    
    // Split into key and value
    *op_pos = '\0';
    char* key = expr_copy;
    char* value = op_pos + op_len;
    
    // Trim whitespace from key
    while (isspace((unsigned char)*key)) key++;
    char* key_end = key + strlen(key) - 1;
    while (key_end > key && isspace((unsigned char)*key_end)) {
        *key_end = '\0';
        key_end--;
    }
    
    // Trim whitespace from value
    while (isspace((unsigned char)*value)) value++;
    char* value_end = value + strlen(value) - 1;
    while (value_end > value && isspace((unsigned char)*value_end)) {
        *value_end = '\0';
        value_end--;
    }

    // Remove quotes from value if present (e.g., 'active' or "active" -> active)
    size_t value_len = strlen(value);
    if (value_len >= 2 &&
        ((value[0] == '\'' && value[value_len - 1] == '\'') ||
         (value[0] == '"' && value[value_len - 1] == '"'))) {
        value[value_len - 1] = '\0';  // Remove trailing quote
        value++;                       // Skip leading quote
    }
    
    // Validate key (security: prevent injection)
    if (!workflow_validate_key_safe(key)) {
        free(expr_copy);
        return false;
    }
    
    // Get state value
    const char* state_value = workflow_state_get(state, key);
    
    bool result = false;
    if (state_value) {
        switch (op_type) {
            case 0: // ==
                result = (strcmp(state_value, value) == 0);
                break;
            case 1: // !=
                result = (strcmp(state_value, value) != 0);
                break;
            case 2: // <=
                result = (strcmp(state_value, value) <= 0);
                break;
            case 3: // >=
                result = (strcmp(state_value, value) >= 0);
                break;
            case 4: // <
                result = (strcmp(state_value, value) < 0);
                break;
            case 5: // >
                result = (strcmp(state_value, value) > 0);
                break;
            default:
                result = false;
                break;
        }
    } else {
        // Key not found
        switch (op_type) {
            case 1: // !=
                result = true; // Key not found, != returns true
                break;
            default:
                result = false; // Other operators return false when key not found
                break;
        }
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
    
    // Security validation: check for dangerous patterns
    if (!workflow_validate_condition_safe(condition_expr)) {
        return false; // Invalid condition, reject for security
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
    // Note: Current design has condition_expr on the current node, not per edge
    // For proper conditional routing, we should check conditions on next nodes
    // For now, if current node has a condition, evaluate it for all next nodes
    // If no condition, return first next node (default routing)
    
    if (current->condition_expr) {
        // Validate condition expression for security
        if (!workflow_validate_condition_safe(current->condition_expr)) {
            // Security violation: invalid condition expression
            if (wf) {
                workflow_security_log(wf, "invalid_condition_expr", current->condition_expr);
            }
            // Use fallback or first node on security violation
            if (current->fallback_node) {
                return current->fallback_node;
            }
            if (current->next_node_count > 0) {
                return current->next_nodes[0];
            }
            return NULL;
        }
        
        // Evaluate condition
        bool condition_met = router_evaluate_condition(current->condition_expr, state);
        
        if (condition_met) {
            // Condition met, return first next node
            if (current->next_node_count > 0) {
                return current->next_nodes[0];
            }
        } else {
            // Condition not met, use fallback
            if (current->fallback_node) {
                return current->fallback_node;
            }
            // No fallback, return first next node anyway (default behavior)
            if (current->next_node_count > 0) {
                return current->next_nodes[0];
            }
        }
    } else {
        // No condition, use first next node (default routing)
        if (current->next_node_count > 0) {
            return current->next_nodes[0];
        }
    }
    
    return NULL;
}

