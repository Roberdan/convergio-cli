/**
 * Streaming Markdown Renderer
 *
 * Renders markdown incrementally as chunks arrive.
 * Uses a state machine with look-ahead buffer for ambiguous tokens.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "nous/stream_md.h"

// ANSI escape codes
#define ANSI_RESET      "\033[0m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_DIM        "\033[2m"
#define ANSI_ITALIC     "\033[3m"
#define ANSI_UNDERLINE  "\033[4m"

// Colors
#define ANSI_CYAN       "\033[36m"
#define ANSI_GREEN      "\033[32m"
#define ANSI_BLUE       "\033[34m"
#define ANSI_WHITE      "\033[37m"

// Bright colors
#define ANSI_BRIGHT_CYAN    "\033[96m"

// Streaming state machine states
typedef enum {
    STATE_NORMAL,
    STATE_MAYBE_BOLD,        // Saw one *
    STATE_IN_BOLD,           // Inside **...**
    STATE_MAYBE_BOLD_END,    // Saw one * inside bold
    STATE_IN_ITALIC,         // Inside *...*
    STATE_MAYBE_CODE,        // Saw one `
    STATE_MAYBE_CODE_BLOCK,  // Saw two ``
    STATE_IN_CODE,           // Inside `...`
    STATE_IN_CODE_BLOCK,     // Inside ```...```
    STATE_MAYBE_HEADER,      // Saw # at line start
    STATE_IN_HEADER,         // Reading header text
    STATE_MAYBE_BULLET,      // Saw - or * at line start
    STATE_IN_LINK_TEXT,      // Inside [text]
    STATE_MAYBE_LINK_URL,    // Saw ](
    STATE_IN_LINK_URL,       // Inside (url)
    STATE_MAYBE_HR,          // Saw -- or ** at line start
} StreamState;

struct StreamMd {
    StreamState state;
    bool line_start;
    int header_level;
    char pending[16];        // Buffer for ambiguous tokens
    int pending_len;
    char link_text[256];     // Buffer for link text
    int link_text_len;
};

StreamMd* stream_md_create(void) {
    StreamMd* sm = calloc(1, sizeof(StreamMd));
    if (sm) {
        sm->state = STATE_NORMAL;
        sm->line_start = true;
    }
    return sm;
}

void stream_md_destroy(StreamMd* sm) {
    if (sm) free(sm);
}

// Flush pending buffer as-is (when token doesn't match expected pattern)
static void flush_pending(StreamMd* sm) {
    if (sm->pending_len > 0) {
        fwrite(sm->pending, 1, sm->pending_len, stdout);
        sm->pending_len = 0;
    }
}

// Output styled text
static void emit(const char* text) {
    fputs(text, stdout);
    fflush(stdout);
}

static void emit_char(char c) {
    putchar(c);
    fflush(stdout);
}

/**
 * Process a single character through the state machine.
 * Outputs formatted text as soon as we know what to do.
 */
void stream_md_process_char(StreamMd* sm, char c) {
    switch (sm->state) {
    case STATE_NORMAL:
        // Check for markdown tokens at line start
        if (sm->line_start) {
            if (c == '#') {
                sm->state = STATE_MAYBE_HEADER;
                sm->header_level = 1;
                return;
            }
            if (c == '-' || c == '*' || c == '+') {
                sm->pending[0] = c;
                sm->pending_len = 1;
                sm->state = STATE_MAYBE_BULLET;
                return;
            }
        }

        // Inline tokens
        if (c == '*') {
            sm->pending[0] = '*';
            sm->pending_len = 1;
            sm->state = STATE_MAYBE_BOLD;
            return;
        }
        if (c == '`') {
            sm->pending[0] = '`';
            sm->pending_len = 1;
            sm->state = STATE_MAYBE_CODE;
            return;
        }
        if (c == '[') {
            sm->state = STATE_IN_LINK_TEXT;
            sm->link_text_len = 0;
            emit(ANSI_UNDERLINE ANSI_BLUE);
            return;
        }

        // Regular character
        emit_char(c);
        sm->line_start = (c == '\n');
        break;

    case STATE_MAYBE_HEADER:
        if (c == '#' && sm->header_level < 6) {
            sm->header_level++;
            return;
        }
        if (c == ' ') {
            // Valid header, start styled output
            sm->state = STATE_IN_HEADER;
            if (sm->header_level == 1) {
                emit(ANSI_BOLD ANSI_BRIGHT_CYAN);
            } else if (sm->header_level == 2) {
                emit(ANSI_BOLD ANSI_CYAN);
            } else {
                emit(ANSI_BOLD);
            }
            return;
        }
        // Not a header, output the #'s
        for (int i = 0; i < sm->header_level; i++) emit_char('#');
        sm->state = STATE_NORMAL;
        sm->line_start = false;
        // Process current char normally
        stream_md_process_char(sm, c);
        break;

    case STATE_IN_HEADER:
        if (c == '\n') {
            emit(ANSI_RESET);
            emit_char('\n');
            sm->state = STATE_NORMAL;
            sm->line_start = true;
            return;
        }
        emit_char(c);
        break;

    case STATE_MAYBE_BULLET:
        if (c == ' ') {
            // It's a bullet point
            emit(ANSI_CYAN "  • " ANSI_RESET);
            sm->pending_len = 0;
            sm->state = STATE_NORMAL;
            sm->line_start = false;
            return;
        }
        if (c == '*' && sm->pending[0] == '*') {
            // It's bold ** at line start, not a bullet
            sm->pending_len = 0;
            sm->state = STATE_IN_BOLD;
            emit(ANSI_BOLD);
            return;
        }
        if (c == '-' && sm->pending[0] == '-') {
            // Could be horizontal rule ---
            sm->pending[1] = '-';
            sm->pending_len = 2;
            sm->state = STATE_MAYBE_HR;
            return;
        }
        // Not a bullet, flush and process
        flush_pending(sm);
        sm->state = STATE_NORMAL;
        sm->line_start = false;
        stream_md_process_char(sm, c);
        break;

    case STATE_MAYBE_HR:
        if (c == '-' && sm->pending_len == 2) {
            // It's a horizontal rule ---
            emit(ANSI_DIM "────────────────────────────────" ANSI_RESET);
            sm->pending_len = 0;
            sm->state = STATE_NORMAL;
            // Skip to end of line would need buffering, simplified: assume it ends
            return;
        }
        // Not a HR, flush and process
        flush_pending(sm);
        sm->state = STATE_NORMAL;
        sm->line_start = false;
        stream_md_process_char(sm, c);
        break;

    case STATE_MAYBE_BOLD:
        if (c == '*') {
            // It's bold **
            sm->pending_len = 0;
            sm->state = STATE_IN_BOLD;
            emit(ANSI_BOLD);
            return;
        }
        // It's italic *
        sm->pending_len = 0;
        sm->state = STATE_IN_ITALIC;
        emit(ANSI_ITALIC);
        stream_md_process_char(sm, c);
        break;

    case STATE_IN_BOLD:
        if (c == '*') {
            sm->state = STATE_MAYBE_BOLD_END;
            return;
        }
        emit_char(c);
        break;

    case STATE_MAYBE_BOLD_END:
        if (c == '*') {
            // End of bold **
            emit(ANSI_RESET);
            sm->state = STATE_NORMAL;
            sm->line_start = false;
            return;
        }
        // Single * inside bold, emit it and continue
        emit_char('*');
        sm->state = STATE_IN_BOLD;
        stream_md_process_char(sm, c);
        break;

    case STATE_IN_ITALIC:
        if (c == '*' || c == '_') {
            // End of italic
            emit(ANSI_RESET);
            sm->state = STATE_NORMAL;
            sm->line_start = false;
            return;
        }
        if (c == '\n') {
            // Unclosed italic, reset
            emit(ANSI_RESET);
            emit_char('\n');
            sm->state = STATE_NORMAL;
            sm->line_start = true;
            return;
        }
        emit_char(c);
        break;

    case STATE_MAYBE_CODE:
        if (c == '`') {
            sm->pending[1] = '`';
            sm->pending_len = 2;
            sm->state = STATE_MAYBE_CODE_BLOCK;
            return;
        }
        // It's inline code `
        sm->pending_len = 0;
        sm->state = STATE_IN_CODE;
        emit(ANSI_DIM ANSI_GREEN);
        stream_md_process_char(sm, c);
        break;

    case STATE_MAYBE_CODE_BLOCK:
        if (c == '`') {
            // It's a code block ```
            sm->pending_len = 0;
            sm->state = STATE_IN_CODE_BLOCK;
            emit(ANSI_DIM ANSI_GREEN);
            // Skip language identifier until newline
            return;
        }
        // It's inline code with empty start ``
        sm->pending_len = 0;
        sm->state = STATE_IN_CODE;
        emit(ANSI_DIM ANSI_GREEN);
        emit_char('`');  // The second backtick
        stream_md_process_char(sm, c);
        break;

    case STATE_IN_CODE:
        if (c == '`') {
            emit(ANSI_RESET);
            sm->state = STATE_NORMAL;
            sm->line_start = false;
            return;
        }
        if (c == '\n') {
            emit(ANSI_RESET);
            emit_char('\n');
            sm->state = STATE_NORMAL;
            sm->line_start = true;
            return;
        }
        emit_char(c);
        break;

    case STATE_IN_CODE_BLOCK:
        if (sm->line_start && c == '`') {
            sm->pending[0] = '`';
            sm->pending_len = 1;
            // Check for closing ```
            return;
        }
        if (sm->pending_len > 0) {
            if (c == '`' && sm->pending_len < 3) {
                sm->pending[sm->pending_len++] = '`';
                if (sm->pending_len == 3) {
                    // End of code block
                    emit(ANSI_RESET);
                    sm->pending_len = 0;
                    sm->state = STATE_NORMAL;
                    sm->line_start = false;
                }
                return;
            } else {
                // Not closing, flush backticks
                flush_pending(sm);
            }
        }
        emit_char(c);
        sm->line_start = (c == '\n');
        break;

    case STATE_IN_LINK_TEXT:
        if (c == ']') {
            sm->state = STATE_MAYBE_LINK_URL;
            return;
        }
        if (c == '\n') {
            // Invalid link
            emit(ANSI_RESET);
            emit_char('[');
            for (int i = 0; i < sm->link_text_len; i++) {
                emit_char(sm->link_text[i]);
            }
            emit_char('\n');
            sm->state = STATE_NORMAL;
            sm->link_text_len = 0;
            sm->line_start = true;
            return;
        }
        if (sm->link_text_len < (int)sizeof(sm->link_text) - 1) {
            sm->link_text[sm->link_text_len++] = c;
        }
        emit_char(c);
        break;

    case STATE_MAYBE_LINK_URL:
        if (c == '(') {
            sm->state = STATE_IN_LINK_URL;
            return;
        }
        // Not a valid link, output as-is
        emit(ANSI_RESET);
        emit_char(']');
        sm->state = STATE_NORMAL;
        sm->line_start = false;
        stream_md_process_char(sm, c);
        break;

    case STATE_IN_LINK_URL:
        if (c == ')') {
            emit(ANSI_RESET);
            sm->state = STATE_NORMAL;
            sm->line_start = false;
            return;
        }
        // Silently consume URL characters
        break;
    }
}

/**
 * Process a chunk of text.
 */
void stream_md_process(StreamMd* sm, const char* chunk, size_t len) {
    for (size_t i = 0; i < len; i++) {
        stream_md_process_char(sm, chunk[i]);
    }
}

/**
 * Finalize rendering - flush any pending state.
 */
void stream_md_finish(StreamMd* sm) {
    // Close any open styles
    switch (sm->state) {
    case STATE_IN_BOLD:
    case STATE_MAYBE_BOLD_END:
    case STATE_IN_ITALIC:
    case STATE_IN_CODE:
    case STATE_IN_CODE_BLOCK:
    case STATE_IN_HEADER:
    case STATE_IN_LINK_TEXT:
    case STATE_IN_LINK_URL:
        emit(ANSI_RESET);
        break;
    default:
        break;
    }

    // Flush any pending characters
    flush_pending(sm);

    // Ensure newline at end
    emit_char('\n');

    sm->state = STATE_NORMAL;
    sm->line_start = true;
    sm->pending_len = 0;
}
