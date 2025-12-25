/**
 * Apple Foundation Models Provider Implementation
 *
 * Objective-C implementation for interfacing with Apple's
 * Foundation Models framework (macOS 26+).
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

// Compile-time check for macOS version
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000

@import FoundationModels;

#define AFM_SUPPORTED 1

#else

#define AFM_SUPPORTED 0

#endif

// ============================================================================
// PRIVATE STATE
// ============================================================================

static bool g_afm_initialized = false;
static os_log_t g_log = NULL;

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

static char* ns_string_to_c(NSString* str) {
    if (!str) return NULL;
    const char* cstr = [str UTF8String];
    if (!cstr) return NULL;
    return strdup(cstr);
}

// ============================================================================
// PUBLIC API - AVAILABILITY CHECK
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
    NSString* verStr = [NSString stringWithFormat:@"macOS %ld.%ld.%ld",
                        (long)ver.majorVersion, (long)ver.minorVersion, (long)ver.patchVersion];
    status.os_version = [verStr UTF8String];

    // Get chip name
    char chip[64] = {0};
    size_t chip_size = sizeof(chip);
    if (sysctlbyname("machdep.cpu.brand_string", chip, &chip_size, NULL, 0) == 0) {
        status.chip_name = strdup(chip);
    } else {
        status.chip_name = "Apple Silicon";
    }

    if (!status.is_apple_silicon) {
        if (out_status) *out_status = status;
        return AFM_ERR_NOT_APPLE_SILICON;
    }

    if (!status.is_macos_26) {
        if (out_status) *out_status = status;
        return AFM_ERR_NOT_MACOS_26;
    }

#if AFM_SUPPORTED
    // Check if Apple Intelligence is enabled and model is ready
    @autoreleasepool {
        @try {
            // Check model availability
            LanguageModelAvailability *availability = [[LanguageModelAvailability alloc] init];

            switch (availability.availability) {
                case LanguageModelAvailabilityAvailable:
                    status.is_available = true;
                    status.intelligence_enabled = true;
                    status.model_ready = true;
                    status.model_size_billions = 3;
                    break;

                case LanguageModelAvailabilityUnavailable:
                    status.is_available = false;
                    status.intelligence_enabled = false;
                    if (out_status) *out_status = status;
                    return AFM_ERR_INTELLIGENCE_DISABLED;

                case LanguageModelAvailabilityModelNotReady:
                    status.is_available = true;
                    status.intelligence_enabled = true;
                    status.model_ready = false;
                    if (out_status) *out_status = status;
                    return AFM_ERR_MODEL_NOT_READY;
            }
        } @catch (NSException *exception) {
            os_log_error(g_log, "Exception checking availability: %{public}@", exception);
            if (out_status) *out_status = status;
            return AFM_ERR_UNKNOWN;
        }
    }

    if (out_status) *out_status = status;
    return AFM_AVAILABLE;
#else
    status.is_available = false;
    if (out_status) *out_status = status;
    return AFM_ERR_NOT_MACOS_26;
#endif
}

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
// SESSION MANAGEMENT
// ============================================================================

#if AFM_SUPPORTED

AppleFoundationError afm_session_create(AFMSession* out_session) {
    if (!out_session) return AFM_ERR_UNKNOWN;

    memset(out_session, 0, sizeof(AFMSession));

    @autoreleasepool {
        @try {
            LanguageModelSession *session = [[LanguageModelSession alloc] init];
            out_session->_session = (__bridge_retained void*)session;
            out_session->is_active = true;
            return AFM_AVAILABLE;
        } @catch (NSException *exception) {
            os_log_error(g_log, "Failed to create session: %{public}@", exception);
            return AFM_ERR_SESSION_FAILED;
        }
    }
}

void afm_session_destroy(AFMSession* session) {
    if (!session || !session->_session) return;

    @autoreleasepool {
        LanguageModelSession *lmSession = (__bridge_transfer LanguageModelSession*)session->_session;
        lmSession = nil;
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

    @autoreleasepool {
        @try {
            LanguageModelSession *lmSession = (__bridge LanguageModelSession*)session->_session;
            NSString *promptStr = [NSString stringWithUTF8String:prompt];

            // Build instructions if system prompt provided
            if (system_prompt) {
                NSString *instructions = [NSString stringWithUTF8String:system_prompt];
                lmSession.instructions = instructions;
            }

            // Run synchronous generation
            __block NSString *responseStr = nil;
            __block NSError *error = nil;

            dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

            [lmSession respondTo:promptStr completionHandler:^(LanguageModelSession.Response *response, NSError *err) {
                if (err) {
                    error = err;
                } else {
                    responseStr = response.content;
                }
                dispatch_semaphore_signal(semaphore);
            }];

            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);

            if (error) {
                os_log_error(g_log, "Generation error: %{public}@", error);
                return AFM_ERR_GENERATION_FAILED;
            }

            *out_response = ns_string_to_c(responseStr);
            session->tokens_generated += responseStr.length / 4; // Rough estimate

            return AFM_AVAILABLE;

        } @catch (NSException *exception) {
            os_log_error(g_log, "Generation exception: %{public}@", exception);
            return AFM_ERR_GENERATION_FAILED;
        }
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

    @autoreleasepool {
        @try {
            LanguageModelSession *lmSession = (__bridge LanguageModelSession*)session->_session;
            NSString *promptStr = [NSString stringWithUTF8String:prompt];

            if (system_prompt) {
                lmSession.instructions = [NSString stringWithUTF8String:system_prompt];
            }

            __block NSError *streamError = nil;
            dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);

            // Use streaming API
            [lmSession streamResponseTo:promptStr handler:^(NSString *partialContent, BOOL isFinal, NSError *error) {
                if (error) {
                    streamError = error;
                    dispatch_semaphore_signal(semaphore);
                    return;
                }

                if (partialContent) {
                    callback([partialContent UTF8String], isFinal, user_ctx);
                }

                if (isFinal) {
                    dispatch_semaphore_signal(semaphore);
                }
            }];

            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);

            if (streamError) {
                os_log_error(g_log, "Streaming error: %{public}@", streamError);
                return AFM_ERR_GENERATION_FAILED;
            }

            return AFM_AVAILABLE;

        } @catch (NSException *exception) {
            os_log_error(g_log, "Streaming exception: %{public}@", exception);
            return AFM_ERR_GENERATION_FAILED;
        }
    }
}

AppleFoundationError afm_simple_generate(const char* prompt, char** out_response) {
    AFMSession session = {0};
    AppleFoundationError err = afm_session_create(&session);
    if (err != AFM_AVAILABLE) {
        return err;
    }

    err = afm_generate(&session, prompt, NULL, NULL, out_response);
    afm_session_destroy(&session);
    return err;
}

#else // !AFM_SUPPORTED

AppleFoundationError afm_session_create(AFMSession* out_session) {
    return AFM_ERR_NOT_MACOS_26;
}

void afm_session_destroy(AFMSession* session) {
    // No-op
}

AppleFoundationError afm_generate(
    AFMSession* session,
    const char* prompt,
    const char* system_prompt,
    const AFMGenerationOptions* options,
    char** out_response
) {
    return AFM_ERR_NOT_MACOS_26;
}

AppleFoundationError afm_generate_stream(
    AFMSession* session,
    const char* prompt,
    const char* system_prompt,
    const AFMGenerationOptions* options,
    AFMStreamCallback callback,
    void* user_ctx
) {
    return AFM_ERR_NOT_MACOS_26;
}

AppleFoundationError afm_simple_generate(const char* prompt, char** out_response) {
    return AFM_ERR_NOT_MACOS_26;
}

#endif // AFM_SUPPORTED

// ============================================================================
// CONVERGIO PROVIDER INTEGRATION
// ============================================================================

typedef struct {
    AFMSession session;
    char* last_error;
} AFMProviderData;

static int afm_provider_init(Provider* provider) {
    AFMProviderData* data = calloc(1, sizeof(AFMProviderData));
    if (!data) return -1;

    AppleFoundationError err = afm_session_create(&data->session);
    if (err != AFM_AVAILABLE) {
        data->last_error = strdup(afm_status_description(err));
        // Don't fail - allow graceful degradation
    }

    provider->data = data;
    return 0;
}

static void afm_provider_cleanup(Provider* provider) {
    if (!provider || !provider->data) return;

    AFMProviderData* data = (AFMProviderData*)provider->data;
    afm_session_destroy(&data->session);
    free(data->last_error);
    free(data);
    provider->data = NULL;
}

static int afm_provider_complete(
    Provider* provider,
    const ProviderRequest* request,
    ProviderResponse* response
) {
    if (!provider || !provider->data || !request || !response) {
        return -1;
    }

    AFMProviderData* data = (AFMProviderData*)provider->data;

    // Ensure session is active
    if (!data->session.is_active) {
        AppleFoundationError err = afm_session_create(&data->session);
        if (err != AFM_AVAILABLE) {
            response->error_message = strdup(afm_status_description(err));
            return -1;
        }
    }

    char* result = NULL;
    AppleFoundationError err = afm_generate(
        &data->session,
        request->prompt,
        request->system_prompt,
        NULL,
        &result
    );

    if (err != AFM_AVAILABLE) {
        response->error_message = strdup(afm_status_description(err));
        return -1;
    }

    response->content = result;
    response->input_tokens = strlen(request->prompt) / 4;  // Estimate
    response->output_tokens = strlen(result) / 4;
    response->cost_usd = 0.0;  // On-device, no cost

    return 0;
}

static void afm_provider_stream_callback_wrapper(
    const char* token,
    bool is_final,
    void* ctx
) {
    ProviderStreamContext* stream_ctx = (ProviderStreamContext*)ctx;
    if (stream_ctx && stream_ctx->callback) {
        stream_ctx->callback(token, stream_ctx->user_data);
    }
}

static int afm_provider_complete_stream(
    Provider* provider,
    const ProviderRequest* request,
    ProviderStreamCallback callback,
    void* user_data,
    ProviderResponse* response
) {
    if (!provider || !provider->data || !request) {
        return -1;
    }

    AFMProviderData* data = (AFMProviderData*)provider->data;

    if (!data->session.is_active) {
        AppleFoundationError err = afm_session_create(&data->session);
        if (err != AFM_AVAILABLE) {
            if (response) response->error_message = strdup(afm_status_description(err));
            return -1;
        }
    }

    ProviderStreamContext ctx = {
        .callback = callback,
        .user_data = user_data
    };

    AppleFoundationError err = afm_generate_stream(
        &data->session,
        request->prompt,
        request->system_prompt,
        NULL,
        afm_provider_stream_callback_wrapper,
        &ctx
    );

    if (err != AFM_AVAILABLE) {
        if (response) response->error_message = strdup(afm_status_description(err));
        return -1;
    }

    if (response) {
        response->cost_usd = 0.0;
    }

    return 0;
}

Provider* afm_provider_create(void) {
    AppleFoundationStatus status;
    AppleFoundationError err = afm_check_availability(&status);

    if (err != AFM_AVAILABLE) {
        LOG_WARN("Apple Foundation Models not available: %s", afm_status_description(err));
        return NULL;
    }

    Provider* provider = calloc(1, sizeof(Provider));
    if (!provider) return NULL;

    provider->name = "apple_foundation";
    provider->display_name = "Apple Intelligence";
    provider->model_id = "apple-foundation-3b";
    provider->supports_streaming = true;
    provider->supports_tools = true;
    provider->is_local = true;
    provider->cost_per_input_token = 0.0;
    provider->cost_per_output_token = 0.0;

    provider->init = afm_provider_init;
    provider->cleanup = afm_provider_cleanup;
    provider->complete = afm_provider_complete;
    provider->complete_stream = afm_provider_complete_stream;

    if (provider->init(provider) != 0) {
        free(provider);
        return NULL;
    }

    LOG_INFO("Apple Foundation Models provider created (3B on-device model)");
    return provider;
}

// ============================================================================
// INTEGRATION HELPERS
// ============================================================================

bool afm_should_prefer_over_mlx(size_t prompt_length, bool needs_tools) {
    AppleFoundationStatus status;
    if (afm_check_availability(&status) != AFM_AVAILABLE) {
        return false;
    }

    // Prefer AFM when:
    // 1. Tool calling is needed (AFM has better tool support)
    // 2. Prompts are moderate length (AFM optimized for typical queries)
    if (needs_tools) return true;
    if (prompt_length < 8000) return true;  // AFM optimized for shorter contexts

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
        LOG_INFO("Apple Foundation Models: Available (%s, %s)",
                 status.os_version, status.chip_name);
        g_afm_initialized = true;
        return 0;
    } else {
        LOG_INFO("Apple Foundation Models: %s", afm_status_description(err));
        // Not a fatal error - graceful degradation to MLX
        return 0;
    }
}

void afm_convergio_shutdown(void) {
    g_afm_initialized = false;
}
