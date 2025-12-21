# Workflow Orchestration Architecture

**Created**: 2025-12-20  
**Status**: Complete  
**Version**: 1.0.0

---

## Overview

The Workflow Orchestration system provides a state machine-based framework for coordinating multi-agent workflows in Convergio. It enables complex task decomposition, conditional routing, checkpointing, and recovery.

---

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Convergio CLI                             │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────┐      ┌──────────────────┐            │
│  │   Orchestrator    │◄────►│  Workflow Engine │            │
│  │   (Ali + Agents)  │      │  (State Machine) │            │
│  └──────────────────┘      └──────────────────┘            │
│         │                            │                       │
│         │                            │                       │
│         ▼                            ▼                       │
│  ┌──────────────────┐      ┌──────────────────┐            │
│  │  Agent Execution  │      │  Task Decomposer  │            │
│  │  (Delegation)     │      │  (LLM-based)     │            │
│  └──────────────────┘      └──────────────────┘            │
│         │                            │                       │
│         │                            │                       │
│         └────────────┬───────────────┘                       │
│                      │                                       │
│                      ▼                                       │
│         ┌──────────────────────┐                            │
│         │   Group Chat Manager │                            │
│         │   (Consensus)        │                            │
│         └──────────────────────┘                            │
│                      │                                       │
│                      ▼                                       │
│         ┌──────────────────────┐                            │
│         │  Conditional Router  │                            │
│         │  (State-based)       │                            │
│         └──────────────────────┘                            │
│                      │                                       │
│                      ▼                                       │
│         ┌──────────────────────┐                            │
│         │  Checkpoint Manager   │                            │
│         │  (SQLite)            │                            │
│         └──────────────────────┘                            │
│                      │                                       │
│                      ▼                                       │
│         ┌──────────────────────┐                            │
│         │  Persistence Layer    │                            │
│         │  (SQLite)             │                            │
│         └──────────────────────┘                            │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. Workflow Engine (`workflow_engine.c`)

**Purpose**: Core state machine execution engine

**Responsibilities**:
- Workflow state transitions
- Node execution coordination
- State management (key-value store)
- Integration with orchestrator for agent execution

**Key Functions**:
- `workflow_execute()` - Execute workflow from start
- `workflow_execute_node()` - Execute single node
- `workflow_get_next_node()` - Determine next node based on state
- `workflow_pause()` / `workflow_resume()` - Control flow

**State Machine**:
```
PENDING → RUNNING → [PAUSED] → COMPLETED
                ↓
             FAILED
                ↓
            CANCELLED
```

### 2. Workflow Types (`workflow_types.c`)

**Purpose**: Data structures and memory management

**Key Structures**:
- `Workflow` - Main workflow container
- `WorkflowNode` - Individual workflow step
- `WorkflowState` - Key-value state store
- `NodeType` - Node type enum (ACTION, DECISION, HUMAN_INPUT, etc.)

**Memory Management**:
- All allocations checked for NULL
- Proper cleanup in `workflow_destroy()`
- String duplication with validation

### 3. Checkpoint Manager (`checkpoint.c`)

**Purpose**: Workflow persistence and recovery

**Responsibilities**:
- Create checkpoints at any workflow state
- Restore workflow from checkpoint
- Serialize/deserialize workflow state (JSON)
- SQLite persistence

**Key Functions**:
- `workflow_checkpoint()` - Create checkpoint
- `workflow_restore_from_checkpoint()` - Restore from checkpoint
- `workflow_list_checkpoints()` - List all checkpoints
- `workflow_save()` / `workflow_load()` - Persistence

**Database Schema**:
- `workflows` - Workflow definitions
- `workflow_nodes` - Node definitions
- `workflow_edges` - Node connections
- `workflow_state` - Key-value state
- `workflow_checkpoints` - Checkpoint snapshots

### 4. Task Decomposer (`task_decomposer.c`)

**Purpose**: Break down complex tasks into subtasks

**Responsibilities**:
- LLM-based task decomposition
- Dependency resolution (topological sort)
- Execution plan creation
- Parallel execution coordination

**Key Functions**:
- `task_decompose()` - Decompose task using LLM
- `task_resolve_dependencies()` - Resolve task dependencies
- `task_create_execution_plan()` - Create execution plan
- `task_execute_parallel()` - Execute independent tasks in parallel

**Inspired By**: CrewAI task decomposition patterns

### 5. Group Chat Manager (`group_chat.c`)

**Purpose**: Multi-agent discussions and consensus

**Responsibilities**:
- Turn-taking logic (round-robin, priority, consensus)
- Consensus detection (voting mechanism)
- Message history management
- Summary generation

**Key Functions**:
- `group_chat_create()` - Create group chat
- `group_chat_add_message()` - Add message
- `group_chat_get_next_speaker()` - Determine next speaker
- `group_chat_check_consensus()` - Check for consensus
- `group_chat_get_summary()` - Generate summary

**Inspired By**: AutoGen group chat patterns

### 6. Conditional Router (`router.c`)

**Purpose**: Dynamic workflow routing based on state

**Responsibilities**:
- Condition evaluation (boolean expressions)
- State-based routing decisions
- Fallback node handling

**Key Functions**:
- `router_evaluate_condition()` - Evaluate condition expression
- Supports: `==`, `!=`, `>`, `<`, `>=`, `<=`, `&&`, `||`, `!`

**Inspired By**: LangGraph conditional routing

### 7. Pattern Library (`patterns.c`)

**Purpose**: Reusable workflow patterns

**Available Patterns**:
- **Review-Refine Loop**: Iterative improvement pattern
- **Parallel Analysis**: Multiple agents analyze in parallel
- **Sequential Planning**: Step-by-step planning workflow
- **Consensus Building**: Group discussion to reach consensus

**Key Functions**:
- `pattern_create_review_refine_loop()`
- `pattern_create_parallel_analysis()`
- `pattern_create_sequential_planning()`
- `pattern_create_consensus_building()`

### 8. Error Handling (`error_handling.c`)

**Purpose**: Comprehensive error handling and recovery

**Error Types**:
- `WORKFLOW_ERROR_TIMEOUT` - Node execution timeout
- `WORKFLOW_ERROR_NETWORK` - Network connectivity issues
- `WORKFLOW_ERROR_FILE_IO` - File I/O errors
- `WORKFLOW_ERROR_CREDIT_EXHAUSTED` - Budget exceeded
- `WORKFLOW_ERROR_LLM_DOWN` - LLM service unavailable
- `WORKFLOW_ERROR_TOOL_FAILED` - Tool execution failure

**Recovery Strategies**:
- Retry logic (configurable max retries)
- Fallback nodes
- Error state handling
- Graceful degradation

### 9. Observability (`workflow_observability.c`)

**Purpose**: Logging, telemetry, and security integration

**Integration Points**:
- Global logging system (`nous_log()`)
- Telemetry system (`telemetry_record_*()`)
- Security audit logging
- Performance metrics

**Key Functions**:
- `workflow_log_event()` - Log workflow events
- `workflow_log_node_execution()` - Log node execution
- `workflow_telemetry_start()` / `workflow_telemetry_end()` - Telemetry
- `workflow_security_audit_event()` - Security audit

### 10. Visualization (`workflow_visualization.c`)

**Purpose**: Mermaid diagram export

**Key Functions**:
- `workflow_export_mermaid()` - Export to Mermaid format
- `workflow_export_mermaid_alloc()` - Allocated version
- Supports all node types with proper shapes

**Output Format**: Mermaid flowchart syntax

---

## Data Flow

### Workflow Execution Flow

```
1. User Input
   ↓
2. Workflow Creation/Load
   ↓
3. Task Decomposition (optional)
   ↓
4. Node Execution Loop:
   ├─ Execute Node (via Orchestrator)
   ├─ Update State
   ├─ Evaluate Conditions
   ├─ Determine Next Node
   └─ Create Checkpoint (optional)
   ↓
5. Conditional Routing
   ↓
6. Next Node or Completion
   ↓
7. Output Generation
```

### State Management Flow

```
Workflow State (Key-Value Store)
   ↓
Node Execution
   ↓
State Updates
   ↓
Condition Evaluation
   ↓
Routing Decision
```

### Checkpoint Flow

```
Workflow Execution
   ↓
Checkpoint Trigger
   ↓
State Serialization (JSON)
   ↓
SQLite Persistence
   ↓
Recovery (on resume)
   ↓
State Deserialization
   ↓
Workflow Restoration
```

---

## Integration Points

### With Orchestrator

- **Agent Execution**: Workflow nodes execute via orchestrator's agent delegation
- **Cost Tracking**: Workflow state stores cost estimates, orchestrator tracks actual costs
- **Session Management**: Workflows use orchestrator's session system

### With Providers

- **LLM Calls**: Task decomposition and agent execution use provider system
- **Telemetry**: Provider telemetry integrated with workflow telemetry
- **Error Handling**: Provider errors propagate to workflow error handling

### With Persistence

- **Database**: SQLite for workflow definitions, state, and checkpoints
- **Migration System**: Uses Convergio's migration system (016_workflow_engine.sql)
- **Foreign Keys**: Proper referential integrity

### With Tools

- **Security**: All file operations use `tools_is_path_safe()`
- **Command Execution**: All commands use `tools_is_command_safe()`
- **Input Validation**: All inputs validated via `workflow_validate_*()`

---

## Design Decisions

### State Machine vs. Direct Execution

**Decision**: State machine-based execution

**Rationale**:
- Enables checkpointing and recovery
- Supports conditional routing
- Clear state transitions
- Better error handling

### SQLite for Persistence

**Decision**: Use SQLite (existing Convergio database)

**Rationale**:
- Already integrated in Convergio
- ACID guarantees
- Foreign key support
- Migration system in place

### JSON for State Serialization

**Decision**: JSON for checkpoint state serialization

**Rationale**:
- Human-readable
- Easy to debug
- Standard format
- cJSON library available

### LLM-based Task Decomposition

**Decision**: Use LLM for task decomposition

**Rationale**:
- Flexible and adaptive
- Can handle complex tasks
- Leverages existing provider system
- No hardcoded rules

---

## Performance Considerations

### Optimization Strategies

1. **Checkpoint Optimization**: Incremental checkpoints (future enhancement)
2. **State Caching**: In-memory state cache
3. **Parallel Execution**: Independent tasks run in parallel
4. **Lazy Loading**: Checkpoints loaded on demand

### Bottlenecks

1. **LLM Calls**: Task decomposition and agent execution
2. **Database I/O**: Checkpoint creation/restoration
3. **State Serialization**: JSON serialization for large states

---

## Security Considerations

### Input Validation

- All workflow names validated (`workflow_validate_name()`)
- All state keys validated (`workflow_validate_key()`)
- All conditions validated (`workflow_validate_condition_safe()`)

### SQL Injection Prevention

- All SQL queries parameterized (`sqlite3_bind_*()`)
- No string concatenation in SQL

### Command Injection Prevention

- All command executions use `tools_is_command_safe()`
- Network checks use hardcoded safe commands

### Path Traversal Prevention

- All file operations use `tools_is_path_safe()`
- Database paths validated

---

## Limitations

1. **No Nested Workflows**: Subgraph nodes not fully implemented
2. **Limited Parallelism**: Parallel nodes execute sequentially (future enhancement)
3. **No Workflow Templates**: Templates exist but not fully integrated
4. **No Workflow Versioning**: Workflow definitions not versioned

---

## Future Enhancements

1. **Incremental Checkpoints**: Only save delta changes
2. **Binary Serialization**: Faster than JSON for large states
3. **Workflow Templates**: Template library integration
4. **Workflow Versioning**: Version control for workflow definitions
5. **Real Parallelism**: True parallel node execution
6. **Workflow Composition**: Combine multiple workflows

---

**Last Updated**: 2025-12-20  
**Maintainer**: Convergio Team

