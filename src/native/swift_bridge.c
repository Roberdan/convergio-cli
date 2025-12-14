/**
 * CONVERGIO NATIVE APP - C TO SWIFT BRIDGE IMPLEMENTATION
 *
 * This file implements the Swift-friendly C API defined in shim.h
 * It wraps the existing C library functions into a clean interface.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include <stdlib.h>
#include <string.h>
#include "nous/orchestrator.h"
#include "nous/nous.h"
#include "nous/provider.h"

// Type aliases for Swift-friendly names (these match shim.h)
typedef enum {
    CAgentRoleOrchestrator = AGENT_ROLE_ORCHESTRATOR,
    CAgentRoleAnalyst = AGENT_ROLE_ANALYST,
    CAgentRoleCoder = AGENT_ROLE_CODER,
    CAgentRoleWriter = AGENT_ROLE_WRITER,
    CAgentRoleCritic = AGENT_ROLE_CRITIC,
    CAgentRolePlanner = AGENT_ROLE_PLANNER,
    CAgentRoleExecutor = AGENT_ROLE_EXECUTOR,
    CAgentRoleMemory = AGENT_ROLE_MEMORY,
} CAgentRole;

typedef enum {
    CAgentWorkStateIdle = WORK_STATE_IDLE,
    CAgentWorkStateThinking = WORK_STATE_THINKING,
    CAgentWorkStateExecuting = WORK_STATE_EXECUTING,
    CAgentWorkStateWaiting = WORK_STATE_WAITING,
    CAgentWorkStateCommunicating = WORK_STATE_COMMUNICATING
} CAgentWorkState;

typedef enum {
    CMessageTypeUserInput = MSG_TYPE_USER_INPUT,
    CMessageTypeAgentThought = MSG_TYPE_AGENT_THOUGHT,
    CMessageTypeAgentAction = MSG_TYPE_AGENT_ACTION,
    CMessageTypeAgentResponse = MSG_TYPE_AGENT_RESPONSE,
    CMessageTypeTaskDelegate = MSG_TYPE_TASK_DELEGATE,
    CMessageTypeTaskReport = MSG_TYPE_TASK_REPORT,
    CMessageTypeConvergence = MSG_TYPE_CONVERGENCE,
    CMessageTypeError = MSG_TYPE_ERROR,
} CMessageType;

typedef enum {
    CProviderAnthropic = PROVIDER_ANTHROPIC,
    CProviderOpenAI = PROVIDER_OPENAI,
    CProviderGemini = PROVIDER_GEMINI,
    CProviderOpenRouter = PROVIDER_OPENROUTER,
    CProviderOllama = PROVIDER_OLLAMA,
    CProviderMLX = PROVIDER_MLX,
} CProviderType;

typedef struct {
    size_t input_tokens;
    size_t output_tokens;
    size_t cached_tokens;
    double estimated_cost;
} CTokenUsage;

// Callback type for streaming
typedef void (*ConvergioStreamCallback)(const char* chunk, void* user_data);

// ============================================================================
// ORCHESTRATOR API IMPLEMENTATION
// ============================================================================

int convergio_init(double budget_limit_usd) {
    // Initialize the kernel first
    if (nous_init() != 0) {
        return -1;
    }

    // Initialize orchestrator with budget
    if (orchestrator_init(budget_limit_usd) != 0) {
        nous_shutdown();
        return -1;
    }

    return 0;
}

void convergio_shutdown(void) {
    orchestrator_shutdown();
    nous_shutdown();
}

bool convergio_is_ready(void) {
    Orchestrator* orch = orchestrator_get();
    return orch != NULL && orch->initialized;
}

Orchestrator* convergio_get_orchestrator(void) {
    return orchestrator_get();
}

char* convergio_process(const char* user_input) {
    if (!user_input) return NULL;
    return orchestrator_process(user_input);
}

char* convergio_process_stream(const char* user_input,
                               ConvergioStreamCallback callback,
                               void* user_data) {
    if (!user_input) return NULL;

    // Cast the callback type - they have compatible signatures
    return orchestrator_process_stream(user_input,
                                       (OrchestratorStreamCallback)callback,
                                       user_data);
}

void convergio_cancel_request(void) {
    claude_cancel_request();
}

void convergio_reset_cancel(void) {
    claude_reset_cancel();
}

bool convergio_is_cancelled(void) {
    return claude_is_cancelled();
}

// ============================================================================
// AGENT MANAGEMENT IMPLEMENTATION
// ============================================================================

size_t convergio_get_agent_count(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return 0;
    return orch->agent_count;
}

ManagedAgent* convergio_get_agent_at(size_t index) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || index >= orch->agent_count) return NULL;
    return orch->agents[index];
}

ManagedAgent* convergio_get_agent_by_name(const char* name) {
    if (!name) return NULL;
    return agent_find_by_name(name);
}

SemanticID convergio_agent_get_id(ManagedAgent* agent) {
    if (!agent) return 0;
    return agent->id;
}

const char* convergio_agent_get_name(ManagedAgent* agent) {
    if (!agent) return NULL;
    return agent->name;
}

const char* convergio_agent_get_description(ManagedAgent* agent) {
    if (!agent) return NULL;
    return agent->description;
}

CAgentRole convergio_agent_get_role(ManagedAgent* agent) {
    if (!agent) return CAgentRoleOrchestrator;
    return (CAgentRole)agent->role;
}

CAgentWorkState convergio_agent_get_work_state(ManagedAgent* agent) {
    if (!agent) return CAgentWorkStateIdle;
    return (CAgentWorkState)agent->work_state;
}

const char* convergio_agent_get_current_task(ManagedAgent* agent) {
    if (!agent) return NULL;
    return agent->current_task;
}

bool convergio_agent_is_active(ManagedAgent* agent) {
    if (!agent) return false;
    return agent->is_active;
}

size_t convergio_get_working_agents(ManagedAgent** out_agents, size_t max_count) {
    if (!out_agents || max_count == 0) return 0;
    return agent_get_working(out_agents, max_count);
}

// ============================================================================
// COST TRACKING IMPLEMENTATION
// ============================================================================

double convergio_get_session_cost(void) {
    return cost_get_session_spend();
}

double convergio_get_total_cost(void) {
    return cost_get_total_spend();
}

double convergio_get_budget_limit(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return 0.0;
    return orch->cost.budget_limit_usd;
}

double convergio_get_budget_remaining(void) {
    return cost_get_remaining_budget();
}

void convergio_set_budget(double limit_usd) {
    cost_set_budget(limit_usd);
}

bool convergio_is_budget_exceeded(void) {
    return !cost_check_budget();
}

char* convergio_get_cost_report(void) {
    return cost_get_report();
}

// ============================================================================
// TOKEN USAGE IMPLEMENTATION
// ============================================================================

void convergio_get_session_usage(CTokenUsage* out_usage) {
    if (!out_usage) return;

    Orchestrator* orch = orchestrator_get();
    if (!orch) {
        memset(out_usage, 0, sizeof(CTokenUsage));
        return;
    }

    out_usage->input_tokens = orch->cost.session_usage.input_tokens;
    out_usage->output_tokens = orch->cost.session_usage.output_tokens;
    out_usage->cached_tokens = orch->cost.session_usage.cached_tokens;
    out_usage->estimated_cost = orch->cost.session_usage.estimated_cost;
}

void convergio_get_total_usage(CTokenUsage* out_usage) {
    if (!out_usage) return;

    Orchestrator* orch = orchestrator_get();
    if (!orch) {
        memset(out_usage, 0, sizeof(CTokenUsage));
        return;
    }

    out_usage->input_tokens = orch->cost.total_usage.input_tokens;
    out_usage->output_tokens = orch->cost.total_usage.output_tokens;
    out_usage->cached_tokens = orch->cost.total_usage.cached_tokens;
    out_usage->estimated_cost = orch->cost.total_usage.estimated_cost;
}

// ============================================================================
// MESSAGE HISTORY IMPLEMENTATION
// ============================================================================

size_t convergio_get_message_count(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return 0;
    return orch->message_count;
}

size_t convergio_get_recent_messages(Message** out_messages, size_t max_count) {
    if (!out_messages || max_count == 0) return 0;

    size_t actual_count = 0;
    Message** history = message_get_history(max_count, &actual_count);
    if (!history) return 0;

    for (size_t i = 0; i < actual_count && i < max_count; i++) {
        out_messages[i] = history[i];
    }

    free(history);  // Free the array, not the messages themselves
    return actual_count;
}

const char* convergio_message_get_content(Message* msg) {
    if (!msg) return NULL;
    return msg->content;
}

CMessageType convergio_message_get_type(Message* msg) {
    if (!msg) return CMessageTypeUserInput;
    return (CMessageType)msg->type;
}

SemanticID convergio_message_get_sender(Message* msg) {
    if (!msg) return 0;
    return msg->sender;
}

int64_t convergio_message_get_timestamp(Message* msg) {
    if (!msg) return 0;
    return (int64_t)msg->timestamp;
}

// ============================================================================
// SESSION MANAGEMENT IMPLEMENTATION
// ============================================================================

const char* convergio_get_session_id(void) {
    return orchestrator_get_session_id();
}

int convergio_new_session(const char* user_name) {
    if (!user_name) return -1;

    // Create new session in persistence
    char* session_id = persistence_create_session(user_name);
    if (!session_id) return -1;

    free(session_id);
    return 0;
}

char* convergio_get_status(void) {
    return orchestrator_status();
}

// ============================================================================
// PROVIDER CONFIGURATION IMPLEMENTATION
// ============================================================================

bool convergio_is_provider_available(CProviderType provider) {
    return provider_is_available((ProviderType)provider);
}

const char* convergio_get_provider_name(CProviderType provider) {
    return provider_name((ProviderType)provider);
}

const char* convergio_get_current_model(void) {
    // Get the default model from config or environment
    const char* model = getenv("CONVERGIO_MODEL");
    if (model) return model;
    return "claude-sonnet-4-20250514";  // Default
}

// ============================================================================
// CONVERGENCE IMPLEMENTATION
// ============================================================================

char* convergio_parallel_analyze(const char* input,
                                 const char** agent_names,
                                 size_t agent_count) {
    if (!input || !agent_names || agent_count == 0) return NULL;
    return orchestrator_parallel_analyze(input, agent_names, agent_count);
}

// ============================================================================
// PERSISTENCE IMPLEMENTATION
// ============================================================================

int convergio_persistence_init(const char* db_path) {
    return persistence_init(db_path);
}

void convergio_persistence_shutdown(void) {
    persistence_shutdown();
}

// ============================================================================
// UTILITY IMPLEMENTATION
// ============================================================================

void convergio_free_string(char* str) {
    if (str) free(str);
}
