/**
 * CONVERGIO GLOBALS
 *
 * Extern declarations for global state variables.
 * These are defined in main.c but used across modules.
 */

#ifndef CONVERGIO_GLOBALS_H
#define CONVERGIO_GLOBALS_H

#include <signal.h>
#include <stdbool.h>

// Global running flag (for signal handlers)
extern volatile sig_atomic_t g_running;

// Current space pointer (void* to avoid circular deps)
extern void* g_current_space;

// Current assistant agent pointer
extern void* g_assistant;

// Streaming output enabled flag
extern bool g_streaming_enabled;

#endif // CONVERGIO_GLOBALS_H
