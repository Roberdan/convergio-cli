/**
 * CONVERGIO CONVERGENCE
 *
 * Response convergence and synthesis from multiple agents
 */

#ifndef CONVERGIO_CONVERGENCE_H
#define CONVERGIO_CONVERGENCE_H

#include "nous/orchestrator.h"

// ============================================================================
// CONVERGENCE API
// ============================================================================

// Converge results from multiple agents into unified response
char* orchestrator_converge(ExecutionPlan* plan);

#endif // CONVERGIO_CONVERGENCE_H
