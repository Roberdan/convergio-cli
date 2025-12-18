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
    cJSON_AddBoolToObject(prompt_caps, "embeddedContext", false);
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

    if (params_json) {
        cJSON* params = cJSON_Parse(params_json);
        if (params) {
            cJSON* cwd_item = cJSON_GetObjectItem(params, "cwd");
            if (cwd_item && cJSON_IsString(cwd_item)) {
                strncpy(cwd, cwd_item->valuestring, sizeof(cwd) - 1);
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

    // Extract prompt text (ACP format: prompt[].text)
    cJSON* prompt_array = cJSON_GetObjectItem(params, "prompt");
    char prompt_text[8192] = {0};

    if (prompt_array && cJSON_IsArray(prompt_array)) {
        cJSON* item;
        cJSON_ArrayForEach(item, prompt_array) {
            cJSON* type = cJSON_GetObjectItem(item, "type");
            if (type && cJSON_IsString(type) && strcmp(type->valuestring, "text") == 0) {
                // ACP format: { "type": "text", "text": "..." }
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

    char* response = NULL;

    // Route to specific agent or orchestrator
    if (g_server.selected_agent[0] != '\0') {
        // Specific agent selected - use direct agent chat (no streaming yet)
        ManagedAgent* agent = agent_find_by_name(g_server.selected_agent);
        if (agent) {
            response = orchestrator_agent_chat(agent, prompt_text);
            // Send full response as single chunk
            if (response) {
                stream_callback(response, NULL);
            }
        } else {
            // Agent not found - fallback to orchestrator
            response = orchestrator_process_stream(prompt_text, stream_callback, NULL);
        }
    } else {
        // Default: orchestrator (Ali) with streaming
        response = orchestrator_process_stream(prompt_text, stream_callback, NULL);
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
