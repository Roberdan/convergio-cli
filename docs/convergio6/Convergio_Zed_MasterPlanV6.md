# Convergio-Zed Integration Master Plan V6

**Last Updated**: 2025-12-21 | **Status**: IN PROGRESS
**Branch**: `feature/acp-zed-integration`
**Repositories**:
- `ConvergioCLI` (ACP server)
- `convergio-zed` (Zed fork with Convergio Panel)

---

## Dashboard

| Status | Tasks |
|--------|-------|
| Completed | 75 (Phase 1-10 + 6 S1-S6 + 7 B1-B6,B8) |
| In Progress | 0 |
| Pending | 23 |
| **Total** | **98** |

```
PHASE 1-10: MVP & POST-MVP    ████████████████████ 100% (62/62) DONE
PHASE 11: STABILITY           ███████████████░░░░░  75% (6/8) IN PROGRESS
PHASE 12: BACKGROUND EXEC FIX █████████████████░░░  87% (7/8) IN PROGRESS
PHASE 13: ALI CONTROL CENTER  ░░░░░░░░░░░░░░░░░░░░   0% (0/5)
PHASE 14: PERFORMANCE         ░░░░░░░░░░░░░░░░░░░░   0% (0/6)
PHASE 15: RELEASE SYSTEM      ░░░░░░░░░░░░░░░░░░░░   0% (0/7)
PHASE 16: EDITIONS SUPPORT    ░░░░░░░░░░░░░░░░░░░░   0% (0/3)
```

---

## Quick Reference

| Repository | Purpose | Local Path |
|------------|---------|------------|
| ConvergioCLI | ACP server + CLI | `/Users/roberdan/GitHub/ConvergioCLI` |
| convergio-zed | Zed fork with panels | `/Users/roberdan/GitHub/convergio-zed` |

### Key Crates (convergio-zed)

| Crate | Purpose |
|-------|---------|
| `convergio_panel` | Multi-agent panel (54 agents, 14 categories) |
| `ali_panel` | Ali Chief of Staff panel |
| `git_graph` | Visual git history panel |
| `agent_servers/acp.rs` | ACP client implementation |

---

## PHASE 11: Stability & Crash Fixes

**Priority**: P0 CRITICAL
**Problem**: Crashes affecting VS Code, Cursor, and system stability

### Tasks

| ID | Task | Status | Effort | Owner |
|----|------|--------|--------|-------|
| S1 | Investigate memory leaks in ACP server | ✅ Done | 1 day | - |
| S2 | Add memory limits and cleanup for long sessions | ✅ Done | 1 day | - |
| S3 | Fix process cleanup on Zed exit (zombie processes) | ✅ Done | 0.5 day | - |
| S4 | Add SIGSEGV/SIGABRT handlers with graceful cleanup | ✅ Done | 0.5 day | - |
| S5 | Implement crash recovery detection (FIX-07 from backlog) | ✅ Done | 1 day | - |
| S6 | Add resource limits (file handles, memory caps) | ✅ Done | 0.5 day | - |
| S7 | Profile CPU usage during idle | Pending | 0.5 day | - |
| S8 | Test stability with multiple IDE instances | Pending | 0.5 day | - |

### Fixes Applied (2025-12-21)

**S1-S2: Memory Leak Prevention**
- Added `cleanup_sessions()` function that frees:
  - All message content strings (`session->messages[i].content`)
  - Background buffers (`session->background_buffer`)
  - Joins worker threads and destroys mutexes
- Called from `acp_server_shutdown()`

**S3: Process Cleanup**
- Added EOF counter with `MAX_EOF_RETRIES = 10`
- Process exits after 1 second of continuous EOF (parent terminated)
- Prevents zombie processes when Zed closes

**S4: Crash Signal Handlers**
- Added `handle_crash_signal()` for SIGSEGV and SIGABRT
- Writes cleanup message to stderr
- Re-raises signal with default handler for core dump

### Investigation Areas

1. **ACP Server Memory**
   - Check for unbounded session storage
   - Review background buffer allocation
   - Verify cleanup on session close

2. **Process Management**
   - Ensure convergio-acp terminates when Zed closes
   - Check for orphan processes
   - Review signal handling

3. **File Handle Leaks**
   - Check FILE* cleanup in context loading
   - Verify SQLite connection management
   - Review directory scanning (dirent)

---

## PHASE 12: Background Execution Fix

**Priority**: P1 HIGH
**Problem**: Agents don't continue working when user switches to another agent

### Root Cause Analysis (2025-12-21)

**IDENTIFIED ISSUE**: The ACP server main loop is **single-threaded and synchronous**.

When `session/prompt` is called:
1. `acp_handle_session_prompt()` calls `orchestrator_agent_chat()` or `orchestrator_process_stream()`
2. These orchestrator calls **BLOCK** until the LLM response is complete
3. During this time, the main loop **CANNOT read** from stdin
4. Any `session/background` request from the client is **queued** but not processed
5. By the time the background request is processed, the prompt is already complete

**Current Code Flow (Broken)**:
```
Client                          Server (single-threaded)
  |                                   |
  |-- session/prompt "analyze..." -->  |
  |                                   +-- orchestrator_process_stream() BLOCKS
  |-- session/background ------------>  |  (not received, server is blocked)
  |                                   |
  |                                   |  ... minutes pass ...
  |                                   |
  |<----- response complete -------   |
  |                                   +-- NOW reads session/background (too late!)
```

### Proposed Solution: Async Prompt Processing

**Option A: Thread-per-request** (Recommended)
- Spawn worker thread for each `session/prompt`
- Main loop continues reading stdin
- Use condition variable to signal completion

**Option B: Non-blocking I/O with select/poll**
- Use `select()` to multiplex stdin reads and orchestrator status
- More complex but single-threaded

**Option C: Process-per-agent**
- Each agent runs in separate process
- IPC between main ACP server and agent workers
- Most isolated but highest overhead

### Tasks (Updated)

| ID | Task | Status | Effort | Owner |
|----|------|--------|--------|-------|
| B1 | ~~Debug why background sessions stop processing~~ Root cause identified | ✅ Done | - | - |
| B2 | Implement async prompt processing (pthread) | ✅ Done | 2 days | - |
| B3 | Add mutex protection for session state | ✅ Done | 0.5 day | - |
| B4 | Implement background message queue | ✅ Done | 1 day | - |
| B5 | Test buffered content retrieval on foreground | ✅ Done | 0.5 day | - |
| B6 | Fix notification delivery for backgroundComplete | ✅ Done | 0.5 day | - |
| B7 | Add visual indicator when background task completes | Pending | 0.5 day | - |
| B8 | E2E test: start task, switch agent, return, verify results | ✅ Done | 1 day | - |

### Progress Notes (2025-12-21)

**B2 Completed:**
- Thread-safe stdout writes (g_stdout_mutex)
- Session mutex initialization in create_session()
- Proper cleanup in cleanup_sessions() (joins workers, destroys mutexes)
- Worker thread structure (PromptWorkerArgs) and thread function
- Thread-local session ID (tl_current_session_id) for callbacks
- worker_stream_callback() - buffers to background_buffer when is_background=true
- process_prompt_internal() - full prompt processing with history, memory, context
- Refactored acp_handle_session_prompt() to spawn worker thread
- Worker thread join handling in cleanup and when starting new prompts
- Cancel handler updated to join worker threads
- All tests passing

**B5, B8 Completed (2025-12-21):**
- Removed unused stream_callback (replaced by worker_stream_callback)
- Removed duplicate buffer_chunk_for_session (inlined in worker_stream_callback)
- Added 10 new E2E tests for Phase 11 & 12 features (Test 9 & Test 10)
- All 38 E2E tests passing, all 37 unit tests passing
- Buffered content retrieval tested via background_buffer_len allocation check

**B7 Remaining:**
- Add visual indicator in convergio-zed when background task completes

### Expected Behavior (Target)

```
User → Ali → "analyze this codebase" → Switch to Satya
     → Ali CONTINUES in background (separate thread)
     → Badge shows "Ali working..."
     → Ali finishes → Notification "Ali completed"
     → Return to Ali → See full analysis
```

### Implementation Notes

```c
// Proposed structure for async prompt handling
typedef struct {
    pthread_t thread;
    ACPSession* session;
    char* prompt;
    bool cancelled;
} PromptWorker;

// Main loop reads stdin
// session/prompt -> spawn worker thread
// session/background -> set session->is_background = true (works!)
// Worker thread uses stream_callback which checks is_background
```

### Files to Modify

- `src/acp/acp_server.c` - Add threading for prompt handling
- `include/nous/acp.h` - Add worker struct to session
- `convergio-zed/crates/agent_servers/src/acp.rs:700-710` - Client calls (may work as-is)
- `convergio-zed/crates/convergio_panel/src/panel.rs` - UI indicators

---

## PHASE 13: Ali Control Center

**Priority**: P1 HIGH
**Problem**: Ali panel needs to be a self-contained chat experience

### Tasks

| ID | Task | Status | Effort | Owner |
|----|------|--------|--------|-------|
| A1 | Redesign Ali panel as full chat interface | Pending | 2 days | - |
| A2 | Add conversation history display | Pending | 1 day | - |
| A3 | Implement input field with Enter to send | Pending | 0.5 day | - |
| A4 | Add "New Conversation" button | Pending | 0.5 day | - |
| A5 | Integrate with session persistence | Pending | 1 day | - |

### Design Requirements

- Full chat view (not just quick actions)
- Message history with user/assistant distinction
- Streaming response display
- Keyboard shortcuts (Enter to send, Shift+Enter for newline)
- Context from open files in Zed

---

## PHASE 14: Performance Optimization

**Priority**: P2 MEDIUM
**Problem**: Slow compilation, slow response times

### Tasks

| ID | Task | Status | Effort | Owner |
|----|------|--------|--------|-------|
| P1 | Profile convergio-zed build time | Pending | 0.5 day | - |
| P2 | Enable incremental compilation for Rust | Pending | 0.5 day | - |
| P3 | Cache sccache for Rust builds | Pending | 0.5 day | - |
| P4 | Profile ACP response latency | Pending | 0.5 day | - |
| P5 | Optimize session lookup (hash map vs linear) | Pending | 0.5 day | - |
| P6 | Add lazy loading for agent list | Pending | 1 day | - |

### Metrics to Track

- Build time (clean, incremental)
- Time to first token from ACP
- Memory usage during long sessions
- CPU usage during idle

---

## PHASE 15: Release System

**Priority**: P2 MEDIUM
**Problem**: No coordinated release for CLI + Zed fork

### Tasks

| ID | Task | Status | Effort | Owner |
|----|------|--------|--------|-------|
| R1 | Create GitHub Actions workflow for convergio-zed release | Pending | 1 day | - |
| R2 | Add VERSION file sync between repos | Pending | 0.5 day | - |
| R3 | Create DMG builder for Zed fork | Pending | 1 day | - |
| R4 | Code sign and notarize Zed fork | Pending | 1 day | - |
| R5 | Create release checklist for app-release-manager | Pending | 0.5 day | - |
| R6 | Add cross-repo version compatibility check | Pending | 0.5 day | - |
| R7 | Document release process in ADR | Pending | 0.5 day | - |

### Release Artifacts

| Artifact | Source | Distribution |
|----------|--------|--------------|
| `convergio` CLI | ConvergioCLI | Homebrew tap |
| `Convergio Studio.dmg` | convergio-zed | GitHub Releases |
| `convergio-acp` | ConvergioCLI | Bundled with CLI |

### Version Alignment

```
ConvergioCLI/VERSION     → CLI version
convergio-zed/CONVERGIO_VERSION → Zed fork version (Convergio features)
convergio-zed/VERSION    → Zed upstream version
```

---

## PHASE 16: Edition Support

**Priority**: P3 LOW
**Problem**: How to handle education/other editions in Zed

### Tasks

| ID | Task | Status | Effort | Owner |
|----|------|--------|--------|-------|
| E1 | Design edition-aware agent filtering for Zed | Pending | 1 day | - |
| E2 | Add edition configuration to convergio-zed settings | Pending | 0.5 day | - |
| E3 | Create build variants for different editions | Pending | 1 day | - |

### Edition Strategy

| Edition | CLI Binary | Zed Fork | Agent Set |
|---------|------------|----------|-----------|
| Base | `convergio` | `Convergio Studio` | 54 agents |
| Education | `convergio --edition=education` | `Convergio Studio Edu` | 54 + Ali Preside, Accessibility |
| Enterprise | TBD | TBD | Custom agent set |

---

## PHASE 17: File Integration

**Priority**: P2 MEDIUM
**Problem**: Verify open files in Zed are accessible to agents

### Tasks

| ID | Task | Status | Effort | Owner |
|----|------|--------|--------|-------|
| F1 | Verify ACP receives current file context | Pending | 0.5 day | - |
| F2 | Test agent access to open buffers | Pending | 0.5 day | - |
| F3 | Implement "Add to context" for selected files | Pending | 1 day | - |

---

## Completed Phases (1-10)

### PHASE 1-4: MVP (Complete)
- ACP server implementation
- 54 agents with routing
- Convergio Panel in Zed
- Ali Panel basics

### PHASE 5-7: Polish (Complete)
- Icons and themes
- Session persistence
- Ali historical memory

### PHASE 8-10: Post-MVP (Complete)
- Git Graph Panel
- Lazy loading
- Background execution (server-side)

---

## ADR References

| ADR | Title | Status |
|-----|-------|--------|
| [016](adr/016-convergio-zed-integration.md) | Convergio-Zed Integration Architecture | Accepted |
| [017](adr/017-acp-session-persistence.md) | ACP Session Persistence | Proposed |
| TBD | Coordinated Multi-Repo Release | Pending |
| TBD | Edition Support in Zed | Pending |

---

## Working Instructions

```bash
# Work on ConvergioCLI (ACP server)
cd /Users/roberdan/GitHub/ConvergioCLI
git checkout feature/acp-zed-integration
make clean && make && make test

# Work on convergio-zed (Zed fork)
cd /Users/roberdan/GitHub/convergio-zed
cargo build --release
./target/release/zed

# Test ACP server standalone
./build/bin/convergio-acp --agent ali-chief-of-staff

# Generate Zed config for all agents
./scripts/generate_zed_config.sh > ~/.config/zed/settings.json
```

---

## Risk Register

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Zed upstream breaks fork | High | Medium | Regular merge, minimal changes to core |
| Memory leaks crash system | High | High | Priority P0, memory profiling |
| Background exec never works | Medium | Medium | Fallback to sync-only mode |
| Release coordination fails | Medium | Low | Automated checks, version lock |

---

## Next Actions

1. **IMMEDIATE**: Fix stability issues (Phase 11 S1-S4)
2. **THIS WEEK**: Debug background execution (Phase 12)
3. **NEXT WEEK**: Ali Control Center (Phase 13)
4. **THEN**: Release system (Phase 15)

---

**Contact**: Roberto D'Angelo with AI agent team
**Last Updated**: 2025-12-21
