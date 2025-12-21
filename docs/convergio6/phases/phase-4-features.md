# PHASE 4 - Advanced Features

**Status**: ✅ COMPLETED
**Completed**: 2025-12-19

## Objective
Ali super chat, context sharing, and conversation persistence.

## Tasks

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| F1 | Ali super chat (bottom panel) | ✅ | 2 days | Ali bottom panel + Enter key + Open Chat button |
| F2 | Ali aware of all conversations | ✅ | 3 days | ACP saves context, Ali loads from ~/.convergio/agent_context/ |
| F3 | Per-agent conversation persistence | ✅ | 2 days | HistoryStore.save_acp_thread + thread_by_agent_name |
| F4 | Branding: Convergio Panel icon | ✅ | 0.5 day | UserGroup icon for Convergio, Ai for Ali |

## Chat Persistence - Details

| ID | Task | Status | Note |
|----|------|--------|------|
| BUG1a | DB: Add agent_name field (Zed) | ✅ | DbThreadMetadata + DbThread + DB column |
| BUG1b | Search: thread_by_agent_name uses agent_name | ✅ | Searches by agent_name instead of title |
| BUG1c | Save: agent_name in save_thread_metadata | ✅ | Convergio threads save agent server name |
| BUG1d | ACP Resume: Session persistence to disk | ✅ | ~/.convergio/sessions/ with JSON |
| BUG1e | ACP Resume: Auto-resume by agent_name | ✅ | Server automatically finds previous sessions |
| BUG1f | ACP Resume: History context in prompt | ✅ | Previous messages included as context |

## Implementation

**Zed side**: Threads saved with `agent_name`

**convergio-acp side**:
- Sessions saved to disk in `~/.convergio/sessions/`
- Auto-resume by agent_name when opening new session
- Message history included as context in prompt
- Agent "remembers" previous conversations

## Modified Files

**convergio-zed**:
- `crates/agent/src/db.rs` - agent_name in DbThreadMetadata + DbThread + query
- `crates/agent/src/history_store.rs` - improved thread_by_agent_name
- `crates/agent_ui/src/agent_panel.rs` - search by full agent name
- `crates/agent_ui/src/acp/thread_view.rs` - saves agent_name + log for ACP resume

**ConvergioCLI**:
- `include/nous/acp.h` - ACPSession with message history
- `src/acp/acp_server.c` - session persistence, auto-resume, history context
