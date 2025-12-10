/**
 * Claude API Integration for Convergio Kernel
 *
 * Connects agents to Claude for intelligent responses
 */

#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include <ctype.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define CLAUDE_API_URL "https://api.anthropic.com/v1/messages"
#define CLAUDE_MODEL "claude-sonnet-4-20250514"
#define MAX_RESPONSE_SIZE (256 * 1024)  // 256KB max response

static char* g_api_key = NULL;
static CURL* g_curl = NULL;
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
static char* json_escape(const char* str) {
    if (!str) return strdup("");

    size_t len = strlen(str);
    size_t escaped_len = len * 2 + 1;
    char* escaped = malloc(escaped_len);
    if (!escaped) return NULL;

    char* out = escaped;
    for (const char* p = str; *p; p++) {
        switch (*p) {
            case '"':  *out++ = '\\'; *out++ = '"'; break;
            case '\\': *out++ = '\\'; *out++ = '\\'; break;
            case '\n': *out++ = '\\'; *out++ = 'n'; break;
            case '\r': *out++ = '\\'; *out++ = 'r'; break;
            case '\t': *out++ = '\\'; *out++ = 't'; break;
            default:
                if ((unsigned char)*p < 32) {
                    out += sprintf(out, "\\u%04x", (unsigned char)*p);
                } else {
                    *out++ = *p;
                }
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

int nous_claude_init(void) {
    if (g_initialized) return 0;

    // Get API key from environment
    const char* key = getenv("ANTHROPIC_API_KEY");
    if (!key || strlen(key) == 0) {
        fprintf(stderr, "ANTHROPIC_API_KEY not set\n");
        return -1;
    }

    g_api_key = strdup(key);
    if (!g_api_key) return -1;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    g_curl = curl_easy_init();
    if (!g_curl) {
        free(g_api_key);
        g_api_key = NULL;
        return -1;
    }

    g_initialized = true;
    return 0;
}

void nous_claude_shutdown(void) {
    if (!g_initialized) return;

    if (g_curl) {
        curl_easy_cleanup(g_curl);
        g_curl = NULL;
    }

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

    // Build JSON request
    char* escaped_system = json_escape(system_prompt ? system_prompt : "");
    char* escaped_user = json_escape(user_message);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        return NULL;
    }

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 512;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
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
    curl_easy_setopt(g_curl, CURLOPT_URL, CLAUDE_API_URL);
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 60L);

    // Execute
    CURLcode res = curl_easy_perform(g_curl);

    curl_slist_free_all(headers);
    free(json_body);

    if (res != CURLE_OK) {
        fprintf(stderr, "Claude API error: %s\n", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        fprintf(stderr, "Claude API HTTP %ld: %s\n", http_code, response.data);
        free(response.data);
        return NULL;
    }

    // Extract text from response
    char* text = extract_response_text(response.data);
    free(response.data);

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

    // Build JSON request with tools
    char* escaped_system = json_escape(system_prompt ? system_prompt : "");
    char* escaped_user = json_escape(user_message);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        return NULL;
    }

    // Calculate buffer size
    size_t tools_len = tools_json ? strlen(tools_json) : 2;
    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + tools_len + 1024;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
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
    curl_easy_setopt(g_curl, CURLOPT_URL, CLAUDE_API_URL);
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 120L);  // Longer timeout for tool use

    // Execute
    CURLcode res = curl_easy_perform(g_curl);

    curl_slist_free_all(headers);
    free(json_body);

    if (res != CURLE_OK) {
        fprintf(stderr, "Claude API error: %s\n", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        fprintf(stderr, "Claude API HTTP %ld: %s\n", http_code, response.data);
        free(response.data);
        return NULL;
    }

    // Check for tool calls
    if (out_tool_calls) {
        *out_tool_calls = extract_tool_calls(response.data);
    }

    // Extract text from response
    char* text = extract_response_text(response.data);
    free(response.data);

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

    // Build JSON request with stream: true
    char* escaped_system = json_escape(system_prompt ? system_prompt : "");
    char* escaped_user = json_escape(user_message);

    if (!escaped_system || !escaped_user) {
        free(escaped_system);
        free(escaped_user);
        return NULL;
    }

    size_t json_size = strlen(escaped_system) + strlen(escaped_user) + 512;
    char* json_body = malloc(json_size);
    if (!json_body) {
        free(escaped_system);
        free(escaped_user);
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
    curl_easy_setopt(g_curl, CURLOPT_URL, CLAUDE_API_URL);
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, stream_write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 120L);

    // Execute
    CURLcode res = curl_easy_perform(g_curl);

    curl_slist_free_all(headers);
    free(json_body);

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

    // Add user message to history
    conversation_add_message(conv, "user", user_message);

    // Build messages JSON array
    size_t messages_capacity = 8192;
    char* messages_json = malloc(messages_capacity);
    if (!messages_json) return NULL;

    strcpy(messages_json, "[");
    size_t offset = 1;

    ConvMessage* msg = conv->messages;
    bool first = true;
    while (msg) {
        char* escaped_content = json_escape(msg->content);
        if (!escaped_content) {
            free(messages_json);
            return NULL;
        }

        size_t needed = strlen(escaped_content) + strlen(msg->role) + 64;
        if (offset + needed >= messages_capacity) {
            messages_capacity = (messages_capacity + needed) * 2;
            messages_json = realloc(messages_json, messages_capacity);
            if (!messages_json) {
                free(escaped_content);
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
    curl_easy_setopt(g_curl, CURLOPT_URL, CLAUDE_API_URL);
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 120L);

    // Execute
    CURLcode res = curl_easy_perform(g_curl);

    curl_slist_free_all(headers);
    free(json_body);

    if (res != CURLE_OK) {
        fprintf(stderr, "Claude API error: %s\n", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        fprintf(stderr, "Claude API HTTP %ld: %s\n", http_code, response.data);
        free(response.data);
        return NULL;
    }

    // Extract text
    char* text = extract_response_text(response.data);
    free(response.data);

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
