# Convergio V7 Architecture Plan: Plugin-Based Orchestration Engine

## Vision Statement

Transform Convergio from a monolithic multi-edition CLI into a **modular orchestration engine** with a plugin architecture. The core becomes a powerful, open-source AI orchestration engine, while editions (Education, Business, Developer) become installable plugins.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         CONVERGIO V7 ARCHITECTURE                        │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                    CONVERGIO CORE (Open Source)                  │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌────────────┐ │    │
│  │  │ Orchestrator│ │  LLM Router │ │ Tool Engine │ │  Memory    │ │    │
│  │  │   Engine    │ │  (Multi-    │ │  (MCP +     │ │  System    │ │    │
│  │  │   (Ali)     │ │  Provider)  │ │  Native)    │ │  (SQLite)  │ │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └────────────┘ │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌────────────┐ │    │
│  │  │   REPL      │ │   Config    │ │  Streaming  │ │  Logging   │ │    │
│  │  │   Engine    │ │   Manager   │ │  Pipeline   │ │  System    │ │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └────────────┘ │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                   │                                      │
│                         Plugin Interface                                 │
│                    ┌──────────────┴──────────────┐                       │
│                    ▼                              ▼                       │
│  ┌─────────────────────────────┐  ┌─────────────────────────────┐       │
│  │      AGENT PLUGINS          │  │     INTERFACE PLUGINS       │       │
│  │  ┌─────────┐ ┌─────────┐   │  │  ┌─────────┐ ┌─────────┐   │       │
│  │  │Education│ │Business │   │  │  │ Native  │ │  Web    │   │       │
│  │  │ Pack    │ │  Pack   │   │  │  │ Mac App │ │   UI    │   │       │
│  │  │(Maestri)│ │(Sales,  │   │  │  │(SwiftUI)│ │(Future) │   │       │
│  │  │         │ │ CRM)    │   │  │  └─────────┘ └─────────┘   │       │
│  │  └─────────┘ └─────────┘   │  │  ┌─────────┐ ┌─────────┐   │       │
│  │  ┌─────────┐ ┌─────────┐   │  │  │   VS    │ │   Zed   │   │       │
│  │  │Developer│ │ Custom  │   │  │  │  Code   │ │ Editor  │   │       │
│  │  │  Pack   │ │ Agents  │   │  │  │Extension│ │ Panel   │   │       │
│  │  │(DevOps) │ │(User)   │   │  │  └─────────┘ └─────────┘   │       │
│  │  └─────────┘ └─────────┘   │  └─────────────────────────────┘       │
│  └─────────────────────────────┘                                        │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                      TOOL PLUGINS                                │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │    │
│  │  │  MCP    │ │ Native  │ │ Voice   │ │ Vision  │ │ Custom  │   │    │
│  │  │ Servers │ │ Tools   │ │  I/O    │ │  (MLX)  │ │ Tools   │   │    │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘   │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Core Engine (Open Source - MIT License)

### What Goes in Core

```
convergio-core/
├── src/
│   ├── orchestrator/      # Ali orchestration engine
│   │   ├── orchestrator.c # Main orchestration logic
│   │   ├── tool_loop.c    # Tool execution pipeline
│   │   ├── parallel.c     # Parallel agent execution
│   │   └── streaming.c    # Streaming responses
│   │
│   ├── llm/               # Multi-provider LLM router
│   │   ├── router.c       # Provider selection & fallback
│   │   ├── anthropic.c    # Claude integration
│   │   ├── openai.c       # OpenAI/Azure integration
│   │   ├── local.c        # MLX/Ollama integration
│   │   └── cache.c        # Response caching
│   │
│   ├── tools/             # Tool execution engine
│   │   ├── tool_engine.c  # Tool parsing & execution
│   │   ├── mcp_client.c   # MCP protocol client
│   │   └── native_tools.c # Built-in tools (file, web, etc.)
│   │
│   ├── memory/            # Memory & persistence
│   │   ├── memory.c       # KV store & context
│   │   ├── persistence.c  # SQLite storage
│   │   └── compaction.c   # Session compaction
│   │
│   ├── repl/              # Interactive REPL
│   │   ├── repl.c         # Main REPL loop
│   │   ├── input.c        # Input handling
│   │   └── output.c       # Output formatting
│   │
│   ├── config/            # Configuration
│   │   ├── config.c       # Config file parsing
│   │   └── keychain.c     # Secure credential storage
│   │
│   └── plugin/            # Plugin system (NEW)
│       ├── plugin_api.h   # Public plugin API
│       ├── plugin_loader.c # Dynamic plugin loading
│       ├── plugin_registry.c # Plugin registration
│       └── plugin_sandbox.c  # Plugin isolation
│
├── include/
│   └── convergio/
│       ├── core.h         # Core API
│       ├── plugin.h       # Plugin development API
│       ├── agent.h        # Agent definition API
│       └── tool.h         # Tool definition API
│
└── plugins/               # Built-in plugins
    └── base-agents/       # Ali, Anna (always included)
```

### Core Features (Always Available)

1. **Ali Orchestrator** - Multi-agent coordination
2. **Anna Assistant** - Task management & reminders
3. **LLM Router** - Anthropic, OpenAI, Azure, Local (MLX/Ollama)
4. **MCP Client** - Model Context Protocol support
5. **Native Tools** - File, web, bash, memory
6. **REPL** - Interactive command line
7. **Memory System** - Persistent context & knowledge
8. **Plugin API** - Load & manage plugins

---

## Plugin System Design

### Plugin API (C Header)

```c
// include/convergio/plugin.h

typedef struct {
    const char* name;           // "education-pack"
    const char* version;        // "1.0.0"
    const char* description;    // "Maestri AI teachers for students"
    const char* author;         // "Roberdan"
    const char* license;        // "MIT" or "Commercial"

    // Lifecycle hooks
    int (*on_load)(void* ctx);
    int (*on_unload)(void* ctx);
    int (*on_config_change)(void* ctx, const char* key, const char* value);

    // Registration
    const ConvergioAgent* agents;      // NULL-terminated array
    size_t agent_count;
    const ConvergioTool* tools;        // NULL-terminated array
    size_t tool_count;
    const ConvergioCommand* commands;  // NULL-terminated array
    size_t command_count;

    // Optional features
    bool has_ui;                       // Provides UI components
    bool requires_license;             // Needs license validation
    const char* required_core_version; // Minimum core version
} ConvergioPlugin;

// Plugin registration macro
#define CONVERGIO_PLUGIN(name, ...) \
    __attribute__((constructor)) void __register_##name(void) { \
        convergio_register_plugin(&(ConvergioPlugin){__VA_ARGS__}); \
    }
```

### Plugin Types

#### 1. Agent Plugins (Add AI personalities)

```c
// Example: education-pack plugin

static const ConvergioAgent education_agents[] = {
    {
        .id = "euclide-matematica",
        .name = "Maestro Euclide",
        .description = "Mathematics teacher using Socratic method",
        .system_prompt_path = "prompts/euclide.md",
        .preferred_model = "gpt-4o-mini",
        .capabilities = AGENT_CAP_EDUCATION | AGENT_CAP_QUIZ,
    },
    {
        .id = "feynman-fisica",
        .name = "Professor Feynman",
        .description = "Physics explained simply",
        .system_prompt_path = "prompts/feynman.md",
        .preferred_model = "gpt-4o-mini",
        .capabilities = AGENT_CAP_EDUCATION | AGENT_CAP_EXPERIMENT,
    },
    // ... 15 more teachers
    {0} // NULL terminator
};

CONVERGIO_PLUGIN(education_pack,
    .name = "education-pack",
    .version = "1.0.0",
    .description = "17 Maestri AI teachers for K-12 education",
    .author = "Roberdan",
    .license = "Commercial",
    .agents = education_agents,
    .agent_count = 17,
    .requires_license = true,
);
```

#### 2. Tool Plugins (Add new capabilities)

```c
// Example: voice-io plugin

static const ConvergioTool voice_tools[] = {
    {
        .name = "speak",
        .description = "Convert text to speech",
        .input_schema = "{\"type\":\"object\",\"properties\":{\"text\":{\"type\":\"string\"}}}",
        .handler = voice_speak_handler,
    },
    {
        .name = "listen",
        .description = "Convert speech to text",
        .input_schema = "{\"type\":\"object\",\"properties\":{\"duration\":{\"type\":\"number\"}}}",
        .handler = voice_listen_handler,
    },
    {0}
};

CONVERGIO_PLUGIN(voice_io,
    .name = "voice-io",
    .version = "1.0.0",
    .description = "Voice input/output using Apple Speech frameworks",
    .author = "Roberdan",
    .license = "MIT",
    .tools = voice_tools,
    .tool_count = 2,
);
```

#### 3. Interface Plugins (Add UIs)

```c
// Example: native-mac plugin (SwiftUI app)

CONVERGIO_PLUGIN(native_mac,
    .name = "native-mac",
    .version = "1.0.0",
    .description = "Native macOS SwiftUI application",
    .author = "Roberdan",
    .license = "MIT",
    .has_ui = true,
    .on_load = native_mac_launch,
    .on_unload = native_mac_quit,
);
```

### Plugin Distribution

```
~/.convergio/plugins/
├── education-pack/
│   ├── plugin.dylib         # Compiled plugin
│   ├── prompts/             # Agent prompts
│   │   ├── euclide.md
│   │   ├── feynman.md
│   │   └── ...
│   ├── assets/              # Icons, images
│   └── manifest.json        # Plugin metadata
│
├── business-pack/
│   ├── plugin.dylib
│   ├── prompts/
│   └── manifest.json
│
└── developer-pack/
    ├── plugin.dylib
    ├── prompts/
    └── manifest.json
```

---

## Business Model: Open Source + Freemium + Paid

### Tier 1: Core (Free & Open Source - MIT)

**What's included:**
- Convergio Core orchestration engine
- Ali orchestrator + Anna assistant
- Multi-provider LLM routing (BYOK - Bring Your Own Key)
- MCP protocol support
- All native tools (file, web, bash, memory)
- Plugin development SDK
- CLI interface

**Target users:** Developers, tinkerers, open-source community

### Tier 2: Freemium Plugins (Free with limits)

**Education Pack (Freemium):**
- Free: 3 teachers (Euclide, Feynman, Darwin)
- Free: 10 questions/day limit
- Paid: All 17 teachers, unlimited usage

**Developer Pack (Freemium):**
- Free: Rex (code review) + Dario (debugger)
- Free: 5 reviews/day limit
- Paid: All 11 agents, unlimited usage

**Business Pack (Freemium):**
- Free: Anna (assistant) + Fiona (market analyst)
- Free: 5 queries/day limit
- Paid: All 10 agents, unlimited usage

### Tier 3: Premium Plugins (Paid only)

**Enterprise Pack:**
- Team management & collaboration
- SSO/SAML integration
- Audit logging
- Custom agent creation
- Priority support

**Native Mac App:**
- Beautiful SwiftUI interface
- Menu bar integration
- Spotlight search integration
- Notification center integration

**Voice I/O Pack:**
- Real-time speech-to-text
- Text-to-speech with multiple voices
- Voice commands

### Tier 4: Enterprise/Custom

- On-premise deployment
- Custom LLM integration
- Custom agent development
- SLA & dedicated support
- Training & consulting

---

## Pricing Model (Draft)

| Tier | Price | What's Included |
|------|-------|-----------------|
| **Core** | Free (Open Source) | Orchestration engine, Ali, Anna, CLI, Plugin SDK |
| **Education Starter** | Free | 3 teachers, 10 questions/day |
| **Education Pro** | $9.99/month | 17 teachers, unlimited, parent dashboard |
| **Education School** | $99/month | Classroom mode, 30 students, analytics |
| **Developer Starter** | Free | 2 agents, 5 reviews/day |
| **Developer Pro** | $19.99/month | 11 agents, unlimited, CI/CD integration |
| **Business Starter** | Free | 2 agents, 5 queries/day |
| **Business Pro** | $29.99/month | 10 agents, unlimited, CRM integration |
| **Native Mac App** | $29.99 (one-time) | Beautiful native UI |
| **Enterprise** | Custom | On-premise, SSO, custom agents, SLA |

---

## Migration Path: V6 → V7

### Phase 1: Prepare Core (Current refactoring)
1. Split monolithic files into modules
2. Create clear boundaries between core and editions
3. Define plugin API interfaces

### Phase 2: Extract Plugins
1. Move education agents to education-pack plugin
2. Move business agents to business-pack plugin
3. Move developer agents to developer-pack plugin
4. Test plugin loading/unloading

### Phase 3: Implement Licensing
1. Build license validation system
2. Implement free tier limits
3. Create license key generation/validation
4. Build payment integration (Stripe/Paddle)

### Phase 4: Build Distribution
1. Create plugin marketplace (GitHub-based or custom)
2. Build `convergio plugin install education-pack` command
3. Create plugin update mechanism
4. Build plugin discovery & search

### Phase 5: Launch V7
1. Release Core as open source (MIT)
2. Launch plugin marketplace
3. Migrate existing users
4. Marketing & community building

---

## Technical Decisions to Make

1. **Plugin format:** Dynamic libraries (.dylib) vs WASM vs JSON+prompts only?
2. **Plugin isolation:** Sandboxing level (full, partial, none)?
3. **License validation:** Online-only vs offline-capable?
4. **Marketplace:** Self-hosted vs GitHub releases vs both?
5. **Versioning:** Semantic versioning for plugins? Core compatibility matrix?

---

## Success Metrics

- **Open Source:** GitHub stars, contributors, forks
- **Freemium conversion:** Free → Paid conversion rate
- **Revenue:** MRR from plugin subscriptions
- **Enterprise:** Enterprise customer count, contract value
- **Community:** Plugin ecosystem size (third-party plugins)

---

## Next Steps After V6 Cleanup

1. Design & document Plugin API (RFC)
2. Implement plugin loader in core
3. Create first plugin (education-pack)
4. Build license validation system
5. Create plugin distribution mechanism
6. Beta test with select users
7. Launch V7

---

## Related Documents

**Master Index:** [V7Plan-MASTER-INDEX.md](./V7Plan-MASTER-INDEX.md) - Complete documentation hub

**Architecture & Technical:**
- [V7Plan-Architecture-DeepDive.md](./V7Plan-Architecture-DeepDive.md) - Core C deployment & scalability
- [V7Plan-Enhanced.md](./V7Plan-Enhanced.md) - Web platform & telemetry
- [V7Plan-Voice-WebPlatform.md](./V7Plan-Voice-WebPlatform.md) - Voice I/O & web stack

**Business & Strategy:**
- [V7Plan-CRITICAL-REVIEW.md](./V7Plan-CRITICAL-REVIEW.md) - Optimized unified plan ⭐
- [V7Plan-Business-Case.md](./V7Plan-Business-Case.md) - Financial analysis
- [V7Plan-Ecosystem-Strategy.md](./V7Plan-Ecosystem-Strategy.md) - Ecosystem vision

**Pitch:**
- [V7Plan-PITCH.md](./V7Plan-PITCH.md) - Investor pitch & vision

---

*This plan represents the future vision for Convergio. Execute after V6 cleanup is complete.*
