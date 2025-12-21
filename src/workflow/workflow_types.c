/**
 * CONVERGIO WORKFLOW TYPES
 *
 * Core data structures and memory management for workflow engine
 * Implements create/destroy functions with proper memory safety
 */

#include "nous/workflow.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// Observability functions
extern bool workflow_validate_name_safe(const char* name);
extern bool workflow_validate_key_safe(const char* key);
extern char* workflow_sanitize_value(const char* value);
extern void workflow_security_log(const Workflow* wf, const char* security_event, const char* details);

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_NAME_LENGTH 256
#define MAX_KEY_LENGTH 128
#define MAX_VALUE_LENGTH 4096
#define INITIAL_STATE_CAPACITY 16
#define INITIAL_NODE_CAPACITY 4

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Safe string duplication with validation
char* workflow_strdup(const char* str) {
    if (!str) {
        return NULL;
    }
    
    size_t len = strlen(str);
    if (len == 0) {
        return NULL;
    }
    
    char* copy = malloc(len + 1);
    if (!copy) {
        return NULL;
    }
    
    memcpy(copy, str, len + 1);
    return copy;
}

// Validate workflow name (unified with _safe version for security)
bool workflow_validate_name(const char* name) {
    // Use the safe version which has better security checks
    return workflow_validate_name_safe(name);
}

// Validate state key (unified with _safe version for security)
bool workflow_validate_key(const char* key) {
    // Use the safe version which has better security checks
    return workflow_validate_key_safe(key);
}

// ============================================================================
// WORKFLOW STATE MANAGEMENT
// ============================================================================

WorkflowState* workflow_state_create(void) {
    WorkflowState* state = calloc(1, sizeof(WorkflowState));
    if (!state) {
        return NULL;
    }
    
    state->entry_capacity = INITIAL_STATE_CAPACITY;
    state->entries = calloc(state->entry_capacity, sizeof(StateEntry));
    if (!state->entries) {
        free(state);
        state = NULL;
        return NULL;
    }
    
    state->entry_count = 0;
    return state;
}

void workflow_state_destroy(WorkflowState* state) {
    if (!state) {
        return;
    }
    
    if (state->entries) {
        for (size_t i = 0; i < state->entry_count; i++) {
            if (state->entries[i].key) {
                free(state->entries[i].key);
                state->entries[i].key = NULL;
            }
            if (state->entries[i].value) {
                free(state->entries[i].value);
                state->entries[i].value = NULL;
            }
        }
        free(state->entries);
        state->entries = NULL;
    }
    
    free(state);
    state = NULL;
}

int workflow_state_set(WorkflowState* state, const char* key, const char* value) {
    if (!state || !key || !value) {
        return -1;
    }
    
    if (!workflow_validate_key(key)) {
        return -1;
    }
    
    size_t value_len = strlen(value);
    if (value_len > MAX_VALUE_LENGTH) {
        return -1;
    }
    
    // Check if key already exists
    for (size_t i = 0; i < state->entry_count; i++) {
        if (state->entries[i].key && strcmp(state->entries[i].key, key) == 0) {
            // Update existing entry
            char* new_value = workflow_strdup(value);
            if (!new_value) {
                return -1;
            }
            free(state->entries[i].value);
            state->entries[i].value = new_value;
            state->entries[i].updated_at = time(NULL);
            return 0;
        }
    }
    
    // Add new entry
    if (state->entry_count >= state->entry_capacity) {
        // Grow capacity
        size_t new_capacity = state->entry_capacity * 2;
        StateEntry* new_entries = realloc(state->entries, new_capacity * sizeof(StateEntry));
        if (!new_entries) {
            return -1;
        }
        // Initialize new entries
        memset(new_entries + state->entry_capacity, 0, 
               (new_capacity - state->entry_capacity) * sizeof(StateEntry));
        state->entries = new_entries;
        state->entry_capacity = new_capacity;
    }
    
    state->entries[state->entry_count].key = workflow_strdup(key);
    state->entries[state->entry_count].value = workflow_strdup(value);
    if (!state->entries[state->entry_count].key || !state->entries[state->entry_count].value) {
        if (state->entries[state->entry_count].key) {
            free(state->entries[state->entry_count].key);
            state->entries[state->entry_count].key = NULL;
        }
        return -1;
    }
    
    state->entries[state->entry_count].updated_at = time(NULL);
    state->entry_count++;
    return 0;
}

const char* workflow_state_get(const WorkflowState* state, const char* key) {
    if (!state || !key) {
        return NULL;
    }
    
    for (size_t i = 0; i < state->entry_count; i++) {
        if (state->entries[i].key && strcmp(state->entries[i].key, key) == 0) {
            return state->entries[i].value;
        }
    }
    
    return NULL;
}

int workflow_state_clear(WorkflowState* state) {
    if (!state) {
        return -1;
    }
    
    for (size_t i = 0; i < state->entry_count; i++) {
        if (state->entries[i].key) {
            free(state->entries[i].key);
            state->entries[i].key = NULL;
        }
        if (state->entries[i].value) {
            free(state->entries[i].value);
            state->entries[i].value = NULL;
        }
    }
    
    state->entry_count = 0;
    return 0;
}

int workflow_state_remove(WorkflowState* state, const char* key) {
    if (!state || !key) {
        return -1;
    }
    
    for (size_t i = 0; i < state->entry_count; i++) {
        if (state->entries[i].key && strcmp(state->entries[i].key, key) == 0) {
            // Found entry, remove it
            free(state->entries[i].key);
            state->entries[i].key = NULL;
            free(state->entries[i].value);
            state->entries[i].value = NULL;
            
            // Shift remaining entries
            for (size_t j = i; j < state->entry_count - 1; j++) {
                state->entries[j] = state->entries[j + 1];
            }
            state->entry_count--;
            return 0;
        }
    }
    
    return -1; // Key not found
}

// ============================================================================
// WORKFLOW NODE MANAGEMENT
// ============================================================================

WorkflowNode* workflow_node_create(const char* name, NodeType type) {
    if (!name || !workflow_validate_name(name)) {
        return NULL;
    }
    
    if (type < NODE_TYPE_ACTION || type > NODE_TYPE_CONVERGE) {
        return NULL;
    }
    
    WorkflowNode* node = calloc(1, sizeof(WorkflowNode));
    if (!node) {
        return NULL;
    }
    
    node->name = workflow_strdup(name);
    if (!node->name) {
        free(node);
        node = NULL;
        return NULL;
    }
    
    node->type = type;
    node->node_id = 0; // Will be set when saved to DB
    node->agent_id = 0;
    node->action_prompt = NULL;
    node->condition_expr = NULL;
    node->next_nodes = NULL;
    node->next_node_count = 0;
    node->next_node_capacity = 0;
    node->fallback_node = NULL;
    node->node_data = NULL;
    node->created_at = time(NULL);
    
    return node;
}

void workflow_node_destroy(WorkflowNode* node) {
    if (!node) {
        return;
    }
    
    if (node->name) {
        free(node->name);
        node->name = NULL;
    }
    
    if (node->action_prompt) {
        free(node->action_prompt);
        node->action_prompt = NULL;
    }
    
    if (node->condition_expr) {
        free(node->condition_expr);
        node->condition_expr = NULL;
    }
    
    if (node->next_nodes) {
        free(node->next_nodes);
        node->next_nodes = NULL;
    }
    
    // Note: fallback_node and node_data are not destroyed here
    // They should be managed separately to avoid double-free
    
    free(node);
    node = NULL;
}

int workflow_node_add_edge(WorkflowNode* from, WorkflowNode* to, const char* condition) {
    if (!from || !to) {
        return -1;
    }
    
    // Grow array if needed
    if (from->next_node_count >= from->next_node_capacity) {
        size_t new_capacity = from->next_node_capacity == 0 ? 
            INITIAL_NODE_CAPACITY : from->next_node_capacity * 2;
        WorkflowNode** new_nodes = realloc(from->next_nodes, 
                                           new_capacity * sizeof(WorkflowNode*));
        if (!new_nodes) {
            return -1;
        }
        from->next_nodes = new_nodes;
        from->next_node_capacity = new_capacity;
    }
    
    from->next_nodes[from->next_node_count] = to;
    from->next_node_count++;
    
    // Set condition if provided
    if (condition) {
        if (from->condition_expr) {
            free(from->condition_expr);
            from->condition_expr = NULL;
        }
        from->condition_expr = workflow_strdup(condition);
        if (!from->condition_expr) {
            return -1;
        }
    }
    
    return 0;
}

int workflow_node_set_agent(WorkflowNode* node, SemanticID agent_id, const char* prompt) {
    if (!node) {
        return -1;
    }
    
    if (node->type != NODE_TYPE_ACTION) {
        return -1; // Only ACTION nodes can have agents
    }
    
    node->agent_id = agent_id;
    
    if (prompt) {
        if (node->action_prompt) {
            free(node->action_prompt);
            node->action_prompt = NULL;
        }
        node->action_prompt = workflow_strdup(prompt);
        if (!node->action_prompt) {
            return -1;
        }
    }
    
    return 0;
}

int workflow_node_set_fallback(WorkflowNode* node, WorkflowNode* fallback) {
    if (!node) {
        return -1;
    }
    
    node->fallback_node = fallback;
    return 0;
}

// ============================================================================
// WORKFLOW MANAGEMENT
// ============================================================================

Workflow* workflow_create(const char* name, const char* description, WorkflowNode* entry_node) {
    if (!name || !workflow_validate_name(name)) {
        return NULL;
    }
    
    Workflow* wf = calloc(1, sizeof(Workflow));
    if (!wf) {
        return NULL;
    }
    
    wf->name = workflow_strdup(name);
    if (!wf->name) {
        free(wf);
        wf = NULL;
        return NULL;
    }
    
    if (description) {
        wf->description = workflow_strdup(description);
        if (!wf->description) {
            free(wf->name);
            wf->name = NULL;
            free(wf);
            wf = NULL;
            return NULL;
        }
    }
    
    wf->entry_node = entry_node;
    wf->workflow_id = 0; // Will be set when saved to DB
    wf->status = WORKFLOW_STATUS_PENDING;
    wf->current_node_id = 0;
    wf->created_at = time(NULL);
    wf->updated_at = wf->created_at;
    wf->last_checkpoint_at = 0;
    wf->error_message = NULL;
    wf->metadata_json = NULL;
    
    // Create empty state
    wf->state = workflow_state_create();
    if (!wf->state) {
        free(wf->description);
        wf->description = NULL;
        free(wf->name);
        wf->name = NULL;
        free(wf);
        wf = NULL;
        return NULL;
    }
    
    return wf;
}

void workflow_destroy(Workflow* wf) {
    if (!wf) {
        return;
    }
    
    if (wf->name) {
        free(wf->name);
        wf->name = NULL;
    }
    
    if (wf->description) {
        free(wf->description);
        wf->description = NULL;
    }
    
    if (wf->error_message) {
        free(wf->error_message);
        wf->error_message = NULL;
    }
    
    if (wf->metadata_json) {
        free(wf->metadata_json);
        wf->metadata_json = NULL;
    }
    
    if (wf->state) {
        workflow_state_destroy(wf->state);
        wf->state = NULL;
    }
    
    // Note: entry_node is not destroyed here
    // It should be managed separately to avoid double-free
    
    free(wf);
    wf = NULL;
}

// ============================================================================
// CHECKPOINT MANAGEMENT
// ============================================================================

Checkpoint* checkpoint_create(uint64_t workflow_id, uint64_t node_id, const char* state_json) {
    if (!state_json) {
        return NULL;
    }
    
    Checkpoint* cp = calloc(1, sizeof(Checkpoint));
    if (!cp) {
        return NULL;
    }
    
    cp->workflow_id = workflow_id;
    cp->node_id = node_id;
    cp->checkpoint_id = 0; // Will be set when saved to DB
    cp->state_json = workflow_strdup(state_json);
    if (!cp->state_json) {
        free(cp);
        cp = NULL;
        return NULL;
    }
    
    cp->created_at = time(NULL);
    cp->metadata_json = NULL;
    
    return cp;
}

void checkpoint_destroy(Checkpoint* cp) {
    if (!cp) {
        return;
    }
    
    if (cp->state_json) {
        free(cp->state_json);
        cp->state_json = NULL;
    }
    
    if (cp->metadata_json) {
        free(cp->metadata_json);
        cp->metadata_json = NULL;
    }
    
    free(cp);
    cp = NULL;
}

