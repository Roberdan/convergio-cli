/**
 * MODEL LOADER - Dynamic model configuration from JSON
 *
 * Loads ALL model configurations from config/models.json
 * This is the SINGLE SOURCE OF TRUTH for model data
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/model_loader.h"
#include "nous/nous.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

// ============================================================================
// STATIC STATE
// ============================================================================

static bool g_loader_initialized = false;
static bool g_loaded_from_json = false;
static char* g_loaded_path = NULL;
static char* g_version = NULL;

// Full model catalog loaded from JSON
#define MAX_PROVIDERS 10
#define MAX_TOTAL_MODELS 200

typedef struct {
    char* provider_name;
    JsonModelConfig config;
} LoadedModel;

static LoadedModel g_models[MAX_TOTAL_MODELS];
static size_t g_model_count = 0;

// Compare defaults
static char* g_compare_defaults[MAX_COMPARE_DEFAULTS] = {NULL};
static size_t g_compare_count = 0;

// Benchmark defaults
static char* g_benchmark_model = NULL;
static size_t g_benchmark_iterations = 3;

// Fallback defaults (used if JSON not found)
static const char* FALLBACK_COMPARE[] = {
    "claude-opus-4.5",
    "gpt-5.2-pro"
};
static const size_t FALLBACK_COMPARE_COUNT = 2;
static const char* FALLBACK_BENCHMARK = "claude-haiku-4.5";

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(content, 1, size, f);
    content[read_bytes] = '\0';
    fclose(f);

    return content;
}

static char* safe_strdup(const char* s) {
    return s ? strdup(s) : NULL;
}

static char* get_home_dir(void) {
    const char* home = getenv("HOME");
    if (home) return strdup(home);

    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) return strdup(pw->pw_dir);

    return NULL;
}

// ============================================================================
// JSON PARSING - FULL MODEL CATALOG
// ============================================================================

static bool parse_single_model(cJSON* model_json, const char* model_id,
                               const char* provider_name) {
    if (g_model_count >= MAX_TOTAL_MODELS) {
        LOG_WARN(LOG_CAT_SYSTEM, "Max models reached, skipping %s", model_id);
        return false;
    }

    LoadedModel* lm = &g_models[g_model_count];
    lm->provider_name = strdup(provider_name);

    JsonModelConfig* cfg = &lm->config;
    memset(cfg, 0, sizeof(JsonModelConfig));

    // Required: ID
    cfg->id = strdup(model_id);

    // Display name
    cJSON* display = cJSON_GetObjectItem(model_json, "display_name");
    cfg->display_name = safe_strdup(display ? display->valuestring : model_id);

    // API ID (what to send to provider API)
    cJSON* api_id = cJSON_GetObjectItem(model_json, "api_id");
    cfg->api_id = safe_strdup(api_id ? api_id->valuestring : model_id);

    // Costs
    cJSON* input_cost = cJSON_GetObjectItem(model_json, "input_cost");
    cfg->input_cost = input_cost ? input_cost->valuedouble : 0.0;

    cJSON* output_cost = cJSON_GetObjectItem(model_json, "output_cost");
    cfg->output_cost = output_cost ? output_cost->valuedouble : 0.0;

    cJSON* thinking_cost = cJSON_GetObjectItem(model_json, "thinking_cost");
    cfg->thinking_cost = thinking_cost ? thinking_cost->valuedouble : 0.0;

    // Context
    cJSON* context = cJSON_GetObjectItem(model_json, "context_window");
    cfg->context_window = context ? (size_t)context->valueint : 128000;

    cJSON* max_out = cJSON_GetObjectItem(model_json, "max_output");
    cfg->max_output = max_out ? (size_t)max_out->valueint : 8192;

    // Capabilities
    cJSON* tools = cJSON_GetObjectItem(model_json, "supports_tools");
    cfg->supports_tools = tools ? cJSON_IsTrue(tools) : true;

    cJSON* vision = cJSON_GetObjectItem(model_json, "supports_vision");
    cfg->supports_vision = vision ? cJSON_IsTrue(vision) : false;

    cJSON* streaming = cJSON_GetObjectItem(model_json, "supports_streaming");
    cfg->supports_streaming = streaming ? cJSON_IsTrue(streaming) : true;

    // Tier
    cJSON* tier = cJSON_GetObjectItem(model_json, "tier");
    cfg->tier = safe_strdup(tier ? tier->valuestring : "mid");

    // Release date
    cJSON* released = cJSON_GetObjectItem(model_json, "released");
    cfg->released = safe_strdup(released ? released->valuestring : "unknown");

    // Deprecated
    cJSON* deprecated = cJSON_GetObjectItem(model_json, "deprecated");
    cfg->deprecated = deprecated ? cJSON_IsTrue(deprecated) : false;

    g_model_count++;
    return true;
}

static bool parse_provider_models(cJSON* provider_json, const char* provider_name) {
    cJSON* models = cJSON_GetObjectItem(provider_json, "models");
    if (!models || !cJSON_IsObject(models)) {
        return false;
    }

    cJSON* model;
    size_t count = 0;
    cJSON_ArrayForEach(model, models) {
        if (parse_single_model(model, model->string, provider_name)) {
            count++;
        }
    }

    LOG_DEBUG(LOG_CAT_SYSTEM, "Loaded %zu models from provider %s", count, provider_name);
    return count > 0;
}

static bool parse_all_providers(cJSON* root) {
    cJSON* providers = cJSON_GetObjectItem(root, "providers");
    if (!providers || !cJSON_IsObject(providers)) {
        LOG_WARN(LOG_CAT_SYSTEM, "No providers found in JSON");
        return false;
    }

    cJSON* provider;
    cJSON_ArrayForEach(provider, providers) {
        parse_provider_models(provider, provider->string);
    }

    LOG_INFO(LOG_CAT_SYSTEM, "Loaded %zu total models from JSON", g_model_count);
    return g_model_count > 0;
}

static bool parse_compare_defaults(cJSON* root) {
    cJSON* compare = cJSON_GetObjectItem(root, "compare_defaults");
    if (!compare) return false;

    cJSON* models = cJSON_GetObjectItem(compare, "models");
    if (!models || !cJSON_IsArray(models)) return false;

    g_compare_count = 0;
    cJSON* model;
    cJSON_ArrayForEach(model, models) {
        if (cJSON_IsString(model) && g_compare_count < MAX_COMPARE_DEFAULTS) {
            g_compare_defaults[g_compare_count++] = strdup(model->valuestring);
        }
    }

    return g_compare_count > 0;
}

static bool parse_benchmark_defaults(cJSON* root) {
    cJSON* benchmark = cJSON_GetObjectItem(root, "benchmark_defaults");
    if (!benchmark) return false;

    cJSON* model = cJSON_GetObjectItem(benchmark, "model");
    if (model && cJSON_IsString(model)) {
        g_benchmark_model = strdup(model->valuestring);
    }

    cJSON* iterations = cJSON_GetObjectItem(benchmark, "iterations");
    if (iterations && cJSON_IsNumber(iterations)) {
        g_benchmark_iterations = (size_t)iterations->valueint;
    }

    return true;
}

static bool load_json_file(const char* path) {
    char* content = read_file(path);
    if (!content) return false;

    cJSON* root = cJSON_Parse(content);
    free(content);

    if (!root) {
        LOG_WARN(LOG_CAT_SYSTEM, "Failed to parse JSON from %s: %s",
                 path, cJSON_GetErrorPtr());
        return false;
    }

    // Get version
    cJSON* version = cJSON_GetObjectItem(root, "version");
    if (version && cJSON_IsString(version)) {
        g_version = strdup(version->valuestring);
    }

    // Parse ALL models from ALL providers
    bool has_models = parse_all_providers(root);

    // Parse compare defaults
    bool has_compare = parse_compare_defaults(root);

    // Parse benchmark defaults
    bool has_benchmark = parse_benchmark_defaults(root);

    cJSON_Delete(root);

    if (has_models || has_compare || has_benchmark) {
        g_loaded_path = strdup(path);
        g_loaded_from_json = true;
        LOG_INFO(LOG_CAT_SYSTEM, "Loaded models config from %s (version: %s, models: %zu)",
                 path, g_version ? g_version : "unknown", g_model_count);
        return true;
    }

    return false;
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool models_loader_init(void) {
    if (g_loader_initialized) return true;

    // Try loading from various paths
    char path[1024];

    // 1. User config (~/.config/convergio/models.json)
    char* home = get_home_dir();
    if (home) {
        snprintf(path, sizeof(path), "%s/.config/convergio/models.json", home);
        free(home);
        if (access(path, R_OK) == 0 && load_json_file(path)) {
            g_loader_initialized = true;
            return true;
        }
    }

    // 2. Project local (./config/models.json)
    if (access("config/models.json", R_OK) == 0 && load_json_file("config/models.json")) {
        g_loader_initialized = true;
        return true;
    }

    // 3. System (/usr/local/share/convergio/models.json)
    if (access("/usr/local/share/convergio/models.json", R_OK) == 0 &&
        load_json_file("/usr/local/share/convergio/models.json")) {
        g_loader_initialized = true;
        return true;
    }

    // 4. Use fallback defaults
    LOG_WARN(LOG_CAT_SYSTEM, "No models.json found, using built-in defaults");
    g_loader_initialized = true;
    g_loaded_from_json = false;
    return true;
}

const char** models_get_compare_defaults(size_t* count) {
    if (!g_loader_initialized) models_loader_init();

    if (g_loaded_from_json && g_compare_count > 0) {
        if (count) *count = g_compare_count;
        return (const char**)g_compare_defaults;
    }

    // Fallback
    if (count) *count = FALLBACK_COMPARE_COUNT;
    return FALLBACK_COMPARE;
}

const char* models_get_benchmark_default(void) {
    if (!g_loader_initialized) models_loader_init();

    if (g_loaded_from_json && g_benchmark_model) {
        return g_benchmark_model;
    }
    return FALLBACK_BENCHMARK;
}

size_t models_get_benchmark_iterations(void) {
    if (!g_loader_initialized) models_loader_init();
    return g_benchmark_iterations;
}

bool models_loaded_from_json(void) {
    return g_loaded_from_json;
}

const char* models_get_loaded_path(void) {
    return g_loaded_path;
}

const char* models_get_version(void) {
    return g_version;
}

const JsonModelConfig* models_get_json_model(const char* model_id) {
    if (!model_id) return NULL;
    if (!g_loader_initialized) models_loader_init();

    for (size_t i = 0; i < g_model_count; i++) {
        if (strcmp(g_models[i].config.id, model_id) == 0) {
            return &g_models[i].config;
        }
    }
    return NULL;
}

const char* models_get_model_provider(const char* model_id) {
    if (!model_id) return NULL;
    if (!g_loader_initialized) models_loader_init();

    for (size_t i = 0; i < g_model_count; i++) {
        if (strcmp(g_models[i].config.id, model_id) == 0) {
            return g_models[i].provider_name;
        }
    }
    return NULL;
}

size_t models_get_loaded_count(void) {
    if (!g_loader_initialized) models_loader_init();
    return g_model_count;
}

bool models_loader_reload(void) {
    // Cleanup existing
    models_loader_shutdown();
    g_loader_initialized = false;

    // Reload
    return models_loader_init();
}

void models_loader_shutdown(void) {
    // Free all loaded models
    for (size_t i = 0; i < g_model_count; i++) {
        free(g_models[i].provider_name);
        free(g_models[i].config.id);
        free(g_models[i].config.display_name);
        free(g_models[i].config.api_id);
        free(g_models[i].config.tier);
        free(g_models[i].config.released);
    }
    g_model_count = 0;

    // Free compare defaults
    for (size_t i = 0; i < g_compare_count; i++) {
        free(g_compare_defaults[i]);
        g_compare_defaults[i] = NULL;
    }
    g_compare_count = 0;

    // Free benchmark model
    if (g_benchmark_model) {
        free(g_benchmark_model);
        g_benchmark_model = NULL;
    }

    // Free path and version
    if (g_loaded_path) {
        free(g_loaded_path);
        g_loaded_path = NULL;
    }
    if (g_version) {
        free(g_version);
        g_version = NULL;
    }

    g_loaded_from_json = false;
    g_loader_initialized = false;
}
