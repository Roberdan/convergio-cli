# Convergio Kernel

A semantic kernel for human-AI symbiosis, optimized for Apple Silicon M3 Max.

## What is Convergio Kernel?

Convergio Kernel is a **multi-agent orchestration system** built in pure C/Objective-C, designed as the foundation for intelligent human-AI collaboration. Unlike typical CLI wrappers around LLM APIs, Convergio implements a complete agent architecture with:

- **Ali** - A Chief of Staff agent that serves as your single point of contact
- **49 specialist agents** that can be spawned on-demand for specific tasks
- **Tool execution** - Ali can read/write files, execute shell commands, fetch web content
- **Cost control** with granular budget tracking and per-agent attribution
- **Persistent memory** via SQLite for conversations and context
- **Memory search** with keyword fallback (semantic search when pre-trained weights available)
- **Streaming responses** for real-time output during AI response generation
- **Local embeddings** via pure Metal/C transformer (infrastructure ready, needs weights)

## How is this different from Claude CLI?

| Feature | Claude CLI | Convergio Kernel |
|---------|------------|------------------|
| Architecture | Single LLM wrapper | Multi-agent orchestration |
| Language | TypeScript/Node.js | Pure C/Objective-C |
| Agent Model | Single assistant | 49 specialist agents + Ali coordinator |
| Tool Execution | Built-in | Custom (file, shell, web, memory) |
| Cost Control | None | Granular budget caps, per-agent tracking |
| Memory | Session-based | SQLite + keyword/semantic search |
| Embeddings | Cloud API | Local Metal/NEON* |
| Streaming | Yes | Yes (SSE real-time) |
| Hardware | Generic | M3 Max optimized (NEON, Metal, Accelerate) |
| Convergence | N/A | All agents report to Ali for synthesis |

*Note: Local embeddings currently use random weights. Pre-trained weight loading planned.

### Key Differentiators

1. **Multi-Agent Architecture**: Instead of a single assistant, Convergio uses Ali as a "Chief of Staff" who can delegate tasks to specialist agents (analysts, developers, researchers, etc.) and synthesize their outputs.

2. **Tool Execution**: Ali can interact with the real world:
   - Read and write files
   - Execute shell commands (with safety restrictions)
   - Fetch web content
   - Store and search memories semantically

3. **Native Performance**: Written in C with Metal GPU shaders and NEON SIMD, specifically tuned for Apple M3 Max. Binary size ~100KB vs hundreds of MB for Node.js apps.

4. **Cost Awareness**: Built-in budget management with real-time tracking. Know exactly how much each conversation costs.

5. **Local Intelligence**: MLX-compatible transformer for generating embeddings locally - semantic search without API calls.

6. **Memory System**: Store information and retrieve it later using keyword search. Semantic similarity matching available when pre-trained weights are loaded.

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
  think        Process an intent
  quit         Exit Convergio

Cost commands:
  cost              Show current spending
  cost report       Detailed cost report
  cost set <USD>    Set budget limit
  cost reset        Reset session spending

Or simply talk to Ali, your Chief of Staff.
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

convergio> esegui make clean
Ali: [Uses shell_exec tool]
Comando eseguito: Cleaned.

convergio> cost
Session: $0.0045 spent | $4.9955 remaining

convergio> agents
Agent Registry Status
=====================
Total agents: 1 / 64
Active agents:
  - Ali (Orchestrator) [active]
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
│  │  file_read | file_write | shell_exec | web_fetch | memory  │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
          │                    │                    │
          ▼                    ▼                    ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│  Agent Pool      │  │  Message Bus     │  │  SQLite DB       │
│  (49 agents)     │  │  (async comms)   │  │  (persistence)   │
└──────────────────┘  └──────────────────┘  └──────────────────┘
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
