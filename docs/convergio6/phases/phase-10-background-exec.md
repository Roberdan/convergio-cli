# PHASE 10 - Background Execution

**Status**: ⏸️ PENDING
**Depends on**: Phase 9

## Objective

Allow agents to continue working when user switches to another agent. When returning, user sees the completed results.

## Tasks

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| B1 | Keep ACP sessions alive on agent switch | ⏸️ | 1 day | Don't close session, just pause UI |
| B2 | Server-side background processing | ⏸️ | 1.5 days | Continue streaming even without active client |
| B3 | Queue results for inactive sessions | ⏸️ | 1 day | Buffer responses until client reconnects |
| B4 | Visual indicator "agent working" | ⏸️ | 0.5 day | Badge/icon in Convergio Panel |
| B5 | Notification when background task completes | ⏸️ | 0.5 day | Toast/sound when agent finishes |

## Architecture

### Current Flow (problematic)
```
User → Ali → [starts task] → Switch to Satya → Ali STOPS → Return to Ali → Lost work
```

### Target Flow
```
User → Ali → [starts task] → Switch to Satya → Ali CONTINUES in background
→ Ali finishes → Notification "Ali completed"
→ Return to Ali → See full results
```

## Implementation Strategy

1. **Client-side**: Don't destroy AcpThread when switching, keep subscription active
2. **Server-side**: Buffer streaming output when client not actively reading
3. **Protocol**: Add `session/background` notification for status updates
4. **UI**: Show spinner/badge for agents with active tasks

## Key Files

- `crates/agent_servers/src/acp.rs` - Session management
- `crates/convergio_panel/src/panel.rs` - Agent switching
- `src/acp/acp_server.c` - Background processing
