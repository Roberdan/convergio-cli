/**
 * CONVERGIO TOOLS
 *
 * Tool execution system for agents
 */

#include "nous/tools.h"
#include "nous/config.h"
#include "nous/projects.h"
#include "nous/todo.h"
#include "nous/notify.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fnmatch.h>
#include <curl/curl.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <fcntl.h>
#include "nous/debug_mutex.h"

// ============================================================================
// SAFETY CONFIGURATION
// ============================================================================

// Mutex for thread-safe access to global configuration
CONVERGIO_MUTEX_DECLARE(g_config_mutex);

static char** g_allowed_paths = NULL;
static size_t g_allowed_paths_count = 0;

static char** g_blocked_commands = NULL;
static size_t g_blocked_commands_count = 0;

// Default blocked patterns (dangerous commands)
static const char* DEFAULT_BLOCKED[] = {
    "rm -rf /",
    "rm -rf /*",
    "mkfs",
    "dd if=",
    ":(){:|:&};:",  // Fork bomb
    "chmod -R 777 /",
    "chown -R",
    "> /dev/sd",
    "mv /* ",
    "wget * | sh",
    "curl * | sh",
    NULL
};

// ============================================================================
// SECURITY UTILITIES
// ============================================================================

/**
 * Escape a string for safe use in shell commands (single-quoted context)
 * Replaces single quotes with '\'' (end quote, escaped quote, start quote)
 * Caller must free returned string
 */
static char* shell_escape(const char* input) {
    if (!input) return NULL;

    // Count single quotes to determine buffer size
    size_t quotes = 0;
    for (const char* p = input; *p; p++) {
        if (*p == '\'') quotes++;
    }

    // Allocate: original length + 3 extra chars per quote + null
    size_t len = strlen(input);
    char* escaped = malloc(len + quotes * 3 + 1);
    if (!escaped) return NULL;

    char* out = escaped;
    for (const char* p = input; *p; p++) {
        if (*p == '\'') {
            // End current quote, add escaped quote, start new quote
            *out++ = '\'';
            *out++ = '\\';
            *out++ = '\'';
            *out++ = '\'';
        } else {
            *out++ = *p;
        }
    }
    *out = '\0';

    return escaped;
}

/**
 * Sanitize input for grep pattern (remove dangerous regex chars)
 * Only allows alphanumeric, spaces, and basic punctuation
 */
static void sanitize_grep_pattern(char* pattern) {
    if (!pattern) return;
    for (char* p = pattern; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != ' ' && *p != '-' && *p != '_' && *p != '.') {
            *p = '_';
        }
    }
}

// ============================================================================
// TOOL DEFINITIONS JSON
// ============================================================================

// Notes and knowledge base directories - get from config with fallback
static const char* get_notes_dir(void) {
    const char* dir = convergio_config_get("notes_dir");
    return (dir && strlen(dir) > 0) ? dir : "data/notes";
}

static const char* get_knowledge_dir(void) {
    const char* dir = convergio_config_get("knowledge_dir");
    return (dir && strlen(dir) > 0) ? dir : "data/knowledge";
}

static const char* TOOLS_JSON =
"[\n"
"  {\n"
"    \"name\": \"file_read\",\n"
"    \"description\": \"Read the contents of a file. Returns the file content as text.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"path\": {\"type\": \"string\", \"description\": \"Absolute path to the file\"},\n"
"        \"start_line\": {\"type\": \"integer\", \"description\": \"Starting line (1-indexed, optional)\"},\n"
"        \"end_line\": {\"type\": \"integer\", \"description\": \"Ending line (optional)\"}\n"
"      },\n"
"      \"required\": [\"path\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"file_write\",\n"
"    \"description\": \"Write content to a file. Can create new files or overwrite/append to existing ones.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"path\": {\"type\": \"string\", \"description\": \"Absolute path to the file\"},\n"
"        \"content\": {\"type\": \"string\", \"description\": \"Content to write\"},\n"
"        \"mode\": {\"type\": \"string\", \"enum\": [\"write\", \"append\"], \"description\": \"Write mode (default: write)\"}\n"
"      },\n"
"      \"required\": [\"path\", \"content\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"file_list\",\n"
"    \"description\": \"List files and directories in a path.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"path\": {\"type\": \"string\", \"description\": \"Directory path to list\"},\n"
"        \"recursive\": {\"type\": \"boolean\", \"description\": \"List recursively (default: false)\"},\n"
"        \"pattern\": {\"type\": \"string\", \"description\": \"Glob pattern to filter (e.g., *.c)\"}\n"
"      },\n"
"      \"required\": [\"path\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"shell_exec\",\n"
"    \"description\": \"Execute a shell command and return the output. Use with caution.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"command\": {\"type\": \"string\", \"description\": \"Shell command to execute\"},\n"
"        \"working_dir\": {\"type\": \"string\", \"description\": \"Working directory (optional)\"},\n"
"        \"timeout\": {\"type\": \"integer\", \"description\": \"Timeout in seconds (default: 30)\"}\n"
"      },\n"
"      \"required\": [\"command\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"web_fetch\",\n"
"    \"description\": \"Fetch content from a URL. Returns the page content as text.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"url\": {\"type\": \"string\", \"description\": \"URL to fetch\"},\n"
"        \"method\": {\"type\": \"string\", \"enum\": [\"GET\", \"POST\"], \"description\": \"HTTP method (default: GET)\"}\n"
"      },\n"
"      \"required\": [\"url\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"memory_store\",\n"
"    \"description\": \"Store information in semantic memory for later retrieval.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"content\": {\"type\": \"string\", \"description\": \"Content to store\"},\n"
"        \"category\": {\"type\": \"string\", \"description\": \"Category tag (e.g., 'user_preference', 'fact', 'task')\"},\n"
"        \"importance\": {\"type\": \"number\", \"description\": \"Importance score 0.0-1.0 (default: 0.5)\"}\n"
"      },\n"
"      \"required\": [\"content\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"memory_search\",\n"
"    \"description\": \"Search semantic memory for relevant information using natural language query.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"query\": {\"type\": \"string\", \"description\": \"Natural language search query\"},\n"
"        \"max_results\": {\"type\": \"integer\", \"description\": \"Maximum results to return (default: 5)\"},\n"
"        \"min_similarity\": {\"type\": \"number\", \"description\": \"Minimum similarity threshold 0.0-1.0 (default: 0.5)\"}\n"
"      },\n"
"      \"required\": [\"query\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"note_write\",\n"
"    \"description\": \"Write or update a markdown note. Notes are stored in data/notes/ for persistent knowledge.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"title\": {\"type\": \"string\", \"description\": \"Note title (becomes filename, e.g. 'meeting-notes' -> meeting-notes.md)\"},\n"
"        \"content\": {\"type\": \"string\", \"description\": \"Markdown content of the note\"},\n"
"        \"tags\": {\"type\": \"string\", \"description\": \"Comma-separated tags for categorization\"}\n"
"      },\n"
"      \"required\": [\"title\", \"content\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"note_read\",\n"
"    \"description\": \"Read a markdown note by title or search for notes by tag/content.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"title\": {\"type\": \"string\", \"description\": \"Note title to read (without .md extension)\"},\n"
"        \"search\": {\"type\": \"string\", \"description\": \"Search term to find notes containing this text\"}\n"
"      }\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"note_list\",\n"
"    \"description\": \"List all available notes with their titles, tags, and modification dates.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"tag\": {\"type\": \"string\", \"description\": \"Filter notes by tag\"}\n"
"      }\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"knowledge_search\",\n"
"    \"description\": \"Search the knowledge base (data/knowledge/) for information. Returns relevant markdown content.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"query\": {\"type\": \"string\", \"description\": \"Search query to find relevant knowledge\"},\n"
"        \"max_results\": {\"type\": \"integer\", \"description\": \"Maximum number of results (default: 5)\"}\n"
"      },\n"
"      \"required\": [\"query\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"knowledge_add\",\n"
"    \"description\": \"Add a new document to the knowledge base for future reference.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"title\": {\"type\": \"string\", \"description\": \"Document title\"},\n"
"        \"content\": {\"type\": \"string\", \"description\": \"Markdown content\"},\n"
"        \"category\": {\"type\": \"string\", \"description\": \"Category folder (e.g. 'projects', 'people', 'processes')\"}\n"
"      },\n"
"      \"required\": [\"title\", \"content\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"project_team\",\n"
"    \"description\": \"Manage the current project's team. Add or remove agents from the project team.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"action\": {\"type\": \"string\", \"enum\": [\"add\", \"remove\", \"list\"], \"description\": \"Action to perform\"},\n"
"        \"agent_name\": {\"type\": \"string\", \"description\": \"Name of the agent to add/remove (e.g. 'baccio', 'stefano')\"}\n"
"      },\n"
"      \"required\": [\"action\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"todo_create\",\n"
"    \"description\": \"Create a new task/todo item. Use for reminders and task management.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"title\": {\"type\": \"string\", \"description\": \"Task title (what to do)\"},\n"
"        \"description\": {\"type\": \"string\", \"description\": \"Optional detailed description\"},\n"
"        \"priority\": {\"type\": \"string\", \"enum\": [\"critical\", \"high\", \"normal\", \"low\"], \"description\": \"Task priority (default: normal)\"},\n"
"        \"due_date\": {\"type\": \"string\", \"description\": \"When task is due (e.g. '2024-12-15 14:30', 'tomorrow', 'in 2 hours', 'tra 2 minuti')\"},\n"
"        \"tags\": {\"type\": \"string\", \"description\": \"Comma-separated tags for categorization\"}\n"
"      },\n"
"      \"required\": [\"title\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"todo_list\",\n"
"    \"description\": \"List tasks/todos with optional filters.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"status\": {\"type\": \"string\", \"enum\": [\"pending\", \"in_progress\", \"completed\", \"all\"], \"description\": \"Filter by status (default: pending)\"},\n"
"        \"priority\": {\"type\": \"string\", \"enum\": [\"critical\", \"high\", \"normal\", \"low\", \"all\"], \"description\": \"Filter by priority\"},\n"
"        \"limit\": {\"type\": \"integer\", \"description\": \"Maximum tasks to return (default: 10)\"}\n"
"      }\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"todo_update\",\n"
"    \"description\": \"Update an existing task by ID.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"task_id\": {\"type\": \"integer\", \"description\": \"ID of the task to update\"},\n"
"        \"status\": {\"type\": \"string\", \"enum\": [\"pending\", \"in_progress\", \"completed\", \"cancelled\"], \"description\": \"New status\"},\n"
"        \"priority\": {\"type\": \"string\", \"enum\": [\"critical\", \"high\", \"normal\", \"low\"], \"description\": \"New priority\"},\n"
"        \"due_date\": {\"type\": \"string\", \"description\": \"New due date\"}\n"
"      },\n"
"      \"required\": [\"task_id\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"todo_delete\",\n"
"    \"description\": \"Delete a task by ID.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"task_id\": {\"type\": \"integer\", \"description\": \"ID of the task to delete\"}\n"
"      },\n"
"      \"required\": [\"task_id\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"notify_schedule\",\n"
"    \"description\": \"Schedule a macOS notification/reminder for a specific time.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"message\": {\"type\": \"string\", \"description\": \"The reminder message to display\"},\n"
"        \"when\": {\"type\": \"string\", \"description\": \"When to show notification (e.g. '14:30', 'in 2 hours', 'tra 5 minuti', 'tomorrow 9am')\"},\n"
"        \"sound\": {\"type\": \"string\", \"enum\": [\"default\", \"ping\", \"basso\", \"blow\", \"bottle\", \"frog\", \"funk\", \"glass\", \"hero\", \"morse\", \"pop\", \"purr\", \"sosumi\", \"submarine\", \"tink\"], \"description\": \"Notification sound (default: default)\"}\n"
"      },\n"
"      \"required\": [\"message\", \"when\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"notify_cancel\",\n"
"    \"description\": \"Cancel a scheduled notification by ID.\",\n"
"    \"input_schema\": {\n"
"      \"type\": \"object\",\n"
"      \"properties\": {\n"
"        \"notify_id\": {\"type\": \"integer\", \"description\": \"ID of the notification to cancel\"}\n"
"      },\n"
"      \"required\": [\"notify_id\"]\n"
"    }\n"
"  }\n"
"]\n";

const char* tools_get_definitions_json(void) {
    return TOOLS_JSON;
}

// ============================================================================
// SAFETY CHECKS
// ============================================================================

void tools_set_allowed_paths(const char** paths, size_t count) {
    // Free existing
    if (g_allowed_paths) {
        for (size_t i = 0; i < g_allowed_paths_count; i++) {
            free(g_allowed_paths[i]);
        }
        free(g_allowed_paths);
        g_allowed_paths = NULL;
        g_allowed_paths_count = 0;
    }

    if (count == 0 || !paths) return;

    g_allowed_paths = malloc(count * sizeof(char*));
    g_allowed_paths_count = count;

    for (size_t i = 0; i < count; i++) {
        g_allowed_paths[i] = strdup(paths[i]);
    }
}

void tools_add_allowed_path(const char* path) {
    if (!path) return;

    // Resolve to absolute path BEFORE taking the lock
    char resolved[PATH_MAX];
    if (!realpath(path, resolved)) {
        return;  // Path doesn't exist
    }

    CONVERGIO_MUTEX_LOCK(&g_config_mutex);

    // Check if already in list
    for (size_t i = 0; i < g_allowed_paths_count; i++) {
        if (strcmp(g_allowed_paths[i], resolved) == 0) {
            CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);
            return;  // Already exists
        }
    }

    // Grow array and add (with proper realloc error handling)
    char** new_paths = realloc(g_allowed_paths, (g_allowed_paths_count + 1) * sizeof(char*));
    if (!new_paths) {
        CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);
        return;  // Allocation failed
    }
    g_allowed_paths = new_paths;
    g_allowed_paths[g_allowed_paths_count] = strdup(resolved);
    if (g_allowed_paths[g_allowed_paths_count]) {
        g_allowed_paths_count++;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);
}

const char** tools_get_allowed_paths(size_t* count) {
    CONVERGIO_MUTEX_LOCK(&g_config_mutex);
    if (count) *count = g_allowed_paths_count;
    const char** result = (const char**)g_allowed_paths;
    CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);
    return result;
}

void tools_clear_allowed_paths(void) {
    CONVERGIO_MUTEX_LOCK(&g_config_mutex);
    if (g_allowed_paths) {
        for (size_t i = 0; i < g_allowed_paths_count; i++) {
            free(g_allowed_paths[i]);
        }
        free(g_allowed_paths);
        g_allowed_paths = NULL;
        g_allowed_paths_count = 0;
    }
    CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);
}

void tools_init_workspace(const char* workspace_path) {
    // Clear any existing paths
    tools_clear_allowed_paths();

    // Resolve and set the workspace path
    char resolved[PATH_MAX];
    if (workspace_path && realpath(workspace_path, resolved)) {
        tools_add_allowed_path(resolved);
    }
}

const char* tools_get_workspace(void) {
    CONVERGIO_MUTEX_LOCK(&g_config_mutex);
    const char* workspace = (g_allowed_paths && g_allowed_paths_count > 0) ? g_allowed_paths[0] : NULL;
    CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);
    return workspace;
}

// Resolve a path - if relative, prepend workspace; if absolute, return as-is
// Caller must free the returned string
static char* tools_resolve_path(const char* path) {
    if (!path) return NULL;

    // If absolute path, return a copy
    if (path[0] == '/') {
        return strdup(path);
    }

    // Relative path - prepend workspace
    const char* workspace = tools_get_workspace();
    if (!workspace) {
        // No workspace set, return copy of original
        return strdup(path);
    }

    size_t len = strlen(workspace) + 1 + strlen(path) + 1;
    char* resolved = malloc(len);
    if (!resolved) return NULL;

    snprintf(resolved, len, "%s/%s", workspace, path);
    return resolved;
}

void tools_set_blocked_commands(const char** patterns, size_t count) {
    CONVERGIO_MUTEX_LOCK(&g_config_mutex);

    // Free existing
    if (g_blocked_commands) {
        for (size_t i = 0; i < g_blocked_commands_count; i++) {
            free(g_blocked_commands[i]);
        }
        free(g_blocked_commands);
    }

    g_blocked_commands = malloc(count * sizeof(char*));
    if (!g_blocked_commands) {
        g_blocked_commands_count = 0;
        CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);
        return;
    }
    g_blocked_commands_count = count;

    for (size_t i = 0; i < count; i++) {
        g_blocked_commands[i] = strdup(patterns[i]);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);
}

/**
 * Check if path is within a directory (proper boundary check)
 * Returns true if 'path' is exactly 'dir' or is inside 'dir'
 */
static bool is_path_within(const char* path, const char* dir) {
    size_t dir_len = strlen(dir);
    size_t path_len = strlen(path);

    // Path must be at least as long as dir
    if (path_len < dir_len) return false;

    // Must match the prefix
    if (strncmp(path, dir, dir_len) != 0) return false;

    // If exact match, it's within
    if (path_len == dir_len) return true;

    // If longer, must be followed by '/' (directory boundary)
    // This prevents /Users/work matching /Users/workbench
    return path[dir_len] == '/';
}

bool tools_is_path_safe(const char* path) {
    if (!path) return false;

    // Resolve to absolute path (also resolves symlinks)
    // Do this BEFORE taking the lock since realpath() can be slow
    char resolved[PATH_MAX];
    if (!realpath(path, resolved)) {
        // Path doesn't exist yet - check parent
        char* parent = strdup(path);
        if (!parent) return false;
        char* last_slash = strrchr(parent, '/');
        if (last_slash && last_slash != parent) {
            *last_slash = '\0';
            if (!realpath(parent, resolved)) {
                free(parent);
                return false;
            }
        } else {
            // No parent directory or root
            free(parent);
            return false;
        }
        free(parent);
    }

    // Block system paths (with proper boundary checking)
    // These are constants, no lock needed
    const char* blocked_prefixes[] = {
        "/System", "/usr", "/bin", "/sbin", "/etc", "/var",
        "/private/etc", "/private/var", "/Library",
        "/Applications", "/cores", "/opt", NULL
    };

    for (int i = 0; blocked_prefixes[i]; i++) {
        if (is_path_within(resolved, blocked_prefixes[i])) {
            return false;
        }
    }

    // Check against allowed paths (with proper boundary checking)
    // Need mutex since g_allowed_paths can be modified by other threads
    CONVERGIO_MUTEX_LOCK(&g_config_mutex);
    bool allowed = false;
    if (g_allowed_paths && g_allowed_paths_count > 0) {
        for (size_t i = 0; i < g_allowed_paths_count; i++) {
            if (is_path_within(resolved, g_allowed_paths[i])) {
                allowed = true;
                break;
            }
        }
    }
    CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);

    // No allowed paths set or path not in list - deny by default
    // This is intentional: workspace must be explicitly initialized
    return allowed;
}

/**
 * Normalize command string for safety checking
 * Removes escape characters and converts to lowercase for comparison
 */
static char* normalize_command(const char* cmd) {
    if (!cmd) return NULL;
    size_t len = strlen(cmd);
    char* normalized = malloc(len + 1);
    if (!normalized) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        // Skip backslash escape characters
        if (cmd[i] == '\\' && i + 1 < len) {
            i++;  // Skip the backslash, include the next char
        }
        normalized[j++] = (char)tolower((unsigned char)cmd[i]);
    }
    normalized[j] = '\0';
    return normalized;
}

bool tools_is_command_safe(const char* command) {
    if (!command) return false;
    if (command[0] == '\0') return false;  // Empty command is not safe

    // BLOCK dangerous shell metacharacters that enable command injection
    // These allow chaining or substitution of commands
    const char* dangerous_chars[] = {
        "`",        // Backtick command substitution
        "$(",       // Modern command substitution
        "$((",      // Arithmetic expansion
        "&&",       // Command chaining (allow single &)
        "||",       // Conditional chaining
        ";",        // Command separator
        "\n",       // Newline separator
        "|",        // Pipe (can chain to dangerous commands)
        NULL
    };

    for (int i = 0; dangerous_chars[i]; i++) {
        if (strstr(command, dangerous_chars[i])) {
            return false;
        }
    }

    // Normalize command for pattern matching
    char* normalized = normalize_command(command);
    if (!normalized) return false;

    // BLOCK dangerous commands (check with and without path)
    const char* dangerous_commands[] = {
        "rm -rf /",
        "rm -rf /*",
        "rm -fr /",
        "rm -fr /*",
        "mkfs",
        "dd if=",
        "dd of=/dev",
        ":(){:|:&};:",
        "chmod -r 777 /",
        "chmod 777 /",
        "chown -r",
        "> /dev/sd",
        "> /dev/nv",
        "mv /* ",
        "mv / ",
        "wget",        // Block entirely - too risky
        "curl",        // Block entirely - too risky for shell exec
        "nc ",         // Netcat
        "ncat ",
        "netcat ",
        "/bin/sh",
        "/bin/bash",
        "/bin/zsh",
        "python -c",
        "python3 -c",
        "perl -e",
        "ruby -e",
        "eval ",
        "exec ",
        "sudo ",
        "su ",
        "pkexec",
        "doas ",
        NULL
    };

    bool is_safe = true;
    for (int i = 0; dangerous_commands[i]; i++) {
        if (strstr(normalized, dangerous_commands[i])) {
            is_safe = false;
            break;
        }
    }

    // Also check default blocked patterns against normalized string
    if (is_safe) {
        for (int i = 0; DEFAULT_BLOCKED[i]; i++) {
            char* norm_blocked = normalize_command(DEFAULT_BLOCKED[i]);
            if (norm_blocked && strstr(normalized, norm_blocked)) {
                is_safe = false;
            }
            free(norm_blocked);
            if (!is_safe) break;
        }
    }

    // Check user-defined blocked patterns (need mutex for thread safety)
    CONVERGIO_MUTEX_LOCK(&g_config_mutex);
    if (is_safe && g_blocked_commands) {
        for (size_t i = 0; i < g_blocked_commands_count; i++) {
            char* norm_blocked = normalize_command(g_blocked_commands[i]);
            if (norm_blocked && strstr(normalized, norm_blocked)) {
                is_safe = false;
            }
            free(norm_blocked);
            if (!is_safe) break;
        }
    }
    CONVERGIO_MUTEX_UNLOCK(&g_config_mutex);

    free(normalized);
    return is_safe;
}

// ============================================================================
// RESULT HELPERS
// ============================================================================

static ToolResult* result_success(const char* output) {
    ToolResult* r = calloc(1, sizeof(ToolResult));
    r->success = true;
    r->output = output ? strdup(output) : strdup("");
    r->exit_code = 0;
    return r;
}

static ToolResult* result_error(const char* error) {
    ToolResult* r = calloc(1, sizeof(ToolResult));
    r->success = false;
    r->error = error ? strdup(error) : strdup("Unknown error");
    r->exit_code = -1;
    return r;
}

void tools_free_result(ToolResult* result) {
    if (!result) return;
    free(result->output);
    free(result->error);
    free(result);
}

void tools_free_call(LocalToolCall* call) {
    if (!call) return;
    free(call->tool_name);
    free(call->parameters_json);
    free(call);
}

// ============================================================================
// FILE TOOLS IMPLEMENTATION
// ============================================================================

/**
 * Safe file open - prevents TOCTOU attacks via symlink swapping
 * Uses O_NOFOLLOW to reject symlinks, fstat() to verify regular file
 * Returns file descriptor or -1 on error
 */
static int safe_open_read(const char* path) {
    // Open with O_NOFOLLOW - fails with ELOOP if path is a symlink
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd < 0) {
        return -1;
    }

    // Verify it's a regular file (not directory, device, etc.)
    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        errno = EINVAL;
        return -1;
    }

    return fd;
}

/**
 * Safe file open for writing - prevents symlink attacks
 * For new files: creates with O_CREAT | O_EXCL
 * For existing files: opens with O_NOFOLLOW
 */
static int safe_open_write(const char* path, bool append) {
    int flags = O_WRONLY | O_NOFOLLOW;
    if (append) {
        flags |= O_APPEND;
    } else {
        flags |= O_TRUNC;
    }

    // Try to open existing file first
    int fd = open(path, flags);
    if (fd < 0 && errno == ENOENT) {
        // File doesn't exist - create it (O_EXCL ensures we create, not follow)
        flags = O_WRONLY | O_CREAT | O_EXCL;
        if (!append) flags |= O_TRUNC;
        fd = open(path, flags, 0644);
    }

    if (fd < 0) {
        return -1;
    }

    // Verify it's a regular file
    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        errno = EINVAL;
        return -1;
    }

    return fd;
}

ToolResult* tool_file_read(const char* path, int start_line, int end_line) {
    clock_t start = clock();

    // Resolve relative paths to workspace
    char* resolved_path = tools_resolve_path(path);
    if (!resolved_path) {
        return result_error("Failed to resolve path");
    }

    if (!tools_is_path_safe(resolved_path)) {
        free(resolved_path);
        return result_error("Path not allowed for security reasons");
    }

    // Use resolved path from here on
    path = resolved_path;

    // Use safe open to prevent TOCTOU attacks
    int fd = safe_open_read(path);
    if (fd < 0) {
        char err[256];
        if (errno == ELOOP) {
            snprintf(err, sizeof(err), "Symlinks not allowed: %s", path);
        } else {
            snprintf(err, sizeof(err), "Cannot open file: %s", strerror(errno));
        }
        free(resolved_path);
        return result_error(err);
    }

    FILE* f = fdopen(fd, "r");
    if (!f) {
        close(fd);
        char err[256];
        snprintf(err, sizeof(err), "Cannot open file: %s", strerror(errno));
        free(resolved_path);
        return result_error(err);
    }

    // Read entire file or specified lines
    size_t capacity = 4096;
    size_t len = 0;
    char* content = malloc(capacity);
    content[0] = '\0';

    char line[4096];
    int line_num = 0;

    while (fgets(line, sizeof(line), f)) {
        line_num++;

        // Apply line range filter
        if (start_line > 0 && line_num < start_line) continue;
        if (end_line > 0 && line_num > end_line) break;

        size_t line_len = strlen(line);
        if (len + line_len + 1 > capacity) {
            size_t new_capacity = capacity * 2;
            char* new_content = realloc(content, new_capacity);
            if (!new_content) {
                free(content);
                free(resolved_path);
                fclose(f);
                return result_error("Out of memory");
            }
            content = new_content;
            capacity = new_capacity;
        }
        memcpy(content + len, line, line_len);
        len += line_len;
        content[len] = '\0';
    }

    fclose(f);

    ToolResult* r = result_success(content);
    r->bytes_read = len;
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    free(content);
    free(resolved_path);

    return r;
}

ToolResult* tool_file_write(const char* path, const char* content, const char* mode) {
    clock_t start = clock();

    // Resolve relative paths to workspace
    char* resolved_path = tools_resolve_path(path);
    if (!resolved_path) {
        return result_error("Failed to resolve path");
    }

    if (!tools_is_path_safe(resolved_path)) {
        free(resolved_path);
        return result_error("Path not allowed for security reasons");
    }

    // Use resolved path from here on
    path = resolved_path;

    bool append = (mode && strcmp(mode, "append") == 0);

    // Use safe open to prevent TOCTOU attacks
    int fd = safe_open_write(path, append);
    if (fd < 0) {
        char err[256];
        if (errno == ELOOP) {
            snprintf(err, sizeof(err), "Symlinks not allowed: %s", path);
        } else {
            snprintf(err, sizeof(err), "Cannot open file for writing: %s", strerror(errno));
        }
        free(resolved_path);
        return result_error(err);
    }

    FILE* f = fdopen(fd, append ? "a" : "w");
    if (!f) {
        close(fd);
        char err[256];
        snprintf(err, sizeof(err), "Cannot open file for writing: %s", strerror(errno));
        free(resolved_path);
        return result_error(err);
    }

    size_t written = fwrite(content, 1, strlen(content), f);
    fclose(f);

    char msg[256];
    snprintf(msg, sizeof(msg), "Written %zu bytes to %s", written, path);

    ToolResult* r = result_success(msg);
    r->bytes_read = written;
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;

    free(resolved_path);
    return r;
}

static void list_dir_recursive(const char* base_path, const char* pattern,
                                char** output, size_t* len, size_t* capacity, int depth) {
    if (depth > 10) return;  // Limit recursion

    DIR* dir = opendir(base_path);
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;  // Skip hidden

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, entry->d_name);

        // Check pattern
        if (pattern && fnmatch(pattern, entry->d_name, 0) != 0 && entry->d_type != DT_DIR) {
            continue;
        }

        // Get file info
        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        // Format line
        char line[PATH_MAX + 64];
        if (S_ISDIR(st.st_mode)) {
            snprintf(line, sizeof(line), "[DIR]  %s/\n", full_path);
        } else {
            snprintf(line, sizeof(line), "[FILE] %s (%lld bytes)\n", full_path, (long long)st.st_size);
        }

        size_t line_len = strlen(line);
        if (*len + line_len + 1 > *capacity) {
            size_t new_capacity = *capacity * 2;
            char* new_output = realloc(*output, new_capacity);
            if (!new_output) {
                // OOM - stop recursion gracefully
                closedir(dir);
                return;
            }
            *output = new_output;
            *capacity = new_capacity;
        }
        memcpy(*output + *len, line, line_len);
        *len += line_len;
        (*output)[*len] = '\0';

        // Recurse into directories
        if (S_ISDIR(st.st_mode)) {
            list_dir_recursive(full_path, pattern, output, len, capacity, depth + 1);
        }
    }

    closedir(dir);
}

ToolResult* tool_file_list(const char* path, bool recursive, const char* pattern) {
    clock_t start = clock();

    // Resolve relative paths to workspace
    char* resolved_path = tools_resolve_path(path);
    if (!resolved_path) {
        return result_error("Failed to resolve path");
    }

    if (!tools_is_path_safe(resolved_path)) {
        free(resolved_path);
        return result_error("Path not allowed for security reasons");
    }

    // Use resolved path from here on
    path = resolved_path;

    size_t capacity = 4096;
    size_t len = 0;
    char* output = malloc(capacity);
    output[0] = '\0';

    if (recursive) {
        list_dir_recursive(path, pattern, &output, &len, &capacity, 0);
    } else {
        DIR* dir = opendir(path);
        if (!dir) {
            free(output);
            free(resolved_path);
            char err[256];
            snprintf(err, sizeof(err), "Cannot open directory: %s", strerror(errno));
            return result_error(err);
        }

        struct dirent* entry;
        while ((entry = readdir(dir))) {
            if (entry->d_name[0] == '.') continue;

            // Check pattern
            if (pattern && fnmatch(pattern, entry->d_name, 0) != 0) {
                continue;
            }

            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            struct stat st;
            if (stat(full_path, &st) != 0) continue;

            char line[PATH_MAX + 64];
            if (S_ISDIR(st.st_mode)) {
                snprintf(line, sizeof(line), "[DIR]  %s\n", entry->d_name);
            } else {
                snprintf(line, sizeof(line), "[FILE] %s (%lld bytes)\n", entry->d_name, (long long)st.st_size);
            }

            size_t line_len = strlen(line);
            if (len + line_len + 1 > capacity) {
                capacity *= 2;
                char* new_output = realloc(output, capacity);
                if (!new_output) {
                    free(output);
                    free(resolved_path);
                    closedir(dir);
                    return result_error("Out of memory");
                }
                output = new_output;
            }
            memcpy(output + len, line, line_len);
            len += line_len;
            output[len] = '\0';
        }
        closedir(dir);
    }

    ToolResult* r = result_success(output);
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    free(output);
    free(resolved_path);

    return r;
}

// ============================================================================
// SHELL TOOL IMPLEMENTATION
// ============================================================================

ToolResult* tool_shell_exec(const char* command, const char* working_dir, int timeout_sec) {
    clock_t start = clock();

    if (!tools_is_command_safe(command)) {
        return result_error("Command blocked for security reasons");
    }

    if (timeout_sec <= 0) timeout_sec = 30;

    // Thread-safe: save current directory as file descriptor
    int old_cwd_fd = open(".", O_RDONLY | O_DIRECTORY);
    if (old_cwd_fd < 0) {
        return result_error("Cannot save current directory");
    }

    // Determine working directory: use provided, or workspace, or stay in current
    const char* effective_dir = working_dir;
    if (!effective_dir || !effective_dir[0]) {
        effective_dir = tools_get_workspace();
    }

    // Change to working dir if we have one
    if (effective_dir && effective_dir[0]) {
        if (!tools_is_path_safe(effective_dir)) {
            close(old_cwd_fd);
            return result_error("Working directory not allowed");
        }
        if (chdir(effective_dir) != 0) {
            close(old_cwd_fd);
            return result_error("Cannot change to working directory");
        }
    }

    // Execute with popen
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        fchdir(old_cwd_fd);
        close(old_cwd_fd);
        return result_error("Failed to execute command");
    }

    size_t capacity = 4096;
    size_t len = 0;
    char* output = malloc(capacity);
    if (!output) {
        pclose(pipe);
        fchdir(old_cwd_fd);
        close(old_cwd_fd);
        return result_error("Out of memory");
    }
    output[0] = '\0';

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        size_t buf_len = strlen(buffer);
        if (len + buf_len + 1 > capacity) {
            capacity *= 2;
            char* new_output = realloc(output, capacity);
            if (!new_output) {
                free(output);
                pclose(pipe);
                fchdir(old_cwd_fd);
                close(old_cwd_fd);
                return result_error("Out of memory");
            }
            output = new_output;
        }
        memcpy(output + len, buffer, buf_len);
        len += buf_len;
        output[len] = '\0';

        // Simple timeout check (not precise)
        if ((clock() - start) / CLOCKS_PER_SEC > (clock_t)timeout_sec) {
            pclose(pipe);
            fchdir(old_cwd_fd);
            close(old_cwd_fd);
            free(output);
            return result_error("Command timed out");
        }
    }

    int status = pclose(pipe);
    int exit_code = -1;
    if (status != -1) {
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exit_code = 128 + WTERMSIG(status);
        }
    }

    fchdir(old_cwd_fd);
    close(old_cwd_fd);

    ToolResult* r = result_success(output);
    r->exit_code = exit_code;
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;

    if (exit_code != 0) {
        r->success = false;
        r->error = strdup(output);
    }

    free(output);
    return r;
}

// ============================================================================
// WEB TOOL IMPLEMENTATION
// ============================================================================

struct MemoryStruct {
    char* memory;
    size_t size;
};

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

/**
 * Parse JSON headers object and add to CURL slist
 * Format: {"Header-Name": "value", "Another": "value2"}
 * Returns curl_slist that must be freed, or NULL if no headers
 */
static struct curl_slist* parse_headers_json(const char* headers_json) {
    if (!headers_json || headers_json[0] == '\0') return NULL;

    struct curl_slist* headers = NULL;
    const char* p = headers_json;

    // Skip opening brace
    while (*p && *p != '{') p++;
    if (*p == '{') p++;

    while (*p) {
        // Skip whitespace
        while (*p && (*p == ' ' || *p == '\n' || *p == '\t' || *p == ',')) p++;
        if (*p == '}' || *p == '\0') break;

        // Parse key (expect quoted string)
        if (*p != '"') break;
        p++;
        const char* key_start = p;
        while (*p && *p != '"') p++;
        size_t key_len = (size_t)(p - key_start);
        if (*p == '"') p++;

        // Skip colon
        while (*p && (*p == ' ' || *p == ':')) p++;

        // Parse value (expect quoted string)
        if (*p != '"') break;
        p++;
        const char* val_start = p;
        while (*p && *p != '"') p++;
        size_t val_len = (size_t)(p - val_start);
        if (*p == '"') p++;

        // Build header string: "Key: Value"
        if (key_len > 0 && val_len > 0) {
            char header[512];
            size_t header_len = key_len + 2 + val_len;  // key + ": " + value
            if (header_len < sizeof(header)) {
                snprintf(header, sizeof(header), "%.*s: %.*s",
                         (int)key_len, key_start, (int)val_len, val_start);
                headers = curl_slist_append(headers, header);
            }
        }
    }

    return headers;
}

ToolResult* tool_web_fetch(const char* url, const char* method, const char* headers_json) {
    clock_t start = clock();

    CURL* curl = curl_easy_init();
    if (!curl) {
        return result_error("Failed to initialize curl");
    }

    struct MemoryStruct chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    // Parse and apply custom headers
    struct curl_slist* custom_headers = parse_headers_json(headers_json);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Convergio/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

    if (custom_headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, custom_headers);
    }

    if (method && strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    }

    CURLcode res = curl_easy_perform(curl);

    // Free custom headers
    if (custom_headers) {
        curl_slist_free_all(custom_headers);
    }

    ToolResult* r;
    if (res != CURLE_OK) {
        char err[256];
        snprintf(err, sizeof(err), "Fetch failed: %s", curl_easy_strerror(res));
        r = result_error(err);
    } else {
        // Simple HTML stripping (basic)
        // In production, use a proper HTML parser
        r = result_success(chunk.memory);
        r->bytes_read = chunk.size;
    }

    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;

    curl_easy_cleanup(curl);
    free(chunk.memory);

    return r;
}

// ============================================================================
// MEMORY/RAG TOOLS IMPLEMENTATION
// ============================================================================

// Forward declaration - implemented in memory module
extern int persistence_save_memory(const char* content, const char* category, float importance);
extern char** persistence_search_memories(const char* query, size_t max_results,
                                          float min_similarity, size_t* out_count);

ToolResult* tool_memory_store(const char* content, const char* category, float importance) {
    clock_t start = clock();

    if (!content || strlen(content) == 0) {
        return result_error("Content cannot be empty");
    }

    if (importance < 0.0f) importance = 0.0f;
    if (importance > 1.0f) importance = 1.0f;

    int result = persistence_save_memory(content, category, importance);

    ToolResult* r;
    if (result == 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Stored memory with importance %.2f", (double)importance);
        r = result_success(msg);
    } else {
        r = result_error("Failed to store memory");
    }

    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    return r;
}

ToolResult* tool_memory_search(const char* query, size_t max_results, float min_similarity) {
    clock_t start = clock();

    if (!query || strlen(query) == 0) {
        return result_error("Query cannot be empty");
    }

    if (max_results == 0) max_results = 5;
    if (min_similarity < 0.0f) min_similarity = 0.5f;

    size_t count = 0;
    char** memories = persistence_search_memories(query, max_results, min_similarity, &count);

    if (!memories || count == 0) {
        ToolResult* r = result_success("No relevant memories found.");
        r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
        return r;
    }

    // Build output
    size_t capacity = 4096;
    char* output = malloc(capacity);
    snprintf(output, capacity, "Found %zu relevant memories:\n\n", count);

    size_t out_len = strlen(output);
    for (size_t i = 0; i < count; i++) {
        size_t mem_len = strlen(memories[i]);
        size_t needed = out_len + mem_len + 32;
        if (needed > capacity) {
            size_t new_capacity = needed * 2;
            char* new_output = realloc(output, new_capacity);
            if (!new_output) {
                // Free remaining memories and return error
                for (size_t j = i; j < count; j++) free(memories[j]);
                free(memories);
                free(output);
                return result_error("Out of memory");
            }
            output = new_output;
            capacity = new_capacity;
        }
        out_len += (size_t)snprintf(output + out_len, capacity - out_len, "[%zu] %s\n\n", i + 1, memories[i]);
        free(memories[i]);
    }
    free(memories);

    ToolResult* r = result_success(output);
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    free(output);

    return r;
}

// ============================================================================
// NOTE TOOLS IMPLEMENTATION
// ============================================================================

// Ensure directory exists
static void ensure_dir(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0755);
    }
}

// Sanitize filename (remove special chars)
static void sanitize_filename(char* name) {
    for (char* p = name; *p; p++) {
        if (!isalnum(*p) && *p != '-' && *p != '_') {
            *p = '-';
        }
    }
}

ToolResult* tool_note_write(const char* title, const char* content, const char* tags) {
    clock_t start = clock();

    if (!title || !content) {
        return result_error("Title and content are required");
    }

    ensure_dir(get_notes_dir());

    // Build filename
    char filename[PATH_MAX];
    char safe_title[256];
    strncpy(safe_title, title, sizeof(safe_title) - 1);
    safe_title[sizeof(safe_title) - 1] = '\0';
    sanitize_filename(safe_title);

    snprintf(filename, sizeof(filename), "%s/%s.md", get_notes_dir(), safe_title);

    // Build markdown content with frontmatter
    size_t full_size = strlen(content) + 512;
    char* full_content = malloc(full_size);

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", tm_info);

    snprintf(full_content, full_size,
        "---\n"
        "title: %s\n"
        "date: %s\n"
        "tags: %s\n"
        "---\n\n"
        "%s",
        title, date_str, tags ? tags : "", content);

    FILE* f = fopen(filename, "w");
    if (!f) {
        free(full_content);
        return result_error("Failed to create note file");
    }

    fputs(full_content, f);
    fclose(f);
    free(full_content);

    char msg[512];
    snprintf(msg, sizeof(msg), "Note '%s' saved to %s", title, filename);

    ToolResult* r = result_success(msg);
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    return r;
}

ToolResult* tool_note_read(const char* title, const char* search) {
    clock_t start = clock();

    if (title) {
        // Read specific note
        char filename[PATH_MAX];
        char safe_title[256];
        strncpy(safe_title, title, sizeof(safe_title) - 1);
        safe_title[sizeof(safe_title) - 1] = '\0';
        sanitize_filename(safe_title);

        snprintf(filename, sizeof(filename), "%s/%s.md", get_notes_dir(), safe_title);

        FILE* f = fopen(filename, "r");
        if (!f) {
            char err[256];
            snprintf(err, sizeof(err), "Note '%s' not found", title);
            return result_error(err);
        }

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        if (size < 0) {
            fclose(f);
            return result_error("Cannot determine file size");
        }
        fseek(f, 0, SEEK_SET);

        char* content = malloc((size_t)size + 1);
        if (!content) {
            fclose(f);
            return result_error("Memory allocation failed");
        }
        size_t bytes_read = fread(content, 1, (size_t)size, f);
        content[bytes_read] = '\0';
        fclose(f);

        ToolResult* r = result_success(content);
        r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
        free(content);
        return r;
    }

    if (search) {
        // Search notes for content
        DIR* dir = opendir(get_notes_dir());
        if (!dir) {
            return result_error("Notes directory not found");
        }

        size_t capacity = 8192;
        char* output = malloc(capacity);
        snprintf(output, capacity, "Notes matching '%s':\n\n", search);
        size_t len = strlen(output);

        struct dirent* entry;
        while ((entry = readdir(dir))) {
            if (entry->d_name[0] == '.' || !strstr(entry->d_name, ".md")) continue;

            char filepath[PATH_MAX];
            snprintf(filepath, sizeof(filepath), "%s/%s", get_notes_dir(), entry->d_name);

            FILE* f = fopen(filepath, "r");
            if (!f) continue;

            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            if (size < 0) {
                fclose(f);
                continue;
            }
            fseek(f, 0, SEEK_SET);

            char* content = malloc((size_t)size + 1);
            if (!content) {
                fclose(f);
                continue;
            }
            size_t bytes_read = fread(content, 1, (size_t)size, f);
            content[bytes_read] = '\0';
            fclose(f);

            // Check if search term is in content (case-insensitive)
            char* lower_content = strdup(content);
            char* lower_search = strdup(search);
            for (char* p = lower_content; *p; p++) *p = (char)tolower((unsigned char)*p);
            for (char* p = lower_search; *p; p++) *p = (char)tolower((unsigned char)*p);

            if (strstr(lower_content, lower_search)) {
                size_t needed = len + strlen(entry->d_name) + 256;
                if (needed > capacity) {
                    capacity = needed * 2;
                    char* new_output = realloc(output, capacity);
                    if (!new_output) {
                        free(output);
                        free(lower_content);
                        free(lower_search);
                        free(content);
                        closedir(dir);
                        return result_error("Out of memory");
                    }
                    output = new_output;
                }

                // Extract first line (title)
                char* first_line = content;
                char* newline = strchr(content, '\n');
                if (newline) *newline = '\0';

                len += (size_t)snprintf(output + len, capacity - len,
                    "- **%s**: %s\n", entry->d_name, first_line);
            }

            free(lower_content);
            free(lower_search);
            free(content);
        }
        closedir(dir);

        ToolResult* r = result_success(output);
        r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
        free(output);
        return r;
    }

    return result_error("Specify either 'title' or 'search' parameter");
}

ToolResult* tool_note_list(const char* tag_filter) {
    clock_t start = clock();

    DIR* dir = opendir(get_notes_dir());
    if (!dir) {
        ensure_dir(get_notes_dir());
        return result_success("Notes directory is empty.");
    }

    size_t capacity = 8192;
    char* output = malloc(capacity);
    snprintf(output, capacity, "Available notes:\n\n");
    size_t len = strlen(output);
    int count = 0;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.' || !strstr(entry->d_name, ".md")) continue;

        char filepath[PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", get_notes_dir(), entry->d_name);

        struct stat st;
        if (stat(filepath, &st) != 0) continue;

        // Read frontmatter to get tags
        FILE* f = fopen(filepath, "r");
        if (!f) continue;

        char line[512];
        char title[256] = "";
        char tags[256] = "";
        char date[64] = "";
        bool in_frontmatter = false;

        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "---", 3) == 0) {
                if (in_frontmatter) break;
                in_frontmatter = true;
                continue;
            }
            if (in_frontmatter) {
                if (strncmp(line, "title:", 6) == 0) {
                    strncpy(title, line + 7, sizeof(title) - 1);
                    title[strcspn(title, "\n")] = '\0';
                } else if (strncmp(line, "tags:", 5) == 0) {
                    strncpy(tags, line + 6, sizeof(tags) - 1);
                    tags[strcspn(tags, "\n")] = '\0';
                } else if (strncmp(line, "date:", 5) == 0) {
                    strncpy(date, line + 6, sizeof(date) - 1);
                    date[strcspn(date, "\n")] = '\0';
                }
            }
        }
        fclose(f);

        // Apply tag filter
        if (tag_filter && tag_filter[0] && !strstr(tags, tag_filter)) {
            continue;
        }

        size_t needed = len + 256;
        if (needed > capacity) {
            size_t new_capacity = needed * 2;
            char* new_output = realloc(output, new_capacity);
            if (!new_output) {
                free(output);
                closedir(dir);
                return result_error("Out of memory");
            }
            output = new_output;
            capacity = new_capacity;
        }

        len += (size_t)snprintf(output + len, capacity - len,
            "- **%s** [%s] - %s\n",
            title[0] ? title : entry->d_name,
            tags[0] ? tags : "no tags",
            date[0] ? date : "unknown date");
        count++;
    }
    closedir(dir);

    if (count == 0) {
        snprintf(output, 8192, "No notes found.");
    }

    ToolResult* r = result_success(output);
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    free(output);
    return r;
}

// ============================================================================
// KNOWLEDGE BASE TOOLS IMPLEMENTATION
// ============================================================================

ToolResult* tool_knowledge_search(const char* query, size_t max_results) {
    clock_t start = clock();

    if (!query || strlen(query) == 0) {
        return result_error("Query cannot be empty");
    }

    if (max_results == 0) max_results = 5;

    ensure_dir(get_knowledge_dir());

    // Recursive search in knowledge directory
    size_t capacity = 16384;
    char* output = malloc(capacity);
    snprintf(output, capacity, "Knowledge search results for '%s':\n\n", query);
    size_t len = strlen(output);
    int found = 0;

    // Sanitize query to prevent command injection
    char* safe_query = strdup(query);
    if (!safe_query) {
        free(output);
        return result_error("Memory allocation failed");
    }
    sanitize_grep_pattern(safe_query);

    // Escape for shell (belt and suspenders after sanitization)
    char* escaped_query = shell_escape(safe_query);
    free(safe_query);
    if (!escaped_query) {
        free(output);
        return result_error("Memory allocation failed");
    }

    // Simple implementation: search all .md files recursively
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "grep -r -l -i '%s' %s 2>/dev/null | head -%zu",
        escaped_query, get_knowledge_dir(), max_results);
    free(escaped_query);

    FILE* pipe = popen(cmd, "r");
    if (pipe) {
        char filepath[PATH_MAX];
        while (fgets(filepath, sizeof(filepath), pipe) && found < (int)max_results) {
            filepath[strcspn(filepath, "\n")] = '\0';

            // Read file content
            FILE* f = fopen(filepath, "r");
            if (!f) continue;

            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            if (size < 0) {
                fclose(f);
                continue;
            }
            if (size > 4096) size = 4096;  // Limit preview
            fseek(f, 0, SEEK_SET);

            char* content = malloc((size_t)size + 1);
            if (!content) {
                fclose(f);
                continue;
            }
            size_t bytes_read = fread(content, 1, (size_t)size, f);
            content[bytes_read] = '\0';
            fclose(f);

            // Add to output
            size_t needed = len + strlen(filepath) + (size_t)size + 64;
            if (needed > capacity) {
                size_t new_capacity = needed * 2;
                char* new_output = realloc(output, new_capacity);
                if (!new_output) {
                    free(content);
                    free(output);
                    pclose(pipe);
                    return result_error("Out of memory");
                }
                output = new_output;
                capacity = new_capacity;
            }

            len += (size_t)snprintf(output + len, capacity - len,
                "### %s\n%s\n\n---\n\n",
                filepath + strlen(get_knowledge_dir()) + 1,  // Remove prefix
                content);

            free(content);
            found++;
        }
        pclose(pipe);
    }

    if (found == 0) {
        snprintf(output, 8192, "No knowledge found matching your query.");
    }

    ToolResult* r = result_success(output);
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    free(output);
    return r;
}

ToolResult* tool_knowledge_add(const char* title, const char* content, const char* category) {
    clock_t start = clock();

    if (!title || !content) {
        return result_error("Title and content are required");
    }

    ensure_dir(get_knowledge_dir());

    // Create category subdirectory if specified
    char dirpath[PATH_MAX];
    if (category && category[0]) {
        snprintf(dirpath, sizeof(dirpath), "%s/%s", get_knowledge_dir(), category);
        ensure_dir(dirpath);
    } else {
        strncpy(dirpath, get_knowledge_dir(), sizeof(dirpath));
    }

    // Build filename
    char filename[PATH_MAX];
    char safe_title[256];
    strncpy(safe_title, title, sizeof(safe_title) - 1);
    safe_title[sizeof(safe_title) - 1] = '\0';
    sanitize_filename(safe_title);

    snprintf(filename, sizeof(filename), "%s/%s.md", dirpath, safe_title);

    // Build markdown content with frontmatter
    size_t full_size = strlen(content) + 512;
    char* full_content = malloc(full_size);

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);

    snprintf(full_content, full_size,
        "---\n"
        "title: %s\n"
        "category: %s\n"
        "created: %s\n"
        "---\n\n"
        "# %s\n\n"
        "%s",
        title, category ? category : "general", date_str, title, content);

    FILE* f = fopen(filename, "w");
    if (!f) {
        free(full_content);
        return result_error("Failed to create knowledge file");
    }

    fputs(full_content, f);
    fclose(f);
    free(full_content);

    char msg[512];
    snprintf(msg, sizeof(msg), "Knowledge '%s' added to %s", title, filename);

    ToolResult* r = result_success(msg);
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    return r;
}

// ============================================================================
// PROJECT TEAM MANAGEMENT TOOL
// ============================================================================

ToolResult* tool_project_team(const char* action, const char* agent_name) {
    clock_t start = clock();

    ConvergioProject* proj = project_current();
    if (!proj) {
        return result_error("No active project. Use 'project use <name>' first.");
    }

    if (!action) {
        return result_error("Action is required: add, remove, or list");
    }

    char msg[1024];

    if (strcmp(action, "list") == 0) {
        // List current team members
        size_t offset = (size_t)snprintf(msg, sizeof(msg),
            "Project '%s' team (%zu members):\n", proj->name, proj->team_count);
        for (size_t i = 0; i < proj->team_count && offset < sizeof(msg) - 64; i++) {
            offset += (size_t)snprintf(msg + offset, sizeof(msg) - offset,
                "- %s%s%s\n",
                proj->team[i].agent_name,
                proj->team[i].role ? " (" : "",
                proj->team[i].role ? proj->team[i].role : "");
            if (proj->team[i].role) offset += (size_t)snprintf(msg + offset, sizeof(msg) - offset, ")");
        }
        ToolResult* r = result_success(msg);
        r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
        return r;
    }

    if (!agent_name || !agent_name[0]) {
        return result_error("Agent name is required for add/remove actions");
    }

    if (strcmp(action, "add") == 0) {
        if (project_has_agent(agent_name)) {
            snprintf(msg, sizeof(msg), "Agent '%s' is already in project '%s'",
                     agent_name, proj->name);
        } else if (project_team_add(proj, agent_name, NULL)) {
            snprintf(msg, sizeof(msg), "Added '%s' to project '%s' team. Team now has %zu members.",
                     agent_name, proj->name, proj->team_count);
        } else {
            return result_error("Failed to add agent to project");
        }
    } else if (strcmp(action, "remove") == 0) {
        if (!project_has_agent(agent_name)) {
            snprintf(msg, sizeof(msg), "Agent '%s' is not in project '%s'",
                     agent_name, proj->name);
        } else if (project_team_remove(proj, agent_name)) {
            snprintf(msg, sizeof(msg), "Removed '%s' from project '%s' team. Team now has %zu members.",
                     agent_name, proj->name, proj->team_count);
        } else {
            return result_error("Failed to remove agent from project");
        }
    } else {
        return result_error("Invalid action. Use: add, remove, or list");
    }

    ToolResult* r = result_success(msg);
    r->execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    return r;
}

// ============================================================================
// TOOL CALL PARSING
// ============================================================================

// Simple JSON string extraction (production should use a JSON library)
static char* json_get_string(const char* json, const char* key) {
    if (!json || !key) return NULL;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    char* pos = strstr(json, pattern);
    if (!pos) return NULL;

    pos = strchr(pos, ':');
    if (!pos) return NULL;

    // Skip whitespace
    pos++;
    while (*pos == ' ' || *pos == '\t' || *pos == '\n') pos++;

    if (*pos == '"') {
        // String value
        pos++;
        char* end = strchr(pos, '"');
        if (!end) return NULL;

        size_t len = (size_t)(end - pos);
        char* value = malloc(len + 1);
        strncpy(value, pos, len);
        value[len] = '\0';
        return value;
    }

    return NULL;
}

static int json_get_int(const char* json, const char* key, int default_val) {
    if (!json || !key) return default_val;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    char* pos = strstr(json, pattern);
    if (!pos) return default_val;

    pos = strchr(pos, ':');
    if (!pos) return default_val;

    return atoi(pos + 1);
}

static double json_get_double(const char* json, const char* key, double default_val) {
    if (!json || !key) return default_val;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    char* pos = strstr(json, pattern);
    if (!pos) return default_val;

    pos = strchr(pos, ':');
    if (!pos) return default_val;

    return atof(pos + 1);
}

static bool json_get_bool(const char* json, const char* key, bool default_val) {
    if (!json || !key) return default_val;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    char* pos = strstr(json, pattern);
    if (!pos) return default_val;

    pos = strchr(pos, ':');
    if (!pos) return default_val;

    pos++;
    while (*pos == ' ' || *pos == '\t') pos++;

    if (strncmp(pos, "true", 4) == 0) return true;
    if (strncmp(pos, "false", 5) == 0) return false;

    return default_val;
}

LocalToolCall* tools_parse_call(const char* tool_name, const char* arguments_json) {
    if (!tool_name) return NULL;

    LocalToolCall* call = calloc(1, sizeof(LocalToolCall));
    call->tool_name = strdup(tool_name);
    call->parameters_json = arguments_json ? strdup(arguments_json) : strdup("{}");

    // Determine type from name
    if (strcmp(tool_name, "file_read") == 0) {
        call->type = TOOL_FILE_READ;
    } else if (strcmp(tool_name, "file_write") == 0) {
        call->type = TOOL_FILE_WRITE;
    } else if (strcmp(tool_name, "file_list") == 0) {
        call->type = TOOL_FILE_LIST;
    } else if (strcmp(tool_name, "shell_exec") == 0) {
        call->type = TOOL_SHELL_EXEC;
    } else if (strcmp(tool_name, "web_fetch") == 0) {
        call->type = TOOL_WEB_FETCH;
    } else if (strcmp(tool_name, "memory_store") == 0) {
        call->type = TOOL_MEMORY_STORE;
    } else if (strcmp(tool_name, "memory_search") == 0) {
        call->type = TOOL_MEMORY_SEARCH;
    } else if (strcmp(tool_name, "note_write") == 0) {
        call->type = TOOL_NOTE_WRITE;
    } else if (strcmp(tool_name, "note_read") == 0) {
        call->type = TOOL_NOTE_READ;
    } else if (strcmp(tool_name, "note_list") == 0) {
        call->type = TOOL_NOTE_LIST;
    } else if (strcmp(tool_name, "knowledge_search") == 0) {
        call->type = TOOL_KNOWLEDGE_SEARCH;
    } else if (strcmp(tool_name, "knowledge_add") == 0) {
        call->type = TOOL_KNOWLEDGE_ADD;
    } else if (strcmp(tool_name, "project_team") == 0) {
        call->type = TOOL_PROJECT_TEAM;
    } else if (strcmp(tool_name, "todo_create") == 0) {
        call->type = TOOL_TODO_CREATE;
    } else if (strcmp(tool_name, "todo_list") == 0) {
        call->type = TOOL_TODO_LIST;
    } else if (strcmp(tool_name, "todo_update") == 0) {
        call->type = TOOL_TODO_UPDATE;
    } else if (strcmp(tool_name, "todo_delete") == 0) {
        call->type = TOOL_TODO_DELETE;
    } else if (strcmp(tool_name, "notify_schedule") == 0) {
        call->type = TOOL_NOTIFY_SCHEDULE;
    } else if (strcmp(tool_name, "notify_cancel") == 0) {
        call->type = TOOL_NOTIFY_CANCEL;
    } else {
        tools_free_call(call);
        return NULL;
    }

    return call;
}

// ============================================================================
// TOOL EXECUTION
// ============================================================================

ToolResult* tools_execute(const LocalToolCall* call) {
    if (!call) return result_error("Invalid tool call");

    const char* args = call->parameters_json;

    switch (call->type) {
        case TOOL_FILE_READ: {
            char* path = json_get_string(args, "path");
            int start = json_get_int(args, "start_line", 0);
            int end = json_get_int(args, "end_line", 0);
            ToolResult* r = tool_file_read(path, start, end);
            free(path);
            return r;
        }

        case TOOL_FILE_WRITE: {
            char* path = json_get_string(args, "path");
            char* content = json_get_string(args, "content");
            char* mode = json_get_string(args, "mode");
            ToolResult* r = tool_file_write(path, content, mode);
            free(path);
            free(content);
            free(mode);
            return r;
        }

        case TOOL_FILE_LIST: {
            char* path = json_get_string(args, "path");
            bool recursive = json_get_bool(args, "recursive", false);
            char* pattern = json_get_string(args, "pattern");
            ToolResult* r = tool_file_list(path, recursive, pattern);
            free(path);
            free(pattern);
            return r;
        }

        case TOOL_SHELL_EXEC: {
            char* command = json_get_string(args, "command");
            char* working_dir = json_get_string(args, "working_dir");
            int timeout = json_get_int(args, "timeout", 30);
            ToolResult* r = tool_shell_exec(command, working_dir, timeout);
            free(command);
            free(working_dir);
            return r;
        }

        case TOOL_WEB_FETCH: {
            char* url = json_get_string(args, "url");
            char* method = json_get_string(args, "method");
            ToolResult* r = tool_web_fetch(url, method, NULL);
            free(url);
            free(method);
            return r;
        }

        case TOOL_MEMORY_STORE: {
            char* content = json_get_string(args, "content");
            char* category = json_get_string(args, "category");
            float importance = (float)json_get_double(args, "importance", 0.5);
            ToolResult* r = tool_memory_store(content, category, importance);
            free(content);
            free(category);
            return r;
        }

        case TOOL_MEMORY_SEARCH: {
            char* query = json_get_string(args, "query");
            int max_results = json_get_int(args, "max_results", 5);
            float min_sim = (float)json_get_double(args, "min_similarity", 0.5);
            ToolResult* r = tool_memory_search(query, (size_t)max_results, min_sim);
            free(query);
            return r;
        }

        case TOOL_NOTE_WRITE: {
            char* title = json_get_string(args, "title");
            char* content = json_get_string(args, "content");
            char* tags = json_get_string(args, "tags");
            ToolResult* r = tool_note_write(title, content, tags);
            free(title);
            free(content);
            free(tags);
            return r;
        }

        case TOOL_NOTE_READ: {
            char* title = json_get_string(args, "title");
            char* search = json_get_string(args, "search");
            ToolResult* r = tool_note_read(title, search);
            free(title);
            free(search);
            return r;
        }

        case TOOL_NOTE_LIST: {
            char* tag = json_get_string(args, "tag");
            ToolResult* r = tool_note_list(tag);
            free(tag);
            return r;
        }

        case TOOL_KNOWLEDGE_SEARCH: {
            char* query = json_get_string(args, "query");
            int max_results = json_get_int(args, "max_results", 5);
            ToolResult* r = tool_knowledge_search(query, (size_t)max_results);
            free(query);
            return r;
        }

        case TOOL_KNOWLEDGE_ADD: {
            char* title = json_get_string(args, "title");
            char* content = json_get_string(args, "content");
            char* category = json_get_string(args, "category");
            ToolResult* r = tool_knowledge_add(title, content, category);
            free(title);
            free(content);
            free(category);
            return r;
        }

        case TOOL_PROJECT_TEAM: {
            char* action = json_get_string(args, "action");
            char* agent_name = json_get_string(args, "agent_name");
            ToolResult* r = tool_project_team(action, agent_name);
            free(action);
            free(agent_name);
            return r;
        }

        case TOOL_TODO_CREATE: {
            char* title = json_get_string(args, "title");
            char* description = json_get_string(args, "description");
            char* priority = json_get_string(args, "priority");
            char* due_date = json_get_string(args, "due_date");
            char* tags = json_get_string(args, "tags");
            ToolResult* r = tool_todo_create(title, description, priority, due_date, tags);
            free(title);
            free(description);
            free(priority);
            free(due_date);
            free(tags);
            return r;
        }

        case TOOL_TODO_LIST: {
            char* status = json_get_string(args, "status");
            char* priority = json_get_string(args, "priority");
            int limit = json_get_int(args, "limit", 10);
            ToolResult* r = tool_todo_list(status, priority, limit);
            free(status);
            free(priority);
            return r;
        }

        case TOOL_TODO_UPDATE: {
            int64_t task_id = (int64_t)json_get_int(args, "task_id", 0);
            char* status = json_get_string(args, "status");
            char* priority = json_get_string(args, "priority");
            char* due_date = json_get_string(args, "due_date");
            ToolResult* r = tool_todo_update(task_id, status, priority, due_date);
            free(status);
            free(priority);
            free(due_date);
            return r;
        }

        case TOOL_TODO_DELETE: {
            int64_t task_id = (int64_t)json_get_int(args, "task_id", 0);
            ToolResult* r = tool_todo_delete(task_id);
            return r;
        }

        case TOOL_NOTIFY_SCHEDULE: {
            char* message = json_get_string(args, "message");
            char* when = json_get_string(args, "when");
            char* sound = json_get_string(args, "sound");
            ToolResult* r = tool_notify_schedule(message, when, sound);
            free(message);
            free(when);
            free(sound);
            return r;
        }

        case TOOL_NOTIFY_CANCEL: {
            int64_t notify_id = (int64_t)json_get_int(args, "notify_id", 0);
            ToolResult* r = tool_notify_cancel(notify_id);
            return r;
        }

        default:
            return result_error("Unknown tool type");
    }
}

// ============================================================================
// TODO TOOLS (Anna's task management)
// ============================================================================

// Helper: Convert priority string to enum
static TodoPriority parse_priority(const char* str) {
    if (!str) return TODO_PRIORITY_NORMAL;
    if (strcasecmp(str, "critical") == 0 || strcasecmp(str, "urgent") == 0) return TODO_PRIORITY_URGENT;
    if (strcasecmp(str, "high") == 0) return TODO_PRIORITY_URGENT;
    if (strcasecmp(str, "low") == 0) return TODO_PRIORITY_LOW;
    return TODO_PRIORITY_NORMAL;
}

// Helper: Convert status string to enum
static TodoStatus parse_status(const char* str) {
    if (!str) return TODO_STATUS_PENDING;
    if (strcasecmp(str, "in_progress") == 0) return TODO_STATUS_IN_PROGRESS;
    if (strcasecmp(str, "completed") == 0) return TODO_STATUS_COMPLETED;
    if (strcasecmp(str, "cancelled") == 0) return TODO_STATUS_CANCELLED;
    return TODO_STATUS_PENDING;
}

ToolResult* tool_todo_create(const char* title, const char* description, const char* priority,
                             const char* due_date, const char* tags) {
    if (!title || strlen(title) == 0) {
        return result_error("Task title is required");
    }

    TodoCreateOptions opts = {
        .title = title,
        .description = description,
        .priority = parse_priority(priority),
        .due_date = due_date ? todo_parse_date(due_date, time(NULL)) : 0,
        .reminder_at = 0,
        .recurrence = TODO_RECURRENCE_NONE,
        .recurrence_rule = NULL,
        .tags = tags,
        .context = NULL,
        .parent_id = 0,
        .source = TODO_SOURCE_AGENT,
        .external_id = NULL
    };

    int64_t task_id = todo_create(&opts);
    if (task_id < 0) {
        return result_error("Failed to create task");
    }

    // Format success response
    char response[512];
    if (opts.due_date > 0) {
        char due_str[64];
        struct tm* tm = localtime(&opts.due_date);
        strftime(due_str, sizeof(due_str), "%Y-%m-%d %H:%M", tm);
        snprintf(response, sizeof(response),
                 "Task created successfully:\n- ID: %lld\n- Title: %s\n- Due: %s\n- Priority: %s",
                 (long long)task_id, title, due_str, priority ? priority : "normal");
    } else {
        snprintf(response, sizeof(response),
                 "Task created successfully:\n- ID: %lld\n- Title: %s\n- Priority: %s",
                 (long long)task_id, title, priority ? priority : "normal");
    }

    return result_success(response);
}

ToolResult* tool_todo_list(const char* status, const char* priority, int limit) {
    TodoFilter filter = {0};
    TodoStatus status_arr[1];
    TodoPriority priority_arr[1];

    // Parse status filter
    if (status && strcasecmp(status, "all") != 0) {
        status_arr[0] = parse_status(status);
        filter.statuses = status_arr;
        filter.status_count = 1;
    }

    // Parse priority filter
    if (priority && strcasecmp(priority, "all") != 0) {
        priority_arr[0] = parse_priority(priority);
        filter.priorities = priority_arr;
        filter.priority_count = 1;
    }

    filter.limit = limit > 0 ? limit : 10;
    filter.include_completed = (status && strcasecmp(status, "completed") == 0);

    int count = 0;
    TodoTask** tasks = todo_list(&filter, &count);

    if (!tasks || count == 0) {
        return result_success("No tasks found matching the filter.");
    }

    // Build response
    size_t buf_size = 4096;
    char* buf = malloc(buf_size);
    size_t pos = 0;

    pos += snprintf(buf + pos, buf_size - pos, "Found %d task(s):\n\n", count);

    for (int i = 0; i < count && pos < buf_size - 200; i++) {
        TodoTask* t = tasks[i];
        const char* status_str = t->status == TODO_STATUS_PENDING ? "pending" :
                                 t->status == TODO_STATUS_IN_PROGRESS ? "in_progress" :
                                 t->status == TODO_STATUS_COMPLETED ? "completed" : "cancelled";
        const char* pri_str = t->priority == TODO_PRIORITY_URGENT ? "high" :
                              t->priority == TODO_PRIORITY_LOW ? "low" : "normal";

        pos += snprintf(buf + pos, buf_size - pos, "[%lld] %s\n", (long long)t->id, t->title);
        pos += snprintf(buf + pos, buf_size - pos, "    Status: %s | Priority: %s\n", status_str, pri_str);

        if (t->due_date > 0) {
            char due_str[64];
            struct tm* tm = localtime(&t->due_date);
            strftime(due_str, sizeof(due_str), "%Y-%m-%d %H:%M", tm);
            pos += snprintf(buf + pos, buf_size - pos, "    Due: %s\n", due_str);
        }

        pos += snprintf(buf + pos, buf_size - pos, "\n");
        todo_free_task(t);
    }
    free(tasks);

    ToolResult* r = result_success(buf);
    free(buf);
    return r;
}

ToolResult* tool_todo_update(int64_t task_id, const char* status, const char* priority,
                             const char* due_date) {
    if (task_id <= 0) {
        return result_error("Invalid task ID");
    }

    // First check if task exists
    TodoTask* existing = todo_get(task_id);
    if (!existing) {
        return result_error("Task not found");
    }

    // Handle status change
    if (status) {
        TodoStatus new_status = parse_status(status);
        if (new_status == TODO_STATUS_COMPLETED) {
            todo_complete(task_id);
        } else {
            TodoCreateOptions opts = {0};
            opts.priority = priority ? parse_priority(priority) : existing->priority;
            opts.due_date = due_date ? todo_parse_date(due_date, time(NULL)) : existing->due_date;
            // Note: We can't directly set status via update, so we just update other fields
            todo_update(task_id, &opts);
        }
    } else if (priority || due_date) {
        TodoCreateOptions opts = {0};
        opts.priority = priority ? parse_priority(priority) : existing->priority;
        opts.due_date = due_date ? todo_parse_date(due_date, time(NULL)) : existing->due_date;
        todo_update(task_id, &opts);
    }

    todo_free_task(existing);

    char response[128];
    snprintf(response, sizeof(response), "Task %lld updated successfully.", (long long)task_id);
    return result_success(response);
}

ToolResult* tool_todo_delete(int64_t task_id) {
    if (task_id <= 0) {
        return result_error("Invalid task ID");
    }

    int result = todo_delete(task_id);
    if (result != 0) {
        return result_error("Failed to delete task (may not exist)");
    }

    char response[128];
    snprintf(response, sizeof(response), "Task %lld deleted successfully.", (long long)task_id);
    return result_success(response);
}

// ============================================================================
// NOTIFICATION TOOLS (Anna's reminder system)
// ============================================================================

ToolResult* tool_notify_schedule(const char* message, const char* when, const char* sound) {
    if (!message || strlen(message) == 0) {
        return result_error("Reminder message is required");
    }
    if (!when || strlen(when) == 0) {
        return result_error("Time is required (e.g., 'in 2 hours', 'tra 5 minuti', '14:30')");
    }

    // Parse the time using natural language parser
    time_t fire_at = todo_parse_date(when, time(NULL));
    if (fire_at <= time(NULL)) {
        return result_error("Scheduled time must be in the future");
    }

    // Create a task for the reminder (so it appears in task list)
    TodoCreateOptions task_opts = {
        .title = message,
        .description = "Scheduled reminder",
        .priority = TODO_PRIORITY_NORMAL,
        .due_date = fire_at,
        .reminder_at = fire_at,
        .source = TODO_SOURCE_AGENT
    };
    int64_t task_id = todo_create(&task_opts);

    // Schedule the notification
    int64_t notify_id = notify_schedule(task_id, fire_at, NOTIFY_METHOD_NATIVE);
    if (notify_id < 0) {
        // Try osascript as fallback
        notify_id = notify_schedule(task_id, fire_at, NOTIFY_METHOD_OSASCRIPT);
    }

    if (notify_id < 0) {
        return result_error("Failed to schedule notification");
    }

    // Format response
    char response[256];
    char time_str[64];
    struct tm* tm = localtime(&fire_at);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm);

    snprintf(response, sizeof(response),
             "Reminder scheduled:\n- Message: %s\n- Time: %s\n- Notification ID: %lld",
             message, time_str, (long long)notify_id);

    return result_success(response);
}

ToolResult* tool_notify_cancel(int64_t notify_id) {
    if (notify_id <= 0) {
        return result_error("Invalid notification ID");
    }

    int result = notify_cancel(notify_id);
    if (result != 0) {
        return result_error("Failed to cancel notification (may not exist)");
    }

    char response[128];
    snprintf(response, sizeof(response), "Notification %lld cancelled successfully.", (long long)notify_id);
    return result_success(response);
}
