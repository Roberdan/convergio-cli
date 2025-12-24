/**
 * CONVERGIO KERNEL - Signal Handling
 *
 * Unix signal handlers for graceful shutdown
 *
 * Signal behavior:
 * - CTRL+C during streaming: cancels the stream, returns to prompt
 * - CTRL+C at prompt: exits Convergio
 * - CTRL+C twice rapidly: force exit
 * - SIGSEGV: log crash and attempt graceful shutdown (FIX-05)
 */

#include "nous/signals.h"
#include "nous/commands.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// FIX-07: Crash marker file for recovery detection
static char g_crash_marker_path[512] = {0};
static bool g_crash_marker_active = false;

// ============================================================================
// STREAM STATE
// ============================================================================

volatile sig_atomic_t g_stream_cancelled = 0;
static volatile sig_atomic_t g_stream_active = 0;
static volatile time_t g_last_sigint = 0;

// ============================================================================
// SIGNAL HANDLING
// ============================================================================

// FIX-06: Cleanup callback for graceful shutdown
static void (*g_cleanup_callback)(void) = NULL;

void signals_set_cleanup_callback(void (*callback)(void)) {
    g_cleanup_callback = callback;
}

// FIX-05: SIGSEGV handler - log crash and attempt cleanup
static void sigsegv_handler(int sig) {
    (void)sig;
    const char* msg = "\n[CRASH] Segmentation fault detected. Attempting cleanup...\n";
    (void)write(STDERR_FILENO, msg, strlen(msg));

    // Log crash to file (async-signal-safe)
    const char* home = getenv("HOME");
    if (home) {
        char crash_log[512];
        snprintf(crash_log, sizeof(crash_log), "%s/.convergio/crash.log", home);
        int fd = open(crash_log, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd >= 0) {
            time_t now = time(NULL);
            char timestamp[64];
            int ts_len = snprintf(timestamp, sizeof(timestamp), "[%ld] SIGSEGV crash\n", (long)now);
            (void)write(fd, timestamp, (size_t)ts_len);
            close(fd);
        }
    }

    // Remove crash marker (we're shutting down)
    if (g_crash_marker_active && g_crash_marker_path[0]) {
        unlink(g_crash_marker_path);
    }

    // Reset handler and re-raise for core dump
    signal(SIGSEGV, SIG_DFL);
    raise(SIGSEGV);
}

void signal_handler(int sig) {
    if (sig == SIGINT) {
        time_t now = time(NULL);

        // Double CTRL+C within 2 seconds = force exit
        if (now - g_last_sigint <= 2) {
            // FIX-06: Try cleanup before force exit
            (void)write(STDOUT_FILENO, "\nForce quit - cleanup...\n", 25);
            if (g_cleanup_callback) {
                g_cleanup_callback();
            }
            // Remove crash marker on clean exit
            if (g_crash_marker_active && g_crash_marker_path[0]) {
                unlink(g_crash_marker_path);
            }
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

    // FIX-05: Register SIGSEGV handler
    struct sigaction sa_segv;
    sa_segv.sa_handler = sigsegv_handler;
    sa_segv.sa_flags = SA_RESETHAND; // One-shot, then default
    sigemptyset(&sa_segv.sa_mask);
    sigaction(SIGSEGV, &sa_segv, NULL);
}

// ============================================================================
// FIX-07: CRASH RECOVERY MARKER
// ============================================================================

void signals_set_crash_marker(const char* data_dir) {
    if (!data_dir)
        return;

    snprintf(g_crash_marker_path, sizeof(g_crash_marker_path), "%s/.convergio_running", data_dir);

    // Create marker file
    int fd = open(g_crash_marker_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        time_t now = time(NULL);
        char buf[64];
        int len = snprintf(buf, sizeof(buf), "%ld\n", (long)now);
        (void)write(fd, buf, (size_t)len);
        close(fd);
        g_crash_marker_active = true;
    }
}

void signals_clear_crash_marker(void) {
    if (g_crash_marker_active && g_crash_marker_path[0]) {
        unlink(g_crash_marker_path);
        g_crash_marker_active = false;
    }
}

bool signals_check_previous_crash(const char* data_dir) {
    if (!data_dir)
        return false;

    char marker_path[512];
    snprintf(marker_path, sizeof(marker_path), "%s/.convergio_running", data_dir);

    struct stat st;
    if (stat(marker_path, &st) == 0) {
        // Marker exists = previous session didn't exit cleanly
        // Remove it now (we're starting fresh)
        unlink(marker_path);
        return true;
    }
    return false;
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
