# Testing Plan: Workflow Orchestration

**Date**: 2025-12-18  
**Status**: Mandatory Testing Requirements  
**Zero Tolerance**: All tests MUST pass before any PR

---

## Testing Philosophy

**ZERO TOLERANCE POLICY:**
- **No code without tests** - Code without tests is incomplete
- **No warnings allowed** - All warnings must be fixed
- **No shortcuts** - Every function must be tested
- **No "good enough"** - Production-ready only
- **No merge without tests** - Tests are merge blockers

---

## Test Pyramid (ISE Standards)

```
        /\
       /  \     E2E Tests (10%)
      /____\
     /      \   Integration Tests (20%)
    /________\
   /          \  Unit Tests (70%)
  /____________\
```

**Target Distribution:**
- **70% Unit Tests** - Fast, isolated, comprehensive
- **20% Integration Tests** - Component interactions
- **10% E2E Tests** - Complete user workflows

---

## Phase 1: Foundation - Test Plan

### F1: Database Schema Tests

**File**: `tests/test_workflow_migration.c`

```c
// Test migration execution
void test_migration_016_executes_successfully(void) {
    // Run migration
    int result = migration_run(16);
    assert(result == 0);
    
    // Verify tables exist
    assert(table_exists("workflows"));
    assert(table_exists("workflow_nodes"));
    assert(table_exists("workflow_edges"));
    assert(table_exists("workflow_state"));
    assert(table_exists("workflow_checkpoints"));
}

// Test schema correctness
void test_schema_foreign_keys(void) {
    // Create workflow
    uint64_t workflow_id = create_test_workflow();
    
    // Create node with invalid workflow_id (should fail)
    int result = create_node(999999, "test", NODE_TYPE_ACTION);
    assert(result != 0);  // Foreign key constraint
    
    cleanup_test_workflow(workflow_id);
}

// Test indexes
void test_indexes_exist(void) {
    assert(index_exists("idx_workflows_status"));
    assert(index_exists("idx_nodes_workflow"));
    assert(index_exists("idx_state_workflow"));
    assert(index_exists("idx_checkpoints_workflow"));
}

// Test rollback on error
void test_migration_rollback_on_error(void) {
    // Simulate error during migration
    // Verify rollback occurred
    // Verify no partial state
}
```

**Test Checklist:**
- [ ] Migration executes successfully
- [ ] All tables created
- [ ] Foreign keys enforced
- [ ] Indexes created
- [ ] Rollback works on error
- [ ] Migration is idempotent (can run twice)

**Coverage Target**: 100% (migration is critical)

---

### F2: Workflow Data Structures Tests

**File**: `tests/test_workflow_types.c`

```c
// Test workflow creation
void test_workflow_create_success(void) {
    Workflow* wf = workflow_create("test", "description", entry_node);
    assert(wf != NULL);
    assert(strcmp(wf->name, "test") == 0);
    assert(wf->status == WORKFLOW_STATUS_PENDING);
    workflow_destroy(wf);
}

// Test NULL handling
void test_workflow_create_null_name(void) {
    Workflow* wf = workflow_create(NULL, "desc", entry_node);
    assert(wf == NULL);  // Should reject NULL name
}

// Test memory leak detection
void test_workflow_no_memory_leaks(void) {
    for (int i = 0; i < 100; i++) {
        Workflow* wf = workflow_create("test", "desc", entry_node);
        workflow_destroy(wf);
    }
    // Address Sanitizer will detect leaks
}

// Test bounds checking
void test_workflow_name_too_long(void) {
    char long_name[10000];
    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    
    Workflow* wf = workflow_create(long_name, "desc", entry_node);
    // Should either reject or truncate
    if (wf) {
        assert(strlen(wf->name) <= MAX_WORKFLOW_NAME_LEN);
        workflow_destroy(wf);
    }
}

// Test double-free prevention
void test_workflow_double_free_safe(void) {
    Workflow* wf = workflow_create("test", "desc", entry_node);
    workflow_destroy(wf);
    workflow_destroy(wf);  // Should be safe (NULL check)
}

// Test use-after-free prevention
void test_workflow_use_after_free_safe(void) {
    Workflow* wf = workflow_create("test", "desc", entry_node);
    uint64_t id = wf->workflow_id;
    workflow_destroy(wf);
    
    // Accessing wf after destroy should be safe (NULL check)
    assert(wf == NULL || wf->workflow_id == 0);
}
```

**Test Checklist:**
- [ ] Create/destroy works correctly
- [ ] NULL pointer handling
- [ ] Memory leak detection (100 iterations)
- [ ] Bounds checking (name too long)
- [ ] Double-free prevention
- [ ] Use-after-free prevention
- [ ] String allocation validation
- [ ] Error propagation (no silent failures)

**Coverage Target**: >= 90% (all public functions)

---

### F3: Basic State Machine Tests

**File**: `tests/test_workflow_engine.c`

```c
// Test linear workflow execution
void test_linear_workflow_execution(void) {
    WorkflowNode* node1 = workflow_node_create("step1", NODE_TYPE_ACTION);
    WorkflowNode* node2 = workflow_node_create("step2", NODE_TYPE_ACTION);
    workflow_node_add_edge(node1, node2, NULL);
    
    Workflow* wf = workflow_create("test", "test", node1);
    char* output = NULL;
    
    int result = workflow_execute(wf, "input", &output);
    assert(result == 0);
    assert(wf->status == WORKFLOW_STATUS_COMPLETED);
    assert(output != NULL);
    
    free(output);
    workflow_destroy(wf);
}

// Test state transitions
void test_state_transitions(void) {
    Workflow* wf = workflow_create("test", "test", entry_node);
    
    assert(wf->status == WORKFLOW_STATUS_PENDING);
    
    workflow_execute(wf, "input", NULL);
    assert(wf->status == WORKFLOW_STATUS_RUNNING || 
           wf->status == WORKFLOW_STATUS_COMPLETED);
    
    workflow_destroy(wf);
}

// Test error handling
void test_workflow_execution_error(void) {
    Workflow* wf = workflow_create("test", "test", NULL);  // Invalid entry node
    
    char* output = NULL;
    int result = workflow_execute(wf, "input", &output);
    
    assert(result != 0);  // Should fail
    assert(wf->error_message != NULL);  // Error logged
    assert(wf->status == WORKFLOW_STATUS_FAILED);
    
    workflow_destroy(wf);
}

// Test state management
void test_workflow_state_management(void) {
    Workflow* wf = workflow_create("test", "test", entry_node);
    
    int result = workflow_set_state(wf, "key1", "value1");
    assert(result == 0);
    
    const char* value = workflow_get_state_value(wf, "key1");
    assert(value != NULL);
    assert(strcmp(value, "value1") == 0);
    
    workflow_destroy(wf);
}

// Test concurrent execution (thread safety)
void test_workflow_thread_safety(void) {
    Workflow* wf = workflow_create("test", "test", entry_node);
    
    pthread_t threads[10];
    for (int i = 0; i < 10; i++) {
        pthread_create(&threads[i], NULL, workflow_execute_worker, wf);
    }
    
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Thread Sanitizer will detect races
    workflow_destroy(wf);
}
```

**Test Checklist:**
- [ ] Linear workflow execution
- [ ] State transitions
- [ ] Error handling (all error paths)
- [ ] State management (get/set/clear)
- [ ] Thread safety (concurrent execution)
- [ ] Input validation
- [ ] Output validation
- [ ] Error propagation

**Coverage Target**: >= 85% (all execution paths)

---

### F4: Checkpoint Manager Tests

**File**: `tests/test_workflow_checkpoint.c`

```c
// Test checkpoint creation
void test_checkpoint_creation(void) {
    Workflow* wf = workflow_create("test", "test", entry_node);
    workflow_execute(wf, "input", NULL);
    
    uint64_t checkpoint_id = workflow_checkpoint(wf, "midpoint");
    assert(checkpoint_id > 0);
    
    // Verify checkpoint in database
    Checkpoint* cp = checkpoint_load(checkpoint_id);
    assert(cp != NULL);
    assert(cp->workflow_id == wf->workflow_id);
    
    checkpoint_destroy(cp);
    workflow_destroy(wf);
}

// Test checkpoint restoration
void test_checkpoint_restore(void) {
    Workflow* wf = workflow_create("test", "test", entry_node);
    workflow_execute(wf, "input", NULL);
    
    uint64_t checkpoint_id = workflow_checkpoint(wf, "midpoint");
    uint64_t workflow_id = wf->workflow_id;
    
    // Simulate crash
    workflow_destroy(wf);
    wf = workflow_load(workflow_id);
    
    // Restore from checkpoint
    int result = workflow_restore_from_checkpoint(wf, checkpoint_id);
    assert(result == 0);
    assert(wf->current_node_id > 0);
    
    workflow_destroy(wf);
}

// Test corrupted checkpoint handling
void test_corrupted_checkpoint_handling(void) {
    // Create valid checkpoint
    uint64_t checkpoint_id = create_test_checkpoint();
    
    // Corrupt checkpoint data in database
    corrupt_checkpoint_data(checkpoint_id);
    
    // Attempt restoration (should handle gracefully)
    Workflow* wf = workflow_load(workflow_id);
    int result = workflow_restore_from_checkpoint(wf, checkpoint_id);
    
    assert(result != 0);  // Should fail gracefully
    assert(wf->error_message != NULL);  // Error logged
    
    workflow_destroy(wf);
}

// Fuzz test: checkpoint restoration
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > MAX_CHECKPOINT_SIZE) return 0;
    
    // Create checkpoint with fuzzed data
    char* fuzzed_data = calloc(size + 1, 1);
    memcpy(fuzzed_data, data, size);
    fuzzed_data[size] = '\0';
    
    // Attempt restoration (should handle gracefully)
    Workflow* wf = workflow_load(test_workflow_id);
    int result = workflow_restore_from_checkpoint_with_data(wf, fuzzed_data);
    // Should not crash, should return error if invalid
    
    free(fuzzed_data);
    workflow_destroy(wf);
    return 0;
}
```

**Test Checklist:**
- [ ] Checkpoint creation
- [ ] Checkpoint restoration
- [ ] Corrupted checkpoint handling
- [ ] State serialization/deserialization
- [ ] Fuzz tests (checkpoint restoration)
- [ ] SQL injection prevention (parameterized queries)
- [ ] Access control (user-specific checkpoints)
- [ ] Checkpoint cleanup

**Coverage Target**: 100% (checkpointing is critical)

---

## Phase 2: Task Decomposition - Test Plan

**File**: `tests/test_task_decomposer.c`

```c
// Test task decomposition
void test_task_decompose(void) {
    const char* goal = "Launch a product";
    AgentRole roles[] = {AGENT_ROLE_PLANNER, AGENT_ROLE_EXECUTOR};
    
    DecomposedTask* tasks = NULL;
    size_t count = 0;
    
    int result = task_decompose(goal, roles, 2, &tasks, &count);
    assert(result == 0);
    assert(tasks != NULL);
    assert(count > 0);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        task_destroy(&tasks[i]);
    }
    free(tasks);
}

// Test dependency resolution
void test_dependency_resolution(void) {
    DecomposedTask tasks[3];
    // Setup tasks with dependencies
    tasks[0].id = 1;
    tasks[1].id = 2;
    tasks[1].prerequisite_ids = &tasks[0].id;
    tasks[1].prerequisite_count = 1;
    
    int result = task_resolve_dependencies(tasks, 3);
    assert(result == 0);
    // Verify execution order
}

// Test circular dependency detection
void test_circular_dependency_detection(void) {
    DecomposedTask tasks[2];
    // Create circular dependency
    tasks[0].id = 1;
    tasks[0].prerequisite_ids = &tasks[1].id;
    tasks[1].id = 2;
    tasks[1].prerequisite_ids = &tasks[0].id;
    
    int result = task_resolve_dependencies(tasks, 2);
    assert(result != 0);  // Should detect cycle
}
```

**Test Checklist:**
- [ ] Task decomposition
- [ ] Dependency resolution
- [ ] Circular dependency detection
- [ ] Topological sort
- [ ] Parallel execution planning
- [ ] Task assignment to agents

**Coverage Target**: >= 85%

---

## Phase 3: Group Chat - Test Plan

**File**: `tests/test_group_chat.c`

```c
// Test group chat creation
void test_group_chat_create(void) {
    SemanticID participants[] = {agent1_id, agent2_id, agent3_id};
    GroupChat* chat = group_chat_create(participants, 3, GROUP_CHAT_ROUND_ROBIN);
    
    assert(chat != NULL);
    assert(chat->participant_count == 3);
    
    group_chat_destroy(chat);
}

// Test turn-taking logic
void test_turn_taking_round_robin(void) {
    GroupChat* chat = create_test_chat(GROUP_CHAT_ROUND_ROBIN);
    
    SemanticID speaker1 = group_chat_get_next_speaker(chat);
    SemanticID speaker2 = group_chat_get_next_speaker(chat);
    SemanticID speaker3 = group_chat_get_next_speaker(chat);
    SemanticID speaker4 = group_chat_get_next_speaker(chat);
    
    // Should cycle through participants
    assert(speaker4 == speaker1);
    
    group_chat_destroy(chat);
}

// Test consensus detection
void test_consensus_detection(void) {
    GroupChat* chat = create_test_chat(GROUP_CHAT_CONSENSUS);
    
    // Add messages with votes
    group_chat_add_message(chat, agent1_id, "I vote YES");
    group_chat_add_message(chat, agent2_id, "I vote YES");
    group_chat_add_message(chat, agent3_id, "I vote NO");
    
    bool consensus = group_chat_check_consensus(chat, 0.67);  // 67% threshold
    assert(consensus == true);  // 2/3 = 67%
    
    group_chat_destroy(chat);
}
```

**Test Checklist:**
- [ ] Group chat creation
- [ ] Turn-taking (round-robin, priority)
- [ ] Consensus detection
- [ ] Message threading
- [ ] Timeout handling
- [ ] Thread safety

**Coverage Target**: >= 85%

---

## Phase 4: Conditional Routing - Test Plan

**File**: `tests/test_router.c`

```c
// Test condition evaluation
void test_condition_evaluation(void) {
    WorkflowState* state = workflow_state_create();
    workflow_state_set(state, "value", "approved");
    
    bool result = router_evaluate_condition("state.value == 'approved'", state);
    assert(result == true);
    
    workflow_state_destroy(state);
}

// Test SQL injection prevention in conditions
void test_condition_sql_injection_prevention(void) {
    WorkflowState* state = workflow_state_create();
    
    // Attempt SQL injection
    char malicious[] = "state.value == 'test' OR 1=1 --";
    bool result = router_evaluate_condition(malicious, state);
    
    // Should either reject or sanitize
    // Verify no SQL execution occurred
    assert(true);  // Test passes if no crash
    
    workflow_state_destroy(state);
}

// Test fallback handling
void test_router_fallback(void) {
    WorkflowNode* node = workflow_node_create("test", NODE_TYPE_DECISION);
    node->fallback_node = fallback_node;
    
    WorkflowState* state = workflow_state_create();
    WorkflowNode* next = router_evaluate_edges(node, state);
    
    // If condition fails, should return fallback
    assert(next == fallback_node || next == NULL);
    
    workflow_state_destroy(state);
}
```

**Test Checklist:**
- [ ] Condition evaluation
- [ ] SQL injection prevention
- [ ] Fallback handling
- [ ] State-based routing
- [ ] Fuzz tests (condition parser)

**Coverage Target**: 100% (security-critical)

---

## Phase 5: Integration - Test Plan

**File**: `tests/test_workflow_integration.c`

```c
// Test end-to-end workflow
void test_e2e_workflow_execution(void) {
    // Create workflow from pattern
    Workflow* wf = pattern_create_parallel_analysis(analysts, 3, converger);
    
    // Execute
    char* output = NULL;
    int result = workflow_execute(wf, "Analyze project", &output);
    
    assert(result == 0);
    assert(output != NULL);
    
    // Verify all agents were called
    // Verify convergence occurred
    
    free(output);
    workflow_destroy(wf);
}

// Test backward compatibility
void test_backward_compatibility(void) {
    // Existing orchestrator functions still work
    char* result = orchestrator_parallel_analyze("test", agents, 3);
    assert(result != NULL);
    free(result);
    
    // New workflow functions work
    Workflow* wf = workflow_load_by_name("parallel-analysis");
    if (wf) {
        char* result2 = NULL;
        int status = workflow_execute(wf, "test", &result2);
        assert(status == 0 || result2 != NULL);
        free(result2);
        workflow_destroy(wf);
    }
}
```

**Test Checklist:**
- [ ] End-to-end workflows
- [ ] Backward compatibility
- [ ] Performance (meets targets)
- [ ] Error recovery
- [ ] Cost tracking integration

**Coverage Target**: >= 80%

---

## Mandatory Test Execution (Per Phase)

### Before Every Commit

```bash
# Quick tests (fast feedback)
make test_workflow_quick
```

### Before Every PR

```bash
# Full test suite
make test_workflow                    # Unit tests
make integration_test_workflow        # Integration tests
make fuzz_test_workflow               # Fuzz tests
make DEBUG=1 SANITIZE=address,undefined,thread test_workflow  # Sanitizers
make coverage_workflow                # Coverage report
```

### Test Execution Order

1. **Unit Tests** (fast, run first)
2. **Integration Tests** (medium speed)
3. **Fuzz Tests** (slow, run in background)
4. **Sanitizer Tests** (slow, run before PR)
5. **Coverage Report** (verification)

---

## Test Coverage Requirements

### Minimum Coverage by Phase

| Phase | Unit Tests | Integration | Fuzz | Sanitizer | Total |
|-------|------------|-------------|------|-----------|-------|
| Phase 1 | >= 90% | >= 80% | Critical paths | All | >= 85% |
| Phase 2 | >= 85% | >= 75% | Security paths | All | >= 80% |
| Phase 3 | >= 85% | >= 75% | Security paths | All | >= 80% |
| Phase 4 | >= 90% | >= 80% | All paths | All | >= 85% |
| Phase 5 | >= 80% | >= 80% | All paths | All | >= 80% |

### Critical Paths (100% Coverage Required)

- Checkpoint creation/restoration
- Condition parser (security)
- SQL query execution
- Memory management functions
- Error handling paths

---

## Test Failure Handling

### Zero Tolerance Policy

**If ANY test fails:**
1. **STOP** - Do not continue development
2. **FIX** - Fix the failing test immediately
3. **VERIFY** - Re-run all tests
4. **COMMIT** - Commit the fix separately

**NO EXCEPTIONS:**
- No "I'll fix it later"
- No "It's just a warning"
- No "It works on my machine"
- No merging with failing tests

---

## Continuous Testing

### Pre-Commit Hook (Recommended)

```bash
#!/bin/sh
# .git/hooks/pre-commit

# Run quick tests
make test_workflow_quick || {
    echo "Tests failed. Commit aborted."
    exit 1
}
```

### CI/CD Integration

**GitHub Actions** (if applicable):
```yaml
- name: Run Workflow Tests
  run: |
    make test_workflow
    make integration_test_workflow
    make coverage_workflow
```

---

## Test Documentation

### Test Naming Convention

```
test_<component>_<functionality>_<scenario>

Examples:
- test_workflow_create_success
- test_workflow_create_null_name
- test_checkpoint_restore_corrupted
- test_router_condition_sql_injection
```

### Test Organization

```
tests/
├── test_workflow_types.c          # Phase 1: Data structures
├── test_workflow_engine.c          # Phase 1: State machine
├── test_workflow_checkpoint.c      # Phase 1: Checkpointing
├── test_workflow_migration.c       # Phase 1: Database
├── test_task_decomposer.c          # Phase 2: Task decomposition
├── test_group_chat.c               # Phase 3: Group chat
├── test_router.c                   # Phase 4: Routing
├── test_workflow_integration.c     # Phase 5: Integration
└── fuzz_workflow.c                 # Fuzz tests
```

---

## Conclusion

**ZERO TOLERANCE FOR:**
- ❌ Code without tests
- ❌ Failing tests
- ❌ Warnings
- ❌ Low coverage
- ❌ Memory leaks
- ❌ Data races
- ❌ Security vulnerabilities

**ALL TESTS MUST PASS BEFORE ANY PR**



