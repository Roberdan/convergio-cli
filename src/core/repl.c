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
#include "nous/projects.h"
#include "nous/embedded_agents.h"
#include "nous/tools.h"
#include "nous/intent_router.h"
#include "nous/edition.h"
#include "nous/education.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>
#include <limits.h>
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
#define ANSI_GREEN         "\033[32m"
#define ANSI_RED           "\033[31m"
#define ANSI_YELLOW        "\033[33m"

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
// CURRENT AGENT CONTEXT
// ============================================================================

// Track which agent we're currently in conversation with (NULL = Ali)
static ManagedAgent* g_current_agent = NULL;

// Get the current agent (NULL means Ali is handling the conversation)
ManagedAgent* repl_get_current_agent(void) {
    return g_current_agent;
}

// Set the current agent for conversation continuity
void repl_set_current_agent(ManagedAgent* agent) {
    g_current_agent = agent;
}

// Clear current agent (return to Ali)
void repl_clear_current_agent(void) {
    g_current_agent = NULL;
}

// ============================================================================
// READLINE COMPLETION FOR @AGENTS
// ============================================================================

// Helper to extract agent short name from filename
// "anna-executive-assistant.md" -> "anna"
// "ali-chief-of-staff.md" -> "ali"
static char* extract_agent_name(const char* filename) {
    if (!filename) return NULL;

    // Skip CommonValuesAndPrinciples.md - not an agent
    if (strstr(filename, "CommonValues") != NULL) return NULL;

    // Find first hyphen to get short name
    const char* hyphen = strchr(filename, '-');
    if (hyphen) {
        size_t short_len = (size_t)(hyphen - filename);
        char* name = malloc(short_len + 1);
        if (name) {
            strncpy(name, filename, short_len);
            name[short_len] = '\0';
            return name;
        }
    }

    // Fallback: remove .md extension
    size_t len = strlen(filename);
    if (len > 3 && strcmp(filename + len - 3, ".md") == 0) {
        char* name = malloc(len - 2);
        if (name) {
            strncpy(name, filename, len - 3);
            name[len - 3] = '\0';
            return name;
        }
    }
    return strdup(filename);
}

// Generator function for @agent completions
static bool g_completion_with_at = true;  // Whether to prefix with @

static char* agent_name_generator(const char* text, int state) {
    static size_t list_index;
    static size_t agent_count;
    static const EmbeddedAgent* agents;

    // First call - get embedded agents list
    if (state == 0) {
        list_index = 0;
        agents = get_all_embedded_agents(&agent_count);
    }

    // Skip the @ prefix for matching
    const char* partial = text;
    if (partial[0] == '@') partial++;
    size_t partial_len = strlen(partial);

    // Return matches from embedded agents
    while (list_index < agent_count) {
        const EmbeddedAgent* agent = &agents[list_index++];
        if (agent && agent->filename) {
            char* name = extract_agent_name(agent->filename);
            if (name) {
                // Check if agent name starts with the partial text (case-insensitive)
                if (strncasecmp(name, partial, partial_len) == 0) {
                    char* match;
                    if (g_completion_with_at) {
                        // Return @name format
                        match = malloc(strlen(name) + 2);
                        if (match) {
                            snprintf(match, strlen(name) + 2, "@%s", name);
                        }
                    } else {
                        // Return name without @
                        match = strdup(name);
                    }
                    free(name);
                    return match;
                }
                free(name);
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
        g_completion_with_at = true;
        rl_attempted_completion_over = 1;  // Don't fall back to filename completion
        return rl_completion_matches(text, agent_name_generator);
    }

    // Check if we're completing after a space followed by @
    // (e.g., "hello @ba<TAB>")
    if (start > 0 && rl_line_buffer[start - 1] == ' ') {
        // Look for @ at current position
        char* at_pos = strchr(rl_line_buffer + start, '@');
        if (at_pos && at_pos == rl_line_buffer + start) {
            g_completion_with_at = true;
            rl_attempted_completion_over = 1;
            return rl_completion_matches(text, agent_name_generator);
        }
    }

    // For other cases, disable completion (no filename completion)
    // Autocomplete only works with @ prefix
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

// Spinner frames - Claude Code style (braille dots animation)
static const char* SPINNER_FRAMES[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
#define SPINNER_FRAME_COUNT 10

// Thinking verbs that rotate
static const char* SPINNER_VERBS[] = {
    "Reasoning",
    "Thinking",
    "Analyzing",
    "Processing",
    "Considering",
    "Evaluating",
};
#define SPINNER_VERB_COUNT 6

// Spinner thread function - polls for ESC key to cancel
static void* spinner_func(void* arg) {
    (void)arg;

    int frame = 0;
    int verb_index = 0;
    int elapsed_seconds = 0;
    int ticks_in_second = 0;

    // Get spinner color from theme (before entering loop, as theme is read-only during spin)
    const Theme* t = theme_get();
    const char* spinner_color = t->spinner ? t->spinner : "\033[38;5;208m";  // Orange fallback

    // Save terminal settings and enable raw mode for ESC detection
    struct termios raw;
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    raw = g_orig_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);  // Disable canonical mode and echo
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

        // Update elapsed time (10 ticks = 1 second at 100ms per tick)
        ticks_in_second++;
        if (ticks_in_second >= 10) {
            ticks_in_second = 0;
            elapsed_seconds++;
            // Change verb every ~4 seconds (skip first second to avoid immediate change)
            if (elapsed_seconds > 0 && elapsed_seconds % 4 == 0) {
                verb_index = (verb_index + 1) % SPINNER_VERB_COUNT;
            }
        }

        // Get current frame and verb
        const char* spinner_char = SPINNER_FRAMES[frame];
        const char* verb = SPINNER_VERBS[verb_index];

        // Format elapsed time
        char time_str[16];
        if (elapsed_seconds < 60) {
            snprintf(time_str, sizeof(time_str), "%ds", elapsed_seconds);
        } else {
            snprintf(time_str, sizeof(time_str), "%dm%02ds", elapsed_seconds / 60, elapsed_seconds % 60);
        }

        // Colored spinner with braille dots animation (Claude Code style) + timer
        printf(ANSI_CURSOR_START "%s%s" ANSI_RESET " " ANSI_DIM "%s ..." ANSI_RESET "   "
               ANSI_DIM "%s" ANSI_RESET "   (ESC to cancel)   ",
               spinner_color, spinner_char, verb, time_str);
        fflush(stdout);

        frame = (frame + 1) % SPINNER_FRAME_COUNT;
        usleep(100000);  // 100ms for smooth animation
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

// Handle direct bash prefix (! or $)
static int repl_execute_direct_bash(const char* command) {
    // Safety check
    if (!tools_is_command_safe(command)) {
        printf(ANSI_RED "⚠ Command blocked: potentially dangerous operation\n" ANSI_RESET);
        printf("Blocked commands: rm -rf, dd, mkfs, etc.\n");
        return 0;
    }

    // Get working directory
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        strlcpy(cwd, ".", sizeof(cwd));
    }

    // Execute with 60 second timeout
    printf(ANSI_DIM "$ %s" ANSI_RESET "\n", command);
    ToolResult* result = tool_shell_exec(command, cwd, 60);

    if (result) {
        if (result->success && result->output) {
            printf("%s", result->output);
            if (result->output[strlen(result->output) - 1] != '\n') {
                printf("\n");
            }
        } else if (result->error) {
            printf(ANSI_RED "Error: %s" ANSI_RESET "\n", result->error);
        }
        if (result->exit_code != 0) {
            printf(ANSI_DIM "Exit code: %d" ANSI_RESET "\n", result->exit_code);
        }
        tools_free_result(result);
    }

    return 0;
}

int repl_process_natural_input(const char* input) {
    if (!input || strlen(input) == 0) return 0;

    // Check for direct bash prefix (! or $)
    if (input[0] == '!' || input[0] == '$') {
        return repl_execute_direct_bash(input + 1);
    }

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

            // In Education edition, transform error messages to friendly ones
            if (education_should_interpret_error(response)) {
                char* friendly = education_interpret_error(response, "ali-principal");
                if (friendly) {
                    free(response);
                    response = friendly;
                }
            }

            // Render markdown to ANSI for nice terminal output
            md_print(response);
            printf("\n");
            free(response);
        } else {
            printf(ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", name);
            printf("Sorry, I encountered a problem. Please try again.\n");
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

    // Find or spawn the agent
    ManagedAgent* agent = agent_find_by_name(agent_name);
    if (!agent) {
        // Try to spawn the agent (it might exist as embedded but not yet spawned)
        agent = agent_spawn(AGENT_ROLE_ANALYST, agent_name, NULL);
    }
    if (!agent) {
        printf(ANSI_DIM "Agent '%s' not found. Use 'agents' to see available agents." ANSI_RESET "\n", agent_name);
        return 0;
    }

    if (!agent->system_prompt) {
        printf(ANSI_DIM "Agent '%s' has no system prompt configured." ANSI_RESET "\n", agent_name);
        return 0;
    }

    // Check if agent is in current project team
    ConvergioProject* proj = project_current();
    if (proj && !project_has_agent(agent_name)) {
        printf("\n" ANSI_YELLOW "⚠ Agent '%s' is not in project '%s' team." ANSI_RESET "\n", agent_name, proj->name);
        printf(ANSI_DIM "Current team: ");
        for (size_t i = 0; i < proj->team_count; i++) {
            printf("%s%s", proj->team[i].agent_name, i < proj->team_count - 1 ? ", " : "");
        }
        printf(ANSI_RESET "\n\n");
        printf("Would you like to add '%s' to the team? [y/N] ", agent_name);

        char response[16];
        if (fgets(response, sizeof(response), stdin) && (response[0] == 'y' || response[0] == 'Y')) {
            if (project_team_add(proj, agent_name, NULL)) {
                printf(ANSI_GREEN "✓ Added '%s' to project team." ANSI_RESET "\n\n", agent_name);
            } else {
                printf(ANSI_RED "✗ Failed to add agent to team." ANSI_RESET "\n");
                return 0;
            }
        } else {
            printf(ANSI_DIM "Use 'project team add %s' to add manually, or 'project clear' to exit project mode." ANSI_RESET "\n", agent_name);
            return 0;
        }
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

        // In Education edition, transform error messages to friendly ones
        if (education_should_interpret_error(response)) {
            char* friendly = education_interpret_error(response, agent_name);
            if (friendly) {
                free(response);
                response = friendly;
            }
        }

        // Render markdown
        md_print(response);
        printf("\n");
        free(response);

        // Set this agent as current for conversation continuity
        repl_set_current_agent(agent);
    } else {
        printf(ANSI_BOLD ANSI_CYAN "%s" ANSI_RESET "\n\n", agent->name);
        printf("I couldn't respond. Please try again.\n");
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

    // Save original input before tokenization (which modifies line in-place)
    char* original_input = strdup(line);
    if (!original_input) return -1;

    // Check for direct agent communication: @agent_name [message]
    // - @agent_name message -> send message to agent
    // - @agent_name        -> switch to agent mode (continue conversation with that agent)
    if (line[0] == '@') {
        // Extract agent name (until first space or end of line)
        char agent_name[128] = {0};
        const char* msg_start = NULL;

        const char* space = strchr(line, ' ');
        if (space) {
            size_t name_len = (size_t)(space - line - 1);  // -1 to skip @
            if (name_len > 0 && name_len < sizeof(agent_name)) {
                strncpy(agent_name, line + 1, name_len);
                agent_name[name_len] = '\0';
                msg_start = space + 1;
                // Skip leading whitespace in message
                while (*msg_start == ' ' || *msg_start == '\t') msg_start++;
            }
        } else {
            // No space - just agent name (switch mode)
            size_t name_len = strlen(line) - 1;  // -1 to skip @
            if (name_len > 0 && name_len < sizeof(agent_name)) {
                strncpy(agent_name, line + 1, name_len);
                agent_name[name_len] = '\0';
            }
        }

        // Validate agent name
        if (strlen(agent_name) == 0) {
            printf("Usage: @agent_name [message]\n");
            printf("  @baccio            Switch to talk with Baccio\n");
            printf("  @baccio ciao!      Send message to Baccio\n");
            printf("Type 'agents' to see available agents.\n");
            free(original_input);
            return 0;
        }

        // Find or spawn the agent
        ManagedAgent* agent = agent_find_by_name(agent_name);
        if (!agent) {
            // Try to spawn the agent (it might exist as embedded but not yet spawned)
            agent = agent_spawn(AGENT_ROLE_ANALYST, agent_name, NULL);
        }
        if (!agent) {
            printf(ANSI_YELLOW "Agent '%s' not found." ANSI_RESET "\n", agent_name);
            printf("Type 'agents' to see available agents, or try Tab completion.\n");
            free(original_input);
            return 0;
        }

        // If message provided, send to agent
        if (msg_start && strlen(msg_start) > 0) {
            int res = repl_direct_agent_communication(agent_name, msg_start);
            free(original_input);
            return res;
        }

        // No message - switch to this agent
        repl_set_current_agent(agent);
        printf(ANSI_GREEN "Switched to " ANSI_BOLD "%s" ANSI_RESET ANSI_GREEN "." ANSI_RESET "\n", agent->name);
        if (agent->description) {
            printf(ANSI_DIM "%s" ANSI_RESET "\n", agent->description);
        }
        printf(ANSI_DIM "All your messages will now go to %s. Type 'ali' or 'back' to return to Ali." ANSI_RESET "\n", agent->name);
        free(original_input);
        return 0;
    }

    // Tokenize for command parsing (with quote handling)
    char* argv[64];
    int argc = 0;

    char* p = line;
    while (*p && argc < 64) {
        // Skip leading whitespace
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        char* start;
        if (*p == '"') {
            // Quoted string - find closing quote
            p++;  // Skip opening quote
            start = p;
            while (*p && *p != '"') p++;
            if (*p == '"') {
                *p = '\0';  // Replace closing quote with null
                p++;
            }
        } else {
            // Unquoted token - find next whitespace
            start = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) {
                *p = '\0';
                p++;
            }
        }

        if (*start) {
            argv[argc++] = start;
        }
    }

    if (argc == 0) {
        free(original_input);
        return 0;
    }

    // Look for built-in command (support both "quit" and "/quit" syntax)
    const char* cmd_name = argv[0];
    if (cmd_name[0] == '/') {
        cmd_name++;  // Skip leading slash
    }

    // =========================================================================
    // NATURAL LANGUAGE AGENT ADDRESSING
    // Handle human-style conversation: "amy come stai?" "baccio aiutami"
    // =========================================================================

    // Special case: "back" alone returns to Ali (it's not an agent name)
    if (argc == 1 && strcasecmp(cmd_name, "back") == 0) {
        if (g_current_agent) {
            printf(ANSI_DIM "Returning to Ali..." ANSI_RESET "\n");
            repl_clear_current_agent();
        } else {
            printf(ANSI_DIM "Already talking to Ali." ANSI_RESET "\n");
        }
        free(original_input);
        return 0;
    }

    // Check if first word is a KNOWN agent name (natural language addressing)
    // Examples: "ali come stai?" "amy puoi aiutarmi?" "baccio rivedi questo"
    // IMPORTANT: Skip if it starts with "/" - those are commands, not agent names
    // IMPORTANT: Only match known agents, don't spawn generic agents for random words
    ManagedAgent* addressed_agent = NULL;
    if (argv[0][0] != '/') {
        // Only find existing agents or known embedded agents
        addressed_agent = agent_find_by_name(argv[0]);
        if (!addressed_agent && agent_is_known_name(argv[0])) {
            // It's a known agent that just needs spawning
            addressed_agent = agent_spawn(AGENT_ROLE_ANALYST, argv[0], NULL);
        }
    }

    if (addressed_agent) {
        if (argc == 1) {
            // Just agent name alone: switch to that agent
            // "amy" -> switch to Amy
            repl_set_current_agent(addressed_agent);
            printf(ANSI_GREEN "Switched to " ANSI_BOLD "%s" ANSI_RESET ANSI_GREEN "." ANSI_RESET "\n", addressed_agent->name);
            if (addressed_agent->description) {
                printf(ANSI_DIM "%s" ANSI_RESET "\n", addressed_agent->description);
            }
            printf(ANSI_DIM "Type 'back' to return to Ali." ANSI_RESET "\n");
            free(original_input);
            return 0;
        } else {
            // Agent name + message: send message to that agent
            // "amy come stai?" -> send "come stai?" to Amy
            // Use original_input because line is corrupted by tokenization
            const char* msg_start = original_input;
            while (*msg_start && !isspace((unsigned char)*msg_start)) msg_start++;
            while (*msg_start && isspace((unsigned char)*msg_start)) msg_start++;

            if (*msg_start) {
                int res = repl_direct_agent_communication(addressed_agent->name, msg_start);
                free(original_input);
                return res;
            }
        }
    }

    const ReplCommand* commands = commands_get_table();
    for (const ReplCommand* cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(cmd_name, cmd->name) == 0) {
            // Check if command is available in current edition
            if (!edition_has_command(cmd->name)) {
                printf("\033[33mCommand '/%s' is not available in %s.\033[0m\n",
                       cmd->name, edition_display_name());
                printf("This command is part of a different Convergio edition.\n\n");
                free(original_input);
                return -1;
            }
            int res = cmd->handler(argc, argv);
            free(original_input);
            return res;
        }
    }

    // Not a command, treat as natural language intent
    // Use intelligent router to decide which agent should handle this
    // Check BEFORE continuing with current agent to catch switch intents like "passami amy"
    // IMPORTANT: Use original_input because line has been corrupted by tokenization
    RouterResult route = intent_router_route(original_input);

    // Handle INTENT_SWITCH: user wants to switch to a different agent
    if (route.type == INTENT_SWITCH && route.confidence >= 0.8f) {
        LOG_INFO(LOG_CAT_AGENT, "Router: switch to %s (%.0f%%)",
                 route.agent, (double)(route.confidence * 100));

        // Find or spawn the target agent
        ManagedAgent* target = agent_find_by_name(route.agent);
        if (!target) {
            target = agent_spawn(AGENT_ROLE_ANALYST, route.agent, NULL);
        }

        if (target) {
            repl_set_current_agent(target);
            printf(ANSI_GREEN "Switched to " ANSI_BOLD "%s" ANSI_RESET ANSI_GREEN "." ANSI_RESET "\n", target->name);
            if (target->description) {
                printf(ANSI_DIM "%s" ANSI_RESET "\n", target->description);
            }
            free(original_input);
            return 0;
        } else {
            printf(ANSI_YELLOW "Could not find agent '%s'." ANSI_RESET "\n", route.agent);
            free(original_input);
            return -1;
        }
    }

    // If we have a current agent and no switch intent, continue with that agent
    if (g_current_agent) {
        int res = repl_direct_agent_communication(g_current_agent->name, original_input);
        free(original_input);
        return res;
    }

    // If router is confident about a specific agent (not Ali), go directly to that agent
    if (strcmp(route.agent, "ali") != 0 && route.confidence >= 0.7f) {
        LOG_INFO(LOG_CAT_AGENT, "Router: %s (%.0f%%) - %s",
                 route.agent, (double)(route.confidence * 100), route.intent);
        int res = repl_direct_agent_communication(route.agent, original_input);
        free(original_input);
        return res;
    }

    // Otherwise, go to Ali via orchestrator (Ali can still delegate if needed)
    int result = repl_process_natural_input(original_input);
    free(original_input);
    return result;
}
