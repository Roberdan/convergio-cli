/**
 * CONVERGIO KERNEL - Detailed Help System
 *
 * Comprehensive help information for all commands
 */

#include "commands_internal.h"

// ============================================================================
// DETAILED HELP DATA
// ============================================================================

static const CommandHelp DETAILED_HELP[] = {
    {"help", "help [command]", "Display help information",
     "Without arguments, shows all available commands.\n"
     "With a command name, shows detailed help for that command.",
     "help           # Show all commands\n"
     "help create    # Detailed help for 'create'\n"
     "help agent     # Detailed help for 'agent'"},
    {"create", "create <essence>", "Create a semantic node in the knowledge graph",
     "Creates a new semantic node with the given essence (description).\n"
     "The essence defines the concept or entity being created.\n"
     "Returns a unique semantic ID for the created node.",
     "create \"un concetto di bellezza\"\n"
     "create \"progetto di machine learning\"\n"
     "create sistema di autenticazione OAuth"},
    {"agent", "agent <subcommand> [args]", "Manage agents in the system",
     "Subcommands:\n"
     "  list                    List all available agents\n"
     "  info <name>             Show detailed info about an agent\n"
     "  create <name> <desc>    Create a new dynamic agent\n"
     "  skill <skill_name>      Add a skill to the current assistant\n\n"
     "Use @<agent_name> <message> to communicate directly with an agent.",
     "agent list\n"
     "agent info baccio\n"
     "agent create helper \"Un assistente per task generici\"\n"
     "agent skill programmazione"},
    {"agents", "agents [working|active]", "List all available agents",
     "Without arguments, shows all agents in the registry with their status.\n"
     "With 'working' or 'active', shows only currently active agents.\n"
     "Displays agent roles, states, and current tasks.",
     "agents           # Show all agents\n"
     "agents working   # Show only working agents\n"
     "agents active    # Same as 'agents working'"},
    {"space", "space <create|join|leave|list|urgency> [args]", "Manage collaborative spaces",
     "Spaces are collaborative environments where agents can work together.\n\n"
     "Subcommands:\n"
     "  create <name> <purpose>   Create a new space\n"
     "  urgency                   Show current space urgency level",
     "space create project \"Sviluppo nuova feature\"\n"
     "space urgency"},
    {"status", "status", "Show comprehensive system status",
     "Displays:\n"
     "  - Kernel status (ready/not ready)\n"
     "  - Current space information\n"
     "  - Active assistant details\n"
     "  - GPU statistics\n"
     "  - Scheduler metrics",
     "status"},
    {"cost", "cost [report|set <amount>|reset]", "Manage cost tracking and budget",
     "Subcommands:\n"
     "  (none)              Show current session spending\n"
     "  report              Show detailed cost breakdown by model\n"
     "  set <amount_usd>    Set a budget limit (stops when reached)\n"
     "  reset               Reset session spending to zero\n\n"
     "Cost tracking includes all API calls with token counts and pricing.",
     "cost              # Quick status\n"
     "cost report       # Detailed breakdown\n"
     "cost set 10.00    # Set $10 budget\n"
     "cost reset        # Reset counters"},
    {"debug", "debug [off|error|warn|info|debug|trace]", "Toggle or set debug output level",
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
     "debug off      # Disable debug output"},
    {"allow-dir", "allow-dir <path>", "Add a directory to the sandbox",
     "Adds a directory to the list of allowed paths for file operations.\n"
     "This is required for agents to read/write files outside the workspace.\n"
     "System directories (/usr, /etc, etc.) are blocked for security.\n"
     "Paths are resolved to absolute paths automatically.",
     "allow-dir ~/Documents/project\n"
     "allow-dir /Users/me/data\n"
     "allow-dir ../other-project"},
    {"allowed-dirs", "allowed-dirs", "Show allowed directories (sandbox)",
     "Lists all directories where file operations are permitted.\n"
     "The first entry is always the current workspace.\n"
     "Additional directories can be added with 'allow-dir'.",
     "allowed-dirs"},
    {"logout", "logout", "Logout and clear credentials",
     "Logs out from the current authentication method.\n"
     "For OAuth (Claude Max): removes tokens from Keychain.\n"
     "Falls back to API key if ANTHROPIC_API_KEY is set.",
     "logout"},
    {"auth", "auth", "Show authentication status",
     "Displays current authentication method and status:\n"
     "  - API Key: Using ANTHROPIC_API_KEY environment variable\n"
     "  - OAuth: Using Claude Max subscription (tokens in Keychain)\n"
     "  - None: Not authenticated",
     "auth"},
    {"update", "update [install|changelog]", "Check for and install updates",
     "Subcommands:\n"
     "  (none)       Check if updates are available\n"
     "  install      Download and install the latest version\n"
     "  changelog    Show recent changes and release notes\n\n"
     "Updates are fetched from GitHub releases or Homebrew.",
     "update            # Check for updates\n"
     "update install    # Install latest version\n"
     "update changelog  # View release notes"},
    {"hardware", "hardware", "Show hardware information",
     "Displays detailed hardware information including:\n"
     "  - CPU model and core count\n"
     "  - Memory (RAM) total and available\n"
     "  - GPU information (Metal support)\n"
     "  - Neural Engine availability",
     "hardware"},
    {"news", "news [version]", "Show release notes for Convergio",
     "Displays the release notes and changelog for a specific version.\n"
     "Without arguments, shows the latest release notes.\n\n"
     "You can specify a version number with or without the 'v' prefix.",
     "news           # Show latest release notes\n"
     "news 3.0.4     # Show notes for v3.0.4\n"
     "news v3.0.3    # Also works with 'v' prefix"},
    {"recall", "recall [load <n>|delete <n>|clear]", "View and reload past session contexts",
     "Shows summaries of past sessions with what was discussed.\n"
     "Sessions are saved when you exit with 'quit'.\n"
     "Subcommands:\n"
     "  load <n>        Load context from session N into current conversation\n"
     "  delete <n>      Delete session N and its summary\n"
     "  clear           Delete all stored summaries (asks for confirmation)\n",
     "recall           # List past sessions with summaries\n"
     "recall load 1    # Load context from session 1\n"
     "recall delete 2  # Delete session 2\n"
     "recall clear     # Delete all sessions"},
    {"stream", "stream [on|off]", "Toggle streaming mode",
     "Controls whether AI responses stream in real-time.\n\n"
     "ON:  Responses appear as they're generated (live)\n"
     "     Tool calls are disabled in this mode\n\n"
     "OFF: Responses wait until complete\n"
     "     Full tool support enabled\n\n"
     "Without arguments, toggles the current setting.",
     "stream        # Toggle streaming\n"
     "stream on     # Enable streaming\n"
     "stream off    # Disable streaming"},
    {"theme", "theme [ocean|forest|sunset|mono]", "Change color theme",
     "Available themes:\n"
     "  ocean   - Cool blue tones (default)\n"
     "  forest  - Natural green tones\n"
     "  sunset  - Warm orange/red tones\n"
     "  mono    - Monochrome (grayscale)\n\n"
     "Without arguments, lists available themes.\n"
     "Theme preference is saved to config.",
     "theme          # List themes\n"
     "theme ocean    # Set ocean theme\n"
     "theme mono     # Set monochrome theme"},
    {"think", "think <intent>", "Process an intent through the assistant",
     "Parses the given text as an intent and has the assistant\n"
     "think through it. Shows:\n"
     "  - Intent classification\n"
     "  - Confidence and urgency scores\n"
     "  - Clarification questions if needed\n"
     "  - Assistant's thoughts",
     "think \"come posso migliorare le performance?\"\n"
     "think implementa una cache per le query"},
    {"compare", "compare <prompt> <model1> <model2> [model3...]",
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
     "compare \"Write a haiku\" claude-sonnet-4 claude-opus-4 --no-diff"},
    {"benchmark", "benchmark <prompt> <model> [iterations]", "Benchmark a model's performance",
     "Runs the same prompt multiple times against a model to measure:\n"
     "  - Average latency\n"
     "  - Token throughput\n"
     "  - Cost per run\n"
     "  - Consistency of responses\n\n"
     "Default iterations: 3\n"
     "Maximum iterations: 100",
     "benchmark \"Write a haiku\" claude-opus-4\n"
     "benchmark \"Summarize this\" claude-sonnet-4 5"},
    {"telemetry", "telemetry <subcommand>", "Manage telemetry settings",
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
     "telemetry delete"},
    {"tools", "tools <subcommand>", "Manage development tools",
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
     "tools install docker"},
    {"project", "project <subcommand> [args]", "Manage projects with dedicated agent teams",
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
     "project status"},
    {"setup", "setup", "Configure providers and agent models",
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
     "setup           # Choose optimization profile"},
    {"quit", "quit", "Exit Convergio",
     "Gracefully shuts down Convergio:\n"
     "  - Shows final cost report\n"
     "  - Saves configuration\n"
     "  - Cleans up resources\n\n"
     "Alias: 'exit'",
     "quit\n"
     "exit"},
    // Semantic memory commands
    {"reset", "reset", "Reset all Convergio memory",
     "Permanently deletes ALL stored data including:\n"
     "  - Knowledge graph (all memories, nodes, and relations)\n"
     "  - Notes and documents\n"
     "  - Cache data\n"
     "  - Execution plans\n"
     "  - Session history\n"
     "  - Voice conversation history\n"
     "  - Education data (in Education edition)\n"
     "  - Generated outputs\n\n"
     "REQUIRES CONFIRMATION: You must type 'RESET' (all caps) to proceed.\n"
     "This action cannot be undone. Restart Convergio for full effect.",
     "reset    # Prompts for confirmation before deleting all data"},
    {"remember", "remember <text>", "Store a memory in the knowledge graph",
     "Creates a persistent memory node that survives across sessions.\n"
     "Memories are stored with high importance (0.9) and can be:\n"
     "  - Searched with 'recall'\n"
     "  - Listed with 'memories'\n"
     "  - Deleted with 'forget'\n\n"
     "Memories persist in SQLite and are loaded on startup.",
     "remember Roberto prefers clean code\n"
     "remember The API key is stored in keychain\n"
     "remember Use snake_case for variables"},
    {"search", "search <query>", "Search memories semantically",
     "Searches the knowledge graph for memories matching your query.\n"
     "Returns up to 10 matching results ordered by importance.\n\n"
     "This is an alias for 'recall <query>' with the same functionality.",
     "search Roberto preferences\n"
     "search API documentation\n"
     "search coding style"},
    {"memories", "memories", "List knowledge graph statistics and important memories",
     "Shows:\n"
     "  - Total nodes and relations in the graph\n"
     "  - Nodes currently loaded in memory\n"
     "  - The 10 most important memories (importance >= 0.5)\n\n"
     "Use this to get an overview of what Convergio remembers.",
     "memories"},
    {"forget", "forget <id>", "Delete a memory by its ID",
     "Permanently removes a memory from the knowledge graph.\n"
     "The ID is a hexadecimal number shown in 'recall' or 'memories' output.\n\n"
     "This also removes all relations connected to that memory.",
     "forget 0x1234567890abcdef\n"
     "forget 1234567890abcdef"},
    {"graph", "graph", "Show knowledge graph statistics",
     "Displays detailed statistics about the semantic knowledge graph:\n"
     "  - Total nodes in database\n"
     "  - Nodes loaded in memory\n"
     "  - Total relations (connections between nodes)\n"
     "  - Breakdown of nodes by type (Memory, Concept, Entity, etc.)\n\n"
     "The knowledge graph stores memories, concepts, and their relationships\n"
     "to enable semantic understanding across sessions.",
     "graph"},
    {"local", "help local", "Local models guide (MLX on Apple Silicon)",
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
     "convergio -l -m llama-3.2-3b"},
    {"test", "test", "Run project tests with auto-detected framework",
     "Automatically detects and runs tests for your project.\n\n"
     "SUPPORTED FRAMEWORKS:\n"
     "  - make test     (Makefile with 'test' target)\n"
     "  - cargo test    (Rust - Cargo.toml)\n"
     "  - go test       (Go - go.mod)\n"
     "  - npm test      (Node.js - package.json)\n"
     "  - pytest        (Python - pytest.ini/pyproject.toml/tests/)\n\n"
     "The command auto-detects which framework to use based on\n"
     "project files in the current directory.",
     "/test    # Run tests with auto-detected framework"},
    {"git", "git [status|commit|push|sync] [args]", "Git workflow helper commands",
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
     "/git sync"},
    {"pr", "pr [title]", "Create pull request via GitHub CLI",
     "Creates a pull request using the 'gh' CLI tool.\n\n"
     "REQUIREMENTS:\n"
     "  - GitHub CLI (gh) must be installed and authenticated\n"
     "  - Must be on a feature branch (not main/master)\n\n"
     "If no title is provided, generates one from branch name.\n"
     "Automatically pushes branch before creating PR.",
     "/pr Add user authentication\n"
     "/pr    # Uses branch name as title"},
    {"todo", "todo <add|list|done|start|delete|inbox|search|stats> [args]",
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
     "/todo search meeting"},
    {"remind", "remind <message> <when> [--note <context>]", "Quick reminder creation",
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
     "/remind \"Review PR\" tomorrow --note \"Check auth changes in #123\""},
    {"reminders", "reminders [today|week|all]", "View upcoming reminders",
     "Shows scheduled reminders filtered by time range.\n\n"
     "FILTERS:\n"
     "  today   Today's reminders (default)\n"
     "  week    Next 7 days\n"
     "  all     All scheduled reminders\n\n"
     "Use /todo done <id> to mark complete\n"
     "Use /todo delete <id> to remove",
     "/reminders        # Today's reminders\n"
     "/reminders week   # Next 7 days\n"
     "/reminders all    # All scheduled"},
    {"daemon", "daemon <command>", "Manage the notification daemon",
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
     "/daemon test      # Send test notification"},
    {"mcp", "mcp <command> [args]", "Manage MCP server connections",
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
     "/mcp call read_file '{\"path\":\"/tmp/test.txt\"}'"},
    {"plan", "plan <subcommand> [args]", "Manage execution plans",
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
     "/plan cleanup 7              # Delete plans older than 7 days"},
    {"workflow", "workflow <list|show|execute|resume> [args]", "Manage workflow orchestration",
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
     "workflow resume 12345 2"},
    {"output", "output <subcommand> [args]", "Manage generated outputs",
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
     "/output cleanup 7            # Delete outputs older than 7 days"},
    {"style", "style [flash|concise|balanced|detailed]", "Set response style",
     "Controls how verbose AI responses are.\n\n"
     "STYLES:\n"
     "  flash       Ultra-concise, immediate answers\n"
     "  concise     Brief, to-the-point responses\n"
     "  balanced    Moderate detail (default)\n"
     "  detailed    Comprehensive, thorough explanations\n\n"
     "Without arguments, shows the current style setting.",
     "/style              # Show current style\n"
     "/style flash        # Set to ultra-concise\n"
     "/style detailed     # Set to comprehensive"},
    // Education Pack commands
    {"education", "education [setup|quick|profile|progress]", "Education Pack setup and management",
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
     "/education profile                   # Detailed profile view"},
    {"study", "study <subject> [topic]", "Start a study session with Pomodoro timer",
     "Start a focused study session with a maestro.\n\n"
     "Features:\n"
     "  - 25-minute focused work sessions (Pomodoro)\n"
     "  - 5-minute breaks (15 min after 4 pomodoros)\n"
     "  - Native macOS notifications\n"
     "  - End-of-session review quiz\n"
     "  - Automatic time tracking in libretto",
     "/study matematica                    # Study math\n"
     "/study fisica \"moto rettilineo\"      # Study specific topic\n"
     "/study italiano                      # Study Italian"},
    {"homework", "homework <description>", "Get help with homework (anti-cheating mode)",
     "Get guided help understanding homework without answers.\n\n"
     "Features:\n"
     "  - Socratic method - guiding questions only\n"
     "  - 5-level progressive hint system (0=subtle, 4=detailed)\n"
     "  - Understanding verification quiz\n"
     "  - Parental transparency log\n\n"
     "The system helps you UNDERSTAND, not do your homework for you.",
     "/homework Matematica: risolvere 3x + 5 = 14\n"
     "/homework Storia: cause della Rivoluzione Francese\n"
     "/homework-hint 2                     # Get level 2 hint"},
    {"quiz", "quiz <topic> [--count n] [--difficulty easy|medium|hard]",
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
     "/quiz storia --difficulty easy       # Easy difficulty"},
    {"flashcards", "flashcards <topic> [--count n] [--export anki|pdf]",
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
     "/flashcards storia --export anki     # Export to Anki"},
    {"mindmap", "mindmap <concept> [--format svg|png|pdf] [--output path]",
     "Generate visual mind maps",
     "Generate Mermaid.js mind maps from any concept.\n\n"
     "Features:\n"
     "  - LLM-powered content generation\n"
     "  - Export to SVG, PNG, or PDF\n"
     "  - Accessibility adaptations\n"
     "  - Auto-opens in browser",
     "/mindmap \"French Revolution\"          # Generate and open\n"
     "/mindmap photosynthesis --format png # Export as PNG\n"
     "/mindmap \"theory of relativity\" --output ~/Desktop/relativity.svg"},
    {"libretto", "libretto [grades|diary|progress|average]", "Student gradebook and activity log",
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
     "/libretto average                    # Subject averages"},
    {"voice", "voice [maestro] [topic]", "Conversational voice mode with AI maestri",
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
     "/voice euclide geometria             # Euclide on geometry"},
    {"upload", "upload [path]", "Upload a document for study help",
     "Open an interactive file picker to upload school materials.\n\n"
     "Supported formats:\n"
     "  - PDF, DOCX, DOC\n"
     "  - PPTX, PPT (presentations)\n"
     "  - XLSX, XLS (spreadsheets)\n"
     "  - Images (JPG, PNG)\n"
     "  - TXT, RTF, CSV\n\n"
     "File picker is restricted to Desktop, Documents, and Downloads\n"
     "for student safety.\n\n"
     "After upload, you can ask teachers about the document content.",
     "/upload                              # Open file picker\n"
     "/upload ~/Documents/math.pdf        # Upload specific file"},
    {"doc", "doc [list|clear|number]", "Manage uploaded documents",
     "View and manage uploaded school materials.\n\n"
     "Subcommands:\n"
     "  list         List all uploaded documents\n"
     "  clear        Remove all uploaded documents\n"
     "  <number>     Select document by number\n\n"
     "The current document is used when asking teachers questions.",
     "/doc                                 # List uploaded documents\n"
     "/doc list                            # Same as /doc\n"
     "/doc 1                               # Select first document\n"
     "/doc clear                           # Remove all documents"},
    {NULL, NULL, NULL, NULL, NULL}};

// ============================================================================
// HELP UTILITY FUNCTIONS
// ============================================================================

const CommandHelp* commands_get_detailed_help(void) {
    return DETAILED_HELP;
}

const CommandHelp* find_detailed_help(const char* cmd_name) {
    for (const CommandHelp* h = DETAILED_HELP; h->name != NULL; h++) {
        if (strcmp(h->name, cmd_name) == 0) {
            return h;
        }
    }
    return NULL;
}

void print_detailed_help(const CommandHelp* h) {
    printf("\n\033[1m%s\033[0m - %s\n", h->name, h->description);
    printf("\n\033[36mUsage:\033[0m\n  %s\n", h->usage);
    printf("\n\033[36mDescription:\033[0m\n");

    // Print description with indentation
    const char* p = h->details;
    printf("  ");
    while (*p) {
        putchar(*p);
        if (*p == '\n' && *(p + 1)) {
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
        if (*p == '\n' && *(p + 1)) {
            printf("  ");
        }
        p++;
    }
    printf("\n\n");
}
