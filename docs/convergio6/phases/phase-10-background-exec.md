# PHASE 10 - Background Execution

**Status**: ✅ COMPLETE
**Depends on**: Phase 9

## Objective

Allow agents to continue working when user switches to another agent. When returning, user sees the completed results.

## Tasks

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| B1 | Keep ACP sessions alive on agent switch | ✅ | 1 day | AgentSessionBackground trait + ext_method calls |
| B2 | Server-side background processing | ✅ | 1.5 days | Buffer chunks when is_background=true |
| B3 | Queue results for inactive sessions | ✅ | 1 day | background_buffer in ACPSession |
| B4 | Visual indicator "agent working" | ✅ | 0.5 day | ArrowCircle icon + processing_agents state |
| B5 | Notification when background task completes | ✅ | 0.5 day | session/backgroundComplete notification |

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
