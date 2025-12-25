/**
 * ANSI Markdown Renderer
 *
 * Converts markdown to ANSI escape codes for terminal display.
 * Supports: headers, bold, italic, code, lists, horizontal rules.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ANSI escape codes
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_DIM "\033[2m"
#define ANSI_ITALIC "\033[3m"
#define ANSI_UNDERLINE "\033[4m"

// Colors
#define ANSI_CYAN "\033[36m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_GREEN "\033[32m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_BLUE "\033[34m"
#define ANSI_WHITE "\033[37m"
#define ANSI_GRAY "\033[90m"

// Bright colors
#define ANSI_BRIGHT_CYAN "\033[96m"
#define ANSI_BRIGHT_YELLOW "\033[93m"
#define ANSI_BRIGHT_GREEN "\033[92m"
#define ANSI_BRIGHT_WHITE "\033[97m"

// Background
#define ANSI_BG_GRAY "\033[100m"

/**
 * Render markdown string to ANSI-formatted string.
 * Caller must free the returned string.
 */
char* md_to_ansi(const char* markdown) {
    if (!markdown)
        return NULL;

    size_t len = strlen(markdown);
    // Allocate generous buffer (ANSI codes add overhead)
    size_t capacity = len * 3 + 1024;
    char* output = malloc(capacity);
    if (!output)
        return NULL;

    size_t out_pos = 0;
    const char* p = markdown;
    bool in_code_block = false;
    bool line_start = true;

#define APPEND(s)                                                                                  \
    do {                                                                                           \
        size_t slen = strlen(s);                                                                   \
        if (out_pos + slen >= capacity) {                                                          \
            capacity *= 2;                                                                         \
            output = realloc(output, capacity);                                                    \
            if (!output)                                                                           \
                return NULL;                                                                       \
        }                                                                                          \
        memcpy(output + out_pos, s, slen);                                                         \
        out_pos += slen;                                                                           \
    } while (0)

#define APPEND_CHAR(c)                                                                             \
    do {                                                                                           \
        if (out_pos + 1 >= capacity) {                                                             \
            capacity *= 2;                                                                         \
            output = realloc(output, capacity);                                                    \
            if (!output)                                                                           \
                return NULL;                                                                       \
        }                                                                                          \
        output[out_pos++] = c;                                                                     \
    } while (0)

    while (*p) {
        // Code block (```)
        if (line_start && strncmp(p, "```", 3) == 0) {
            if (in_code_block) {
                APPEND(ANSI_RESET);
                in_code_block = false;
            } else {
                APPEND(ANSI_DIM ANSI_GREEN);
                in_code_block = true;
            }
            // Skip to end of line
            p += 3;
            while (*p && *p != '\n')
                p++;
            if (*p == '\n') {
                APPEND_CHAR('\n');
                p++;
            }
            line_start = true;
            continue;
        }

        // Inside code block - pass through with color
        if (in_code_block) {
            if (*p == '\n') {
                APPEND_CHAR('\n');
                line_start = true;
            } else {
                APPEND_CHAR(*p);
                line_start = false;
            }
            p++;
            continue;
        }

        // Headers at line start
        if (line_start && *p == '#') {
            int level = 0;
            while (p[level] == '#' && level < 6)
                level++;

            if (p[level] == ' ') {
                p += level + 1; // Skip "### "

                // Style based on level
                if (level == 1) {
                    APPEND(ANSI_BOLD ANSI_BRIGHT_CYAN);
                } else if (level == 2) {
                    APPEND(ANSI_BOLD ANSI_CYAN);
                } else if (level == 3) {
                    APPEND(ANSI_BOLD ANSI_WHITE);
                } else {
                    APPEND(ANSI_BOLD);
                }

                // Copy header text until newline
                while (*p && *p != '\n') {
                    APPEND_CHAR(*p);
                    p++;
                }
                APPEND(ANSI_RESET);

                if (*p == '\n') {
                    APPEND_CHAR('\n');
                    p++;
                }
                line_start = true;
                continue;
            }
        }

        // Horizontal rule
        if (line_start && (strncmp(p, "---", 3) == 0 || strncmp(p, "***", 3) == 0)) {
            APPEND(ANSI_DIM "────────────────────────────────" ANSI_RESET "\n");
            p += 3;
            while (*p && *p != '\n')
                p++;
            if (*p == '\n')
                p++;
            line_start = true;
            continue;
        }

        // Bullet lists
        if (line_start && (*p == '-' || *p == '*' || *p == '+') && p[1] == ' ') {
            APPEND(ANSI_CYAN "  • " ANSI_RESET);
            p += 2;
            line_start = false;
            continue;
        }

        // Numbered lists
        if (line_start && *p >= '1' && *p <= '9') {
            const char* num_start = p;
            while (*p >= '0' && *p <= '9')
                p++;
            if (*p == '.' && p[1] == ' ') {
                APPEND(ANSI_CYAN "  ");
                // Copy number
                while (num_start < p) {
                    APPEND_CHAR(*num_start);
                    num_start++;
                }
                APPEND(". " ANSI_RESET);
                p += 2;
                line_start = false;
                continue;
            }
            p = num_start; // Reset if not a list
        }

        // Inline code (`code`)
        if (*p == '`' && p[1] != '`') {
            p++;
            APPEND(ANSI_DIM ANSI_GREEN);
            while (*p && *p != '`' && *p != '\n') {
                APPEND_CHAR(*p);
                p++;
            }
            APPEND(ANSI_RESET);
            if (*p == '`')
                p++;
            line_start = false;
            continue;
        }

        // Bold (**text**)
        if (*p == '*' && p[1] == '*') {
            p += 2;
            APPEND(ANSI_BOLD);
            while (*p && !(p[0] == '*' && p[1] == '*')) {
                APPEND_CHAR(*p);
                p++;
            }
            APPEND(ANSI_RESET);
            if (*p == '*')
                p += 2;
            line_start = false;
            continue;
        }

        // Italic (*text* or _text_)
        if ((*p == '*' || *p == '_') && p[1] != *p && p[1] != ' ') {
            char marker = *p;
            p++;
            APPEND(ANSI_ITALIC);
            while (*p && *p != marker && *p != '\n') {
                APPEND_CHAR(*p);
                p++;
            }
            APPEND(ANSI_RESET);
            if (*p == marker)
                p++;
            line_start = false;
            continue;
        }

        // Links [text](url) - show just text in underline
        if (*p == '[') {
            const char* bracket_end = strchr(p, ']');
            if (bracket_end && bracket_end[1] == '(') {
                const char* paren_end = strchr(bracket_end, ')');
                if (paren_end) {
                    p++; // Skip [
                    APPEND(ANSI_UNDERLINE ANSI_BLUE);
                    while (p < bracket_end) {
                        APPEND_CHAR(*p);
                        p++;
                    }
                    APPEND(ANSI_RESET);
                    p = paren_end + 1; // Skip ](url)
                    line_start = false;
                    continue;
                }
            }
        }

        // Regular character
        if (*p == '\n') {
            APPEND_CHAR('\n');
            line_start = true;
        } else {
            APPEND_CHAR(*p);
            line_start = false;
        }
        p++;
    }

    output[out_pos] = '\0';

#undef APPEND
#undef APPEND_CHAR

    return output;
}

/**
 * Print markdown directly to stdout with ANSI formatting.
 * Falls back to plain markdown if stdout is not a TTY.
 */
void md_print(const char* markdown) {
    if (!markdown)
        return;

    // If not a TTY (e.g., redirected to file), output plain markdown
    if (!isatty(STDOUT_FILENO)) {
        printf("%s", markdown);
        return;
    }

    char* formatted = md_to_ansi(markdown);
    if (formatted) {
        printf("%s", formatted);
        free(formatted);
    } else {
        printf("%s", markdown); // Fallback to raw
    }
}
