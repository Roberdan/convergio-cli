/**
 * TEST STUBS
 *
 * Provides stub implementations of global variables and functions
 * that are needed by test binaries but not provided by main.c
 */

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Global state stubs (normally defined in main.c)
volatile sig_atomic_t g_running = 1;
void* g_current_space = NULL;
void* g_assistant = NULL;
bool g_streaming_enabled = false;

// Provider stubs for education tests
typedef struct Provider Provider;
Provider* provider_get(int provider_type) {
    (void)provider_type;
    return NULL;  // Return NULL so llm_generate uses fallback
}

// LLM generate stub for education tests (overrides weak symbol)
char* llm_generate(const char* prompt, const char* system_prompt) {
    (void)system_prompt;
    if (!prompt) return NULL;
    // Return a test response
    return strdup("[Test Mode] LLM risposta di test per: prompt ricevuto");
}
