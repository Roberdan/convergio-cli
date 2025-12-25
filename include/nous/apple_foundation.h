/**
 * CONVERGIO APPLE FOUNDATION MODELS PROVIDER
 *
 * Native integration with Apple's Foundation Models framework (macOS 26+).
 * Provides access to Apple Intelligence's on-device 3B parameter LLM
 * with full privacy, zero latency, and offline capability.
 *
 * Features:
 * - On-device inference with Apple Intelligence
 * - Guided generation (structured output with Swift types)
 * - Tool calling support
 * - Streaming responses
 * - Full privacy - all processing on device
 *
 * Requirements:
 * - macOS Tahoe (26.0) or later
 * - Apple Silicon (M1/M2/M3/M4/M5)
 * - Apple Intelligence enabled in System Settings
 * - Xcode 26+ for development
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_APPLE_FOUNDATION_H
#define CONVERGIO_APPLE_FOUNDATION_H

#include "nous/provider.h"
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// AVAILABILITY & FEATURE FLAGS
// ============================================================================

typedef enum {
    AFM_AVAILABLE = 0,              // Foundation Models available
    AFM_ERR_NOT_MACOS_26,           // Requires macOS 26+
    AFM_ERR_NOT_APPLE_SILICON,      // Requires Apple Silicon
    AFM_ERR_INTELLIGENCE_DISABLED,  // Apple Intelligence not enabled
    AFM_ERR_MODEL_NOT_READY,        // Model downloading or unavailable
    AFM_ERR_SESSION_FAILED,         // Failed to create session
    AFM_ERR_GENERATION_FAILED,      // Generation error
    AFM_ERR_TOOL_CALL_FAILED,       // Tool calling error
    AFM_ERR_GUIDED_GEN_FAILED,      // Guided generation error
    AFM_ERR_UNKNOWN
} AppleFoundationError;

typedef struct {
    bool is_available;              // Foundation Models framework available
    bool is_apple_silicon;          // Running on Apple Silicon
    bool is_macos_26;               // Running macOS 26+
    bool intelligence_enabled;      // Apple Intelligence enabled
    bool model_ready;               // On-device model ready
    size_t model_size_billions;     // Model size (typically 3B)
    const char* os_version;         // macOS version string
    const char* chip_name;          // Apple Silicon chip name
} AppleFoundationStatus;

// ============================================================================
// GUIDED GENERATION (STRUCTURED OUTPUT)
// ============================================================================

typedef enum {
    AFM_TYPE_STRING,
    AFM_TYPE_INT,
    AFM_TYPE_FLOAT,
    AFM_TYPE_BOOL,
    AFM_TYPE_ARRAY,
    AFM_TYPE_OBJECT,
    AFM_TYPE_ENUM
} AFMSchemaType;

typedef struct AFMSchemaField {
    const char* name;
    const char* description;
    AFMSchemaType type;
    bool required;
    const char** enum_values;       // For AFM_TYPE_ENUM
    size_t enum_count;
    struct AFMSchemaField* nested;  // For AFM_TYPE_OBJECT/ARRAY
    size_t nested_count;
} AFMSchemaField;

typedef struct {
    const char* name;
    const char* description;
    AFMSchemaField* fields;
    size_t field_count;
} AFMSchema;

// ============================================================================
// TOOL CALLING
// ============================================================================

typedef struct {
    const char* name;
    const char* description;
    AFMSchema* input_schema;
    AFMSchema* output_schema;
} AFMToolDefinition;

typedef struct {
    const char* tool_name;
    const char* arguments_json;     // JSON string of arguments
} AFMToolCall;

typedef void (*AFMToolHandler)(
    const char* tool_name,
    const char* arguments_json,
    char** out_result_json,
    void* user_ctx
);

// ============================================================================
// SESSION & GENERATION
// ============================================================================

typedef struct {
    void* _session;                 // Opaque LanguageModelSession handle
    bool is_active;
    size_t tokens_generated;
    size_t context_used;
} AFMSession;

typedef struct {
    float temperature;              // 0.0 - 2.0 (default: 0.7)
    size_t max_tokens;              // Max output tokens
    bool use_streaming;             // Stream tokens as generated
    AFMSchema* output_schema;       // For guided generation (NULL for free-form)
    AFMToolDefinition* tools;       // Available tools
    size_t tool_count;
    AFMToolHandler tool_handler;    // Callback for tool execution
    void* tool_handler_ctx;
} AFMGenerationOptions;

typedef void (*AFMStreamCallback)(
    const char* token,
    bool is_final,
    void* user_ctx
);

// ============================================================================
// APPLE FOUNDATION MODELS API
// ============================================================================

/**
 * Check if Apple Foundation Models is available
 * @param out_status Optional status output for detailed info
 * @return AFM_AVAILABLE if ready, error code otherwise
 */
AppleFoundationError afm_check_availability(AppleFoundationStatus* out_status);

/**
 * Get human-readable availability status
 */
const char* afm_status_description(AppleFoundationError error);

/**
 * Create provider instance for Convergio orchestrator
 * @return Provider instance or NULL if not available
 */
Provider* afm_provider_create(void);

/**
 * Create a new Foundation Models session
 * @param out_session Session to initialize
 * @return AFM_AVAILABLE on success
 */
AppleFoundationError afm_session_create(AFMSession* out_session);

/**
 * Destroy a session
 */
void afm_session_destroy(AFMSession* session);

/**
 * Generate response (non-streaming)
 * @param session Active session
 * @param prompt User prompt
 * @param system_prompt System prompt (can be NULL)
 * @param options Generation options
 * @param out_response Generated text (caller frees)
 * @return AFM_AVAILABLE on success
 */
AppleFoundationError afm_generate(
    AFMSession* session,
    const char* prompt,
    const char* system_prompt,
    const AFMGenerationOptions* options,
    char** out_response
);

/**
 * Generate response with streaming
 * @param session Active session
 * @param prompt User prompt
 * @param system_prompt System prompt (can be NULL)
 * @param options Generation options (use_streaming should be true)
 * @param callback Streaming callback
 * @param user_ctx User context for callback
 * @return AFM_AVAILABLE on success
 */
AppleFoundationError afm_generate_stream(
    AFMSession* session,
    const char* prompt,
    const char* system_prompt,
    const AFMGenerationOptions* options,
    AFMStreamCallback callback,
    void* user_ctx
);

/**
 * Generate structured output (guided generation)
 * @param session Active session
 * @param prompt User prompt
 * @param schema Output schema
 * @param out_json Generated JSON (caller frees)
 * @return AFM_AVAILABLE on success
 */
AppleFoundationError afm_generate_structured(
    AFMSession* session,
    const char* prompt,
    const AFMSchema* schema,
    char** out_json
);

/**
 * Simple generation helper (creates temporary session)
 * @param prompt User prompt
 * @param out_response Generated text (caller frees)
 * @return AFM_AVAILABLE on success
 */
AppleFoundationError afm_simple_generate(
    const char* prompt,
    char** out_response
);

// ============================================================================
// SCHEMA HELPERS
// ============================================================================

/**
 * Create a simple text response schema
 */
AFMSchema* afm_schema_text_response(void);

/**
 * Create a JSON object schema
 */
AFMSchema* afm_schema_create(
    const char* name,
    const char* description
);

/**
 * Add field to schema
 */
void afm_schema_add_field(
    AFMSchema* schema,
    const char* name,
    const char* description,
    AFMSchemaType type,
    bool required
);

/**
 * Add enum field to schema
 */
void afm_schema_add_enum(
    AFMSchema* schema,
    const char* name,
    const char* description,
    const char** values,
    size_t value_count,
    bool required
);

/**
 * Free schema
 */
void afm_schema_free(AFMSchema* schema);

// ============================================================================
// CONVERGIO INTEGRATION
// ============================================================================

/**
 * Check if Apple Foundation Models should be preferred over MLX
 * Returns true if AFM is available and suitable for the task
 * @param prompt_length Approximate prompt length
 * @param needs_tools Whether tool calling is needed
 * @return true if AFM should be used
 */
bool afm_should_prefer_over_mlx(size_t prompt_length, bool needs_tools);

/**
 * Get recommended provider for local inference
 * Returns "apple_foundation" or "mlx" based on availability and task
 */
const char* afm_get_recommended_local_provider(void);

/**
 * Initialize Apple Foundation Models for Convergio
 * Called during convergio startup
 * @return 0 on success (or graceful fallback), -1 on fatal error
 */
int afm_convergio_init(void);

/**
 * Shutdown Apple Foundation Models
 */
void afm_convergio_shutdown(void);

#endif // CONVERGIO_APPLE_FOUNDATION_H
