# Context Compaction Implementation Plan

## Overview
Implementare un sistema di compaction automatica del contesto quando supera una soglia, usando summarization via LLM economico per ridurre i token mantenendo il significato semantico.

## Target Directory
`/Users/roberdan/GitHub/ConvergioCLI-context-compaction`

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    build_context_prompt()                        │
│                                                                  │
│  1. Project Context                                              │
│  2. Important Memories (5)                                       │
│  3. Relevant Memories (3)         ┌──────────────────────┐      │
│  4. Conversation History ────────►│ TOKEN THRESHOLD CHECK │      │
│  5. User Input                    │   > 50K tokens?       │      │
│                                   └──────────┬───────────┘      │
│                                              │                   │
│                              YES ◄───────────┴──────────► NO     │
│                               │                          │       │
│                    ┌──────────▼──────────┐              │       │
│                    │ context_summarize() │              │       │
│                    │  - Load checkpoint  │              │       │
│                    │  - Summarize old    │              │       │
│                    │  - Save checkpoint  │              │       │
│                    └──────────┬──────────┘              │       │
│                               │                          │       │
│                    ┌──────────▼──────────────────────────▼──┐   │
│                    │         FINAL CONTEXT                   │   │
│                    │  [Checkpoint Summary] + [Recent 10 msg] │   │
│                    └─────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Files to Modify

| File | Changes |
|------|---------|
| `src/orchestrator/orchestrator.c` | Add threshold check + summarization call |
| `src/memory/persistence.c` | Add checkpoint storage functions |
| `include/nous/persistence.h` | Add checkpoint function declarations |
| `src/providers/tokens.c` | Add `tokens_estimate_context()` helper |

## Files to Create

| File | Purpose |
|------|---------|
| `src/context/compaction.c` | Context summarization logic |
| `include/nous/compaction.h` | Public API for compaction |

---

## Implementation Steps

### Step 1: Database Schema Extension

Add to `persistence.c` initialization:

```sql
CREATE TABLE IF NOT EXISTS checkpoint_summaries (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
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
CREATE INDEX IF NOT EXISTS idx_checkpoint_session
  ON checkpoint_summaries(session_id);
```

### Step 2: Compaction Module

Create `src/context/compaction.c`:

```c
// Configuration (User choices: Haiku + 80K threshold)
#define COMPACTION_THRESHOLD_TOKENS  80000
#define COMPACTION_KEEP_RECENT_MSGS  10
#define COMPACTION_MODEL            "claude-haiku-4.5"

// Public API
typedef struct {
    char* summary;
    size_t original_tokens;
    size_t compressed_tokens;
    double compression_ratio;
    double cost_usd;
} CompactionResult;

// Check if compaction needed
bool compaction_needed(const char* session_id, size_t current_tokens);

// Perform compaction
CompactionResult* compaction_summarize(
    const char* session_id,
    int64_t from_msg_id,
    int64_t to_msg_id,
    const char* messages_text
);

// Get active checkpoint for session
char* compaction_get_checkpoint(const char* session_id);

// Free result
void compaction_result_free(CompactionResult* result);
```

### Step 3: Summarization Prompt

```
You are a conversation summarizer. Compress the following conversation
into a concise summary that preserves:

1. Key decisions made
2. Important facts learned
3. Current state of any tasks
4. User preferences expressed

Be extremely concise. Use bullet points. Maximum 500 tokens.

CONVERSATION:
{messages}

SUMMARY:
```

### Step 4: Integration in build_context_prompt()

Modify `orchestrator.c` line ~740:

```c
// After loading conversation history
char* conversation = persistence_load_conversation_context(session_id, 100);

// Estimate tokens
size_t conv_tokens = tokens_estimate(conversation, g_orchestrator->provider_type);

// Check threshold
if (conv_tokens > COMPACTION_THRESHOLD_TOKENS) {
    // Get or create checkpoint
    char* checkpoint = compaction_get_checkpoint(session_id);

    if (!checkpoint) {
        // Create new checkpoint
        CompactionResult* result = compaction_summarize(
            session_id,
            first_msg_id,
            last_msg_id - COMPACTION_KEEP_RECENT_MSGS,
            conversation
        );
        checkpoint = strdup(result->summary);
        compaction_result_free(result);
    }

    // Build context with checkpoint + recent messages
    char* recent = persistence_load_conversation_context(
        session_id,
        COMPACTION_KEEP_RECENT_MSGS
    );

    snprintf(context_section, capacity,
        "## Previous Context (Summarized)\n%s\n\n"
        "## Recent Conversation\n%s\n",
        checkpoint, recent);

    free(checkpoint);
    free(recent);
} else {
    // Use full conversation as-is
    snprintf(context_section, capacity,
        "## Conversation History\n%s\n",
        conversation);
}

free(conversation);
```

### Step 5: Persistence Functions

Add to `persistence.c`:

```c
int persistence_save_checkpoint(
    const char* session_id,
    int checkpoint_num,
    int64_t from_msg_id,
    int64_t to_msg_id,
    int messages_compressed,
    const char* summary,
    size_t original_tokens,
    size_t compressed_tokens,
    double cost
);

char* persistence_load_latest_checkpoint(const char* session_id);

int persistence_get_checkpoint_count(const char* session_id);
```

### Step 6: Update Makefile/CMake

Add new source file to build:
```makefile
SOURCES += src/context/compaction.c
```

---

## Configuration

Add to `config/defaults.json`:

```json
{
  "context_compaction": {
    "enabled": true,
    "threshold_tokens": 80000,
    "keep_recent_messages": 10,
    "summarization_model": "claude-haiku-4.5",
    "max_summary_tokens": 500
  }
}
```

---

## Testing Plan (Comprehensive)

### File: `tests/test_compaction.c` (NEW)

```c
// Unit tests following existing test_unit.c pattern
void test_compaction_threshold_detection(void);
void test_compaction_token_estimation(void);
void test_compaction_summarize_basic(void);
void test_compaction_checkpoint_save(void);
void test_compaction_checkpoint_load(void);
void test_compaction_multi_checkpoint(void);
void test_compaction_fallback_on_error(void);
void test_compaction_thread_safety(void);
```

**Test Cases:**

| Test | Description | Expected |
|------|-------------|----------|
| `threshold_below_80k` | 50K tokens context | NO compaction trigger |
| `threshold_above_80k` | 100K tokens context | Compaction triggers |
| `summarize_preserves_key_facts` | Input with decisions | Decisions in output |
| `summarize_reduces_tokens` | 80K input | <20K output |
| `checkpoint_save_load_roundtrip` | Save + Load | Identical content |
| `checkpoint_multiple_sessions` | 3 sessions | Correct isolation |
| `fallback_on_api_failure` | Mock API error | Graceful truncation |
| `concurrent_compaction_safe` | 2 threads | No corruption |

### File: `tests/integration/test_context_flow.c` (NEW)

```c
// Integration tests with mock provider
void test_full_conversation_100_messages(void);
void test_agent_delegation_with_compaction(void);
void test_context_consistency_multi_agent(void);
void test_cost_tracking_with_compaction(void);
```

**Test Cases:**

| Test | Description | Expected |
|------|-------------|----------|
| `conversation_100_msgs` | 100 msgs → compaction | Context <80K |
| `delegation_same_context` | Ali→Marco,Sara | Same snapshot |
| `cost_reduction` | Before/after compare | 50%+ savings |
| `no_info_loss_critical` | Key decisions present | All preserved |

### Update: `tests/e2e_test.sh`

Add new section:

```bash
# ============================================
# CONTEXT COMPACTION TESTS
# ============================================

test_context_compaction() {
    print_section "Context Compaction"

    # Test 1: Long conversation triggers compaction
    for i in {1..50}; do
        echo "Message $i with some content to fill context" | $CONVERGIO_BIN
    done

    # Verify checkpoint created
    sqlite3 "$DATA_DIR/convergio.db" \
        "SELECT COUNT(*) FROM checkpoint_summaries" | grep -q "[1-9]"
    check_result "Checkpoint created after long conversation"

    # Test 2: Context size reduced
    CONTEXT_SIZE=$(sqlite3 "$DATA_DIR/convergio.db" \
        "SELECT compressed_tokens FROM checkpoint_summaries ORDER BY id DESC LIMIT 1")
    [ "$CONTEXT_SIZE" -lt 20000 ]
    check_result "Context compressed to <20K tokens"

    # Test 3: Cost tracking accurate
    COMPACTION_COST=$(sqlite3 "$DATA_DIR/convergio.db" \
        "SELECT SUM(summary_cost_usd) FROM checkpoint_summaries")
    [ "$(echo "$COMPACTION_COST < 0.01" | bc)" -eq 1 ]
    check_result "Compaction cost minimal (<$0.01)"
}
```

### Makefile Updates

```makefile
# Add to Makefile
compaction_test: $(BUILD_DIR)/test_compaction
	@echo "Running compaction tests..."
	@$(BUILD_DIR)/test_compaction

$(BUILD_DIR)/test_compaction: tests/test_compaction.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(filter-out $(BUILD_DIR)/main.o,$(OBJS)) \
		tests/test_stubs.c $(LDFLAGS)

test: fuzz_test unit_test compaction_test check-docs
```

### Test Execution Order

```bash
# Full test suite execution
make clean
make
make test              # Runs all: fuzz + unit + compaction + docs
./tests/e2e_test.sh    # End-to-end with real API
```

### Mock Provider for Tests

Use existing `tests/mock_provider.c` to simulate LLM responses:

```c
// In test_compaction.c
static const char* mock_summary_response =
    "## Summary\n"
    "- User requested feature X\n"
    "- Decided to use approach Y\n"
    "- Key constraint: budget limit\n";

void setup_mock_summarization(void) {
    mock_provider_set_response(mock_summary_response);
    mock_provider_set_tokens(150, 50);  // Input/output tokens
}

---

## Expected Results

| Metric | Before | After |
|--------|--------|-------|
| Context tokens (100 msgs) | ~80,000 | ~15,000 |
| Token cost per request | $0.08 | $0.015 |
| Context quality | Full | 90%+ preserved |
| Summarization overhead | N/A | ~$0.001/checkpoint |

**Net savings: ~80% token reduction per long conversation**

---

## Risk Mitigation

1. **Information Loss**: Keep `key_facts` JSON for critical data extraction
2. **Summarization Failure**: Fallback to truncation if API fails
3. **Cost Spike**: Cap summarization calls per session (max 5)
4. **Thread Safety**: Use existing mutex pattern from persistence.c

## Multi-Agent Context Consistency (VERIFIED SAFE)

L'architettura attuale garantisce che **tutti gli agenti ricevano lo stesso contesto**:

```
Timeline di una request:
─────────────────────────────────────────────────────────────
T0: User Input arriva
T1: build_context_prompt() → CHECK 80K tokens
T2: SE > 80K: compaction_summarize() → salva checkpoint
T3: Costruisce contesto finale (checkpoint + recent 10 msg)
T4: Ali processa con contesto compattato
T5: Ali delega a Marco, Sara, Leo
    └─ Ogni agente riceve strdup(context) ← STESSA SNAPSHOT
T6: Agenti lavorano in parallelo (contesto immutabile)
T7: Ali sintetizza risposte
─────────────────────────────────────────────────────────────

GARANZIA: La compaction avviene SOLO a T2, MAI durante T5-T6.
          Tutti gli agenti vedono esattamente lo stesso contesto.
```

**Perche' e' sicuro:**
- Ogni agente riceve una COPIA (`strdup()`) non un riferimento
- Il contesto e' costruito UNA VOLTA prima della delegazione
- Nessun agente puo' modificare il contesto degli altri
- La persistence e' thread-safe (mutex gia' implementato)

---

## Rollout

1. Feature flag `COMPACTION_ENABLED` (default: false)
2. Test with single user session
3. Monitor compression ratios
4. Enable by default after validation
