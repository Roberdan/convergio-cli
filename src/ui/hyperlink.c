/**
 * CONVERGIO OSC 8 TERMINAL HYPERLINKS
 *
 * Implementation of clickable file paths using OSC 8 escape sequences.
 *
 * OSC 8 Format:
 *   \033]8;;URL\033\\DISPLAY_TEXT\033]8;;\033\\
 *
 * Where:
 *   \033]8;; = Start hyperlink
 *   URL      = file:///path or http(s)://url
 *   \033\\   = Separator (ST - String Terminator)
 *   DISPLAY_TEXT = Visible text
 *   \033]8;;\033\\ = End hyperlink
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/hyperlink.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

// ============================================================================
// OSC 8 ESCAPE SEQUENCES
// ============================================================================

// ESC character as hex to avoid preprocessor interpretation issues
#define ESC_CHAR "\x1b"
#define OSC8_START  ESC_CHAR "]8;;"
#define OSC8_SEP    ESC_CHAR "\\"
#define OSC8_END    ESC_CHAR "]8;;" ESC_CHAR "\\"

// ============================================================================
// TERMINAL DETECTION STATE
// ============================================================================

typedef enum {
    HYPERLINK_AUTO,
    HYPERLINK_FORCED_ON,
    HYPERLINK_FORCED_OFF
} HyperlinkMode;

static HyperlinkMode g_hyperlink_mode = HYPERLINK_AUTO;
static bool g_detection_done = false;
static bool g_terminal_supports_hyperlinks = false;
static const char* g_terminal_name = "unknown";

// ============================================================================
// TERMINAL DETECTION
// ============================================================================

static void detect_terminal(void) {
    if (g_detection_done) return;
    g_detection_done = true;

    // Check if stdout is a TTY
    if (!isatty(STDOUT_FILENO)) {
        g_terminal_supports_hyperlinks = false;
        g_terminal_name = "non-tty";
        return;
    }

    // Check TERM for basic compatibility
    const char* term = getenv("TERM");
    if (!term) {
        g_terminal_supports_hyperlinks = false;
        g_terminal_name = "unknown (no TERM)";
        return;
    }

    // Check specific terminals that support OSC 8 (December 2025)
    const char* term_program = getenv("TERM_PROGRAM");
    const char* vte_version = getenv("VTE_VERSION");
    const char* wt_session = getenv("WT_SESSION");
    const char* colorterm = getenv("COLORTERM");

    // iTerm2
    if (term_program && strstr(term_program, "iTerm")) {
        g_terminal_supports_hyperlinks = true;
        g_terminal_name = "iTerm2";
        return;
    }

    // VS Code integrated terminal
    if (term_program && strstr(term_program, "vscode")) {
        g_terminal_supports_hyperlinks = true;
        g_terminal_name = "VS Code";
        return;
    }

    // WezTerm
    if (term_program && strstr(term_program, "WezTerm")) {
        g_terminal_supports_hyperlinks = true;
        g_terminal_name = "WezTerm";
        return;
    }

    // Ghostty
    if (term_program && strstr(term_program, "Ghostty")) {
        g_terminal_supports_hyperlinks = true;
        g_terminal_name = "Ghostty";
        return;
    }

    // Kitty
    if (getenv("KITTY_WINDOW_ID")) {
        g_terminal_supports_hyperlinks = true;
        g_terminal_name = "Kitty";
        return;
    }

    // VTE-based terminals (GNOME Terminal, Tilix, etc.) version 0.50+
    if (vte_version) {
        int version = atoi(vte_version);
        if (version >= 5000) {  // VTE 0.50+
            g_terminal_supports_hyperlinks = true;
            g_terminal_name = "VTE-based";
            return;
        }
    }

    // Windows Terminal
    if (wt_session) {
        g_terminal_supports_hyperlinks = true;
        g_terminal_name = "Windows Terminal";
        return;
    }

    // Alacritty 0.13+ (check COLORTERM as a heuristic)
    if (term && strstr(term, "alacritty")) {
        // Alacritty 0.13+ supports OSC 8
        g_terminal_supports_hyperlinks = true;
        g_terminal_name = "Alacritty";
        return;
    }

    // Hyper
    if (term_program && strstr(term_program, "Hyper")) {
        g_terminal_supports_hyperlinks = true;
        g_terminal_name = "Hyper";
        return;
    }

    // COLORTERM=truecolor often indicates modern terminal
    if (colorterm && strcmp(colorterm, "truecolor") == 0) {
        // Be conservative - only enable if we recognize the terminal
        // Many truecolor terminals don't support OSC 8
    }

    // macOS Terminal.app does NOT support OSC 8
    if (term_program && strstr(term_program, "Apple_Terminal")) {
        g_terminal_supports_hyperlinks = false;
        g_terminal_name = "macOS Terminal (no OSC 8)";
        return;
    }

    // Default: assume no support
    g_terminal_supports_hyperlinks = false;
    g_terminal_name = term_program ? term_program : "unknown";
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool hyperlink_supported(void) {
    detect_terminal();
    return g_terminal_supports_hyperlinks;
}

const char* hyperlink_get_terminal(void) {
    detect_terminal();
    return g_terminal_name;
}

bool hyperlink_enabled(void) {
    switch (g_hyperlink_mode) {
        case HYPERLINK_FORCED_ON:
            return true;
        case HYPERLINK_FORCED_OFF:
            return false;
        case HYPERLINK_AUTO:
        default:
            return hyperlink_supported();
    }
}

void hyperlink_force(bool enabled) {
    g_hyperlink_mode = enabled ? HYPERLINK_FORCED_ON : HYPERLINK_FORCED_OFF;
}

void hyperlink_auto(void) {
    g_hyperlink_mode = HYPERLINK_AUTO;
}

// ============================================================================
// HYPERLINK FORMATTING
// ============================================================================

char* hyperlink_file(const char* filepath, const char* display_text) {
    if (!filepath) return strdup("");

    const char* display = display_text ? display_text : filepath;

    // If hyperlinks not enabled, just return display text
    if (!hyperlink_enabled()) {
        return strdup(display);
    }

    // Resolve to absolute path
    char* abs_path = realpath(filepath, NULL);
    if (!abs_path) {
        // File might not exist yet, construct absolute path manually
        if (filepath[0] == '/') {
            abs_path = strdup(filepath);
        } else {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd))) {
                size_t len = strlen(cwd) + strlen(filepath) + 2;
                abs_path = malloc(len);
                if (abs_path) {
                    snprintf(abs_path, len, "%s/%s", cwd, filepath);
                }
            } else {
                abs_path = strdup(filepath);
            }
        }
    }

    if (!abs_path) {
        return strdup(display);
    }

    // Format: ESC]8;;file:///path ESC-backslash display ESC]8;; ESC-backslash
    size_t start_len = strlen(OSC8_START);
    size_t sep_len = strlen(OSC8_SEP);
    size_t end_len = strlen(OSC8_END);
    size_t total_len = start_len + 7 + strlen(abs_path) + sep_len + strlen(display) + end_len + 1;
    char* result = malloc(total_len);

    if (result) {
        snprintf(result, total_len, "%sfile://%s%s%s%s",
                 OSC8_START, abs_path, OSC8_SEP, display, OSC8_END);
    }

    free(abs_path);
    return result ? result : strdup(display);
}

char* hyperlink_file_line(const char* filepath, int line, const char* display_text) {
    if (!filepath) return strdup("");

    // Create default display text if not provided
    char default_display[512];
    if (!display_text) {
        snprintf(default_display, sizeof(default_display), "%s:%d", filepath, line);
        display_text = default_display;
    }

    // If hyperlinks not enabled, just return display text
    if (!hyperlink_enabled()) {
        return strdup(display_text);
    }

    // Resolve to absolute path
    char* abs_path = realpath(filepath, NULL);
    if (!abs_path) {
        if (filepath[0] == '/') {
            abs_path = strdup(filepath);
        } else {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd))) {
                size_t len = strlen(cwd) + strlen(filepath) + 2;
                abs_path = malloc(len);
                if (abs_path) {
                    snprintf(abs_path, len, "%s/%s", cwd, filepath);
                }
            } else {
                abs_path = strdup(filepath);
            }
        }
    }

    if (!abs_path) {
        return strdup(display_text);
    }

    // Use file://path#L<line> for line support
    // Many editors support this format (VS Code, Sublime, etc.)
    char line_suffix[32];
    snprintf(line_suffix, sizeof(line_suffix), "#L%d", line);

    size_t len = strlen(OSC8_START) + 7 + strlen(abs_path) + strlen(line_suffix) +
                 strlen(OSC8_SEP) + strlen(display_text) + strlen(OSC8_END) + 1;
    char* result = malloc(len);

    if (result) {
        snprintf(result, len, "%sfile://%s%s%s%s%s",
                 OSC8_START, abs_path, line_suffix, OSC8_SEP, display_text, OSC8_END);
    }

    free(abs_path);
    return result ? result : strdup(display_text);
}

char* hyperlink_file_line_col(const char* filepath, int line, int column, const char* display_text) {
    if (!filepath) return strdup("");

    // Create default display text if not provided
    char default_display[512];
    if (!display_text) {
        snprintf(default_display, sizeof(default_display), "%s:%d:%d", filepath, line, column);
        display_text = default_display;
    }

    // If hyperlinks not enabled, just return display text
    if (!hyperlink_enabled()) {
        return strdup(display_text);
    }

    // Resolve to absolute path
    char* abs_path = realpath(filepath, NULL);
    if (!abs_path) {
        if (filepath[0] == '/') {
            abs_path = strdup(filepath);
        } else {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd))) {
                size_t len = strlen(cwd) + strlen(filepath) + 2;
                abs_path = malloc(len);
                if (abs_path) {
                    snprintf(abs_path, len, "%s/%s", cwd, filepath);
                }
            } else {
                abs_path = strdup(filepath);
            }
        }
    }

    if (!abs_path) {
        return strdup(display_text);
    }

    // Use file://path#L<line>,<col> for line and column
    char line_col_suffix[64];
    snprintf(line_col_suffix, sizeof(line_col_suffix), "#L%d,%d", line, column);

    size_t len = strlen(OSC8_START) + 7 + strlen(abs_path) + strlen(line_col_suffix) +
                 strlen(OSC8_SEP) + strlen(display_text) + strlen(OSC8_END) + 1;
    char* result = malloc(len);

    if (result) {
        snprintf(result, len, "%sfile://%s%s%s%s%s",
                 OSC8_START, abs_path, line_col_suffix, OSC8_SEP, display_text, OSC8_END);
    }

    free(abs_path);
    return result ? result : strdup(display_text);
}

char* hyperlink_url(const char* url, const char* display_text) {
    if (!url) return strdup("");

    const char* display = display_text ? display_text : url;

    // If hyperlinks not enabled, just return display text
    if (!hyperlink_enabled()) {
        return strdup(display);
    }

    // Format: ESC]8;;URL ESC-backslash display ESC]8;; ESC-backslash
    size_t start_len = strlen(OSC8_START);
    size_t sep_len = strlen(OSC8_SEP);
    size_t end_len = strlen(OSC8_END);
    size_t total_len = start_len + strlen(url) + sep_len + strlen(display) + end_len + 1;
    char* result = malloc(total_len);

    if (result) {
        snprintf(result, total_len, "%s%s%s%s%s",
                 OSC8_START, url, OSC8_SEP, display, OSC8_END);
    }

    return result ? result : strdup(display);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

char* hyperlink_strip(const char* text) {
    if (!text) return strdup("");

    // Allocate result buffer (same size as input is safe upper bound)
    size_t len = strlen(text);
    char* result = malloc(len + 1);
    if (!result) return strdup(text);

    const char* src = text;
    char* dst = result;

    while (*src) {
        // Check for OSC 8 start sequence: \033]8;;
        if (src[0] == '\033' && src[1] == ']' && src[2] == '8' &&
            src[3] == ';' && src[4] == ';') {
            // Skip past the URL until we find the separator
            src += 5;
            while (*src && !(src[0] == '\033' && src[1] == '\\')) {
                src++;
            }
            if (*src) src += 2;  // Skip \033\\ separator
            // Now we're at the display text, which we want to keep
            // Continue until we find the end sequence
            while (*src && !(src[0] == '\033' && src[1] == ']' && src[2] == '8' &&
                            src[3] == ';' && src[4] == ';')) {
                *dst++ = *src++;
            }
            // Skip the end sequence
            if (*src) {
                src += 5;  // Skip \033]8;;
                while (*src && !(src[0] == '\033' && src[1] == '\\')) {
                    src++;
                }
                if (*src) src += 2;  // Skip ESC-backslash
            }
        } else {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
    return result;
}

size_t hyperlink_display_len(const char* hyperlink_text) {
    if (!hyperlink_text) return 0;

    char* stripped = hyperlink_strip(hyperlink_text);
    if (!stripped) return strlen(hyperlink_text);

    size_t len = strlen(stripped);
    free(stripped);
    return len;
}
