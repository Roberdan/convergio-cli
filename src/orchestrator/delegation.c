/**
 * CONVERGIO DELEGATION
 *
 * Agent delegation logic - handles routing tasks to specialist agents
 */

#include "nous/delegation.h"
#include "nous/orchestrator.h"
#include "nous/nous.h"
#include "nous/projects.h"
#include "nous/provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dispatch/dispatch.h>

// Model used for agent delegation (matches previous claude.c default)
#define DELEGATION_MODEL "claude-sonnet-4-20250514"

// External functions from orchestrator
extern int persistence_save_conversation(const char* session_id, const char* role,
                                          const char* content, int tokens);
extern void agent_set_working(ManagedAgent* agent, AgentWorkState state, const char* task);
extern void agent_set_idle(ManagedAgent* agent);
extern ManagedAgent* agent_find_by_name(const char* name);
extern ManagedAgent* agent_spawn(AgentRole role, const char* name, const char* context);
extern void cost_record_agent_usage(ManagedAgent* agent, uint64_t input_tokens, uint64_t output_tokens);

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

        size_t name_len = (size_t)(end - pos);
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
            size_t reason_len = (size_t)(reason_end - pos);
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

void free_delegation_list(DelegationList* list) {
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

// ============================================================================
// DELEGATION EXECUTION
// ============================================================================

// Execute delegated tasks in parallel and return synthesized response
char* execute_delegations(DelegationList* delegations, const char* user_input,
                          const char* ali_response, ManagedAgent* ali) {
    if (!delegations || delegations->count == 0 || !user_input || !ali) {
        return NULL;
    }

    // Prepare parallel execution
    AgentTask* tasks = calloc(delegations->count, sizeof(AgentTask));
    dispatch_group_t group = dispatch_group_create();
    dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);

    // Spawn all agent tasks in parallel
    for (size_t i = 0; i < delegations->count; i++) {
        DelegationRequest* req = delegations->requests[i];

        // Check if agent is in current project team
        if (!project_has_agent(req->agent_name)) {
            ConvergioProject* proj = project_current();
            LOG_WARN(LOG_CAT_AGENT, "Agent '%s' not in project team '%s', skipping",
                     req->agent_name, proj ? proj->name : "none");
            tasks[i].agent = NULL;
            tasks[i].completed = true;  // Mark as done (skipped)
            continue;
        }

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
                                                    ali->id,
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

                    // Use Provider interface for agent chat
                    Provider* provider = provider_get(PROVIDER_ANTHROPIC);
                    if (provider && provider->chat) {
                        TokenUsage usage = {0};
                        tasks[i].response = provider->chat(
                            provider,
                            DELEGATION_MODEL,
                            prompt_with_context,
                            tasks[i].user_input,
                            &usage
                        );
                    }
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

    size_t offset = (size_t)snprintf(convergence_prompt, convergence_size,
        "You delegated to %zu specialist agents. Here are their responses:\n\n",
        delegations->count);

    for (size_t i = 0; i < delegations->count; i++) {
        if (tasks[i].completed && tasks[i].response) {
            offset += (size_t)snprintf(convergence_prompt + offset, convergence_size - offset,
                "## %s's Response\n%s\n\n",
                tasks[i].agent ? tasks[i].agent->name : "Agent",
                tasks[i].response);
        }
    }

    offset += (size_t)snprintf(convergence_prompt + offset, convergence_size - offset,
        "---\n\nOriginal user request: %s\n\n"
        "Please synthesize all these specialist perspectives into a unified, comprehensive response for the user. "
        "Integrate insights from each agent, highlight agreements and different viewpoints, "
        "and provide actionable conclusions.",
        user_input);

    // Ali synthesizes all responses using Provider interface
    char* synthesized = NULL;
    Provider* synth_provider = provider_get(PROVIDER_ANTHROPIC);
    if (synth_provider && synth_provider->chat) {
        TokenUsage synth_usage = {0};
        synthesized = synth_provider->chat(
            synth_provider,
            DELEGATION_MODEL,
            ali->system_prompt,
            convergence_prompt,
            &synth_usage
        );
    }
    free(convergence_prompt);

    if (synthesized) {
        cost_record_agent_usage(ali, 1000, strlen(synthesized) / 4);
    }

    // Cleanup
    for (size_t i = 0; i < delegations->count; i++) {
        free(tasks[i].response);
    }
    free(tasks);

    return synthesized;
}
