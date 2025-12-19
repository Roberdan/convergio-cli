/**
 * ACP Server - Agent Client Protocol for Zed
 *
 * Implements JSON-RPC 2.0 over stdio for Zed integration.
 * This is a standalone binary that exposes Convergio agents via ACP.
 */

#include "nous/acp.h"
#include "nous/orchestrator.h"
#include "nous/config.h"
#include "nous/embedded_agents.h"
#include "nous/nous.h"
#include "nous/updater.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cjson/cJSON.h>

// ============================================================================
// CONTEXT SHARING (F2: Ali consapevole di tutte le conversazioni)
// ============================================================================

#define CONTEXT_DIR "~/.convergio/agent_context"
#define MAX_CONTEXT_SIZE 2048

// Expand ~ to home directory
static char* expand_path(const char* path) {
    if (path[0] != '~') {
        return strdup(path);
    }
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    size_t len = strlen(home) + strlen(path);
    char* expanded = malloc(len);
    snprintf(expanded, len, "%s%s", home, path + 1);
    return expanded;
}

// Ensure context directory exists
static void ensure_context_dir(void) {
    char* dir = expand_path(CONTEXT_DIR);
    mkdir(dir, 0755);
    free(dir);
}

// Save agent conversation context (summary of last interaction)
static void save_agent_context(const char* agent_name, const char* user_prompt, const char* agent_response) {
    if (!agent_name || !user_prompt || !agent_response) return;

    ensure_context_dir();

    char* dir = expand_path(CONTEXT_DIR);
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s.json", dir, agent_name);
    free(dir);

    // Create JSON with context
    cJSON* ctx = cJSON_CreateObject();
    cJSON_AddStringToObject(ctx, "agent", agent_name);
    cJSON_AddNumberToObject(ctx, "timestamp", (double)time(NULL));

    // Truncate to reasonable size
    char user_summary[MAX_CONTEXT_SIZE];
    char agent_summary[MAX_CONTEXT_SIZE];
    strncpy(user_summary, user_prompt, MAX_CONTEXT_SIZE - 1);
    user_summary[MAX_CONTEXT_SIZE - 1] = '\0';
    strncpy(agent_summary, agent_response, MAX_CONTEXT_SIZE - 1);
    agent_summary[MAX_CONTEXT_SIZE - 1] = '\0';

    cJSON_AddStringToObject(ctx, "last_user_message", user_summary);
    cJSON_AddStringToObject(ctx, "last_agent_response", agent_summary);

    char* json = cJSON_Print(ctx);
    cJSON_Delete(ctx);

    FILE* f = fopen(filepath, "w");
    if (f) {
        fputs(json, f);
        fclose(f);
    }
    free(json);
}

// Load all agent contexts for Ali to be aware of other conversations
static char* load_all_agent_contexts(void) {
    char* dir = expand_path(CONTEXT_DIR);
    DIR* d = opendir(dir);
    if (!d) {
        free(dir);
        return NULL;
    }

    // Build context summary
    size_t capacity = 8192;
    char* summary = malloc(capacity);
    size_t len = 0;
    len += snprintf(summary + len, capacity - len,
        "\n## Recent Agent Conversations (Context for Ali)\n\n");

    struct dirent* entry;
    int count = 0;

    while ((entry = readdir(d)) != NULL) {
        if (strstr(entry->d_name, ".json") == NULL) continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

        FILE* f = fopen(filepath, "r");
        if (!f) continue;

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        char* content = malloc(fsize + 1);
        fread(content, 1, fsize, f);
        content[fsize] = '\0';
        fclose(f);

        cJSON* ctx = cJSON_Parse(content);
        free(content);
        if (!ctx) continue;

        cJSON* agent = cJSON_GetObjectItem(ctx, "agent");
        cJSON* user_msg = cJSON_GetObjectItem(ctx, "last_user_message");
        cJSON* agent_resp = cJSON_GetObjectItem(ctx, "last_agent_response");

        if (agent && user_msg && agent_resp &&
            cJSON_IsString(agent) && cJSON_IsString(user_msg) && cJSON_IsString(agent_resp)) {
            len += snprintf(summary + len, capacity - len,
                "### %s\n"
                "**User asked**: %.200s%s\n"
                "**Agent replied**: %.300s%s\n\n",
                agent->valuestring,
                user_msg->valuestring,
                strlen(user_msg->valuestring) > 200 ? "..." : "",
                agent_resp->valuestring,
                strlen(agent_resp->valuestring) > 300 ? "..." : "");
            count++;
        }

        cJSON_Delete(ctx);

        if (len > capacity - 1024) break;  // Prevent overflow
    }

    closedir(d);
    free(dir);

    if (count == 0) {
        free(summary);
        return NULL;
    }

    return summary;
}

// Global server state
static ACPServer g_server = {0};
static volatile sig_atomic_t g_running = 1;

// Current streaming session for callbacks
static char g_current_session_id[64] = {0};

// Forward declarations
static void handle_signal(int sig);
static void dispatch_request(cJSON* request);
static char* generate_session_id(void);

// ============================================================================
// SIGNAL HANDLING
// ============================================================================

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

// ============================================================================
// JSON-RPC RESPONSE HELPERS
// ============================================================================

void acp_send_response(int id, const char* result_json) {
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(response, "id", id);

    if (result_json) {
        cJSON* result = cJSON_Parse(result_json);
        if (result) {
            cJSON_AddItemToObject(response, "result", result);
        } else {
            cJSON_AddNullToObject(response, "result");
        }
    } else {
        cJSON_AddNullToObject(response, "result");
    }

    char* json = cJSON_PrintUnformatted(response);
    fprintf(stdout, "%s\n", json);
    fflush(stdout);

    free(json);
    cJSON_Delete(response);
}

void acp_send_error(int id, int code, const char* message) {
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(response, "id", id);

    cJSON* error = cJSON_CreateObject();
    cJSON_AddNumberToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(response, "error", error);

    char* json = cJSON_PrintUnformatted(response);
    fprintf(stdout, "%s\n", json);
    fflush(stdout);

    free(json);
    cJSON_Delete(response);
}

void acp_send_notification(const char* method, const char* params_json) {
    cJSON* notification = cJSON_CreateObject();
    cJSON_AddStringToObject(notification, "jsonrpc", "2.0");
    cJSON_AddStringToObject(notification, "method", method);

    if (params_json) {
        cJSON* params = cJSON_Parse(params_json);
        if (params) {
            cJSON_AddItemToObject(notification, "params", params);
        }
    }

    char* json = cJSON_PrintUnformatted(notification);
    fprintf(stdout, "%s\n", json);
    fflush(stdout);

    free(json);
    cJSON_Delete(notification);
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

#define SESSIONS_DIR "~/.convergio/sessions"

// Get session file path
static char* get_session_filepath(const char* session_id) {
    char* dir = expand_path(SESSIONS_DIR);
    mkdir(dir, 0755);

    size_t filepath_len = strlen(dir) + strlen(session_id) + 10;
    char* filepath = malloc(filepath_len);
    snprintf(filepath, filepath_len, "%s/%s.json", dir, session_id);
    free(dir);
    return filepath;
}

// Find most recent session file for an agent name
static char* find_session_by_agent_name(const char* agent_name) {
    char* dir = expand_path(SESSIONS_DIR);
    DIR* d = opendir(dir);
    if (!d) {
        free(dir);
        return NULL;
    }

    char* best_session_id = NULL;
    long best_timestamp = 0;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strstr(entry->d_name, ".json") == NULL) continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

        FILE* f = fopen(filepath, "r");
        if (!f) continue;

        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        char* content = malloc(fsize + 1);
        fread(content, 1, fsize, f);
        content[fsize] = '\0';
        fclose(f);

        cJSON* root = cJSON_Parse(content);
        free(content);
        if (!root) continue;

        cJSON* saved_agent = cJSON_GetObjectItem(root, "agent_name");
        cJSON* timestamp = cJSON_GetObjectItem(root, "timestamp");
        cJSON* session_id = cJSON_GetObjectItem(root, "session_id");

        if (saved_agent && cJSON_IsString(saved_agent) &&
            session_id && cJSON_IsString(session_id) &&
            strcmp(saved_agent->valuestring, agent_name) == 0) {

            long ts = timestamp && cJSON_IsNumber(timestamp) ? (long)timestamp->valuedouble : 0;
            if (ts > best_timestamp) {
                best_timestamp = ts;
                if (best_session_id) free(best_session_id);
                best_session_id = strdup(session_id->valuestring);
            }
        }

        cJSON_Delete(root);
    }

    closedir(d);
    free(dir);
    return best_session_id;
}

// Save session to disk
int acp_session_save(ACPSession* session) {
    if (!session || !session->active) return -1;

    char* filepath = get_session_filepath(session->session_id);

    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "session_id", session->session_id);
    cJSON_AddStringToObject(root, "agent_name", session->agent_name);
    cJSON_AddStringToObject(root, "cwd", session->cwd);
    cJSON_AddNumberToObject(root, "timestamp", (double)time(NULL));

    // Save message history
    cJSON* messages = cJSON_CreateArray();
    for (int i = 0; i < session->message_count; i++) {
        cJSON* msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", session->messages[i].role);
        cJSON_AddStringToObject(msg, "content", session->messages[i].content ? session->messages[i].content : "");
        cJSON_AddNumberToObject(msg, "timestamp", (double)session->messages[i].timestamp);
        cJSON_AddItemToArray(messages, msg);
    }
    cJSON_AddItemToObject(root, "messages", messages);

    char* json = cJSON_Print(root);
    cJSON_Delete(root);

    FILE* f = fopen(filepath, "w");
    if (f) {
        fputs(json, f);
        fclose(f);
        free(json);
        free(filepath);
        return 0;
    }

    free(json);
    free(filepath);
    return -1;
}

// Load session from disk
ACPSession* acp_session_load(const char* session_id) {
    char* filepath = get_session_filepath(session_id);

    FILE* f = fopen(filepath, "r");
    if (!f) {
        free(filepath);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc(fsize + 1);
    fread(content, 1, fsize, f);
    content[fsize] = '\0';
    fclose(f);
    free(filepath);

    cJSON* root = cJSON_Parse(content);
    free(content);
    if (!root) return NULL;

    // Create new session slot
    if (g_server.session_count >= ACP_MAX_SESSIONS) {
        cJSON_Delete(root);
        return NULL;
    }

    ACPSession* session = &g_server.sessions[g_server.session_count++];
    memset(session, 0, sizeof(ACPSession));

    // Load basic info
    cJSON* item = cJSON_GetObjectItem(root, "session_id");
    if (item && cJSON_IsString(item)) {
        strncpy(session->session_id, item->valuestring, sizeof(session->session_id) - 1);
    }

    item = cJSON_GetObjectItem(root, "agent_name");
    if (item && cJSON_IsString(item)) {
        strncpy(session->agent_name, item->valuestring, sizeof(session->agent_name) - 1);
    }

    item = cJSON_GetObjectItem(root, "cwd");
    if (item && cJSON_IsString(item)) {
        strncpy(session->cwd, item->valuestring, sizeof(session->cwd) - 1);
    }

    // Load message history
    cJSON* messages = cJSON_GetObjectItem(root, "messages");
    if (messages && cJSON_IsArray(messages)) {
        cJSON* msg;
        cJSON_ArrayForEach(msg, messages) {
            if (session->message_count >= ACP_MAX_MESSAGES) break;

            cJSON* role = cJSON_GetObjectItem(msg, "role");
            cJSON* content_item = cJSON_GetObjectItem(msg, "content");
            cJSON* ts = cJSON_GetObjectItem(msg, "timestamp");

            if (role && cJSON_IsString(role) && content_item && cJSON_IsString(content_item)) {
                strncpy(session->messages[session->message_count].role,
                        role->valuestring, sizeof(session->messages[0].role) - 1);
                session->messages[session->message_count].content = strdup(content_item->valuestring);
                session->messages[session->message_count].timestamp =
                    ts && cJSON_IsNumber(ts) ? (long)ts->valuedouble : time(NULL);
                session->message_count++;
            }
        }
    }

    session->active = true;
    session->orchestrator_ctx = NULL;

    cJSON_Delete(root);
    return session;
}

// Add message to session history
void acp_session_add_message(ACPSession* session, const char* role, const char* content) {
    if (!session || !role || !content) return;
    if (session->message_count >= ACP_MAX_MESSAGES) {
        // Shift messages to make room (remove oldest)
        if (session->messages[0].content) free(session->messages[0].content);
        memmove(&session->messages[0], &session->messages[1],
                sizeof(ACPMessage) * (ACP_MAX_MESSAGES - 1));
        session->message_count--;
    }

    int idx = session->message_count++;
    strncpy(session->messages[idx].role, role, sizeof(session->messages[0].role) - 1);
    session->messages[idx].content = strdup(content);
    session->messages[idx].timestamp = time(NULL);
}

static char* generate_session_id(void) {
    static char id[64];
    snprintf(id, sizeof(id), "sess_%d_%ld", g_server.session_count + 1, time(NULL));
    return id;
}

static ACPSession* find_session(const char* session_id) {
    for (int i = 0; i < g_server.session_count; i++) {
        if (g_server.sessions[i].active &&
            strcmp(g_server.sessions[i].session_id, session_id) == 0) {
            return &g_server.sessions[i];
        }
    }
    return NULL;
}

static ACPSession* create_session(const char* cwd, const char* agent_name) {
    if (g_server.session_count >= ACP_MAX_SESSIONS) {
        return NULL;
    }

    ACPSession* session = &g_server.sessions[g_server.session_count++];
    memset(session, 0, sizeof(ACPSession));
    strncpy(session->session_id, generate_session_id(), sizeof(session->session_id) - 1);
    strncpy(session->cwd, cwd ? cwd : ".", sizeof(session->cwd) - 1);
    if (agent_name) {
        strncpy(session->agent_name, agent_name, sizeof(session->agent_name) - 1);
    } else if (g_server.selected_agent[0] != '\0') {
        // Use server's selected agent as default
        snprintf(session->agent_name, sizeof(session->agent_name), "Convergio-%s", g_server.selected_agent);
    }
    session->active = true;
    session->orchestrator_ctx = NULL;
    session->message_count = 0;

    return session;
}

// ============================================================================
// PROTOCOL HANDLERS
// ============================================================================

void acp_handle_initialize(int request_id, const char* params_json) {
    (void)params_json;

    g_server.initialized = true;
    g_server.protocol_version = ACP_PROTOCOL_VERSION;

    // Determine agent name and title
    const char* agent_name = "convergio";
    char agent_title[128] = "Convergio AI Assistant";

    if (g_server.selected_agent[0] != '\0') {
        // Specific agent selected
        ManagedAgent* agent = agent_find_by_name(g_server.selected_agent);
        if (agent) {
            agent_name = agent->name;
            snprintf(agent_title, sizeof(agent_title), "Convergio: %s", agent->name);
        }
    }

    // Build response following exact ACP schema
    cJSON* result = cJSON_CreateObject();
    cJSON_AddNumberToObject(result, "protocolVersion", ACP_PROTOCOL_VERSION);

    // agentInfo (required fields: name, version; optional: title)
    cJSON* agent_info = cJSON_CreateObject();
    cJSON_AddStringToObject(agent_info, "name", agent_name);
    cJSON_AddStringToObject(agent_info, "version", convergio_get_version());
    cJSON_AddStringToObject(agent_info, "title", agent_title);
    cJSON_AddItemToObject(result, "agentInfo", agent_info);

    // agentCapabilities (ACP schema format)
    cJSON* caps = cJSON_CreateObject();
    cJSON_AddBoolToObject(caps, "loadSession", false);

    cJSON* mcp_caps = cJSON_CreateObject();
    cJSON_AddBoolToObject(mcp_caps, "http", false);
    cJSON_AddBoolToObject(mcp_caps, "sse", false);
    cJSON_AddItemToObject(caps, "mcpCapabilities", mcp_caps);

    cJSON* prompt_caps = cJSON_CreateObject();
    cJSON_AddBoolToObject(prompt_caps, "image", false);
    cJSON_AddBoolToObject(prompt_caps, "audio", false);
    cJSON_AddBoolToObject(prompt_caps, "embeddedContext", true);  // X3: Enable editor context
    cJSON_AddItemToObject(caps, "promptCapabilities", prompt_caps);

    cJSON* session_caps = cJSON_CreateObject();
    cJSON_AddItemToObject(caps, "sessionCapabilities", session_caps);

    cJSON_AddItemToObject(result, "agentCapabilities", caps);

    // authMethods (empty array = no auth required)
    cJSON* auth = cJSON_CreateArray();
    cJSON_AddItemToObject(result, "authMethods", auth);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

void acp_handle_session_new(int request_id, const char* params_json) {
    char cwd[1024] = ".";
    char resume_session_id[64] = {0};

    if (params_json) {
        cJSON* params = cJSON_Parse(params_json);
        if (params) {
            cJSON* cwd_item = cJSON_GetObjectItem(params, "cwd");
            if (cwd_item && cJSON_IsString(cwd_item)) {
                strncpy(cwd, cwd_item->valuestring, sizeof(cwd) - 1);
            }
            // Check for resume session ID (optional, for explicit resume)
            cJSON* resume_item = cJSON_GetObjectItem(params, "resumeSessionId");
            if (resume_item && cJSON_IsString(resume_item)) {
                strncpy(resume_session_id, resume_item->valuestring, sizeof(resume_session_id) - 1);
            }
            cJSON_Delete(params);
        }
    }

    ACPSession* session = NULL;
    bool resumed = false;

    // Try to resume existing session by explicit ID
    if (resume_session_id[0] != '\0') {
        session = find_session(resume_session_id);
        if (!session) {
            session = acp_session_load(resume_session_id);
        }
        if (session) {
            resumed = true;
            if (cwd[0] != '.' || cwd[1] != '\0') {
                strncpy(session->cwd, cwd, sizeof(session->cwd) - 1);
            }
        }
    }

    // If no explicit session ID, try to auto-resume by agent name
    if (!session && g_server.selected_agent[0] != '\0') {
        char agent_name[128];
        snprintf(agent_name, sizeof(agent_name), "Convergio-%s", g_server.selected_agent);

        char* found_session_id = find_session_by_agent_name(agent_name);
        if (found_session_id) {
            // Check if already in memory
            session = find_session(found_session_id);
            if (!session) {
                session = acp_session_load(found_session_id);
            }
            if (session) {
                resumed = true;
                if (cwd[0] != '.' || cwd[1] != '\0') {
                    strncpy(session->cwd, cwd, sizeof(session->cwd) - 1);
                }
            }
            free(found_session_id);
        }
    }

    // Create new session if not resuming
    if (!session) {
        session = create_session(cwd, NULL);
        if (session) {
            // Save new session immediately so it persists even if no prompts are sent
            acp_session_save(session);
        }
    }

    if (!session) {
        acp_send_error(request_id, -32000, "Max sessions reached");
        return;
    }

    // Initialize workspace for tools
    extern void tools_init_workspace(const char* path);
    tools_init_workspace(session->cwd);

    // Build response
    cJSON* result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "sessionId", session->session_id);

    // If resumed, send previous messages as context
    if (resumed && session->message_count > 0) {
        cJSON* history = cJSON_CreateArray();
        for (int i = 0; i < session->message_count; i++) {
            cJSON* msg = cJSON_CreateObject();
            cJSON_AddStringToObject(msg, "role", session->messages[i].role);
            cJSON_AddStringToObject(msg, "content", session->messages[i].content ? session->messages[i].content : "");
            cJSON_AddItemToArray(history, msg);
        }
        cJSON_AddItemToObject(result, "history", history);
        cJSON_AddBoolToObject(result, "resumed", true);
        cJSON_AddNumberToObject(result, "messageCount", session->message_count);
    }

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

// Streaming callback for orchestrator
static void stream_callback(const char* chunk, void* user_data) {
    (void)user_data;

    if (!chunk || strlen(chunk) == 0) return;

    // Build session/update notification (ACP schema format)
    cJSON* params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "sessionId", g_current_session_id);

    cJSON* update = cJSON_CreateObject();
    cJSON_AddStringToObject(update, "sessionUpdate", "agent_message_chunk");

    // Content block: { "type": "text", "text": "..." }
    cJSON* content = cJSON_CreateObject();
    cJSON_AddStringToObject(content, "type", "text");
    cJSON_AddStringToObject(content, "text", chunk);
    cJSON_AddItemToObject(update, "content", content);

    cJSON_AddItemToObject(params, "update", update);

    char* params_json = cJSON_PrintUnformatted(params);
    acp_send_notification("session/update", params_json);
    free(params_json);
    cJSON_Delete(params);
}

void acp_handle_session_prompt(int request_id, const char* params_json) {
    if (!params_json) {
        acp_send_error(request_id, -32602, "Missing params");
        return;
    }

    cJSON* params = cJSON_Parse(params_json);
    if (!params) {
        acp_send_error(request_id, -32700, "Parse error");
        return;
    }

    // Get session ID
    cJSON* session_id_item = cJSON_GetObjectItem(params, "sessionId");
    if (!session_id_item || !cJSON_IsString(session_id_item)) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32602, "Missing sessionId");
        return;
    }

    // Copy session ID before freeing params (avoid use-after-free)
    char session_id[64] = {0};
    strncpy(session_id, session_id_item->valuestring, sizeof(session_id) - 1);

    ACPSession* session = find_session(session_id);
    if (!session) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32000, "Session not found");
        return;
    }

    // Extract prompt text and context (ACP format: prompt[])
    cJSON* prompt_array = cJSON_GetObjectItem(params, "prompt");
    char prompt_text[16384] = {0};  // Larger buffer for context
    char context_text[8192] = {0};  // Buffer for embedded context

    if (prompt_array && cJSON_IsArray(prompt_array)) {
        cJSON* item;
        cJSON_ArrayForEach(item, prompt_array) {
            cJSON* type = cJSON_GetObjectItem(item, "type");
            if (!type || !cJSON_IsString(type)) continue;

            if (strcmp(type->valuestring, "text") == 0) {
                // ACP format: { "type": "text", "text": "..." }
                cJSON* text = cJSON_GetObjectItem(item, "text");
                if (text && cJSON_IsString(text)) {
                    strncat(prompt_text, text->valuestring, sizeof(prompt_text) - strlen(prompt_text) - 1);
                }
            } else if (strcmp(type->valuestring, "context") == 0) {
                // X3: Handle embedded context (file, selection, cursor)
                // ACP format: { "type": "context", "path": "...", "content": "...", "selection": {...} }
                cJSON* path = cJSON_GetObjectItem(item, "path");
                cJSON* content = cJSON_GetObjectItem(item, "content");
                cJSON* selection = cJSON_GetObjectItem(item, "selection");

                size_t ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                if (path && cJSON_IsString(path) && ctx_remaining > 0) {
                    char ctx_header[512];
                    snprintf(ctx_header, sizeof(ctx_header), "\n[File: %s]\n", path->valuestring);
                    strncat(context_text, ctx_header, ctx_remaining);
                }
                if (content && cJSON_IsString(content)) {
                    ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                    strncat(context_text, content->valuestring, ctx_remaining);
                    strncat(context_text, "\n", ctx_remaining - 1);
                }
                if (selection && cJSON_IsObject(selection)) {
                    cJSON* sel_text = cJSON_GetObjectItem(selection, "text");
                    if (sel_text && cJSON_IsString(sel_text)) {
                        ctx_remaining = sizeof(context_text) - strlen(context_text) - 1;
                        char sel_header[64] = "\n[Selection]:\n";
                        strncat(context_text, sel_header, ctx_remaining);
                        strncat(context_text, sel_text->valuestring, ctx_remaining - 20);
                        strncat(context_text, "\n", 1);
                    }
                }
            }
        }
    }

    // Prepend context to prompt if available
    if (strlen(context_text) > 0) {
        char combined[24576];
        snprintf(combined, sizeof(combined), "[Editor Context]%s\n[User Message]\n%s",
                 context_text, prompt_text);
        strncpy(prompt_text, combined, sizeof(prompt_text) - 1);
    }

    cJSON_Delete(params);

    if (strlen(prompt_text) == 0) {
        acp_send_error(request_id, -32602, "Empty prompt");
        return;
    }

    // Set current session for callback
    strncpy(g_current_session_id, session_id, sizeof(g_current_session_id) - 1);

    char* response = NULL;

    // Build conversation history context from session (for resume support)
    char* history_context = NULL;
    if (session->message_count > 0) {
        size_t ctx_size = 16384;
        history_context = malloc(ctx_size);
        size_t ctx_len = 0;
        ctx_len += snprintf(history_context + ctx_len, ctx_size - ctx_len,
            "\n[Previous conversation history - continue from where we left off]\n");

        // Include recent messages (limit to last 10 for context window)
        int start = session->message_count > 10 ? session->message_count - 10 : 0;
        for (int i = start; i < session->message_count; i++) {
            const char* role = session->messages[i].role;
            const char* content = session->messages[i].content;
            if (content) {
                // Truncate very long messages
                size_t max_len = 500;
                ctx_len += snprintf(history_context + ctx_len, ctx_size - ctx_len,
                    "\n**%s**: %.500s%s\n",
                    strcmp(role, "user") == 0 ? "You" : "Assistant",
                    content,
                    strlen(content) > max_len ? "..." : "");
            }
            if (ctx_len > ctx_size - 1024) break;
        }
        ctx_len += snprintf(history_context + ctx_len, ctx_size - ctx_len,
            "\n[End of history - now responding to new message]\n\n");
    }

    // Route to specific agent or orchestrator
    if (g_server.selected_agent[0] != '\0') {
        // Specific agent selected - use direct agent chat (no streaming yet)
        ManagedAgent* agent = agent_find_by_name(g_server.selected_agent);
        if (agent) {
            // Build enhanced prompt with history context if available
            char* enhanced_prompt = prompt_text;
            if (history_context) {
                size_t ep_len = strlen(prompt_text) + strlen(history_context) + 64;
                enhanced_prompt = malloc(ep_len);
                snprintf(enhanced_prompt, ep_len, "%s%s", history_context, prompt_text);
            }

            response = orchestrator_agent_chat(agent, enhanced_prompt);

            if (enhanced_prompt != prompt_text) free(enhanced_prompt);

            // Send full response as single chunk
            if (response) {
                stream_callback(response, NULL);
                // Save context for other agents to see (F2: context sharing)
                save_agent_context(g_server.selected_agent, prompt_text, response);
            }
        } else {
            // Agent not found - fallback to orchestrator
            response = orchestrator_process_stream(prompt_text, stream_callback, NULL);
        }
    } else {
        // Default: orchestrator (Ali) with streaming
        // F2: Load context from other agent conversations
        char* agent_contexts = load_all_agent_contexts();
        if (agent_contexts) {
            // Prepend context info to the prompt for Ali
            size_t enhanced_len = strlen(prompt_text) + strlen(agent_contexts) + 256;
            char* enhanced_prompt = malloc(enhanced_len);
            snprintf(enhanced_prompt, enhanced_len,
                "%s\n\n---\n[Context: Recent conversations with other agents]\n%s\n---\n",
                prompt_text, agent_contexts);
            free(agent_contexts);
            response = orchestrator_process_stream(enhanced_prompt, stream_callback, NULL);
            free(enhanced_prompt);
        } else {
            response = orchestrator_process_stream(prompt_text, stream_callback, NULL);
        }
    }

    // Save messages to session history for resume support
    acp_session_add_message(session, "user", prompt_text);
    if (response) {
        acp_session_add_message(session, "assistant", response);
    }

    // Persist session to disk
    acp_session_save(session);

    // Cleanup history context
    if (history_context) {
        free(history_context);
    }

    // Send final response
    cJSON* result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "stopReason", "end_turn");

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);

    if (response) {
        free(response);
    }

    g_current_session_id[0] = '\0';
}

void acp_handle_session_cancel(int request_id, const char* params_json) {
    (void)params_json;
    // TODO: Implement cancellation
    cJSON* result = cJSON_CreateObject();
    cJSON_AddBoolToObject(result, "cancelled", true);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

// ============================================================================
// REQUEST DISPATCHER
// ============================================================================

static void dispatch_request(cJSON* request) {
    cJSON* id_item = cJSON_GetObjectItem(request, "id");
    cJSON* method_item = cJSON_GetObjectItem(request, "method");
    cJSON* params_item = cJSON_GetObjectItem(request, "params");

    int request_id = id_item ? id_item->valueint : 0;
    const char* method = method_item ? method_item->valuestring : "";

    char* params_json = NULL;
    if (params_item) {
        params_json = cJSON_PrintUnformatted(params_item);
    }

    // Dispatch based on method
    if (strcmp(method, "initialize") == 0) {
        acp_handle_initialize(request_id, params_json);
    } else if (strcmp(method, "session/new") == 0) {
        acp_handle_session_new(request_id, params_json);
    } else if (strcmp(method, "session/prompt") == 0) {
        acp_handle_session_prompt(request_id, params_json);
    } else if (strcmp(method, "session/cancel") == 0) {
        acp_handle_session_cancel(request_id, params_json);
    } else if (strcmp(method, "shutdown") == 0) {
        acp_send_response(request_id, "{}");
        g_running = 0;
    } else {
        acp_send_error(request_id, -32601, "Method not found");
    }

    if (params_json) {
        free(params_json);
    }
}

// ============================================================================
// MAIN LOOP
// ============================================================================

int acp_server_init(void) {
    // Initialize config
    if (convergio_config_init() != 0) {
        fprintf(stderr, "Failed to initialize config\n");
        return -1;
    }

    // Initialize auth
    extern int auth_init(void);
    if (auth_init() != 0) {
        fprintf(stderr, "Warning: No API key configured\n");
    }

    // Initialize core systems
    if (nous_init() != 0) {
        fprintf(stderr, "Failed to initialize nous\n");
        return -1;
    }

    // Initialize orchestrator with generous budget for ACP sessions
    if (orchestrator_init(100.0) != 0) {
        fprintf(stderr, "Failed to initialize orchestrator\n");
        return -1;
    }

    // Reset session to clear any budget_exceeded state from previous runs
    extern void cost_reset_session(void);
    cost_reset_session();

    return 0;
}

int acp_server_run(void) {
    char line[ACP_MAX_LINE_LENGTH];
    size_t line_pos = 0;

    // Setup signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Main loop - read JSON-RPC from stdin byte by byte
    while (g_running) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);

        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (n == 0) {
            // EOF - wait a bit and retry (pipe might not be ready yet)
            usleep(100000);
            continue;
        }

        // Build line
        if (c == '\n') {
            line[line_pos] = '\0';

            // Skip empty lines
            if (line_pos == 0) {
                continue;
            }

            // Parse JSON
            cJSON* request = cJSON_Parse(line);
            if (!request) {
                acp_send_error(-1, -32700, "Parse error");
            } else {
                dispatch_request(request);
                cJSON_Delete(request);
            }

            line_pos = 0;
        } else if (line_pos < sizeof(line) - 1) {
            line[line_pos++] = c;
        }
    }

    return 0;
}

void acp_server_shutdown(void) {
    orchestrator_shutdown();
    nous_shutdown();
    convergio_config_shutdown();
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

static void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s [--agent <name>] [--list-agents]\n", prog);
    fprintf(stderr, "  --agent <name>   Route to specific agent (default: ali)\n");
    fprintf(stderr, "  --list-agents    List available agents and exit\n");
}

static void list_agents(void) {
    // Quick init just to load agents
    convergio_config_init();
    nous_init();
    orchestrator_init(1.0);  // Minimal budget for listing

    ManagedAgent* agents[64];
    size_t count = agent_get_all(agents, 64);

    fprintf(stdout, "Available agents (%zu):\n", count);
    for (size_t i = 0; i < count; i++) {
        fprintf(stdout, "  %-20s  %s\n", agents[i]->name,
                agents[i]->description ? agents[i]->description : "");
    }

    orchestrator_shutdown();
    nous_shutdown();
    convergio_config_shutdown();
}

int main(int argc, char** argv) {
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--agent") == 0 && i + 1 < argc) {
            strncpy(g_server.selected_agent, argv[++i], sizeof(g_server.selected_agent) - 1);
        } else if (strcmp(argv[i], "--list-agents") == 0) {
            list_agents();
            return 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    // Disable buffering for real-time communication
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (acp_server_init() != 0) {
        return 1;
    }

    int result = acp_server_run();

    acp_server_shutdown();

    return result;
}
