/**
 * CONVERGIO KERNEL - Todo Commands
 *
 * Todo manager, daemon, and MCP commands (Anna Executive Assistant)
 */

#include "commands_internal.h"

// TODO MANAGER COMMANDS (Anna Executive Assistant)
// ============================================================================

/**
 * Helper: Print a task with formatting
 */
static void print_task_item(const TodoTask* task) {
    // Status indicator
    const char* status_icon;
    const char* status_color;
    switch (task->status) {
    case TODO_STATUS_IN_PROGRESS:
        status_icon = "[>]";
        status_color = "\033[33m"; // Yellow
        break;
    case TODO_STATUS_COMPLETED:
        status_icon = "[x]";
        status_color = "\033[32m"; // Green
        break;
    case TODO_STATUS_CANCELLED:
        status_icon = "[-]";
        status_color = "\033[90m"; // Gray
        break;
    default:
        status_icon = "[ ]";
        status_color = "\033[0m";
    }

    // Priority indicator
    const char* priority = "";
    if (task->priority == 1)
        priority = " \033[31m!!\033[0m";
    else if (task->priority == 3)
        priority = " \033[90m~\033[0m";

    // Print main line
    printf("  %s%s\033[0m %lld. %s%s", status_color, status_icon, (long long)task->id, task->title,
           priority);

    // Context tag
    if (task->context && task->context[0]) {
        printf(" \033[35m@%s\033[0m", task->context);
    }

    // Due date
    if (task->due_date > 0) {
        char date_buf[64];
        todo_format_date(task->due_date, date_buf, sizeof(date_buf), true);
        time_t now = time(NULL);
        if (task->due_date < now && task->status == TODO_STATUS_PENDING) {
            printf(" \033[31m(overdue: %s)\033[0m", date_buf);
        } else {
            printf(" \033[90m(due: %s)\033[0m", date_buf);
        }
    }

    printf("\n");

    // Description if present
    if (task->description && task->description[0]) {
        printf("      \033[90m%s\033[0m\n", task->description);
    }
}

/**
 * /todo - Task manager command
 */
int cmd_todo(int argc, char** argv) {
    if (argc < 2) {
        // Show usage
        printf("\n\033[1m📋 Todo Manager\033[0m (Anna Executive Assistant)\n\n");
        printf("Usage: todo <subcommand> [args]\n\n");
        printf("Subcommands:\n");
        printf("  add <title> [options]  Add a new task\n");
        printf("  list [filter]          List tasks\n");
        printf("  done <id>              Mark task completed\n");
        printf("  start <id>             Mark task in progress\n");
        printf("  delete <id>            Delete a task\n");
        printf("  inbox [text]           Quick capture / list inbox\n");
        printf("  search <query>         Search tasks\n");
        printf("  stats                  Show statistics\n");
        printf("\nRun 'help todo' for detailed options.\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    // --- todo add ---
    if (strcmp(subcommand, "add") == 0) {
        if (argc < 3) {
            printf("Usage: todo add <title> [--due <date>] [--remind <time>] [--priority <1-3>] "
                   "[--context <ctx>]\n");
            return -1;
        }

        char title[256] = {0};
        char* due_str = NULL;
        char* remind_str = NULL;
        int priority = 2;
        char* context = NULL;

        // Parse arguments
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "--due") == 0 && i + 1 < argc) {
                due_str = argv[++i];
            } else if (strcmp(argv[i], "--remind") == 0 && i + 1 < argc) {
                remind_str = argv[++i];
            } else if (strcmp(argv[i], "--priority") == 0 && i + 1 < argc) {
                priority = atoi(argv[++i]);
            } else if (strcmp(argv[i], "--context") == 0 && i + 1 < argc) {
                context = argv[++i];
            } else {
                // Part of title
                if (title[0])
                    strncat(title, " ", sizeof(title) - strlen(title) - 1);
                strncat(title, argv[i], sizeof(title) - strlen(title) - 1);
            }
            i++;
        }

        if (!title[0]) {
            printf("Error: Task title required.\n");
            return -1;
        }

        time_t due = 0;
        if (due_str) {
            due = todo_parse_date(due_str, time(NULL));
        }

        int remind = 0;
        if (remind_str) {
            remind = (int)todo_parse_duration(remind_str);
        }

        TodoCreateOptions opts = {
            .title = title,
            .description = NULL,
            .priority = (TodoPriority)priority,
            .due_date = due,
            .reminder_at = remind > 0 ? (due > 0 ? due - remind : time(NULL) + remind) : 0,
            .recurrence = TODO_RECURRENCE_NONE,
            .recurrence_rule = NULL,
            .tags = NULL,
            .context = context,
            .parent_id = 0,
            .source = TODO_SOURCE_USER,
            .external_id = NULL};
        int64_t id = todo_create(&opts);
        if (id > 0) {
            printf("\033[32m✓ Task added:\033[0m %s (ID: %lld)\n", title, (long long)id);
            if (due > 0) {
                char buf[64];
                todo_format_date(due, buf, sizeof(buf), true);
                printf("  Due: %s\n", buf);
            }
        } else {
            printf("\033[31mError: Failed to add task.\033[0m\n");
            return -1;
        }
        return 0;
    }

    // --- todo list ---
    if (strcmp(subcommand, "list") == 0) {
        const char* list_filter = (argc > 2) ? argv[2] : NULL;

        int count = 0;
        TodoTask** tasks = NULL;

        if (list_filter && strcmp(list_filter, "today") == 0) {
            tasks = todo_list_today(&count);
            printf("\n\033[1m📋 Today's Tasks\033[0m\n\n");
        } else if (list_filter && strcmp(list_filter, "overdue") == 0) {
            tasks = todo_list_overdue(&count);
            printf("\n\033[1m📋 Overdue Tasks\033[0m\n\n");
        } else if (list_filter && strcmp(list_filter, "upcoming") == 0) {
            int days = (argc > 3) ? atoi(argv[3]) : 7;
            tasks = todo_list_upcoming(days, &count);
            printf("\n\033[1m📋 Upcoming Tasks (next %d days)\033[0m\n\n", days);
        } else if (list_filter && strcmp(list_filter, "all") == 0) {
            TodoFilter all_filter = {0};
            all_filter.include_completed = true;
            all_filter.include_cancelled = true;
            tasks = todo_list(&all_filter, &count);
            printf("\n\033[1m📋 All Tasks\033[0m\n\n");
        } else {
            tasks = todo_list(NULL, &count);
            printf("\n\033[1m📋 Pending Tasks\033[0m\n\n");
        }

        if (!tasks || count == 0) {
            printf("  \033[90mNo tasks found.\033[0m\n\n");
            return 0;
        }

        for (int i = 0; i < count; i++) {
            print_task_item(tasks[i]);
        }
        printf("\n");

        todo_free_tasks(tasks, count);
        return 0;
    }

    // --- todo done ---
    if (strcmp(subcommand, "done") == 0) {
        if (argc < 3) {
            printf("Usage: todo done <id>\n");
            return -1;
        }
        int64_t id = atoll(argv[2]);
        if (todo_complete(id) == 0) {
            printf("\033[32m✓ Task %lld completed!\033[0m\n", (long long)id);
        } else {
            printf("\033[31mError: Failed to complete task.\033[0m\n");
            return -1;
        }
        return 0;
    }

    // --- todo start ---
    if (strcmp(subcommand, "start") == 0) {
        if (argc < 3) {
            printf("Usage: todo start <id>\n");
            return -1;
        }
        int64_t id = atoll(argv[2]);
        if (todo_start(id) == 0) {
            printf("\033[33m→ Task %lld in progress\033[0m\n", (long long)id);
        } else {
            printf("\033[31mError: Failed to start task.\033[0m\n");
            return -1;
        }
        return 0;
    }

    // --- todo delete / rm ---
    if (strcmp(subcommand, "delete") == 0 || strcmp(subcommand, "rm") == 0) {
        if (argc < 3) {
            printf("Usage: todo delete <id>\n");
            return -1;
        }
        int64_t id = atoll(argv[2]);
        if (todo_delete(id) == 0) {
            printf("\033[32m✓ Task %lld deleted.\033[0m\n", (long long)id);
        } else {
            printf("\033[31mError: Failed to delete task.\033[0m\n");
            return -1;
        }
        return 0;
    }

    // --- todo inbox ---
    if (strcmp(subcommand, "inbox") == 0) {
        if (argc < 3) {
            // List inbox items
            int count = 0;
            TodoInboxItem** items = inbox_list_unprocessed(&count);
            printf("\n\033[1m📥 Inbox\033[0m\n\n");
            if (!items || count == 0) {
                printf("  \033[90mInbox is empty.\033[0m\n\n");
                return 0;
            }
            for (int i = 0; i < count; i++) {
                printf("  %lld. %s\n", (long long)items[i]->id, items[i]->content);
            }
            printf("\n");
            todo_free_inbox_items(items, count);
        } else {
            // Quick capture
            char content[512] = {0};
            for (int i = 2; i < argc; i++) {
                if (i > 2)
                    strncat(content, " ", sizeof(content) - strlen(content) - 1);
                strncat(content, argv[i], sizeof(content) - strlen(content) - 1);
            }
            int64_t id = inbox_capture(content, "cli");
            if (id > 0) {
                printf("\033[32m✓ Captured to inbox:\033[0m %s\n", content);
            } else {
                printf("\033[31mError: Failed to capture.\033[0m\n");
                return -1;
            }
        }
        return 0;
    }

    // --- todo search / find ---
    if (strcmp(subcommand, "search") == 0 || strcmp(subcommand, "find") == 0) {
        if (argc < 3) {
            printf("Usage: todo search <query>\n");
            return -1;
        }
        char query[256] = {0};
        for (int i = 2; i < argc; i++) {
            if (i > 2)
                strncat(query, " ", sizeof(query) - strlen(query) - 1);
            strncat(query, argv[i], sizeof(query) - strlen(query) - 1);
        }

        int count = 0;
        TodoTask** results = todo_search(query, &count);
        printf("\n\033[1m🔍 Search: \"%s\"\033[0m\n\n", query);
        if (!results || count == 0) {
            printf("  \033[90mNo matching tasks found.\033[0m\n\n");
            return 0;
        }
        for (int i = 0; i < count; i++) {
            print_task_item(results[i]);
        }
        printf("\n");
        todo_free_tasks(results, count);
        return 0;
    }

    // --- todo stats ---
    if (strcmp(subcommand, "stats") == 0) {
        TodoStats stats = todo_get_stats();
        printf("\n\033[1m📊 Todo Statistics\033[0m\n\n");
        printf("  Pending:       %d\n", stats.total_pending);
        printf("  In Progress:   %d\n", stats.total_in_progress);
        printf("  Completed:     today: %d, week: %d\n", stats.total_completed_today,
               stats.total_completed_week);
        printf("  Overdue:       %d\n", stats.total_overdue);
        printf("  Inbox items:   %d\n", stats.inbox_unprocessed);
        printf("\n");
        return 0;
    }

    printf("Unknown todo command: %s\n", subcommand);
    printf("Run 'todo' without arguments for usage.\n");
    return -1;
}

/**
 * Helper: Try to parse time from a word
 * Returns true if the word looks like a time specifier
 */
static bool try_parse_time(const char* word) {
    if (!word)
        return false;

    // Keywords that indicate time
    if (strstr(word, "tomorrow") || strstr(word, "domani"))
        return true;
    if (strstr(word, "tonight") || strstr(word, "stasera"))
        return true;
    if (strstr(word, "morning") || strstr(word, "mattina"))
        return true;
    if (strstr(word, "afternoon") || strstr(word, "pomeriggio"))
        return true;
    if (strstr(word, "evening") || strstr(word, "sera"))
        return true;
    if (strstr(word, "next") || strstr(word, "prossimo"))
        return true;
    if (strstr(word, "monday") || strstr(word, "lunedi"))
        return true;
    if (strstr(word, "tuesday") || strstr(word, "martedi"))
        return true;
    if (strstr(word, "wednesday") || strstr(word, "mercoledi"))
        return true;
    if (strstr(word, "thursday") || strstr(word, "giovedi"))
        return true;
    if (strstr(word, "friday") || strstr(word, "venerdi"))
        return true;
    if (strstr(word, "saturday") || strstr(word, "sabato"))
        return true;
    if (strstr(word, "sunday") || strstr(word, "domenica"))
        return true;
    if (strncmp(word, "in", 2) == 0 || strncmp(word, "tra", 3) == 0)
        return true;
    if (strncmp(word, "at", 2) == 0 || strncmp(word, "alle", 4) == 0)
        return true;

    // Month names
    const char* months[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct",
                            "nov", "dec", "gen", "mag", "giu", "lug", "ago", "set", "ott", "dic"};
    for (int i = 0; i < 20; i++) {
        if (strncasecmp(word, months[i], 3) == 0)
            return true;
    }

    // ISO date format
    if (strlen(word) >= 10 && word[4] == '-' && word[7] == '-')
        return true;

    return false;
}

/**
 * /remind - Quick reminder creation
 */
int cmd_remind(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: remind <message> <when> [--note <context>]\n");
        printf("       remind <when> <message> [--note <context>]\n");
        printf("\nExamples:\n");
        printf("  remind \"Call mom\" tomorrow morning\n");
        printf("  remind tonight \"Buy groceries\"\n");
        printf("  remind \"Meeting\" next tuesday at 10am --note \"Bring slides\"\n");
        return -1;
    }

    char message[256] = {0};
    char time_str[256] = {0};
    char note[512] = {0};

    // Parse arguments, separating message from time
    bool in_time = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--note") == 0 && i + 1 < argc) {
            // Collect all remaining args as note
            for (int j = i + 1; j < argc; j++) {
                if (note[0])
                    strncat(note, " ", sizeof(note) - strlen(note) - 1);
                strncat(note, argv[j], sizeof(note) - strlen(note) - 1);
            }
            break;
        }

        // Check if this looks like a time specifier
        bool is_time = try_parse_time(argv[i]);

        // If starts with quote, it's message
        if (argv[i][0] == '"' || argv[i][0] == '\'') {
            is_time = false;
        }

        if (is_time) {
            in_time = true;
            if (time_str[0])
                strncat(time_str, " ", sizeof(time_str) - strlen(time_str) - 1);
            strncat(time_str, argv[i], sizeof(time_str) - strlen(time_str) - 1);
        } else if (!in_time || !message[0]) {
            // Part of message
            if (message[0])
                strncat(message, " ", sizeof(message) - strlen(message) - 1);
            strncat(message, argv[i], sizeof(message) - strlen(message) - 1);
        } else {
            // Continuation of time
            if (time_str[0])
                strncat(time_str, " ", sizeof(time_str) - strlen(time_str) - 1);
            strncat(time_str, argv[i], sizeof(time_str) - strlen(time_str) - 1);
        }
    }

    // Strip quotes from message
    size_t msg_len = strlen(message);
    if (msg_len > 1 && (message[0] == '"' || message[0] == '\'')) {
        memmove(message, message + 1, msg_len);
        msg_len--;
        if (msg_len > 0 && (message[msg_len - 1] == '"' || message[msg_len - 1] == '\'')) {
            message[msg_len - 1] = '\0';
        }
    }

    if (!message[0]) {
        printf("\033[31mError: Reminder message required.\033[0m\n");
        return -1;
    }

    if (!time_str[0]) {
        printf("\033[31mError: When should I remind you?\033[0m\n");
        return -1;
    }

    // Parse the time
    time_t remind_time = todo_parse_date(time_str, time(NULL));
    if (remind_time <= 0) {
        printf("\033[31mError: Could not understand time: %s\033[0m\n", time_str);
        return -1;
    }

    // Create the reminder as a todo with due date
    TodoCreateOptions opts = {.title = message,
                              .description = note[0] ? note : NULL,
                              .priority = TODO_PRIORITY_NORMAL,
                              .due_date = remind_time,
                              .reminder_at = remind_time,
                              .recurrence = TODO_RECURRENCE_NONE,
                              .recurrence_rule = NULL,
                              .tags = NULL,
                              .context = "reminder",
                              .parent_id = 0,
                              .source = TODO_SOURCE_USER,
                              .external_id = NULL};
    int64_t id = todo_create(&opts);

    if (id > 0) {
        char date_buf[64];
        todo_format_date(remind_time, date_buf, sizeof(date_buf), true);
        printf("\033[32m✓ Reminder set:\033[0m %s\n", message);
        printf("  When: %s\n", date_buf);
        if (note[0]) {
            printf("  Note: %s\n", note);
        }
    } else {
        printf("\033[31mError: Failed to create reminder.\033[0m\n");
        return -1;
    }

    return 0;
}

/**
 * /reminders - View upcoming reminders
 */
int cmd_reminders(int argc, char** argv) {
    const char* rem_filter = (argc > 1) ? argv[1] : "today";

    int count = 0;
    TodoTask** tasks = NULL;

    if (strcmp(rem_filter, "week") == 0) {
        tasks = todo_list_upcoming(7, &count);
        printf("\n\033[1m⏰ Reminders (next 7 days)\033[0m\n\n");
    } else if (strcmp(rem_filter, "all") == 0) {
        // Get all tasks with context "reminder"
        TodoFilter filter = {0};
        filter.context = "reminder";
        tasks = todo_list(&filter, &count);
        printf("\n\033[1m⏰ All Reminders\033[0m\n\n");
    } else {
        tasks = todo_list_today(&count);
        printf("\n\033[1m⏰ Today's Reminders\033[0m\n\n");
    }

    // Filter to only show reminders
    int reminder_count = 0;
    if (tasks && count > 0) {
        for (int i = 0; i < count; i++) {
            if (tasks[i]->context && strcmp(tasks[i]->context, "reminder") == 0) {
                print_task_item(tasks[i]);
                reminder_count++;
            }
        }
    }

    if (reminder_count == 0) {
        printf("  \033[90mNo reminders scheduled.\033[0m\n");
    }

    printf("\n");
    if (tasks)
        todo_free_tasks(tasks, count);
    return 0;
}

// ============================================================================
// DAEMON COMMAND - Notification Daemon Management
// ============================================================================

int cmd_daemon(int argc, char** argv) {
    if (argc < 2) {
        printf("\n");
        printf("╔═══════════════════════════════════════════════════╗\n");
        printf("║           NOTIFICATION DAEMON                     ║\n");
        printf("╠═══════════════════════════════════════════════════╣\n");
        printf("║ Usage: /daemon <command>                          ║\n");
        printf("╠═══════════════════════════════════════════════════╣\n");
        printf("║ Commands:                                         ║\n");
        printf("║   start       Start the daemon                    ║\n");
        printf("║   stop        Stop the daemon                     ║\n");
        printf("║   restart     Restart the daemon                  ║\n");
        printf("║   status      Show daemon status                  ║\n");
        printf("║   health      Show detailed health info           ║\n");
        printf("║   install     Install LaunchAgent                 ║\n");
        printf("║   uninstall   Remove LaunchAgent                  ║\n");
        printf("║   test        Send a test notification            ║\n");
        printf("╚═══════════════════════════════════════════════════╝\n");
        printf("\n");
        return 0;
    }

    const char* subcmd = argv[1];

    if (strcmp(subcmd, "start") == 0) {
        printf("Starting notification daemon...\n");
        int result = notify_daemon_start();
        if (result == 0) {
            printf("\033[32m✓ Daemon started (PID %d)\033[0m\n", notify_daemon_get_pid());
        } else {
            printf("\033[31m✗ Failed to start daemon\033[0m\n");
        }
        return result;
    }

    if (strcmp(subcmd, "stop") == 0) {
        printf("Stopping notification daemon...\n");
        int result = notify_daemon_stop();
        if (result == 0) {
            printf("\033[32m✓ Daemon stopped\033[0m\n");
        } else {
            printf("\033[31m✗ Failed to stop daemon\033[0m\n");
        }
        return result;
    }

    if (strcmp(subcmd, "restart") == 0) {
        printf("Restarting notification daemon...\n");
        int result = notify_daemon_restart();
        if (result == 0) {
            printf("\033[32m✓ Daemon restarted (PID %d)\033[0m\n", notify_daemon_get_pid());
        } else {
            printf("\033[31m✗ Failed to restart daemon\033[0m\n");
        }
        return result;
    }

    if (strcmp(subcmd, "status") == 0) {
        bool running = notify_daemon_is_running();
        pid_t pid = notify_daemon_get_pid();

        printf("\n");
        printf("Daemon Status: %s%s\033[0m\n", running ? "\033[32m" : "\033[31m",
               running ? "RUNNING" : "STOPPED");

        if (running && pid > 0) {
            printf("Process ID:    %d\n", pid);
        }

        printf("Best Method:   %s\n", notify_method_to_string(notify_get_best_method()));
        printf("\n");
        return 0;
    }

    if (strcmp(subcmd, "health") == 0) {
        notify_print_health();
        return 0;
    }

    if (strcmp(subcmd, "install") == 0) {
        printf("Installing LaunchAgent...\n");
        int result = notify_daemon_install();
        if (result == 0) {
            printf("\033[32m✓ LaunchAgent installed\033[0m\n");
            printf("  The daemon will now start automatically at login.\n");
        } else {
            printf("\033[31m✗ Failed to install LaunchAgent\033[0m\n");
        }
        return result;
    }

    if (strcmp(subcmd, "uninstall") == 0) {
        printf("Uninstalling LaunchAgent...\n");
        int result = notify_daemon_uninstall();
        if (result == 0) {
            printf("\033[32m✓ LaunchAgent uninstalled\033[0m\n");
        } else {
            printf("\033[31m✗ Failed to uninstall LaunchAgent\033[0m\n");
        }
        return result;
    }

    if (strcmp(subcmd, "test") == 0) {
        printf("Sending test notification...\n");
        printf("(Click the notification to open Convergio)\n");
        // Use reminder group to test click-to-open functionality
        NotifyOptions opts = {.title = "Test Notification",
                              .subtitle = NULL,
                              .body = "Click here to open Convergio",
                              .sound = "Glass",
                              .group = "convergio-reminders", // This enables click-to-open
                              .action_url = NULL,
                              .timeout_ms = 0};
        NotifyResult result = notify_send(&opts);
        if (result == NOTIFY_SUCCESS) {
            printf("\033[32m✓ Test notification sent\033[0m\n");
        } else {
            printf("\033[31m✗ Failed to send notification (code %d)\033[0m\n", result);
        }
        return (result == NOTIFY_SUCCESS) ? 0 : -1;
    }

    printf("\033[31mUnknown daemon command: %s\033[0m\n", subcmd);
    printf("Use '/daemon' to see available commands.\n");
    return -1;
}

// ============================================================================
// MCP COMMAND - Model Context Protocol Client Management
// ============================================================================

int cmd_mcp(int argc, char** argv) {
    if (argc < 2) {
        printf("\n");
        printf("╔═══════════════════════════════════════════════════╗\n");
        printf("║              MCP CLIENT                           ║\n");
        printf("╠═══════════════════════════════════════════════════╣\n");
        printf("║ Usage: /mcp <command> [args]                      ║\n");
        printf("╠═══════════════════════════════════════════════════╣\n");
        printf("║ Commands:                                         ║\n");
        printf("║   list              List configured servers       ║\n");
        printf("║   status            Show connection status        ║\n");
        printf("║   health            Show detailed health info     ║\n");
        printf("║   connect <name>    Connect to a server           ║\n");
        printf("║   disconnect <name> Disconnect from a server      ║\n");
        printf("║   connect-all       Connect to all enabled        ║\n");
        printf("║   enable <name>     Enable a server               ║\n");
        printf("║   disable <name>    Disable a server              ║\n");
        printf("║   tools             List all available tools      ║\n");
        printf("║   tools <server>    List tools from server        ║\n");
        printf("║   call <tool> [json] Call a tool                  ║\n");
        printf("╚═══════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("Configuration file: ~/.convergio/mcp.json\n");
        printf("\n");
        return 0;
    }

    const char* subcmd = argv[1];

    if (strcmp(subcmd, "list") == 0) {
        int count;
        char** servers = mcp_list_servers(&count);

        printf("\n");
        printf("Configured MCP Servers:\n");
        printf("───────────────────────\n");

        if (count == 0) {
            printf("  \033[90mNo servers configured.\033[0m\n");
            printf("  Add servers in ~/.convergio/mcp.json\n");
        } else {
            for (int i = 0; i < count; i++) {
                MCPServerConfig* config = mcp_get_server_config(servers[i]);
                MCPConnectionStatus status = mcp_get_status(servers[i]);

                const char* status_icon;
                const char* status_color;

                switch (status) {
                case MCP_STATUS_CONNECTED:
                    status_icon = "●";
                    status_color = "\033[32m";
                    break;
                case MCP_STATUS_CONNECTING:
                    status_icon = "○";
                    status_color = "\033[33m";
                    break;
                case MCP_STATUS_ERROR:
                    status_icon = "✗";
                    status_color = "\033[31m";
                    break;
                default:
                    status_icon = "○";
                    status_color = "\033[90m";
                    break;
                }

                printf("  %s%s\033[0m %-20s %s%s\033[0m\n", status_color, status_icon, servers[i],
                       config && config->enabled ? "" : "\033[90m",
                       config && config->enabled ? "" : "(disabled)");

                free(servers[i]);
            }
            free(servers);
        }
        printf("\n");
        return 0;
    }

    if (strcmp(subcmd, "status") == 0) {
        int count;
        char** connected = mcp_list_connected(&count);

        printf("\n");
        printf("Connected MCP Servers: %d\n", count);
        printf("───────────────────────────\n");

        if (count == 0) {
            printf("  \033[90mNo servers connected.\033[0m\n");
            printf("  Use '/mcp connect <name>' to connect.\n");
        } else {
            for (int i = 0; i < count; i++) {
                MCPServer* server = mcp_get_server(connected[i]);
                if (server) {
                    printf("  \033[32m●\033[0m %-20s %d tools\n", connected[i], server->tool_count);
                }
                free(connected[i]);
            }
            free(connected);
        }
        printf("\n");
        return 0;
    }

    if (strcmp(subcmd, "health") == 0) {
        mcp_print_health();
        return 0;
    }

    if (strcmp(subcmd, "connect") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: /mcp connect <server_name>\033[0m\n");
            return -1;
        }

        const char* name = argv[2];
        printf("Connecting to %s...\n", name);

        int result = mcp_connect(name);
        if (result == MCP_OK) {
            MCPServer* server = mcp_get_server(name);
            printf("\033[32m✓ Connected to %s\033[0m\n", name);
            if (server) {
                printf("  Tools: %d\n", server->tool_count);
                printf("  Resources: %d\n", server->resource_count);
                printf("  Prompts: %d\n", server->prompt_count);
            }
        } else {
            const char* err = mcp_get_last_error(name);
            printf("\033[31m✗ Failed to connect: %s\033[0m\n", err ? err : "unknown error");
        }
        return result;
    }

    if (strcmp(subcmd, "disconnect") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: /mcp disconnect <server_name>\033[0m\n");
            return -1;
        }

        const char* name = argv[2];
        printf("Disconnecting from %s...\n", name);

        int result = mcp_disconnect(name);
        if (result == 0) {
            printf("\033[32m✓ Disconnected from %s\033[0m\n", name);
        } else {
            printf("\033[31m✗ Failed to disconnect\033[0m\n");
        }
        return result;
    }

    if (strcmp(subcmd, "connect-all") == 0) {
        printf("Connecting to all enabled servers...\n");
        int connected = mcp_connect_all();
        printf("\033[32m✓ Connected to %d servers\033[0m\n", connected);
        return 0;
    }

    if (strcmp(subcmd, "enable") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: /mcp enable <server_name>\033[0m\n");
            return -1;
        }

        int result = mcp_enable_server(argv[2]);
        if (result == 0) {
            printf("\033[32m✓ Server %s enabled\033[0m\n", argv[2]);
        } else {
            printf("\033[31m✗ Server not found: %s\033[0m\n", argv[2]);
        }
        return result;
    }

    if (strcmp(subcmd, "disable") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: /mcp disable <server_name>\033[0m\n");
            return -1;
        }

        int result = mcp_disable_server(argv[2]);
        if (result == 0) {
            printf("\033[32m✓ Server %s disabled\033[0m\n", argv[2]);
        } else {
            printf("\033[31m✗ Server not found: %s\033[0m\n", argv[2]);
        }
        return result;
    }

    if (strcmp(subcmd, "tools") == 0) {
        printf("\n");

        if (argc >= 3) {
            // List tools from specific server
            const char* server_name = argv[2];
            int count;
            MCPTool** tools = mcp_list_tools(server_name, &count);

            printf("Tools from %s (%d):\n", server_name, count);
            printf("───────────────────────────\n");

            if (!tools || count == 0) {
                printf("  \033[90mNo tools available.\033[0m\n");
            } else {
                for (int i = 0; i < count; i++) {
                    printf("  • \033[36m%s\033[0m\n", tools[i]->name);
                    if (tools[i]->description) {
                        printf("    %s\n", tools[i]->description);
                    }
                }
                free(tools);
            }
        } else {
            // List all tools from all servers
            int count;
            MCPToolRef* tools = mcp_list_all_tools(&count);

            printf("All Available Tools (%d):\n", count);
            printf("───────────────────────────\n");

            if (!tools || count == 0) {
                printf("  \033[90mNo tools available.\033[0m\n");
                printf("  Connect to a server first with '/mcp connect <name>'\n");
            } else {
                const char* last_server = NULL;
                for (int i = 0; i < count; i++) {
                    if (!last_server || strcmp(last_server, tools[i].server_name) != 0) {
                        if (last_server)
                            printf("\n");
                        printf("  \033[1m%s:\033[0m\n", tools[i].server_name);
                        last_server = tools[i].server_name;
                    }
                    printf("    • \033[36m%s\033[0m\n", tools[i].tool->name);
                }
                free(tools);
            }
        }
        printf("\n");
        return 0;
    }

    if (strcmp(subcmd, "call") == 0) {
        if (argc < 3) {
            printf("\033[31mUsage: /mcp call <tool_name> [json_arguments]\033[0m\n");
            return -1;
        }

        const char* tool_name = argv[2];
        cJSON* args = NULL;

        if (argc >= 4) {
            args = cJSON_Parse(argv[3]);
            if (!args) {
                printf("\033[31mInvalid JSON arguments\033[0m\n");
                return -1;
            }
        }

        printf("Calling tool: %s\n", tool_name);

        MCPToolResult* result = mcp_call_tool_auto(tool_name, args);
        if (args)
            cJSON_Delete(args);

        if (result->is_error) {
            printf("\033[31m✗ Error: %s\033[0m\n",
                   result->error_message ? result->error_message : "unknown error");
        } else {
            printf("\033[32m✓ Success\033[0m\n");
            if (result->content) {
                char* output = cJSON_Print(result->content);
                printf("%s\n", output);
                free(output);
            }
        }

        mcp_free_result(result);
        return 0;
    }

    printf("\033[31mUnknown MCP command: %s\033[0m\n", subcmd);
    printf("Use '/mcp' to see available commands.\n");
    return -1;
}

// ============================================================================
