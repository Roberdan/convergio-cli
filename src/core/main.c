/**
 * CONVERGIO KERNEL
 *
 * Main entry point and CLI interface
 * A semantic kernel for human-AI symbiosis
 * With Ali as Chief of Staff orchestrating all agents
 */

#include "nous/nous.h"
#include "nous/orchestrator.h"
#include "nous/tools.h"
#include "nous/config.h"
#include "nous/hardware.h"
#include "nous/updater.h"
#include "../auth/oauth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <termios.h>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

extern int nous_gpu_init(void);
extern void nous_gpu_shutdown(void);
extern void nous_gpu_print_stats(void);
extern int nous_scheduler_init(void);
extern void nous_scheduler_shutdown(void);
extern void nous_scheduler_print_metrics(void);
extern void nous_destroy_agent(NousAgent* agent);

// Default budget in USD
#define DEFAULT_BUDGET_USD 5.00

// ============================================================================
// DEBUG LOGGING IMPLEMENTATION
// ============================================================================

LogLevel g_log_level = LOG_LEVEL_NONE;

static const char* LOG_LEVEL_NAMES[] = {
    "NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
};

static const char* LOG_CAT_NAMES[] = {
    "SYSTEM", "AGENT", "TOOL", "API", "MEMORY", "MSGBUS", "COST"
};

static const char* LOG_CAT_COLORS[] = {
    "\033[36m",   // Cyan - SYSTEM
    "\033[33m",   // Yellow - AGENT
    "\033[32m",   // Green - TOOL
    "\033[35m",   // Magenta - API
    "\033[34m",   // Blue - MEMORY
    "\033[37m",   // White - MSGBUS
    "\033[31m"    // Red - COST
};

void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...) {
    if (level > g_log_level || g_log_level == LOG_LEVEL_NONE) return;

    // Timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    // Level indicator
    const char* level_color;
    switch (level) {
        case LOG_LEVEL_ERROR: level_color = "\033[31m"; break;  // Red
        case LOG_LEVEL_WARN:  level_color = "\033[33m"; break;  // Yellow
        case LOG_LEVEL_INFO:  level_color = "\033[32m"; break;  // Green
        case LOG_LEVEL_DEBUG: level_color = "\033[36m"; break;  // Cyan
        case LOG_LEVEL_TRACE: level_color = "\033[2m"; break;   // Dim
        default: level_color = "\033[0m";
    }

    // Print header
    fprintf(stderr, "\033[2m[%s]\033[0m %s[%-5s]\033[0m %s[%s]\033[0m ",
            time_str,
            level_color, LOG_LEVEL_NAMES[level],
            LOG_CAT_COLORS[cat], LOG_CAT_NAMES[cat]);

    // Print message
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\033[0m\n");
}

void nous_log_set_level(LogLevel level) {
    g_log_level = level;
}

LogLevel nous_log_get_level(void) {
    return g_log_level;
}

const char* nous_log_level_name(LogLevel level) {
    if (level <= LOG_LEVEL_TRACE) return LOG_LEVEL_NAMES[level];
    return "UNKNOWN";
}

// ============================================================================
// GLOBAL STATE
// ============================================================================

static volatile sig_atomic_t g_running = 1;
static NousSpace* g_current_space = NULL;
static NousAgent* g_assistant = NULL;

// ============================================================================
// SIGNAL HANDLING
// ============================================================================

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    // Use write() instead of printf() - it's async-signal-safe
    // printf() can deadlock if signal arrives during malloc/free
    (void)write(STDOUT_FILENO, "\n", 1);
}

// ============================================================================
// REPL COMMANDS
// ============================================================================

typedef struct {
    const char* name;
    const char* description;
    int (*handler)(int argc, char** argv);
} ReplCommand;

static int cmd_help(int argc, char** argv);
static int cmd_create(int argc, char** argv);
static int cmd_agent(int argc, char** argv);
static int cmd_space(int argc, char** argv);
static int cmd_status(int argc, char** argv);
static int cmd_quit(int argc, char** argv);
static int cmd_think(int argc, char** argv);
static int cmd_cost(int argc, char** argv);
static int cmd_agents(int argc, char** argv);
static int cmd_debug(int argc, char** argv);
static int cmd_allow_dir(int argc, char** argv);
static int cmd_allowed_dirs(int argc, char** argv);
// static int cmd_login(int argc, char** argv);  // Disabled - see ADR 005
static int cmd_logout(int argc, char** argv);
static int cmd_auth(int argc, char** argv);
static int cmd_update(int argc, char** argv);
static int cmd_hardware(int argc, char** argv);

static const ReplCommand COMMANDS[] = {
    {"help",        "Show available commands",           cmd_help},
    {"create",      "Create a semantic node",            cmd_create},
    {"agent",       "Manage agents",                     cmd_agent},
    {"agents",      "List all available agents",         cmd_agents},
    {"space",       "Manage collaborative spaces",       cmd_space},
    {"status",      "Show system status",                cmd_status},
    {"cost",        "Show/set cost and budget",          cmd_cost},
    {"debug",       "Toggle debug mode (off/error/warn/info/debug/trace)", cmd_debug},
    {"allow-dir",   "Add directory to sandbox",          cmd_allow_dir},
    {"allowed-dirs","Show allowed directories",          cmd_allowed_dirs},
    {"logout",      "Logout and clear credentials",      cmd_logout},
    {"auth",        "Show authentication status",        cmd_auth},
    {"update",      "Check for and install updates",     cmd_update},
    {"hardware",    "Show hardware information",         cmd_hardware},
    {"think",       "Process an intent",                 cmd_think},
    {"quit",        "Exit Convergio",                    cmd_quit},
    {"exit",        "Exit Convergio",                    cmd_quit},
    {NULL, NULL, NULL}
};

static int cmd_help(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("\nCONVERGIO KERNEL - Multi-Agent Orchestration\n");
    printf("=============================================\n\n");
    printf("Available commands:\n\n");

    for (const ReplCommand* cmd = COMMANDS; cmd->name != NULL; cmd++) {
        printf("  %-12s %s\n", cmd->name, cmd->description);
    }

    printf("\nCost commands:\n");
    printf("  cost              Show current spending\n");
    printf("  cost report       Detailed cost report\n");
    printf("  cost set <USD>    Set budget limit\n");
    printf("  cost reset        Reset session spending\n");

    printf("\nWorkspace/Sandbox:\n");
    printf("  allowed-dirs      Show allowed directories\n");
    printf("  allow-dir <path>  Add directory to sandbox\n");

    printf("\nAuthentication:\n");
    printf("  auth              Show authentication status\n");
    printf("  logout            Logout and clear credentials\n");

    printf("\nOr simply talk to Ali, your Chief of Staff.\n");
    printf("Ali will coordinate specialist agents as needed.\n\n");

    return 0;
}

static int cmd_cost(int argc, char** argv) {
    if (argc < 2) {
        // Show brief cost status
        char* status = cost_get_status_line();
        if (status) {
            printf("%s\n", status);
            free(status);
        }
        return 0;
    }

    if (strcmp(argv[1], "report") == 0) {
        char* report = cost_get_report();
        if (report) {
            printf("%s", report);
            free(report);
        }
        return 0;
    }

    if (strcmp(argv[1], "set") == 0) {
        if (argc < 3) {
            printf("Usage: cost set <amount_usd>\n");
            printf("Example: cost set 10.00\n");
            return -1;
        }
        double budget = atof(argv[2]);
        if (budget <= 0) {
            printf("Invalid budget amount.\n");
            return -1;
        }
        cost_set_budget(budget);
        printf("Budget set to $%.2f\n", budget);
        return 0;
    }

    if (strcmp(argv[1], "reset") == 0) {
        cost_reset_session();
        printf("Session spending reset.\n");
        return 0;
    }

    printf("Unknown cost command: %s\n", argv[1]);
    printf("Try: cost, cost report, cost set <amount>, cost reset\n");
    return -1;
}

static int cmd_agents(int argc, char** argv) {
    (void)argc;

    // Check for subcommands
    if (argc >= 2) {
        if (strcmp(argv[1], "working") == 0 || strcmp(argv[1], "active") == 0) {
            // Show only working agents
            char* working = agent_get_working_status();
            if (working) {
                printf("\n%s\n", working);
                free(working);
            }
            return 0;
        }
    }

    // Show working status first
    char* working = agent_get_working_status();
    if (working) {
        printf("\n%s", working);
        free(working);
    }

    // Then show full registry
    printf("\n");
    char* status = agent_registry_status();
    if (status) {
        printf("%s", status);
        free(status);
    }
    return 0;
}

static int cmd_debug(int argc, char** argv) {
    if (argc < 2) {
        // Show current level and toggle
        LogLevel current = nous_log_get_level();
        if (current == LOG_LEVEL_NONE) {
            nous_log_set_level(LOG_LEVEL_INFO);
            printf("\033[32m✓ Debug mode enabled (level: INFO)\033[0m\n");
            printf("  Use 'debug <level>' to change: off, error, warn, info, debug, trace\n");
        } else {
            nous_log_set_level(LOG_LEVEL_NONE);
            printf("\033[33m✗ Debug mode disabled\033[0m\n");
        }
        return 0;
    }

    // Parse level argument
    LogLevel new_level = LOG_LEVEL_NONE;
    const char* level_arg = argv[1];

    if (strcmp(level_arg, "off") == 0 || strcmp(level_arg, "none") == 0) {
        new_level = LOG_LEVEL_NONE;
    } else if (strcmp(level_arg, "error") == 0) {
        new_level = LOG_LEVEL_ERROR;
    } else if (strcmp(level_arg, "warn") == 0 || strcmp(level_arg, "warning") == 0) {
        new_level = LOG_LEVEL_WARN;
    } else if (strcmp(level_arg, "info") == 0) {
        new_level = LOG_LEVEL_INFO;
    } else if (strcmp(level_arg, "debug") == 0) {
        new_level = LOG_LEVEL_DEBUG;
    } else if (strcmp(level_arg, "trace") == 0 || strcmp(level_arg, "all") == 0) {
        new_level = LOG_LEVEL_TRACE;
    } else {
        printf("Unknown debug level: %s\n", level_arg);
        printf("Valid levels: off, error, warn, info, debug, trace\n");
        return -1;
    }

    nous_log_set_level(new_level);

    if (new_level == LOG_LEVEL_NONE) {
        printf("\033[33m✗ Debug mode disabled\033[0m\n");
    } else {
        printf("\033[32m✓ Debug level set to: %s\033[0m\n", nous_log_level_name(new_level));
    }

    return 0;
}

// ============================================================================
// WORKSPACE/SANDBOX COMMANDS
// ============================================================================

static int cmd_allow_dir(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: allow-dir <path>\n");
        printf("Add a directory to the sandbox (allows file operations)\n");
        return -1;
    }

    // Resolve to absolute path
    char resolved[PATH_MAX];
    if (!realpath(argv[1], resolved)) {
        printf("Error: Path not found: %s\n", argv[1]);
        return -1;
    }

    // Check if it's a system path (block for safety)
    const char* blocked_prefixes[] = {
        "/System", "/usr", "/bin", "/sbin", "/etc", "/var",
        "/private/etc", "/private/var", "/Library", NULL
    };

    for (int i = 0; blocked_prefixes[i]; i++) {
        if (strncmp(resolved, blocked_prefixes[i], strlen(blocked_prefixes[i])) == 0) {
            printf("Error: Cannot add system paths for security reasons\n");
            return -1;
        }
    }

    // Add to allowed paths
    tools_add_allowed_path(resolved);
    printf("\033[32m✓ Added to sandbox: %s\033[0m\n", resolved);

    return 0;
}

static int cmd_allowed_dirs(int argc, char** argv) {
    (void)argc; (void)argv;

    size_t count = 0;
    const char** paths = tools_get_allowed_paths(&count);

    printf("\n\033[1mAllowed Directories (Sandbox)\033[0m\n");
    printf("================================\n");

    if (count == 0) {
        printf("  (none - workspace not initialized)\n");
    } else {
        for (size_t i = 0; i < count; i++) {
            if (i == 0) {
                printf("  \033[32m✓\033[0m %s \033[2m(workspace)\033[0m\n", paths[i]);
            } else {
                printf("  \033[32m✓\033[0m %s\n", paths[i]);
            }
        }
    }

    printf("\nUse 'allow-dir <path>' to add more directories.\n\n");

    return 0;
}

// ============================================================================
// AUTHENTICATION COMMANDS
// ============================================================================

// NOTE: OAuth login disabled - requires Anthropic OAuth client registration.
// See docs/adr/005-oauth-authentication.md for details on re-enabling.
// To re-enable: uncomment cmd_login and add to COMMANDS array.
#if 0
static int cmd_login(int argc, char** argv) {
    (void)argc; (void)argv;

    // Check if already authenticated with OAuth
    if (auth_get_mode() == AUTH_MODE_OAUTH) {
        printf("Already logged in with Claude Max.\n");
        char* status = auth_get_status_string();
        if (status) {
            printf("Status: %s\n", status);
            free(status);
        }
        return 0;
    }

    printf("\n\033[1mClaude Max Login\033[0m\n");
    printf("================\n\n");
    printf("This will open your browser to authenticate with Claude Max.\n");
    printf("After logging in, you'll be able to use Convergio without API charges.\n\n");

    int result = auth_oauth_login();

    if (result == 0) {
        printf("\n\033[32m✓ Successfully logged in with Claude Max!\033[0m\n");
        printf("Your tokens have been securely stored in the macOS Keychain.\n\n");
    } else {
        printf("\n\033[31m✗ Login failed.\033[0m\n");
        printf("You can still use Convergio by setting the ANTHROPIC_API_KEY environment variable.\n\n");
    }

    return result;
}
#endif

static int cmd_logout(int argc, char** argv) {
    (void)argc; (void)argv;

    if (auth_get_mode() == AUTH_MODE_NONE) {
        printf("Not currently authenticated.\n");
        return 0;
    }

    AuthMode prev_mode = auth_get_mode();
    auth_logout();

    if (prev_mode == AUTH_MODE_OAUTH) {
        printf("\033[32m✓ Logged out from Claude Max.\033[0m\n");
        printf("OAuth tokens have been removed from Keychain.\n");
    }

    // Check if API key is available as fallback
    if (auth_get_mode() == AUTH_MODE_API_KEY) {
        printf("\nNow using API key authentication (ANTHROPIC_API_KEY).\n");
    } else {
        printf("\nNo authentication configured.\n");
        printf("Run 'convergio setup' or set ANTHROPIC_API_KEY environment variable.\n");
    }

    return 0;
}

static int cmd_auth(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("\n\033[1mAuthentication Status\033[0m\n");
    printf("=====================\n\n");

    char* status = auth_get_status_string();
    if (status) {
        AuthMode mode = auth_get_mode();
        const char* mode_name;
        switch (mode) {
            case AUTH_MODE_API_KEY: mode_name = "API Key"; break;
            case AUTH_MODE_OAUTH:   mode_name = "Claude Max (OAuth)"; break;
            default:                mode_name = "None"; break;
        }

        printf("  Mode:   %s\n", mode_name);
        printf("  Status: %s\n", status);

        if (mode == AUTH_MODE_OAUTH) {
            printf("\n  \033[2mTokens stored in macOS Keychain\033[0m\n");
        } else if (mode == AUTH_MODE_API_KEY) {
            printf("\n  \033[2mUsing ANTHROPIC_API_KEY environment variable\033[0m\n");
        }

        free(status);
    } else {
        printf("  Not authenticated\n");
    }

    printf("\n");
    return 0;
}

static int cmd_update(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "install") == 0) {
        return convergio_cmd_update_install();
    }
    if (argc >= 2 && strcmp(argv[1], "changelog") == 0) {
        return convergio_cmd_update_changelog();
    }
    // Default: check for updates
    return convergio_cmd_update_check();
}

static int cmd_hardware(int argc, char** argv) {
    (void)argc; (void)argv;
    convergio_print_hardware_info();
    return 0;
}

static int cmd_create(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: create <essence>\n");
        printf("Example: create \"un concetto di bellezza\"\n");
        return -1;
    }

    // Join all arguments as the essence (with bounds checking)
    char essence[1024] = {0};
    size_t ess_len = 0;
    for (int i = 1; i < argc && ess_len < sizeof(essence) - 2; i++) {
        if (i > 1 && ess_len < sizeof(essence) - 1) {
            essence[ess_len++] = ' ';
        }
        size_t arg_len = strlen(argv[i]);
        size_t copy_len = (ess_len + arg_len < sizeof(essence) - 1) ? arg_len : (sizeof(essence) - 1 - ess_len);
        memcpy(essence + ess_len, argv[i], copy_len);
        ess_len += copy_len;
    }
    essence[ess_len] = '\0';

    SemanticID id = nous_create_node(SEMANTIC_TYPE_CONCEPT, essence);
    if (id == SEMANTIC_ID_NULL) {
        printf("Failed to create node.\n");
        return -1;
    }

    printf("Created semantic node: 0x%016llx\n", (unsigned long long)id);
    printf("Essence: \"%s\"\n", essence);

    return 0;
}

static int cmd_agent(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: agent <create|list|skill> [args]\n");
        return -1;
    }

    if (strcmp(argv[1], "create") == 0) {
        if (argc < 4) {
            printf("Usage: agent create <name> <essence>\n");
            return -1;
        }

        char essence[512] = {0};
        size_t ess_len = 0;
        for (int i = 3; i < argc && ess_len < sizeof(essence) - 2; i++) {
            if (i > 3 && ess_len < sizeof(essence) - 1) {
                essence[ess_len++] = ' ';
            }
            size_t arg_len = strlen(argv[i]);
            size_t copy_len = (ess_len + arg_len < sizeof(essence) - 1) ? arg_len : (sizeof(essence) - 1 - ess_len);
            memcpy(essence + ess_len, argv[i], copy_len);
            ess_len += copy_len;
        }
        essence[ess_len] = '\0';

        NousAgent* agent = nous_create_agent(argv[2], essence);
        if (!agent) {
            printf("Failed to create agent.\n");
            return -1;
        }

        printf("Created agent \"%s\"\n", agent->name);
        printf("  Patience: %.2f\n", agent->patience);
        printf("  Creativity: %.2f\n", agent->creativity);
        printf("  Assertiveness: %.2f\n", agent->assertiveness);

        // Make this the assistant if none exists
        if (!g_assistant) {
            g_assistant = agent;
            printf("Set as primary assistant.\n");
        }

        return 0;
    }

    if (strcmp(argv[1], "skill") == 0) {
        if (argc < 3 || !g_assistant) {
            printf("Usage: agent skill <skill_name>\n");
            return -1;
        }

        if (nous_agent_add_skill(g_assistant, argv[2]) == 0) {
            printf("Added skill \"%s\" to %s\n", argv[2], g_assistant->name);
        }
        return 0;
    }

    printf("Unknown agent command: %s\n", argv[1]);
    return -1;
}

static int cmd_space(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: space <create|join|leave|list> [args]\n");
        return -1;
    }

    if (strcmp(argv[1], "create") == 0) {
        if (argc < 4) {
            printf("Usage: space create <name> <purpose>\n");
            return -1;
        }

        char purpose[512] = {0};
        size_t purp_len = 0;
        for (int i = 3; i < argc && purp_len < sizeof(purpose) - 2; i++) {
            if (i > 3 && purp_len < sizeof(purpose) - 1) {
                purpose[purp_len++] = ' ';
            }
            size_t arg_len = strlen(argv[i]);
            size_t copy_len = (purp_len + arg_len < sizeof(purpose) - 1) ? arg_len : (sizeof(purpose) - 1 - purp_len);
            memcpy(purpose + purp_len, argv[i], copy_len);
            purp_len += copy_len;
        }
        purpose[purp_len] = '\0';

        NousSpace* space = nous_create_space(argv[2], purpose);
        if (!space) {
            printf("Failed to create space.\n");
            return -1;
        }

        printf("Created space \"%s\"\n", space->name);
        printf("  Purpose: %s\n", space->purpose);

        g_current_space = space;
        printf("Entered space.\n");

        // Auto-join assistant if exists
        if (g_assistant) {
            nous_join_space(g_assistant->id, space->id);
            printf("Assistant joined space.\n");
        }

        return 0;
    }

    if (strcmp(argv[1], "urgency") == 0 && g_current_space) {
        printf("Current urgency: %.2f\n", nous_space_urgency(g_current_space));
        return 0;
    }

    printf("Unknown space command: %s\n", argv[1]);
    return -1;
}

static int cmd_status(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("\n=== NOUS System Status ===\n\n");

    // Kernel status
    printf("Kernel: %s\n", nous_is_ready() ? "READY" : "NOT READY");

    // Current space
    if (g_current_space) {
        printf("\nCurrent Space: %s\n", g_current_space->name);
        printf("  Purpose: %s\n", g_current_space->purpose);
        printf("  Participants: %zu\n", nous_space_participant_count(g_current_space));
        printf("  Urgency: %.2f\n", nous_space_urgency(g_current_space));
        printf("  Active: %s\n", nous_space_is_active(g_current_space) ? "Yes" : "No");
    } else {
        printf("\nNo active space.\n");
    }

    // Assistant
    if (g_assistant) {
        printf("\nAssistant: %s\n", g_assistant->name);
        printf("  State: %d\n", g_assistant->state);
        printf("  Skills: %zu\n", g_assistant->skill_count);
    }

    printf("\n");

    // GPU stats
    nous_gpu_print_stats();

    // Scheduler metrics
    nous_scheduler_print_metrics();

    printf("\n");
    return 0;
}

static int cmd_quit(int argc, char** argv) {
    (void)argc; (void)argv;
    g_running = 0;
    return 0;
}

static void on_thought(NousAgent* agent, const char* thought, void* ctx) {
    (void)ctx;
    printf("\n%s: %s\n\n", agent->name, thought);
}

static int cmd_think(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: think <intent>\n");
        return -1;
    }

    if (!g_assistant) {
        printf("No assistant available. Create one with: agent create <name> <essence>\n");
        return -1;
    }

    // Join all arguments as intent (with bounds checking)
    char input[1024] = {0};
    size_t input_len = 0;
    for (int i = 1; i < argc && input_len < sizeof(input) - 2; i++) {
        if (i > 1 && input_len < sizeof(input) - 1) {
            input[input_len++] = ' ';
        }
        size_t arg_len = strlen(argv[i]);
        size_t copy_len = (input_len + arg_len < sizeof(input) - 1) ? arg_len : (sizeof(input) - 1 - input_len);
        memcpy(input + input_len, argv[i], copy_len);
        input_len += copy_len;
    }
    input[input_len] = '\0';

    // Parse intent
    ParsedIntent* intent = nous_parse_intent(input, strlen(input));
    if (!intent) {
        printf("Failed to parse intent.\n");
        return -1;
    }

    printf("Intent parsed:\n");
    printf("  Kind: %d\n", intent->kind);
    printf("  Confidence: %.2f\n", intent->confidence);
    printf("  Urgency: %.2f\n", intent->urgency);

    if (intent->question_count > 0) {
        printf("\nClarification needed:\n");
        for (size_t i = 0; i < intent->question_count; i++) {
            printf("  - %s\n", intent->questions[i]);
        }
    }

    // Have assistant think about it
    extern int nous_agent_think(NousAgent*, ParsedIntent*,
                                void (*)(NousAgent*, const char*, void*), void*);
    nous_agent_think(g_assistant, intent, on_thought, NULL);

    return 0;
}

// ============================================================================
// MARKDOWN TO ANSI RENDERING
// ============================================================================

extern char* md_to_ansi(const char* markdown);
extern void md_print(const char* markdown);

// ============================================================================
// UI HELPERS - Spinner, separators, clearing
// ============================================================================

// ANSI escape codes for cursor control
#define ANSI_CLEAR_LINE    "\033[2K"
#define ANSI_CURSOR_UP     "\033[1A"
#define ANSI_CURSOR_START  "\r"
#define ANSI_HIDE_CURSOR   "\033[?25l"
#define ANSI_SHOW_CURSOR   "\033[?25h"
#define ANSI_DIM           "\033[2m"
#define ANSI_RESET         "\033[0m"
#define ANSI_CYAN          "\033[36m"
#define ANSI_BOLD          "\033[1m"

// Spinner state
static volatile bool g_spinner_active = false;
static volatile bool g_spinner_cancelled = false;
static pthread_t g_spinner_thread;
static struct termios g_orig_termios;

// Print a visual separator
static void print_separator(void) {
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

// Start spinner
static void spinner_start(void) {
    g_spinner_active = true;
    g_spinner_cancelled = false;
    claude_reset_cancel();
    pthread_create(&g_spinner_thread, NULL, spinner_func, NULL);
}

// Stop spinner
static void spinner_stop(void) {
    if (g_spinner_active) {
        g_spinner_active = false;
        pthread_join(g_spinner_thread, NULL);
    }
}

// Check if request was cancelled
static bool spinner_was_cancelled(void) {
    return g_spinner_cancelled;
}

// ============================================================================
// NATURAL LANGUAGE PROCESSING - Via Ali Orchestrator
// ============================================================================

// Streaming buffer for accumulating response before rendering
static char* g_stream_buffer = NULL;
static size_t g_stream_size = 0;
static size_t g_stream_capacity = 0;

// Streaming callback - accumulates chunks for final rendering (reserved for streaming feature)
__attribute__((unused))
static void stream_print_callback(const char* chunk, void* user_data) {
    (void)user_data;
    size_t chunk_len = strlen(chunk);

    if (g_stream_size + chunk_len >= g_stream_capacity) {
        g_stream_capacity = (g_stream_capacity + chunk_len) * 2 + 1024;
        g_stream_buffer = realloc(g_stream_buffer, g_stream_capacity);
    }

    if (g_stream_buffer) {
        memcpy(g_stream_buffer + g_stream_size, chunk, chunk_len);
        g_stream_size += chunk_len;
        g_stream_buffer[g_stream_size] = '\0';
    }

    // Also print raw for immediate feedback (will be replaced with progress indicator)
    printf("%s", chunk);
    fflush(stdout);
}

// External streaming function (reserved for future streaming feature)
extern char* nous_claude_chat_stream(const char* system_prompt, const char* user_message,
                                      void (*callback)(const char*, void*), void* user_data);

static int process_natural_input(const char* input) {
    if (!input || strlen(input) == 0) return 0;

    Orchestrator* orch = orchestrator_get();
    if (!orch || !orch->initialized) {
        // Fallback to old Aria if orchestrator not ready
        if (g_assistant) {
            char* response = nous_agent_think_with_claude(g_assistant, input);
            if (response) {
                printf("\n%s: %s\n\n", g_assistant->name, response);
                free(response);
            }
        } else {
            printf("System not ready. Try 'help' for commands.\n");
        }
        return 0;
    }

    // Print separator between input and output
    print_separator();

    // Get Ali's name
    const char* name = orch->ali ? orch->ali->name : "Ali";

    // Start spinner while waiting for response
    spinner_start();

    char* response = NULL;

    // Always use orchestrator - it handles both simple chat and tool calls
    response = orchestrator_process(input);

    // Stop spinner
    spinner_stop();

    // Check if request was cancelled by user
    if (spinner_was_cancelled()) {
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

    printf("\n");
    return 0;
}

// ============================================================================
// DIRECT AGENT COMMUNICATION
// ============================================================================

// Talk directly to a specific agent by name, bypassing Ali
static int direct_agent_communication(const char* agent_name, const char* message) {
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
    print_separator();

    // Start spinner
    spinner_start();

    // Use orchestrator_agent_chat for full tool support (web_fetch, file_read, etc.)
    char* response = orchestrator_agent_chat(agent, message);

    // Stop spinner
    spinner_stop();

    // Check if cancelled
    if (spinner_was_cancelled()) {
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
// COMMAND PARSING
// ============================================================================

static int parse_and_execute(char* line) {
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
            return direct_agent_communication(agent_name, msg_start);
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

    for (const ReplCommand* cmd = COMMANDS; cmd->name != NULL; cmd++) {
        if (strcmp(cmd_name, cmd->name) == 0) {
            return cmd->handler(argc, argv);
        }
    }

    // Not a command, treat as natural language intent
    // Reconstruct the original line
    char* original = strdup(rl_line_buffer);
    int result = process_natural_input(original);
    free(original);
    return result;
}

// ============================================================================
// MAIN
// ============================================================================

// Print a single UTF-8 character with color
static void print_colored_char(const char* ch, int len, int col, int total_cols) {
    // Gradient: cyan (#51) -> blue (#39) -> purple (#135) -> pink (#168)
    // Map column position to color
    float t = (float)col / (float)total_cols;

    int color;
    if (t < 0.25f) {
        color = 81;  // Light cyan
    } else if (t < 0.40f) {
        color = 75;  // Cyan-blue
    } else if (t < 0.55f) {
        color = 69;  // Blue
    } else if (t < 0.70f) {
        color = 105; // Blue-purple
    } else if (t < 0.85f) {
        color = 141; // Purple
    } else {
        color = 175; // Pink
    }

    printf("\033[1m\033[38;5;%dm%.*s\033[0m", color, len, ch);
}

// Print a line with horizontal gradient
static void print_gradient_line(const char* line) {
    int col = 0;
    int total_cols = 0;

    // First pass: count visible columns (skip spaces for color calculation)
    const char* p = line;
    while (*p) {
        if ((unsigned char)*p >= 0x80) {
            // UTF-8 multibyte
            if ((*p & 0xE0) == 0xC0) p += 2;
            else if ((*p & 0xF0) == 0xE0) p += 3;
            else if ((*p & 0xF8) == 0xF0) p += 4;
            else p++;
        } else {
            p++;
        }
        total_cols++;
    }

    // Second pass: print with colors
    p = line;
    col = 0;
    while (*p) {
        int char_len = 1;
        if ((unsigned char)*p >= 0x80) {
            if ((*p & 0xE0) == 0xC0) char_len = 2;
            else if ((*p & 0xF0) == 0xE0) char_len = 3;
            else if ((*p & 0xF8) == 0xF0) char_len = 4;
        }

        if (*p == ' ') {
            printf(" ");
        } else {
            print_colored_char(p, char_len, col, total_cols);
        }

        p += char_len;
        col++;
    }
    printf("\n");
}

static void print_banner(void) {
    const char* rst = "\033[0m";
    const char* dim = "\033[2m";
    const char* c3 = "\033[38;5;75m";

    printf("\n");
    print_gradient_line("   ██████╗ ██████╗ ███╗   ██╗██╗   ██╗███████╗██████╗  ██████╗ ██╗ ██████╗ ");
    print_gradient_line("  ██╔════╝██╔═══██╗████╗  ██║██║   ██║██╔════╝██╔══██╗██╔════╝ ██║██╔═══██╗");
    print_gradient_line("  ██║     ██║   ██║██╔██╗ ██║██║   ██║█████╗  ██████╔╝██║  ███╗██║██║   ██║");
    print_gradient_line("  ██║     ██║   ██║██║╚██╗██║╚██╗ ██╔╝██╔══╝  ██╔══██╗██║   ██║██║██║   ██║");
    print_gradient_line("  ╚██████╗╚██████╔╝██║ ╚████║ ╚████╔╝ ███████╗██║  ██║╚██████╔╝██║╚██████╔╝");
    print_gradient_line("   ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝ ╚═════╝ ");
    printf("\n");
    print_gradient_line("                          K E R N E L");
    printf("\n");
    printf("  %sA semantic kernel for human-AI symbiosis%s\n", dim, rst);
    printf("  %sv%s - Optimized for Apple Silicon%s\n", dim, convergio_get_version(), rst);
    printf("  %sDeveloped by Roberdan@FightTheStroke.org%s\n", dim, rst);
    printf("\n");
    printf("  Type %s'help'%s for commands, or express your intent naturally.\n", c3, rst);
    printf("\n");
}

int main(int argc, char** argv) {
    // Workspace path (default: current directory)
    char workspace[PATH_MAX];
    getcwd(workspace, sizeof(workspace));

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-d") == 0) {
            nous_log_set_level(LOG_LEVEL_DEBUG);
        } else if (strcmp(argv[i], "--trace") == 0 || strcmp(argv[i], "-t") == 0) {
            nous_log_set_level(LOG_LEVEL_TRACE);
        } else if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0) {
            nous_log_set_level(LOG_LEVEL_ERROR);
        } else if ((strcmp(argv[i], "--workspace") == 0 || strcmp(argv[i], "-w") == 0) && i + 1 < argc) {
            // Custom workspace path
            char resolved[PATH_MAX];
            if (realpath(argv[++i], resolved)) {
                strncpy(workspace, resolved, sizeof(workspace) - 1);
                workspace[sizeof(workspace) - 1] = '\0';
            } else {
                fprintf(stderr, "Error: Cannot access workspace path: %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Convergio - A semantic kernel for human-AI symbiosis\n\n");
            printf("Usage: convergio [OPTIONS] [COMMAND]\n\n");
            printf("Commands:\n");
            printf("  setup                   Configure API key and settings\n");
            printf("  update [check|install]  Check for or install updates\n\n");
            printf("Options:\n");
            printf("  -w, --workspace <path>  Set workspace directory (default: current dir)\n");
            printf("  -d, --debug             Enable debug logging\n");
            printf("  -t, --trace             Enable trace logging (verbose)\n");
            printf("  -q, --quiet             Suppress non-error output\n");
            printf("  -v, --version           Show version\n");
            printf("  -h, --help              Show this help message\n");
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("Convergio %s\n", convergio_get_version());
            return 0;
        } else if (strcmp(argv[i], "setup") == 0) {
            convergio_config_init();
            return convergio_setup_wizard();
        } else if (strcmp(argv[i], "update") == 0) {
            if (i + 1 < argc && strcmp(argv[i + 1], "install") == 0) {
                return convergio_cmd_update_install();
            }
            return convergio_cmd_update_check();
        }
    }

    // Setup signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    print_banner();

    // Show debug mode if enabled
    if (g_log_level != LOG_LEVEL_NONE) {
        printf("  \033[33m⚡ Debug mode: %s\033[0m\n\n", nous_log_level_name(g_log_level));
    }

    // Initialize subsystems
    printf("Initializing Convergio Kernel...\n");

    // Initialize configuration first
    if (convergio_config_init() != 0) {
        fprintf(stderr, "Warning: Failed to initialize configuration.\n");
    } else {
        printf("  ✓ Configuration (~/.convergio/)\n");
    }

    // Detect hardware
    if (convergio_detect_hardware() != 0) {
        fprintf(stderr, "Warning: Failed to detect hardware.\n");
    } else {
        printf("  ✓ Hardware: %s\n", g_hardware.chip_name);
    }

    // Initialize authentication
    if (auth_init() != 0) {
        printf("  \033[33m⚠ No authentication configured\033[0m\n");
        printf("    Run 'convergio setup' or set ANTHROPIC_API_KEY\n");
    } else {
        char* auth_status = auth_get_status_string();
        printf("  ✓ Authentication: %s\n", auth_status ? auth_status : "configured");
        free(auth_status);
    }

    if (nous_init() != 0) {
        fprintf(stderr, "Failed to initialize semantic fabric.\n");
        return 1;
    }
    printf("  ✓ Semantic Fabric\n");

    if (nous_scheduler_init() != 0) {
        fprintf(stderr, "Failed to initialize scheduler.\n");
        nous_shutdown();
        return 1;
    }
    printf("  ✓ Scheduler (P-cores: %d, E-cores: %d)\n",
           g_hardware.p_cores, g_hardware.e_cores);

    if (nous_gpu_init() != 0) {
        fprintf(stderr, "Warning: GPU initialization failed, using CPU fallback.\n");
    } else {
        printf("  ✓ GPU (%d cores)\n", g_hardware.gpu_cores);
    }

    // Initialize Orchestrator with Ali
    if (orchestrator_init(DEFAULT_BUDGET_USD) != 0) {
        fprintf(stderr, "Warning: Orchestrator initialization failed.\n");
    } else {
        printf("  ✓ Orchestrator (Ali - Chief of Staff)\n");
        printf("  ✓ Budget: $%.2f\n", DEFAULT_BUDGET_USD);
    }

    // Initialize workspace sandbox
    tools_init_workspace(workspace);
    printf("  ✓ Workspace: %s\n", workspace);

    printf("\nConvergio is ready.\n");
    printf("Talk to Ali - your Chief of Staff will coordinate specialist agents.\n\n");

    // Create fallback assistant (only used if orchestrator fails)
    g_assistant = nous_create_agent("Aria", "assistente creativo e collaborativo");
    if (g_assistant) {
        nous_agent_add_skill(g_assistant, "programmazione");
        nous_agent_add_skill(g_assistant, "analisi");
        nous_agent_add_skill(g_assistant, "creatività");
    }

    // REPL with cost in prompt
    char prompt[256];
    char* line;

    using_history();

    while (g_running) {
        // Fancy prompt with colored Convergio and arrow
        // \033[38;5;39m = bright blue color
        // \033[1m = bold
        // \033[0m = reset
        snprintf(prompt, sizeof(prompt),
            "\033[38;5;39m\033[1mConvergio\033[0m \033[38;5;39m❯\033[0m ");

        line = readline(prompt);

        if (!line) {
            // EOF
            break;
        }

        // Add to history if non-empty
        if (strlen(line) > 0) {
            add_history(line);
            parse_and_execute(line);
        }

        free(line);
    }

    // Cleanup
    printf("\nShutting down Convergio...\n");

    // Show final cost report
    char* final_report = cost_get_report();
    if (final_report) {
        printf("%s", final_report);
        free(final_report);
    }

    // Shutdown orchestrator
    orchestrator_shutdown();

    if (g_assistant) {
        nous_destroy_agent(g_assistant);
    }

    nous_gpu_shutdown();
    nous_scheduler_shutdown();
    nous_shutdown();
    auth_shutdown();
    convergio_config_shutdown();

    printf("Goodbye.\n");
    return 0;
}
