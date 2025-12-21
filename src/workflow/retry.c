/**
 * CONVERGIO WORKFLOW RETRY LOGIC
 *
 * Retry mechanism for failed workflow nodes
 */

#include "nous/workflow.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ============================================================================
// RETRY CONFIGURATION
// ============================================================================

typedef struct {
    int max_retries;
    int current_retry;
    time_t last_retry_at;
    int base_delay_seconds;  // Base delay for exponential backoff
    char* last_error;
    WorkflowErrorType last_error_type;
} RetryState;

// ============================================================================
// ERROR CLASSIFICATION
// ============================================================================

// Classify error as retryable or not
static bool is_retryable_error(WorkflowErrorType error_type) {
    switch (error_type) {
        case WORKFLOW_ERROR_TIMEOUT:
        case WORKFLOW_ERROR_NETWORK:
        case WORKFLOW_ERROR_LLM_DOWN:
        case WORKFLOW_ERROR_PROVIDER_UNAVAILABLE:
        case WORKFLOW_ERROR_RATE_LIMIT:
            return true; // Transient errors, retryable
        
        case WORKFLOW_ERROR_NONE:
        case WORKFLOW_ERROR_FILE_IO:
        case WORKFLOW_ERROR_CREDIT_EXHAUSTED:
        case WORKFLOW_ERROR_TOOL_FAILED:
        case WORKFLOW_ERROR_AGENT_NOT_FOUND:
        case WORKFLOW_ERROR_AUTHENTICATION:
        case WORKFLOW_ERROR_UNKNOWN:
            return false; // Permanent errors, not retryable
        
        default:
            return false;
    }
}

// Get error type from workflow error message
static WorkflowErrorType classify_error_from_message(const char* error_message) {
    if (!error_message) {
        return WORKFLOW_ERROR_UNKNOWN;
    }
    
    // Simple classification based on error message content
    if (strstr(error_message, "timeout") || strstr(error_message, "Timeout")) {
        return WORKFLOW_ERROR_TIMEOUT;
    }
    if (strstr(error_message, "network") || strstr(error_message, "Network") || 
        strstr(error_message, "connection") || strstr(error_message, "Connection")) {
        return WORKFLOW_ERROR_NETWORK;
    }
    if (strstr(error_message, "rate limit") || strstr(error_message, "Rate limit")) {
        return WORKFLOW_ERROR_RATE_LIMIT;
    }
    if (strstr(error_message, "credit") || strstr(error_message, "Credit") ||
        strstr(error_message, "quota") || strstr(error_message, "Quota")) {
        return WORKFLOW_ERROR_CREDIT_EXHAUSTED;
    }
    if (strstr(error_message, "authentication") || strstr(error_message, "Authentication") ||
        strstr(error_message, "unauthorized") || strstr(error_message, "Unauthorized")) {
        return WORKFLOW_ERROR_AUTHENTICATION;
    }
    if (strstr(error_message, "agent not found") || strstr(error_message, "Agent not found")) {
        return WORKFLOW_ERROR_AGENT_NOT_FOUND;
    }
    
    return WORKFLOW_ERROR_UNKNOWN;
}

// ============================================================================
// RETRY LOGIC
// ============================================================================

// Execute node with retry logic (exponential backoff)
int workflow_execute_with_retry(
    Workflow* wf,
    WorkflowNode* node,
    const char* input,
    char** output,
    int max_retries,
    int base_delay_seconds
) {
    if (!wf || !node) {
        return -1;
    }
    
    if (max_retries < 0) {
        max_retries = 0;
    }
    
    if (base_delay_seconds < 0) {
        base_delay_seconds = 1; // Default 1 second
    }
    
    int attempt = 0;
    int result = -1;
    WorkflowErrorType last_error_type = WORKFLOW_ERROR_UNKNOWN;
    
    while (attempt <= max_retries) {
        result = workflow_execute_node(wf, node, input, output);
        
        if (result == 0) {
            // Success
            return 0;
        }
        
        // Classify error
        if (wf->error_message) {
            last_error_type = classify_error_from_message(wf->error_message);
            
            // Check if error is retryable
            if (!is_retryable_error(last_error_type)) {
                // Non-retryable error, don't retry
                return -1;
            }
        }
        
        attempt++;
        
        if (attempt <= max_retries) {
            // Calculate exponential backoff delay: base_delay * 2^(attempt-1)
            // Cap at 60 seconds to avoid excessive delays
            int delay = base_delay_seconds * (int)pow(2, attempt - 1);
            if (delay > 60) {
                delay = 60;
            }
            
            // Wait before retry (exponential backoff)
            if (delay > 0) {
                sleep(delay);
            }
            
            // Log retry attempt
            if (wf && node) {
                extern void workflow_log_node_execution(const Workflow* wf, const WorkflowNode* node, const char* status, const char* details);
                char retry_msg[256];
                snprintf(retry_msg, sizeof(retry_msg), "retry attempt %d/%d (delay: %ds)", 
                         attempt, max_retries, delay);
                workflow_log_node_execution(wf, node, "retrying", retry_msg);
            }
        }
    }
    
    // All retries exhausted
    return -1;
}

// Check if node should be retried based on error type and retry policy
bool workflow_should_retry(WorkflowNode* node, int max_retries, WorkflowErrorType error_type) {
    if (!node || max_retries <= 0) {
        return false;
    }
    
    // Check if error is retryable
    if (!is_retryable_error(error_type)) {
        return false;
    }
    
    // Check node-specific retry policy (if implemented in node_data)
    // For now, use default policy: retry transient errors up to max_retries
    
    return true;
}

