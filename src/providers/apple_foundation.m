/**
 * Apple Foundation Models Provider Implementation
 *
 * Objective-C bridge to the Swift FoundationModels wrapper.
 * Uses C-compatible functions exported from FoundationModelsBridge.swift.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#import <Foundation/Foundation.h>
#import <os/log.h>
#import <sys/sysctl.h>
#include "nous/apple_foundation.h"
#include "nous/nous.h"
#include "nous/provider.h"
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

// ============================================================================
// SWIFT BRIDGE FUNCTION DECLARATIONS
// ============================================================================

// These functions are exported from ConvergioAFM Swift library
extern int32_t swift_afm_check_availability(
    bool* outIsAvailable,
    bool* outIntelligenceEnabled,
    bool* outModelReady
) __attribute__((weak));

extern int32_t swift_afm_session_create(int64_t* outSessionId) __attribute__((weak));
extern void swift_afm_session_destroy(int64_t sessionId) __attribute__((weak));
extern int32_t swift_afm_session_set_instructions(int64_t sessionId, const char* instructions) __attribute__((weak));

extern int32_t swift_afm_generate(
    int64_t sessionId,
    const char* prompt,
    char** outResponse
) __attribute__((weak));

typedef void (*AFMSwiftStreamCallback)(const char*, bool, void*);
extern int32_t swift_afm_generate_stream(
    int64_t sessionId,
    const char* prompt,
    AFMSwiftStreamCallback callback,
    void* userCtx
) __attribute__((weak));

extern void swift_afm_free_string(char* str) __attribute__((weak));
extern int32_t swift_afm_get_model_info(char** outName, float* outSizeBillions) __attribute__((weak));

// ============================================================================
// PRIVATE STATE
// ============================================================================

static bool g_afm_initialized = false;
static os_log_t g_log = NULL;

// Provider-specific data
typedef struct {
    int64_t session_id;
    bool session_active;
    ProviderErrorInfo last_error;
} AFMProviderData;

static AFMProviderData g_afm_data = {0};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static bool is_apple_silicon(void) {
    int ret = 0;
    size_t size = sizeof(ret);
    if (sysctlbyname("hw.optional.arm64", &ret, &size, NULL, 0) == 0) {
        return ret == 1;
    }
    return false;
}

static bool is_macos_26_or_later(void) {
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    return version.majorVersion >= 26;
}

static bool swift_bridge_available(void) {
    // Check if Swift bridge functions are linked
    return swift_afm_check_availability != NULL;
}

// ============================================================================
// PUBLIC API - ERROR DESCRIPTIONS
// ============================================================================

const char* afm_status_description(AppleFoundationError error) {
    switch (error) {
        case AFM_AVAILABLE:
            return "Apple Foundation Models available";
        case AFM_ERR_NOT_MACOS_26:
            return "Requires macOS Tahoe (26.0) or later";
        case AFM_ERR_NOT_APPLE_SILICON:
            return "Requires Apple Silicon (M1/M2/M3/M4/M5)";
        case AFM_ERR_INTELLIGENCE_DISABLED:
            return "Apple Intelligence is not enabled in System Settings";
        case AFM_ERR_MODEL_NOT_READY:
            return "On-device model is downloading or not ready";
        case AFM_ERR_SESSION_FAILED:
            return "Failed to create language model session";
        case AFM_ERR_GENERATION_FAILED:
            return "Text generation failed";
        case AFM_ERR_TOOL_CALL_FAILED:
            return "Tool calling failed";
        case AFM_ERR_GUIDED_GEN_FAILED:
            return "Guided generation failed";
        default:
            return "Unknown error";
    }
}

// ============================================================================
// FOUNDATION MODELS API IMPLEMENTATION
// ============================================================================

AppleFoundationError afm_check_availability(AppleFoundationStatus* out_status) {
    if (!g_log) {
        g_log = os_log_create("com.convergio.cli", "apple_foundation");
    }

    AppleFoundationStatus status = {0};
    status.is_apple_silicon = is_apple_silicon();
    status.is_macos_26 = is_macos_26_or_later();

    // Get OS version string
    NSOperatingSystemVersion ver = [[NSProcessInfo processInfo] operatingSystemVersion];
    static char version_str[64];
    snprintf(version_str, sizeof(version_str), "macOS %ld.%ld.%ld",
             (long)ver.majorVersion, (long)ver.minorVersion, (long)ver.patchVersion);
    status.os_version = version_str;

    // Get chip name
    char chip[64] = {0};
    size_t chip_size = sizeof(chip);
    if (sysctlbyname("machdep.cpu.brand_string", chip, &chip_size, NULL, 0) == 0) {
        static char chip_name[64];
        strncpy(chip_name, chip, sizeof(chip_name) - 1);
        status.chip_name = chip_name;
    } else {
        status.chip_name = is_apple_silicon() ? "Apple Silicon" : "Intel";
    }

    if (!status.is_apple_silicon) {
        if (out_status) *out_status = status;
        return AFM_ERR_NOT_APPLE_SILICON;
    }

    if (!status.is_macos_26) {
        if (out_status) *out_status = status;
        os_log_info(g_log, "Apple Foundation Models: Not available (requires macOS 26+, current: %{public}s)",
                    status.os_version);
        return AFM_ERR_NOT_MACOS_26;
    }

    // Check Swift bridge availability
    if (!swift_bridge_available()) {
        os_log_info(g_log, "Apple Foundation Models: Swift bridge not available");
        if (out_status) *out_status = status;
        return AFM_ERR_NOT_MACOS_26;
    }

    // Call Swift bridge to check actual availability
    bool isAvailable = false, intelligenceEnabled = false, modelReady = false;
    int32_t result = swift_afm_check_availability(&isAvailable, &intelligenceEnabled, &modelReady);

    status.is_available = isAvailable;
    status.intelligence_enabled = intelligenceEnabled;
    status.model_ready = modelReady;
    status.model_size_billions = 3;  // Apple's on-device model is ~3B

    if (out_status) *out_status = status;

    // Convert Swift error code to our enum
    switch (result) {
        case 0: return AFM_AVAILABLE;
        case -3: return AFM_ERR_INTELLIGENCE_DISABLED;
        case -4: return AFM_ERR_MODEL_NOT_READY;
        default: return AFM_ERR_UNKNOWN;
    }
}

AppleFoundationError afm_session_create(AFMSession* out_session) {
    if (!out_session) return AFM_ERR_UNKNOWN;

    memset(out_session, 0, sizeof(AFMSession));

    if (!swift_bridge_available()) {
        return AFM_ERR_NOT_MACOS_26;
    }

    int64_t sessionId = 0;
    int32_t result = swift_afm_session_create(&sessionId);

    if (result != 0) {
        return AFM_ERR_SESSION_FAILED;
    }

    out_session->_session = (void*)(intptr_t)sessionId;
    out_session->is_active = true;
    return AFM_AVAILABLE;
}

void afm_session_destroy(AFMSession* session) {
    if (!session || !session->_session) return;

    if (swift_bridge_available() && swift_afm_session_destroy) {
        int64_t sessionId = (int64_t)(intptr_t)session->_session;
        swift_afm_session_destroy(sessionId);
    }

    session->_session = NULL;
    session->is_active = false;
}

AppleFoundationError afm_generate(
    AFMSession* session,
    const char* prompt,
    const char* system_prompt,
    const AFMGenerationOptions* options,
    char** out_response
) {
    if (!session || !session->_session || !prompt || !out_response) {
        return AFM_ERR_UNKNOWN;
    }

    (void)options; // Reserved for future use

    if (!swift_bridge_available()) {
        *out_response = NULL;
        return AFM_ERR_NOT_MACOS_26;
    }

    int64_t sessionId = (int64_t)(intptr_t)session->_session;

    // Set system prompt if provided
    if (system_prompt && swift_afm_session_set_instructions) {
        swift_afm_session_set_instructions(sessionId, system_prompt);
    }

    // Generate response
    char* response = NULL;
    int32_t result = swift_afm_generate(sessionId, prompt, &response);

    if (result != 0) {
        if (response && swift_afm_free_string) {
            swift_afm_free_string(response);
        }
        *out_response = NULL;
        return AFM_ERR_GENERATION_FAILED;
    }

    *out_response = response;
    session->tokens_generated += strlen(response) / 4;

    return AFM_AVAILABLE;
}

// Streaming callback wrapper
typedef struct {
    AFMStreamCallback user_callback;
    void* user_ctx;
} StreamCallbackContext;

static void stream_callback_wrapper(const char* content, bool isFinal, void* ctx) {
    StreamCallbackContext* wrapper = (StreamCallbackContext*)ctx;
    if (wrapper && wrapper->user_callback) {
        wrapper->user_callback(content, isFinal, wrapper->user_ctx);
    }
}

AppleFoundationError afm_generate_stream(
    AFMSession* session,
    const char* prompt,
    const char* system_prompt,
    const AFMGenerationOptions* options,
    AFMStreamCallback callback,
    void* user_ctx
) {
    if (!session || !session->_session || !prompt || !callback) {
        return AFM_ERR_UNKNOWN;
    }

    (void)options;

    if (!swift_bridge_available() || !swift_afm_generate_stream) {
        return AFM_ERR_NOT_MACOS_26;
    }

    int64_t sessionId = (int64_t)(intptr_t)session->_session;

    if (system_prompt && swift_afm_session_set_instructions) {
        swift_afm_session_set_instructions(sessionId, system_prompt);
    }

    StreamCallbackContext ctx = {
        .user_callback = callback,
        .user_ctx = user_ctx
    };

    int32_t result = swift_afm_generate_stream(
        sessionId,
        prompt,
        stream_callback_wrapper,
        &ctx
    );

    return result == 0 ? AFM_AVAILABLE : AFM_ERR_GENERATION_FAILED;
}

AppleFoundationError afm_generate_structured(
    AFMSession* session,
    const char* prompt,
    const AFMSchema* schema,
    char** out_json
) {
    (void)session;
    (void)prompt;
    (void)schema;
    if (out_json) *out_json = NULL;
    return AFM_ERR_GUIDED_GEN_FAILED;
}

AppleFoundationError afm_simple_generate(const char* prompt, char** out_response) {
    AFMSession session = {0};
    AppleFoundationError err = afm_session_create(&session);
    if (err != AFM_AVAILABLE) {
        if (out_response) *out_response = NULL;
        return err;
    }

    err = afm_generate(&session, prompt, NULL, NULL, out_response);
    afm_session_destroy(&session);
    return err;
}

// ============================================================================
// CONVERGIO PROVIDER INTERFACE IMPLEMENTATION
// ============================================================================

static ProviderError afm_provider_init(Provider* self) {
    if (!self) return PROVIDER_ERR_NOT_INITIALIZED;

    AFMProviderData* data = (AFMProviderData*)self->impl_data;
    if (!data) return PROVIDER_ERR_NOT_INITIALIZED;

    // Check availability
    AppleFoundationStatus status;
    AppleFoundationError err = afm_check_availability(&status);

    if (err != AFM_AVAILABLE) {
        data->last_error.code = PROVIDER_ERR_NOT_INITIALIZED;
        data->last_error.message = (char*)afm_status_description(err);
        return PROVIDER_ERR_NOT_INITIALIZED;
    }

    // Create session
    int64_t sessionId = 0;
    if (swift_afm_session_create && swift_afm_session_create(&sessionId) == 0) {
        data->session_id = sessionId;
        data->session_active = true;
    }

    self->initialized = true;
    return PROVIDER_OK;
}

static void afm_provider_shutdown(Provider* self) {
    if (!self) return;

    AFMProviderData* data = (AFMProviderData*)self->impl_data;
    if (data && data->session_active && swift_afm_session_destroy) {
        swift_afm_session_destroy(data->session_id);
        data->session_active = false;
        data->session_id = 0;
    }

    self->initialized = false;
}

static bool afm_provider_validate_key(Provider* self) {
    (void)self;
    // No API key needed for local on-device model
    return true;
}

static char* afm_provider_chat(Provider* self, const char* model,
                               const char* system, const char* user,
                               TokenUsage* usage) {
    if (!self || !user) return NULL;
    (void)model;  // Single model

    AFMProviderData* data = (AFMProviderData*)self->impl_data;
    if (!data || !data->session_active) return NULL;

    // Set system prompt if provided
    if (system && swift_afm_session_set_instructions) {
        swift_afm_session_set_instructions(data->session_id, system);
    }

    // Generate response
    char* response = NULL;
    if (swift_afm_generate && swift_afm_generate(data->session_id, user, &response) == 0) {
        if (usage) {
            usage->input_tokens = strlen(user) / 4;
            usage->output_tokens = response ? strlen(response) / 4 : 0;
            usage->estimated_cost = 0.0;  // Local model, no cost
        }
        return response;
    }

    data->last_error.code = PROVIDER_ERR_UNKNOWN;
    data->last_error.message = (char*)"Generation failed";
    return NULL;
}

static char* afm_provider_chat_with_tools(Provider* self, const char* model,
                                          const char* system, const char* user,
                                          ToolDefinition* tools, size_t tool_count,
                                          ToolCall** out_tool_calls, size_t* out_tool_count,
                                          TokenUsage* usage) {
    (void)tools;
    (void)tool_count;
    if (out_tool_calls) *out_tool_calls = NULL;
    if (out_tool_count) *out_tool_count = 0;

    // For now, just do regular chat (tool calling to be implemented)
    return afm_provider_chat(self, model, system, user, usage);
}

// Streaming handler context
typedef struct {
    StreamHandler* handler;
    Provider* provider;
} AFMStreamContext;

static void afm_stream_callback(const char* content, bool isFinal, void* ctx) {
    AFMStreamContext* stream_ctx = (AFMStreamContext*)ctx;
    if (!stream_ctx || !stream_ctx->handler) return;

    if (content && stream_ctx->handler->on_chunk) {
        stream_ctx->handler->on_chunk(content, isFinal, stream_ctx->handler->user_ctx);
    }

    if (isFinal && stream_ctx->handler->on_complete) {
        stream_ctx->handler->on_complete(content, stream_ctx->handler->user_ctx);
    }
}

static ProviderError afm_provider_stream_chat(Provider* self, const char* model,
                                              const char* system, const char* user,
                                              StreamHandler* handler, TokenUsage* usage) {
    if (!self || !user || !handler) return PROVIDER_ERR_INVALID_REQUEST;
    (void)model;

    AFMProviderData* data = (AFMProviderData*)self->impl_data;
    if (!data || !data->session_active) return PROVIDER_ERR_NOT_INITIALIZED;

    if (!swift_afm_generate_stream) {
        return PROVIDER_ERR_UNKNOWN;
    }

    // Set system prompt
    if (system && swift_afm_session_set_instructions) {
        swift_afm_session_set_instructions(data->session_id, system);
    }

    AFMStreamContext ctx = {
        .handler = handler,
        .provider = self
    };

    int32_t result = swift_afm_generate_stream(
        data->session_id,
        user,
        afm_stream_callback,
        &ctx
    );

    if (usage) {
        usage->input_tokens = strlen(user) / 4;
        usage->output_tokens = 0;  // Hard to track in streaming
        usage->estimated_cost = 0.0;  // Local model, no cost
    }

    return result == 0 ? PROVIDER_OK : PROVIDER_ERR_UNKNOWN;
}

static size_t afm_provider_estimate_tokens(Provider* self, const char* text) {
    (void)self;
    if (!text) return 0;
    // Rough estimate: ~4 characters per token
    return strlen(text) / 4;
}

static ProviderErrorInfo* afm_provider_get_last_error(Provider* self) {
    if (!self || !self->impl_data) return NULL;
    AFMProviderData* data = (AFMProviderData*)self->impl_data;
    return &data->last_error;
}

static ProviderError afm_provider_list_models(Provider* self,
                                              ModelConfig** out_models,
                                              size_t* out_count) {
    (void)self;

    if (!out_models || !out_count) return PROVIDER_ERR_INVALID_REQUEST;

    // Single on-device model
    ModelConfig* models = calloc(1, sizeof(ModelConfig));
    if (!models) return PROVIDER_ERR_NOT_INITIALIZED;

    models[0].id = "apple-foundation-3b";
    models[0].display_name = "Apple Foundation Model (3B)";
    models[0].provider = PROVIDER_APPLE_FOUNDATION;
    models[0].context_window = 32768;
    models[0].max_output = 4096;
    models[0].input_cost_per_mtok = 0.0;
    models[0].output_cost_per_mtok = 0.0;
    models[0].supports_vision = false;
    models[0].supports_streaming = true;
    models[0].supports_tools = true;

    *out_models = models;
    *out_count = 1;
    return PROVIDER_OK;
}

// ============================================================================
// PROVIDER FACTORY
// ============================================================================

Provider* afm_provider_create(void) {
    // Check basic requirements first
    if (!is_apple_silicon()) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "AFM provider not created: not Apple Silicon");
        return NULL;
    }

    if (!is_macos_26_or_later()) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "AFM provider not created: requires macOS 26+");
        return NULL;
    }

    if (!swift_bridge_available()) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "AFM provider not created: Swift bridge not available");
        return NULL;
    }

    Provider* provider = calloc(1, sizeof(Provider));
    if (!provider) return NULL;

    provider->type = PROVIDER_APPLE_FOUNDATION;
    provider->name = "Apple Foundation Models";
    provider->api_key_env = NULL;  // No API key needed
    provider->base_url = NULL;     // Local inference
    provider->initialized = false;

    // Core operations
    provider->init = afm_provider_init;
    provider->shutdown = afm_provider_shutdown;
    provider->validate_key = afm_provider_validate_key;

    // Chat operations
    provider->chat = afm_provider_chat;
    provider->chat_with_tools = afm_provider_chat_with_tools;
    provider->stream_chat = afm_provider_stream_chat;

    // Utilities
    provider->estimate_tokens = afm_provider_estimate_tokens;
    provider->get_last_error = afm_provider_get_last_error;
    provider->list_models = afm_provider_list_models;

    provider->impl_data = &g_afm_data;

    LOG_INFO(LOG_CAT_SYSTEM, "Apple Foundation Models provider created (on-device 3B)");
    return provider;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool afm_should_prefer_over_mlx(size_t prompt_length, bool needs_tools) {
    AppleFoundationStatus status;
    if (afm_check_availability(&status) != AFM_AVAILABLE) {
        return false;
    }

    if (needs_tools) return true;
    if (prompt_length < 8000) return true;

    return false;
}

const char* afm_get_recommended_local_provider(void) {
    AppleFoundationStatus status;
    if (afm_check_availability(&status) == AFM_AVAILABLE) {
        return "apple_foundation";
    }
    return "mlx";
}

int afm_convergio_init(void) {
    if (g_afm_initialized) return 0;

    g_log = os_log_create("com.convergio.cli", "apple_foundation");

    AppleFoundationStatus status;
    AppleFoundationError err = afm_check_availability(&status);

    if (err == AFM_AVAILABLE) {
        os_log_info(g_log, "Apple Foundation Models: Available (%{public}s, %{public}s)",
                    status.os_version, status.chip_name);
    } else {
        os_log_info(g_log, "Apple Foundation Models: %{public}s", afm_status_description(err));
    }

    g_afm_initialized = true;
    return 0;
}

void afm_convergio_shutdown(void) {
    g_afm_initialized = false;
}

// Schema helpers
AFMSchema* afm_schema_text_response(void) {
    return afm_schema_create("TextResponse", "A simple text response");
}

AFMSchema* afm_schema_create(const char* name, const char* description) {
    AFMSchema* schema = calloc(1, sizeof(AFMSchema));
    if (!schema) return NULL;
    schema->name = name;
    schema->description = description;
    return schema;
}

void afm_schema_add_field(AFMSchema* schema, const char* name, const char* description,
                          AFMSchemaType type, bool required) {
    (void)schema; (void)name; (void)description; (void)type; (void)required;
}

void afm_schema_add_enum(AFMSchema* schema, const char* name, const char* description,
                         const char** values, size_t value_count, bool required) {
    (void)schema; (void)name; (void)description; (void)values; (void)value_count; (void)required;
}

void afm_schema_free(AFMSchema* schema) {
    free(schema);
}
