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
#include "nous/projects.h"
#include "nous/semantic_persistence.h"
#include "nous/model_loader.h"
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

// Session functions are declared in nous/orchestrator.h

// ============================================================================
// COMMAND TABLE
// ============================================================================

// Forward declaration for project command
int cmd_project(int argc, char** argv);

// Forward declaration for setup wizard
int cmd_setup(int argc, char** argv);

// Forward declaration for session recall
int cmd_recall(int argc, char** argv);

// Forward declarations for semantic memory commands
int cmd_remember(int argc, char** argv);
int cmd_search(int argc, char** argv);
int cmd_memories(int argc, char** argv);
int cmd_forget(int argc, char** argv);
int cmd_graph(int argc, char** argv);

static const ReplCommand COMMANDS[] = {
    {"help",        "Show available commands",           cmd_help},
    {"agent",       "Manage agents",                     cmd_agent},
    {"agents",      "List all available agents",         cmd_agents},
    {"project",     "Manage projects with dedicated teams", cmd_project},
    {"setup",       "Configure providers and agent models", cmd_setup},
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
    {"theme",       "Interactive theme selector (or /theme <name>)", cmd_theme},
    {"compare",     "Compare responses from 2-3 models", cmd_compare},
    {"benchmark",   "Test ONE model's speed (N runs)",   cmd_benchmark},
    {"telemetry",   "Manage telemetry settings",         cmd_telemetry},
    {"tools",       "Manage development tools",          cmd_tools},
    {"news",        "Show release notes",                cmd_news},
    // Session recall
    {"recall",      "View/load past sessions",           cmd_recall},
    // Semantic memory commands
    {"remember",    "Store a memory",                    cmd_remember},
    {"search",      "Search memories semantically",      cmd_search},
    {"memories",    "List recent/important memories",    cmd_memories},
    {"forget",      "Delete a memory by ID",             cmd_forget},
    {"graph",       "Show knowledge graph stats",        cmd_graph},
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
        "recall",
        "recall [load <n>|delete <n>|clear]",
        "View and reload past session contexts",
        "Shows summaries of past sessions with what was discussed.\n"
        "Sessions are saved when you exit with 'quit'.\n"
        "Subcommands:\n"
        "  load <n>        Load context from session N into current conversation\n"
        "  delete <n>      Delete session N and its summary\n"
        "  clear           Delete all stored summaries (asks for confirmation)\n",
        "recall           # List past sessions with summaries\n"
        "recall load 1    # Load context from session 1\n"
        "recall delete 2  # Delete session 2\n"
        "recall clear     # Delete all sessions"
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
        "project",
        "project <subcommand> [args]",
        "Manage projects with dedicated agent teams",
        "Create and manage projects with focused agent teams.\n"
        "Each project has a purpose, team, and persistent context.\n\n"
        "Subcommands:\n"
        "  create <name>           Create a new project\n"
        "    --purpose \"...\"       Set project description\n"
        "    --team agents         Comma-separated agent names\n"
        "    --template name       Use a template (app-dev, marketing, etc.)\n"
        "  list                    List all projects\n"
        "  use <name>              Switch to a project\n"
        "  status                  Show current project details\n"
        "  team add <agent>        Add agent to current project\n"
        "  team remove <agent>     Remove agent from project\n"
        "  templates               List available templates\n"
        "  focus <text>            Set current focus\n"
        "  decision <text>         Record a key decision\n"
        "  archive <name>          Archive a project\n"
        "  clear                   Clear current project",
        "project create \"MyApp 2.0\" --template app-dev\n"
        "project create Marketing --team matteo,copywriter,analyst\n"
        "project use MyApp\n"
        "project team add tester\n"
        "project status"
    },
    {
        "setup",
        "setup",
        "Configure providers and agent models",
        "Interactive setup wizard for configuring AI providers:\n"
        "  - Anthropic (Claude Opus, Sonnet, Haiku)\n"
        "  - OpenAI (GPT-5, GPT-4o, o3, o4-mini)\n"
        "  - Google Gemini (Pro, Ultra, Flash)\n"
        "  - OpenRouter (300+ models via unified API)\n"
        "  - Ollama (local models - free, private)\n\n"
        "Quick Setup Profiles:\n"
        "  - Cost-Optimized: Cheapest models (~$0.50/day)\n"
        "  - Balanced: Quality/cost mix (~$2-5/day)\n"
        "  - Performance: Best models (~$10-20/day)\n"
        "  - Local-First: Ollama with cloud fallback (free)\n\n"
        "API keys can be stored in environment variables or session.",
        "setup           # Start interactive wizard\n"
        "setup           # Configure API keys\n"
        "setup           # Choose optimization profile"
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
    // Semantic memory commands
    {
        "remember",
        "remember <text>",
        "Store a memory in the knowledge graph",
        "Creates a persistent memory node that survives across sessions.\n"
        "Memories are stored with high importance (0.9) and can be:\n"
        "  - Searched with 'recall'\n"
        "  - Listed with 'memories'\n"
        "  - Deleted with 'forget'\n\n"
        "Memories persist in SQLite and are loaded on startup.",
        "remember Roberto prefers clean code\n"
        "remember The API key is stored in keychain\n"
        "remember Use snake_case for variables"
    },
    {
        "recall",
        "recall <query>",
        "Search memories by keyword",
        "Searches the knowledge graph for memories matching your query.\n"
        "Returns up to 10 matching results with their importance scores.\n\n"
        "Currently uses keyword matching. Semantic similarity search\n"
        "will be added when the embedding system is fully implemented.",
        "recall Roberto\n"
        "recall API key\n"
        "recall code style"
    },
    {
        "memories",
        "memories",
        "List knowledge graph statistics and important memories",
        "Shows:\n"
        "  - Total nodes and relations in the graph\n"
        "  - Nodes currently loaded in memory\n"
        "  - The 10 most important memories (importance >= 0.5)\n\n"
        "Use this to get an overview of what Convergio remembers.",
        "memories"
    },
    {
        "forget",
        "forget <id>",
        "Delete a memory by its ID",
        "Permanently removes a memory from the knowledge graph.\n"
        "The ID is a hexadecimal number shown in 'recall' or 'memories' output.\n\n"
        "This also removes all relations connected to that memory.",
        "forget 0x1234567890abcdef\n"
        "forget 1234567890abcdef"
    },
    {
        "graph",
        "graph",
        "Show knowledge graph statistics",
        "Displays detailed statistics about the semantic knowledge graph:\n"
        "  - Total nodes in database\n"
        "  - Nodes loaded in memory\n"
        "  - Total relations (connections between nodes)\n"
        "  - Breakdown of nodes by type (Memory, Concept, Entity, etc.)\n\n"
        "The knowledge graph stores memories, concepts, and their relationships\n"
        "to enable semantic understanding across sessions.",
        "graph"
    },
    {
        "local",
        "help local",
        "Local models guide (MLX on Apple Silicon)",
        "Run AI models 100% offline on your Mac without cloud APIs or internet.\n"
        "Requires Apple Silicon (M1/M2/M3/M4/M5).\n\n"
        "QUICK START:\n"
        "  /setup -> Local Models -> Download a model\n\n"
        "AVAILABLE MODELS:\n"
        "  - Llama 3.2 1B/3B    - Fast, general purpose\n"
        "  - DeepSeek R1 Distill - Reasoning, coding, math (1.5B/7B/14B)\n"
        "  - Qwen 2.5 Coder 7B  - Code generation\n"
        "  - Phi-3 Mini         - Fast, efficient\n"
        "  - Mistral 7B Q4      - Multilingual, European\n"
        "  - Llama 3.1 8B Q4    - Best quality, long context\n\n"
        "BENEFITS:\n"
        "  - 100% offline operation (no internet required)\n"
        "  - Complete privacy (data never leaves your Mac)\n"
        "  - No API costs (free forever)\n"
        "  - Low latency (no network roundtrip)\n"
        "  - Apple Silicon optimized (Neural Engine + GPU)\n\n"
        "LIMITATIONS:\n"
        "  - Model download required (1-9 GB per model)\n"
        "  - Quality varies vs cloud models for complex tasks\n"
        "  - RAM requirements (4-16GB depending on model)\n"
        "  - Tool calling less reliable than Claude\n\n"
        "CLI OPTIONS:\n"
        "  convergio --local              Use MLX provider\n"
        "  convergio --local -m llama-3.2-3b  Specific model",
        "/setup           # Open wizard, select Local Models\n"
        "convergio --local --model deepseek-r1-7b\n"
        "convergio -l -m llama-3.2-3b"
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

    // Show general help - redesigned to showcase features
    printf("\n");
    printf("\033[36m┌──────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[36m│  \033[1;37mCONVERGIO\033[0;36m - Your AI Team with Human Purpose                 │\033[0m\n");
    printf("\033[36m└──────────────────────────────────────────────────────────────┘\033[0m\n\n");

    // 1. YOUR AI TEAM - The most important feature
    printf("\033[1;33m🤖 YOUR AI TEAM\033[0m  \033[2m(49 specialized agents ready to help)\033[0m\n");
    printf("   \033[36m@ali\033[0m               Chief of Staff - orchestrates everything\n");
    printf("   \033[36m@baccio\033[0m            Software Architect\n");
    printf("   \033[36m@marco\033[0m             Senior Developer\n");
    printf("   \033[36m@jenny\033[0m             Accessibility Expert\n");
    printf("   \033[36m@<name> message\033[0m    Talk directly to any agent\n");
    printf("   \033[36magents\033[0m             See all 49 agents with their specialties\n");
    printf("   \033[2m   Tip: Just type @ali to return to Ali from any agent\033[0m\n\n");

    // 2. PROJECTS - Team-based work
    printf("\033[1;33m📁 PROJECTS\033[0m  \033[2m(dedicated agent teams per project)\033[0m\n");
    printf("   \033[36mproject new <name>\033[0m         Create project with dedicated team\n");
    printf("   \033[36mproject team add <agent>\033[0m   Add agent to project team\n");
    printf("   \033[36mproject switch <name>\033[0m      Switch between projects\n");
    printf("   \033[36mproject\033[0m                    Show current project & team\n\n");

    // 3. KNOWLEDGE GRAPH - Persistent semantic memory
    printf("\033[1;33m🧠 KNOWLEDGE GRAPH\033[0m  \033[2m(persistent memory across sessions)\033[0m\n");
    printf("   \033[36mremember <text>\033[0m    Store important facts and preferences\n");
    printf("   \033[36mrecall <query>\033[0m     Search your memories by keyword\n");
    printf("   \033[36mmemories\033[0m           List stored memories and graph stats\n");
    printf("   \033[36mgraph\033[0m              Show knowledge graph statistics\n");
    printf("   \033[36mforget <id>\033[0m        Remove a memory\n");
    printf("   \033[2m   Tip: Memories persist in SQLite and survive restarts\033[0m\n\n");

    // 4. POWER FEATURES
    printf("\033[1;33m⚡ POWER FEATURES\033[0m\n");
    printf("   \033[36mcompare \"prompt\"\033[0m           Compare responses from 2-3 different models\n");
    printf("   \033[36mbenchmark \"prompt\" <model>\033[0m Test ONE model's speed & cost (N runs)\n");
    printf("   \033[36msetup\033[0m                      Configure providers & models per agent\n\n");

    // 5. CUSTOMIZATION
    printf("\033[1;33m🎨 CUSTOMIZATION\033[0m\n");
    printf("   \033[36mtheme\033[0m              Interactive theme selector with preview\n");
    printf("   \033[36magent edit <name>\033[0m  Customize any agent's personality & model\n");
    printf("   \033[36magent create\033[0m       Create your own custom agent\n\n");

    // 6. SYSTEM
    printf("\033[1;33m⚙️  SYSTEM\033[0m\n");
    printf("   \033[36mcost\033[0m / \033[36mcost report\033[0m   Track spending across all providers\n");
    printf("   \033[36mstatus\033[0m             System health & active agents\n");
    printf("   \033[36mhardware\033[0m           Show Apple Silicon optimization info\n");
    printf("   \033[36mtools\033[0m              Manage agentic tools (file, web, code)\n");
    printf("   \033[36mrecall\033[0m             View past sessions, \033[36mrecall load <n>\033[0m to reload\n");
    printf("   \033[36mdebug <level>\033[0m      Set debug level (off/error/warn/info/debug/trace)\n");
    printf("   \033[36mnews\033[0m               What's new in this version\n\n");

    printf("\033[2m───────────────────────────────────────────────────────────────────\033[0m\n");
    printf("\033[2mType \033[0mhelp <command>\033[2m for details  •  Or just talk to Ali!\033[0m\n\n");

    return 0;
}

// Progress callback for session compaction
static void quit_progress_callback(int percent, const char* msg) {
    // Simple progress bar: [████████░░] 80% Saving...
    int filled = percent / 10;
    int empty = 10 - filled;

    printf("\r\033[K[");  // Clear line and start bar
    for (int i = 0; i < filled; i++) printf("\033[32m█\033[0m");
    for (int i = 0; i < empty; i++) printf("\033[90m░\033[0m");
    printf("] %d%% %s", percent, msg ? msg : "");
    fflush(stdout);

    if (percent >= 100) {
        printf("\n");
    }
}

int cmd_quit(int argc, char** argv) {
    (void)argc; (void)argv;

    // Compact current session before exit
    printf("\n");
    orchestrator_compact_session(quit_progress_callback);

    g_running = 0;
    return 0;
}

// Static storage for session ID mapping (for recall load/delete by number)
static char* g_recall_session_ids[50] = {0};
static size_t g_recall_session_count = 0;

static void recall_clear_cache(void) {
    for (size_t i = 0; i < g_recall_session_count; i++) {
        free(g_recall_session_ids[i]);
        g_recall_session_ids[i] = NULL;
    }
    g_recall_session_count = 0;
}

static const char* recall_get_session_id(int index) {
    if (index < 1 || (size_t)index > g_recall_session_count) return NULL;
    return g_recall_session_ids[index - 1];
}

int cmd_recall(int argc, char** argv) {
    // Handle subcommand: recall clear
    if (argc >= 2 && strcmp(argv[1], "clear") == 0) {
        printf("\n\033[33mAre you sure you want to clear all session summaries?\033[0m\n");
        printf("Type 'yes' to confirm: ");
        fflush(stdout);

        char confirm[10] = {0};
        if (fgets(confirm, sizeof(confirm), stdin) && strncmp(confirm, "yes", 3) == 0) {
            if (persistence_clear_all_summaries() == 0) {
                recall_clear_cache();
                printf("\033[32mAll session summaries cleared.\033[0m\n\n");
            } else {
                printf("\033[31mFailed to clear summaries.\033[0m\n\n");
            }
        } else {
            printf("Cancelled.\n\n");
        }
        return 0;
    }

    // Handle subcommand: recall delete <num>
    if (argc >= 3 && strcmp(argv[1], "delete") == 0) {
        int index = atoi(argv[2]);
        const char* session_id = recall_get_session_id(index);
        if (!session_id) {
            // Maybe they passed the full UUID
            session_id = argv[2];
        }
        if (persistence_delete_session(session_id) == 0) {
            printf("\033[32mSession deleted.\033[0m\n\n");
            recall_clear_cache();  // Invalidate cache
        } else {
            printf("\033[31mFailed to delete session. Run 'recall' first to see valid numbers.\033[0m\n\n");
        }
        return 0;
    }

    // Handle subcommand: recall load <num>
    if (argc >= 3 && strcmp(argv[1], "load") == 0) {
        int index = atoi(argv[2]);
        const char* session_id = recall_get_session_id(index);
        if (!session_id) {
            printf("\n\033[31mInvalid session number. Run 'recall' first to see available sessions.\033[0m\n\n");
            return -1;
        }

        // Load the checkpoint/summary for this session
        char* checkpoint = persistence_load_latest_checkpoint(session_id);
        if (checkpoint && strlen(checkpoint) > 0) {
            printf("\n\033[1;36m=== Loaded Context from Session %d ===\033[0m\n\n", index);
            printf("%s\n", checkpoint);
            printf("\n\033[32m✓ Context loaded. Ali now has this context for your conversation.\033[0m\n\n");

            // Inject into orchestrator context
            Orchestrator* orch = orchestrator_get();
            if (orch) {
                free(orch->user_preferences);
                size_t len = strlen(checkpoint) + 100;
                orch->user_preferences = malloc(len);
                if (orch->user_preferences) {
                    snprintf(orch->user_preferences, len,
                        "Previous session context:\n%s", checkpoint);
                }
            }
            free(checkpoint);
        } else {
            printf("\n\033[33mNo detailed context found for this session.\033[0m\n");
            printf("The session may not have been compacted on exit.\n\n");
            free(checkpoint);
        }
        return 0;
    }

    // Default: show all session summaries
    SessionSummaryList* list = persistence_get_session_summaries();
    if (!list || list->count == 0) {
        printf("\n\033[90mNo past sessions found.\033[0m\n");
        printf("\033[90mSessions are saved when you type 'quit'.\033[0m\n\n");
        if (list) persistence_free_session_summaries(list);
        return 0;
    }

    // Cache session IDs for load/delete by number
    recall_clear_cache();
    for (size_t i = 0; i < list->count && i < 50; i++) {
        if (list->items[i].session_id) {
            g_recall_session_ids[i] = strdup(list->items[i].session_id);
            g_recall_session_count++;
        }
    }

    printf("\n\033[1m📚 Past Sessions\033[0m\n");
    printf("\033[90m────────────────────────────────────────────────────────\033[0m\n\n");

    for (size_t i = 0; i < list->count; i++) {
        SessionSummary* s = &list->items[i];

        // Header: [num] date (messages)
        printf("\033[1;36m[%zu]\033[0m \033[33m%s\033[0m", i + 1, s->started_at ? s->started_at : "Unknown");
        printf(" \033[90m(%d msgs)\033[0m\n", s->message_count);

        // Summary - the important part!
        if (s->summary && strlen(s->summary) > 0) {
            printf("    \033[37m");
            // Word wrap at ~70 chars with proper indentation
            const char* p = s->summary;
            int col = 0;
            size_t max_len = 300;  // Show more of the summary
            size_t printed = 0;
            while (*p && printed < max_len) {
                if (*p == '\n') {
                    printf("\n    ");
                    col = 0;
                } else {
                    putchar(*p);
                    col++;
                    if (col > 65 && *p == ' ') {
                        printf("\n    ");
                        col = 0;
                    }
                }
                p++;
                printed++;
            }
            if (printed >= max_len && *p) printf("...");
            printf("\033[0m\n");
        } else {
            printf("    \033[90m(no summary - quit with 'quit' to save)\033[0m\n");
        }
        printf("\n");
    }

    printf("\033[90m────────────────────────────────────────────────────────\033[0m\n");
    printf("\033[36mrecall load <n>\033[0m   Load context into current session\n");
    printf("\033[36mrecall delete <n>\033[0m Delete a session\n");
    printf("\033[36mrecall clear\033[0m      Delete all sessions\n\n");

    persistence_free_session_summaries(list);
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
        printf("  Urgency: %.2f\n", (double)nous_space_urgency(space));
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
        printf("\n\033[1mCommand: agent\033[0m - Agent management\n\n");
        printf("\033[1mSubcommands:\033[0m\n");
        printf("  \033[36mlist\033[0m                    List all available agents\n");
        printf("  \033[36minfo <name>\033[0m             Show agent details (model, role, etc.)\n");
        printf("  \033[36medit <name>\033[0m             Open agent in editor to modify\n");
        printf("  \033[36mreload\033[0m                  Reload all agents after changes\n");
        printf("  \033[36mcreate <name> <desc>\033[0m    Create a new dynamic agent\n");
        printf("  \033[36mskill <skill_name>\033[0m      Add skill to assistant\n");
        printf("\n\033[1mExamples:\033[0m\n");
        printf("  agent list              # Show all agents\n");
        printf("  agent info baccio       # Details about Baccio\n");
        printf("  agent edit amy          # Edit Amy in your editor\n");
        printf("  agent reload            # Reload after changes\n");
        printf("  agent create helper \"A generic assistant\"\n");
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
            printf("Usage: agent info <agent_name>\n");
            printf("Example: agent info baccio\n");
            return -1;
        }

        ManagedAgent* agent = agent_find_by_name(argv[2]);
        if (!agent) {
            printf("Agent '%s' not found.\n", argv[2]);
            printf("Use 'agent list' to see available agents.\n");
            return -1;
        }

        printf("\n\033[1m📋 Agent Info: %s\033[0m\n\n", agent->name);
        printf("  \033[36mName:\033[0m        %s\n", agent->name);
        printf("  \033[36mDescription:\033[0m %s\n", agent->description ? agent->description : "-");

        const char* role_names[] = {
            "Orchestrator", "Analyst", "Coder", "Writer", "Critic", "Planner", "Executor", "Memory"
        };
        printf("  \033[36mRole:\033[0m        %s\n", role_names[agent->role]);

        // Model is determined by role for now
        const char* model = "claude-sonnet-4-20250514";  // Default
        if (agent->role == AGENT_ROLE_ORCHESTRATOR) {
            model = "claude-opus-4-20250514";
        }
        printf("  \033[36mModel:\033[0m       %s\n", model);

        printf("  \033[36mActive:\033[0m      %s\n", agent->is_active ? "Yes" : "No");

        const char* state_names[] = {"Idle", "Thinking", "Executing", "Reviewing", "Waiting"};
        printf("  \033[36mState:\033[0m       %s\n", state_names[agent->work_state]);

        if (agent->current_task) {
            printf("  \033[36mTask:\033[0m        %s\n", agent->current_task);
        }

        printf("\n  \033[2mUse @%s <message> to communicate with this agent\033[0m\n\n", agent->name);
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
            printf("Usage: agent create <name> <description>\n");
            printf("Example: agent create helper \"A generic task assistant\"\n");
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
            printf("Error: unable to create agent.\n");
            return -1;
        }

        printf("Created agent \"%s\"\n", agent->name);
        printf("  Patience: %.2f\n", (double)agent->patience);
        printf("  Creativity: %.2f\n", (double)agent->creativity);
        printf("  Assertiveness: %.2f\n", (double)agent->assertiveness);

        NousAgent* assistant = (NousAgent*)g_assistant;
        if (!assistant) {
            g_assistant = agent;
            printf("Set as primary assistant.\n");
        }

        return 0;
    }

    // agent skill <skill_name>
    if (strcmp(argv[1], "skill") == 0) {
        if (argc < 3) {
            printf("Usage: agent skill <skill_name>\n");
            return -1;
        }
        NousAgent* assistant = (NousAgent*)g_assistant;
        if (!assistant) {
            printf("Error: no active assistant.\n");
            printf("Create an agent first with: agent create <name> <description>\n");
            return -1;
        }

        if (nous_agent_add_skill(assistant, argv[2]) == 0) {
            printf("Added skill \"%s\" to %s\n", argv[2], assistant->name);
        }
        return 0;
    }

    printf("Unknown subcommand: %s\n", argv[1]);
    printf("Use 'agent' without arguments to see help.\n");
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
    printf("  Confidence: %.2f\n", (double)intent->confidence);
    printf("  Urgency: %.2f\n", (double)intent->urgency);

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
        printf("Current urgency: %.2f\n", (double)nous_space_urgency(space));
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
        // Interactive theme selector with arrow keys and preview
        ThemeId selected = theme_select_interactive();
        if (selected != theme_get_current_id()) {
            theme_set(selected);
            theme_save();
            const Theme* t = theme_get();
            printf("Theme changed to: %s%s%s\n", t->prompt_name, t->name, theme_reset());
        } else {
            printf("Theme unchanged: %s\n", theme_get_name(selected));
        }
    }
    return 0;
}

// ============================================================================
// MODEL COMPARISON COMMANDS
// ============================================================================

int cmd_compare(int argc, char** argv) {
    // Get defaults from JSON config (or fallback)
    size_t default_count = 0;
    const char** default_models = models_get_compare_defaults(&default_count);

    if (argc < 2) {
        printf("\n\033[1mCommand: compare\033[0m - Compare models side-by-side\n\n");
        printf("\033[1mUsage:\033[0m\n");
        printf("  compare <prompt>                    # Uses default models\n");
        printf("  compare <prompt> <model1> <model2>  # Custom models\n\n");
        printf("\033[1mDefault models:\033[0m (most powerful from each provider)\n");
        for (size_t i = 0; i < default_count; i++) {
            printf("  - %s\n", default_models[i]);
        }
        printf("\n\033[1mExample:\033[0m\n");
        printf("  compare \"Explain quantum computing\"\n");
        printf("  compare \"Write a poem\" claude-opus-4 gpt-5\n\n");
        printf("\033[1mOptions:\033[0m\n");
        printf("  --no-diff      Skip diff generation\n");
        printf("  --json         Output as JSON\n");
        printf("  --sequential   Run sequentially instead of parallel\n\n");
        if (models_loaded_from_json()) {
            printf("\033[2mConfig: %s (v%s)\033[0m\n\n",
                   models_get_loaded_path(), models_get_version());
        }
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
        // Use default models from JSON config
        models_to_use = default_models;
        model_count = default_count;
        using_defaults = true;
        printf("\033[36mUsing default models:");
        for (size_t i = 0; i < default_count; i++) {
            printf(" %s%s", default_models[i], i < default_count - 1 ? "," : "");
        }
        printf("\033[0m\n\n");
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
    // Get defaults from JSON config
    const char* default_model = models_get_benchmark_default();
    size_t default_iterations = models_get_benchmark_iterations();

    if (argc < 2) {
        printf("\n\033[1mCommand: benchmark\033[0m - Benchmark a model's performance\n\n");
        printf("\033[1mUsage:\033[0m\n");
        printf("  benchmark <prompt>                    # Uses %s, %zu iterations\n",
               default_model, default_iterations);
        printf("  benchmark <prompt> <model>            # Custom model, %zu iterations\n",
               default_iterations);
        printf("  benchmark <prompt> <model> <N>        # Custom model, N iterations\n\n");
        printf("\033[1mDefaults:\033[0m\n");
        printf("  Model: %s\n", default_model);
        printf("  Iterations: %zu\n\n", default_iterations);
        printf("\033[1mExample:\033[0m\n");
        printf("  benchmark \"Write a haiku\"\n");
        printf("  benchmark \"Explain AI\" gpt-4o-mini 5\n\n");
        return 0;
    }

    const char* prompt = argv[1];
    const char* model = default_model;
    size_t iterations = default_iterations;

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

// ============================================================================
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
        printf("  project create <name> [--purpose \"...\"] [--team agent1,agent2] [--template name]\n");
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
            printf("Usage: project create <name> [--purpose \"...\"] [--team agent1,agent2] [--template name]\n");
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
            printf("%s%s", proj->team[i].agent_name,
                   i < proj->team_count - 1 ? ", " : "");
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

                printf("  %s \033[1m%-20s\033[0m ",
                       is_current ? "\033[32m●\033[0m" : " ",
                       p->name);

                // Show team members
                printf("\033[36m");
                for (size_t j = 0; j < p->team_count; j++) {
                    printf("%s%s", p->team[j].agent_name,
                           j < p->team_count - 1 ? ", " : "");
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
                printf("%s%s", proj->team[i].agent_name,
                       i < proj->team_count - 1 ? ", " : "");
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
        for (int i = header_len; i < 54; i++) printf("─");
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
                printf("│    • %-15s (%s)%-*s│\n",
                       proj->team[i].agent_name,
                       proj->team[i].role,
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
                       j < templates[i].team_count - 1 && templates[i].default_team[j + 1] ? ", " : "");
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
        size_t len = 0;
        for (int i = 2; i < argc && len < sizeof(focus) - 2; i++) {
            if (i > 2) focus[len++] = ' ';
            size_t arg_len = strlen(argv[i]);
            if (len + arg_len < sizeof(focus) - 1) {
                memcpy(focus + len, argv[i], arg_len);
                len += arg_len;
            }
        }

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
        size_t len = 0;
        for (int i = 2; i < argc && len < sizeof(decision) - 2; i++) {
            if (i > 2) decision[len++] = ' ';
            size_t arg_len = strlen(argv[i]);
            if (len + arg_len < sizeof(decision) - 1) {
                memcpy(decision + len, argv[i], arg_len);
                len += arg_len;
            }
        }

        project_add_decision(proj, decision);
        printf("\033[32m✓ Decision recorded: %s\033[0m\n", decision);
        return 0;
    }

    printf("Unknown project command: %s\n", subcommand);
    printf("Run 'project' without arguments for usage information.\n");
    return -1;
}

// ============================================================================
// SEMANTIC MEMORY COMMANDS
// ============================================================================

/**
 * /remember <text> - Store a memory with high importance
 */
int cmd_remember(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: remember <text to remember>\n");
        printf("Example: remember Roberto prefers clean, readable code\n");
        return -1;
    }

    // Join all arguments into one string
    char content[2048] = {0};
    size_t len = 0;
    for (int i = 1; i < argc && len < sizeof(content) - 2; i++) {
        if (i > 1) content[len++] = ' ';
        size_t arg_len = strlen(argv[i]);
        if (len + arg_len < sizeof(content) - 1) {
            memcpy(content + len, argv[i], arg_len);
            len += arg_len;
        }
    }

    // Create semantic node
    SemanticID id = nous_create_node(SEMANTIC_TYPE_MEMORY, content);
    if (id == SEMANTIC_ID_NULL) {
        printf("\033[31mError: Failed to store memory.\033[0m\n");
        return -1;
    }

    // Set high importance for explicitly remembered items
    sem_persist_update_importance(id, 0.9f);

    printf("\033[32m✓ Remembered:\033[0m \"%s\"\n", content);
    printf("\033[90mMemory ID: 0x%llx\033[0m\n", (unsigned long long)id);

    return 0;
}

/**
 * /search <query> - Search memories semantically
 */
int cmd_search(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: search <search query>\n");
        printf("Example: search what does Roberto prefer\n");
        return -1;
    }

    // Join all arguments into search query
    char query[1024] = {0};
    size_t len = 0;
    for (int i = 1; i < argc && len < sizeof(query) - 2; i++) {
        if (i > 1) query[len++] = ' ';
        size_t arg_len = strlen(argv[i]);
        if (len + arg_len < sizeof(query) - 1) {
            memcpy(query + len, argv[i], arg_len);
            len += arg_len;
        }
    }

    // Search by essence (keyword search for now)
    size_t count = 0;
    SemanticID* results = sem_persist_search_essence(query, 10, &count);

    if (!results || count == 0) {
        printf("\033[33mNo memories found for:\033[0m \"%s\"\n", query);
        return 0;
    }

    printf("\033[1mFound %zu matching memories:\033[0m\n\n", count);

    for (size_t i = 0; i < count; i++) {
        NousSemanticNode* node = nous_get_node(results[i]);
        if (node && node->essence) {
            printf("  \033[36m[%zu]\033[0m %s\n", i + 1, node->essence);
            printf("      \033[90mID: 0x%llx | Importance: %.2f\033[0m\n",
                   (unsigned long long)node->id, node->importance);
            nous_release_node(node);
        }
    }

    free(results);
    return 0;
}

/**
 * /memories - List recent and important memories
 */
int cmd_memories(int argc, char** argv) {
    (void)argc; (void)argv;

    // Get graph statistics
    GraphStats stats = sem_persist_get_stats();

    printf("\033[1m📚 Knowledge Graph\033[0m\n");
    printf("   Total nodes: %zu\n", stats.total_nodes);
    printf("   Total relations: %zu\n", stats.total_relations);
    printf("   Nodes in memory: %zu\n", stats.nodes_in_memory);
    printf("\n");

    // Load most important memories
    size_t count = 0;
    SemanticID* important = sem_persist_load_important(10, 0.5f, &count);

    if (important && count > 0) {
        printf("\033[1m⭐ Most Important Memories:\033[0m\n\n");
        for (size_t i = 0; i < count; i++) {
            NousSemanticNode* node = nous_get_node(important[i]);
            if (node && node->essence) {
                // Truncate long essences
                char display[80];
                if (strlen(node->essence) > 75) {
                    snprintf(display, sizeof(display), "%.72s...", node->essence);
                } else {
                    snprintf(display, sizeof(display), "%s", node->essence);
                }
                printf("  \033[36m[%zu]\033[0m %s\n", i + 1, display);
                printf("      \033[90mImportance: %.2f | Accessed: %llu times\033[0m\n",
                       node->importance, (unsigned long long)node->access_count);
                nous_release_node(node);
            }
        }
        free(important);
    } else {
        printf("\033[33mNo memories stored yet.\033[0m\n");
        printf("Use \033[1mremember <text>\033[0m to store your first memory!\n");
    }

    return 0;
}

/**
 * /forget <id> - Delete a memory by ID
 */
int cmd_forget(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: forget <memory_id>\n");
        printf("Example: forget 0x1234567890abcdef\n");
        printf("\nUse 'memories' or 'recall' to find memory IDs.\n");
        return -1;
    }

    // Parse hex ID
    SemanticID id = 0;
    if (strncmp(argv[1], "0x", 2) == 0 || strncmp(argv[1], "0X", 2) == 0) {
        id = strtoull(argv[1] + 2, NULL, 16);
    } else {
        id = strtoull(argv[1], NULL, 16);
    }

    if (id == 0) {
        printf("\033[31mError: Invalid memory ID.\033[0m\n");
        return -1;
    }

    // Check if it exists
    if (!sem_persist_node_exists(id)) {
        printf("\033[31mError: Memory 0x%llx not found.\033[0m\n", (unsigned long long)id);
        return -1;
    }

    // Delete from persistence
    int rc = sem_persist_delete_node(id);
    if (rc != 0) {
        printf("\033[31mError: Failed to delete memory.\033[0m\n");
        return -1;
    }

    // Also delete from in-memory fabric (if loaded)
    nous_delete_node(id);

    printf("\033[32m✓ Forgotten memory 0x%llx\033[0m\n", (unsigned long long)id);
    return 0;
}

/**
 * /graph - Show knowledge graph statistics
 */
int cmd_graph(int argc, char** argv) {
    (void)argc; (void)argv;

    GraphStats stats = sem_persist_get_stats();

    printf("\033[1m🧠 Semantic Knowledge Graph\033[0m\n\n");

    printf("  \033[36mNodes\033[0m\n");
    printf("    Total in database:    %zu\n", stats.total_nodes);
    printf("    Loaded in memory:     %zu\n", stats.nodes_in_memory);
    printf("\n");

    printf("  \033[36mRelations\033[0m\n");
    printf("    Total connections:    %zu\n", stats.total_relations);
    printf("\n");

    printf("  \033[36mNodes by Type\033[0m\n");
    const char* type_names[] = {
        "Void", "Concept", "Entity", "Relation", "Intent",
        "Agent", "Space", "Event", "Feeling", "Memory", "Pattern"
    };
    for (int i = 0; i < 11 && i < 16; i++) {
        if (stats.nodes_by_type[i] > 0) {
            printf("    %-12s: %zu\n", type_names[i], stats.nodes_by_type[i]);
        }
    }

    return 0;
}
