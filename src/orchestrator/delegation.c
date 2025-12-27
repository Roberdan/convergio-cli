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
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Model used for agent delegation (matches previous claude.c default)
#define DELEGATION_MODEL "claude-sonnet-4-20250514"

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
                          const char* ali_response, ManagedAgent* ali) {
    if (!delegations || delegations->count == 0 || !user_input || !ali) {
        LOG_ERROR(
            LOG_CAT_AGENT,
            "execute_delegations: invalid params (delegations=%p, count=%zu, input=%p, ali=%p)",
            (void*)delegations, delegations ? delegations->count : 0, (void*)user_input,
            (void*)ali);
        telemetry_record_error("orchestrator_delegation_invalid_params");
        return NULL;
    }

    // Measure latency for telemetry
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    LOG_INFO(LOG_CAT_AGENT, "Starting delegation to %zu agents", delegations->count);
    fprintf(stderr, "\n[DELEGATION DEBUG] Starting delegation to %zu agents\n", delegations->count);

    // DEBUG: Show total agents in registry
    Orchestrator* orch = orchestrator_get();
    if (orch) {
        LOG_INFO(LOG_CAT_AGENT, "[DELEGATION] Registry has %zu agents loaded", orch->agent_count);
        fprintf(stderr, "[DELEGATION DEBUG] Registry has %zu agents loaded\n", orch->agent_count);
    } else {
        LOG_ERROR(LOG_CAT_AGENT, "[DELEGATION] Orchestrator not initialized!");
        fprintf(stderr, "[DELEGATION DEBUG] ERROR: Orchestrator not initialized!\n");
    }

    // DEBUG: List all delegation requests
    for (size_t i = 0; i < delegations->count; i++) {
        LOG_INFO(LOG_CAT_AGENT, "[DELEGATION DEBUG] Request %zu: agent='%s'",
                 i, delegations->requests[i]->agent_name ? delegations->requests[i]->agent_name : "(null)");
    }

    // Prepare parallel execution
    AgentTask* tasks = calloc(delegations->count, sizeof(AgentTask));
    if (!tasks) {
        LOG_ERROR(LOG_CAT_AGENT, "Failed to allocate tasks array for %zu delegations", delegations->count);
        return NULL;
    }

    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

    size_t agents_scheduled = 0;

    // Spawn all agent tasks in parallel
    for (size_t i = 0; i < delegations->count; i++) {
        DelegationRequest* req = delegations->requests[i];

        // Check if agent is in current project team - warn but allow delegation
        // Ali can delegate to anyone, project team is just a recommendation
        if (!project_has_agent(req->agent_name)) {
            ConvergioProject* proj = project_current();
            LOG_INFO(LOG_CAT_AGENT, "Agent '%s' not in project team '%s', but allowing delegation",
                     req->agent_name, proj ? proj->name : "none");
            // Don't skip - Ali can delegate to any agent
        }

        // Find or spawn the requested agent
        fprintf(stderr, "[DELEGATION DEBUG] Looking for agent '%s'...\n", req->agent_name);
        ManagedAgent* specialist = agent_find_by_name(req->agent_name);
        if (!specialist) {
            LOG_INFO(LOG_CAT_AGENT, "Agent '%s' not found, spawning new instance", req->agent_name);
            fprintf(stderr, "[DELEGATION DEBUG] Agent '%s' not found in registry, spawning...\n", req->agent_name);
            specialist = agent_spawn(AGENT_ROLE_ANALYST, req->agent_name, NULL);
        } else {
            fprintf(stderr, "[DELEGATION DEBUG] Found agent '%s' -> '%s'\n", req->agent_name, specialist->name);
        }

        if (!specialist) {
            LOG_ERROR(LOG_CAT_AGENT, "Failed to find or spawn agent '%s'", req->agent_name);
            fprintf(stderr, "[DELEGATION DEBUG] ERROR: Failed to spawn agent '%s'\n", req->agent_name);
            continue;
        }

        if (!specialist->system_prompt) {
            LOG_ERROR(LOG_CAT_AGENT, "Agent '%s' has no system prompt", req->agent_name);
            fprintf(stderr, "[DELEGATION DEBUG] ERROR: Agent '%s' has no system prompt\n", req->agent_name);
            continue;
        }

        LOG_INFO(LOG_CAT_AGENT, "Delegating to agent '%s'", specialist->name);
        fprintf(stderr, "[DELEGATION DEBUG] Scheduling agent '%s' for parallel execution\n", specialist->name);
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

        // FIX: Capture loop index by value to avoid async closure bug
        // Without this, all blocks would reference the same (final) value of i
        const size_t task_idx = i;

        // Execute in parallel
        dispatch_group_async(group, queue, ^{
            // Set agent as working
            agent_set_working(tasks[task_idx].agent, WORK_STATE_THINKING,
                              tasks[task_idx].context ? tasks[task_idx].context : "Analyzing request");

            size_t prompt_size = strlen(tasks[task_idx].agent->system_prompt) +
                                 (tasks[task_idx].context ? strlen(tasks[task_idx].context) : 0) + 256;
            char* prompt_with_context = malloc(prompt_size);
            if (prompt_with_context) {
                snprintf(prompt_with_context, prompt_size, "%s\n\nContext from Ali: %s",
                         tasks[task_idx].agent->system_prompt,
                         tasks[task_idx].context ? tasks[task_idx].context : "Please analyze and respond.");

                // Use Provider interface for agent chat
                Provider* provider = provider_get(PROVIDER_ANTHROPIC);
                if (provider && provider->chat) {
                    TokenUsage usage = {0};
                    tasks[task_idx].response =
                        provider->chat(provider, DELEGATION_MODEL, prompt_with_context,
                                       tasks[task_idx].user_input, &usage);
                } else {
                    LOG_ERROR(LOG_CAT_AGENT, "Provider not available for agent '%s'",
                              tasks[task_idx].agent->name);
                }
                free(prompt_with_context);

                if (tasks[task_idx].response) {
                    cost_record_agent_usage(
                        tasks[task_idx].agent,
                        strlen(tasks[task_idx].agent->system_prompt) / 4 + strlen(tasks[task_idx].user_input) / 4,
                        strlen(tasks[task_idx].response) / 4);
                    tasks[task_idx].completed = true;
                    LOG_INFO(LOG_CAT_AGENT, "Agent '%s' completed response", tasks[task_idx].agent->name);
                } else {
                    LOG_ERROR(LOG_CAT_AGENT, "Agent '%s' returned empty response",
                              tasks[task_idx].agent->name);
                }
            }

            // Set agent back to idle
            agent_set_idle(tasks[task_idx].agent);
        });
    }

    // Summary of scheduled agents
    LOG_INFO(LOG_CAT_AGENT, "[DELEGATION] Scheduled %zu/%zu agents for parallel execution",
             agents_scheduled, delegations->count);
    fprintf(stderr, "[DELEGATION DEBUG] Scheduled %zu/%zu agents for parallel execution\n",
            agents_scheduled, delegations->count);

    if (agents_scheduled == 0) {
        LOG_ERROR(LOG_CAT_AGENT, "[DELEGATION] No agents were scheduled! Check agent names and registry.");
        fprintf(stderr, "[DELEGATION DEBUG] ERROR: No agents were scheduled! Returning NULL.\n");
        free(tasks);
        return NULL;
    }

    fprintf(stderr, "[DELEGATION DEBUG] Waiting for agents to complete...\n");

    // Wait for all agents to complete
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

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

    // Ali synthesizes all responses using Provider interface
    char* synthesized = NULL;
    Provider* synth_provider = provider_get(PROVIDER_ANTHROPIC);
    if (synth_provider && synth_provider->chat) {
        TokenUsage synth_usage = {0};
        synthesized = synth_provider->chat(synth_provider, DELEGATION_MODEL, ali->system_prompt,
                                           convergence_prompt, &synth_usage);
    }
    free(convergence_prompt);

    if (synthesized) {
        cost_record_agent_usage(ali, 1000, strlen(synthesized) / 4);
    }

    // Calculate latency and record telemetry
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double latency_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000.0);

    if (synthesized) {
        // Record successful delegation as API call (orchestrator internal operation)
        telemetry_record_api_call("orchestrator", "delegation", delegations->count,
                                  strlen(synthesized) / 4, latency_ms);
        LOG_DEBUG(LOG_CAT_AGENT, "Delegation completed in %.2f ms", latency_ms);
    } else {
        telemetry_record_error("orchestrator_delegation_failed");
        LOG_ERROR(LOG_CAT_AGENT, "Delegation failed - synthesis returned NULL");
    }

    // Cleanup
    for (size_t i = 0; i < delegations->count; i++) {
        free(tasks[i].response);
    }
    free(tasks);

    return synthesized;
}
