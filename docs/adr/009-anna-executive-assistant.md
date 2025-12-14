# ADR-009: Anna Executive Assistant Architecture

**Status**: Proposed
**Date**: 2025-12-14
**Author**: Roberto with AI Team
**Issue**: [#36](https://github.com/Roberdan/convergio-cli/issues/36)

## Context

Convergio CLI needs a personal productivity assistant that can:
1. Manage tasks and reminders locally
2. Integrate with external calendar/email services
3. Send native notifications for deadlines
4. Work with any LLM provider (online or local)

The market for AI executive assistants is growing rapidly ($3.35B → $21B by 2030, CAGR 44.5%), and there's a gap for privacy-first, local-first solutions.

## Decision

We will implement **Anna**, an Executive Assistant agent with three core components:

### 1. Native Todo Manager

**Rationale**: Local-first, no external dependencies, works offline.

- SQLite persistence (extends existing `convergio.db`)
- Full CRUD operations for tasks
- Recurrence support (iCal RRULE format)
- Full-text search (FTS5)
- Quick capture inbox

### 2. Generic MCP Client

**Rationale**: Flexibility over hardcoded integrations. Users choose their services.

- JSON-RPC 2.0 over stdio and HTTP transports
- Configuration file (`~/.convergio/mcp.json`)
- Auto tool discovery
- No official C SDK → implement from [MCP Spec 2025-06-18](https://modelcontextprotocol.io/specification/2025-06-18)

### 3. Notification Daemon

**Rationale**: Background service for timely reminders without keeping CLI running.

- launchd-managed daemon (auto-start, auto-restart)
- Multiple notification backends (terminal-notifier, osascript, log)
- Health monitoring and error recovery

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CONVERGIO CLI                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                      ANNA AGENT                              │    │
│  │  Natural Language → Intent Recognition → Action Dispatch     │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                              │                                       │
│         ┌────────────────────┼────────────────────┐                 │
│         ▼                    ▼                    ▼                 │
│  ┌──────────────┐   ┌──────────────────┐   ┌──────────────────┐    │
│  │ TODO MANAGER │   │ MCP CLIENT       │   │ NOTIFICATION     │    │
│  │              │   │                  │   │ DAEMON           │    │
│  │ • SQLite     │   │ • JSON-RPC 2.0   │   │ • launchd        │    │
│  │ • FTS5       │   │ • stdio/HTTP     │   │ • Fallback chain │    │
│  │ • RRULE      │   │ • Tool discovery │   │ • Health monitor │    │
│  └──────────────┘   └──────────────────┘   └──────────────────┘    │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## Alternatives Considered

### Alternative 1: Use External Todo Service (Todoist, etc.)

**Rejected because**:
- Requires internet connection
- Privacy concerns with sensitive task data
- Dependency on third-party service availability
- MCP integration can still provide this as an option

### Alternative 2: Hardcoded Calendar Integrations

**Rejected because**:
- Maintenance burden for multiple APIs
- No flexibility for user preferences
- MCP ecosystem already has mature servers

### Alternative 3: In-Process Notifications (no daemon)

**Rejected because**:
- Requires CLI to be running
- No reminders when user not at terminal
- Poor UX for time-sensitive notifications

## Model Compatibility

Critical requirement: System must work with ALL providers.

| Component | LLM Required | Online Required |
|-----------|--------------|-----------------|
| Todo CRUD | No | No |
| Todo CLI | No | No |
| Notifications | No | No |
| MCP Client | No | Depends on server |
| Anna NL Interface | Yes (any provider) | Depends on provider |

The NL layer uses existing provider router, supporting:
- Claude, OpenAI, Gemini, OpenRouter (online)
- Ollama, MLX (local)

## Security Considerations

1. **OAuth Tokens**: Stored in macOS Keychain, not config files
2. **MCP Trust**: User must explicitly enable each server
3. **Data Privacy**: All todo data local by default
4. **Daemon Permissions**: Runs as user, not root
5. **Notification Content**: Sensitive data redacted by default

## Performance Targets

| Metric | Target |
|--------|--------|
| Todo operation latency | < 50ms |
| Notification delivery | < 5s from scheduled time |
| MCP tool call | < 2s |
| Daemon memory | < 20MB |
| Daemon CPU | E-cores only (QOS_CLASS_UTILITY) |

## Implementation Plan

See detailed plan: `annaexecassistantPlan.md`

| Phase | Duration | Parallel |
|-------|----------|----------|
| 1: Todo Manager | Week 1-2 | Can start immediately |
| 2: Notifications | Week 2-3 | After Phase 1 schema |
| 3: MCP Client | Week 3-5 | Can start immediately |
| 4: Anna Agent | Week 5-6 | After Phases 1-3 APIs |
| 5: Documentation | Week 6-7 | Incremental |

Phases 1 and 3 can run **fully in parallel** (no dependencies).

## Consequences

### Positive

- Privacy-first: All core data local
- Flexible: Users choose MCP integrations
- Offline-capable: Todo + notifications work without internet
- Future-proof: New MCP servers work automatically
- Model-agnostic: Works with any LLM provider

### Negative

- More code to maintain (MCP client in C)
- macOS-specific daemon (launchd)
- Notification reliability depends on fallback chain

### Neutral

- Users need to configure MCP servers
- Daemon needs to be running for reminders

## References

- [MCP Specification 2025-06-18](https://modelcontextprotocol.io/specification/2025-06-18)
- [MCP November 2025 Release](http://blog.modelcontextprotocol.io/posts/2025-11-25-first-mcp-anniversary/)
- [Apple launchd Documentation](https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingLaunchdJobs.html)
- [AI Assistant Market Report](https://www.marketsandmarkets.com/Market-Reports/ai-assistant-market-40111511.html)
