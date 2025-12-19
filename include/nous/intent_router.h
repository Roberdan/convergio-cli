/**
 * CONVERGIO INTENT ROUTER
 *
 * Intelligent routing using fast LLM (Haiku) to understand user intent
 */

#ifndef NOUS_INTENT_ROUTER_H
#define NOUS_INTENT_ROUTER_H

#include <stdbool.h>

// Intent types
typedef enum {
    INTENT_MESSAGE,      // Normal message to agent
    INTENT_SWITCH,       // Switch to different agent (no message)
    INTENT_DELEGATE      // Delegate task to agent
} IntentType;

// Result of intent routing
typedef struct {
    char agent[64];      // Target agent name
    float confidence;    // 0.0-1.0 confidence level
    char intent[128];    // Brief intent description
    bool used_llm;       // Whether LLM was called (vs pattern match)
    IntentType type;     // Type of intent (switch, message, delegate)
} RouterResult;

// Route user input to appropriate agent
// Returns RouterResult with agent name and confidence
// Uses caching and pattern matching before falling back to LLM
RouterResult intent_router_route(const char* user_input);

// Cleanup router resources (cache)
void intent_router_cleanup(void);

#endif // NOUS_INTENT_ROUTER_H
