/**
 * CONVERGIO DELEGATION
 *
 * Agent delegation logic - handles routing tasks to specialist agents
 * and managing parallel execution
 */

#ifndef CONVERGIO_DELEGATION_H
#define CONVERGIO_DELEGATION_H

#include "nous/orchestrator.h"
#include <stddef.h>

// ============================================================================
// DELEGATION STRUCTURES
// ============================================================================

typedef struct {
    char* agent_name;
    char* reason;
} DelegationRequest;

typedef struct {
    DelegationRequest** requests;
    size_t count;
    size_t capacity;
} DelegationList;

// ============================================================================
// DELEGATION API
// ============================================================================

// Parse ALL delegation requests from response
DelegationList* parse_all_delegations(const char* response);

// Free delegation list
void free_delegation_list(DelegationList* list);

// Progress callback type for delegation updates
typedef void (*DelegationProgressCallback)(const char* message, void* user_data);

// Execute delegated tasks in parallel and return synthesized response
// callback: optional callback for progress updates (can be NULL)
// user_data: passed to callback
char* execute_delegations(DelegationList* delegations, const char* user_input,
                          const char* ali_response, ManagedAgent* ali,
                          DelegationProgressCallback callback, void* user_data);

#endif // CONVERGIO_DELEGATION_H
