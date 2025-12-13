/**
 * CONVERGIO TERMINAL HANDLING
 *
 * Terminal management including:
 * - SIGWINCH (window resize) handling
 * - Terminal capability detection
 * - Raw mode management
 * - Input handling
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/statusbar.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pthread.h>

// ============================================================================
// STATE
// ============================================================================

typedef struct {
    // Terminal size
    int width;
    int height;

    // Original terminal settings (for restore)
    struct termios original_termios;
    bool termios_saved;

    // Raw mode state
    bool raw_mode;

    // Signal handling
    bool sigwinch_handler_installed;
    struct sigaction old_sigwinch_action;

    // Callbacks
    void (*on_resize)(int width, int height, void* ctx);
    void* resize_ctx;

    // Mutex for thread safety
    pthread_mutex_t mutex;

    bool initialized;
} TerminalState;

static TerminalState g_terminal = {
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static void update_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        g_terminal.width = ws.ws_col;
        g_terminal.height = ws.ws_row;
    } else {
        g_terminal.width = 80;
        g_terminal.height = 24;
    }
}

// ============================================================================
// SIGNAL HANDLER
// ============================================================================

static void sigwinch_handler(int sig) {
    (void)sig;

    // Update terminal size
    pthread_mutex_lock(&g_terminal.mutex);
    update_size();

    // Store values locally before unlocking
    void (*callback)(int, int, void*) = g_terminal.on_resize;
    void* ctx = g_terminal.resize_ctx;
    int w = g_terminal.width;
    int h = g_terminal.height;

    pthread_mutex_unlock(&g_terminal.mutex);

    // Notify status bar
    statusbar_handle_resize();

    // Invoke user callback if set
    if (callback) {
        callback(w, h, ctx);
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int terminal_init(void) {
    pthread_mutex_lock(&g_terminal.mutex);

    if (g_terminal.initialized) {
        pthread_mutex_unlock(&g_terminal.mutex);
        return 0;
    }

    // Get initial size
    update_size();

    // Save original termios
    if (isatty(STDIN_FILENO)) {
        if (tcgetattr(STDIN_FILENO, &g_terminal.original_termios) == 0) {
            g_terminal.termios_saved = true;
        }
    }

    // Install SIGWINCH handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGWINCH, &sa, &g_terminal.old_sigwinch_action) == 0) {
        g_terminal.sigwinch_handler_installed = true;
    }

    g_terminal.initialized = true;

    pthread_mutex_unlock(&g_terminal.mutex);

    LOG_DEBUG(LOG_CAT_SYSTEM, "Terminal initialized: %dx%d",
              g_terminal.width, g_terminal.height);

    return 0;
}

void terminal_shutdown(void) {
    pthread_mutex_lock(&g_terminal.mutex);

    if (!g_terminal.initialized) {
        pthread_mutex_unlock(&g_terminal.mutex);
        return;
    }

    // Restore original SIGWINCH handler
    if (g_terminal.sigwinch_handler_installed) {
        sigaction(SIGWINCH, &g_terminal.old_sigwinch_action, NULL);
        g_terminal.sigwinch_handler_installed = false;
    }

    // Restore original termios if we modified it
    if (g_terminal.termios_saved && g_terminal.raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_terminal.original_termios);
        g_terminal.raw_mode = false;
    }

    g_terminal.initialized = false;

    pthread_mutex_unlock(&g_terminal.mutex);

    LOG_DEBUG(LOG_CAT_SYSTEM, "Terminal shutdown");
}

// ============================================================================
// TERMINAL SIZE
// ============================================================================

void terminal_get_size(int* width, int* height) {
    pthread_mutex_lock(&g_terminal.mutex);

    if (width) *width = g_terminal.width;
    if (height) *height = g_terminal.height;

    pthread_mutex_unlock(&g_terminal.mutex);
}

void terminal_refresh_size(void) {
    pthread_mutex_lock(&g_terminal.mutex);
    update_size();
    pthread_mutex_unlock(&g_terminal.mutex);
}

// ============================================================================
// RESIZE CALLBACK
// ============================================================================

void terminal_set_resize_callback(void (*callback)(int, int, void*), void* ctx) {
    pthread_mutex_lock(&g_terminal.mutex);

    g_terminal.on_resize = callback;
    g_terminal.resize_ctx = ctx;

    pthread_mutex_unlock(&g_terminal.mutex);
}

// ============================================================================
// RAW MODE
// ============================================================================

int terminal_enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO)) return -1;

    pthread_mutex_lock(&g_terminal.mutex);

    if (g_terminal.raw_mode) {
        pthread_mutex_unlock(&g_terminal.mutex);
        return 0;
    }

    if (!g_terminal.termios_saved) {
        if (tcgetattr(STDIN_FILENO, &g_terminal.original_termios) != 0) {
            pthread_mutex_unlock(&g_terminal.mutex);
            return -1;
        }
        g_terminal.termios_saved = true;
    }

    struct termios raw = g_terminal.original_termios;

    // Disable echo and canonical mode
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | ISIG | IEXTEN);

    // Disable flow control
    raw.c_iflag &= (tcflag_t)~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);

    // Disable output processing
    raw.c_oflag &= (tcflag_t)~(OPOST);

    // Set character size to 8 bits
    raw.c_cflag |= CS8;

    // Read returns after 1 byte or 100ms timeout
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        pthread_mutex_unlock(&g_terminal.mutex);
        return -1;
    }

    g_terminal.raw_mode = true;

    pthread_mutex_unlock(&g_terminal.mutex);
    return 0;
}

int terminal_disable_raw_mode(void) {
    pthread_mutex_lock(&g_terminal.mutex);

    if (!g_terminal.raw_mode || !g_terminal.termios_saved) {
        pthread_mutex_unlock(&g_terminal.mutex);
        return 0;
    }

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_terminal.original_termios) != 0) {
        pthread_mutex_unlock(&g_terminal.mutex);
        return -1;
    }

    g_terminal.raw_mode = false;

    pthread_mutex_unlock(&g_terminal.mutex);
    return 0;
}

bool terminal_is_raw_mode(void) {
    pthread_mutex_lock(&g_terminal.mutex);
    bool raw = g_terminal.raw_mode;
    pthread_mutex_unlock(&g_terminal.mutex);
    return raw;
}

// ============================================================================
// CAPABILITY DETECTION
// ============================================================================

bool terminal_supports_color(void) {
    const char* term = getenv("TERM");
    if (!term) return false;

    // Check for common color-supporting terminals
    if (strstr(term, "color") || strstr(term, "xterm") ||
        strstr(term, "screen") || strstr(term, "tmux") ||
        strstr(term, "256color") || strstr(term, "ansi")) {
        return true;
    }

    // Check COLORTERM environment variable
    const char* colorterm = getenv("COLORTERM");
    if (colorterm) return true;

    return false;
}

bool terminal_supports_truecolor(void) {
    const char* colorterm = getenv("COLORTERM");
    if (!colorterm) return false;

    return (strcmp(colorterm, "truecolor") == 0 ||
            strcmp(colorterm, "24bit") == 0);
}

bool terminal_supports_hyperlinks(void) {
    // Check for terminals known to support OSC 8 hyperlinks
    const char* term = getenv("TERM");
    const char* term_program = getenv("TERM_PROGRAM");

    if (term_program) {
        if (strcmp(term_program, "iTerm.app") == 0 ||
            strcmp(term_program, "WezTerm") == 0 ||
            strcmp(term_program, "vscode") == 0 ||
            strcmp(term_program, "Hyper") == 0) {
            return true;
        }
    }

    if (term) {
        // VTE-based terminals support hyperlinks
        const char* vte = getenv("VTE_VERSION");
        if (vte && atoi(vte) >= 5000) return true;

        // Kitty supports hyperlinks
        if (getenv("KITTY_WINDOW_ID")) return true;
    }

    return false;
}

bool terminal_supports_unicode(void) {
    const char* lang = getenv("LANG");
    const char* lc_all = getenv("LC_ALL");
    const char* lc_ctype = getenv("LC_CTYPE");

    const char* locale = lc_all ? lc_all : (lc_ctype ? lc_ctype : lang);
    if (!locale) return false;

    return (strstr(locale, "UTF-8") || strstr(locale, "utf-8") ||
            strstr(locale, "UTF8") || strstr(locale, "utf8"));
}

// ============================================================================
// CURSOR CONTROL
// ============================================================================

void terminal_move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

void terminal_save_cursor(void) {
    printf("\033[s");
    fflush(stdout);
}

void terminal_restore_cursor(void) {
    printf("\033[u");
    fflush(stdout);
}

void terminal_hide_cursor(void) {
    printf("\033[?25l");
    fflush(stdout);
}

void terminal_show_cursor(void) {
    printf("\033[?25h");
    fflush(stdout);
}

// ============================================================================
// SCREEN CONTROL
// ============================================================================

void terminal_clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void terminal_clear_line(void) {
    printf("\033[2K");
    fflush(stdout);
}

void terminal_clear_to_end_of_line(void) {
    printf("\033[K");
    fflush(stdout);
}

void terminal_clear_to_end_of_screen(void) {
    printf("\033[J");
    fflush(stdout);
}

// ============================================================================
// ALTERNATE SCREEN BUFFER
// ============================================================================

void terminal_enter_alternate_screen(void) {
    printf("\033[?1049h");
    fflush(stdout);
}

void terminal_exit_alternate_screen(void) {
    printf("\033[?1049l");
    fflush(stdout);
}

// ============================================================================
// INPUT READING
// ============================================================================

typedef enum {
    KEY_UNKNOWN = 0,
    KEY_ENTER = '\r',
    KEY_TAB = '\t',
    KEY_BACKSPACE = 127,
    KEY_ESCAPE = 27,

    // Special keys (values > 256 to avoid conflicts)
    KEY_UP = 300,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_INSERT,
    KEY_DELETE,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
} TerminalKey;

// Read a single keypress (may return special keys)
int terminal_read_key(void) {
    char c;
    ssize_t nread = read(STDIN_FILENO, &c, 1);
    if (nread <= 0) return -1;

    // Handle escape sequences
    if (c == '\033') {
        char seq[5];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESCAPE;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESCAPE;

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return KEY_ESCAPE;
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return KEY_HOME;
                        case '3': return KEY_DELETE;
                        case '4': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        case '7': return KEY_HOME;
                        case '8': return KEY_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_UP;
                    case 'B': return KEY_DOWN;
                    case 'C': return KEY_RIGHT;
                    case 'D': return KEY_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                case 'P': return KEY_F1;
                case 'Q': return KEY_F2;
                case 'R': return KEY_F3;
                case 'S': return KEY_F4;
            }
        }

        return KEY_ESCAPE;
    }

    return (int)(unsigned char)c;
}

// ============================================================================
// UTILITY
// ============================================================================

bool terminal_is_tty(void) {
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

const char* terminal_get_term(void) {
    return getenv("TERM");
}

// Print terminal info for debugging
char* terminal_info(void) {
    char* info = malloc(1024);
    if (!info) return NULL;

    pthread_mutex_lock(&g_terminal.mutex);

    snprintf(info, 1024,
        "Terminal Information:\n"
        "  Size: %dx%d\n"
        "  TERM: %s\n"
        "  TERM_PROGRAM: %s\n"
        "  COLORTERM: %s\n"
        "  Is TTY: %s\n"
        "  Raw mode: %s\n"
        "  Supports color: %s\n"
        "  Supports truecolor: %s\n"
        "  Supports hyperlinks: %s\n"
        "  Supports unicode: %s\n",
        g_terminal.width, g_terminal.height,
        getenv("TERM") ? getenv("TERM") : "(not set)",
        getenv("TERM_PROGRAM") ? getenv("TERM_PROGRAM") : "(not set)",
        getenv("COLORTERM") ? getenv("COLORTERM") : "(not set)",
        terminal_is_tty() ? "yes" : "no",
        g_terminal.raw_mode ? "yes" : "no",
        terminal_supports_color() ? "yes" : "no",
        terminal_supports_truecolor() ? "yes" : "no",
        terminal_supports_hyperlinks() ? "yes" : "no",
        terminal_supports_unicode() ? "yes" : "no");

    pthread_mutex_unlock(&g_terminal.mutex);
    return info;
}
