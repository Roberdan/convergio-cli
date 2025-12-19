/**
 * ACP STUBS
 *
 * Provides stub implementations of globals and functions
 * that are needed by convergio-acp but defined in main.c
 */

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include "nous/nous.h"

// Global state stubs (normally defined in main.c)
volatile sig_atomic_t g_running = 1;
void* g_current_space = NULL;
void* g_assistant = NULL;
bool g_streaming_enabled = false;

// Log level (normally defined in main.c)
LogLevel g_log_level = LOG_LEVEL_NONE;

// Logging stubs
void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    // Silent in ACP mode - could optionally log to stderr
    (void)level;
    (void)cat;
    (void)fmt;
}

void nous_log_set_level(LogLevel level) {
    g_log_level = level;
}

LogLevel nous_log_get_level(void) {
    return g_log_level;
}

const char* nous_log_level_name(LogLevel level) {
    static const char* names[] = {"NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
    if (level <= LOG_LEVEL_TRACE) return names[level];
    return "UNKNOWN";
}
