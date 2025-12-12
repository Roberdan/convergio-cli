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

#include "nous/signals.h"
#include "nous/commands.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// STREAM STATE
// ============================================================================

volatile sig_atomic_t g_stream_cancelled = 0;
static volatile sig_atomic_t g_stream_active = 0;
static volatile time_t g_last_sigint = 0;

// ============================================================================
// SIGNAL HANDLING
// ============================================================================

void signal_handler(int sig) {
    if (sig == SIGINT) {
        time_t now = time(NULL);

        // Double CTRL+C within 2 seconds = force exit
        if (now - g_last_sigint <= 2) {
            // Force exit immediately
            (void)write(STDOUT_FILENO, "\nForce quit.\n", 13);
            _exit(1);
        }
        g_last_sigint = now;

        if (g_stream_active) {
            // Cancel streaming, don't exit
            g_stream_cancelled = 1;
            (void)write(STDOUT_FILENO, "\n[Cancelled]\n", 13);
        } else {
            // Exit Convergio
            g_running = 0;
            (void)write(STDOUT_FILENO, "\n", 1);
        }
    } else if (sig == SIGTERM) {
        // SIGTERM always exits
        g_running = 0;
        (void)write(STDOUT_FILENO, "\n", 1);
    }
}

void signals_init(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

// ============================================================================
// STREAM CANCELLATION API
// ============================================================================

bool stream_should_cancel(void) {
    return g_stream_cancelled != 0;
}

void stream_reset_cancel(void) {
    g_stream_cancelled = 0;
}

void stream_set_active(bool active) {
    g_stream_active = active ? 1 : 0;
    if (active) {
        g_stream_cancelled = 0;
    }
}

bool stream_is_active(void) {
    return g_stream_active != 0;
}
