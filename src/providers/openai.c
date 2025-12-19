/**
 * CONVERGIO OPENAI PROVIDER ADAPTER
 *
 * Implements the Provider interface for OpenAI (GPT) models
 * Supports GPT-5.2, GPT-5, GPT-4o, o3, o4-mini, GPT-5 nano
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "nous/provider_common.h"
#include "nous/model_loader.h"
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

#define OPENAI_API_URL "https://api.openai.com/v1/chat/completions"
#define OPENAI_EMBED_URL "https://api.openai.com/v1/embeddings"
#define OPENAI_EMBED_MODEL "text-embedding-3-small"
#define OPENAI_EMBED_DIM 768
#define MAX_RESPONSE_SIZE (256 * 1024)
#define DEFAULT_MAX_TOKENS 8192

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

// write_callback now from provider_common.h (provider_write_callback)

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

// Get model API ID from model name (uses JSON config as source of truth)
static const char* get_model_api_id(const char* model) {
    if (!model) return "gpt-5.2";

    // FIRST: Check JSON config for api_id
    const JsonModelConfig* json = models_get_json_model(model);
    if (json && json->api_id) {
        return json->api_id;
    }

    // FALLBACK: Return model as-is (it might already be an API ID)
    return model;
}

// Check if web_search tool is in the tools list
static bool has_web_search_tool(ToolDefinition* tools, size_t tool_count) {
    for (size_t i = 0; i < tool_count; i++) {
        if (tools[i].name && strcmp(tools[i].name, "web_search") == 0) {
            return true;
        }
    }
    return false;
}

// Build tools JSON excluding web_search (for native search)
static char* build_tools_json_excluding_web_search(ToolDefinition* tools, size_t count) {
    if (!tools || count == 0) return strdup("[]");

    // Count non-web_search tools
    size_t non_ws_count = 0;
    for (size_t i = 0; i < count; i++) {
        if (!tools[i].name || strcmp(tools[i].name, "web_search") != 0) {
            non_ws_count++;
        }
    }

    if (non_ws_count == 0) return strdup("[]");

    size_t size = 256 + non_ws_count * 2048;
    char* json = malloc(size);
    if (!json) return NULL;

    size_t offset = (size_t)snprintf(json, size, "[");
    bool first = true;

    for (size_t i = 0; i < count; i++) {
        if (tools[i].name && strcmp(tools[i].name, "web_search") == 0) {
            continue;  // Skip web_search
        }
        if (!first) {
            offset += (size_t)snprintf(json + offset, size - offset, ",");
        }
        offset += (size_t)snprintf(json + offset, size - offset,
            "{\"type\":\"function\",\"function\":{\"name\":\"%s\",\"description\":\"%s\",\"parameters\":%s}}",
            tools[i].name, tools[i].description,
            tools[i].parameters_json ? tools[i].parameters_json : "{\"type\":\"object\",\"properties\":{}}");
        first = false;
    }

    snprintf(json + offset, size - offset, "]");
    return json;
}

// OpenAI search model - use for native web search
#define OPENAI_SEARCH_MODEL "gpt-4o-search-preview"

// Check if model is GPT-5.x series (requires max_completion_tokens instead of max_tokens)
static bool is_gpt5_model(const char* model) {
    if (!model) return false;
    return strstr(model, "gpt-5") != NULL || strstr(model, "o3") != NULL || strstr(model, "o4") != NULL;
}

// Get the correct token limit parameter name for the model
static const char* get_token_param_name(const char* model) {
    return is_gpt5_model(model) ? "max_completion_tokens" : "max_tokens";
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

    // Note: curl_global_init is called once in main.c at startup
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

    const char* api_model = get_model_api_id(model);

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    StyleSettings style = convergio_get_style_settings();
    snprintf(json_body, json_size,
        "{"
        "\"model\": \"%s\","
        "\"%s\": %d,"
        "\"temperature\": %.2f,"
        "\"messages\": ["
        "{\"role\": \"system\", \"content\": \"%s\"},"
        "{\"role\": \"user\", \"content\": \"%s\"}"
        "]"
        "}",
        api_model, get_token_param_name(api_model), style.max_tokens, style.temperature, escaped_system, escaped_user);

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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, provider_write_callback);
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
        data->last_error.code = provider_map_http_error(http_code);
        data->last_error.message = strdup(response.data ? response.data : "Unknown error");
        LOG_WARN(LOG_CAT_API, "OpenAI API error: HTTP %ld -> %d", http_code, data->last_error.code);
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

    // Check if web_search is requested - use OpenAI native search
    bool use_native_search = has_web_search_tool(tools, tool_count);

    // Build tools JSON (excluding web_search if using native search)
    char* tools_json = use_native_search
        ? build_tools_json_excluding_web_search(tools, tool_count)
        : build_openai_tools_json(tools, tool_count);
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

    // Use search model if native web search is enabled
    const char* api_model = use_native_search ? OPENAI_SEARCH_MODEL : get_model_api_id(model);

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + strlen(tools_json) + 2048;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        free(tools_json);
        curl_easy_cleanup(curl);
        return NULL;
    }

    StyleSettings style = convergio_get_style_settings();

    // Add web_search_options if using native search
    if (use_native_search) {
        LOG_INFO(LOG_CAT_API, "OpenAI: Using native web search with %s", OPENAI_SEARCH_MODEL);
        snprintf(json_body, json_size,
            "{"
            "\"model\": \"%s\","
            "\"%s\": %d,"
            "\"temperature\": %.2f,"
            "\"web_search_options\": {\"search_context_size\": \"medium\"},"
            "\"tools\": %s,"
            "\"messages\": ["
            "{\"role\": \"system\", \"content\": \"%s\"},"
            "{\"role\": \"user\", \"content\": \"%s\"}"
            "]"
            "}",
            api_model, get_token_param_name(api_model), style.max_tokens, style.temperature, tools_json, escaped_system, escaped_user);
    } else {
        snprintf(json_body, json_size,
            "{"
            "\"model\": \"%s\","
            "\"%s\": %d,"
            "\"temperature\": %.2f,"
            "\"tools\": %s,"
            "\"messages\": ["
            "{\"role\": \"system\", \"content\": \"%s\"},"
            "{\"role\": \"user\", \"content\": \"%s\"}"
            "]"
            "}",
            api_model, get_token_param_name(api_model), style.max_tokens, style.temperature, tools_json, escaped_system, escaped_user);
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

    const char* api_model = get_model_api_id(model);

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        return PROVIDER_ERR_NETWORK;
    }

    StyleSettings style = convergio_get_style_settings();
    snprintf(json_body, json_size,
        "{"
        "\"model\": \"%s\","
        "\"%s\": %d,"
        "\"temperature\": %.2f,"
        "\"stream\": true,"
        "\"messages\": ["
        "{\"role\": \"system\", \"content\": \"%s\"},"
        "{\"role\": \"user\", \"content\": \"%s\"}"
        "]"
        "}",
        api_model, get_token_param_name(api_model), style.max_tokens, style.temperature, escaped_system, escaped_user);

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
// EMBEDDINGS API
// ============================================================================

/**
 * Generate text embeddings using OpenAI text-embedding-3-small
 * Returns 768-dimensional float array (caller must free)
 * Returns NULL on error
 */
float* openai_embed_text(const char* text, size_t* out_dim) {
    if (!text || !out_dim) return NULL;

    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key || strlen(api_key) == 0) {
        LOG_WARN(LOG_CAT_API, "OpenAI embeddings: OPENAI_API_KEY not set");
        return NULL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR(LOG_CAT_API, "OpenAI embeddings: Failed to create curl handle");
        return NULL;
    }

    // Build JSON request
    char* escaped_text = json_escape(text);
    if (!escaped_text) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    size_t json_size = strlen(escaped_text) + 256;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_text);
        curl_easy_cleanup(curl);
        return NULL;
    }

    snprintf(json_body, json_size,
        "{\"model\": \"%s\", \"input\": \"%s\", \"dimensions\": %d}",
        OPENAI_EMBED_MODEL, escaped_text, OPENAI_EMBED_DIM);

    free(escaped_text);

    // Setup response buffer
    ResponseBuffer response = {
        .data = malloc(65536),
        .size = 0,
        .capacity = 65536
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
    curl_easy_setopt(curl, CURLOPT_URL, OPENAI_EMBED_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, provider_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    LOG_DEBUG(LOG_CAT_API, "OpenAI embeddings API call: model=%s dim=%d", OPENAI_EMBED_MODEL, OPENAI_EMBED_DIM);
    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(json_body);

    if (res != CURLE_OK) {
        LOG_ERROR(LOG_CAT_API, "OpenAI embeddings: curl error: %s", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    if (http_code != 200) {
        LOG_ERROR(LOG_CAT_API, "OpenAI embeddings: HTTP %ld: %s", http_code, response.data);
        free(response.data);
        return NULL;
    }

    // Parse embedding from response
    // Format: {"data":[{"embedding":[0.1,0.2,...]}]}
    const char* embed_key = "\"embedding\":[";
    const char* found = strstr(response.data, embed_key);
    if (!found) {
        LOG_ERROR(LOG_CAT_API, "OpenAI embeddings: no embedding in response");
        free(response.data);
        return NULL;
    }

    found += strlen(embed_key);

    // Allocate result
    float* embedding = calloc(OPENAI_EMBED_DIM, sizeof(float));
    if (!embedding) {
        free(response.data);
        return NULL;
    }

    // Parse float array
    int idx = 0;
    const char* p = found;
    while (*p && idx < OPENAI_EMBED_DIM) {
        while (*p && (isspace(*p) || *p == ',')) p++;
        if (*p == ']') break;

        char* end;
        embedding[idx] = strtof(p, &end);
        if (end == p) break;
        p = end;
        idx++;
    }

    if (idx != OPENAI_EMBED_DIM) {
        LOG_WARN(LOG_CAT_API, "OpenAI embeddings: got %d dims, expected %d", idx, OPENAI_EMBED_DIM);
    }

    free(response.data);
    *out_dim = (size_t)idx;

    LOG_DEBUG(LOG_CAT_API, "OpenAI embeddings: generated %d-dim vector", idx);
    return embedding;
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
