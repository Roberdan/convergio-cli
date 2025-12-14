/**
 * CONVERGIO ANTHROPIC PROVIDER ADAPTER
 *
 * Implements the Provider interface for Anthropic (Claude) models
 * Supports Claude Opus 4.5, Sonnet 4.5, Sonnet 4, Haiku 4.5
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "nous/model_loader.h"
#include "nous/config.h"
#include "nous/nous.h"
#include "../auth/oauth.h"
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

#define ANTHROPIC_API_URL "https://api.anthropic.com/v1/messages"
#define ANTHROPIC_VERSION "2023-06-01"
#define MAX_RESPONSE_SIZE (256 * 1024)  // 256KB max response
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
} AnthropicProviderData;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static ProviderError anthropic_init(Provider* self);
static void anthropic_shutdown(Provider* self);
static bool anthropic_validate_key(Provider* self);
static char* anthropic_chat(Provider* self, const char* model, const char* system,
                            const char* user, TokenUsage* usage);
static char* anthropic_chat_with_tools(Provider* self, const char* model, const char* system,
                                       const char* user, ToolDefinition* tools, size_t tool_count,
                                       ToolCall** out_tool_calls, size_t* out_tool_count,
                                       TokenUsage* usage);
static ProviderError anthropic_stream_chat(Provider* self, const char* model, const char* system,
                                           const char* user, StreamHandler* handler, TokenUsage* usage);
static size_t anthropic_estimate_tokens(Provider* self, const char* text);
static ProviderErrorInfo* anthropic_get_last_error(Provider* self);
static ProviderError anthropic_list_models(Provider* self, ModelConfig** out_models, size_t* out_count);

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
    AnthropicProviderData* data = (AnthropicProviderData*)clientp;
    if (data && data->request_cancelled) {
        return 1;  // Non-zero aborts the transfer
    }
    return 0;
}

// JSON escape helper
static int is_surrogate(uint32_t cp) {
    return cp >= 0xD800 && cp <= 0xDFFF;
}

static int utf8_seq_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 0;
}

static int32_t utf8_decode(const unsigned char* p, int len) {
    if (len == 1) return p[0];
    if (len == 2) {
        if ((p[1] & 0xC0) != 0x80) return -1;
        return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    }
    if (len == 3) {
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return -1;
        return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    }
    if (len == 4) {
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80) return -1;
        return ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
    }
    return -1;
}

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
                case '\b': *out++ = '\\'; *out++ = 'b'; break;
                case '\f': *out++ = '\\'; *out++ = 'f'; break;
                default:
                    if (*p < 32) {
                        int written = snprintf(out, 7, "\\u%04x", *p);
                        if (written > 0) out += written;
                    } else {
                        *out++ = (char)*p;
                    }
            }
            p++;
        } else {
            int seq_len = utf8_seq_len(*p);
            if (seq_len == 0) {
                int written = snprintf(out, 7, "\\uFFFD");
                if (written > 0) out += written;
                p++;
                continue;
            }

            int valid = 1;
            for (int i = 1; i < seq_len && valid; i++) {
                if (p[i] == 0 || (p[i] & 0xC0) != 0x80) {
                    valid = 0;
                }
            }

            if (!valid) {
                int written = snprintf(out, 7, "\\uFFFD");
                if (written > 0) out += written;
                p++;
                continue;
            }

            int32_t cp = utf8_decode(p, seq_len);
            if (cp < 0 || is_surrogate((uint32_t)cp) ||
                (seq_len == 2 && cp < 0x80) ||
                (seq_len == 3 && cp < 0x800) ||
                (seq_len == 4 && cp < 0x10000)) {
                int written = snprintf(out, 7, "\\uFFFD");
                if (written > 0) out += written;
                p++;
                continue;
            }

            for (int i = 0; i < seq_len; i++) {
                *out++ = (char)p[i];
            }
            p += seq_len;
        }
    }
    *out = '\0';
    return escaped;
}

// Check if a quote is escaped
static bool is_quote_escaped(const char* start, const char* pos) {
    if (pos <= start) return false;

    int backslash_count = 0;
    const char* p = pos - 1;
    while (p >= start && *p == '\\') {
        backslash_count++;
        p--;
    }
    return (backslash_count % 2) == 1;
}

// Extract text content from Claude response
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
    while (*end) {
        if (*end == '"' && !is_quote_escaped(start, end)) {
            break;
        }
        end++;
    }

    if (*end != '"') return NULL;

    size_t len = (size_t)(end - start);
    char* result = malloc(len + 1);
    if (!result) return NULL;

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
                case '/': *out++ = '/'; break;
                case 'b': *out++ = '\b'; break;
                case 'f': *out++ = '\f'; break;
                default: *out++ = *p;
            }
        } else {
            *out++ = *p;
        }
    }
    *out = '\0';

    return result;
}

// Extract token usage from response
static void extract_token_usage(const char* json, TokenUsage* usage) {
    if (!json || !usage) return;

    // Look for "usage": {...}
    const char* usage_key = "\"usage\":";
    const char* found = strstr(json, usage_key);
    if (!found) return;

    // Extract input_tokens
    const char* input = strstr(found, "\"input_tokens\":");
    if (input) {
        input += strlen("\"input_tokens\":");
        while (*input && isspace(*input)) input++;
        usage->input_tokens = (size_t)atol(input);
    }

    // Extract output_tokens
    const char* output = strstr(found, "\"output_tokens\":");
    if (output) {
        output += strlen("\"output_tokens\":");
        while (*output && isspace(*output)) output++;
        usage->output_tokens = (size_t)atol(output);
    }

    // Extract cache_read_input_tokens if present
    const char* cached = strstr(found, "\"cache_read_input_tokens\":");
    if (cached) {
        cached += strlen("\"cache_read_input_tokens\":");
        while (*cached && isspace(*cached)) cached++;
        usage->cached_tokens = (size_t)atol(cached);
    }
}

// Get model API ID from model name
// Uses api_id from JSON config (single source of truth)
// Falls back to hardcoded only if JSON not available
static const char* get_model_api_id(const char* model) {
    if (!model) return "claude-sonnet-4-5-20250929";

    // FIRST: Check JSON config for api_id
    const JsonModelConfig* json = models_get_json_model(model);
    if (json && json->api_id) {
        return json->api_id;
    }

    // FALLBACK: Hardcoded mappings (for when JSON not available)
    if (strcmp(model, "claude-opus-4.5") == 0) {
        return "claude-opus-4-5-20251101";
    } else if (strcmp(model, "claude-sonnet-4.5") == 0) {
        return "claude-sonnet-4-5-20250929";
    } else if (strcmp(model, "claude-haiku-4.5") == 0) {
        return "claude-haiku-4-5-20251001";
    }

    // If already an API ID, return as-is
    return model;
}

// Build authentication header
static char* build_auth_header(void) {
    char* auth_value = auth_get_header();
    if (!auth_value) {
        return NULL;
    }

    char* header = malloc(512);
    if (!header) {
        free(auth_value);
        return NULL;
    }

    if (auth_get_mode() == AUTH_MODE_OAUTH) {
        snprintf(header, 512, "Authorization: Bearer %s", auth_value);
    } else {
        snprintf(header, 512, "x-api-key: %s", auth_value);
    }

    free(auth_value);
    return header;
}

// Parse error from API response
static void parse_api_error(const char* response, AnthropicProviderData* data) {
    if (!response || !data) return;

    // Clear previous error
    free(data->last_error.message);
    free(data->last_error.provider_code);
    memset(&data->last_error, 0, sizeof(data->last_error));

    // Look for error type
    const char* type_key = "\"type\":";
    const char* type_found = strstr(response, type_key);
    if (type_found) {
        type_found += strlen(type_key);
        while (*type_found && isspace(*type_found)) type_found++;
        if (*type_found == '"') {
            type_found++;
            const char* end = strchr(type_found, '"');
            if (end) {
                size_t len = (size_t)(end - type_found);
                data->last_error.provider_code = malloc(len + 1);
                if (data->last_error.provider_code) {
                    strncpy(data->last_error.provider_code, type_found, len);
                    data->last_error.provider_code[len] = '\0';
                }

                // Map to ProviderError
                if (strstr(type_found, "authentication_error")) {
                    data->last_error.code = PROVIDER_ERR_AUTH;
                } else if (strstr(type_found, "rate_limit_error")) {
                    data->last_error.code = PROVIDER_ERR_RATE_LIMIT;
                    data->last_error.is_retryable = true;
                } else if (strstr(type_found, "overloaded_error")) {
                    data->last_error.code = PROVIDER_ERR_OVERLOADED;
                    data->last_error.is_retryable = true;
                } else if (strstr(type_found, "invalid_request_error")) {
                    data->last_error.code = PROVIDER_ERR_INVALID_REQUEST;
                } else {
                    data->last_error.code = PROVIDER_ERR_UNKNOWN;
                }
            }
        }
    }

    // Look for error message
    const char* msg_key = "\"message\":";
    const char* msg_found = strstr(response, msg_key);
    if (msg_found) {
        msg_found += strlen(msg_key);
        while (*msg_found && isspace(*msg_found)) msg_found++;
        if (*msg_found == '"') {
            msg_found++;
            const char* end = strchr(msg_found, '"');
            if (end) {
                size_t len = (size_t)(end - msg_found);
                data->last_error.message = malloc(len + 1);
                if (data->last_error.message) {
                    strncpy(data->last_error.message, msg_found, len);
                    data->last_error.message[len] = '\0';
                }
            }
        }
    }
}

// ============================================================================
// PROVIDER INTERFACE IMPLEMENTATION
// ============================================================================

static ProviderError anthropic_init(Provider* self) {
    if (!self) return PROVIDER_ERR_INVALID_REQUEST;

    AnthropicProviderData* data = (AnthropicProviderData*)self->impl_data;
    if (!data) return PROVIDER_ERR_INVALID_REQUEST;

    pthread_mutex_lock(&data->mutex);

    if (data->initialized) {
        pthread_mutex_unlock(&data->mutex);
        return PROVIDER_OK;
    }

    // Check authentication
    if (!auth_is_authenticated()) {
        if (auth_init() != 0) {
            data->last_error.code = PROVIDER_ERR_AUTH;
            data->last_error.message = strdup("No authentication configured");
            pthread_mutex_unlock(&data->mutex);
            return PROVIDER_ERR_AUTH;
        }
    }

    // Note: curl_global_init is called once in main.c at startup
    data->initialized = true;
    self->initialized = true;

    pthread_mutex_unlock(&data->mutex);

    LOG_INFO(LOG_CAT_API, "Anthropic provider initialized");
    return PROVIDER_OK;
}

static void anthropic_shutdown(Provider* self) {
    if (!self) return;

    AnthropicProviderData* data = (AnthropicProviderData*)self->impl_data;
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

    LOG_INFO(LOG_CAT_API, "Anthropic provider shutdown");
}

static bool anthropic_validate_key(Provider* self) {
    if (!self) return false;

    // Check if API key environment variable is set
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (api_key && strlen(api_key) > 0) {
        return true;
    }

    // Check OAuth
    if (auth_is_authenticated()) {
        return true;
    }

    return false;
}

static char* anthropic_chat(Provider* self, const char* model, const char* system,
                            const char* user, TokenUsage* usage) {
    if (!self || !user) return NULL;

    AnthropicProviderData* data = (AnthropicProviderData*)self->impl_data;
    if (!data) return NULL;

    // Ensure initialized
    if (!data->initialized) {
        ProviderError err = anthropic_init(self);
        if (err != PROVIDER_OK) return NULL;
    }

    // Create curl handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup("Failed to create curl handle");
        return NULL;
    }

    // Build JSON request
    char* escaped_system = json_escape(system ? system : "");
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
        "\"max_tokens\": %d,"
        "\"temperature\": %.2f,"
        "\"system\": \"%s\","
        "\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]"
        "}",
        api_model, style.max_tokens, style.temperature, escaped_system, escaped_user);

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
    char* auth_header = build_auth_header();
    if (!auth_header) {
        free(json_body);
        free(response.data);
        curl_easy_cleanup(curl);
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("Failed to get authentication");
        return NULL;
    }

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "anthropic-version: " ANTHROPIC_VERSION);
    free(auth_header);

    // Setup curl
    curl_easy_setopt(curl, CURLOPT_URL, ANTHROPIC_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);

    // Reset cancel flag
    data->request_cancelled = 0;

    // Perform request
    LOG_DEBUG(LOG_CAT_API, "Anthropic API call: model=%s", api_model);
    CURLcode res = curl_easy_perform(curl);

    // Check result
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
        parse_api_error(response.data, data);
        LOG_WARN(LOG_CAT_API, "Anthropic API error: HTTP %ld", http_code);
    } else {
        // Extract response text
        result = extract_response_text(response.data);
        if (!result) {
            data->last_error.code = PROVIDER_ERR_INVALID_REQUEST;
            data->last_error.message = strdup("Failed to parse response");
        } else {
            // Extract token usage
            if (usage) {
                memset(usage, 0, sizeof(TokenUsage));
                extract_token_usage(response.data, usage);
                usage->estimated_cost = model_estimate_cost(model, usage->input_tokens, usage->output_tokens);
                LOG_DEBUG(LOG_CAT_COST, "Tokens: in=%zu out=%zu cost=$%.6f",
                         usage->input_tokens, usage->output_tokens, usage->estimated_cost);
            }
        }
    }

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(json_body);
    free(response.data);

    return result;
}

static char* anthropic_chat_with_tools(Provider* self, const char* model, const char* system,
                                       const char* user, ToolDefinition* tools, size_t tool_count,
                                       ToolCall** out_tool_calls, size_t* out_tool_count,
                                       TokenUsage* usage) {
    if (out_tool_calls) *out_tool_calls = NULL;
    if (out_tool_count) *out_tool_count = 0;

    // If no tools, fall back to regular chat
    if (!tools || tool_count == 0) {
        return anthropic_chat(self, model, system, user, usage);
    }

    if (!self || !user) return NULL;

    AnthropicProviderData* data = (AnthropicProviderData*)self->impl_data;
    if (!data) return NULL;

    if (!data->initialized) {
        ProviderError err = anthropic_init(self);
        if (err != PROVIDER_OK) return NULL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup("Failed to create curl handle");
        return NULL;
    }

    // Build tools JSON
    char* tools_json = build_anthropic_tools_json(tools, tool_count);
    if (!tools_json) {
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
        curl_easy_cleanup(curl);
        return NULL;
    }

    const char* api_model = get_model_api_id(model);

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
    snprintf(json_body, json_size,
        "{"
        "\"model\": \"%s\","
        "\"max_tokens\": %d,"
        "\"temperature\": %.2f,"
        "\"system\": \"%s\","
        "\"tools\": %s,"
        "\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]"
        "}",
        api_model, style.max_tokens, style.temperature, escaped_system, tools_json, escaped_user);

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
    char* auth_header = build_auth_header();
    if (!auth_header) {
        free(json_body);
        free(response.data);
        curl_easy_cleanup(curl);
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("Failed to get authentication");
        return NULL;
    }

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "anthropic-version: " ANTHROPIC_VERSION);
    free(auth_header);

    // Setup curl
    curl_easy_setopt(curl, CURLOPT_URL, ANTHROPIC_API_URL);
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
    ToolCall* tc = parse_anthropic_tool_calls(response.data, &tc_count);
    if (tc && tc_count > 0) {
        if (out_tool_calls) *out_tool_calls = tc;
        if (out_tool_count) *out_tool_count = tc_count;
    }

    // Extract text response
    char* result = extract_response_text(response.data);
    if (usage) {
        memset(usage, 0, sizeof(TokenUsage));
        extract_token_usage(response.data, usage);
        usage->estimated_cost = model_estimate_cost(model, usage->input_tokens, usage->output_tokens);
    }

    free(response.data);
    return result;
}

// Bridge context for streaming callbacks
typedef struct {
    StreamHandler* handler;
    TokenUsage* usage;
    ProviderError error;
} StreamBridgeContext;

// Bridge callback for stream chunks
static void stream_chunk_bridge(const char* chunk, void* ctx) {
    StreamBridgeContext* bridge = (StreamBridgeContext*)ctx;
    if (bridge && bridge->handler && bridge->handler->on_chunk) {
        // Unescape JSON content
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
static void stream_complete_bridge(const char* full_response, TokenUsage* usage, void* ctx) {
    StreamBridgeContext* bridge = (StreamBridgeContext*)ctx;
    if (bridge) {
        if (bridge->usage && usage) {
            *bridge->usage = *usage;
        }
        if (bridge->handler) {
            // Send final chunk marker
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
static void stream_error_bridge(ProviderError error, const char* message, void* ctx) {
    StreamBridgeContext* bridge = (StreamBridgeContext*)ctx;
    if (bridge) {
        bridge->error = error;
        if (bridge->handler && bridge->handler->on_error) {
            bridge->handler->on_error(message ? message : "Stream error", bridge->handler->user_ctx);
        }
    }
}

static ProviderError anthropic_stream_chat(Provider* self, const char* model, const char* system,
                                           const char* user, StreamHandler* handler, TokenUsage* usage) {
    if (!self || !user) return PROVIDER_ERR_INVALID_REQUEST;

    AnthropicProviderData* data = (AnthropicProviderData*)self->impl_data;
    if (!data) return PROVIDER_ERR_INVALID_REQUEST;

    // Ensure initialized
    if (!data->initialized) {
        ProviderError err = anthropic_init(self);
        if (err != PROVIDER_OK) return err;
    }

    // Build JSON request with stream: true
    char* escaped_system = json_escape(system ? system : "");
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
        "\"max_tokens\": %d,"
        "\"temperature\": %.2f,"
        "\"stream\": true,"
        "\"system\": \"%s\","
        "\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]"
        "}",
        api_model, style.max_tokens, style.temperature, escaped_system, escaped_user);

    free(escaped_system);
    free(escaped_user);

    // Get API key
    char* api_key = auth_get_header();
    if (!api_key) {
        free(json_body);
        data->last_error.code = PROVIDER_ERR_AUTH;
        data->last_error.message = strdup("Failed to get authentication");
        return PROVIDER_ERR_AUTH;
    }

    // Create streaming context
    StreamContext* stream_ctx = stream_context_create(PROVIDER_ANTHROPIC);
    if (!stream_ctx) {
        free(json_body);
        free(api_key);
        return PROVIDER_ERR_NETWORK;
    }

    // Setup bridge context
    StreamBridgeContext bridge = {
        .handler = handler,
        .usage = usage,
        .error = PROVIDER_OK
    };

    // Set callbacks
    stream_set_callbacks(stream_ctx, stream_chunk_bridge, stream_complete_bridge,
                         stream_error_bridge, &bridge);

    // Execute streaming request
    LOG_DEBUG(LOG_CAT_API, "Starting Anthropic stream to %s", ANTHROPIC_API_URL);
    int result = stream_execute(stream_ctx, ANTHROPIC_API_URL, json_body, api_key);

    // Cleanup
    free(json_body);
    free(api_key);
    stream_context_destroy(stream_ctx);

    if (result < 0) {
        return bridge.error != PROVIDER_OK ? bridge.error : PROVIDER_ERR_NETWORK;
    } else if (result == 1) {
        return PROVIDER_OK;  // Cancelled
    }

    return PROVIDER_OK;
}

static size_t anthropic_estimate_tokens(Provider* self, const char* text) {
    (void)self;
    if (!text) return 0;

    // Simple estimation: ~4 characters per token for English text
    // This is a rough approximation; Claude uses a different tokenizer
    size_t len = strlen(text);
    return (len + 3) / 4;
}

static ProviderErrorInfo* anthropic_get_last_error(Provider* self) {
    if (!self) return NULL;

    AnthropicProviderData* data = (AnthropicProviderData*)self->impl_data;
    if (!data) return NULL;

    return &data->last_error;
}

static ProviderError anthropic_list_models(Provider* self, ModelConfig** out_models, size_t* out_count) {
    (void)self;
    if (out_models) {
        *out_models = (ModelConfig*)model_get_by_provider(PROVIDER_ANTHROPIC, out_count);
    }
    return PROVIDER_OK;
}

// ============================================================================
// PROVIDER CREATION
// ============================================================================

Provider* anthropic_provider_create(void) {
    Provider* provider = calloc(1, sizeof(Provider));
    if (!provider) return NULL;

    AnthropicProviderData* data = calloc(1, sizeof(AnthropicProviderData));
    if (!data) {
        free(provider);
        return NULL;
    }

    pthread_mutex_init(&data->mutex, NULL);

    provider->type = PROVIDER_ANTHROPIC;
    provider->name = "Anthropic";
    provider->api_key_env = "ANTHROPIC_API_KEY";
    provider->base_url = ANTHROPIC_API_URL;
    provider->initialized = false;

    // Set function pointers
    provider->init = anthropic_init;
    provider->shutdown = anthropic_shutdown;
    provider->validate_key = anthropic_validate_key;
    provider->chat = anthropic_chat;
    provider->chat_with_tools = anthropic_chat_with_tools;
    provider->stream_chat = anthropic_stream_chat;
    provider->estimate_tokens = anthropic_estimate_tokens;
    provider->get_last_error = anthropic_get_last_error;
    provider->list_models = anthropic_list_models;

    provider->impl_data = data;

    LOG_DEBUG(LOG_CAT_SYSTEM, "Anthropic provider created");
    return provider;
}

// ============================================================================
// REQUEST CANCELLATION (Global functions for backward compatibility)
// ============================================================================

static AnthropicProviderData* get_provider_data(void) {
    Provider* provider = provider_get(PROVIDER_ANTHROPIC);
    if (!provider) return NULL;
    return (AnthropicProviderData*)provider->impl_data;
}

void anthropic_cancel_request(void) {
    AnthropicProviderData* data = get_provider_data();
    if (data) {
        data->request_cancelled = 1;
    }
}

void anthropic_reset_cancel(void) {
    AnthropicProviderData* data = get_provider_data();
    if (data) {
        data->request_cancelled = 0;
    }
}

bool anthropic_is_cancelled(void) {
    AnthropicProviderData* data = get_provider_data();
    if (data) {
        return data->request_cancelled != 0;
    }
    return false;
}
