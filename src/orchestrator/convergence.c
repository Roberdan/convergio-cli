/**
 * CONVERGIO CONVERGENCE
 *
 * Response convergence and synthesis from multiple agents
 */

#include "nous/convergence.h"
#include "nous/orchestrator.h"
#include "nous/provider.h"
#include "nous/telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Model used for orchestrator convergence (matches previous claude.c default)
#define ORCHESTRATOR_MODEL "claude-sonnet-4-20250514"

// External functions
extern Orchestrator* orchestrator_get(void);

// ============================================================================
// CONVERGENCE
// ============================================================================

// Converge results from multiple agents into unified response
char* orchestrator_converge(ExecutionPlan* plan) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !plan) {
        telemetry_record_error("orchestrator_convergence_invalid_params");
        return NULL;
    }

    // Measure latency for telemetry
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Collect all task results
    size_t buf_size = 8192;
    char* combined = malloc(buf_size);
    if (!combined) {
        telemetry_record_error("orchestrator_convergence_alloc_failed");
        return NULL;
    }

    size_t offset = (size_t)snprintf(combined, buf_size,
        "Synthesize the following results into a unified response:\n\nGoal: %s\n\n",
        plan->goal);

    Task* task = plan->tasks;
    while (task && offset < buf_size - 512) {
        if (task->status == TASK_STATUS_COMPLETED && task->result) {
            ManagedAgent* agent = NULL;
            for (size_t i = 0; i < orch->agent_count; i++) {
                if (orch->agents[i]->id == task->assigned_to) {
                    agent = orch->agents[i];
                    break;
                }
            }

            offset += (size_t)snprintf(combined + offset, buf_size - offset,
                "## %s's Analysis\n%s\n\n",
                agent ? agent->name : "Agent",
                task->result);
        }
        task = task->next;
    }

    // Ask Ali to synthesize using Provider interface
    char* final = NULL;
    Provider* provider = provider_get(PROVIDER_ANTHROPIC);
    if (provider && provider->chat) {
        TokenUsage usage = {0};
        final = provider->chat(
            provider,
            ORCHESTRATOR_MODEL,
            "You are Ali. Synthesize the following multi-agent analysis into a clear, actionable response.",
            combined,
            &usage
        );
    }

    free(combined);

    // Calculate latency and record telemetry
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double latency_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000.0);

    if (final) {
        plan->final_result = strdup(final);
        plan->is_complete = true;
        // Record successful convergence
        telemetry_record_api_call("orchestrator", "convergence", 0, strlen(final) / 4, latency_ms);
        LOG_DEBUG(LOG_CAT_AGENT, "Convergence completed in %.2f ms", latency_ms);
    } else {
        telemetry_record_error("orchestrator_convergence_failed");
        LOG_ERROR(LOG_CAT_AGENT, "Convergence failed - provider returned NULL");
    }

    return final;
}
