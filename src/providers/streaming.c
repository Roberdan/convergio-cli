/**
 * CONVERGIO STREAMING HANDLER
 *
 * Server-Sent Events (SSE) streaming for LLM responses:
 * - Chunk processing and buffering
 * - Multi-provider stream parsing
 * - Progress callbacks
 * - Cancellation support
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include "../auth/oauth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>

// ============================================================================
// STREAM STATE
// ============================================================================

typedef enum {
    STREAM_STATE_IDLE,
    STREAM_STATE_CONNECTING,
    STREAM_STATE_RECEIVING,
    STREAM_STATE_COMPLETE,
    STREAM_STATE_ERROR,
    STREAM_STATE_CANCELLED,
} StreamState;

struct StreamContext {
    // State
    StreamState state;
    ProviderType provider;

    // Callbacks
    void (*on_chunk)(const char* text, void* ctx);
    void (*on_complete)(const char* full_response, TokenUsage* usage, void* ctx);
    void (*on_error)(ProviderError error, const char* message, void* ctx);
    void* callback_ctx;

    // Buffering
    char* buffer;
    size_t buffer_size;
    size_t buffer_used;

    // Response accumulation
    char* full_response;
    size_t response_size;
    size_t response_used;

    // Token tracking
    TokenUsage usage;

    // Cancellation
    volatile bool cancelled;

    // Thread safety
    pthread_mutex_t mutex;
};

// ============================================================================
// BUFFER MANAGEMENT
// ============================================================================

#define INITIAL_BUFFER_SIZE 4096
#define MAX_BUFFER_SIZE (1024 * 1024)  // 1MB max

static int buffer_init(StreamContext* ctx) {
    ctx->buffer_size = INITIAL_BUFFER_SIZE;
    ctx->buffer = malloc(ctx->buffer_size);
    if (!ctx->buffer) return -1;
    ctx->buffer_used = 0;

    ctx->response_size = INITIAL_BUFFER_SIZE;
    ctx->full_response = malloc(ctx->response_size);
    if (!ctx->full_response) {
        free(ctx->buffer);
        return -1;
    }
    ctx->response_used = 0;
    ctx->full_response[0] = '\0';

    return 0;
}

static int buffer_append(char** buf, size_t* size, size_t* used,
                         const char* data, size_t len) {
    // Grow buffer if needed
    while (*used + len + 1 > *size) {
        size_t new_size = *size * 2;
        if (new_size > MAX_BUFFER_SIZE) return -1;

        char* new_buf = realloc(*buf, new_size);
        if (!new_buf) return -1;

        *buf = new_buf;
        *size = new_size;
    }

    memcpy(*buf + *used, data, len);
    *used += len;
    (*buf)[*used] = '\0';

    return 0;
}

static void buffer_clear(StreamContext* ctx) {
    ctx->buffer_used = 0;
    if (ctx->buffer) ctx->buffer[0] = '\0';
}

// ============================================================================
// SSE PARSING
// ============================================================================

/**
 * Parse SSE data line and extract content
 * Returns extracted text or NULL
 */
static char* parse_sse_line(const char* line, ProviderType provider) {
    if (!line || strncmp(line, "data: ", 6) != 0) {
        return NULL;
    }

    const char* data = line + 6;

    // Check for end of stream
    if (strcmp(data, "[DONE]") == 0) {
        return NULL;
    }

    // Parse JSON based on provider format
    // This is a simplified parser - real implementation would use cJSON

    char* content = NULL;

    switch (provider) {
        case PROVIDER_ANTHROPIC: {
            // Anthropic: {"type":"content_block_delta","delta":{"type":"text_delta","text":"..."}}
            const char* text_marker = "\"text\":\"";
            const char* text_start = strstr(data, text_marker);
            if (text_start) {
                text_start += strlen(text_marker);
                const char* text_end = strchr(text_start, '"');
                // Handle escaped quotes
                while (text_end && text_end > text_start && *(text_end - 1) == '\\') {
                    text_end = strchr(text_end + 1, '"');
                }
                if (text_end) {
                    size_t len = text_end - text_start;
                    content = malloc(len + 1);
                    if (content) {
                        memcpy(content, text_start, len);
                        content[len] = '\0';
                    }
                }
            }
            break;
        }

        case PROVIDER_OPENAI: {
            // OpenAI: {"choices":[{"delta":{"content":"..."}}]}
            const char* content_marker = "\"content\":\"";
            const char* content_start = strstr(data, content_marker);
            if (content_start) {
                content_start += strlen(content_marker);
                const char* content_end = strchr(content_start, '"');
                while (content_end && content_end > content_start && *(content_end - 1) == '\\') {
                    content_end = strchr(content_end + 1, '"');
                }
                if (content_end) {
                    size_t len = content_end - content_start;
                    content = malloc(len + 1);
                    if (content) {
                        memcpy(content, content_start, len);
                        content[len] = '\0';
                    }
                }
            }
            break;
        }

        case PROVIDER_GEMINI: {
            // Gemini: {"candidates":[{"content":{"parts":[{"text":"..."}]}}]}
            const char* text_marker = "\"text\":\"";
            const char* text_start = strstr(data, text_marker);
            if (text_start) {
                text_start += strlen(text_marker);
                const char* text_end = strchr(text_start, '"');
                while (text_end && text_end > text_start && *(text_end - 1) == '\\') {
                    text_end = strchr(text_end + 1, '"');
                }
                if (text_end) {
                    size_t len = text_end - text_start;
                    content = malloc(len + 1);
                    if (content) {
                        memcpy(content, text_start, len);
                        content[len] = '\0';
                    }
                }
            }
            break;
        }

        case PROVIDER_OPENROUTER: {
            // OpenRouter uses OpenAI-compatible format
            // {"choices":[{"delta":{"content":"..."}}]}
            const char* content_marker = "\"content\":\"";
            const char* content_start = strstr(data, content_marker);
            if (content_start) {
                content_start += strlen(content_marker);
                const char* content_end = strchr(content_start, '"');
                while (content_end && content_end > content_start && *(content_end - 1) == '\\') {
                    content_end = strchr(content_end + 1, '"');
                }
                if (content_end) {
                    size_t len = content_end - content_start;
                    content = malloc(len + 1);
                    if (content) {
                        memcpy(content, content_start, len);
                        content[len] = '\0';
                    }
                }
            }
            break;
        }

        case PROVIDER_OLLAMA: {
            // Ollama NDJSON: {"message":{"content":"..."}} or {"response":"..."}
            // Try message.content first (chat endpoint)
            const char* content_marker = "\"content\":\"";
            const char* content_start = strstr(data, content_marker);
            if (content_start) {
                content_start += strlen(content_marker);
                const char* content_end = strchr(content_start, '"');
                while (content_end && content_end > content_start && *(content_end - 1) == '\\') {
                    content_end = strchr(content_end + 1, '"');
                }
                if (content_end) {
                    size_t len = content_end - content_start;
                    content = malloc(len + 1);
                    if (content) {
                        memcpy(content, content_start, len);
                        content[len] = '\0';
                    }
                }
            } else {
                // Try response field (generate endpoint)
                const char* resp_marker = "\"response\":\"";
                const char* resp_start = strstr(data, resp_marker);
                if (resp_start) {
                    resp_start += strlen(resp_marker);
                    const char* resp_end = strchr(resp_start, '"');
                    while (resp_end && resp_end > resp_start && *(resp_end - 1) == '\\') {
                        resp_end = strchr(resp_end + 1, '"');
                    }
                    if (resp_end) {
                        size_t len = resp_end - resp_start;
                        content = malloc(len + 1);
                        if (content) {
                            memcpy(content, resp_start, len);
                            content[len] = '\0';
                        }
                    }
                }
            }
            break;
        }

        default:
            break;
    }

    return content;
}

/**
 * Process a complete SSE event
 */
static void process_sse_event(StreamContext* ctx, const char* event) {
    if (ctx->cancelled) return;

    char* content = parse_sse_line(event, ctx->provider);
    if (content) {
        // Append to full response
        buffer_append(&ctx->full_response, &ctx->response_size,
                      &ctx->response_used, content, strlen(content));

        // Invoke callback
        if (ctx->on_chunk) {
            ctx->on_chunk(content, ctx->callback_ctx);
        }

        free(content);
    }
}

// ============================================================================
// CURL CALLBACKS
// ============================================================================

static size_t stream_write_callback(char* ptr, size_t size, size_t nmemb,
                                     void* userdata) {
    StreamContext* ctx = (StreamContext*)userdata;
    size_t total = size * nmemb;

    if (ctx->cancelled) {
        return 0;  // Abort transfer
    }

    pthread_mutex_lock(&ctx->mutex);

    // Append to buffer
    if (buffer_append(&ctx->buffer, &ctx->buffer_size, &ctx->buffer_used,
                      ptr, total) != 0) {
        pthread_mutex_unlock(&ctx->mutex);
        return 0;  // Error, abort
    }

    // Process complete lines
    char* line_start = ctx->buffer;
    char* newline;

    while ((newline = strchr(line_start, '\n')) != NULL) {
        *newline = '\0';

        // Remove carriage return if present
        if (newline > line_start && *(newline - 1) == '\r') {
            *(newline - 1) = '\0';
        }

        // Process this line
        if (strlen(line_start) > 0) {
            process_sse_event(ctx, line_start);
        }

        line_start = newline + 1;
    }

    // Move remaining data to start of buffer
    if (line_start > ctx->buffer) {
        size_t remaining = ctx->buffer + ctx->buffer_used - line_start;
        memmove(ctx->buffer, line_start, remaining);
        ctx->buffer_used = remaining;
        ctx->buffer[remaining] = '\0';
    }

    pthread_mutex_unlock(&ctx->mutex);

    return total;
}

// ============================================================================
// STREAM CONTEXT MANAGEMENT
// ============================================================================

StreamContext* stream_context_create(ProviderType provider) {
    StreamContext* ctx = calloc(1, sizeof(StreamContext));
    if (!ctx) return NULL;

    ctx->provider = provider;
    ctx->state = STREAM_STATE_IDLE;
    ctx->cancelled = false;

    if (buffer_init(ctx) != 0) {
        free(ctx);
        return NULL;
    }

    pthread_mutex_init(&ctx->mutex, NULL);

    return ctx;
}

void stream_context_destroy(StreamContext* ctx) {
    if (!ctx) return;

    pthread_mutex_destroy(&ctx->mutex);
    free(ctx->buffer);
    free(ctx->full_response);
    free(ctx);
}

void stream_set_callbacks(StreamContext* ctx,
                          void (*on_chunk)(const char*, void*),
                          void (*on_complete)(const char*, TokenUsage*, void*),
                          void (*on_error)(ProviderError, const char*, void*),
                          void* user_ctx) {
    if (!ctx) return;

    ctx->on_chunk = on_chunk;
    ctx->on_complete = on_complete;
    ctx->on_error = on_error;
    ctx->callback_ctx = user_ctx;
}

void stream_cancel(StreamContext* ctx) {
    if (!ctx) return;
    ctx->cancelled = true;
    ctx->state = STREAM_STATE_CANCELLED;
}

bool stream_is_cancelled(StreamContext* ctx) {
    return ctx ? ctx->cancelled : true;
}

StreamState stream_get_state(StreamContext* ctx) {
    return ctx ? ctx->state : STREAM_STATE_ERROR;
}

const char* stream_get_response(StreamContext* ctx) {
    return ctx ? ctx->full_response : NULL;
}

// ============================================================================
// STREAMING EXECUTION
// ============================================================================

/**
 * Execute a streaming request
 */
int stream_execute(StreamContext* ctx, const char* url, const char* body,
                   const char* api_key) {
    if (!ctx || !url || !body) return -1;

    ctx->state = STREAM_STATE_CONNECTING;
    buffer_clear(ctx);
    ctx->response_used = 0;
    ctx->full_response[0] = '\0';

    CURL* curl = curl_easy_init();
    if (!curl) {
        ctx->state = STREAM_STATE_ERROR;
        if (ctx->on_error) {
            ctx->on_error(PROVIDER_ERR_NETWORK, "Failed to init curl", ctx->callback_ctx);
        }
        return -1;
    }

    // Set headers based on provider
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: text/event-stream");

    char auth_header[256];
    switch (ctx->provider) {
        case PROVIDER_ANTHROPIC:
            // Use correct header based on auth mode (OAuth vs API key)
            if (auth_get_mode() == AUTH_MODE_OAUTH) {
                snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
            } else {
                snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", api_key);
            }
            headers = curl_slist_append(headers, auth_header);
            headers = curl_slist_append(headers, "anthropic-version: 2024-01-01");
            break;
        case PROVIDER_OPENAI:
            snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
            headers = curl_slist_append(headers, auth_header);
            break;
        case PROVIDER_GEMINI:
            // Gemini uses query param, not header
            break;
        case PROVIDER_OPENROUTER:
            // OpenRouter requires Bearer auth + HTTP-Referer header
            snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
            headers = curl_slist_append(headers, auth_header);
            headers = curl_slist_append(headers, "HTTP-Referer: https://convergio.dev");
            headers = curl_slist_append(headers, "X-Title: ConvergioCLI");
            break;
        case PROVIDER_OLLAMA:
            // Ollama is local - no auth needed
            break;
        default:
            break;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stream_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);  // 5 min timeout for long responses

    ctx->state = STREAM_STATE_RECEIVING;

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (ctx->cancelled) {
        ctx->state = STREAM_STATE_CANCELLED;
        return 1;  // Cancelled, not error
    }

    if (res != CURLE_OK) {
        ctx->state = STREAM_STATE_ERROR;
        if (ctx->on_error) {
            ctx->on_error(PROVIDER_ERR_NETWORK, curl_easy_strerror(res), ctx->callback_ctx);
        }
        return -1;
    }

    ctx->state = STREAM_STATE_COMPLETE;

    // Invoke completion callback
    if (ctx->on_complete) {
        ctx->on_complete(ctx->full_response, &ctx->usage, ctx->callback_ctx);
    }

    return 0;
}

// ============================================================================
// HIGH-LEVEL STREAMING API
// ============================================================================

/**
 * Stream chat completion with simple callback
 */
ProviderError provider_stream_chat(Provider* provider, const char* model,
                                    const char* system, const char* user,
                                    StreamHandler* handler, TokenUsage* usage) {
    if (!provider || !model || !user) return PROVIDER_ERR_INVALID_REQUEST;

    // Use provider's native streaming if available
    if (provider->stream_chat) {
        return provider->stream_chat(provider, model, system, user, handler, usage);
    }

    // Fallback to non-streaming
    char* response = provider->chat(provider, model, system, user, usage);
    if (response) {
        if (handler && handler->on_chunk) {
            handler->on_chunk(response, true, handler->user_ctx);
        }
        if (handler && handler->on_complete) {
            handler->on_complete(response, handler->user_ctx);
        }
        free(response);
        return PROVIDER_OK;
    }

    return PROVIDER_ERR_UNKNOWN;
}

// ============================================================================
// STREAM UTILITIES
// ============================================================================

/**
 * Unescape JSON string content
 */
char* stream_unescape_json(const char* input) {
    if (!input) return NULL;

    size_t len = strlen(input);
    char* output = malloc(len + 1);
    if (!output) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '\\' && i + 1 < len) {
            switch (input[i + 1]) {
                case 'n': output[j++] = '\n'; i++; break;
                case 'r': output[j++] = '\r'; i++; break;
                case 't': output[j++] = '\t'; i++; break;
                case '"': output[j++] = '"'; i++; break;
                case '\\': output[j++] = '\\'; i++; break;
                default: output[j++] = input[i]; break;
            }
        } else {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';

    return output;
}
