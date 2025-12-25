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

// ============================================================================
// SWIFT AFM FUNCTION STUBS
// These are needed for tests that link against apple_foundation.o
// The real implementations are in the Swift AFM library
// ============================================================================

#include <stdint.h>

// Swift AFM is not available in test environment, return error codes
int32_t swift_afm_check_availability(
    bool* outIsAvailable,
    bool* outIntelligenceEnabled,
    bool* outModelReady
) {
    if (outIsAvailable) *outIsAvailable = false;
    if (outIntelligenceEnabled) *outIntelligenceEnabled = false;
    if (outModelReady) *outModelReady = false;
    return -1;  // AFM_ERR_NOT_MACOS_26
}

int32_t swift_afm_session_create(int64_t* outSessionId) {
    if (outSessionId) *outSessionId = 0;
    return -5;  // AFM_ERR_SESSION_FAILED
}

void swift_afm_session_destroy(int64_t sessionId) {
    (void)sessionId;
}

int32_t swift_afm_session_set_instructions(int64_t sessionId, const char* instructions) {
    (void)sessionId;
    (void)instructions;
    return -5;  // AFM_ERR_SESSION_FAILED
}

int32_t swift_afm_generate(int64_t sessionId, const char* prompt, char** outResponse) {
    (void)sessionId;
    (void)prompt;
    if (outResponse) *outResponse = NULL;
    return -6;  // AFM_ERR_GENERATION_FAILED
}

typedef void (*AFMStreamCallback)(const char*, bool, void*);

int32_t swift_afm_generate_stream(
    int64_t sessionId,
    const char* prompt,
    AFMStreamCallback callback,
    void* userCtx
) {
    (void)sessionId;
    (void)prompt;
    (void)callback;
    (void)userCtx;
    return -6;  // AFM_ERR_GENERATION_FAILED
}

void swift_afm_free_string(char* str) {
    (void)str;
}

int32_t swift_afm_get_model_info(char** outName, float* outSizeBillions) {
    if (outName) *outName = NULL;
    if (outSizeBillions) *outSizeBillions = 0.0f;
    return -100;  // AFM_ERR_UNKNOWN
}

// ============================================================================
// EDUCATION TEST STUBS
// ============================================================================

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

// ============================================================================
// MESSAGE BUS STUBS FOR TESTS
// ============================================================================

// Stub for message_send - do NOT destroy message since tests store them in history
// This fixes the sanitizer issue where message_send would free memory still in use
__attribute__((weak))
void message_send(void* msg) {
    (void)msg;
    // In tests, we don't actually send messages - just keep them in memory
    // The original implementation would destroy msg if orchestrator is NULL,
    // causing use-after-free when tests access message_history
}
