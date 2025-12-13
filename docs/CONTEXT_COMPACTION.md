# Context Compaction

## Overview

Context Compaction is an automatic optimization feature that compresses long conversation histories when they exceed a configurable token threshold. This reduces API costs while preserving semantic meaning through LLM-based summarization.

## How It Works

```
┌─────────────────────────────────────────────────────────────────┐
│                    build_context_prompt()                        │
│                                                                  │
│  1. Project Context                                              │
│  2. Important Memories                                           │
│  3. Relevant Memories              ┌──────────────────────┐     │
│  4. Conversation History ─────────►│ TOKEN CHECK: > 80K?  │     │
│  5. User Input                     └──────────┬───────────┘     │
│                                               │                  │
│                              YES ◄────────────┴─────────► NO     │
│                               │                          │       │
│                    ┌──────────▼──────────┐              │       │
│                    │ compaction_summarize │              │       │
│                    │  - LLM summarization │              │       │
│                    │  - Save checkpoint   │              │       │
│                    └──────────┬──────────┘              │       │
│                               │                          │       │
│                    ┌──────────▼──────────────────────────▼──┐   │
│                    │         FINAL CONTEXT                   │   │
│                    │  [Checkpoint Summary] + [Recent 10 msg] │   │
│                    └─────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `COMPACTION_THRESHOLD_TOKENS` | 80,000 | Trigger compaction when context exceeds this |
| `COMPACTION_KEEP_RECENT_MSGS` | 10 | Number of recent messages to keep uncompacted |
| `COMPACTION_MODEL` | `claude-haiku-4.5` | Model used for summarization (economical) |
| `COMPACTION_MAX_SUMMARY_TOKENS` | 500 | Maximum tokens in generated summary |
| `COMPACTION_MAX_CHECKPOINTS` | 5 | Maximum checkpoints per session |

## Database Schema

Checkpoints are stored in the `checkpoint_summaries` table:

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

## Multi-Agent Safety

Context compaction is designed to work safely with the multi-agent architecture:

```
Timeline di una request:
─────────────────────────────────────────────────────────────
T0: User Input arrives
T1: build_context_prompt() → CHECK 80K tokens
T2: IF > 80K: compaction_summarize() → save checkpoint
T3: Build final context (checkpoint + recent 10 msgs)
T4: Ali processes with compacted context
T5: Ali delegates to Marco, Sara, Leo
    └─ Each agent receives strdup(context) ← SAME SNAPSHOT
T6: Agents work in parallel (context is immutable)
T7: Ali synthesizes responses
─────────────────────────────────────────────────────────────

GUARANTEE: Compaction happens ONLY at T2, NEVER during T5-T6.
           All agents see exactly the same context.
```

**Why it's safe:**
- Each agent receives a COPY (`strdup()`) not a reference
- Context is built ONCE before delegation
- No agent can modify other agents' context
- Persistence layer is thread-safe (mutex protected)

## API Reference

### Core Functions

```c
// Initialize compaction system (call after persistence_init)
int compaction_init(void);

// Shutdown compaction
void compaction_shutdown(void);

// Check if compaction is needed
bool compaction_needed(const char* session_id, size_t current_tokens);

// Perform summarization
CompactionResult* compaction_summarize(
    const char* session_id,
    int64_t from_msg_id,
    int64_t to_msg_id,
    const char* messages_text
);

// Build context with automatic compaction
char* compaction_build_context(
    const char* session_id,
    const char* user_input,
    bool* out_was_compacted
);

// Get latest checkpoint for session
char* compaction_get_checkpoint(const char* session_id);
```

### CompactionResult Structure

```c
typedef struct {
    char* summary;              // Compressed summary text
    size_t original_tokens;     // Tokens in original messages
    size_t compressed_tokens;   // Tokens in summary
    double compression_ratio;   // original / compressed
    double cost_usd;            // Cost of summarization call
    int checkpoint_num;         // Checkpoint sequence number
} CompactionResult;
```

## Cost Analysis

| Scenario | Original Tokens | After Compaction | Cost Savings |
|----------|-----------------|------------------|--------------|
| 100 messages (~80K tokens) | 80,000 | ~15,000 | ~80% |
| 200 messages (~160K tokens) | 160,000 | ~20,000 | ~87% |

**Summarization Cost:**
- Using `claude-haiku-4.5`: ~$0.001 per compaction
- Net savings far exceed summarization cost for long conversations

## Fallback Behavior

If LLM summarization fails (API error, rate limit), the system falls back to:

1. **Truncation**: First 2000 chars + "[... truncated ...]" + Last 2000 chars
2. Maintains conversation coherence even without full summarization

## Files

| File | Purpose |
|------|---------|
| `include/nous/compaction.h` | Public API header |
| `src/context/compaction.c` | Implementation |
| `src/memory/persistence.c` | Checkpoint storage functions |
| `src/orchestrator/orchestrator.c` | Integration point |
| `tests/test_compaction.c` | Unit tests |

## Usage Example

Context compaction is automatic and transparent. When a conversation exceeds the threshold:

1. The system detects the threshold breach
2. Summarizes older messages using Haiku
3. Saves the checkpoint to the database
4. Builds context as: `[Summary] + [Recent 10 messages]`
5. Logs the compaction event for monitoring

No user action is required - it "just works."

## Monitoring

Check compaction activity via SQLite:

```sql
-- View all checkpoints
SELECT session_id, checkpoint_num, compression_ratio, summary_cost_usd
FROM checkpoint_summaries
ORDER BY created_at DESC;

-- Total savings by session
SELECT session_id,
       SUM(original_tokens - compressed_tokens) as tokens_saved,
       SUM(summary_cost_usd) as total_cost
FROM checkpoint_summaries
GROUP BY session_id;
```

## Future Enhancements

1. **Key Facts Extraction**: Automatically extract and store important facts in structured JSON
2. **Hierarchical Summaries**: Create summaries of summaries for very long sessions
3. **Configurable Prompts**: Allow custom summarization prompts per use case
4. **Memory Integration**: Feed summaries back into semantic memory for RAG
