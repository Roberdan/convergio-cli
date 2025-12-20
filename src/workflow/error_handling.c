/**
 * CONVERGIO WORKFLOW ERROR HANDLING
 *
 * Comprehensive error handling for workflow execution:
 * - Timeout handling
 * - Network errors
 * - File I/O errors
 * - Credit/budget exhaustion
 * - LLM service downtime
 * - Tool execution errors
 */

#include "nous/workflow.h"
#include "nous/orchestrator.h"
#include "nous/provider.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

// Helper function for string duplication (if not in workflow_types.c)
static char* workflow_strdup(const char* str) {
    if (!str) {
        return NULL;
    }
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, str, len + 1);
    }
    return dup;
}

// ============================================================================
// ERROR TYPES
// ============================================================================

typedef enum {
    WORKFLOW_ERROR_NONE = 0,
    WORKFLOW_ERROR_TIMEOUT,
    WORKFLOW_ERROR_NETWORK,
    WORKFLOW_ERROR_FILE_IO,
    WORKFLOW_ERROR_CREDIT_EXHAUSTED,
    WORKFLOW_ERROR_LLM_DOWN,
    WORKFLOW_ERROR_TOOL_FAILED,
    WORKFLOW_ERROR_AGENT_NOT_FOUND,
    WORKFLOW_ERROR_PROVIDER_UNAVAILABLE,
    WORKFLOW_ERROR_AUTHENTICATION,
    WORKFLOW_ERROR_RATE_LIMIT,
    WORKFLOW_ERROR_UNKNOWN
} WorkflowErrorType;

// ============================================================================
// TIMEOUT HANDLING
// ============================================================================

#ifndef DEFAULT_NODE_TIMEOUT_SECONDS
#define DEFAULT_NODE_TIMEOUT_SECONDS 300  // 5 minutes per nodo
#endif

/**
 * @brief Check if a node execution has timed out
 * @param start_time When the node execution started
 * @param timeout_seconds Maximum execution time
 * @return true if timed out, false otherwise
 */
bool workflow_check_timeout(time_t start_time, int timeout_seconds) {
    if (timeout_seconds <= 0) {
        timeout_seconds = DEFAULT_NODE_TIMEOUT_SECONDS;
    }
    time_t now = time(NULL);
    return (now - start_time) >= timeout_seconds;
}

/**
 * @brief Set timeout for a workflow node
 * @param wf The workflow
 * @param node The node
 * @param timeout_seconds Timeout in seconds (0 = default)
 * @return 0 on success, -1 on failure
 */
int workflow_set_node_timeout(Workflow* wf, WorkflowNode* node, int timeout_seconds) {
    if (!wf || !node) {
        return -1;
    }
    
    char timeout_key[128];
    snprintf(timeout_key, sizeof(timeout_key), "node_%llu_timeout", node->node_id);
    
    char timeout_str[32];
    snprintf(timeout_str, sizeof(timeout_str), "%d", timeout_seconds);
    
    return workflow_set_state(wf, timeout_key, timeout_str);
}

// ============================================================================
// NETWORK ERROR HANDLING
// ============================================================================

/**
 * @brief Check network connectivity
 * @return true if network is available, false otherwise
 */
bool workflow_check_network(void) {
    // Try to resolve a well-known host
    FILE* fp = popen("ping -c 1 -W 1000 8.8.8.8 > /dev/null 2>&1", "r");
    if (fp) {
        int status = pclose(fp);
        return (status == 0);
    }
    return false;
}

/**
 * @brief Handle network errors during workflow execution
 * @param wf The workflow
 * @param error_msg Error message to set
 * @return WorkflowErrorType
 */
WorkflowErrorType workflow_handle_network_error(Workflow* wf, const char* error_msg) {
    if (!wf) {
        return WORKFLOW_ERROR_NETWORK;
    }
    
    if (wf->error_message) {
        free(wf->error_message);
    }
    
    if (error_msg) {
        wf->error_message = workflow_strdup(error_msg);
    } else {
        wf->error_message = workflow_strdup("Network error: Unable to connect to required services");
    }
    
    wf->status = WORKFLOW_STATUS_FAILED;
    workflow_set_state(wf, "last_error_type", "network");
    workflow_set_state(wf, "last_error_time", "0"); // Will be set to current time
    
    return WORKFLOW_ERROR_NETWORK;
}

// ============================================================================
// FILE I/O ERROR HANDLING
// ============================================================================

/**
 * @brief Check if a file can be read
 * @param filepath Path to the file
 * @return true if readable, false otherwise
 */
bool workflow_check_file_readable(const char* filepath) {
    if (!filepath) {
        return false;
    }
    
    FILE* fp = fopen(filepath, "r");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}

/**
 * @brief Check if a file can be written
 * @param filepath Path to the file
 * @return true if writable, false otherwise
 */
bool workflow_check_file_writable(const char* filepath) {
    if (!filepath) {
        return false;
    }
    
    // Check if directory exists and is writable
    char* dir = strdup(filepath);
    char* last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (access(dir, W_OK) == 0) {
            free(dir);
            return true;
        }
    }
    free(dir);
    
    // Check if file exists and is writable
    if (access(filepath, W_OK) == 0) {
        return true;
    }
    
    // Check if we can create the file
    FILE* fp = fopen(filepath, "w");
    if (fp) {
        fclose(fp);
        unlink(filepath); // Clean up
        return true;
    }
    
    return false;
}

/**
 * @brief Handle file I/O errors during workflow execution
 * @param wf The workflow
 * @param filepath The file that caused the error
 * @param operation "read" or "write"
 * @return WorkflowErrorType
 */
WorkflowErrorType workflow_handle_file_io_error(Workflow* wf, const char* filepath, const char* operation) {
    if (!wf) {
        return WORKFLOW_ERROR_FILE_IO;
    }
    
    if (wf->error_message) {
        free(wf->error_message);
    }
    
    char error_msg[512];
    if (filepath && operation) {
        snprintf(error_msg, sizeof(error_msg), "File I/O error: Cannot %s file '%s' (errno: %d, %s)",
                 operation, filepath, errno, strerror(errno));
    } else {
        snprintf(error_msg, sizeof(error_msg), "File I/O error: %s", strerror(errno));
    }
    
    wf->error_message = workflow_strdup(error_msg);
    wf->status = WORKFLOW_STATUS_FAILED;
    workflow_set_state(wf, "last_error_type", "file_io");
    
    return WORKFLOW_ERROR_FILE_IO;
}

// ============================================================================
// CREDIT/BUDGET ERROR HANDLING
// ============================================================================

/**
 * @brief Check if budget is available for workflow execution
 * @param wf The workflow
 * @return true if budget available, false otherwise
 */
bool workflow_check_budget(Workflow* wf) {
    if (!wf) {
        return false;
    }
    
    // Check budget via orchestrator cost controller
    // This is a placeholder - actual implementation would check CostController
    const char* budget_exceeded = workflow_get_state_value(wf, "budget_exceeded");
    if (budget_exceeded && strcmp(budget_exceeded, "true") == 0) {
        return false;
    }
    
    return true;
}

/**
 * @brief Handle credit/budget exhaustion errors
 * @param wf The workflow
 * @return WorkflowErrorType
 */
WorkflowErrorType workflow_handle_credit_exhausted(Workflow* wf) {
    if (!wf) {
        return WORKFLOW_ERROR_CREDIT_EXHAUSTED;
    }
    
    if (wf->error_message) {
        free(wf->error_message);
    }
    
    wf->error_message = workflow_strdup("Credit exhausted: API budget limit reached. Cannot continue workflow execution.");
    wf->status = WORKFLOW_STATUS_FAILED;
    workflow_set_state(wf, "last_error_type", "credit_exhausted");
    workflow_set_state(wf, "budget_exceeded", "true");
    
    return WORKFLOW_ERROR_CREDIT_EXHAUSTED;
}

// ============================================================================
// LLM SERVICE DOWNTIME HANDLING
// ============================================================================

/**
 * @brief Check if LLM provider is available
 * @param provider_type The provider type to check
 * @return true if available, false otherwise
 */
bool workflow_check_llm_available(ProviderType provider_type) {
    // This is a placeholder - actual implementation would check provider status
    // For now, we assume provider is available if network is available
    return workflow_check_network();
}

/**
 * @brief Handle LLM service downtime errors
 * @param wf The workflow
 * @param provider_type The provider that is down
 * @return WorkflowErrorType
 */
WorkflowErrorType workflow_handle_llm_down(Workflow* wf, ProviderType provider_type) {
    if (!wf) {
        return WORKFLOW_ERROR_LLM_DOWN;
    }
    
    if (wf->error_message) {
        free(wf->error_message);
    }
    
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "LLM service down: Provider %d is unavailable. Workflow execution paused.",
             (int)provider_type);
    
    wf->error_message = workflow_strdup(error_msg);
    wf->status = WORKFLOW_STATUS_PAUSED; // Pause instead of fail - might recover
    workflow_set_state(wf, "last_error_type", "llm_down");
    workflow_set_state(wf, "provider_unavailable", "true");
    
    return WORKFLOW_ERROR_LLM_DOWN;
}

// ============================================================================
// TOOL EXECUTION ERROR HANDLING
// ============================================================================

/**
 * @brief Handle tool execution errors
 * @param wf The workflow
 * @param tool_name Name of the tool that failed
 * @param error_msg Error message from tool
 * @return WorkflowErrorType
 */
WorkflowErrorType workflow_handle_tool_error(Workflow* wf, const char* tool_name, const char* error_msg) {
    if (!wf) {
        return WORKFLOW_ERROR_TOOL_FAILED;
    }
    
    if (wf->error_message) {
        free(wf->error_message);
    }
    
    char full_error[512];
    if (tool_name && error_msg) {
        snprintf(full_error, sizeof(full_error), "Tool execution failed: Tool '%s' returned error: %s",
                 tool_name, error_msg);
    } else if (tool_name) {
        snprintf(full_error, sizeof(full_error), "Tool execution failed: Tool '%s' failed", tool_name);
    } else {
        snprintf(full_error, sizeof(full_error), "Tool execution failed: %s", error_msg ? error_msg : "Unknown error");
    }
    
    wf->error_message = workflow_strdup(full_error);
    wf->status = WORKFLOW_STATUS_FAILED;
    workflow_set_state(wf, "last_error_type", "tool_failed");
    workflow_set_state(wf, "failed_tool", tool_name ? tool_name : "unknown");
    
    return WORKFLOW_ERROR_TOOL_FAILED;
}

// ============================================================================
// COMPREHENSIVE ERROR HANDLING
// ============================================================================

/**
 * @brief Handle any error during workflow execution with retry logic
 * @param wf The workflow
 * @param node The node that failed
 * @param error_type Type of error
 * @param error_msg Error message
 * @return true if error is recoverable (should retry), false otherwise
 */
bool workflow_handle_error(Workflow* wf, WorkflowNode* node, WorkflowErrorType error_type, const char* error_msg) {
    if (!wf || !node) {
        return false;
    }
    
    // Set error message
    if (wf->error_message) {
        free(wf->error_message);
    }
    wf->error_message = error_msg ? workflow_strdup(error_msg) : workflow_strdup("Unknown error");
    
    // Determine if error is recoverable
    bool recoverable = false;
    
    switch (error_type) {
        case WORKFLOW_ERROR_TIMEOUT:
            // Timeout might be recoverable with retry
            recoverable = true;
            wf->status = WORKFLOW_STATUS_FAILED;
            workflow_set_state(wf, "last_error_type", "timeout");
            break;
            
        case WORKFLOW_ERROR_NETWORK:
            // Network errors are often transient
            recoverable = true;
            wf->status = WORKFLOW_STATUS_PAUSED; // Pause, might recover
            workflow_set_state(wf, "last_error_type", "network");
            break;
            
        case WORKFLOW_ERROR_FILE_IO:
            // File I/O errors might be recoverable (permissions, etc.)
            recoverable = false; // Usually not recoverable without user intervention
            wf->status = WORKFLOW_STATUS_FAILED;
            workflow_set_state(wf, "last_error_type", "file_io");
            break;
            
        case WORKFLOW_ERROR_CREDIT_EXHAUSTED:
            // Credit exhaustion is not recoverable without user action
            recoverable = false;
            wf->status = WORKFLOW_STATUS_FAILED;
            workflow_set_state(wf, "last_error_type", "credit_exhausted");
            break;
            
        case WORKFLOW_ERROR_LLM_DOWN:
            // LLM downtime might be temporary
            recoverable = true;
            wf->status = WORKFLOW_STATUS_PAUSED; // Pause, might recover
            workflow_set_state(wf, "last_error_type", "llm_down");
            break;
            
        case WORKFLOW_ERROR_TOOL_FAILED:
            // Tool failures might be recoverable
            recoverable = true;
            wf->status = WORKFLOW_STATUS_FAILED;
            workflow_set_state(wf, "last_error_type", "tool_failed");
            break;
            
        case WORKFLOW_ERROR_AGENT_NOT_FOUND:
        case WORKFLOW_ERROR_PROVIDER_UNAVAILABLE:
        case WORKFLOW_ERROR_AUTHENTICATION:
            // These are not recoverable
            recoverable = false;
            wf->status = WORKFLOW_STATUS_FAILED;
            workflow_set_state(wf, "last_error_type", "unrecoverable");
            break;
            
        case WORKFLOW_ERROR_RATE_LIMIT:
            // Rate limits are usually recoverable after delay
            recoverable = true;
            wf->status = WORKFLOW_STATUS_PAUSED; // Pause, retry after delay
            workflow_set_state(wf, "last_error_type", "rate_limit");
            break;
            
        default:
            recoverable = false;
            wf->status = WORKFLOW_STATUS_FAILED;
            workflow_set_state(wf, "last_error_type", "unknown");
            break;
    }
    
    // Record error timestamp
    char time_str[32];
    snprintf(time_str, sizeof(time_str), "%lld", (long long)time(NULL));
    workflow_set_state(wf, "last_error_time", time_str);
    
    return recoverable;
}

