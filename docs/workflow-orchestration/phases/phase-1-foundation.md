# PHASE 1 - Foundation: Workflow Engine Core

**Status**: ⏸️ PENDING  
**Estimated Duration**: 2 weeks (10 working days)  
**Branch**: `phase-1-foundation`

---

## Objective

Implement the core workflow engine with state machine execution and checkpointing capabilities.

---

## Tasks

| ID | Task | Status | Effort | Dependencies |
|----|------|--------|--------|--------------|
| F1 | Database Schema | ⏸️ | 2 days | None |
| F2 | Workflow Data Structures | ⏸️ | 2 days | F1 |
| F3 | Basic State Machine | ⏸️ | 3 days | F2 |
| F4 | Checkpoint Manager | ⏸️ | 3 days | F2, F3 |

---

## Task Details

### F1: Database Schema (Day 1-2)

**Goal**: Create database tables for workflows, nodes, edges, state, and checkpoints.

**Files to Create:**
- `src/memory/migrations/016_workflow_engine.sql`

**Implementation:**
```sql
-- Migration 016: Workflow Engine
BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS workflows (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    entry_node_id INTEGER,
    status INTEGER NOT NULL DEFAULT 0,
    current_node_id INTEGER,
    created_at INTEGER NOT NULL,
    updated_at INTEGER,
    last_checkpoint_at INTEGER,
    error_message TEXT,
    metadata_json TEXT
);

-- ... (see full schema in architecture.md)
```

**Testing:**
- Migration runs successfully
- Tables created with correct schema
- Foreign keys work correctly
- Indexes created

**Acceptance Criteria:**
- [ ] Migration script complete
- [ ] All tables created
- [ ] Foreign keys enforced
- [ ] Indexes created
- [ ] Migration test passes

---

### F2: Workflow Data Structures (Day 3-4)

**Goal**: Define and implement core data structures for workflows.

**Files to Create:**
- `include/nous/workflow.h` - Header file
- `src/workflow/workflow_types.c` - Type definitions and memory management

**Implementation:**
```c
// Core types
typedef struct WorkflowNode { ... } WorkflowNode;
typedef struct Workflow { ... } Workflow;
typedef struct WorkflowState { ... } WorkflowState;

// Memory management
Workflow* workflow_create(const char* name, ...);
void workflow_destroy(Workflow* wf);
WorkflowNode* workflow_node_create(...);
void workflow_node_destroy(WorkflowNode* node);
```

**Security Requirements:**
- All string allocations checked for NULL
- All frees followed by NULL assignment
- Bounds checking on all string operations
- Input validation on all create functions

**Testing:**
- Unit tests for create/destroy
- Memory leak detection (Address Sanitizer)
- NULL pointer handling
- Bounds checking

**Acceptance Criteria:**
- [ ] All data structures defined
- [ ] Memory management functions implemented
- [ ] No memory leaks
- [ ] Unit tests passing
- [ ] Security checklist complete

---

### F3: Basic State Machine (Day 5-7)

**Goal**: Implement basic workflow execution with state machine.

**Files to Create:**
- `src/workflow/workflow_engine.c` - Core execution engine

**Implementation:**
```c
// Core execution
int workflow_execute(Workflow* wf, const char* input, char** output);
int workflow_execute_node(Workflow* wf, WorkflowNode* node, char** output);
WorkflowNode* workflow_get_next_node(Workflow* wf, WorkflowNode* current);

// State management
WorkflowState* workflow_get_state(Workflow* wf);
int workflow_set_state(Workflow* wf, const char* key, const char* value);
```

**Security Requirements:**
- Input validation on all functions
- Bounds checking on state values
- Error handling (no silent failures)
- Thread safety (if global state)

**Testing:**
- Unit tests for linear workflows
- Error handling tests
- State management tests
- Integration with existing orchestrator

**Acceptance Criteria:**
- [ ] Linear workflows execute correctly
- [ ] State transitions work
- [ ] Error handling complete
- [ ] Unit tests passing (>= 80% coverage)
- [ ] Integration tests passing

---

### F4: Checkpoint Manager (Day 8-10)

**Goal**: Implement checkpoint creation, storage, and restoration.

**Files to Create:**
- `src/workflow/checkpoint.c` - Checkpoint management

**Implementation:**
```c
// Checkpoint operations
uint64_t workflow_checkpoint(Workflow* wf, const char* node_name);
int workflow_restore_from_checkpoint(Workflow* wf, uint64_t checkpoint_id);
Checkpoint* workflow_list_checkpoints(Workflow* wf, size_t* count);
```

**Security Requirements:**
- SQL queries parameterized (prevent SQL injection)
- State serialization validated
- Corrupted checkpoint handling
- Access control (user-specific checkpoints)

**Testing:**
- Unit tests for checkpoint create/restore
- Corrupted checkpoint handling
- State serialization/deserialization
- Fuzz tests for checkpoint restoration

**Acceptance Criteria:**
- [ ] Checkpoints created successfully
- [ ] Checkpoints restored correctly
- [ ] Corrupted checkpoints handled gracefully
- [ ] Unit tests passing
- [ ] Fuzz tests passing
- [ ] Security checklist complete

---

## Modified Files

- `src/memory/migrations/016_workflow_engine.sql` (new)
- `include/nous/workflow.h` (new)
- `src/workflow/workflow_types.c` (new)
- `src/workflow/workflow_engine.c` (new)
- `src/workflow/checkpoint.c` (new)
- `src/core/commands/workflow.c` (new - CLI commands)
- `src/core/commands/commands.c` (updated - add workflow commands)
- `tests/test_workflow.c` (new)
- `CMakeLists.txt` (updated - add new sources)
- `Makefile` (updated - add test targets)
- `README.md` (updated - add workflow section)
- `docs/workflow-orchestration/USER_GUIDE.md` (new)

---

## Testing Requirements

**See [TESTING_PLAN.md](../TESTING_PLAN.md) for complete test specifications.**

### Mandatory Tests (ZERO TOLERANCE)

**Every function MUST have tests. Every test MUST pass.**

#### F1: Database Schema Tests

**File**: `tests/test_workflow_migration.c`

- [ ] Migration executes successfully
- [ ] All tables created with correct schema
- [ ] Foreign keys enforced
- [ ] Indexes created
- [ ] Rollback works on error
- [ ] Migration is idempotent

**Coverage Target**: 100% (migration is critical)

#### F2: Data Structures Tests

**File**: `tests/test_workflow_types.c`

- [ ] Create/destroy works correctly
- [ ] NULL pointer handling (all create functions)
- [ ] Memory leak detection (100 iterations with Address Sanitizer)
- [ ] Bounds checking (name too long, value too long)
- [ ] Double-free prevention
- [ ] Use-after-free prevention
- [ ] String allocation validation
- [ ] Error propagation (no silent failures)

**Coverage Target**: >= 90% (all public functions)

#### F3: State Machine Tests

**File**: `tests/test_workflow_engine.c`

- [ ] Linear workflow execution
- [ ] State transitions (all states)
- [ ] Error handling (all error paths)
- [ ] State management (get/set/clear)
- [ ] Thread safety (concurrent execution with Thread Sanitizer)
- [ ] Input validation (NULL, invalid states)
- [ ] Output validation
- [ ] Error propagation

**Coverage Target**: >= 85% (all execution paths)

#### F4: Checkpoint Manager Tests

**File**: `tests/test_workflow_checkpoint.c`

- [ ] Checkpoint creation
- [ ] Checkpoint restoration
- [ ] Corrupted checkpoint handling (graceful failure)
- [ ] State serialization/deserialization
- [ ] Fuzz tests (checkpoint restoration with LLVMFuzzerTestOneInput)
- [ ] SQL injection prevention (parameterized queries verified)
- [ ] Access control (user-specific checkpoints)
- [ ] Checkpoint cleanup

**Coverage Target**: 100% (checkpointing is critical)

### Test Execution (Before Every Commit)

```bash
# Quick tests (fast feedback)
make test_workflow_quick
```

### Test Execution (Before Every PR)

```bash
# Full test suite
make test_workflow                    # Unit tests
make integration_test_workflow       # Integration tests
make fuzz_test_workflow               # Fuzz tests (run in background)
make DEBUG=1 SANITIZE=address,undefined,thread test_workflow  # Sanitizers
make coverage_workflow                # Coverage report (must be >= 80%)
```

### Quality Gate (Mandatory)

```bash
# Run all quality gates
make quality_gate_workflow
```

**Must pass ALL:**
- ✅ Build succeeds
- ✅ No warnings
- ✅ All tests pass
- ✅ Coverage >= 80%
- ✅ No memory leaks
- ✅ No data races
- ✅ Static analysis clean
- ✅ Security audit passed

**See [ZERO_TOLERANCE_POLICY.md](../ZERO_TOLERANCE_POLICY.md) for details.**

---

## Security Checklist

- [ ] All SQL queries parameterized
- [ ] All string operations have bounds checking
- [ ] All global state protected by mutex (if applicable)
- [ ] All errors are logged (no silent failures)
- [ ] All memory allocations checked for NULL
- [ ] All frees followed by NULL assignment
- [ ] Input validation complete
- [ ] Security tests pass

---

## Performance Targets

| Operation | Target | Measurement |
|-----------|--------|-------------|
| Workflow creation | < 10ms | Time to create workflow |
| Node execution | < 100ms | Time per node (excluding agent call) |
| Checkpoint creation | < 50ms | Time to create checkpoint |
| Checkpoint restore | < 100ms | Time to restore from checkpoint |

---

## Integration Tasks (Phase 1)

### CLI Commands

- [ ] Implement `cmd_workflow()` in `src/core/commands/workflow.c`
- [ ] Add workflow commands to `COMMANDS[]` array
- [ ] Add workflow help entries to `DETAILED_HELP[]`
- [ ] Test all CLI commands

### Documentation

- [ ] Update README.md with workflow section
- [ ] Create `docs/workflow-orchestration/USER_GUIDE.md`
- [ ] Add workflow commands to help system
- [ ] Update main documentation index

### Immediate Usability

- [ ] Users can list workflows (`/workflow list`)
- [ ] Users can execute workflows (`/workflow execute`)
- [ ] Users can resume from checkpoints (`/workflow resume`)
- [ ] Help system shows workflow commands (`/help workflow`)

## Crash Recovery

**If development is interrupted or needs restart:**

1. **Check Git Status**
   ```bash
   git status
   git log --oneline -5  # Check last commits
   ```

2. **Recover Work**
   ```bash
   # If uncommitted work exists
   git stash save "WIP: [describe work]"
   # OR commit it
   git commit -m "WIP: [describe work]"
   ```

3. **Resume from Checkpoint**
   ```bash
   # Check for checkpoint tags
   git tag | grep checkpoint
   
   # Resume from last checkpoint
   git checkout checkpoint/phase-1-f1  # Or last good checkpoint
   ```

**See [CRASH_RECOVERY.md](../CRASH_RECOVERY.md) for complete procedures.**

## Parallel Development

**Within Phase 1:**
- While implementing F(N), write tests for F(N-1) in parallel
- Documentation can be written in parallel with implementation

**Between Phases:**
- Phases 2, 3, 4 can run in parallel (after Phase 1)
- Saves ~14 days of development time

**See [PARALLEL_DEVELOPMENT.md](../PARALLEL_DEVELOPMENT.md) for strategy.**

## Zero Tolerance Checklist

**Before PR, verify ALL:**

- [ ] Build passes with zero warnings
- [ ] All unit tests pass (>= 80% coverage)
- [ ] All integration tests pass
- [ ] All fuzz tests pass
- [ ] All sanitizer tests pass (ASan, UBSan, TSan)
- [ ] No memory leaks detected
- [ ] No data races detected
- [ ] Static analysis clean (no issues)
- [ ] Security audit passed
- [ ] All errors handled (no silent failures)
- [ ] All SQL queries parameterized
- [ ] All input validated
- [ ] Documentation complete

**See [ZERO_TOLERANCE_POLICY.md](../ZERO_TOLERANCE_POLICY.md) for enforcement details.**

## Definition of Done

- [ ] All tasks completed
- [ ] **ALL tests passing** (unit, integration, fuzz, sanitizer)
- [ ] **Code coverage >= 80%** (verified with `make coverage_workflow`)
- [ ] **Zero warnings** (verified with `make 2>&1 | grep warning`)
- [ ] **No memory leaks** (Address Sanitizer clean)
- [ ] **No data races** (Thread Sanitizer clean)
- [ ] **Security checklist complete** (all items checked)
- [ ] **Performance targets met** (all operations within targets)
- [ ] **CLI commands implemented and tested** (all commands work)
- [ ] **Documentation updated** (README, USER_GUIDE, help system)
- [ ] **Quality gate passed** (`make quality_gate_workflow`)
- [ ] **App-release-manager verification passed** (`@app-release-manager check quality gates`)
- [ ] **PR created and reviewed**
- [ ] **All review comments addressed**

**ZERO TOLERANCE: All items must be checked before PR merge.**

---

## Next Phase

After Phase 1 completion, proceed to [Phase 2 - Task Decomposition](phase-2-task-decomposition.md).

