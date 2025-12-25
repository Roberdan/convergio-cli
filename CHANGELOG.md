# Changelog

All notable changes to Convergio CLI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [6.0.1] - 2025-12-25

### Fixed

- **Critical Build Fix** - Resolved duplicate symbol linker error
  - Removed redundant `mastery_get_level()` wrapper from `mastery_gate.c`
  - Removed redundant `mastery_is_mastered()` wrapper from `mastery_gate.c`
  - Both functions remain in their canonical locations (`mastery.c` and `education_db.c`)
  - v6.0.0 was released with a build regression that prevented compilation

## [6.0.0] - 2025-12-25

### Added

- **Apple Foundation Models (AFM)** - On-device AI for macOS 26+
  - Native Swift bridge to FoundationModels framework
  - Session-based inference with streaming support
  - Graceful fallback to MLX on pre-macOS 26 systems
  - Conditional compilation for cross-version compatibility
  - ADR: `docs/adr/019-apple-foundation-models.md`

- **Education Edition Complete** - Full Scuola 2026 integration
  - FSRS (Free Spaced Repetition Scheduler) algorithm for optimal learning
  - Mastery tracking with skill gap identification
  - Italian school curriculum support (Liceo, Tecnico, Professionale)
  - Accessibility runtime for students with disabilities (DSA, ADHD, Autism)
  - Ali Preside dashboard for school administrators
  - Error interpreter for proactive teaching
  - Multi-profile system for family/school use
  - Document upload and camera integration
  - Azure OpenAI GDPR-compliant backend for EU schools

- **Multi-Edition Build System**
  - Master edition: Full-featured for power users
  - Education edition: Optimized for schools (convergio-edu)
  - Business edition: Enterprise features (convergio-biz)
  - Developer edition: Debug tools and APIs (convergio-dev)

- **Voice Gateway** - Real-time voice interaction
  - OpenAI Realtime API integration
  - Azure Realtime API for enterprise
  - Voice history management
  - VoiceOver accessibility integration

- **ProviderManager** - Dynamic LLM provider registry
  - Runtime provider registration and discovery
  - Fallback chain configuration
  - Health monitoring and auto-failover

### Changed

- **Build System** - Enhanced for multi-edition support
  - Edition-specific feature flags
  - Optimized linking for Apple Silicon
  - Swift Package Manager integration for AFM and MLX

### Fixed

- **AFM Compatibility** - Conditional compilation for pre-macOS 26
- **CI Warning Filter** - Excludes macOS version linker warnings
- **Mastery API** - Fixed `mastery_identify_gaps` symbol resolution

### Technical

- Consolidated 4 feature branches into stable v6.0.0 release
- Added `.editorconfig` and `.swiftformat` for code consistency
- Zero-tolerance CI with comprehensive test coverage

## [5.4.0] - 2025-12-21

### Added

- **Advanced Workflow Orchestration** - Complete multi-agent coordination system
  - State machine-based workflow execution with checkpointing
  - Task decomposition with dependency resolution and topological sort
  - Group chat coordination with consensus detection
  - Conditional routing with expression evaluation (==, !=, <, >, <=, >=)
  - Retry logic with exponential backoff and error classification
  - Mermaid diagram export for workflow visualization
  - 9 workflow templates and 4 reusable patterns
  - CLI commands: `workflow list`, `workflow show`, `workflow execute`, `workflow resume`
  - Comprehensive documentation: USER_GUIDE.md, PATTERN_GUIDE.md, MIGRATION_GUIDE.md
  - ADR: `docs/adr/018-workflow-orchestration.md`

- **Extended Telemetry Events** - Enhanced observability
  - Performance metrics (operation duration, memory usage)
  - Security audit events (validation results, context)
  - Retry tracking (attempt, reason, max retries)
  - Checkpoint events (workflow type, duration)

- **Security Enforcement Phase 2** - Safe file operations
  - Replaced `fopen()` with `safe_path_open()` in 5 core files
  - Path validation with boundary checking
  - Consistent security patterns across codebase

- **Web Search Tool** - Multi-provider web search support
  - Anthropic: Native `web_search_20250305` tool with citations
  - OpenAI: Native search via `gpt-4o-search-preview` model with `web_search_options`
  - Gemini/Ollama/MLX: Local fallback using DuckDuckGo Lite
  - ADR: `docs/adr/015-web-search-architecture.md`

- **ACP Multi-Agent Support**
  - New `--agent` flag to specify agent directly (e.g., `--agent amy-cfo`)
  - Context sharing between agents via `acp/context/share` endpoint
  - Session persistence with auto-resume on reconnection
  - History context injection for better conversation continuity

- **Date/Time Context for Agents**
  - Agents now receive current date/time in system prompt
  - Enables time-aware responses and scheduling tasks

### Fixed

- **Router Provider Initialization** - Fixed "No router provider available" warning when using `--agent` flag
- **ACP Session Persistence** - New sessions now saved immediately for reliable persistence
- **Sign Comparison Warning** - Fixed compiler warning in ACP history context

### Technical

- New tool type: `TOOL_WEB_SEARCH` for local web search fallback
- OpenAI provider auto-detects `web_search` tool and switches to search model
- DuckDuckGo Lite HTML parser with proper URL encoding and buffer limits

## [5.3.1] - 2025-12-17

### Fixed

- **MLX Swift Dependency**: Pinned mlx-swift-lm to v2.29.2 (fixes Jamba.swift syntax error on main branch)
- **Metal Shaders**: Pre-compiled Metal library included for reliable GPU acceleration on CI
- **CI Pipeline**: Optimized workflow structure - reduced CI time from ~22min to ~11min
  - CI now runs only on PR (not duplicated on push to main)
  - Auto-release workflow creates tag when VERSION changes
  - Sanitizer tests moved to nightly schedule

### Changed

- **Release Workflow**: Removed failing push to protected main branch
- **Build System**: Metal shaders fallback to pre-compiled bundle when xcodebuild unavailable

## [5.3.0] - 2025-12-16

### Added

- **Advanced Filesystem Tools** (Claude Code-style)
  - `glob` tool: Find files by pattern (e.g., `**/*.c`) with recursive `**` support
  - `grep` tool: Search file contents with regex, uses ripgrep when available
  - `edit` tool: Precise string replacement with automatic backups to `~/.convergio/backups/`
  - `file_delete` tool: Safe file deletion that moves to Trash (macOS Finder integration or fallback to `~/.convergio/trash/`)
  - ADR: `docs/adr/014-advanced-filesystem-tools.md`

- **Direct Bash Prefix**
  - Execute shell commands directly with `!command` or `$command` prefix
  - Bypasses AI processing for immediate execution
  - Safety checks still applied (blocked commands list)

### Safety

- **Automatic backup before file edits**: All `edit` operations create timestamped backups
- **Trash-based deletion**: `file_delete` moves files to Trash instead of permanent deletion
- **Path safety enforcement**: All new tools respect workspace boundaries
- **Command safety blocklist**: Direct bash prefix uses same blocklist as shell_exec

### Technical

- New tool types: TOOL_GLOB, TOOL_GREP, TOOL_EDIT, TOOL_FILE_DELETE
- Updated Ali system prompt with guidance to prefer new tools
- Backup directory: `~/.convergio/backups/`
- Trash directory: `~/.convergio/trash/` (fallback when Finder unavailable)

## [5.2.2] - 2025-12-16

### Fixed

- **Dynamic ConvergioNotify.app Path Resolution**
  - Notification helper now searches for app in multiple locations (Applications, build/bin, user Applications)
  - Fixes "ConvergioNotify.app not found" errors when installed in non-standard locations
  - Improved error messages when notification helper is unavailable

- **Orchestrator Error Messages**
  - Enhanced error messages for better debugging when agent delegation fails
  - Clearer feedback when API calls encounter issues

- **Buffer Overflow in Compaction**
  - Fixed global-buffer-overflow in `compaction_summarize()` caused by incorrect marker length calculation
  - Now uses `sizeof()` instead of hardcoded length for compile-time safety

## [5.2.1] - 2025-12-16

### Fixed

- **Duplicate Notification Entries**
  - Removed old `Convergio.app` stub that caused duplicate entries in System Settings > Notifications
  - Only `ConvergioNotify.app` is now installed for notifications
  - Install target now cleans up old `Convergio.app` if present

## [5.2.0] - 2025-12-16

### Added

- **Native Notification Helper** (ConvergioNotify.app)
  - Native Swift helper app for macOS notifications with proper Convergio icon
  - Replaces terminal-notifier which has icon display bugs on recent macOS
  - Uses NSUserNotification API for reliable notification delivery
  - Auto-installed to /Applications during `make install` or Homebrew install
  - Signed and notarized as part of release workflow
  - ADR: `docs/adr/013-native-notification-helper.md`

- **Notification Daemon Auto-Start**
  - Daemon now starts automatically on Convergio launch
  - Stops cleanly on Convergio shutdown
  - LaunchAgent support for persistent background operation

### Fixed

- **Notification Icon Display**
  - Notifications now display Convergio logo instead of Terminal icon
  - Works on all recent macOS versions (12-15)

- **E2E Test Cleanup**
  - Test tasks now use `E2E_TEST_` prefix for identification
  - Proper cleanup prevents test data pollution in user database

### Technical

- Added `notify-helper` target to Makefile
- Release workflow signs and notarizes ConvergioNotify.app
- Homebrew formula installs helper to /Applications
- Updated notification fallback chain to prefer native helper

## [5.1.0] - 2025-12-16

### Added

- **Plan Database** (PR #60)
  - SQLite-backed persistent storage for execution plans
  - ACID transactions with WAL mode for concurrent access
  - Thread-safe task claiming for multi-agent coordination
  - New `/plan` command with subcommands:
    - `list` - Show all plans with status
    - `status <id>` - Show detailed plan progress
    - `export <id>` - Export to Markdown with Mermaid Gantt charts
    - `cleanup <days>` - Delete old plans
  - Files: `src/orchestrator/plan_db.c`, `include/nous/plan_db.h`
  - ADR: `docs/adr/011-centralized-plan-database.md`

- **Output Service** (PR #62)
  - Centralized service for structured document generation
  - Mermaid diagram support (flowchart, sequence, gantt, pie, mindmap)
  - OSC8 terminal hyperlinks for clickable file paths
  - Auto-organization by date and project
  - New `/output` command with subcommands:
    - `list` - Show recent outputs
    - `latest` - Show most recent output
    - `open <path>` - Open in default app
    - `delete <path>` - Remove output file
    - `size` - Show disk usage
    - `cleanup <days>` - Delete old outputs
  - Files: `src/tools/output_service.c`, `include/nous/output_service.h`
  - ADR: `docs/adr/012-output-service.md`

### Fixed

- **Code Quality**
  - Fixed 41 compiler warnings in new plan_db.c and output_service.c
  - Replaced deprecated `sprintf` with `snprintf` in plan_db.c
  - Fixed sign conversion warnings throughout output_service.c
  - Added safe SNPRINTF_ADD macro for size_t position tracking
  - Fixed double promotion warnings in commands.c

### Technical

- All new code follows zero-warnings policy
- Plan database uses SQLite with WAL mode (busy timeout 5s)
- Output files stored in `~/.convergio/outputs/` organized by date
- Plans stored in `~/.convergio/plans.db`

## [5.0.1] - 2025-12-14

### Fixed

- **E2E Test Suite Updates**
  - Updated test expectations to match current CLI output format
  - Fixed readline static linking for macOS code signing compatibility
  - All 70 E2E tests now pass (9 skipped by design)

### Changed

- **Build System**
  - Use direct path for readline prefix in Makefile
  - Link readline statically to avoid dyld issues on user machines

## [5.0.0] - 2025-12-14

### Added

- **Major Version 5.0 Release** - Production-ready release with comprehensive quality gates
  - Zero compiler warnings policy enforced
  - Full Microsoft Engineering Fundamentals compliance
  - Comprehensive E2E test suite with 50+ test scenarios

### Changed

- **Code Quality Improvements**
  - Fixed all sign conversion warnings in todo.c (26 warnings eliminated)
  - Fixed sign conversion warnings in tools.c (5 warnings eliminated)
  - Fixed implicit conversion warning in commands.c (1 warning eliminated)
  - Explicit type casts for TodoPriority, TodoStatus, TodoRecurrence, TodoSource enums
  - Proper size_t usage for buffer position tracking

- **Release Process**
  - Integrated brutal release manager with zero-tolerance quality gates
  - All compiler warnings are now blocking for releases
  - Automated model freshness verification for AI providers

### Security

- All memory allocation patterns verified safe
- No unsafe C functions in release code paths
- Keychain storage with iCloud sync disabled

### Technical

- Apple Silicon M1-M5 support verified (hardware.h updated)
- AI model configurations current (config/models.json version 2025.12.4)
  - Anthropic Claude: claude-opus-4-5-20251101, claude-sonnet-4-5-20250929
  - OpenAI: gpt-5.2, gpt-5.2-pro, gpt-5.2-instant, o3, o3-mini, o4-mini
  - Google Gemini: gemini-3.0-pro, gemini-3.0-deep-think, gemini-2.0-flash
- Build system produces zero warnings with -Wsign-conversion enabled

## [4.2.0] - 2025-12-14

### Added

- **Anna Executive Assistant Agent**
  - Personal productivity assistant with native task management
  - Intelligent scheduling and proactive reminders
  - Integration with the agent ecosystem via Ali coordination
  - Natural language task understanding (English + Italian)
  - ADR-009 documenting the architecture decision

- **Native Todo Manager (`/todo` command)**
  - SQLite-based local task storage (privacy-first, no external dependencies)
  - Full CRUD operations: add, list, done, start, delete
  - Task priorities (urgent, normal, low) and status tracking
  - Due dates with natural language parsing ("tomorrow", "next monday", "domani", "tra 2 ore")
  - Recurrence support (daily, weekly, monthly)
  - Quick capture inbox for thoughts and ideas
  - Full-text search (FTS5) across all tasks
  - Context/project tagging system

- **Quick Reminders (`/remind` command)**
  - Fast reminder creation with flexible syntax
  - Natural language time parsing (English + Italian support)
  - Time-of-day shortcuts: "tonight", "tomorrow morning", "stasera"
  - Relative times: "in 2 hours", "in 30 minutes", "tra 2 ore"
  - Weekday support: "next monday", "thursday in two weeks"
  - Optional notes for context

- **Notification System (`/daemon` command)**
  - Native macOS notifications via terminal-notifier or osascript
  - Background daemon for reminder delivery when CLI not running
  - LaunchAgent for auto-start at login
  - Multiple notification backends with automatic fallback
  - Apple Silicon optimized (E-core scheduling)
  - Health monitoring and error recovery

- **MCP Client Integration (`/mcp` command)**
  - Generic Model Context Protocol client
  - JSON-RPC 2.0 over stdio and HTTP transports
  - Auto tool discovery from connected servers
  - Multi-server support with connection pooling
  - Configuration via `~/.convergio/mcp.json`
  - Implements MCP Specification 2025-06-18

- **New Commands**
  - `/reminders` - View upcoming scheduled reminders
  - `/daemon status|start|stop|health` - Manage notification daemon
  - `/mcp list|connect|tools|call` - Manage MCP server connections

### Changed

- Agent count increased from 53 to 54 specialists (added Anna)
- Enhanced help documentation system with new command docs

### Security

- Security fixes from Copilot code review
- Improved input validation in task/reminder parsing

### Documentation

- ADR-009: Anna Executive Assistant Architecture
- Help documentation for `/todo`, `/remind`, `/reminders`, `/daemon`, `/mcp`

## [4.1.0] - 2025-12-14

### Added
- **Hybrid Embedding Strategy** (Issues #1, #2, #3)
  - OpenAI `text-embedding-3-small` for online semantic search
  - Local MLX fallback when offline
  - `openai_embed_text()` in `src/providers/openai.c`
  - Updated `nous_generate_embedding()` and `mlx_embed_text()` for hybrid approach
  - ADR-004 updated with decision

- **Git/Test Workflow Commands** (Issue #15)
  - `/test` - Auto-detect and run project tests (make, cargo, go, npm, pytest)
  - `/git status|commit|push|sync` - Git workflow helper
  - `/pr [title]` - Create pull request via gh CLI
  - Detailed help documentation for all commands

- **4 New Development-Focused Agents**
  - Rex (System Design Reviewer) - Architecture and code review
  - Dario (Developer Experience) - DevEx and tooling optimization
  - Otto (CI/CD Specialist) - Pipeline and automation expert
  - Paolo (Backend Architect) - API and microservices design

- **Feature Release Manager Agent**
  - Automated release quality gates
  - Microsoft Engineering Fundamentals compliance
  - Pre-release validation

- **E2E Test Expansion**
  - Tests for /test, /git, /pr commands
  - Embedding strategy tests

### Changed
- Updated agent count display in README (+41 to +45 specialists)
- ISE Engineering Fundamentals added to all technical agents
- Mermaid diagrams updated for MLX, OpenRouter, Ollama, Semantic Memory

### Fixed
- Budget and theme settings now persist across sessions
- License badge corrected from Apache 2.0 to MIT
- CI pipeline optimizations with Xcode 16 for Swift 6.0 / MLX support
- MLX stubs for CI builds without Swift 6.0
- Emoji spinner icons for better UX
- Resolved compiler warnings (pthread_mutex_t type compatibility)
- Fixed sign conversion and double promotion warnings

## [4.0.2] - 2025-12-13

### Fixed
- Link cJSON statically to avoid dylib code signature issues on user machines
- CI workflow now installs cJSON dependency before build
- Cleaned up startup banner layout

## [4.0.0] - 2025-12-13

### Added - Multi-Provider Expansion

**OpenRouter Provider**
- Access to 300+ models via unified OpenAI-compatible API
- `src/providers/openrouter.c` - Full OpenRouter adapter (~780 lines)
- Models: DeepSeek R1, Mistral Large, Llama 3.3 70B, Qwen 2.5 72B, Gemini via OR
- HTTP-Referer and X-Title headers for OpenRouter compliance
- Environment variable: `OPENROUTER_API_KEY`

**Ollama Local Provider**
- Run models locally with zero API costs
- `src/providers/ollama.c` - Full Ollama adapter (~580 lines)
- Models: Llama 3.2, Mistral 7B, Code Llama, DeepSeek Coder V2, Phi-3
- Graceful handling when Ollama service not running
- Environment variable: `OLLAMA_HOST` (default: localhost:11434)

**Setup Wizard**
- `src/core/commands/setup_wizard.c` - Interactive configuration wizard
- New `/setup` command for guided provider configuration
- API key configuration with step-by-step guidance (Keychain, env vars, or session)
- Quick setup profiles:
  - **Cost-Optimized** (default): Cheapest models everywhere
  - **Balanced**: Mix of quality and cost
  - **Performance**: Best models everywhere
  - **Local-First**: Ollama-first with cloud fallback
- View current configuration dashboard
- Provider status with API key validation

**Test Coverage**
- `tests/mocks/mock_openrouter.c` - OpenRouter mock with DeepSeek, Llama, Mistral variants
- `tests/mocks/mock_ollama.c` - Ollama mock with local model simulations
- E2E tests for setup wizard (Section 9)
- Unit tests for OpenRouter and Ollama mock providers

### Changed

- Provider count increased from 4 to 5 (added PROVIDER_OPENROUTER, PROVIDER_OLLAMA)
- `config/models.json` updated with OpenRouter and Ollama model catalogs
- `provider.c` updated with new provider registrations and model arrays
- `tools.c` updated with OpenRouter tool calling (OpenAI format) and Ollama (no tools)
- `streaming.c` updated with OpenRouter SSE and Ollama NDJSON parsing
- Mock provider header consolidated with all provider-specific mock declarations

### Technical

- OpenRouter uses OpenAI-compatible format but requires `HTTP-Referer: https://convergio.dev` header
- Ollama uses different JSON format for chat/streaming - separate parser implementation
- Ollama does NOT support native tool calling (`supports_tools = false`)
- All Ollama models have `$0.00` cost (local inference)
- Setup wizard defaults to cost optimization per user preference

## [3.0.13] - 2025-12-13

### Added

- **Project-aware agent filtering**: `agents` command now shows only project team members when a project is active
- **Project team tool for AI**: New `project_team` tool allows Ali and agents to add/remove team members programmatically
- **Project-aware prompt**: Prompt now shows project name as "Convergio [ProjectName] >" when in project context
- **Non-team agent blocking**: Warning when trying to @mention agents outside the current project team, with option to add them

### Changed

- Agent delegation filtering improved to respect project team boundaries

## [3.0.12] - 2025-12-13

### Added

- **Projects Feature**: Create named projects with dedicated agent teams and persistent context
  - `project create <name>` - Create new project with purpose, team, and optional template
  - `project list` - List all projects with status
  - `project use <name>` - Switch to a project (filters available agents)
  - `project status` - Show detailed project dashboard
  - `project team add/remove` - Manage project team membership
  - `project templates` - Built-in templates: app-dev, marketing, research, executive, finance
  - `project focus/decision` - Track current focus and key decisions
  - `project archive/clear` - Archive projects or clear current context
  - Project history stored in `~/.convergio/projects/<slug>/history.jsonl`
  - Context shared with Ali including purpose, focus, and key decisions
  - Agent delegation filtered to project team members only
- **Fiona Market Analyst**: New finance agent specialized in real-time stock quotes, balance sheet analysis, and market research via web tools
- **Agent prefix matching**: Can now use `@baccio` instead of `@baccio-tech-architect` - matches first segment of agent names
- **E2E test suite**: Comprehensive end-to-end tests in `tests/e2e_test.sh` covering all commands and real API calls
- **Agent delegation tests**: E2E tests now verify Ali's delegation to specialist agents
- **Projects E2E tests**: Full test coverage for project commands
- **ADR 007**: Architecture Decision Record for Projects feature

### Fixed

- **Tool execution**: Disabled streaming by default - tools now work correctly (streaming mode doesn't support tool_use API)
- **Path resolution**: Tools (file_read, file_write, file_list) now resolve relative paths to workspace automatically

### Changed

- **Release workflow**: E2E tests are now BLOCKING for releases in app-release-manager
- **E2E tester**: Moved from Convergio agent to Claude Code command (`/test-convergio`) for external testing

## [3.0.11] - 2025-12-13

### Added

- **Anti-Hallucination Protocol**: New protocol in Ali agent and CommonValuesAndPrinciples.md that forces agents to use tools before making factual claims about files, git status, code state, or any verifiable information. Prevents fabricated data in agent responses.

## [3.0.10] - 2025-12-13

### Added

- **Agent editing from CLI (Issue #20)**: New `agent edit <name>` command opens agent definition in your editor
- **Agent reload command**: New `agent reload` command regenerates embedded agents after editing

### Removed

- **Status bar footer**: Disabled status bar that was causing terminal display issues

## [3.0.9] - 2025-12-12

### Fixed

- **Terminal scrolling**: Removed scroll region that was breaking terminal scrollback

## [3.0.7] - 2025-12-12

### Fixed

- **Auto-update without manual sudo**: Update process now works seamlessly without requiring manual sudo invocation
- **Clean download display**: Hidden useless curl progress bar (GitHub API doesn't send Content-Length header)
- **Download size display**: Shows clean file size in KB during download

### Security

- **Safe privilege escalation**: Uses `posix_spawn()` for sudo calls instead of `system()` - prevents shell injection vulnerabilities
- **Auto-detect sudo requirement**: Automatically detects when elevated privileges are needed and prompts for password securely

## [3.0.6] - 2025-12-12

### Removed

- **Status bar UI**: Completely removed cluttered status bar for cleaner interface

### Fixed

- **Auto-update URL parsing**: Fixed `browser_download_url` JSON parsing that was causing "No download URL found" errors in the update checker

### Added

- **Makefile release target**: Added `make release` target for streamlined release builds

## [3.0.5] - 2025-12-12

### Security

- **CRITICAL: Mutex initialization fix**: Fixed uninitialized mutex in streaming.c causing potential deadlocks
- **HIGH: Keychain security hardening**: Disabled iCloud sync for API keys stored in Keychain (kSecAttrSynchronizable = false)
- **MEDIUM: Buffer overflow prevention**: Replaced sprintf with snprintf in oauth.m

### Fixed

- **Model pricing accuracy**: cost.c now uses actual model prices instead of hardcoded values
- **OOM safety**: Added NULL checks after memory allocations in multiple files
- **TTY fallback**: ANSI output now falls back gracefully when not in a TTY
- **Test linking**: Fixed test compilation with globals.h and test_stubs.c
- **Agent config loading**: agent_config_load() was never being called - now properly loads agent configurations from JSON files

### Added

- **Enhanced agentic capabilities (Issue #15)**: Integrated agentic tools with REPL commands
- **Privacy-first telemetry system (Issue #14)**: Optional anonymous usage analytics
- **Model Competition & Benchmark commands (Issue #13)**: Compare model responses side-by-side
- **Multi-provider API key check**: Validates API keys at startup for all configured providers
- **News command**: View release notes directly from CLI
- **Help documentation verification**: Build-time check ensures all commands have proper documentation

### Changed

- **File organization (Issue #11)**: Split oversized main.c and orchestrator.c into smaller modules
- **Prompt styling**: Restored 'Convergio >' prompt with blinking block cursor

### Technical

- Updated .env.example with all required provider keys
- Added separator line styling like Claude Code
- Improved test suite with globals.h integration

## [3.0.4] - 2025-12-12

### Fixed

- **Update command error handling**: Now shows clear error messages instead of silent failures
- **Homebrew detection**: When installed via Homebrew, redirects user to `brew upgrade convergio`
- **Permission errors**: Clear messages when update fails due to filesystem permissions

## [3.0.3] - 2025-12-12

### Added

**Anti-Hallucination Constitution**
- All agents now have a mandatory constitution enforcing brutal honesty
- Rule 1: ABSOLUTE HONESTY - Never fabricate or guess information
- Rule 2: UNCERTAINTY DISCLOSURE - Explicitly state uncertainty levels (0-100%)
- Rule 3: SOURCE ATTRIBUTION - Clearly state where information comes from
- Rule 4: ERROR ACKNOWLEDGMENT - Admit mistakes immediately when discovered
- Rule 5: LIMITATION TRANSPARENCY - Be clear about what the agent cannot do

**Agent Command Improvements**
- Added `agent list` subcommand for listing all available agents
- Added `agent info <name>` subcommand for detailed agent information
- Added `agent_get_all()` function for autocomplete of all registered agents

### Fixed

- Improved updater error messages for empty download URLs

### Technical

**Complete Makefile Integration** - Added 13 missing source files from the multimodal roadmap:
- Provider layer: `provider.c`, `anthropic.c`, `openai.c`, `gemini.c`, `retry.c`, `streaming.c`, `tokens.c`, `tools.c`
- Router: `model_router.c`, `cost_optimizer.c`
- Agent config: `agent_config.c`
- Sync: `file_lock.c`
- UI: `hyperlink.c`

## [3.0.2] - 2025-12-12

### Changed

- **Branding update**: Updated startup banner gradient colors to match Convergio logo (teal to turquoise to cyan to slate blue to purple to magenta)
- **Tagline**: Changed from "A semantic kernel for human-AI symbiosis" to "Human purpose. AI momentum."
- Updated help text and file header comment with new tagline

## [3.0.1] - 2025-12-12

### Security

- **CRITICAL: Command injection fix in updater**: Replaced `system()` calls with `posix_spawn()` in `src/core/updater.c`
  - Added version string validation to prevent malicious version strings
  - All shell commands now use direct exec (mkdir, tar, rm) without shell interpretation
  - Eliminates command injection vector in auto-update system
- **HIGH: Thread safety fix in shell_exec**: Changed from `getcwd()`/`chdir()` to `fchdir()` in `src/tools/tools.c`
  - Uses file descriptor to save/restore working directory
  - Prevents race conditions in multi-threaded tool execution
- **MEDIUM: OOM safety improvements**: Added NULL checks after `realloc()` in `src/tools/tools.c`
  - `tool_file_list()` - prevents crash on memory exhaustion
  - `tool_shell_exec()` - prevents crash on memory exhaustion
  - `tool_note_read()` - prevents crash on memory exhaustion
- **LOW: Undefined behavior fix**: Cast to `unsigned char` before `tolower()` calls
  - `src/intent/interpreter.c` - `detect_pattern()` function
  - `src/tools/tools.c` - `sanitize_grep_pattern()` and `tool_note_read()` functions
  - Prevents UB with negative char values on non-ASCII input

### Fixed

- **Homebrew formula URL**: Corrected tarball URL format in `Formula/convergio.rb`

## [3.0.0] - 2025-12-12

### Added - Multi-Provider Architecture

**Provider Support**
- **Anthropic** (Claude Opus 4.5, Sonnet 4.5, Haiku 4.5)
- **OpenAI** (GPT-5.2-pro, GPT-5, GPT-4o, o3, o4-mini, GPT-5-nano)
- **Google Gemini** (Gemini 3 Ultra, Pro, Flash)
- **Ollama** (Local models - planned)

**Provider Abstraction Layer**
- `include/nous/provider.h` - Unified provider interface
- `src/providers/provider.c` - Provider registry and model catalog
- `src/providers/anthropic.c` - Anthropic Claude adapter
- `src/providers/openai.c` - OpenAI GPT adapter
- `src/providers/gemini.c` - Google Gemini adapter
- `src/providers/retry.c` - Exponential backoff with circuit breaker
- `src/providers/streaming.c` - Server-Sent Events streaming handler
- `src/providers/tools.c` - Multi-provider tool/function calling
- `src/providers/tokens.c` - Token estimation and cost calculation

**Intelligent Model Routing**
- `src/router/model_router.c` - Smart model selection per agent
- `src/router/cost_optimizer.c` - Prompt caching, batch processing, budget management
- Agent-specific model assignments (Ali→Opus, Marco→Sonnet, etc.)
- Automatic fallback chains when providers fail
- Budget-aware model downgrading

**Agent Configuration System**
- `src/agents/agent_config.c` - JSON-based agent configuration
- Per-agent model, provider, and behavior settings
- Runtime configuration updates
- Default configurations for all built-in agents

**Synchronization & Concurrency**
- `include/nous/file_lock.h` + `src/sync/file_lock.c` - File-level locking
- Deadlock detection for multi-agent file access
- Read-write lock semantics with timeout support
- Enhanced message bus with provider-aware routing
- Priority message queue and topic-based subscriptions

**UI Enhancements**
- `include/nous/statusbar.h` + `src/ui/statusbar.c` - Two-line status bar
- `include/nous/hyperlink.h` + `src/ui/hyperlink.c` - OSC 8 terminal hyperlinks
- `src/ui/terminal.c` - SIGWINCH handling and terminal capability detection
- Real-time token counter and cost display
- Clickable file paths in output (iTerm2, WezTerm, VS Code Terminal)

**Testing Framework**
- `tests/mock_provider.h` + `tests/mock_provider.c` - Mock LLM provider
- `tests/test_providers.c` - Comprehensive provider unit tests
- Configurable latency, error injection, rate limiting simulation
- Request logging and assertion helpers

**Documentation**
- `docs/PROVIDERS.md` - Provider setup guide with model pricing
- `docs/MODEL_SELECTION.md` - Model selection and routing guide
- `docs/COST_OPTIMIZATION.md` - Cost management and optimization guide
- `docs/AGENT_DEVELOPMENT.md` - Custom agent creation guide
- `docs/MIGRATION_v3.md` - v2.x to v3.0 migration guide
- `docs/TROUBLESHOOTING.md` - Common issues and solutions
- `docs/DISCLAIMER.md` - AI limitations and liability disclaimer
- `TERMS_OF_SERVICE.md` - Terms of Service
- `PRIVACY_POLICY.md` - Privacy Policy
- `config/models.json` - Complete model catalog with pricing

### Changed
- **BREAKING**: Configuration moved from `~/.nous/` to `~/.convergio/`
- **BREAKING**: Agent definitions now use JSON format instead of Markdown
- Default model changed from `claude-sonnet-4` to `claude-sonnet-4.5`
- CMakeLists.txt updated with all new source files
- Enhanced message bus with multi-provider support

### Technical
- Provider interface uses function pointers for polymorphism
- Circuit breaker pattern prevents cascade failures
- Token estimation based on content type detection
- BPE-approximation token counting per provider
- Context window management with automatic truncation
- Cost tracking per provider, model, and agent

## [2.0.11] - 2025-12-12

### Added
- **Clipboard image paste support**: Ctrl+V now pastes clipboard content
  - If clipboard contains an image, it's saved to /tmp and the path is inserted
  - Also works with regular text paste
  - New files: `include/nous/clipboard.h`, `src/core/clipboard.m`
- **Autocomplete @agent anywhere**: Tab-completion for agent names now works anywhere in the line, not just at the beginning

### Changed
- **Working directory improvements**: CLI now works in the directory from which it's launched
  - Workspace is communicated to Ali in system prompt
  - `shell_exec` uses workspace as default directory
- **Bold prompt**: "Convergio" in the prompt now uses combined ANSI sequences for guaranteed bold display
- **Improved agents list display**: `/agents` command shows a more compact and readable list with role grouping and colors
- **Simplified update command**: `convergio update` now checks and directly asks to install (no longer need separate `update install`)

## [2.0.10] - 2025-12-12

### Fixed
- Agents streaming markdown improvements
- Various stability fixes

## [2.0.9] - 2025-12-12

### Fixed
- Hardware info box drawing alignment

## [2.0.8] - 2025-12-12

### Added
- **Custom HTTP headers in web_fetch**: Support for `headers_json` parameter
  - Parse JSON object `{"Header": "Value"}` and apply to CURL requests
  - Enables authenticated API calls from tools
- **Category field in memory_store**: Memories now support categorization
  - New `category` column in database schema
  - Pass category as second parameter to `persistence_save_memory()`

### Changed
- TODO comments now reference GitHub issues for tracking (#1, #2, #3)
- Updated `persistence_save_memory()` signature to include category parameter

### Technical
- Added `parse_headers_json()` helper in tools.c for JSON header parsing
- Updated memory schema with category column (default: 'general')
- Created GitHub issues for future enhancements:
  - Issue #1: Semantic search implementation
  - Issue #2: Load pre-trained MLX embedding weights
  - Issue #3: Use proper embedding model

## [2.0.7] - 2025-12-12

### Added
- **Debug mutex system**: ERRORCHECK mutex in debug builds for deadlock detection
  - Automatically detects double-lock and unlock-not-owned errors
  - Lazy initialization for compatibility with static declarations
- **Safe path helper**: New `safe_path.h` module for secure file operations
  - `safe_path_resolve()` - realpath with boundary checking
  - `safe_path_join()` - safe path concatenation
  - `safe_path_open()` - TOCTOU-safe file opening
  - Protection against path traversal attacks
- **Centralized CURL helpers**: `curl_helpers.h` for consistent API calls
  - `claude_build_headers()` - unified header construction
  - `claude_setup_common_opts()` - common CURL options
  - `claude_handle_result()` - consistent error handling
- **Unit test suite**: 50 new tests for core components
  - Safe path resolution and boundary checking
  - Command and path sandbox validation
  - Run with `make unit_test` or `make test`
- **clang-tidy configuration**: Static analysis rules in `.clang-tidy`
- **Enhanced CI workflow**: Additional quality gates
  - Debug build with sanitizers
  - Static analysis job
  - Security scanning

### Changed
- All mutex operations now use `CONVERGIO_MUTEX_*` macros
- Refactored CURL usage in `claude.c` to use centralized helpers
- Improved Makefile with `test`, `unit_test` targets

### Technical
- Added `include/nous/debug_mutex.h` - Debug mutex wrapper
- Added `include/nous/safe_path.h` - Safe path operations
- Added `include/nous/curl_helpers.h` - CURL utilities
- Added `src/core/safe_path.c` - Safe path implementation
- Added `tests/test_unit.c` - Unit test suite
- Updated `.clang-tidy` - Static analysis configuration
- Updated `.github/workflows/ci.yml` - Enhanced CI pipeline
- Updated `.claude/agents/app-release-manager.md` - New quality checks

## [2.0.6] - 2025-12-12

### Added
- **Theme system**: 4 terminal color themes to customize appearance
  - `theme ocean` - Cool blue/cyan tones (default)
  - `theme forest` - Green nature tones
  - `theme sunset` - Warm orange/red tones
  - `theme mono` - Classic grayscale
- **Streaming markdown**: Live rendering as responses arrive (toggle with `stream on/off`)
- **Colored user input**: User text now has distinct color based on theme
- **Interactive budget management**: When budget is exceeded, choose to increase/set/view
- **First-run onboarding**: Clear API key setup wizard with browser auto-open

### Fixed
- **Readline prompt bug**: Fixed text corruption when typing long inputs (proper ANSI marker wrapping)
- **CRITICAL: DB path**: Now uses `~/.convergio/convergio.db` instead of repo-relative `data/`
- **CRITICAL: NULL crash**: Fixed crash when `metadata_json` is NULL in persistence
- **Notes/knowledge paths**: Now use `~/.convergio/notes` and `~/.convergio/knowledge`
- CMakeLists.txt updated with all source files for CMake builds

### Changed
- Default streaming mode enabled for faster perceived response time
- Theme preference persisted in config

## [2.0.5] - 2025-12-11

### Fixed
- **Agents now use tools**: Direct agent communication (`@agent`) now includes tool instructions
- Agents are explicitly told they CAN use file_write, web_fetch, etc.
- Enhanced system prompt appended to all agents when called directly

## [2.0.4] - 2025-12-11

### Added
- **ESC key cancellation**: Press ESC while "pensando..." to cancel requests instantly
- **Direct agent communication**: Use `@agent_name message` to talk directly to any specialist
  - Example: `@baccio How should I architect this API?`
  - Example: `@luca Is this code secure?`
- **Full tool support for all agents**: Every agent can now use web_fetch, file_read, shell_exec, etc.
- New `orchestrator_agent_chat()` function for direct agent communication with tool loop

### Changed
- Spinner now shows "(ESC to cancel)" hint
- `agents` command shows `@name` format for easy copy-paste
- Improved agents list with usage tip

### Technical
- Added curl progress callback (`CURLOPT_XFERINFOFUNCTION`) to all API calls
- Terminal switches to raw mode during spinner for ESC detection
- Request cancellation via `claude_cancel_request()` / `claude_reset_cancel()`

## [2.0.3] - 2025-12-11

### Fixed
- **CRITICAL**: Agent definitions now embedded in binary (fixes Homebrew installation showing only 1 agent)
- Added `agent_load_definitions()` call during orchestrator initialization

### Changed
- Agent definitions compiled into binary instead of loaded from filesystem
- Build system now auto-generates `embedded_agents.c` from markdown files
- No longer requires `src/agents/definitions/` directory at runtime

### Added
- `scripts/embed_agents.sh` - Script to embed agent definitions in binary
- `include/nous/embedded_agents.h` - Header for embedded agents API

## [2.0.2] - 2025-12-11

### Removed
- OAuth `login` command (requires Anthropic OAuth client registration - see ADR 005)

### Changed
- Authentication now uses API key only (via environment variable, Keychain, or config file)
- Updated help messages to reflect API key authentication

### Added
- ADR 005: OAuth Authentication documentation explaining requirements for future enablement

## [2.0.1] - 2025-12-11

### Fixed
- OAuth scope URL encoding causing "Richiesta OAuth non valida" error

## [2.0.0] - 2025-12-11

### Changed
- **BREAKING**: Auto-detect Apple Silicon chip type at runtime (M1/M2/M3/M4 all variants)
- Removed hardcoded M3 Max optimizations - now dynamically configured
- Updated Makefile to use generic arm64 target instead of apple-m3

### Added
- **Auto-Update System**: `convergio update` command to check/install updates from GitHub
- **Configuration Management**: TOML config file at `~/.convergio/config.toml`
- **Keychain Integration**: Secure API key storage in macOS Keychain
- **Hardware Detection**: Runtime detection of CPU cores, GPU cores, Neural Engine, memory
- **Homebrew Distribution**: Install via `brew tap Roberdan/convergio-cli && brew install convergio`
- **GitHub Actions CI/CD**: Automated builds, code signing, notarization
- **Version Management**: Semantic versioning with `--version` flag
- **Setup Wizard**: `convergio setup` command for initial configuration
- New CLI commands: `update`, `hardware`, `setup`

### Technical Details
- Added `include/nous/hardware.h` - Hardware detection API
- Added `include/nous/config.h` - Configuration management API
- Added `include/nous/updater.h` - Auto-update API
- Added `src/core/hardware.m` - Apple Silicon detection via sysctl/Metal
- Added `src/core/config.c` - TOML parser and path management
- Added `src/core/updater.c` - GitHub Releases API integration
- Added `src/auth/keychain.m` - macOS Security.framework integration
- Added `.github/workflows/ci.yml` - Continuous integration
- Added `.github/workflows/release.yml` - Release automation
- Added `Formula/convergio.rb` - Homebrew formula
- Added `VERSION` file for version tracking

## [1.1.0] - 2025-12-11

### Security Hardening
- **CRITICAL**: Fixed buffer overflow vulnerabilities in `claude.c` (offset-based bounds tracking)
- **CRITICAL**: Fixed tool result buffer overflow in `orchestrator.c` (dynamic allocation)
- **CRITICAL**: Fixed command injection via grep in `tools.c` (sanitize_grep_pattern)
- **CRITICAL**: Fixed shell blocklist bypass in `tools.c` (normalize_command with escape stripping)
- **HIGH**: Fixed path traversal boundary check in `tools.c` (is_path_within function)
- **HIGH**: Fixed SQLite use-after-free in `persistence.c` (SQLITE_TRANSIENT)
- **HIGH**: Fixed JSON parsing escape issues in `claude.c` (is_quote_escaped)
- **MEDIUM**: Fixed signal handler unsafe printf in `main.c` (async-signal-safe write)
- **MEDIUM**: Fixed race conditions in `tools.c` (pthread mutex for global state)
- **MEDIUM**: Fixed timing attacks in `oauth.m` (secure_memcmp constant-time)
- **MEDIUM**: Fixed TOCTOU in file operations in `tools.c` (safe_open with O_NOFOLLOW)
- **LOW**: Fixed integer overflow in cost calculations in `persistence.c` (INT64_MAX clamping)
- **LOW**: Fixed empty command not blocked in `tools.c` (early return)

### Added
- Fuzz test suite (`make fuzz_test`) - 37 tests for security validation
- Constant-time comparison function for token validation
- Secure memory wiping functions (secure_zero, secure_free)
- Shell escaping function for safe command construction
- TOCTOU-safe file open functions

### Fixed
- `__bridge_transfer` warning in oauth.m (CFBridgingRelease)
- ftell() error handling in tools.c (3 locations)
- Double-free prevention in orchestrator.c (NULL after free)
- Memory leak of ExecutionPlan in orchestrator.c

## [1.0.0] - 2025-12-11

### Added
- **Ali - Chief of Staff**: Central orchestrator agent that coordinates all other agents
- **53 Specialist Agents**: Dynamically loaded from `src/agents/definitions/`
- **Parallel Multi-Agent Orchestration**: GCD-based parallel execution of multiple agents
- **Real-time Agent Status Tracking**: See which agents are working and on what
- **Inter-Agent Communication**: Message bus for agent-to-agent messaging
- **Tool Execution System**:
  - `file_read`, `file_write`, `file_list` - File operations with safety restrictions
  - `shell_exec` - Shell command execution with blocked dangerous commands
  - `web_fetch` - HTTP/HTTPS content fetching
  - `memory_store`, `memory_search` - Semantic memory with RAG
  - `note_write`, `note_read`, `note_list` - Markdown notes system
  - `knowledge_add`, `knowledge_search` - Knowledge base management
- **Conversation Memory**: Persistent conversation history across sessions
- **Cost Control**: Granular budget tracking with per-agent attribution
- **Debug Logging System**: 5 levels (ERROR, WARN, INFO, DEBUG, TRACE)
- **CLI Options**: `--debug`, `--trace`, `--quiet` flags
- **Command Support**: Both `quit` and `/quit` syntax supported
- **Colorful ASCII Banner**: Horizontal gradient banner at startup
- **SQLite Persistence**: WAL mode for better concurrency
- **Thread-Safe Design**: Per-request curl handles for parallel API calls

### Technical Details
- Written in pure C/Objective-C
- Optimized for Apple Silicon M3 Max
- Uses Metal GPU shaders and NEON SIMD
- MLX-compatible transformer for local embeddings (weights pending)
- Binary size ~100KB

## [0.1.0] - 2025-12-10

### Added
- Initial project structure
- Basic Claude API integration
- Simple REPL interface
- Semantic fabric foundation

---

[Unreleased]: https://github.com/Roberdan/convergio-cli/compare/v5.4.0...HEAD
[5.4.0]: https://github.com/Roberdan/convergio-cli/compare/v5.3.1...v5.4.0
[5.3.1]: https://github.com/Roberdan/convergio-cli/compare/v5.3.0...v5.3.1
[5.3.0]: https://github.com/Roberdan/convergio-cli/compare/v5.2.2...v5.3.0
[5.2.2]: https://github.com/Roberdan/convergio-cli/compare/v5.2.1...v5.2.2
[5.2.1]: https://github.com/Roberdan/convergio-cli/compare/v5.2.0...v5.2.1
[5.2.0]: https://github.com/Roberdan/convergio-cli/compare/v5.1.0...v5.2.0
[5.1.0]: https://github.com/Roberdan/convergio-cli/compare/v5.0.1...v5.1.0
[5.0.1]: https://github.com/Roberdan/convergio-cli/compare/v5.0.0...v5.0.1
[5.0.0]: https://github.com/Roberdan/convergio-cli/compare/v4.2.0...v5.0.0
[4.2.0]: https://github.com/Roberdan/convergio-cli/compare/v4.1.0...v4.2.0
[4.1.0]: https://github.com/Roberdan/convergio-cli/compare/v4.0.0...v4.1.0
[4.0.0]: https://github.com/Roberdan/convergio-cli/compare/v3.0.13...v4.0.0
[3.0.13]: https://github.com/Roberdan/convergio-cli/compare/v3.0.12...v3.0.13
[3.0.12]: https://github.com/Roberdan/convergio-cli/compare/v3.0.11...v3.0.12
[3.0.11]: https://github.com/Roberdan/convergio-cli/compare/v3.0.10...v3.0.11
[3.0.10]: https://github.com/Roberdan/convergio-cli/compare/v3.0.9...v3.0.10
[3.0.9]: https://github.com/Roberdan/convergio-cli/compare/v3.0.7...v3.0.9
[3.0.7]: https://github.com/Roberdan/convergio-cli/compare/v3.0.6...v3.0.7
[3.0.6]: https://github.com/Roberdan/convergio-cli/compare/v3.0.5...v3.0.6
[3.0.5]: https://github.com/Roberdan/convergio-cli/compare/v3.0.4...v3.0.5
[3.0.4]: https://github.com/Roberdan/convergio-cli/compare/v3.0.3...v3.0.4
[3.0.3]: https://github.com/Roberdan/convergio-cli/compare/v3.0.2...v3.0.3
[3.0.2]: https://github.com/Roberdan/convergio-cli/compare/v3.0.1...v3.0.2
[3.0.1]: https://github.com/Roberdan/convergio-cli/compare/v3.0.0...v3.0.1
[3.0.0]: https://github.com/Roberdan/convergio-cli/compare/v2.0.11...v3.0.0
[2.0.11]: https://github.com/Roberdan/convergio-cli/compare/v2.0.10...v2.0.11
[2.0.10]: https://github.com/Roberdan/convergio-cli/compare/v2.0.9...v2.0.10
[2.0.9]: https://github.com/Roberdan/convergio-cli/compare/v2.0.8...v2.0.9
[2.0.8]: https://github.com/Roberdan/convergio-cli/compare/v2.0.7...v2.0.8
[2.0.7]: https://github.com/Roberdan/convergio-cli/compare/v2.0.6...v2.0.7
[2.0.6]: https://github.com/Roberdan/convergio-cli/compare/v2.0.5...v2.0.6
[2.0.5]: https://github.com/Roberdan/convergio-cli/compare/v2.0.4...v2.0.5
[2.0.4]: https://github.com/Roberdan/convergio-cli/compare/v2.0.3...v2.0.4
[2.0.3]: https://github.com/Roberdan/convergio-cli/compare/v2.0.2...v2.0.3
[2.0.2]: https://github.com/Roberdan/convergio-cli/compare/v2.0.1...v2.0.2
[2.0.1]: https://github.com/Roberdan/convergio-cli/compare/v2.0.0...v2.0.1
[2.0.0]: https://github.com/Roberdan/convergio-cli/releases/tag/v2.0.0
[1.1.0]: https://github.com/Roberdan/convergio-cli/releases/tag/v1.1.0
[1.0.0]: https://github.com/Roberdan/convergio-cli/releases/tag/v1.0.0
[0.1.0]: https://github.com/Roberdan/convergio-cli/releases/tag/v0.1.0
