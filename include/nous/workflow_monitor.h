/**
 * CONVERGIO WORKFLOW MONITOR
 *
 * Real-time ASCII visualization for workflow execution
 * Shows agents, their status, progress, and workflow structure
 *
 * Supports:
 * - Parallel execution (all agents at once)
 * - Sequential execution (one after another)
 * - Pipeline execution (output feeds into next)
 * - Conditional/decision nodes
 * - Nested workflows (phases, sub-workflows)
 */

#ifndef CONVERGIO_WORKFLOW_MONITOR_H
#define CONVERGIO_WORKFLOW_MONITOR_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// ============================================================================
// AGENT STATUS
// ============================================================================

typedef enum {
    AGENT_STATUS_PENDING,    // Not yet started
    AGENT_STATUS_THINKING,   // Currently processing
    AGENT_STATUS_COMPLETED,  // Successfully finished
    AGENT_STATUS_FAILED,     // Error occurred
    AGENT_STATUS_SKIPPED,    // Skipped (e.g., conditional branch not taken)
    AGENT_STATUS_WAITING     // Waiting for dependency
} AgentStatus;

// ============================================================================
// WORKFLOW TYPES
// ============================================================================

typedef enum {
    WORKFLOW_PARALLEL,       // All agents execute in parallel
    WORKFLOW_SEQUENTIAL,     // Agents execute one after another
    WORKFLOW_PIPELINE,       // Output of one feeds into next
    WORKFLOW_CONDITIONAL     // Decision-based routing
} WorkflowType;

// ============================================================================
// NODE TYPES (for complex workflows)
// ============================================================================

typedef enum {
    NODE_AGENT,              // A single agent
    NODE_DECISION,           // Decision/conditional node
    NODE_GROUP,              // Group of parallel agents
    NODE_PHASE               // Named phase containing nodes
} NodeType;

// ============================================================================
// WORKFLOW MONITOR STRUCTURES
// ============================================================================

// Forward declaration
struct WorkflowNode;

typedef struct {
    char* name;              // Agent name (e.g., "rex-code-reviewer")
    char* task;              // Brief task description
    AgentStatus status;      // Current status
    struct timespec start;   // When agent started
    struct timespec end;     // When agent finished
    double duration_ms;      // Execution time in ms
    int parent_idx;          // Parent node index (-1 for root level)
    int depth;               // Nesting depth for rendering
} MonitoredAgent;

typedef struct WorkflowNode {
    NodeType type;           // Type of node
    char* label;             // Display label
    AgentStatus status;      // Node status
    int* children;           // Array of child indices
    size_t child_count;      // Number of children
    size_t child_capacity;   // Children array capacity
    char* condition;         // Condition expression (for DECISION nodes)
    int parent_idx;          // Parent node index (-1 for root)
    int depth;               // Nesting depth
    struct timespec start;   // When node started
    struct timespec end;     // When node finished
    double duration_ms;      // Execution time
} WorkflowNode;

typedef struct {
    // Legacy flat agent list (for simple parallel workflows)
    MonitoredAgent* agents;   // Array of agents
    size_t agent_count;       // Number of agents
    size_t capacity;          // Array capacity

    // Extended node structure (for complex workflows)
    WorkflowNode* nodes;      // Array of nodes
    size_t node_count;        // Number of nodes
    size_t node_capacity;     // Nodes array capacity

    // Workflow metadata
    const char* workflow_name; // Name of this workflow
    WorkflowType type;         // Workflow execution type
    struct timespec start;     // Overall start time
    bool is_active;            // Is monitoring active?
    bool use_ansi;             // Use ANSI codes for dynamic update
    bool use_nodes;            // Use extended node structure

    // Phase tracking
    int current_phase;         // Current executing phase index
    char** phase_names;        // Array of phase names
    size_t phase_count;        // Number of phases
} WorkflowMonitor;

// ============================================================================
// WORKFLOW MONITOR API
// ============================================================================

/**
 * @brief Create a new workflow monitor
 * @param workflow_name Name for this workflow (e.g., "delegation")
 * @param use_ansi If true, use ANSI escape codes for dynamic terminal update
 * @return New monitor instance, or NULL on failure
 */
WorkflowMonitor* workflow_monitor_create(const char* workflow_name, bool use_ansi);

/**
 * @brief Free workflow monitor
 */
void workflow_monitor_free(WorkflowMonitor* monitor);

/**
 * @brief Add an agent to monitor
 * @param monitor The monitor instance
 * @param name Agent name
 * @param task Brief task description
 * @return Index of agent, or -1 on failure
 */
int workflow_monitor_add_agent(WorkflowMonitor* monitor, const char* name, const char* task);

/**
 * @brief Update agent status
 * @param monitor The monitor instance
 * @param agent_idx Agent index from workflow_monitor_add_agent
 * @param status New status
 */
void workflow_monitor_set_status(WorkflowMonitor* monitor, int agent_idx, AgentStatus status);

/**
 * @brief Find agent by name and update status
 * @param monitor The monitor instance
 * @param name Agent name
 * @param status New status
 */
void workflow_monitor_set_status_by_name(WorkflowMonitor* monitor, const char* name, AgentStatus status);

/**
 * @brief Render current state to terminal
 * @param monitor The monitor instance
 *
 * If use_ansi is true, this will clear and redraw the visualization.
 * Call this periodically during workflow execution.
 */
void workflow_monitor_render(WorkflowMonitor* monitor);

/**
 * @brief Render final summary
 * @param monitor The monitor instance
 */
void workflow_monitor_render_summary(WorkflowMonitor* monitor);

/**
 * @brief Get ASCII status icon for agent status
 */
const char* workflow_monitor_status_icon(AgentStatus status);

/**
 * @brief Get status name string
 */
const char* workflow_monitor_status_name(AgentStatus status);

/**
 * @brief Start monitoring (records start time)
 */
void workflow_monitor_start(WorkflowMonitor* monitor);

/**
 * @brief Stop monitoring (records end time)
 */
void workflow_monitor_stop(WorkflowMonitor* monitor);

// ============================================================================
// EXTENDED API FOR COMPLEX WORKFLOWS
// ============================================================================

/**
 * @brief Create a workflow monitor with specific type
 * @param workflow_name Name for this workflow
 * @param type Workflow execution type (parallel, sequential, etc.)
 * @param use_ansi Use ANSI escape codes for dynamic terminal update
 * @return New monitor instance, or NULL on failure
 */
WorkflowMonitor* workflow_monitor_create_typed(const char* workflow_name, WorkflowType type,
                                                bool use_ansi);

/**
 * @brief Add a phase to the workflow
 * @param monitor The monitor instance
 * @param phase_name Name of the phase
 * @return Phase index, or -1 on failure
 */
int workflow_monitor_add_phase(WorkflowMonitor* monitor, const char* phase_name);

/**
 * @brief Add a node to the workflow
 * @param monitor The monitor instance
 * @param type Node type
 * @param label Display label
 * @param parent_idx Parent node index (-1 for root)
 * @return Node index, or -1 on failure
 */
int workflow_monitor_add_node(WorkflowMonitor* monitor, NodeType type, const char* label,
                               int parent_idx);

/**
 * @brief Add an agent to a specific phase
 * @param monitor The monitor instance
 * @param phase_idx Phase index
 * @param name Agent name
 * @param task Brief task description
 * @return Agent index, or -1 on failure
 */
int workflow_monitor_add_agent_to_phase(WorkflowMonitor* monitor, int phase_idx,
                                         const char* name, const char* task);

/**
 * @brief Set a decision node's condition
 * @param monitor The monitor instance
 * @param node_idx Node index
 * @param condition Condition expression to display
 */
void workflow_monitor_set_condition(WorkflowMonitor* monitor, int node_idx, const char* condition);

/**
 * @brief Set node status
 * @param monitor The monitor instance
 * @param node_idx Node index
 * @param status New status
 */
void workflow_monitor_set_node_status(WorkflowMonitor* monitor, int node_idx, AgentStatus status);

/**
 * @brief Set current phase
 * @param monitor The monitor instance
 * @param phase_idx Phase index to mark as current
 */
void workflow_monitor_set_current_phase(WorkflowMonitor* monitor, int phase_idx);

/**
 * @brief Get workflow type name as string
 */
const char* workflow_monitor_type_name(WorkflowType type);

/**
 * @brief Render complex workflow with phases and nodes
 * @param monitor The monitor instance
 *
 * Renders hierarchical view with phases, groups, and decision branches
 */
void workflow_monitor_render_complex(WorkflowMonitor* monitor);

/**
 * @brief Create a sequential workflow from agent names
 * @param workflow_name Name for this workflow
 * @param agent_names Array of agent names
 * @param tasks Array of task descriptions
 * @param count Number of agents
 * @param use_ansi Use ANSI escape codes
 * @return New monitor instance, or NULL on failure
 *
 * Convenience function for creating sequential workflows
 */
WorkflowMonitor* workflow_monitor_create_sequential(const char* workflow_name,
                                                     const char** agent_names,
                                                     const char** tasks,
                                                     size_t count,
                                                     bool use_ansi);

/**
 * @brief Create a pipeline workflow from agent names
 * @param workflow_name Name for this workflow
 * @param agent_names Array of agent names
 * @param tasks Array of task descriptions
 * @param count Number of agents
 * @param use_ansi Use ANSI escape codes
 * @return New monitor instance, or NULL on failure
 *
 * Similar to sequential but shows data flow between agents
 */
WorkflowMonitor* workflow_monitor_create_pipeline(const char* workflow_name,
                                                   const char** agent_names,
                                                   const char** tasks,
                                                   size_t count,
                                                   bool use_ansi);

#endif // CONVERGIO_WORKFLOW_MONITOR_H
