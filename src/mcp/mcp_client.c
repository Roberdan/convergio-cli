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

#include "nous/mcp_client.h"
#include "nous/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <poll.h>
#include <sys/wait.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_MCP_SERVERS 16
#define DEFAULT_TIMEOUT_MS 30000
#define DEFAULT_RETRY_COUNT 3
#define DEFAULT_RETRY_DELAY_MS 1000
#define MCP_PROTOCOL_VERSION "2024-11-05"
#define MCP_CONFIG_FILE "~/.convergio/mcp.json"
#define READ_BUFFER_SIZE 65536

// ============================================================================
// STATIC DATA
// ============================================================================

// Server registry
static struct {
    MCPServerConfig* configs[MAX_MCP_SERVERS];
    int config_count;
    MCPServer* servers[MAX_MCP_SERVERS];
    int server_count;
    pthread_mutex_t lock;
    bool initialized;
    char last_error[256];
} g_mcp = {0};

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// JSON-RPC
static char* jsonrpc_create_request(int64_t id, const char* method, cJSON* params);
static cJSON* jsonrpc_parse_response(const char* json_str, bool* is_error, char** error_msg);

// Transport - stdio
static int stdio_connect(MCPServer* server);
static void stdio_disconnect(MCPServer* server);
static char* stdio_send_receive(MCPServer* server, const char* request, int timeout_ms);

// Transport - HTTP
static int http_connect(MCPServer* server);
static void http_disconnect(MCPServer* server);
static char* http_send_receive(MCPServer* server, const char* request, int timeout_ms);

// Protocol
static int mcp_handshake(MCPServer* server);
static int mcp_discover_tools(MCPServer* server);
static int mcp_discover_resources(MCPServer* server);
static int mcp_discover_prompts(MCPServer* server);

// Helpers
static char* expand_path(const char* path);
static char* expand_env_vars(const char* str);
static void free_server(MCPServer* server);
static void free_config(MCPServerConfig* config);
static MCPServer* find_server(const char* name);

// ============================================================================
// INITIALIZATION
// ============================================================================

int mcp_init(void) {
    if (g_mcp.initialized) return 0;

    pthread_mutex_init(&g_mcp.lock, NULL);

    // Note: curl_global_init is called once in main.c at startup
    g_mcp.initialized = true;

    // Try to load default config
    mcp_load_config(NULL);

    return 0;
}

void mcp_shutdown(void) {
    if (!g_mcp.initialized) return;

    // Disconnect all servers
    mcp_disconnect_all();

    // Free all configs
    for (int i = 0; i < g_mcp.config_count; i++) {
        free_config(g_mcp.configs[i]);
        g_mcp.configs[i] = NULL;
    }
    g_mcp.config_count = 0;

    curl_global_cleanup();
    pthread_mutex_destroy(&g_mcp.lock);

    g_mcp.initialized = false;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

int mcp_load_config(const char* config_path) {
    char* path = config_path ? strdup(config_path) : expand_path(MCP_CONFIG_FILE);
    if (!path) return -1;

    FILE* f = fopen(path, "r");
    if (!f) {
        // Config file doesn't exist - that's OK, just use empty config
        free(path);
        return 0;
    }

    // Read file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc((size_t)size + 1);
    if (!content) {
        fclose(f);
        free(path);
        return -1;
    }

    size_t read = fread(content, 1, (size_t)size, f);
    content[read] = '\0';
    fclose(f);
    free(path);

    // Parse JSON
    cJSON* json = cJSON_Parse(content);
    free(content);

    if (!json) {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "Failed to parse MCP config: %s", cJSON_GetErrorPtr());
        return -1;
    }

    // Parse servers
    cJSON* servers = cJSON_GetObjectItem(json, "servers");
    if (servers && cJSON_IsObject(servers)) {
        cJSON* server;
        cJSON_ArrayForEach(server, servers) {
            MCPServerConfig* config = calloc(1, sizeof(MCPServerConfig));
            config->name = strdup(server->string);

            // Enabled
            cJSON* enabled = cJSON_GetObjectItem(server, "enabled");
            config->enabled = enabled ? cJSON_IsTrue(enabled) : true;

            // Transport
            cJSON* transport = cJSON_GetObjectItem(server, "transport");
            if (transport && cJSON_IsString(transport)) {
                if (strcmp(transport->valuestring, "stdio") == 0) {
                    config->transport = MCP_TRANSPORT_STDIO;
                } else if (strcmp(transport->valuestring, "http") == 0) {
                    config->transport = MCP_TRANSPORT_HTTP;
                } else if (strcmp(transport->valuestring, "sse") == 0) {
                    config->transport = MCP_TRANSPORT_SSE;
                }
            }

            // stdio transport settings
            cJSON* command = cJSON_GetObjectItem(server, "command");
            if (command && cJSON_IsString(command)) {
                config->command = strdup(command->valuestring);
            }

            cJSON* args = cJSON_GetObjectItem(server, "args");
            if (args && cJSON_IsArray(args)) {
                config->arg_count = cJSON_GetArraySize(args);
                config->args = calloc((size_t)config->arg_count, sizeof(char*));
                for (int i = 0; i < config->arg_count; i++) {
                    cJSON* arg = cJSON_GetArrayItem(args, i);
                    if (arg && cJSON_IsString(arg)) {
                        config->args[i] = strdup(arg->valuestring);
                    }
                }
            }

            cJSON* env = cJSON_GetObjectItem(server, "env");
            if (env && cJSON_IsObject(env)) {
                config->env_count = cJSON_GetArraySize(env);
                config->env = calloc((size_t)config->env_count, sizeof(char*));
                int i = 0;
                cJSON* item;
                cJSON_ArrayForEach(item, env) {
                    char* expanded_value = expand_env_vars(item->valuestring);
                    size_t len = strlen(item->string) + strlen(expanded_value) + 2;
                    config->env[i] = malloc(len);
                    snprintf(config->env[i], len, "%s=%s", item->string, expanded_value);
                    free(expanded_value);
                    i++;
                }
            }

            // HTTP transport settings
            cJSON* url = cJSON_GetObjectItem(server, "url");
            if (url && cJSON_IsString(url)) {
                config->url = strdup(url->valuestring);
            }

            cJSON* headers = cJSON_GetObjectItem(server, "headers");
            if (headers && cJSON_IsObject(headers)) {
                config->header_count = cJSON_GetArraySize(headers);
                config->headers = calloc((size_t)config->header_count, sizeof(char*));
                int i = 0;
                cJSON* item;
                cJSON_ArrayForEach(item, headers) {
                    char* expanded_value = expand_env_vars(item->valuestring);
                    size_t len = strlen(item->string) + strlen(expanded_value) + 3;
                    config->headers[i] = malloc(len);
                    snprintf(config->headers[i], len, "%s: %s", item->string, expanded_value);
                    free(expanded_value);
                    i++;
                }
            }

            // Timeouts
            cJSON* timeout = cJSON_GetObjectItem(server, "timeout_ms");
            config->timeout_ms = timeout ? (int)cJSON_GetNumberValue(timeout) : DEFAULT_TIMEOUT_MS;

            cJSON* retry_count = cJSON_GetObjectItem(server, "retry_count");
            config->retry_count = retry_count ? (int)cJSON_GetNumberValue(retry_count) : DEFAULT_RETRY_COUNT;

            cJSON* retry_delay = cJSON_GetObjectItem(server, "retry_delay_ms");
            config->retry_delay_ms = retry_delay ? (int)cJSON_GetNumberValue(retry_delay) : DEFAULT_RETRY_DELAY_MS;

            // Add to registry
            if (g_mcp.config_count < MAX_MCP_SERVERS) {
                g_mcp.configs[g_mcp.config_count++] = config;
            } else {
                free_config(config);
            }
        }
    }

    cJSON_Delete(json);
    return 0;
}

int mcp_save_config(const char* config_path) {
    char* path = config_path ? strdup(config_path) : expand_path(MCP_CONFIG_FILE);
    if (!path) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON* servers = cJSON_CreateObject();

    for (int i = 0; i < g_mcp.config_count; i++) {
        MCPServerConfig* config = g_mcp.configs[i];
        cJSON* server = cJSON_CreateObject();

        cJSON_AddBoolToObject(server, "enabled", config->enabled);

        const char* transport = "stdio";
        if (config->transport == MCP_TRANSPORT_HTTP) transport = "http";
        else if (config->transport == MCP_TRANSPORT_SSE) transport = "sse";
        cJSON_AddStringToObject(server, "transport", transport);

        if (config->command) {
            cJSON_AddStringToObject(server, "command", config->command);
        }

        if (config->args && config->arg_count > 0) {
            cJSON* args = cJSON_CreateArray();
            for (int j = 0; j < config->arg_count; j++) {
                cJSON_AddItemToArray(args, cJSON_CreateString(config->args[j]));
            }
            cJSON_AddItemToObject(server, "args", args);
        }

        if (config->url) {
            cJSON_AddStringToObject(server, "url", config->url);
        }

        cJSON_AddNumberToObject(server, "timeout_ms", config->timeout_ms);

        cJSON_AddItemToObject(servers, config->name, server);
    }

    cJSON_AddItemToObject(json, "servers", servers);

    char* output = cJSON_Print(json);
    cJSON_Delete(json);

    FILE* f = fopen(path, "w");
    if (!f) {
        free(path);
        free(output);
        return -1;
    }

    fputs(output, f);
    fclose(f);
    free(path);
    free(output);

    return 0;
}

MCPServerConfig* mcp_get_server_config(const char* name) {
    for (int i = 0; i < g_mcp.config_count; i++) {
        if (strcmp(g_mcp.configs[i]->name, name) == 0) {
            return g_mcp.configs[i];
        }
    }
    return NULL;
}

int mcp_add_server(const MCPServerConfig* config) {
    if (!config || !config->name) return -1;
    if (g_mcp.config_count >= MAX_MCP_SERVERS) return -1;

    // Check for duplicate
    if (mcp_get_server_config(config->name)) {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "Server '%s' already exists", config->name);
        return -1;
    }

    // Copy config
    MCPServerConfig* new_config = calloc(1, sizeof(MCPServerConfig));
    new_config->name = strdup(config->name);
    new_config->enabled = config->enabled;
    new_config->transport = config->transport;

    if (config->command) new_config->command = strdup(config->command);
    if (config->url) new_config->url = strdup(config->url);
    if (config->working_dir) new_config->working_dir = strdup(config->working_dir);

    new_config->timeout_ms = config->timeout_ms ? config->timeout_ms : DEFAULT_TIMEOUT_MS;
    new_config->retry_count = config->retry_count ? config->retry_count : DEFAULT_RETRY_COUNT;
    new_config->retry_delay_ms = config->retry_delay_ms ? config->retry_delay_ms : DEFAULT_RETRY_DELAY_MS;

    // Copy args
    if (config->args && config->arg_count > 0) {
        new_config->arg_count = config->arg_count;
        new_config->args = calloc((size_t)config->arg_count, sizeof(char*));
        for (int i = 0; i < config->arg_count; i++) {
            new_config->args[i] = strdup(config->args[i]);
        }
    }

    // Copy env
    if (config->env && config->env_count > 0) {
        new_config->env_count = config->env_count;
        new_config->env = calloc((size_t)config->env_count, sizeof(char*));
        for (int i = 0; i < config->env_count; i++) {
            new_config->env[i] = strdup(config->env[i]);
        }
    }

    // Copy headers
    if (config->headers && config->header_count > 0) {
        new_config->header_count = config->header_count;
        new_config->headers = calloc((size_t)config->header_count, sizeof(char*));
        for (int i = 0; i < config->header_count; i++) {
            new_config->headers[i] = strdup(config->headers[i]);
        }
    }

    g_mcp.configs[g_mcp.config_count++] = new_config;
    return 0;
}

int mcp_remove_server(const char* name) {
    // Disconnect first
    mcp_disconnect(name);

    // Find and remove config
    for (int i = 0; i < g_mcp.config_count; i++) {
        if (strcmp(g_mcp.configs[i]->name, name) == 0) {
            free_config(g_mcp.configs[i]);

            // Shift remaining configs
            for (int j = i; j < g_mcp.config_count - 1; j++) {
                g_mcp.configs[j] = g_mcp.configs[j + 1];
            }
            g_mcp.config_count--;
            return 0;
        }
    }

    return -1;
}

int mcp_enable_server(const char* name) {
    MCPServerConfig* config = mcp_get_server_config(name);
    if (!config) return -1;
    config->enabled = true;
    return 0;
}

int mcp_disable_server(const char* name) {
    MCPServerConfig* config = mcp_get_server_config(name);
    if (!config) return -1;
    config->enabled = false;
    mcp_disconnect(name);
    return 0;
}

char** mcp_list_servers(int* count) {
    if (g_mcp.config_count == 0) {
        *count = 0;
        return NULL;
    }

    char** list = calloc((size_t)g_mcp.config_count, sizeof(char*));
    for (int i = 0; i < g_mcp.config_count; i++) {
        list[i] = strdup(g_mcp.configs[i]->name);
    }

    *count = g_mcp.config_count;
    return list;
}

char** mcp_list_enabled_servers(int* count) {
    int enabled_count = 0;
    for (int i = 0; i < g_mcp.config_count; i++) {
        if (g_mcp.configs[i]->enabled) enabled_count++;
    }

    if (enabled_count == 0) {
        *count = 0;
        return NULL;
    }

    char** list = calloc((size_t)enabled_count, sizeof(char*));
    int j = 0;
    for (int i = 0; i < g_mcp.config_count; i++) {
        if (g_mcp.configs[i]->enabled) {
            list[j++] = strdup(g_mcp.configs[i]->name);
        }
    }

    *count = enabled_count;
    return list;
}

// ============================================================================
// CONNECTION MANAGEMENT
// ============================================================================

int mcp_connect(const char* name) {
    MCPServerConfig* config = mcp_get_server_config(name);
    if (!config) {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "Server '%s' not found", name);
        return MCP_ERROR_NOT_FOUND;
    }

    if (!config->enabled) {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "Server '%s' is disabled", name);
        return MCP_ERROR_INVALID;
    }

    // Check if already connected
    MCPServer* existing = find_server(name);
    if (existing && existing->status == MCP_STATUS_CONNECTED) {
        return MCP_OK;
    }

    // Create server instance
    MCPServer* server = calloc(1, sizeof(MCPServer));
    server->name = strdup(name);
    server->config = config;
    server->status = MCP_STATUS_CONNECTING;
    server->next_request_id = 1;

    // Connect based on transport
    int result;
    if (config->transport == MCP_TRANSPORT_STDIO) {
        result = stdio_connect(server);
    } else if (config->transport == MCP_TRANSPORT_HTTP) {
        result = http_connect(server);
    } else {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "Unsupported transport type");
        free_server(server);
        return MCP_ERROR_TRANSPORT;
    }

    if (result != 0) {
        free_server(server);
        return MCP_ERROR_CONNECT;
    }

    // Perform MCP handshake
    result = mcp_handshake(server);
    if (result != 0) {
        if (config->transport == MCP_TRANSPORT_STDIO) {
            stdio_disconnect(server);
        } else {
            http_disconnect(server);
        }
        free_server(server);
        return MCP_ERROR_PROTOCOL;
    }

    // Discover capabilities
    if (server->capabilities.supports_tools) {
        mcp_discover_tools(server);
    }
    if (server->capabilities.supports_resources) {
        mcp_discover_resources(server);
    }
    if (server->capabilities.supports_prompts) {
        mcp_discover_prompts(server);
    }

    server->status = MCP_STATUS_CONNECTED;
    server->connected_at = time(NULL);
    server->last_success = time(NULL);

    // Add to server list
    pthread_mutex_lock(&g_mcp.lock);
    if (g_mcp.server_count < MAX_MCP_SERVERS) {
        g_mcp.servers[g_mcp.server_count++] = server;
    }
    pthread_mutex_unlock(&g_mcp.lock);

    return MCP_OK;
}

int mcp_connect_all(void) {
    int connected = 0;
    for (int i = 0; i < g_mcp.config_count; i++) {
        if (g_mcp.configs[i]->enabled) {
            if (mcp_connect(g_mcp.configs[i]->name) == MCP_OK) {
                connected++;
            }
        }
    }
    return connected;
}

int mcp_disconnect(const char* name) {
    pthread_mutex_lock(&g_mcp.lock);

    for (int i = 0; i < g_mcp.server_count; i++) {
        if (strcmp(g_mcp.servers[i]->name, name) == 0) {
            MCPServer* server = g_mcp.servers[i];

            // Disconnect transport
            if (server->config->transport == MCP_TRANSPORT_STDIO) {
                stdio_disconnect(server);
            } else if (server->config->transport == MCP_TRANSPORT_HTTP) {
                http_disconnect(server);
            }

            // Remove from list
            for (int j = i; j < g_mcp.server_count - 1; j++) {
                g_mcp.servers[j] = g_mcp.servers[j + 1];
            }
            g_mcp.server_count--;

            free_server(server);
            pthread_mutex_unlock(&g_mcp.lock);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_mcp.lock);
    return -1;
}

void mcp_disconnect_all(void) {
    pthread_mutex_lock(&g_mcp.lock);

    for (int i = 0; i < g_mcp.server_count; i++) {
        MCPServer* server = g_mcp.servers[i];

        if (server->config->transport == MCP_TRANSPORT_STDIO) {
            stdio_disconnect(server);
        } else if (server->config->transport == MCP_TRANSPORT_HTTP) {
            http_disconnect(server);
        }

        free_server(server);
    }
    g_mcp.server_count = 0;

    pthread_mutex_unlock(&g_mcp.lock);
}

int mcp_reconnect(const char* name) {
    mcp_disconnect(name);
    return mcp_connect(name);
}

MCPServer* mcp_get_server(const char* name) {
    return find_server(name);
}

MCPConnectionStatus mcp_get_status(const char* name) {
    MCPServer* server = find_server(name);
    return server ? server->status : MCP_STATUS_DISCONNECTED;
}

char** mcp_list_connected(int* count) {
    pthread_mutex_lock(&g_mcp.lock);

    if (g_mcp.server_count == 0) {
        pthread_mutex_unlock(&g_mcp.lock);
        *count = 0;
        return NULL;
    }

    char** list = calloc((size_t)g_mcp.server_count, sizeof(char*));
    for (int i = 0; i < g_mcp.server_count; i++) {
        list[i] = strdup(g_mcp.servers[i]->name);
    }

    *count = g_mcp.server_count;
    pthread_mutex_unlock(&g_mcp.lock);
    return list;
}

// ============================================================================
// TOOL DISCOVERY
// ============================================================================

int mcp_refresh_tools(const char* name) {
    MCPServer* server = find_server(name);
    if (!server) return -1;
    return mcp_discover_tools(server);
}

MCPTool* mcp_get_tool(const char* server_name, const char* tool_name) {
    MCPServer* server = find_server(server_name);
    if (!server) return NULL;

    for (int i = 0; i < server->tool_count; i++) {
        if (strcmp(server->tools[i].name, tool_name) == 0) {
            return &server->tools[i];
        }
    }
    return NULL;
}

MCPTool** mcp_list_tools(const char* server_name, int* count) {
    MCPServer* server = find_server(server_name);
    if (!server || server->tool_count == 0) {
        *count = 0;
        return NULL;
    }

    MCPTool** list = calloc((size_t)server->tool_count, sizeof(MCPTool*));
    for (int i = 0; i < server->tool_count; i++) {
        list[i] = &server->tools[i];
    }

    *count = server->tool_count;
    return list;
}

MCPToolRef* mcp_list_all_tools(int* count) {
    pthread_mutex_lock(&g_mcp.lock);

    // Count total tools
    int total = 0;
    for (int i = 0; i < g_mcp.server_count; i++) {
        total += g_mcp.servers[i]->tool_count;
    }

    if (total == 0) {
        pthread_mutex_unlock(&g_mcp.lock);
        *count = 0;
        return NULL;
    }

    MCPToolRef* list = calloc((size_t)total, sizeof(MCPToolRef));
    int idx = 0;

    for (int i = 0; i < g_mcp.server_count; i++) {
        MCPServer* server = g_mcp.servers[i];
        for (int j = 0; j < server->tool_count; j++) {
            list[idx].server_name = server->name;
            list[idx].tool = &server->tools[j];
            idx++;
        }
    }

    pthread_mutex_unlock(&g_mcp.lock);
    *count = total;
    return list;
}

MCPTool* mcp_find_tool(const char* tool_name, const char** server_name) {
    pthread_mutex_lock(&g_mcp.lock);

    for (int i = 0; i < g_mcp.server_count; i++) {
        MCPServer* server = g_mcp.servers[i];
        for (int j = 0; j < server->tool_count; j++) {
            if (strcmp(server->tools[j].name, tool_name) == 0) {
                if (server_name) *server_name = server->name;
                pthread_mutex_unlock(&g_mcp.lock);
                return &server->tools[j];
            }
        }
    }

    pthread_mutex_unlock(&g_mcp.lock);
    return NULL;
}

// ============================================================================
// TOOL INVOCATION
// ============================================================================

MCPToolResult* mcp_call_tool(const char* server_name, const char* tool_name, cJSON* arguments) {
    MCPServer* server = find_server(server_name);
    if (!server) {
        MCPToolResult* result = calloc(1, sizeof(MCPToolResult));
        result->is_error = true;
        result->error_message = strdup("Server not connected");
        result->error_code = MCP_ERROR_NOT_FOUND;
        return result;
    }

    // Build request
    cJSON* params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "name", tool_name);
    if (arguments) {
        cJSON_AddItemToObject(params, "arguments", cJSON_Duplicate(arguments, true));
    } else {
        cJSON_AddItemToObject(params, "arguments", cJSON_CreateObject());
    }

    char* request = jsonrpc_create_request(server->next_request_id++, "tools/call", params);
    cJSON_Delete(params);

    // Send request
    char* response_str;
    if (server->config->transport == MCP_TRANSPORT_STDIO) {
        response_str = stdio_send_receive(server, request, server->config->timeout_ms);
    } else {
        response_str = http_send_receive(server, request, server->config->timeout_ms);
    }
    free(request);

    MCPToolResult* result = calloc(1, sizeof(MCPToolResult));

    if (!response_str) {
        result->is_error = true;
        result->error_message = strdup("No response from server");
        result->error_code = MCP_ERROR_TIMEOUT;
        server->consecutive_errors++;
        return result;
    }

    // Parse response
    bool is_error = false;
    char* error_msg = NULL;
    cJSON* response = jsonrpc_parse_response(response_str, &is_error, &error_msg);
    free(response_str);

    if (is_error || !response) {
        result->is_error = true;
        result->error_message = error_msg ? error_msg : strdup("Failed to parse response");
        result->error_code = MCP_ERROR_PROTOCOL;
        server->consecutive_errors++;
        if (response) cJSON_Delete(response);
        return result;
    }

    // Extract content
    cJSON* content = cJSON_GetObjectItem(response, "content");
    if (content) {
        result->content = cJSON_Duplicate(content, true);
    }

    // Check for tool error
    cJSON* is_err = cJSON_GetObjectItem(response, "isError");
    if (is_err && cJSON_IsTrue(is_err)) {
        result->is_error = true;
        cJSON* first_content = cJSON_GetArrayItem(content, 0);
        if (first_content) {
            cJSON* text = cJSON_GetObjectItem(first_content, "text");
            if (text && cJSON_IsString(text)) {
                result->error_message = strdup(text->valuestring);
            }
        }
    }

    cJSON_Delete(response);
    server->consecutive_errors = 0;
    server->last_success = time(NULL);

    return result;
}

MCPToolResult* mcp_call_tool_auto(const char* tool_name, cJSON* arguments) {
    const char* server_name;
    MCPTool* tool = mcp_find_tool(tool_name, &server_name);

    if (!tool) {
        MCPToolResult* result = calloc(1, sizeof(MCPToolResult));
        result->is_error = true;
        result->error_message = strdup("Tool not found");
        result->error_code = MCP_ERROR_NOT_FOUND;
        return result;
    }

    return mcp_call_tool(server_name, tool_name, arguments);
}

void mcp_free_result(MCPToolResult* result) {
    if (!result) return;
    if (result->content) cJSON_Delete(result->content);
    free(result->error_message);
    free(result);
}

// ============================================================================
// RESOURCE ACCESS
// ============================================================================

MCPResource** mcp_list_resources(const char* server_name, int* count) {
    MCPServer* server = find_server(server_name);
    if (!server || server->resource_count == 0) {
        *count = 0;
        return NULL;
    }

    MCPResource** list = calloc((size_t)server->resource_count, sizeof(MCPResource*));
    for (int i = 0; i < server->resource_count; i++) {
        list[i] = &server->resources[i];
    }

    *count = server->resource_count;
    return list;
}

cJSON* mcp_read_resource(const char* server_name, const char* uri) {
    MCPServer* server = find_server(server_name);
    if (!server) return NULL;

    cJSON* params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "uri", uri);

    char* request = jsonrpc_create_request(server->next_request_id++, "resources/read", params);
    cJSON_Delete(params);

    char* response_str;
    if (server->config->transport == MCP_TRANSPORT_STDIO) {
        response_str = stdio_send_receive(server, request, server->config->timeout_ms);
    } else {
        response_str = http_send_receive(server, request, server->config->timeout_ms);
    }
    free(request);

    if (!response_str) return NULL;

    bool is_error;
    char* error_msg = NULL;
    cJSON* response = jsonrpc_parse_response(response_str, &is_error, &error_msg);
    free(response_str);
    free(error_msg);

    if (is_error || !response) {
        if (response) cJSON_Delete(response);
        return NULL;
    }

    cJSON* contents = cJSON_DetachItemFromObject(response, "contents");
    cJSON_Delete(response);

    return contents;
}

// ============================================================================
// PROMPTS
// ============================================================================

MCPPrompt** mcp_list_prompts(const char* server_name, int* count) {
    MCPServer* server = find_server(server_name);
    if (!server || server->prompt_count == 0) {
        *count = 0;
        return NULL;
    }

    MCPPrompt** list = calloc((size_t)server->prompt_count, sizeof(MCPPrompt*));
    for (int i = 0; i < server->prompt_count; i++) {
        list[i] = &server->prompts[i];
    }

    *count = server->prompt_count;
    return list;
}

cJSON* mcp_get_prompt(const char* server_name, const char* prompt_name, cJSON* arguments) {
    MCPServer* server = find_server(server_name);
    if (!server) return NULL;

    cJSON* params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "name", prompt_name);
    if (arguments) {
        cJSON_AddItemToObject(params, "arguments", cJSON_Duplicate(arguments, true));
    }

    char* request = jsonrpc_create_request(server->next_request_id++, "prompts/get", params);
    cJSON_Delete(params);

    char* response_str;
    if (server->config->transport == MCP_TRANSPORT_STDIO) {
        response_str = stdio_send_receive(server, request, server->config->timeout_ms);
    } else {
        response_str = http_send_receive(server, request, server->config->timeout_ms);
    }
    free(request);

    if (!response_str) return NULL;

    bool is_error;
    char* error_msg = NULL;
    cJSON* response = jsonrpc_parse_response(response_str, &is_error, &error_msg);
    free(response_str);
    free(error_msg);

    if (is_error || !response) {
        if (response) cJSON_Delete(response);
        return NULL;
    }

    cJSON* messages = cJSON_DetachItemFromObject(response, "messages");
    cJSON_Delete(response);

    return messages;
}

// ============================================================================
// ERROR HANDLING
// ============================================================================

const char* mcp_get_last_error(const char* server_name) {
    if (server_name) {
        MCPServer* server = find_server(server_name);
        if (server && server->last_error) {
            return server->last_error;
        }
    }
    return g_mcp.last_error[0] ? g_mcp.last_error : NULL;
}

void mcp_clear_error(const char* server_name) {
    if (server_name) {
        MCPServer* server = find_server(server_name);
        if (server) {
            free(server->last_error);
            server->last_error = NULL;
        }
    } else {
        g_mcp.last_error[0] = '\0';
    }
}

// ============================================================================
// HEALTH
// ============================================================================

MCPHealth* mcp_get_health(void) {
    MCPHealth* health = calloc(1, sizeof(MCPHealth));

    pthread_mutex_lock(&g_mcp.lock);

    health->total_servers = g_mcp.config_count;
    health->connected_servers = g_mcp.server_count;

    // Count servers with errors
    for (int i = 0; i < g_mcp.server_count; i++) {
        if (g_mcp.servers[i]->consecutive_errors > 0) {
            health->servers_with_errors++;
        }
    }

    // Build server status array
    if (g_mcp.config_count > 0) {
        health->server_count = g_mcp.config_count;
        health->server_status = calloc((size_t)g_mcp.config_count, sizeof(*health->server_status));

        for (int i = 0; i < g_mcp.config_count; i++) {
            MCPServerConfig* config = g_mcp.configs[i];
            health->server_status[i].name = strdup(config->name);

            MCPServer* server = find_server(config->name);
            if (server) {
                health->server_status[i].status = server->status;
                health->server_status[i].tool_count = server->tool_count;
                health->server_status[i].last_success = server->last_success;
                if (server->last_error) {
                    health->server_status[i].last_error = strdup(server->last_error);
                }
            } else {
                health->server_status[i].status = MCP_STATUS_DISCONNECTED;
            }
        }
    }

    pthread_mutex_unlock(&g_mcp.lock);
    return health;
}

void mcp_free_health(MCPHealth* health) {
    if (!health) return;

    for (int i = 0; i < health->server_count; i++) {
        free(health->server_status[i].name);
        free(health->server_status[i].last_error);
    }
    free(health->server_status);
    free(health);
}

void mcp_print_health(void) {
    MCPHealth* health = mcp_get_health();
    if (!health) {
        printf("Failed to get MCP health\n");
        return;
    }

    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║              MCP CLIENT HEALTH                    ║\n");
    printf("╠═══════════════════════════════════════════════════╣\n");
    printf("║ Total Servers:     %-23d ║\n", health->total_servers);
    printf("║ Connected:         %-23d ║\n", health->connected_servers);
    printf("║ With Errors:       %-23d ║\n", health->servers_with_errors);
    printf("╠═══════════════════════════════════════════════════╣\n");

    for (int i = 0; i < health->server_count; i++) {
        const char* status_str;
        const char* status_color;

        switch (health->server_status[i].status) {
            case MCP_STATUS_CONNECTED:
                status_str = "CONNECTED";
                status_color = "\033[32m";
                break;
            case MCP_STATUS_CONNECTING:
                status_str = "CONNECTING";
                status_color = "\033[33m";
                break;
            case MCP_STATUS_ERROR:
                status_str = "ERROR";
                status_color = "\033[31m";
                break;
            default:
                status_str = "DISCONNECTED";
                status_color = "\033[90m";
                break;
        }

        printf("║ %-18s %s%-10s\033[0m %2d tools   ║\n",
               health->server_status[i].name,
               status_color, status_str,
               health->server_status[i].tool_count);

        if (health->server_status[i].last_error) {
            printf("║   └─ Error: %-30.30s ║\n", health->server_status[i].last_error);
        }
    }

    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");

    mcp_free_health(health);
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void mcp_free_config(MCPServerConfig* config) {
    free_config(config);
}

void mcp_free_strings(char** strings, int count) {
    if (!strings) return;
    for (int i = 0; i < count; i++) {
        free(strings[i]);
    }
    free(strings);
}

// ============================================================================
// JSON-RPC IMPLEMENTATION
// ============================================================================

static char* jsonrpc_create_request(int64_t id, const char* method, cJSON* params) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(json, "id", (double)id);
    cJSON_AddStringToObject(json, "method", method);

    if (params) {
        cJSON_AddItemToObject(json, "params", cJSON_Duplicate(params, true));
    }

    char* str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    // Append newline for stdio transport
    size_t len = strlen(str);
    str = realloc(str, len + 2);
    str[len] = '\n';
    str[len + 1] = '\0';

    return str;
}

static cJSON* jsonrpc_parse_response(const char* json_str, bool* is_error, char** error_msg) {
    *is_error = false;
    *error_msg = NULL;

    cJSON* json = cJSON_Parse(json_str);
    if (!json) {
        *is_error = true;
        *error_msg = strdup("Invalid JSON response");
        return NULL;
    }

    // Check for error
    cJSON* error = cJSON_GetObjectItem(json, "error");
    if (error && cJSON_IsObject(error)) {
        *is_error = true;
        cJSON* message = cJSON_GetObjectItem(error, "message");
        if (message && cJSON_IsString(message)) {
            *error_msg = strdup(message->valuestring);
        } else {
            *error_msg = strdup("Unknown error");
        }
        cJSON_Delete(json);
        return NULL;
    }

    // Return result
    cJSON* result = cJSON_DetachItemFromObject(json, "result");
    cJSON_Delete(json);

    return result;
}

// ============================================================================
// STDIO TRANSPORT
// ============================================================================

typedef struct {
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
} StdioTransport;

static int stdio_connect(MCPServer* server) {
    MCPServerConfig* config = server->config;

    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "Failed to create pipes: %s", strerror(errno));
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "Failed to fork: %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        // Child process
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        // Set environment using setenv (safer - copies the value)
        for (int i = 0; i < config->env_count; i++) {
            char* env_entry = config->env[i];
            char* equal_sign = strchr(env_entry, '=');
            if (equal_sign) {
                size_t key_len = (size_t)(equal_sign - env_entry);
                char key[256];
                if (key_len < sizeof(key)) {
                    memcpy(key, env_entry, key_len);
                    key[key_len] = '\0';
                    setenv(key, equal_sign + 1, 1);
                }
            }
        }

        // Change directory if specified
        if (config->working_dir) {
            if (chdir(config->working_dir) != 0) {
                _exit(126);
            }
        }

        // Build argv
        char** argv = malloc((size_t)(config->arg_count + 2) * sizeof(char*));
        argv[0] = (char*)config->command;
        for (int i = 0; i < config->arg_count; i++) {
            argv[i + 1] = config->args[i];
        }
        argv[config->arg_count + 1] = NULL;

        execvp(config->command, argv);
        _exit(127);  // exec failed
    }

    // Parent process
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Set non-blocking on stdout
    fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);

    StdioTransport* transport = calloc(1, sizeof(StdioTransport));
    transport->pid = pid;
    transport->stdin_fd = stdin_pipe[1];
    transport->stdout_fd = stdout_pipe[0];
    transport->stderr_fd = stderr_pipe[0];

    server->transport_data = transport;
    return 0;
}

static void stdio_disconnect(MCPServer* server) {
    StdioTransport* transport = (StdioTransport*)server->transport_data;
    if (!transport) return;

    // Close pipes
    if (transport->stdin_fd >= 0) close(transport->stdin_fd);
    if (transport->stdout_fd >= 0) close(transport->stdout_fd);
    if (transport->stderr_fd >= 0) close(transport->stderr_fd);

    // Kill process
    if (transport->pid > 0) {
        kill(transport->pid, SIGTERM);
        usleep(100000);  // 100ms grace period
        kill(transport->pid, SIGKILL);
        waitpid(transport->pid, NULL, WNOHANG);
    }

    free(transport);
    server->transport_data = NULL;
}

static char* stdio_send_receive(MCPServer* server, const char* request, int timeout_ms) {
    StdioTransport* transport = (StdioTransport*)server->transport_data;
    if (!transport) return NULL;

    // Write request
    ssize_t written = write(transport->stdin_fd, request, strlen(request));
    if (written < 0) {
        server->last_error = strdup("Failed to write to server");
        return NULL;
    }

    // Read response
    char* buffer = malloc(READ_BUFFER_SIZE);
    size_t buffer_pos = 0;

    struct pollfd pfd = { .fd = transport->stdout_fd, .events = POLLIN };

    while (buffer_pos < READ_BUFFER_SIZE - 1) {
        int poll_result = poll(&pfd, 1, timeout_ms);
        if (poll_result <= 0) {
            free(buffer);
            return NULL;  // Timeout or error
        }

        ssize_t n = read(transport->stdout_fd, buffer + buffer_pos,
                        READ_BUFFER_SIZE - buffer_pos - 1);
        if (n <= 0) break;

        buffer_pos += (size_t)n;
        buffer[buffer_pos] = '\0';

        // Check for newline (message delimiter)
        char* newline = strchr(buffer, '\n');
        if (newline) {
            *newline = '\0';
            return buffer;
        }
    }

    free(buffer);
    return NULL;
}

// ============================================================================
// HTTP TRANSPORT
// ============================================================================

typedef struct {
    CURL* curl;
    struct curl_slist* headers;
} HTTPTransport;

static int http_connect(MCPServer* server) {
    MCPServerConfig* config = server->config;

    HTTPTransport* transport = calloc(1, sizeof(HTTPTransport));
    transport->curl = curl_easy_init();

    if (!transport->curl) {
        free(transport);
        return -1;
    }

    // Set headers
    transport->headers = curl_slist_append(transport->headers, "Content-Type: application/json");
    for (int i = 0; i < config->header_count; i++) {
        transport->headers = curl_slist_append(transport->headers, config->headers[i]);
    }

    curl_easy_setopt(transport->curl, CURLOPT_HTTPHEADER, transport->headers);
    curl_easy_setopt(transport->curl, CURLOPT_TIMEOUT_MS, (long)config->timeout_ms);
    curl_easy_setopt(transport->curl, CURLOPT_URL, config->url);

    server->transport_data = transport;
    return 0;
}

static void http_disconnect(MCPServer* server) {
    HTTPTransport* transport = (HTTPTransport*)server->transport_data;
    if (!transport) return;

    if (transport->headers) curl_slist_free_all(transport->headers);
    if (transport->curl) curl_easy_cleanup(transport->curl);

    free(transport);
    server->transport_data = NULL;
}

// curl write callback
typedef struct {
    char* data;
    size_t size;
} CurlResponse;

// Maximum response size to prevent OOM from malicious/broken MCP servers
// 1MB is generous for MCP protocol responses (resources can be large)
#define MAX_MCP_RESPONSE_SIZE (1024 * 1024)

static size_t mcp_curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t realsize = size * nmemb;
    CurlResponse* resp = (CurlResponse*)userdata;

    // Check response size limit to prevent OOM
    if (resp->size + realsize > MAX_MCP_RESPONSE_SIZE) {
        LOG_ERROR(LOG_CAT_MCP, "MCP response exceeds maximum size (%d bytes)", MAX_MCP_RESPONSE_SIZE);
        return 0;  // Abort transfer
    }

    resp->data = realloc(resp->data, resp->size + realsize + 1);
    if (!resp->data) {
        LOG_ERROR(LOG_CAT_MCP, "Failed to allocate memory for MCP response");
        return 0;
    }
    memcpy(resp->data + resp->size, ptr, realsize);
    resp->size += realsize;
    resp->data[resp->size] = '\0';

    return realsize;
}

static char* http_send_receive(MCPServer* server, const char* request, int timeout_ms) {
    HTTPTransport* transport = (HTTPTransport*)server->transport_data;
    if (!transport) return NULL;

    CurlResponse response = { .data = malloc(1), .size = 0 };
    response.data[0] = '\0';

    curl_easy_setopt(transport->curl, CURLOPT_POSTFIELDS, request);
    curl_easy_setopt(transport->curl, CURLOPT_WRITEFUNCTION, mcp_curl_write_cb);
    curl_easy_setopt(transport->curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(transport->curl, CURLOPT_TIMEOUT_MS, (long)timeout_ms);

    CURLcode res = curl_easy_perform(transport->curl);

    if (res != CURLE_OK) {
        free(response.data);
        server->last_error = strdup(curl_easy_strerror(res));
        return NULL;
    }

    return response.data;
}

// ============================================================================
// MCP PROTOCOL
// ============================================================================

static int mcp_handshake(MCPServer* server) {
    // Create initialize request
    cJSON* params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "protocolVersion", MCP_PROTOCOL_VERSION);

    cJSON* capabilities = cJSON_CreateObject();
    cJSON_AddItemToObject(params, "capabilities", capabilities);

    cJSON* client_info = cJSON_CreateObject();
    cJSON_AddStringToObject(client_info, "name", "convergio");
    cJSON_AddStringToObject(client_info, "version", "1.0.0");
    cJSON_AddItemToObject(params, "clientInfo", client_info);

    char* request = jsonrpc_create_request(server->next_request_id++, "initialize", params);
    cJSON_Delete(params);

    char* response_str;
    if (server->config->transport == MCP_TRANSPORT_STDIO) {
        response_str = stdio_send_receive(server, request, server->config->timeout_ms);
    } else {
        response_str = http_send_receive(server, request, server->config->timeout_ms);
    }
    free(request);

    if (!response_str) {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "No response from server during handshake");
        return -1;
    }

    bool is_error = false;
    char* error_msg = NULL;
    cJSON* result = jsonrpc_parse_response(response_str, &is_error, &error_msg);
    free(response_str);

    if (is_error || !result) {
        snprintf(g_mcp.last_error, sizeof(g_mcp.last_error),
                 "Handshake failed: %s", error_msg ? error_msg : "unknown error");
        free(error_msg);
        return -1;
    }

    // Parse capabilities
    cJSON* server_caps = cJSON_GetObjectItem(result, "capabilities");
    if (server_caps) {
        server->capabilities.supports_tools =
            cJSON_GetObjectItem(server_caps, "tools") != NULL;
        server->capabilities.supports_resources =
            cJSON_GetObjectItem(server_caps, "resources") != NULL;
        server->capabilities.supports_prompts =
            cJSON_GetObjectItem(server_caps, "prompts") != NULL;
        server->capabilities.supports_logging =
            cJSON_GetObjectItem(server_caps, "logging") != NULL;
        server->capabilities.supports_sampling =
            cJSON_GetObjectItem(server_caps, "sampling") != NULL;
    }

    // Parse server info
    cJSON* server_info = cJSON_GetObjectItem(result, "serverInfo");
    if (server_info) {
        cJSON* name = cJSON_GetObjectItem(server_info, "name");
        if (name && cJSON_IsString(name)) {
            server->capabilities.server_name = strdup(name->valuestring);
        }
        cJSON* version = cJSON_GetObjectItem(server_info, "version");
        if (version && cJSON_IsString(version)) {
            server->capabilities.server_version = strdup(version->valuestring);
        }
    }

    cJSON* protocol = cJSON_GetObjectItem(result, "protocolVersion");
    if (protocol && cJSON_IsString(protocol)) {
        server->capabilities.protocol_version = strdup(protocol->valuestring);
    }

    cJSON_Delete(result);

    // Send initialized notification
    char* notif = jsonrpc_create_request(0, "notifications/initialized", NULL);
    if (server->config->transport == MCP_TRANSPORT_STDIO) {
        StdioTransport* t = server->transport_data;
        write(t->stdin_fd, notif, strlen(notif));
    } else {
        http_send_receive(server, notif, server->config->timeout_ms);
    }
    free(notif);

    return 0;
}

static int mcp_discover_tools(MCPServer* server) {
    char* request = jsonrpc_create_request(server->next_request_id++, "tools/list", NULL);

    char* response_str;
    if (server->config->transport == MCP_TRANSPORT_STDIO) {
        response_str = stdio_send_receive(server, request, server->config->timeout_ms);
    } else {
        response_str = http_send_receive(server, request, server->config->timeout_ms);
    }
    free(request);

    if (!response_str) return -1;

    bool is_error;
    char* error_msg = NULL;
    cJSON* result = jsonrpc_parse_response(response_str, &is_error, &error_msg);
    free(response_str);
    free(error_msg);

    if (is_error || !result) return -1;

    cJSON* tools = cJSON_GetObjectItem(result, "tools");
    if (!tools || !cJSON_IsArray(tools)) {
        cJSON_Delete(result);
        return 0;
    }

    int count = cJSON_GetArraySize(tools);
    server->tools = calloc((size_t)count, sizeof(MCPTool));
    server->tool_count = count;

    for (int i = 0; i < count; i++) {
        cJSON* tool = cJSON_GetArrayItem(tools, i);

        cJSON* name = cJSON_GetObjectItem(tool, "name");
        if (name && cJSON_IsString(name)) {
            server->tools[i].name = strdup(name->valuestring);
        }

        cJSON* desc = cJSON_GetObjectItem(tool, "description");
        if (desc && cJSON_IsString(desc)) {
            server->tools[i].description = strdup(desc->valuestring);
        }

        cJSON* schema = cJSON_GetObjectItem(tool, "inputSchema");
        if (schema) {
            server->tools[i].input_schema = cJSON_Duplicate(schema, true);
        }
    }

    cJSON_Delete(result);
    return 0;
}

static int mcp_discover_resources(MCPServer* server) {
    char* request = jsonrpc_create_request(server->next_request_id++, "resources/list", NULL);

    char* response_str;
    if (server->config->transport == MCP_TRANSPORT_STDIO) {
        response_str = stdio_send_receive(server, request, server->config->timeout_ms);
    } else {
        response_str = http_send_receive(server, request, server->config->timeout_ms);
    }
    free(request);

    if (!response_str) return -1;

    bool is_error;
    char* error_msg = NULL;
    cJSON* result = jsonrpc_parse_response(response_str, &is_error, &error_msg);
    free(response_str);
    free(error_msg);

    if (is_error || !result) return -1;

    cJSON* resources = cJSON_GetObjectItem(result, "resources");
    if (!resources || !cJSON_IsArray(resources)) {
        cJSON_Delete(result);
        return 0;
    }

    int count = cJSON_GetArraySize(resources);
    server->resources = calloc((size_t)count, sizeof(MCPResource));
    server->resource_count = count;

    for (int i = 0; i < count; i++) {
        cJSON* resource = cJSON_GetArrayItem(resources, i);

        cJSON* uri = cJSON_GetObjectItem(resource, "uri");
        if (uri && cJSON_IsString(uri)) {
            server->resources[i].uri = strdup(uri->valuestring);
        }

        cJSON* name = cJSON_GetObjectItem(resource, "name");
        if (name && cJSON_IsString(name)) {
            server->resources[i].name = strdup(name->valuestring);
        }

        cJSON* desc = cJSON_GetObjectItem(resource, "description");
        if (desc && cJSON_IsString(desc)) {
            server->resources[i].description = strdup(desc->valuestring);
        }

        cJSON* mime = cJSON_GetObjectItem(resource, "mimeType");
        if (mime && cJSON_IsString(mime)) {
            server->resources[i].mime_type = strdup(mime->valuestring);
        }
    }

    cJSON_Delete(result);
    return 0;
}

static int mcp_discover_prompts(MCPServer* server) {
    char* request = jsonrpc_create_request(server->next_request_id++, "prompts/list", NULL);

    char* response_str;
    if (server->config->transport == MCP_TRANSPORT_STDIO) {
        response_str = stdio_send_receive(server, request, server->config->timeout_ms);
    } else {
        response_str = http_send_receive(server, request, server->config->timeout_ms);
    }
    free(request);

    if (!response_str) return -1;

    bool is_error;
    char* error_msg = NULL;
    cJSON* result = jsonrpc_parse_response(response_str, &is_error, &error_msg);
    free(response_str);
    free(error_msg);

    if (is_error || !result) return -1;

    cJSON* prompts = cJSON_GetObjectItem(result, "prompts");
    if (!prompts || !cJSON_IsArray(prompts)) {
        cJSON_Delete(result);
        return 0;
    }

    int count = cJSON_GetArraySize(prompts);
    server->prompts = calloc((size_t)count, sizeof(MCPPrompt));
    server->prompt_count = count;

    for (int i = 0; i < count; i++) {
        cJSON* prompt = cJSON_GetArrayItem(prompts, i);

        cJSON* name = cJSON_GetObjectItem(prompt, "name");
        if (name && cJSON_IsString(name)) {
            server->prompts[i].name = strdup(name->valuestring);
        }

        cJSON* desc = cJSON_GetObjectItem(prompt, "description");
        if (desc && cJSON_IsString(desc)) {
            server->prompts[i].description = strdup(desc->valuestring);
        }

        cJSON* args = cJSON_GetObjectItem(prompt, "arguments");
        if (args) {
            server->prompts[i].arguments = cJSON_Duplicate(args, true);
        }
    }

    cJSON_Delete(result);
    return 0;
}

// ============================================================================
// HELPERS
// ============================================================================

static char* expand_path(const char* path) {
    if (!path) return NULL;

    if (path[0] == '~') {
        const char* home = getenv("HOME");
        if (!home) return strdup(path);

        size_t len = strlen(home) + strlen(path);
        char* expanded = malloc(len);
        snprintf(expanded, len, "%s%s", home, path + 1);
        return expanded;
    }

    return strdup(path);
}

static char* expand_env_vars(const char* str) {
    if (!str) return strdup("");

    // Simple ${VAR} expansion
    char* result = strdup(str);
    char* pos;

    while ((pos = strstr(result, "${")) != NULL) {
        char* end = strchr(pos, '}');
        if (!end) break;

        // Extract var name
        size_t name_len = (size_t)(end - pos - 2);
        char* var_name = malloc(name_len + 1);
        strncpy(var_name, pos + 2, name_len);
        var_name[name_len] = '\0';

        // Get value
        const char* value = getenv(var_name);
        if (!value) value = "";

        // Build new string
        size_t prefix_len = (size_t)(pos - result);
        size_t suffix_len = strlen(end + 1);
        size_t new_len = prefix_len + strlen(value) + suffix_len + 1;

        char* new_result = malloc(new_len);
        if (!new_result) {
            free(var_name);
            return result;
        }
        memcpy(new_result, result, prefix_len);
        new_result[prefix_len] = '\0';
        strlcat(new_result, value, new_len);
        strlcat(new_result, end + 1, new_len);

        free(var_name);
        free(result);
        result = new_result;
    }

    return result;
}

static void free_server(MCPServer* server) {
    if (!server) return;

    free(server->name);
    free(server->last_error);

    // Free capabilities
    free(server->capabilities.protocol_version);
    free(server->capabilities.server_name);
    free(server->capabilities.server_version);

    // Free tools
    for (int i = 0; i < server->tool_count; i++) {
        free(server->tools[i].name);
        free(server->tools[i].description);
        if (server->tools[i].input_schema) {
            cJSON_Delete(server->tools[i].input_schema);
        }
    }
    free(server->tools);

    // Free resources
    for (int i = 0; i < server->resource_count; i++) {
        free(server->resources[i].uri);
        free(server->resources[i].name);
        free(server->resources[i].description);
        free(server->resources[i].mime_type);
    }
    free(server->resources);

    // Free prompts
    for (int i = 0; i < server->prompt_count; i++) {
        free(server->prompts[i].name);
        free(server->prompts[i].description);
        if (server->prompts[i].arguments) {
            cJSON_Delete(server->prompts[i].arguments);
        }
    }
    free(server->prompts);

    free(server);
}

static void free_config(MCPServerConfig* config) {
    if (!config) return;

    free(config->name);
    free(config->command);
    free(config->working_dir);
    free(config->url);

    for (int i = 0; i < config->arg_count; i++) {
        free(config->args[i]);
    }
    free(config->args);

    for (int i = 0; i < config->env_count; i++) {
        free(config->env[i]);
    }
    free(config->env);

    for (int i = 0; i < config->header_count; i++) {
        free(config->headers[i]);
    }
    free(config->headers);

    free(config);
}

static MCPServer* find_server(const char* name) {
    for (int i = 0; i < g_mcp.server_count; i++) {
        if (strcmp(g_mcp.servers[i]->name, name) == 0) {
            return g_mcp.servers[i];
        }
    }
    return NULL;
}
