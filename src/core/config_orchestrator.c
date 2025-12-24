/**
 * CONVERGIO CONFIG ORCHESTRATOR - Implementation (REF-03)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/config_orchestrator.h"
#include "nous/nous.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CONFIG ENTRY
// ============================================================================

typedef struct ConfigEntry {
    char key[128];
    ConfigValue value;
    struct ConfigEntry* next;
} ConfigEntry;

// ============================================================================
// CONFIG STATE
// ============================================================================

#define CONFIG_HASH_SIZE 64

static ConfigEntry* g_config_table[CONFIG_HASH_SIZE];
static pthread_mutex_t g_config_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_config_initialized = false;

// ============================================================================
// HASH FUNCTION
// ============================================================================

static unsigned int config_hash(const char* key) {
    unsigned int hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % CONFIG_HASH_SIZE;
}

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

static ConfigEntry* config_find_entry(const char* key) {
    unsigned int idx = config_hash(key);
    ConfigEntry* entry = g_config_table[idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static void config_set_internal(const char* key, ConfigValueType type, const void* value,
                                ConfigSource source) {
    pthread_mutex_lock(&g_config_mutex);

    ConfigEntry* entry = config_find_entry(key);
    if (!entry) {
        entry = calloc(1, sizeof(ConfigEntry));
        if (!entry) {
            pthread_mutex_unlock(&g_config_mutex);
            return;
        }
        strncpy(entry->key, key, sizeof(entry->key) - 1);

        unsigned int idx = config_hash(key);
        entry->next = g_config_table[idx];
        g_config_table[idx] = entry;
    }

    // Only override if new source has higher priority
    if (source >= entry->value.source) {
        entry->value.type = type;
        entry->value.source = source;

        switch (type) {
        case CONFIG_TYPE_STRING:
            entry->value.value.str_val = strdup((const char*)value);
            break;
        case CONFIG_TYPE_INT:
            entry->value.value.int_val = *(const int*)value;
            break;
        case CONFIG_TYPE_BOOL:
            entry->value.value.bool_val = *(const bool*)value;
            break;
        case CONFIG_TYPE_DOUBLE:
            entry->value.value.double_val = *(const double*)value;
            break;
        }
    }

    pthread_mutex_unlock(&g_config_mutex);
}

// ============================================================================
// LOAD DEFAULTS
// ============================================================================

static void config_load_defaults(void) {
    // Default values
    int default_max_tokens = 4096;
    double default_temp = 0.7;
    bool default_telemetry = true;

    config_set_internal(CONFIG_KEY_MODEL, CONFIG_TYPE_STRING, "claude-sonnet-4-20250514",
                        CONFIG_SOURCE_DEFAULT);
    config_set_internal(CONFIG_KEY_MAX_TOKENS, CONFIG_TYPE_INT, &default_max_tokens,
                        CONFIG_SOURCE_DEFAULT);
    config_set_internal(CONFIG_KEY_TEMPERATURE, CONFIG_TYPE_DOUBLE, &default_temp,
                        CONFIG_SOURCE_DEFAULT);
    config_set_internal(CONFIG_KEY_EDITION, CONFIG_TYPE_STRING, "master", CONFIG_SOURCE_DEFAULT);
    config_set_internal(CONFIG_KEY_THEME, CONFIG_TYPE_STRING, "dark", CONFIG_SOURCE_DEFAULT);
    config_set_internal(CONFIG_KEY_LOG_LEVEL, CONFIG_TYPE_STRING, "info", CONFIG_SOURCE_DEFAULT);
    config_set_internal(CONFIG_KEY_TELEMETRY, CONFIG_TYPE_BOOL, &default_telemetry,
                        CONFIG_SOURCE_DEFAULT);
}

// ============================================================================
// LOAD ENVIRONMENT VARIABLES
// ============================================================================

static void config_load_env(void) {
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (api_key) {
        config_set_internal(CONFIG_KEY_API_KEY, CONFIG_TYPE_STRING, api_key, CONFIG_SOURCE_ENV);
    }

    const char* model = getenv("CONVERGIO_MODEL");
    if (model) {
        config_set_internal(CONFIG_KEY_MODEL, CONFIG_TYPE_STRING, model, CONFIG_SOURCE_ENV);
    }

    const char* edition = getenv("CONVERGIO_EDITION");
    if (edition) {
        config_set_internal(CONFIG_KEY_EDITION, CONFIG_TYPE_STRING, edition, CONFIG_SOURCE_ENV);
    }

    const char* log_level = getenv("CONVERGIO_LOG_LEVEL");
    if (log_level) {
        config_set_internal(CONFIG_KEY_LOG_LEVEL, CONFIG_TYPE_STRING, log_level, CONFIG_SOURCE_ENV);
    }

    const char* telemetry = getenv("CONVERGIO_TELEMETRY");
    if (telemetry) {
        bool enabled = (strcmp(telemetry, "1") == 0 || strcmp(telemetry, "true") == 0);
        config_set_internal(CONFIG_KEY_TELEMETRY, CONFIG_TYPE_BOOL, &enabled, CONFIG_SOURCE_ENV);
    }

    const char* data_dir = getenv("CONVERGIO_DATA_DIR");
    if (data_dir) {
        config_set_internal(CONFIG_KEY_DATA_DIR, CONFIG_TYPE_STRING, data_dir, CONFIG_SOURCE_ENV);
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

int config_orchestrator_init(void) {
    if (g_config_initialized) {
        return 0;
    }

    memset(g_config_table, 0, sizeof(g_config_table));

    // Load in priority order
    config_load_defaults(); // Lowest priority
    // config_load_file();      // Would load TOML here if available
    config_load_env(); // Highest priority (before CLI)

    g_config_initialized = true;
    LOG_INFO(LOG_CAT_SYSTEM, "Config orchestrator initialized");
    return 0;
}

void config_orchestrator_shutdown(void) {
    pthread_mutex_lock(&g_config_mutex);

    for (int i = 0; i < CONFIG_HASH_SIZE; i++) {
        ConfigEntry* entry = g_config_table[i];
        while (entry) {
            ConfigEntry* next = entry->next;
            if (entry->value.type == CONFIG_TYPE_STRING && entry->value.value.str_val) {
                free((void*)entry->value.value.str_val);
            }
            free(entry);
            entry = next;
        }
        g_config_table[i] = NULL;
    }

    g_config_initialized = false;
    pthread_mutex_unlock(&g_config_mutex);
}

const char* config_get_string(const char* key, const char* default_val) {
    pthread_mutex_lock(&g_config_mutex);
    ConfigEntry* entry = config_find_entry(key);
    const char* result = (entry && entry->value.type == CONFIG_TYPE_STRING)
                             ? entry->value.value.str_val
                             : default_val;
    pthread_mutex_unlock(&g_config_mutex);
    return result;
}

int config_get_int(const char* key, int default_val) {
    pthread_mutex_lock(&g_config_mutex);
    ConfigEntry* entry = config_find_entry(key);
    int result =
        (entry && entry->value.type == CONFIG_TYPE_INT) ? entry->value.value.int_val : default_val;
    pthread_mutex_unlock(&g_config_mutex);
    return result;
}

bool config_get_bool(const char* key, bool default_val) {
    pthread_mutex_lock(&g_config_mutex);
    ConfigEntry* entry = config_find_entry(key);
    bool result = (entry && entry->value.type == CONFIG_TYPE_BOOL) ? entry->value.value.bool_val
                                                                   : default_val;
    pthread_mutex_unlock(&g_config_mutex);
    return result;
}

double config_get_double(const char* key, double default_val) {
    pthread_mutex_lock(&g_config_mutex);
    ConfigEntry* entry = config_find_entry(key);
    double result = (entry && entry->value.type == CONFIG_TYPE_DOUBLE)
                        ? entry->value.value.double_val
                        : default_val;
    pthread_mutex_unlock(&g_config_mutex);
    return result;
}

void config_set_override(const char* key, const char* value) {
    config_set_internal(key, CONFIG_TYPE_STRING, value, CONFIG_SOURCE_RUNTIME);
}

ConfigSource config_get_source(const char* key) {
    pthread_mutex_lock(&g_config_mutex);
    ConfigEntry* entry = config_find_entry(key);
    ConfigSource source = entry ? entry->value.source : CONFIG_SOURCE_DEFAULT;
    pthread_mutex_unlock(&g_config_mutex);
    return source;
}

int config_reload(void) {
    // Preserve runtime overrides, reload file and env
    LOG_INFO(LOG_CAT_SYSTEM, "Config reload requested");
    config_load_env();
    return 0;
}

void config_dump(void) {
    const char* source_names[] = {"DEFAULT", "FILE", "ENV", "CLI", "RUNTIME"};

    pthread_mutex_lock(&g_config_mutex);
    printf("=== Config Dump ===\n");
    for (int i = 0; i < CONFIG_HASH_SIZE; i++) {
        ConfigEntry* entry = g_config_table[i];
        while (entry) {
            printf("  %s = ", entry->key);
            switch (entry->value.type) {
            case CONFIG_TYPE_STRING:
                printf("\"%s\"",
                       entry->value.value.str_val ? entry->value.value.str_val : "(null)");
                break;
            case CONFIG_TYPE_INT:
                printf("%d", entry->value.value.int_val);
                break;
            case CONFIG_TYPE_BOOL:
                printf("%s", entry->value.value.bool_val ? "true" : "false");
                break;
            case CONFIG_TYPE_DOUBLE:
                printf("%.2f", entry->value.value.double_val);
                break;
            }
            printf(" [%s]\n", source_names[entry->value.source]);
            entry = entry->next;
        }
    }
    pthread_mutex_unlock(&g_config_mutex);
}
