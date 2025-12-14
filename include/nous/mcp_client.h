/**
 * CONVERGIO MCP CLIENT
 *
 * Generic Model Context Protocol client with:
 * - JSON-RPC 2.0 over stdio and HTTP transports
 * - Auto tool discovery
 * - Multi-server support
 * - Connection pooling
 *
 * Implements: MCP Specification 2025-06-18
 * See: https://modelcontextprotocol.io/specification/2025-06-18
 *
 * Part of Anna Executive Assistant feature.
 * See: ADR-009
 */

#ifndef NOUS_MCP_CLIENT_H
#define NOUS_MCP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration for cJSON
struct cJSON;
typedef struct cJSON cJSON;

// ============================================================================
// ENUMS
// ============================================================================

// Transport types
typedef enum {
    MCP_TRANSPORT_STDIO = 0,   // stdio (subprocess)
    MCP_TRANSPORT_HTTP = 1,    // HTTP POST
    MCP_TRANSPORT_SSE = 2      // Server-Sent Events
} MCPTransportType;

// Connection status
typedef enum {
    MCP_STATUS_DISCONNECTED = 0,
    MCP_STATUS_CONNECTING = 1,
    MCP_STATUS_CONNECTED = 2,
    MCP_STATUS_ERROR = 3
} MCPConnectionStatus;

// Error codes
typedef enum {
    MCP_OK = 0,
    MCP_ERROR_NOT_FOUND = -1,
    MCP_ERROR_CONNECT = -2,
    MCP_ERROR_TIMEOUT = -3,
    MCP_ERROR_PROTOCOL = -4,
    MCP_ERROR_TRANSPORT = -5,
    MCP_ERROR_AUTH = -6,
    MCP_ERROR_INVALID = -7,
    MCP_ERROR_UNKNOWN = -99
} MCPError;

// ============================================================================
// STRUCTURES
// ============================================================================

// Tool definition (discovered from server)
typedef struct {
    char* name;
    char* description;
    cJSON* input_schema;       // JSON Schema for parameters
    bool requires_confirmation;
} MCPTool;

// Resource definition
typedef struct {
    char* uri;
    char* name;
    char* description;
    char* mime_type;
} MCPResource;

// Prompt definition
typedef struct {
    char* name;
    char* description;
    cJSON* arguments;          // Array of argument definitions
} MCPPrompt;

// Server capabilities (discovered on connect)
typedef struct {
    bool supports_tools;
    bool supports_resources;
    bool supports_prompts;
    bool supports_logging;
    bool supports_sampling;
    char* protocol_version;
    char* server_name;
    char* server_version;
} MCPCapabilities;

// Server configuration
typedef struct {
    char* name;                // Unique server name
    bool enabled;
    MCPTransportType transport;

    // stdio transport
    char* command;             // Command to run
    char** args;               // Arguments
    int arg_count;
    char** env;                // KEY=VALUE pairs
    int env_count;
    char* working_dir;         // Working directory

    // HTTP transport
    char* url;
    char** headers;            // Header: Value pairs
    int header_count;

    // Common options
    int timeout_ms;            // Default: 30000
    int retry_count;           // Default: 3
    int retry_delay_ms;        // Default: 1000
} MCPServerConfig;

// Connected server state
typedef struct {
    char* name;
    MCPServerConfig* config;
    MCPConnectionStatus status;
    MCPCapabilities capabilities;

    // Discovered capabilities
    MCPTool* tools;
    int tool_count;
    MCPResource* resources;
    int resource_count;
    MCPPrompt* prompts;
    int prompt_count;

    // Transport state (internal)
    void* transport_data;

    // Request tracking
    int64_t next_request_id;

    // Error tracking
    char* last_error;
    int consecutive_errors;
    time_t last_success;
    time_t connected_at;
} MCPServer;

// Tool call result
typedef struct {
    bool is_error;
    cJSON* content;            // Array of content blocks
    char* error_message;
    int error_code;
} MCPToolResult;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize the MCP client subsystem.
 * Loads configuration from default or specified path.
 * @return 0 on success, -1 on error
 */
int mcp_init(void);

/**
 * Shutdown the MCP client subsystem.
 * Disconnects all servers and frees resources.
 */
void mcp_shutdown(void);

// ============================================================================
// CONFIGURATION
// ============================================================================

/**
 * Load MCP configuration from file.
 * @param config_path Path to config file (NULL for default ~/.convergio/mcp.json)
 * @return 0 on success, -1 on error
 */
int mcp_load_config(const char* config_path);

/**
 * Save current configuration to file.
 * @param config_path Path to config file (NULL for default)
 * @return 0 on success, -1 on error
 */
int mcp_save_config(const char* config_path);

/**
 * Get server configuration by name.
 * @param name Server name
 * @return Config or NULL if not found
 */
MCPServerConfig* mcp_get_server_config(const char* name);

/**
 * Add a new server configuration.
 * @param config Server configuration (copied)
 * @return 0 on success, -1 on error
 */
int mcp_add_server(const MCPServerConfig* config);

/**
 * Remove a server configuration.
 * @param name Server name
 * @return 0 on success, -1 on error
 */
int mcp_remove_server(const char* name);

/**
 * Enable a server.
 * @param name Server name
 * @return 0 on success, -1 on error
 */
int mcp_enable_server(const char* name);

/**
 * Disable a server.
 * @param name Server name
 * @return 0 on success, -1 on error
 */
int mcp_disable_server(const char* name);

/**
 * List all configured server names.
 * @param count Output: number of servers
 * @return Array of server names (caller must free)
 */
char** mcp_list_servers(int* count);

/**
 * List enabled server names.
 * @param count Output: number of servers
 * @return Array of server names (caller must free)
 */
char** mcp_list_enabled_servers(int* count);

// ============================================================================
// CONNECTION MANAGEMENT
// ============================================================================

/**
 * Connect to a server.
 * Performs handshake and discovers capabilities.
 * @param name Server name
 * @return 0 on success, MCP error code on failure
 */
int mcp_connect(const char* name);

/**
 * Connect to all enabled servers.
 * @return Number of successful connections
 */
int mcp_connect_all(void);

/**
 * Disconnect from a server.
 * @param name Server name
 * @return 0 on success, -1 on error
 */
int mcp_disconnect(const char* name);

/**
 * Disconnect from all servers.
 */
void mcp_disconnect_all(void);

/**
 * Reconnect to a server.
 * @param name Server name
 * @return 0 on success, MCP error code on failure
 */
int mcp_reconnect(const char* name);

/**
 * Get connected server by name.
 * @param name Server name
 * @return Server or NULL if not connected
 */
MCPServer* mcp_get_server(const char* name);

/**
 * Get connection status for a server.
 * @param name Server name
 * @return Connection status
 */
MCPConnectionStatus mcp_get_status(const char* name);

/**
 * List connected server names.
 * @param count Output: number of connected servers
 * @return Array of server names (caller must free)
 */
char** mcp_list_connected(int* count);

// ============================================================================
// TOOL DISCOVERY
// ============================================================================

/**
 * Refresh tools from a server.
 * Called automatically on connect.
 * @param name Server name
 * @return 0 on success, -1 on error
 */
int mcp_refresh_tools(const char* name);

/**
 * Get a tool by name from a specific server.
 * @param server_name Server name
 * @param tool_name Tool name
 * @return Tool or NULL if not found
 */
MCPTool* mcp_get_tool(const char* server_name, const char* tool_name);

/**
 * List tools from a specific server.
 * @param server_name Server name
 * @param count Output: number of tools
 * @return Array of tools (do not free, owned by server)
 */
MCPTool** mcp_list_tools(const char* server_name, int* count);

/**
 * List all tools from all connected servers.
 * @param count Output: number of tools
 * @return Array of {server_name, tool} pairs (caller must free array, not contents)
 */
typedef struct {
    const char* server_name;
    MCPTool* tool;
} MCPToolRef;

MCPToolRef* mcp_list_all_tools(int* count);

/**
 * Find a tool by name across all servers.
 * @param tool_name Tool name
 * @param server_name Output: server that has the tool
 * @return Tool or NULL if not found
 */
MCPTool* mcp_find_tool(const char* tool_name, const char** server_name);

// ============================================================================
// TOOL INVOCATION
// ============================================================================

/**
 * Call a tool synchronously.
 * @param server_name Server name
 * @param tool_name Tool name
 * @param arguments JSON arguments object
 * @return Tool result (caller must free with mcp_free_result)
 */
MCPToolResult* mcp_call_tool(const char* server_name, const char* tool_name,
                              cJSON* arguments);

/**
 * Call a tool by auto-discovering which server has it.
 * @param tool_name Tool name
 * @param arguments JSON arguments object
 * @return Tool result (caller must free with mcp_free_result)
 */
MCPToolResult* mcp_call_tool_auto(const char* tool_name, cJSON* arguments);

/**
 * Free a tool result.
 */
void mcp_free_result(MCPToolResult* result);

// ============================================================================
// RESOURCE ACCESS
// ============================================================================

/**
 * List resources from a server.
 * @param server_name Server name
 * @param count Output: number of resources
 * @return Array of resources (do not free, owned by server)
 */
MCPResource** mcp_list_resources(const char* server_name, int* count);

/**
 * Read a resource.
 * @param server_name Server name
 * @param uri Resource URI
 * @return Resource content as JSON (caller must free with cJSON_Delete)
 */
cJSON* mcp_read_resource(const char* server_name, const char* uri);

// ============================================================================
// PROMPTS
// ============================================================================

/**
 * List prompts from a server.
 * @param server_name Server name
 * @param count Output: number of prompts
 * @return Array of prompts (do not free, owned by server)
 */
MCPPrompt** mcp_list_prompts(const char* server_name, int* count);

/**
 * Get a prompt with arguments filled in.
 * @param server_name Server name
 * @param prompt_name Prompt name
 * @param arguments Prompt arguments
 * @return Prompt messages as JSON array (caller must free)
 */
cJSON* mcp_get_prompt(const char* server_name, const char* prompt_name,
                       cJSON* arguments);

// ============================================================================
// ERROR HANDLING
// ============================================================================

/**
 * Get last error message for a server.
 * @param server_name Server name (NULL for global error)
 * @return Error message or NULL
 */
const char* mcp_get_last_error(const char* server_name);

/**
 * Clear error for a server.
 * @param server_name Server name (NULL for global)
 */
void mcp_clear_error(const char* server_name);

// ============================================================================
// HEALTH
// ============================================================================

typedef struct {
    int total_servers;
    int connected_servers;
    int servers_with_errors;
    struct {
        char* name;
        MCPConnectionStatus status;
        int tool_count;
        time_t last_success;
        char* last_error;
    }* server_status;
    int server_count;
} MCPHealth;

/**
 * Get health information for all servers.
 * @return Health struct (caller must free with mcp_free_health)
 */
MCPHealth* mcp_get_health(void);

/**
 * Free health struct.
 */
void mcp_free_health(MCPHealth* health);

/**
 * Print health to stdout.
 */
void mcp_print_health(void);

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

/**
 * Free a server config.
 */
void mcp_free_config(MCPServerConfig* config);

/**
 * Free a string array.
 */
void mcp_free_strings(char** strings, int count);

#ifdef __cplusplus
}
#endif

#endif // NOUS_MCP_CLIENT_H
