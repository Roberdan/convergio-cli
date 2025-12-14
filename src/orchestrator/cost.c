/**
 * CONVERGIO COST CONTROLLER
 *
 * Tracks API usage, calculates costs, enforces budgets
 * All prices in USD, tokens tracked per-message
 */

#include "nous/orchestrator.h"
#include "nous/provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "nous/debug_mutex.h"

// Thread-safe cost tracking
CONVERGIO_MUTEX_DECLARE(g_cost_mutex);

// Forward declarations
extern Orchestrator* orchestrator_get(void);
extern const char* router_get_agent_model(const char* agent_name);

// Persistence functions
extern double persistence_get_total_cost(void);
extern int persistence_save_cost_daily(const char* date, uint64_t input_tokens,
                                        uint64_t output_tokens, double cost, uint32_t calls);

// Claude Max subscription check
extern bool nous_claude_is_max_subscription(void);

// Get current date string
static void get_today_date(char* buf, size_t size) {
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    strftime(buf, size, "%Y-%m-%d", tm);
}

// ============================================================================
// COST INITIALIZATION (Load historical data)
// ============================================================================

void cost_load_historical(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return;

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    // Load total historical cost from database
    double historical_cost = persistence_get_total_cost();
    orch->cost.total_spend_usd = historical_cost;
    orch->cost.total_usage.estimated_cost = historical_cost;

    // Check if budget already exceeded from historical usage
    if (orch->cost.budget_limit_usd > 0 &&
        orch->cost.total_spend_usd >= orch->cost.budget_limit_usd) {
        orch->cost.budget_exceeded = true;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
}

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

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    // Update session usage (always track tokens for statistics)
    orch->cost.session_usage.input_tokens += input_tokens;
    orch->cost.session_usage.output_tokens += output_tokens;
    // Note: cached_tokens tracked separately when cache hits occur

    // Calculate cost - $0 if Claude Max subscription
    double call_cost = 0.0;
    if (!nous_claude_is_max_subscription()) {
        call_cost = calculate_cost(input_tokens, output_tokens);
    }

    orch->cost.session_usage.estimated_cost += call_cost;
    orch->cost.current_spend_usd += call_cost;

    // Update total usage
    orch->cost.total_usage.input_tokens += input_tokens;
    orch->cost.total_usage.output_tokens += output_tokens;
    orch->cost.total_usage.estimated_cost += call_cost;
    orch->cost.total_spend_usd += call_cost;

    // Check budget against cumulative total (only if not Claude Max)
    if (!nous_claude_is_max_subscription() &&
        orch->cost.budget_limit_usd > 0 &&
        orch->cost.total_spend_usd >= orch->cost.budget_limit_usd) {
        orch->cost.budget_exceeded = true;
    }

    // Persist to database for cumulative tracking
    char today[16];
    get_today_date(today, sizeof(today));
    persistence_save_cost_daily(today, input_tokens, output_tokens, call_cost, 1);

    // Callback if registered
    if (orch->on_cost_update) {
        orch->on_cost_update(&orch->cost, orch->callback_ctx);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
}

// Record usage with model-specific pricing
void cost_record_usage_for_model(const char* model_id, uint64_t input_tokens, uint64_t output_tokens) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !orch->initialized) return;

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    // Update session usage (always track tokens for statistics)
    orch->cost.session_usage.input_tokens += input_tokens;
    orch->cost.session_usage.output_tokens += output_tokens;

    // Calculate cost using actual model pricing - $0 if Claude Max subscription
    double call_cost = 0.0;
    if (!nous_claude_is_max_subscription()) {
        call_cost = model_estimate_cost(model_id, input_tokens, output_tokens);
    }

    orch->cost.session_usage.estimated_cost += call_cost;
    orch->cost.current_spend_usd += call_cost;

    // Update total usage
    orch->cost.total_usage.input_tokens += input_tokens;
    orch->cost.total_usage.output_tokens += output_tokens;
    orch->cost.total_usage.estimated_cost += call_cost;
    orch->cost.total_spend_usd += call_cost;

    // Check budget against cumulative total (only if not Claude Max)
    if (!nous_claude_is_max_subscription() &&
        orch->cost.budget_limit_usd > 0 &&
        orch->cost.total_spend_usd >= orch->cost.budget_limit_usd) {
        orch->cost.budget_exceeded = true;
    }

    // Persist to database for cumulative tracking
    char today[16];
    get_today_date(today, sizeof(today));
    persistence_save_cost_daily(today, input_tokens, output_tokens, call_cost, 1);

    // Callback if registered
    if (orch->on_cost_update) {
        orch->on_cost_update(&orch->cost, orch->callback_ctx);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
}

// Record usage for a specific agent (uses agent's configured model for pricing)
void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens, uint64_t output_tokens) {
    if (!agent) return;

    // Get the model configured for this agent
    const char* model_id = router_get_agent_model(agent->name);

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    agent->usage.input_tokens += input_tokens;
    agent->usage.output_tokens += output_tokens;
    // Use model-specific pricing
    agent->usage.estimated_cost += model_estimate_cost(model_id, input_tokens, output_tokens);

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    // Also record globally with model-specific pricing
    cost_record_usage_for_model(model_id, input_tokens, output_tokens);
}

// ============================================================================
// COST QUERIES
// ============================================================================

double cost_get_session_spend(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return 0.0;

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);
    double spend = orch->cost.current_spend_usd;
    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    return spend;
}

double cost_get_total_spend(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return 0.0;

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);
    double spend = orch->cost.total_spend_usd;
    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    return spend;
}

bool cost_check_budget(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return false;

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);
    bool exceeded = orch->cost.budget_exceeded;
    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    return !exceeded;  // Returns true if within budget
}

double cost_get_remaining_budget(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || orch->cost.budget_limit_usd <= 0) return -1.0;  // No budget set

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);
    // Use total spend, not just session spend
    double remaining = orch->cost.budget_limit_usd - orch->cost.total_spend_usd;
    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    return remaining > 0 ? remaining : 0.0;
}

// ============================================================================
// BUDGET MANAGEMENT
// ============================================================================

void cost_set_budget(double limit_usd) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return;

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);
    orch->cost.budget_limit_usd = limit_usd;
    // Check budget against total spend, not just session
    orch->cost.budget_exceeded = (orch->cost.total_spend_usd >= limit_usd);
    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
}

void cost_reset_session(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return;

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    orch->cost.current_spend_usd = 0.0;
    memset(&orch->cost.session_usage, 0, sizeof(TokenUsage));
    orch->cost.session_start = time(NULL);

    // Preserve budget_exceeded if total spend is still over budget
    if (orch->cost.budget_limit_usd > 0) {
        orch->cost.budget_exceeded = (orch->cost.total_spend_usd >= orch->cost.budget_limit_usd);
    } else {
        orch->cost.budget_exceeded = false;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
}

// ============================================================================
// COST REPORTING
// ============================================================================

char* cost_get_report(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return strdup("Error: Orchestrator not initialized");

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    char* report = malloc(2048);
    if (!report) {
        CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
        return NULL;
    }

    // Calculate session duration
    time_t now = time(NULL);
    int session_minutes = (int)((now - orch->cost.session_start) / 60);

    // Check if we're in LOCAL MODE (tokens used but zero cost)
    bool is_local_mode = (orch->cost.session_usage.input_tokens > 0 ||
                          orch->cost.session_usage.output_tokens > 0) &&
                         orch->cost.session_usage.estimated_cost < 0.0001;

    // Build report
    int offset = 0;

    if (is_local_mode) {
        // ========================================================================
        // LOCAL MODE REPORT (MLX / Ollama - Free inference)
        // ========================================================================
        offset += snprintf(report + offset, 2048 - offset,
            "\n\033[1m╔════════════════════════════════════════════════════╗\033[0m\n"
            "\033[1m║  \033[32m🏠 LOCAL MODE - FREE INFERENCE\033[0m                    \033[1m║\033[0m\n"
            "\033[1m╠════════════════════════════════════════════════════╣\033[0m\n");

        // Session section
        offset += snprintf(report + offset, 2048 - offset,
            "\033[36m║ SESSION\033[0m (%d min)\n"
            "║   Input tokens:  %'12llu\n"
            "║   Output tokens: %'12llu\n"
            "║   \033[32m✓ Cost:          $0.00 (local inference)\033[0m\n",
            session_minutes,
            (unsigned long long)orch->cost.session_usage.input_tokens,
            (unsigned long long)orch->cost.session_usage.output_tokens);

        offset += snprintf(report + offset, 2048 - offset,
            "\033[1m╠════════════════════════════════════════════════════╣\033[0m\n"
            "║ \033[90mRunning on Apple Silicon with MLX - no API costs!\033[0m\n"
            "\033[1m╚════════════════════════════════════════════════════╝\033[0m\n");
    } else {
        // ========================================================================
        // STANDARD API MODE REPORT (with costs)
        // ========================================================================

        // Format budget status - use TOTAL spend, not just session
        char budget_line[64];
        if (orch->cost.budget_limit_usd > 0) {
            double pct = (orch->cost.total_spend_usd / orch->cost.budget_limit_usd) * 100;
            if (orch->cost.budget_exceeded) {
                snprintf(budget_line, sizeof(budget_line),
                    "$%.2f / $%.2f (%.0f%%) EXCEEDED",
                    orch->cost.total_spend_usd,
                    orch->cost.budget_limit_usd,
                    pct);
            } else {
                snprintf(budget_line, sizeof(budget_line),
                    "$%.2f / $%.2f (%.0f%%)",
                    orch->cost.total_spend_usd,
                    orch->cost.budget_limit_usd,
                    pct);
            }
        } else {
            snprintf(budget_line, sizeof(budget_line), "No limit set");
        }

        offset += snprintf(report + offset, 2048 - offset,
            "\n\033[1m╔════════════════════════════════════════════════════╗\033[0m\n"
            "\033[1m║               COST REPORT                          ║\033[0m\n"
            "\033[1m╠════════════════════════════════════════════════════╣\033[0m\n");

        // Session section
        offset += snprintf(report + offset, 2048 - offset,
            "\033[36m║ SESSION\033[0m (%d min)\n"
            "║   Input tokens:  %'12llu  ($%.4f)\n"
            "║   Output tokens: %'12llu  ($%.4f)\n"
            "║   \033[1mTotal cost:      $%.4f\033[0m\n",
            session_minutes,
            (unsigned long long)orch->cost.session_usage.input_tokens,
            (orch->cost.session_usage.input_tokens / 1000000.0) * CLAUDE_SONNET_INPUT_COST,
            (unsigned long long)orch->cost.session_usage.output_tokens,
            (orch->cost.session_usage.output_tokens / 1000000.0) * CLAUDE_SONNET_OUTPUT_COST,
            orch->cost.session_usage.estimated_cost);

        offset += snprintf(report + offset, 2048 - offset,
            "\033[1m╠════════════════════════════════════════════════════╣\033[0m\n");

        // All-time section
        offset += snprintf(report + offset, 2048 - offset,
            "\033[36m║ ALL-TIME\033[0m\n"
            "║   Input tokens:  %'12llu  ($%.4f)\n"
            "║   Output tokens: %'12llu  ($%.4f)\n"
            "║   \033[1mTotal cost:      $%.4f\033[0m\n",
            (unsigned long long)orch->cost.total_usage.input_tokens,
            (orch->cost.total_usage.input_tokens / 1000000.0) * CLAUDE_SONNET_INPUT_COST,
            (unsigned long long)orch->cost.total_usage.output_tokens,
            (orch->cost.total_usage.output_tokens / 1000000.0) * CLAUDE_SONNET_OUTPUT_COST,
            orch->cost.total_usage.estimated_cost);

        offset += snprintf(report + offset, 2048 - offset,
            "\033[1m╠════════════════════════════════════════════════════╣\033[0m\n");

        // Budget section
        if (orch->cost.budget_exceeded) {
            offset += snprintf(report + offset, 2048 - offset,
                "\033[31m║ BUDGET: %s\033[0m\n", budget_line);
        } else if (orch->cost.budget_limit_usd > 0) {
            offset += snprintf(report + offset, 2048 - offset,
                "\033[32m║ BUDGET: %s\033[0m\n", budget_line);
        } else {
            offset += snprintf(report + offset, 2048 - offset,
                "║ BUDGET: %s\n", budget_line);
        }

        offset += snprintf(report + offset, 2048 - offset,
            "\033[1m╚════════════════════════════════════════════════════╝\033[0m\n");
    }

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    return report;
}

// Compact single-line cost display for prompts
char* cost_get_status_line(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return strdup("");

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    char* line = malloc(128);
    if (!line) {
        CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
        return NULL;
    }

    // Check if we're in LOCAL MODE (tokens used but zero cost)
    bool is_local_mode = (orch->cost.session_usage.input_tokens > 0 ||
                          orch->cost.session_usage.output_tokens > 0) &&
                         orch->cost.current_spend_usd < 0.0001;

    if (is_local_mode) {
        snprintf(line, 128, "[🏠 Local Mode - Free]");
    } else if (orch->cost.budget_limit_usd > 0) {
        double remaining = orch->cost.budget_limit_usd - orch->cost.current_spend_usd;
        snprintf(line, 128, "[$%.4f spent | $%.4f remaining]",
            orch->cost.current_spend_usd, remaining > 0 ? remaining : 0);
    } else {
        snprintf(line, 128, "[$%.4f spent]", orch->cost.current_spend_usd);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    return line;
}

// ============================================================================
// AGENT-SPECIFIC COST REPORTING
// ============================================================================

char* cost_get_agent_report(ManagedAgent* agent) {
    if (!agent) return strdup("Error: Invalid agent");

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    char* report = malloc(512);
    if (!report) {
        CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
        return NULL;
    }

    snprintf(report, 512,
        "Agent: %s\n"
        "  Input tokens:  %llu ($%.4f)\n"
        "  Output tokens: %llu ($%.4f)\n"
        "  Total cost:    $%.4f\n",
        agent->name,
        (unsigned long long)agent->usage.input_tokens,
        (agent->usage.input_tokens / 1000000.0) * CLAUDE_SONNET_INPUT_COST,
        (unsigned long long)agent->usage.output_tokens,
        (agent->usage.output_tokens / 1000000.0) * CLAUDE_SONNET_OUTPUT_COST,
        agent->usage.estimated_cost
    );

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    return report;
}

// Get top N agents by cost
void cost_get_top_agents(ManagedAgent** out_agents, size_t* out_count, size_t max_count) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !out_agents || !out_count) return;

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);

    // Create array of agents sorted by cost (simple bubble sort for small N)
    size_t count = orch->agent_count < max_count ? orch->agent_count : max_count;

    // Copy pointers
    for (size_t i = 0; i < count && i < orch->agent_count; i++) {
        out_agents[i] = orch->agents[i];
    }

    // Sort by cost descending
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - i - 1; j++) {
            if (out_agents[j]->usage.estimated_cost < out_agents[j+1]->usage.estimated_cost) {
                ManagedAgent* tmp = out_agents[j];
                out_agents[j] = out_agents[j+1];
                out_agents[j+1] = tmp;
            }
        }
    }

    *out_count = count;

    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);
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

    CONVERGIO_MUTEX_LOCK(&g_cost_mutex);
    double remaining = orch->cost.budget_limit_usd - orch->cost.current_spend_usd;
    CONVERGIO_MUTEX_UNLOCK(&g_cost_mutex);

    return estimated_cost <= remaining;
}
