/**
 * CONVERGIO WORKFLOW MONITOR
 *
 * Real-time ASCII visualization for workflow execution
 * Shows agents, their status, progress, and workflow structure
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
    AGENT_STATUS_FAILED      // Error occurred
} AgentStatus;

// ============================================================================
// WORKFLOW MONITOR STRUCTURES
// ============================================================================

typedef struct {
    char* name;              // Agent name (e.g., "rex-code-reviewer")
    char* task;              // Brief task description
    AgentStatus status;      // Current status
    struct timespec start;   // When agent started
    struct timespec end;     // When agent finished
    double duration_ms;      // Execution time in ms
} MonitoredAgent;

typedef struct {
    MonitoredAgent* agents;   // Array of agents
    size_t agent_count;       // Number of agents
    size_t capacity;          // Array capacity
    const char* workflow_name; // Name of this workflow
    struct timespec start;    // Overall start time
    bool is_active;           // Is monitoring active?
    bool use_ansi;            // Use ANSI codes for dynamic update
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

#endif // CONVERGIO_WORKFLOW_MONITOR_H
