/**
 * CONVERGIO KERNEL - Signal Handling
 *
 * Unix signal handlers for graceful shutdown
 *
 * Signal behavior:
 * - CTRL+C during streaming: cancels the stream, returns to prompt
 * - CTRL+C at prompt: exits Convergio
 * - CTRL+C twice rapidly: force exit
 */

#ifndef NOUS_SIGNALS_H
#define NOUS_SIGNALS_H

#include <signal.h>
#include <stdbool.h>

// ============================================================================
// SIGNAL HANDLING
// ============================================================================

// Initialize signal handlers (SIGINT, SIGTERM, SIGSEGV)
void signals_init(void);

// Signal handler function
void signal_handler(int sig);

// FIX-06: Set cleanup callback for graceful shutdown
void signals_set_cleanup_callback(void (*callback)(void));

// FIX-07: Crash recovery marker functions
void signals_set_crash_marker(const char* data_dir);
void signals_clear_crash_marker(void);
bool signals_check_previous_crash(const char* data_dir);

// ============================================================================
// STREAM CANCELLATION
// ============================================================================

// Global flag to cancel streaming operations
extern volatile sig_atomic_t g_stream_cancelled;

// Check if stream should be cancelled
bool stream_should_cancel(void);

// Reset stream cancellation flag
void stream_reset_cancel(void);

// Set that streaming is in progress (affects CTRL+C behavior)
void stream_set_active(bool active);

// Check if streaming is active
bool stream_is_active(void);

#endif // NOUS_SIGNALS_H
