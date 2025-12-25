/**
 * CONVERGIO OLLAMA PROVIDER ADAPTER
 *
 * Implements the Provider interface for Ollama local models
 * Runs models locally with zero API costs
 * Supports Llama, Mistral, CodeLlama, Qwen, and more
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/nous.h"
#include "nous/provider.h"
#include "nous/provider_common.h"
#include "nous/telemetry.h"
#include <ctype.h>
#include <curl/curl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define OLLAMA_DEFAULT_HOST "http://localhost:11434"
#define OLLAMA_GENERATE_ENDPOINT "/api/generate"
#define OLLAMA_CHAT_ENDPOINT "/api/chat"
#define OLLAMA_TAGS_ENDPOINT "/api/tags"
#define MAX_RESPONSE_SIZE (256 * 1024)
#define OLLAMA_DEFAULT_NUM_CTX 4096

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
    char* host; // Configurable host (default: localhost:11434)
} OllamaProviderData;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static ProviderError ollama_init(Provider* self);
static void ollama_shutdown(Provider* self);
static bool ollama_validate_key(Provider* self);
static char* ollama_chat(Provider* self, const char* model, const char* system, const char* user,
                         TokenUsage* usage);
static char* ollama_chat_with_tools(Provider* self, const char* model, const char* system,
                                    const char* user, ToolDefinition* tools, size_t tool_count,
                                    ToolCall** out_tool_calls, size_t* out_tool_count,
                                    TokenUsage* usage);
static ProviderError ollama_stream_chat(Provider* self, const char* model, const char* system,
                                        const char* user, StreamHandler* handler,
                                        TokenUsage* usage);
static size_t ollama_estimate_tokens(Provider* self, const char* text);
static ProviderErrorInfo* ollama_get_last_error(Provider* self);
static ProviderError ollama_list_models(Provider* self, ModelConfig** out_models,
                                        size_t* out_count);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// write_callback now from provider_common.h (provider_write_callback)

static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                             curl_off_t ultotal, curl_off_t ulnow) {
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;
    OllamaProviderData* data = (OllamaProviderData*)clientp;
    if (data && data->request_cancelled) {
        return 1;
    }
    return 0;
}

// JSON escape helper (simplified)
static char* json_escape(const char* str) {
    if (!str)
        return strdup("");

    size_t len = strlen(str);
    size_t escaped_len = len * 6 + 1;
    char* escaped = malloc(escaped_len);
    if (!escaped)
        return NULL;

    char* out = escaped;
    const unsigned char* p = (const unsigned char*)str;

    while (*p) {
        if (*p < 128) {
            switch (*p) {
            case '"':
                *out++ = '\\';
                *out++ = '"';
                break;
            case '\\':
                *out++ = '\\';
                *out++ = '\\';
                break;
            case '\n':
                *out++ = '\\';
                *out++ = 'n';
                break;
            case '\r':
                *out++ = '\\';
                *out++ = 'r';
                break;
            case '\t':
                *out++ = '\\';
                *out++ = 't';
                break;
            default:
                if (*p < 32) {
                    int written = snprintf(out, 7, "\\u%04x", *p);
                    if (written > 0)
                        out += written;
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

// Extract response from Ollama /api/generate response
// Format: {"response": "...", "done": true, "eval_count": 123, "prompt_eval_count": 45}
static char* extract_ollama_response(const char* json) {
    const char* response_key = "\"response\":";
    const char* found = strstr(json, response_key);
    if (!found)
        return NULL;

    found += strlen(response_key);
    while (*found && isspace(*found))
        found++;

    if (*found != '"')
        return NULL;
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

    if (*end != '"')
        return NULL;

    size_t len = (size_t)(end - start);
    char* result = malloc(len + 1);
    if (!result)
        return NULL;

    // Unescape
    char* out = result;
    for (const char* p = start; p < end; p++) {
        if (*p == '\\' && p + 1 < end) {
            p++;
            switch (*p) {
            case 'n':
                *out++ = '\n';
                break;
            case 'r':
                *out++ = '\r';
                break;
            case 't':
                *out++ = '\t';
                break;
            case '"':
                *out++ = '"';
                break;
            case '\\':
                *out++ = '\\';
                break;
            default:
                *out++ = *p;
            }
        } else {
            *out++ = *p;
        }
    }
    *out = '\0';

    return result;
}

// Extract message content from Ollama /api/chat response
// Format: {"message": {"role": "assistant", "content": "..."}, "done": true}
static char* extract_ollama_chat_content(const char* json) {
    const char* content_key = "\"content\":";
    const char* found = strstr(json, content_key);
    if (!found)
        return NULL;

    found += strlen(content_key);
    while (*found && isspace(*found))
        found++;

    if (*found != '"')
        return NULL;
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

    if (*end != '"')
        return NULL;

    size_t len = (size_t)(end - start);
    char* result = malloc(len + 1);
    if (!result)
        return NULL;

    // Unescape
    char* out = result;
    for (const char* p = start; p < end; p++) {
        if (*p == '\\' && p + 1 < end) {
            p++;
            switch (*p) {
            case 'n':
                *out++ = '\n';
                break;
            case 'r':
                *out++ = '\r';
                break;
            case 't':
                *out++ = '\t';
                break;
            case '"':
                *out++ = '"';
                break;
            case '\\':
                *out++ = '\\';
                break;
            default:
                *out++ = *p;
            }
        } else {
            *out++ = *p;
        }
    }
    *out = '\0';

    return result;
}

// Extract token usage from Ollama response
static void extract_ollama_token_usage(const char* json, TokenUsage* usage) {
    if (!json || !usage)
        return;

    // prompt_eval_count (input tokens)
    const char* prompt = strstr(json, "\"prompt_eval_count\":");
    if (prompt) {
        prompt += strlen("\"prompt_eval_count\":");
        while (*prompt && isspace(*prompt))
            prompt++;
        usage->input_tokens = (size_t)atol(prompt);
    }

    // eval_count (output tokens)
    const char* eval = strstr(json, "\"eval_count\":");
    if (eval) {
        eval += strlen("\"eval_count\":");
        while (*eval && isspace(*eval))
            eval++;
        usage->output_tokens = (size_t)atol(eval);
    }

    // Local models have zero cost
    usage->estimated_cost = 0.0;
}

// Get Ollama host URL (from env or default)
static const char* get_ollama_host(OllamaProviderData* data) {
    if (data->host)
        return data->host;
    const char* env_host = getenv("OLLAMA_HOST");
    if (env_host && strlen(env_host) > 0) {
        data->host = strdup(env_host);
        return data->host;
    }
    return OLLAMA_DEFAULT_HOST;
}

// Check if Ollama is running by pinging the API
static bool ollama_ping(const char* host) {
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;

    char url[256];
    snprintf(url, sizeof(url), "%s/api/tags", host);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}

// ============================================================================
// PROVIDER INTERFACE IMPLEMENTATION
// ============================================================================

static ProviderError ollama_init(Provider* self) {
    if (!self)
        return PROVIDER_ERR_INVALID_REQUEST;

    OllamaProviderData* data = (OllamaProviderData*)self->impl_data;
    if (!data)
        return PROVIDER_ERR_INVALID_REQUEST;

    pthread_mutex_lock(&data->mutex);

    if (data->initialized) {
        pthread_mutex_unlock(&data->mutex);
        return PROVIDER_OK;
    }

    // Get host URL
    const char* host = get_ollama_host(data);

    // Check if Ollama is running
    if (!ollama_ping(host)) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        char* msg = malloc(256);
        if (msg) {
            snprintf(msg, 256, "Ollama not running at %s. Start with: ollama serve", host);
            data->last_error.message = msg;
        }
        pthread_mutex_unlock(&data->mutex);
        return PROVIDER_ERR_NETWORK;
    }

    // Note: curl_global_init is called once in main.c at startup
    data->initialized = true;
    self->initialized = true;

    pthread_mutex_unlock(&data->mutex);

    LOG_INFO(LOG_CAT_API, "Ollama provider initialized at %s", host);
    return PROVIDER_OK;
}

static void ollama_shutdown(Provider* self) {
    if (!self)
        return;

    OllamaProviderData* data = (OllamaProviderData*)self->impl_data;
    if (!data)
        return;

    pthread_mutex_lock(&data->mutex);

    if (data->initialized) {
        curl_global_cleanup();
        data->initialized = false;
        self->initialized = false;
    }

    free(data->host);
    data->host = NULL;

    free(data->last_error.message);
    free(data->last_error.provider_code);
    memset(&data->last_error, 0, sizeof(data->last_error));

    pthread_mutex_unlock(&data->mutex);
    pthread_mutex_destroy(&data->mutex);

    LOG_INFO(LOG_CAT_API, "Ollama provider shutdown");
}

static bool ollama_validate_key(Provider* self) {
    // Ollama doesn't require an API key - just check if it's running
    if (!self)
        return false;
    OllamaProviderData* data = (OllamaProviderData*)self->impl_data;
    if (!data)
        return false;

    const char* host = get_ollama_host(data);
    return ollama_ping(host);
}

static char* ollama_chat(Provider* self, const char* model, const char* system, const char* user,
                         TokenUsage* usage) {
    if (!self || !user)
        return NULL;

    OllamaProviderData* data = (OllamaProviderData*)self->impl_data;
    if (!data)
        return NULL;

    // Ensure initialized
    if (!data->initialized) {
        ProviderError err = ollama_init(self);
        if (err != PROVIDER_OK)
            return NULL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup("Failed to create curl handle");
        return NULL;
    }

    // Build URL
    const char* host = get_ollama_host(data);
    char url[512];
    snprintf(url, sizeof(url), "%s%s", host, OLLAMA_CHAT_ENDPOINT);

    // Build JSON request (Ollama chat format)
    char* escaped_system = json_escape(system ? system : "You are a helpful assistant.");
    char* escaped_user = json_escape(user);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    const char* api_model = model ? model : "llama3.2";

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Ollama /api/chat format with messages array
    snprintf(json_body, json_size,
             "{"
             "\"model\": \"%s\","
             "\"stream\": false,"
             "\"messages\": ["
             "{\"role\": \"system\", \"content\": \"%s\"},"
             "{\"role\": \"user\", \"content\": \"%s\"}"
             "],"
             "\"options\": {"
             "\"num_ctx\": %d"
             "}"
             "}",
             api_model, escaped_system, escaped_user, OLLAMA_DEFAULT_NUM_CTX);

    free(escaped_system);
    free(escaped_user);

    // Setup response buffer
    ResponseBuffer response = {.data = malloc(4096), .size = 0, .capacity = 4096};
    if (!response.data) {
        free(json_body);
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.data[0] = '\0';

    // Build headers (no auth needed for local Ollama)
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Setup curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, provider_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // Longer timeout for local inference
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);

    data->request_cancelled = 0;

    // Measure latency for telemetry
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    LOG_DEBUG(LOG_CAT_API, "Ollama API call: model=%s", api_model);
    CURLcode res = curl_easy_perform(curl);

    // Calculate latency
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double latency_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000.0);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    data->last_error.http_status = (int)http_code;

    char* result = NULL;
    uint64_t tokens_input = 0;
    uint64_t tokens_output = 0;

    if (res == CURLE_ABORTED_BY_CALLBACK) {
        data->last_error.code = PROVIDER_ERR_TIMEOUT;
        data->last_error.message = strdup("Request cancelled");
        telemetry_record_error("provider_timeout");
    } else if (res != CURLE_OK) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup(curl_easy_strerror(res));
        telemetry_record_error("provider_network_error");
    } else if (http_code != 200) {
        data->last_error.code = PROVIDER_ERR_UNKNOWN;
        data->last_error.message = strdup(response.data ? response.data : "Unknown error");
        LOG_WARN(LOG_CAT_API, "Ollama API error: HTTP %ld", http_code);
        telemetry_record_error("provider_api_error");
    } else {
        result = extract_ollama_chat_content(response.data);
        if (!result) {
            // Try generate format as fallback
            result = extract_ollama_response(response.data);
        }
        if (!result) {
            data->last_error.code = PROVIDER_ERR_INVALID_REQUEST;
            data->last_error.message = strdup("Failed to parse response");
            telemetry_record_error("provider_parse_error");
        } else if (usage) {
            memset(usage, 0, sizeof(TokenUsage));
            extract_ollama_token_usage(response.data, usage);
            // Local models are free
            usage->estimated_cost = 0.0;
            tokens_input = usage->input_tokens;
            tokens_output = usage->output_tokens;
            LOG_DEBUG(LOG_CAT_COST, "Tokens: in=%zu out=%zu cost=$0.00 (local)",
                      usage->input_tokens, usage->output_tokens);
        }
        // Record successful API call in telemetry (local models, cost=0)
        if (result) {
            telemetry_record_api_call("ollama", api_model, tokens_input, tokens_output, latency_ms);
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(json_body);
    free(response.data);

    return result;
}

static char* ollama_chat_with_tools(Provider* self, const char* model, const char* system,
                                    const char* user, ToolDefinition* tools, size_t tool_count,
                                    ToolCall** out_tool_calls, size_t* out_tool_count,
                                    TokenUsage* usage) {
    // Ollama doesn't support native tool calling yet
    // Fall back to regular chat
    (void)tools;
    (void)tool_count;

    if (out_tool_calls)
        *out_tool_calls = NULL;
    if (out_tool_count)
        *out_tool_count = 0;

    LOG_DEBUG(LOG_CAT_API, "Ollama: tool calling not supported, falling back to chat");
    return ollama_chat(self, model, system, user, usage);
}

// Streaming callback context for Ollama
typedef struct {
    StreamHandler* handler;
    char* accumulated;
    size_t accumulated_size;
    size_t accumulated_capacity;
} OllamaStreamContext;

// Write callback for streaming that processes NDJSON lines
static size_t ollama_stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    OllamaStreamContext* ctx = (OllamaStreamContext*)userp;

    // Ensure buffer space
    if (ctx->accumulated_size + total >= ctx->accumulated_capacity) {
        size_t new_cap = ctx->accumulated_capacity * 2;
        if (new_cap < ctx->accumulated_size + total + 1) {
            new_cap = ctx->accumulated_size + total + 1;
        }
        char* new_data = realloc(ctx->accumulated, new_cap);
        if (!new_data)
            return 0;
        ctx->accumulated = new_data;
        ctx->accumulated_capacity = new_cap;
    }

    memcpy(ctx->accumulated + ctx->accumulated_size, contents, total);
    ctx->accumulated_size += total;
    ctx->accumulated[ctx->accumulated_size] = '\0';

    // Process complete lines (NDJSON format)
    char* line_start = ctx->accumulated;
    char* newline;
    while ((newline = strchr(line_start, '\n')) != NULL) {
        *newline = '\0';

        // Parse JSON line
        char* chunk = extract_ollama_chat_content(line_start);
        if (!chunk) {
            chunk = extract_ollama_response(line_start);
        }

        if (chunk && ctx->handler && ctx->handler->on_chunk) {
            ctx->handler->on_chunk(chunk, false, ctx->handler->user_ctx);
            free(chunk);
        }

        // Check for done
        if (strstr(line_start, "\"done\":true") || strstr(line_start, "\"done\": true")) {
            if (ctx->handler && ctx->handler->on_chunk) {
                ctx->handler->on_chunk("", true, ctx->handler->user_ctx);
            }
        }

        line_start = newline + 1;
    }

    // Keep remaining incomplete line
    if (line_start != ctx->accumulated) {
        size_t remaining = ctx->accumulated_size - (size_t)(line_start - ctx->accumulated);
        memmove(ctx->accumulated, line_start, remaining);
        ctx->accumulated_size = remaining;
        ctx->accumulated[remaining] = '\0';
    }

    return total;
}

static ProviderError ollama_stream_chat(Provider* self, const char* model, const char* system,
                                        const char* user, StreamHandler* handler,
                                        TokenUsage* usage) {
    if (!self || !user)
        return PROVIDER_ERR_INVALID_REQUEST;

    OllamaProviderData* data = (OllamaProviderData*)self->impl_data;
    if (!data)
        return PROVIDER_ERR_INVALID_REQUEST;

    if (!data->initialized) {
        ProviderError err = ollama_init(self);
        if (err != PROVIDER_OK)
            return err;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup("Failed to create curl handle");
        return PROVIDER_ERR_NETWORK;
    }

    // Build URL
    const char* host = get_ollama_host(data);
    char url[512];
    snprintf(url, sizeof(url), "%s%s", host, OLLAMA_CHAT_ENDPOINT);

    // Build JSON request with stream: true
    char* escaped_system = json_escape(system ? system : "You are a helpful assistant.");
    char* escaped_user = json_escape(user);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return PROVIDER_ERR_INVALID_REQUEST;
    }

    const char* api_model = model ? model : "llama3.2";

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return PROVIDER_ERR_NETWORK;
    }

    snprintf(json_body, json_size,
             "{"
             "\"model\": \"%s\","
             "\"stream\": true,"
             "\"messages\": ["
             "{\"role\": \"system\", \"content\": \"%s\"},"
             "{\"role\": \"user\", \"content\": \"%s\"}"
             "],"
             "\"options\": {"
             "\"num_ctx\": %d"
             "}"
             "}",
             api_model, escaped_system, escaped_user, OLLAMA_DEFAULT_NUM_CTX);

    free(escaped_system);
    free(escaped_user);

    // Setup streaming context
    OllamaStreamContext stream_ctx = {.handler = handler,
                                      .accumulated = malloc(4096),
                                      .accumulated_size = 0,
                                      .accumulated_capacity = 4096};
    if (!stream_ctx.accumulated) {
        free(json_body);
        curl_easy_cleanup(curl);
        return PROVIDER_ERR_NETWORK;
    }
    stream_ctx.accumulated[0] = '\0';

    // Build headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Setup curl
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ollama_stream_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream_ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);

    data->request_cancelled = 0;

    // Measure latency for telemetry
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    LOG_DEBUG(LOG_CAT_API, "Starting Ollama stream: model=%s", api_model);
    CURLcode res = curl_easy_perform(curl);

    // Calculate latency
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double latency_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000.0);

    ProviderError result = PROVIDER_OK;
    if (res != CURLE_OK) {
        data->last_error.code = PROVIDER_ERR_NETWORK;
        data->last_error.message = strdup(curl_easy_strerror(res));
        result = PROVIDER_ERR_NETWORK;
        telemetry_record_error("provider_network_error");
        if (handler && handler->on_error) {
            handler->on_error(curl_easy_strerror(res), handler->user_ctx);
        }
    } else {
        // Record successful streaming API call (local models, cost=0, tokens estimated)
        telemetry_record_api_call("ollama", api_model, 0, 0, latency_ms);
        if (handler && handler->on_complete) {
            handler->on_complete("", handler->user_ctx);
        }
    }

    // Set usage (estimate - Ollama doesn't report in streaming)
    if (usage) {
        memset(usage, 0, sizeof(TokenUsage));
        usage->estimated_cost = 0.0; // Local is free
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(json_body);
    free(stream_ctx.accumulated);

    return result;
}

static size_t ollama_estimate_tokens(Provider* self, const char* text) {
    (void)self;
    if (!text)
        return 0;
    // Generic tokenizer estimate: ~4 chars per token
    size_t len = strlen(text);
    return (len + 3) / 4;
}

static ProviderErrorInfo* ollama_get_last_error(Provider* self) {
    if (!self)
        return NULL;
    OllamaProviderData* data = (OllamaProviderData*)self->impl_data;
    if (!data)
        return NULL;
    return &data->last_error;
}

static ProviderError ollama_list_models(Provider* self, ModelConfig** out_models,
                                        size_t* out_count) {
    (void)self;
    if (out_models) {
        *out_models = (ModelConfig*)model_get_by_provider(PROVIDER_OLLAMA, out_count);
    }
    return PROVIDER_OK;
}

// ============================================================================
// PROVIDER CREATION
// ============================================================================

Provider* ollama_provider_create(void) {
    Provider* provider = calloc(1, sizeof(Provider));
    if (!provider)
        return NULL;

    OllamaProviderData* data = calloc(1, sizeof(OllamaProviderData));
    if (!data) {
        free(provider);
        return NULL;
    }

    pthread_mutex_init(&data->mutex, NULL);

    provider->type = PROVIDER_OLLAMA;
    provider->name = "Ollama";
    provider->api_key_env = NULL; // No API key needed
    provider->base_url = OLLAMA_DEFAULT_HOST;
    provider->initialized = false;

    provider->init = ollama_init;
    provider->shutdown = ollama_shutdown;
    provider->validate_key = ollama_validate_key;
    provider->chat = ollama_chat;
    provider->chat_with_tools = ollama_chat_with_tools;
    provider->stream_chat = ollama_stream_chat;
    provider->estimate_tokens = ollama_estimate_tokens;
    provider->get_last_error = ollama_get_last_error;
    provider->list_models = ollama_list_models;

    provider->impl_data = data;

    LOG_DEBUG(LOG_CAT_SYSTEM, "Ollama provider created");
    return provider;
}
