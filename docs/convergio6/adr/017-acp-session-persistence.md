# ADR 017: ACP Session Persistence Architecture

**Status**: Proposed
**Date**: 2025-12-20
**Deciders**: Roberto D'Angelo, AI Team

## Context

The Convergio-Zed integration requires conversation persistence so that users can:
1. Resume conversations with agents across Zed restarts
2. See historical messages in the UI
3. Reset and start new conversations
4. Handle long conversations via compaction

Currently:
- **Server (ConvergioCLI)**: Already saves sessions to `~/.convergio/sessions/` with full message history
- **Client (convergio-zed)**: Creates threads but saves `messages: vec![]` (empty)
- **Protocol (agent-client-protocol)**: External crate, cannot be modified

## Decision Drivers

- Minimal changes to external dependencies
- Reuse existing server-side persistence
- Good UX with visible message history
- Support for conversation reset and compaction

## Considered Options

### Option A: Server-Only Persistence
- Server saves messages and injects them as context
- Client shows no historical messages
- **Pros**: Already works, zero changes
- **Cons**: Poor UX - users don't see history

### Option B: Client-Side Persistence in Zed DB
- Modify `thread_view.rs` to save messages to Zed's SQLite DB
- Implement `replay()` for `AcpThread` to restore from `DbThread`
- **Pros**: Uses native Zed persistence
- **Cons**: Complex type conversion between `AgentThreadEntry` and `DbMessage`

### Option C: Hybrid - Server Sends History via Notification (Recommended)
- Server sends `session/history` notification after `session/new`
- Client receives notification and populates UI
- **Pros**: Reuses server work, clean separation, no protocol fork needed
- **Cons**: Custom extension to ACP

## Decision

**Option C: Hybrid Server-Client Persistence**

### Implementation

#### 1. Server Side (ConvergioCLI)
In `acp_server.c`, after responding to `session/new` with a resumed session:

```c
// After session/new response, if resumed with history
if (resumed && session->message_count > 0) {
    // Send session/history notification
    cJSON* history_params = cJSON_CreateObject();
    cJSON_AddStringToObject(history_params, "sessionId", session->session_id);

    cJSON* messages = cJSON_CreateArray();
    for (int i = 0; i < session->message_count; i++) {
        cJSON* msg = cJSON_CreateObject();
        cJSON_AddStringToObject(msg, "role", session->messages[i].role);
        cJSON_AddStringToObject(msg, "content", session->messages[i].content);
        cJSON_AddItemToArray(messages, msg);
    }
    cJSON_AddItemToObject(history_params, "messages", messages);

    acp_send_notification("session/history", params_json);
}
```

#### 2. Client Side (convergio-zed)
In `crates/agent_servers/src/acp.rs`, handle the notification:

```rust
// Add handler for session/history notification
"session/history" => {
    if let Ok(history) = serde_json::from_value::<SessionHistory>(params) {
        // Populate thread with historical messages
        for msg in history.messages {
            match msg.role.as_str() {
                "user" => thread.add_user_message(msg.content),
                "assistant" => thread.add_assistant_message(msg.content),
                _ => {}
            }
        }
    }
}
```

#### 3. Reset Conversation
Add a "New Conversation" action:
- Clears current thread entries
- Calls `session/cancel` on server to clean up
- Creates new session with `session/new`

#### 4. Compaction Integration
When conversation exceeds token threshold:
1. Server's `compaction_needed()` returns true
2. Server calls LLM to summarize old messages
3. Old messages replaced with summary checkpoint
4. `session/history` sends only: `[checkpoint_summary, recent_messages...]`

### File Changes

| File | Change |
|------|--------|
| `src/acp/acp_server.c` | Add `session/history` notification |
| `crates/agent_servers/src/acp.rs` | Handle `session/history` notification |
| `crates/acp_thread/src/acp_thread.rs` | Add `add_historical_message()` method |
| `crates/ali_panel/src/panel.rs` | Add "New Conversation" action |

## Consequences

### Positive
- Full message history visible in UI on resume
- Reuses existing server-side persistence (no duplicate storage)
- No fork of external `agent-client-protocol` crate needed
- Compaction keeps conversations manageable
- Reset allows fresh starts

### Negative
- Custom protocol extension (non-standard ACP)
- Requires changes in both server and client

### Risks
- **Sync issues**: Server and client history could diverge
  - Mitigation: Server is source of truth, client only displays
- **Large histories**: Could slow down session start
  - Mitigation: Compaction limits message count

## Implementation Order

1. [ ] Server: Send `session/history` notification
2. [ ] Client: Handle `session/history` notification
3. [ ] Client: Add `add_historical_message()` to AcpThread
4. [ ] Client: Add "New Conversation" button
5. [ ] Server: Integrate compaction with session resume
6. [ ] Test end-to-end

## References

- ADR 016: Convergio 6.0 Zed Integration
- `src/context/compaction.c`: Existing compaction system
- `src/acp/acp_server.c`: ACP server implementation
