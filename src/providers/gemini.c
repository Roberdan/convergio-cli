/**
 * CONVERGIO GOOGLE GEMINI PROVIDER ADAPTER
 *
 * Implements the Provider interface for Google Gemini models
 * Supports Gemini 3 Pro, Ultra, Flash
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "nous/provider_common.h"
#include "nous/config.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <curl/curl.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define GEMINI_API_BASE "https://generativelanguage.googleapis.com/v1beta/models"
#define MAX_RESPONSE_SIZE (256 * 1024)
#define DEFAULT_MAX_TOKENS 8192  // Fallback only

// ============================================================================
// INTERNAL DATA STRUCTURES
// ============================================================================

// ResponseBuffer now from provider_common.h

typedef struct {
    bool initialized;
    CURL* curl;
    ProviderErrorInfo last_error;
    pthread_mutex_t mutex;
    volatile sig_atomic_t request_cancelled;
} GeminiProviderData;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static ProviderError gemini_init(Provider* self);
static void gemini_shutdown(Provider* self);
static bool gemini_validate_key(Provider* self);
static char* gemini_chat(Provider* self, const char* model, const char* system,
                         const char* user, TokenUsage* usage);
static char* gemini_chat_with_tools(Provider* self, const char* model, const char* system,
                                    const char* user, ToolDefinition* tools, size_t tool_count,
                                    ToolCall** out_tool_calls, size_t* out_tool_count,
                                    TokenUsage* usage);
static ProviderError gemini_stream_chat(Provider* self, const char* model, const char* system,
                                        const char* user, StreamHandler* handler, TokenUsage* usage);
static size_t gemini_estimate_tokens(Provider* self, const char* text);
static ProviderErrorInfo* gemini_get_last_error(Provider* self);
static ProviderError gemini_list_models(Provider* self, ModelConfig** out_models, size_t* out_count);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// write_callback now from provider_common.h (provider_write_callback)

static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow) {
    (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    GeminiProviderData* data = (GeminiProviderData*)clientp;
    if (data && data->request_cancelled) {
        return 1;
    }
    return 0;
}

// JSON escape helper
static char* json_escape(const char* str) {
    if (!str) return strdup("");

    size_t len = strlen(str);
    size_t escaped_len = len * 6 + 1;
    char* escaped = malloc(escaped_len);
    if (!escaped) return NULL;

    char* out = escaped;
    const unsigned char* p = (const unsigned char*)str;

    while (*p) {
        if (*p < 128) {
            switch (*p) {
                case '"':  *out++ = '\\'; *out++ = '"'; break;
                case '\\': *out++ = '\\'; *out++ = '\\'; break;
                case '\n': *out++ = '\\'; *out++ = 'n'; break;
                case '\r': *out++ = '\\'; *out++ = 'r'; break;
                case '\t': *out++ = '\\'; *out++ = 't'; break;
                default:
                    if (*p < 32) {
                        int written = snprintf(out, 7, "\\u%04x", *p);
                        if (written > 0) out += (size_t)written;
                    } else {
                        *out++ = (char)*p;
                    }
            }
            p++;
        } else {
            *out++ = (char)*p++;
        }
    }
    *out = '\0';
    return escaped;
}

// Extract content from Gemini response
// Format: {"candidates":[{"content":{"parts":[{"text":"..."}]}}]}
static char* extract_response_text(const char* json) {
    const char* text_key = "\"text\":";
    const char* found = strstr(json, text_key);
    if (!found) return NULL;

    found += strlen(text_key);
    while (*found && isspace(*found)) found++;

    if (*found != '"') return NULL;
    found++;

    const char* start = found;
    const char* end = start;
    int escape = 0;
    while (*end) {
        if (escape) {
            escape = 0;
            end++;
            continue;
        }
        if (*end == '\\') {
            escape = 1;
            end++;
            continue;
        }
        if (*end == '"') {
            break;
        }
        end++;
    }

    if (*end != '"') return NULL;

    size_t len = (size_t)(end - start);
    char* result = malloc(len + 1);
    if (!result) return NULL;

    // Unescape
    char* out = result;
    for (const char* p = start; p < end; p++) {
        if (*p == '\\' && p + 1 < end) {
            p++;
            switch (*p) {
                case 'n': *out++ = '\n'; break;
                case 'r': *out++ = '\r'; break;
                case 't': *out++ = '\t'; break;
                case '"': *out++ = '"'; break;
                case '\\': *out++ = '\\'; break;
                default: *out++ = *p;
            }
        } else {
            *out++ = *p;
        }
    }
    *out = '\0';

    return result;
}

// Extract token usage from Gemini response
static void extract_token_usage(const char* json, TokenUsage* usage) {
    if (!json || !usage) return;

    const char* usage_key = "\"usageMetadata\":";
    const char* found = strstr(json, usage_key);
    if (!found) return;

    // promptTokenCount
    const char* prompt = strstr(found, "\"promptTokenCount\":");
    if (prompt) {
        prompt += strlen("\"promptTokenCount\":");
        while (*prompt && isspace(*prompt)) prompt++;
        usage->input_tokens = (size_t)atol(prompt);
    }

    // candidatesTokenCount
    const char* candidates = strstr(found, "\"candidatesTokenCount\":");
    if (candidates) {
        candidates += strlen("\"candidatesTokenCount\":");
        while (*candidates && isspace(*candidates)) candidates++;
        usage->output_tokens = (size_t)atol(candidates);
    }

    // cachedContentTokenCount
    const char* cached = strstr(found, "\"cachedContentTokenCount\":");
    if (cached) {
        cached += strlen("\"cachedContentTokenCount\":");
        while (*cached && isspace(*cached)) cached++;
        usage->cached_tokens = (size_t)atol(cached);
    }
}

// Build Gemini API URL
static char* build_api_url(const char* model, const char* api_key) {
    // Format: https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={key}
    const char* model_name = model ? model : "gemini-2.0-flash";

    size_t url_len = strlen(GEMINI_API_BASE) + strlen(model_name) + strlen(api_key) + 64;
    char* url = malloc(url_len);
    if (!url) return NULL;

    snprintf(url, url_len, "%s/%s:generateContent?key=%s",
             GEMINI_API_BASE, model_name, api_key);
    return url;
}

// ============================================================================
// PROVIDER INTERFACE IMPLEMENTATION
// ============================================================================

static ProviderError gemini_init(Provider* self) {
    if (!self) return PROVIDER_ERR_INVALID_REQUEST;

    GeminiProviderData* data = (GeminiProviderData*)self->impl_data;
    if (!data) return PROVIDER_ERR_INVALID_REQUEST;

    pthread_mutex_lock(&data->mutex);

    if (data->initialized) {
        pthread_mutex_unlock(&data->mutex);
        return PROVIDER_OK;
    }

    // Check API key
    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key || strlen(api_key) == 0) {
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("GEMINI_API_KEY not set");
        pthread_mutex_unlock(&data->mutex);
        return PROVIDER_ERR_AUTH;
    }

    // Note: curl_global_init is called once in main.c at startup
    data->initialized = true;
    self->initialized = true;

    pthread_mutex_unlock(&data->mutex);

    LOG_INFO(LOG_CAT_API, "Gemini provider initialized");
    return PROVIDER_OK;
}

static void gemini_shutdown(Provider* self) {
    if (!self) return;

    GeminiProviderData* data = (GeminiProviderData*)self->impl_data;
    if (!data) return;

    pthread_mutex_lock(&data->mutex);

    if (data->initialized) {
        curl_global_cleanup();
        data->initialized = false;
        self->initialized = false;
    }

    free(data->last_error.message);
    free(data->last_error.provider_code);
    memset(&data->last_error, 0, sizeof(data->last_error));

    pthread_mutex_unlock(&data->mutex);
    pthread_mutex_destroy(&data->mutex);

    LOG_INFO(LOG_CAT_API, "Gemini provider shutdown");
}

static bool gemini_validate_key(Provider* self) {
    (void)self;
    const char* api_key = getenv("GEMINI_API_KEY");
    return (api_key && strlen(api_key) > 0);
}

static char* gemini_chat(Provider* self, const char* model, const char* system,
                         const char* user, TokenUsage* usage) {
    if (!self || !user) return NULL;

    GeminiProviderData* data = (GeminiProviderData*)self->impl_data;
    if (!data) return NULL;

    // Ensure initialized
    if (!data->initialized) {
        ProviderError err = gemini_init(self);
        if (err != PROVIDER_OK) return NULL;
    }

    // Get API key
    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("GEMINI_API_KEY not set");
        return NULL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup("Failed to create curl handle");
        return NULL;
    }

    // Build URL
    char* url = build_api_url(model, api_key);
    if (!url) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Build JSON request (Gemini format)
    char* escaped_system = json_escape(system ? system : "");
    char* escaped_user = json_escape(user);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        free(url);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Gemini uses a different message format
    // Include system prompt in the first user message if provided
    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        free(url);
        curl_easy_cleanup(curl);
        return NULL;
    }

    StyleSettings style = convergio_get_style_settings();
    if (system && strlen(system) > 0) {
        snprintf(json_body, json_size,
            "{"
            "\"systemInstruction\": {\"parts\": [{\"text\": \"%s\"}]},"
            "\"contents\": [{\"parts\": [{\"text\": \"%s\"}]}],"
            "\"generationConfig\": {\"maxOutputTokens\": %d, \"temperature\": %.2f}"
            "}",
            escaped_system, escaped_user, style.max_tokens, style.temperature);
    } else {
        snprintf(json_body, json_size,
            "{"
            "\"contents\": [{\"parts\": [{\"text\": \"%s\"}]}],"
            "\"generationConfig\": {\"maxOutputTokens\": %d, \"temperature\": %.2f}"
            "}",
            escaped_user, style.max_tokens, style.temperature);
    }

    free(escaped_system);
    free(escaped_user);

    // Setup response buffer
    ResponseBuffer response = {
        .data = malloc(4096),
        .size = 0,
        .capacity = 4096
    };
    if (!response.data) {
        free(json_body);
        free(url);
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.data[0] = '\0';

    // Headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Setup curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, provider_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);

    data->request_cancelled = 0;

    LOG_DEBUG(LOG_CAT_API, "Gemini API call: model=%s", model ? model : "gemini-2.0-flash");
    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    data->last_error.http_status = (int)http_code;

    char* result = NULL;

    if (res == CURLE_ABORTED_BY_CALLBACK) {
        data->last_error.code = PROVIDER_ERR_TIMEOUT;
        data->last_error.message = strdup("Request cancelled");
    } else if (res != CURLE_OK) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup(curl_easy_strerror(res));
    } else if (http_code != 200) {
        data->last_error.code = provider_map_http_error(http_code);
        data->last_error.message = strdup(response.data ? response.data : "Unknown error");
        LOG_WARN(LOG_CAT_API, "Gemini API error: HTTP %ld -> %d", http_code, data->last_error.code);
    } else {
        result = extract_response_text(response.data);
        if (!result) {
            data->last_error.code = PROVIDER_ERR_INVALID_REQUEST;
            data->last_error.message = strdup("Failed to parse response");
        } else if (usage) {
            memset(usage, 0, sizeof(TokenUsage));
            extract_token_usage(response.data, usage);
            usage->estimated_cost = model_estimate_cost(model, usage->input_tokens, usage->output_tokens);
            LOG_DEBUG(LOG_CAT_COST, "Tokens: in=%zu out=%zu cost=$%.6f",
                     usage->input_tokens, usage->output_tokens, usage->estimated_cost);
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(json_body);
    free(url);
    free(response.data);

    return result;
}

static char* gemini_chat_with_tools(Provider* self, const char* model, const char* system,
                                    const char* user, ToolDefinition* tools, size_t tool_count,
                                    ToolCall** out_tool_calls, size_t* out_tool_count,
                                    TokenUsage* usage) {
    if (out_tool_calls) *out_tool_calls = NULL;
    if (out_tool_count) *out_tool_count = 0;

    // If no tools, fall back to regular chat
    if (!tools || tool_count == 0) {
        return gemini_chat(self, model, system, user, usage);
    }

    if (!self || !user) return NULL;

    GeminiProviderData* data = (GeminiProviderData*)self->impl_data;
    if (!data) return NULL;

    if (!data->initialized) {
        ProviderError err = gemini_init(self);
        if (err != PROVIDER_OK) return NULL;
    }

    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("GEMINI_API_KEY not set");
        return NULL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup("Failed to create curl handle");
        return NULL;
    }

    // Build URL
    char* url = build_api_url(model, api_key);
    if (!url) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Build tools JSON
    char* tools_json = build_gemini_tools_json(tools, tool_count);
    if (!tools_json) {
        free(url);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Build JSON request with tools
    char* escaped_system = json_escape(system ? system : "");
    char* escaped_user = json_escape(user);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        free(tools_json);
        free(url);
        curl_easy_cleanup(curl);
        return NULL;
    }

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + strlen(tools_json) + 2048;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        free(tools_json);
        free(url);
        curl_easy_cleanup(curl);
        return NULL;
    }

    StyleSettings style = convergio_get_style_settings();
    if (system && strlen(system) > 0) {
        snprintf(json_body, json_size,
            "{"
            "\"systemInstruction\": {\"parts\": [{\"text\": \"%s\"}]},"
            "\"contents\": [{\"parts\": [{\"text\": \"%s\"}]}],"
            "\"tools\": %s,"
            "\"generationConfig\": {\"maxOutputTokens\": %d, \"temperature\": %.2f}"
            "}",
            escaped_system, escaped_user, tools_json, style.max_tokens, style.temperature);
    } else {
        snprintf(json_body, json_size,
            "{"
            "\"contents\": [{\"parts\": [{\"text\": \"%s\"}]}],"
            "\"tools\": %s,"
            "\"generationConfig\": {\"maxOutputTokens\": %d, \"temperature\": %.2f}"
            "}",
            escaped_user, tools_json, style.max_tokens, style.temperature);
    }

    free(escaped_system);
    free(escaped_user);
    free(tools_json);

    // Setup response buffer
    ResponseBuffer response = {
        .data = malloc(4096),
        .size = 0,
        .capacity = 4096
    };
    if (!response.data) {
        free(json_body);
        free(url);
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.data[0] = '\0';

    // Headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Setup curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, provider_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);

    data->request_cancelled = 0;

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(json_body);
    free(url);

    if (res != CURLE_OK) {
        free(response.data);
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup(curl_easy_strerror(res));
        return NULL;
    }

    // Parse for tool calls
    size_t tc_count = 0;
    ToolCall* tc = parse_gemini_tool_calls(response.data, &tc_count);
    if (tc && tc_count > 0) {
        if (out_tool_calls) *out_tool_calls = tc;
        if (out_tool_count) *out_tool_count = tc_count;
    }

    // Extract text response
    char* result = extract_response_text(response.data);
    if (usage) {
        memset(usage, 0, sizeof(TokenUsage));
        extract_token_usage(response.data, usage);
    }

    free(response.data);
    return result;
}

// Build streaming API URL (Gemini uses streamGenerateContent)
static char* build_stream_api_url(const char* model, const char* api_key) {
    const char* model_name = model ? model : "gemini-2.0-flash";
    size_t url_len = strlen(GEMINI_API_BASE) + strlen(model_name) + strlen(api_key) + 80;
    char* url = malloc(url_len);
    if (!url) return NULL;
    snprintf(url, url_len, "%s/%s:streamGenerateContent?alt=sse&key=%s",
             GEMINI_API_BASE, model_name, api_key);
    return url;
}

// Bridge context for streaming callbacks (Gemini)
typedef struct {
    StreamHandler* handler;
    TokenUsage* usage;
    ProviderError error;
} GeminiStreamBridge;

// Bridge callback for stream chunks
static void gemini_stream_chunk_bridge(const char* chunk, void* ctx) {
    GeminiStreamBridge* bridge = (GeminiStreamBridge*)ctx;
    if (bridge && bridge->handler && bridge->handler->on_chunk) {
        char* unescaped = stream_unescape_json(chunk);
        if (unescaped) {
            bridge->handler->on_chunk(unescaped, false, bridge->handler->user_ctx);
            free(unescaped);
        } else {
            bridge->handler->on_chunk(chunk, false, bridge->handler->user_ctx);
        }
    }
}

// Bridge callback for stream completion
static void gemini_stream_complete_bridge(const char* full_response, TokenUsage* usage, void* ctx) {
    GeminiStreamBridge* bridge = (GeminiStreamBridge*)ctx;
    if (bridge) {
        if (bridge->usage && usage) {
            *bridge->usage = *usage;
        }
        if (bridge->handler) {
            if (bridge->handler->on_chunk) {
                bridge->handler->on_chunk("", true, bridge->handler->user_ctx);
            }
            if (bridge->handler->on_complete) {
                bridge->handler->on_complete(full_response, bridge->handler->user_ctx);
            }
        }
    }
}

// Bridge callback for stream errors
static void gemini_stream_error_bridge(ProviderError error, const char* message, void* ctx) {
    GeminiStreamBridge* bridge = (GeminiStreamBridge*)ctx;
    if (bridge) {
        bridge->error = error;
        if (bridge->handler && bridge->handler->on_error) {
            bridge->handler->on_error(message ? message : "Stream error", bridge->handler->user_ctx);
        }
    }
}

static ProviderError gemini_stream_chat(Provider* self, const char* model, const char* system,
                                        const char* user, StreamHandler* handler, TokenUsage* usage) {
    if (!self || !user) return PROVIDER_ERR_INVALID_REQUEST;

    GeminiProviderData* data = (GeminiProviderData*)self->impl_data;
    if (!data) return PROVIDER_ERR_INVALID_REQUEST;

    if (!data->initialized) {
        ProviderError err = gemini_init(self);
        if (err != PROVIDER_OK) return err;
    }

    // Get API key
    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("GEMINI_API_KEY not set");
        return PROVIDER_ERR_AUTH;
    }

    // Build streaming URL
    char* url = build_stream_api_url(model, api_key);
    if (!url) {
        return PROVIDER_ERR_NETWORK;
    }

    // Build JSON request (Gemini format)
    char* escaped_system = json_escape(system ? system : "");
    char* escaped_user = json_escape(user);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        free(url);
        return PROVIDER_ERR_INVALID_REQUEST;
    }

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        free(url);
        return PROVIDER_ERR_NETWORK;
    }

    StyleSettings style = convergio_get_style_settings();
    if (system && strlen(system) > 0) {
        snprintf(json_body, json_size,
            "{"
            "\"systemInstruction\": {\"parts\": [{\"text\": \"%s\"}]},"
            "\"contents\": [{\"parts\": [{\"text\": \"%s\"}]}],"
            "\"generationConfig\": {\"maxOutputTokens\": %d, \"temperature\": %.2f}"
            "}",
            escaped_system, escaped_user, style.max_tokens, style.temperature);
    } else {
        snprintf(json_body, json_size,
            "{"
            "\"contents\": [{\"parts\": [{\"text\": \"%s\"}]}],"
            "\"generationConfig\": {\"maxOutputTokens\": %d, \"temperature\": %.2f}"
            "}",
            escaped_user, style.max_tokens, style.temperature);
    }

    free(escaped_system);
    free(escaped_user);

    // Create streaming context
    StreamContext* stream_ctx = stream_context_create(PROVIDER_GEMINI);
    if (!stream_ctx) {
        free(json_body);
        free(url);
        return PROVIDER_ERR_NETWORK;
    }

    // Setup bridge context
    GeminiStreamBridge bridge = {
        .handler = handler,
        .usage = usage,
        .error = PROVIDER_OK
    };

    // Set callbacks
    stream_set_callbacks(stream_ctx, gemini_stream_chunk_bridge, gemini_stream_complete_bridge,
                         gemini_stream_error_bridge, &bridge);

    // Execute streaming request (Gemini uses URL-based auth, pass empty string for api_key)
    LOG_DEBUG(LOG_CAT_API, "Starting Gemini stream to %s", url);
    int result = stream_execute(stream_ctx, url, json_body, "");

    // Cleanup
    free(json_body);
    free(url);
    stream_context_destroy(stream_ctx);

    if (result < 0) {
        return bridge.error != PROVIDER_OK ? bridge.error : PROVIDER_ERR_NETWORK;
    } else if (result == 1) {
        return PROVIDER_OK;  // Cancelled
    }

    return PROVIDER_OK;
}

static size_t gemini_estimate_tokens(Provider* self, const char* text) {
    (void)self;
    if (!text) return 0;
    // SentencePiece tokenizer: ~4 chars per token
    size_t len = strlen(text);
    return (len + 3) / 4;
}

static ProviderErrorInfo* gemini_get_last_error(Provider* self) {
    if (!self) return NULL;
    GeminiProviderData* data = (GeminiProviderData*)self->impl_data;
    if (!data) return NULL;
    return &data->last_error;
}

static ProviderError gemini_list_models(Provider* self, ModelConfig** out_models, size_t* out_count) {
    (void)self;
    if (out_models) {
        *out_models = (ModelConfig*)model_get_by_provider(PROVIDER_GEMINI, out_count);
    }
    return PROVIDER_OK;
}

// ============================================================================
// PROVIDER CREATION
// ============================================================================

Provider* gemini_provider_create(void) {
    Provider* provider = calloc(1, sizeof(Provider));
    if (!provider) return NULL;

    GeminiProviderData* data = calloc(1, sizeof(GeminiProviderData));
    if (!data) {
        free(provider);
        return NULL;
    }

    pthread_mutex_init(&data->mutex, NULL);

    provider->type = PROVIDER_GEMINI;
    provider->name = "Google Gemini";
    provider->api_key_env = "GEMINI_API_KEY";
    provider->base_url = GEMINI_API_BASE;
    provider->initialized = false;

    provider->init = gemini_init;
    provider->shutdown = gemini_shutdown;
    provider->validate_key = gemini_validate_key;
    provider->chat = gemini_chat;
    provider->chat_with_tools = gemini_chat_with_tools;
    provider->stream_chat = gemini_stream_chat;
    provider->estimate_tokens = gemini_estimate_tokens;
    provider->get_last_error = gemini_get_last_error;
    provider->list_models = gemini_list_models;

    provider->impl_data = data;

    LOG_DEBUG(LOG_CAT_SYSTEM, "Gemini provider created");
    return provider;
}
