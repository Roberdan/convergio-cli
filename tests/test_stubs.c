/**
 * TEST STUBS
 *
 * Provides stub implementations of global variables and functions
 * that are needed by test binaries but not provided by main.c
 */

#include "nous/nous.h"
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Global state stubs (normally defined in main.c)
volatile sig_atomic_t g_running = 1;
void* g_current_space = NULL;
void* g_assistant = NULL;
bool g_streaming_enabled = false;

// Log level for stub (global, matches extern in nous.h)
// Use weak symbols to avoid conflicts with other test files
__attribute__((weak))
LogLevel g_log_level = LOG_LEVEL_INFO;

// nous_log stub - required by many modules
// Use weak symbols so test files can override with their own implementations
__attribute__((weak))
void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    (void)level; (void)cat; (void)fmt;
    // Silent in tests
}

__attribute__((weak))
void nous_log_set_level(LogLevel level) { g_log_level = level; }

__attribute__((weak))
LogLevel nous_log_get_level(void) { return g_log_level; }

__attribute__((weak))
const char* nous_log_level_name(LogLevel level) {
    (void)level;
    return "INFO";
}

// Provider stubs for education tests - use weak symbol to avoid duplicate
typedef struct Provider Provider;
__attribute__((weak))
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
