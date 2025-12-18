/**
 * ACP - Agent Client Protocol
 *
 * Implements the Agent Client Protocol for integration with Zed editor
 * and other ACP-compatible clients.
 *
 * Protocol: JSON-RPC 2.0 over stdio
 * Spec: https://agentclientprotocol.com
 */

#ifndef NOUS_ACP_H
#define NOUS_ACP_H

#include <stdbool.h>
#include <stddef.h>

// Protocol version
#define ACP_PROTOCOL_VERSION 1

// Maximum sessions
#define ACP_MAX_SESSIONS 16

// Maximum line length for JSON-RPC
#define ACP_MAX_LINE_LENGTH 65536

// Session state
typedef struct {
    char session_id[64];
    char cwd[1024];
    bool active;
    void* orchestrator_ctx;  // Opaque pointer to orchestrator context
} ACPSession;

// Server state
typedef struct {
    ACPSession sessions[ACP_MAX_SESSIONS];
    int session_count;
    bool initialized;
    int protocol_version;
} ACPServer;

// Streaming callback for sending updates to client
typedef void (*ACPStreamCallback)(const char* session_id, const char* delta, bool done);

/**
 * Initialize ACP server
 */
int acp_server_init(void);

/**
 * Run ACP server main loop (blocking)
 * Reads JSON-RPC from stdin, writes to stdout
 */
int acp_server_run(void);

/**
 * Shutdown ACP server
 */
void acp_server_shutdown(void);

// Internal handlers (exposed for testing)
void acp_handle_initialize(int request_id, const char* params_json);
void acp_handle_session_new(int request_id, const char* params_json);
void acp_handle_session_prompt(int request_id, const char* params_json);
void acp_handle_session_cancel(int request_id, const char* params_json);

// Response helpers
void acp_send_response(int id, const char* result_json);
void acp_send_error(int id, int code, const char* message);
void acp_send_notification(const char* method, const char* params_json);

#endif // NOUS_ACP_H
