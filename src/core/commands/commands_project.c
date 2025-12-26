/**
 * CONVERGIO KERNEL - Project Commands
 *
 * Project management and setup wizard integration
 */

#include "commands_internal.h"

// Helper function to concatenate command arguments into a buffer
static size_t concat_args(char** argv, int argc, int start_idx, char* buffer, size_t buffer_size) {
    size_t len = 0;
    for (int i = start_idx; i < argc && len < buffer_size - 2; i++) {
        if (i > start_idx)
            buffer[len++] = ' ';
        size_t arg_len = strlen(argv[i]);
        if (len + arg_len < buffer_size - 1) {
            memcpy(buffer + len, argv[i], arg_len);
            len += arg_len;
        }
    }
    buffer[len] = '\0';
    return len;
}

// PROJECT COMMAND
// ============================================================================

int cmd_project(int argc, char** argv) {
    // Initialize projects if not already done
    static bool projects_initialized = false;
    if (!projects_initialized) {
        projects_init();
        projects_initialized = true;
    }

    if (argc < 2) {
        // Show current project status
        ConvergioProject* current = project_current();
        if (current) {
            printf("\n\033[1mCurrent Project: %s\033[0m\n", current->name);
            printf("  Purpose: %s\n", current->purpose ? current->purpose : "(none)");
            printf("  Team: ");
            for (size_t i = 0; i < current->team_count; i++) {
                printf("%s%s", current->team[i].agent_name,
                       i < current->team_count - 1 ? ", " : "");
            }
            printf("\n");
            if (current->current_focus) {
                printf("  Focus: %s\n", current->current_focus);
            }
            printf("\n");
        } else {
            printf("\n\033[1mNo active project.\033[0m\n\n");
        }

        printf("\033[36mUsage:\033[0m\n");
        printf("  project create <name> [--purpose \"...\"] [--team agent1,agent2] [--template "
               "name]\n");
        printf("  project list                    List all projects\n");
        printf("  project use <name>              Switch to a project\n");
        printf("  project status                  Show current project details\n");
        printf("  project team add <agent>        Add agent to current project\n");
        printf("  project team remove <agent>     Remove agent from project\n");
        printf("  project templates               List available templates\n");
        printf("  project archive <name>          Archive a project\n");
        printf("  project clear                   Clear current project\n");
        printf("\n");
        return 0;
    }

    const char* subcommand = argv[1];

    // project create <name> [options]
    if (strcmp(subcommand, "create") == 0) {
        if (argc < 3) {
            printf("Usage: project create <name> [--purpose \"...\"] [--team agent1,agent2] "
                   "[--template name]\n");
            return -1;
        }

        const char* name = argv[2];
        const char* purpose = NULL;
        const char* team = NULL;
        const char* template_name = NULL;

        // Parse options
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--purpose") == 0 && i + 1 < argc) {
                purpose = argv[++i];
            } else if (strcmp(argv[i], "--team") == 0 && i + 1 < argc) {
                team = argv[++i];
            } else if (strcmp(argv[i], "--template") == 0 && i + 1 < argc) {
                template_name = argv[++i];
            }
        }

        ConvergioProject* proj = project_create(name, purpose, team, template_name);
        if (!proj) {
            printf("\033[31mError: Failed to create project.\033[0m\n");
            return -1;
        }

        printf("\033[32m✓ Created project: %s\033[0m\n", proj->name);
        printf("  Slug: %s\n", proj->slug);
        printf("  Team: ");
        for (size_t i = 0; i < proj->team_count; i++) {
            printf("%s%s", proj->team[i].agent_name, i < proj->team_count - 1 ? ", " : "");
        }
        printf("\n");

        // Auto-activate new project
        project_use(proj->slug);
        printf("\n\033[36mProject activated. Only team agents will respond.\033[0m\n\n");
        return 0;
    }

    // project list
    if (strcmp(subcommand, "list") == 0) {
        size_t count = 0;
        ConvergioProject** projects = project_list_all(&count);

        printf("\n\033[1mProjects\033[0m (%zu)\n", count);
        printf("════════════════════════════════════════════\n");

        if (count == 0) {
            printf("  No projects yet. Create one with: project create <name>\n");
        } else {
            ConvergioProject* current = project_current();
            for (size_t i = 0; i < count; i++) {
                ConvergioProject* p = projects[i];
                bool is_current = current && strcmp(current->slug, p->slug) == 0;

                printf("  %s \033[1m%-20s\033[0m ", is_current ? "\033[32m●\033[0m" : " ", p->name);

                // Show team members
                printf("\033[36m");
                for (size_t j = 0; j < p->team_count; j++) {
                    printf("%s%s", p->team[j].agent_name, j < p->team_count - 1 ? ", " : "");
                }
                printf("\033[0m");

                if (p->template_name) {
                    printf(" \033[2m[%s]\033[0m", p->template_name);
                }
                printf("\n");
            }
        }
        printf("\n");
        return 0;
    }

    // project use <name>
    if (strcmp(subcommand, "use") == 0) {
        if (argc < 3) {
            printf("Usage: project use <name>\n");
            return -1;
        }

        if (project_use(argv[2])) {
            ConvergioProject* proj = project_current();
            printf("\033[32m✓ Switched to project: %s\033[0m\n", proj->name);
            printf("  Team: ");
            for (size_t i = 0; i < proj->team_count; i++) {
                printf("%s%s", proj->team[i].agent_name, i < proj->team_count - 1 ? ", " : "");
            }
            printf("\n");
        } else {
            printf("\033[31mError: Project not found: %s\033[0m\n", argv[2]);
            return -1;
        }
        return 0;
    }

    // project status
    if (strcmp(subcommand, "status") == 0) {
        ConvergioProject* proj = project_current();
        if (!proj) {
            printf("\033[33mNo active project.\033[0m\n");
            printf("Use 'project use <name>' or 'project create <name>' to start.\n");
            return 0;
        }

        printf("\n╭─ \033[1;36m%s\033[0m ", proj->name);
        int header_len = 5 + (int)strlen(proj->name);
        for (int i = header_len; i < 54; i++)
            printf("─");
        printf("╮\n");

        if (proj->purpose) {
            printf("│  Purpose: %-43s│\n", proj->purpose);
        }

        if (proj->template_name) {
            printf("│  Template: %-42s│\n", proj->template_name);
        }

        printf("├──────────────────────────────────────────────────────┤\n");
        printf("│  \033[1mTeam\033[0m                                                 │\n");
        for (size_t i = 0; i < proj->team_count; i++) {
            if (proj->team[i].role) {
                printf("│    • %-15s (%s)%-*s│\n", proj->team[i].agent_name, proj->team[i].role,
                       (int)(30 - strlen(proj->team[i].role)), "");
            } else {
                printf("│    • %-47s│\n", proj->team[i].agent_name);
            }
        }

        if (proj->context_summary || proj->current_focus) {
            printf("├──────────────────────────────────────────────────────┤\n");
            if (proj->context_summary) {
                printf("│  Summary: %-43s│\n", proj->context_summary);
            }
            if (proj->current_focus) {
                printf("│  Focus: %-45s│\n", proj->current_focus);
            }
        }

        if (proj->decision_count > 0) {
            printf("├──────────────────────────────────────────────────────┤\n");
            printf("│  \033[1mKey Decisions\033[0m                                      │\n");
            for (size_t i = 0; i < proj->decision_count && i < 5; i++) {
                printf("│    • %-47s│\n", proj->key_decisions[i]);
            }
            if (proj->decision_count > 5) {
                printf("│    ... and %zu more                                   │\n",
                       proj->decision_count - 5);
            }
        }

        printf("╰──────────────────────────────────────────────────────╯\n\n");
        return 0;
    }

    // project team <add|remove> <agent>
    if (strcmp(subcommand, "team") == 0) {
        ConvergioProject* proj = project_current();
        if (!proj) {
            printf("\033[31mError: No active project. Use 'project use <name>' first.\033[0m\n");
            return -1;
        }

        if (argc < 4) {
            printf("Usage: project team <add|remove> <agent_name>\n");
            return -1;
        }

        if (strcmp(argv[2], "add") == 0) {
            if (project_team_add(proj, argv[3], NULL)) {
                printf("\033[32m✓ Added %s to team.\033[0m\n", argv[3]);
            } else {
                printf("\033[31mError: Failed to add agent (may already be in team).\033[0m\n");
                return -1;
            }
        } else if (strcmp(argv[2], "remove") == 0) {
            if (project_team_remove(proj, argv[3])) {
                printf("\033[32m✓ Removed %s from team.\033[0m\n", argv[3]);
            } else {
                printf("\033[31mError: Agent not found in team.\033[0m\n");
                return -1;
            }
        } else {
            printf("Unknown team command: %s\n", argv[2]);
            printf("Use: project team add <agent> or project team remove <agent>\n");
            return -1;
        }
        return 0;
    }

    // project templates
    if (strcmp(subcommand, "templates") == 0) {
        size_t count = 0;
        const ProjectTemplate* templates = project_get_templates(&count);

        printf("\n\033[1mProject Templates\033[0m\n");
        printf("════════════════════════════════════════════\n");

        for (size_t i = 0; i < count; i++) {
            printf("\n  \033[36m%s\033[0m - %s\n", templates[i].name, templates[i].description);
            printf("    Default team: ");
            for (size_t j = 0; j < templates[i].team_count && templates[i].default_team[j]; j++) {
                printf("%s%s", templates[i].default_team[j],
                       j < templates[i].team_count - 1 && templates[i].default_team[j + 1] ? ", "
                                                                                           : "");
            }
            printf("\n");
        }

        printf("\n\033[2mUsage: project create <name> --template <template_name>\033[0m\n\n");
        return 0;
    }

    // project archive <name>
    if (strcmp(subcommand, "archive") == 0) {
        if (argc < 3) {
            printf("Usage: project archive <name>\n");
            return -1;
        }

        if (project_archive(argv[2])) {
            printf("\033[32m✓ Project archived: %s\033[0m\n", argv[2]);
        } else {
            printf("\033[31mError: Failed to archive project.\033[0m\n");
            return -1;
        }
        return 0;
    }

    // project clear
    if (strcmp(subcommand, "clear") == 0) {
        project_clear_current();
        printf("\033[32m✓ Cleared current project. All agents now available.\033[0m\n");
        return 0;
    }

    // project focus <text>
    if (strcmp(subcommand, "focus") == 0) {
        ConvergioProject* proj = project_current();
        if (!proj) {
            printf("\033[31mError: No active project.\033[0m\n");
            return -1;
        }

        if (argc < 3) {
            if (proj->current_focus) {
                printf("Current focus: %s\n", proj->current_focus);
            } else {
                printf("No current focus set.\n");
            }
            printf("Usage: project focus <description>\n");
            return 0;
        }

        // Concatenate remaining args
        char focus[512] = {0};
        concat_args(argv, argc, 2, focus, sizeof(focus));

        project_update_context(proj, NULL, focus);
        printf("\033[32m✓ Focus updated: %s\033[0m\n", focus);
        return 0;
    }

    // project decision <text>
    if (strcmp(subcommand, "decision") == 0) {
        ConvergioProject* proj = project_current();
        if (!proj) {
            printf("\033[31mError: No active project.\033[0m\n");
            return -1;
        }

        if (argc < 3) {
            printf("Usage: project decision <decision_text>\n");
            return -1;
        }

        // Concatenate remaining args
        char decision[512] = {0};
        concat_args(argv, argc, 2, decision, sizeof(decision));

        project_add_decision(proj, decision);
        printf("\033[32m✓ Decision recorded: %s\033[0m\n", decision);
        return 0;
    }

    printf("Unknown project command: %s\n", subcommand);
    printf("Run 'project' without arguments for usage information.\n");
    return -1;
}

// ============================================================================
