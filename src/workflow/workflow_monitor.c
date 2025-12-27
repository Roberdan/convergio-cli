/**
 * CONVERGIO WORKFLOW MONITOR
 *
 * Real-time ASCII visualization for workflow execution
 * Shows agents, their status, progress, and workflow structure
 */

#include "nous/workflow_monitor.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// ANSI ESCAPE CODES
// ============================================================================

#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_DIM        "\033[2m"
#define ANSI_GREEN      "\033[32m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_RED        "\033[31m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_BLUE       "\033[34m"
#define ANSI_MAGENTA    "\033[35m"
#define ANSI_CLEAR_LINE "\033[2K"
#define ANSI_CURSOR_UP  "\033[%dA"
#define ANSI_HIDE_CURSOR "\033[?25l"
#define ANSI_SHOW_CURSOR "\033[?25h"

// Box drawing characters (UTF-8)
#define BOX_H   "─"
#define BOX_V   "│"
#define BOX_TL  "┌"
#define BOX_TR  "┐"
#define BOX_BL  "└"
#define BOX_BR  "┘"
#define BOX_LT  "├"
#define BOX_RT  "┤"
#define BOX_TB  "┬"
#define BOX_BT  "┴"
#define BOX_X   "┼"

// Status icons
#define ICON_PENDING   "○"
#define ICON_THINKING  "◐"
#define ICON_COMPLETED "●"
#define ICON_FAILED    "✗"
#define ICON_SKIPPED   "⊘"
#define ICON_WAITING   "◷"

// Workflow icons
#define ICON_DECISION  "◇"
#define ICON_GROUP     "▣"
#define ICON_PHASE     "▶"
#define ICON_ARROW     "→"
#define ICON_PIPELINE  "⟶"

// Progress bar
#define PROGRESS_FILLED "█"
#define PROGRESS_EMPTY  "░"

// ============================================================================
// STATUS HELPERS
// ============================================================================

const char* workflow_monitor_status_icon(AgentStatus status) {
    switch (status) {
    case AGENT_STATUS_PENDING:
        return ICON_PENDING;
    case AGENT_STATUS_THINKING:
        return ICON_THINKING;
    case AGENT_STATUS_COMPLETED:
        return ICON_COMPLETED;
    case AGENT_STATUS_FAILED:
        return ICON_FAILED;
    case AGENT_STATUS_SKIPPED:
        return ICON_SKIPPED;
    case AGENT_STATUS_WAITING:
        return ICON_WAITING;
    default:
        return "?";
    }
}

const char* workflow_monitor_status_name(AgentStatus status) {
    switch (status) {
    case AGENT_STATUS_PENDING:
        return "pending";
    case AGENT_STATUS_THINKING:
        return "thinking";
    case AGENT_STATUS_COMPLETED:
        return "completed";
    case AGENT_STATUS_FAILED:
        return "failed";
    case AGENT_STATUS_SKIPPED:
        return "skipped";
    case AGENT_STATUS_WAITING:
        return "waiting";
    default:
        return "unknown";
    }
}

const char* workflow_monitor_type_name(WorkflowType type) {
    switch (type) {
    case WORKFLOW_PARALLEL:
        return "parallel";
    case WORKFLOW_SEQUENTIAL:
        return "sequential";
    case WORKFLOW_PIPELINE:
        return "pipeline";
    case WORKFLOW_CONDITIONAL:
        return "conditional";
    default:
        return "unknown";
    }
}

static const char* status_color(AgentStatus status) {
    switch (status) {
    case AGENT_STATUS_PENDING:
        return ANSI_DIM;
    case AGENT_STATUS_THINKING:
        return ANSI_YELLOW;
    case AGENT_STATUS_COMPLETED:
        return ANSI_GREEN;
    case AGENT_STATUS_FAILED:
        return ANSI_RED;
    case AGENT_STATUS_SKIPPED:
        return ANSI_BLUE;
    case AGENT_STATUS_WAITING:
        return ANSI_MAGENTA;
    default:
        return ANSI_RESET;
    }
}

// ============================================================================
// CREATE / FREE
// ============================================================================

WorkflowMonitor* workflow_monitor_create(const char* workflow_name, bool use_ansi) {
    WorkflowMonitor* monitor = calloc(1, sizeof(WorkflowMonitor));
    if (!monitor) {
        return NULL;
    }

    monitor->capacity = 16;
    monitor->agents = calloc(monitor->capacity, sizeof(MonitoredAgent));
    if (!monitor->agents) {
        free(monitor);
        return NULL;
    }

    monitor->workflow_name = workflow_name ? strdup(workflow_name) : strdup("workflow");
    monitor->type = WORKFLOW_PARALLEL;  // Default to parallel
    monitor->use_ansi = use_ansi;
    monitor->is_active = false;
    monitor->use_nodes = false;
    monitor->current_phase = -1;

    return monitor;
}

WorkflowMonitor* workflow_monitor_create_typed(const char* workflow_name, WorkflowType type,
                                                bool use_ansi) {
    WorkflowMonitor* monitor = workflow_monitor_create(workflow_name, use_ansi);
    if (monitor) {
        monitor->type = type;
        monitor->use_nodes = true;

        // Allocate node array
        monitor->node_capacity = 16;
        monitor->nodes = calloc(monitor->node_capacity, sizeof(WorkflowNode));
        if (!monitor->nodes) {
            workflow_monitor_free(monitor);
            return NULL;
        }
    }
    return monitor;
}

void workflow_monitor_free(WorkflowMonitor* monitor) {
    if (!monitor) {
        return;
    }

    // Free agents
    for (size_t i = 0; i < monitor->agent_count; i++) {
        free(monitor->agents[i].name);
        free(monitor->agents[i].task);
    }
    free(monitor->agents);

    // Free nodes
    if (monitor->nodes) {
        for (size_t i = 0; i < monitor->node_count; i++) {
            free(monitor->nodes[i].label);
            free(monitor->nodes[i].condition);
            free(monitor->nodes[i].children);
        }
        free(monitor->nodes);
    }

    // Free phase names
    if (monitor->phase_names) {
        for (size_t i = 0; i < monitor->phase_count; i++) {
            free(monitor->phase_names[i]);
        }
        free(monitor->phase_names);
    }

    free((void*)monitor->workflow_name);
    free(monitor);
}

// ============================================================================
// AGENT MANAGEMENT
// ============================================================================

int workflow_monitor_add_agent(WorkflowMonitor* monitor, const char* name, const char* task) {
    if (!monitor || !name) {
        return -1;
    }

    // Expand if needed
    if (monitor->agent_count >= monitor->capacity) {
        size_t new_capacity = monitor->capacity * 2;
        MonitoredAgent* new_agents = realloc(monitor->agents,
                                              new_capacity * sizeof(MonitoredAgent));
        if (!new_agents) {
            return -1;
        }
        monitor->agents = new_agents;
        monitor->capacity = new_capacity;
    }

    int idx = (int)monitor->agent_count;
    monitor->agents[idx].name = strdup(name);
    monitor->agents[idx].task = task ? strdup(task) : strdup("");
    monitor->agents[idx].status = AGENT_STATUS_PENDING;
    monitor->agents[idx].duration_ms = 0;
    monitor->agent_count++;

    return idx;
}

void workflow_monitor_set_status(WorkflowMonitor* monitor, int agent_idx, AgentStatus status) {
    if (!monitor || agent_idx < 0 || (size_t)agent_idx >= monitor->agent_count) {
        return;
    }

    MonitoredAgent* agent = &monitor->agents[agent_idx];
    AgentStatus old_status = agent->status;
    agent->status = status;

    // Record timing
    if (status == AGENT_STATUS_THINKING && old_status == AGENT_STATUS_PENDING) {
        clock_gettime(CLOCK_MONOTONIC, &agent->start);
    } else if ((status == AGENT_STATUS_COMPLETED || status == AGENT_STATUS_FAILED) &&
               old_status == AGENT_STATUS_THINKING) {
        clock_gettime(CLOCK_MONOTONIC, &agent->end);
        agent->duration_ms = ((agent->end.tv_sec - agent->start.tv_sec) * 1000.0) +
                            ((agent->end.tv_nsec - agent->start.tv_nsec) / 1000000.0);
    }
}

void workflow_monitor_set_status_by_name(WorkflowMonitor* monitor, const char* name,
                                          AgentStatus status) {
    if (!monitor || !name) {
        return;
    }

    for (size_t i = 0; i < monitor->agent_count; i++) {
        if (monitor->agents[i].name && strcmp(monitor->agents[i].name, name) == 0) {
            workflow_monitor_set_status(monitor, (int)i, status);
            return;
        }
    }
}

// ============================================================================
// TIMING
// ============================================================================

void workflow_monitor_start(WorkflowMonitor* monitor) {
    if (!monitor) {
        return;
    }
    clock_gettime(CLOCK_MONOTONIC, &monitor->start);
    monitor->is_active = true;
}

void workflow_monitor_stop(WorkflowMonitor* monitor) {
    if (!monitor) {
        return;
    }
    monitor->is_active = false;
}

// ============================================================================
// RENDERING
// ============================================================================

static void render_header(WorkflowMonitor* monitor, FILE* out) {
    fprintf(out, "\n");
    fprintf(out, "%s%s WORKFLOW: %s %s\n",
            ANSI_BOLD, ANSI_CYAN, monitor->workflow_name, ANSI_RESET);
    fprintf(out, "%s══════════════════════════════════════════════════%s\n\n",
            ANSI_DIM, ANSI_RESET);
}

static void render_progress_bar(WorkflowMonitor* monitor, FILE* out) {
    size_t completed = 0;
    size_t in_progress = 0;
    size_t failed = 0;

    for (size_t i = 0; i < monitor->agent_count; i++) {
        switch (monitor->agents[i].status) {
        case AGENT_STATUS_COMPLETED:
            completed++;
            break;
        case AGENT_STATUS_THINKING:
            in_progress++;
            break;
        case AGENT_STATUS_FAILED:
            failed++;
            break;
        default:
            break;
        }
    }

    size_t total = monitor->agent_count;
    if (total == 0) {
        total = 1;
    }

    int bar_width = 30;
    int filled = (int)((completed * bar_width) / total);
    int partial = (int)((in_progress * bar_width) / total);
    if (partial == 0 && in_progress > 0) partial = 1;

    fprintf(out, "  Progress: [");

    // Completed portion (green)
    fprintf(out, "%s", ANSI_GREEN);
    for (int i = 0; i < filled; i++) {
        fprintf(out, "%s", PROGRESS_FILLED);
    }

    // In progress portion (yellow)
    fprintf(out, "%s", ANSI_YELLOW);
    for (int i = 0; i < partial && (filled + i) < bar_width; i++) {
        fprintf(out, "%s", PROGRESS_FILLED);
    }

    // Empty portion
    fprintf(out, "%s", ANSI_DIM);
    for (int i = filled + partial; i < bar_width; i++) {
        fprintf(out, "%s", PROGRESS_EMPTY);
    }

    fprintf(out, "%s] %zu/%zu", ANSI_RESET, completed, total);

    if (failed > 0) {
        fprintf(out, " %s(%zu failed)%s", ANSI_RED, failed, ANSI_RESET);
    }

    fprintf(out, "\n\n");
}

static void render_agent_tree(WorkflowMonitor* monitor, FILE* out) {
    fprintf(out, "  %s%s%s Ali (orchestrator)\n", ANSI_CYAN, ICON_COMPLETED, ANSI_RESET);

    for (size_t i = 0; i < monitor->agent_count; i++) {
        MonitoredAgent* agent = &monitor->agents[i];
        bool is_last = (i == monitor->agent_count - 1);

        // Tree branch
        if (is_last) {
            fprintf(out, "   %s%s%s ", ANSI_DIM, BOX_BL, BOX_H);
        } else {
            fprintf(out, "   %s%s%s ", ANSI_DIM, BOX_LT, BOX_H);
        }

        // Status icon with color
        fprintf(out, "%s%s%s ", status_color(agent->status),
                workflow_monitor_status_icon(agent->status), ANSI_RESET);

        // Agent name
        fprintf(out, "%s%-25s%s ", ANSI_BOLD, agent->name, ANSI_RESET);

        // Status and timing
        fprintf(out, "%s[%s]%s", status_color(agent->status),
                workflow_monitor_status_name(agent->status), ANSI_RESET);

        if (agent->duration_ms > 0) {
            fprintf(out, " %.1fs", agent->duration_ms / 1000.0);
        }

        fprintf(out, "\n");

        // Task description on next line if not empty
        if (agent->task && agent->task[0] != '\0') {
            if (is_last) {
                fprintf(out, "        %s%s%s\n", ANSI_DIM, agent->task, ANSI_RESET);
            } else {
                fprintf(out, "   %s%s%s   %s%s%s\n", ANSI_DIM, BOX_V, ANSI_RESET,
                        ANSI_DIM, agent->task, ANSI_RESET);
            }
        }
    }

    fprintf(out, "\n");
}

static void render_legend(FILE* out) {
    fprintf(out, "  %sLegend:%s %s%s pending%s  %s%s thinking%s  %s%s completed%s  %s%s failed%s\n\n",
            ANSI_DIM, ANSI_RESET,
            ANSI_DIM, ICON_PENDING, ANSI_RESET,
            ANSI_YELLOW, ICON_THINKING, ANSI_RESET,
            ANSI_GREEN, ICON_COMPLETED, ANSI_RESET,
            ANSI_RED, ICON_FAILED, ANSI_RESET);
}

void workflow_monitor_render(WorkflowMonitor* monitor) {
    if (!monitor) {
        return;
    }

    FILE* out = stderr;  // Use stderr for status output

    // For ANSI mode, move cursor up to redraw
    // This creates the "dynamic" effect
    static int last_line_count = 0;

    if (monitor->use_ansi && last_line_count > 0) {
        // Move cursor up and clear lines
        for (int i = 0; i < last_line_count; i++) {
            fprintf(out, "%s\033[1A", ANSI_CLEAR_LINE);
        }
    }

    // Count lines we'll output
    int line_count = 4;  // Header + divider + progress + blank
    line_count += 1;     // Ali line
    line_count += (int)(monitor->agent_count * 2);  // Agent + task per agent
    line_count += 2;     // Legend + blank

    render_header(monitor, out);
    render_progress_bar(monitor, out);
    render_agent_tree(monitor, out);
    render_legend(out);

    fflush(out);
    last_line_count = line_count;
}

void workflow_monitor_render_summary(WorkflowMonitor* monitor) {
    if (!monitor) {
        return;
    }

    FILE* out = stderr;

    // Calculate totals
    size_t completed = 0;
    size_t failed = 0;
    size_t skipped = 0;
    double total_time = 0;

    for (size_t i = 0; i < monitor->agent_count; i++) {
        if (monitor->agents[i].status == AGENT_STATUS_COMPLETED) {
            completed++;
            total_time += monitor->agents[i].duration_ms;
        } else if (monitor->agents[i].status == AGENT_STATUS_FAILED) {
            failed++;
        } else if (monitor->agents[i].status == AGENT_STATUS_SKIPPED) {
            skipped++;
        }
    }

    fprintf(out, "\n");
    fprintf(out, "%s══════════════════════════════════════════════════%s\n",
            ANSI_DIM, ANSI_RESET);
    fprintf(out, "%s%s WORKFLOW COMPLETE %s\n", ANSI_BOLD, ANSI_CYAN, ANSI_RESET);
    fprintf(out, "\n");
    fprintf(out, "  %sType:%s       %s\n",
            ANSI_BOLD, ANSI_RESET, workflow_monitor_type_name(monitor->type));
    fprintf(out, "  %sAgents:%s     %zu completed, %zu failed",
            ANSI_BOLD, ANSI_RESET, completed, failed);
    if (skipped > 0) {
        fprintf(out, ", %zu skipped", skipped);
    }
    fprintf(out, "\n");
    fprintf(out, "  %sTotal time:%s %.1fs\n", ANSI_BOLD, ANSI_RESET, total_time / 1000.0);
    fprintf(out, "\n");

    // Show individual agent times
    fprintf(out, "  %sAgent timing:%s\n", ANSI_DIM, ANSI_RESET);
    for (size_t i = 0; i < monitor->agent_count; i++) {
        MonitoredAgent* agent = &monitor->agents[i];
        fprintf(out, "    %s%s%s %-25s %s%.1fs%s\n",
                status_color(agent->status),
                workflow_monitor_status_icon(agent->status),
                ANSI_RESET,
                agent->name,
                ANSI_DIM,
                agent->duration_ms / 1000.0,
                ANSI_RESET);
    }
    fprintf(out, "\n");

    fflush(out);
}

// ============================================================================
// EXTENDED API - PHASE MANAGEMENT
// ============================================================================

int workflow_monitor_add_phase(WorkflowMonitor* monitor, const char* phase_name) {
    if (!monitor || !phase_name) {
        return -1;
    }

    // Allocate or expand phase_names array
    if (!monitor->phase_names) {
        monitor->phase_names = calloc(8, sizeof(char*));
        if (!monitor->phase_names) {
            return -1;
        }
    }

    int idx = (int)monitor->phase_count;
    monitor->phase_names[idx] = strdup(phase_name);
    monitor->phase_count++;

    // Also add as a NODE_PHASE node if using nodes
    if (monitor->use_nodes) {
        workflow_monitor_add_node(monitor, NODE_PHASE, phase_name, -1);
    }

    return idx;
}

void workflow_monitor_set_current_phase(WorkflowMonitor* monitor, int phase_idx) {
    if (!monitor || phase_idx < 0 || (size_t)phase_idx >= monitor->phase_count) {
        return;
    }
    monitor->current_phase = phase_idx;
}

// ============================================================================
// EXTENDED API - NODE MANAGEMENT
// ============================================================================

int workflow_monitor_add_node(WorkflowMonitor* monitor, NodeType type, const char* label,
                               int parent_idx) {
    if (!monitor || !label) {
        return -1;
    }

    // Enable node mode
    monitor->use_nodes = true;

    // Allocate if needed
    if (!monitor->nodes) {
        monitor->node_capacity = 16;
        monitor->nodes = calloc(monitor->node_capacity, sizeof(WorkflowNode));
        if (!monitor->nodes) {
            return -1;
        }
    }

    // Expand if needed
    if (monitor->node_count >= monitor->node_capacity) {
        size_t new_capacity = monitor->node_capacity * 2;
        WorkflowNode* new_nodes = realloc(monitor->nodes, new_capacity * sizeof(WorkflowNode));
        if (!new_nodes) {
            return -1;
        }
        monitor->nodes = new_nodes;
        monitor->node_capacity = new_capacity;
    }

    int idx = (int)monitor->node_count;
    WorkflowNode* node = &monitor->nodes[idx];

    node->type = type;
    node->label = strdup(label);
    node->status = AGENT_STATUS_PENDING;
    node->parent_idx = parent_idx;
    node->depth = (parent_idx >= 0) ? monitor->nodes[parent_idx].depth + 1 : 0;
    node->child_capacity = 8;
    node->children = calloc(node->child_capacity, sizeof(int));

    // Add to parent's children list
    if (parent_idx >= 0 && (size_t)parent_idx < monitor->node_count) {
        WorkflowNode* parent = &monitor->nodes[parent_idx];
        if (parent->child_count >= parent->child_capacity) {
            parent->child_capacity *= 2;
            parent->children = realloc(parent->children, parent->child_capacity * sizeof(int));
        }
        parent->children[parent->child_count++] = idx;
    }

    monitor->node_count++;
    return idx;
}

int workflow_monitor_add_agent_to_phase(WorkflowMonitor* monitor, int phase_idx,
                                         const char* name, const char* task) {
    if (!monitor || !name || phase_idx < 0) {
        return -1;
    }

    // Add agent normally
    int agent_idx = workflow_monitor_add_agent(monitor, name, task);
    if (agent_idx >= 0) {
        // Set parent to phase
        monitor->agents[agent_idx].parent_idx = phase_idx;
        monitor->agents[agent_idx].depth = 1;

        // Also add as node
        if (monitor->use_nodes) {
            workflow_monitor_add_node(monitor, NODE_AGENT, name, phase_idx);
        }
    }
    return agent_idx;
}

void workflow_monitor_set_condition(WorkflowMonitor* monitor, int node_idx, const char* condition) {
    if (!monitor || node_idx < 0 || (size_t)node_idx >= monitor->node_count || !condition) {
        return;
    }

    WorkflowNode* node = &monitor->nodes[node_idx];
    free(node->condition);
    node->condition = strdup(condition);
}

void workflow_monitor_set_node_status(WorkflowMonitor* monitor, int node_idx, AgentStatus status) {
    if (!monitor || node_idx < 0 || (size_t)node_idx >= monitor->node_count) {
        return;
    }

    WorkflowNode* node = &monitor->nodes[node_idx];
    AgentStatus old_status = node->status;
    node->status = status;

    // Record timing
    if (status == AGENT_STATUS_THINKING && old_status == AGENT_STATUS_PENDING) {
        clock_gettime(CLOCK_MONOTONIC, &node->start);
    } else if ((status == AGENT_STATUS_COMPLETED || status == AGENT_STATUS_FAILED) &&
               old_status == AGENT_STATUS_THINKING) {
        clock_gettime(CLOCK_MONOTONIC, &node->end);
        node->duration_ms = ((node->end.tv_sec - node->start.tv_sec) * 1000.0) +
                           ((node->end.tv_nsec - node->start.tv_nsec) / 1000000.0);
    }
}

// ============================================================================
// EXTENDED RENDERING - COMPLEX WORKFLOWS
// ============================================================================

static const char* node_type_icon(NodeType type) {
    switch (type) {
    case NODE_AGENT:
        return ICON_PENDING;
    case NODE_DECISION:
        return ICON_DECISION;
    case NODE_GROUP:
        return ICON_GROUP;
    case NODE_PHASE:
        return ICON_PHASE;
    default:
        return "?";
    }
}

static void render_node_recursive(WorkflowMonitor* monitor, int node_idx, FILE* out, int depth,
                                   bool is_last) {
    if (node_idx < 0 || (size_t)node_idx >= monitor->node_count) {
        return;
    }

    WorkflowNode* node = &monitor->nodes[node_idx];

    // Indent based on depth
    for (int i = 0; i < depth; i++) {
        fprintf(out, "   ");
    }

    // Tree branch
    if (depth > 0) {
        if (is_last) {
            fprintf(out, "%s%s%s ", ANSI_DIM, BOX_BL, BOX_H);
        } else {
            fprintf(out, "%s%s%s ", ANSI_DIM, BOX_LT, BOX_H);
        }
    }

    // Node icon based on type and status
    const char* icon;
    if (node->type == NODE_AGENT) {
        icon = workflow_monitor_status_icon(node->status);
    } else {
        icon = node_type_icon(node->type);
    }

    fprintf(out, "%s%s%s ", status_color(node->status), icon, ANSI_RESET);

    // Label
    fprintf(out, "%s%s%s", ANSI_BOLD, node->label, ANSI_RESET);

    // Status for agents
    if (node->type == NODE_AGENT) {
        fprintf(out, " %s[%s]%s", status_color(node->status),
                workflow_monitor_status_name(node->status), ANSI_RESET);
        if (node->duration_ms > 0) {
            fprintf(out, " %.1fs", node->duration_ms / 1000.0);
        }
    }

    // Condition for decision nodes
    if (node->type == NODE_DECISION && node->condition) {
        fprintf(out, " %s(%s)%s", ANSI_DIM, node->condition, ANSI_RESET);
    }

    fprintf(out, "\n");

    // Render children
    for (size_t i = 0; i < node->child_count; i++) {
        render_node_recursive(monitor, node->children[i], out, depth + 1,
                              i == node->child_count - 1);
    }
}

static void render_sequential_workflow(WorkflowMonitor* monitor, FILE* out) {
    fprintf(out, "  %s%s Sequential Workflow %s\n\n", ANSI_BOLD, ANSI_CYAN, ANSI_RESET);

    for (size_t i = 0; i < monitor->agent_count; i++) {
        MonitoredAgent* agent = &monitor->agents[i];

        // Step number
        fprintf(out, "  %s%zu.%s ", ANSI_DIM, i + 1, ANSI_RESET);

        // Status icon
        fprintf(out, "%s%s%s ", status_color(agent->status),
                workflow_monitor_status_icon(agent->status), ANSI_RESET);

        // Agent name
        fprintf(out, "%s%-20s%s", ANSI_BOLD, agent->name, ANSI_RESET);

        // Status
        fprintf(out, " %s[%s]%s", status_color(agent->status),
                workflow_monitor_status_name(agent->status), ANSI_RESET);

        if (agent->duration_ms > 0) {
            fprintf(out, " %.1fs", agent->duration_ms / 1000.0);
        }
        fprintf(out, "\n");

        // Task description
        if (agent->task && agent->task[0]) {
            fprintf(out, "       %s%s%s\n", ANSI_DIM, agent->task, ANSI_RESET);
        }

        // Arrow to next step (except last)
        if (i < monitor->agent_count - 1) {
            fprintf(out, "       %s%s%s\n", ANSI_DIM, ICON_ARROW, ANSI_RESET);
        }
    }
    fprintf(out, "\n");
}

static void render_pipeline_workflow(WorkflowMonitor* monitor, FILE* out) {
    fprintf(out, "  %s%s Pipeline Workflow %s\n\n", ANSI_BOLD, ANSI_CYAN, ANSI_RESET);

    // Horizontal layout for pipeline
    fprintf(out, "  ");
    for (size_t i = 0; i < monitor->agent_count; i++) {
        MonitoredAgent* agent = &monitor->agents[i];

        fprintf(out, "%s%s%s",
                status_color(agent->status),
                workflow_monitor_status_icon(agent->status),
                ANSI_RESET);

        // Truncate name for horizontal display
        char short_name[15];
        strncpy(short_name, agent->name, 14);
        short_name[14] = '\0';
        fprintf(out, "%s", short_name);

        if (i < monitor->agent_count - 1) {
            fprintf(out, " %s%s%s ", ANSI_CYAN, ICON_PIPELINE, ANSI_RESET);
        }
    }
    fprintf(out, "\n\n");

    // Detailed view below
    for (size_t i = 0; i < monitor->agent_count; i++) {
        MonitoredAgent* agent = &monitor->agents[i];
        fprintf(out, "  %s%s%s %s: %s%s%s\n",
                status_color(agent->status),
                workflow_monitor_status_icon(agent->status),
                ANSI_RESET,
                agent->name,
                ANSI_DIM,
                agent->task ? agent->task : "",
                ANSI_RESET);
    }
    fprintf(out, "\n");
}

void workflow_monitor_render_complex(WorkflowMonitor* monitor) {
    if (!monitor) {
        return;
    }

    FILE* out = stderr;

    render_header(monitor, out);
    render_progress_bar(monitor, out);

    // Render based on workflow type
    switch (monitor->type) {
    case WORKFLOW_SEQUENTIAL:
        render_sequential_workflow(monitor, out);
        break;

    case WORKFLOW_PIPELINE:
        render_pipeline_workflow(monitor, out);
        break;

    case WORKFLOW_CONDITIONAL:
        // Use node tree for conditional workflows
        if (monitor->use_nodes) {
            // Find root nodes (parent_idx == -1)
            for (size_t i = 0; i < monitor->node_count; i++) {
                if (monitor->nodes[i].parent_idx == -1) {
                    render_node_recursive(monitor, (int)i, out, 0, true);
                }
            }
        } else {
            render_agent_tree(monitor, out);
        }
        break;

    case WORKFLOW_PARALLEL:
    default:
        render_agent_tree(monitor, out);
        break;
    }

    // Extended legend for complex workflows
    fprintf(out, "  %sLegend:%s %s%s pending%s  %s%s thinking%s  %s%s completed%s  "
                 "%s%s failed%s  %s%s skipped%s  %s%s waiting%s\n\n",
            ANSI_DIM, ANSI_RESET,
            ANSI_DIM, ICON_PENDING, ANSI_RESET,
            ANSI_YELLOW, ICON_THINKING, ANSI_RESET,
            ANSI_GREEN, ICON_COMPLETED, ANSI_RESET,
            ANSI_RED, ICON_FAILED, ANSI_RESET,
            ANSI_BLUE, ICON_SKIPPED, ANSI_RESET,
            ANSI_MAGENTA, ICON_WAITING, ANSI_RESET);

    fflush(out);
}

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

WorkflowMonitor* workflow_monitor_create_sequential(const char* workflow_name,
                                                     const char** agent_names,
                                                     const char** tasks,
                                                     size_t count,
                                                     bool use_ansi) {
    WorkflowMonitor* monitor = workflow_monitor_create_typed(workflow_name, WORKFLOW_SEQUENTIAL,
                                                              use_ansi);
    if (!monitor) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        workflow_monitor_add_agent(monitor, agent_names[i], tasks ? tasks[i] : NULL);
    }

    return monitor;
}

WorkflowMonitor* workflow_monitor_create_pipeline(const char* workflow_name,
                                                   const char** agent_names,
                                                   const char** tasks,
                                                   size_t count,
                                                   bool use_ansi) {
    WorkflowMonitor* monitor = workflow_monitor_create_typed(workflow_name, WORKFLOW_PIPELINE,
                                                              use_ansi);
    if (!monitor) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        workflow_monitor_add_agent(monitor, agent_names[i], tasks ? tasks[i] : NULL);
    }

    return monitor;
}
