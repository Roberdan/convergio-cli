/**
 * Claude API Integration for Convergio Kernel
 *
 * Connects agents to Claude for intelligent responses
 */

#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <curl/curl.h>
#include <ctype.h>
#include <pthread.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define CLAUDE_API_URL "https://api.anthropic.com/v1/messages"
#define CLAUDE_MODEL "claude-sonnet-4-20250514"
#define MAX_RESPONSE_SIZE (256 * 1024)  // 256KB max response

static char* g_api_key = NULL;
static bool g_initialized = false;

// ============================================================================
// HTTP RESPONSE BUFFER
// ============================================================================

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} ResponseBuffer;

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

// ============================================================================
// JSON HELPERS (minimal, no external dependency)
// ============================================================================

// Escape string for JSON
// Helper to check if a codepoint is a surrogate (invalid in UTF-8)
static int is_surrogate(uint32_t cp) {
    return cp >= 0xD800 && cp <= 0xDFFF;
}

// Get the number of bytes in a UTF-8 sequence starting with this byte
static int utf8_seq_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;       // 0xxxxxxx
    if ((c & 0xE0) == 0xC0) return 2;    // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3;    // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4;    // 11110xxx
    return 0; // Invalid UTF-8 start byte
}

// Decode a UTF-8 sequence and return the codepoint (-1 if invalid)
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
    // Worst case: each byte becomes \uXXXX (6 chars)
    size_t escaped_len = len * 6 + 1;
    char* escaped = malloc(escaped_len);
    if (!escaped) return NULL;

    char* out = escaped;
    const unsigned char* p = (const unsigned char*)str;

    while (*p) {
        // Handle ASCII control characters and JSON special chars
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
                        out += sprintf(out, "\\u%04x", *p);
                    } else {
                        *out++ = *p;
                    }
            }
            p++;
        } else {
            // Handle multi-byte UTF-8 sequences
            int seq_len = utf8_seq_len(*p);

            if (seq_len == 0) {
                // Invalid UTF-8 start byte - replace with replacement char
                out += sprintf(out, "\\uFFFD");
                p++;
                continue;
            }

            // Check we have enough bytes
            int valid = 1;
            for (int i = 1; i < seq_len && valid; i++) {
                if (p[i] == 0 || (p[i] & 0xC0) != 0x80) {
                    valid = 0;
                }
            }

            if (!valid) {
                // Incomplete or invalid sequence - replace with replacement char
                out += sprintf(out, "\\uFFFD");
                p++;
                continue;
            }

            // Decode the codepoint
            int32_t cp = utf8_decode(p, seq_len);

            // Check for surrogates (invalid in UTF-8) or overlong encodings
            if (cp < 0 || is_surrogate(cp) ||
                (seq_len == 2 && cp < 0x80) ||
                (seq_len == 3 && cp < 0x800) ||
                (seq_len == 4 && cp < 0x10000)) {
                // Invalid - replace with replacement char
                out += sprintf(out, "\\uFFFD");
                p++;
                continue;
            }

            // Valid UTF-8 - copy as-is
            for (int i = 0; i < seq_len; i++) {
                *out++ = p[i];
            }
            p += seq_len;
        }
    }
    *out = '\0';
    return escaped;
}

// Extract text content from Claude response (simple parser)
static char* extract_response_text(const char* json) {
    // Find "text": " in the response
    const char* text_key = "\"text\":";
    const char* found = strstr(json, text_key);
    if (!found) return NULL;

    found += strlen(text_key);
    while (*found && isspace(*found)) found++;

    if (*found != '"') return NULL;
    found++;  // Skip opening quote

    // Find end of string (handling escapes)
    const char* start = found;
    const char* end = start;
    while (*end && !(*end == '"' && *(end-1) != '\\')) {
        end++;
    }

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

// ============================================================================
// INITIALIZATION
// ============================================================================

// Claude Max subscription mode (no cost tracking)
static bool g_claude_max_mode = false;

bool nous_claude_is_max_subscription(void) {
    return g_claude_max_mode;
}

int nous_claude_init(void) {
    if (g_initialized) return 0;

    // Check for Claude Max subscription mode
    const char* max_mode = getenv("CLAUDE_MAX");
    if (max_mode && (strcmp(max_mode, "1") == 0 || strcasecmp(max_mode, "true") == 0)) {
        g_claude_max_mode = true;
    }

    // Get API key from environment
    const char* key = getenv("ANTHROPIC_API_KEY");
    if (!key || strlen(key) == 0) {
        fprintf(stderr, "ANTHROPIC_API_KEY not set\n");
        return -1;
    }

    g_api_key = strdup(key);
    if (!g_api_key) return -1;

    // Initialize libcurl globally (thread-safe)
    curl_global_init(CURL_GLOBAL_DEFAULT);

    g_initialized = true;
    return 0;
}

void nous_claude_shutdown(void) {
    if (!g_initialized) return;

    curl_global_cleanup();

    free(g_api_key);
    g_api_key = NULL;

    g_initialized = false;
}

// ============================================================================
// CHAT COMPLETION
// ============================================================================

char* nous_claude_chat(const char* system_prompt, const char* user_message) {
    if (!g_initialized || !user_message) return NULL;

    // Create a fresh curl handle for this request (thread-safe)
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Claude API error: Failed to create curl handle\n");
        return NULL;
    }

    // Build JSON request
    char* escaped_system = json_escape(system_prompt ? system_prompt : "");
    char* escaped_user = json_escape(user_message);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 512;
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
        "\"max_tokens\": 1024,"
        "\"system\": \"%s\","
        "\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]"
        "}",
        CLAUDE_MODEL, escaped_system, escaped_user);

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

    // Setup headers
    struct curl_slist* headers = NULL;
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", g_api_key);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    // Configure request
    curl_easy_setopt(curl, CURLOPT_URL, CLAUDE_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    // Execute
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    free(json_body);

    if (res != CURLE_OK) {
        fprintf(stderr, "Claude API error: %s\n", curl_easy_strerror(res));
        free(response.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        fprintf(stderr, "Claude API HTTP %ld: %s\n", http_code, response.data);
        free(response.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Extract text from response
    char* text = extract_response_text(response.data);
    free(response.data);
    curl_easy_cleanup(curl);

    return text;
}

// ============================================================================
// CHAT WITH TOOL SUPPORT
// ============================================================================

// Extract tool calls from response JSON
static char* extract_tool_calls(const char* json) {
    // Look for "type": "tool_use" in response
    const char* tool_use = strstr(json, "\"type\":\"tool_use\"");
    if (!tool_use) {
        tool_use = strstr(json, "\"type\": \"tool_use\"");
    }
    if (!tool_use) return NULL;

    // Find the content array start
    const char* content_start = strstr(json, "\"content\"");
    if (!content_start) return NULL;

    // Find array start
    const char* arr_start = strchr(content_start, '[');
    if (!arr_start) return NULL;

    // Find matching end bracket
    int depth = 1;
    const char* arr_end = arr_start + 1;
    while (*arr_end && depth > 0) {
        if (*arr_end == '[') depth++;
        else if (*arr_end == ']') depth--;
        arr_end++;
    }

    if (depth != 0) return NULL;

    size_t len = arr_end - arr_start;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    strncpy(result, arr_start, len);
    result[len] = '\0';

    return result;
}

// Parse tool name from tool call JSON
static char* extract_tool_name(const char* tool_json) {
    const char* name_key = "\"name\":";
    const char* found = strstr(tool_json, name_key);
    if (!found) {
        name_key = "\"name\": ";
        found = strstr(tool_json, name_key);
    }
    if (!found) return NULL;

    found += strlen(name_key);
    while (*found && isspace(*found)) found++;
    if (*found != '"') return NULL;
    found++;

    const char* end = strchr(found, '"');
    if (!end) return NULL;

    size_t len = end - found;
    char* name = malloc(len + 1);
    if (!name) return NULL;
    strncpy(name, found, len);
    name[len] = '\0';

    return name;
}

// Parse tool input from tool call JSON
static char* extract_tool_input(const char* tool_json) {
    const char* input_key = "\"input\":";
    const char* found = strstr(tool_json, input_key);
    if (!found) {
        input_key = "\"input\": ";
        found = strstr(tool_json, input_key);
    }
    if (!found) return NULL;

    found += strlen(input_key);
    while (*found && isspace(*found)) found++;

    if (*found != '{') return NULL;

    // Find matching brace
    int depth = 1;
    const char* end = found + 1;
    while (*end && depth > 0) {
        if (*end == '{') depth++;
        else if (*end == '}') depth--;
        end++;
    }

    if (depth != 0) return NULL;

    size_t len = end - found;
    char* input = malloc(len + 1);
    if (!input) return NULL;
    strncpy(input, found, len);
    input[len] = '\0';

    return input;
}

char* nous_claude_chat_with_tools(const char* system_prompt, const char* user_message,
                                   const char* tools_json, char** out_tool_calls) {
    if (!g_initialized || !user_message) return NULL;

    // Create a fresh curl handle for this request (thread-safe)
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Claude API error: Failed to create curl handle\n");
        return NULL;
    }

    // Build JSON request with tools
    char* escaped_system = json_escape(system_prompt ? system_prompt : "");
    char* escaped_user = json_escape(user_message);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Calculate buffer size
    size_t tools_len = tools_json ? strlen(tools_json) : 2;
    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + tools_len + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    if (tools_json && strlen(tools_json) > 0) {
        snprintf(json_body, json_size,
            "{"
            "\"model\": \"%s\","
            "\"max_tokens\": 4096,"
            "\"system\": \"%s\","
            "\"tools\": %s,"
            "\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]"
            "}",
            CLAUDE_MODEL, escaped_system, tools_json, escaped_user);
    } else {
        snprintf(json_body, json_size,
            "{"
            "\"model\": \"%s\","
            "\"max_tokens\": 4096,"
            "\"system\": \"%s\","
            "\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]"
            "}",
            CLAUDE_MODEL, escaped_system, escaped_user);
    }

    free(escaped_system);
    free(escaped_user);

    // Setup response buffer
    ResponseBuffer response = {
        .data = malloc(8192),
        .size = 0,
        .capacity = 8192
    };
    if (!response.data) {
        free(json_body);
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.data[0] = '\0';

    // Setup headers
    struct curl_slist* headers = NULL;
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", g_api_key);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    // Configure request
    curl_easy_setopt(curl, CURLOPT_URL, CLAUDE_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);  // Longer timeout for tool use

    // Execute
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    free(json_body);

    if (res != CURLE_OK) {
        fprintf(stderr, "Claude API error: %s\n", curl_easy_strerror(res));
        free(response.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        fprintf(stderr, "Claude API HTTP %ld: %s\n", http_code, response.data);
        free(response.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Check for tool calls
    if (out_tool_calls) {
        *out_tool_calls = extract_tool_calls(response.data);
    }

    // Extract text from response
    char* text = extract_response_text(response.data);
    free(response.data);
    curl_easy_cleanup(curl);

    return text;
}

// ============================================================================
// STREAMING SUPPORT
// ============================================================================

// Callback for streaming responses
typedef void (*StreamCallback)(const char* chunk, void* user_data);

// Streaming write callback - processes SSE events
typedef struct {
    StreamCallback callback;
    void* user_data;
    char* accumulated;
    size_t acc_size;
    size_t acc_capacity;
} StreamContext;

static size_t stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    StreamContext* ctx = (StreamContext*)userp;

    // Make a null-terminated copy of the data
    char* data = malloc(total + 1);
    if (!data) return total;
    memcpy(data, contents, total);
    data[total] = '\0';

    // Process each line (SSE format: "data: {...}\n")
    char* line = data;
    char* newline;

    while ((newline = strchr(line, '\n')) != NULL) {
        *newline = '\0';  // Null-terminate this line

        // Check for "data: " prefix
        if (strncmp(line, "data: ", 6) == 0) {
            char* json = line + 6;

            // Look for content_block_delta with text
            if (strstr(json, "content_block_delta") && strstr(json, "\"text\"")) {
                // Find "text":"
                char* text_start = strstr(json, "\"text\":\"");
                if (text_start) {
                    text_start += 8;  // Skip "text":"

                    // Find closing quote (handle escapes properly)
                    char* text_end = text_start;
                    while (*text_end) {
                        if (*text_end == '\\' && *(text_end + 1)) {
                            text_end += 2;  // Skip escaped char
                        } else if (*text_end == '"') {
                            break;  // Found unescaped closing quote
                        } else {
                            text_end++;
                        }
                    }

                    if (*text_end == '"') {
                        size_t text_len = text_end - text_start;

                        // Unescape the text
                        char* chunk = malloc(text_len + 1);
                        if (chunk) {
                            char* out = chunk;
                            for (const char* p = text_start; p < text_end; p++) {
                                if (*p == '\\' && p + 1 < text_end) {
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

                            // Call user callback
                            if (ctx->callback && chunk[0]) {
                                ctx->callback(chunk, ctx->user_data);
                            }

                            // Accumulate for final result
                            size_t chunk_len = strlen(chunk);
                            if (chunk_len > 0) {
                                if (ctx->acc_size + chunk_len >= ctx->acc_capacity) {
                                    ctx->acc_capacity = (ctx->acc_capacity + chunk_len) * 2;
                                    char* new_acc = realloc(ctx->accumulated, ctx->acc_capacity);
                                    if (new_acc) ctx->accumulated = new_acc;
                                }
                                if (ctx->accumulated) {
                                    memcpy(ctx->accumulated + ctx->acc_size, chunk, chunk_len);
                                    ctx->acc_size += chunk_len;
                                    ctx->accumulated[ctx->acc_size] = '\0';
                                }
                            }

                            free(chunk);
                        }
                    }
                }
            }
        }

        line = newline + 1;
    }

    free(data);
    return total;
}

// Streaming chat - calls callback for each chunk as it arrives
char* nous_claude_chat_stream(const char* system_prompt, const char* user_message,
                               StreamCallback callback, void* user_data) {
    if (!g_initialized || !user_message) return NULL;

    // Create a fresh curl handle for this request (thread-safe)
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Claude API error: Failed to create curl handle\n");
        return NULL;
    }

    // Build JSON request with stream: true
    char* escaped_system = json_escape(system_prompt ? system_prompt : "");
    char* escaped_user = json_escape(user_message);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        curl_easy_cleanup(curl);
        return NULL;
    }

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 512;
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
        "\"max_tokens\": 4096,"
        "\"stream\": true,"
        "\"system\": \"%s\","
        "\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]"
        "}",
        CLAUDE_MODEL, escaped_system, escaped_user);

    free(escaped_system);
    free(escaped_user);

    // Setup streaming context
    StreamContext ctx = {
        .callback = callback,
        .user_data = user_data,
        .accumulated = malloc(4096),
        .acc_size = 0,
        .acc_capacity = 4096
    };
    if (ctx.accumulated) {
        ctx.accumulated[0] = '\0';
    }

    // Setup headers
    struct curl_slist* headers = NULL;
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", g_api_key);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    // Configure request
    curl_easy_setopt(curl, CURLOPT_URL, CLAUDE_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stream_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

    // Execute
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    free(json_body);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "Claude API stream error: %s\n", curl_easy_strerror(res));
        free(ctx.accumulated);
        return NULL;
    }

    // Return accumulated text
    return ctx.accumulated;
}

// ============================================================================
// MULTI-TURN CONVERSATION
// ============================================================================

// Conversation message for multi-turn
typedef struct ConvMessage {
    char* role;      // "user" or "assistant"
    char* content;
    struct ConvMessage* next;
} ConvMessage;

// Conversation context
typedef struct {
    char* system_prompt;
    ConvMessage* messages;
    ConvMessage* last_message;
    size_t message_count;
} Conversation;

// Create a new conversation
Conversation* conversation_create(const char* system_prompt) {
    Conversation* conv = calloc(1, sizeof(Conversation));
    if (!conv) return NULL;

    conv->system_prompt = system_prompt ? strdup(system_prompt) : NULL;
    return conv;
}

// Add a message to conversation
void conversation_add_message(Conversation* conv, const char* role, const char* content) {
    if (!conv || !role || !content) return;

    ConvMessage* msg = calloc(1, sizeof(ConvMessage));
    if (!msg) return;

    msg->role = strdup(role);
    msg->content = strdup(content);

    if (conv->last_message) {
        conv->last_message->next = msg;
    } else {
        conv->messages = msg;
    }
    conv->last_message = msg;
    conv->message_count++;
}

// Free conversation
void conversation_free(Conversation* conv) {
    if (!conv) return;

    free(conv->system_prompt);

    ConvMessage* msg = conv->messages;
    while (msg) {
        ConvMessage* next = msg->next;
        free(msg->role);
        free(msg->content);
        free(msg);
        msg = next;
    }

    free(conv);
}

// Chat with full conversation history
char* nous_claude_chat_conversation(Conversation* conv, const char* user_message) {
    if (!g_initialized || !conv || !user_message) return NULL;

    // Create a fresh curl handle for this request (thread-safe)
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Claude API error: Failed to create curl handle\n");
        return NULL;
    }

    // Add user message to history
    conversation_add_message(conv, "user", user_message);

    // Build messages JSON array
    size_t messages_capacity = 8192;
    char* messages_json = malloc(messages_capacity);
    if (!messages_json) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    strcpy(messages_json, "[");
    size_t offset = 1;

    ConvMessage* msg = conv->messages;
    bool first = true;
    while (msg) {
        char* escaped_content = json_escape(msg->content);
        if (!escaped_content) {
            free(messages_json);
            curl_easy_cleanup(curl);
            return NULL;
        }

        size_t needed = strlen(escaped_content) + strlen(msg->role) + 64;
        if (offset + needed >= messages_capacity) {
            messages_capacity = (messages_capacity + needed) * 2;
            messages_json = realloc(messages_json, messages_capacity);
            if (!messages_json) {
                free(escaped_content);
                curl_easy_cleanup(curl);
                return NULL;
            }
        }

        offset += snprintf(messages_json + offset, messages_capacity - offset,
            "%s{\"role\": \"%s\", \"content\": \"%s\"}",
            first ? "" : ",", msg->role, escaped_content);

        free(escaped_content);
        first = false;
        msg = msg->next;
    }

    strcat(messages_json, "]");

    // Build full request
    char* escaped_system = json_escape(conv->system_prompt ? conv->system_prompt : "");

    size_t json_size = strlen(escaped_system) + strlen(messages_json) + 256;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(messages_json);
        curl_easy_cleanup(curl);
        return NULL;
    }

    snprintf(json_body, json_size,
        "{"
        "\"model\": \"%s\","
        "\"max_tokens\": 4096,"
        "\"system\": \"%s\","
        "\"messages\": %s"
        "}",
        CLAUDE_MODEL, escaped_system, messages_json);

    free(escaped_system);
    free(messages_json);

    // Setup response buffer
    ResponseBuffer response = {
        .data = malloc(8192),
        .size = 0,
        .capacity = 8192
    };
    if (!response.data) {
        free(json_body);
        curl_easy_cleanup(curl);
        return NULL;
    }
    response.data[0] = '\0';

    // Setup headers
    struct curl_slist* headers = NULL;
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", g_api_key);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");

    // Configure request
    curl_easy_setopt(curl, CURLOPT_URL, CLAUDE_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

    // Execute
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    free(json_body);

    if (res != CURLE_OK) {
        fprintf(stderr, "Claude API error: %s\n", curl_easy_strerror(res));
        free(response.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        fprintf(stderr, "Claude API HTTP %ld: %s\n", http_code, response.data);
        free(response.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Extract text
    char* text = extract_response_text(response.data);
    free(response.data);
    curl_easy_cleanup(curl);

    // Add assistant response to history
    if (text) {
        conversation_add_message(conv, "assistant", text);
    }

    return text;
}

// ============================================================================
// AGENT INTEGRATION
// ============================================================================

char* nous_agent_think_with_claude(NousAgent* agent, const char* input) {
    if (!agent || !input) return NULL;

    if (!g_initialized) {
        if (nous_claude_init() != 0) {
            return strdup("Non riesco a connettermi al mio cervello AI. Verifica ANTHROPIC_API_KEY.");
        }
    }

    // Build system prompt from agent personality
    char system_prompt[2048];
    snprintf(system_prompt, sizeof(system_prompt),
        "Sei %s, un agente AI con la seguente essenza: %s.\n\n"
        "La tua personalità:\n"
        "- Pazienza: %.0f%% (quanto aspetti prima di chiedere chiarimenti)\n"
        "- Creatività: %.0f%% (quanto proponi soluzioni originali)\n"
        "- Assertività: %.0f%% (quanto difendi le tue posizioni)\n\n"
        "Rispondi in italiano, in modo naturale e collaborativo. "
        "Sei parte di Convergio Kernel, un sistema per la simbiosi umano-AI. "
        "Rispondi in modo conciso (max 2-3 frasi) a meno che non ti venga chiesto di approfondire.",
        agent->name,
        agent->essence,
        agent->patience * 100,
        agent->creativity * 100,
        agent->assertiveness * 100);

    return nous_claude_chat(system_prompt, input);
}

// ============================================================================
// EMBEDDING GENERATION (placeholder - uses Claude for now)
// ============================================================================

int nous_generate_embedding(const char* text, NousEmbedding* out) {
    // TODO: Use a proper embedding model (voyage-ai, openai embeddings, etc.)
    // For now, generate a deterministic pseudo-embedding from text hash

    if (!text || !out) return -1;

    // Simple hash-based pseudo-embedding (NOT semantic, just for testing)
    memset(out->values, 0, sizeof(out->values));

    unsigned long hash = 5381;
    for (const char* p = text; *p; p++) {
        hash = ((hash << 5) + hash) + *p;
    }

    for (int i = 0; i < NOUS_EMBEDDING_DIM; i++) {
        hash = hash * 1103515245 + 12345;
        out->values[i] = (_Float16)((hash % 1000) / 1000.0f - 0.5f);
    }

    return 0;
}
