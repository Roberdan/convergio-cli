# ADR-008: Context Compaction

**Date**: 2025-12-13
**Status**: Approved
**Author**: AI Team

## Context

Long conversations accumulate context that:
- Exceeds model context windows (200K tokens for Claude)
- Increases API costs linearly with conversation length
- Reduces model performance due to "lost in the middle" effect
- Wastes tokens on old, potentially irrelevant information

Research from JetBrains (December 2025) and Anthropic shows:
- Effective context length is often 60-120K tokens, not the nominal 200K
- Context rot degrades recall as token count increases
- Summarization can preserve 90%+ of relevant information

## Decision

### Implement Automatic Context Compaction

When conversation context exceeds 80,000 tokens:
1. Summarize older messages using an economical LLM (Claude Haiku)
2. Store the summary as a "checkpoint" in the database
3. Load context as: `[Checkpoint Summary] + [Recent 10 messages]`

### Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    CONTEXT BUILDING FLOW                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   1. Estimate tokens for full conversation                       │
│   2. IF tokens > 80K:                                           │
│        a. Load existing checkpoint OR create new one            │
│        b. Summarize messages [first_id → last_id - 10]          │
│        c. Save checkpoint to database                           │
│   3. Build context = [checkpoint] + [recent 10 messages]        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Summarization Strategy

Using Claude Haiku for cost efficiency:
- Input: $1.00 / 1M tokens
- Output: $5.00 / 1M tokens
- ~$0.001 per summarization call

Summarization prompt extracts:
- Key decisions made
- Important facts learned
- Current task state
- User preferences expressed

### Database Schema

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

### Multi-Agent Consistency Guarantee

```
Timeline:
─────────────────────────────────────────────────────────────
T0: User Input
T1: build_context_prompt() → CHECK tokens
T2: IF > 80K: compaction_summarize() → SAVE checkpoint
T3: Build final context (immutable after this point)
T4: Ali processes
T5: Ali delegates to Marco, Sara, Leo
    └─ Each agent receives strdup(context) ← SAME SNAPSHOT
T6: Agents work in parallel (context cannot change)
T7: Ali synthesizes
─────────────────────────────────────────────────────────────

CRITICAL: Compaction ONLY happens at T2, before any delegation.
          All delegated agents see identical context.
```

### Configuration

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| `COMPACTION_THRESHOLD_TOKENS` | 80,000 | 80% of Claude's effective context |
| `COMPACTION_KEEP_RECENT_MSGS` | 10 | Balance between context and freshness |
| `COMPACTION_MODEL` | `claude-haiku-4.5` | Cheapest available Anthropic model |
| `COMPACTION_MAX_CHECKPOINTS` | 5 | Prevent runaway summarization costs |

### Fallback Behavior

If LLM summarization fails:
1. Log warning
2. Fall back to truncation: `first_2000_chars + [truncated] + last_2000_chars`
3. Continue operation with degraded but functional context

## Consequences

### Positive

- **Cost Reduction**: ~80% reduction in context tokens for long conversations
- **Maintained Quality**: Semantic meaning preserved through LLM summarization
- **Transparent**: No user action required, works automatically
- **Persistent**: Checkpoints saved to database, survives restarts
- **Safe**: Multi-agent consistency guaranteed by architecture

### Negative

- **Information Loss**: Some details may be lost in summarization
- **Summarization Cost**: Small per-checkpoint cost (~$0.001)
- **Latency**: Additional LLM call when compaction triggers

### Mitigations

| Risk | Mitigation |
|------|------------|
| Information loss | Keep `key_facts` JSON for critical data |
| API failure | Fallback to truncation |
| Cost spike | Cap at 5 checkpoints per session |
| Thread safety | Use existing mutex from persistence layer |

## Alternatives Considered

### 1. Simple Truncation
- **Rejected**: Loses too much context without semantic preservation

### 2. Sliding Window (Keep Last N)
- **Rejected**: Loses important decisions from early conversation

### 3. RAG-Based Retrieval
- **Deferred**: More complex, requires robust embedding infrastructure

### 4. Token Pruning
- **Rejected**: Requires model internals access, not available via API

## Implementation Files

| File | Purpose |
|------|---------|
| `include/nous/compaction.h` | Public API |
| `src/context/compaction.c` | Core implementation |
| `src/memory/persistence.c` | Checkpoint storage |
| `src/orchestrator/orchestrator.c` | Integration point |
| `tests/test_compaction.c` | Unit tests |
| `docs/CONTEXT_COMPACTION.md` | User documentation |

## References

- [JetBrains Research: Efficient Context Management](https://blog.jetbrains.com/research/2025/12/efficient-context-management/)
- [Anthropic: Effective Context Engineering](https://www.anthropic.com/engineering/effective-context-engineering-for-ai-agents)
- [Google: Multi-agent Framework Architecture](https://developers.googleblog.com/architecting-efficient-context-aware-multi-agent-framework-for-production/)
