/**
 * Convergio Projects Module - Implementation
 *
 * Manages named projects with dedicated agent teams and persistent context.
 */

#include "projects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define MAX_PROJECTS 100
#define MAX_TEAM_SIZE 20
#define MAX_DECISIONS 50
#define MAX_HISTORY_LINE 8192
#define PROJECT_FILE "project.yaml"
#define CONTEXT_FILE "context.json"
#define HISTORY_FILE "history.jsonl"

// ============================================================================
// GLOBAL STATE
// ============================================================================

static ProjectManager* g_project_manager = NULL;

// ============================================================================
// PROJECT TEMPLATES
// ============================================================================

static const char* template_app_dev_team[] = {
    "baccio", "davide", "stefano", "tester", "devops", NULL
};

static const char* template_marketing_team[] = {
    "copywriter", "designer", "analyst", "matteo", NULL
};

static const char* template_research_team[] = {
    "omri", "data-scientist", "researcher", "writer", NULL
};

static const char* template_executive_team[] = {
    "matteo", "amy", "consultant", "writer", NULL
};

static const char* template_finance_team[] = {
    "amy", "fiona", "analyst", "consultant", NULL
};

static ProjectTemplate g_templates[] = {
    {
        .name = "app-dev",
        .description = "Application development with architecture, coding, testing, and DevOps",
        .default_team = template_app_dev_team,
        .team_count = 5
    },
    {
        .name = "marketing",
        .description = "Marketing campaign with creative, analytics, and strategy",
        .default_team = template_marketing_team,
        .team_count = 4
    },
    {
        .name = "research",
        .description = "Research project with data science and documentation",
        .default_team = template_research_team,
        .team_count = 4
    },
    {
        .name = "executive",
        .description = "Executive presentation with strategy and finance",
        .default_team = template_executive_team,
        .team_count = 4
    },
    {
        .name = "finance",
        .description = "Financial analysis with CFO, market analyst, and consulting",
        .default_team = template_finance_team,
        .team_count = 4
    }
};

static const size_t g_template_count = sizeof(g_templates) / sizeof(g_templates[0]);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Create directory recursively
static bool mkdir_recursive(const char* path) {
    char* p = strdup(path);
    if (!p) return false;

    for (char* s = p + 1; *s; s++) {
        if (*s == '/') {
            *s = '\0';
            if (mkdir(p, 0755) != 0 && errno != EEXIST) {
                free(p);
                return false;
            }
            *s = '/';
        }
    }
    if (mkdir(p, 0755) != 0 && errno != EEXIST) {
        free(p);
        return false;
    }
    free(p);
    return true;
}

// Check if file exists
static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

// Read entire file into string
static char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = malloc((size_t)size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(content, 1, (size_t)size, f);
    content[read] = '\0';
    fclose(f);

    return content;
}

// Write string to file
static bool write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "w");
    if (!f) return false;

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);

    return written == len;
}

// Escape string for JSON
static char* json_escape_str(const char* str) {
    if (!str) return strdup("");

    size_t len = strlen(str);
    char* escaped = malloc(len * 2 + 1);
    if (!escaped) return NULL;

    char* out = escaped;
    for (const char* p = str; *p; p++) {
        switch (*p) {
            case '"': *out++ = '\\'; *out++ = '"'; break;
            case '\\': *out++ = '\\'; *out++ = '\\'; break;
            case '\n': *out++ = '\\'; *out++ = 'n'; break;
            case '\r': *out++ = '\\'; *out++ = 'r'; break;
            case '\t': *out++ = '\\'; *out++ = 't'; break;
            default: *out++ = *p;
        }
    }
    *out = '\0';
    return escaped;
}

// Simple JSON string extraction (finds "key": "value")
static char* json_get_string(const char* json, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\"", key);

    const char* found = strstr(json, search);
    if (!found) return NULL;

    // Find the colon
    const char* colon = strchr(found + strlen(search), ':');
    if (!colon) return NULL;

    // Skip whitespace and find opening quote
    const char* start = colon + 1;
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n')) start++;
    if (*start != '"') return NULL;
    start++;

    // Find closing quote (handle escapes)
    const char* end = start;
    while (*end && *end != '"') {
        if (*end == '\\' && *(end + 1)) end += 2;
        else end++;
    }

    size_t len = (size_t)(end - start);
    char* value = malloc(len + 1);
    if (!value) return NULL;
    strncpy(value, start, len);
    value[len] = '\0';

    return value;
}

// ============================================================================
// NAME TO SLUG CONVERSION
// ============================================================================

char* project_name_to_slug(const char* name) {
    if (!name) return NULL;

    size_t len = strlen(name);
    char* slug = malloc(len + 1);
    if (!slug) return NULL;

    char* out = slug;
    bool last_was_dash = true;  // Prevent leading dash

    for (const char* p = name; *p; p++) {
        if (isalnum(*p)) {
            *out++ = (char)tolower(*p);
            last_was_dash = false;
        } else if (!last_was_dash) {
            *out++ = '-';
            last_was_dash = true;
        }
    }

    // Remove trailing dash
    if (out > slug && *(out - 1) == '-') {
        out--;
    }

    *out = '\0';
    return slug;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool projects_init(void) {
    if (g_project_manager) return true;  // Already initialized

    g_project_manager = calloc(1, sizeof(ProjectManager));
    if (!g_project_manager) return false;

    // Set base path
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";

    size_t path_len = strlen(home) + 32;
    g_project_manager->projects_base_path = malloc(path_len);
    if (!g_project_manager->projects_base_path) {
        free(g_project_manager);
        g_project_manager = NULL;
        return false;
    }
    snprintf(g_project_manager->projects_base_path, path_len,
             "%s/.convergio/projects", home);

    // Create projects directory if it doesn't exist
    mkdir_recursive(g_project_manager->projects_base_path);

    // Load existing projects
    DIR* dir = opendir(g_project_manager->projects_base_path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) && g_project_manager->project_count < MAX_PROJECTS) {
            if (entry->d_name[0] == '.') continue;

            // Check if it's a directory with project.yaml
            char project_file[1024];
            snprintf(project_file, sizeof(project_file), "%s/%s/%s",
                     g_project_manager->projects_base_path, entry->d_name, PROJECT_FILE);

            if (file_exists(project_file)) {
                ConvergioProject* proj = project_load(entry->d_name);
                if (proj) {
                    ConvergioProject** new_all_projects = realloc(
                        g_project_manager->all_projects,
                        (g_project_manager->project_count + 1) * sizeof(ConvergioProject*)
                    );
                    if (!new_all_projects) {
                        project_free(proj);
                    } else {
                        g_project_manager->all_projects = new_all_projects;
                        g_project_manager->all_projects[g_project_manager->project_count++] = proj;
                    }
                }
            }
        }
        closedir(dir);
    }

    // Check for last used project
    char last_project_file[1024];
    snprintf(last_project_file, sizeof(last_project_file),
             "%s/.convergio/last_project", home);

    if (file_exists(last_project_file)) {
        char* last_slug = read_file(last_project_file);
        if (last_slug) {
            // Trim whitespace
            char* end = last_slug + strlen(last_slug) - 1;
            while (end > last_slug && isspace(*end)) *end-- = '\0';

            project_use(last_slug);
            free(last_slug);
        }
    }

    return true;
}

void projects_shutdown(void) {
    if (!g_project_manager) return;

    // Save current project
    if (g_project_manager->current) {
        project_save(g_project_manager->current);

        // Save last project slug
        const char* home = getenv("HOME");
        if (home) {
            char last_project_file[1024];
            snprintf(last_project_file, sizeof(last_project_file),
                     "%s/.convergio/last_project", home);
            write_file(last_project_file, g_project_manager->current->slug);
        }
    }

    // Free all projects
    for (size_t i = 0; i < g_project_manager->project_count; i++) {
        project_free(g_project_manager->all_projects[i]);
    }
    free(g_project_manager->all_projects);
    free(g_project_manager->projects_base_path);
    free(g_project_manager);
    g_project_manager = NULL;
}

ProjectManager* projects_get_manager(void) {
    return g_project_manager;
}

// ============================================================================
// PROJECT STORAGE PATH
// ============================================================================

const char* project_get_storage_path(const char* slug) {
    if (!g_project_manager || !slug) return NULL;

    static char path[1024];
    snprintf(path, sizeof(path), "%s/%s",
             g_project_manager->projects_base_path, slug);
    return path;
}

// ============================================================================
// PROJECT CREATE
// ============================================================================

ConvergioProject* project_create(const char* name, const char* purpose,
                                  const char* team_names, const char* template_name) {
    if (!g_project_manager || !name) return NULL;

    ConvergioProject* proj = calloc(1, sizeof(ConvergioProject));
    if (!proj) return NULL;

    proj->name = strdup(name);
    proj->slug = project_name_to_slug(name);
    proj->purpose = purpose ? strdup(purpose) : strdup("");
    proj->template_name = template_name ? strdup(template_name) : NULL;
    proj->created = time(NULL);
    proj->last_active = proj->created;

    // Set storage path
    const char* base_path = project_get_storage_path(proj->slug);
    proj->storage_path = strdup(base_path);

    // Create directory
    mkdir_recursive(proj->storage_path);

    // Create artifacts directory
    char artifacts_path[1024];
    snprintf(artifacts_path, sizeof(artifacts_path), "%s/artifacts", proj->storage_path);
    mkdir_recursive(artifacts_path);

    // Parse team names or use template
    if (team_names && strlen(team_names) > 0) {
        // Parse comma-separated team names
        char* team_copy = strdup(team_names);
        if (!team_copy) {
            project_free(proj);
            return NULL;
        }
        char* token = strtok(team_copy, ",");
        while (token && proj->team_count < MAX_TEAM_SIZE) {
            // Trim whitespace
            while (*token == ' ') token++;
            char* end = token + strlen(token) - 1;
            while (end > token && *end == ' ') *end-- = '\0';

            if (strlen(token) > 0) {
                ProjectTeamMember* new_team = realloc(
                    proj->team,
                    (proj->team_count + 1) * sizeof(ProjectTeamMember)
                );
                if (!new_team) {
                    break;
                }
                proj->team = new_team;
                proj->team[proj->team_count].agent_name = strdup(token);
                proj->team[proj->team_count].role = NULL;
                proj->team_count++;
            }
            token = strtok(NULL, ",");
        }
        free(team_copy);
    } else if (template_name) {
        // Use template team
        const ProjectTemplate* tmpl = project_get_template(template_name);
        if (tmpl) {
            for (size_t i = 0; i < tmpl->team_count && tmpl->default_team[i]; i++) {
                ProjectTeamMember* new_team = realloc(
                    proj->team,
                    (proj->team_count + 1) * sizeof(ProjectTeamMember)
                );
                if (!new_team) {
                    break;
                }
                proj->team = new_team;
                proj->team[proj->team_count].agent_name = strdup(tmpl->default_team[i]);
                proj->team[proj->team_count].role = NULL;
                proj->team_count++;
            }
        }
    }

    // Save to disk
    if (!project_save(proj)) {
        project_free(proj);
        return NULL;
    }

    // Add to manager
    ConvergioProject** new_all_projects = realloc(
        g_project_manager->all_projects,
        (g_project_manager->project_count + 1) * sizeof(ConvergioProject*)
    );
    if (!new_all_projects) {
        project_free(proj);
        return NULL;
    }
    g_project_manager->all_projects = new_all_projects;
    g_project_manager->all_projects[g_project_manager->project_count++] = proj;

    return proj;
}

// ============================================================================
// PROJECT SAVE
// ============================================================================

bool project_save(ConvergioProject* project) {
    if (!project || !project->storage_path) return false;

    const char* name = project->name ? project->name : "";
    const char* slug = project->slug ? project->slug : "";
    const char* purpose = project->purpose ? project->purpose : "";
    const char* template_name = project->template_name ? project->template_name : NULL;

    // Build YAML content (allocate sufficient space to avoid truncation/UB)
    size_t yaml_size = 0;
    yaml_size += 128;
    yaml_size += strlen(name) + strlen(slug) + strlen(purpose);
    if (template_name) {
        yaml_size += strlen(template_name) + 32;
    }
    yaml_size += 64;
    for (size_t i = 0; i < project->team_count; i++) {
        const char* agent_name = project->team[i].agent_name ? project->team[i].agent_name : "";
        yaml_size += strlen(agent_name) + 32;
        if (project->team[i].role) {
            yaml_size += strlen(project->team[i].role) + 32;
        }
    }

    char* yaml = malloc(yaml_size);
    if (!yaml) return false;

    size_t off = 0;
    int written = snprintf(
        yaml + off,
        yaml_size - off,
        "version: 1\n"
        "name: \"%s\"\n"
        "slug: \"%s\"\n"
        "purpose: \"%s\"\n",
        name, slug, purpose
    );
    if (written < 0 || (size_t)written >= yaml_size - off) {
        free(yaml);
        return false;
    }
    off += (size_t)written;

    if (template_name) {
        written = snprintf(yaml + off, yaml_size - off, "template: \"%s\"\n", template_name);
        if (written < 0 || (size_t)written >= yaml_size - off) {
            free(yaml);
            return false;
        }
        off += (size_t)written;
    }

    written = snprintf(
        yaml + off,
        yaml_size - off,
        "created: %ld\n"
        "last_active: %ld\n"
        "team:\n",
        (long)project->created, (long)project->last_active
    );
    if (written < 0 || (size_t)written >= yaml_size - off) {
        free(yaml);
        return false;
    }
    off += (size_t)written;

    for (size_t i = 0; i < project->team_count; i++) {
        const char* agent_name = project->team[i].agent_name ? project->team[i].agent_name : "";
        written = snprintf(yaml + off, yaml_size - off, "  - name: \"%s\"\n", agent_name);
        if (written < 0 || (size_t)written >= yaml_size - off) {
            free(yaml);
            return false;
        }
        off += (size_t)written;

        if (project->team[i].role) {
            written = snprintf(yaml + off, yaml_size - off, "    role: \"%s\"\n", project->team[i].role);
            if (written < 0 || (size_t)written >= yaml_size - off) {
                free(yaml);
                return false;
            }
            off += (size_t)written;
        }
    }

    // Write project.yaml
    char project_file[1024];
    snprintf(project_file, sizeof(project_file), "%s/%s", project->storage_path, PROJECT_FILE);
    bool success = write_file(project_file, yaml);
    free(yaml);

    if (!success) return false;

    // Also save context
    project_save_context(project);

    return true;
}

// ============================================================================
// PROJECT LOAD
// ============================================================================

ConvergioProject* project_load(const char* slug) {
    if (!g_project_manager || !slug) return NULL;

    const char* base_path = project_get_storage_path(slug);
    char project_file[1024];
    snprintf(project_file, sizeof(project_file), "%s/%s", base_path, PROJECT_FILE);

    char* yaml = read_file(project_file);
    if (!yaml) return NULL;

    ConvergioProject* proj = calloc(1, sizeof(ConvergioProject));
    if (!proj) {
        free(yaml);
        return NULL;
    }

    proj->storage_path = strdup(base_path);
    proj->slug = strdup(slug);

    // Parse YAML (simple line-by-line parsing)
    char* line = strtok(yaml, "\n");
    bool in_team = false;
    char* current_agent = NULL;

    while (line) {
        // Skip whitespace
        while (*line == ' ' || *line == '\t') line++;

        if (strncmp(line, "name:", 5) == 0) {
            char* val = strchr(line, '"');
            if (val) {
                val++;
                char* end = strchr(val, '"');
                if (end) {
                    *end = '\0';
                    proj->name = strdup(val);
                }
            }
            in_team = false;
        } else if (strncmp(line, "purpose:", 8) == 0) {
            char* val = strchr(line, '"');
            if (val) {
                val++;
                char* end = strchr(val, '"');
                if (end) {
                    *end = '\0';
                    proj->purpose = strdup(val);
                }
            }
            in_team = false;
        } else if (strncmp(line, "template:", 9) == 0) {
            char* val = strchr(line, '"');
            if (val) {
                val++;
                char* end = strchr(val, '"');
                if (end) {
                    *end = '\0';
                    proj->template_name = strdup(val);
                }
            }
            in_team = false;
        } else if (strncmp(line, "created:", 8) == 0) {
            proj->created = atol(line + 8);
            in_team = false;
        } else if (strncmp(line, "last_active:", 12) == 0) {
            proj->last_active = atol(line + 12);
            in_team = false;
        } else if (strncmp(line, "team:", 5) == 0) {
            in_team = true;
        } else if (in_team && strncmp(line, "- name:", 7) == 0) {
            char* val = strchr(line, '"');
            if (val) {
                val++;
                char* end = strchr(val, '"');
                if (end) {
                    *end = '\0';
                    current_agent = strdup(val);

                    ProjectTeamMember* new_team = realloc(
                        proj->team,
                        (proj->team_count + 1) * sizeof(ProjectTeamMember)
                    );
                    if (!new_team) {
                        free(current_agent);
                        current_agent = NULL;
                    } else {
                        proj->team = new_team;
                        proj->team[proj->team_count].agent_name = current_agent;
                        proj->team[proj->team_count].role = NULL;
                        proj->team_count++;
                    }
                }
            }
        } else if (in_team && strncmp(line, "role:", 5) == 0) {
            char* val = strchr(line, '"');
            if (val && proj->team_count > 0) {
                val++;
                char* end = strchr(val, '"');
                if (end) {
                    *end = '\0';
                    proj->team[proj->team_count - 1].role = strdup(val);
                }
            }
        }

        line = strtok(NULL, "\n");
    }

    free(yaml);

    // Load context
    project_load_context(proj);

    return proj;
}

// ============================================================================
// PROJECT DELETE / ARCHIVE
// ============================================================================

bool project_delete(const char* slug) {
    if (!g_project_manager || !slug) return false;

    // Remove from current if active
    if (g_project_manager->current && strcmp(g_project_manager->current->slug, slug) == 0) {
        g_project_manager->current = NULL;
    }

    // Find and remove from list
    for (size_t i = 0; i < g_project_manager->project_count; i++) {
        if (strcmp(g_project_manager->all_projects[i]->slug, slug) == 0) {
            project_free(g_project_manager->all_projects[i]);

            // Shift remaining projects
            for (size_t j = i; j < g_project_manager->project_count - 1; j++) {
                g_project_manager->all_projects[j] = g_project_manager->all_projects[j + 1];
            }
            g_project_manager->project_count--;
            break;
        }
    }

    // Delete directory (just rename to .deleted for safety)
    const char* path = project_get_storage_path(slug);
    char deleted_path[1024];
    snprintf(deleted_path, sizeof(deleted_path), "%s.deleted", path);
    rename(path, deleted_path);

    return true;
}

bool project_archive(const char* slug) {
    if (!g_project_manager || !slug) return false;

    // Create archives directory
    char archives_path[1024];
    snprintf(archives_path, sizeof(archives_path), "%s/../archives",
             g_project_manager->projects_base_path);
    mkdir_recursive(archives_path);

    // Move project
    const char* src = project_get_storage_path(slug);
    char dst[1024];
    snprintf(dst, sizeof(dst), "%s/%s", archives_path, slug);

    if (rename(src, dst) != 0) return false;

    // Remove from current if active
    if (g_project_manager->current && strcmp(g_project_manager->current->slug, slug) == 0) {
        g_project_manager->current = NULL;
    }

    // Remove from list
    for (size_t i = 0; i < g_project_manager->project_count; i++) {
        if (strcmp(g_project_manager->all_projects[i]->slug, slug) == 0) {
            project_free(g_project_manager->all_projects[i]);
            for (size_t j = i; j < g_project_manager->project_count - 1; j++) {
                g_project_manager->all_projects[j] = g_project_manager->all_projects[j + 1];
            }
            g_project_manager->project_count--;
            break;
        }
    }

    return true;
}

// ============================================================================
// PROJECT LIST & FIND
// ============================================================================

ConvergioProject** project_list_all(size_t* count) {
    if (!g_project_manager) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = g_project_manager->project_count;
    return g_project_manager->all_projects;
}

ConvergioProject* project_find(const char* name_or_slug) {
    if (!g_project_manager || !name_or_slug) return NULL;

    for (size_t i = 0; i < g_project_manager->project_count; i++) {
        ConvergioProject* p = g_project_manager->all_projects[i];
        if (strcasecmp(p->slug, name_or_slug) == 0 ||
            strcasecmp(p->name, name_or_slug) == 0) {
            return p;
        }
    }
    return NULL;
}

// ============================================================================
// CURRENT PROJECT
// ============================================================================

bool project_use(const char* name_or_slug) {
    if (!g_project_manager || !name_or_slug) return false;

    ConvergioProject* proj = project_find(name_or_slug);
    if (!proj) return false;

    // Save previous project
    if (g_project_manager->current) {
        project_save(g_project_manager->current);
    }

    g_project_manager->current = proj;
    project_touch(proj);

    return true;
}

ConvergioProject* project_current(void) {
    return g_project_manager ? g_project_manager->current : NULL;
}

void project_clear_current(void) {
    if (g_project_manager) {
        if (g_project_manager->current) {
            project_save(g_project_manager->current);
        }
        g_project_manager->current = NULL;
    }
}

bool project_has_agent(const char* agent_name) {
    ConvergioProject* proj = project_current();
    if (!proj || !agent_name) return true;  // No project = all agents available

    for (size_t i = 0; i < proj->team_count; i++) {
        if (strcasecmp(proj->team[i].agent_name, agent_name) == 0) {
            return true;
        }
        // Also check prefix match
        size_t name_len = strlen(proj->team[i].agent_name);
        if (strncasecmp(agent_name, proj->team[i].agent_name, name_len) == 0) {
            char next = agent_name[name_len];
            if (next == '\0' || next == '-' || next == '_') {
                return true;
            }
        }
    }
    return false;
}

const char** project_get_team_agents(void) {
    ConvergioProject* proj = project_current();
    if (!proj) return NULL;

    static const char* agents[MAX_TEAM_SIZE + 1];
    for (size_t i = 0; i < proj->team_count && i < MAX_TEAM_SIZE; i++) {
        agents[i] = proj->team[i].agent_name;
    }
    agents[proj->team_count] = NULL;
    return agents;
}

// ============================================================================
// TEAM MANAGEMENT
// ============================================================================

bool project_team_add(ConvergioProject* project, const char* agent_name, const char* role) {
    if (!project || !agent_name || project->team_count >= MAX_TEAM_SIZE) return false;

    // Check if already in team
    for (size_t i = 0; i < project->team_count; i++) {
        if (strcasecmp(project->team[i].agent_name, agent_name) == 0) {
            return false;  // Already in team
        }
    }

    ProjectTeamMember* new_team = realloc(
        project->team,
        (project->team_count + 1) * sizeof(ProjectTeamMember)
    );
    if (!new_team) return false;
    project->team = new_team;

    project->team[project->team_count].agent_name = strdup(agent_name);
    project->team[project->team_count].role = role ? strdup(role) : NULL;
    project->team_count++;

    project_touch(project);
    return project_save(project);
}

bool project_team_remove(ConvergioProject* project, const char* agent_name) {
    if (!project || !agent_name) return false;

    for (size_t i = 0; i < project->team_count; i++) {
        if (strcasecmp(project->team[i].agent_name, agent_name) == 0) {
            free(project->team[i].agent_name);
            free(project->team[i].role);

            // Shift remaining
            for (size_t j = i; j < project->team_count - 1; j++) {
                project->team[j] = project->team[j + 1];
            }
            project->team_count--;

            project_touch(project);
            return project_save(project);
        }
    }
    return false;
}

bool project_team_set_role(ConvergioProject* project, const char* agent_name, const char* role) {
    if (!project || !agent_name) return false;

    for (size_t i = 0; i < project->team_count; i++) {
        if (strcasecmp(project->team[i].agent_name, agent_name) == 0) {
            free(project->team[i].role);
            project->team[i].role = role ? strdup(role) : NULL;

            project_touch(project);
            return project_save(project);
        }
    }
    return false;
}

// ============================================================================
// CONTEXT MANAGEMENT
// ============================================================================

bool project_update_context(ConvergioProject* project, const char* summary,
                            const char* current_focus) {
    if (!project) return false;

    if (summary) {
        free(project->context_summary);
        project->context_summary = strdup(summary);
    }
    if (current_focus) {
        free(project->current_focus);
        project->current_focus = strdup(current_focus);
    }

    project_touch(project);
    return project_save_context(project);
}

bool project_add_decision(ConvergioProject* project, const char* decision) {
    if (!project || !decision || project->decision_count >= MAX_DECISIONS) return false;

    char** new_decisions = realloc(
        project->key_decisions,
        (project->decision_count + 1) * sizeof(char*)
    );
    if (!new_decisions) return false;
    project->key_decisions = new_decisions;

    project->key_decisions[project->decision_count++] = strdup(decision);

    project_touch(project);
    return project_save_context(project);
}

bool project_load_context(ConvergioProject* project) {
    if (!project || !project->storage_path) return false;

    char context_file[1024];
    snprintf(context_file, sizeof(context_file), "%s/%s", project->storage_path, CONTEXT_FILE);

    char* json = read_file(context_file);
    if (!json) return false;

    project->context_summary = json_get_string(json, "summary");
    project->current_focus = json_get_string(json, "current_focus");

    // Parse key_decisions array (simplified)
    const char* decisions = strstr(json, "\"key_decisions\"");
    if (decisions) {
        const char* arr_start = strchr(decisions, '[');
        const char* arr_end = strchr(decisions, ']');
        if (arr_start && arr_end) {
            // Parse each string in array
            const char* p = arr_start + 1;
            while (p < arr_end) {
                const char* quote_start = strchr(p, '"');
                if (!quote_start || quote_start >= arr_end) break;
                quote_start++;

                const char* quote_end = quote_start;
                while (*quote_end && *quote_end != '"') {
                    if (*quote_end == '\\') quote_end++;
                    quote_end++;
                }

                size_t len = (size_t)(quote_end - quote_start);
                if (len > 0 && project->decision_count < MAX_DECISIONS) {
                    char** new_decisions = realloc(
                        project->key_decisions,
                        (project->decision_count + 1) * sizeof(char*)
                    );
                    if (new_decisions) {
                        project->key_decisions = new_decisions;
                        project->key_decisions[project->decision_count] = malloc(len + 1);
                        if (project->key_decisions[project->decision_count]) {
                            strncpy(project->key_decisions[project->decision_count], quote_start, len);
                            project->key_decisions[project->decision_count][len] = '\0';
                            project->decision_count++;
                        }
                    }
                }
                p = quote_end + 1;
            }
        }
    }

    free(json);
    return true;
}

bool project_save_context(ConvergioProject* project) {
    if (!project || !project->storage_path) return false;

    // Build JSON
    size_t json_size = 4096 + (project->decision_count * 256);
    char* json = malloc(json_size);
    if (!json) return false;

    char* p = json;
    char* escaped;

    p += snprintf(p, json_size - (size_t)(p - json), "{\n");

    escaped = json_escape_str(project->context_summary);
    p += snprintf(p, json_size - (size_t)(p - json), "  \"summary\": \"%s\",\n", escaped ? escaped : "");
    free(escaped);

    escaped = json_escape_str(project->current_focus);
    p += snprintf(p, json_size - (size_t)(p - json), "  \"current_focus\": \"%s\",\n", escaped ? escaped : "");
    free(escaped);

    p += snprintf(p, json_size - (size_t)(p - json), "  \"key_decisions\": [\n");
    for (size_t i = 0; i < project->decision_count; i++) {
        escaped = json_escape_str(project->key_decisions[i]);
        p += snprintf(p, json_size - (size_t)(p - json), "    \"%s\"%s\n",
                      escaped ? escaped : "", i < project->decision_count - 1 ? "," : "");
        free(escaped);
    }
    p += snprintf(p, json_size - (size_t)(p - json), "  ]\n}\n");

    char context_file[1024];
    snprintf(context_file, sizeof(context_file), "%s/%s", project->storage_path, CONTEXT_FILE);
    bool success = write_file(context_file, json);
    free(json);

    return success;
}

// ============================================================================
// HISTORY MANAGEMENT
// ============================================================================

bool project_append_history(ConvergioProject* project, const char* role,
                            const char* content, const char* agent_name) {
    if (!project || !project->storage_path || !role || !content) return false;

    char history_file[1024];
    snprintf(history_file, sizeof(history_file), "%s/%s", project->storage_path, HISTORY_FILE);

    FILE* f = fopen(history_file, "a");
    if (!f) return false;

    char* escaped_content = json_escape_str(content);
    char* escaped_agent = agent_name ? json_escape_str(agent_name) : NULL;

    fprintf(f, "{\"role\":\"%s\",\"content\":\"%s\"",
            role, escaped_content ? escaped_content : "");
    if (escaped_agent) {
        fprintf(f, ",\"agent\":\"%s\"", escaped_agent);
    }
    fprintf(f, ",\"timestamp\":%ld}\n", (long)time(NULL));

    free(escaped_content);
    free(escaped_agent);
    fclose(f);

    return true;
}

size_t project_load_history(ConvergioProject* project, size_t max_turns,
                            char*** roles, char*** contents, char*** agents) {
    if (!project || !project->storage_path) return 0;

    char history_file[1024];
    snprintf(history_file, sizeof(history_file), "%s/%s", project->storage_path, HISTORY_FILE);

    FILE* f = fopen(history_file, "r");
    if (!f) return 0;

    // Count lines first
    size_t line_count = 0;
    char line[MAX_HISTORY_LINE];
    while (fgets(line, sizeof(line), f)) line_count++;

    // Calculate starting line
    size_t start_line = line_count > max_turns ? line_count - max_turns : 0;
    size_t read_count = line_count > max_turns ? max_turns : line_count;

    // Allocate arrays
    *roles = calloc(read_count + 1, sizeof(char*));
    *contents = calloc(read_count + 1, sizeof(char*));
    *agents = calloc(read_count + 1, sizeof(char*));

    if (!*roles || !*contents || !*agents) {
        free(*roles); free(*contents); free(*agents);
        fclose(f);
        return 0;
    }

    // Read from start_line
    rewind(f);
    size_t current_line = 0;
    size_t read_idx = 0;

    while (fgets(line, sizeof(line), f) && read_idx < read_count) {
        if (current_line >= start_line) {
            (*roles)[read_idx] = json_get_string(line, "role");
            (*contents)[read_idx] = json_get_string(line, "content");
            (*agents)[read_idx] = json_get_string(line, "agent");
            read_idx++;
        }
        current_line++;
    }

    fclose(f);
    return read_idx;
}

bool project_clear_history(ConvergioProject* project) {
    if (!project || !project->storage_path) return false;

    char history_file[1024];
    snprintf(history_file, sizeof(history_file), "%s/%s", project->storage_path, HISTORY_FILE);

    // Truncate file
    FILE* f = fopen(history_file, "w");
    if (f) fclose(f);
    return true;
}

// ============================================================================
// TEMPLATES
// ============================================================================

const ProjectTemplate* project_get_templates(size_t* count) {
    if (count) *count = g_template_count;
    return g_templates;
}

const ProjectTemplate* project_get_template(const char* name) {
    if (!name) return NULL;

    for (size_t i = 0; i < g_template_count; i++) {
        if (strcasecmp(g_templates[i].name, name) == 0) {
            return &g_templates[i];
        }
    }
    return NULL;
}

// ============================================================================
// UTILITIES
// ============================================================================

void project_free(ConvergioProject* project) {
    if (!project) return;

    free(project->name);
    free(project->slug);
    free(project->purpose);
    free(project->template_name);
    free(project->storage_path);
    free(project->context_summary);
    free(project->current_focus);

    for (size_t i = 0; i < project->team_count; i++) {
        free(project->team[i].agent_name);
        free(project->team[i].role);
    }
    free(project->team);

    for (size_t i = 0; i < project->decision_count; i++) {
        free(project->key_decisions[i]);
    }
    free(project->key_decisions);

    free(project);
}

void project_touch(ConvergioProject* project) {
    if (project) {
        project->last_active = time(NULL);
    }
}
