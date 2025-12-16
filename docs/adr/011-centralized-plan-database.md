# ADR-011: Centralized Execution Plan Database

**Date**: 2025-12-16
**Status**: Implemented
**Author**: AI Team

## Context

Convergio's multi-agent architecture requires coordination between agents working on complex, multi-step tasks. The existing in-memory planning system (`orchestrator/planning.c`) lacks:

1. **Persistence** - Plans are lost when the process exits
2. **Thread-safety** - No atomic operations for concurrent agent access
3. **Progress tracking** - No way to resume interrupted work
4. **Audit trail** - No history of completed tasks

Inspired by Anthropic's `claude-quickstarts/autonomous-coding` pattern, we need a centralized, persistent plan system.

## Decision

Implement a **SQLite-backed plan database** with the following characteristics:

### Architecture

```
┌─────────────────────────────────────────────────────┐
│                   Plan Database                      │
│  ┌───────────────┐    ┌────────────────────────┐   │
│  │ plans table   │    │ tasks table            │   │
│  │ - id (UUID)   │◄───│ - id (UUID)            │   │
│  │ - description │    │ - plan_id (FK)         │   │
│  │ - status      │    │ - parent_task_id (FK)  │   │
│  │ - created_at  │    │ - description          │   │
│  │ - updated_at  │    │ - assigned_agent       │   │
│  │ - completed_at│    │ - status               │   │
│  └───────────────┘    │ - priority             │   │
│                       │ - started_at           │   │
│                       │ - completed_at         │   │
│                       │ - output/error         │   │
│                       └────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

### Thread-Safety Strategy

1. **WAL mode** - SQLite Write-Ahead Logging for concurrent reads
2. **IMMEDIATE transactions** - Exclusive write lock on transaction start
3. **Atomic task claiming** - `UPDATE ... WHERE status='pending'` pattern
4. **Busy timeout** - 5 second retry with exponential backoff

### Key Operations

| Operation | Thread-Safe | Description |
|-----------|-------------|-------------|
| `plan_db_create_plan()` | Yes | Create new plan with UUID |
| `plan_db_claim_task()` | Yes | Atomic status change, prevents double-claiming |
| `plan_db_complete_task()` | Yes | Mark task done with output |
| `plan_db_get_progress()` | Yes | Real-time progress stats |
| `plan_db_export_markdown()` | Yes | Export to MD with Mermaid |

### Database Location

Default: `~/.convergio/plans.db`

Can be overridden via `plan_db_init(custom_path)`.

## Alternatives Considered

### Option A: File-based JSON (like autonomous-coding)

**Pros:**
- Simple, human-readable
- No database dependency

**Cons:**
- Race conditions with concurrent writes
- No atomic operations
- Manual file locking required

**Decision:** Rejected - SQLite already linked in project, provides ACID guarantees.

### Option B: Redis

**Pros:**
- Fast, concurrent
- Pub/sub for real-time updates

**Cons:**
- External dependency
- Overkill for single-machine CLI

**Decision:** Rejected - External service dependency not acceptable.

### Option C: Shared memory + mutex

**Pros:**
- Fastest possible

**Cons:**
- Complex crash recovery
- Data loss on process crash

**Decision:** Rejected - Persistence is a hard requirement.

## Implementation

### Files Added

| File | Purpose |
|------|---------|
| `include/nous/plan_db.h` | Public API (50+ functions) |
| `src/orchestrator/plan_db.c` | SQLite implementation (~900 LOC) |
| `tests/test_plan_db.c` | Unit + concurrency tests (25 cases) |

### Integration with Existing Planner

The existing `orch_plan_create()` and `orch_task_create()` remain for in-memory operations. The new `plan_db_*` functions provide persistent backing when needed.

```c
// Existing in-memory (unchanged)
ExecutionPlan* plan = orch_plan_create("goal");

// New persistent (optional)
char plan_id[64];
plan_db_create_plan("goal", "context", plan_id);
```

### Schema

```sql
CREATE TABLE plans (
    id TEXT PRIMARY KEY,
    description TEXT NOT NULL,
    context TEXT,
    status TEXT CHECK(status IN ('pending','active','completed','failed','cancelled')),
    created_at INTEGER,
    updated_at INTEGER,
    completed_at INTEGER
);

CREATE TABLE tasks (
    id TEXT PRIMARY KEY,
    plan_id TEXT REFERENCES plans(id) ON DELETE CASCADE,
    parent_task_id TEXT REFERENCES tasks(id),
    description TEXT NOT NULL,
    assigned_agent TEXT,
    status TEXT CHECK(status IN ('pending','in_progress','completed','failed','blocked','skipped')),
    priority INTEGER CHECK(priority >= 0 AND priority <= 100),
    created_at INTEGER,
    started_at INTEGER,
    completed_at INTEGER,
    output TEXT,
    error TEXT,
    retry_count INTEGER DEFAULT 0
);
```

## Consequences

### Positive

- **Resumable work** - Plans survive process restart
- **Multi-agent safe** - Atomic task claiming
- **Auditable** - Full history of what was done
- **Exportable** - Markdown reports with Mermaid diagrams
- **Cleanable** - Automatic cleanup of old plans

### Negative

- **Disk I/O** - SQLite writes (mitigated by WAL mode)
- **Complexity** - New API to learn
- **Migration** - Existing code uses in-memory planner

### Risks

- **Schema evolution** - May need migrations in future
- **Performance at scale** - Not tested beyond 10K tasks

## Testing

Run tests: `make plan_db_test`

Coverage:
- CRUD operations
- Thread-safe task claiming (4 concurrent workers)
- Progress tracking
- Export functions
- Cascade delete
- Cleanup policies

## Future Work

1. **Planner integration** - Have existing planner auto-persist to plan_db
2. **CLI commands** - `convergio plan list`, `convergio plan status`
3. **Schema versioning** - Migration system for future changes
4. **Metrics** - Track completion times, failure rates
