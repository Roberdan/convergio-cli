/**
 * CONVERGIO ORCHESTRATOR
 *
 * The heart of the system - Ali coordinates everything:
 * - User input processing
 * - Agent delegation
 * - Task planning
 * - Convergence
 * - Cost management
 */

#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dispatch/dispatch.h>
#include <dirent.h>
#include <limits.h>

// Global orchestrator instance
static Orchestrator* g_orchestrator = NULL;
static pthread_mutex_t g_orch_mutex = PTHREAD_MUTEX_INITIALIZER;

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

// Load all agent definitions and build a list for the system prompt
static char* load_agent_list(void) {
    size_t agent_count = 0;
    const EmbeddedAgent* agents = get_all_embedded_agents(&agent_count);

    if (!agents || agent_count == 0) {
        return strdup("No agents found.");
    }

    size_t capacity = 8192;
    char* list = malloc(capacity);
    list[0] = '\0';
    size_t len = 0;

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
        }
    }

    return list;
}

// ============================================================================
// ALI'S SYSTEM PROMPT
// ============================================================================

static const char* ALI_SYSTEM_PROMPT_TEMPLATE =
    "You are Ali, the Chief of Staff and master orchestrator for the Convergio ecosystem.\n\n"
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
    "## Response Guidelines\n"
    "1. Be concise but comprehensive\n"
    "2. Use memory tools proactively to store and retrieve relevant context\n"
    "3. Reference past conversations naturally when relevant\n"
    "4. Use tools when the task requires file access, command execution, or web fetching\n"
    "5. Delegate to specialists for deep analysis\n"
    "6. Always synthesize insights into actionable recommendations\n"
    "7. Be honest about limitations and uncertainties\n\n"
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
    pthread_mutex_lock(&g_orch_mutex);

    if (g_orchestrator != NULL) {
        pthread_mutex_unlock(&g_orch_mutex);
        return 0;  // Already initialized
    }

    // Allocate orchestrator
    g_orchestrator = calloc(1, sizeof(Orchestrator));
    if (!g_orchestrator) {
        pthread_mutex_unlock(&g_orch_mutex);
        return -1;
    }

    // Initialize agent pool
    g_orchestrator->agent_capacity = 64;
    g_orchestrator->agents = calloc(g_orchestrator->agent_capacity, sizeof(ManagedAgent*));
    if (!g_orchestrator->agents) {
        free(g_orchestrator);
        g_orchestrator = NULL;
        pthread_mutex_unlock(&g_orch_mutex);
        return -1;
    }

    // Initialize cost controller
    g_orchestrator->cost.budget_limit_usd = budget_limit_usd;
    g_orchestrator->cost.session_start = time(NULL);

    // Initialize subsystems
    if (persistence_init(NULL) != 0) {
        fprintf(stderr, "Warning: persistence init failed, continuing without DB\n");
    }

    if (msgbus_init() != 0) {
        fprintf(stderr, "Warning: message bus init failed\n");
    }

    if (nous_claude_init() != 0) {
        fprintf(stderr, "Warning: Claude API init failed\n");
    }

    // Create Ali - the chief of staff
    g_orchestrator->ali = calloc(1, sizeof(ManagedAgent));
    if (g_orchestrator->ali) {
        g_orchestrator->ali->id = 1;
        g_orchestrator->ali->name = strdup("Ali");
        g_orchestrator->ali->role = AGENT_ROLE_ORCHESTRATOR;

        // Build system prompt with dynamic agent list
        char* agent_list = load_agent_list();
        size_t prompt_size = strlen(ALI_SYSTEM_PROMPT_TEMPLATE) + strlen(agent_list) + 256;
        char* full_prompt = malloc(prompt_size);
        snprintf(full_prompt, prompt_size, ALI_SYSTEM_PROMPT_TEMPLATE, agent_list);
        g_orchestrator->ali->system_prompt = full_prompt;
        free(agent_list);

        g_orchestrator->ali->is_active = true;
        g_orchestrator->ali->created_at = time(NULL);

        // Add to agent pool
        g_orchestrator->agents[g_orchestrator->agent_count++] = g_orchestrator->ali;
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

    pthread_mutex_unlock(&g_orch_mutex);

    return 0;
}

void orchestrator_shutdown(void) {
    pthread_mutex_lock(&g_orch_mutex);

    if (!g_orchestrator) {
        pthread_mutex_unlock(&g_orch_mutex);
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

    pthread_mutex_unlock(&g_orch_mutex);
}

Orchestrator* orchestrator_get(void) {
    return g_orchestrator;
}

// ============================================================================
// TASK PLANNING
// ============================================================================

static uint64_t g_next_task_id = 1;
static uint64_t g_next_plan_id = 1;

ExecutionPlan* orch_plan_create(const char* goal) {
    ExecutionPlan* plan = calloc(1, sizeof(ExecutionPlan));
    if (!plan) return NULL;

    plan->id = __sync_fetch_and_add(&g_next_plan_id, 1);
    plan->goal = strdup(goal);
    plan->created_at = time(NULL);

    return plan;
}

Task* orch_task_create(const char* description, SemanticID assignee) {
    Task* task = calloc(1, sizeof(Task));
    if (!task) return NULL;

    task->id = __sync_fetch_and_add(&g_next_task_id, 1);
    task->description = strdup(description);
    task->assigned_to = assignee;
    task->status = TASK_STATUS_PENDING;
    task->created_at = time(NULL);

    return task;
}

void orch_plan_add_task(ExecutionPlan* plan, Task* task) {
    if (!plan || !task) return;

    // Add to linked list
    task->next = plan->tasks;
    plan->tasks = task;
}

void orch_task_complete(Task* task, const char* result) {
    if (!task) return;

    task->status = TASK_STATUS_COMPLETED;
    task->result = result ? strdup(result) : NULL;
    task->completed_at = time(NULL);
}

// ============================================================================
// AGENT DELEGATION
// ============================================================================

// Parse Ali's response for delegation requests (supports multiple)
typedef struct {
    char* agent_name;
    char* reason;
} DelegationRequest;

typedef struct {
    DelegationRequest** requests;
    size_t count;
    size_t capacity;
} DelegationList;

// Parse ALL delegation requests from response
static DelegationList* parse_all_delegations(const char* response) {
    DelegationList* list = calloc(1, sizeof(DelegationList));
    if (!list) return NULL;

    list->capacity = 16;
    list->requests = calloc(list->capacity, sizeof(DelegationRequest*));
    if (!list->requests) {
        free(list);
        return NULL;
    }

    const char* marker = "[DELEGATE:";
    const char* pos = response;

    while ((pos = strstr(pos, marker)) != NULL) {
        DelegationRequest* req = calloc(1, sizeof(DelegationRequest));
        if (!req) break;

        // Extract agent name
        pos += strlen(marker);
        while (*pos == ' ') pos++;

        const char* end = strchr(pos, ']');
        if (!end) {
            free(req);
            break;
        }

        size_t name_len = end - pos;
        req->agent_name = malloc(name_len + 1);
        if (req->agent_name) {
            strncpy(req->agent_name, pos, name_len);
            req->agent_name[name_len] = '\0';
            // Trim trailing spaces
            char* trim = req->agent_name + strlen(req->agent_name) - 1;
            while (trim > req->agent_name && *trim == ' ') *trim-- = '\0';
        }

        // Extract reason (until next [DELEGATE: or newline)
        pos = end + 1;
        while (*pos == ' ') pos++;

        const char* reason_end = strstr(pos, "[DELEGATE:");
        if (!reason_end) {
            // Find end of line or end of string
            reason_end = strchr(pos, '\n');
            if (!reason_end) reason_end = pos + strlen(pos);
        }

        if (reason_end > pos) {
            size_t reason_len = reason_end - pos;
            req->reason = malloc(reason_len + 1);
            if (req->reason) {
                strncpy(req->reason, pos, reason_len);
                req->reason[reason_len] = '\0';
                // Trim
                char* trim = req->reason + strlen(req->reason) - 1;
                while (trim > req->reason && (*trim == ' ' || *trim == '\n')) *trim-- = '\0';
            }
        }

        // Add to list
        if (list->count >= list->capacity) {
            list->capacity *= 2;
            list->requests = realloc(list->requests, list->capacity * sizeof(DelegationRequest*));
        }
        list->requests[list->count++] = req;

        pos = reason_end;
    }

    if (list->count == 0) {
        free(list->requests);
        free(list);
        return NULL;
    }

    return list;
}

static void free_delegation_list(DelegationList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        if (list->requests[i]) {
            free(list->requests[i]->agent_name);
            free(list->requests[i]->reason);
            free(list->requests[i]);
        }
    }
    free(list->requests);
    free(list);
}

// Structure for parallel agent execution
typedef struct {
    ManagedAgent* agent;
    const char* user_input;
    const char* context;
    char* response;
    bool completed;
} AgentTask;

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
    ToolCall* call = tools_parse_call(tool_name, tool_input);
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

    // Save user message to persistence
    if (g_current_session_id) {
        persistence_save_conversation(g_current_session_id, "user", user_input, (int)strlen(user_input) / 4);
    }

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
        // Prepare parallel execution
        AgentTask* tasks = calloc(delegations->count, sizeof(AgentTask));
        dispatch_group_t group = dispatch_group_create();
        dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

        // Spawn all agent tasks in parallel
        for (size_t i = 0; i < delegations->count; i++) {
            DelegationRequest* req = delegations->requests[i];

            // Find or spawn the requested agent
            ManagedAgent* specialist = agent_find_by_name(req->agent_name);
            if (!specialist) {
                specialist = agent_spawn(AGENT_ROLE_ANALYST, req->agent_name, NULL);
            }

            if (specialist && specialist->system_prompt) {
                tasks[i].agent = specialist;
                tasks[i].user_input = user_input;
                tasks[i].context = req->reason;
                tasks[i].response = NULL;
                tasks[i].completed = false;

                // Create delegation message
                Message* delegate_msg = message_create(MSG_TYPE_TASK_DELEGATE,
                                                        g_orchestrator->ali->id,
                                                        specialist->id,
                                                        user_input);
                if (delegate_msg) {
                    message_send(delegate_msg);
                }

                // Execute in parallel
                dispatch_group_async(group, queue, ^{
                    // Set agent as working
                    agent_set_working(tasks[i].agent, WORK_STATE_THINKING,
                                      tasks[i].context ? tasks[i].context : "Analyzing request");

                    size_t prompt_size = strlen(tasks[i].agent->system_prompt) +
                                         (tasks[i].context ? strlen(tasks[i].context) : 0) + 256;
                    char* prompt_with_context = malloc(prompt_size);
                    if (prompt_with_context) {
                        snprintf(prompt_with_context, prompt_size, "%s\n\nContext from Ali: %s",
                                tasks[i].agent->system_prompt,
                                tasks[i].context ? tasks[i].context : "Please analyze and respond.");

                        tasks[i].response = nous_claude_chat(prompt_with_context, tasks[i].user_input);
                        free(prompt_with_context);

                        if (tasks[i].response) {
                            cost_record_agent_usage(tasks[i].agent,
                                                    strlen(tasks[i].agent->system_prompt) / 4 + strlen(tasks[i].user_input) / 4,
                                                    strlen(tasks[i].response) / 4);
                            tasks[i].completed = true;
                        }
                    }

                    // Set agent back to idle
                    agent_set_idle(tasks[i].agent);
                });
            }
        }

        // Wait for all agents to complete
        dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

        // Build convergence prompt with all responses
        size_t convergence_size = strlen(final_response) + 4096;
        for (size_t i = 0; i < delegations->count; i++) {
            if (tasks[i].response) {
                convergence_size += strlen(tasks[i].response) + 256;
            }
        }

        char* convergence_prompt = malloc(convergence_size);
        if (convergence_prompt) {
            size_t offset = snprintf(convergence_prompt, convergence_size,
                "You delegated to %zu specialist agents. Here are their responses:\n\n",
                delegations->count);

            for (size_t i = 0; i < delegations->count; i++) {
                if (tasks[i].completed && tasks[i].response) {
                    offset += snprintf(convergence_prompt + offset, convergence_size - offset,
                        "## %s's Response\n%s\n\n",
                        tasks[i].agent ? tasks[i].agent->name : "Agent",
                        tasks[i].response);
                }
            }

            offset += snprintf(convergence_prompt + offset, convergence_size - offset,
                "---\n\nOriginal user request: %s\n\n"
                "Please synthesize all these specialist perspectives into a unified, comprehensive response for the user. "
                "Integrate insights from each agent, highlight agreements and different viewpoints, "
                "and provide actionable conclusions.",
                user_input);

            // Ali synthesizes all responses
            char* synthesized = nous_claude_chat(g_orchestrator->ali->system_prompt, convergence_prompt);
            free(convergence_prompt);

            if (synthesized) {
                cost_record_agent_usage(g_orchestrator->ali, 1000, strlen(synthesized) / 4);

                // Cleanup
                for (size_t i = 0; i < delegations->count; i++) {
                    free(tasks[i].response);
                }
                free(tasks);
                free_delegation_list(delegations);
                free(final_response);

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
        }

        // Cleanup on failure
        for (size_t i = 0; i < delegations->count; i++) {
            free(tasks[i].response);
        }
        free(tasks);
        free_delegation_list(delegations);
    }

    // Save assistant response to persistence
    if (g_current_session_id && final_response) {
        persistence_save_conversation(g_current_session_id, "assistant", final_response,
                                       (int)strlen(final_response) / 4);
    }

    // Create response message
    Message* response_msg = message_create(MSG_TYPE_AGENT_RESPONSE,
                                            g_orchestrator->ali->id, 0, final_response);
    if (response_msg) {
        message_send(response_msg);
    }

    return final_response;
}

// ============================================================================
// CONVERGENCE
// ============================================================================

// Converge results from multiple agents into unified response
char* orchestrator_converge(ExecutionPlan* plan) {
    if (!g_orchestrator || !plan) return NULL;

    // Collect all task results
    size_t buf_size = 8192;
    char* combined = malloc(buf_size);
    if (!combined) return NULL;

    size_t offset = snprintf(combined, buf_size,
        "Synthesize the following results into a unified response:\n\nGoal: %s\n\n",
        plan->goal);

    Task* task = plan->tasks;
    while (task && offset < buf_size - 512) {
        if (task->status == TASK_STATUS_COMPLETED && task->result) {
            ManagedAgent* agent = NULL;
            for (size_t i = 0; i < g_orchestrator->agent_count; i++) {
                if (g_orchestrator->agents[i]->id == task->assigned_to) {
                    agent = g_orchestrator->agents[i];
                    break;
                }
            }

            offset += snprintf(combined + offset, buf_size - offset,
                "## %s's Analysis\n%s\n\n",
                agent ? agent->name : "Agent",
                task->result);
        }
        task = task->next;
    }

    // Ask Ali to synthesize
    char* final = nous_claude_chat(
        "You are Ali. Synthesize the following multi-agent analysis into a clear, actionable response.",
        combined);

    free(combined);

    if (final) {
        plan->final_result = strdup(final);
        plan->is_complete = true;
    }

    return final;
}

// ============================================================================
// DIRECT AGENT COMMUNICATION (with tools support)
// ============================================================================

// Tools instructions to append to agent system prompts
static const char* AGENT_TOOLS_INSTRUCTIONS =
    "\n\n## Tools Available\n"
    "You have access to these tools - USE THEM when needed:\n"
    "- **file_read**: Read file contents from the filesystem\n"
    "- **file_write**: Write content to files (create or modify) - USE THIS to make changes\n"
    "- **file_list**: List directory contents\n"
    "- **shell_exec**: Execute shell commands\n"
    "- **web_fetch**: Fetch content from URLs (for research)\n"
    "- **memory_store**: Store information in semantic memory\n"
    "- **memory_search**: Search stored memories\n\n"
    "IMPORTANT: When asked to modify files, DO IT directly using file_write. "
    "Do not say you cannot write files - you CAN and SHOULD use the tools.\n";

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
    strcpy(conversation, user_message);

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

    pthread_mutex_lock(&g_orch_mutex);

    free(g_orchestrator->user_name);
    g_orchestrator->user_name = name ? strdup(name) : NULL;

    free(g_orchestrator->user_preferences);
    g_orchestrator->user_preferences = preferences ? strdup(preferences) : NULL;

    pthread_mutex_unlock(&g_orch_mutex);
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
