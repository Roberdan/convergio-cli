# Convergio Kernel

A semantic kernel for human-AI symbiosis, optimized for Apple Silicon M3 Max.

## What is Convergio Kernel?

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
- **Memory search** with keyword fallback (semantic search when pre-trained weights available)
- **Streaming responses** for real-time output during AI response generation
- **Debug logging system** - Comprehensive logging with multiple levels for troubleshooting
- **Local embeddings** via pure Metal/C transformer (infrastructure ready, needs weights)

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
| Streaming | Yes | Yes (SSE real-time) |
| Hardware | Generic | M3 Max optimized (NEON, Metal, Accelerate) |
| Debug Mode | N/A | Multi-level logging (ERROR→TRACE) |
| Convergence | N/A | All agents report to Ali for synthesis |

*Note: Local embeddings currently use random weights. Pre-trained weight loading planned.

### Key Differentiators

1. **Multi-Agent Architecture**: Instead of a single assistant, Convergio uses Ali as a "Chief of Staff" who can delegate tasks to specialist agents (analysts, developers, researchers, etc.) and synthesize their outputs.

2. **Parallel Multi-Agent Orchestration**: When Ali delegates to multiple agents, they execute **in parallel** using GCD (Grand Central Dispatch). Agent status is tracked in real-time and responses are converged into a unified answer.

3. **Inter-Agent Communication**: Agents can see each other's status, send messages, and collaborate during execution. The message bus enables asynchronous communication between all agents.

4. **Tool Execution**: Ali can interact with the real world:
   - Read and write files
   - Execute shell commands (with safety restrictions)
   - Fetch web content
   - Store and search memories semantically
   - Create and manage markdown notes
   - Build a searchable knowledge base

5. **Conversation Memory**: Full conversation history is persisted across sessions. Ali loads relevant context from previous conversations to maintain continuity.

6. **Native Performance**: Written in C with Metal GPU shaders and NEON SIMD, specifically tuned for Apple M3 Max. Binary size ~100KB vs hundreds of MB for Node.js apps.

7. **Cost Awareness**: Built-in budget management with real-time tracking. Know exactly how much each conversation costs.

8. **Debug Logging**: Comprehensive logging system with 5 levels (ERROR, WARN, INFO, DEBUG, TRACE) for troubleshooting and development.

9. **Local Intelligence**: MLX-compatible transformer for generating embeddings locally - semantic search without API calls.

## Requirements

- macOS 14+ (Sonoma)
- Apple Silicon (M1/M2/M3)
- Xcode Command Line Tools
- Claude API key (`ANTHROPIC_API_KEY` environment variable)

## Build

```bash
make
```

## Run

```bash
./build/bin/convergio
```

## Commands

```
convergio> help

Available commands:

  help         Show available commands
  create       Create a semantic node
  agent        Manage agents
  agents       List all available agents
  space        Manage collaborative spaces
  status       Show system status
  cost         Show/set cost and budget
  debug        Toggle/set debug logging level
  think        Process an intent
  quit         Exit Convergio

Cost commands:
  cost              Show current spending
  cost report       Detailed cost report
  cost set <USD>    Set budget limit
  cost reset        Reset session spending

Debug commands:
  debug             Toggle debug mode on/off
  debug <level>     Set level (none/error/warn/info/debug/trace)

Or simply talk to Ali, your Chief of Staff.
```

## Command Line Options

```bash
./build/bin/convergio [options]

Options:
  --debug     Enable debug logging (INFO level)
  --trace     Enable trace logging (maximum verbosity)
  --quiet     Disable all logging
```

## Tools Available to Ali

Ali can use these tools to interact with the real world:

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

## Example Session

```
convergio> ciao Ali, puoi leggere il file Makefile?
Ali: Certo! Leggo il file Makefile per te.
[Uses file_read tool]
Il Makefile contiene la configurazione di build per il progetto...

convergio> ricorda che preferisco risposte concise
Ali: [Uses memory_store tool]
Ho memorizzato la tua preferenza per risposte concise.

convergio> cerca nei tuoi ricordi le mie preferenze
Ali: [Uses memory_search tool]
Ho trovato questa preferenza: "risposte concise"

convergio> come stanno i tuoi agenti sviluppatori?
Ali: [Delegates to multiple agents in parallel]
⚡ Working: dev-backend (thinking), dev-frontend (thinking), dev-mobile (thinking)
[Agents respond in parallel, Ali converges responses]

convergio> cost
Session: $0.0045 spent | $4.9955 remaining

convergio> agents
Agent Registry Status
=====================
Total agents: 49 (dynamically loaded)
Active agents:
  - Ali (Orchestrator) [active]

convergio> debug trace
Debug level set to: TRACE
```

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
└── data/                 # Runtime data (SQLite, etc.)
```

## Inspired By

- [Convergio](https://github.com/Roberdan/Convergio) - The original multi-agent system
- Apple's MLX framework - Local ML on Apple Silicon
- Anthropic's Claude - The underlying LLM

## License

MIT

---

*Roberto & AI Team - Convergio Kernel Project*
