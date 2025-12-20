# PHASE 9 - Ali Panel & Persistence Optimization

**Status**: 🔄 IN PROGRESS
**Started**: 2025-12-20

## Objective

Fix Ali Panel visibility and optimize conversation persistence for long sessions.

## Tasks

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| A1 | Fix Ali Panel button visibility | 🔄 | 0.5 day | AliPanel loads after AgentPanel |
| A2 | Add "working" indicator for active agents | ⏸️ | 0.5 day | Visual badge when agent is processing |
| A3 | Lazy load conversations (last first) | ✅ | 1 day | Load last 5 messages, session/loadMore for more |
| A4 | Summarize old messages for long convos | ⏸️ | 1 day | AI summary of messages beyond threshold |
| A5 | Add detailed debug logging | ⏸️ | 0.5 day | For development troubleshooting |
| A6 | Error handling for long sessions | ⏸️ | 0.5 day | Graceful recovery from session issues |

## Implementation Details

### A1: Ali Panel Button Fix
- Problem: AliPanel requires AgentPanel but they initialize in parallel
- Solution: Initialize AliPanel AFTER AgentPanel in `zed.rs`
- Files: `crates/zed/src/zed.rs`

### A3-A4: Lazy Load Strategy
```
1. On session resume, send only last 2 messages immediately
2. Include summary of previous N messages
3. Load full history on scroll-up or explicit request
4. Threshold: summarize after 20 messages
```

## Dependencies

- Requires Phase 4 persistence working (✅)
- Requires ACP server session management (✅)
