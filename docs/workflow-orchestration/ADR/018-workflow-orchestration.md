# ADR-018: Workflow Orchestration System

**Status**: Accepted
**Date**: 2025-12-21
**Deciders**: AI Team, Roberto
**Context**: Need for advanced multi-agent coordination beyond simple delegation

---

## Context

Convergio's current orchestrator provides basic agent delegation and cost tracking, but lacks:
1. **Complex workflow coordination** - Multi-step, conditional, iterative workflows
2. **State persistence** - No checkpoint/resume capability for long-running workflows
3. **Structured collaboration** - No built-in patterns for multi-agent discussions
4. **Conditional routing** - No dynamic path selection based on results

Users and the Ali agent need sophisticated workflow patterns like:
- Review-refine loops (generate → critique → refine → repeat)
- Parallel analysis (multiple analysts → converge results)
- Consensus building (multi-agent discussion → voting → decision)
- Sequential planning with validation checkpoints

---

## Decision

We implement a comprehensive **Workflow Orchestration System** with the following components:

### 1. State Machine-Based Workflow Engine

A workflow engine that:
- Executes workflows as directed acyclic graphs (DAGs) with loops
- Supports multiple node types (ACTION, DECISION, PARALLEL, CONVERGE, HUMAN_INPUT)
- Manages state transitions based on conditions
- Integrates with existing orchestrator for agent execution

### 2. Checkpoint and Recovery System

A persistence layer that:
- Creates checkpoints at critical workflow points
- Stores workflow state in SQLite
- Enables resume from any checkpoint
- Survives process restarts

### 3. Task Decomposition Engine

An LLM-powered decomposer that:
- Breaks complex tasks into subtasks
- Creates dependency graphs
- Assigns agents by role matching
- Provides execution ordering via topological sort

### 4. Group Chat Manager

A collaboration system that:
- Manages multi-agent discussions
- Supports round-robin, priority, and consensus modes
- Detects consensus via voting mechanism
- Maintains message history

### 5. Conditional Router

A routing engine that:
- Evaluates condition expressions against workflow state
- Supports comparison operators (==, !=, <, >, <=, >=)
- Validates conditions for security (prevents injection)
- Enables dynamic path selection

### 6. Pattern Library

Reusable workflow patterns:
- Review-Refine Loop
- Parallel Analysis
- Sequential Planning
- Consensus Building

---

## Consequences

### Positive

1. **Enhanced Capabilities**
   - Complex multi-step workflows now possible
   - Workflows can pause, checkpoint, and resume
   - Multiple agents can collaborate with structured coordination
   - Conditional paths enable dynamic behavior

2. **Backward Compatibility**
   - 100% backward compatible with existing orchestrator
   - Workflows are optional (existing code unchanged)
   - Gradual adoption possible

3. **Production Ready**
   - Comprehensive error handling
   - Full observability (logging, telemetry, audit)
   - Security validation on all inputs
   - 80+ test cases

4. **Developer Experience**
   - CLI commands for workflow management
   - Reusable pattern library
   - Clear documentation

### Negative

1. **Complexity**
   - Additional code to maintain (~10,000 lines)
   - New concepts for users to learn
   - More integration points

2. **Performance**
   - Slight overhead for state management
   - Database operations for checkpointing
   - Condition evaluation per routing decision

3. **Dependencies**
   - Requires SQLite for persistence
   - LLM calls for task decomposition

### Mitigation

1. **Complexity**: Comprehensive documentation, patterns for common cases
2. **Performance**: Lazy loading, caching, optimized queries
3. **Dependencies**: Graceful degradation when LLM unavailable

---

## Alternatives Considered

### Alternative 1: External Workflow Engine (Temporal, Airflow)

**Pros:**
- Battle-tested, feature-rich
- Built-in UI and monitoring

**Cons:**
- Heavy dependencies (requires external services)
- Breaks Convergio's zero-dependency philosophy
- Complex integration with existing agents
- Overkill for our use cases

**Decision**: Rejected - too heavyweight

### Alternative 2: Simple Sequential Pipeline

**Pros:**
- Easy to implement
- Minimal complexity

**Cons:**
- No conditional routing
- No parallel execution
- No checkpointing
- Limited patterns

**Decision**: Rejected - insufficient functionality

### Alternative 3: Embedded Workflow DSL

**Pros:**
- Declarative workflow definition
- JSON/YAML-based configuration

**Cons:**
- Requires parser implementation
- Limited expressiveness
- Learning curve for DSL

**Decision**: Partially adopted - JSON templates supported, but not required

---

## Implementation

### Files Created

| File | Purpose | Lines |
|------|---------|-------|
| `src/workflow/workflow_types.c` | Data structures, memory management | ~400 |
| `src/workflow/workflow_engine.c` | Core execution engine | ~600 |
| `src/workflow/checkpoint.c` | Persistence and recovery | ~300 |
| `src/workflow/task_decomposer.c` | LLM-based decomposition | ~500 |
| `src/workflow/group_chat.c` | Multi-agent discussions | ~400 |
| `src/workflow/router.c` | Conditional routing | ~300 |
| `src/workflow/patterns.c` | Pattern library | ~400 |
| `src/workflow/error_handling.c` | Error management | ~400 |
| `src/workflow/workflow_observability.c` | Logging, telemetry | ~300 |
| `include/nous/workflow.h` | Public API header | ~200 |
| `include/nous/router.h` | Router header | ~50 |
| `include/nous/patterns.h` | Patterns header | ~50 |

### Database Schema

Migration `016_workflow_engine.sql` creates:
- `workflows` - Workflow definitions
- `workflow_nodes` - Node definitions
- `workflow_edges` - Transitions
- `workflow_state` - Key-value state
- `workflow_checkpoints` - Checkpoint data

### CLI Commands

```bash
/workflow list              # List workflows
/workflow show <name>       # Show workflow details + Mermaid diagram
/workflow execute <name>    # Execute workflow
/workflow resume <id>       # Resume from checkpoint
```

---

## Testing

### Test Coverage

| Test File | Test Cases | Focus |
|-----------|------------|-------|
| `test_workflow_types.c` | 10+ | Data structures, memory |
| `test_workflow_engine.c` | 15+ | Execution, transitions |
| `test_checkpoint.c` | 10+ | Persistence, recovery |
| `test_task_decomposer.c` | 8+ | Decomposition, ordering |
| `test_group_chat.c` | 10+ | Turn-taking, consensus |
| `test_router.c` | 12+ | Condition evaluation |
| `test_patterns.c` | 8+ | Pattern creation |
| `test_workflow_error_handling.c` | 7+ | Error scenarios |
| `test_workflow_e2e.c` | 10+ | End-to-end scenarios |

### Quality Gates

- All tests passing
- Coverage >= 80%
- Zero memory leaks (ASan)
- Zero data races (TSan)
- Zero undefined behavior (UBSan)
- Security audit passed

---

## References

- [Master Plan](../MASTER_PLAN.md)
- [Architecture](../architecture.md)
- [Technical Documentation](../TECHNICAL_DOCUMENTATION.md)
- [User Guide](../USER_GUIDE.md)
- [Security Checklist](../SECURITY_CHECKLIST.md)
- [Testing Plan](../TESTING_PLAN.md)
