# PHASE 11 - Native File Interaction

**Status**: ⏸️ PENDING
**Depends on**: Phase 9

## Objective

Enable all agents to natively interact with files currently open in Zed editor. Agents can read, modify, and create files through the editor's buffer system.

## Tasks

| ID | Task | Status | Effort | Note |
|----|------|--------|--------|------|
| F1 | Expose open buffers to ACP | ⏸️ | 1 day | Send buffer list to agents |
| F2 | Read current file content | ⏸️ | 0.5 day | Agent reads file being edited |
| F3 | Modify file through editor | ⏸️ | 1 day | Agent edits go through undo stack |
| F4 | Selection-aware context | ⏸️ | 0.5 day | Agent sees current selection |

## Use Cases

### Current (limited)
- Agents can only use file tools (read_file, write_file)
- Works with filesystem, not editor buffers
- Unsaved changes not visible to agents

### Target (native)
- Agent sees exactly what user sees (including unsaved edits)
- Modifications go through editor (undo/redo works)
- Agent can reference selected text
- Multi-cursor support

## Protocol Extension

New ACP capabilities:
```json
{
  "editor": {
    "getOpenBuffers": true,
    "readBuffer": true,
    "editBuffer": true,
    "getSelection": true
  }
}
```

New requests:
- `editor/listBuffers` - List open files
- `editor/readBuffer` - Read buffer content
- `editor/applyEdit` - Apply edit to buffer
- `editor/getSelection` - Get current selection

## Key Files

- `crates/agent_servers/src/acp.rs` - Add editor capabilities
- `crates/acp_thread/src/acp_thread.rs` - Handle editor requests
- `src/acp/acp_server.c` - Server-side handling
