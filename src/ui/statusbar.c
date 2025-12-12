/**
 * CONVERGIO STATUS BAR
 *
 * Real-time status display implementation.
 *
 * Layout (2 lines):
 * Line 1: ◆ user ▶ workspace ▶ Model ▶ [profile]    NNNN tokens
 * Line 2: ▶▶ bypass permissions on · N background tasks    current: X.X.X ▶ Editor
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/statusbar.h"
#include "nous/theme.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <pthread.h>

// ============================================================================
// VERSION
// ============================================================================

#ifndef CONVERGIO_VERSION
#define CONVERGIO_VERSION "3.0.0-dev"
#endif

// ============================================================================
// ANSI ESCAPE CODES
// ============================================================================

#define ANSI_SAVE_CURSOR    "\033[s"
#define ANSI_RESTORE_CURSOR "\033[u"
#define ANSI_MOVE_TO(row, col) "\033[" #row ";" #col "H"
#define ANSI_CLEAR_LINE     "\033[2K"
#define ANSI_RESET          "\033[0m"
#define ANSI_DIM            "\033[2m"
#define ANSI_BOLD           "\033[1m"

// Colors (using 256-color mode for consistency)
#define COLOR_USER          "\033[38;5;81m"   // Cyan
#define COLOR_PATH          "\033[38;5;252m"  // Light gray
#define COLOR_MODEL         "\033[38;5;214m"  // Orange
#define COLOR_PROFILE       "\033[38;5;141m"  // Purple
#define COLOR_TOKENS        "\033[38;5;77m"   // Green
#define COLOR_COST          "\033[38;5;220m"  // Yellow
#define COLOR_WARNING       "\033[38;5;208m"  // Orange warning
#define COLOR_AGENT         "\033[38;5;117m"  // Light blue
#define COLOR_VERSION       "\033[38;5;245m"  // Gray
#define COLOR_EDITOR        "\033[38;5;183m"  // Light purple
#define COLOR_ARROW         "\033[38;5;240m"  // Dark gray

// ============================================================================
// STATE
// ============================================================================

static StatusBarState g_status = {0};
static pthread_mutex_t g_status_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_initialized = false;

// ============================================================================
// HELPERS
// ============================================================================

static void update_terminal_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        g_status.terminal_width = ws.ws_col;
        g_status.terminal_height = ws.ws_row;
    } else {
        g_status.terminal_width = 80;
        g_status.terminal_height = 24;
    }
}

static char* get_username(void) {
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_name) {
        return strdup(pw->pw_name);
    }
    return strdup("user");
}

static char* get_cwd_basename(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        char* basename = strrchr(cwd, '/');
        return strdup(basename ? basename + 1 : cwd);
    }
    return strdup(".");
}

static char* detect_editor(void) {
    const char* editor = getenv("EDITOR");
    if (!editor) editor = getenv("VISUAL");
    if (!editor) return strdup("vim");

    // Extract basename
    const char* base = strrchr(editor, '/');
    return strdup(base ? base + 1 : editor);
}

static void format_tokens(char* buf, size_t size, uint64_t tokens) {
    if (tokens >= 1000000) {
        snprintf(buf, size, "%.1fM", (double)tokens / 1000000);
    } else if (tokens >= 1000) {
        snprintf(buf, size, "%.1fK", (double)tokens / 1000);
    } else {
        snprintf(buf, size, "%llu", (unsigned long long)tokens);
    }
}

// ============================================================================
// LIFECYCLE
// ============================================================================

int statusbar_init(void) {
    pthread_mutex_lock(&g_status_mutex);

    if (g_initialized) {
        pthread_mutex_unlock(&g_status_mutex);
        return 0;
    }

    // Initialize state
    memset(&g_status, 0, sizeof(g_status));

    // Detect terminal size
    update_terminal_size();

    // Get user info
    g_status.username = get_username();
    g_status.cwd_basename = get_cwd_basename();

    // Defaults
    g_status.active_model = strdup("Sonnet 4.5");
    g_status.profile_name = strdup("default");
    g_status.version = strdup(CONVERGIO_VERSION);
    g_status.editor = detect_editor();

    g_status.session_tokens = 0;
    g_status.session_cost = 0.0;
    g_status.active_agents = 0;
    g_status.background_tasks = 0;
    g_status.bypass_permissions = false;

    g_status.visible = true;
    g_status.dirty = true;

    g_initialized = true;

    pthread_mutex_unlock(&g_status_mutex);

    LOG_DEBUG(LOG_CAT_SYSTEM, "Status bar initialized");
    return 0;
}

void statusbar_shutdown(void) {
    pthread_mutex_lock(&g_status_mutex);

    if (!g_initialized) {
        pthread_mutex_unlock(&g_status_mutex);
        return;
    }

    // Clear the status bar from display
    if (g_status.visible && isatty(STDOUT_FILENO)) {
        statusbar_clear();
    }

    // Free state
    free(g_status.username);
    free(g_status.cwd_basename);
    free(g_status.active_model);
    free(g_status.profile_name);
    free(g_status.version);
    free(g_status.editor);

    memset(&g_status, 0, sizeof(g_status));
    g_initialized = false;

    pthread_mutex_unlock(&g_status_mutex);

    LOG_DEBUG(LOG_CAT_SYSTEM, "Status bar shutdown");
}

bool statusbar_available(void) {
    if (!isatty(STDOUT_FILENO)) return false;

    update_terminal_size();

    // Need at least 80 columns and 10 rows for status bar
    return g_status.terminal_width >= 80 && g_status.terminal_height >= 10;
}

// ============================================================================
// RENDERING
// ============================================================================

void statusbar_render(void) {
    if (!g_initialized || !g_status.visible || !statusbar_available()) {
        return;
    }

    pthread_mutex_lock(&g_status_mutex);

    // Save cursor position
    printf("%s", ANSI_SAVE_CURSOR);

    int width = g_status.terminal_width;
    int height = g_status.terminal_height;

    // ========================================================================
    // LINE 1: User info, model, tokens
    // ========================================================================
    printf("\033[%d;1H%s", height - 1, ANSI_CLEAR_LINE);

    // Left side: ◆ user ▶ workspace ▶ Model ▶ [profile]
    printf("%s◆%s %s%s%s",
           COLOR_AGENT, ANSI_RESET,
           COLOR_USER, g_status.username, ANSI_RESET);

    printf(" %s▶%s %s%s%s",
           COLOR_ARROW, ANSI_RESET,
           COLOR_PATH, g_status.cwd_basename, ANSI_RESET);

    printf(" %s▶%s %s%s%s",
           COLOR_ARROW, ANSI_RESET,
           COLOR_MODEL, g_status.active_model, ANSI_RESET);

    printf(" %s▶%s %s[%s]%s",
           COLOR_ARROW, ANSI_RESET,
           COLOR_PROFILE, g_status.profile_name, ANSI_RESET);

    // Calculate left side length (approximate)
    int left_len = 4 + strlen(g_status.username) + 3 +
                   strlen(g_status.cwd_basename) + 3 +
                   strlen(g_status.active_model) + 3 +
                   strlen(g_status.profile_name) + 4;

    // Right side: tokens count
    char token_str[32];
    format_tokens(token_str, sizeof(token_str), g_status.session_tokens);

    char right_str[64];
    snprintf(right_str, sizeof(right_str), "%s tokens", token_str);
    int right_len = strlen(right_str);

    // Padding
    int padding = width - left_len - right_len - 2;
    if (padding > 0) {
        printf("%*s", padding, "");
    }

    printf("%s%s%s", COLOR_TOKENS, right_str, ANSI_RESET);

    // ========================================================================
    // LINE 2: Permissions, tasks, version, editor
    // ========================================================================
    printf("\033[%d;1H%s", height, ANSI_CLEAR_LINE);

    // Left side
    printf("%s▶▶%s ", COLOR_ARROW, ANSI_RESET);

    if (g_status.bypass_permissions) {
        printf("%sbypass permissions on%s %s·%s ",
               COLOR_WARNING, ANSI_RESET, COLOR_ARROW, ANSI_RESET);
    }

    printf("%s%d background tasks%s",
           COLOR_AGENT, g_status.background_tasks, ANSI_RESET);

    // Cost display if > 0
    if (g_status.session_cost > 0.001) {
        printf(" %s·%s %s$%.4f%s",
               COLOR_ARROW, ANSI_RESET,
               COLOR_COST, g_status.session_cost, ANSI_RESET);
    }

    // Right side: version and editor
    char right2_str[128];
    if (g_status.editor) {
        snprintf(right2_str, sizeof(right2_str), "current: %s ▶ %s",
                 g_status.version, g_status.editor);
    } else {
        snprintf(right2_str, sizeof(right2_str), "current: %s", g_status.version);
    }

    // Calculate remaining space
    int left2_len = 3 + 18 + (g_status.bypass_permissions ? 22 : 0) +
                    (g_status.session_cost > 0.001 ? 12 : 0);
    int right2_len = strlen(right2_str);
    int padding2 = width - left2_len - right2_len - 4;

    if (padding2 > 0) {
        printf("%*s", padding2, "");
    }

    printf("%s%s%s", COLOR_VERSION, right2_str, ANSI_RESET);

    // Restore cursor position
    printf("%s", ANSI_RESTORE_CURSOR);
    fflush(stdout);

    g_status.dirty = false;

    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_clear(void) {
    if (!g_initialized || !isatty(STDOUT_FILENO)) return;

    pthread_mutex_lock(&g_status_mutex);

    int height = g_status.terminal_height;

    // Clear the last two lines
    printf("%s", ANSI_SAVE_CURSOR);
    printf("\033[%d;1H%s", height - 1, ANSI_CLEAR_LINE);
    printf("\033[%d;1H%s", height, ANSI_CLEAR_LINE);
    printf("%s", ANSI_RESTORE_CURSOR);
    fflush(stdout);

    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_set_visible(bool visible) {
    pthread_mutex_lock(&g_status_mutex);
    if (g_status.visible != visible) {
        g_status.visible = visible;
        g_status.dirty = true;
        if (!visible) {
            pthread_mutex_unlock(&g_status_mutex);
            statusbar_clear();
            return;
        }
    }
    pthread_mutex_unlock(&g_status_mutex);

    if (visible) {
        statusbar_render();
    }
}

bool statusbar_is_visible(void) {
    pthread_mutex_lock(&g_status_mutex);
    bool visible = g_status.visible;
    pthread_mutex_unlock(&g_status_mutex);
    return visible;
}

// ============================================================================
// STATE UPDATES
// ============================================================================

void statusbar_set_cwd(const char* path) {
    pthread_mutex_lock(&g_status_mutex);
    free(g_status.cwd_basename);

    if (path) {
        const char* basename = strrchr(path, '/');
        g_status.cwd_basename = strdup(basename ? basename + 1 : path);
    } else {
        g_status.cwd_basename = get_cwd_basename();
    }

    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_set_model(const char* model) {
    pthread_mutex_lock(&g_status_mutex);
    free(g_status.active_model);
    g_status.active_model = strdup(model ? model : "Unknown");
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_set_profile(const char* profile) {
    pthread_mutex_lock(&g_status_mutex);
    free(g_status.profile_name);
    g_status.profile_name = strdup(profile ? profile : "default");
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_add_tokens(uint64_t tokens) {
    pthread_mutex_lock(&g_status_mutex);
    g_status.session_tokens += tokens;
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_add_cost(double cost) {
    pthread_mutex_lock(&g_status_mutex);
    g_status.session_cost += cost;
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_set_agent_count(int count) {
    pthread_mutex_lock(&g_status_mutex);
    g_status.active_agents = count;
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_set_background_tasks(int count) {
    pthread_mutex_lock(&g_status_mutex);
    g_status.background_tasks = count;
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_set_bypass_permissions(bool enabled) {
    pthread_mutex_lock(&g_status_mutex);
    g_status.bypass_permissions = enabled;
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

void statusbar_set_editor(const char* editor) {
    pthread_mutex_lock(&g_status_mutex);
    free(g_status.editor);
    g_status.editor = editor ? strdup(editor) : NULL;
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);
}

// ============================================================================
// GETTERS
// ============================================================================

uint64_t statusbar_get_tokens(void) {
    pthread_mutex_lock(&g_status_mutex);
    uint64_t tokens = g_status.session_tokens;
    pthread_mutex_unlock(&g_status_mutex);
    return tokens;
}

double statusbar_get_cost(void) {
    pthread_mutex_lock(&g_status_mutex);
    double cost = g_status.session_cost;
    pthread_mutex_unlock(&g_status_mutex);
    return cost;
}

const StatusBarState* statusbar_get_state(void) {
    return &g_status;
}

// ============================================================================
// TERMINAL HANDLING
// ============================================================================

void statusbar_handle_resize(void) {
    pthread_mutex_lock(&g_status_mutex);
    update_terminal_size();
    g_status.dirty = true;
    pthread_mutex_unlock(&g_status_mutex);

    if (g_status.visible) {
        statusbar_render();
    }
}

void statusbar_get_terminal_size(int* width, int* height) {
    pthread_mutex_lock(&g_status_mutex);
    if (width) *width = g_status.terminal_width;
    if (height) *height = g_status.terminal_height;
    pthread_mutex_unlock(&g_status_mutex);
}
