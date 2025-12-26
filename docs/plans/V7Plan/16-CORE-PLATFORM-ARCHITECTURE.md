# Convergio V7 — Core Platform Architecture

**Status:** Draft for approval
**Date:** 2025-12-26
**Purpose:** Define Convergio Core as a standalone open-source platform, independent of any vertical.

---

## 1) Strategic Intent

Convergio Core is **NOT an education product**. It is a **multi-agent orchestration platform** that happens to launch with education as its first vertical.

```
┌─────────────────────────────────────────────────────────────┐
│                    CONVERGIO CORE (OSS)                      │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│  │ Multi-Agent │ │  Provider   │ │   Plugin    │            │
│  │Orchestration│ │ Abstraction │ │   System    │            │
│  └─────────────┘ └─────────────┘ └─────────────┘            │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│  │   BYOK      │ │   Local/    │ │  Protocol   │            │
│  │   Vault     │ │   Offline   │ │   Layer     │            │
│  └─────────────┘ └─────────────┘ └─────────────┘            │
└─────────────────────────────────────────────────────────────┘
                            │
            ┌───────────────┼───────────────┐
            ▼               ▼               ▼
      ┌──────────┐   ┌──────────┐   ┌──────────┐
      │Education │   │Healthcare│   │ Business │
      │ Vertical │   │ Vertical │   │ Vertical │
      └──────────┘   └──────────┘   └──────────┘
```

---

## 2) Core Components (Open Source)

### 2.1 Multi-Agent Orchestration Engine
- **Agent graph**: DAG-based execution of multiple agents
- **Planning layer**: Task decomposition, goal-oriented planning
- **Agent composition**: Agents can spawn/coordinate other agents
- **State management**: Shared context, memory, checkpoints
- **Execution modes**: Sequential, parallel, conditional branching

### 2.2 Provider Abstraction Layer
- **Unified interface** for all LLM providers
- **Supported providers**:
  - Azure OpenAI
  - OpenAI
  - Anthropic
  - Google (Gemini)
  - Local (MLX, Ollama, llama.cpp)
- **Automatic fallback**: Provider failure → next provider
- **Cost routing**: Route to cheapest acceptable model

### 2.3 Plugin System
- **WASM sandbox** for untrusted plugins (wasmtime)
- **Capability-based permissions**: Network, filesystem, secrets, etc.
- **Plugin types**:
  - Tools (functions agents can call)
  - Agents (specialized agent behaviors)
  - Verticals (full domain packages)
- **Hot reload**: Plugins can be updated without restart

### 2.4 BYOK Vault
- **Key storage**: Encrypted at rest (Azure Key Vault / local keychain)
- **Per-provider keys**: Users bring their own API keys
- **Usage tracking**: Per-key metering for cost visibility
- **Key rotation**: Support for automatic rotation

### 2.5 Local/Offline Runtime
- **MLX backend**: Apple Silicon native inference
- **Ollama integration**: Cross-platform local models
- **Zero cloud mode**: Complete offline operation
- **Model management**: Download, cache, update local models

### 2.6 Protocol Layer
- **MCP (Model Context Protocol)**: Full support
- **A2A (Agent-to-Agent)**: Priority implementation
- **ACP/ANP**: Monitor, implement when adoption warrants
- **Custom protocols**: Extensible protocol system

---

## 3) Core APIs (Platform Level)

### 3.1 Agent API
```
POST   /agents                    # Create agent
GET    /agents/{id}               # Get agent state
POST   /agents/{id}/run           # Execute agent
DELETE /agents/{id}               # Terminate agent
WS     /agents/{id}/stream        # Real-time streaming
```

### 3.2 Orchestration API
```
POST   /graphs                    # Create agent graph
POST   /graphs/{id}/execute       # Run graph
GET    /graphs/{id}/status        # Execution status
POST   /graphs/{id}/checkpoint    # Save state
POST   /graphs/{id}/resume        # Resume from checkpoint
```

### 3.3 Plugin API
```
POST   /plugins                   # Register plugin
GET    /plugins                   # List plugins
GET    /plugins/{id}/manifest     # Plugin capabilities
POST   /plugins/{id}/invoke       # Call plugin
DELETE /plugins/{id}              # Unregister plugin
```

### 3.4 Provider API
```
GET    /providers                 # List configured providers
POST   /providers/{id}/keys       # Add BYOK key
GET    /providers/{id}/models     # Available models
GET    /providers/{id}/usage      # Usage statistics
```

---

## 4) Separation of Concerns

| Layer | Responsibility | Open Source? |
|-------|----------------|--------------|
| **Core Platform** | Orchestration, providers, plugins, protocols | Yes (MIT/Apache) |
| **Vertical Plugins** | Domain-specific agents, tools, UX | Per-vertical license |
| **SaaS Infrastructure** | Auth, billing, multi-tenancy, compliance | No (proprietary) |
| **Managed Service** | Hosted Convergio with SLAs | No (commercial) |

---

## 5) Extension Points

### 5.1 Custom Agents
Developers can create agents with:
- Custom system prompts
- Specific tool sets
- Domain knowledge (RAG)
- Behavioral constraints

### 5.2 Custom Tools
Tools are functions agents can call:
- Input/output schema (JSON Schema)
- Permission requirements
- WASM or native implementation

### 5.3 Custom Verticals
A vertical is a complete domain package:
- Agent configurations
- Tool bundles
- UI components (optional)
- Domain-specific policies

---

## 6) Licensing Model

| Component | License | Rationale |
|-----------|---------|-----------|
| **Core runtime** | MIT or Apache 2.0 | Maximum adoption |
| **Plugin SDK** | MIT | Encourage ecosystem |
| **Official verticals** | Commercial | Revenue stream |
| **Community verticals** | Author's choice | Ecosystem growth |

---

## 7) Core vs. Vertical Boundaries

**Core handles:**
- Agent execution
- Provider routing
- Plugin loading
- Protocol translation
- Key management
- Metering events

**Verticals handle:**
- Domain-specific agents
- Specialized tools
- Industry compliance
- Custom UX
- Domain knowledge

**Rule:** If it's not domain-specific, it belongs in Core.

---

## 8) Success Metrics (Core Platform)

- **Adoption**: GitHub stars, forks, contributors
- **Plugin ecosystem**: Number of published plugins
- **Provider coverage**: Supported LLM providers
- **Performance**: Latency, throughput benchmarks
- **Stability**: Uptime, error rates

---

## 9) What This Enables

With Core as a proper platform:
1. **Community can build verticals** without forking
2. **Enterprise can self-host** with their own verticals
3. **We monetize through** official verticals + managed service
4. **Education is V1**, not the entire product
5. **Multi-agent is the moat**, not a single use case

