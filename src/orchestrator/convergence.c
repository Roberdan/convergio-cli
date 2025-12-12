/**
 * CONVERGIO CONVERGENCE
 *
 * Response convergence and synthesis from multiple agents
 */

#include "nous/convergence.h"
#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External functions
extern char* nous_claude_chat(const char* system_prompt, const char* user_message);
extern Orchestrator* orchestrator_get(void);

// ============================================================================
// CONVERGENCE
// ============================================================================

// Converge results from multiple agents into unified response
char* orchestrator_converge(ExecutionPlan* plan) {
    Orchestrator* orch = orchestrator_get();
    if (!orch || !plan) return NULL;

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
            for (size_t i = 0; i < orch->agent_count; i++) {
                if (orch->agents[i]->id == task->assigned_to) {
                    agent = orch->agents[i];
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
