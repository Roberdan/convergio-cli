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
#include <stdint.h>

// ============================================================================
// TOOL TYPES
// ============================================================================

typedef enum {
    TOOL_FILE_READ,       // Read file contents
    TOOL_FILE_WRITE,      // Write/append to file
    TOOL_FILE_LIST,       // List directory contents
    TOOL_FILE_DELETE,     // Safe delete (moves to trash)
    TOOL_SHELL_EXEC,      // Execute shell command
    TOOL_WEB_FETCH,       // Fetch URL content
    TOOL_WEB_SEARCH,      // Search the web (local fallback for non-Anthropic)
    TOOL_MEMORY_STORE,    // Store in semantic memory
    TOOL_MEMORY_SEARCH,   // Search semantic memory (RAG)
    TOOL_NOTE_WRITE,      // Write markdown note
    TOOL_NOTE_READ,       // Read markdown note
    TOOL_NOTE_LIST,       // List notes
    TOOL_KNOWLEDGE_SEARCH,// Search knowledge base
    TOOL_KNOWLEDGE_ADD,   // Add to knowledge base
    TOOL_PROJECT_TEAM,    // Manage project team
    TOOL_AGENT_DELEGATE,  // Delegate to another agent
    // Anna's task management tools
    TOOL_TODO_CREATE,     // Create a new task
    TOOL_TODO_LIST,       // List tasks with filters
    TOOL_TODO_UPDATE,     // Update task status/details
    TOOL_TODO_DELETE,     // Delete a task
    TOOL_NOTIFY_SCHEDULE, // Schedule a notification/reminder
    TOOL_NOTIFY_CANCEL,   // Cancel a scheduled notification
    // Advanced Claude Code-style tools
    TOOL_GLOB,            // Find files by glob pattern
    TOOL_GREP,            // Search file contents with regex
    TOOL_EDIT,            // Precise string replacement in file
} ToolType;

// Local tool definition (distinct from provider.h's ToolDefinition for API calls)
typedef struct {
    ToolType type;
    char* name;
    char* description;
    char* parameters_json;  // JSON schema for parameters
} LocalToolDefinition;

typedef struct {
    bool success;
    char* output;          // Result content
    char* error;           // Error message if failed
    int exit_code;         // For shell commands
    size_t bytes_read;     // For file/web operations
    double execution_time; // Seconds
} ToolResult;

// Local tool call (distinct from provider.h's ToolCall for API calls)
typedef struct {
    ToolType type;
    char* tool_name;
    char* parameters_json; // JSON with actual parameters
} LocalToolCall;

// ============================================================================
// TOOL DEFINITIONS (for Claude's tool_use)
// ============================================================================

// Get all available tools as JSON for Claude API
const char* tools_get_definitions_json(void);

// Get a specific tool definition
const LocalToolDefinition* tools_get_definition(ToolType type);

// ============================================================================
// TOOL EXECUTION
// ============================================================================

// Parse tool call from Claude's response
LocalToolCall* tools_parse_call(const char* tool_name, const char* arguments_json);

// Execute a tool call
ToolResult* tools_execute(const LocalToolCall* call);

// Free results
void tools_free_result(ToolResult* result);
void tools_free_call(LocalToolCall* call);

// ============================================================================
// FILE TOOLS
// ============================================================================

// Read file (with optional line range)
ToolResult* tool_file_read(const char* path, int start_line, int end_line);

// Write file (mode: "write" or "append")
ToolResult* tool_file_write(const char* path, const char* content, const char* mode);

// List directory
ToolResult* tool_file_list(const char* path, bool recursive, const char* pattern);

// Safe delete (moves to Trash on macOS, fallback to ~/.convergio/trash/)
ToolResult* tool_file_delete(const char* path, bool permanent);

// ============================================================================
// ADVANCED FILE TOOLS (Claude Code-style)
// ============================================================================

// Find files matching glob pattern (e.g., "**/*.c")
ToolResult* tool_glob(const char* pattern, const char* path, int max_results);

// Search file contents with regex
ToolResult* tool_grep(const char* pattern, const char* path, const char* glob_filter,
                      int context_before, int context_after, bool ignore_case,
                      const char* output_mode, int max_matches);

// Precise string replacement in file (creates backup)
ToolResult* tool_edit(const char* path, const char* old_string, const char* new_string);

// ============================================================================
// SHELL TOOL
// ============================================================================

// Execute shell command (with timeout in seconds, 0 = no timeout)
ToolResult* tool_shell_exec(const char* command, const char* working_dir, int timeout_sec);

// ============================================================================
// WEB TOOLS
// ============================================================================

// Fetch URL content (extracts text, handles HTML)
ToolResult* tool_web_fetch(const char* url, const char* method, const char* headers_json);

// Search the web using DuckDuckGo Lite (local fallback for non-native providers)
ToolResult* tool_web_search(const char* query, int max_results);

// ============================================================================
// MEMORY/RAG TOOLS
// ============================================================================

// Store content in semantic memory with embedding
ToolResult* tool_memory_store(const char* content, const char* category, float importance);

// Search memory using semantic similarity
ToolResult* tool_memory_search(const char* query, size_t max_results, float min_similarity);

// ============================================================================
// NOTE TOOLS
// ============================================================================

// Write a markdown note with frontmatter
ToolResult* tool_note_write(const char* title, const char* content, const char* tags);

// Read a note by title or search for notes
ToolResult* tool_note_read(const char* title, const char* search);

// List all notes, optionally filtered by tag
ToolResult* tool_note_list(const char* tag_filter);

// ============================================================================
// KNOWLEDGE BASE TOOLS
// ============================================================================

// Search the knowledge base
ToolResult* tool_knowledge_search(const char* query, size_t max_results);

// Add a document to the knowledge base
ToolResult* tool_knowledge_add(const char* title, const char* content, const char* category);

// ============================================================================
// TODO/TASK TOOLS (Anna's task management)
// ============================================================================

// Create a new task
ToolResult* tool_todo_create(const char* title, const char* description, const char* priority,
                             const char* due_date, const char* tags);

// List tasks with optional filters
ToolResult* tool_todo_list(const char* status, const char* priority, int limit);

// Update a task (by ID)
ToolResult* tool_todo_update(int64_t task_id, const char* status, const char* priority,
                             const char* due_date);

// Delete a task (by ID)
ToolResult* tool_todo_delete(int64_t task_id);

// ============================================================================
// NOTIFICATION TOOLS (Anna's reminder system)
// ============================================================================

// Schedule a notification for a specific time
ToolResult* tool_notify_schedule(const char* message, const char* when, const char* sound);

// Cancel a scheduled notification
ToolResult* tool_notify_cancel(int64_t notify_id);

// ============================================================================
// SAFETY
// ============================================================================

// Check if a path is safe to access (within allowed directories)
bool tools_is_path_safe(const char* path);

// Check if a command is safe to execute (no dangerous patterns)
bool tools_is_command_safe(const char* command);

// Set allowed directories for file operations (replaces existing)
void tools_set_allowed_paths(const char** paths, size_t count);

// Add a single path to allowed directories
void tools_add_allowed_path(const char* path);

// Get current allowed paths
const char** tools_get_allowed_paths(size_t* count);

// Clear all allowed paths
void tools_clear_allowed_paths(void);

// Initialize workspace sandbox (call at startup with CWD)
void tools_init_workspace(const char* workspace_path);

// Get the current workspace path (first allowed path)
const char* tools_get_workspace(void);

// Set blocked commands/patterns
void tools_set_blocked_commands(const char** patterns, size_t count);

#endif // CONVERGIO_TOOLS_H
