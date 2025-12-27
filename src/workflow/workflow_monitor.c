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
    monitor->use_ansi = use_ansi;
    monitor->is_active = false;

    return monitor;
}

void workflow_monitor_free(WorkflowMonitor* monitor) {
    if (!monitor) {
        return;
    }

    for (size_t i = 0; i < monitor->agent_count; i++) {
        free(monitor->agents[i].name);
        free(monitor->agents[i].task);
    }
    free(monitor->agents);
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
    double total_time = 0;

    for (size_t i = 0; i < monitor->agent_count; i++) {
        if (monitor->agents[i].status == AGENT_STATUS_COMPLETED) {
            completed++;
            total_time += monitor->agents[i].duration_ms;
        } else if (monitor->agents[i].status == AGENT_STATUS_FAILED) {
            failed++;
        }
    }

    fprintf(out, "\n");
    fprintf(out, "%s══════════════════════════════════════════════════%s\n",
            ANSI_DIM, ANSI_RESET);
    fprintf(out, "%s%s WORKFLOW COMPLETE %s\n", ANSI_BOLD, ANSI_CYAN, ANSI_RESET);
    fprintf(out, "\n");
    fprintf(out, "  %sAgents:%s     %zu completed, %zu failed\n",
            ANSI_BOLD, ANSI_RESET, completed, failed);
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
