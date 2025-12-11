# Convergio Kernel

A semantic kernel for human-AI symbiosis, optimized for Apple Silicon M3 Max.

**Developed by [Roberdan@FightTheStroke.org](mailto:Roberdan@FightTheStroke.org)**

## Overview

Convergio Kernel is a **multi-agent orchestration system** built in pure C/Objective-C, designed as the foundation for intelligent human-AI collaboration. Unlike typical CLI wrappers around LLM APIs, Convergio implements a complete agent architecture with:

- **Ali** - A Chief of Staff agent that serves as your single point of contact
- **49 specialist agents** that can be spawned on-demand for specific tasks
- **Parallel multi-agent orchestration** - Ali can delegate to multiple agents simultaneously
- **Real-time agent status tracking** - See which agents are working and on what
- **Inter-agent communication** - Agents can communicate and collaborate during execution
- **Tool execution** - Ali can read/write files, execute shell commands, fetch web content
- **Notes & Knowledge base** - Persistent markdown notes and searchable knowledge storage
- **Cost control** with granular budget tracking and per-agent attribution
- **Conversation memory** - Persistent memory across sessions with context loading
- **Debug logging system** - Comprehensive logging with multiple levels for troubleshooting
- **Local embeddings** via pure Metal/C transformer (infrastructure ready, needs weights)

## Quick Start

### Prerequisites

- macOS 14+ (Sonoma)
- Apple Silicon (M1/M2/M3/M4)
- Xcode Command Line Tools
- libcurl (usually pre-installed on macOS)
- SQLite3 (usually pre-installed on macOS)

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/Roberdan/kernel.git
   cd kernel
   ```

2. **Set your Anthropic API key**
   ```bash
   export ANTHROPIC_API_KEY="your-api-key-here"
   ```

   To make it permanent, add to your `~/.zshrc` or `~/.bashrc`:
   ```bash
   echo 'export ANTHROPIC_API_KEY="your-api-key-here"' >> ~/.zshrc
   source ~/.zshrc
   ```

3. **Build the project**
   ```bash
   make
   ```

4. **Run Convergio**
   ```bash
   ./build/bin/convergio
   ```

### First Run

When you start Convergio, you'll see a colorful ASCII banner and a prompt. You can:

- **Talk naturally** to Ali, your Chief of Staff agent
- **Use commands** like `help`, `cost`, `agents`, `debug`
- **Ask Ali to use tools** - he can read files, execute commands, search the web

Example interaction:
```
convergio> Hello Ali, can you read the Makefile and tell me what it does?
Ali: [Uses file_read tool]
The Makefile defines the build process for Convergio Kernel...

convergio> cost
Session: $0.0032 spent | $4.9968 remaining

convergio> quit
```

## Command Line Options

```bash
./build/bin/convergio [options]

Options:
  --debug     Enable debug logging (INFO level)
  --trace     Enable trace logging (maximum verbosity)
  --quiet     Disable all logging
```

## REPL Commands

| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `agents` | List all available agents |
| `status` | Show system status |
| `cost` | Show current spending |
| `cost report` | Detailed cost report |
| `cost set <USD>` | Set budget limit (default: $5.00) |
| `cost reset` | Reset session spending |
| `debug` | Toggle debug mode on/off |
| `debug <level>` | Set level: none/error/warn/info/debug/trace |
| `quit` | Exit Convergio |

Or simply talk to Ali naturally - no commands needed!

## Tools Available to Ali

Ali can interact with the real world using these tools:

| Tool | Description | Safety |
|------|-------------|--------|
| `file_read` | Read file contents | System paths blocked |
| `file_write` | Write/append to files | System paths blocked |
| `file_list` | List directory contents | System paths blocked |
| `shell_exec` | Execute shell commands | Dangerous commands blocked |
| `web_fetch` | Fetch URL content | Standard HTTP/HTTPS |
| `memory_store` | Store info for later retrieval | SQLite + local embeddings |
| `memory_search` | Semantic search (RAG) | Cosine similarity |
| `note_write` | Create/update markdown notes | Stored in data/notes/ |
| `note_read` | Read note contents | - |
| `note_list` | List all available notes | - |
| `knowledge_add` | Add to knowledge base | Stored in data/knowledge/ |
| `knowledge_search` | Search knowledge base | Keyword matching |

## How is this different from Claude CLI?

| Feature | Claude CLI | Convergio Kernel |
|---------|------------|------------------|
| Architecture | Single LLM wrapper | Multi-agent orchestration |
| Language | TypeScript/Node.js | Pure C/Objective-C |
| Agent Model | Single assistant | 49 specialist agents + Ali coordinator |
| Parallel Execution | N/A | GCD-based parallel agent delegation |
| Agent Communication | N/A | Real-time inter-agent messaging |
| Tool Execution | Built-in | Custom (file, shell, web, memory, notes, knowledge) |
| Cost Control | None | Granular budget caps, per-agent tracking |
| Memory | Session-based | SQLite + conversation history + keyword/semantic search |
| Notes & Knowledge | N/A | Persistent markdown notes + knowledge base |
| Embeddings | Cloud API | Local Metal/NEON* |
| Hardware | Generic | M3 Max optimized (NEON, Metal, Accelerate) |
| Debug Mode | N/A | Multi-level logging (ERROR→TRACE) |
| Convergence | N/A | All agents report to Ali for synthesis |

*Note: Local embeddings currently use random weights. Pre-trained weight loading planned.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         USER INPUT                               │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                    ALI - Chief of Staff                          │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐       │
│  │ Cost Control  │  │ Task Planner  │  │ Memory/RAG    │       │
│  └───────────────┘  └───────────────┘  └───────────────┘       │
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                     TOOL EXECUTION                         │  │
│  │  file | shell | web | memory | notes | knowledge           │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │              PARALLEL DELEGATION (GCD)                     │  │
│  │     [Agent 1] ──┐                                          │  │
│  │     [Agent 2] ──┼──→ CONVERGENCE ──→ Unified Response     │  │
│  │     [Agent N] ──┘                                          │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
          │                    │                    │
          ▼                    ▼                    ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│  Agent Pool      │  │  Message Bus     │  │  SQLite DB       │
│  (49 agents)     │  │  (inter-agent)   │  │  (persistence)   │
│  Status Tracking │  │  Real-time comm  │  │  Conversations   │
└──────────────────┘  └──────────────────┘  └──────────────────┘
          │                                          │
          ▼                                          ▼
┌──────────────────┐                       ┌──────────────────┐
│  Debug Logging   │                       │  Notes/Knowledge │
│  ERROR→TRACE     │                       │  data/notes/     │
│  5 levels        │                       │  data/knowledge/ │
└──────────────────┘                       └──────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────┐
│              MLX EMBEDDINGS (Pure Metal/C)                       │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐               │
│  │Tokenizer│ │6-Layer  │ │  NEON   │ │  Mean   │               │
│  │  BPE    │→│Transform│→│  SIMD   │→│ Pooling │→ 384-dim      │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

### Key Differentiators

1. **Multi-Agent Architecture**: Instead of a single assistant, Convergio uses Ali as a "Chief of Staff" who can delegate tasks to specialist agents (analysts, developers, researchers, etc.) and synthesize their outputs.

2. **Parallel Multi-Agent Orchestration**: When Ali delegates to multiple agents, they execute **in parallel** using GCD (Grand Central Dispatch). Agent status is tracked in real-time and responses are converged into a unified answer.

3. **Inter-Agent Communication**: Agents can see each other's status, send messages, and collaborate during execution. The message bus enables asynchronous communication between all agents.

4. **Native Performance**: Written in C with Metal GPU shaders and NEON SIMD, specifically tuned for Apple M3 Max. Binary size ~100KB vs hundreds of MB for Node.js apps.

5. **Cost Awareness**: Built-in budget management with real-time tracking. Know exactly how much each conversation costs.

## Project Structure

```
kernel/
├── include/nous/          # Public headers
│   ├── nous.h            # Core semantic fabric
│   ├── orchestrator.h    # Multi-agent orchestration
│   └── tools.h           # Tool execution
├── src/
│   ├── core/             # Main entry point, fabric
│   ├── orchestrator/     # Ali, cost, registry, msgbus
│   ├── neural/           # Claude API, MLX embeddings
│   ├── memory/           # SQLite persistence + RAG
│   ├── tools/            # Tool execution (file, shell, web)
│   ├── agents/           # Agent definitions (49 specialists)
│   ├── intent/           # Natural language parsing
│   ├── metal/            # GPU compute
│   └── runtime/          # Scheduler
├── shaders/              # Metal compute shaders
├── docs/adr/             # Architecture Decision Records
├── data/                 # Runtime data (SQLite, notes, knowledge)
├── Makefile              # Build configuration
└── README.md             # This file
```

## Development

### Building from Source

```bash
# Clean build
make clean && make

# Build with debug symbols
make DEBUG=1

# Run tests (when available)
make test
```

### Adding New Agents

Agent definitions are markdown files in `src/agents/definitions/`. Each file defines:
- Agent name and role
- System prompt
- Specialized context
- Available tools

### Debug Logging

Enable debug logging to see what's happening under the hood:

```bash
# Via command line
./build/bin/convergio --debug

# Or at runtime
convergio> debug trace
```

Log levels: `none` < `error` < `warn` < `info` < `debug` < `trace`

## Troubleshooting

### "ANTHROPIC_API_KEY not set"
Make sure you've exported your API key:
```bash
export ANTHROPIC_API_KEY="your-key-here"
```

### Build Errors
Ensure you have Xcode Command Line Tools installed:
```bash
xcode-select --install
```

### Permission Denied
The binary needs execute permissions:
```bash
chmod +x ./build/bin/convergio
```

## Roadmap

- [ ] Pre-trained embedding weights for semantic search
- [ ] Voice input/output support
- [ ] Plugin system for custom tools
- [ ] Web UI interface
- [ ] Multi-user support

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Acknowledgments

- [FightTheStroke.org](https://fightthestroke.org) - Supporting the project
- Anthropic's Claude - The underlying LLM
- Apple's MLX framework - Inspiration for local ML on Apple Silicon

---

*Convergio Kernel - A semantic kernel for human-AI symbiosis*

*Developed by Roberto with AI assistance*
