# ADR 016: Convergio 6.0 - Zed Integration Architecture

**Date**: 2025-12-19
**Status**: Accepted
**Deciders**: Roberto D'Angelo with AI Agents team

## Context

Convergio 6.0 introduces a native integration with Zed editor through the Agent Client Protocol (ACP). The goal is to provide a multi-agent panel with 54 specialized agents accessible directly from the editor.

Key challenges:
1. How to resume conversations across editor restarts
2. How to provide Ali with cross-session memory
3. How to maintain version alignment across multiple repositories

## Decision 1: Server-Side Session Resume (H1)

### Problem
Zed's ACP implementation doesn't natively support session resume for external agent servers. The original plan proposed implementing `cx.observe + pending_resume` pattern in Zed's Rust code.

### Decision
Implement session resume **server-side** in `convergio-acp` instead of client-side in Zed.

### Implementation
```
┌─────────────────────────────────────────────────────────────┐
│                    SESSION RESUME FLOW                       │
├─────────────────────────────────────────────────────────────┤
│  1. Zed sends session/new (agent: "Convergio-ali")          │
│  2. ACP server calls find_session_by_agent_name()           │
│  3. If found: load session from ~/.convergio/sessions/      │
│  4. Include last 10 messages as history context in prompt   │
│  5. Agent "remembers" previous conversation                  │
└─────────────────────────────────────────────────────────────┘
```

**Files modified**:
- `src/acp/acp_server.c:605-660` - Auto-resume logic
- `src/acp/acp_server.c:795-820` - History injection in prompts

### Rationale
- No Zed core changes required (easier maintenance with upstream)
- Works transparently - Zed doesn't need to know about resume
- Simpler implementation with same user experience
- Sessions persist in JSON files for debugging/backup

### Trade-offs
- Zed UI doesn't show "Resumed" indicator (acceptable)
- History limited to last 10 messages per prompt (context window)
- New session ID created even when resuming (cosmetic)

## Decision 2: Ali Historical Memory System (H3-H6)

### Problem
Ali, as Chief of Staff agent, needs to remember insights from past conversations across all sessions and agents.

### Decision
Implement a dedicated memory system with:
1. Automatic summary generation every 4 messages
2. JSON storage in `~/.convergio/memory/summaries/`
3. Memory injection in Ali's system prompt
4. Search and retrieval by agent, topic, or keyword

### Implementation
```
┌─────────────────────────────────────────────────────────────┐
│                  MEMORY SYSTEM ARCHITECTURE                  │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Conversation ──► memory_generate_summary() ──► JSON file   │
│       │                    │                       │        │
│       │              (every 4 msgs)                │        │
│       │                    │                       ▼        │
│       │                    │         ~/.convergio/memory/   │
│       │                    │              summaries/        │
│       ▼                    ▼                       │        │
│  Ali prompt ◄── memory_build_context() ◄──────────┘        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

**Files created**:
- `include/nous/memory.h` - API header
- `src/memory/memory.c` - Implementation (~400 lines)

**Files modified**:
- `src/acp/acp_server.c:857-876` - Memory injection in Ali prompts
- `CMakeLists.txt`, `Makefile` - Build integration

### Memory Entry Structure
```c
typedef struct {
    char id[64];           // UUID
    char agent_name[128];  // Source agent
    char summary[4096];    // LLM-generated summary
    char topics[512];      // Comma-separated topics
    char decisions[2048];  // Key decisions made
    char action_items[1024]; // Action items extracted
    time_t timestamp;
    int importance;        // 1-10 scale
} MemoryEntry;
```

### Rationale
- Ali becomes institutional memory for the user
- Summaries are human-readable and debuggable
- JSON format allows easy backup/export
- Importance scoring enables prioritization

## Decision 3: Multi-Repository Release Alignment

### Problem
Three artifacts must stay version-aligned:
- `convergio` (CLI)
- `convergio-acp` (ACP server)
- `convergio-zed` (Zed fork)

### Decision (Planned - Post-MVP)
Implement a unified release system:
1. Shared `VERSION` file in both repos
2. Release script that:
   - Increments version in both repos
   - Creates synchronized git tags
   - Builds all artifacts
   - Generates unified CHANGELOG
3. CI/CD check for ACP protocol compatibility
4. Release notes documenting inter-version dependencies

### Rationale
- Prevents version drift between components
- Ensures ACP protocol compatibility
- Simplifies user upgrades

## Consequences

### Positive
- Seamless conversation continuity across restarts
- Ali has persistent memory across all sessions
- No Zed core modifications for session resume
- Clear separation of concerns

### Negative
- Two storage locations (sessions/ and memory/)
- Memory summarization adds latency (async mitigates)
- Version alignment requires discipline until automated

### Risks
- Memory files could grow large over time (mitigate: pruning)
- Session files need cleanup mechanism (TODO)

## References
- Master Plan: `docs/Convergio6MasterPlan.md`
- ACP Protocol: Zed's Agent Client Protocol specification
- Memory API: `include/nous/memory.h`
