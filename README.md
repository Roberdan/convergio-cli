<p align="center">
  <img src="docs/logo/convergio_header.jpeg" alt="Convergio" width="600"/>
</p>

<h1 align="center">Convergio CLI</h1>

<p align="center">
  <strong>Your Virtual AI Executive Team</strong><br/>
  <em>54 specialized AI agents. One command. Unlimited expertise.</em>
</p>

<p align="center">
  <a href="https://github.com/Roberdan/convergio-cli/actions/workflows/ci.yml"><img src="https://github.com/Roberdan/convergio-cli/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/Roberdan/convergio-cli/releases/latest"><img src="https://img.shields.io/badge/version-5.3.1-blue" alt="Version 5.3.1"></a>
  <a href="https://github.com/Roberdan/convergio-cli/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License"></a>
  <a href="https://github.com/Roberdan/convergio-cli"><img src="https://img.shields.io/badge/platform-macOS%20(Apple%20Silicon)-black" alt="Platform"></a>
  <a href="https://github.com/Roberdan/convergio-cli/stargazers"><img src="https://img.shields.io/github/stars/Roberdan/convergio-cli?style=social" alt="Stars"></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/coverage-96%25-brightgreen" alt="Coverage 96%">
  <img src="https://img.shields.io/badge/tests-306%20passing-success" alt="306 Tests">
  <img src="https://img.shields.io/badge/agents-54%20specialists-purple" alt="54 AI Agents">
  <img src="https://img.shields.io/badge/C17-Standard-blue" alt="C17">
  <img src="https://img.shields.io/badge/Swift-5.9-orange" alt="Swift 5.9">
  <img src="https://img.shields.io/badge/Metal-GPU%20Accelerated-gray" alt="Metal GPU">
</p>

<p align="center">
  <a href="#-quick-start">Quick Start</a> •
  <a href="#-features">Features</a> •
  <a href="#-anna-executive-assistant">Anna Assistant</a> •
  <a href="#-mlx-local-ai">MLX Local AI</a> •
  <a href="#-documentation">Docs</a>
</p>

---

<h2 align="center">Install in 10 Seconds</h2>

```bash
brew tap Roberdan/convergio-cli && brew install convergio
```

<p align="center">
  <em>Then just run <code>convergio</code> and start talking to Ali!</em>
</p>

---

## The Vision

**Imagine having an entire consulting firm at your fingertips.** Not just one AI assistant — a full team of 54 specialists who work together, debate ideas, and deliver integrated solutions.

```
┌─────────────────────────────────────────────────────────────────┐
│                         YOU                                      │
│              "Launch a global SaaS platform"                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    ALI (Chief of Staff)                          │
│         Analyzes → Delegates → Coordinates → Synthesizes         │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
   ┌─────────┐          ┌─────────┐          ┌─────────┐
   │  Domik  │          │  Baccio │          │   Amy   │   ← PARALLEL
   │Strategy │          │  Tech   │          │ Finance │
   └─────────┘          └─────────┘          └─────────┘
        │                     │                     │
        └─────────────────────┴─────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    CONVERGENCE                                   │
│    Integrated strategy + architecture + financial plan           │
└─────────────────────────────────────────────────────────────────┘
```

**This is not a chatbot. This is a virtual consulting team.**

---

## Coming Soon: Convergio Native for macOS

<p align="center">
  <img src="https://img.shields.io/badge/🚧_IN_DEVELOPMENT-Native_macOS_App-purple?style=for-the-badge" alt="In Development"/>
</p>

We're building a **stunning native macOS application** with Apple's Liquid Glass design language! The CLI will remain fully supported, but soon you'll have a beautiful GUI option.

**Features in development:**
- 🎨 Liquid Glass UI with glass-morphism effects
- 🤖 Real-time Agent Interaction Visualizer
- ⚙️ Full Settings UI (API keys, budgets, MCP servers)
- 📝 Agent Markdown Editor with live preview
- 📚 Comprehensive in-app Help System
- 🔔 Native macOS notifications
- ⌨️ Global hotkey (Cmd+Shift+Space)

**Want to contribute or follow progress?**
→ Check out the [`feature/native-app`](https://github.com/Roberdan/convergio-cli/tree/feature/native-app) branch!

```bash
git clone https://github.com/Roberdan/convergio-cli.git
cd convergio-cli
git checkout feature/native-app
cd ConvergioApp && xcodegen && open ConvergioApp.xcodeproj
```

---

## What's New in v5.1.0

### Plan Database - Persistent Execution Plans

Track and manage multi-step execution plans with SQLite-backed persistent storage.

```bash
# List all execution plans
> /plan list

# Show detailed plan status
> /plan status abc123

# Export plan to Markdown with Mermaid diagrams
> /plan export abc123

# Cleanup old plans
> /plan cleanup 30
```

**Plan Database Features:**
- **ACID transactions** - Thread-safe multi-agent coordination
- **Progress tracking** - Real-time task completion stats
- **Export to Markdown** - With Mermaid Gantt charts
- **Automatic cleanup** - Remove old plans after N days

### Output Service - Structured Document Generation

Generate rich Markdown documents with Mermaid diagrams, tables, and clickable links.

```bash
# List recent outputs
> /output list

# Show most recent output
> /output latest

# Open output in default app
> /output open /path/to/report.md

# Check disk usage
> /output size

# Cleanup old outputs
> /output cleanup 30
```

**Output Service Features:**
- **Mermaid diagrams** - Flowcharts, sequence, Gantt, pie, mindmap
- **Table generation** - Formatted Markdown tables
- **OSC8 hyperlinks** - Clickable file paths in terminal (iTerm2, Warp, Kitty)
- **Auto-organization** - Files sorted by date/project

---

## What's New in v5.0.0

### Anna Executive Assistant - Your AI-Powered Personal Productivity System

Anna is your dedicated executive assistant agent with native task management, smart reminders, and proactive scheduling — all running locally with zero cloud dependencies.

```bash
# Natural language task creation
> /todo add "Review Q4 budget" high due:tomorrow

# Quick reminders with natural language dates
> /remind "Call John about the merger" in 2 hours
> /remind "Quarterly review" next Monday at 9am
> /remind "Dentist appointment" Dec 25 at 10:00

# Smart scheduling (English + Italian)
> /remind "Riunione team" domani alle 15:00
```

**Anna Features:**
- **Native Todo Manager**: Priorities, due dates, subtasks, tags, recurrence (iCal RRULE)
- **Full-Text Search**: FTS5-powered search across all tasks
- **Smart Reminders**: Natural language date parsing in English and Italian
- **Background Daemon**: macOS notifications even when Convergio is closed
- **Inbox System**: Quick capture without immediate processing
- **Statistics**: Daily/weekly completion tracking

### MLX Local AI - 100% Offline, Zero Cost

Run AI models completely offline on your Mac. No API keys, no internet, no costs. Pure Apple Silicon performance.

```bash
# Start with local AI
convergio --local --model llama-3.2-3b

# Or use the setup wizard
> /setup
# Select "Local Models" → Download a model
```

**Available Local Models:**

| Model | Size | RAM | Best For |
|-------|------|-----|----------|
| **Llama 3.2 1B** | 1.5GB | 8GB | Ultra-fast, basic tasks |
| **Llama 3.2 3B** | 3.1GB | 8GB | Balanced (recommended) |
| **DeepSeek R1 1.5B** | 1.2GB | 8GB | Fast reasoning |
| **DeepSeek R1 7B** | 4.5GB | 16GB | Strong reasoning |
| **DeepSeek R1 14B** | 8.5GB | 24GB | Best reasoning |
| **Qwen 2.5 Coder 7B** | 4.5GB | 16GB | Code generation |
| **Mistral 7B Q4** | 4.5GB | 16GB | Multilingual |
| **Llama 3.1 8B Q4** | 5GB | 16GB | Best overall quality |

**Why Local?**
- **Privacy**: Data never leaves your Mac
- **Offline**: Works without internet
- **Free**: Zero API costs forever
- **Fast**: No network latency
- **Optimized**: Metal GPU + Neural Engine acceleration

### MCP Integration - Extensible Tool Ecosystem

Connect to any Model Context Protocol server to extend Convergio's capabilities.

```bash
# List connected MCP servers
> /mcp list

# Connect to a new server
> /mcp connect filesystem

# View available tools
> /mcp tools
```

**MCP Features:**
- **Multi-Transport**: stdio, HTTP POST, Server-Sent Events (SSE)
- **Auto-Discovery**: Automatic tool detection from servers
- **JSON-RPC 2.0**: Full protocol compliance
- **MCP 2025-06-18**: Latest specification support

### Latest AI Models (December 2025)

| Provider | Models | Highlights |
|----------|--------|------------|
| **Anthropic** | Claude Opus 4.5, Claude Sonnet 4.5, Claude Haiku 4.5 | Extended thinking, tool use, vision |
| **OpenAI** | GPT-5.2, GPT-5.2 Pro, o3, o3-mini, o4-mini | Advanced reasoning, multimodal |
| **Google** | Gemini 3.0 Pro, Gemini 3.0 Deep Think, Gemini 2.0 Flash | 2M token context, thinking mode |
| **OpenRouter** | DeepSeek R1, Llama 3.3, Mistral Large 2, Qwen 2.5 | 300+ models, competitive pricing |
| **Ollama** | Any local model | Self-hosted, privacy-first |
| **MLX** | Llama, DeepSeek, Qwen, Mistral | 100% offline Apple Silicon |

### Apple Silicon M5 Support

Full support for the complete Apple Silicon family:

| Chip | Status | Optimization |
|------|--------|--------------|
| **M5 / M5 Pro / M5 Max / M5 Ultra** | Native | Full Neural Engine + Metal 3 |
| **M4 / M4 Pro / M4 Max** | Native | Neural Engine + Metal 3 |
| **M3 / M3 Pro / M3 Max** | Native | Neural Engine + Metal 3 |
| **M2 / M2 Pro / M2 Max / M2 Ultra** | Native | Neural Engine + Metal |
| **M1 / M1 Pro / M1 Max / M1 Ultra** | Native | Neural Engine + Metal |

---

## Features

### Multi-Agent Orchestration

**54 Specialist Agents** working in parallel, coordinated by Ali:

| Agent | Role | Specialty |
|-------|------|-----------|
| **Ali** | Chief of Staff | Orchestration, synthesis |
| **Domik** | McKinsey Partner | Strategic decisions, ISE prioritization |
| **Amy** | CFO | Financial analysis, ROI modeling |
| **Baccio** | Tech Architect | System design, scalable architecture |
| **Luca** | Security Expert | Cybersecurity, penetration testing |
| **Marco** | DevOps Engineer | CI/CD, Infrastructure as Code |
| **Sara** | UX Designer | User-centered design |
| **Omri** | Data Scientist | ML, predictive modeling, analytics |
| **Sofia** | Marketing Strategist | Growth hacking, brand strategy |
| **Anna** | Executive Assistant | Tasks, reminders, productivity |
| **Rex** | System Design | Architecture patterns |
| **Dario** | DevEx | Developer experience |
| **Otto** | CI/CD Specialist | Pipeline optimization |
| **Paolo** | Backend Architecture | API design, microservices |
| ... | **+40 more** | Legal, HR, Creative, PM, QA, etc. |

### Intelligent Model Routing

Automatically select the best model for each task:

```
Budget > $3.00  →  Claude Opus 4.5 / GPT-5.2 Pro (full capability)
Budget > $1.00  →  Claude Sonnet 4.5 / GPT-5.2 (balanced)
Budget > $0.10  →  Claude Haiku 4.5 / Gemini Flash (fast, cheap)
Budget < $0.10  →  Session paused (user confirmation required)
```

### Semantic Memory & Knowledge Graph

Persistent, interconnected memories that survive restarts:

```bash
# Store memories
> /remember Roberto prefers TypeScript over JavaScript

# Semantic search (by meaning, not keywords)
> /search what languages does Roberto prefer

# View knowledge graph
> /graph

# Delete memories
> /forget 0x12345678
```

### Context Compaction - Unlimited Conversations

Never lose context. Convergio automatically compacts long conversations:

- **Dynamic Thresholds**: Adapts to model context window
- **Intelligent Summarization**: Preserves key information
- **5 Checkpoints**: Restore any previous state
- **Cost Optimized**: Uses economical models for summarization

### Response Styles

Customize how agents respond:

```bash
> /style flash      # Ultra-concise, immediate
> /style concise    # Brief, to-the-point
> /style balanced   # Moderate detail (default)
> /style detailed   # Comprehensive, thorough
```

### Model Comparison & Benchmarking

Compare responses from multiple models:

```bash
# Compare models side-by-side
> /compare "Explain quantum computing"

# Benchmark model performance
> /benchmark claude-sonnet-4-5
```

### Development Tools Integration

```bash
# Auto-detect and run tests
> /test

# Git workflow
> /git status
> /git commit
> /git push

# Create pull request
> /pr "Add new feature"

# Check development tools
> /tools check
> /tools install
```

### Themes & Customization

```bash
# Interactive theme selector
> /theme

# Available themes: default, dark, light, monokai, dracula, nord
```

---

## Quick Start

### Prerequisites

- macOS 14+ (Sonoma or later)
- Apple Silicon (M1/M2/M3/M4/M5)
- Xcode Command Line Tools (for building from source)

### Installation via Homebrew

```bash
brew tap Roberdan/convergio-cli
brew install convergio
```

### Installation from Source

```bash
# Clone
git clone https://github.com/Roberdan/convergio-cli.git
cd convergio-cli

# Build
make

# Setup (interactive wizard)
./build/bin/convergio setup

# Run
./build/bin/convergio
```

### Configuration

API keys can be configured via:

1. **macOS Keychain** (Recommended): `convergio setup`
2. **Environment variables**:
   ```bash
   export ANTHROPIC_API_KEY=sk-ant-...
   export OPENAI_API_KEY=sk-proj-...
   export GEMINI_API_KEY=AIza...
   export OPENROUTER_API_KEY=sk-or-...
   ```
3. **Config file**: `~/.convergio/config.json`

---

## Editions

Convergio is available in multiple editions, each focused on specific use cases:

| Edition | Agents | Target Audience | Installation |
|---------|--------|-----------------|--------------|
| **Master** | 60+ | Developers, power users | `brew install convergio` |
| **Education** | 18 | Students, teachers | `brew install convergio-edu` |
| **Business** | 10 | SMBs, sales teams | `--edition business` |
| **Developer** | 10 | DevOps, tech leads | `--edition developer` |

### Switching Editions at Runtime

```bash
# Via CLI flag
convergio --edition business

# Via environment variable
export CONVERGIO_EDITION=developer
convergio

# Via config.toml
# Add to ~/.convergio/config.toml:
# [ui]
# edition = "business"
```

**Note:** Education edition requires a dedicated binary (`convergio-edu`) for child safety compliance. It cannot be switched at runtime.

### Building Specific Editions

```bash
make                      # Master (default)
make EDITION=education    # Education (outputs convergio-edu)
make EDITION=business     # Business (outputs convergio-biz)
make EDITION=developer    # Developer (outputs convergio-dev)
```

---

## Commands Reference

### Core Commands

| Command | Description |
|---------|-------------|
| `/help [command]` | Interactive help system |
| `/status` | System status and agent info |
| `/quit` | Exit Convergio |

### Agent Management

| Command | Description |
|---------|-------------|
| `/agents` | List all 54 agents |
| `/agent <name>` | Configure individual agent |
| `/think <query>` | Direct agent thinking mode |

### Anna Executive Assistant

| Command | Description |
|---------|-------------|
| `/todo add <task>` | Create a task |
| `/todo list` | Show all tasks |
| `/todo done <id>` | Complete a task |
| `/todo start <id>` | Start working on a task |
| `/todo delete <id>` | Delete a task |
| `/remind <msg> <when>` | Create a reminder (natural language) |
| `/reminders` | Show upcoming reminders |
| `/daemon start/stop/status` | Manage notification daemon |

### Memory & Knowledge

| Command | Description |
|---------|-------------|
| `/remember <text>` | Store a memory |
| `/search <query>` | Semantic search |
| `/memories` | List recent memories |
| `/forget <id>` | Delete a memory |
| `/graph` | Knowledge graph stats |

### Model & Provider

| Command | Description |
|---------|-------------|
| `/setup` | Interactive configuration wizard |
| `/auth` | Authentication status |
| `/cost` | Show/set budget |
| `/compare` | Compare multiple models |
| `/benchmark` | Test model speed |

### Development Tools

| Command | Description |
|---------|-------------|
| `/test` | Run project tests (auto-detect) |
| `/git <status/commit/push>` | Git workflow |
| `/pr [title]` | Create pull request |
| `/tools <check/install>` | Manage dev tools |

### MCP Integration

| Command | Description |
|---------|-------------|
| `/mcp list` | List MCP servers |
| `/mcp connect <server>` | Connect to server |
| `/mcp tools` | Show available tools |

### Plan Database

| Command | Description |
|---------|-------------|
| `/plan list` | List all execution plans |
| `/plan status <id>` | Show plan details and progress |
| `/plan export <id>` | Export to Markdown with Mermaid |
| `/plan cleanup <days>` | Delete plans older than N days |

### Output Service

| Command | Description |
|---------|-------------|
| `/output list` | Show recent output files |
| `/output latest` | Show most recent output |
| `/output open <path>` | Open file in default app |
| `/output delete <path>` | Remove output file |
| `/output size` | Show disk usage |
| `/output cleanup <days>` | Delete outputs older than N days |

### System

| Command | Description |
|---------|-------------|
| `/debug <level>` | Set log level |
| `/stream <on/off>` | Toggle streaming |
| `/theme <name>` | Change theme |
| `/style <style>` | Set response style |
| `/hardware` | Show hardware info |
| `/update` | Check for updates |

---

## Architecture

### System Overview

```mermaid
flowchart TB
    subgraph UI["User Interface Layer"]
        REPL["REPL Commands"]
        StatusBar["Status Bar"]
        SetupWizard["Setup Wizard"]
    end

    subgraph ORCH["Orchestrator Layer"]
        Ali["Ali (Chief of Staff)"]
        MsgBus["Message Bus"]
        Conv["Convergence"]
    end

    subgraph ROUTER["Intelligent Routing"]
        ModelRouter["Model Router"]
        CostOpt["Cost Optimizer"]
        Failover["Provider Failover"]
    end

    subgraph PROVIDERS["Multi-Provider Layer"]
        subgraph Cloud["Cloud APIs"]
            Anthropic["Anthropic"]
            OpenAI["OpenAI"]
            Gemini["Google Gemini"]
            OpenRouter["OpenRouter"]
        end
        subgraph Local["Local (FREE)"]
            MLX["MLX Swift"]
            Ollama["Ollama"]
        end
    end

    subgraph AGENTS["Agent Execution"]
        AgentPool["54 Specialists"]
        GCD["GCD Parallelization"]
    end

    subgraph TOOLS["Tool Execution"]
        FileTools["File Tools"]
        ShellExec["Shell Exec"]
        WebFetch["Web Fetch"]
        MCPClient["MCP Client"]
    end

    subgraph FABRIC["Semantic Fabric"]
        SemanticGraph["Knowledge Graph"]
        SQLite["SQLite Storage"]
    end

    subgraph SILICON["Apple Silicon"]
        Metal["Metal GPU"]
        NeuralEngine["Neural Engine"]
        GCDQueues["GCD Queues"]
    end

    UI --> ORCH
    ORCH --> ROUTER
    ROUTER --> PROVIDERS
    ORCH --> AGENTS
    AGENTS --> TOOLS
    TOOLS --> FABRIC
    FABRIC --> SILICON
```

### Why Convergio vs Others?

| Feature | Convergio | Claude Code | Warp | VSCode + Copilot | Codex CLI |
|---------|-----------|-------------|------|------------------|-----------|
| **Multi-Provider** | 6 providers | Claude only | Multi | OpenAI only | OpenAI only |
| **Agents** | 54 specialists | Subagents | Single | Autocomplete | Single |
| **Inter-Agent Comms** | Message Bus | None | None | None | None |
| **Parallel Execution** | GCD native | Batched | Sequential | N/A | Sequential |
| **Convergence** | Automatic | Manual | N/A | N/A | N/A |
| **Per-Agent Models** | Yes | No | No | No | No |
| **Cost Management** | Granular budget | None | Credits | Subscription | API credits |
| **Local AI** | MLX + Ollama | No | No | No | No |
| **Task Management** | Native (Anna) | No | No | Extensions | No |
| **MCP Support** | Native client | Server | No | No | No |
| **Open Source** | Full | Closed | Partial | Extensions only | Closed |
| **Apple Optimized** | Metal + Neural Engine | Node.js | Rust | Electron | Python |

**Key Differentiators:**

- **vs Claude Code**: Convergio supports 6 providers (not just Claude), has 54 real specialist agents (not isolated subagents), automatic convergence, and per-agent model routing
- **vs Warp**: Convergio has true multi-agent orchestration with message bus, not just AI-assisted terminal
- **vs VSCode + Copilot**: Convergio is a full AI orchestration platform, not just code autocomplete. Native task management, semantic memory, and tool execution
- **vs Codex CLI**: Convergio offers multi-agent collaboration, local AI support, and granular cost control

---

## Technical Specifications

### Performance

| Metric | Value |
|--------|-------|
| Source Files | 71 C/Objective-C/Swift |
| Lines of Code | ~58,837 LOC |
| AI Agents | 54 specialists |
| Providers | 6 (Cloud + Local) |
| Commands | 40+ REPL commands |
| Tools | 15+ execution tools |
| Models Supported | 300+ across providers |
| Local Models | 8 MLX models |

### Apple Silicon Optimizations

- **Metal GPU**: Hardware-accelerated compute shaders
- **Neural Engine**: MLX model inference
- **GCD**: Optimal thread scheduling (P-cores + E-cores)
- **Unified Memory**: Zero-copy CPU/GPU data sharing
- **Accelerate**: BLAS optimizations for matrix operations

---

## Documentation

- [Provider Setup Guide](docs/PROVIDERS.md)
- [Model Selection Guide](docs/MODEL_SELECTION.md)
- [Cost Optimization](docs/COST_OPTIMIZATION.md)
- [Agent Development](docs/AGENT_DEVELOPMENT.md)
- [MCP Integration](docs/MCP_INTEGRATION.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)

---

## Development

### Building

```bash
# Clean build
make clean && make

# Debug build
make DEBUG=1

# Run tests
make test
```

### Testing

```bash
make unit_test        # Unit tests
make e2e_test         # End-to-end tests (requires API keys)
make fuzz_test        # Security fuzzing
make compaction_test  # Context compaction
make compare_test     # Model comparison
```

**Testing Strategy:**

| Test Type | CI (GitHub) | Local | Notes |
|-----------|-------------|-------|-------|
| Unit Tests | Required | Yes | Core functionality |
| Sanitizer Tests | Required | Yes | Memory & UB detection |
| Lint & Static Analysis | Required | Yes | Code quality |
| Fuzz Tests | Yes | Yes | Security testing |
| **E2E Tests** | **No** | **Always** | Requires API keys |

**Why E2E tests run only locally:**

E2E tests require real API keys (Anthropic, OpenAI, etc.) to test AI interactions, tool execution, and agent communication. For security and cost reasons, we run them locally before every release, not in CI.

**Local E2E testing is mandatory before release:**

Before every release, E2E tests are run locally using the `app-release-manager` agent or manually:

```bash
# Run full E2E suite locally
./tests/e2e_test.sh

# Or via release manager (includes all quality gates)
convergio
> @app-release-manager prepare release v5.0.0
```

This ensures all features are tested with real AI providers while keeping API keys secure.

---

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## Legal

- [Terms of Service](TERMS_OF_SERVICE.md)
- [Privacy Policy](PRIVACY_POLICY.md)
- [Disclaimer](docs/DISCLAIMER.md)

---

## License

MIT License - see [LICENSE](LICENSE) for details.

---

## Acknowledgments

- [FightTheStroke.org](https://fightthestroke.org) - Supporting the project
- Anthropic, OpenAI, Google - LLM providers
- Apple's MLX framework - Local ML on Apple Silicon

---

<p align="center">
  <strong>Convergio CLI v5.1.0</strong><br/>
  <em>Multi-Model AI Orchestration for Apple Silicon</em>
</p>

<p align="center">
  Developed by <a href="mailto:Roberdan@FightTheStroke.org">Roberto D'Angelo</a> with AI assistance
</p>

<p align="center">
  <strong>Built with love for Mario by the Convergio Team</strong><br/>
  <em>Making AI work for humans, not the other way around.</em>
</p>
