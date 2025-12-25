/**
 * Convergio Projects Module
 *
 * Manages named projects with dedicated agent teams and persistent context.
 * Each project has a purpose, team of agents, and shared memory.
 */

#ifndef CONVERGIO_PROJECTS_H
#define CONVERGIO_PROJECTS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Team member in a project
typedef struct {
    char* agent_name; // Short name (e.g., "baccio")
    char* role;       // Optional role description
} ProjectTeamMember;

// Project definition
typedef struct {
    char* name;          // Project name (e.g., "MyApp 2.0")
    char* slug;          // URL-safe name (e.g., "myapp-2.0")
    char* purpose;       // Project description/goal
    char* template_name; // Template used (if any)

    ProjectTeamMember* team;
    size_t team_count;

    time_t created;
    time_t last_active;

    char* storage_path; // Path to project directory

    // Context summary (loaded from context.json)
    char* context_summary;
    char** key_decisions;
    size_t decision_count;
    char* current_focus;
} ConvergioProject;

// Project manager state
typedef struct {
    ConvergioProject* current; // Currently active project (NULL if none)
    ConvergioProject** all_projects;
    size_t project_count;
    char* projects_base_path; // ~/.convergio/projects/
} ProjectManager;

// Project template definition
typedef struct {
    const char* name;
    const char* description;
    const char** default_team;
    size_t team_count;
} ProjectTemplate;

// ============================================================================
// INITIALIZATION & CLEANUP
// ============================================================================

// Initialize project manager
bool projects_init(void);

// Shutdown and cleanup
void projects_shutdown(void);

// Get project manager instance
ProjectManager* projects_get_manager(void);

// ============================================================================
// PROJECT CRUD OPERATIONS
// ============================================================================

// Create a new project
// team_names is comma-separated list of agent names (e.g., "baccio,davide,stefano")
ConvergioProject* project_create(const char* name, const char* purpose, const char* team_names,
                                 const char* template_name);

// Load a project from disk
ConvergioProject* project_load(const char* slug);

// Save project to disk
bool project_save(ConvergioProject* project);

// Delete a project
bool project_delete(const char* slug);

// Archive a project (move to archives/)
bool project_archive(const char* slug);

// List all projects
ConvergioProject** project_list_all(size_t* count);

// Find project by name or slug
ConvergioProject* project_find(const char* name_or_slug);

// ============================================================================
// CURRENT PROJECT
// ============================================================================

// Set the current active project
bool project_use(const char* name_or_slug);

// Get current project (NULL if none)
ConvergioProject* project_current(void);

// Clear current project
void project_clear_current(void);

// Check if an agent is in the current project's team
bool project_has_agent(const char* agent_name);

// Get list of agent names in current project (NULL-terminated)
const char** project_get_team_agents(void);

// ============================================================================
// TEAM MANAGEMENT
// ============================================================================

// Add an agent to project team
bool project_team_add(ConvergioProject* project, const char* agent_name, const char* role);

// Remove an agent from project team
bool project_team_remove(ConvergioProject* project, const char* agent_name);

// Set agent role in project
bool project_team_set_role(ConvergioProject* project, const char* agent_name, const char* role);

// ============================================================================
// CONTEXT MANAGEMENT
// ============================================================================

// Update project context summary
bool project_update_context(ConvergioProject* project, const char* summary,
                            const char* current_focus);

// Add a key decision to project
bool project_add_decision(ConvergioProject* project, const char* decision);

// Load project context from disk
bool project_load_context(ConvergioProject* project);

// Save project context to disk
bool project_save_context(ConvergioProject* project);

// ============================================================================
// HISTORY MANAGEMENT
// ============================================================================

// Append a conversation turn to project history
bool project_append_history(ConvergioProject* project, const char* role, const char* content,
                            const char* agent_name);

// Load recent history (returns number of turns loaded)
size_t project_load_history(ConvergioProject* project, size_t max_turns, char*** roles,
                            char*** contents, char*** agents);

// Clear project history
bool project_clear_history(ConvergioProject* project);

// ============================================================================
// TEMPLATES
// ============================================================================

// Get all available templates
const ProjectTemplate* project_get_templates(size_t* count);

// Get a specific template by name
const ProjectTemplate* project_get_template(const char* name);

// ============================================================================
// UTILITIES
// ============================================================================

// Convert name to URL-safe slug
char* project_name_to_slug(const char* name);

// Get project storage path
const char* project_get_storage_path(const char* slug);

// Free a project structure
void project_free(ConvergioProject* project);

// Update last_active timestamp
void project_touch(ConvergioProject* project);

#endif // CONVERGIO_PROJECTS_H
