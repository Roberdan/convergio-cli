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
#include <cjson/cJSON.h>

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

static ACPSession* create_session(const char* cwd) {
    if (g_server.session_count >= ACP_MAX_SESSIONS) {
        return NULL;
    }

    ACPSession* session = &g_server.sessions[g_server.session_count++];
    strncpy(session->session_id, generate_session_id(), sizeof(session->session_id) - 1);
    strncpy(session->cwd, cwd ? cwd : ".", sizeof(session->cwd) - 1);
    session->active = true;
    session->orchestrator_ctx = NULL;

    return session;
}

// ============================================================================
// PROTOCOL HANDLERS
// ============================================================================

void acp_handle_initialize(int request_id, const char* params_json) {
    (void)params_json;

    g_server.initialized = true;
    g_server.protocol_version = ACP_PROTOCOL_VERSION;

    // Build capabilities
    cJSON* result = cJSON_CreateObject();
    cJSON_AddNumberToObject(result, "protocolVersion", ACP_PROTOCOL_VERSION);

    // Agent info
    cJSON* agent_info = cJSON_CreateObject();
    cJSON_AddStringToObject(agent_info, "name", "convergio");
    cJSON_AddStringToObject(agent_info, "title", "Convergio");
    cJSON_AddStringToObject(agent_info, "version", convergio_get_version());
    cJSON_AddItemToObject(result, "agentInfo", agent_info);

    // Capabilities
    cJSON* caps = cJSON_CreateObject();
    cJSON_AddBoolToObject(caps, "streaming", true);
    cJSON_AddBoolToObject(caps, "tools", true);
    cJSON_AddBoolToObject(caps, "sessionLoad", false);  // Not implemented yet
    cJSON_AddItemToObject(result, "agentCapabilities", caps);

    // Available agents
    cJSON* agents = cJSON_CreateArray();
    size_t agent_count = 0;
    const EmbeddedAgent* embedded = get_all_embedded_agents(&agent_count);

    for (size_t i = 0; i < agent_count && i < 50; i++) {  // Limit to 50
        cJSON* agent_obj = cJSON_CreateObject();

        // Use filename (without .md) as ID
        char agent_id[128] = {0};
        strncpy(agent_id, embedded[i].filename, sizeof(agent_id) - 1);
        char* dot = strrchr(agent_id, '.');
        if (dot) *dot = '\0';

        // Extract name from first line (# Agent Name)
        char agent_name[128] = {0};
        const char* content = embedded[i].content;
        if (content && content[0] == '#') {
            const char* start = content + 1;
            while (*start == ' ') start++;
            const char* end = strchr(start, '\n');
            if (end) {
                size_t len = (size_t)(end - start);
                if (len > sizeof(agent_name) - 1) len = sizeof(agent_name) - 1;
                strncpy(agent_name, start, len);
            }
        }
        if (agent_name[0] == '\0') {
            strncpy(agent_name, agent_id, sizeof(agent_name) - 1);
        }

        cJSON_AddStringToObject(agent_obj, "id", agent_id);
        cJSON_AddStringToObject(agent_obj, "name", agent_name);
        cJSON_AddStringToObject(agent_obj, "description", agent_name);  // Use name as description for now
        cJSON_AddItemToArray(agents, agent_obj);
    }
    cJSON_AddItemToObject(result, "agents", agents);

    // No auth required
    cJSON* auth = cJSON_CreateArray();
    cJSON_AddItemToObject(result, "authMethods", auth);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

void acp_handle_session_new(int request_id, const char* params_json) {
    const char* cwd = ".";

    if (params_json) {
        cJSON* params = cJSON_Parse(params_json);
        if (params) {
            cJSON* cwd_item = cJSON_GetObjectItem(params, "cwd");
            if (cwd_item && cJSON_IsString(cwd_item)) {
                cwd = cwd_item->valuestring;
            }
            cJSON_Delete(params);
        }
    }

    ACPSession* session = create_session(cwd);
    if (!session) {
        acp_send_error(request_id, -32000, "Max sessions reached");
        return;
    }

    // Initialize workspace for tools
    extern void tools_init_workspace(const char* path);
    tools_init_workspace(session->cwd);

    cJSON* result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "sessionId", session->session_id);

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);
}

// Streaming callback for orchestrator
static void stream_callback(const char* chunk, void* user_data) {
    (void)user_data;

    if (!chunk || strlen(chunk) == 0) return;

    // Build session/update notification with agent_message_chunk
    cJSON* params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "sessionId", g_current_session_id);

    cJSON* update = cJSON_CreateObject();
    cJSON_AddStringToObject(update, "type", "agent_message_chunk");

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

    const char* session_id = session_id_item->valuestring;
    ACPSession* session = find_session(session_id);
    if (!session) {
        cJSON_Delete(params);
        acp_send_error(request_id, -32000, "Session not found");
        return;
    }

    // Extract prompt text
    cJSON* prompt_array = cJSON_GetObjectItem(params, "prompt");
    char prompt_text[8192] = {0};

    if (prompt_array && cJSON_IsArray(prompt_array)) {
        cJSON* item;
        cJSON_ArrayForEach(item, prompt_array) {
            cJSON* type = cJSON_GetObjectItem(item, "type");
            if (type && cJSON_IsString(type) && strcmp(type->valuestring, "text") == 0) {
                cJSON* text = cJSON_GetObjectItem(item, "text");
                if (text && cJSON_IsString(text)) {
                    strncat(prompt_text, text->valuestring, sizeof(prompt_text) - strlen(prompt_text) - 1);
                }
            }
        }
    }

    cJSON_Delete(params);

    if (strlen(prompt_text) == 0) {
        acp_send_error(request_id, -32602, "Empty prompt");
        return;
    }

    // Set current session for callback
    strncpy(g_current_session_id, session_id, sizeof(g_current_session_id) - 1);

    // Process with streaming
    char* response = orchestrator_process_stream(prompt_text, stream_callback, NULL);

    // Send final response
    cJSON* result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "stopReason", "end_turn");

    if (response) {
        // Note: response already streamed, just confirm completion
        free(response);
    }

    char* result_json = cJSON_PrintUnformatted(result);
    acp_send_response(request_id, result_json);
    free(result_json);
    cJSON_Delete(result);

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

    // Initialize orchestrator with default budget
    if (orchestrator_init(5.0) != 0) {
        fprintf(stderr, "Failed to initialize orchestrator\n");
        return -1;
    }

    return 0;
}

int acp_server_run(void) {
    char line[ACP_MAX_LINE_LENGTH];

    // Setup signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Main loop - read JSON-RPC from stdin
    while (g_running && fgets(line, sizeof(line), stdin)) {
        // Skip empty lines
        size_t len = strlen(line);
        if (len == 0 || (len == 1 && line[0] == '\n')) {
            continue;
        }

        // Parse JSON
        cJSON* request = cJSON_Parse(line);
        if (!request) {
            acp_send_error(-1, -32700, "Parse error");
            continue;
        }

        // Dispatch
        dispatch_request(request);
        cJSON_Delete(request);
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

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    // Disable buffering for real-time communication
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (acp_server_init() != 0) {
        fprintf(stderr, "Failed to initialize ACP server\n");
        return 1;
    }

    int result = acp_server_run();

    acp_server_shutdown();

    return result;
}
