/**
 * CONVERGIO KERNEL
 *
 * Main entry point and CLI interface
 * A semantic kernel for human-AI symbiosis
 * With Ali as Chief of Staff orchestrating all agents
 */

#include "nous/nous.h"
#include "nous/orchestrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

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
    printf("\n");
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

static const ReplCommand COMMANDS[] = {
    {"help",    "Show available commands",           cmd_help},
    {"create",  "Create a semantic node",            cmd_create},
    {"agent",   "Manage agents",                     cmd_agent},
    {"agents",  "List all available agents",         cmd_agents},
    {"space",   "Manage collaborative spaces",       cmd_space},
    {"status",  "Show system status",                cmd_status},
    {"cost",    "Show/set cost and budget",          cmd_cost},
    {"think",   "Process an intent",                 cmd_think},
    {"quit",    "Exit Convergio",                    cmd_quit},
    {"exit",    "Exit Convergio",                    cmd_quit},
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
    (void)argc; (void)argv;

    char* status = agent_registry_status();
    if (status) {
        printf("%s", status);
        free(status);
    }
    return 0;
}

static int cmd_create(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: create <essence>\n");
        printf("Example: create \"un concetto di bellezza\"\n");
        return -1;
    }

    // Join all arguments as the essence
    char essence[1024] = {0};
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(essence, " ");
        strcat(essence, argv[i]);
    }

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
        for (int i = 3; i < argc; i++) {
            if (i > 3) strcat(essence, " ");
            strcat(essence, argv[i]);
        }

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
        for (int i = 3; i < argc; i++) {
            if (i > 3) strcat(purpose, " ");
            strcat(purpose, argv[i]);
        }

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

    // Join all arguments as intent
    char input[1024] = {0};
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(input, " ");
        strcat(input, argv[i]);
    }

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
// NATURAL LANGUAGE PROCESSING - Via Ali Orchestrator
// ============================================================================

// Streaming callback - prints chunks in real-time
static void stream_print_callback(const char* chunk, void* user_data) {
    (void)user_data;
    printf("%s", chunk);
    fflush(stdout);  // Force immediate output
}

// Check if streaming mode is enabled (default: yes)
static bool g_streaming_enabled = true;

// External streaming function
extern char* nous_claude_chat_stream(const char* system_prompt, const char* user_message,
                                      void (*callback)(const char*, void*), void* user_data);

// Process with streaming (for simple queries without tools)
static char* process_with_streaming(const char* system_prompt, const char* input) {
    return nous_claude_chat_stream(system_prompt, input, stream_print_callback, NULL);
}

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

    // Get Ali's name
    const char* name = orch->ali ? orch->ali->name : "Ali";

    printf("\n%s: ", name);
    fflush(stdout);

    // Detect if this needs tools (complex request) or can stream directly (simple chat)
    // Simple heuristic: if request contains file/shell/web keywords, use tools (non-streaming)
    bool needs_tools = false;
    const char* tool_keywords[] = {"leggi", "scrivi", "file", "esegui", "comando", "shell",
                                   "fetch", "url", "http", "ricorda", "memoria", "cerca",
                                   "read", "write", "execute", "run", "search", "memory", NULL};

    for (int i = 0; tool_keywords[i] != NULL; i++) {
        if (strcasestr(input, tool_keywords[i]) != NULL) {
            needs_tools = true;
            break;
        }
    }

    char* response = NULL;

    if (g_streaming_enabled && !needs_tools && orch->ali && orch->ali->system_prompt) {
        // Use streaming for simple chat (faster perceived response)
        response = process_with_streaming(orch->ali->system_prompt, input);
        printf("\n");  // Add newline after streamed response

        // Record cost
        if (response && orch->ali) {
            extern void cost_record_agent_usage(ManagedAgent* agent, uint64_t input, uint64_t output);
            cost_record_agent_usage(orch->ali,
                strlen(orch->ali->system_prompt) / 4 + strlen(input) / 4,
                strlen(response) / 4);
        }
    } else {
        // Use full orchestrator with tools
        response = orchestrator_process(input);
        if (response) {
            printf("%s\n", response);
        } else {
            printf("Mi dispiace, ho avuto un problema. Riprova.\n");
        }
    }

    if (response) {
        free(response);
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

    // Tokenize
    char* argv[64];
    int argc = 0;

    char* token = strtok(line, " \t");
    while (token && argc < 64) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }

    if (argc == 0) return 0;

    // Look for built-in command
    for (const ReplCommand* cmd = COMMANDS; cmd->name != NULL; cmd++) {
        if (strcmp(argv[0], cmd->name) == 0) {
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

static void print_banner(void) {
    printf("\n");
    printf("   ██████╗ ██████╗ ███╗   ██╗██╗   ██╗███████╗██████╗  ██████╗ ██╗ ██████╗ \n");
    printf("  ██╔════╝██╔═══██╗████╗  ██║██║   ██║██╔════╝██╔══██╗██╔════╝ ██║██╔═══██╗\n");
    printf("  ██║     ██║   ██║██╔██╗ ██║██║   ██║█████╗  ██████╔╝██║  ███╗██║██║   ██║\n");
    printf("  ██║     ██║   ██║██║╚██╗██║╚██╗ ██╔╝██╔══╝  ██╔══██╗██║   ██║██║██║   ██║\n");
    printf("  ╚██████╗╚██████╔╝██║ ╚████║ ╚████╔╝ ███████╗██║  ██║╚██████╔╝██║╚██████╔╝\n");
    printf("   ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝  ╚═══╝  ╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝ ╚═════╝ \n");
    printf("\n");
    printf("                         K E R N E L\n");
    printf("\n");
    printf("  A semantic kernel for human-AI symbiosis\n");
    printf("  Optimized for Apple M3 Max (10P + 4E + 30GPU + 16NE)\n");
    printf("\n");
    printf("  Type 'help' for commands, or express your intent naturally.\n");
    printf("\n");
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    // Setup signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    print_banner();

    // Initialize subsystems
    printf("Initializing Convergio Kernel...\n");

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
    printf("  ✓ Scheduler (P-cores: 10, E-cores: 4)\n");

    if (nous_gpu_init() != 0) {
        fprintf(stderr, "Warning: GPU initialization failed, using CPU fallback.\n");
    } else {
        printf("  ✓ GPU (30 cores, Metal 4)\n");
    }

    // Initialize Orchestrator with Ali
    if (orchestrator_init(DEFAULT_BUDGET_USD) != 0) {
        fprintf(stderr, "Warning: Orchestrator initialization failed.\n");
    } else {
        printf("  ✓ Orchestrator (Ali - Chief of Staff)\n");
        printf("  ✓ Budget: $%.2f\n", DEFAULT_BUDGET_USD);
    }

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
    char prompt[128];
    char* line;

    using_history();

    while (g_running) {
        // Simple prompt without cost (use 'cost' command to see spending)
        snprintf(prompt, sizeof(prompt), "convergio> ");

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

    printf("Goodbye.\n");
    return 0;
}
