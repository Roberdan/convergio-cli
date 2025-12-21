/**
 * CONVERGIO WORKFLOW ENGINE
 *
 * Core state machine execution engine for workflows
 * Handles linear workflow execution with state transitions
 */

#include "nous/workflow.h"
#include "nous/orchestrator.h"
#include "nous/provider.h"
#include "nous/router.h"
#include "nous/nous.h"
#include "nous/telemetry.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// Error handling functions (defined in error_handling.c)
// Observability functions (defined in workflow_observability.c)
extern void workflow_log_event(LogLevel level, const char* event, const char* workflow_name, uint64_t workflow_id, const char* details);
extern void workflow_log_node_execution(const Workflow* wf, const WorkflowNode* node, const char* status, const char* details);
extern void workflow_log_error(const Workflow* wf, const char* error_type, const char* error_message);
extern void workflow_telemetry_start(const Workflow* wf);
extern void workflow_telemetry_end(const Workflow* wf, bool success);
extern void workflow_telemetry_node(const Workflow* wf, const WorkflowNode* node, bool success, double latency_ms);
extern void workflow_telemetry_error(const Workflow* wf, const char* error_type);
extern void workflow_audit_log(const Workflow* wf, const char* operation, const char* details);
extern void workflow_security_log(const Workflow* wf, const char* security_event, const char* details);
extern bool workflow_validate_name_safe(const char* name);
extern bool workflow_validate_key_safe(const char* key);
extern char* workflow_sanitize_value(const char* value);
extern bool workflow_validate_condition_safe(const char* condition);

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

extern Provider* provider_get(ProviderType type);
extern size_t agent_get_all(ManagedAgent** out_agents, size_t max_count);

// ============================================================================
// STATE MANAGEMENT HELPERS
// ============================================================================

WorkflowState* workflow_get_state(Workflow* wf) {
    if (!wf) {
        return NULL;
    }
    return wf->state;
}

int workflow_set_state(Workflow* wf, const char* key, const char* value) {
    if (!wf || !wf->state) {
        return -1;
    }
    return workflow_state_set(wf->state, key, value);
}

const char* workflow_get_state_value(Workflow* wf, const char* key) {
    if (!wf || !wf->state) {
        return NULL;
    }
    return workflow_state_get(wf->state, key);
}

int workflow_clear_state(Workflow* wf) {
    if (!wf || !wf->state) {
        return -1;
    }
    return workflow_state_clear(wf->state);
}

// ============================================================================
// NODE EXECUTION
// ============================================================================

// Execute an ACTION node (agent execution)
static int execute_action_node(Workflow* wf, WorkflowNode* node, const char* input, char** output) {
    if (!wf || !node || node->type != NODE_TYPE_ACTION) {
        return -1;
    }
    
    if (node->agent_id == 0) {
        // No agent assigned
        if (wf->error_message) {
            free(wf->error_message);
            wf->error_message = NULL;
        }
        wf->error_message = workflow_strdup("Action node has no agent assigned");
        return -1;
    }
    
    // Find agent by ID
    ManagedAgent* agent = NULL;
    ManagedAgent* agents[256];
    size_t agent_count = agent_get_all(agents, 256);
    for (size_t i = 0; i < agent_count; i++) {
        if (agents[i] && agents[i]->id == node->agent_id) {
            agent = agents[i];
            break;
        }
    }
    
    if (!agent) {
        if (wf->error_message) {
            free(wf->error_message);
            wf->error_message = NULL;
        }
        wf->error_message = workflow_strdup("Agent not found");
        return -1;
    }
    
    // Build prompt from action_prompt and input
    char* effective_prompt = NULL;
    if (node->action_prompt) {
        size_t prompt_len = strlen(node->action_prompt);
        size_t input_len = input ? strlen(input) : 0;
        size_t total_len = prompt_len + input_len + 64; // Extra space for formatting
        
        effective_prompt = malloc(total_len);
        if (!effective_prompt) {
            return -1;
        }
        
        if (input && strlen(input) > 0) {
            snprintf(effective_prompt, total_len, "%s\n\nInput: %s", node->action_prompt, input);
        } else {
            strncpy(effective_prompt, node->action_prompt, total_len - 1);
            effective_prompt[total_len - 1] = '\0';
        }
    } else if (input) {
        effective_prompt = workflow_strdup(input);
        if (!effective_prompt) {
            return -1;
        }
    } else {
        effective_prompt = workflow_strdup("Execute task");
        if (!effective_prompt) {
            return -1;
        }
    }
    
    // Ethical guardrails: Validate content before execution
    EthicalResult ethical_check = workflow_validate_ethical(effective_prompt);
    if (ethical_check == ETHICAL_BLOCK) {
        if (wf->error_message) {
            free(wf->error_message);
            wf->error_message = NULL;
        }
        wf->error_message = workflow_strdup("Action blocked by ethical guardrails");
        workflow_security_log(wf, "ethical_block", "Content blocked by ethical validation");
        free(effective_prompt);
        return -1;
    }

    if (ethical_check == ETHICAL_HUMAN_REVIEW) {
        SensitiveCategory category = SENSITIVE_NONE;
        workflow_is_sensitive_operation(effective_prompt, &category);
        if (!workflow_request_human_approval(effective_prompt, category)) {
            if (wf->error_message) {
                free(wf->error_message);
                wf->error_message = NULL;
            }
            wf->error_message = workflow_strdup("Action requires human approval which was denied");
            workflow_security_log(wf, "human_approval_denied", "Human-in-the-loop approval denied");
            free(effective_prompt);
            return -1;
        }
        workflow_security_log(wf, "human_approval_granted", "Human-in-the-loop approval granted");
    }

    // Pre-execution checks
    time_t start_time = time(NULL);

    // Check network connectivity
    if (!workflow_check_network(5)) {
        workflow_handle_network_error(wf, "Network unavailable: Cannot connect to required services");
        free(effective_prompt);
        return -1;
    }
    
    // Check budget/credit
    if (!workflow_check_budget(wf)) {
        workflow_handle_credit_exhausted(wf);
        free(effective_prompt);
        return -1;
    }
    
    // Execute agent via provider
    Provider* provider = provider_get(PROVIDER_ANTHROPIC);
    if (!provider || !provider->chat) {
        free(effective_prompt);
        workflow_handle_error(wf, node, WORKFLOW_ERROR_PROVIDER_UNAVAILABLE, "Provider not available");
        return -1;
    }
    
    // Check if LLM service is available
    if (!workflow_check_llm_available(PROVIDER_ANTHROPIC)) {
        workflow_handle_llm_down(wf, PROVIDER_ANTHROPIC);
        free(effective_prompt);
        return -1;
    }
    
    // Check timeout before execution
    int timeout_seconds = 300; // Default 5 minutes
    const char* timeout_str = workflow_get_state_value(wf, "node_timeout");
    if (timeout_str) {
        timeout_seconds = atoi(timeout_str);
    }
    
    TokenUsage usage = {0};
    char* response = provider->chat(
        provider,
        "claude-sonnet-4-20250514",
        agent->system_prompt,
        effective_prompt,
        &usage
    );
    
    free(effective_prompt);
    effective_prompt = NULL;
    
    // Check timeout after execution
    if (workflow_check_timeout(start_time, timeout_seconds)) {
        if (response) {
            free(response);
        }
        workflow_handle_error(wf, node, WORKFLOW_ERROR_TIMEOUT, "Node execution exceeded timeout");
        return -1;
    }
    
    if (!response) {
        // Provider chat failed - this could be network, auth, rate limit, etc.
        // We can't distinguish easily, so we mark as general failure
        workflow_handle_error(wf, node, WORKFLOW_ERROR_UNKNOWN, "Agent execution failed - check network, API key, and credit");
        return -1;
    }
    
    // Store result in state
    char state_key[64];
    snprintf(state_key, sizeof(state_key), "node_%llu_result", (unsigned long long)node->node_id);
    workflow_set_state(wf, state_key, response);
    
    if (output) {
        *output = workflow_strdup(response);
    }
    
    // Record cost (approximate token counting)
    extern void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens, uint64_t output_tokens);
    cost_record_agent_usage(agent, usage.input_tokens, usage.output_tokens);
    
    free(response);
    response = NULL;
    
    return 0;
}

// Execute a node based on its type
int workflow_execute_node(Workflow* wf, WorkflowNode* node, const char* input, char** output) {
    if (!wf || !node) {
        return -1;
    }
    
    // Update current node
    wf->current_node_id = node->node_id;
    wf->updated_at = time(NULL);
    
    switch (node->type) {
        case NODE_TYPE_ACTION:
            return execute_action_node(wf, node, input, output);
            
        case NODE_TYPE_DECISION:
            // Decision nodes are handled by workflow_get_next_node
            // Just mark as executed
            return 0;
            
        case NODE_TYPE_HUMAN_INPUT:
            // Pause workflow and wait for input
            wf->status = WORKFLOW_STATUS_PAUSED;
            return 0;
            
        case NODE_TYPE_SUBGRAPH:
        case NODE_TYPE_PARALLEL:
        case NODE_TYPE_CONVERGE:
            // These will be implemented in later phases
            if (wf->error_message) {
                free(wf->error_message);
                wf->error_message = NULL;
            }
            wf->error_message = workflow_strdup("Node type not yet implemented");
            return -1;
            
        default:
            if (wf->error_message) {
                free(wf->error_message);
                wf->error_message = NULL;
            }
            wf->error_message = workflow_strdup("Unknown node type");
            return -1;
    }
}

// ============================================================================
// NODE NAVIGATION
// ============================================================================

// Get next node based on current node and workflow state
WorkflowNode* workflow_get_next_node(Workflow* wf, WorkflowNode* current) {
    if (!wf || !current) {
        return NULL;
    }
    
    // If no next nodes, workflow is complete
    if (current->next_node_count == 0) {
        return NULL;
    }
    
    // Use router for conditional routing (if conditions are present)
    // Otherwise, use simple linear routing
    if (current->condition_expr || current->fallback_node) {
        // Use conditional router
        return router_get_next_node(wf, current, wf->state);
    }
    
    // Simple linear routing: return first next node
    if (current->next_node_count > 0) {
        return current->next_nodes[0];
    }
    
    return NULL;
}

// Get current node from workflow
WorkflowNode* workflow_get_current_node(Workflow* wf) {
    if (!wf || !wf->entry_node) {
        return NULL;
    }
    
    // For Phase 1, we only support linear workflows
    // Start from entry node and traverse
    WorkflowNode* current = wf->entry_node;
    uint64_t target_id = wf->current_node_id;
    
    if (target_id == 0) {
        return wf->entry_node;
    }
    
    // Simple linear traversal (will be improved in later phases)
    while (current) {
        if (current->node_id == target_id) {
            return current;
        }
        if (current->next_node_count > 0) {
            current = current->next_nodes[0];
        } else {
            break;
        }
    }
    
    return wf->entry_node; // Fallback to entry
}

// ============================================================================
// WORKFLOW EXECUTION - Helper Functions
// ============================================================================

// Handle node execution failure
static void handle_node_failure(Workflow* wf, WorkflowNode* node, 
                                char* current_input, char* node_output,
                                double latency_ms) {
    wf->status = WORKFLOW_STATUS_FAILED;
    
    // Logging: Log node failure
    workflow_log_node_execution(wf, node, "failed", wf->error_message);
    workflow_log_error(wf, "node_execution_failed", wf->error_message);
    
    // Telemetry: Record node failure
    workflow_telemetry_node(wf, node, false, latency_ms);
    workflow_telemetry_error(wf, "node_execution_failed");
    
    // Audit: Log failure
    workflow_audit_log(wf, "node_execute_failed", node->name ? node->name : "unnamed");
    
    // Cleanup
    if (current_input) {
        free(current_input);
    }
    if (node_output) {
        free(node_output);
    }
    
    // Telemetry: Record workflow end (failure)
    workflow_telemetry_end(wf, false);
    workflow_log_event(LOG_LEVEL_ERROR, "workflow_execute_failed", wf->name, wf->workflow_id, wf->error_message);
}

// Handle node execution success
static void handle_node_success(Workflow* wf, WorkflowNode* node, double latency_ms) {
    // Logging: Log node success
    workflow_log_node_execution(wf, node, "completed", NULL);
    
    // Telemetry: Record node success
    workflow_telemetry_node(wf, node, true, latency_ms);
}

// Prepare workflow for execution
static int prepare_workflow_execution(Workflow* wf, const char* input) {
    if (!wf->entry_node) {
        if (wf->error_message) {
            free(wf->error_message);
            wf->error_message = NULL;
        }
        wf->error_message = workflow_strdup("Workflow has no entry node");
        return -1;
    }
    
    // Set initial state
    wf->status = WORKFLOW_STATUS_RUNNING;
    wf->current_node_id = wf->entry_node->node_id;
    wf->updated_at = time(NULL);
    
    if (wf->error_message) {
        free(wf->error_message);
        wf->error_message = NULL;
    }
    
    // Store input in state
    if (input) {
        workflow_set_state(wf, "input", input);
    }
    
    return 0;
}

// ============================================================================
// WORKFLOW EXECUTION
// ============================================================================

// Execute a workflow from start to completion (linear workflows only for Phase 1)
int workflow_execute(Workflow* wf, const char* input, char** output) {
    if (!wf) {
        return -1;
    }
    
    // Prepare workflow for execution
    if (prepare_workflow_execution(wf, input) != 0) {
        return -1;
    }
    
    // Execute linear workflow
    WorkflowNode* current = wf->entry_node;
    char* current_input = input ? workflow_strdup(input) : NULL;
    char* node_output = NULL;
    
    while (current) {
        // Logging: Log node execution start
        workflow_log_node_execution(wf, current, "executing", NULL);
        
        // Audit: Log node execution
        workflow_audit_log(wf, "node_execute", current->name ? current->name : "unnamed");
        
        time_t node_start = time(NULL);
        
        // Execute current node
        int result = workflow_execute_node(wf, current, current_input, &node_output);
        
        time_t node_end = time(NULL);
        double latency_ms = (node_end - node_start) * 1000.0;
        
        if (result != 0) {
            // Execution failed
            handle_node_failure(wf, current, current_input, node_output, latency_ms);
            return -1;
        }
        
        // Logging: Log node success
        handle_node_success(wf, current, latency_ms);
        
        // Check if workflow was paused (human input required)
        if (wf->status == WORKFLOW_STATUS_PAUSED) {
            if (current_input) {
                free(current_input);
            }
            if (node_output) {
                free(node_output);
            }
            return 0; // Paused, not an error
        }
        
        // Get next node
        WorkflowNode* next = workflow_get_next_node(wf, current);
        
        // Update input for next node (use output from current)
        if (current_input) {
            free(current_input);
        }
        current_input = node_output;
        node_output = NULL;
        
        current = next;
    }
    
    // Workflow completed successfully
    wf->status = WORKFLOW_STATUS_COMPLETED;
    wf->updated_at = time(NULL);
    
    // Get final output from state
    if (output) {
        const char* final_output = workflow_get_state_value(wf, "output");
        if (final_output) {
            *output = workflow_strdup(final_output);
        } else if (current_input) {
            *output = workflow_strdup(current_input);
        } else {
            *output = workflow_strdup("Workflow completed");
        }
    }
    
    if (current_input) {
        free(current_input);
    }
    
    // Telemetry: Record workflow end (success)
    workflow_telemetry_end(wf, true);
    workflow_log_event(LOG_LEVEL_INFO, "workflow_execute_completed", wf->name, wf->workflow_id, NULL);
    
    return 0;
}

// ============================================================================
// WORKFLOW CONTROL
// ============================================================================

int workflow_pause(Workflow* wf) {
    if (!wf) {
        return -1;
    }
    
    if (wf->status != WORKFLOW_STATUS_RUNNING) {
        return -1;
    }
    
    wf->status = WORKFLOW_STATUS_PAUSED;
    wf->updated_at = time(NULL);
    return 0;
}

int workflow_cancel(Workflow* wf) {
    if (!wf) {
        return -1;
    }
    
    if (wf->status == WORKFLOW_STATUS_COMPLETED || 
        wf->status == WORKFLOW_STATUS_CANCELLED) {
        return -1;
    }
    
    wf->status = WORKFLOW_STATUS_CANCELLED;
    wf->updated_at = time(NULL);
    return 0;
}

int workflow_resume(Workflow* wf, uint64_t checkpoint_id) {
    // Checkpoint restoration will be implemented in F4
    (void)checkpoint_id;
    
    if (!wf) {
        return -1;
    }
    
    if (wf->status != WORKFLOW_STATUS_PAUSED) {
        return -1;
    }
    
    wf->status = WORKFLOW_STATUS_RUNNING;
    wf->updated_at = time(NULL);
    return 0;
}

