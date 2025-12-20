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

// ============================================================================
// RETRY CONFIGURATION
// ============================================================================

typedef struct {
    int max_retries;
    int current_retry;
    time_t last_retry_at;
    int retry_delay_seconds;
    char* last_error;
} RetryState;

// ============================================================================
// RETRY LOGIC
// ============================================================================

// Execute node with retry logic
int workflow_execute_with_retry(
    Workflow* wf,
    WorkflowNode* node,
    const char* input,
    char** output,
    int max_retries,
    int retry_delay_seconds
) {
    if (!wf || !node) {
        return -1;
    }
    
    int attempt = 0;
    int result = -1;
    
    while (attempt <= max_retries) {
        result = workflow_execute_node(wf, node, input, output);
        
        if (result == 0) {
            // Success
            return 0;
        }
        
        attempt++;
        
        if (attempt <= max_retries) {
            // Wait before retry
            if (retry_delay_seconds > 0) {
                sleep(retry_delay_seconds);
            }
            
            // Log retry
            if (wf->error_message) {
                // Error message already set by workflow_execute_node
            }
        }
    }
    
    // All retries exhausted
    return -1;
}

// Check if node should be retried
bool workflow_should_retry(WorkflowNode* node, int max_retries) {
    // For now, always retry on failure
    // Full implementation would check node-specific retry policy
    (void)node;
    (void)max_retries;
    return true;
}

