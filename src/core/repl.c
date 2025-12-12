/**
 * CONVERGIO KERNEL - REPL Implementation
 *
 * Read-Eval-Print Loop and input processing
 */

#include "nous/repl.h"
#include "nous/commands.h"
#include "nous/signals.h"
#include "nous/nous.h"
#include "nous/orchestrator.h"
#include "nous/stream_md.h"
#include "nous/theme.h"
#include "nous/clipboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <readline/readline.h>

// ============================================================================
// ANSI ESCAPE CODES
// ============================================================================

#define ANSI_CLEAR_LINE    "\033[2K"
#define ANSI_CURSOR_UP     "\033[1A"
#define ANSI_CURSOR_START  "\r"
#define ANSI_HIDE_CURSOR   "\033[?25l"
#define ANSI_SHOW_CURSOR   "\033[?25h"
#define ANSI_DIM           "\033[2m"
#define ANSI_RESET         "\033[0m"
#define ANSI_CYAN          "\033[36m"
#define ANSI_BOLD          "\033[1m"

// ============================================================================
// SPINNER STATE
// ============================================================================

static volatile bool g_spinner_active = false;
static volatile bool g_spinner_cancelled = false;
static pthread_t g_spinner_thread;
static struct termios g_orig_termios;

// ============================================================================
// STREAMING STATE
// ============================================================================

static StreamMd* g_stream_md = NULL;

// ============================================================================
// READLINE COMPLETION FOR @AGENTS
// ============================================================================

// Generator function for @agent completions
static char* agent_name_generator(const char* text, int state) {
    static size_t list_index;
    static ManagedAgent* agents[64];
    static size_t agent_count;

    // First call - build agent list
    if (state == 0) {
        list_index = 0;
        agent_count = agent_get_all(agents, 64);
    }

    // Skip the @ prefix for matching
    const char* partial = text;
    if (partial[0] == '@') partial++;

    // Return matches
    while (list_index < agent_count) {
        ManagedAgent* agent = agents[list_index++];
        if (agent && agent->name) {
            // Check if agent name starts with the partial text (case-insensitive)
            if (strncasecmp(agent->name, partial, strlen(partial)) == 0) {
                // Return @name format
                char* match = malloc(strlen(agent->name) + 2);
                if (match) {
                    snprintf(match, strlen(agent->name) + 2, "@%s", agent->name);
                    return match;
                }
            }
        }
    }

    return NULL;
}

// Completion function - called when tab is pressed
char** repl_agent_completion(const char* text, int start, int end) {
    (void)end;

    // Complete @agent names anywhere in the line if text starts with @
    if (text[0] == '@') {
        rl_attempted_completion_over = 1;  // Don't fall back to filename completion
        return rl_completion_matches(text, agent_name_generator);
    }

    // Check if we're completing after a space followed by @
    // (e.g., "hello @ba<TAB>")
    if (start > 0 && rl_line_buffer[start - 1] == ' ') {
        // Look for @ at current position
        char* at_pos = strchr(rl_line_buffer + start, '@');
        if (at_pos && at_pos == rl_line_buffer + start) {
            rl_attempted_completion_over = 1;
            return rl_completion_matches(text, agent_name_generator);
        }
    }

    // For other cases, disable completion (no filename completion)
    rl_attempted_completion_over = 1;
    return NULL;
}

// ============================================================================
// CLIPBOARD IMAGE PASTE (Ctrl+I)
// ============================================================================

int repl_paste_clipboard_image(int count, int key) {
    (void)count;
    (void)key;

    if (clipboard_has_image()) {
        // Save image to temp directory
        char* image_path = clipboard_save_image(NULL);  // NULL = use temp directory
        if (image_path) {
            // Insert path at cursor position
            rl_insert_text(image_path);
            free(image_path);
            rl_redisplay();
            return 0;
        } else {
            printf("\a");  // Beep on error
            fflush(stdout);
            return 1;
        }
    } else {
        // No image in clipboard - try normal paste
        char* text = clipboard_get_text();
        if (text) {
            rl_insert_text(text);
            free(text);
            rl_redisplay();
            return 0;
        }
        printf("\a");  // Beep - nothing to paste
        fflush(stdout);
        return 1;
    }
}

// ============================================================================
// UI HELPERS
// ============================================================================

void repl_print_separator(void) {
    printf("\n" ANSI_DIM "────────────────────────────────────────────────────────────────" ANSI_RESET "\n\n");
}

// Spinner thread function - polls for ESC key to cancel
static void* spinner_func(void* arg) {
    (void)arg;
    const char* frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    int frame = 0;

    // Save terminal settings and enable raw mode for ESC detection
    struct termios raw;
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    raw = g_orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
    raw.c_cc[VMIN] = 0;   // Non-blocking read
    raw.c_cc[VTIME] = 0;  // No timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    printf(ANSI_HIDE_CURSOR);
    while (g_spinner_active) {
        // Check for ESC key (ASCII 27)
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == 27) {  // ESC key
                g_spinner_cancelled = true;
                claude_cancel_request();
                break;
            }
        }

        printf(ANSI_CURSOR_START ANSI_DIM "%s pensando... " ANSI_RESET "(ESC to cancel)  ", frames[frame]);
        fflush(stdout);
        frame = (frame + 1) % 10;
        usleep(80000);  // 80ms
    }

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);

    // Clear spinner line
    printf(ANSI_CURSOR_START ANSI_CLEAR_LINE);
    printf(ANSI_SHOW_CURSOR);
    fflush(stdout);
    return NULL;
}

void repl_spinner_start(void) {
    g_spinner_active = true;
    g_spinner_cancelled = false;
    claude_reset_cancel();
    pthread_create(&g_spinner_thread, NULL, spinner_func, NULL);
}

void repl_spinner_stop(void) {
    if (g_spinner_active) {
        g_spinner_active = false;
        pthread_join(g_spinner_thread, NULL);
    }
}

bool repl_spinner_was_cancelled(void) {
    return g_spinner_cancelled;
}

// ============================================================================
// STREAMING CALLBACK
// ============================================================================

// Streaming callback - renders markdown incrementally as chunks arrive
static void stream_md_callback(const char* chunk, void* user_data) {
    (void)user_data;
    if (!chunk) return;

    // Create renderer if needed
    if (!g_stream_md) {
        g_stream_md = stream_md_create();
    }

    // Process chunk through streaming markdown renderer
    if (g_stream_md) {
        stream_md_process(g_stream_md, chunk, strlen(chunk));
    }
}

// ============================================================================
// BUDGET HANDLING
// ============================================================================

bool repl_handle_budget_exceeded(void) {
    Orchestrator* orch = orchestrator_get();
    if (!orch) return false;

    const Theme* t = theme_get();

    printf("\n");
    printf("  %s┌─────────────────────────────────────────────────────────────┐%s\n", t->warning, theme_reset());
    printf("  %s│  ⚠  BUDGET LIMIT REACHED                                    │%s\n", t->warning, theme_reset());
    printf("  %s└─────────────────────────────────────────────────────────────┘%s\n", t->warning, theme_reset());
    printf("\n");
    printf("  Current spend:  %s$%.4f%s\n", t->cost, orch->cost.current_spend_usd, theme_reset());
    printf("  Budget limit:   %s$%.2f%s\n", t->cost, orch->cost.budget_limit_usd, theme_reset());
    printf("\n");
    printf("  What would you like to do?\n");
    printf("\n");
    printf("    %s1%s) Increase budget by $5.00\n", t->prompt_arrow, theme_reset());
    printf("    %s2%s) Increase budget by $10.00\n", t->prompt_arrow, theme_reset());
    printf("    %s3%s) Set custom budget\n", t->prompt_arrow, theme_reset());
    printf("    %s4%s) View cost report\n", t->prompt_arrow, theme_reset());
    printf("    %s5%s) Cancel (don't send this message)\n", t->prompt_arrow, theme_reset());
    printf("\n");
    printf("  Choice [1-5]: ");
    fflush(stdout);

    char choice[16] = {0};
    if (!fgets(choice, sizeof(choice), stdin)) {
        return false;
    }

    switch (choice[0]) {
        case '1':
            cost_set_budget(orch->cost.budget_limit_usd + 5.0);
            printf("\n  %s✓ Budget increased to $%.2f%s\n\n", t->success, orch->cost.budget_limit_usd, theme_reset());
            return true;

        case '2':
            cost_set_budget(orch->cost.budget_limit_usd + 10.0);
            printf("\n  %s✓ Budget increased to $%.2f%s\n\n", t->success, orch->cost.budget_limit_usd, theme_reset());
            return true;

        case '3': {
            printf("  Enter new budget limit (USD): $");
            fflush(stdout);
            char amount[32] = {0};
            if (fgets(amount, sizeof(amount), stdin)) {
                double new_budget = atof(amount);
                if (new_budget > 0) {
                    cost_set_budget(new_budget);
                    printf("\n  %s✓ Budget set to $%.2f%s\n\n", t->success, new_budget, theme_reset());
                    return true;
                } else {
                    printf("\n  %sInvalid amount.%s\n\n", t->error, theme_reset());
                }
            }
            return false;
        }

        case '4': {
            char* report = cost_get_report();
            if (report) {
                printf("\n%s\n", report);
                free(report);
            }
            // Show menu again
            return repl_handle_budget_exceeded();
        }

        case '5':
        default:
            printf("\n  %sMessage cancelled.%s\n\n", t->info, theme_reset());
            return false;
    }
}

// ============================================================================
// NATURAL LANGUAGE PROCESSING
// ============================================================================

// External markdown renderer
extern void md_print(const char* markdown);

int repl_process_natural_input(const char* input) {
    if (!input || strlen(input) == 0) return 0;

    Orchestrator* orch = orchestrator_get();
    if (!orch || !orch->initialized) {
        // Fallback to old Aria if orchestrator not ready
        NousAgent* assistant = (NousAgent*)g_assistant;
        if (assistant) {
            char* response = nous_agent_think_with_claude(assistant, input);
            if (response) {
                printf("\n%s: %s\n\n", assistant->name, response);
                free(response);
            }
        } else {
            printf("System not ready. Try 'help' for commands.\n");
        }
        return 0;
    }

    // Check budget before processing
    if (orch->cost.budget_exceeded) {
        if (!repl_handle_budget_exceeded()) {
            return 0;  // User cancelled
        }
    }

    // Print separator between input and output
    repl_print_separator();

    // Get Ali's name
    const char* name = orch->ali ? orch->ali->name : "Ali";

    char* response = NULL;

    if (g_streaming_enabled) {
        // STREAMING MODE: Live markdown rendering as response arrives
        // Print Ali's name header before streaming starts
        printf(ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", name);

        // Initialize streaming markdown renderer
        g_stream_md = stream_md_create();

        // Mark streaming as active (affects CTRL+C behavior)
        stream_set_active(true);

        // Process with streaming callback - output renders live
        response = orchestrator_process_stream(input, stream_md_callback, NULL);

        // Mark streaming as inactive
        stream_set_active(false);

        // Finalize streaming renderer
        if (g_stream_md) {
            stream_md_finish(g_stream_md);
            stream_md_destroy(g_stream_md);
            g_stream_md = NULL;
        }

        // Reset cancellation flag for next request
        stream_reset_cancel();

        // Response already displayed via streaming, just free it
        if (response) {
            free(response);
        }
    } else {
        // BATCH MODE: Wait for full response, then render with nice formatting
        // Start spinner while waiting for response
        repl_spinner_start();

        // Use orchestrator with full tool support
        response = orchestrator_process(input);

        // Stop spinner
        repl_spinner_stop();

        // Check if request was cancelled by user
        if (repl_spinner_was_cancelled()) {
            printf(ANSI_DIM "Request cancelled" ANSI_RESET "\n");
            if (response) free(response);
            return 0;
        }

        if (response) {
            // Print Ali's name as header
            printf(ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", name);

            // Render markdown to ANSI for nice terminal output
            md_print(response);
            printf("\n");
            free(response);
        } else {
            printf(ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", name);
            printf("Mi dispiace, ho avuto un problema. Riprova.\n");
        }
    }

    printf("\n");
    return 0;
}

// ============================================================================
// DIRECT AGENT COMMUNICATION
// ============================================================================

int repl_direct_agent_communication(const char* agent_name, const char* message) {
    if (!agent_name || !message || strlen(message) == 0) {
        printf("Usage: @agent_name your message\n");
        return 0;
    }

    // Find the agent
    ManagedAgent* agent = agent_find_by_name(agent_name);
    if (!agent) {
        printf(ANSI_DIM "Agent '%s' not found. Use 'agents' to see available agents." ANSI_RESET "\n", agent_name);
        return 0;
    }

    if (!agent->system_prompt) {
        printf(ANSI_DIM "Agent '%s' has no system prompt configured." ANSI_RESET "\n", agent_name);
        return 0;
    }

    // Print separator
    repl_print_separator();

    // Start spinner
    repl_spinner_start();

    // Use orchestrator_agent_chat for full tool support (web_fetch, file_read, etc.)
    char* response = orchestrator_agent_chat(agent, message);

    // Stop spinner
    repl_spinner_stop();

    // Check if cancelled
    if (repl_spinner_was_cancelled()) {
        printf(ANSI_DIM "Request cancelled" ANSI_RESET "\n");
        if (response) free(response);
        return 0;
    }

    if (response) {
        // Print agent's name as header
        printf(ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", agent->name);

        // Render markdown
        md_print(response);
        printf("\n");
        free(response);
    } else {
        printf(ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", agent->name);
        printf("Non sono riuscito a rispondere. Riprova.\n");
    }

    printf("\n");
    return 0;
}

// ============================================================================
// COMMAND PARSING AND EXECUTION
// ============================================================================

int repl_parse_and_execute(char* line) {
    // Skip empty lines
    if (!line || strlen(line) == 0) return 0;

    // Check for direct agent communication: @agent_name message
    if (line[0] == '@') {
        // Extract agent name (until first space)
        char agent_name[128] = {0};
        const char* msg_start = NULL;

        const char* space = strchr(line, ' ');
        if (space) {
            size_t name_len = space - line - 1;  // -1 to skip @
            if (name_len > 0 && name_len < sizeof(agent_name)) {
                strncpy(agent_name, line + 1, name_len);
                agent_name[name_len] = '\0';
                msg_start = space + 1;
                // Skip leading whitespace in message
                while (*msg_start == ' ' || *msg_start == '\t') msg_start++;
            }
        }

        if (strlen(agent_name) > 0 && msg_start && strlen(msg_start) > 0) {
            return repl_direct_agent_communication(agent_name, msg_start);
        } else {
            printf("Usage: @agent_name your message\n");
            printf("Example: @baccio What's the best architecture for this system?\n");
            return 0;
        }
    }

    // Tokenize for command parsing
    char* argv[64];
    int argc = 0;

    char* token = strtok(line, " \t");
    while (token && argc < 64) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }

    if (argc == 0) return 0;

    // Look for built-in command (support both "quit" and "/quit" syntax)
    const char* cmd_name = argv[0];
    if (cmd_name[0] == '/') {
        cmd_name++;  // Skip leading slash
    }

    const ReplCommand* commands = commands_get_table();
    for (const ReplCommand* cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(cmd_name, cmd->name) == 0) {
            return cmd->handler(argc, argv);
        }
    }

    // Not a command, treat as natural language intent
    // Reconstruct the original line
    char* original = strdup(rl_line_buffer);
    int result = repl_process_natural_input(original);
    free(original);
    return result;
}
