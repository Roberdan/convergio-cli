/**
 * CONVERGIO KERNEL - Logging Implementation
 *
 * Centralized logging with levels and categories
 * Extracted from main.c for better separation of concerns
 */

#include "nous/nous.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

LogLevel g_log_level = LOG_LEVEL_NONE;

// ============================================================================
// LEVEL AND CATEGORY NAMES
// ============================================================================

static const char* LOG_LEVEL_NAMES[] = {
    "NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
};

static const char* LOG_CAT_NAMES[] = {
    "SYSTEM", "AGENT", "TOOL", "API", "MEMORY", "MSGBUS", "COST", "WORKFLOW"
};

static const char* LOG_CAT_COLORS[] = {
    "\033[36m",   // Cyan - SYSTEM
    "\033[33m",   // Yellow - AGENT
    "\033[32m",   // Green - TOOL
    "\033[35m",   // Magenta - API
    "\033[34m",   // Blue - MEMORY
    "\033[37m",   // White - MSGBUS
    "\033[31m",   // Red - COST
    "\033[93m"    // Bright Yellow - WORKFLOW
};

// ============================================================================
// LOGGING IMPLEMENTATION
// ============================================================================

void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    if (level > g_log_level || g_log_level == LOG_LEVEL_NONE) return;

    // Timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    // Level indicator
    const char* level_color;
    switch (level) {
        case LOG_LEVEL_ERROR: level_color = "\033[31m"; break;  // Red
        case LOG_LEVEL_WARN:  level_color = "\033[33m"; break;  // Yellow
        case LOG_LEVEL_INFO:  level_color = "\033[32m"; break;  // Green
        case LOG_LEVEL_DEBUG: level_color = "\033[36m"; break;  // Cyan
        case LOG_LEVEL_TRACE: level_color = "\033[2m"; break;   // Dim
        default: level_color = "\033[0m";
    }

    // Print header
    fprintf(stderr, "\033[2m[%s]\033[0m %s[%-5s]\033[0m %s[%s]\033[0m ",
            time_str,
            level_color, LOG_LEVEL_NAMES[level],
            LOG_CAT_COLORS[cat], LOG_CAT_NAMES[cat]);

    // Print message
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\033[0m\n");
}

void nous_log_set_level(LogLevel level) {
    g_log_level = level;
}

LogLevel nous_log_get_level(void) {
    return g_log_level;
}

const char* nous_log_level_name(LogLevel level) {
    if (level <= LOG_LEVEL_TRACE) return LOG_LEVEL_NAMES[level];
    return "UNKNOWN";
}
