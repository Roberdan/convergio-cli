/**
 * CONVERGIO COST CONTROLLER
 *
 * Tracks API usage, calculates costs, enforces budgets
 * All prices in USD, tokens tracked per-message
 */

#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Thread-safe cost tracking
static pthread_mutex_t g_cost_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declaration
extern Orchestrator* orchestrator_get(void);

// ============================================================================
// COST CALCULATION
// ============================================================================

static double calculate_cost(uint64_t input_tokens, uint64_t output_tokens) {
    double input_cost = (input_tokens / 1000000.0) * CLAUDE_SONNET_INPUT_COST;
    double output_cost = (output_tokens / 1000000.0) * CLAUDE_SONNET_OUTPUT_COST;
    return input_cost + output_cost;
}

// ============================================================================
// COST RECORDING
// ============================================================================

void cost_record_usage(uint64_t input_tokens, uint64_t output_tokens) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !orch->initialized) return;

    pthread_mutex_lock(&g_cost_mutex);

    // Update session usage
    orch->cost.session_usage.input_tokens += input_tokens;
    orch->cost.session_usage.output_tokens += output_tokens;
    orch->cost.session_usage.total_tokens += (input_tokens + output_tokens);
    orch->cost.session_usage.api_calls++;

    // Calculate cost
    double call_cost = calculate_cost(input_tokens, output_tokens);
    orch->cost.session_usage.cost_usd += call_cost;
    orch->cost.current_spend_usd += call_cost;

    // Update total usage
    orch->cost.total_usage.input_tokens += input_tokens;
    orch->cost.total_usage.output_tokens += output_tokens;
    orch->cost.total_usage.total_tokens += (input_tokens + output_tokens);
    orch->cost.total_usage.api_calls++;
    orch->cost.total_usage.cost_usd += call_cost;
    orch->cost.total_spend_usd += call_cost;

    // Check budget
    if (orch->cost.budget_limit_usd > 0 &&
        orch->cost.current_spend_usd >= orch->cost.budget_limit_usd) {
        orch->cost.budget_exceeded = true;
    }

    // Callback if registered
    if (orch->on_cost_update) {
        orch->on_cost_update(&orch->cost, orch->callback_ctx);
    }

    pthread_mutex_unlock(&g_cost_mutex);
}

// Record usage for a specific agent
void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens, uint64_t output_tokens) {
    if (!agent) return;

    pthread_mutex_lock(&g_cost_mutex);

    agent->usage.input_tokens += input_tokens;
    agent->usage.output_tokens += output_tokens;
    agent->usage.total_tokens += (input_tokens + output_tokens);
    agent->usage.api_calls++;
    agent->usage.cost_usd += calculate_cost(input_tokens, output_tokens);

    pthread_mutex_unlock(&g_cost_mutex);

    // Also record globally
    cost_record_usage(input_tokens, output_tokens);
}

// ============================================================================
// COST QUERIES
// ============================================================================

double cost_get_session_spend(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return 0.0;

    pthread_mutex_lock(&g_cost_mutex);
    double spend = orch->cost.current_spend_usd;
    pthread_mutex_unlock(&g_cost_mutex);

    return spend;
}

double cost_get_total_spend(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return 0.0;

    pthread_mutex_lock(&g_cost_mutex);
    double spend = orch->cost.total_spend_usd;
    pthread_mutex_unlock(&g_cost_mutex);

    return spend;
}

bool cost_check_budget(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return false;

    pthread_mutex_lock(&g_cost_mutex);
    bool exceeded = orch->cost.budget_exceeded;
    pthread_mutex_unlock(&g_cost_mutex);

    return !exceeded;  // Returns true if within budget
}

double cost_get_remaining_budget(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || orch->cost.budget_limit_usd <= 0) return -1.0;  // No budget set

    pthread_mutex_lock(&g_cost_mutex);
    double remaining = orch->cost.budget_limit_usd - orch->cost.current_spend_usd;
    pthread_mutex_unlock(&g_cost_mutex);

    return remaining > 0 ? remaining : 0.0;
}

// ============================================================================
// BUDGET MANAGEMENT
// ============================================================================

void cost_set_budget(double limit_usd) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return;

    pthread_mutex_lock(&g_cost_mutex);
    orch->cost.budget_limit_usd = limit_usd;
    orch->cost.budget_exceeded = (orch->cost.current_spend_usd >= limit_usd);
    pthread_mutex_unlock(&g_cost_mutex);
}

void cost_reset_session(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return;

    pthread_mutex_lock(&g_cost_mutex);

    orch->cost.current_spend_usd = 0.0;
    memset(&orch->cost.session_usage, 0, sizeof(TokenUsage));
    orch->cost.budget_exceeded = false;
    orch->cost.session_start = time(NULL);

    pthread_mutex_unlock(&g_cost_mutex);
}

// ============================================================================
// COST REPORTING
// ============================================================================

char* cost_get_report(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return strdup("Error: Orchestrator not initialized");

    pthread_mutex_lock(&g_cost_mutex);

    char* report = malloc(2048);
    if (!report) {
        pthread_mutex_unlock(&g_cost_mutex);
        return NULL;
    }

    // Calculate session duration
    time_t now = time(NULL);
    int session_minutes = (int)((now - orch->cost.session_start) / 60);

    // Format budget status
    char budget_status[128];
    if (orch->cost.budget_limit_usd > 0) {
        double pct = (orch->cost.current_spend_usd / orch->cost.budget_limit_usd) * 100;
        snprintf(budget_status, sizeof(budget_status),
            "Budget: $%.2f / $%.2f (%.1f%%)%s",
            orch->cost.current_spend_usd,
            orch->cost.budget_limit_usd,
            pct,
            orch->cost.budget_exceeded ? " [EXCEEDED]" : "");
    } else {
        snprintf(budget_status, sizeof(budget_status), "Budget: No limit set");
    }

    snprintf(report, 2048,
        "╔══════════════════════════════════════════════════════════════╗\n"
        "║                    COST REPORT                               ║\n"
        "╠══════════════════════════════════════════════════════════════╣\n"
        "║ SESSION (%d min)                                              \n"
        "║   Input tokens:  %'12llu ($%.4f)                             \n"
        "║   Output tokens: %'12llu ($%.4f)                             \n"
        "║   API calls:     %'12u                                       \n"
        "║   Total cost:    $%.4f                                       \n"
        "╠══════════════════════════════════════════════════════════════╣\n"
        "║ ALL-TIME                                                     \n"
        "║   Input tokens:  %'12llu ($%.4f)                             \n"
        "║   Output tokens: %'12llu ($%.4f)                             \n"
        "║   API calls:     %'12u                                       \n"
        "║   Total cost:    $%.4f                                       \n"
        "╠══════════════════════════════════════════════════════════════╣\n"
        "║ %s\n"
        "╚══════════════════════════════════════════════════════════════╝\n",
        session_minutes,
        (unsigned long long)orch->cost.session_usage.input_tokens,
        (orch->cost.session_usage.input_tokens / 1000000.0) * CLAUDE_SONNET_INPUT_COST,
        (unsigned long long)orch->cost.session_usage.output_tokens,
        (orch->cost.session_usage.output_tokens / 1000000.0) * CLAUDE_SONNET_OUTPUT_COST,
        orch->cost.session_usage.api_calls,
        orch->cost.session_usage.cost_usd,
        (unsigned long long)orch->cost.total_usage.input_tokens,
        (orch->cost.total_usage.input_tokens / 1000000.0) * CLAUDE_SONNET_INPUT_COST,
        (unsigned long long)orch->cost.total_usage.output_tokens,
        (orch->cost.total_usage.output_tokens / 1000000.0) * CLAUDE_SONNET_OUTPUT_COST,
        orch->cost.total_usage.api_calls,
        orch->cost.total_usage.cost_usd,
        budget_status
    );

    pthread_mutex_unlock(&g_cost_mutex);

    return report;
}

// Compact single-line cost display for prompts
char* cost_get_status_line(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return strdup("");

    pthread_mutex_lock(&g_cost_mutex);

    char* line = malloc(128);
    if (!line) {
        pthread_mutex_unlock(&g_cost_mutex);
        return NULL;
    }

    if (orch->cost.budget_limit_usd > 0) {
        double remaining = orch->cost.budget_limit_usd - orch->cost.current_spend_usd;
        snprintf(line, 128, "[$%.4f spent | $%.4f remaining]",
            orch->cost.current_spend_usd, remaining > 0 ? remaining : 0);
    } else {
        snprintf(line, 128, "[$%.4f spent]", orch->cost.current_spend_usd);
    }

    pthread_mutex_unlock(&g_cost_mutex);

    return line;
}

// ============================================================================
// AGENT-SPECIFIC COST REPORTING
// ============================================================================

char* cost_get_agent_report(ManagedAgent* agent) {
    if (!agent) return strdup("Error: Invalid agent");

    pthread_mutex_lock(&g_cost_mutex);

    char* report = malloc(512);
    if (!report) {
        pthread_mutex_unlock(&g_cost_mutex);
        return NULL;
    }

    snprintf(report, 512,
        "Agent: %s\n"
        "  Input tokens:  %llu ($%.4f)\n"
        "  Output tokens: %llu ($%.4f)\n"
        "  API calls:     %u\n"
        "  Total cost:    $%.4f\n",
        agent->name,
        (unsigned long long)agent->usage.input_tokens,
        (agent->usage.input_tokens / 1000000.0) * CLAUDE_SONNET_INPUT_COST,
        (unsigned long long)agent->usage.output_tokens,
        (agent->usage.output_tokens / 1000000.0) * CLAUDE_SONNET_OUTPUT_COST,
        agent->usage.api_calls,
        agent->usage.cost_usd
    );

    pthread_mutex_unlock(&g_cost_mutex);

    return report;
}

// Get top N agents by cost
void cost_get_top_agents(ManagedAgent** out_agents, size_t* out_count, size_t max_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_agents || !out_count) return;

    pthread_mutex_lock(&g_cost_mutex);

    // Create array of agents sorted by cost (simple bubble sort for small N)
    size_t count = orch->agent_count < max_count ? orch->agent_count : max_count;

    // Copy pointers
    for (size_t i = 0; i < count && i < orch->agent_count; i++) {
        out_agents[i] = orch->agents[i];
    }

    // Sort by cost descending
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - i - 1; j++) {
            if (out_agents[j]->usage.cost_usd < out_agents[j+1]->usage.cost_usd) {
                ManagedAgent* tmp = out_agents[j];
                out_agents[j] = out_agents[j+1];
                out_agents[j+1] = tmp;
            }
        }
    }

    *out_count = count;

    pthread_mutex_unlock(&g_cost_mutex);
}

// ============================================================================
// COST ESTIMATION
// ============================================================================

// Estimate cost for a given text (rough estimation)
double cost_estimate_message(const char* text, bool is_input) {
    if (!text) return 0.0;

    // Rough token estimation: ~4 characters per token for English
    // ~3 characters per token for code/technical content
    size_t len = strlen(text);
    uint64_t estimated_tokens = (len / 3) + 1;  // Conservative estimate

    if (is_input) {
        return (estimated_tokens / 1000000.0) * CLAUDE_SONNET_INPUT_COST;
    } else {
        return (estimated_tokens / 1000000.0) * CLAUDE_SONNET_OUTPUT_COST;
    }
}

// Check if we can afford a conversation of estimated length
bool cost_can_afford(size_t estimated_turns, size_t avg_input_tokens, size_t avg_output_tokens) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || orch->cost.budget_limit_usd <= 0) return true;  // No budget = unlimited

    double estimated_cost = 0.0;
    for (size_t i = 0; i < estimated_turns; i++) {
        estimated_cost += calculate_cost(avg_input_tokens, avg_output_tokens);
    }

    pthread_mutex_lock(&g_cost_mutex);
    double remaining = orch->cost.budget_limit_usd - orch->cost.current_spend_usd;
    pthread_mutex_unlock(&g_cost_mutex);

    return estimated_cost <= remaining;
}
