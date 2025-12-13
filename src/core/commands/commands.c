/**
 * CONVERGIO KERNEL - Command Implementations
 *
 * All REPL command handlers
 */

#include "nous/commands.h"
#include "nous/nous.h"
#include "nous/orchestrator.h"
#include "nous/tools.h"
#include "nous/config.h"
#include "nous/hardware.h"
#include "nous/updater.h"
#include "nous/theme.h"
#include "nous/compare.h"
#include "nous/telemetry.h"
#include "nous/agentic.h"
#include "../../auth/oauth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>

// ============================================================================
// EXTERNAL DECLARATIONS
// ============================================================================

extern int nous_gpu_init(void);
extern void nous_gpu_shutdown(void);
extern void nous_gpu_print_stats(void);
extern int nous_scheduler_init(void);
extern void nous_scheduler_shutdown(void);
extern void nous_scheduler_print_metrics(void);

// ============================================================================
// COMMAND TABLE
// ============================================================================

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
    {"stream",      "Toggle streaming mode (on/off)",    cmd_stream},
    {"theme",       "Change color theme (ocean/forest/sunset/mono)", cmd_theme},
    {"think",       "Process an intent",                 cmd_think},
    {"compare",     "Compare models side-by-side",       cmd_compare},
    {"benchmark",   "Benchmark a model's performance",   cmd_benchmark},
    {"telemetry",   "Manage telemetry settings",         cmd_telemetry},
    {"tools",       "Manage development tools",          cmd_tools},
    {"news",        "Show release notes",                cmd_news},
    {"quit",        "Exit Convergio",                    cmd_quit},
    {"exit",        "Exit Convergio",                    cmd_quit},
    {NULL, NULL, NULL}
};

const ReplCommand* commands_get_table(void) {
    return COMMANDS;
}

// ============================================================================
// DETAILED HELP SYSTEM
// ============================================================================

typedef struct {
    const char* name;
    const char* usage;
    const char* description;
    const char* details;
    const char* examples;
} CommandHelp;

static const CommandHelp DETAILED_HELP[] = {
    {
        "help",
        "help [command]",
        "Display help information",
        "Without arguments, shows all available commands.\n"
        "With a command name, shows detailed help for that command.",
        "help           # Show all commands\n"
        "help create    # Detailed help for 'create'\n"
        "help agent     # Detailed help for 'agent'"
    },
    {
        "create",
        "create <essence>",
        "Create a semantic node in the knowledge graph",
        "Creates a new semantic node with the given essence (description).\n"
        "The essence defines the concept or entity being created.\n"
        "Returns a unique semantic ID for the created node.",
        "create \"un concetto di bellezza\"\n"
        "create \"progetto di machine learning\"\n"
        "create sistema di autenticazione OAuth"
    },
    {
        "agent",
        "agent <subcommand> [args]",
        "Manage agents in the system",
        "Subcommands:\n"
        "  list                    List all available agents\n"
        "  info <name>             Show detailed info about an agent\n"
        "  create <name> <desc>    Create a new dynamic agent\n"
        "  skill <skill_name>      Add a skill to the current assistant\n\n"
        "Use @<agent_name> <message> to communicate directly with an agent.",
        "agent list\n"
        "agent info baccio\n"
        "agent create helper \"Un assistente per task generici\"\n"
        "agent skill programmazione"
    },
    {
        "agents",
        "agents [working|active]",
        "List all available agents",
        "Without arguments, shows all agents in the registry with their status.\n"
        "With 'working' or 'active', shows only currently active agents.\n"
        "Displays agent roles, states, and current tasks.",
        "agents           # Show all agents\n"
        "agents working   # Show only working agents\n"
        "agents active    # Same as 'agents working'"
    },
    {
        "space",
        "space <create|join|leave|list|urgency> [args]",
        "Manage collaborative spaces",
        "Spaces are collaborative environments where agents can work together.\n\n"
        "Subcommands:\n"
        "  create <name> <purpose>   Create a new space\n"
        "  urgency                   Show current space urgency level",
        "space create project \"Sviluppo nuova feature\"\n"
        "space urgency"
    },
    {
        "status",
        "status",
        "Show comprehensive system status",
        "Displays:\n"
        "  - Kernel status (ready/not ready)\n"
        "  - Current space information\n"
        "  - Active assistant details\n"
        "  - GPU statistics\n"
        "  - Scheduler metrics",
        "status"
    },
    {
        "cost",
        "cost [report|set <amount>|reset]",
        "Manage cost tracking and budget",
        "Subcommands:\n"
        "  (none)              Show current session spending\n"
        "  report              Show detailed cost breakdown by model\n"
        "  set <amount_usd>    Set a budget limit (stops when reached)\n"
        "  reset               Reset session spending to zero\n\n"
        "Cost tracking includes all API calls with token counts and pricing.",
        "cost              # Quick status\n"
        "cost report       # Detailed breakdown\n"
        "cost set 10.00    # Set $10 budget\n"
        "cost reset        # Reset counters"
    },
    {
        "debug",
        "debug [off|error|warn|info|debug|trace]",
        "Toggle or set debug output level",
        "Without arguments, toggles between OFF and INFO level.\n"
        "With a level argument, sets that specific level.\n\n"
        "Levels (from least to most verbose):\n"
        "  off/none    No debug output\n"
        "  error       Only errors\n"
        "  warn        Errors and warnings\n"
        "  info        General information\n"
        "  debug       Detailed debug info\n"
        "  trace/all   Everything including low-level traces",
        "debug          # Toggle debug mode\n"
        "debug info     # Set to INFO level\n"
        "debug trace    # Enable all logging\n"
        "debug off      # Disable debug output"
    },
    {
        "allow-dir",
        "allow-dir <path>",
        "Add a directory to the sandbox",
        "Adds a directory to the list of allowed paths for file operations.\n"
        "This is required for agents to read/write files outside the workspace.\n"
        "System directories (/usr, /etc, etc.) are blocked for security.\n"
        "Paths are resolved to absolute paths automatically.",
        "allow-dir ~/Documents/project\n"
        "allow-dir /Users/me/data\n"
        "allow-dir ../other-project"
    },
    {
        "allowed-dirs",
        "allowed-dirs",
        "Show allowed directories (sandbox)",
        "Lists all directories where file operations are permitted.\n"
        "The first entry is always the current workspace.\n"
        "Additional directories can be added with 'allow-dir'.",
        "allowed-dirs"
    },
    {
        "logout",
        "logout",
        "Logout and clear credentials",
        "Logs out from the current authentication method.\n"
        "For OAuth (Claude Max): removes tokens from Keychain.\n"
        "Falls back to API key if ANTHROPIC_API_KEY is set.",
        "logout"
    },
    {
        "auth",
        "auth",
        "Show authentication status",
        "Displays current authentication method and status:\n"
        "  - API Key: Using ANTHROPIC_API_KEY environment variable\n"
        "  - OAuth: Using Claude Max subscription (tokens in Keychain)\n"
        "  - None: Not authenticated",
        "auth"
    },
    {
        "update",
        "update [install|changelog]",
        "Check for and install updates",
        "Subcommands:\n"
        "  (none)       Check if updates are available\n"
        "  install      Download and install the latest version\n"
        "  changelog    Show recent changes and release notes\n\n"
        "Updates are fetched from GitHub releases or Homebrew.",
        "update            # Check for updates\n"
        "update install    # Install latest version\n"
        "update changelog  # View release notes"
    },
    {
        "hardware",
        "hardware",
        "Show hardware information",
        "Displays detailed hardware information including:\n"
        "  - CPU model and core count\n"
        "  - Memory (RAM) total and available\n"
        "  - GPU information (Metal support)\n"
        "  - Neural Engine availability",
        "hardware"
    },
    {
        "news",
        "news [version]",
        "Show release notes for Convergio",
        "Displays the release notes and changelog for a specific version.\n"
        "Without arguments, shows the latest release notes.\n\n"
        "You can specify a version number with or without the 'v' prefix.",
        "news           # Show latest release notes\n"
        "news 3.0.4     # Show notes for v3.0.4\n"
        "news v3.0.3    # Also works with 'v' prefix"
    },
    {
        "stream",
        "stream [on|off]",
        "Toggle streaming mode",
        "Controls whether AI responses stream in real-time.\n\n"
        "ON:  Responses appear as they're generated (live)\n"
        "     Tool calls are disabled in this mode\n\n"
        "OFF: Responses wait until complete\n"
        "     Full tool support enabled\n\n"
        "Without arguments, toggles the current setting.",
        "stream        # Toggle streaming\n"
        "stream on     # Enable streaming\n"
        "stream off    # Disable streaming"
    },
    {
        "theme",
        "theme [ocean|forest|sunset|mono]",
        "Change color theme",
        "Available themes:\n"
        "  ocean   - Cool blue tones (default)\n"
        "  forest  - Natural green tones\n"
        "  sunset  - Warm orange/red tones\n"
        "  mono    - Monochrome (grayscale)\n\n"
        "Without arguments, lists available themes.\n"
        "Theme preference is saved to config.",
        "theme          # List themes\n"
        "theme ocean    # Set ocean theme\n"
        "theme mono     # Set monochrome theme"
    },
    {
        "think",
        "think <intent>",
        "Process an intent through the assistant",
        "Parses the given text as an intent and has the assistant\n"
        "think through it. Shows:\n"
        "  - Intent classification\n"
        "  - Confidence and urgency scores\n"
        "  - Clarification questions if needed\n"
        "  - Assistant's thoughts",
        "think \"come posso migliorare le performance?\"\n"
        "think implementa una cache per le query"
    },
    {
        "compare",
        "compare <prompt> <model1> <model2> [model3...]",
        "Compare multiple models side-by-side",
        "Compares responses from different AI models using the same prompt.\n"
        "Runs models in parallel and shows:\n"
        "  - Response from each model\n"
        "  - Token counts (input/output)\n"
        "  - Latency and cost per model\n"
        "  - Diff between responses\n\n"
        "Options:\n"
        "  --no-diff      Skip diff generation\n"
        "  --json         Output as JSON\n"
        "  --sequential   Run sequentially instead of parallel",
        "compare \"Explain quantum computing\" claude-opus-4 gpt-4\n"
        "compare \"Write a haiku\" claude-sonnet-4 claude-opus-4 --no-diff"
    },
    {
        "benchmark",
        "benchmark <prompt> <model> [iterations]",
        "Benchmark a model's performance",
        "Runs the same prompt multiple times against a model to measure:\n"
        "  - Average latency\n"
        "  - Token throughput\n"
        "  - Cost per run\n"
        "  - Consistency of responses\n\n"
        "Default iterations: 3\n"
        "Maximum iterations: 100",
        "benchmark \"Write a haiku\" claude-opus-4\n"
        "benchmark \"Summarize this\" claude-sonnet-4 5"
    },
    {
        "telemetry",
        "telemetry <subcommand>",
        "Manage telemetry settings",
        "Privacy-first, opt-in telemetry for improving Convergio.\n\n"
        "Subcommands:\n"
        "  status     Show current telemetry status\n"
        "  info       Show what data is collected\n"
        "  enable     Enable telemetry (opt-in)\n"
        "  disable    Disable telemetry (opt-out)\n"
        "  view       View collected data\n"
        "  export     Export data as JSON\n"
        "  delete     Delete all collected data\n\n"
        "Core Principles:\n"
        "  - OPT-IN ONLY (never enabled by default)\n"
        "  - Privacy-first (no PII, anonymous metrics only)\n"
        "  - User control (view/export/delete at any time)",
        "telemetry status\n"
        "telemetry enable\n"
        "telemetry view\n"
        "telemetry delete"
    },
    {
        "tools",
        "tools <subcommand>",
        "Manage development tools",
        "Check for and install development tools used by Convergio.\n\n"
        "Subcommands:\n"
        "  check            Show installed/missing development tools\n"
        "  install <tool>   Install a tool (requires approval)\n\n"
        "Checks for common development tools like:\n"
        "  - gh (GitHub CLI)\n"
        "  - git, node, npm, python3\n"
        "  - docker, make, cmake\n"
        "  - curl, wget, jq",
        "tools check\n"
        "tools install gh\n"
        "tools install docker"
    },
    {
        "quit",
        "quit",
        "Exit Convergio",
        "Gracefully shuts down Convergio:\n"
        "  - Shows final cost report\n"
        "  - Saves configuration\n"
        "  - Cleans up resources\n\n"
        "Alias: 'exit'",
        "quit\n"
        "exit"
    },
    {NULL, NULL, NULL, NULL, NULL}
};

static const CommandHelp* find_detailed_help(const char* cmd_name) {
    for (const CommandHelp* h = DETAILED_HELP; h->name != NULL; h++) {
        if (strcmp(h->name, cmd_name) == 0) {
            return h;
        }
    }
    return NULL;
}

static void print_detailed_help(const CommandHelp* h) {
    printf("\n\033[1m%s\033[0m - %s\n", h->name, h->description);
    printf("\n\033[36mUsage:\033[0m\n  %s\n", h->usage);
    printf("\n\033[36mDescription:\033[0m\n");

    // Print description with indentation
    const char* p = h->details;
    printf("  ");
    while (*p) {
        putchar(*p);
        if (*p == '\n' && *(p+1)) {
            printf("  ");
        }
        p++;
    }
    printf("\n");

    printf("\n\033[36mExamples:\033[0m\n");
    p = h->examples;
    printf("  ");
    while (*p) {
        putchar(*p);
        if (*p == '\n' && *(p+1)) {
            printf("  ");
        }
        p++;
    }
    printf("\n\n");
}

// ============================================================================
// CORE COMMANDS
// ============================================================================

int cmd_help(int argc, char** argv) {
    // If a specific command is requested, show detailed help
    if (argc >= 2) {
        const CommandHelp* h = find_detailed_help(argv[1]);
        if (h) {
            print_detailed_help(h);
            return 0;
        }

        // Check if it's a known command without detailed help
        for (const ReplCommand* cmd = COMMANDS; cmd->name != NULL; cmd++) {
            if (strcmp(cmd->name, argv[1]) == 0) {
                printf("\n\033[1m%s\033[0m - %s\n", cmd->name, cmd->description);
                printf("\nNo detailed help available for this command.\n\n");
                return 0;
            }
        }

        printf("\nUnknown command: %s\n", argv[1]);
        printf("Type 'help' to see available commands.\n\n");
        return -1;
    }

    // Show general help
    printf("\n\033[1mCONVERGIO KERNEL\033[0m - Multi-Agent Orchestration\n");
    printf("═══════════════════════════════════════════════\n\n");
    printf("\033[36mAvailable commands:\033[0m\n\n");

    for (const ReplCommand* cmd = COMMANDS; cmd->name != NULL; cmd++) {
        printf("  \033[33m%-12s\033[0m %s\n", cmd->name, cmd->description);
    }

    printf("\n\033[36mCost management:\033[0m\n");
    printf("  \033[33mcost\033[0m              Show current spending\n");
    printf("  \033[33mcost report\033[0m       Detailed cost report\n");
    printf("  \033[33mcost set <USD>\033[0m    Set budget limit\n");
    printf("  \033[33mcost reset\033[0m        Reset session spending\n");

    printf("\n\033[36mWorkspace/Sandbox:\033[0m\n");
    printf("  \033[33mallowed-dirs\033[0m      Show allowed directories\n");
    printf("  \033[33mallow-dir <path>\033[0m  Add directory to sandbox\n");

    printf("\n\033[36mAuthentication:\033[0m\n");
    printf("  \033[33mauth\033[0m              Show authentication status\n");
    printf("  \033[33mlogout\033[0m            Logout and clear credentials\n");

    printf("\n\033[36mModel Comparison:\033[0m\n");
    printf("  \033[33mcompare\033[0m           Compare multiple models side-by-side\n");
    printf("  \033[33mbenchmark\033[0m         Benchmark a model's performance\n");

    printf("\n\033[2mType 'help <command>' for detailed help on a specific command.\033[0m\n");
    printf("\033[2mOr simply talk to Ali, your Chief of Staff.\033[0m\n\n");

    return 0;
}

int cmd_quit(int argc, char** argv) {
    (void)argc; (void)argv;
    g_running = 0;
    return 0;
}

int cmd_status(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("\n=== NOUS System Status ===\n\n");

    // Kernel status
    printf("Kernel: %s\n", nous_is_ready() ? "READY" : "NOT READY");

    // Current space
    NousSpace* space = (NousSpace*)g_current_space;
    if (space) {
        printf("\nCurrent Space: %s\n", space->name);
        printf("  Purpose: %s\n", space->purpose);
        printf("  Participants: %zu\n", nous_space_participant_count(space));
        printf("  Urgency: %.2f\n", nous_space_urgency(space));
        printf("  Active: %s\n", nous_space_is_active(space) ? "Yes" : "No");
    } else {
        printf("\nNo active space.\n");
    }

    // Assistant
    NousAgent* assistant = (NousAgent*)g_assistant;
    if (assistant) {
        printf("\nAssistant: %s\n", assistant->name);
        printf("  State: %d\n", assistant->state);
        printf("  Skills: %zu\n", assistant->skill_count);
    }

    printf("\n");

    // GPU stats
    nous_gpu_print_stats();

    // Scheduler metrics
    nous_scheduler_print_metrics();

    printf("\n");
    return 0;
}

// ============================================================================
// COST COMMANDS
// ============================================================================

int cmd_cost(int argc, char** argv) {
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

// ============================================================================
// AGENT COMMANDS
// ============================================================================

int cmd_agents(int argc, char** argv) {
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

int cmd_agent(int argc, char** argv) {
    if (argc < 2) {
        printf("\n\033[1mComando: agent\033[0m - Gestione agenti\n\n");
        printf("\033[1mSottocomandi:\033[0m\n");
        printf("  \033[36mlist\033[0m                    Lista tutti gli agenti disponibili\n");
        printf("  \033[36minfo <nome>\033[0m             Mostra dettagli agente (modello, ruolo, etc.)\n");
        printf("  \033[36medit <nome>\033[0m             Apri l'agente nell'editor per modificarlo\n");
        printf("  \033[36mreload\033[0m                  Ricarica tutti gli agenti dopo modifiche\n");
        printf("  \033[36mcreate <nome> <desc>\033[0m    Crea un nuovo agente dinamico\n");
        printf("  \033[36mskill <skill_name>\033[0m      Aggiungi skill all'assistente\n");
        printf("\n\033[1mEsempi:\033[0m\n");
        printf("  agent list              # Mostra tutti gli agenti\n");
        printf("  agent info baccio       # Dettagli su Baccio\n");
        printf("  agent edit amy          # Modifica Amy nel tuo editor\n");
        printf("  agent reload            # Ricarica dopo modifiche\n");
        printf("  agent create helper \"Un assistente generico\"\n");
        printf("\n");
        return 0;
    }

    // agent list
    if (strcmp(argv[1], "list") == 0) {
        char* status = agent_registry_status();
        if (status) {
            printf("\n%s", status);
            free(status);
        }
        return 0;
    }

    // agent info <name>
    if (strcmp(argv[1], "info") == 0) {
        if (argc < 3) {
            printf("Usage: agent info <nome_agente>\n");
            printf("Esempio: agent info baccio\n");
            return -1;
        }

        ManagedAgent* agent = agent_find_by_name(argv[2]);
        if (!agent) {
            printf("Agente '%s' non trovato.\n", argv[2]);
            printf("Usa 'agent list' per vedere gli agenti disponibili.\n");
            return -1;
        }

        printf("\n\033[1m📋 Informazioni Agente: %s\033[0m\n\n", agent->name);
        printf("  \033[36mNome:\033[0m        %s\n", agent->name);
        printf("  \033[36mDescrizione:\033[0m %s\n", agent->description ? agent->description : "-");

        const char* role_names[] = {
            "Orchestrator", "Analyst", "Coder", "Writer", "Critic", "Planner", "Executor", "Memory"
        };
        printf("  \033[36mRuolo:\033[0m       %s\n", role_names[agent->role]);

        // Model is determined by role for now
        const char* model = "claude-sonnet-4-20250514";  // Default
        if (agent->role == AGENT_ROLE_ORCHESTRATOR) {
            model = "claude-opus-4-20250514";
        }
        printf("  \033[36mModello:\033[0m     %s\n", model);

        printf("  \033[36mAttivo:\033[0m      %s\n", agent->is_active ? "Sì" : "No");

        const char* state_names[] = {"Idle", "Thinking", "Executing", "Reviewing", "Waiting"};
        printf("  \033[36mStato:\033[0m       %s\n", state_names[agent->work_state]);

        if (agent->current_task) {
            printf("  \033[36mTask:\033[0m        %s\n", agent->current_task);
        }

        printf("\n  \033[2mUsa @%s <messaggio> per comunicare con questo agente\033[0m\n\n", agent->name);
        return 0;
    }

    // agent edit <name>
    if (strcmp(argv[1], "edit") == 0) {
        if (argc < 3) {
            printf("Usage: agent edit <agent_name>\n");
            printf("Example: agent edit amy\n");
            return -1;
        }

        const char* agent_name = argv[2];

        // Build path to agent definition file
        char path[PATH_MAX];
        bool found = false;

        // First try exact match
        snprintf(path, sizeof(path), "src/agents/definitions/%s.md", agent_name);
        if (access(path, F_OK) == 0) {
            found = true;
        }

        // Try to find by prefix (e.g., "amy" -> "amy-cfo.md")
        if (!found) {
            DIR* dir = opendir("src/agents/definitions");
            if (dir) {
                struct dirent* entry;
                size_t name_len = strlen(agent_name);
                while ((entry = readdir(dir)) != NULL) {
                    if (strncasecmp(entry->d_name, agent_name, name_len) == 0 &&
                        (entry->d_name[name_len] == '-' || entry->d_name[name_len] == '.')) {
                        snprintf(path, sizeof(path), "src/agents/definitions/%s", entry->d_name);
                        found = true;
                        break;
                    }
                }
                closedir(dir);
            }
        }

        if (!found) {
            printf("\033[31mAgent '%s' not found.\033[0m\n", agent_name);
            printf("Use 'agent list' to see available agents.\n");
            return -1;
        }

        // Get editor from environment
        const char* editor = getenv("EDITOR");
        if (!editor) editor = getenv("VISUAL");

        char cmd[PATH_MAX + 64];

        if (editor) {
            // Use $EDITOR
            snprintf(cmd, sizeof(cmd), "%s \"%s\"", editor, path);
        } else {
            // macOS: use 'open' command
            snprintf(cmd, sizeof(cmd), "open \"%s\"", path);
        }

        printf("\033[36mOpening %s...\033[0m\n", path);
        int result = system(cmd);

        if (result != 0) {
            printf("\033[31mFailed to open editor.\033[0m\n");
            return -1;
        }

        printf("\n\033[33mAfter editing, run 'agent reload' to apply changes.\033[0m\n");
        return 0;
    }

    // agent reload
    if (strcmp(argv[1], "reload") == 0) {
        printf("\033[36mReloading agent definitions...\033[0m\n");

        // Re-run the embed script if available
        if (access("scripts/embed_agents.sh", X_OK) == 0) {
            printf("  Running embed_agents.sh...\n");
            int result = system("./scripts/embed_agents.sh");
            if (result != 0) {
                printf("\033[31mFailed to regenerate embedded agents.\033[0m\n");
                printf("You may need to rebuild: make clean && make\n");
                return -1;
            }
            printf("\033[32m✓ Agents regenerated.\033[0m\n");
            printf("\n\033[33mNote: Rebuild required to apply changes: make\033[0m\n");
        } else {
            printf("\033[33mNo embed script found. Manual rebuild required.\033[0m\n");
            printf("Run: make clean && make\n");
        }
        return 0;
    }

    // agent create <name> <essence>
    if (strcmp(argv[1], "create") == 0) {
        if (argc < 4) {
            printf("Usage: agent create <nome> <descrizione>\n");
            printf("Esempio: agent create helper \"Un assistente per task generici\"\n");
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
            printf("Errore: impossibile creare l'agente.\n");
            return -1;
        }

        printf("✅ Creato agente \"%s\"\n", agent->name);
        printf("  Patience: %.2f\n", agent->patience);
        printf("  Creativity: %.2f\n", agent->creativity);
        printf("  Assertiveness: %.2f\n", agent->assertiveness);

        NousAgent* assistant = (NousAgent*)g_assistant;
        if (!assistant) {
            g_assistant = agent;
            printf("Impostato come assistente principale.\n");
        }

        return 0;
    }

    // agent skill <skill_name>
    if (strcmp(argv[1], "skill") == 0) {
        if (argc < 3) {
            printf("Usage: agent skill <nome_skill>\n");
            return -1;
        }
        NousAgent* assistant = (NousAgent*)g_assistant;
        if (!assistant) {
            printf("Errore: nessun assistente attivo.\n");
            printf("Crea prima un agente con: agent create <nome> <descrizione>\n");
            return -1;
        }

        if (nous_agent_add_skill(assistant, argv[2]) == 0) {
            printf("✅ Aggiunta skill \"%s\" a %s\n", argv[2], assistant->name);
        }
        return 0;
    }

    printf("Sottocomando sconosciuto: %s\n", argv[1]);
    printf("Usa 'agent' senza argomenti per vedere l'help.\n");
    return -1;
}

static void on_thought(NousAgent* agent, const char* thought, void* ctx) {
    (void)ctx;
    printf("\n%s: %s\n\n", agent->name, thought);
}

int cmd_think(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: think <intent>\n");
        return -1;
    }

    NousAgent* assistant = (NousAgent*)g_assistant;
    if (!assistant) {
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
    nous_agent_think(assistant, intent, on_thought, NULL);

    return 0;
}

// ============================================================================
// SPACE COMMANDS
// ============================================================================

int cmd_create(int argc, char** argv) {
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

int cmd_space(int argc, char** argv) {
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
        NousAgent* assistant = (NousAgent*)g_assistant;
        if (assistant) {
            nous_join_space(assistant->id, space->id);
            printf("Assistant joined space.\n");
        }

        return 0;
    }

    NousSpace* space = (NousSpace*)g_current_space;
    if (strcmp(argv[1], "urgency") == 0 && space) {
        printf("Current urgency: %.2f\n", nous_space_urgency(space));
        return 0;
    }

    printf("Unknown space command: %s\n", argv[1]);
    return -1;
}

// ============================================================================
// DEBUG COMMANDS
// ============================================================================

int cmd_debug(int argc, char** argv) {
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

int cmd_allow_dir(int argc, char** argv) {
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

int cmd_allowed_dirs(int argc, char** argv) {
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

int cmd_logout(int argc, char** argv) {
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

int cmd_auth(int argc, char** argv) {
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

// ============================================================================
// SYSTEM COMMANDS
// ============================================================================

int cmd_update(int argc, char** argv) {
    if (argc >= 2 && strcmp(argv[1], "install") == 0) {
        return convergio_cmd_update_install();
    }
    if (argc >= 2 && strcmp(argv[1], "changelog") == 0) {
        return convergio_cmd_update_changelog();
    }
    // Default: check for updates
    return convergio_cmd_update_check();
}

int cmd_hardware(int argc, char** argv) {
    (void)argc; (void)argv;
    convergio_print_hardware_info();
    return 0;
}

int cmd_news(int argc, char** argv) {
    UpdateInfo info;
    const char* version = NULL;

    // Optional version argument: news v3.0.3 or news 3.0.3
    if (argc > 1) {
        version = argv[1];
    }

    // Fetch release info (NULL = latest, otherwise specific version)
    if (convergio_fetch_release(version, &info) != 0) {
        if (version) {
            printf("\033[31mError:\033[0m Could not fetch release notes for version %s\n", version);
            printf("  Make sure the version exists (e.g., 3.0.4 or v3.0.4)\n");
        } else {
            printf("\033[31mError:\033[0m Could not fetch latest release notes\n");
        }
        return -1;
    }

    // Print release notes in a nice box
    printf("\n");
    printf("╭─ \033[1;36mConvergio v%s\033[0m ", info.latest_version);

    // Pad the header line
    int header_len = 15 + (int)strlen(info.latest_version);
    for (int i = header_len; i < 54; i++) printf("─");
    printf("╮\n");

    if (info.is_prerelease) {
        printf("│  \033[33m⚠ Pre-release\033[0m                                       │\n");
    }

    if (info.published_at[0]) {
        // Format: 2025-12-12T... -> 2025-12-12
        char date[11] = {0};
        strncpy(date, info.published_at, 10);
        printf("│  Released: %s                                  │\n", date);
    }

    printf("├──────────────────────────────────────────────────────┤\n");

    if (strlen(info.release_notes) > 0) {
        // Print release notes with word wrap
        const char* p = info.release_notes;
        while (*p) {
            printf("│  ");
            int col = 0;
            while (*p && *p != '\n' && col < 52) {
                // Handle \r\n
                if (*p == '\r') {
                    p++;
                    continue;
                }
                putchar(*p);
                p++;
                col++;
            }
            while (col < 52) {
                putchar(' ');
                col++;
            }
            printf(" │\n");
            if (*p == '\n') p++;
        }
    } else {
        printf("│  No release notes available.                         │\n");
    }

    printf("╰──────────────────────────────────────────────────────╯\n");
    printf("\n");

    return 0;
}

int cmd_stream(int argc, char** argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "on") == 0) {
            g_streaming_enabled = true;
            printf("Streaming mode: \033[32mON\033[0m\n");
            printf("  Live markdown rendering enabled\n");
            printf("  Note: Tool calls are disabled in streaming mode\n");
        } else if (strcmp(argv[1], "off") == 0) {
            g_streaming_enabled = false;
            printf("Streaming mode: \033[2mOFF\033[0m\n");
            printf("  Full tool support enabled, responses wait until complete\n");
        } else {
            printf("Usage: stream [on|off]\n");
        }
    } else {
        // Toggle
        g_streaming_enabled = !g_streaming_enabled;
        if (g_streaming_enabled) {
            printf("Streaming mode: \033[32mON\033[0m\n");
            printf("  Live markdown rendering enabled\n");
            printf("  Note: Tool calls are disabled in streaming mode\n");
        } else {
            printf("Streaming mode: \033[2mOFF\033[0m\n");
            printf("  Full tool support enabled, responses wait until complete\n");
        }
    }
    return 0;
}

int cmd_theme(int argc, char** argv) {
    if (argc > 1) {
        if (theme_set_by_name(argv[1])) {
            const Theme* t = theme_get();
            printf("Theme changed to: %s%s%s\n", t->prompt_name, t->name, theme_reset());
            theme_save();
        } else {
            printf("Unknown theme: %s\n", argv[1]);
            theme_list();
        }
    } else {
        theme_list();
    }
    return 0;
}

// ============================================================================
// MODEL COMPARISON COMMANDS
// ============================================================================

// Default cheap models for comparison (one per provider)
static const char* DEFAULT_COMPARE_MODELS[] = {
    "claude-haiku-4.5",   // Anthropic - cheapest
    "gpt-5-nano",         // OpenAI - cheapest
    "gemini-3-flash",     // Google - cheapest
};
static const size_t DEFAULT_COMPARE_MODEL_COUNT = 3;

int cmd_compare(int argc, char** argv) {
    if (argc < 2) {
        printf("\n\033[1mCommand: compare\033[0m - Compare models side-by-side\n\n");
        printf("\033[1mUsage:\033[0m\n");
        printf("  compare <prompt>                    # Uses default models (cheapest)\n");
        printf("  compare <prompt> <model1> <model2>  # Custom models\n\n");
        printf("\033[1mDefault models:\033[0m (cheapest from each provider)\n");
        printf("  - claude-haiku-4.5 (Anthropic)\n");
        printf("  - gpt-5-nano (OpenAI)\n");
        printf("  - gemini-3-flash (Google)\n\n");
        printf("\033[1mExample:\033[0m\n");
        printf("  compare \"Explain quantum computing\"\n");
        printf("  compare \"Write a poem\" claude-opus-4 gpt-5\n\n");
        printf("\033[1mOptions:\033[0m\n");
        printf("  --no-diff      Skip diff generation\n");
        printf("  --json         Output as JSON\n");
        printf("  --sequential   Run sequentially instead of parallel\n\n");
        return 0;
    }

    // Parse prompt
    const char* prompt = argv[1];

    // Count models and check for options
    CompareOptions opts = compare_options_default();
    size_t model_count = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--no-diff") == 0) {
            opts.show_diff = false;
        } else if (strcmp(argv[i], "--json") == 0) {
            opts.output_format = "json";
        } else if (strcmp(argv[i], "--sequential") == 0) {
            opts.mode = COMPARE_MODE_SEQUENTIAL;
        } else if (argv[i][0] == '-') {
            printf("Warning: Unknown option '%s' ignored.\n", argv[i]);
        } else {
            model_count++;
        }
    }

    // Use default models if none specified
    const char** models_to_use;
    bool using_defaults = false;

    if (model_count == 0) {
        // Use default cheap models
        models_to_use = DEFAULT_COMPARE_MODELS;
        model_count = DEFAULT_COMPARE_MODEL_COUNT;
        using_defaults = true;
        printf("\033[36mUsing default models: haiku, gpt-5-nano, gemini-flash\033[0m\n\n");
    } else if (model_count == 1) {
        printf("Error: Need at least 2 models to compare (or none for defaults).\n");
        return -1;
    } else {
        // Filter out options from models list
        models_to_use = malloc(sizeof(char*) * model_count);
        if (!models_to_use) {
            printf("Error: Memory allocation failed.\n");
            return -1;
        }

        size_t idx = 0;
        for (int i = 2; i < argc; i++) {
            if (argv[i][0] != '-') {
                models_to_use[idx++] = argv[i];
            }
        }
    }

    // Run comparison
    CompareResult* results = NULL;
    int ret = compare_models(prompt, NULL, models_to_use, model_count, &opts, &results);

    // Cleanup
    if (results) {
        compare_results_free(results, model_count);
    }
    if (!using_defaults) {
        free((void*)models_to_use);
    }

    return ret;
}

int cmd_benchmark(int argc, char** argv) {
    if (argc < 2) {
        printf("\n\033[1mCommand: benchmark\033[0m - Benchmark a model's performance\n\n");
        printf("\033[1mUsage:\033[0m\n");
        printf("  benchmark <prompt>                    # Uses claude-haiku-4.5, 3 iterations\n");
        printf("  benchmark <prompt> <model>            # Custom model, 3 iterations\n");
        printf("  benchmark <prompt> <model> <N>        # Custom model, N iterations\n\n");
        printf("\033[1mDefaults:\033[0m\n");
        printf("  Model: claude-haiku-4.5 (cheapest)\n");
        printf("  Iterations: 3\n\n");
        printf("\033[1mExample:\033[0m\n");
        printf("  benchmark \"Write a haiku\"\n");
        printf("  benchmark \"Explain AI\" gpt-5-nano 5\n\n");
        return 0;
    }

    const char* prompt = argv[1];
    const char* model = "claude-haiku-4.5";  // Default to cheapest
    size_t iterations = 3;

    if (argc >= 3) {
        model = argv[2];
    }

    if (argc >= 4) {
        iterations = (size_t)atoi(argv[3]);
        if (iterations == 0 || iterations > 100) {
            printf("Error: Iterations must be between 1 and 100.\n");
            return -1;
        }
    }

    printf("Starting benchmark: %zu iterations of %s\n\n", iterations, model);

    CompareResult result = {0};
    int ret = benchmark_model(prompt, NULL, model, iterations, &result);

    // Cleanup
    if (result.model_id) free(result.model_id);
    if (result.response) free(result.response);
    if (result.error) free(result.error);

    return ret;
}

// ============================================================================
// TELEMETRY COMMAND
// ============================================================================

int cmd_telemetry(int argc, char** argv) {
    if (argc < 2) {
        printf("\n\033[1mTelemetry Management\033[0m\n\n");
        printf("Privacy-first, opt-in telemetry for improving Convergio\n\n");
        printf("\033[1mUsage:\033[0m\n");
        printf("  telemetry status   - Show current telemetry status\n");
        printf("  telemetry info     - Show what data is collected\n");
        printf("  telemetry enable   - Enable telemetry (opt-in)\n");
        printf("  telemetry disable  - Disable telemetry (opt-out)\n");
        printf("  telemetry view     - View collected data\n");
        printf("  telemetry export   - Export data as JSON\n");
        printf("  telemetry delete   - Delete all collected data\n\n");
        printf("\033[1mCore Principles:\033[0m\n");
        printf("  • OPT-IN ONLY (never enabled by default)\n");
        printf("  • Privacy-first (no PII, anonymous metrics only)\n");
        printf("  • User control (view/export/delete at any time)\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "status") == 0) {
        telemetry_status();
        return 0;
    }

    if (strcmp(subcommand, "info") == 0) {
        telemetry_show_consent_prompt();
        return 0;
    }

    if (strcmp(subcommand, "enable") == 0) {
        int ret = telemetry_enable();
        if (ret == 0) {
            printf("\nTelemetry has been enabled.\n");
            printf("Thank you for helping improve Convergio!\n\n");
            printf("You can view collected data with: telemetry view\n");
            printf("You can disable at any time with: telemetry disable\n\n");
        } else {
            printf("\nFailed to enable telemetry.\n");
        }
        return ret;
    }

    if (strcmp(subcommand, "disable") == 0) {
        int ret = telemetry_disable();
        if (ret == 0) {
            printf("\nTelemetry has been disabled.\n");
            printf("No further data will be collected.\n\n");
            printf("Existing data is still stored locally.\n");
            printf("To delete it, use: telemetry delete\n\n");
        } else {
            printf("\nFailed to disable telemetry.\n");
        }
        return ret;
    }

    if (strcmp(subcommand, "view") == 0) {
        telemetry_view();
        return 0;
    }

    if (strcmp(subcommand, "export") == 0) {
        char* data = telemetry_export();
        if (data) {
            printf("\n");
            printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
            printf("║                     TELEMETRY DATA EXPORT                             ║\n");
            printf("╚═══════════════════════════════════════════════════════════════════════╝\n");
            printf("\n");
            printf("%s\n", data);
            printf("\n");
            printf("You can save this output with:\n");
            printf("  telemetry export > telemetry_export.json\n\n");
            free(data);
            return 0;
        } else {
            printf("\nNo telemetry data to export.\n");
            return -1;
        }
    }

    if (strcmp(subcommand, "delete") == 0) {
        return telemetry_delete();
    }

    printf("Unknown telemetry subcommand: %s\n", subcommand);
    printf("Run 'telemetry' without arguments for usage information.\n");
    return -1;
}

// ============================================================================
// DEVELOPMENT TOOLS COMMAND
// ============================================================================

int cmd_tools(int argc, char** argv) {
    if (argc < 2) {
        printf("\n\033[1mCommand: tools\033[0m - Manage development tools\n\n");
        printf("Usage:\n");
        printf("  tools check            - Show installed/missing development tools\n");
        printf("  tools install <tool>   - Install a tool (requires approval)\n\n");
        printf("Example:\n");
        printf("  tools check            - List all tools\n");
        printf("  tools install gh       - Install GitHub CLI\n\n");
        return 0;
    }

    const char* subcommand = argv[1];

    if (strcmp(subcommand, "check") == 0) {
        printf("\n\033[1mDevelopment Tools Status\033[0m\n");
        printf("═══════════════════════════════════════════\n\n");

        const char* tools[] = {"gh", "git", "node", "npm", "python3", "pip3",
                               "cargo", "go", "make", "cmake", "docker", "jq",
                               "curl", "wget", NULL};

        int installed = 0, missing = 0;

        for (int i = 0; tools[i] != NULL; i++) {
            bool exists = tool_exists(tools[i]);
            if (exists) {
                printf("  \033[32m✓\033[0m %-12s installed\n", tools[i]);
                installed++;
            } else {
                printf("  \033[31m✗\033[0m %-12s not found\n", tools[i]);
                missing++;
            }
        }

        printf("\n%d installed, %d missing\n\n", installed, missing);

        if (missing > 0) {
            printf("To install: \033[33mtools install <tool>\033[0m\n\n");
        }
        return 0;
    }

    if (strcmp(subcommand, "install") == 0) {
        if (argc < 3) {
            printf("Usage: tools install <tool>\n");
            printf("Example: tools install gh\n");
            return -1;
        }

        const char* tool = argv[2];

        // Check if already installed
        if (tool_exists(tool)) {
            printf("\033[32m%s is already installed.\033[0m\n", tool);
            return 0;
        }

        // Get install command
        const char* install_cmd = get_install_command(tool);
        if (!install_cmd) {
            printf("\033[31mError: Don't know how to install '%s'\033[0m\n", tool);
            printf("Please install manually.\n");
            return -1;
        }

        // Request approval
        ApprovalRequest req = {
            .action = tool,
            .reason = "Development tool needed",
            .command = install_cmd,
            .is_destructive = false
        };

        if (!request_user_approval(&req)) {
            printf("\nInstallation cancelled.\n");
            return 0;
        }

        // Install
        printf("\nInstalling %s...\n", tool);
        int ret = install_tool(tool, "Development tool needed");

        if (ret == 0) {
            printf("\033[32m✓ %s installed successfully.\033[0m\n", tool);
        } else {
            printf("\033[31m✗ Failed to install %s.\033[0m\n", tool);
        }

        return ret;
    }

    printf("Unknown tools subcommand: %s\n", subcommand);
    printf("Run 'tools' without arguments for usage information.\n");
    return -1;
}
