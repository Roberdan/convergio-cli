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

// Maximum messages per session history
#define ACP_MAX_MESSAGES 100

// Lazy load: initial messages to send on resume
#define ACP_LAZY_LOAD_INITIAL 5

// Background execution buffer size
#define ACP_BACKGROUND_BUFFER_SIZE 65536

// Session message (for history)
typedef struct {
    char role[16];          // "user" or "assistant"
    char* content;          // Message content (dynamically allocated)
    long timestamp;         // Unix timestamp
} ACPMessage;

// Session state
typedef struct {
    char session_id[64];
    char agent_name[64];    // Agent server name (e.g., "Convergio-Ali")
    char cwd[1024];
    bool active;
    void* orchestrator_ctx; // Opaque pointer to orchestrator context
    // Message history for session resume
    ACPMessage messages[ACP_MAX_MESSAGES];
    int message_count;
    // Background execution support
    bool is_background;         // Session switched to background
    bool is_processing;         // Agent is still processing
    char* background_buffer;    // Buffered output while in background
    size_t background_buffer_len;
    size_t background_buffer_cap;
} ACPSession;

// Server state
typedef struct {
    ACPSession sessions[ACP_MAX_SESSIONS];
    int session_count;
    bool initialized;
    int protocol_version;
    char selected_agent[64];  // --agent argument, empty = orchestrator (Ali)
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
void acp_handle_session_load_more(int request_id, const char* params_json);
void acp_handle_session_background(int request_id, const char* params_json);
void acp_handle_session_foreground(int request_id, const char* params_json);
void acp_handle_session_status(int request_id, const char* params_json);

// Response helpers
void acp_send_response(int id, const char* result_json);
void acp_send_error(int id, int code, const char* message);
void acp_send_notification(const char* method, const char* params_json);

// Session persistence (for resume support)
int acp_session_save(ACPSession* session);
ACPSession* acp_session_load(const char* session_id);
void acp_session_add_message(ACPSession* session, const char* role, const char* content);

#endif // NOUS_ACP_H
