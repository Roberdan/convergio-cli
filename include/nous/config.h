/**
 * CONVERGIO CONFIGURATION
 *
 * User configuration management with TOML parsing
 * Supports ~/.convergio/ directory structure
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_CONFIG_H
#define CONVERGIO_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// CONFIGURATION STRUCTURE
// ============================================================================

typedef struct {
    // API
    char anthropic_api_key[256];

    // Budget
    double budget_limit;
    int budget_warn_percent;

    // UI
    bool color_enabled;
    char debug_level[16];  // none, error, warn, info, debug, trace
    char theme[32];        // Theme name (Ocean, Forest, Sunset, etc.)
    char style[16];        // Response style: flash, concise, balanced, detailed
    char edition[32];      // Edition: master, business, developer (education is compile-time only)

    // Updates
    bool check_updates_on_startup;
    bool auto_update;

    // Paths (calculated at init)
    char config_dir[512];     // ~/.convergio
    char config_file[512];    // ~/.convergio/config.toml
    char db_path[512];        // ~/.convergio/convergio.db
    char notes_dir[512];      // ~/.convergio/notes
    char knowledge_dir[512];  // ~/.convergio/knowledge
    char cache_dir[512];      // ~/.convergio/cache

    // State
    bool initialized;

} ConvergioConfig;

// ============================================================================
// GLOBAL CONFIG
// ============================================================================

extern ConvergioConfig g_config;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize configuration system
 * Creates ~/.convergio directory structure if needed
 * Loads config from file or creates default
 * Returns 0 on success, -1 on failure
 */
int convergio_config_init(void);

/**
 * Shutdown configuration system
 * Saves any pending changes
 */
void convergio_config_shutdown(void);

// ============================================================================
// CONFIG FILE OPERATIONS
// ============================================================================

/**
 * Load configuration from file
 * Returns 0 on success, -1 on failure
 */
int convergio_config_load(void);

/**
 * Save configuration to file
 * Returns 0 on success, -1 on failure
 */
int convergio_config_save(void);

/**
 * Reset configuration to defaults
 */
void convergio_config_reset(void);

// ============================================================================
// CONFIG ACCESSORS
// ============================================================================

/**
 * Get config value by key (returns NULL if not found)
 * Supported keys: api_key, budget_limit, debug_level, etc.
 */
const char* convergio_config_get(const char* key);

/**
 * Set config value by key
 * Returns 0 on success, -1 on invalid key
 */
int convergio_config_set(const char* key, const char* value);

// ============================================================================
// API KEY MANAGEMENT
// ============================================================================

/**
 * Get API key with fallback chain:
 * 1. Environment variable ANTHROPIC_API_KEY
 * 2. macOS Keychain
 * 3. Config file
 * Returns NULL if not found anywhere
 */
const char* convergio_get_api_key(void);

/**
 * Store API key in macOS Keychain
 * Returns 0 on success, -1 on failure
 */
int convergio_store_api_key(const char* key);

/**
 * Delete API key from Keychain
 * Returns 0 on success, -1 on failure
 */
int convergio_delete_api_key(void);

// ============================================================================
// SETUP WIZARD
// ============================================================================

/**
 * Run interactive setup wizard
 * Prompts for API key and basic configuration
 * Returns 0 on success, -1 on failure/cancel
 */
int convergio_setup_wizard(void);

/**
 * Check if setup has been completed
 */
bool convergio_setup_complete(void);

// ============================================================================
// RESPONSE STYLE SETTINGS
// ============================================================================

/**
 * Style settings for LLM responses
 */
typedef struct {
    int max_tokens;      // Maximum output tokens
    double temperature;  // Generation temperature
    bool markdown;       // Allow markdown formatting
} StyleSettings;

/**
 * Get current style settings based on /style configuration
 * Returns settings struct with max_tokens, temperature, markdown
 */
StyleSettings convergio_get_style_settings(void);

/**
 * Get style name (flash, concise, balanced, detailed)
 */
const char* convergio_get_style_name(void);

#endif // CONVERGIO_CONFIG_H
