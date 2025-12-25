/**
 * CONVERGIO CONFIG ORCHESTRATOR (REF-03)
 *
 * Unified configuration loading with priority:
 * 1. Defaults (compiled-in)
 * 2. Config file (TOML)
 * 3. Environment variables (override)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef NOUS_CONFIG_ORCHESTRATOR_H
#define NOUS_CONFIG_ORCHESTRATOR_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// CONFIG SOURCE PRIORITY
// ============================================================================

typedef enum {
    CONFIG_SOURCE_DEFAULT = 0,    // Compiled-in defaults
    CONFIG_SOURCE_FILE,           // TOML config file
    CONFIG_SOURCE_ENV,            // Environment variable
    CONFIG_SOURCE_CLI,            // Command line argument
    CONFIG_SOURCE_RUNTIME         // Runtime override
} ConfigSource;

// ============================================================================
// CONFIG VALUE TYPES
// ============================================================================

typedef enum {
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_INT,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_DOUBLE
} ConfigValueType;

typedef struct {
    ConfigValueType type;
    ConfigSource source;
    union {
        const char* str_val;
        int int_val;
        bool bool_val;
        double double_val;
    } value;
} ConfigValue;

// ============================================================================
// CONFIG ORCHESTRATOR API
// ============================================================================

/**
 * Initialize config orchestrator
 * Loads in order: defaults -> file -> env
 */
int config_orchestrator_init(void);

/**
 * Shutdown and free resources
 */
void config_orchestrator_shutdown(void);

/**
 * Get string config value
 */
const char* config_get_string(const char* key, const char* default_val);

/**
 * Get int config value
 */
int config_get_int(const char* key, int default_val);

/**
 * Get bool config value
 */
bool config_get_bool(const char* key, bool default_val);

/**
 * Get double config value
 */
double config_get_double(const char* key, double default_val);

/**
 * Set runtime override
 */
void config_set_override(const char* key, const char* value);

/**
 * Get source of a config value
 */
ConfigSource config_get_source(const char* key);

/**
 * Reload config from file (preserves runtime overrides)
 */
int config_reload(void);

/**
 * Dump all config for debugging
 */
void config_dump(void);

// ============================================================================
// WELL-KNOWN CONFIG KEYS
// ============================================================================

#define CONFIG_KEY_API_KEY          "api_key"
#define CONFIG_KEY_MODEL            "model"
#define CONFIG_KEY_TEMPERATURE      "temperature"
#define CONFIG_KEY_MAX_TOKENS       "max_tokens"
#define CONFIG_KEY_EDITION          "edition"
#define CONFIG_KEY_THEME            "theme"
#define CONFIG_KEY_LOG_LEVEL        "log_level"
#define CONFIG_KEY_TELEMETRY        "telemetry_enabled"
#define CONFIG_KEY_DATA_DIR         "data_dir"
#define CONFIG_KEY_CACHE_DIR        "cache_dir"

#endif // NOUS_CONFIG_ORCHESTRATOR_H
