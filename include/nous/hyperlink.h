/**
 * CONVERGIO OSC 8 TERMINAL HYPERLINKS
 *
 * Enables clickable file paths in terminal output using OSC 8 escape sequences.
 * Supported terminals: iTerm2, Kitty, WezTerm, Ghostty, VS Code, Alacritty 0.13+
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_HYPERLINK_H
#define CONVERGIO_HYPERLINK_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// TERMINAL DETECTION
// ============================================================================

/**
 * Check if current terminal supports OSC 8 hyperlinks
 * Checks TERM_PROGRAM, VTE_VERSION, KITTY_WINDOW_ID, etc.
 *
 * @return true if terminal supports clickable hyperlinks
 */
bool hyperlink_supported(void);

/**
 * Get the name of the detected terminal emulator
 *
 * @return Terminal name or "unknown"
 */
const char* hyperlink_get_terminal(void);

// ============================================================================
// HYPERLINK FORMATTING
// ============================================================================

/**
 * Format a file path as a clickable hyperlink
 * Uses file:// URL scheme
 *
 * @param filepath Path to the file (relative or absolute)
 * @param display_text Text to display (NULL uses filepath)
 * @return Allocated string with hyperlink escape codes, caller must free
 *         If terminal doesn't support hyperlinks, returns just display_text
 */
char* hyperlink_file(const char* filepath, const char* display_text);

/**
 * Format a file path with line number as a clickable hyperlink
 * Opens the file at the specific line in supported editors
 *
 * @param filepath Path to the file
 * @param line Line number (1-indexed)
 * @param display_text Text to display (NULL uses "filepath:line")
 * @return Allocated string with hyperlink escape codes, caller must free
 */
char* hyperlink_file_line(const char* filepath, int line, const char* display_text);

/**
 * Format a file path with line and column as a clickable hyperlink
 *
 * @param filepath Path to the file
 * @param line Line number (1-indexed)
 * @param column Column number (1-indexed)
 * @param display_text Text to display (NULL uses "filepath:line:column")
 * @return Allocated string with hyperlink escape codes, caller must free
 */
char* hyperlink_file_line_col(const char* filepath, int line, int column, const char* display_text);

/**
 * Format a URL as a clickable hyperlink
 *
 * @param url The URL (http://, https://, etc.)
 * @param display_text Text to display (NULL uses URL)
 * @return Allocated string with hyperlink escape codes, caller must free
 */
char* hyperlink_url(const char* url, const char* display_text);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Strip all OSC 8 hyperlink escape sequences from text
 * Useful for logging or non-TTY output
 *
 * @param text Text potentially containing hyperlinks
 * @return Allocated string with hyperlinks removed, caller must free
 */
char* hyperlink_strip(const char* text);

/**
 * Get the length of the display text inside a hyperlink
 * (ignores escape sequences)
 *
 * @param hyperlink_text Text containing hyperlink
 * @return Length of visible display text
 */
size_t hyperlink_display_len(const char* hyperlink_text);

// ============================================================================
// CONFIGURATION
// ============================================================================

/**
 * Force enable/disable hyperlinks regardless of terminal detection
 *
 * @param enabled true to force enable, false to force disable
 */
void hyperlink_force(bool enabled);

/**
 * Reset hyperlink support to auto-detect
 */
void hyperlink_auto(void);

/**
 * Check if hyperlinks are currently enabled (after force/auto settings)
 *
 * @return true if hyperlinks will be generated
 */
bool hyperlink_enabled(void);

#endif // CONVERGIO_HYPERLINK_H
