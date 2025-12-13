/**
 * CONVERGIO AGENT CONFIGURATION
 *
 * Agent model configuration and router integration:
 * - YAML/JSON agent definition parsing
 * - Model assignment per agent
 * - Provider fallback chains
 * - Dynamic reconfiguration
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// ============================================================================
// AGENT MODEL CONFIGURATION
// ============================================================================

typedef struct {
    ProviderType provider;
    char* model_id;
} ModelSpec;

typedef struct {
    char* agent_name;
    char* description;
    AgentRole role;

    // Model configuration
    ModelSpec primary;
    ModelSpec fallback;
    ModelSpec economy;     // For budget-constrained scenarios

    // Behavior settings
    int max_tokens;
    double temperature;
    bool streaming_enabled;
    bool tool_calling_enabled;

    // Cost settings
    double max_cost_per_call;
    double session_budget;
} AgentConfig;

// ============================================================================
// DEFAULT CONFIGURATIONS
// ============================================================================

static AgentConfig g_default_configs[] = {
    // Ali - Orchestrator (needs best reasoning)
    {
        .agent_name = "ali",
        .description = "Chief of Staff - orchestrates all agents",
        .role = AGENT_ROLE_ORCHESTRATOR,
        .primary = {PROVIDER_ANTHROPIC, "claude-opus-4"},
        .fallback = {PROVIDER_ANTHROPIC, "claude-sonnet-4"},
        .economy = {PROVIDER_OPENAI, "gpt-4o"},
        .max_tokens = 8192,
        .temperature = 0.7,
        .streaming_enabled = true,
        .tool_calling_enabled = true,
        .max_cost_per_call = 1.0,
        .session_budget = 10.0,
    },
    // Marco - Coder
    {
        .agent_name = "marco",
        .description = "Expert coder - code generation and review",
        .role = AGENT_ROLE_CODER,
        .primary = {PROVIDER_ANTHROPIC, "claude-sonnet-4"},
        .fallback = {PROVIDER_OPENAI, "o1"},
        .economy = {PROVIDER_GEMINI, "gemini-1.5-pro"},
        .max_tokens = 16384,
        .temperature = 0.3,
        .streaming_enabled = true,
        .tool_calling_enabled = true,
        .max_cost_per_call = 0.5,
        .session_budget = 5.0,
    },
    // Sara - Writer
    {
        .agent_name = "sara",
        .description = "Content writer - documentation and copywriting",
        .role = AGENT_ROLE_WRITER,
        .primary = {PROVIDER_ANTHROPIC, "claude-sonnet-4"},
        .fallback = {PROVIDER_GEMINI, "gemini-1.5-pro"},
        .economy = {PROVIDER_OPENAI, "gpt-4o"},
        .max_tokens = 8192,
        .temperature = 0.8,
        .streaming_enabled = true,
        .tool_calling_enabled = false,
        .max_cost_per_call = 0.3,
        .session_budget = 3.0,
    },
    // Leo - Analyst
    {
        .agent_name = "leo",
        .description = "Deep analyst - research and analysis",
        .role = AGENT_ROLE_ANALYST,
        .primary = {PROVIDER_OPENAI, "gpt-4o"},
        .fallback = {PROVIDER_ANTHROPIC, "claude-sonnet-4"},
        .economy = {PROVIDER_GEMINI, "gemini-1.5-pro"},
        .max_tokens = 16384,
        .temperature = 0.5,
        .streaming_enabled = true,
        .tool_calling_enabled = true,
        .max_cost_per_call = 0.5,
        .session_budget = 5.0,
    },
    // Nina - Critic
    {
        .agent_name = "nina",
        .description = "Critic - review and validation",
        .role = AGENT_ROLE_CRITIC,
        .primary = {PROVIDER_ANTHROPIC, "claude-haiku-4.5"},
        .fallback = {PROVIDER_OPENAI, "gpt-4o-mini"},
        .economy = {PROVIDER_GEMINI, "gemini-1.5-flash"},
        .max_tokens = 4096,
        .temperature = 0.3,
        .streaming_enabled = false,
        .tool_calling_enabled = false,
        .max_cost_per_call = 0.1,
        .session_budget = 1.0,
    },
    // Router - Task routing (ultra fast)
    {
        .agent_name = "router",
        .description = "Task router - fast classification",
        .role = AGENT_ROLE_EXECUTOR,
        .primary = {PROVIDER_OPENAI, "gpt-4o-mini"},
        .fallback = {PROVIDER_GEMINI, "gemini-1.5-flash"},
        .economy = {PROVIDER_GEMINI, "gemini-1.5-flash"},
        .max_tokens = 1024,
        .temperature = 0.1,
        .streaming_enabled = false,
        .tool_calling_enabled = false,
        .max_cost_per_call = 0.01,
        .session_budget = 0.5,
    },
    // Sentinel
    {.agent_name = NULL}
};

// ============================================================================
// CONFIGURATION REGISTRY
// ============================================================================

static AgentConfig* g_configs = NULL;
static size_t g_config_count = 0;
static size_t g_config_capacity = 0;

static AgentConfig* config_find(const char* agent_name) {
    for (size_t i = 0; i < g_config_count; i++) {
        if (strcmp(g_configs[i].agent_name, agent_name) == 0) {
            return &g_configs[i];
        }
    }
    return NULL;
}

static AgentConfig* config_find_default(const char* agent_name) {
    for (int i = 0; g_default_configs[i].agent_name != NULL; i++) {
        if (strcmp(g_default_configs[i].agent_name, agent_name) == 0) {
            return &g_default_configs[i];
        }
    }
    return NULL;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int agent_config_init(void) {
    // Load defaults
    g_config_capacity = 16;
    g_configs = calloc(g_config_capacity, sizeof(AgentConfig));
    if (!g_configs) return -1;

    // Copy default configs
    for (int i = 0; g_default_configs[i].agent_name != NULL; i++) {
        if (g_config_count >= g_config_capacity) {
            g_config_capacity *= 2;
            g_configs = realloc(g_configs, g_config_capacity * sizeof(AgentConfig));
            if (!g_configs) return -1;
        }

        AgentConfig* config = &g_configs[g_config_count++];
        *config = g_default_configs[i];
        config->agent_name = strdup(g_default_configs[i].agent_name);
        config->description = strdup(g_default_configs[i].description);
        config->primary.model_id = strdup(g_default_configs[i].primary.model_id);
        config->fallback.model_id = strdup(g_default_configs[i].fallback.model_id);
        config->economy.model_id = strdup(g_default_configs[i].economy.model_id);
    }

    return 0;
}

void agent_config_shutdown(void) {
    for (size_t i = 0; i < g_config_count; i++) {
        free(g_configs[i].agent_name);
        free(g_configs[i].description);
        free(g_configs[i].primary.model_id);
        free(g_configs[i].fallback.model_id);
        free(g_configs[i].economy.model_id);
    }
    free(g_configs);
    g_configs = NULL;
    g_config_count = 0;
    g_config_capacity = 0;
}

// ============================================================================
// CONFIGURATION ACCESS
// ============================================================================

AgentConfig* agent_config_get(const char* agent_name) {
    AgentConfig* config = config_find(agent_name);
    if (config) return config;

    // Check defaults
    return config_find_default(agent_name);
}

const char* agent_config_get_model(const char* agent_name, double remaining_budget) {
    AgentConfig* config = agent_config_get(agent_name);
    if (!config) return "claude-sonnet-4";  // Ultimate fallback

    // Budget-based selection
    if (remaining_budget < 0.1) {
        return config->economy.model_id;
    } else if (remaining_budget < 1.0) {
        return config->fallback.model_id;
    }

    return config->primary.model_id;
}

ProviderType agent_config_get_provider(const char* agent_name, double remaining_budget) {
    AgentConfig* config = agent_config_get(agent_name);
    if (!config) return PROVIDER_ANTHROPIC;

    if (remaining_budget < 0.1) {
        return config->economy.provider;
    } else if (remaining_budget < 1.0) {
        return config->fallback.provider;
    }

    return config->primary.provider;
}

// ============================================================================
// CONFIGURATION UPDATE
// ============================================================================

int agent_config_set_model(const char* agent_name, ProviderType provider,
                           const char* model_id) {
    AgentConfig* config = config_find(agent_name);
    if (!config) return -1;

    free(config->primary.model_id);
    config->primary.provider = provider;
    config->primary.model_id = strdup(model_id);

    return 0;
}

int agent_config_set_fallback(const char* agent_name, ProviderType provider,
                              const char* model_id) {
    AgentConfig* config = config_find(agent_name);
    if (!config) return -1;

    free(config->fallback.model_id);
    config->fallback.provider = provider;
    config->fallback.model_id = strdup(model_id);

    return 0;
}

int agent_config_set_temperature(const char* agent_name, double temperature) {
    AgentConfig* config = config_find(agent_name);
    if (!config) return -1;

    config->temperature = temperature;
    return 0;
}

int agent_config_set_max_tokens(const char* agent_name, int max_tokens) {
    AgentConfig* config = config_find(agent_name);
    if (!config) return -1;

    config->max_tokens = max_tokens;
    return 0;
}

// ============================================================================
// JSON CONFIGURATION LOADING
// ============================================================================

/**
 * Parse a single agent config from JSON
 * Expected format:
 * {
 *   "name": "marco",
 *   "description": "Expert coder",
 *   "role": "coder",
 *   "model": {
 *     "provider": "anthropic",
 *     "model_id": "claude-sonnet-4"
 *   },
 *   "fallback": {
 *     "provider": "openai",
 *     "model_id": "o1"
 *   },
 *   "settings": {
 *     "max_tokens": 16384,
 *     "temperature": 0.3,
 *     "streaming": true,
 *     "tools": true
 *   },
 *   "budget": {
 *     "max_per_call": 0.5,
 *     "session": 5.0
 *   }
 * }
 */
static ProviderType parse_provider(const char* str) {
    if (!str) return PROVIDER_ANTHROPIC;
    if (strstr(str, "anthropic")) return PROVIDER_ANTHROPIC;
    if (strstr(str, "openai")) return PROVIDER_OPENAI;
    if (strstr(str, "gemini")) return PROVIDER_GEMINI;
    if (strstr(str, "ollama")) return PROVIDER_OLLAMA;
    return PROVIDER_ANTHROPIC;
}

static AgentRole parse_role(const char* str) {
    if (!str) return AGENT_ROLE_EXECUTOR;
    if (strstr(str, "orchestrator")) return AGENT_ROLE_ORCHESTRATOR;
    if (strstr(str, "analyst")) return AGENT_ROLE_ANALYST;
    if (strstr(str, "coder")) return AGENT_ROLE_CODER;
    if (strstr(str, "writer")) return AGENT_ROLE_WRITER;
    if (strstr(str, "critic")) return AGENT_ROLE_CRITIC;
    if (strstr(str, "planner")) return AGENT_ROLE_PLANNER;
    if (strstr(str, "executor")) return AGENT_ROLE_EXECUTOR;
    if (strstr(str, "memory")) return AGENT_ROLE_MEMORY;
    return AGENT_ROLE_EXECUTOR;
}

// Simple JSON string extraction (real implementation would use cJSON)
static char* extract_string(const char* json, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":\"", key);

    const char* start = strstr(json, search);
    if (!start) return NULL;

    start += strlen(search);
    const char* end = strchr(start, '"');
    if (!end) return NULL;

    return strndup(start, (size_t)(end - start));
}

static double extract_number(const char* json, const char* key, double default_val) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char* start = strstr(json, search);
    if (!start) return default_val;

    start += strlen(search);
    while (*start == ' ') start++;

    return atof(start);
}

static bool extract_bool(const char* json, const char* key, bool default_val) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char* start = strstr(json, search);
    if (!start) return default_val;

    start += strlen(search);
    while (*start == ' ') start++;

    return strncmp(start, "true", 4) == 0;
}

int agent_config_load_json(const char* json) {
    if (!json) return -1;

    char* name = extract_string(json, "name");
    if (!name) return -1;

    // Find or create config
    AgentConfig* config = config_find(name);
    bool is_new = (config == NULL);

    if (is_new) {
        if (g_config_count >= g_config_capacity) {
            g_config_capacity *= 2;
            g_configs = realloc(g_configs, g_config_capacity * sizeof(AgentConfig));
            if (!g_configs) {
                free(name);
                return -1;
            }
        }
        config = &g_configs[g_config_count++];
        memset(config, 0, sizeof(AgentConfig));
        config->agent_name = name;
    } else {
        free(name);
    }

    // Parse fields
    char* description = extract_string(json, "description");
    if (description) {
        free(config->description);
        config->description = description;
    }

    char* role = extract_string(json, "role");
    if (role) {
        config->role = parse_role(role);
        free(role);
    }

    // Parse model section
    const char* model_section = strstr(json, "\"model\":");
    if (model_section) {
        char* provider = extract_string(model_section, "provider");
        char* model_id = extract_string(model_section, "model_id");
        if (provider && model_id) {
            config->primary.provider = parse_provider(provider);
            free(config->primary.model_id);
            config->primary.model_id = model_id;
        }
        free(provider);
    }

    // Parse fallback section
    const char* fallback_section = strstr(json, "\"fallback\":");
    if (fallback_section) {
        char* provider = extract_string(fallback_section, "provider");
        char* model_id = extract_string(fallback_section, "model_id");
        if (provider && model_id) {
            config->fallback.provider = parse_provider(provider);
            free(config->fallback.model_id);
            config->fallback.model_id = model_id;
        }
        free(provider);
    }

    // Parse settings
    const char* settings_section = strstr(json, "\"settings\":");
    if (settings_section) {
        config->max_tokens = (int)extract_number(settings_section, "max_tokens", config->max_tokens);
        config->temperature = extract_number(settings_section, "temperature", config->temperature);
        config->streaming_enabled = extract_bool(settings_section, "streaming", config->streaming_enabled);
        config->tool_calling_enabled = extract_bool(settings_section, "tools", config->tool_calling_enabled);
    }

    // Parse budget
    const char* budget_section = strstr(json, "\"budget\":");
    if (budget_section) {
        config->max_cost_per_call = extract_number(budget_section, "max_per_call", config->max_cost_per_call);
        config->session_budget = extract_number(budget_section, "session", config->session_budget);
    }

    return 0;
}

// ============================================================================
// CONFIGURATION LOADING FROM DIRECTORY
// ============================================================================

int agent_config_load_directory(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) return -1;

    struct dirent* entry;
    int loaded = 0;

    while ((entry = readdir(dir)) != NULL) {
        // Skip non-JSON files
        const char* ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".json") != 0) continue;

        // Build full path
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        // Read file
        FILE* f = fopen(path, "r");
        if (!f) continue;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        char* content = malloc((size_t)size + 1);
        if (!content) {
            fclose(f);
            continue;
        }

        fread(content, 1, (size_t)size, f);
        content[size] = '\0';
        fclose(f);

        // Parse
        if (agent_config_load_json(content) == 0) {
            loaded++;
        }

        free(content);
    }

    closedir(dir);
    return loaded;
}

// ============================================================================
// CONFIGURATION EXPORT
// ============================================================================

char* agent_config_to_json(const char* agent_name) {
    AgentConfig* config = agent_config_get(agent_name);
    if (!config) return NULL;

    const char* provider_names[] = {"anthropic", "openai", "gemini", "ollama"};
    const char* role_names[] = {"orchestrator", "analyst", "coder", "writer",
                                 "critic", "planner", "executor", "memory"};

    size_t size = 1024;
    char* json = malloc(size);
    if (!json) return NULL;

    snprintf(json, size,
        "{"
        "\"name\":\"%s\","
        "\"description\":\"%s\","
        "\"role\":\"%s\","
        "\"model\":{\"provider\":\"%s\",\"model_id\":\"%s\"},"
        "\"fallback\":{\"provider\":\"%s\",\"model_id\":\"%s\"},"
        "\"settings\":{\"max_tokens\":%d,\"temperature\":%.2f,\"streaming\":%s,\"tools\":%s},"
        "\"budget\":{\"max_per_call\":%.2f,\"session\":%.2f}"
        "}",
        config->agent_name,
        config->description ? config->description : "",
        role_names[config->role],
        provider_names[config->primary.provider],
        config->primary.model_id ? config->primary.model_id : "",
        provider_names[config->fallback.provider],
        config->fallback.model_id ? config->fallback.model_id : "",
        config->max_tokens,
        config->temperature,
        config->streaming_enabled ? "true" : "false",
        config->tool_calling_enabled ? "true" : "false",
        config->max_cost_per_call,
        config->session_budget);

    return json;
}

char* agent_config_list_json(void) {
    size_t size = 256 + g_config_count * 128;
    char* json = malloc(size);
    if (!json) return NULL;

    size_t offset = (size_t)snprintf(json, size, "[");

    for (size_t i = 0; i < g_config_count; i++) {
        if (i > 0) {
            offset += (size_t)snprintf(json + offset, size - offset, ",");
        }
        offset += (size_t)snprintf(json + offset, size - offset,
            "{\"name\":\"%s\",\"role\":\"%s\",\"model\":\"%s\"}",
            g_configs[i].agent_name,
            g_configs[i].description ? g_configs[i].description : "",
            g_configs[i].primary.model_id ? g_configs[i].primary.model_id : "");
    }

    snprintf(json + offset, size - offset, "]");
    return json;
}
