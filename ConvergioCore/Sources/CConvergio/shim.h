/**
 * CONVERGIO NATIVE APP - C TO SWIFT BRIDGE
 *
 * This header provides a clean interface between the C library and Swift.
 * It exposes only the functions needed by the native app, with Swift-friendly
 * naming and types where possible.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_SWIFT_SHIM_H
#define CONVERGIO_SWIFT_SHIM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// Opaque types for Swift - actual definitions in C headers
typedef uint64_t SemanticID;
typedef struct ManagedAgent ManagedAgent;
typedef struct Message Message;
typedef struct Orchestrator Orchestrator;

// ============================================================================
// AGENT STATE ENUMS (mirrored for Swift)
// ============================================================================

typedef enum {
    CAgentRoleOrchestrator = 0,  // Ali - coordinates everything
    CAgentRoleAnalyst = 1,       // Deep analysis, research
    CAgentRoleCoder = 2,         // Code generation/review
    CAgentRoleWriter = 3,        // Content creation
    CAgentRoleCritic = 4,        // Review and validate
    CAgentRolePlanner = 5,       // Break down tasks
    CAgentRoleExecutor = 6,      // Execute tools/actions
    CAgentRoleMemory = 7,        // RAG and context retrieval
} CAgentRole;

typedef enum {
    CAgentWorkStateIdle = 0,          // Not currently working
    CAgentWorkStateThinking = 1,      // Processing a request
    CAgentWorkStateExecuting = 2,     // Executing tools
    CAgentWorkStateWaiting = 3,       // Waiting for another agent
    CAgentWorkStateCommunicating = 4  // Talking to another agent
} CAgentWorkState;

typedef enum {
    CMessageTypeUserInput = 0,      // From human
    CMessageTypeAgentThought = 1,   // Internal reasoning
    CMessageTypeAgentAction = 2,    // Tool/action request
    CMessageTypeAgentResponse = 3,  // Response to user/other agent
    CMessageTypeTaskDelegate = 4,   // Delegate to sub-agent
    CMessageTypeTaskReport = 5,     // Report back to orchestrator
    CMessageTypeConvergence = 6,    // Final converged answer
    CMessageTypeError = 7,          // Error condition
} CMessageType;

// ============================================================================
// ORCHESTRATOR API
// ============================================================================

/**
 * Initialize the orchestrator with a budget limit
 * @param budget_limit_usd Maximum spend allowed in USD
 * @return 0 on success, -1 on error
 */
int convergio_init(double budget_limit_usd);

/**
 * Shutdown the orchestrator and free all resources
 */
void convergio_shutdown(void);

/**
 * Check if orchestrator is initialized
 * @return true if ready
 */
bool convergio_is_ready(void);

/**
 * Get the global orchestrator instance
 * @return Orchestrator pointer or NULL if not initialized
 */
Orchestrator* convergio_get_orchestrator(void);

/**
 * Process user input through the orchestrator (blocking)
 * @param user_input The user's message
 * @return Response string (caller must free) or NULL on error
 */
char* convergio_process(const char* user_input);

/**
 * Process user input with streaming callback
 * @param user_input The user's message
 * @param callback Function called for each chunk
 * @param user_data Context passed to callback
 * @return Complete response (caller must free) or NULL on error
 */
typedef void (*ConvergioStreamCallback)(const char* chunk, void* user_data);
char* convergio_process_stream(const char* user_input,
                               ConvergioStreamCallback callback,
                               void* user_data);

/**
 * Cancel the current request (for ESC key interrupt)
 */
void convergio_cancel_request(void);

/**
 * Reset cancellation state
 */
void convergio_reset_cancel(void);

/**
 * Check if request was cancelled
 * @return true if cancelled
 */
bool convergio_is_cancelled(void);

// ============================================================================
// AGENT MANAGEMENT
// ============================================================================

/**
 * Get the total number of available agents
 * @return Agent count
 */
size_t convergio_get_agent_count(void);

/**
 * Get an agent by index
 * @param index Agent index (0 to count-1)
 * @return Agent pointer or NULL if invalid index
 */
ManagedAgent* convergio_get_agent_at(size_t index);

/**
 * Get an agent by name
 * @param name Agent name (e.g., "Ali", "Angela")
 * @return Agent pointer or NULL if not found
 */
ManagedAgent* convergio_get_agent_by_name(const char* name);

/**
 * Get agent ID
 * @param agent Agent pointer
 * @return Semantic ID
 */
SemanticID convergio_agent_get_id(ManagedAgent* agent);

/**
 * Get agent name
 * @param agent Agent pointer
 * @return Agent name (do not free)
 */
const char* convergio_agent_get_name(ManagedAgent* agent);

/**
 * Get agent description
 * @param agent Agent pointer
 * @return Agent description (do not free)
 */
const char* convergio_agent_get_description(ManagedAgent* agent);

/**
 * Get agent role
 * @param agent Agent pointer
 * @return Agent role enum value
 */
CAgentRole convergio_agent_get_role(ManagedAgent* agent);

/**
 * Get agent work state
 * @param agent Agent pointer
 * @return Current work state
 */
CAgentWorkState convergio_agent_get_work_state(ManagedAgent* agent);

/**
 * Get agent's current task description
 * @param agent Agent pointer
 * @return Current task or NULL if idle (do not free)
 */
const char* convergio_agent_get_current_task(ManagedAgent* agent);

/**
 * Check if agent is active
 * @param agent Agent pointer
 * @return true if active
 */
bool convergio_agent_is_active(ManagedAgent* agent);

/**
 * Get all working (non-idle) agents
 * @param out_agents Output array (caller allocates)
 * @param max_count Maximum agents to return
 * @return Actual number of working agents
 */
size_t convergio_get_working_agents(ManagedAgent** out_agents, size_t max_count);

// ============================================================================
// COST TRACKING
// ============================================================================

/**
 * Get current session spend in USD
 * @return Session cost
 */
double convergio_get_session_cost(void);

/**
 * Get total spend in USD (all time)
 * @return Total cost
 */
double convergio_get_total_cost(void);

/**
 * Get budget limit in USD
 * @return Budget limit
 */
double convergio_get_budget_limit(void);

/**
 * Get remaining budget in USD
 * @return Remaining budget
 */
double convergio_get_budget_remaining(void);

/**
 * Set new budget limit
 * @param limit_usd New limit in USD
 */
void convergio_set_budget(double limit_usd);

/**
 * Check if budget is exceeded
 * @return true if over budget
 */
bool convergio_is_budget_exceeded(void);

/**
 * Get cost report as formatted string
 * @return Report string (caller must free)
 */
char* convergio_get_cost_report(void);

// ============================================================================
// TOKEN USAGE
// ============================================================================

typedef struct {
    size_t input_tokens;
    size_t output_tokens;
    size_t cached_tokens;
    double estimated_cost;
} CTokenUsage;

/**
 * Get session token usage
 * @param out_usage Output structure
 */
void convergio_get_session_usage(CTokenUsage* out_usage);

/**
 * Get total token usage
 * @param out_usage Output structure
 */
void convergio_get_total_usage(CTokenUsage* out_usage);

// ============================================================================
// MESSAGE HISTORY
// ============================================================================

/**
 * Get message count in current session
 * @return Message count
 */
size_t convergio_get_message_count(void);

/**
 * Get recent messages
 * @param out_messages Output array (caller allocates)
 * @param max_count Maximum messages to return
 * @return Actual number of messages
 */
size_t convergio_get_recent_messages(Message** out_messages, size_t max_count);

/**
 * Get message content
 * @param msg Message pointer
 * @return Content string (do not free)
 */
const char* convergio_message_get_content(Message* msg);

/**
 * Get message type
 * @param msg Message pointer
 * @return Message type
 */
CMessageType convergio_message_get_type(Message* msg);

/**
 * Get message sender ID
 * @param msg Message pointer
 * @return Sender's semantic ID
 */
SemanticID convergio_message_get_sender(Message* msg);

/**
 * Get message timestamp (Unix epoch seconds)
 * @param msg Message pointer
 * @return Timestamp
 */
int64_t convergio_message_get_timestamp(Message* msg);

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

/**
 * Get current session ID
 * @return Session ID string (do not free)
 */
const char* convergio_get_session_id(void);

/**
 * Create a new session
 * @param user_name User's name
 * @return 0 on success
 */
int convergio_new_session(const char* user_name);

/**
 * Get orchestrator status as formatted string
 * @return Status string (caller must free)
 */
char* convergio_get_status(void);

// ============================================================================
// PROVIDER CONFIGURATION
// ============================================================================

typedef enum {
    CProviderAnthropic = 0,
    CProviderOpenAI = 1,
    CProviderGemini = 2,
    CProviderOpenRouter = 3,
    CProviderOllama = 4,
    CProviderMLX = 5,
} CProviderType;

/**
 * Check if a provider is available
 * @param provider Provider type
 * @return true if API key is configured
 */
bool convergio_is_provider_available(CProviderType provider);

/**
 * Get provider name
 * @param provider Provider type
 * @return Provider name string
 */
const char* convergio_get_provider_name(CProviderType provider);

/**
 * Get current model name
 * @return Model name (do not free)
 */
const char* convergio_get_current_model(void);

// ============================================================================
// CONVERGENCE
// ============================================================================

/**
 * Request parallel analysis from multiple agents
 * @param input User input
 * @param agent_names Array of agent names
 * @param agent_count Number of agents
 * @return Converged response (caller must free)
 */
char* convergio_parallel_analyze(const char* input,
                                 const char** agent_names,
                                 size_t agent_count);

// ============================================================================
// PERSISTENCE
// ============================================================================

/**
 * Initialize persistence layer
 * @param db_path Path to SQLite database (NULL for default)
 * @return 0 on success
 */
int convergio_persistence_init(const char* db_path);

/**
 * Shutdown persistence layer
 */
void convergio_persistence_shutdown(void);

// ============================================================================
// UTILITY
// ============================================================================

/**
 * Free a string allocated by convergio functions
 * @param str String to free
 */
void convergio_free_string(char* str);

#ifdef __cplusplus
}
#endif

#endif // CONVERGIO_SWIFT_SHIM_H
