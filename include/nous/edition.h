/**
 * CONVERGIO KERNEL - EDITION SYSTEM
 *
 * Verticalization support for multiple Convergio editions.
 * Each edition has specific agents, features, and branding.
 *
 * Runtime switching supported for Master/Business/Developer.
 * Education edition is compile-time locked for child safety.
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef NOUS_EDITION_H
#define NOUS_EDITION_H

#include <stdbool.h>

// ============================================================================
// EDITION TYPES
// ============================================================================

typedef enum {
    EDITION_MASTER = 0,      // ALL agents - the complete Convergio experience
    EDITION_EDUCATION = 1,   // Maestri + Education tools + Ali (compile-time only)
    EDITION_BUSINESS = 2,    // Business agents + Ali
    EDITION_DEVELOPER = 3    // Developer agents + Ali
} ConvergioEdition;

// Backwards compatibility
#define EDITION_FULL EDITION_MASTER

// ============================================================================
// EDITION INFO
// ============================================================================

typedef struct {
    ConvergioEdition id;
    const char *name;           // "Convergio Education"
    const char *short_name;     // "Education"
    const char *version_suffix; // "-edu"
    const char *description;
    const char *target_audience;

    // Agent whitelist (NULL-terminated array of agent IDs)
    const char **allowed_agents;

    // Feature whitelist (NULL-terminated array of feature IDs)
    const char **allowed_features;

    // CLI commands whitelist (NULL-terminated array)
    const char **allowed_commands;

} EditionInfo;

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * Get current edition
 */
ConvergioEdition edition_current(void);

/**
 * Set edition at runtime
 * Returns true on success, false if:
 * - Edition is locked (Education binary)
 * - Trying to switch TO Education (not allowed at runtime)
 */
bool edition_set(ConvergioEdition edition);

/**
 * Set edition by name string
 * Valid names: "master", "business", "developer"
 * Returns true on success, false on failure
 */
bool edition_set_by_name(const char *name);

/**
 * Set edition by CLI flag (marks as CLI priority)
 * Use this when parsing --edition command line argument
 * CLI takes priority over env var and config
 */
bool edition_set_by_cli(const char *name);

/**
 * Check if edition was set via CLI flag
 * Config and env var should skip setting if this returns true
 */
bool edition_was_set_by_cli(void);

/**
 * Check if edition can be changed at runtime
 * Returns false for Education binary
 */
bool edition_is_mutable(void);

/**
 * Get edition info for current or specified edition
 */
const EditionInfo *edition_get_info(ConvergioEdition edition);
const EditionInfo *edition_get_current_info(void);

/**
 * Check if agent is available in current edition
 */
bool edition_has_agent(const char *agent_id);

/**
 * Check if feature is available in current edition
 */
bool edition_has_feature(const char *feature_id);

/**
 * Check if command is available in current edition
 */
bool edition_has_command(const char *command);

/**
 * Get edition display name for UI
 */
const char *edition_display_name(void);

/**
 * Get edition-specific system prompt prefix
 */
const char *edition_system_prompt(void);

/**
 * Initialize edition system
 * Call after config is loaded to apply saved edition
 */
void edition_init(void);

/**
 * Get edition name for config/display
 */
const char *edition_get_name(ConvergioEdition edition);

/**
 * Parse edition from name string
 * Returns EDITION_MASTER if not recognized
 */
ConvergioEdition edition_from_name(const char *name);

// ============================================================================
// EDITION-SPECIFIC PROVIDER CONFIGURATION
// ============================================================================

/**
 * Get the preferred LLM provider for the current edition.
 * Education: Azure OpenAI (GDPR, content safety)
 * Business: Anthropic Claude
 * Developer: Anthropic Claude
 * Master: Best available
 *
 * @return Provider type (0=Anthropic, 1=OpenAI, etc.)
 */
int edition_get_preferred_provider(void);

/**
 * Get the preferred model for the current edition.
 * @return Model ID string
 */
const char* edition_get_preferred_model(void);

/**
 * Check if the current edition uses Azure OpenAI.
 * Only true for Education edition.
 */
bool edition_uses_azure_openai(void);

#endif /* NOUS_EDITION_H */
