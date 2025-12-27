/**
 * CONVERGIO CONFIGURATION
 *
 * User configuration management with simple TOML parsing
 * Manages ~/.convergio/ directory structure
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/config.h"
#include "nous/edition.h"
#include "nous/nous.h"
#include "nous/safe_path.h"
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// ============================================================================
// GLOBAL CONFIG
// ============================================================================

ConvergioConfig g_config = {0};

// ============================================================================
// EXTERNAL KEYCHAIN FUNCTIONS (implemented in keychain.m)
// ============================================================================

extern int convergio_keychain_store(const char* service, const char* account, const char* password);
extern char* convergio_keychain_read(const char* service, const char* account);
extern int convergio_keychain_delete(const char* service, const char* account);

// FIX: Dynamic keychain service per edition to avoid conflicts
static const char* get_keychain_service(void) {
    ConvergioEdition edition = edition_current();
    switch (edition) {
    case EDITION_EDUCATION:
        return "com.fightthestroke.convergio-edu";
    case EDITION_BUSINESS:
        return "com.fightthestroke.convergio-biz";
    case EDITION_DEVELOPER:
        return "com.fightthestroke.convergio-dev";
    default:
        return "com.fightthestroke.convergio";
    }
}
#define KEYCHAIN_SERVICE get_keychain_service()
#define KEYCHAIN_ACCOUNT "api_key"

// ============================================================================
// PATH HELPERS
// ============================================================================

static const char* get_home_dir(void) {
    const char* home = getenv("HOME");
    if (home && strlen(home) > 0) {
        return home;
    }

    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return pw->pw_dir;
    }

    return "/tmp"; // Last resort fallback
}

static int ensure_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }

    if (mkdir(path, 0700) == 0) {
        return 0;
    }

    return -1;
}

// ============================================================================
// TOML PARSING (minimal implementation)
// ============================================================================

static void trim_whitespace(char* str) {
    if (!str)
        return;

    // Trim leading
    char* start = str;
    while (*start == ' ' || *start == '\t')
        start++;

    // Trim trailing
    char* end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }

    // Shift if needed
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

static void remove_quotes(char* str) {
    if (!str)
        return;
    size_t len = strlen(str);
    if (len >= 2 &&
        ((str[0] == '"' && str[len - 1] == '"') || (str[0] == '\'' && str[len - 1] == '\''))) {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

static int parse_config_line(const char* line, char* section, size_t section_size) {
    char buf[512];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim_whitespace(buf);

    // Skip empty lines and comments
    if (buf[0] == '\0' || buf[0] == '#') {
        return 0;
    }

    // Section header
    if (buf[0] == '[') {
        char* end = strchr(buf, ']');
        if (end) {
            *end = '\0';
            strncpy(section, buf + 1, section_size - 1);
            section[section_size - 1] = '\0';
        }
        return 0;
    }

    // Key = value
    char* eq = strchr(buf, '=');
    if (!eq)
        return 0;

    *eq = '\0';
    char* key = buf;
    char* value = eq + 1;
    trim_whitespace(key);
    trim_whitespace(value);
    remove_quotes(value);

    // Handle based on section and key
    if (strcmp(section, "api") == 0) {
        if (strcmp(key, "anthropic_key") == 0) {
            strncpy(g_config.anthropic_api_key, value, sizeof(g_config.anthropic_api_key) - 1);
        }
    } else if (strcmp(section, "budget") == 0) {
        if (strcmp(key, "default_limit") == 0) {
            g_config.budget_limit = atof(value);
        } else if (strcmp(key, "warn_at_percent") == 0) {
            g_config.budget_warn_percent = atoi(value);
        }
    } else if (strcmp(section, "ui") == 0) {
        if (strcmp(key, "color") == 0) {
            g_config.color_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "debug_level") == 0) {
            strncpy(g_config.debug_level, value, sizeof(g_config.debug_level) - 1);
        } else if (strcmp(key, "theme") == 0) {
            strncpy(g_config.theme, value, sizeof(g_config.theme) - 1);
        } else if (strcmp(key, "style") == 0) {
            strncpy(g_config.style, value, sizeof(g_config.style) - 1);
        } else if (strcmp(key, "edition") == 0) {
            strncpy(g_config.edition, value, sizeof(g_config.edition) - 1);
        }
    } else if (strcmp(section, "updates") == 0) {
        if (strcmp(key, "check_on_startup") == 0) {
            g_config.check_updates_on_startup =
                (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        } else if (strcmp(key, "auto_update") == 0) {
            g_config.auto_update = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        }
    }

    return 0;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

static void set_defaults(void) {
    memset(&g_config, 0, sizeof(g_config));

    g_config.budget_limit = 5.00;
    g_config.budget_warn_percent = 80;
    g_config.color_enabled = true;
    strncpy(g_config.debug_level, "none", sizeof(g_config.debug_level) - 1);
    strncpy(g_config.theme, "Ocean", sizeof(g_config.theme) - 1);      // Default theme
    strncpy(g_config.style, "balanced", sizeof(g_config.style) - 1);   // Default style
    strncpy(g_config.edition, "master", sizeof(g_config.edition) - 1); // Default edition
    g_config.check_updates_on_startup = true;
    g_config.auto_update = false;
}

static int setup_paths(void) {
    const char* home = get_home_dir();

    // FIX: Separate config directories per edition to avoid conflicts
    // Master uses ~/.convergio/, others use ~/.convergio-{edition}/
    ConvergioEdition edition = edition_current();
    const char* edition_suffix = "";
    switch (edition) {
    case EDITION_EDUCATION:
        edition_suffix = "-edu";
        break;
    case EDITION_BUSINESS:
        edition_suffix = "-biz";
        break;
    case EDITION_DEVELOPER:
        edition_suffix = "-dev";
        break;
    default:
        edition_suffix = "";
        break;
    }

    snprintf(g_config.config_dir, sizeof(g_config.config_dir), "%s/.convergio%s", home,
             edition_suffix);
    snprintf(g_config.config_file, sizeof(g_config.config_file), "%s/config.toml",
             g_config.config_dir);
    snprintf(g_config.db_path, sizeof(g_config.db_path), "%s/convergio.db", g_config.config_dir);
    snprintf(g_config.notes_dir, sizeof(g_config.notes_dir), "%s/notes", g_config.config_dir);
    snprintf(g_config.knowledge_dir, sizeof(g_config.knowledge_dir), "%s/knowledge",
             g_config.config_dir);
    snprintf(g_config.cache_dir, sizeof(g_config.cache_dir), "%s/cache", g_config.config_dir);

    return 0;
}

static int create_directories(void) {
    if (ensure_directory(g_config.config_dir) != 0) {
        return -1;
    }
    if (ensure_directory(g_config.notes_dir) != 0) {
        return -1;
    }
    if (ensure_directory(g_config.knowledge_dir) != 0) {
        return -1;
    }
    if (ensure_directory(g_config.cache_dir) != 0) {
        return -1;
    }
    return 0;
}

int convergio_config_init(void) {
    if (g_config.initialized) {
        return 0;
    }

    set_defaults();
    setup_paths();

    // Create directories if they don't exist
    if (create_directories() != 0) {
        LOG_WARN(LOG_CAT_SYSTEM, "Could not create config directories");
    }

    // Load config file if it exists
    convergio_config_load();

    // Apply edition from config/env (priority: CLI > env > config)
    // Skip if already set by CLI flag (CLI has highest priority)
    if (!edition_was_set_by_cli() && edition_is_mutable()) {
        // Check environment variable first (higher priority than config)
        const char* env_edition = getenv("CONVERGIO_EDITION");
        if (env_edition && strlen(env_edition) > 0) {
            if (!edition_set_by_name(env_edition)) {
                LOG_WARN(LOG_CAT_SYSTEM,
                         "Invalid edition '%s' in CONVERGIO_EDITION env var; using default",
                         env_edition);
            }
        } else if (g_config.edition[0]) {
            // Fall back to config file setting
            if (!edition_set_by_name(g_config.edition)) {
                LOG_WARN(LOG_CAT_SYSTEM, "Invalid edition '%s' in config; using default",
                         g_config.edition);
            }
        }
    }

    g_config.initialized = true;
    return 0;
}

void convergio_config_shutdown(void) {
    // Save any pending changes
    convergio_config_save();
    g_config.initialized = false;
}

// ============================================================================
// CONFIG FILE OPERATIONS
// ============================================================================

int convergio_config_load(void) {
    int fd = safe_path_open(g_config.config_file, safe_path_get_user_boundary(), O_RDONLY, 0);
    FILE* f = fd >= 0 ? fdopen(fd, "r") : NULL;
    if (!f) {
        return -1; // File doesn't exist, use defaults
    }

    char line[512];
    char section[64] = "";

    while (fgets(line, sizeof(line), f)) {
        parse_config_line(line, section, sizeof(section));
    }

    fclose(f);
    return 0;
}

int convergio_config_save(void) {
    int fd = safe_path_open(g_config.config_file, safe_path_get_user_boundary(),
                            O_WRONLY | O_CREAT | O_TRUNC, 0644);
    FILE* f = fd >= 0 ? fdopen(fd, "w") : NULL;
    if (!f) {
        return -1;
    }

    fprintf(f, "# Convergio Configuration\n");
    fprintf(f, "# Generated automatically - edit with care\n\n");

    fprintf(f, "[api]\n");
    fprintf(f, "# API key is stored in macOS Keychain for security\n");
    fprintf(f, "# Use 'convergio setup' to configure\n\n");

    fprintf(f, "[budget]\n");
    fprintf(f, "default_limit = %.2f\n", g_config.budget_limit);
    fprintf(f, "warn_at_percent = %d\n\n", g_config.budget_warn_percent);

    fprintf(f, "[ui]\n");
    fprintf(f, "color = %s\n", g_config.color_enabled ? "true" : "false");
    fprintf(f, "debug_level = \"%s\"\n", g_config.debug_level);
    fprintf(f, "theme = \"%s\"\n", g_config.theme[0] ? g_config.theme : "Ocean");
    fprintf(f, "style = \"%s\"\n", g_config.style[0] ? g_config.style : "balanced");
    // Save current runtime edition (may differ from config if set via CLI/env)
    fprintf(f, "edition = \"%s\"\n\n", edition_get_name(edition_current()));

    fprintf(f, "[updates]\n");
    fprintf(f, "check_on_startup = %s\n", g_config.check_updates_on_startup ? "true" : "false");
    fprintf(f, "auto_update = %s\n", g_config.auto_update ? "true" : "false");

    fclose(f);
    return 0;
}

void convergio_config_reset(void) {
    set_defaults();
    setup_paths();
}

// ============================================================================
// CONFIG ACCESSORS
// ============================================================================

const char* convergio_config_get(const char* key) {
    if (!key)
        return NULL;

    if (strcmp(key, "api_key") == 0) {
        return convergio_get_api_key();
    }
    if (strcmp(key, "debug_level") == 0) {
        return g_config.debug_level;
    }
    if (strcmp(key, "config_dir") == 0) {
        return g_config.config_dir;
    }
    if (strcmp(key, "db_path") == 0) {
        return g_config.db_path;
    }
    if (strcmp(key, "notes_dir") == 0) {
        return g_config.notes_dir;
    }
    if (strcmp(key, "knowledge_dir") == 0) {
        return g_config.knowledge_dir;
    }
    if (strcmp(key, "theme") == 0) {
        return g_config.theme[0] ? g_config.theme : "Ocean";
    }
    if (strcmp(key, "style") == 0) {
        return g_config.style[0] ? g_config.style : "balanced";
    }

    return NULL;
}

int convergio_config_set(const char* key, const char* value) {
    if (!key || !value)
        return -1;

    if (strcmp(key, "budget_limit") == 0) {
        g_config.budget_limit = atof(value);
        return 0;
    }
    if (strcmp(key, "debug_level") == 0) {
        strncpy(g_config.debug_level, value, sizeof(g_config.debug_level) - 1);
        return 0;
    }
    if (strcmp(key, "color") == 0) {
        g_config.color_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        return 0;
    }
    if (strcmp(key, "theme") == 0) {
        strncpy(g_config.theme, value, sizeof(g_config.theme) - 1);
        return 0;
    }
    if (strcmp(key, "style") == 0) {
        strncpy(g_config.style, value, sizeof(g_config.style) - 1);
        return 0;
    }

    return -1; // Unknown key
}

// ============================================================================
// API KEY MANAGEMENT
// ============================================================================

const char* convergio_get_api_key(void) {
    // 1. Check environment variable first
    const char* env_key = getenv("ANTHROPIC_API_KEY");
    if (env_key && strlen(env_key) > 0) {
        return env_key;
    }

    // 2. Try Keychain
    static char keychain_key[256] = {0};
    char* kc_key = convergio_keychain_read(KEYCHAIN_SERVICE, KEYCHAIN_ACCOUNT);
    if (kc_key) {
        strncpy(keychain_key, kc_key, sizeof(keychain_key) - 1);
        free(kc_key);
        return keychain_key;
    }

    // 3. Check config file
    if (strlen(g_config.anthropic_api_key) > 0) {
        return g_config.anthropic_api_key;
    }

    return NULL;
}

int convergio_store_api_key(const char* key) {
    if (!key || strlen(key) == 0) {
        return -1;
    }
    return convergio_keychain_store(KEYCHAIN_SERVICE, KEYCHAIN_ACCOUNT, key);
}

int convergio_delete_api_key(void) {
    return convergio_keychain_delete(KEYCHAIN_SERVICE, KEYCHAIN_ACCOUNT);
}

// ============================================================================
// SETUP WIZARD
// ============================================================================

int convergio_setup_wizard(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║             CONVERGIO SETUP WIZARD                   ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Check if already configured
    const char* existing_key = convergio_get_api_key();
    if (existing_key) {
        printf("An API key is already configured.\n");
        printf("Do you want to replace it? (y/N): ");
        fflush(stdout);

        char response[16];
        if (fgets(response, sizeof(response), stdin)) {
            if (response[0] != 'y' && response[0] != 'Y') {
                printf("\nSetup cancelled. Existing configuration preserved.\n");
                return 0;
            }
        }
    }

    printf("\n");
    printf("Enter your Anthropic API key:\n");
    printf("(Get one at https://console.anthropic.com/settings/keys)\n");
    printf("\n");
    printf("API Key: ");
    fflush(stdout);

    char api_key[256];
    if (!fgets(api_key, sizeof(api_key), stdin)) {
        printf("\nError reading input.\n");
        return -1;
    }

    // Trim newline
    size_t len = strlen(api_key);
    if (len > 0 && api_key[len - 1] == '\n') {
        api_key[len - 1] = '\0';
        len--;
    }

    // Validate format
    if (len < 10 || strncmp(api_key, "sk-", 3) != 0) {
        printf("\nInvalid API key format. Keys should start with 'sk-'.\n");
        return -1;
    }

    // Store in Keychain
    if (convergio_store_api_key(api_key) != 0) {
        printf("\nFailed to store API key in Keychain.\n");
        printf("Falling back to config file storage.\n");
        strncpy(g_config.anthropic_api_key, api_key, sizeof(g_config.anthropic_api_key) - 1);
    } else {
        printf("\nAPI key stored securely in macOS Keychain.\n");
    }

    // Save config
    convergio_config_save();

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║               SETUP COMPLETE!                        ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  You can now start Convergio by running:             ║\n");
    printf("║                                                      ║\n");
    printf("║    convergio                                         ║\n");
    printf("║                                                      ║\n");
    printf("║  For help:                                           ║\n");
    printf("║    convergio --help                                  ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}

bool convergio_setup_complete(void) {
    return convergio_get_api_key() != NULL;
}

// ============================================================================
// RESPONSE STYLE SETTINGS
// ============================================================================

// Style definitions - must match commands.c
// Token limits increased in v6.2.0 to prevent response truncation
typedef struct {
    const char* name;
    int max_tokens;
    double temperature;
    bool markdown;
} StyleDef;

static const StyleDef STYLE_DEFS[] = {
    {"flash", 4096, 0.3, false},      // Quick responses
    {"concise", 8192, 0.5, true},     // Concise but complete
    {"balanced", 16384, 0.7, true},   // Default: detailed responses
    {"detailed", 32768, 0.9, true},   // Maximum detail (Opus 4.5 supports 128k output)
};
#define STYLE_DEF_COUNT 4

StyleSettings convergio_get_style_settings(void) {
    const char* style = g_config.style[0] ? g_config.style : "balanced";

    for (int i = 0; i < STYLE_DEF_COUNT; i++) {
        if (strcmp(STYLE_DEFS[i].name, style) == 0) {
            return (StyleSettings){.max_tokens = STYLE_DEFS[i].max_tokens,
                                   .temperature = STYLE_DEFS[i].temperature,
                                   .markdown = STYLE_DEFS[i].markdown};
        }
    }

    // Default to balanced (16384 tokens to prevent truncation)
    return (StyleSettings){.max_tokens = 16384, .temperature = 0.7, .markdown = true};
}

const char* convergio_get_style_name(void) {
    return g_config.style[0] ? g_config.style : "balanced";
}
