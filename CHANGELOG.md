# Changelog

All notable changes to Convergio CLI will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/Roberdan/convergio-cli/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/Roberdan/convergio-cli/releases/tag/v2.0.0
[1.1.0]: https://github.com/Roberdan/convergio-cli/releases/tag/v1.1.0
[1.0.0]: https://github.com/Roberdan/convergio-cli/releases/tag/v1.0.0
[0.1.0]: https://github.com/Roberdan/convergio-cli/releases/tag/v0.1.0
