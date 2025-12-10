/**
 * CONVERGIO ORCHESTRATOR
 *
 * Ali - The Chief of Staff
 * Coordinates all agents, manages cost, ensures convergence
 */

#ifndef CONVERGIO_ORCHESTRATOR_H
#define CONVERGIO_ORCHESTRATOR_H

#include "nous/nous.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ============================================================================
// COST TRACKING
// ============================================================================

typedef struct {
    uint64_t input_tokens;
    uint64_t output_tokens;
    uint64_t total_tokens;
    double cost_usd;
    uint32_t api_calls;
} TokenUsage;

typedef struct {
    double budget_limit_usd;      // Max spend allowed
    double current_spend_usd;     // Current session spend
    double total_spend_usd;       // All-time spend (from DB)
    TokenUsage session_usage;     // Current session
    TokenUsage total_usage;       // All-time
    bool budget_exceeded;
    time_t session_start;
} CostController;

// Claude pricing (per 1M tokens) - Sonnet 4
#define CLAUDE_SONNET_INPUT_COST   3.00   // $3 per 1M input
#define CLAUDE_SONNET_OUTPUT_COST  15.00  // $15 per 1M output

// ============================================================================
// MESSAGE TYPES
// ============================================================================

typedef enum {
    MSG_TYPE_USER_INPUT,      // From human
    MSG_TYPE_AGENT_THOUGHT,   // Internal reasoning
    MSG_TYPE_AGENT_ACTION,    // Tool/action request
    MSG_TYPE_AGENT_RESPONSE,  // Response to user/other agent
    MSG_TYPE_TASK_DELEGATE,   // Delegate to sub-agent
    MSG_TYPE_TASK_REPORT,     // Report back to orchestrator
    MSG_TYPE_CONVERGENCE,     // Final converged answer
    MSG_TYPE_ERROR,           // Error condition
} MessageType;

typedef struct Message {
    uint64_t id;
    MessageType type;
    SemanticID sender;        // Agent or user ID
    SemanticID recipient;     // Target agent (0 = broadcast)
    char* content;
    char* metadata_json;      // Additional context
    time_t timestamp;
    uint64_t parent_id;       // For threading
    TokenUsage tokens_used;   // Cost tracking per message
    struct Message* next;     // Linked list
} Message;

// ============================================================================
// AGENT SPECIALIZATIONS
// ============================================================================

typedef enum {
    AGENT_ROLE_ORCHESTRATOR,  // Ali - coordinates everything
    AGENT_ROLE_ANALYST,       // Deep analysis, research
    AGENT_ROLE_CODER,         // Code generation/review
    AGENT_ROLE_WRITER,        // Content creation
    AGENT_ROLE_CRITIC,        // Review and validate
    AGENT_ROLE_PLANNER,       // Break down tasks
    AGENT_ROLE_EXECUTOR,      // Execute tools/actions
    AGENT_ROLE_MEMORY,        // RAG and context retrieval
} AgentRole;

typedef struct {
    SemanticID id;
    char* name;
    AgentRole role;
    char* system_prompt;
    char* specialized_context;
    bool is_active;
    TokenUsage usage;
    Message* pending_messages;
    time_t created_at;
    time_t last_active;
} ManagedAgent;

// ============================================================================
// TASK & PLAN
// ============================================================================

typedef enum {
    TASK_STATUS_PENDING,
    TASK_STATUS_IN_PROGRESS,
    TASK_STATUS_WAITING,      // Waiting for sub-task
    TASK_STATUS_COMPLETED,
    TASK_STATUS_FAILED,
} TaskStatus;

typedef struct Task {
    uint64_t id;
    char* description;
    SemanticID assigned_to;
    TaskStatus status;
    char* result;
    struct Task* subtasks;
    struct Task* next;
    uint64_t parent_task_id;
    time_t created_at;
    time_t completed_at;
} Task;

typedef struct {
    uint64_t id;
    char* goal;
    Task* tasks;
    bool is_complete;
    char* final_result;
    time_t created_at;
} ExecutionPlan;

// ============================================================================
// ORCHESTRATOR STATE
// ============================================================================

typedef struct {
    ManagedAgent* ali;              // The chief of staff
    ManagedAgent** agents;          // Pool of available agents
    size_t agent_count;
    size_t agent_capacity;

    CostController cost;            // Budget and spending

    Message* message_history;       // Conversation history
    size_t message_count;

    ExecutionPlan* current_plan;    // Active execution plan

    SemanticID user_id;             // Current user identity
    char* user_name;
    char* user_preferences;         // Learned from memory

    // Callbacks
    void (*on_message)(Message* msg, void* ctx);
    void (*on_cost_update)(CostController* cost, void* ctx);
    void (*on_agent_spawn)(ManagedAgent* agent, void* ctx);
    void* callback_ctx;

    bool initialized;
} Orchestrator;

// ============================================================================
// API
// ============================================================================

// Lifecycle
int orchestrator_init(double budget_limit_usd);
void orchestrator_shutdown(void);
Orchestrator* orchestrator_get(void);

// Cost control
void cost_record_usage(uint64_t input_tokens, uint64_t output_tokens);
double cost_get_session_spend(void);
double cost_get_total_spend(void);
bool cost_check_budget(void);
void cost_set_budget(double limit_usd);
char* cost_get_report(void);

// Agent management
ManagedAgent* agent_spawn(AgentRole role, const char* name, const char* context);
void agent_despawn(ManagedAgent* agent);
ManagedAgent* agent_find_by_role(AgentRole role);
ManagedAgent* agent_find_by_name(const char* name);

// Messaging
Message* message_create(MessageType type, SemanticID sender,
                        SemanticID recipient, const char* content);
void message_send(Message* msg);
void message_broadcast(Message* msg);

// Task execution
ExecutionPlan* orch_plan_create(const char* goal);
Task* orch_task_create(const char* description, SemanticID assignee);
void orch_plan_add_task(ExecutionPlan* plan, Task* task);
void orch_task_complete(Task* task, const char* result);

// Main entry point - process user input through orchestrator
char* orchestrator_process(const char* user_input);

// Convergence
char* orchestrator_converge(ExecutionPlan* plan);

// Parallel execution
char* orchestrator_parallel_analyze(const char* input, const char** agent_names, size_t agent_count);

// User management
void orchestrator_set_user(const char* name, const char* preferences);

// Status
char* orchestrator_status(void);

// ============================================================================
// PERSISTENCE API
// ============================================================================

int persistence_init(const char* db_path);
void persistence_shutdown(void);

int persistence_save_message(const char* session_id, Message* msg);
Message** persistence_load_recent_messages(const char* session_id, size_t limit, size_t* out_count);

int persistence_save_agent(const char* name, AgentRole role, const char* system_prompt,
                           const char* context, const char* color, const char* tools_json);
char* persistence_load_agent_prompt(const char* name);

int persistence_set_pref(const char* key, const char* value);
char* persistence_get_pref(const char* key);

int persistence_save_cost_daily(const char* date, uint64_t input_tokens,
                                 uint64_t output_tokens, double cost, uint32_t calls);
double persistence_get_total_cost(void);

int persistence_save_memory(const char* content, float importance);
char** persistence_get_important_memories(size_t limit, size_t* out_count);

char* persistence_create_session(const char* user_name);
int persistence_end_session(const char* session_id, double total_cost, int total_messages);

// ============================================================================
// MESSAGE BUS API
// ============================================================================

int msgbus_init(void);
void msgbus_shutdown(void);

void message_destroy(Message* msg);
Message* message_get_pending(ManagedAgent* agent);
Message** message_get_history(size_t limit, size_t* out_count);
Message** message_get_by_type(MessageType type, size_t limit, size_t* out_count);
void message_send_async(Message* msg, void (*on_delivered)(Message*, void*), void* ctx);
Message* message_reply(Message* original, MessageType type, const char* content);
Message** message_get_thread(uint64_t message_id, size_t* out_count);
Message* message_create_convergence(SemanticID sender, const char* content,
                                     Message** source_messages, size_t source_count);
void message_print(Message* msg);

// ============================================================================
// REGISTRY API
// ============================================================================

size_t agent_get_active(ManagedAgent** out_agents, size_t max_count);
int agent_load_definitions(const char* dir_path);
size_t agent_select_for_task(const char* task_description, ManagedAgent** out_agents, size_t max_count);
void agent_execute_parallel(ManagedAgent** agents, size_t count, const char* input, char** outputs);
char* agent_registry_status(void);

// ============================================================================
// COST API (EXTENDED)
// ============================================================================

void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens, uint64_t output_tokens);
double cost_get_remaining_budget(void);
void cost_reset_session(void);
char* cost_get_status_line(void);
char* cost_get_agent_report(ManagedAgent* agent);
void cost_get_top_agents(ManagedAgent** out_agents, size_t* out_count, size_t max_count);
double cost_estimate_message(const char* text, bool is_input);
bool cost_can_afford(size_t estimated_turns, size_t avg_input_tokens, size_t avg_output_tokens);

#endif // CONVERGIO_ORCHESTRATOR_H
