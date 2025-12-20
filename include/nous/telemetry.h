/**
 * CONVERGIO TELEMETRY
 *
 * Privacy-first, opt-in telemetry system
 * Collects anonymous, aggregated usage metrics for improving Convergio
 *
 * CORE PRINCIPLES:
 * - OPT-IN ONLY (never enabled by default)
 * - Privacy-first (no PII, anonymous aggregate metrics only)
 * - User control (view/export/delete at any time)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_TELEMETRY_H
#define CONVERGIO_TELEMETRY_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ============================================================================
// TELEMETRY STRUCTURES
// ============================================================================

/**
 * Telemetry event types
 */
typedef enum {
    TELEMETRY_EVENT_API_CALL,       // API call to a provider
    TELEMETRY_EVENT_ERROR,          // Error occurred (type only, no content)
    TELEMETRY_EVENT_FALLBACK,       // Provider fallback triggered
    TELEMETRY_EVENT_SESSION_START,  // Session started
    TELEMETRY_EVENT_SESSION_END,    // Session ended
    TELEMETRY_EVENT_WORKFLOW_START, // Workflow execution started
    TELEMETRY_EVENT_WORKFLOW_END,   // Workflow execution ended
    TELEMETRY_EVENT_WORKFLOW_NODE,  // Workflow node executed
    TELEMETRY_EVENT_WORKFLOW_ERROR, // Workflow error occurred
    TELEMETRY_EVENT_ORCHESTRATOR_DELEGATION, // Agent delegation event
    TELEMETRY_EVENT_ORCHESTRATOR_PLANNING,   // Task planning event
    TELEMETRY_EVENT_ORCHESTRATOR_CONVERGENCE, // Response convergence event
} TelemetryEventType;

/**
 * Telemetry event structure
 * Contains only anonymous, aggregated metrics
 */
typedef struct {
    TelemetryEventType type;
    time_t timestamp;

    // API call metrics (type == TELEMETRY_EVENT_API_CALL)
    char provider[64];              // e.g., "anthropic", "openai"
    char model[128];                // e.g., "claude-sonnet-4.5", "gpt-4"
    uint64_t tokens_input;          // Input tokens consumed
    uint64_t tokens_output;         // Output tokens consumed
    double latency_ms;              // API call latency in milliseconds

    // Error metrics (type == TELEMETRY_EVENT_ERROR)
    char error_type[64];            // e.g., "network_error", "auth_error"

    // Fallback metrics (type == TELEMETRY_EVENT_FALLBACK)
    char from_provider[64];         // Provider that failed
    char to_provider[64];           // Provider used as fallback

} TelemetryEvent;

/**
 * Telemetry configuration
 */
typedef struct {
    bool enabled;                   // Telemetry enabled (opt-in)
    char anonymous_id[65];          // Anonymous random hash (SHA-256)
    char convergio_version[32];     // Convergio version
    char os_type[32];               // OS type (e.g., "darwin", "linux")
    char config_path[512];          // Path to telemetry config
    char data_path[512];            // Path to telemetry data file
} TelemetryConfig;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize telemetry system
 * Loads configuration from ~/.convergio/telemetry_config.json
 * Creates data directory if needed
 * Returns 0 on success, -1 on failure
 */
int telemetry_init(void);

/**
 * Shutdown telemetry system
 * Flushes any pending events to disk
 */
void telemetry_shutdown(void);

// ============================================================================
// STATUS
// ============================================================================

/**
 * Check if telemetry is enabled
 * Returns true if user has opted in
 */
bool telemetry_is_enabled(void);

/**
 * Get telemetry configuration
 * Returns NULL if not initialized
 */
const TelemetryConfig* telemetry_get_config(void);

// ============================================================================
// EVENT RECORDING
// ============================================================================

/**
 * Record an API call event
 * Only recorded if telemetry is enabled
 */
void telemetry_record_api_call(
    const char* provider,
    const char* model,
    uint64_t tokens_input,
    uint64_t tokens_output,
    double latency_ms
);

/**
 * Record an error event
 * Only recorded if telemetry is enabled
 * NOTE: Only error type is recorded, no error messages or content
 */
void telemetry_record_error(const char* error_type);

/**
 * Record a provider fallback event
 * Only recorded if telemetry is enabled
 */
void telemetry_record_fallback(
    const char* from_provider,
    const char* to_provider
);

/**
 * Record a session start event
 * Only recorded if telemetry is enabled
 */
void telemetry_record_session_start(void);

/**
 * Record a session end event
 * Only recorded if telemetry is enabled
 */
void telemetry_record_session_end(void);

// ============================================================================
// DATA MANAGEMENT
// ============================================================================

/**
 * Get aggregated statistics
 * Returns JSON string with aggregated stats (must be freed by caller)
 * Returns NULL on failure
 */
char* telemetry_get_stats(void);

/**
 * Flush pending events to disk
 * Writes to ~/.convergio/telemetry.json
 * Returns 0 on success, -1 on failure
 */
int telemetry_flush(void);

/**
 * Export all telemetry data as JSON
 * Returns JSON string (must be freed by caller)
 * Returns NULL on failure
 */
char* telemetry_export(void);

/**
 * Delete all collected telemetry data
 * Returns 0 on success, -1 on failure
 */
int telemetry_delete(void);

/**
 * View collected telemetry data in human-readable format
 * Prints to stdout
 */
void telemetry_view(void);

// ============================================================================
// CONSENT MANAGEMENT
// ============================================================================

/**
 * Show telemetry consent prompt
 * Explains what data is collected and why
 * Does NOT automatically enable telemetry
 */
void telemetry_show_consent_prompt(void);

/**
 * Enable telemetry (user opt-in)
 * Generates anonymous ID if not already set
 * Returns 0 on success, -1 on failure
 */
int telemetry_enable(void);

/**
 * Disable telemetry (user opt-out)
 * Returns 0 on success, -1 on failure
 */
int telemetry_disable(void);

/**
 * Show telemetry status
 * Prints current status, anonymous ID, and summary stats
 */
void telemetry_status(void);

#endif // CONVERGIO_TELEMETRY_H
