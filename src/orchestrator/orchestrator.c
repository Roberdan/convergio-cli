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

// Global orchestrator instance
static Orchestrator* g_orchestrator = NULL;
static pthread_mutex_t g_orch_mutex = PTHREAD_MUTEX_INITIALIZER;

// External functions
extern int persistence_init(const char* db_path);
extern void persistence_shutdown(void);
extern int msgbus_init(void);
extern void msgbus_shutdown(void);
extern char* persistence_create_session(const char* user_name);

// Claude API (from neural/claude.c)
extern int nous_claude_init(void);
extern void nous_claude_shutdown(void);
extern char* nous_claude_chat(const char* system_prompt, const char* user_message);
extern char* nous_claude_chat_with_tools(const char* system_prompt, const char* user_message,
                                          const char* tools_json, char** out_tool_calls);

// Tools header
#include "nous/tools.h"

// ============================================================================
// ALI'S SYSTEM PROMPT
// ============================================================================

static const char* ALI_SYSTEM_PROMPT =
    "You are Ali, the Chief of Staff and master orchestrator for the Convergio ecosystem.\n\n"
    "## Your Role\n"
    "You are the single point of contact for the user. You coordinate all specialist agents and use tools to deliver comprehensive solutions.\n\n"
    "## Tools Available\n"
    "You have access to these tools to interact with the real world:\n"
    "- **file_read**: Read file contents from the filesystem\n"
    "- **file_write**: Write content to files (create or modify)\n"
    "- **file_list**: List directory contents\n"
    "- **shell_exec**: Execute shell commands (with safety restrictions)\n"
    "- **web_fetch**: Fetch content from URLs\n"
    "- **memory_store**: Store information for later retrieval\n"
    "- **memory_search**: Search stored memories semantically\n\n"
    "Use these tools proactively when the user's request requires interacting with files, running commands, or fetching web data.\n\n"
    "## Specialist Agents\n"
    "You can delegate complex analysis to specialist agents:\n"
    "- **Baccio** (Tech Architect): System design, architecture decisions\n"
    "- **Domik** (Strategy): McKinsey-level strategic analysis\n"
    "- **Omri** (Data Scientist): ML, statistics, data analysis\n"
    "- **Luca** (Security): Cybersecurity, risk assessment\n"
    "- **Thor** (Quality): Quality assurance, validation\n"
    "- **Sara** (UX/UI): User experience, design\n"
    "- **Elena** (Legal): Compliance, legal guidance\n"
    "- **Wanda** (Workflow): Process orchestration\n\n"
    "## Response Guidelines\n"
    "1. Be concise but comprehensive\n"
    "2. Use tools when the task requires file access, command execution, or web fetching\n"
    "3. Delegate to specialists for deep analysis\n"
    "4. Always synthesize insights into actionable recommendations\n"
    "5. Be honest about limitations and uncertainties\n\n"
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
        g_orchestrator->ali->system_prompt = strdup(ALI_SYSTEM_PROMPT);
        g_orchestrator->ali->is_active = true;
        g_orchestrator->ali->created_at = time(NULL);

        // Add to agent pool
        g_orchestrator->agents[g_orchestrator->agent_count++] = g_orchestrator->ali;
    }

    // Create session
    char* session_id = persistence_create_session("default");
    if (session_id) {
        free(session_id);  // We'd store this for the session
    }

    g_orchestrator->initialized = true;

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

// Parse Ali's response for delegation requests
typedef struct {
    char* agent_name;
    char* reason;
} DelegationRequest;

static DelegationRequest* parse_delegation(const char* response) {
    const char* marker = "[DELEGATE:";
    char* pos = strstr(response, marker);
    if (!pos) return NULL;

    DelegationRequest* req = calloc(1, sizeof(DelegationRequest));
    if (!req) return NULL;

    // Extract agent name
    pos += strlen(marker);
    while (*pos == ' ') pos++;

    char* end = strchr(pos, ']');
    if (!end) {
        free(req);
        return NULL;
    }

    size_t name_len = end - pos;
    req->agent_name = malloc(name_len + 1);
    if (req->agent_name) {
        strncpy(req->agent_name, pos, name_len);
        req->agent_name[name_len] = '\0';
    }

    // Extract reason
    pos = end + 1;
    while (*pos == ' ') pos++;
    if (*pos) {
        req->reason = strdup(pos);
    }

    return req;
}

static void free_delegation(DelegationRequest* req) {
    if (!req) return;
    free(req->agent_name);
    free(req->reason);
    free(req);
}

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

char* orchestrator_process(const char* user_input) {
    if (!g_orchestrator || !g_orchestrator->initialized || !user_input) {
        return strdup("Error: Orchestrator not initialized");
    }

    // Check budget
    if (g_orchestrator->cost.budget_exceeded) {
        return strdup("Budget exceeded. Use 'cost set <amount>' to increase budget.");
    }

    // Create user message
    Message* user_msg = message_create(MSG_TYPE_USER_INPUT, 0, g_orchestrator->ali->id, user_input);
    if (user_msg) {
        message_send(user_msg);
    }

    // Get tools definition
    const char* tools_json = tools_get_definitions_json();

    // Build conversation for multi-turn tool use
    // Start with user message
    size_t conv_capacity = 32768;
    char* conversation = malloc(conv_capacity);
    if (!conversation) {
        return strdup("Error: Memory allocation failed");
    }

    // Initial conversation is just the user input
    snprintf(conversation, conv_capacity, "%s", user_input);

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
            char* tool_results = malloc(16384);
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

                        if (results_len + strlen(result_entry) < 16000) {
                            strcat(tool_results, result_entry);
                            results_len += strlen(result_entry);
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

            if (tool_count > 0) {
                // Build new conversation with tool results
                size_t new_conv_len = strlen(conversation) + strlen(tool_results) + 256;
                if (new_conv_len > conv_capacity) {
                    conv_capacity = new_conv_len + 4096;
                    conversation = realloc(conversation, conv_capacity);
                }

                // Append tool results and ask for final response
                strcat(conversation, "\n\n[Tool Results]");
                strcat(conversation, tool_results);
                strcat(conversation, "\n\nBased on these tool results, provide your response to the user.");

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

    // Check for delegation request in final response
    DelegationRequest* delegation = parse_delegation(final_response);
    if (delegation) {
        // Find or spawn the requested agent
        ManagedAgent* specialist = agent_find_by_name(delegation->agent_name);
        if (!specialist) {
            specialist = agent_spawn(AGENT_ROLE_ANALYST, delegation->agent_name, NULL);
        }

        if (specialist && specialist->system_prompt) {
            // Create delegation message
            Message* delegate_msg = message_create(MSG_TYPE_TASK_DELEGATE,
                                                    g_orchestrator->ali->id,
                                                    specialist->id,
                                                    user_input);
            if (delegate_msg) {
                message_send(delegate_msg);
            }

            // Get specialist response
            char* prompt_with_context = malloc(strlen(specialist->system_prompt) +
                                               strlen(delegation->reason) + 256);
            if (prompt_with_context) {
                sprintf(prompt_with_context, "%s\n\nContext from Ali: %s",
                        specialist->system_prompt, delegation->reason);

                char* specialist_response = nous_claude_chat(prompt_with_context, user_input);
                free(prompt_with_context);

                if (specialist_response) {
                    // Record specialist cost
                    cost_record_agent_usage(specialist,
                                            strlen(specialist->system_prompt) / 4 + strlen(user_input) / 4,
                                            strlen(specialist_response) / 4);

                    // Create convergence - Ali synthesizes
                    char* convergence_prompt = malloc(strlen(final_response) +
                                                      strlen(specialist_response) + 512);
                    if (convergence_prompt) {
                        sprintf(convergence_prompt,
                            "You previously said: %s\n\n"
                            "%s responded: %s\n\n"
                            "Please synthesize this into a final response for the user.",
                            final_response, delegation->agent_name, specialist_response);

                        char* synthesized = nous_claude_chat(g_orchestrator->ali->system_prompt,
                                                                convergence_prompt);
                        free(convergence_prompt);
                        free(final_response);
                        free(specialist_response);
                        free_delegation(delegation);

                        if (synthesized) {
                            cost_record_agent_usage(g_orchestrator->ali, 500, strlen(synthesized) / 4);
                            return synthesized;
                        }
                    }
                    free(specialist_response);
                }
            }
        }
        free_delegation(delegation);
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
