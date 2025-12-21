# Convergio 6.0 - Architecture

## Overview

```
┌────────────────────────────────────────────────────────────────────────┐
│                        CONVERGIO-ZED                                   │
├──────────────────┬─────────────────────────────────────────────────────┤
│                  │                                                      │
│  CONVERGIO       │              ZED AGENT PANEL                         │
│  PANEL           │              (existing UI)                           │
│  ──────────────  │                                                      │
│  🔍 Search...    │  ┌────────────────────────────────────────────────┐ │
│  ──────────────  │  │ Chat with: Baccio - Architect                  │ │
│                  │  │                                                │ │
│  ▼ Leadership (2)│  │ YOU: Help me design the system architecture    │ │
│    ● Ali         │  │                                                │ │
│    ○ Satya       │  │ BACCIO: Based on your requirements, I suggest  │ │
│  ▼ Technology (7)│  │ a microservices approach with...               │ │
│    ● Baccio ◄────┼──│                                                │ │
│    ○ Dario       │  │                                                │ │
│    ○ Rex         │  └────────────────────────────────────────────────┘ │
│    ...           │                                                      │
│  ▶ Finance (4)   │                                                      │
│  ▶ Security (5)  │                                                      │
│  ...             │                                                      │
│  [54 agents]     │                                                      │
│  [14 categories] │                                                      │
│                  │                                                      │
└──────────────────┴─────────────────────────────────────────────────────┘
```

## Ali Bottom Panel (Super Chat)

```
┌─────────────────────────────────────────────────────────────────────┐
│                      CODE EDITOR                                     │
│  function calculate() {                                             │
│    // ...                                                           │
│  }                                                                  │
├─────────────────────────────────────────────────────────────────────┤
│  ALI - CHIEF OF STAFF (always visible, like terminal)              │
│  ───────────────────────────────────────────────────────────────── │
│  YOU: What's the status of the project?                            │
│  ALI: Based on conversations with other agents:                     │
│       - Baccio suggests microservices architecture                  │
│       - Dario found 3 bugs in the auth module                       │
│       - Rex reviewed PR #42, approved with minor changes            │
└─────────────────────────────────────────────────────────────────────┘
```

## Build Artifacts

| Artifact | Path | Description |
|----------|------|-------------|
| convergio-acp | `build/bin/convergio-acp` | ACP server for Zed |
| Convergio-Zed | Release build in convergio-zed | Zed.app with Convergio Panel |

## Repositories

| Repository | Path | Branch | Description |
|------------|------|--------|-------------|
| ConvergioCLI | `/Users/roberdan/GitHub/ConvergioCLI` | `feature/acp-zed-integration` | CLI + ACP server |
| convergio-zed | `/Users/roberdan/GitHub/convergio-zed` | `main` | Zed fork with Convergio Panel |

**GitHub**: https://github.com/Roberdan/convergio-zed

## Storage Paths

| Path | Content |
|------|---------|
| `~/.convergio/sessions/` | Persistent ACP sessions |
| `~/.convergio/agent_context/` | Shared context between agents |
| `~/.convergio/memory/summaries/` | Ali historical memory |

## Data Flow

```
User → Zed UI → ACP Protocol → convergio-acp → Orchestrator → LLM
                                      ↓
                               Session Storage
                               Memory Storage
                               Context Sharing
```
