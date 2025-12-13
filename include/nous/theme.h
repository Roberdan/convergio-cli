/**
 * CONVERGIO THEME SYSTEM
 *
 * Terminal color themes for customizing the CLI appearance.
 */

#ifndef CONVERGIO_THEME_H
#define CONVERGIO_THEME_H

#include <stdbool.h>

// Theme identifiers
typedef enum {
    THEME_OCEAN,      // Default: Blue/cyan tones (cool, professional)
    THEME_FOREST,     // Green tones (nature, calm)
    THEME_SUNSET,     // Orange/red tones (warm, energetic)
    THEME_MONO,       // Grayscale (minimal, classic terminal)
    THEME_LIGHT,      // Light mode for bright environments
    THEME_DARK,       // Dark mode for OLED screens
    THEME_COLORBLIND, // Accessible colors (blue/orange, safe for all types)
    THEME_COUNT
} ThemeId;

// Color roles in the UI
typedef struct {
    const char* name;           // Theme name for display

    // Prompt colors
    const char* prompt_name;    // "Convergio" text color
    const char* prompt_arrow;   // ">" arrow color
    const char* user_input;     // User typed text color

    // Agent/response colors
    const char* agent_name;     // Agent name header
    const char* agent_text;     // Normal response text

    // Markdown colors
    const char* md_header1;     // # Header
    const char* md_header2;     // ## Header
    const char* md_header3;     // ### Header
    const char* md_bold;        // **bold**
    const char* md_italic;      // *italic*
    const char* md_code;        // `code` and ```blocks```
    const char* md_link;        // [links](url)
    const char* md_bullet;      // Bullet points

    // Status colors
    const char* success;        // Success messages
    const char* warning;        // Warning messages
    const char* error;          // Error messages
    const char* info;           // Info/dim text

    // Special
    const char* separator;      // Line separators
    const char* cost;           // Cost display
} Theme;

// Initialize theme system (loads saved preference)
void theme_init(void);

// Get current theme
const Theme* theme_get(void);

// Get theme by ID
const Theme* theme_get_by_id(ThemeId id);

// Set current theme
void theme_set(ThemeId id);

// Set theme by name (returns false if not found)
bool theme_set_by_name(const char* name);

// Get current theme ID
ThemeId theme_get_current_id(void);

// Get theme name
const char* theme_get_name(ThemeId id);

// List all available themes
void theme_list(void);

// Save theme preference
void theme_save(void);

// Helper: Get ANSI reset code
const char* theme_reset(void);

// Helper: Apply color and return the escape sequence
// Usage: printf("%sText%s", theme_color(t->success), theme_reset());
#define theme_color(c) (c)

#endif // CONVERGIO_THEME_H
