/**
 * CONVERGIO KERNEL - Plan Commands
 *
 * Execution plan and output management commands
 */

#include "commands_internal.h"

// PLAN COMMAND
// ============================================================================

/**
 * /plan - Execution plan management
 *
 * Subcommands:
 *   list             List all plans
 *   status <id>      Show plan status and progress
 *   export <id>      Export plan to markdown
 *   delete <id>      Delete a plan
 *   cleanup [days]   Clean up old plans (default: 30 days)
 */
int cmd_plan(int argc, char** argv) {
    if (!plan_db_is_ready()) {
        printf("\033[31m✗ Plan database not initialized.\033[0m\n");
        return -1;
    }

    if (argc < 2) {
        printf("\n\033[1m📋 Execution Plan Manager\033[0m\n\n");
        printf("Usage: plan <subcommand> [args]\n\n");
        printf("Subcommands:\n");
        printf("  list              List all plans\n");
        printf("  status <id>       Show plan status and progress\n");
        printf("  export <id>       Export plan to markdown file\n");
        printf("  delete <id>       Delete a plan\n");
        printf("  cleanup [days]    Clean up old plans (default: 30)\n");
        printf("\n");
        return 0;
    }

    const char* subcmd = argv[1];

    // --- plan list ---
    if (strcmp(subcmd, "list") == 0) {
        printf("\n\033[1m📋 Execution Plans\033[0m\n");
        printf("──────────────────────────────────────────────────────────\n");

        PlanRecord plans[50];
        int count = 0;
        PlanDbError err = plan_db_list_plans(-1, 50, 0, plans, 50, &count);

        if (err != PLAN_DB_OK || count == 0) {
            printf("  \033[90mNo plans found.\033[0m\n\n");
            return 0;
        }

        for (int i = 0; i < count; i++) {
            PlanRecord* p = &plans[i];
            const char* status_icon = "⏳";
            const char* status_color = "\033[33m";

            switch (p->status) {
            case PLAN_STATUS_ACTIVE:
                status_icon = "🔄";
                status_color = "\033[36m";
                break;
            case PLAN_STATUS_COMPLETED:
                status_icon = "✅";
                status_color = "\033[32m";
                break;
            case PLAN_STATUS_FAILED:
                status_icon = "❌";
                status_color = "\033[31m";
                break;
            case PLAN_STATUS_CANCELLED:
                status_icon = "⛔";
                status_color = "\033[90m";
                break;
            default:
                break;
            }

            // Get progress
            PlanProgress progress = {0};
            plan_db_get_progress(p->id, &progress);

            printf("  %s %s%s\033[0m\n", status_icon, status_color, p->description);
            printf("     ID: \033[90m%.8s...\033[0m  Tasks: %d/%d (%.0f%%)\n", p->id,
                   progress.completed, progress.total, progress.percent_complete);

            // Free strings allocated by plan_db
            plan_record_free(p);
        }

        printf("\n  Total: %d plan(s)\n\n", count);
        return 0;
    }

    // --- plan status ---
    if (strcmp(subcmd, "status") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: plan status <plan_id>\033[0m\n");
            return -1;
        }

        const char* plan_id = argv[2];
        PlanRecord plan;
        PlanDbError err = plan_db_get_plan(plan_id, &plan);

        if (err != PLAN_DB_OK) {
            printf("\033[31m✗ Plan not found: %s\033[0m\n", plan_id);
            return -1;
        }

        PlanProgress progress;
        plan_db_get_progress(plan_id, &progress);

        printf("\n\033[1m📋 Plan Status\033[0m\n");
        printf("──────────────────────────────────────────────────────────\n");
        printf("  Goal: %s\n", plan.description);
        printf("  ID: %s\n", plan.id);
        printf("  Progress: %d/%d tasks (%.1f%%)\n", progress.completed, progress.total,
               progress.percent_complete);

        // Show progress bar
        int bar_width = 30;
        int filled = (int)((double)progress.percent_complete / 100.0 * (double)bar_width);
        printf("  [");
        for (int i = 0; i < bar_width; i++) {
            if (i < filled)
                printf("\033[32m█\033[0m");
            else
                printf("\033[90m░\033[0m");
        }
        printf("]\n");

        // List tasks
        TaskRecord* tasks = NULL;
        plan_db_get_tasks(plan_id, -1, &tasks);

        if (tasks) {
            printf("\n  Tasks:\n");
            for (TaskRecord* t = tasks; t; t = t->next) {
                const char* icon = "○";
                const char* color = "\033[90m";

                switch (t->status) {
                case TASK_DB_STATUS_IN_PROGRESS:
                    icon = "●";
                    color = "\033[36m";
                    break;
                case TASK_DB_STATUS_COMPLETED:
                    icon = "✓";
                    color = "\033[32m";
                    break;
                case TASK_DB_STATUS_FAILED:
                    icon = "✗";
                    color = "\033[31m";
                    break;
                default:
                    break;
                }

                printf("    %s%s\033[0m %s", color, icon, t->description);
                if (t->assigned_agent) {
                    printf(" \033[35m@%s\033[0m", t->assigned_agent);
                }
                printf("\n");
            }
            task_record_free_list(tasks);
        }

        printf("\n");
        plan_record_free(&plan);
        return 0;
    }

    // --- plan export ---
    if (strcmp(subcmd, "export") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: plan export <plan_id>\033[0m\n");
            return -1;
        }

        const char* plan_id = argv[2];
        char filepath[PATH_MAX];
        snprintf(filepath, sizeof(filepath), "/tmp/plan-%s.md", plan_id);

        PlanDbError err = plan_db_export_markdown(plan_id, filepath, true);

        if (err == PLAN_DB_OK) {
            printf("\033[32m✓ Plan exported to: %s\033[0m\n", filepath);
        } else {
            printf("\033[31m✗ Export failed\033[0m\n");
            return -1;
        }
        return 0;
    }

    // --- plan delete ---
    if (strcmp(subcmd, "delete") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: plan delete <plan_id>\033[0m\n");
            return -1;
        }

        const char* plan_id = argv[2];
        PlanDbError err = plan_db_delete_plan(plan_id);

        if (err == PLAN_DB_OK) {
            printf("\033[32m✓ Plan deleted\033[0m\n");
        } else {
            printf("\033[31m✗ Delete failed (plan not found?)\033[0m\n");
            return -1;
        }
        return 0;
    }

    // --- plan cleanup ---
    if (strcmp(subcmd, "cleanup") == 0) {
        int days = 30;
        if (argc >= 3) {
            days = atoi(argv[2]);
            if (days <= 0)
                days = 30;
        }

        int deleted = plan_db_cleanup_old(days, -1);
        printf("\033[32m✓ Cleaned up %d old plan(s) (older than %d days)\033[0m\n", deleted, days);
        return 0;
    }

    printf("\033[31mUnknown plan command: %s\033[0m\n", subcmd);
    printf("Use '/plan' to see available commands.\n");
    return -1;
}

// ============================================================================
// OUTPUT COMMAND
// ============================================================================

/**
 * /output - Output service management
 *
 * Subcommands:
 *   list              List recent outputs
 *   open <file>       Open an output file
 *   delete <file>     Delete an output file
 *   size              Show total size of outputs
 *   cleanup [days]    Clean up old outputs (default: 30 days)
 */
int cmd_output(int argc, char** argv) {
    if (!output_service_is_ready()) {
        printf("\033[31m✗ Output service not initialized.\033[0m\n");
        return -1;
    }

    if (argc < 2) {
        printf("\n\033[1m📄 Output Service Manager\033[0m\n\n");
        printf("Usage: output <subcommand> [args]\n\n");
        printf("Subcommands:\n");
        printf("  list              List recent outputs\n");
        printf("  latest            Show latest output\n");
        printf("  open <path>       Open an output file\n");
        printf("  delete <path>     Delete an output file\n");
        printf("  size              Show total size of outputs\n");
        printf("  cleanup [days]    Clean up old outputs (default: 30)\n");
        printf("\nOutputs are stored in ~/.convergio/outputs/\n\n");
        return 0;
    }

    const char* subcmd = argv[1];

    // --- output list ---
    if (strcmp(subcmd, "list") == 0) {
        printf("\n\033[1m📄 Recent Outputs\033[0m\n");
        printf("──────────────────────────────────────────────────────────\n");

        char* paths[20];
        int count = 0;
        OutputError err = output_list_recent(20, paths, &count);

        if (err != OUTPUT_OK || count == 0) {
            printf("  \033[90mNo outputs found.\033[0m\n\n");
            return 0;
        }

        for (int i = 0; i < count; i++) {
            // Extract filename from path
            const char* filename = strrchr(paths[i], '/');
            filename = filename ? filename + 1 : paths[i];

            // Determine icon based on format
            const char* icon = "📄";
            if (strstr(filename, ".md"))
                icon = "📝";
            else if (strstr(filename, ".json"))
                icon = "📊";
            else if (strstr(filename, ".html"))
                icon = "🌐";

            printf("  %s %s\n", icon, filename);
            printf("     \033[90m%s\033[0m\n", paths[i]);

            free(paths[i]);
        }

        printf("\n  Total: %d file(s)\n\n", count);
        return 0;
    }

    // --- output latest ---
    if (strcmp(subcmd, "latest") == 0) {
        OutputResult result;
        OutputError err = output_get_latest(&result);

        if (err != OUTPUT_OK) {
            printf("\033[31m✗ No recent outputs found.\033[0m\n");
            return -1;
        }

        printf("\n\033[1m📄 Latest Output\033[0m\n");
        printf("──────────────────────────────────────────────────────────\n");
        printf("  Path: %s\n", result.filepath);
        output_print_link(result.filepath, "Open in default app");
        printf("\n");
        return 0;
    }

    // --- output open ---
    if (strcmp(subcmd, "open") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: output open <filepath>\033[0m\n");
            printf("Use 'output list' to see available files.\n");
            return -1;
        }

        const char* filepath = argv[2];

        // Open with system default
        char cmd[PATH_MAX + 16];
        snprintf(cmd, sizeof(cmd), "open \"%s\"", filepath);
        int ret = system(cmd);

        if (ret == 0) {
            printf("\033[32m✓ Opened: %s\033[0m\n", filepath);
        } else {
            printf("\033[31m✗ Failed to open file\033[0m\n");
            return -1;
        }
        return 0;
    }

    // --- output delete ---
    if (strcmp(subcmd, "delete") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: output delete <filepath>\033[0m\n");
            return -1;
        }

        const char* filepath = argv[2];
        OutputError err = output_delete(filepath);

        if (err == OUTPUT_OK) {
            printf("\033[32m✓ Deleted: %s\033[0m\n", filepath);
        } else {
            printf("\033[31m✗ Delete failed (file not found?)\033[0m\n");
            return -1;
        }
        return 0;
    }

    // --- output size ---
    if (strcmp(subcmd, "size") == 0) {
        size_t total_size = output_get_total_size();
        double size_mb = (double)total_size / (1024.0 * 1024.0);

        printf("\n\033[1m📊 Output Storage\033[0m\n");
        printf("──────────────────────────────────────────────────────────\n");
        printf("  Total size: %.2f MB\n", size_mb);
        printf("  Location: ~/.convergio/outputs/\n\n");
        return 0;
    }

    // --- output cleanup ---
    if (strcmp(subcmd, "cleanup") == 0) {
        int days = 30;
        if (argc >= 3) {
            days = atoi(argv[2]);
            if (days <= 0)
                days = 30;
        }

        int deleted = output_cleanup(days);
        printf("\033[32m✓ Cleaned up %d file(s) older than %d days\033[0m\n", deleted, days);
        return 0;
    }

    printf("\033[31mUnknown output command: %s\033[0m\n", subcmd);
    printf("Use '/output' to see available commands.\n");
    return -1;
}
