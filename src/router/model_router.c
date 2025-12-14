/**
 * CONVERGIO MODEL ROUTER
 *
 * Intelligent model selection based on:
 * - Agent configuration (primary/fallback models)
 * - Provider availability
 * - Budget constraints
 * - Task complexity hints
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "nous/nous.h"
#include "nous/debug_mutex.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define MAX_AGENT_CONFIGS 64
#define BUDGET_WARNING_THRESHOLD 0.8  // Warn at 80% budget usage

// ============================================================================
// AGENT MODEL CONFIGURATION
// ============================================================================

typedef struct {
    char* agent_name;               // e.g., "ali", "marco", "thor"
    char* primary_model;            // e.g., "anthropic/claude-opus-4"
    char* fallback_model;           // e.g., "openai/gpt-4o"
    CostTier cost_tier;             // Cost tier preference
    bool auto_downgrade;            // Auto-downgrade on budget limits
    char* reason;                   // Reason for model selection
} AgentModelConfig;

// ============================================================================
// ROUTER STATE
// ============================================================================

typedef struct {
    // Agent configurations
    AgentModelConfig configs[MAX_AGENT_CONFIGS];
    size_t config_count;

    // Budget tracking
    double daily_budget;
    double session_budget;
    double daily_spent;
    double session_spent;
    time_t budget_reset_time;

    // Statistics
    size_t total_requests;
    size_t fallback_requests;
    size_t downgrade_requests;

    ConvergioMutex mutex;
    bool initialized;
} RouterState;

static RouterState g_router = {
    .configs = {{0}},
    .config_count = 0,
    .daily_budget = 0.0,
    .session_budget = 0.0,
    .daily_spent = 0.0,
    .session_spent = 0.0,
    .budget_reset_time = 0,
    .total_requests = 0,
    .fallback_requests = 0,
    .downgrade_requests = 0,
#ifdef DEBUG
    .mutex = {
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .once = PTHREAD_ONCE_INIT,
        .initialized = 0
    },
#else
    .mutex = PTHREAD_MUTEX_INITIALIZER,
#endif
    .initialized = false
};

// ============================================================================
// LOCAL MLX MODE
// ============================================================================

static bool g_local_mlx_mode = false;
static char g_local_mlx_model[128] = "mlx/deepseek-r1-1.5b";  // Default local model

void router_set_local_mode(bool enabled, const char* model_id) {
    g_local_mlx_mode = enabled;
    if (model_id && model_id[0]) {
        // If model doesn't have mlx/ prefix, add it
        if (strncmp(model_id, "mlx/", 4) != 0) {
            snprintf(g_local_mlx_model, sizeof(g_local_mlx_model), "mlx/%s", model_id);
        } else {
            strncpy(g_local_mlx_model, model_id, sizeof(g_local_mlx_model) - 1);
            g_local_mlx_model[sizeof(g_local_mlx_model) - 1] = '\0';
        }
    }
    if (enabled) {
        LOG_INFO(LOG_CAT_SYSTEM, "Local MLX mode enabled with model: %s", g_local_mlx_model);
    }
}

bool router_is_local_mode(void) {
    return g_local_mlx_mode;
}

const char* router_get_local_model(void) {
    return g_local_mlx_model;
}

// ============================================================================
// DEFAULT AGENT MODEL CONFIGURATIONS (from models.json)
// ============================================================================

static void load_default_configs(void) {
    // These defaults match the agent_defaults in models.json
    static const struct {
        const char* name;
        const char* primary;
        const char* fallback;
        CostTier tier;
        const char* reason;
    } defaults[] = {
        {"ali", "anthropic/claude-opus-4", "openai/gpt-4o", COST_TIER_PREMIUM,
         "Chief of Staff needs best reasoning for delegation"},
        {"baccio", "anthropic/claude-opus-4", "openai/gpt-4o", COST_TIER_PREMIUM,
         "Architecture requires deep reasoning and planning"},
        {"marco", "anthropic/claude-sonnet-4", "openai/gpt-4o", COST_TIER_MID,
         "Sonnet 4 for coding, GPT-4o as fallback"},
        {"luca", "openai/o1", "anthropic/claude-opus-4", COST_TIER_PREMIUM,
         "o1 excels at deep reasoning for security analysis"},
        {"thor", "openai/gpt-4o-mini", "gemini/gemini-1.5-flash", COST_TIER_CHEAP,
         "Fast, cheap for quick reviews"},
        {"router", "openai/gpt-4o-mini", "gemini/gemini-1.5-flash", COST_TIER_CHEAP,
         "Fastest for routing decisions"},
        {NULL, NULL, NULL, COST_TIER_MID, NULL}
    };

    for (int i = 0; defaults[i].name != NULL; i++) {
        AgentModelConfig* cfg = &g_router.configs[g_router.config_count];
        cfg->agent_name = strdup(defaults[i].name);
        cfg->primary_model = strdup(defaults[i].primary);
        cfg->fallback_model = strdup(defaults[i].fallback);
        cfg->reason = strdup(defaults[i].reason);

        // OOM check - if any allocation failed, clean up and skip this entry
        if (!cfg->agent_name || !cfg->primary_model || !cfg->fallback_model || !cfg->reason) {
            free(cfg->agent_name);
            free(cfg->primary_model);
            free(cfg->fallback_model);
            free(cfg->reason);
            cfg->agent_name = NULL;
            cfg->primary_model = NULL;
            cfg->fallback_model = NULL;
            cfg->reason = NULL;
            LOG_ERROR(LOG_CAT_SYSTEM, "OOM loading agent config for %s", defaults[i].name);
            continue;
        }

        cfg->cost_tier = defaults[i].tier;
        cfg->auto_downgrade = true;
        g_router.config_count++;
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int router_init(void) {
    CONVERGIO_MUTEX_LOCK(&g_router.mutex);

    if (g_router.initialized) {
        CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
        return 0;
    }

    // Initialize provider registry first
    ProviderError err = provider_registry_init();
    if (err != PROVIDER_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
        return -1;
    }

    // Load default configurations
    load_default_configs();

    // Set default budgets (can be overridden via config)
    g_router.daily_budget = 50.0;     // $50/day default
    g_router.session_budget = 10.0;   // $10/session default
    g_router.daily_spent = 0.0;
    g_router.session_spent = 0.0;
    g_router.budget_reset_time = time(NULL);

    g_router.initialized = true;

    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);

    LOG_INFO(LOG_CAT_SYSTEM, "Model router initialized with %zu agent configs",
             g_router.config_count);
    return 0;
}

void router_shutdown(void) {
    CONVERGIO_MUTEX_LOCK(&g_router.mutex);

    if (!g_router.initialized) {
        CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
        return;
    }

    // Free agent configurations
    for (size_t i = 0; i < g_router.config_count; i++) {
        free(g_router.configs[i].agent_name);
        free(g_router.configs[i].primary_model);
        free(g_router.configs[i].fallback_model);
        free(g_router.configs[i].reason);
    }
    g_router.config_count = 0;

    provider_registry_shutdown();

    g_router.initialized = false;

    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);

    LOG_INFO(LOG_CAT_SYSTEM, "Model router shutdown. Total requests: %zu, Fallbacks: %zu",
             g_router.total_requests, g_router.fallback_requests);
}

// ============================================================================
// AGENT CONFIGURATION
// ============================================================================

static AgentModelConfig* find_agent_config(const char* agent_name) {
    if (!agent_name) return NULL;

    for (size_t i = 0; i < g_router.config_count; i++) {
        if (strcmp(g_router.configs[i].agent_name, agent_name) == 0) {
            return &g_router.configs[i];
        }
    }
    return NULL;
}

int router_set_agent_model(const char* agent_name, const char* primary_model,
                           const char* fallback_model) {
    // Validate required parameters
    if (!agent_name || !primary_model) {
        return -1;
    }

    CONVERGIO_MUTEX_LOCK(&g_router.mutex);

    AgentModelConfig* cfg = find_agent_config(agent_name);
    if (!cfg) {
        // Create new config
        if (g_router.config_count >= MAX_AGENT_CONFIGS) {
            CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
            return -1;
        }
        cfg = &g_router.configs[g_router.config_count];
        cfg->agent_name = strdup(agent_name);
        if (!cfg->agent_name) {
            CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
            return -1;  // OOM
        }
        cfg->primary_model = NULL;
        cfg->fallback_model = NULL;
        cfg->auto_downgrade = true;
        cfg->cost_tier = COST_TIER_MID;
        g_router.config_count++;
    }

    // Update models (free old values if any)
    char* new_primary = strdup(primary_model);
    char* new_fallback = fallback_model ? strdup(fallback_model) : NULL;

    // OOM check
    if (!new_primary || (fallback_model && !new_fallback)) {
        free(new_primary);
        free(new_fallback);
        CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
        return -1;
    }

    free(cfg->primary_model);
    free(cfg->fallback_model);
    cfg->primary_model = new_primary;
    cfg->fallback_model = new_fallback;

    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);

    LOG_INFO(LOG_CAT_AGENT, "Agent %s model config updated: primary=%s, fallback=%s",
             agent_name, primary_model, fallback_model ? fallback_model : "(none)");
    return 0;
}

const char* router_get_agent_model(const char* agent_name) {
    // If local MLX mode is enabled, always return the local model
    if (g_local_mlx_mode) {
        return g_local_mlx_model;
    }

    AgentModelConfig* cfg = find_agent_config(agent_name);
    if (cfg) {
        return cfg->primary_model;
    }
    // Default model if agent not configured
    return "anthropic/claude-sonnet-4";
}

// ============================================================================
// BUDGET MANAGEMENT
// ============================================================================

void router_set_budget(double daily, double session) {
    CONVERGIO_MUTEX_LOCK(&g_router.mutex);
    g_router.daily_budget = daily;
    g_router.session_budget = session;
    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);

    LOG_INFO(LOG_CAT_COST, "Budget set: daily=$%.2f, session=$%.2f", daily, session);
}

void router_reset_session_budget(void) {
    CONVERGIO_MUTEX_LOCK(&g_router.mutex);
    g_router.session_spent = 0.0;
    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
}

double router_get_remaining_budget(void) {
    CONVERGIO_MUTEX_LOCK(&g_router.mutex);

    // Check if daily budget needs reset
    time_t now = time(NULL);
    if (now - g_router.budget_reset_time >= 86400) {  // 24 hours
        g_router.daily_spent = 0.0;
        g_router.budget_reset_time = now;
    }

    double daily_remaining = g_router.daily_budget - g_router.daily_spent;
    double session_remaining = g_router.session_budget - g_router.session_spent;
    double remaining = (daily_remaining < session_remaining) ? daily_remaining : session_remaining;

    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
    return remaining;
}

void router_record_cost(double cost) {
    CONVERGIO_MUTEX_LOCK(&g_router.mutex);
    g_router.daily_spent += cost;
    g_router.session_spent += cost;

    // Check budget warnings
    double daily_pct = g_router.daily_spent / g_router.daily_budget;
    double session_pct = g_router.session_spent / g_router.session_budget;

    if (daily_pct >= BUDGET_WARNING_THRESHOLD && daily_pct < 1.0) {
        LOG_WARN(LOG_CAT_COST, "Daily budget %.0f%% used ($%.2f of $%.2f)",
                 daily_pct * 100, g_router.daily_spent, g_router.daily_budget);
    }
    if (session_pct >= BUDGET_WARNING_THRESHOLD && session_pct < 1.0) {
        LOG_WARN(LOG_CAT_COST, "Session budget %.0f%% used ($%.2f of $%.2f)",
                 session_pct * 100, g_router.session_spent, g_router.session_budget);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
}

// ============================================================================
// MODEL SELECTION
// ============================================================================

typedef enum {
    SELECT_REASON_PRIMARY,          // Using primary model
    SELECT_REASON_FALLBACK,         // Primary unavailable, using fallback
    SELECT_REASON_BUDGET_DOWNGRADE, // Budget low, downgraded model
    SELECT_REASON_DEFAULT,          // Using system default
} SelectReason;

typedef struct {
    const char* model_id;
    ProviderType provider;
    SelectReason reason;
    bool is_fallback;
} ModelSelection;

static bool is_model_available(const char* model_id) {
    const ModelConfig* config = model_get_config(model_id);
    if (!config) return false;
    return provider_is_available(config->provider);
}

static const char* get_cheaper_model(ProviderType provider) {
    // Find cheapest available model for provider
    const ModelConfig* cheapest = model_get_cheapest(provider);
    if (cheapest && provider_is_available(provider)) {
        return cheapest->id;
    }
    return NULL;
}

ModelSelection router_select_model_for_agent(const char* agent_name,
                                             double remaining_budget) {
    ModelSelection selection = {
        .model_id = NULL,
        .provider = PROVIDER_ANTHROPIC,
        .reason = SELECT_REASON_DEFAULT,
        .is_fallback = false
    };

    CONVERGIO_MUTEX_LOCK(&g_router.mutex);
    g_router.total_requests++;

    AgentModelConfig* cfg = find_agent_config(agent_name);

    // 1. Try primary model
    if (cfg && cfg->primary_model) {
        if (is_model_available(cfg->primary_model)) {
            // Check budget for auto-downgrade
            const ModelConfig* model = model_get_config(cfg->primary_model);
            if (model && cfg->auto_downgrade && remaining_budget < 1.0) {
                // Low budget - try to find cheaper alternative
                const char* cheaper = get_cheaper_model(model->provider);
                if (cheaper && strcmp(cheaper, cfg->primary_model) != 0) {
                    selection.model_id = cheaper;
                    selection.provider = model->provider;
                    selection.reason = SELECT_REASON_BUDGET_DOWNGRADE;
                    g_router.downgrade_requests++;
                    LOG_INFO(LOG_CAT_COST, "Agent %s downgraded from %s to %s (budget: $%.2f)",
                             agent_name, cfg->primary_model, cheaper, remaining_budget);
                    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
                    return selection;
                }
            }

            selection.model_id = cfg->primary_model;
            selection.provider = model ? model->provider : PROVIDER_ANTHROPIC;
            selection.reason = SELECT_REASON_PRIMARY;
            CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
            return selection;
        }
    }

    // 2. Try fallback model
    if (cfg && cfg->fallback_model) {
        if (is_model_available(cfg->fallback_model)) {
            const ModelConfig* model = model_get_config(cfg->fallback_model);
            selection.model_id = cfg->fallback_model;
            selection.provider = model ? model->provider : PROVIDER_ANTHROPIC;
            selection.reason = SELECT_REASON_FALLBACK;
            selection.is_fallback = true;
            g_router.fallback_requests++;
            LOG_WARN(LOG_CAT_AGENT, "Agent %s using fallback model %s",
                     agent_name, cfg->fallback_model);
            CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
            return selection;
        }
    }

    // 3. Use system default
    const char* default_model = "anthropic/claude-sonnet-4";
    if (provider_is_available(PROVIDER_ANTHROPIC)) {
        selection.model_id = default_model;
        selection.provider = PROVIDER_ANTHROPIC;
    } else if (provider_is_available(PROVIDER_OPENAI)) {
        selection.model_id = "openai/gpt-4o";
        selection.provider = PROVIDER_OPENAI;
    } else if (provider_is_available(PROVIDER_GEMINI)) {
        selection.model_id = "gemini/gemini-1.5-flash";
        selection.provider = PROVIDER_GEMINI;
    } else {
        LOG_ERROR(LOG_CAT_API, "No providers available!");
    }

    selection.reason = SELECT_REASON_DEFAULT;
    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
    return selection;
}

// ============================================================================
// HIGH-LEVEL API
// ============================================================================

/**
 * Select the best model for an agent and execute a chat request
 *
 * @param agent_name Name of the agent
 * @param system System prompt
 * @param user User message
 * @param usage Output token usage (can be NULL)
 * @return Response string (caller must free) or NULL on error
 */
char* router_chat(const char* agent_name, const char* system, const char* user, TokenUsage* usage) {
    if (!g_router.initialized) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Router not initialized");
        return NULL;
    }

    // Get remaining budget
    double remaining = router_get_remaining_budget();

    // Select model
    ModelSelection selection = router_select_model_for_agent(agent_name, remaining);
    if (!selection.model_id) {
        LOG_ERROR(LOG_CAT_API, "No model available for agent %s", agent_name);
        return NULL;
    }

    // Get provider
    Provider* provider = provider_get(selection.provider);
    if (!provider) {
        LOG_ERROR(LOG_CAT_API, "Provider %d not available", selection.provider);
        return NULL;
    }

    // Ensure provider is initialized
    if (!provider->initialized && provider->init) {
        ProviderError err = provider->init(provider);
        if (err != PROVIDER_OK) {
            LOG_ERROR(LOG_CAT_API, "Failed to initialize provider: %s",
                     provider_error_message(err));
            return NULL;
        }
    }

    // Execute chat
    LOG_DEBUG(LOG_CAT_API, "Routing %s request to %s via %s",
              agent_name, selection.model_id, provider->name);

    TokenUsage local_usage = {0};
    char* response = provider->chat(provider, selection.model_id, system, user, &local_usage);

    // Record cost
    if (response) {
        router_record_cost(local_usage.estimated_cost);
    }

    // Copy usage if requested
    if (usage) {
        *usage = local_usage;
    }

    return response;
}

/**
 * List all available models across all providers
 */
void router_list_models(void) {
    printf("\n━━━ Available Models ━━━\n\n");

    const char* provider_names[] = {"Anthropic", "OpenAI", "Gemini"};
    ProviderType providers[] = {PROVIDER_ANTHROPIC, PROVIDER_OPENAI, PROVIDER_GEMINI};

    for (int p = 0; p < 3; p++) {
        bool available = provider_is_available(providers[p]);
        printf("%s%s%s\n",
               available ? "\033[32m" : "\033[90m",
               provider_names[p],
               available ? "" : " (not configured)");
        printf("\033[0m");

        size_t count;
        const ModelConfig* models = model_get_by_provider(providers[p], &count);
        for (size_t i = 0; i < count; i++) {
            const ModelConfig* m = &models[i];
            printf("  %-22s $%.2f/$%.2f MTok   %zuK ctx",
                   m->id, m->input_cost_per_mtok, m->output_cost_per_mtok,
                   m->context_window / 1000);
            if (m->deprecated) {
                printf("   \033[33m(deprecated)\033[0m");
            }
            printf("\n");
        }
        printf("\n");
    }

    printf("Use: convergio -m <provider>/<model> to override default\n");
}

/**
 * Get router statistics
 */
void router_get_stats(size_t* total, size_t* fallbacks, size_t* downgrades,
                      double* daily_spent, double* session_spent) {
    CONVERGIO_MUTEX_LOCK(&g_router.mutex);
    if (total) *total = g_router.total_requests;
    if (fallbacks) *fallbacks = g_router.fallback_requests;
    if (downgrades) *downgrades = g_router.downgrade_requests;
    if (daily_spent) *daily_spent = g_router.daily_spent;
    if (session_spent) *session_spent = g_router.session_spent;
    CONVERGIO_MUTEX_UNLOCK(&g_router.mutex);
}
