/**
 * CONVERGIO TOOLS
 *
 * Tool execution system for agents
 * Allows Ali and sub-agents to interact with the real world:
 * - File operations (read/write/list)
 * - Shell execution
 * - Web fetching
 * - Memory/RAG search
 */

#ifndef CONVERGIO_TOOLS_H
#define CONVERGIO_TOOLS_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// TOOL TYPES
// ============================================================================

typedef enum {
    TOOL_FILE_READ,       // Read file contents
    TOOL_FILE_WRITE,      // Write/append to file
    TOOL_FILE_LIST,       // List directory contents
    TOOL_SHELL_EXEC,      // Execute shell command
    TOOL_WEB_FETCH,       // Fetch URL content
    TOOL_MEMORY_STORE,    // Store in semantic memory
    TOOL_MEMORY_SEARCH,   // Search semantic memory (RAG)
    TOOL_AGENT_DELEGATE,  // Delegate to another agent
} ToolType;

typedef struct {
    ToolType type;
    char* name;
    char* description;
    char* parameters_json;  // JSON schema for parameters
} ToolDefinition;

typedef struct {
    bool success;
    char* output;          // Result content
    char* error;           // Error message if failed
    int exit_code;         // For shell commands
    size_t bytes_read;     // For file/web operations
    double execution_time; // Seconds
} ToolResult;

typedef struct {
    ToolType type;
    char* tool_name;
    char* parameters_json; // JSON with actual parameters
} ToolCall;

// ============================================================================
// TOOL DEFINITIONS (for Claude's tool_use)
// ============================================================================

// Get all available tools as JSON for Claude API
const char* tools_get_definitions_json(void);

// Get a specific tool definition
const ToolDefinition* tools_get_definition(ToolType type);

// ============================================================================
// TOOL EXECUTION
// ============================================================================

// Parse tool call from Claude's response
ToolCall* tools_parse_call(const char* tool_name, const char* arguments_json);

// Execute a tool call
ToolResult* tools_execute(const ToolCall* call);

// Free results
void tools_free_result(ToolResult* result);
void tools_free_call(ToolCall* call);

// ============================================================================
// FILE TOOLS
// ============================================================================

// Read file (with optional line range)
ToolResult* tool_file_read(const char* path, int start_line, int end_line);

// Write file (mode: "write" or "append")
ToolResult* tool_file_write(const char* path, const char* content, const char* mode);

// List directory
ToolResult* tool_file_list(const char* path, bool recursive, const char* pattern);

// ============================================================================
// SHELL TOOL
// ============================================================================

// Execute shell command (with timeout in seconds, 0 = no timeout)
ToolResult* tool_shell_exec(const char* command, const char* working_dir, int timeout_sec);

// ============================================================================
// WEB TOOL
// ============================================================================

// Fetch URL content (extracts text, handles HTML)
ToolResult* tool_web_fetch(const char* url, const char* method, const char* headers_json);

// ============================================================================
// MEMORY/RAG TOOLS
// ============================================================================

// Store content in semantic memory with embedding
ToolResult* tool_memory_store(const char* content, const char* category, float importance);

// Search memory using semantic similarity
ToolResult* tool_memory_search(const char* query, size_t max_results, float min_similarity);

// ============================================================================
// SAFETY
// ============================================================================

// Check if a path is safe to access (within allowed directories)
bool tools_is_path_safe(const char* path);

// Check if a command is safe to execute (no dangerous patterns)
bool tools_is_command_safe(const char* command);

// Set allowed directories for file operations
void tools_set_allowed_paths(const char** paths, size_t count);

// Set blocked commands/patterns
void tools_set_blocked_commands(const char** patterns, size_t count);

#endif // CONVERGIO_TOOLS_H
