/**
 * CONVERGIO TELEMETRY - Core Implementation
 *
 * Privacy-first, opt-in telemetry system
 * Collects anonymous, aggregated usage metrics locally
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/telemetry.h"
#include "nous/config.h"
#include "nous/safe_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define TELEMETRY_CONFIG_FILE "telemetry_config.json"
#define TELEMETRY_DATA_FILE "telemetry.json"
#define MAX_EVENTS_IN_MEMORY 1000
#define MAX_EVENTS_ON_DISK 10000

// ============================================================================
// GLOBAL STATE
// ============================================================================

static TelemetryConfig g_telemetry_config = {0};
static TelemetryEvent* g_events = NULL;
static size_t g_event_count = 0;
static size_t g_event_capacity = 0;
static bool g_initialized = false;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static int load_config(void);
static int save_config(void);
static void generate_anonymous_id(char* id_out, size_t size);
static int add_event(const TelemetryEvent* event);
static int load_events_from_disk(void);

// ============================================================================
// INITIALIZATION
// ============================================================================

int telemetry_init(void) {
    if (g_initialized) {
        return 0;
    }

    // Build paths
    snprintf(g_telemetry_config.config_path, sizeof(g_telemetry_config.config_path),
             "%s/%s", g_config.config_dir, TELEMETRY_CONFIG_FILE);
    snprintf(g_telemetry_config.data_path, sizeof(g_telemetry_config.data_path),
             "%s/%s", g_config.config_dir, TELEMETRY_DATA_FILE);

    // Load or create configuration
    if (load_config() != 0) {
        // Create default config (disabled by default)
        g_telemetry_config.enabled = false;
        g_telemetry_config.anonymous_id[0] = '\0';

        #ifdef CONVERGIO_VERSION
        snprintf(g_telemetry_config.convergio_version,
                 sizeof(g_telemetry_config.convergio_version),
                 "%s", CONVERGIO_VERSION);
        #else
        snprintf(g_telemetry_config.convergio_version,
                 sizeof(g_telemetry_config.convergio_version),
                 "unknown");
        #endif

        #ifdef __APPLE__
        snprintf(g_telemetry_config.os_type,
                 sizeof(g_telemetry_config.os_type),
                 "darwin");
        #elif defined(__linux__)
        snprintf(g_telemetry_config.os_type,
                 sizeof(g_telemetry_config.os_type),
                 "linux");
        #else
        snprintf(g_telemetry_config.os_type,
                 sizeof(g_telemetry_config.os_type),
                 "unknown");
        #endif

        save_config();
    }

    // Allocate event buffer
    g_event_capacity = MAX_EVENTS_IN_MEMORY;
    g_events = calloc(g_event_capacity, sizeof(TelemetryEvent));
    if (!g_events) {
        fprintf(stderr, "telemetry: Failed to allocate event buffer\n");
        return -1;
    }
    g_event_count = 0;

    // Load existing events from disk if telemetry is enabled
    if (g_telemetry_config.enabled) {
        load_events_from_disk();
    }

    g_initialized = true;
    return 0;
}

void telemetry_shutdown(void) {
    if (!g_initialized) {
        return;
    }

    // Flush pending events
    if (g_telemetry_config.enabled && g_event_count > 0) {
        telemetry_flush();
    }

    // Free event buffer
    free(g_events);
    g_events = NULL;
    g_event_count = 0;
    g_event_capacity = 0;

    g_initialized = false;
}

// ============================================================================
// STATUS
// ============================================================================

bool telemetry_is_enabled(void) {
    if (!g_initialized) {
        telemetry_init();
    }
    return g_telemetry_config.enabled;
}

const TelemetryConfig* telemetry_get_config(void) {
    if (!g_initialized) {
        telemetry_init();
    }
    return &g_telemetry_config;
}

// ============================================================================
// EVENT RECORDING
// ============================================================================

void telemetry_record_api_call(
    const char* provider,
    const char* model,
    uint64_t tokens_input,
    uint64_t tokens_output,
    double latency_ms
) {
    if (!telemetry_is_enabled() || !provider || !model) {
        return;
    }

    TelemetryEvent event = {0};
    event.type = TELEMETRY_EVENT_API_CALL;
    event.timestamp = time(NULL);

    snprintf(event.provider, sizeof(event.provider), "%s", provider);
    snprintf(event.model, sizeof(event.model), "%s", model);
    event.tokens_input = tokens_input;
    event.tokens_output = tokens_output;
    event.latency_ms = latency_ms;

    add_event(&event);
}

void telemetry_record_error(const char* error_type) {
    if (!telemetry_is_enabled() || !error_type) {
        return;
    }

    TelemetryEvent event = {0};
    event.type = TELEMETRY_EVENT_ERROR;
    event.timestamp = time(NULL);

    snprintf(event.error_type, sizeof(event.error_type), "%s", error_type);

    add_event(&event);
}

void telemetry_record_fallback(
    const char* from_provider,
    const char* to_provider
) {
    if (!telemetry_is_enabled() || !from_provider || !to_provider) {
        return;
    }

    TelemetryEvent event = {0};
    event.type = TELEMETRY_EVENT_FALLBACK;
    event.timestamp = time(NULL);

    snprintf(event.from_provider, sizeof(event.from_provider), "%s", from_provider);
    snprintf(event.to_provider, sizeof(event.to_provider), "%s", to_provider);

    add_event(&event);
}

void telemetry_record_session_start(void) {
    if (!telemetry_is_enabled()) {
        return;
    }

    TelemetryEvent event = {0};
    event.type = TELEMETRY_EVENT_SESSION_START;
    event.timestamp = time(NULL);

    add_event(&event);
}

void telemetry_record_session_end(void) {
    if (!telemetry_is_enabled()) {
        return;
    }

    TelemetryEvent event = {0};
    event.type = TELEMETRY_EVENT_SESSION_END;
    event.timestamp = time(NULL);

    add_event(&event);
}

// ============================================================================
// DATA MANAGEMENT
// ============================================================================

char* telemetry_get_stats(void) {
    if (!g_initialized) {
        telemetry_init();
    }

    // Calculate aggregated statistics
    uint64_t total_api_calls = 0;
    uint64_t total_tokens_input = 0;
    uint64_t total_tokens_output = 0;
    double total_latency = 0.0;
    uint64_t total_errors = 0;
    uint64_t total_fallbacks = 0;

    for (size_t i = 0; i < g_event_count; i++) {
        const TelemetryEvent* e = &g_events[i];
        switch (e->type) {
            case TELEMETRY_EVENT_API_CALL:
                total_api_calls++;
                total_tokens_input += e->tokens_input;
                total_tokens_output += e->tokens_output;
                total_latency += e->latency_ms;
                break;
            case TELEMETRY_EVENT_ERROR:
                total_errors++;
                break;
            case TELEMETRY_EVENT_FALLBACK:
                total_fallbacks++;
                break;
            default:
                break;
        }
    }

    double avg_latency = total_api_calls > 0 ? total_latency / total_api_calls : 0.0;

    // Build JSON response
    char* stats = malloc(2048);
    if (!stats) {
        return NULL;
    }

    snprintf(stats, 2048,
        "{\n"
        "  \"total_api_calls\": %llu,\n"
        "  \"total_tokens_input\": %llu,\n"
        "  \"total_tokens_output\": %llu,\n"
        "  \"average_latency_ms\": %.2f,\n"
        "  \"total_errors\": %llu,\n"
        "  \"total_fallbacks\": %llu,\n"
        "  \"events_recorded\": %zu\n"
        "}",
        (unsigned long long)total_api_calls,
        (unsigned long long)total_tokens_input,
        (unsigned long long)total_tokens_output,
        avg_latency,
        (unsigned long long)total_errors,
        (unsigned long long)total_fallbacks,
        g_event_count
    );

    return stats;
}

int telemetry_flush(void) {
    if (!g_initialized || !g_telemetry_config.enabled) {
        return -1;
    }

    // Open data file for writing
    int fd = safe_path_open(g_telemetry_config.data_path, safe_path_get_user_boundary(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    FILE* f = fd >= 0 ? fdopen(fd, "w") : NULL;
    if (!f) {
        fprintf(stderr, "telemetry: Failed to open data file for writing: %s\n",
                strerror(errno));
        return -1;
    }

    // Write JSON array of events
    fprintf(f, "{\n");
    fprintf(f, "  \"version\": \"%s\",\n", g_telemetry_config.convergio_version);
    fprintf(f, "  \"os_type\": \"%s\",\n", g_telemetry_config.os_type);
    fprintf(f, "  \"anonymous_id\": \"%s\",\n", g_telemetry_config.anonymous_id);
    fprintf(f, "  \"events\": [\n");

    for (size_t i = 0; i < g_event_count; i++) {
        const TelemetryEvent* e = &g_events[i];
        fprintf(f, "    {\n");
        fprintf(f, "      \"type\": ");

        switch (e->type) {
            case TELEMETRY_EVENT_API_CALL:
                fprintf(f, "\"api_call\",\n");
                fprintf(f, "      \"timestamp\": %ld,\n", e->timestamp);
                fprintf(f, "      \"provider\": \"%s\",\n", e->provider);
                fprintf(f, "      \"model\": \"%s\",\n", e->model);
                fprintf(f, "      \"tokens_input\": %llu,\n", (unsigned long long)e->tokens_input);
                fprintf(f, "      \"tokens_output\": %llu,\n", (unsigned long long)e->tokens_output);
                fprintf(f, "      \"latency_ms\": %.2f\n", e->latency_ms);
                break;
            case TELEMETRY_EVENT_ERROR:
                fprintf(f, "\"error\",\n");
                fprintf(f, "      \"timestamp\": %ld,\n", e->timestamp);
                fprintf(f, "      \"error_type\": \"%s\"\n", e->error_type);
                break;
            case TELEMETRY_EVENT_FALLBACK:
                fprintf(f, "\"fallback\",\n");
                fprintf(f, "      \"timestamp\": %ld,\n", e->timestamp);
                fprintf(f, "      \"from_provider\": \"%s\",\n", e->from_provider);
                fprintf(f, "      \"to_provider\": \"%s\"\n", e->to_provider);
                break;
            case TELEMETRY_EVENT_SESSION_START:
                fprintf(f, "\"session_start\",\n");
                fprintf(f, "      \"timestamp\": %ld\n", e->timestamp);
                break;
            case TELEMETRY_EVENT_SESSION_END:
                fprintf(f, "\"session_end\",\n");
                fprintf(f, "      \"timestamp\": %ld\n", e->timestamp);
                break;
            case TELEMETRY_EVENT_WORKFLOW_START:
            case TELEMETRY_EVENT_WORKFLOW_END:
            case TELEMETRY_EVENT_WORKFLOW_NODE:
            case TELEMETRY_EVENT_WORKFLOW_ERROR:
            case TELEMETRY_EVENT_ORCHESTRATOR_DELEGATION:
            case TELEMETRY_EVENT_ORCHESTRATOR_PLANNING:
            case TELEMETRY_EVENT_ORCHESTRATOR_CONVERGENCE:
                // These events are handled by workflow_observability.c
                // For export, just include basic info
                fprintf(f, "\"workflow_event\",\n");
                fprintf(f, "      \"timestamp\": %ld\n", e->timestamp);
                break;
            default:
                fprintf(f, "\"unknown\",\n");
                fprintf(f, "      \"timestamp\": %ld\n", e->timestamp);
                break;
        }

        fprintf(f, "    }%s\n", (i < g_event_count - 1) ? "," : "");
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    return 0;
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static int load_config(void) {
    int fd = safe_path_open(g_telemetry_config.config_path, safe_path_get_user_boundary(), O_RDONLY, 0);
    FILE* f = fd >= 0 ? fdopen(fd, "r") : NULL;
    if (!f) {
        return -1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char key[256], value[512];
        if (sscanf(line, " \"%255[^\"]\" : \"%511[^\"]\"", key, value) == 2) {
            if (strcmp(key, "enabled") == 0) {
                g_telemetry_config.enabled = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "anonymous_id") == 0) {
                snprintf(g_telemetry_config.anonymous_id,
                         sizeof(g_telemetry_config.anonymous_id),
                         "%s", value);
            } else if (strcmp(key, "convergio_version") == 0) {
                snprintf(g_telemetry_config.convergio_version,
                         sizeof(g_telemetry_config.convergio_version),
                         "%s", value);
            } else if (strcmp(key, "os_type") == 0) {
                snprintf(g_telemetry_config.os_type,
                         sizeof(g_telemetry_config.os_type),
                         "%s", value);
            }
        } else if (sscanf(line, " \"%255[^\"]\" : %511[^,}]", key, value) == 2) {
            if (strcmp(key, "enabled") == 0) {
                g_telemetry_config.enabled = (strncmp(value, "true", 4) == 0);
            }
        }
    }

    fclose(f);
    return 0;
}

static int save_config(void) {
    int fd = safe_path_open(g_telemetry_config.config_path, safe_path_get_user_boundary(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    FILE* f = fd >= 0 ? fdopen(fd, "w") : NULL;
    if (!f) {
        fprintf(stderr, "telemetry: Failed to save config: %s\n", strerror(errno));
        return -1;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"enabled\": %s,\n", g_telemetry_config.enabled ? "true" : "false");
    fprintf(f, "  \"anonymous_id\": \"%s\",\n", g_telemetry_config.anonymous_id);
    fprintf(f, "  \"convergio_version\": \"%s\",\n", g_telemetry_config.convergio_version);
    fprintf(f, "  \"os_type\": \"%s\"\n", g_telemetry_config.os_type);
    fprintf(f, "}\n");

    fclose(f);
    return 0;
}

static void generate_anonymous_id(char* id_out, size_t size) {
    // Generate a random anonymous ID using /dev/urandom
    unsigned char random_bytes[32];
    FILE* urandom = fopen("/dev/urandom", "r");
    if (!urandom) {
        // Fallback to timestamp-based ID
        snprintf(id_out, size, "%lx", (unsigned long)time(NULL));
        return;
    }

    size_t read_bytes = fread(random_bytes, 1, sizeof(random_bytes), urandom);
    fclose(urandom);

    if (read_bytes != sizeof(random_bytes)) {
        snprintf(id_out, size, "%lx", (unsigned long)time(NULL));
        return;
    }

    // Convert to hex string
    char hex[65] = {0};
    for (size_t i = 0; i < sizeof(random_bytes) && i * 2 < 64; i++) {
        snprintf(hex + i * 2, 3, "%02x", random_bytes[i]);
    }
    hex[64] = '\0';

    snprintf(id_out, size, "%s", hex);
}

static int add_event(const TelemetryEvent* event) {
    if (!event) {
        return -1;
    }

    // Check capacity
    if (g_event_count >= g_event_capacity) {
        // Flush to disk and reset
        telemetry_flush();
        g_event_count = 0;
    }

    // Add event
    memcpy(&g_events[g_event_count], event, sizeof(TelemetryEvent));
    g_event_count++;

    return 0;
}

static int load_events_from_disk(void) {
    // Note: For simplicity, we don't load events from disk on init
    // Events are only loaded for export/view operations
    // This prevents memory issues with large event logs
    return 0;
}

// ============================================================================
// PUBLIC API - Enable/Disable
// ============================================================================

int telemetry_enable(void) {
    if (!g_initialized) {
        telemetry_init();
    }

    // Generate anonymous ID if not set
    if (g_telemetry_config.anonymous_id[0] == '\0') {
        generate_anonymous_id(g_telemetry_config.anonymous_id,
                            sizeof(g_telemetry_config.anonymous_id));
    }

    g_telemetry_config.enabled = true;

    int result = save_config();
    if (result == 0) {
        printf("Telemetry enabled. Anonymous ID: %s\n",
               g_telemetry_config.anonymous_id);
    }

    return result;
}

int telemetry_disable(void) {
    if (!g_initialized) {
        telemetry_init();
    }

    g_telemetry_config.enabled = false;

    int result = save_config();
    if (result == 0) {
        printf("Telemetry disabled.\n");
    }

    return result;
}
