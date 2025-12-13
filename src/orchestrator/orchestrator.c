/**
 * CONVERGIO ORCHESTRATOR
 *
 * The heart of the system - Ali coordinates everything:
 * - User input processing
 * - Agent delegation (see delegation.c)
 * - Task planning (see planning.c)
 * - Convergence (see convergence.c)
 * - Cost management
 */

#include "nous/orchestrator.h"
#include "nous/delegation.h"
#include "nous/planning.h"
#include "nous/convergence.h"
#include "nous/updater.h"
#include "nous/nous.h"
#include "nous/projects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dispatch/dispatch.h>
#include <dirent.h>
#include <limits.h>
#include "nous/debug_mutex.h"

// Global orchestrator instance
static Orchestrator* g_orchestrator = NULL;
CONVERGIO_MUTEX_DECLARE(g_orch_mutex);

// External functions
extern int persistence_init(const char* db_path);
extern void persistence_shutdown(void);
extern int msgbus_init(void);
extern void msgbus_shutdown(void);
extern char* persistence_create_session(const char* user_name);
extern char* persistence_get_or_create_session(void);
extern int persistence_save_conversation(const char* session_id, const char* role,
                                          const char* content, int tokens);
extern char* persistence_load_conversation_context(const char* session_id, size_t max_messages);
extern char* persistence_load_recent_context(size_t max_messages);
extern char** persistence_get_important_memories(size_t limit, size_t* out_count);
extern char** persistence_search_memories(const char* query, size_t max_results,
                                          float min_similarity, size_t* out_count);

// Agent state management
extern void agent_set_working(ManagedAgent* agent, AgentWorkState state, const char* task);
extern void agent_set_idle(ManagedAgent* agent);
extern void agent_set_collaborating(ManagedAgent* agent, SemanticID partner_id);
extern char* agent_get_working_status(void);

// Session management
static char* g_current_session_id = NULL;

// Helper to save conversation to both persistence and project history
static void save_conversation(const char* role, const char* content, const char* agent_name) {
    // Save to regular persistence
    if (g_current_session_id) {
        persistence_save_conversation(g_current_session_id, role, content, (int)strlen(content) / 4);
    }

    // Also save to project history if project is active
    ConvergioProject* proj = project_current();
    if (proj) {
        project_append_history(proj, role, content, agent_name);
    }
}

// Claude API (from neural/claude.c)
extern int nous_claude_init(void);
extern void nous_claude_shutdown(void);
extern char* nous_claude_chat(const char* system_prompt, const char* user_message);
extern char* nous_claude_chat_with_tools(const char* system_prompt, const char* user_message,
                                          const char* tools_json, char** out_tool_calls);

// Tools header
#include "nous/tools.h"

// Embedded agents
#include "nous/embedded_agents.h"

// ============================================================================
// ANTI-HALLUCINATION CONSTITUTION (MANDATORY FOR ALI AND ALL AGENTS)
// ============================================================================
// This constitution ensures brutal honesty and prevents hallucinations.
// It is prepended to ALL agent prompts, including Ali.

static const char* ALI_CONSTITUTION =
    "## MANDATORY CONSTITUTION (NON-NEGOTIABLE)\n\n"
    "**You are bound by this constitution. Violating it is unacceptable.**\n\n"
    "### Rule 1: ABSOLUTE HONESTY\n"
    "- NEVER fabricate, invent, or guess information\n"
    "- NEVER pretend to have done something you haven't done\n"
    "- NEVER claim capabilities you don't have\n"
    "- If you don't know something, say \"I don't know\"\n"
    "- If you're not 100% certain, explicitly state your uncertainty level\n\n"
    "### Rule 2: UNCERTAINTY DISCLOSURE\n"
    "- When uncertain, preface with: \"I'm not certain, but...\"\n"
    "- When making assumptions, clearly state: \"I'm assuming...\"\n"
    "- When guessing, say: \"This is my best guess...\"\n"
    "- Distinguish clearly between facts you know and inferences you make\n\n"
    "### Rule 3: SOURCE ATTRIBUTION\n"
    "- If you read it from a file, say so\n"
    "- If you searched for it, say so\n"
    "- If you're inferring it, say so\n"
    "- If it's from your training knowledge, acknowledge the cutoff date\n\n"
    "### Rule 4: ERROR ACKNOWLEDGMENT\n"
    "- If you make a mistake, immediately acknowledge it\n"
    "- Never double down on errors\n"
    "- Correct yourself promptly when wrong\n\n"
    "### Rule 5: LIMITATION TRANSPARENCY\n"
    "- State clearly what you cannot do\n"
    "- Don't overpromise capabilities\n"
    "- Recommend external resources when you've reached your limits\n\n"
    "**END OF CONSTITUTION - Your specific role follows below:**\n\n";

// ============================================================================
// DYNAMIC AGENT LIST LOADER (using embedded agents)
// ============================================================================

// Helper to parse YAML frontmatter from embedded content
static void parse_agent_frontmatter(const char* content, char* name, size_t name_size,
                                     char* description, size_t desc_size) {
    name[0] = '\0';
    description[0] = '\0';

    const char* ptr = content;
    bool in_frontmatter = false;

    while (*ptr) {
        // Find end of line
        const char* eol = strchr(ptr, '\n');
        if (!eol) eol = ptr + strlen(ptr);
        size_t line_len = eol - ptr;

        // Check for frontmatter delimiter
        if (line_len >= 3 && strncmp(ptr, "---", 3) == 0) {
            if (in_frontmatter) break;  // End of frontmatter
            in_frontmatter = true;
            ptr = (*eol) ? eol + 1 : eol;
            continue;
        }

        if (in_frontmatter && line_len > 0) {
            if (strncmp(ptr, "name:", 5) == 0) {
                const char* val = ptr + 5;
                while (*val == ' ') val++;
                size_t val_len = eol - val;
                if (val_len >= name_size) val_len = name_size - 1;
                strncpy(name, val, val_len);
                name[val_len] = '\0';
            } else if (strncmp(ptr, "description:", 12) == 0) {
                const char* val = ptr + 12;
                while (*val == ' ') val++;
                size_t val_len = eol - val;
                if (val_len >= desc_size) val_len = desc_size - 1;
                strncpy(description, val, val_len);
                description[val_len] = '\0';
            }
        }

        ptr = (*eol) ? eol + 1 : eol;
    }
}

// Check if agent is in current project team (returns true if no project active)
static bool agent_in_project_team(const char* agent_name) {
    ConvergioProject* proj = project_current();
    if (!proj) return true;  // No project = all agents available
    return project_has_agent(agent_name);
}

// Load all agent definitions and build a list for the system prompt
// Filters by current project team if a project is active
static char* load_agent_list(void) {
    size_t agent_count = 0;
    const EmbeddedAgent* agents = get_all_embedded_agents(&agent_count);

    if (!agents || agent_count == 0) {
        return strdup("No agents found.");
    }

    // Check if we have an active project
    ConvergioProject* current_project = project_current();
    bool filtering = (current_project != NULL);

    size_t capacity = 8192;
    char* list = malloc(capacity);
    list[0] = '\0';
    size_t len = 0;

    // If filtering by project, add a header
    if (filtering) {
        len += snprintf(list + len, capacity - len,
            "**Project Team: %s** (%zu members)\n\n",
            current_project->name, current_project->team_count);
    }

    int included_count = 0;
    for (size_t i = 0; i < agent_count; i++) {
        const EmbeddedAgent* agent = &agents[i];

        // Skip non-agent files
        if (strstr(agent->filename, "CommonValues")) continue;
        if (strstr(agent->filename, "ali-chief")) continue;  // Skip Ali himself

        char name[256] = "";
        char description[512] = "";
        parse_agent_frontmatter(agent->content, name, sizeof(name),
                                description, sizeof(description));

        if (name[0] && description[0]) {
            // Extract short name (first part before -)
            char short_name[64];
            strncpy(short_name, name, sizeof(short_name) - 1);
            short_name[sizeof(short_name) - 1] = '\0';
            char* dash = strchr(short_name, '-');
            if (dash) *dash = '\0';

            // Filter by project team if active
            if (filtering && !agent_in_project_team(short_name)) {
                continue;  // Skip agents not in project team
            }

            // Capitalize first letter
            if (short_name[0] >= 'a' && short_name[0] <= 'z') {
                short_name[0] -= 32;
            }

            // Truncate description if too long
            if (strlen(description) > 80) {
                description[77] = '.';
                description[78] = '.';
                description[79] = '.';
                description[80] = '\0';
            }

            size_t needed = len + strlen(short_name) + strlen(description) + 32;
            if (needed > capacity) {
                capacity = needed * 2;
                list = realloc(list, capacity);
            }

            len += snprintf(list + len, capacity - len,
                "- **%s**: %s\n", short_name, description);
            included_count++;
        }
    }

    // Add note if filtering limited agents
    if (filtering && included_count < (int)agent_count / 2) {
        size_t needed = len + 128;
        if (needed > capacity) {
            capacity = needed * 2;
            list = realloc(list, capacity);
        }
        len += snprintf(list + len, capacity - len,
            "\n_Note: Other agents available via `project clear`_\n");
    }

    return list;
}

// ============================================================================
// ALI'S SYSTEM PROMPT
// ============================================================================

static const char* ALI_SYSTEM_PROMPT_TEMPLATE =
    "You are Ali, the Chief of Staff and master orchestrator for the Convergio ecosystem.\n\n"
    "## System Information\n"
    "- **Current date**: %s\n"
    "- **Convergio version**: %s\n"
    "- **Your model**: Claude Sonnet 4 (claude-sonnet-4-20250514)\n"
    "- **Available agents**: %d specialists ready to assist\n\n"
    "## Working Directory\n"
    "**Current workspace**: `%s`\n"
    "All file operations and shell commands should use paths relative to this directory, or absolute paths within it.\n"
    "When the user references files without a full path, assume they are relative to this workspace.\n\n"
    "## Your Role\n"
    "You are the single point of contact for the user. You coordinate all specialist agents and use tools to deliver comprehensive solutions.\n"
    "You have MEMORY - you remember past conversations and can store important information for future reference.\n\n"
    "## Memory System\n"
    "You have access to:\n"
    "- **Conversation history**: Previous messages from this and past sessions are loaded automatically\n"
    "- **Important memories**: Key information is retrieved and shown in context\n"
    "- **Notes**: Persistent markdown notes you can create and reference\n"
    "- **Knowledge base**: A searchable repository of documents and information\n\n"
    "When you learn something important about the user (preferences, projects, context), store it using memory_store or note_write.\n\n"
    "## Tools Available\n"
    "### File & System Tools\n"
    "- **file_read**: Read file contents from the filesystem\n"
    "- **file_write**: Write content to files (create or modify)\n"
    "- **file_list**: List directory contents\n"
    "- **shell_exec**: Execute shell commands (with safety restrictions)\n"
    "- **web_fetch**: Fetch content from URLs\n\n"
    "### Memory Tools\n"
    "- **memory_store**: Store information in semantic memory (with importance 0.0-1.0)\n"
    "- **memory_search**: Search stored memories by natural language query\n\n"
    "### Note Tools (for persistent markdown notes)\n"
    "- **note_write**: Create/update a markdown note with title, content, and tags\n"
    "- **note_read**: Read a note by title or search notes by content\n"
    "- **note_list**: List all notes, optionally filtered by tag\n\n"
    "### Knowledge Base Tools\n"
    "- **knowledge_search**: Search the knowledge base for relevant documents\n"
    "- **knowledge_add**: Add a new document to the knowledge base (with optional category)\n\n"
    "## When to Use Memory\n"
    "- User tells you their name, preferences, or context -> memory_store with high importance\n"
    "- User starts a project or gives you ongoing context -> note_write with relevant tags\n"
    "- You learn facts that will be useful later -> memory_store or knowledge_add\n"
    "- User asks 'do you remember...' -> memory_search and note_read\n\n"
    "## Specialist Agents & Multi-Agent Orchestration\n"
    "You can delegate to specialist agents. The system supports PARALLEL execution.\n\n"
    "### Single Agent Delegation\n"
    "Use: [DELEGATE: agent_name] reason/context\n\n"
    "### Multiple Agents (Parallel)\n"
    "To query multiple agents simultaneously, list them all:\n"
    "[DELEGATE: Baccio] technical architecture review\n"
    "[DELEGATE: Luca] security assessment\n"
    "[DELEGATE: Thor] quality validation\n\n"
    "All agents execute IN PARALLEL and their responses are automatically converged.\n"
    "Use multiple agents when you need diverse perspectives or comprehensive analysis.\n\n"
    "### Available Agents:\n"
    "%s\n"
    "## CRITICAL: MANDATORY TOOL USAGE\n"
    "**When the user asks you to perform an action, you MUST use the appropriate tool:**\n"
    "- Create/write/modify files → MUST call `file_write`\n"
    "- Read file contents → MUST call `file_read`\n"
    "- Execute shell commands → MUST call `shell_exec`\n"
    "- Fetch web content → MUST call `web_fetch`\n"
    "- Check files/directories → MUST call `file_list`\n\n"
    "**VIOLATIONS ARE UNACCEPTABLE:**\n"
    "- NEVER say 'I created the file' without calling `file_write`\n"
    "- NEVER report file contents without calling `file_read`\n"
    "- NEVER claim a command was executed without calling `shell_exec`\n"
    "- If a tool fails, report the ACTUAL error - do not claim success\n\n"
    "## Response Guidelines\n"
    "1. Be concise but comprehensive\n"
    "2. Use memory tools proactively to store and retrieve relevant context\n"
    "3. Reference past conversations naturally when relevant\n"
    "4. Delegate to specialists for deep analysis\n"
    "5. Always synthesize insights into actionable recommendations\n"
    "6. Be honest about limitations and uncertainties\n\n"
    "## Output Format\n"
    "IMPORTANT: Never show technical details of tool calls in your response.\n"
    "Do NOT output XML, function_calls, invoke tags, or raw tool results.\n"
    "Instead, silently use tools and present only the final, user-friendly result.\n"
    "Format your response with clean markdown: headers, bullet points, code blocks.\n"
    "The user should see polished output, not implementation details.\n\n"
    "## Delegation Format\n"
    "When you need a specialist, respond with:\n"
    "[DELEGATE: agent_name] reason for delegation\n\n"
    "The system will automatically route to that agent and you will synthesize their response.";

// ============================================================================
// INITIALIZATION
// ============================================================================

int orchestrator_init(double budget_limit_usd) {
    CONVERGIO_MUTEX_LOCK(&g_orch_mutex);

    if (g_orchestrator != NULL) {
        CONVERGIO_MUTEX_UNLOCK(&g_orch_mutex);
        return 0;  // Already initialized
    }

    // Allocate orchestrator
    g_orchestrator = calloc(1, sizeof(Orchestrator));
    if (!g_orchestrator) {
        CONVERGIO_MUTEX_UNLOCK(&g_orch_mutex);
        return -1;
    }

    // Initialize agent pool
    g_orchestrator->agent_capacity = 64;
    g_orchestrator->agents = calloc(g_orchestrator->agent_capacity, sizeof(ManagedAgent*));
    if (!g_orchestrator->agents) {
        free(g_orchestrator);
        g_orchestrator = NULL;
        CONVERGIO_MUTEX_UNLOCK(&g_orch_mutex);
        return -1;
    }

    // Initialize cost controller
    g_orchestrator->cost.budget_limit_usd = budget_limit_usd;
    g_orchestrator->cost.session_start = time(NULL);

    // Initialize subsystems
    if (persistence_init(NULL) != 0) {
        LOG_WARN(LOG_CAT_SYSTEM, "persistence init failed, continuing without DB");
    }

    if (msgbus_init() != 0) {
        LOG_WARN(LOG_CAT_SYSTEM, "message bus init failed");
    }

    if (nous_claude_init() != 0) {
        LOG_WARN(LOG_CAT_API, "Claude API init failed");
    }

    // Create Ali - the chief of staff
    g_orchestrator->ali = calloc(1, sizeof(ManagedAgent));
    if (g_orchestrator->ali) {
        g_orchestrator->ali->id = 1;
        g_orchestrator->ali->name = strdup("Ali");
        if (!g_orchestrator->ali->name) {
            free(g_orchestrator->ali);
            g_orchestrator->ali = NULL;
        } else {
            g_orchestrator->ali->role = AGENT_ROLE_ORCHESTRATOR;

            // Build system prompt with date, version, workspace, and dynamic agent list
            char* agent_list = load_agent_list();
            const char* workspace = tools_get_workspace();
            if (!workspace) workspace = ".";  // Fallback to current directory

            // Get current date
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char date_str[32];
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);

            // Get version
            const char* version = convergio_get_version();

            // Count agents (will be set properly after all agents are loaded)
            int agent_count = 48;  // Approximate count

            // CRITICAL: Include the anti-hallucination constitution in Ali's prompt
            size_t constitution_len = strlen(ALI_CONSTITUTION);
            size_t prompt_size = constitution_len + strlen(ALI_SYSTEM_PROMPT_TEMPLATE) + strlen(workspace) + strlen(agent_list) + strlen(date_str) + strlen(version) + 512;
            char* full_prompt = malloc(prompt_size);
            if (!full_prompt) {
                free(g_orchestrator->ali->name);
                free(g_orchestrator->ali);
                g_orchestrator->ali = NULL;
                free(agent_list);
            } else {
                // Prepend constitution, then add the role-specific prompt
                memcpy(full_prompt, ALI_CONSTITUTION, constitution_len);
                snprintf(full_prompt + constitution_len, prompt_size - constitution_len, ALI_SYSTEM_PROMPT_TEMPLATE, date_str, version, agent_count, workspace, agent_list);
                g_orchestrator->ali->system_prompt = full_prompt;
                free(agent_list);

                g_orchestrator->ali->is_active = true;
                g_orchestrator->ali->created_at = time(NULL);

                // Add to agent pool
                g_orchestrator->agents[g_orchestrator->agent_count++] = g_orchestrator->ali;
            }
        }
    }

    // Initialize hash tables for O(1) agent lookup
    g_orchestrator->agent_by_id = agent_hash_create();
    g_orchestrator->agent_by_name = agent_hash_create();
    g_orchestrator->message_pool = message_pool_create();

    // Add Ali to hash tables
    if (g_orchestrator->ali && g_orchestrator->agent_by_id) {
        agent_hash_insert_by_id(g_orchestrator->agent_by_id, g_orchestrator->ali);
        agent_hash_insert_by_name(g_orchestrator->agent_by_name, g_orchestrator->ali);
    }

    // Load all agent definitions from embedded data
    int loaded = agent_load_definitions(NULL);  // NULL = use embedded agents
    if (loaded > 0) {
        // Agents loaded successfully
    }

    // Create or resume session
    g_current_session_id = persistence_get_or_create_session();
    if (g_current_session_id) {
        // Load conversation context from this session
        char* context = persistence_load_conversation_context(g_current_session_id, 20);
        if (context) {
            // Store as specialized context for Ali
            if (g_orchestrator->ali) {
                g_orchestrator->ali->specialized_context = context;
            } else {
                free(context);
            }
        }
    }

    g_orchestrator->initialized = true;

    // Load cumulative cost history from database
    cost_load_historical();

    CONVERGIO_MUTEX_UNLOCK(&g_orch_mutex);

    return 0;
}

void orchestrator_shutdown(void) {
    CONVERGIO_MUTEX_LOCK(&g_orch_mutex);

    if (!g_orchestrator) {
        CONVERGIO_MUTEX_UNLOCK(&g_orch_mutex);
        return;
    }

    // Shutdown subsystems
    persistence_shutdown();
    msgbus_shutdown();
    nous_claude_shutdown();

    // Free hash tables and message pool
    if (g_orchestrator->agent_by_id) {
        agent_hash_destroy(g_orchestrator->agent_by_id);
    }
    if (g_orchestrator->agent_by_name) {
        agent_hash_destroy(g_orchestrator->agent_by_name);
    }
    if (g_orchestrator->message_pool) {
        message_pool_destroy(g_orchestrator->message_pool);
    }

    // Free agents
    for (size_t i = 0; i < g_orchestrator->agent_count; i++) {
        ManagedAgent* agent = g_orchestrator->agents[i];
        if (agent) {
            free(agent->name);
            free(agent->system_prompt);
            free(agent->specialized_context);
            // Free pending messages
            Message* msg = agent->pending_messages;
            while (msg) {
                Message* next = msg->next;
                free(msg->content);
                free(msg->metadata_json);
                free(msg);
                msg = next;
            }
            free(agent);
        }
    }
    free(g_orchestrator->agents);

    // Free message history
    Message* msg = g_orchestrator->message_history;
    while (msg) {
        Message* next = msg->next;
        free(msg->content);
        free(msg->metadata_json);
        free(msg);
        msg = next;
    }

    // Free execution plan
    if (g_orchestrator->current_plan) {
        free(g_orchestrator->current_plan->goal);
        free(g_orchestrator->current_plan->final_result);
        // Free tasks...
        free(g_orchestrator->current_plan);
    }

    free(g_orchestrator->user_name);
    free(g_orchestrator->user_preferences);
    free(g_orchestrator);
    g_orchestrator = NULL;

    CONVERGIO_MUTEX_UNLOCK(&g_orch_mutex);
}

Orchestrator* orchestrator_get(void) {
    return g_orchestrator;
}

// ============================================================================
// NOTE: Task planning moved to planning.c
// NOTE: Agent delegation moved to delegation.c
// NOTE: Convergence logic moved to convergence.c
// ============================================================================

// ============================================================================
// TOOL EXECUTION HELPERS
// ============================================================================

// Parse tool name from JSON content block
static char* parse_tool_name_from_block(const char* block) {
    const char* name_key = "\"name\"";
    const char* pos = strstr(block, name_key);
    if (!pos) return NULL;

    pos = strchr(pos, ':');
    if (!pos) return NULL;
    pos++;

    while (*pos == ' ' || *pos == '"') pos++;

    const char* end = pos;
    while (*end && *end != '"' && *end != ',' && *end != '}') end++;

    size_t len = end - pos;
    char* name = malloc(len + 1);
    strncpy(name, pos, len);
    name[len] = '\0';

    return name;
}

// Parse tool input JSON from content block
static char* parse_tool_input_from_block(const char* block) {
    const char* input_key = "\"input\"";
    const char* pos = strstr(block, input_key);
    if (!pos) return NULL;

    pos = strchr(pos, ':');
    if (!pos) return NULL;
    pos++;

    while (*pos == ' ') pos++;

    if (*pos != '{') return NULL;

    // Find matching brace
    int depth = 1;
    const char* start = pos;
    pos++;

    while (*pos && depth > 0) {
        if (*pos == '{') depth++;
        else if (*pos == '}') depth--;
        pos++;
    }

    size_t len = pos - start;
    char* input = malloc(len + 1);
    strncpy(input, start, len);
    input[len] = '\0';

    return input;
}

// Parse tool_use id from content block
static char* parse_tool_id_from_block(const char* block) {
    const char* id_key = "\"id\"";
    const char* pos = strstr(block, id_key);
    if (!pos) return NULL;

    pos = strchr(pos, ':');
    if (!pos) return NULL;
    pos++;

    while (*pos == ' ' || *pos == '"') pos++;

    const char* end = pos;
    while (*end && *end != '"' && *end != ',' && *end != '}') end++;

    size_t len = end - pos;
    char* id = malloc(len + 1);
    strncpy(id, pos, len);
    id[len] = '\0';

    return id;
}

// Execute a tool and return result
static char* execute_tool_call(const char* tool_name, const char* tool_input) {
    LocalToolCall* call = tools_parse_call(tool_name, tool_input);
    if (!call) {
        return strdup("Error: Failed to parse tool call");
    }

    ToolResult* result = tools_execute(call);
    tools_free_call(call);

    if (!result) {
        return strdup("Error: Tool execution failed");
    }

    char* output;
    if (result->success) {
        output = strdup(result->output ? result->output : "Success (no output)");
    } else {
        size_t len = strlen(result->error ? result->error : "Unknown error") + 32;
        output = malloc(len);
        snprintf(output, len, "Error: %s", result->error ? result->error : "Unknown error");
    }

    tools_free_result(result);
    return output;
}

// ============================================================================
// MAIN PROCESSING WITH TOOL LOOP
// ============================================================================

#define MAX_TOOL_ITERATIONS 10

// Build context from memories and conversation history
static char* build_context_prompt(const char* user_input) {
    size_t capacity = 65536;
    char* context = malloc(capacity);
    if (!context) return NULL;
    context[0] = '\0';
    size_t len = 0;

    // 0. Add project context if active
    ConvergioProject* proj = project_current();
    if (proj) {
        len += snprintf(context + len, capacity - len,
            "## Active Project: %s\n", proj->name);
        if (proj->purpose) {
            len += snprintf(context + len, capacity - len,
                "**Purpose**: %s\n", proj->purpose);
        }
        if (proj->current_focus) {
            len += snprintf(context + len, capacity - len,
                "**Current Focus**: %s\n", proj->current_focus);
        }
        len += snprintf(context + len, capacity - len, "**Team**: ");
        for (size_t i = 0; i < proj->team_count; i++) {
            len += snprintf(context + len, capacity - len, "%s%s",
                proj->team[i].agent_name,
                i < proj->team_count - 1 ? ", " : "");
        }
        len += snprintf(context + len, capacity - len, "\n");
        if (proj->decision_count > 0) {
            len += snprintf(context + len, capacity - len, "**Key Decisions**:\n");
            for (size_t i = 0; i < proj->decision_count && i < 5; i++) {
                len += snprintf(context + len, capacity - len, "- %s\n", proj->key_decisions[i]);
            }
        }
        len += snprintf(context + len, capacity - len,
            "\n**Note**: Only delegate to team members listed above.\n\n");
    }

    // 1. Load important memories
    size_t mem_count = 0;
    char** memories = persistence_get_important_memories(5, &mem_count);
    if (memories && mem_count > 0) {
        len += snprintf(context + len, capacity - len,
            "## Important Memories\n");
        for (size_t i = 0; i < mem_count; i++) {
            if (memories[i]) {
                len += snprintf(context + len, capacity - len, "- %s\n", memories[i]);
                free(memories[i]);
            }
        }
        free(memories);
        len += snprintf(context + len, capacity - len, "\n");
    }

    // 2. Search for relevant memories based on user input
    size_t rel_count = 0;
    char** relevant = persistence_search_memories(user_input, 3, 0.3f, &rel_count);
    if (relevant && rel_count > 0) {
        len += snprintf(context + len, capacity - len,
            "## Relevant Context\n");
        for (size_t i = 0; i < rel_count; i++) {
            if (relevant[i]) {
                len += snprintf(context + len, capacity - len, "- %s\n", relevant[i]);
                free(relevant[i]);
            }
        }
        free(relevant);
        len += snprintf(context + len, capacity - len, "\n");
    }

    // 3. Load recent conversation history from current session
    if (g_current_session_id) {
        char* conv_history = persistence_load_conversation_context(g_current_session_id, 10);
        if (conv_history) {
            len += snprintf(context + len, capacity - len,
                "## Recent Conversation (this session)\n%s\n", conv_history);
            free(conv_history);
        }
    }

    // 4. Add current user input
    len += snprintf(context + len, capacity - len,
        "## Current Request\n%s", user_input);

    return context;
}

char* orchestrator_process(const char* user_input) {
    if (!g_orchestrator || !g_orchestrator->initialized || !user_input) {
        return strdup("Error: Orchestrator not initialized");
    }

    // Check budget
    if (g_orchestrator->cost.budget_exceeded) {
        return strdup("Budget exceeded. Use 'cost set <amount>' to increase budget.");
    }

    // Save user message to persistence and project history
    save_conversation("user", user_input, NULL);

    // Create user message
    Message* user_msg = message_create(MSG_TYPE_USER_INPUT, 0, g_orchestrator->ali->id, user_input);
    if (user_msg) {
        message_send(user_msg);
    }

    // Get tools definition
    const char* tools_json = tools_get_definitions_json();

    // Build conversation with context
    char* conversation = build_context_prompt(user_input);
    if (!conversation) {
        return strdup("Error: Memory allocation failed");
    }
    size_t conv_capacity = strlen(conversation) + 32768;

    char* final_response = NULL;
    int iteration = 0;

    while (iteration < MAX_TOOL_ITERATIONS) {
        iteration++;

        // Call Claude with tools
        char* tool_calls_json = NULL;
        char* response = nous_claude_chat_with_tools(
            g_orchestrator->ali->system_prompt,
            conversation,
            tools_json,
            &tool_calls_json
        );

        if (!response && !tool_calls_json) {
            free(conversation);
            return strdup("Error: Failed to get response from Ali");
        }

        // Record cost
        cost_record_agent_usage(g_orchestrator->ali,
                                strlen(g_orchestrator->ali->system_prompt) / 4 + strlen(conversation) / 4,
                                (response ? strlen(response) : 0) / 4);

        // Check if there are tool calls to process
        if (tool_calls_json && strstr(tool_calls_json, "tool_use")) {
            // Parse and execute each tool call
            // Find tool_use blocks in the content array
            const char* search_pos = tool_calls_json;
            size_t tool_results_capacity = 16384;
            char* tool_results = malloc(tool_results_capacity);
            if (!tool_results) {
                free(tool_calls_json);
                free(conversation);
                if (response) free(response);
                return strdup("Error: Memory allocation failed");
            }
            tool_results[0] = '\0';
            size_t results_len = 0;
            int tool_count = 0;

            while ((search_pos = strstr(search_pos, "\"type\"")) != NULL) {
                // Check if this is a tool_use type
                if (strstr(search_pos, "\"tool_use\"") &&
                    (strstr(search_pos, "\"tool_use\"") - search_pos) < 50) {

                    // Find the enclosing object
                    // Go back to find the opening brace
                    const char* block_start = search_pos;
                    while (block_start > tool_calls_json && *block_start != '{') block_start--;

                    // Find closing brace
                    int depth = 1;
                    const char* block_end = block_start + 1;
                    while (*block_end && depth > 0) {
                        if (*block_end == '{') depth++;
                        else if (*block_end == '}') depth--;
                        block_end++;
                    }

                    // Extract the block
                    size_t block_len = block_end - block_start;
                    char* block = malloc(block_len + 1);
                    strncpy(block, block_start, block_len);
                    block[block_len] = '\0';

                    // Parse tool call from block
                    char* tool_name = parse_tool_name_from_block(block);
                    char* tool_input = parse_tool_input_from_block(block);
                    char* tool_id = parse_tool_id_from_block(block);

                    if (tool_name && tool_input) {
                        // Execute tool
                        char* tool_result = execute_tool_call(tool_name, tool_input);

                        // Append to results
                        char result_entry[8192];
                        snprintf(result_entry, sizeof(result_entry),
                            "\n\n[Tool: %s]\nResult: %s",
                            tool_name, tool_result);

                        size_t entry_len = strlen(result_entry);
                        if (results_len + entry_len + 1 < tool_results_capacity) {
                            memcpy(tool_results + results_len, result_entry, entry_len + 1);
                            results_len += entry_len;
                        }

                        tool_count++;
                        free(tool_result);
                    }

                    free(tool_name);
                    free(tool_input);
                    free(tool_id);
                    free(block);

                    search_pos = block_end;
                } else {
                    search_pos++;
                }
            }

            free(tool_calls_json);
            tool_calls_json = NULL;  // Prevent double-free

            if (tool_count > 0) {
                // Build new conversation with tool results
                size_t new_conv_len = strlen(conversation) + strlen(tool_results) + 256;
                if (new_conv_len > conv_capacity) {
                    conv_capacity = new_conv_len + 4096;
                    conversation = realloc(conversation, conv_capacity);
                }

                // Append tool results and ask for final response using snprintf
                size_t conv_len = strlen(conversation);
                size_t append_len = snprintf(conversation + conv_len, conv_capacity - conv_len,
                    "\n\n[Tool Results]%s\n\nBased on these tool results, provide your response to the user.",
                    tool_results);
                (void)append_len;  // Suppress unused warning

                free(tool_results);
                free(response);

                // Continue loop to get response with tool results
                continue;
            }

            free(tool_results);
        }

        // No more tool calls, we have the final response
        if (response) {
            final_response = response;
        }
        free(tool_calls_json);
        break;
    }

    free(conversation);

    if (!final_response) {
        return strdup("Error: No response generated");
    }

    // Check for delegation requests in final response (supports multiple)
    DelegationList* delegations = parse_all_delegations(final_response);
    if (delegations && delegations->count > 0) {
        // Execute all delegations in parallel and get synthesized result
        char* synthesized = execute_delegations(delegations, user_input, final_response, g_orchestrator->ali);
        free_delegation_list(delegations);
        free(final_response);

        if (synthesized) {
            // Save synthesized response
            if (g_current_session_id) {
                persistence_save_conversation(g_current_session_id, "assistant", synthesized,
                                               (int)strlen(synthesized) / 4);
            }

            Message* response_msg = message_create(MSG_TYPE_AGENT_RESPONSE,
                                                    g_orchestrator->ali->id, 0, synthesized);
            if (response_msg) {
                message_send(response_msg);
            }

            return synthesized;
        }

        // If delegation failed, fall through to return original response
        // (already freed above, so this shouldn't happen - return error)
        return strdup("Error: Delegation failed");
    }

    // Save assistant response to persistence and project history
    if (final_response) {
        save_conversation("assistant", final_response, "Ali");
    }

    // Create response message
    Message* response_msg = message_create(MSG_TYPE_AGENT_RESPONSE,
                                            g_orchestrator->ali->id, 0, final_response);
    if (response_msg) {
        message_send(response_msg);
    }

    return final_response;
}

// External streaming function from claude.c
extern char* nous_claude_chat_stream(const char* system_prompt, const char* user_message,
                                      void (*callback)(const char*, void*), void* user_data);

// Streaming variant - uses callback for live output, no tool support
char* orchestrator_process_stream(const char* user_input, OrchestratorStreamCallback callback, void* user_data) {
    if (!g_orchestrator || !g_orchestrator->initialized || !user_input) {
        const char* err = "Error: Orchestrator not initialized";
        if (callback) callback(err, user_data);
        return strdup(err);
    }

    // Check budget
    if (g_orchestrator->cost.budget_exceeded) {
        const char* err = "Budget exceeded. Use 'cost set <amount>' to increase budget.";
        if (callback) callback(err, user_data);
        return strdup(err);
    }

    // Save user message to persistence and project history
    save_conversation("user", user_input, NULL);

    // Create user message
    Message* user_msg = message_create(MSG_TYPE_USER_INPUT, 0, g_orchestrator->ali->id, user_input);
    if (user_msg) {
        message_send(user_msg);
    }

    // Build conversation with context
    char* conversation = build_context_prompt(user_input);
    if (!conversation) {
        const char* err2 = "Error: Memory allocation failed";
        if (callback) callback(err2, user_data);
        return strdup(err2);
    }

    // Call Claude with streaming - no tools in streaming mode
    char* response = nous_claude_chat_stream(
        g_orchestrator->ali->system_prompt,
        conversation,
        callback,
        user_data
    );

    free(conversation);

    // Track costs (estimate based on input/output length)
    if (response) {
        size_t input_tokens = strlen(g_orchestrator->ali->system_prompt) / 4 + strlen(user_input) / 4;
        size_t output_tokens = strlen(response) / 4;
        cost_record_usage(input_tokens, output_tokens);
        cost_record_agent_usage(g_orchestrator->ali, input_tokens, output_tokens);

        // Save response to persistence and project history
        save_conversation("assistant", response, "Ali");

        // Create response message
        Message* response_msg = message_create(MSG_TYPE_AGENT_RESPONSE,
                                                g_orchestrator->ali->id, 0, response);
        if (response_msg) {
            message_send(response_msg);
        }
    }

    return response;
}

// ============================================================================
// NOTE: Convergence logic moved to convergence.c
// ============================================================================

// ============================================================================
// DIRECT AGENT COMMUNICATION (with tools support)
// ============================================================================

// Tools instructions to append to agent system prompts
static const char* AGENT_TOOLS_INSTRUCTIONS =
    "\n\n## CRITICAL: MANDATORY TOOL USAGE\n"
    "**When asked to perform an action, you MUST use the appropriate tool:**\n"
    "- Create/write/modify files → MUST call `file_write`\n"
    "- Read file contents → MUST call `file_read`\n"
    "- Execute shell commands → MUST call `shell_exec`\n"
    "- Fetch web content → MUST call `web_fetch`\n"
    "- Check files/directories → MUST call `file_list`\n\n"
    "**VIOLATIONS ARE UNACCEPTABLE:**\n"
    "- NEVER say 'I created the file' without calling `file_write`\n"
    "- NEVER report file contents without calling `file_read`\n"
    "- NEVER claim a command was executed without calling `shell_exec`\n"
    "- If a tool fails, report the ACTUAL error - do not claim success\n\n"
    "## Tools Available\n"
    "- **file_read**: Read file contents from the filesystem\n"
    "- **file_write**: Write content to files (create or modify)\n"
    "- **file_list**: List directory contents\n"
    "- **shell_exec**: Execute shell commands\n"
    "- **web_fetch**: Fetch content from URLs (for research)\n"
    "- **memory_store**: Store information in semantic memory\n"
    "- **memory_search**: Search stored memories\n";

// Chat directly with a specific agent, with full tool support
char* orchestrator_agent_chat(ManagedAgent* agent, const char* user_message) {
    if (!g_orchestrator || !agent || !user_message) return NULL;

    const char* tools_json = tools_get_definitions_json();

    // Build enhanced system prompt with tools instructions
    size_t prompt_len = strlen(agent->system_prompt) + strlen(AGENT_TOOLS_INSTRUCTIONS) + 1;
    char* enhanced_prompt = malloc(prompt_len);
    if (!enhanced_prompt) return NULL;
    snprintf(enhanced_prompt, prompt_len, "%s%s", agent->system_prompt, AGENT_TOOLS_INSTRUCTIONS);

    // Build conversation
    size_t conv_capacity = strlen(user_message) + 4096;
    char* conversation = malloc(conv_capacity);
    if (!conversation) {
        free(enhanced_prompt);
        return NULL;
    }
    snprintf(conversation, conv_capacity, "%s", user_message);

    char* final_response = NULL;
    int max_iterations = 5;  // Max tool loop iterations
    int iteration = 0;

    while (iteration < max_iterations) {
        iteration++;

        // Check if cancelled
        if (claude_is_cancelled()) {
            free(conversation);
            free(enhanced_prompt);
            return NULL;
        }

        // Call Claude with tools (using enhanced prompt with tools instructions)
        char* tool_calls_json = NULL;
        char* response = nous_claude_chat_with_tools(
            enhanced_prompt,
            conversation,
            tools_json,
            &tool_calls_json
        );

        if (!response && !tool_calls_json) {
            free(conversation);
            free(enhanced_prompt);
            return strdup("Error: Failed to get response from agent");
        }

        // Record cost
        cost_record_agent_usage(agent,
                                strlen(enhanced_prompt) / 4 + strlen(conversation) / 4,
                                (response ? strlen(response) : 0) / 4);

        // Check if there are tool calls to process
        if (tool_calls_json && strstr(tool_calls_json, "tool_use")) {
            // Parse and execute each tool call
            const char* search_pos = tool_calls_json;
            size_t tool_results_capacity = 16384;
            char* tool_results = malloc(tool_results_capacity);
            if (!tool_results) {
                free(tool_calls_json);
                free(conversation);
                free(enhanced_prompt);
                if (response) free(response);
                return strdup("Error: Memory allocation failed");
            }
            tool_results[0] = '\0';
            size_t results_len = 0;
            int tool_count = 0;

            while ((search_pos = strstr(search_pos, "\"type\"")) != NULL) {
                if (strstr(search_pos, "\"tool_use\"") &&
                    (strstr(search_pos, "\"tool_use\"") - search_pos) < 50) {

                    const char* block_start = search_pos;
                    while (block_start > tool_calls_json && *block_start != '{') block_start--;

                    int depth = 1;
                    const char* block_end = block_start + 1;
                    while (*block_end && depth > 0) {
                        if (*block_end == '{') depth++;
                        else if (*block_end == '}') depth--;
                        block_end++;
                    }

                    size_t block_len = block_end - block_start;
                    char* block = malloc(block_len + 1);
                    strncpy(block, block_start, block_len);
                    block[block_len] = '\0';

                    char* tool_name = parse_tool_name_from_block(block);
                    char* tool_input = parse_tool_input_from_block(block);

                    if (tool_name && tool_input) {
                        char* tool_result = execute_tool_call(tool_name, tool_input);

                        char result_entry[8192];
                        snprintf(result_entry, sizeof(result_entry),
                            "\n\n[Tool: %s]\nResult: %s",
                            tool_name, tool_result);

                        size_t entry_len = strlen(result_entry);
                        if (results_len + entry_len + 1 < tool_results_capacity) {
                            memcpy(tool_results + results_len, result_entry, entry_len + 1);
                            results_len += entry_len;
                        }

                        tool_count++;
                        free(tool_result);
                    }

                    free(tool_name);
                    free(tool_input);
                    free(block);
                    search_pos = block_end;
                } else {
                    search_pos++;
                }
            }

            free(tool_calls_json);
            tool_calls_json = NULL;

            if (tool_count > 0) {
                size_t new_conv_len = strlen(conversation) + strlen(tool_results) + 256;
                if (new_conv_len > conv_capacity) {
                    conv_capacity = new_conv_len + 4096;
                    conversation = realloc(conversation, conv_capacity);
                }

                size_t conv_len = strlen(conversation);
                snprintf(conversation + conv_len, conv_capacity - conv_len,
                    "\n\n[Tool Results]%s\n\nBased on these tool results, provide your response to the user.",
                    tool_results);

                free(tool_results);
                free(response);
                continue;  // Continue loop to get response with tool results
            }

            free(tool_results);
        }

        // No more tool calls, we have the final response
        if (response) {
            final_response = response;
        }
        free(tool_calls_json);
        break;
    }

    free(conversation);
    free(enhanced_prompt);

    if (!final_response) {
        return strdup("Error: No response generated");
    }

    return final_response;
}

// ============================================================================
// PARALLEL EXECUTION
// ============================================================================

typedef struct {
    ManagedAgent* agent;
    const char* input;
    char* output;
} ParallelTask;

// Execute task with multiple agents in parallel
char* orchestrator_parallel_analyze(const char* input, const char** agent_names, size_t agent_count) {
    if (!g_orchestrator || !input || !agent_names || agent_count == 0) {
        return NULL;
    }

    // Create execution plan
    ExecutionPlan* plan = orch_plan_create(input);
    if (!plan) return NULL;

    // Create dispatch group for parallel execution
    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

    ParallelTask* tasks = calloc(agent_count, sizeof(ParallelTask));
    if (!tasks) {
        free(plan);
        return NULL;
    }

    // Launch agents in parallel
    for (size_t i = 0; i < agent_count; i++) {
        ManagedAgent* agent = agent_find_by_name(agent_names[i]);
        if (!agent) {
            agent = agent_spawn(AGENT_ROLE_ANALYST, agent_names[i], NULL);
        }

        if (agent) {
            tasks[i].agent = agent;
            tasks[i].input = input;
            tasks[i].output = NULL;

            dispatch_group_async(group, queue, ^{
                char* response = nous_claude_chat(tasks[i].agent->system_prompt, tasks[i].input);
                tasks[i].output = response;
                if (response) {
                    // Create task record
                    Task* t = orch_task_create(tasks[i].input, tasks[i].agent->id);
                    if (t) {
                        orch_task_complete(t, response);
                        orch_plan_add_task(plan, t);
                    }
                    cost_record_agent_usage(tasks[i].agent, 500, strlen(response) / 3);
                }
            });
        }
    }

    // Wait for all to complete
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

    // Converge results
    char* result = orchestrator_converge(plan);

    // Cleanup
    for (size_t i = 0; i < agent_count; i++) {
        free(tasks[i].output);
    }
    free(tasks);

    // Free the execution plan (including all tasks as linked list)
    if (plan) {
        Task* task = plan->tasks;
        while (task) {
            Task* next = task->next;
            free(task->description);
            free(task->result);
            free(task);
            task = next;
        }
        free(plan->goal);
        free(plan->final_result);
        free(plan);
    }

    return result;
}

// ============================================================================
// USER MANAGEMENT
// ============================================================================

void orchestrator_set_user(const char* name, const char* preferences) {
    if (!g_orchestrator) return;

    CONVERGIO_MUTEX_LOCK(&g_orch_mutex);

    free(g_orchestrator->user_name);
    g_orchestrator->user_name = name ? strdup(name) : NULL;

    free(g_orchestrator->user_preferences);
    g_orchestrator->user_preferences = preferences ? strdup(preferences) : NULL;

    CONVERGIO_MUTEX_UNLOCK(&g_orch_mutex);
}

// ============================================================================
// STATUS
// ============================================================================

char* orchestrator_status(void) {
    if (!g_orchestrator) return strdup("Orchestrator not initialized");

    char* status = malloc(4096);
    if (!status) return NULL;

    char* cost_line = cost_get_status_line();

    snprintf(status, 4096,
        "╔═══════════════════════════════════════════════════════════════╗\n"
        "║                 CONVERGIO ORCHESTRATOR                        ║\n"
        "╠═══════════════════════════════════════════════════════════════╣\n"
        "║ Chief of Staff: Ali %s                                        \n"
        "║ Active Agents:  %zu                                           \n"
        "║ Messages:       %zu                                           \n"
        "║ Cost:           %s                                            \n"
        "╚═══════════════════════════════════════════════════════════════╝\n",
        g_orchestrator->ali && g_orchestrator->ali->is_active ? "[ACTIVE]" : "[INACTIVE]",
        g_orchestrator->agent_count,
        g_orchestrator->message_count,
        cost_line ? cost_line : "N/A"
    );

    free(cost_line);

    return status;
}
