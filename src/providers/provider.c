/**
 * CONVERGIO PROVIDER REGISTRY
 *
 * Central registry for managing multiple LLM providers
 * Handles initialization, model lookup, and provider selection
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <ctype.h>

// ============================================================================
// PROVIDER REGISTRY STATE
// ============================================================================

static Provider* g_providers[PROVIDER_COUNT] = {NULL};
static bool g_registry_initialized = false;
static pthread_mutex_t g_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

// ============================================================================
// BUILT-IN MODEL CONFIGURATIONS (December 2025)
// ============================================================================

// Anthropic Models
static ModelConfig g_anthropic_models[] = {
    {
        .id = "claude-opus-4.5",
        .display_name = "Claude Opus 4.5",
        .provider = PROVIDER_ANTHROPIC,
        .input_cost_per_mtok = 15.0,
        .output_cost_per_mtok = 75.0,
        .thinking_cost_per_mtok = 40.0,
        .context_window = 200000,
        .max_output = 8192,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_PREMIUM,
        .released = "2025-02-01",
        .deprecated = false
    },
    {
        .id = "claude-sonnet-4.5",
        .display_name = "Claude Sonnet 4.5",
        .provider = PROVIDER_ANTHROPIC,
        .input_cost_per_mtok = 3.0,
        .output_cost_per_mtok = 15.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 1000000,
        .max_output = 8192,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_MID,
        .released = "2025-05-01",
        .deprecated = false
    },
    {
        .id = "claude-sonnet-4",
        .display_name = "Claude Sonnet 4",
        .provider = PROVIDER_ANTHROPIC,
        .input_cost_per_mtok = 3.0,
        .output_cost_per_mtok = 15.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 200000,
        .max_output = 8192,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_MID,
        .released = "2025-05-01",
        .deprecated = false
    },
    {
        .id = "claude-haiku-4.5",
        .display_name = "Claude Haiku 4.5",
        .provider = PROVIDER_ANTHROPIC,
        .input_cost_per_mtok = 1.0,
        .output_cost_per_mtok = 5.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 200000,
        .max_output = 8192,
        .supports_tools = true,
        .supports_vision = false,
        .supports_streaming = true,
        .tier = COST_TIER_CHEAP,
        .released = "2025-03-01",
        .deprecated = false
    }
};
static size_t g_anthropic_model_count = sizeof(g_anthropic_models) / sizeof(g_anthropic_models[0]);

// OpenAI Models
static ModelConfig g_openai_models[] = {
    {
        .id = "gpt-5.2-pro",
        .display_name = "GPT-5.2 Pro",
        .provider = PROVIDER_OPENAI,
        .input_cost_per_mtok = 5.0,
        .output_cost_per_mtok = 20.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 400000,
        .max_output = 128000,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_MID,
        .released = "2025-12-01",
        .deprecated = false
    },
    {
        .id = "gpt-5.2-thinking",
        .display_name = "GPT-5.2 Thinking",
        .provider = PROVIDER_OPENAI,
        .input_cost_per_mtok = 2.5,
        .output_cost_per_mtok = 15.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 400000,
        .max_output = 128000,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_MID,
        .released = "2025-12-01",
        .deprecated = false
    },
    {
        .id = "gpt-5.2-instant",
        .display_name = "GPT-5.2 Instant",
        .provider = PROVIDER_OPENAI,
        .input_cost_per_mtok = 1.25,
        .output_cost_per_mtok = 10.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 400000,
        .max_output = 128000,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_CHEAP,
        .released = "2025-12-01",
        .deprecated = false
    },
    {
        .id = "gpt-5",
        .display_name = "GPT-5",
        .provider = PROVIDER_OPENAI,
        .input_cost_per_mtok = 1.25,
        .output_cost_per_mtok = 10.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 256000,
        .max_output = 64000,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_CHEAP,
        .released = "2025-06-01",
        .deprecated = false
    },
    {
        .id = "gpt-4o",
        .display_name = "GPT-4o",
        .provider = PROVIDER_OPENAI,
        .input_cost_per_mtok = 5.0,
        .output_cost_per_mtok = 15.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 128000,
        .max_output = 16384,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_MID,
        .released = "2024-05-01",
        .deprecated = false
    },
    {
        .id = "o3",
        .display_name = "o3",
        .provider = PROVIDER_OPENAI,
        .input_cost_per_mtok = 10.0,
        .output_cost_per_mtok = 40.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 128000,
        .max_output = 32000,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_PREMIUM,
        .released = "2025-01-01",
        .deprecated = false
    },
    {
        .id = "o4-mini",
        .display_name = "o4-mini",
        .provider = PROVIDER_OPENAI,
        .input_cost_per_mtok = 0.15,
        .output_cost_per_mtok = 0.60,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 128000,
        .max_output = 16384,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_CHEAP,
        .released = "2025-04-01",
        .deprecated = false
    },
    {
        .id = "gpt-5-nano",
        .display_name = "GPT-5 Nano",
        .provider = PROVIDER_OPENAI,
        .input_cost_per_mtok = 0.05,
        .output_cost_per_mtok = 0.40,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 128000,
        .max_output = 16384,
        .supports_tools = true,
        .supports_vision = false,
        .supports_streaming = true,
        .tier = COST_TIER_CHEAP,
        .released = "2025-09-01",
        .deprecated = false
    }
};
static size_t g_openai_model_count = sizeof(g_openai_models) / sizeof(g_openai_models[0]);

// Gemini Models
static ModelConfig g_gemini_models[] = {
    {
        .id = "gemini-3-pro",
        .display_name = "Gemini 3 Pro",
        .provider = PROVIDER_GEMINI,
        .input_cost_per_mtok = 2.0,
        .output_cost_per_mtok = 12.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 200000,
        .max_output = 8192,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_MID,
        .released = "2025-06-01",
        .deprecated = false
    },
    {
        .id = "gemini-3-ultra",
        .display_name = "Gemini 3 Ultra",
        .provider = PROVIDER_GEMINI,
        .input_cost_per_mtok = 7.0,
        .output_cost_per_mtok = 21.0,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 2000000,
        .max_output = 8192,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_MID,
        .released = "2025-06-01",
        .deprecated = false
    },
    {
        .id = "gemini-3-flash",
        .display_name = "Gemini 3 Flash",
        .provider = PROVIDER_GEMINI,
        .input_cost_per_mtok = 0.075,
        .output_cost_per_mtok = 0.30,
        .thinking_cost_per_mtok = 0.0,
        .context_window = 1000000,
        .max_output = 8192,
        .supports_tools = true,
        .supports_vision = true,
        .supports_streaming = true,
        .tier = COST_TIER_CHEAP,
        .released = "2025-06-01",
        .deprecated = false
    }
};
static size_t g_gemini_model_count = sizeof(g_gemini_models) / sizeof(g_gemini_models[0]);

// ============================================================================
// PROVIDER NAME MAPPING
// ============================================================================

static const char* g_provider_names[] = {
    [PROVIDER_ANTHROPIC] = "anthropic",
    [PROVIDER_OPENAI] = "openai",
    [PROVIDER_GEMINI] = "gemini",
    [PROVIDER_OLLAMA] = "ollama"
};

__attribute__((unused))
static const char* g_provider_display_names[] = {
    [PROVIDER_ANTHROPIC] = "Anthropic",
    [PROVIDER_OPENAI] = "OpenAI",
    [PROVIDER_GEMINI] = "Google Gemini",
    [PROVIDER_OLLAMA] = "Ollama (Local)"
};

static const char* g_provider_api_key_envs[] = {
    [PROVIDER_ANTHROPIC] = "ANTHROPIC_API_KEY",
    [PROVIDER_OPENAI] = "OPENAI_API_KEY",
    [PROVIDER_GEMINI] = "GEMINI_API_KEY",
    [PROVIDER_OLLAMA] = NULL  // No API key needed for local
};

// ============================================================================
// ERROR MESSAGES
// ============================================================================

static const char* g_error_messages[] = {
    [PROVIDER_OK] = "Success",
    [PROVIDER_ERR_AUTH] = "API key invalid or expired. Run 'convergio setup' to reconfigure.",
    [PROVIDER_ERR_RATE_LIMIT] = "Rate limit exceeded. Retrying automatically...",
    [PROVIDER_ERR_QUOTA] = "API quota exceeded. Check your provider dashboard.",
    [PROVIDER_ERR_CONTEXT_LENGTH] = "Input too long for this model. Consider using a model with larger context.",
    [PROVIDER_ERR_CONTENT_FILTER] = "Content was filtered by the provider's safety system.",
    [PROVIDER_ERR_MODEL_NOT_FOUND] = "Model not found. Run 'convergio models' to see available models.",
    [PROVIDER_ERR_OVERLOADED] = "Provider service is overloaded. Retrying...",
    [PROVIDER_ERR_TIMEOUT] = "Request timed out. Please try again.",
    [PROVIDER_ERR_NETWORK] = "Network error. Check your internet connection.",
    [PROVIDER_ERR_INVALID_REQUEST] = "Invalid request. This may be a bug - please report it.",
    [PROVIDER_ERR_NOT_INITIALIZED] = "Provider not initialized. Call provider_registry_init() first.",
    [PROVIDER_ERR_UNKNOWN] = "An unexpected error occurred."
};

// ============================================================================
// PROVIDER REGISTRY IMPLEMENTATION
// ============================================================================

// Forward declarations for provider adapters
extern Provider* anthropic_provider_create(void);
extern Provider* openai_provider_create(void);
extern Provider* gemini_provider_create(void);
extern Provider* ollama_provider_create(void);

ProviderError provider_registry_init(void) {
    pthread_mutex_lock(&g_registry_mutex);

    if (g_registry_initialized) {
        pthread_mutex_unlock(&g_registry_mutex);
        return PROVIDER_OK;
    }

    LOG_INFO(LOG_CAT_SYSTEM, "Initializing provider registry...");

    // Create provider instances (they don't need to be fully initialized yet)
    // Full initialization happens when the provider is first used and has valid API key

    // Anthropic provider is always created (currently supported)
    g_providers[PROVIDER_ANTHROPIC] = anthropic_provider_create();
    if (g_providers[PROVIDER_ANTHROPIC]) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "Anthropic provider created");
    }

    // OpenAI provider (stub for now)
    // g_providers[PROVIDER_OPENAI] = openai_provider_create();

    // Gemini provider (stub for now)
    // g_providers[PROVIDER_GEMINI] = gemini_provider_create();

    // Ollama provider (stub for now)
    // g_providers[PROVIDER_OLLAMA] = ollama_provider_create();

    g_registry_initialized = true;
    pthread_mutex_unlock(&g_registry_mutex);

    LOG_INFO(LOG_CAT_SYSTEM, "Provider registry initialized");
    return PROVIDER_OK;
}

void provider_registry_shutdown(void) {
    pthread_mutex_lock(&g_registry_mutex);

    if (!g_registry_initialized) {
        pthread_mutex_unlock(&g_registry_mutex);
        return;
    }

    LOG_INFO(LOG_CAT_SYSTEM, "Shutting down provider registry...");

    for (int i = 0; i < PROVIDER_COUNT; i++) {
        if (g_providers[i]) {
            if (g_providers[i]->shutdown) {
                g_providers[i]->shutdown(g_providers[i]);
            }
            free(g_providers[i]);
            g_providers[i] = NULL;
        }
    }

    g_registry_initialized = false;
    pthread_mutex_unlock(&g_registry_mutex);

    LOG_INFO(LOG_CAT_SYSTEM, "Provider registry shutdown complete");
}

Provider* provider_get(ProviderType type) {
    if (type < 0 || type >= PROVIDER_COUNT) {
        return NULL;
    }

    pthread_mutex_lock(&g_registry_mutex);
    Provider* provider = g_providers[type];
    pthread_mutex_unlock(&g_registry_mutex);

    return provider;
}

bool provider_is_available(ProviderType type) {
    Provider* provider = provider_get(type);
    if (!provider) {
        return false;
    }

    // Check if API key is set
    const char* env_var = g_provider_api_key_envs[type];
    if (env_var) {
        const char* api_key = getenv(env_var);
        if (!api_key || strlen(api_key) == 0) {
            return false;
        }
    }

    // Validate key if provider is initialized
    if (provider->initialized && provider->validate_key) {
        return provider->validate_key(provider);
    }

    return true;
}

const char* provider_name(ProviderType type) {
    if (type < 0 || type >= PROVIDER_COUNT) {
        return "unknown";
    }
    return g_provider_names[type];
}

// ============================================================================
// MODEL REGISTRY IMPLEMENTATION
// ============================================================================

static ModelConfig* find_model_in_array(const char* model_id, ModelConfig* models, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(models[i].id, model_id) == 0) {
            return &models[i];
        }
    }
    return NULL;
}

const ModelConfig* model_get_config(const char* model_id) {
    if (!model_id) return NULL;

    // Handle prefixed model IDs (e.g., "anthropic/claude-opus-4.5")
    const char* slash = strchr(model_id, '/');
    const char* actual_id = slash ? slash + 1 : model_id;

    // Determine provider from prefix if present
    ProviderType hint = PROVIDER_COUNT;  // Invalid, search all
    if (slash) {
        size_t prefix_len = slash - model_id;
        for (int i = 0; i < PROVIDER_COUNT; i++) {
            if (strncmp(model_id, g_provider_names[i], prefix_len) == 0 &&
                strlen(g_provider_names[i]) == prefix_len) {
                hint = i;
                break;
            }
        }
    }

    // Search in appropriate provider(s)
    ModelConfig* result = NULL;

    if (hint == PROVIDER_ANTHROPIC || hint == PROVIDER_COUNT) {
        result = find_model_in_array(actual_id, g_anthropic_models, g_anthropic_model_count);
        if (result) return result;
    }

    if (hint == PROVIDER_OPENAI || hint == PROVIDER_COUNT) {
        result = find_model_in_array(actual_id, g_openai_models, g_openai_model_count);
        if (result) return result;
    }

    if (hint == PROVIDER_GEMINI || hint == PROVIDER_COUNT) {
        result = find_model_in_array(actual_id, g_gemini_models, g_gemini_model_count);
        if (result) return result;
    }

    return NULL;
}

const ModelConfig* model_get_by_provider(ProviderType type, size_t* out_count) {
    switch (type) {
        case PROVIDER_ANTHROPIC:
            if (out_count) *out_count = g_anthropic_model_count;
            return g_anthropic_models;
        case PROVIDER_OPENAI:
            if (out_count) *out_count = g_openai_model_count;
            return g_openai_models;
        case PROVIDER_GEMINI:
            if (out_count) *out_count = g_gemini_model_count;
            return g_gemini_models;
        default:
            if (out_count) *out_count = 0;
            return NULL;
    }
}

const ModelConfig* model_get_by_tier(CostTier tier, size_t* out_count) {
    // This would require building a dynamic array - for now return NULL
    // In production, this should be implemented properly
    if (out_count) *out_count = 0;
    return NULL;
}

const ModelConfig* model_get_cheapest(ProviderType type) {
    size_t count;
    const ModelConfig* models = model_get_by_provider(type, &count);
    if (!models || count == 0) return NULL;

    const ModelConfig* cheapest = &models[0];
    double min_cost = cheapest->input_cost_per_mtok + cheapest->output_cost_per_mtok;

    for (size_t i = 1; i < count; i++) {
        double cost = models[i].input_cost_per_mtok + models[i].output_cost_per_mtok;
        if (cost < min_cost && !models[i].deprecated) {
            cheapest = &models[i];
            min_cost = cost;
        }
    }

    return cheapest;
}

double model_estimate_cost(const char* model_id, size_t input_tokens, size_t output_tokens) {
    const ModelConfig* model = model_get_config(model_id);
    if (!model) return 0.0;

    double input_cost = (double)input_tokens / 1000000.0 * model->input_cost_per_mtok;
    double output_cost = (double)output_tokens / 1000000.0 * model->output_cost_per_mtok;

    return input_cost + output_cost;
}

// ============================================================================
// ERROR HANDLING UTILITIES
// ============================================================================

const char* provider_error_message(ProviderError code) {
    if (code < 0 || code > PROVIDER_ERR_UNKNOWN) {
        return g_error_messages[PROVIDER_ERR_UNKNOWN];
    }
    return g_error_messages[code];
}

bool provider_error_is_retryable(ProviderError code) {
    switch (code) {
        case PROVIDER_ERR_RATE_LIMIT:
        case PROVIDER_ERR_OVERLOADED:
        case PROVIDER_ERR_TIMEOUT:
        case PROVIDER_ERR_NETWORK:
            return true;
        default:
            return false;
    }
}

void provider_error_free(ProviderErrorInfo* info) {
    if (!info) return;
    free(info->message);
    free(info->provider_code);
    free(info);
}

// ============================================================================
// TOOL CALL UTILITIES
// ============================================================================

void tool_calls_free(ToolCall* calls, size_t count) {
    if (!calls) return;
    for (size_t i = 0; i < count; i++) {
        free(calls[i].tool_name);
        free(calls[i].tool_id);
        free(calls[i].arguments_json);
    }
    free(calls);
}

// ============================================================================
// RETRY CONFIGURATION
// ============================================================================

RetryConfig retry_config_default(void) {
    return (RetryConfig){
        .max_retries = 3,
        .base_delay_ms = 1000,
        .max_delay_ms = 60000,
        .jitter_factor = 0.2,
        .retry_on_rate_limit = true,
        .retry_on_server_error = true
    };
}

int retry_calculate_delay(const RetryConfig* cfg, int attempt) {
    if (!cfg || attempt < 0) return 1000;

    // Exponential backoff: base * 2^attempt
    int delay = cfg->base_delay_ms * (1 << attempt);
    if (delay > cfg->max_delay_ms) {
        delay = cfg->max_delay_ms;
    }

    // Add jitter to prevent thundering herd
    // Random value between -jitter/2 and +jitter/2
    double jitter_range = delay * cfg->jitter_factor;
    int jitter = (int)((((double)rand() / RAND_MAX) * jitter_range) - (jitter_range / 2));
    delay += jitter;

    if (delay < 0) delay = cfg->base_delay_ms;

    return delay;
}
