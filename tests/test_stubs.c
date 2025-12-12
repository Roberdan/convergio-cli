/**
 * TEST STUBS
 *
 * Provides stub implementations of global variables and functions
 * that are needed by test binaries but not provided by main.c
 */

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

// Global state stubs (normally defined in main.c)
volatile sig_atomic_t g_running = 1;
void* g_current_space = NULL;
void* g_assistant = NULL;
bool g_streaming_enabled = false;
