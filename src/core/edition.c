/**
 * CONVERGIO KERNEL - EDITION SYSTEM
 *
 * Implements verticalization for multiple Convergio editions.
 * Supports runtime switching between Master, Business, and Developer editions.
 * Education edition is compile-time locked for child safety.
 *
 * Build with: make EDITION=education (or business, developer)
 *
 * Copyright (c) 2025 Convergio.io
 * Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "nous/edition.h"
#include "nous/nous.h"
#include <string.h>
#include <stdio.h>

// Version is passed as -DCONVERGIO_VERSION from Makefile
#ifndef CONVERGIO_VERSION
#define CONVERGIO_VERSION "0.0.0"
#endif

// ============================================================================
// RUNTIME EDITION STATE
// ============================================================================

// Education binary is locked at compile time
#ifdef CONVERGIO_EDITION_EDUCATION
static const bool g_edition_locked = true;
static ConvergioEdition g_current_edition = EDITION_EDUCATION;
#else
static const bool g_edition_locked = false;
static ConvergioEdition g_current_edition = EDITION_MASTER;
#endif

// Track if edition was set via CLI (takes priority over config/env)
static bool g_edition_set_by_cli = false;

// ============================================================================
// EDUCATION EDITION WHITELIST
// ============================================================================

static const char *EDUCATION_AGENTS[] = {
    // 17 Maestri (teaching agents)
    "euclide-matematica",
    "feynman-fisica",
    "manzoni-italiano",
    "darwin-scienze",
    "erodoto-storia",
    "humboldt-geografia",
    "leonardo-arte",
    "shakespeare-inglese",
    "mozart-musica",
    "cicerone-civica",
    "smith-economia",
    "lovelace-informatica",
    "ippocrate-corpo",
    "socrate-filosofia",
    "chris-storytelling",
    "curie-chimica",
    "galileo-astronomia",
    // Coordination
    "ali-principal",
    "anna-executive-assistant",
    "jenny-inclusive-accessibility-champion",
    NULL
};

static const char *EDUCATION_FEATURES[] = {
    "quiz", "flashcards", "mindmap", "study-session", "homework",
    "libretto", "audio-tts", "html-interactive", "calculator",
    "voice", "accessibility",
    NULL
};

static const char *EDUCATION_COMMANDS[] = {
    "education", "study", "homework", "quiz", "flashcards",
    "mindmap", "libretto", "voice", "html", "calc",
    "define", "conjugate", "pronounce", "grammar",
    "xp", "video", "periodic", "convert",
    "help", "agent", "agents", "status", "quit", "exit",
    "setup", "debug", "theme", "style", "cost",
    "todo", "remind", "reminders",
    NULL
};

// ============================================================================
// BUSINESS EDITION WHITELIST
// ============================================================================

static const char *BUSINESS_AGENTS[] = {
    "ali-chief-of-staff",
    "fabio-sales-business-development",
    "andrea-customer-success-manager",
    "sofia-marketing-strategist",
    "anna-executive-assistant",
    "fiona-market-analyst",
    "matteo-strategic-business-architect",
    "amy-cfo",
    "michael-vc",
    "wiz-investor-venture-capital",
    NULL
};

static const char *BUSINESS_FEATURES[] = {
    "crm", "pipeline", "analytics", "reports", "finance",
    NULL
};

static const char *BUSINESS_COMMANDS[] = {
    "help", "agent", "agents", "status", "quit", "exit",
    "setup", "debug", "theme", "style", "cost",
    "todo", "remind", "reminders",
    NULL
};

// ============================================================================
// DEVELOPER EDITION WHITELIST
// ============================================================================

static const char *DEVELOPER_AGENTS[] = {
    "ali-chief-of-staff",
    "anna-executive-assistant",
    "rex-code-reviewer",
    "paolo-best-practices-enforcer",
    "baccio-tech-architect",
    "dario-debugger",
    "otto-performance-optimizer",
    "marco-devops-engineer",
    "luca-security-expert",
    "guardian-ai-security-validator",
    NULL
};

static const char *DEVELOPER_FEATURES[] = {
    "code-review", "architecture", "debugging", "security",
    "ci-cd", "performance", "best-practices",
    NULL
};

static const char *DEVELOPER_COMMANDS[] = {
    "help", "agent", "agents", "status", "quit", "exit",
    "setup", "debug", "theme", "style", "cost",
    "todo", "remind", "reminders",
    "test", "git", "pr", "commit",
    NULL
};

// ============================================================================
// EDITION DEFINITIONS
// ============================================================================

static const EditionInfo EDITIONS[] = {
    // EDITION_MASTER - all agents
    {
        .id = EDITION_MASTER,
        .name = "Convergio",
        .short_name = "Master",
        .version_suffix = "",
        .description = "Complete Convergio with all agents and features",
        .target_audience = "Developers and power users",
        .allowed_agents = NULL,  // NULL = all agents
        .allowed_features = NULL,
        .allowed_commands = NULL,
    },
    // EDITION_EDUCATION
    {
        .id = EDITION_EDUCATION,
        .name = "Convergio Education",
        .short_name = "Education",
        .version_suffix = "-edu",
        .description = "Virtual classroom with historical maestri",
        .target_audience = "Students 6-19, parents, teachers",
        .allowed_agents = EDUCATION_AGENTS,
        .allowed_features = EDUCATION_FEATURES,
        .allowed_commands = EDUCATION_COMMANDS,
    },
    // EDITION_BUSINESS
    {
        .id = EDITION_BUSINESS,
        .name = "Convergio Business",
        .short_name = "Business",
        .version_suffix = "-biz",
        .description = "Business productivity and sales tools",
        .target_audience = "SMBs, startups, sales teams",
        .allowed_agents = BUSINESS_AGENTS,
        .allowed_features = BUSINESS_FEATURES,
        .allowed_commands = BUSINESS_COMMANDS,
    },
    // EDITION_DEVELOPER
    {
        .id = EDITION_DEVELOPER,
        .name = "Convergio Developer",
        .short_name = "Developer",
        .version_suffix = "-dev",
        .description = "Code review, architecture, and DevOps tools",
        .target_audience = "Developers, DevOps, Tech Leads",
        .allowed_agents = DEVELOPER_AGENTS,
        .allowed_features = DEVELOPER_FEATURES,
        .allowed_commands = DEVELOPER_COMMANDS,
    },
};

// ============================================================================
// SYSTEM PROMPTS PER EDITION
// ============================================================================

static const char *EDUCATION_SYSTEM_PROMPT =
    "You are part of Convergio Education, a virtual classroom with the greatest "
    "teachers in history. Your role is to help students learn through the Socratic "
    "method, encouraging curiosity and understanding rather than just giving answers. "
    "Always adapt to the student's level and accessibility needs.";

static const char *BUSINESS_SYSTEM_PROMPT =
    "You are part of Convergio Business, a professional productivity suite. "
    "Focus on actionable insights, data-driven decisions, and business outcomes. "
    "Be concise and professional.";

static const char *DEVELOPER_SYSTEM_PROMPT =
    "You are part of Convergio Developer, a code assistant for professional developers. "
    "Focus on code quality, best practices, and architectural decisions. "
    "Be precise and technical.";

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

ConvergioEdition edition_current(void) {
    return g_current_edition;
}

bool edition_is_mutable(void) {
    return !g_edition_locked;
}

bool edition_set(ConvergioEdition edition) {
    // Education binary cannot change edition
    if (g_edition_locked) {
        LOG_WARN(LOG_CAT_SYSTEM, "[Edition] Cannot change edition: binary is locked to Education");
        return false;
    }

    // Cannot switch TO education at runtime (security)
    if (edition == EDITION_EDUCATION) {
        LOG_WARN(LOG_CAT_SYSTEM, "[Edition] Cannot switch to Education at runtime");
        return false;
    }

    // Validate edition
    if (edition < EDITION_MASTER || edition > EDITION_DEVELOPER) {
        LOG_WARN(LOG_CAT_SYSTEM, "[Edition] Invalid edition: %d", edition);
        return false;
    }

    // No change needed
    if (edition == g_current_edition) {
        return true;
    }

    g_current_edition = edition;
    LOG_INFO(LOG_CAT_SYSTEM, "[Edition] Switched to %s", EDITIONS[edition].name);
    return true;
}

bool edition_set_by_name(const char *name) {
    if (!name) {
        LOG_WARN(LOG_CAT_SYSTEM, "[Edition] Edition name cannot be NULL");
        return false;
    }

    // Check for valid edition name (don't silently default to master)
    if (strcmp(name, "master") == 0 || strcmp(name, "full") == 0) {
        return edition_set(EDITION_MASTER);
    } else if (strcmp(name, "education") == 0 || strcmp(name, "edu") == 0) {
        return edition_set(EDITION_EDUCATION);
    } else if (strcmp(name, "business") == 0 || strcmp(name, "biz") == 0) {
        return edition_set(EDITION_BUSINESS);
    } else if (strcmp(name, "developer") == 0 || strcmp(name, "dev") == 0) {
        return edition_set(EDITION_DEVELOPER);
    }

    LOG_WARN(LOG_CAT_SYSTEM, "[Edition] Unknown edition name: '%s'", name);
    return false;
}

bool edition_set_by_cli(const char *name) {
    if (edition_set_by_name(name)) {
        g_edition_set_by_cli = true;
        return true;
    }
    return false;
}

bool edition_was_set_by_cli(void) {
    return g_edition_set_by_cli;
}

ConvergioEdition edition_from_name(const char *name) {
    if (!name) return EDITION_MASTER;

    // "full" is an alias for master (primarily for internal use)
    if (strcmp(name, "master") == 0 || strcmp(name, "full") == 0) {
        return EDITION_MASTER;
    } else if (strcmp(name, "education") == 0 || strcmp(name, "edu") == 0) {
        return EDITION_EDUCATION;
    } else if (strcmp(name, "business") == 0 || strcmp(name, "biz") == 0) {
        return EDITION_BUSINESS;
    } else if (strcmp(name, "developer") == 0 || strcmp(name, "dev") == 0) {
        return EDITION_DEVELOPER;
    }

    return EDITION_MASTER;
}

const char *edition_get_name(ConvergioEdition edition) {
    switch (edition) {
        case EDITION_MASTER: return "master";
        case EDITION_EDUCATION: return "education";
        case EDITION_BUSINESS: return "business";
        case EDITION_DEVELOPER: return "developer";
        default: return "master";
    }
}

const EditionInfo *edition_get_info(ConvergioEdition edition) {
    if (edition >= 0 && edition <= EDITION_DEVELOPER) {
        return &EDITIONS[edition];
    }
    return &EDITIONS[EDITION_MASTER];
}

const EditionInfo *edition_get_current_info(void) {
    return edition_get_info(g_current_edition);
}

static bool string_in_list(const char *str, const char **list) {
    // NULL list = allow all (master edition)
    if (!list) return true;
    // NULL str = not allowed (invalid input)
    if (!str) return false;

    for (const char **p = list; *p != NULL; p++) {
        if (strcmp(str, *p) == 0) return true;
    }
    return false;
}

bool edition_has_agent(const char *agent_id) {
    const EditionInfo *info = edition_get_current_info();
    return string_in_list(agent_id, info->allowed_agents);
}

bool edition_has_feature(const char *feature_id) {
    const EditionInfo *info = edition_get_current_info();
    return string_in_list(feature_id, info->allowed_features);
}

bool edition_has_command(const char *command) {
    const EditionInfo *info = edition_get_current_info();
    return string_in_list(command, info->allowed_commands);
}

const char *edition_display_name(void) {
    return edition_get_current_info()->name;
}

const char *edition_system_prompt(void) {
    switch (g_current_edition) {
        case EDITION_EDUCATION:
            return EDUCATION_SYSTEM_PROMPT;
        case EDITION_BUSINESS:
            return BUSINESS_SYSTEM_PROMPT;
        case EDITION_DEVELOPER:
            return DEVELOPER_SYSTEM_PROMPT;
        default:
            return "";
    }
}

void edition_init(void) {
    const EditionInfo *info = edition_get_current_info();
    if (g_current_edition != EDITION_MASTER) {
        LOG_INFO(LOG_CAT_SYSTEM, "[Edition] %s v%s%s",
                 info->name,
                 CONVERGIO_VERSION,
                 info->version_suffix);
    }
}

// ============================================================================
// EDITION-SPECIFIC PROVIDER CONFIGURATION
// ============================================================================

/**
 * Get the preferred LLM provider for the current edition.
 * Education edition uses Azure OpenAI exclusively for GDPR compliance
 * and built-in content safety filters.
 *
 * @return Preferred provider type (PROVIDER_OPENAI for Azure, etc.)
 */
int edition_get_preferred_provider(void) {
    switch (g_current_edition) {
        case EDITION_EDUCATION:
            // Education uses Azure OpenAI exclusively (GDPR, content safety)
            return 1;  // PROVIDER_OPENAI
        case EDITION_BUSINESS:
            // Business prefers Claude for complex reasoning
            return 0;  // PROVIDER_ANTHROPIC
        case EDITION_DEVELOPER:
            // Developer prefers Claude for code generation
            return 0;  // PROVIDER_ANTHROPIC
        default:
            // Master uses best available
            return 0;  // PROVIDER_ANTHROPIC
    }
}

/**
 * Check if current edition uses Azure OpenAI (Education edition only)
 * @return true if Education edition, false otherwise
 */
bool edition_uses_azure_openai(void) {
    return g_current_edition == EDITION_EDUCATION;
}

/**
 * Get the preferred model for the current edition.
 * Returns a model ID string that should be used.
 */
const char* edition_get_preferred_model(void) {
    switch (g_current_edition) {
        case EDITION_EDUCATION:
            // Azure OpenAI GPT-5-Mini for education (cost-effective, good safety)
            // Use gpt-5.2-edu for complex queries, gpt-5-nano for fast responses
            return "gpt-5-edu-mini";
        case EDITION_BUSINESS:
            return "claude-sonnet-4";
        case EDITION_DEVELOPER:
            return "claude-sonnet-4";
        default:
            return "claude-opus-4";
    }
}
