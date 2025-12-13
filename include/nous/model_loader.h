/**
 * MODEL LOADER - Dynamic model configuration from JSON
 *
 * Loads model configurations from config/models.json
 * Allows runtime updates without recompilation
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef NOUS_MODEL_LOADER_H
#define NOUS_MODEL_LOADER_H

#include <stdbool.h>
#include <stddef.h>

// Maximum models per provider
#define MAX_MODELS_PER_PROVIDER 32
#define MAX_COMPARE_DEFAULTS 5

// Model configuration loaded from JSON
typedef struct {
    char* id;
    char* display_name;
    char* api_id;
    double input_cost;
    double output_cost;
    double thinking_cost;
    size_t context_window;
    size_t max_output;
    bool supports_tools;
    bool supports_vision;
    bool supports_streaming;
    char* tier;  // "premium", "mid", "cheap"
    char* released;
    bool deprecated;
} JsonModelConfig;

// Provider configuration loaded from JSON
typedef struct {
    char* name;
    char* api_key_env;
    char* base_url;
    JsonModelConfig models[MAX_MODELS_PER_PROVIDER];
    size_t model_count;
} JsonProviderConfig;

// Compare defaults loaded from JSON
typedef struct {
    char* models[MAX_COMPARE_DEFAULTS];
    size_t count;
    char* description;
    char* rationale;
} CompareDefaults;

// Benchmark defaults
typedef struct {
    char* model;
    size_t iterations;
} BenchmarkDefaults;

/**
 * Initialize model loader and load from JSON
 * Searches for config in:
 *   1. ~/.config/convergio/models.json (user override)
 *   2. ./config/models.json (project local)
 *   3. /usr/local/share/convergio/models.json (system)
 *
 * Returns true if successfully loaded, false otherwise
 */
bool models_loader_init(void);

/**
 * Get compare default models
 * Returns array of model IDs to use for compare command
 */
const char** models_get_compare_defaults(size_t* count);

/**
 * Get benchmark default model
 */
const char* models_get_benchmark_default(void);

/**
 * Get benchmark default iterations
 */
size_t models_get_benchmark_iterations(void);

/**
 * Check if models were loaded from JSON (vs using hardcoded fallbacks)
 */
bool models_loaded_from_json(void);

/**
 * Get the path of the loaded JSON file (or NULL if using fallbacks)
 */
const char* models_get_loaded_path(void);

/**
 * Cleanup and free model loader resources
 */
void models_loader_shutdown(void);

/**
 * Reload models from JSON (hot reload)
 */
bool models_loader_reload(void);

/**
 * Get JSON file version string
 */
const char* models_get_version(void);

/**
 * Get model configuration by ID from JSON
 * Returns NULL if model not found
 * The returned config is owned by the loader - do not free
 */
const JsonModelConfig* models_get_json_model(const char* model_id);

/**
 * Get provider name for a model ID from JSON
 * Returns NULL if model not found
 */
const char* models_get_model_provider(const char* model_id);

/**
 * Get number of loaded models
 */
size_t models_get_loaded_count(void);

#endif // NOUS_MODEL_LOADER_H
