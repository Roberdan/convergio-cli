/**
 * CONVERGIO STATUS BAR
 *
 * Real-time status display at the bottom of the terminal showing:
 * - Current user and workspace
 * - Active model and profile
 * - Token usage and cost
 * - Active agents and background tasks
 * - Editor integration
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_STATUSBAR_H
#define CONVERGIO_STATUSBAR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// STATUS BAR STATE
// ============================================================================

typedef struct {
    // User info
    char* username;
    char* cwd_basename;         // Current working directory basename

    // Model info
    char* active_model;         // Current model (e.g., "Opus 4.5")
    char* profile_name;         // Active profile (e.g., "default")

    // Token/cost tracking
    uint64_t session_tokens;
    double session_cost;

    // Agent info
    int active_agents;
    int background_tasks;

    // Permissions
    bool bypass_permissions;

    // Version
    char* version;

    // Editor
    char* editor;

    // Display state
    bool visible;
    bool dirty;                 // Needs redraw
    int terminal_width;
    int terminal_height;
} StatusBarState;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * Initialize the status bar
 * Detects terminal size and username
 *
 * @return 0 on success, -1 on error
 */
int statusbar_init(void);

/**
 * Shutdown and free resources
 */
void statusbar_shutdown(void);

/**
 * Check if status bar is available (TTY, sufficient size)
 *
 * @return true if status bar can be displayed
 */
bool statusbar_available(void);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * Render the status bar at the bottom of the terminal
 * Should be called after terminal output
 */
void statusbar_render(void);

/**
 * Clear the status bar (restore terminal lines)
 */
void statusbar_clear(void);

/**
 * Show or hide the status bar
 *
 * @param visible true to show, false to hide
 */
void statusbar_set_visible(bool visible);

/**
 * Check if status bar is visible
 *
 * @return true if visible
 */
bool statusbar_is_visible(void);

// ============================================================================
// STATE UPDATES
// ============================================================================

/**
 * Set the current working directory
 *
 * @param path Full path (will extract basename)
 */
void statusbar_set_cwd(const char* path);

/**
 * Set the active model name
 *
 * @param model Model display name (e.g., "Opus 4.5")
 */
void statusbar_set_model(const char* model);

/**
 * Set the active profile name
 *
 * @param profile Profile name (e.g., "default")
 */
void statusbar_set_profile(const char* profile);

/**
 * Add tokens to the session counter
 *
 * @param tokens Number of tokens used
 */
void statusbar_add_tokens(uint64_t tokens);

/**
 * Add cost to the session counter
 *
 * @param cost Cost in USD
 */
void statusbar_add_cost(double cost);

/**
 * Set the number of active agents
 *
 * @param count Number of active agents
 */
void statusbar_set_agent_count(int count);

/**
 * Set the number of background tasks
 *
 * @param count Number of background tasks
 */
void statusbar_set_background_tasks(int count);

/**
 * Set bypass permissions mode
 *
 * @param enabled true if bypass mode is on
 */
void statusbar_set_bypass_permissions(bool enabled);

/**
 * Set the detected editor name
 *
 * @param editor Editor name (e.g., "Zed", "nvim")
 */
void statusbar_set_editor(const char* editor);

// ============================================================================
// GETTERS
// ============================================================================

/**
 * Get current session token count
 *
 * @return Total tokens used in session
 */
uint64_t statusbar_get_tokens(void);

/**
 * Get current session cost
 *
 * @return Total cost in USD
 */
double statusbar_get_cost(void);

/**
 * Get the current status bar state
 *
 * @return Pointer to state (do not free)
 */
const StatusBarState* statusbar_get_state(void);

// ============================================================================
// TERMINAL HANDLING
// ============================================================================

/**
 * Handle terminal resize (SIGWINCH)
 * Call this when terminal size changes
 */
void statusbar_handle_resize(void);

/**
 * Get terminal dimensions
 *
 * @param width Output for terminal width
 * @param height Output for terminal height
 */
void statusbar_get_terminal_size(int* width, int* height);

#endif // CONVERGIO_STATUSBAR_H
