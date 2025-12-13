# Changelog

All notable changes to Convergio CLI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
- **49 Specialist Agents**: Dynamically loaded from `src/agents/definitions/`
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

[Unreleased]: https://github.com/Roberdan/convergio-cli/compare/v4.0.0...HEAD
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
