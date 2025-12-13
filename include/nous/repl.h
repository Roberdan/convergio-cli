/**
 * CONVERGIO KERNEL - REPL Interface
 *
 * Read-Eval-Print Loop implementation
 */

#ifndef NOUS_REPL_H
#define NOUS_REPL_H

#include <stdbool.h>

// Forward declaration
struct ManagedAgent;
typedef struct ManagedAgent ManagedAgent;

// ============================================================================
// CURRENT AGENT CONTEXT
// ============================================================================

// Get the current agent in conversation (NULL = Ali)
ManagedAgent* repl_get_current_agent(void);

// Set the current agent for conversation continuity
void repl_set_current_agent(ManagedAgent* agent);

// Clear current agent (return to Ali)
void repl_clear_current_agent(void);

// ============================================================================
// REPL FUNCTIONS
// ============================================================================

// Parse and execute a command or natural language input
int repl_parse_and_execute(char* line);

// Process natural language input through orchestrator
int repl_process_natural_input(const char* input);

// Direct communication with a specific agent
int repl_direct_agent_communication(const char* agent_name, const char* message);

// ============================================================================
// UI HELPERS
// ============================================================================

// Print visual separator
void repl_print_separator(void);

// Spinner control (for showing progress during API calls)
void repl_spinner_start(void);
void repl_spinner_stop(void);
bool repl_spinner_was_cancelled(void);

// ============================================================================
// READLINE COMPLETION
// ============================================================================

// Setup readline completion for agent names
char** repl_agent_completion(const char* text, int start, int end);

// Clipboard image paste handler (Ctrl+I)
int repl_paste_clipboard_image(int count, int key);

// ============================================================================
// BUDGET HANDLING
// ============================================================================

// Handle budget exceeded interactively
// Returns: true if user wants to continue (budget increased), false to abort
bool repl_handle_budget_exceeded(void);

#endif // NOUS_REPL_H
