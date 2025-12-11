# Changelog

All notable changes to Convergio Kernel will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/Roberdan/kernel/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Roberdan/kernel/releases/tag/v1.0.0
[0.1.0]: https://github.com/Roberdan/kernel/releases/tag/v0.1.0
