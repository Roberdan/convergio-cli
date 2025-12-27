/**
 * CONVERGIO DELEGATION
 *
 * Agent delegation logic - handles routing tasks to specialist agents
 */

#include "nous/delegation.h"
#include "nous/nous.h"
#include "nous/orchestrator.h"
#include "nous/projects.h"
#include "nous/provider.h"
#include "nous/telemetry.h"
#include "nous/workflow_monitor.h"
#include <dispatch/dispatch.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Default model for agent delegation (can be overridden by --provider)
#define DELEGATION_MODEL_DEFAULT "claude-sonnet-4-20250514"
#define DELEGATION_MODEL_OLLAMA "qwen2.5:0.5b"  // Default Ollama model

// External router functions for provider override
extern bool router_has_provider_override(void);
extern ProviderType router_get_forced_provider(void);
extern const char* router_get_forced_model(void);

// External functions from orchestrator
extern int persistence_save_conversation(const char* session_id, const char* role,
                                         const char* content, int tokens);
extern void agent_set_working(ManagedAgent* agent, AgentWorkState state, const char* task);
extern void agent_set_idle(ManagedAgent* agent);
extern ManagedAgent* agent_find_by_name(const char* name);
extern ManagedAgent* agent_spawn(AgentRole role, const char* name, const char* context);
extern void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens,
                                    uint64_t output_tokens);

// Structure for parallel agent execution
typedef struct {
    ManagedAgent* agent;
    const char* user_input;
    const char* context;
    char* response;
    bool completed;
} AgentTask;

// Helper: Get provider and model for delegation (respects --provider override)
static void get_delegation_provider(Provider** out_provider, const char** out_model) {
    if (router_has_provider_override()) {
        ProviderType forced = router_get_forced_provider();
        const char* forced_model = router_get_forced_model();

        *out_provider = provider_get(forced);
        if (forced_model) {
            *out_model = forced_model;
        } else {
            // Use default model for the forced provider
            switch (forced) {
                case PROVIDER_OLLAMA:
                    *out_model = DELEGATION_MODEL_OLLAMA;
                    break;
                case PROVIDER_ANTHROPIC:
                    *out_model = DELEGATION_MODEL_DEFAULT;
                    break;
                default:
                    *out_model = DELEGATION_MODEL_DEFAULT;
                    break;
            }
        }
        LOG_INFO(LOG_CAT_AGENT, "Delegation using overridden provider %d, model %s",
                 (int)forced, *out_model);
    } else {
        // Default: Anthropic with Claude Sonnet
        *out_provider = provider_get(PROVIDER_ANTHROPIC);
        *out_model = DELEGATION_MODEL_DEFAULT;
    }
}

// ============================================================================
// DELEGATION PARSING
// ============================================================================

// Parse ALL delegation requests from response
DelegationList* parse_all_delegations(const char* response) {
    DelegationList* list = calloc(1, sizeof(DelegationList));
    if (!list)
        return NULL;

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
        if (!req)
            break;

        // Extract agent name
        pos += strlen(marker);
        while (*pos == ' ')
            pos++;

        const char* end = strchr(pos, ']');
        if (!end) {
            free(req);
            break;
        }

        size_t name_len = (size_t)(end - pos);

        // Validate name length (from v7-plans)
        if (name_len == 0 || name_len > 256) {
            LOG_WARN(LOG_CAT_AGENT, "Invalid agent name length: %zu", name_len);
            free(req);
            pos = end + 1;
            continue;
        }

        req->agent_name = malloc(name_len + 1);
        if (!req->agent_name) {
            LOG_ERROR(LOG_CAT_AGENT, "Failed to allocate agent name");
            free(req);
            pos = end + 1;
            continue;
        }

        strncpy(req->agent_name, pos, name_len);
        req->agent_name[name_len] = '\0';

        // FIX: Trim BOTH leading and trailing spaces from agent name
        char* start = req->agent_name;
        while (*start == ' ') start++;  // Skip leading spaces

        char* trim = start + strlen(start) - 1;
        while (trim >= start && *trim == ' ')
            *trim-- = '\0';

        // If we trimmed leading spaces, shift the string
        if (start != req->agent_name) {
            memmove(req->agent_name, start, strlen(start) + 1);
        }

        // Validate not empty after trimming (from v7-plans)
        if (strlen(req->agent_name) == 0) {
            LOG_WARN(LOG_CAT_AGENT, "Empty agent name after trimming");
            free(req->agent_name);
            free(req);
            pos = end + 1;
            continue;
        }

        // Convert to lowercase for case-insensitive matching
        for (char* p = req->agent_name; *p; p++) {
            if (*p >= 'A' && *p <= 'Z') {
                *p = *p + ('a' - 'A');
            }
        }

        // Extract reason (until next [DELEGATE: or newline)
        pos = end + 1;
        while (*pos == ' ')
            pos++;

        const char* reason_end = strstr(pos, "[DELEGATE:");
        if (!reason_end) {
            // Find end of line or end of string
            reason_end = strchr(pos, '\n');
            if (!reason_end)
                reason_end = pos + strlen(pos);
        }

        if (reason_end > pos) {
            size_t reason_len = (size_t)(reason_end - pos);
            req->reason = malloc(reason_len + 1);
            if (req->reason) {
                strncpy(req->reason, pos, reason_len);
                req->reason[reason_len] = '\0';
                // Trim
                char* trim = req->reason + strlen(req->reason) - 1;
                while (trim > req->reason && (*trim == ' ' || *trim == '\n'))
                    *trim-- = '\0';
            }
        }

        // Add to list
        if (list->count >= list->capacity) {
            list->capacity *= 2;
            list->requests = realloc(list->requests, list->capacity * sizeof(DelegationRequest*));
        }
        list->requests[list->count++] = req;

        // DEBUG: Log parsed delegation
        LOG_INFO(LOG_CAT_AGENT, "Parsed delegation #%zu: agent='%s', reason='%.50s...'",
                 list->count, req->agent_name ? req->agent_name : "(null)",
                 req->reason ? req->reason : "(none)");

        pos = reason_end;
    }

    if (list->count == 0) {
        free(list->requests);
        free(list);
        return NULL;
    }

    return list;
}

void free_delegation_list(DelegationList* list) {
    if (!list)
        return;
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

// ============================================================================
// DELEGATION EXECUTION
// ============================================================================

// Execute delegated tasks in parallel and return synthesized response
char* execute_delegations(DelegationList* delegations, const char* user_input,
                          const char* ali_response, ManagedAgent* ali,
                          DelegationProgressCallback callback, void* user_data) {
    if (!delegations || delegations->count == 0 || !user_input || !ali) {
        LOG_ERROR(
            LOG_CAT_AGENT,
            "execute_delegations: invalid params (delegations=%p, count=%zu, input=%p, ali=%p)",
            (void*)delegations, delegations ? delegations->count : 0, (void*)user_input,
            (void*)ali);
        telemetry_record_error("orchestrator_delegation_invalid_params");
        return NULL;
    }

    // Helper macro for progress updates
    #define PROGRESS(fmt, ...) do { \
        char _progress_buf[512]; \
        snprintf(_progress_buf, sizeof(_progress_buf), fmt, ##__VA_ARGS__); \
        if (callback) callback(_progress_buf, user_data); \
        LOG_INFO(LOG_CAT_AGENT, "%s", _progress_buf); \
    } while(0)

    // Measure latency for telemetry
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Create workflow monitor for ASCII visualization (only for 2+ agents)
    __block WorkflowMonitor* monitor = NULL;
    if (delegations->count >= 2) {
        monitor = workflow_monitor_create("delegation", true);
        if (monitor) {
            workflow_monitor_start(monitor);
            // Pre-add all agents to monitor
            for (size_t i = 0; i < delegations->count; i++) {
                workflow_monitor_add_agent(monitor,
                                           delegations->requests[i]->agent_name,
                                           delegations->requests[i]->reason);
            }
            workflow_monitor_render(monitor);
        }
    }

    PROGRESS("\n🚀 **Delegating to %zu specialist agents...**\n", delegations->count);

    Orchestrator* orch = orchestrator_get();
    if (!orch) {
        PROGRESS("❌ Orchestrator not initialized!\n");
        return NULL;
    }

    // Initialize provider registry (required for delegation)
    provider_registry_init();

    // Prepare parallel execution
    AgentTask* tasks = calloc(delegations->count, sizeof(AgentTask));
    if (!tasks) {
        PROGRESS("❌ Failed to allocate tasks\n");
        return NULL;
    }

    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

    // Thread-safe counter for completed agents (using C11 atomics)
    __block _Atomic int completed_count = 0;
    __block _Atomic int failed_count = 0;
    const size_t total_agents = delegations->count;

    // Limit concurrent Ollama requests to prevent memory explosion
    // Ollama can only process one request at a time efficiently
    __block dispatch_semaphore_t ollama_semaphore = NULL;
    if (router_has_provider_override() && router_get_forced_provider() == PROVIDER_OLLAMA) {
        // Limit to 2 concurrent requests for Ollama (some batching is ok)
        ollama_semaphore = dispatch_semaphore_create(2);
        PROGRESS("  ℹ️  Ollama detected: limiting to 2 concurrent agents\n");
    }

    size_t agents_scheduled = 0;

    // Spawn all agent tasks in parallel
    for (size_t i = 0; i < delegations->count; i++) {
        DelegationRequest* req = delegations->requests[i];

        // Find or spawn the requested agent
        ManagedAgent* specialist = agent_find_by_name(req->agent_name);
        if (!specialist) {
            PROGRESS("  ⏳ Spawning agent %s...\n", req->agent_name);
            specialist = agent_spawn(AGENT_ROLE_ANALYST, req->agent_name, NULL);
        }

        if (!specialist) {
            PROGRESS("  ❌ Agent %s not found\n", req->agent_name);
            continue;
        }

        if (!specialist->system_prompt) {
            PROGRESS("  ❌ Agent %s has no system prompt\n", req->agent_name);
            continue;
        }

        PROGRESS("  💭 %s is thinking...\n", specialist->name);
        agents_scheduled++;

        // Agent is valid, set up task
        tasks[i].agent = specialist;
        tasks[i].user_input = user_input;
        tasks[i].context = req->reason;
        tasks[i].response = NULL;
        tasks[i].completed = false;

        // Create delegation message
        Message* delegate_msg =
            message_create(MSG_TYPE_TASK_DELEGATE, ali->id, specialist->id, user_input);
        if (delegate_msg) {
            message_send(delegate_msg);
        }

        // Capture values for block
        const size_t task_idx = i;
        const char* agent_name = specialist->name;

        // Execute in parallel (with optional concurrency limit for Ollama)
        dispatch_group_async(group, queue, ^{
            // Wait for semaphore if Ollama (limits concurrent requests)
            if (ollama_semaphore) {
                dispatch_semaphore_wait(ollama_semaphore, DISPATCH_TIME_FOREVER);
            }

            // Set agent as working
            agent_set_working(tasks[task_idx].agent, WORK_STATE_THINKING,
                              tasks[task_idx].context ? tasks[task_idx].context : "Analyzing request");

            // Update monitor status to thinking
            if (monitor) {
                workflow_monitor_set_status_by_name(monitor, agent_name, AGENT_STATUS_THINKING);
            }

            size_t prompt_size = strlen(tasks[task_idx].agent->system_prompt) +
                                 (tasks[task_idx].context ? strlen(tasks[task_idx].context) : 0) + 256;
            char* prompt_with_context = malloc(prompt_size);
            if (prompt_with_context) {
                snprintf(prompt_with_context, prompt_size, "%s\n\nContext from Ali: %s",
                         tasks[task_idx].agent->system_prompt,
                         tasks[task_idx].context ? tasks[task_idx].context : "Please analyze and respond.");

                // Use Provider interface for agent chat (respects --provider override)
                Provider* provider = NULL;
                const char* model = NULL;
                get_delegation_provider(&provider, &model);

                if (provider && provider->chat) {
                    TokenUsage usage = {0};
                    tasks[task_idx].response =
                        provider->chat(provider, model, prompt_with_context,
                                       tasks[task_idx].user_input, &usage);
                } else {
                    LOG_ERROR(LOG_CAT_AGENT, "Provider not available for agent '%s'", agent_name);
                }
                free(prompt_with_context);

                if (tasks[task_idx].response) {
                    cost_record_agent_usage(
                        tasks[task_idx].agent,
                        strlen(tasks[task_idx].agent->system_prompt) / 4 + strlen(tasks[task_idx].user_input) / 4,
                        strlen(tasks[task_idx].response) / 4);
                    tasks[task_idx].completed = true;
                    int count = atomic_fetch_add(&completed_count, 1) + 1;
                    LOG_INFO(LOG_CAT_AGENT, "Agent '%s' completed (%d/%zu)", agent_name,
                             count, total_agents);
                    // Update monitor status to completed
                    if (monitor) {
                        workflow_monitor_set_status_by_name(monitor, agent_name, AGENT_STATUS_COMPLETED);
                    }
                } else {
                    atomic_fetch_add(&failed_count, 1);
                    LOG_ERROR(LOG_CAT_AGENT, "Agent '%s' failed", agent_name);
                    // Update monitor status to failed
                    if (monitor) {
                        workflow_monitor_set_status_by_name(monitor, agent_name, AGENT_STATUS_FAILED);
                    }
                }
            } else {
                atomic_fetch_add(&failed_count, 1);
                // Update monitor status to failed
                if (monitor) {
                    workflow_monitor_set_status_by_name(monitor, agent_name, AGENT_STATUS_FAILED);
                }
            }

            // Set agent back to idle
            agent_set_idle(tasks[task_idx].agent);

            // Signal semaphore to allow next agent (if using Ollama)
            if (ollama_semaphore) {
                dispatch_semaphore_signal(ollama_semaphore);
            }
        });
    }

    if (agents_scheduled == 0) {
        PROGRESS("❌ No agents could be scheduled!\n");
        free(tasks);
        if (monitor) {
            workflow_monitor_free(monitor);
        }
        return NULL;
    }

    PROGRESS("\n⏳ Waiting for %zu agents to complete...\n", agents_scheduled);

    // Poll for completion with progress updates and monitor rendering
    long timeout_ms = 500; // Check every 500ms
    int last_completed = 0;
    int last_rendered = -1;  // Track last rendered state to avoid excessive rendering
    while (dispatch_group_wait(group, dispatch_time(DISPATCH_TIME_NOW, timeout_ms * NSEC_PER_MSEC)) != 0) {
        int current = completed_count + failed_count;
        // Only render when there's actual progress, not on every poll
        if (current > last_completed) {
            if (monitor) {
                workflow_monitor_render(monitor);
            }
            PROGRESS("  📊 Progress: %d/%zu agents done\n", current, agents_scheduled);
            last_completed = current;
            last_rendered = current;
        } else if (monitor && last_rendered < 0) {
            // Initial render once when first entering the wait loop
            workflow_monitor_render(monitor);
            last_rendered = 0;
        }
    }

    // Final render and summary
    if (monitor) {
        workflow_monitor_stop(monitor);
        workflow_monitor_render(monitor);
        workflow_monitor_render_summary(monitor);
    }

    PROGRESS("✅ All agents completed! (%d succeeded, %d failed)\n", completed_count, failed_count);

    // Build convergence prompt with all responses
    size_t convergence_size = (ali_response ? strlen(ali_response) : 0) + 4096;
    for (size_t i = 0; i < delegations->count; i++) {
        if (tasks[i].response) {
            convergence_size += strlen(tasks[i].response) + 256;
        }
    }

    char* convergence_prompt = malloc(convergence_size);
    if (!convergence_prompt) {
        // Cleanup
        for (size_t i = 0; i < delegations->count; i++) {
            free(tasks[i].response);
        }
        free(tasks);
        if (monitor) {
            workflow_monitor_free(monitor);
        }
        return NULL;
    }

    size_t offset =
        (size_t)snprintf(convergence_prompt, convergence_size,
                         "You delegated to %zu specialist agents. Here are their responses:\n\n",
                         delegations->count);

    for (size_t i = 0; i < delegations->count; i++) {
        if (tasks[i].completed && tasks[i].response) {
            offset += (size_t)snprintf(
                convergence_prompt + offset, convergence_size - offset, "## %s's Response\n%s\n\n",
                tasks[i].agent ? tasks[i].agent->name : "Agent", tasks[i].response);
        }
    }

    offset += (size_t)snprintf(
        convergence_prompt + offset, convergence_size - offset,
        "---\n\nOriginal user request: %s\n\n"
        "Please synthesize all these specialist perspectives into a unified, comprehensive "
        "response for the user. "
        "Integrate insights from each agent, highlight agreements and different viewpoints, "
        "and provide actionable conclusions.",
        user_input);

    // Ali synthesizes all responses using Provider interface (respects --provider override)
    PROGRESS("\n🔄 Ali is synthesizing responses...\n");

    char* synthesized = NULL;
    Provider* synth_provider = NULL;
    const char* synth_model = NULL;
    get_delegation_provider(&synth_provider, &synth_model);

    if (synth_provider && synth_provider->chat) {
        TokenUsage synth_usage = {0};
        synthesized = synth_provider->chat(synth_provider, synth_model, ali->system_prompt,
                                           convergence_prompt, &synth_usage);
    } else {
        PROGRESS("❌ Provider not available for synthesis\n");
    }
    free(convergence_prompt);

    // Calculate latency and record telemetry
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double latency_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000.0);

    if (synthesized) {
        cost_record_agent_usage(ali, 1000, strlen(synthesized) / 4);
        telemetry_record_api_call("orchestrator", "delegation", delegations->count,
                                  strlen(synthesized) / 4, latency_ms);
        PROGRESS("✅ Synthesis complete (%.1fs total)\n\n", latency_ms / 1000.0);
    } else {
        telemetry_record_error("orchestrator_delegation_failed");
        PROGRESS("❌ Synthesis failed\n");
    }

    // Cleanup
    for (size_t i = 0; i < delegations->count; i++) {
        free(tasks[i].response);
    }
    free(tasks);

    // Free workflow monitor
    if (monitor) {
        workflow_monitor_free(monitor);
    }

    #undef PROGRESS
    return synthesized;
}
