/**
 * CONVERGIO OPENAI PROVIDER ADAPTER
 *
 * Implements the Provider interface for OpenAI (GPT) models
 * Supports GPT-5.2, GPT-5, GPT-4o, o3, o4-mini, GPT-5 nano
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
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

#define OPENAI_API_URL "https://api.openai.com/v1/chat/completions"
#define MAX_RESPONSE_SIZE (256 * 1024)
#define DEFAULT_MAX_TOKENS 8192

// ============================================================================
// INTERNAL DATA STRUCTURES
// ============================================================================

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} ResponseBuffer;

typedef struct {
    bool initialized;
    CURL* curl;
    ProviderErrorInfo last_error;
    pthread_mutex_t mutex;
    volatile sig_atomic_t request_cancelled;
} OpenAIProviderData;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static ProviderError openai_init(Provider* self);
static void openai_shutdown(Provider* self);
static bool openai_validate_key(Provider* self);
static char* openai_chat(Provider* self, const char* model, const char* system,
                         const char* user, TokenUsage* usage);
static char* openai_chat_with_tools(Provider* self, const char* model, const char* system,
                                    const char* user, ToolDefinition* tools, size_t tool_count,
                                    ToolCall** out_tool_calls, size_t* out_tool_count,
                                    TokenUsage* usage);
static ProviderError openai_stream_chat(Provider* self, const char* model, const char* system,
                                        const char* user, StreamHandler* handler, TokenUsage* usage);
static size_t openai_estimate_tokens(Provider* self, const char* text);
static ProviderErrorInfo* openai_get_last_error(Provider* self);
static ProviderError openai_list_models(Provider* self, ModelConfig** out_models, size_t* out_count);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    ResponseBuffer* buf = (ResponseBuffer*)userp;

    if (buf->size + total >= buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap < buf->size + total + 1) {
            new_cap = buf->size + total + 1;
        }
        char* new_data = realloc(buf->data, new_cap);
        if (!new_data) return 0;
        buf->data = new_data;
        buf->capacity = new_cap;
    }

    memcpy(buf->data + buf->size, contents, total);
    buf->size += total;
    buf->data[buf->size] = '\0';

    return total;
}

static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow) {
    (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    OpenAIProviderData* data = (OpenAIProviderData*)clientp;
    if (data && data->request_cancelled) {
        return 1;
    }
    return 0;
}

// JSON escape helper (simplified)
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
                        if (written > 0) out += written;
                    } else {
                        *out++ = *p;
                    }
            }
            p++;
        } else {
            *out++ = *p++;
        }
    }
    *out = '\0';
    return escaped;
}

// Extract content from OpenAI response
// Format: {"choices":[{"message":{"content":"..."}}]}
static char* extract_response_content(const char* json) {
    const char* content_key = "\"content\":";
    const char* found = strstr(json, content_key);
    if (!found) return NULL;

    found += strlen(content_key);
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

    size_t len = end - start;
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

// Extract token usage from OpenAI response
static void extract_token_usage(const char* json, TokenUsage* usage) {
    if (!json || !usage) return;

    const char* usage_key = "\"usage\":";
    const char* found = strstr(json, usage_key);
    if (!found) return;

    // prompt_tokens
    const char* prompt = strstr(found, "\"prompt_tokens\":");
    if (prompt) {
        prompt += strlen("\"prompt_tokens\":");
        while (*prompt && isspace(*prompt)) prompt++;
        usage->input_tokens = (size_t)atol(prompt);
    }

    // completion_tokens
    const char* completion = strstr(found, "\"completion_tokens\":");
    if (completion) {
        completion += strlen("\"completion_tokens\":");
        while (*completion && isspace(*completion)) completion++;
        usage->output_tokens = (size_t)atol(completion);
    }
}

// ============================================================================
// PROVIDER INTERFACE IMPLEMENTATION
// ============================================================================

static ProviderError openai_init(Provider* self) {
    if (!self) return PROVIDER_ERR_INVALID_REQUEST;

    OpenAIProviderData* data = (OpenAIProviderData*)self->impl_data;
    if (!data) return PROVIDER_ERR_INVALID_REQUEST;

    pthread_mutex_lock(&data->mutex);

    if (data->initialized) {
        pthread_mutex_unlock(&data->mutex);
        return PROVIDER_OK;
    }

    // Check API key
    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key || strlen(api_key) == 0) {
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("OPENAI_API_KEY not set");
        pthread_mutex_unlock(&data->mutex);
        return PROVIDER_ERR_AUTH;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    data->initialized = true;
    self->initialized = true;

    pthread_mutex_unlock(&data->mutex);

    LOG_INFO(LOG_CAT_API, "OpenAI provider initialized");
    return PROVIDER_OK;
}

static void openai_shutdown(Provider* self) {
    if (!self) return;

    OpenAIProviderData* data = (OpenAIProviderData*)self->impl_data;
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

    LOG_INFO(LOG_CAT_API, "OpenAI provider shutdown");
}

static bool openai_validate_key(Provider* self) {
    (void)self;
    const char* api_key = getenv("OPENAI_API_KEY");
    return (api_key && strlen(api_key) > 0);
}

static char* openai_chat(Provider* self, const char* model, const char* system,
                         const char* user, TokenUsage* usage) {
    if (!self || !user) return NULL;

    OpenAIProviderData* data = (OpenAIProviderData*)self->impl_data;
    if (!data) return NULL;

    // Ensure initialized
    if (!data->initialized) {
        ProviderError err = openai_init(self);
        if (err != PROVIDER_OK) return NULL;
    }

    // Get API key
    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("OPENAI_API_KEY not set");
        return NULL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup("Failed to create curl handle");
        return NULL;
    }

    // Build JSON request (OpenAI format)
    char* escaped_system = json_escape(system ? system : "You are a helpful assistant.");
    char* escaped_user = json_escape(user);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    const char* api_model = model ? model : "gpt-4o";

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    snprintf(json_body, json_size,
        "{"
        "\"model\": \"%s\","
        "\"max_tokens\": %d,"
        "\"messages\": ["
        "{\"role\": \"system\", \"content\": \"%s\"},"
        "{\"role\": \"user\", \"content\": \"%s\"}"
        "]"
        "}",
        api_model, DEFAULT_MAX_TOKENS, escaped_system, escaped_user);

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
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.data[0] = '\0';

    // Build headers
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    // Setup curl
    curl_easy_setopt(curl, CURLOPT_URL, OPENAI_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);

    data->request_cancelled = 0;

    LOG_DEBUG(LOG_CAT_API, "OpenAI API call: model=%s", api_model);
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
        data->last_error.code = PROVIDER_ERR_UNKNOWN;
        data->last_error.message = strdup(response.data ? response.data : "Unknown error");
        LOG_WARN(LOG_CAT_API, "OpenAI API error: HTTP %ld", http_code);
    } else {
        result = extract_response_content(response.data);
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
    free(response.data);

    return result;
}

static char* openai_chat_with_tools(Provider* self, const char* model, const char* system,
                                    const char* user, ToolDefinition* tools, size_t tool_count,
                                    ToolCall** out_tool_calls, size_t* out_tool_count,
                                    TokenUsage* usage) {
    if (out_tool_calls) *out_tool_calls = NULL;
    if (out_tool_count) *out_tool_count = 0;

    // If no tools, fall back to regular chat
    if (!tools || tool_count == 0) {
        return openai_chat(self, model, system, user, usage);
    }

    if (!self || !user) return NULL;

    OpenAIProviderData* data = (OpenAIProviderData*)self->impl_data;
    if (!data) return NULL;

    if (!data->initialized) {
        ProviderError err = openai_init(self);
        if (err != PROVIDER_OK) return NULL;
    }

    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("OPENAI_API_KEY not set");
        return NULL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup("Failed to create curl handle");
        return NULL;
    }

    // Build tools JSON
    char* tools_json = build_openai_tools_json(tools, tool_count);
    if (!tools_json) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Build JSON request with tools
    char* escaped_system = json_escape(system ? system : "You are a helpful assistant.");
    char* escaped_user = json_escape(user);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        free(tools_json);
        curl_easy_cleanup(curl);
        return NULL;
    }

    const char* api_model = model ? model : "gpt-4o";

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + strlen(tools_json) + 2048;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        free(tools_json);
        curl_easy_cleanup(curl);
        return NULL;
    }

    snprintf(json_body, json_size,
        "{"
        "\"model\": \"%s\","
        "\"max_tokens\": %d,"
        "\"tools\": %s,"
        "\"messages\": ["
        "{\"role\": \"system\", \"content\": \"%s\"},"
        "{\"role\": \"user\", \"content\": \"%s\"}"
        "]"
        "}",
        api_model, DEFAULT_MAX_TOKENS, tools_json, escaped_system, escaped_user);

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
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.data[0] = '\0';

    // Build headers
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    // Setup curl
    curl_easy_setopt(curl, CURLOPT_URL, OPENAI_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
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

    if (res != CURLE_OK) {
        free(response.data);
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup(curl_easy_strerror(res));
        return NULL;
    }

    // Parse for tool calls
    size_t tc_count = 0;
    ToolCall* tc = parse_openai_tool_calls(response.data, &tc_count);
    if (tc && tc_count > 0) {
        if (out_tool_calls) *out_tool_calls = tc;
        if (out_tool_count) *out_tool_count = tc_count;
    }

    // Extract text response
    char* result = extract_response_content(response.data);
    if (usage) {
        memset(usage, 0, sizeof(TokenUsage));
        extract_token_usage(response.data, usage);
    }

    free(response.data);
    return result;
}

// Bridge context for streaming callbacks (OpenAI)
typedef struct {
    StreamHandler* handler;
    TokenUsage* usage;
    ProviderError error;
} OpenAIStreamBridge;

// Bridge callback for stream chunks
static void openai_stream_chunk_bridge(const char* chunk, void* ctx) {
    OpenAIStreamBridge* bridge = (OpenAIStreamBridge*)ctx;
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
static void openai_stream_complete_bridge(const char* full_response, TokenUsage* usage, void* ctx) {
    OpenAIStreamBridge* bridge = (OpenAIStreamBridge*)ctx;
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
static void openai_stream_error_bridge(ProviderError error, const char* message, void* ctx) {
    OpenAIStreamBridge* bridge = (OpenAIStreamBridge*)ctx;
    if (bridge) {
        bridge->error = error;
        if (bridge->handler && bridge->handler->on_error) {
            bridge->handler->on_error(message ? message : "Stream error", bridge->handler->user_ctx);
        }
    }
}

static ProviderError openai_stream_chat(Provider* self, const char* model, const char* system,
                                        const char* user, StreamHandler* handler, TokenUsage* usage) {
    if (!self || !user) return PROVIDER_ERR_INVALID_REQUEST;

    OpenAIProviderData* data = (OpenAIProviderData*)self->impl_data;
    if (!data) return PROVIDER_ERR_INVALID_REQUEST;

    if (!data->initialized) {
        ProviderError err = openai_init(self);
        if (err != PROVIDER_OK) return err;
    }

    // Get API key
    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("OPENAI_API_KEY not set");
        return PROVIDER_ERR_AUTH;
    }

    // Build JSON request with stream: true
    char* escaped_system = json_escape(system ? system : "You are a helpful assistant.");
    char* escaped_user = json_escape(user);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        return PROVIDER_ERR_INVALID_REQUEST;
    }

    const char* api_model = model ? model : "gpt-4o";

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        return PROVIDER_ERR_NETWORK;
    }

    snprintf(json_body, json_size,
        "{"
        "\"model\": \"%s\","
        "\"max_tokens\": %d,"
        "\"stream\": true,"
        "\"messages\": ["
        "{\"role\": \"system\", \"content\": \"%s\"},"
        "{\"role\": \"user\", \"content\": \"%s\"}"
        "]"
        "}",
        api_model, DEFAULT_MAX_TOKENS, escaped_system, escaped_user);

    free(escaped_system);
    free(escaped_user);

    // Create streaming context
    StreamContext* stream_ctx = stream_context_create(PROVIDER_OPENAI);
    if (!stream_ctx) {
        free(json_body);
        return PROVIDER_ERR_NETWORK;
    }

    // Setup bridge context
    OpenAIStreamBridge bridge = {
        .handler = handler,
        .usage = usage,
        .error = PROVIDER_OK
    };

    // Set callbacks
    stream_set_callbacks(stream_ctx, openai_stream_chunk_bridge, openai_stream_complete_bridge,
                         openai_stream_error_bridge, &bridge);

    // Execute streaming request
    LOG_DEBUG(LOG_CAT_API, "Starting OpenAI stream to %s", OPENAI_API_URL);
    int result = stream_execute(stream_ctx, OPENAI_API_URL, json_body, api_key);

    // Cleanup
    free(json_body);
    stream_context_destroy(stream_ctx);

    if (result < 0) {
        return bridge.error != PROVIDER_OK ? bridge.error : PROVIDER_ERR_NETWORK;
    } else if (result == 1) {
        return PROVIDER_OK;  // Cancelled
    }

    return PROVIDER_OK;
}

static size_t openai_estimate_tokens(Provider* self, const char* text) {
    (void)self;
    if (!text) return 0;
    // cl100k_base tokenizer: ~4 chars per token
    size_t len = strlen(text);
    return (len + 3) / 4;
}

static ProviderErrorInfo* openai_get_last_error(Provider* self) {
    if (!self) return NULL;
    OpenAIProviderData* data = (OpenAIProviderData*)self->impl_data;
    if (!data) return NULL;
    return &data->last_error;
}

static ProviderError openai_list_models(Provider* self, ModelConfig** out_models, size_t* out_count) {
    (void)self;
    if (out_models) {
        *out_models = (ModelConfig*)model_get_by_provider(PROVIDER_OPENAI, out_count);
    }
    return PROVIDER_OK;
}

// ============================================================================
// PROVIDER CREATION
// ============================================================================

Provider* openai_provider_create(void) {
    Provider* provider = calloc(1, sizeof(Provider));
    if (!provider) return NULL;

    OpenAIProviderData* data = calloc(1, sizeof(OpenAIProviderData));
    if (!data) {
        free(provider);
        return NULL;
    }

    pthread_mutex_init(&data->mutex, NULL);

    provider->type = PROVIDER_OPENAI;
    provider->name = "OpenAI";
    provider->api_key_env = "OPENAI_API_KEY";
    provider->base_url = OPENAI_API_URL;
    provider->initialized = false;

    provider->init = openai_init;
    provider->shutdown = openai_shutdown;
    provider->validate_key = openai_validate_key;
    provider->chat = openai_chat;
    provider->chat_with_tools = openai_chat_with_tools;
    provider->stream_chat = openai_stream_chat;
    provider->estimate_tokens = openai_estimate_tokens;
    provider->get_last_error = openai_get_last_error;
    provider->list_models = openai_list_models;

    provider->impl_data = data;

    LOG_DEBUG(LOG_CAT_SYSTEM, "OpenAI provider created");
    return provider;
}
