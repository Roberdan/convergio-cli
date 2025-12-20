/**
 * CONVERGIO WORKFLOW ENGINE
 *
 * State machine-based workflow orchestration system
 * Supports checkpointing, conditional routing, and multi-agent coordination
 */

#ifndef CONVERGIO_WORKFLOW_H
#define CONVERGIO_WORKFLOW_H

#include "nous/nous.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ============================================================================
// WORKFLOW TYPES
// ============================================================================

// Workflow node types
typedef enum {
    NODE_TYPE_ACTION = 0,        // Execute agent action
    NODE_TYPE_DECISION = 1,      // Conditional routing
    NODE_TYPE_HUMAN_INPUT = 2,   // Wait for user input
    NODE_TYPE_SUBGRAPH = 3,      // Nested workflow
    NODE_TYPE_PARALLEL = 4,     // Parallel execution
    NODE_TYPE_CONVERGE = 5       // Converge parallel results
} NodeType;

// Workflow status
typedef enum {
    WORKFLOW_STATUS_PENDING = 0,
    WORKFLOW_STATUS_RUNNING = 1,
    WORKFLOW_STATUS_PAUSED = 2,      // Waiting for human input
    WORKFLOW_STATUS_COMPLETED = 3,
    WORKFLOW_STATUS_FAILED = 4,
    WORKFLOW_STATUS_CANCELLED = 5
} WorkflowStatus;

// Workflow node structure
typedef struct WorkflowNode {
    uint64_t node_id;
    char* name;
    NodeType type;
    SemanticID agent_id;          // Agent to execute (for ACTION nodes)
    char* action_prompt;          // What the agent should do
    char* condition_expr;         // Condition for conditional edges
    struct WorkflowNode** next_nodes;  // Possible next nodes
    size_t next_node_count;
    size_t next_node_capacity;
    struct WorkflowNode* fallback_node; // Fallback if condition fails
    void* node_data;              // Type-specific data
    time_t created_at;
} WorkflowNode;

// Workflow state entry (key-value pair)
typedef struct {
    char* key;
    char* value;
    time_t updated_at;
} StateEntry;

// Workflow state (key-value store)
typedef struct {
    StateEntry* entries;
    size_t entry_count;
    size_t entry_capacity;
} WorkflowState;

// Workflow checkpoint
typedef struct {
    uint64_t checkpoint_id;
    uint64_t workflow_id;
    uint64_t node_id;
    char* state_json;             // Serialized workflow state
    time_t created_at;
    char* metadata_json;         // Additional checkpoint metadata
} Checkpoint;

// Workflow structure
typedef struct {
    uint64_t workflow_id;
    char* name;
    char* description;
    WorkflowNode* entry_node;
    WorkflowState* state;
    WorkflowStatus status;
    uint64_t current_node_id;
    time_t created_at;
    time_t updated_at;
    time_t last_checkpoint_at;
    char* error_message;
    char* metadata_json;
} Workflow;

// ============================================================================
// WORKFLOW LIFECYCLE
// ============================================================================

// Create and destroy workflows
Workflow* workflow_create(const char* name, const char* description, WorkflowNode* entry_node);
void workflow_destroy(Workflow* wf);

// ============================================================================
// NODE MANAGEMENT
// ============================================================================

// Create and destroy nodes
WorkflowNode* workflow_node_create(const char* name, NodeType type);
void workflow_node_destroy(WorkflowNode* node);

// Node operations
int workflow_node_add_edge(WorkflowNode* from, WorkflowNode* to, const char* condition);
int workflow_node_set_agent(WorkflowNode* node, SemanticID agent_id, const char* prompt);
int workflow_node_set_fallback(WorkflowNode* node, WorkflowNode* fallback);

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

// Create and destroy state
WorkflowState* workflow_state_create(void);
void workflow_state_destroy(WorkflowState* state);

// State operations
int workflow_state_set(WorkflowState* state, const char* key, const char* value);
const char* workflow_state_get(const WorkflowState* state, const char* key);
int workflow_state_clear(WorkflowState* state);
int workflow_state_remove(WorkflowState* state, const char* key);

// ============================================================================
// CHECKPOINT MANAGEMENT
// ============================================================================

// Checkpoint operations
uint64_t workflow_checkpoint(Workflow* wf, const char* node_name);
int workflow_restore_from_checkpoint(Workflow* wf, uint64_t checkpoint_id);
Checkpoint* workflow_list_checkpoints(Workflow* wf, size_t* count);
void workflow_free_checkpoints(Checkpoint* checkpoints, size_t count);

// ============================================================================
// WORKFLOW EXECUTION
// ============================================================================

// Core execution
int workflow_execute(Workflow* wf, const char* input, char** output);
int workflow_execute_node(Workflow* wf, WorkflowNode* node, const char* input, char** output);
WorkflowNode* workflow_get_next_node(Workflow* wf, WorkflowNode* current);
WorkflowNode* workflow_get_current_node(Workflow* wf);

// Workflow control
int workflow_pause(Workflow* wf);
int workflow_cancel(Workflow* wf);
int workflow_resume(Workflow* wf, uint64_t checkpoint_id);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// String validation and allocation helpers
char* workflow_strdup(const char* str);
bool workflow_validate_name(const char* name);
bool workflow_validate_key(const char* key);

#endif // CONVERGIO_WORKFLOW_H

