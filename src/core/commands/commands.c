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
#include "nous/todo.h"
#include "nous/notify.h"
#include "nous/mcp_client.h"
#include "nous/plan_db.h"
#include "nous/output_service.h"
#include "nous/workflow.h"
#include "nous/edition.h"
#include "../../auth/oauth.h"
#include <cjson/cJSON.h>
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

// Forward declarations for todo and reminder commands
int cmd_todo(int argc, char** argv);
int cmd_remind(int argc, char** argv);
int cmd_reminders(int argc, char** argv);

// Forward declarations for git/test workflow commands
int cmd_test(int argc, char** argv);
int cmd_git(int argc, char** argv);
int cmd_pr(int argc, char** argv);

// Forward declarations for daemon and MCP commands
int cmd_daemon(int argc, char** argv);
int cmd_mcp(int argc, char** argv);

// Forward declaration for style command
int cmd_style(int argc, char** argv);

// Forward declaration for plan command
int cmd_plan(int argc, char** argv);

// Forward declaration for output command
int cmd_output(int argc, char** argv);

// Forward declaration for workflow command (implemented in workflow.c)
extern int cmd_workflow(int argc, char** argv);

// Forward declarations for Education Pack commands
int cmd_education(int argc, char** argv);
int cmd_study(int argc, char** argv);
int cmd_homework(int argc, char** argv);
int cmd_quiz(int argc, char** argv);
int cmd_flashcards(int argc, char** argv);
int cmd_mindmap(int argc, char** argv);
int cmd_libretto(int argc, char** argv);
int cmd_voice(int argc, char** argv);

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
    {"style",       "Set response style (flash/concise/balanced/detailed)", cmd_style},
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
    // Todo manager (Anna Executive Assistant)
    {"todo",        "Manage tasks and reminders",        cmd_todo},
    {"remind",      "Quick reminder: /remind <msg> <when>", cmd_remind},
    {"reminders",   "Show upcoming reminders",           cmd_reminders},
    // Git/Test workflow commands
    {"test",        "Run project tests (auto-detect framework)", cmd_test},
    {"git",         "Git workflow helper (status/commit/push)", cmd_git},
    {"pr",          "Create pull request via gh CLI",    cmd_pr},
    // Anna Executive Assistant - Daemon & MCP
    {"daemon",      "Manage notification daemon",        cmd_daemon},
    {"mcp",         "Manage MCP servers and tools",      cmd_mcp},
    // Execution Plan management
    {"plan",        "Manage execution plans (list/status/export)", cmd_plan},
    // Output Service
    {"output",      "Manage generated outputs (list/open/delete)", cmd_output},
    // Workflow orchestration
    {"workflow",    "Manage workflows (list/show/execute/resume)", cmd_workflow},
    // Education Pack commands
    {"education",   "Education setup and management",    cmd_education},
    {"study",       "Start a study session with a maestro", cmd_study},
    {"homework",    "Get help with homework (anti-cheating)", cmd_homework},
    {"quiz",        "Generate adaptive quizzes",         cmd_quiz},
    {"flashcards",  "Create and review flashcards",      cmd_flashcards},
    {"mindmap",     "Generate visual mind maps",         cmd_mindmap},
    {"libretto",    "Student gradebook and activity log", cmd_libretto},
    {"voice",       "Conversational voice mode with maestri", cmd_voice},
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
        "search",
        "search <query>",
        "Search memories semantically",
        "Searches the knowledge graph for memories matching your query.\n"
        "Returns up to 10 matching results ordered by importance.\n\n"
        "This is an alias for 'recall <query>' with the same functionality.",
        "search Roberto preferences\n"
        "search API documentation\n"
        "search coding style"
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
    {
        "test",
        "test",
        "Run project tests with auto-detected framework",
        "Automatically detects and runs tests for your project.\n\n"
        "SUPPORTED FRAMEWORKS:\n"
        "  - make test     (Makefile with 'test' target)\n"
        "  - cargo test    (Rust - Cargo.toml)\n"
        "  - go test       (Go - go.mod)\n"
        "  - npm test      (Node.js - package.json)\n"
        "  - pytest        (Python - pytest.ini/pyproject.toml/tests/)\n\n"
        "The command auto-detects which framework to use based on\n"
        "project files in the current directory.",
        "/test    # Run tests with auto-detected framework"
    },
    {
        "git",
        "git [status|commit|push|sync] [args]",
        "Git workflow helper commands",
        "Simplified git workflow commands for common operations.\n\n"
        "SUBCOMMANDS:\n"
        "  status, s       Show git status and recent commits\n"
        "  commit, c <msg> Stage all changes and commit\n"
        "  push, p         Push to remote\n"
        "  sync            Pull --rebase and push\n\n"
        "The commit command automatically adds the Claude Code signature.",
        "/git status\n"
        "/git commit Fix login bug\n"
        "/git push\n"
        "/git sync"
    },
    {
        "pr",
        "pr [title]",
        "Create pull request via GitHub CLI",
        "Creates a pull request using the 'gh' CLI tool.\n\n"
        "REQUIREMENTS:\n"
        "  - GitHub CLI (gh) must be installed and authenticated\n"
        "  - Must be on a feature branch (not main/master)\n\n"
        "If no title is provided, generates one from branch name.\n"
        "Automatically pushes branch before creating PR.",
        "/pr Add user authentication\n"
        "/pr    # Uses branch name as title"
    },
    {
        "todo",
        "todo <add|list|done|start|delete|inbox|search|stats> [args]",
        "Native task manager with reminders",
        "A local task manager with SQLite storage and natural language dates.\n\n"
        "SUBCOMMANDS:\n"
        "  add <title> [--due <date>] [--remind <time>] [--priority <1-3>] [--context <ctx>]\n"
        "              Add a new task\n"
        "  list [today|overdue|upcoming|all]\n"
        "              List tasks (default: pending)\n"
        "  done <id>   Mark task as completed\n"
        "  start <id>  Mark task as in progress\n"
        "  delete <id> Delete a task\n"
        "  inbox [text] Quick capture or list inbox items\n"
        "  search <q>  Full-text search across tasks\n"
        "  stats       Show task statistics\n\n"
        "DATE FORMATS:\n"
        "  Natural: tomorrow, tonight, next monday, in 2 hours\n"
        "  Italian: domani, stasera, lunedi prossimo, tra 2 ore\n"
        "  Specific: dec 25, 2025-12-25, at 3pm, alle 14",
        "/todo add \"Review PR\" --due tomorrow --remind 1h\n"
        "/todo list today\n"
        "/todo done 5\n"
        "/todo inbox \"Call dentist\"\n"
        "/todo search meeting"
    },
    {
        "remind",
        "remind <message> <when> [--note <context>]",
        "Quick reminder creation",
        "Create reminders quickly with natural language.\n"
        "The order of message and time is flexible - Anna figures it out.\n\n"
        "TIME FORMATS:\n"
        "  Time of day: tonight, tomorrow morning, tomorrow evening\n"
        "  Specific: at 3pm, at 15:00, alle 14\n"
        "  Relative: in 30 minutes, in 2 hours, tra 2 ore\n"
        "  Weekdays: next monday, lunedi prossimo\n"
        "  Dates: dec 15, 2025-12-25\n\n"
        "Add context with --note for extra details.",
        "/remind \"Call mom\" tomorrow morning\n"
        "/remind tonight \"Buy groceries\"\n"
        "/remind \"Team meeting\" next tuesday at 10am\n"
        "/remind \"Review PR\" tomorrow --note \"Check auth changes in #123\""
    },
    {
        "reminders",
        "reminders [today|week|all]",
        "View upcoming reminders",
        "Shows scheduled reminders filtered by time range.\n\n"
        "FILTERS:\n"
        "  today   Today's reminders (default)\n"
        "  week    Next 7 days\n"
        "  all     All scheduled reminders\n\n"
        "Use /todo done <id> to mark complete\n"
        "Use /todo delete <id> to remove",
        "/reminders        # Today's reminders\n"
        "/reminders week   # Next 7 days\n"
        "/reminders all    # All scheduled"
    },
    {
        "daemon",
        "daemon <command>",
        "Manage the notification daemon",
        "Controls the background daemon for delivering scheduled reminders.\n\n"
        "COMMANDS:\n"
        "  start       Start the daemon\n"
        "  stop        Stop the daemon\n"
        "  restart     Restart the daemon\n"
        "  status      Show daemon status (running/stopped)\n"
        "  health      Show detailed health info\n"
        "  install     Install LaunchAgent for auto-start\n"
        "  uninstall   Remove LaunchAgent\n"
        "  test        Send a test notification\n\n"
        "The daemon delivers notifications even when Convergio isn't running.\n"
        "Uses terminal-notifier or osascript with automatic fallback.",
        "/daemon start     # Start the daemon\n"
        "/daemon status    # Check if running\n"
        "/daemon health    # Detailed health info\n"
        "/daemon install   # Auto-start at login\n"
        "/daemon test      # Send test notification"
    },
    {
        "mcp",
        "mcp <command> [args]",
        "Manage MCP server connections",
        "Controls connections to Model Context Protocol (MCP) servers.\n\n"
        "COMMANDS:\n"
        "  list              List all configured servers\n"
        "  status            Show connection status\n"
        "  health            Show detailed health info\n"
        "  connect <name>    Connect to a specific server\n"
        "  disconnect <name> Disconnect from a server\n"
        "  connect-all       Connect to all enabled servers\n"
        "  enable <name>     Enable a server\n"
        "  disable <name>    Disable a server\n"
        "  tools             List all tools from connected servers\n"
        "  tools <server>    List tools from specific server\n"
        "  call <tool> [json] Call a tool with arguments\n\n"
        "Configuration: ~/.convergio/mcp.json",
        "/mcp list                    # Show configured servers\n"
        "/mcp connect filesystem      # Connect to server\n"
        "/mcp tools                   # List available tools\n"
        "/mcp call read_file '{\"path\":\"/tmp/test.txt\"}'"
    },
    {
        "plan",
        "plan <subcommand> [args]",
        "Manage execution plans",
        "View, export, and manage multi-step execution plans.\n"
        "Plans are created automatically when agents work on complex tasks.\n\n"
        "Subcommands:\n"
        "  list              List all plans with status and progress\n"
        "  status <id>       Show detailed plan status with tasks\n"
        "  export <id>       Export plan to markdown file\n"
        "  delete <id>       Delete a plan\n"
        "  cleanup [days]    Clean up old plans (default: 30 days)\n\n"
        "Plans are stored in ~/.convergio/plans.db (SQLite)",
        "/plan list                   # Show all plans\n"
        "/plan status abc123          # Show plan details\n"
        "/plan export abc123          # Export to /tmp/plan-abc123.md\n"
        "/plan cleanup 7              # Delete plans older than 7 days"
    },
    {
        "workflow",
        "workflow <list|show|execute|resume> [args]",
        "Manage workflow orchestration",
        "Workflow orchestration system for multi-agent coordination.\n\n"
        "Subcommands:\n"
        "  list                    List all available workflows\n"
        "  show <name>             Show workflow details and Mermaid diagram\n"
        "  execute <name> [input]  Execute a workflow with optional input\n"
        "  resume <id> [checkpoint] Resume workflow from checkpoint\n\n"
        "Workflows enable complex multi-step agent collaboration with\n"
        "state machine execution, checkpointing, and conditional routing.",
        "workflow list\n"
        "workflow show code-review\n"
        "workflow execute parallel-analysis \"Analyze this project\"\n"
        "workflow resume 12345\n"
        "workflow resume 12345 2"
    },
    {
        "output",
        "output <subcommand> [args]",
        "Manage generated outputs",
        "Browse, open, and manage generated output documents.\n"
        "Outputs include reports, analyses, and diagrams created by agents.\n\n"
        "Subcommands:\n"
        "  list              List recent outputs\n"
        "  latest            Show the most recent output\n"
        "  open <path>       Open an output file in default app\n"
        "  delete <path>     Delete an output file\n"
        "  size              Show total storage used by outputs\n"
        "  cleanup [days]    Clean up old outputs (default: 30 days)\n\n"
        "Outputs are stored in ~/.convergio/outputs/",
        "/output list                 # Show recent outputs\n"
        "/output latest               # Show the latest output\n"
        "/output open /path/to/file   # Open in default app\n"
        "/output size                 # Show disk usage\n"
        "/output cleanup 7            # Delete outputs older than 7 days"
    },
    {
        "style",
        "style [flash|concise|balanced|detailed]",
        "Set response style",
        "Controls how verbose AI responses are.\n\n"
        "STYLES:\n"
        "  flash       Ultra-concise, immediate answers\n"
        "  concise     Brief, to-the-point responses\n"
        "  balanced    Moderate detail (default)\n"
        "  detailed    Comprehensive, thorough explanations\n\n"
        "Without arguments, shows the current style setting.",
        "/style              # Show current style\n"
        "/style flash        # Set to ultra-concise\n"
        "/style detailed     # Set to comprehensive"
    },
    // Education Pack commands
    {
        "education",
        "education [setup|quick|profile|progress]",
        "Education Pack setup and management",
        "Manage student profiles and education settings.\n\n"
        "Subcommands:\n"
        "  setup          Run the interactive setup wizard\n"
        "  quick <name> <curriculum> <grade>  Quick profile creation\n"
        "  profile        Show current student profile\n"
        "  progress       Show learning progress\n\n"
        "Available curricula:\n"
        "  elementari, scuola_media, liceo_scientifico, liceo_classico,\n"
        "  liceo_linguistico, liceo_artistico, iti_informatica",
        "/education                           # Show current profile\n"
        "/education setup                     # Interactive wizard\n"
        "/education quick Mario liceo_scientifico 1\n"
        "/education profile                   # Detailed profile view"
    },
    {
        "study",
        "study <subject> [topic]",
        "Start a study session with Pomodoro timer",
        "Start a focused study session with a maestro.\n\n"
        "Features:\n"
        "  - 25-minute focused work sessions (Pomodoro)\n"
        "  - 5-minute breaks (15 min after 4 pomodoros)\n"
        "  - Native macOS notifications\n"
        "  - End-of-session review quiz\n"
        "  - Automatic time tracking in libretto",
        "/study matematica                    # Study math\n"
        "/study fisica \"moto rettilineo\"      # Study specific topic\n"
        "/study italiano                      # Study Italian"
    },
    {
        "homework",
        "homework <description>",
        "Get help with homework (anti-cheating mode)",
        "Get guided help understanding homework without answers.\n\n"
        "Features:\n"
        "  - Socratic method - guiding questions only\n"
        "  - 5-level progressive hint system (0=subtle, 4=detailed)\n"
        "  - Understanding verification quiz\n"
        "  - Parental transparency log\n\n"
        "The system helps you UNDERSTAND, not do your homework for you.",
        "/homework Matematica: risolvere 3x + 5 = 14\n"
        "/homework Storia: cause della Rivoluzione Francese\n"
        "/homework-hint 2                     # Get level 2 hint"
    },
    {
        "quiz",
        "quiz <topic> [--count n] [--difficulty easy|medium|hard]",
        "Generate adaptive quizzes",
        "Generate quizzes with multiple question types.\n\n"
        "Question types:\n"
        "  - Multiple choice\n"
        "  - True/False\n"
        "  - Open answer\n"
        "  - Sequence ordering\n"
        "  - Matching pairs\n"
        "  - Fill in the blanks\n\n"
        "Grades are automatically saved to the libretto.",
        "/quiz frazioni                       # 5 questions (default)\n"
        "/quiz \"equazioni\" --count 10         # 10 questions\n"
        "/quiz storia --difficulty easy       # Easy difficulty"
    },
    {
        "flashcards",
        "flashcards <topic> [--count n] [--export anki|pdf]",
        "Create and review flashcards with spaced repetition",
        "Study with flashcards using the SM-2 algorithm.\n\n"
        "Features:\n"
        "  - SM-2 spaced repetition algorithm (Anki-like)\n"
        "  - Text-to-speech support\n"
        "  - Terminal UI for study sessions\n"
        "  - Export to Anki (.apkg) or PDF\n\n"
        "Rate your recall: 0 (forgot) to 5 (perfect)",
        "/flashcards \"verbi latini\"           # Create 10 flashcards\n"
        "/flashcards vocabolario --count 20   # Create 20 flashcards\n"
        "/flashcards storia --export anki     # Export to Anki"
    },
    {
        "mindmap",
        "mindmap <concept> [--format svg|png|pdf] [--output path]",
        "Generate visual mind maps",
        "Generate Mermaid.js mind maps from any concept.\n\n"
        "Features:\n"
        "  - LLM-powered content generation\n"
        "  - Export to SVG, PNG, or PDF\n"
        "  - Accessibility adaptations\n"
        "  - Auto-opens in browser",
        "/mindmap \"French Revolution\"          # Generate and open\n"
        "/mindmap photosynthesis --format png # Export as PNG\n"
        "/mindmap \"theory of relativity\" --output ~/Desktop/relativity.svg"
    },
    {
        "libretto",
        "libretto [grades|diary|progress|average]",
        "Student gradebook and activity log",
        "View your complete student record (libretto).\n\n"
        "Subcommands:\n"
        "  (none)     Dashboard summary (last 30 days)\n"
        "  grades     Grade history by subject\n"
        "  diary      Daily activity log\n"
        "  progress   Progress graphs and trends\n"
        "  average    Grade averages by subject\n\n"
        "Grades from quizzes are automatically recorded.",
        "/libretto                            # Dashboard\n"
        "/libretto grades                     # All grades\n"
        "/libretto grades mathematics         # Math grades only\n"
        "/libretto diary 14                   # Last 14 days activity\n"
        "/libretto average                    # Subject averages"
    },
    {
        "voice",
        "voice [maestro] [topic]",
        "Conversational voice mode with AI maestri",
        "Start natural conversational voice mode with a maestro.\n\n"
        "Like ChatGPT Advanced Voice Mode:\n"
        "  - Natural conversation flow\n"
        "  - Interrupt anytime (barge-in)\n"
        "  - No push-to-talk needed\n"
        "  - Real-time transcript\n\n"
        "During voice mode:\n"
        "  ESC - Exit voice mode\n"
        "  M   - Toggle mute microphone\n"
        "  T   - Toggle transcript display\n"
        "  S   - Save conversation\n\n"
        "Requires: make VOICE=1 (voice feature enabled)\n"
        "Dependency: brew install libwebsockets openssl",
        "/voice                               # With default maestro\n"
        "/voice feynman                       # With Feynman\n"
        "/voice euclide geometria             # Euclide on geometry"
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

// Education Edition help
static void print_help_education(void) {
    printf("\n");
    printf("\033[32m┌──────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[32m│  \033[1;37mCONVERGIO EDUCATION\033[0;32m - Learn from History's Greatest       │\033[0m\n");
    printf("\033[32m└──────────────────────────────────────────────────────────────┘\033[0m\n\n");

    // 1. YOUR TEACHERS - The 15 Maestri
    printf("\033[1;33m📚 YOUR TEACHERS\033[0m  \033[2m(15 historical maestri ready to teach)\033[0m\n");
    printf("   \033[36m@ali\033[0m               Principal - guides your learning journey\n");
    printf("   \033[36m@euclide\033[0m           Mathematics - logic, geometry, algebra\n");
    printf("   \033[36m@feynman\033[0m           Physics - makes complex ideas simple\n");
    printf("   \033[36m@manzoni\033[0m           Italian - literature and storytelling\n");
    printf("   \033[36m@darwin\033[0m            Sciences - observation and curiosity\n");
    printf("   \033[36m@erodoto\033[0m           History - bringing the past alive\n");
    printf("   \033[36m@humboldt\033[0m          Geography - nature and culture\n");
    printf("   \033[36m@leonardo\033[0m          Art - creativity and observation\n");
    printf("   \033[36m@shakespeare\033[0m       English - language and expression\n");
    printf("   \033[36m@mozart\033[0m            Music - joy of musical creation\n");
    printf("   \033[36m@cicerone\033[0m          Civics/Latin - rhetoric and citizenship\n");
    printf("   \033[36m@smith\033[0m             Economics - understanding markets\n");
    printf("   \033[36m@lovelace\033[0m          Computer Science - computational thinking\n");
    printf("   \033[36m@ippocrate\033[0m         Health - wellness and body care\n");
    printf("   \033[36m@socrate\033[0m           Philosophy - asking the right questions\n");
    printf("   \033[36m@chris\033[0m             Storytelling - narrative and communication\n");
    printf("   \033[36magents\033[0m             See all teachers and their specialties\n");
    printf("   \033[2m   Tip: @ali or 'back' returns to the Principal\033[0m\n\n");

    // 2. STUDY TOOLS
    printf("\033[1;33m📖 STUDY TOOLS\033[0m  \033[2m(interactive learning features)\033[0m\n");
    printf("   \033[36meducation\033[0m          Enter Education mode (all features)\n");
    printf("   \033[36mstudy <topic>\033[0m      Start a study session on any topic\n");
    printf("   \033[36mhomework <desc>\033[0m    Get help with your homework\n");
    printf("   \033[36mquiz <topic>\033[0m       Test your knowledge with a quiz\n");
    printf("   \033[36mflashcards <topic>\033[0m Create and practice flashcards\n");
    printf("   \033[36mmindmap <topic>\033[0m    Generate a visual mind map\n\n");

    // 3. LANGUAGE TOOLS
    printf("\033[1;33m🗣️  LANGUAGE TOOLS\033[0m  \033[2m(vocabulary and grammar)\033[0m\n");
    printf("   \033[36mdefine <word>\033[0m      Get definition with examples\n");
    printf("   \033[36mconjugate <verb>\033[0m   Show verb conjugations\n");
    printf("   \033[36mpronounce <word>\033[0m   Learn pronunciation\n");
    printf("   \033[36mgrammar <topic>\033[0m    Explain grammar rules\n\n");

    // 4. PROGRESS TRACKING
    printf("\033[1;33m📊 PROGRESS TRACKING\033[0m  \033[2m(your learning journey)\033[0m\n");
    printf("   \033[36mlibretto\033[0m           View your digital report card\n");
    printf("   \033[36mxp\033[0m                 Check your experience points\n");
    printf("   \033[2m   Tip: Complete quizzes and study sessions to earn XP!\033[0m\n\n");

    // 5. SPECIAL FEATURES
    printf("\033[1;33m✨ SPECIAL FEATURES\033[0m\n");
    printf("   \033[36mvoice\033[0m              Enable voice mode (text-to-speech)\n");
    printf("   \033[36mhtml <topic>\033[0m       Generate interactive HTML content\n");
    printf("   \033[36mcalc\033[0m               Scientific calculator\n");
    printf("   \033[36mperiodic\033[0m           Interactive periodic table\n");
    printf("   \033[36mconvert <expr>\033[0m     Unit converter (5km to miles)\n");
    printf("   \033[36mvideo <topic>\033[0m      Search educational videos\n\n");

    // 6. ORGANIZATION
    printf("\033[1;33m📅 ORGANIZATION\033[0m  \033[2m(Anna helps you stay organized)\033[0m\n");
    printf("   \033[36m@anna\033[0m              Ask Anna for help with scheduling\n");
    printf("   \033[36mtodo\033[0m               View your task list\n");
    printf("   \033[36mtodo add <task>\033[0m    Add homework or study tasks\n");
    printf("   \033[36mremind <time> <msg>\033[0m Set study reminders\n\n");

    // 7. SYSTEM
    printf("\033[1;33m⚙️  SYSTEM\033[0m\n");
    printf("   \033[36mstatus\033[0m             System health & active teachers\n");
    printf("   \033[36mtheme\033[0m              Change colors and appearance\n");
    printf("   \033[36msetup\033[0m              Configure your settings\n");
    printf("   \033[36mcost\033[0m               Track API usage\n\n");

    printf("\033[2m───────────────────────────────────────────────────────────────────\033[0m\n");
    printf("\033[2mType \033[0mhelp <command>\033[2m for details  •  Or ask your teacher!\033[0m\n\n");
}

// Master/Full Edition help
static void print_help_master(void) {
    printf("\n");
    printf("\033[36m┌──────────────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[36m│  \033[1;37mCONVERGIO\033[0;36m - Your AI Team with Human Purpose                 │\033[0m\n");
    printf("\033[36m└──────────────────────────────────────────────────────────────┘\033[0m\n\n");

    // 1. YOUR AI TEAM - The most important feature
    printf("\033[1;33m🤖 YOUR AI TEAM\033[0m  \033[2m(53 specialized agents ready to help)\033[0m\n");
    printf("   \033[36m@ali\033[0m               Chief of Staff - orchestrates everything\n");
    printf("   \033[36m@baccio\033[0m            Software Architect\n");
    printf("   \033[36m@marco\033[0m             Senior Developer\n");
    printf("   \033[36m@jenny\033[0m             Accessibility Expert\n");
    printf("   \033[36m@<name>\033[0m            Switch to talk with agent (Tab autocomplete)\n");
    printf("   \033[36m@<name> message\033[0m    Send message directly to agent\n");
    printf("   \033[36magents\033[0m             See all 53 agents with their specialties\n");
    printf("   \033[2m   Tip: @ali or 'back' returns to Ali from any agent\033[0m\n\n");

    // ANNA - Executive Assistant
    printf("\033[1;33m👩‍💼 ANNA - Executive Assistant\033[0m  \033[2m(your personal productivity hub)\033[0m\n");
    printf("   \033[36m@anna\033[0m                  Switch to Anna for task management\n");
    printf("   \033[36m@anna <task>\033[0m           Send task to Anna (IT/EN supported)\n");
    printf("   \033[36mtodo\033[0m / \033[36mtodo list\033[0m      List your tasks with priorities\n");
    printf("   \033[36mtodo add <task>\033[0m        Add a new task (supports @agent delegation)\n");
    printf("   \033[36mtodo done <id>\033[0m         Mark task as completed\n");
    printf("   \033[36mremind <time> <msg>\033[0m    Set reminders (e.g., remind 10m call Bob)\n");
    printf("   \033[36mreminders\033[0m              List pending reminders\n");
    printf("   \033[36mdaemon start\033[0m           Background agent for scheduled tasks\n");
    printf("   \033[2m   Tip: Anna speaks Italian too! \"ricordami tra 5 minuti\"\033[0m\n\n");

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
}

int cmd_help(int argc, char** argv) {
    // If a specific command is requested, show detailed help
    if (argc >= 2) {
        // Check if command is available in current edition
        if (!edition_has_command(argv[1])) {
            printf("\n\033[33mCommand '%s' is not available in %s.\033[0m\n\n",
                   argv[1], edition_display_name());
            return -1;
        }

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

    // Show edition-specific general help
    switch (edition_current()) {
        case EDITION_EDUCATION:
            print_help_education();
            break;
        case EDITION_BUSINESS:
        case EDITION_DEVELOPER:
            // TODO: Add Business and Developer specific help
            print_help_master();
            break;
        default:
            print_help_master();
            break;
    }

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
// RESPONSE STYLE COMMAND
// ============================================================================

// Style definitions
typedef struct {
    const char* name;
    const char* description;
    int max_tokens;
    double temperature;
    bool markdown;
} StyleDef;

static const StyleDef STYLES[] = {
    {"flash",    "Ultra fast, direct answers, no formatting",    1024,  0.3, false},
    {"concise",  "Brief but formatted, good balance",            2048,  0.5, true},
    {"balanced", "Default, equilibrated detail and speed",       4096,  0.7, true},
    {"detailed", "In-depth analysis, maximum detail",            8192,  0.8, true},
};
#define STYLE_COUNT 4

static const StyleDef* style_get_def(const char* name) {
    for (int i = 0; i < STYLE_COUNT; i++) {
        if (strcmp(STYLES[i].name, name) == 0) {
            return &STYLES[i];
        }
    }
    return NULL;
}

int cmd_style(int argc, char** argv) {
    const char* current = convergio_config_get("style");
    if (!current) current = "balanced";

    if (argc > 1) {
        // Set style
        const StyleDef* def = style_get_def(argv[1]);
        if (def) {
            convergio_config_set("style", argv[1]);
            convergio_config_save();
            printf("\n\033[1mStyle changed to: %s\033[0m\n", def->name);
            printf("  %s\n", def->description);
            printf("  Max tokens: %d | Temperature: %.1f | Markdown: %s\n\n",
                   def->max_tokens, def->temperature, def->markdown ? "yes" : "no");
        } else {
            printf("\033[31mUnknown style: %s\033[0m\n\n", argv[1]);
            printf("Available styles:\n");
            for (int i = 0; i < STYLE_COUNT; i++) {
                printf("  \033[1m%-10s\033[0m %s\n", STYLES[i].name, STYLES[i].description);
            }
            printf("\n");
        }
    } else {
        // Show current style and options
        printf("\n\033[1mResponse Style Configuration\033[0m\n\n");
        printf("Current style: \033[1;36m%s\033[0m\n\n", current);
        printf("Available styles:\n");
        for (int i = 0; i < STYLE_COUNT; i++) {
            const char* marker = strcmp(STYLES[i].name, current) == 0 ? " *" : "  ";
            printf("%s\033[1m%-10s\033[0m %s\n", marker, STYLES[i].name, STYLES[i].description);
            printf("              tokens: %d | temp: %.1f | markdown: %s\n",
                   STYLES[i].max_tokens, STYLES[i].temperature,
                   STYLES[i].markdown ? "yes" : "no");
        }
        printf("\nUsage: /style <name>\n\n");
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

    // Set high importance for explicitly remembered items (both DB and in-memory)
    sem_persist_update_importance(id, 0.9f);
    NousSemanticNode* node = nous_get_node(id);
    if (node) {
        node->importance = 0.9f;
        nous_release_node(node);
    }

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
                   (unsigned long long)node->id, (double)node->importance);
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
                       (double)node->importance, (unsigned long long)node->access_count);
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

    // Parse hex ID with proper error checking
    char* endptr = NULL;
    const char* num_str = argv[1];
    if (strncmp(num_str, "0x", 2) == 0 || strncmp(num_str, "0X", 2) == 0) {
        num_str += 2;
    }

    errno = 0;
    SemanticID id = strtoull(num_str, &endptr, 16);

    // Check for parsing errors: no digits parsed, or trailing garbage
    if (endptr == num_str || *endptr != '\0' || errno == ERANGE) {
        printf("\033[31mError: Invalid memory ID format. Use hex format like 0x1234.\033[0m\n");
        return -1;
    }

    // Note: id == 0 is technically valid but very unlikely to be a real ID
    if (id == 0) {
        printf("\033[33mWarning: ID 0 is unusual. Proceeding anyway.\033[0m\n");
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

// ============================================================================
// GIT/TEST WORKFLOW COMMANDS (Issue #15)
// ============================================================================

/**
 * /test - Run project tests with auto-detection of test framework
 */
int cmd_test(int argc, char** argv) {
    (void)argc; (void)argv;

    // Check what test frameworks/files exist
    bool has_makefile = (access("Makefile", F_OK) == 0);
    bool has_package_json = (access("package.json", F_OK) == 0);
    bool has_cargo_toml = (access("Cargo.toml", F_OK) == 0);
    bool has_go_mod = (access("go.mod", F_OK) == 0);
    bool has_pytest = (access("pytest.ini", F_OK) == 0 || access("pyproject.toml", F_OK) == 0);
    bool has_tests_dir = (access("tests", F_OK) == 0);

    const char* cmd = NULL;
    const char* framework = NULL;

    // Priority: explicit test directory structure first
    if (has_makefile) {
        // Check if make test target exists
        FILE* fp = popen("make -n test 2>/dev/null", "r");
        if (fp) {
            char buf[256];
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                cmd = "make test";
                framework = "make";
            }
            pclose(fp);
        }
    }

    if (!cmd && has_cargo_toml) {
        cmd = "cargo test";
        framework = "cargo";
    }

    if (!cmd && has_go_mod) {
        cmd = "go test ./...";
        framework = "go";
    }

    if (!cmd && has_package_json) {
        cmd = "npm test";
        framework = "npm";
    }

    if (!cmd && (has_pytest || has_tests_dir)) {
        cmd = "python3 -m pytest -v";
        framework = "pytest";
    }

    if (!cmd) {
        printf("\033[33m⚠ No test framework detected.\033[0m\n\n");
        printf("Supported frameworks:\n");
        printf("  • make test     (Makefile with 'test' target)\n");
        printf("  • cargo test    (Rust - Cargo.toml)\n");
        printf("  • go test       (Go - go.mod)\n");
        printf("  • npm test      (Node.js - package.json)\n");
        printf("  • pytest        (Python - pytest.ini/pyproject.toml/tests/)\n");
        return -1;
    }

    printf("\033[1;36m🧪 Running tests with %s\033[0m\n", framework);
    printf("  Command: %s\n\n", cmd);

    int result = system(cmd);

    printf("\n");
    if (result == 0) {
        printf("\033[32m✓ Tests passed!\033[0m\n");
    } else {
        printf("\033[31m✗ Tests failed (exit code: %d)\033[0m\n", WEXITSTATUS(result));
    }

    return result == 0 ? 0 : -1;
}

/**
 * /git - Git workflow helper
 */
int cmd_git(int argc, char** argv) {
    // Check if we're in a git repo
    if (access(".git", F_OK) != 0) {
        printf("\033[31mError: Not in a git repository.\033[0m\n");
        return -1;
    }

    bool has_gh = (system("which gh >/dev/null 2>&1") == 0);
    const char* subcommand = (argc > 1) ? argv[1] : "status";

    if (strcmp(subcommand, "status") == 0 || strcmp(subcommand, "s") == 0) {
        printf("\033[1;36m📊 Git Status\033[0m\n\n");
        system("git status --short --branch");
        printf("\n\033[36mRecent commits:\033[0m\n");
        system("git log --oneline -5");
        return 0;
    }

    if (strcmp(subcommand, "commit") == 0 || strcmp(subcommand, "c") == 0) {
        if (argc < 3) {
            printf("Usage: git commit <message>\n");
            return -1;
        }

        char msg[1024] = {0};
        for (int i = 2; i < argc; i++) {
            if (i > 2) strncat(msg, " ", sizeof(msg) - strlen(msg) - 1);
            strncat(msg, argv[i], sizeof(msg) - strlen(msg) - 1);
        }

        printf("\033[36mStaging changes...\033[0m\n");
        system("git add -A");

        int status = system("git diff --cached --quiet");
        if (status == 0) {
            printf("\033[33mNo changes to commit.\033[0m\n");
            return 0;
        }

        char cmd_buf[2048];
        snprintf(cmd_buf, sizeof(cmd_buf),
            "git commit -m \"%s\" -m \"\" -m \"🤖 Generated with [Claude Code](https://claude.com/claude-code)\" -m \"Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>\"",
            msg);

        printf("\033[36mCommitting...\033[0m\n");
        int result = system(cmd_buf);
        if (result == 0) {
            printf("\033[32m✓ Committed!\033[0m\n");
        }
        return result == 0 ? 0 : -1;
    }

    if (strcmp(subcommand, "push") == 0 || strcmp(subcommand, "p") == 0) {
        printf("\033[36mPushing...\033[0m\n");
        int result = system("git push");
        if (result == 0) printf("\033[32m✓ Pushed!\033[0m\n");
        return result == 0 ? 0 : -1;
    }

    if (strcmp(subcommand, "sync") == 0) {
        printf("\033[36mSyncing...\033[0m\n");
        int result = system("git pull --rebase && git push");
        if (result == 0) printf("\033[32m✓ Synced!\033[0m\n");
        return result == 0 ? 0 : -1;
    }

    printf("\033[1;36m📦 Git Workflow\033[0m\n\n");
    printf("Subcommands:\n");
    printf("  status, s       Show status and recent commits\n");
    printf("  commit, c <msg> Stage all and commit\n");
    printf("  push, p         Push to remote\n");
    printf("  sync            Pull --rebase and push\n");
    if (has_gh) printf("\nFor PRs: /pr <title>\n");
    return 0;
}

/**
 * /pr - Create pull request via gh CLI
 */
int cmd_pr(int argc, char** argv) {
    if (system("which gh >/dev/null 2>&1") != 0) {
        printf("\033[31mError: 'gh' CLI not installed.\033[0m\n");
        printf("Install: brew install gh && gh auth login\n");
        return -1;
    }

    if (access(".git", F_OK) != 0) {
        printf("\033[31mError: Not in a git repository.\033[0m\n");
        return -1;
    }

    FILE* fp = popen("git branch --show-current", "r");
    if (!fp) return -1;
    char branch[256] = {0};
    if (fgets(branch, sizeof(branch), fp)) {
        branch[strcspn(branch, "\n")] = 0;
    }
    pclose(fp);

    if (strcmp(branch, "main") == 0 || strcmp(branch, "master") == 0) {
        printf("\033[31mError: Cannot create PR from %s.\033[0m\n", branch);
        printf("Create a feature branch first.\n");
        return -1;
    }

    char title[512] = {0};
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (i > 1) strncat(title, " ", sizeof(title) - strlen(title) - 1);
            strncat(title, argv[i], sizeof(title) - strlen(title) - 1);
        }
    } else {
        strncpy(title, branch, sizeof(title) - 1);
        for (char* p = title; *p; p++) {
            if (*p == '-' || *p == '_') *p = ' ';
        }
    }

    printf("\033[1;36m🔀 Creating PR\033[0m\n");
    printf("  Branch: %s\n  Title: %s\n\n", branch, title);

    char push_cmd[512];
    snprintf(push_cmd, sizeof(push_cmd), "git push -u origin %s 2>&1", branch);
    system(push_cmd);

    char pr_cmd[2048];
    snprintf(pr_cmd, sizeof(pr_cmd),
        "gh pr create --title \"%s\" --body \"## Summary\\n\\n## Test Plan\\n- [ ] Tests pass\\n\\n🤖 Generated with [Claude Code](https://claude.com/claude-code)\"",
        title);

    int result = system(pr_cmd);
    if (result == 0) printf("\n\033[32m✓ PR created!\033[0m\n");
    return result == 0 ? 0 : -1;
}

// ============================================================================
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
            status_color = "\033[33m";  // Yellow
            break;
        case TODO_STATUS_COMPLETED:
            status_icon = "[x]";
            status_color = "\033[32m";  // Green
            break;
        case TODO_STATUS_CANCELLED:
            status_icon = "[-]";
            status_color = "\033[90m";  // Gray
            break;
        default:
            status_icon = "[ ]";
            status_color = "\033[0m";
    }

    // Priority indicator
    const char* priority = "";
    if (task->priority == 1) priority = " \033[31m!!\033[0m";
    else if (task->priority == 3) priority = " \033[90m~\033[0m";

    // Print main line
    printf("  %s%s\033[0m %lld. %s%s",
           status_color, status_icon, (long long)task->id, task->title, priority);

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
            printf("Usage: todo add <title> [--due <date>] [--remind <time>] [--priority <1-3>] [--context <ctx>]\n");
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
                if (title[0]) strncat(title, " ", sizeof(title) - strlen(title) - 1);
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
            .external_id = NULL
        };
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
                if (i > 2) strncat(content, " ", sizeof(content) - strlen(content) - 1);
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
            if (i > 2) strncat(query, " ", sizeof(query) - strlen(query) - 1);
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
        printf("  Completed:     today: %d, week: %d\n",
               stats.total_completed_today, stats.total_completed_week);
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
    if (!word) return false;

    // Keywords that indicate time
    if (strstr(word, "tomorrow") || strstr(word, "domani")) return true;
    if (strstr(word, "tonight") || strstr(word, "stasera")) return true;
    if (strstr(word, "morning") || strstr(word, "mattina")) return true;
    if (strstr(word, "afternoon") || strstr(word, "pomeriggio")) return true;
    if (strstr(word, "evening") || strstr(word, "sera")) return true;
    if (strstr(word, "next") || strstr(word, "prossimo")) return true;
    if (strstr(word, "monday") || strstr(word, "lunedi")) return true;
    if (strstr(word, "tuesday") || strstr(word, "martedi")) return true;
    if (strstr(word, "wednesday") || strstr(word, "mercoledi")) return true;
    if (strstr(word, "thursday") || strstr(word, "giovedi")) return true;
    if (strstr(word, "friday") || strstr(word, "venerdi")) return true;
    if (strstr(word, "saturday") || strstr(word, "sabato")) return true;
    if (strstr(word, "sunday") || strstr(word, "domenica")) return true;
    if (strncmp(word, "in", 2) == 0 || strncmp(word, "tra", 3) == 0) return true;
    if (strncmp(word, "at", 2) == 0 || strncmp(word, "alle", 4) == 0) return true;

    // Month names
    const char* months[] = {"jan", "feb", "mar", "apr", "may", "jun",
                            "jul", "aug", "sep", "oct", "nov", "dec",
                            "gen", "mag", "giu", "lug", "ago", "set", "ott", "dic"};
    for (int i = 0; i < 20; i++) {
        if (strncasecmp(word, months[i], 3) == 0) return true;
    }

    // ISO date format
    if (strlen(word) >= 10 && word[4] == '-' && word[7] == '-') return true;

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
                if (note[0]) strncat(note, " ", sizeof(note) - strlen(note) - 1);
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
            if (time_str[0]) strncat(time_str, " ", sizeof(time_str) - strlen(time_str) - 1);
            strncat(time_str, argv[i], sizeof(time_str) - strlen(time_str) - 1);
        } else if (!in_time || !message[0]) {
            // Part of message
            if (message[0]) strncat(message, " ", sizeof(message) - strlen(message) - 1);
            strncat(message, argv[i], sizeof(message) - strlen(message) - 1);
        } else {
            // Continuation of time
            if (time_str[0]) strncat(time_str, " ", sizeof(time_str) - strlen(time_str) - 1);
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
    TodoCreateOptions opts = {
        .title = message,
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
        .external_id = NULL
    };
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
    if (tasks) todo_free_tasks(tasks, count);
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
        printf("Daemon Status: %s%s\033[0m\n",
               running ? "\033[32m" : "\033[31m",
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
        NotifyOptions opts = {
            .title = "Test Notification",
            .subtitle = NULL,
            .body = "Click here to open Convergio",
            .sound = "Glass",
            .group = "convergio-reminders",  // This enables click-to-open
            .action_url = NULL,
            .timeout_ms = 0
        };
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

                printf("  %s%s\033[0m %-20s %s%s\033[0m\n",
                       status_color, status_icon,
                       servers[i],
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
                    printf("  \033[32m●\033[0m %-20s %d tools\n",
                           connected[i], server->tool_count);
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
            printf("\033[31m✗ Failed to connect: %s\033[0m\n",
                   err ? err : "unknown error");
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
                        if (last_server) printf("\n");
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
        if (args) cJSON_Delete(args);

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
            printf("     ID: \033[90m%.8s...\033[0m  Tasks: %d/%d (%.0f%%)\n",
                   p->id, progress.completed, progress.total, progress.percent_complete);

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
        printf("  Progress: %d/%d tasks (%.1f%%)\n",
               progress.completed, progress.total, progress.percent_complete);

        // Show progress bar
        int bar_width = 30;
        int filled = (int)((double)progress.percent_complete / 100.0 * (double)bar_width);
        printf("  [");
        for (int i = 0; i < bar_width; i++) {
            if (i < filled) printf("\033[32m█\033[0m");
            else printf("\033[90m░\033[0m");
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
            if (days <= 0) days = 30;
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
            if (strstr(filename, ".md")) icon = "📝";
            else if (strstr(filename, ".json")) icon = "📊";
            else if (strstr(filename, ".html")) icon = "🌐";

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
            if (days <= 0) days = 30;
        }

        int deleted = output_cleanup(days);
        printf("\033[32m✓ Cleaned up %d file(s) older than %d days\033[0m\n", deleted, days);
        return 0;
    }

    printf("\033[31mUnknown output command: %s\033[0m\n", subcmd);
    printf("Use '/output' to see available commands.\n");
    return -1;
}
