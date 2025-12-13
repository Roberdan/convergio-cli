# Context Compaction

## Overview

Context Compaction automatically summarizes conversation sessions when you exit Convergio. This provides:
- **Fast responses**: No loading of full history on every message
- **Clean sessions**: Each session starts fresh with only semantic memories
- **Full history access**: Use `/recall` to view past session summaries when needed

## How It Works

```
┌─────────────────────────────────────────────────────────────────┐
│                    NEW SESSION FLOW                              │
│                                                                  │
│  Session Start:                                                  │
│    - Load semantic memories (important facts, relevant context)  │
│    - NO full history loading                                     │
│                                                                  │
│  During Session:                                                 │
│    - Only last 10 messages kept in context                       │
│    - Fast, lightweight responses                                 │
│                                                                  │
│  On Quit:                                                        │
│    [████████░░] 80% Generating summary...                        │
│    - LLM summarizes the full session                             │
│    - Summary saved to database                                   │
│    - Available via /recall command                               │
└─────────────────────────────────────────────────────────────────┘
```

## Commands

### `/recall`
View summaries of all past sessions:

```
=== Past Session Summaries ===

[1] 2025-12-13 14:30:00 (42 messages)
    Discussed implementing user authentication with OAuth2.
    Decided to use JWT tokens. Fixed bug in login flow.
    ID: session_1765616604_6807

[2] 2025-12-12 09:15:00 (28 messages)
    Code review of the payment module. Added error handling...
    ID: session_1765540212_6807

Commands:
  recall clear           - Clear all summaries
  recall delete <id>     - Delete specific session
```

### `/recall clear`
Delete all session summaries (with confirmation).

### `/recall delete <session_id>`
Delete a specific session and its summary.

## Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `COMPACTION_MODEL` | `claude-haiku-4.5` | Model used for summarization (economical) |
| `COMPACTION_MAX_SUMMARY_TOKENS` | 500 | Maximum tokens in generated summary |

## Database Schema

Session summaries are stored in the `checkpoint_summaries` table:

```sql
CREATE TABLE checkpoint_summaries (
  id INTEGER PRIMARY KEY,
  session_id TEXT NOT NULL,
  checkpoint_num INTEGER NOT NULL,
  from_message_id INTEGER NOT NULL,
  to_message_id INTEGER NOT NULL,
  messages_compressed INTEGER,
  summary_content TEXT NOT NULL,
  key_facts TEXT,
  original_tokens INTEGER,
  compressed_tokens INTEGER,
  compression_ratio REAL,
  summary_cost_usd REAL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  UNIQUE(session_id, checkpoint_num)
);
```

## Session Flow

### During Normal Use

1. **Session Start**: Fresh context with only semantic memories
2. **Each Message**: Only last 10 messages included in context
3. **Response**: Fast, no history loading overhead

### On Exit (`quit`)

1. Progress bar shown: `[████░░░░░░] 30% Generating summary...`
2. Full session loaded from database
3. LLM generates concise summary
4. Summary saved to `checkpoint_summaries`
5. Exit completes

### When You Need History

Use `/recall` to view past summaries. The summaries are read-only context - they help you remember what was discussed but don't automatically inject into new sessions.

## API Reference

### Core Functions

```c
// Compact current session (called on quit)
int orchestrator_compact_session(void (*progress_callback)(int, const char*));

// Get all session summaries (for /recall)
SessionSummaryList* persistence_get_session_summaries(void);

// Delete specific session
int persistence_delete_session(const char* session_id);

// Clear all summaries
int persistence_clear_all_summaries(void);
```

## Cost Analysis

| Action | Cost |
|--------|------|
| Summarization per session | ~$0.001 (using Haiku) |
| Per-message overhead | $0 (no compaction during session) |

**Benefits:**
- Responses are faster (no 100-message loading)
- API costs reduced (smaller context per request)
- Clean separation between sessions

## Fallback Behavior

If LLM summarization fails (API error, rate limit), the system falls back to:

1. **Truncation**: First 2000 chars + "[... truncated ...]" + Last 2000 chars
2. Maintains conversation coherence even without full summarization

## Files

| File | Purpose |
|------|---------|
| `include/nous/compaction.h` | Public API header |
| `src/context/compaction.c` | Summarization implementation |
| `src/memory/persistence.c` | Session storage functions |
| `src/orchestrator/orchestrator.c` | `orchestrator_compact_session()` |
| `src/core/commands/commands.c` | `cmd_recall()` implementation |

## Monitoring

Check session summaries via SQLite:

```sql
-- View all summaries
SELECT session_id, created_at, compression_ratio, summary_cost_usd
FROM checkpoint_summaries
ORDER BY created_at DESC;

-- Total cost of summarizations
SELECT SUM(summary_cost_usd) as total_cost
FROM checkpoint_summaries;
```

## Migration from v3

The old behavior (loading 100 messages on every request) has been removed. The new flow:

| Old (v3) | New (v4) |
|----------|----------|
| Load 100 messages every request | Load only last 10 messages |
| Compaction triggered during request | Compaction only on quit |
| Automatic context injection | Manual recall via `/recall` |
| Slow responses with long history | Fast responses always |
