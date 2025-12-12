/**
 * CONVERGIO THEME SYSTEM
 *
 * Terminal color themes implementation.
 */

#include "nous/theme.h"
#include "nous/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// ANSI Reset
#define RST "\033[0m"

// ANSI Styles
#define BOLD "\033[1m"
#define DIM  "\033[2m"
#define ITAL "\033[3m"
#define ULINE "\033[4m"

// ANSI Colors (foreground)
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define GRAY    "\033[90m"

// Bright colors
#define BRIGHT_RED     "\033[91m"
#define BRIGHT_GREEN   "\033[92m"
#define BRIGHT_YELLOW  "\033[93m"
#define BRIGHT_BLUE    "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN    "\033[96m"
#define BRIGHT_WHITE   "\033[97m"

// 256-color mode
#define COLOR256(n) "\033[38;5;" #n "m"

// ============================================================================
// THEME DEFINITIONS
// ============================================================================

static const Theme THEMES[THEME_COUNT] = {
    // THEME_OCEAN - Cool blue/cyan tones (default)
    {
        .name = "Ocean",

        .prompt_name   = BOLD COLOR256(39),      // Bright blue
        .prompt_arrow  = COLOR256(39),           // Blue arrow
        .user_input    = COLOR256(117),          // Light cyan for user text

        .agent_name    = BOLD CYAN,
        .agent_text    = RST,

        .md_header1    = BOLD BRIGHT_CYAN,
        .md_header2    = BOLD CYAN,
        .md_header3    = BOLD WHITE,
        .md_bold       = BOLD,
        .md_italic     = ITAL,
        .md_code       = DIM GREEN,
        .md_link       = ULINE BLUE,
        .md_bullet     = CYAN,

        .success       = BRIGHT_GREEN,
        .warning       = YELLOW,
        .error         = BRIGHT_RED,
        .info          = DIM,

        .separator     = DIM CYAN,
        .cost          = COLOR256(39),
    },

    // THEME_FOREST - Green nature tones
    {
        .name = "Forest",

        .prompt_name   = BOLD COLOR256(34),      // Forest green
        .prompt_arrow  = COLOR256(76),           // Lime green arrow
        .user_input    = COLOR256(157),          // Light green for user text

        .agent_name    = BOLD GREEN,
        .agent_text    = RST,

        .md_header1    = BOLD BRIGHT_GREEN,
        .md_header2    = BOLD GREEN,
        .md_header3    = BOLD COLOR256(157),
        .md_bold       = BOLD,
        .md_italic     = ITAL,
        .md_code       = DIM COLOR256(22),       // Dark green
        .md_link       = ULINE COLOR256(30),     // Teal
        .md_bullet     = GREEN,

        .success       = BRIGHT_GREEN,
        .warning       = YELLOW,
        .error         = BRIGHT_RED,
        .info          = DIM,

        .separator     = DIM GREEN,
        .cost          = COLOR256(34),
    },

    // THEME_SUNSET - Warm orange/red tones
    {
        .name = "Sunset",

        .prompt_name   = BOLD COLOR256(208),     // Orange
        .prompt_arrow  = COLOR256(203),          // Coral arrow
        .user_input    = COLOR256(223),          // Peach for user text

        .agent_name    = BOLD COLOR256(208),
        .agent_text    = RST,

        .md_header1    = BOLD COLOR256(196),     // Red
        .md_header2    = BOLD COLOR256(208),     // Orange
        .md_header3    = BOLD COLOR256(220),     // Gold
        .md_bold       = BOLD,
        .md_italic     = ITAL,
        .md_code       = DIM COLOR256(130),      // Brown
        .md_link       = ULINE COLOR256(203),    // Coral
        .md_bullet     = COLOR256(208),

        .success       = BRIGHT_GREEN,
        .warning       = COLOR256(220),          // Gold warning
        .error         = BRIGHT_RED,
        .info          = DIM,

        .separator     = DIM COLOR256(208),
        .cost          = COLOR256(208),
    },

    // THEME_MONO - Classic grayscale
    {
        .name = "Mono",

        .prompt_name   = BOLD WHITE,
        .prompt_arrow  = GRAY,
        .user_input    = BRIGHT_WHITE,           // Bright white for user text

        .agent_name    = BOLD WHITE,
        .agent_text    = RST,

        .md_header1    = BOLD BRIGHT_WHITE,
        .md_header2    = BOLD WHITE,
        .md_header3    = WHITE,
        .md_bold       = BOLD,
        .md_italic     = ITAL,
        .md_code       = DIM,
        .md_link       = ULINE,
        .md_bullet     = GRAY,

        .success       = BRIGHT_WHITE,
        .warning       = WHITE,
        .error         = BOLD WHITE,
        .info          = DIM,

        .separator     = DIM,
        .cost          = WHITE,
    },
};

// Current theme
static ThemeId g_current_theme = THEME_OCEAN;

// ============================================================================
// THEME API
// ============================================================================

void theme_init(void) {
    // Load saved preference
    const char* saved = convergio_config_get("theme");
    if (saved) {
        theme_set_by_name(saved);
    }
}

const Theme* theme_get(void) {
    return &THEMES[g_current_theme];
}

const Theme* theme_get_by_id(ThemeId id) {
    if (id < THEME_COUNT) {
        return &THEMES[id];
    }
    return &THEMES[THEME_OCEAN];
}

void theme_set(ThemeId id) {
    if (id < THEME_COUNT) {
        g_current_theme = id;
    }
}

bool theme_set_by_name(const char* name) {
    if (!name) return false;

    for (int i = 0; i < THEME_COUNT; i++) {
        if (strcasecmp(name, THEMES[i].name) == 0) {
            g_current_theme = (ThemeId)i;
            return true;
        }
    }
    return false;
}

ThemeId theme_get_current_id(void) {
    return g_current_theme;
}

const char* theme_get_name(ThemeId id) {
    if (id < THEME_COUNT) {
        return THEMES[id].name;
    }
    return "Unknown";
}

void theme_list(void) {
    const Theme* current = theme_get();

    printf("\nAvailable themes:\n\n");
    for (int i = 0; i < THEME_COUNT; i++) {
        const Theme* t = &THEMES[i];
        bool is_current = ((ThemeId)i == g_current_theme);

        // Show theme name with its colors
        printf("  %s%s%-10s%s",
               is_current ? "▶ " : "  ",
               t->prompt_name,
               t->name,
               RST);

        // Show preview
        printf(" │ %sPrompt%s %s❯%s %suser input%s │ %sAgent%s\n",
               t->prompt_name, RST,
               t->prompt_arrow, RST,
               t->user_input, RST,
               t->agent_name, RST);
    }
    printf("\n");
    printf("Current: %s%s%s\n", current->prompt_name, current->name, RST);
    printf("Use: theme <name> to change\n\n");
}

void theme_save(void) {
    convergio_config_set("theme", THEMES[g_current_theme].name);
    convergio_config_save();
}

const char* theme_reset(void) {
    return RST;
}
