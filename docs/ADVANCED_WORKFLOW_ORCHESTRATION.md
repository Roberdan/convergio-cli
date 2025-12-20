# Advanced Workflow Orchestration - Complete Specification

**Version**: 1.0  
**Date**: 2025-12-18  
**Last Updated**: 2025-12-18  
**Author**: Convergio Team  
**Status**: Specification - Ready for Implementation  
**Branch**: `feature/workflow-orchestration`  
**Worktree**: `workflow-orchestration` (create with `git worktree add ../ConvergioCLI-workflow workflow-orchestration`)

> **📋 IMPLEMENTATION GUIDE**: This document contains the complete specification. For implementation workflow, see:
> - **[Master Plan](workflow-orchestration/MASTER_PLAN.md)** - Overview, benefits summary, development workflow, PR process
> - **[Phase 1](workflow-orchestration/phases/phase-1-foundation.md)** - Foundation (workflow engine, checkpointing)
> - **[Phase 2](workflow-orchestration/phases/phase-2-task-decomposition.md)** - Task Decomposition (CrewAI patterns)
> - **[Phase 3](workflow-orchestration/phases/phase-3-group-chat.md)** - Group Chat (AutoGen patterns)
> - **[Phase 4](workflow-orchestration/phases/phase-4-conditional-routing.md)** - Conditional Routing (LangGraph patterns)
> - **[Phase 5](workflow-orchestration/phases/phase-5-integration.md)** - Integration & Polish

---

## Executive Summary

This document specifies the implementation of advanced workflow orchestration patterns inspired by LangGraph, CrewAI, and AutoGen, while maintaining Convergio's native C/Swift architecture and Apple Silicon optimizations.

### Expected Benefits

**1. Enhanced Multi-Agent Coordination**
- **Current**: Hardcoded parallel/sequential execution patterns
- **After**: Dynamic state machine-based workflows with conditional routing
- **Impact**: More flexible agent coordination, easier to add new patterns

**2. Workflow Persistence & Recovery**
- **Current**: Lost progress on crashes, no resume capability
- **After**: Checkpoint-based persistence, resume from any point
- **Impact**: Better reliability, no lost work, supports long-running workflows

**3. Reusable Workflow Patterns**
- **Current**: Each workflow pattern must be coded manually
- **After**: Pre-built pattern library (Review-Refine, Parallel Analysis, etc.)
- **Impact**: Faster development, consistent patterns, easier maintenance

**4. Better Debugging & Observability**
- **Current**: Limited visibility into workflow execution
- **After**: Workflow state visualization, checkpoint inspection, execution logs
- **Impact**: Easier debugging, better understanding of agent coordination

**5. Advanced Task Decomposition**
- **Current**: Simple task breakdown
- **After**: Hierarchical task decomposition with dependency resolution (CrewAI-inspired)
- **Impact**: More sophisticated task planning, automatic parallelization

**6. Group Chat & Consensus Building**
- **Current**: Agents work independently
- **After**: Multi-agent discussions with consensus detection (AutoGen-inspired)
- **Impact**: Better collaboration, consensus-driven decisions

**7. Conditional Routing & Dynamic Adaptation**
- **Current**: Fixed execution paths
- **After**: State-based conditional routing, dynamic workflow adaptation
- **Impact**: More intelligent workflows, adaptive behavior

**8. Zero Performance Impact**
- **Current**: Native C/Swift performance
- **After**: Maintains native performance, no Python dependencies
- **Impact**: No performance regression, Apple Silicon optimizations preserved

**9. Cost Management Integration**
- **Current**: Basic cost tracking
- **After**: Per-workflow cost estimation and tracking
- **Impact**: Better cost control, workflow-level budgeting

**10. Security & Quality Standards**
- **Current**: Convergio security standards
- **After**: Full compliance with SECURITY_AUDIT.md, comprehensive testing
- **Impact**: Maintains security posture, production-ready code

### Key Goals
- State machine-based workflow execution with checkpointing
- Advanced task decomposition patterns (inspired by CrewAI)
- Group chat and iterative refinement (inspired by AutoGen)
- Conditional routing and dynamic workflow adaptation
- Zero Python dependencies - pure C/Swift implementation
- Full compliance with Convergio security and quality standards

---

## Table of Contents

1. [Current State Analysis](#current-state-analysis)
2. [Pattern Analysis from Other Frameworks](#pattern-analysis-from-other-frameworks)
3. [Architecture Design](#architecture-design)
4. [Implementation Plan](#implementation-plan)
5. [API Specification](#api-specification)
6. [Database Schema](#database-schema)
7. [Testing Strategy](#testing-strategy)
8. [Migration Path](#migration-path)

---

## Current State Analysis

### Existing Capabilities

Convergio currently supports:

1. **Parallel Execution** (ADR-002)
   - GCD-based parallel agent execution
   - P-core and E-core allocation
   - Parallel groups for independent tasks

2. **Sequential Pipelines**
   - Dependency-based task chains
   - Simple sequential execution

3. **Critic Loops**
   - Iterative refinement with validation
   - Basic approval/rejection logic

4. **Message Bus**
   - Asynchronous inter-agent communication
   - Message history and threading

5. **Execution Plans**
   - Task decomposition
   - Plan persistence (ADR-011)

### Current Limitations

1. **No State Machine**
   - Workflows are hardcoded in orchestrator logic
   - No visual representation of workflow
   - Difficult to modify workflows without code changes

2. **No Checkpointing**
   - Cannot resume interrupted workflows
   - No state persistence during execution
   - Lost progress on crashes

3. **Limited Conditional Routing**
   - Simple if/else logic only
   - No complex decision trees
   - No dynamic workflow adaptation

4. **No Workflow Templates**
   - Each workflow must be coded manually
   - No reusable workflow patterns
   - Difficult to share workflows between agents

5. **Limited Error Recovery**
   - Basic retry logic only
   - No fallback strategies
   - No error state handling

---

## Pattern Analysis from Other Frameworks

### 1. LangGraph Patterns

#### State Machine Architecture

**Concept:**
- Workflows defined as directed graphs
- Nodes = agent actions or decision points
- Edges = state transitions
- State persists between nodes

**Key Features:**
- Checkpointing at each node
- Conditional edges (routing based on state)
- Human-in-the-loop nodes
- Sub-graphs (nested workflows)

**Implementation Approach:**
```c
typedef struct {
    uint64_t node_id;
    char* name;
    NodeType type;  // ACTION, DECISION, HUMAN_INPUT, SUBGRAPH
    AgentID agent_id;  // Which agent executes this node
    char* condition_expr;  // For conditional edges
    WorkflowNode* next_nodes;  // Possible next nodes
} WorkflowNode;

typedef struct {
    uint64_t workflow_id;
    char* name;
    WorkflowNode* entry_node;
    WorkflowState* current_state;
    time_t created_at;
    time_t last_checkpoint;
} Workflow;
```

#### Checkpointing

**Concept:**
- Save workflow state after each node execution
- Enable resume from any checkpoint
- Support for rollback

**Implementation:**
- SQLite-backed checkpoint storage
- JSON serialization of workflow state
- Automatic checkpoint on node completion

### 2. CrewAI Patterns

#### Task Decomposition

**Concept:**
- Hierarchical task breakdown
- Role-based task assignment
- Task dependencies and prerequisites

**Key Features:**
- Task templates
- Automatic dependency resolution
- Parallel execution of independent tasks
- Task validation and approval

**Implementation Approach:**
```c
typedef struct {
    uint64_t task_id;
    char* description;
    AgentRole required_role;
    TaskStatus status;
    char** prerequisites;  // Task IDs that must complete first
    size_t prerequisite_count;
    char* validation_criteria;
    int max_retries;
    int current_retry;
} DecomposedTask;
```

#### Crew Formation

**Concept:**
- Dynamic agent selection based on task requirements
- Role matching algorithm
- Agent availability checking

**Implementation:**
- Agent registry with role tags
- Matching algorithm for task-agent pairing
- Load balancing across agents

### 3. AutoGen Patterns

#### Group Chat

**Concept:**
- Multiple agents participate in discussion
- Round-robin or priority-based turn-taking
- Consensus building

**Key Features:**
- Message threading
- Agent voting mechanisms
- Consensus detection
- Timeout handling

**Implementation Approach:**
```c
typedef struct {
    uint64_t chat_id;
    AgentID* participants;
    size_t participant_count;
    Message* message_history;
    GroupChatMode mode;  // ROUND_ROBIN, PRIORITY, CONSENSUS
    int max_rounds;
    int current_round;
    bool consensus_reached;
} GroupChat;
```

#### Iterative Refinement

**Concept:**
- Multi-round improvement loop
- Critic agent provides feedback
- Generator agent refines output
- Termination conditions

**Implementation:**
- Refinement loop state machine
- Quality metrics tracking
- Automatic termination on convergence

---

## Architecture Design

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    WORKFLOW ENGINE                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ State        │  │ Checkpoint   │  │ Router       │     │
│  │ Machine      │  │ Manager      │  │ (Conditional)│     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
                          │
        ┌─────────────────┼─────────────────┐
        ▼                 ▼                 ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ Task         │  │ Group Chat   │  │ Pattern      │
│ Decomposer   │  │ Manager      │  │ Library      │
└──────────────┘  └──────────────┘  └──────────────┘
        │                 │                 │
        └─────────────────┼─────────────────┘
                          ▼
                ┌──────────────────┐
                │  Orchestrator   │
                │  (Existing)      │
                └──────────────────┘
```

### Component Breakdown

#### 1. Workflow Engine (`src/workflow/workflow_engine.c`)

**Responsibilities:**
- Workflow state machine execution
- Node transition management
- State persistence
- Error handling and recovery

**Key Functions:**
```c
// Workflow lifecycle
Workflow* workflow_create(const char* name, WorkflowNode* entry_node);
int workflow_execute(Workflow* wf, const char* input, char** output);
int workflow_resume(Workflow* wf, uint64_t checkpoint_id);
void workflow_destroy(Workflow* wf);

// State management
WorkflowState* workflow_get_state(Workflow* wf);
int workflow_update_state(Workflow* wf, const char* key, const char* value);
int workflow_checkpoint(Workflow* wf);

// Node execution
int workflow_execute_node(Workflow* wf, WorkflowNode* node, char** output);
WorkflowNode* workflow_get_next_node(Workflow* wf, WorkflowNode* current);
```

#### 2. Checkpoint Manager (`src/workflow/checkpoint.c`)

**Responsibilities:**
- Checkpoint creation and storage
- Checkpoint retrieval
- State restoration
- Checkpoint cleanup

**Key Functions:**
```c
uint64_t checkpoint_create(Workflow* wf, const char* node_name);
int checkpoint_restore(Workflow* wf, uint64_t checkpoint_id);
int checkpoint_delete(uint64_t checkpoint_id);
Checkpoint* checkpoint_list(Workflow* wf, size_t* count);
```

#### 3. Task Decomposer (`src/workflow/task_decomposer.c`)

**Responsibilities:**
- Hierarchical task breakdown
- Dependency resolution
- Task scheduling
- Parallel execution planning

**Key Functions:**
```c
DecomposedTask* task_decompose(const char* goal, AgentRole* roles, size_t role_count);
int task_resolve_dependencies(DecomposedTask* tasks, size_t task_count);
ExecutionPlan* task_create_execution_plan(DecomposedTask* tasks, size_t task_count);
int task_execute_parallel(DecomposedTask* tasks, size_t task_count, dispatch_group_t group);
```

#### 4. Group Chat Manager (`src/workflow/group_chat.c`)

**Responsibilities:**
- Multi-agent conversation management
- Turn-taking logic
- Consensus detection
- Message threading

**Key Functions:**
```c
GroupChat* group_chat_create(AgentID* participants, size_t count, GroupChatMode mode);
int group_chat_add_message(GroupChat* chat, AgentID sender, const char* content);
AgentID group_chat_get_next_speaker(GroupChat* chat);
bool group_chat_check_consensus(GroupChat* chat, double threshold);
char* group_chat_get_summary(GroupChat* chat);
```

#### 5. Pattern Library (`src/workflow/patterns.c`)

**Responsibilities:**
- Pre-built workflow templates
- Common pattern implementations
- Pattern composition
- Pattern validation

**Key Patterns:**
- **Review-Refine Loop**: Generator → Critic → Refiner → Validator
- **Parallel Analysis**: Multiple agents analyze same input → Convergence
- **Sequential Planning**: Strategy → Plan → Architecture → Implementation
- **Consensus Building**: Discussion → Voting → Consensus → Action

**Key Functions:**
```c
Workflow* pattern_create_review_refine_loop(AgentID generator, AgentID critic, AgentID refiner);
Workflow* pattern_create_parallel_analysis(AgentID* analysts, size_t count, AgentID converger);
Workflow* pattern_create_sequential_planning(AgentID* planners, size_t count);
Workflow* pattern_create_consensus_building(AgentID* participants, size_t count);
```

#### 6. Conditional Router (`src/workflow/router.c`)

**Responsibilities:**
- Conditional edge evaluation
- Dynamic routing decisions
- State-based path selection
- Fallback handling

**Key Functions:**
```c
WorkflowNode* router_evaluate_edges(WorkflowNode* node, WorkflowState* state);
bool router_evaluate_condition(const char* condition_expr, WorkflowState* state);
WorkflowNode* router_get_fallback_node(WorkflowNode* node);
```

---

## Implementation Plan

### Phase 1: Foundation (Week 1-2)

**Goal:** Core workflow engine and state machine

#### Tasks:

1. **Database Schema** (Day 1-2)
   - Create workflow tables
   - Create checkpoint tables
   - Create workflow state tables
   - Migration script

2. **Workflow Data Structures** (Day 3-4)
   - `WorkflowNode` struct
   - `Workflow` struct
   - `WorkflowState` struct
   - Memory management functions

3. **Basic State Machine** (Day 5-7)
   - Node execution logic
   - State transitions
   - Simple linear workflows
   - Unit tests

4. **Checkpoint Manager** (Day 8-10)
   - Checkpoint creation
   - Checkpoint storage (SQLite)
   - Checkpoint restoration
   - Unit tests

**Deliverables:**
- `src/workflow/workflow_engine.c`
- `src/workflow/checkpoint.c`
- `include/nous/workflow.h`
- Database migration `016_workflow_engine.sql`
- Unit tests in `tests/test_workflow.c`

### Phase 2: Task Decomposition (Week 3-4)

**Goal:** CrewAI-inspired task decomposition

#### Tasks:

1. **Task Decomposer Core** (Day 11-13)
   - Hierarchical task structure
   - Dependency graph building
   - Topological sort for execution order
   - Unit tests

2. **Integration with Orchestrator** (Day 14-15)
   - Connect decomposer to existing orchestrator
   - Task assignment to agents
   - Parallel execution planning
   - Integration tests

3. **Task Templates** (Day 16-17)
   - Template system
   - Role-based task matching
   - Template library
   - Unit tests

**Deliverables:**
- `src/workflow/task_decomposer.c`
- `include/nous/task_decomposer.h`
- Template library in `src/workflow/templates/`
- Integration tests

### Phase 3: Group Chat & Iterative Refinement (Week 5-6)

**Goal:** AutoGen-inspired multi-agent collaboration

#### Tasks:

1. **Group Chat Manager** (Day 18-20)
   - Multi-agent conversation structure
   - Turn-taking logic (round-robin, priority)
   - Message threading
   - Unit tests

2. **Consensus Detection** (Day 21-22)
   - Voting mechanism
   - Consensus threshold
   - Agreement detection
   - Unit tests

3. **Iterative Refinement Loop** (Day 23-25)
   - Refinement state machine
   - Quality metrics
   - Termination conditions
   - Integration with critic agents
   - Unit tests

**Deliverables:**
- `src/workflow/group_chat.c`
- `src/workflow/refinement_loop.c`
- `include/nous/group_chat.h`
- Unit tests

### Phase 4: Conditional Routing & Advanced Patterns (Week 7-8)

**Goal:** LangGraph-inspired conditional routing and pattern library

#### Tasks:

1. **Conditional Router** (Day 26-28)
   - Condition expression parser
   - State-based routing
   - Fallback handling
   - Unit tests

2. **Pattern Library** (Day 29-32)
   - Review-Refine Loop pattern
   - Parallel Analysis pattern
   - Sequential Planning pattern
   - Consensus Building pattern
   - Pattern composition
   - Unit tests

3. **Workflow Visualization** (Day 33-35)
   - Graph export (Mermaid format)
   - Workflow status visualization
   - Debug output
   - CLI commands

**Deliverables:**
- `src/workflow/router.c`
- `src/workflow/patterns.c`
- `include/nous/patterns.h`
- Pattern templates
- Visualization utilities
- CLI commands (`/workflow list`, `/workflow show`, `/workflow resume`)

### Phase 5: Integration & Polish (Week 9-10)

**Goal:** Full integration with existing system

#### Tasks:

1. **Orchestrator Integration** (Day 36-38)
   - Replace hardcoded workflows with state machine
   - Migrate existing patterns
   - Backward compatibility
   - Integration tests

2. **Error Handling & Recovery** (Day 39-40)
   - Retry logic with state machine
   - Error state handling
   - Recovery strategies
   - Unit tests

3. **Performance Optimization** (Day 41-42)
   - Checkpoint optimization
   - State serialization optimization
   - Memory management
   - Performance tests

4. **Documentation & Examples** (Day 43-45)
   - API documentation
   - Usage examples
   - Pattern guide
   - Migration guide

**Deliverables:**
- Full integration
- Performance optimizations
- Complete documentation
- Example workflows
- Migration guide

---

## API Specification

### Workflow Engine API

#### Core Types

```c
// Workflow node types
typedef enum {
    NODE_TYPE_ACTION,        // Execute agent action
    NODE_TYPE_DECISION,       // Conditional routing
    NODE_TYPE_HUMAN_INPUT,    // Wait for user input
    NODE_TYPE_SUBGRAPH,       // Nested workflow
    NODE_TYPE_PARALLEL,       // Parallel execution
    NODE_TYPE_CONVERGE,       // Converge parallel results
} NodeType;

// Workflow node
typedef struct WorkflowNode {
    uint64_t node_id;
    char* name;
    NodeType type;
    SemanticID agent_id;      // Agent to execute (for ACTION nodes)
    char* action_prompt;      // What the agent should do
    char* condition_expr;     // Condition for conditional edges
    struct WorkflowNode** next_nodes;  // Possible next nodes
    size_t next_node_count;
    struct WorkflowNode* fallback_node; // Fallback if condition fails
    void* node_data;          // Type-specific data
} WorkflowNode;

// Workflow state (key-value store)
typedef struct {
    char* key;
    char* value;
    time_t updated_at;
} StateEntry;

typedef struct {
    StateEntry* entries;
    size_t entry_count;
    size_t entry_capacity;
} WorkflowState;

// Workflow
typedef struct {
    uint64_t workflow_id;
    char* name;
    char* description;
    WorkflowNode* entry_node;
    WorkflowState* state;
    WorkflowStatus status;
    uint64_t current_node_id;
    time_t created_at;
    time_t last_checkpoint;
    char* error_message;
} Workflow;

// Workflow status
typedef enum {
    WORKFLOW_STATUS_PENDING,
    WORKFLOW_STATUS_RUNNING,
    WORKFLOW_STATUS_PAUSED,      // Waiting for human input
    WORKFLOW_STATUS_COMPLETED,
    WORKFLOW_STATUS_FAILED,
    WORKFLOW_STATUS_CANCELLED,
} WorkflowStatus;
```

#### Core Functions

```c
// Workflow lifecycle
Workflow* workflow_create(const char* name, const char* description, WorkflowNode* entry_node);
int workflow_execute(Workflow* wf, const char* input, char** output);
int workflow_resume(Workflow* wf, uint64_t checkpoint_id);
int workflow_pause(Workflow* wf);
int workflow_cancel(Workflow* wf);
void workflow_destroy(Workflow* wf);

// State management
WorkflowState* workflow_get_state(Workflow* wf);
int workflow_set_state(Workflow* wf, const char* key, const char* value);
const char* workflow_get_state_value(Workflow* wf, const char* key);
int workflow_clear_state(Workflow* wf);

// Checkpointing
uint64_t workflow_checkpoint(Workflow* wf, const char* node_name);
int workflow_restore_from_checkpoint(Workflow* wf, uint64_t checkpoint_id);
Checkpoint* workflow_list_checkpoints(Workflow* wf, size_t* count);

// Node management
WorkflowNode* workflow_node_create(const char* name, NodeType type);
int workflow_node_add_edge(WorkflowNode* from, WorkflowNode* to, const char* condition);
int workflow_node_set_agent(WorkflowNode* node, SemanticID agent_id, const char* prompt);
WorkflowNode* workflow_get_current_node(Workflow* wf);
WorkflowNode* workflow_get_next_node(Workflow* wf, WorkflowNode* current);

// Persistence
int workflow_save(Workflow* wf);
Workflow* workflow_load(uint64_t workflow_id);
int workflow_delete(uint64_t workflow_id);
Workflow* workflow_list(size_t* count);
```

### Task Decomposer API

```c
// Decomposed task
typedef struct DecomposedTask {
    uint64_t task_id;
    char* description;
    AgentRole required_role;
    TaskStatus status;
    uint64_t* prerequisite_ids;
    size_t prerequisite_count;
    char* validation_criteria;
    int max_retries;
    int current_retry;
    char* result;
    time_t created_at;
    time_t completed_at;
} DecomposedTask;

// Task decomposition
DecomposedTask* task_decompose(const char* goal, AgentRole* roles, size_t role_count, size_t* out_count);
int task_resolve_dependencies(DecomposedTask* tasks, size_t task_count);
ExecutionPlan* task_create_execution_plan(DecomposedTask* tasks, size_t task_count);
int task_execute_parallel(DecomposedTask* tasks, size_t task_count, dispatch_group_t group);
```

### Group Chat API

```c
// Group chat modes
typedef enum {
    GROUP_CHAT_ROUND_ROBIN,   // Take turns in order
    GROUP_CHAT_PRIORITY,       // Priority-based speaking
    GROUP_CHAT_CONSENSUS,      // Build consensus
    GROUP_CHAT_DEBATE,         // Structured debate
} GroupChatMode;

// Group chat
typedef struct {
    uint64_t chat_id;
    SemanticID* participants;
    size_t participant_count;
    GroupChatMode mode;
    Message* message_history;
    size_t message_count;
    int current_round;
    int max_rounds;
    bool consensus_reached;
    double consensus_threshold;
    time_t created_at;
} GroupChat;

// Group chat functions
GroupChat* group_chat_create(SemanticID* participants, size_t count, GroupChatMode mode);
int group_chat_add_message(GroupChat* chat, SemanticID sender, const char* content);
SemanticID group_chat_get_next_speaker(GroupChat* chat);
bool group_chat_check_consensus(GroupChat* chat, double threshold);
char* group_chat_get_summary(GroupChat* chat);
void group_chat_destroy(GroupChat* chat);
```

### Pattern Library API

```c
// Pattern creation
Workflow* pattern_create_review_refine_loop(
    SemanticID generator_id,
    SemanticID critic_id,
    SemanticID refiner_id,
    int max_iterations
);

Workflow* pattern_create_parallel_analysis(
    SemanticID* analyst_ids,
    size_t analyst_count,
    SemanticID converger_id
);

Workflow* pattern_create_sequential_planning(
    SemanticID* planner_ids,
    size_t planner_count
);

Workflow* pattern_create_consensus_building(
    SemanticID* participant_ids,
    size_t participant_count,
    double consensus_threshold
);

// Pattern composition
Workflow* pattern_compose(Workflow* wf1, Workflow* wf2, const char* join_condition);
```

---

## Database Schema

### Workflows Table

```sql
CREATE TABLE workflows (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    entry_node_id INTEGER,
    status INTEGER NOT NULL DEFAULT 0,  -- WorkflowStatus enum
    current_node_id INTEGER,
    created_at INTEGER NOT NULL,
    updated_at INTEGER,
    last_checkpoint_at INTEGER,
    error_message TEXT,
    metadata_json TEXT
);

CREATE INDEX idx_workflows_status ON workflows(status);
CREATE INDEX idx_workflows_created ON workflows(created_at DESC);
```

### Workflow Nodes Table

```sql
CREATE TABLE workflow_nodes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflow_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    type INTEGER NOT NULL,  -- NodeType enum
    agent_id INTEGER,
    action_prompt TEXT,
    condition_expr TEXT,
    fallback_node_id INTEGER,
    node_data_json TEXT,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,
    FOREIGN KEY (fallback_node_id) REFERENCES workflow_nodes(id)
);

CREATE INDEX idx_nodes_workflow ON workflow_nodes(workflow_id);
```

### Workflow Edges Table

```sql
CREATE TABLE workflow_edges (
    from_node_id INTEGER NOT NULL,
    to_node_id INTEGER NOT NULL,
    condition_expr TEXT,
    priority INTEGER DEFAULT 0,
    PRIMARY KEY (from_node_id, to_node_id),
    FOREIGN KEY (from_node_id) REFERENCES workflow_nodes(id) ON DELETE CASCADE,
    FOREIGN KEY (to_node_id) REFERENCES workflow_nodes(id) ON DELETE CASCADE
);
```

### Workflow State Table

```sql
CREATE TABLE workflow_state (
    workflow_id INTEGER NOT NULL,
    key TEXT NOT NULL,
    value TEXT,
    updated_at INTEGER NOT NULL,
    PRIMARY KEY (workflow_id, key),
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE
);

CREATE INDEX idx_state_workflow ON workflow_state(workflow_id);
```

### Checkpoints Table

```sql
CREATE TABLE workflow_checkpoints (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflow_id INTEGER NOT NULL,
    node_id INTEGER NOT NULL,
    state_snapshot_json TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,
    FOREIGN KEY (node_id) REFERENCES workflow_nodes(id)
);

CREATE INDEX idx_checkpoints_workflow ON workflow_checkpoints(workflow_id);
CREATE INDEX idx_checkpoints_created ON workflow_checkpoints(created_at DESC);
```

### Migration Script

File: `src/memory/migrations/016_workflow_engine.sql`

```sql
-- Migration 016: Workflow Engine
-- Date: 2025-12-18

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

CREATE TABLE IF NOT EXISTS workflow_nodes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflow_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    type INTEGER NOT NULL,
    agent_id INTEGER,
    action_prompt TEXT,
    condition_expr TEXT,
    fallback_node_id INTEGER,
    node_data_json TEXT,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,
    FOREIGN KEY (fallback_node_id) REFERENCES workflow_nodes(id)
);

CREATE TABLE IF NOT EXISTS workflow_edges (
    from_node_id INTEGER NOT NULL,
    to_node_id INTEGER NOT NULL,
    condition_expr TEXT,
    priority INTEGER DEFAULT 0,
    PRIMARY KEY (from_node_id, to_node_id),
    FOREIGN KEY (from_node_id) REFERENCES workflow_nodes(id) ON DELETE CASCADE,
    FOREIGN KEY (to_node_id) REFERENCES workflow_nodes(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS workflow_state (
    workflow_id INTEGER NOT NULL,
    key TEXT NOT NULL,
    value TEXT,
    updated_at INTEGER NOT NULL,
    PRIMARY KEY (workflow_id, key),
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS workflow_checkpoints (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workflow_id INTEGER NOT NULL,
    node_id INTEGER NOT NULL,
    state_snapshot_json TEXT NOT NULL,
    created_at INTEGER NOT NULL,
    FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,
    FOREIGN KEY (node_id) REFERENCES workflow_nodes(id)
);

CREATE INDEX IF NOT EXISTS idx_workflows_status ON workflows(status);
CREATE INDEX IF NOT EXISTS idx_workflows_created ON workflows(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_nodes_workflow ON workflow_nodes(workflow_id);
CREATE INDEX IF NOT EXISTS idx_state_workflow ON workflow_state(workflow_id);
CREATE INDEX IF NOT EXISTS idx_checkpoints_workflow ON workflow_checkpoints(workflow_id);
CREATE INDEX IF NOT EXISTS idx_checkpoints_created ON workflow_checkpoints(created_at DESC);

COMMIT;
```

---

## Testing Strategy

### Testing Requirements (Following Convergio Standards)

All tests must follow Convergio's testing guidelines:
- **Unit Tests**: Required for all functions
- **Integration Tests**: Required for cross-component behavior
- **Fuzz Tests**: Required for security-critical paths
- **Sanitizer Tests**: Required for memory safety (Address Sanitizer, UBSan)
- **Coverage**: Minimum 80% coverage for new code

### Unit Tests

**File:** `tests/test_workflow.c`

#### Test Categories:

1. **Workflow Engine Tests**
   - Workflow creation and destruction
   - Node execution
   - State transitions
   - Error handling
   - Memory leak detection

2. **Checkpoint Tests**
   - Checkpoint creation
   - Checkpoint restoration
   - State serialization/deserialization
   - Corrupted checkpoint handling

3. **Task Decomposer Tests**
   - Task decomposition
   - Dependency resolution
   - Execution plan creation
   - Circular dependency detection

4. **Group Chat Tests**
   - Turn-taking logic
   - Consensus detection
   - Message threading
   - Timeout handling

5. **Pattern Tests**
   - Pattern creation
   - Pattern execution
   - Pattern composition
   - Pattern validation

6. **Security Tests** (MANDATORY)
   - SQL injection prevention
   - Buffer overflow prevention
   - Command injection prevention
   - Path traversal prevention
   - Input validation
   - Condition expression sanitization

### Integration Tests

**File:** `tests/test_workflow_integration.c`

- End-to-end workflow execution
- Multi-agent coordination
- Error recovery scenarios
- Performance tests
- Cost tracking integration
- Database transaction handling

### Fuzz Tests

**File:** `tests/fuzz_workflow.c`

**Required fuzz targets:**

```c
// Fuzz: Condition expression parser
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > MAX_CONDITION_EXPR_LEN) return 0;
    
    char* expr = calloc(size + 1, 1);
    memcpy(expr, data, size);
    expr[size] = '\0';
    
    ConditionExpr parsed;
    condition_parse_safe(expr, &parsed);
    
    free(expr);
    return 0;
}

// Fuzz: Workflow state values
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    Workflow* wf = workflow_create("fuzz", "fuzz", entry_node);
    
    char* key = "test_key";
    char* value = calloc(size + 1, 1);
    memcpy(value, data, size);
    value[size] = '\0';
    
    workflow_set_state(wf, key, value);
    
    free(value);
    workflow_destroy(wf);
    return 0;
}

// Fuzz: Checkpoint restoration
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Create valid checkpoint, then corrupt it
    Workflow* wf = workflow_create("fuzz", "fuzz", entry_node);
    uint64_t checkpoint_id = workflow_checkpoint(wf, "test");
    
    // Corrupt checkpoint data in database
    // ... corruption logic ...
    
    // Attempt restoration (should handle gracefully)
    int result = workflow_restore_from_checkpoint(wf, checkpoint_id);
    // Should return error, not crash
    
    workflow_destroy(wf);
    return 0;
}
```

**Fuzz Test Execution:**
```bash
# Build with fuzzing support
make fuzz_test_workflow

# Run fuzzer
./build/bin/fuzz_workflow -max_total_time=300
```

### Sanitizer Tests

**Required sanitizers:**
- **Address Sanitizer (ASan)**: Memory errors, use-after-free, buffer overflows
- **Undefined Behavior Sanitizer (UBSan)**: Integer overflow, null pointer dereference
- **Thread Sanitizer (TSan)**: Data races, deadlocks

**Build with sanitizers:**
```bash
make DEBUG=1 SANITIZE=address,undefined,thread
make test
```

**All tests must pass with sanitizers enabled.**

### Example Test Cases

```c
// Test: Simple linear workflow
void test_linear_workflow(void) {
    WorkflowNode* node1 = workflow_node_create("analyze", NODE_TYPE_ACTION);
    WorkflowNode* node2 = workflow_node_create("plan", NODE_TYPE_ACTION);
    workflow_node_add_edge(node1, node2, NULL);
    
    Workflow* wf = workflow_create("test", "Test workflow", node1);
    assert(wf != NULL);
    
    char* output = NULL;
    int result = workflow_execute(wf, "Test input", &output);
    
    assert(result == 0);
    assert(wf->status == WORKFLOW_STATUS_COMPLETED);
    assert(output != NULL);
    
    free(output);
    workflow_destroy(wf);
}

// Test: Checkpoint and resume
void test_checkpoint_resume(void) {
    Workflow* wf = workflow_create("test", "Test", entry_node);
    assert(wf != NULL);
    
    int result = workflow_execute(wf, "Input", NULL);
    assert(result == 0);
    
    uint64_t checkpoint_id = workflow_checkpoint(wf, "midpoint");
    assert(checkpoint_id > 0);
    
    // Simulate crash/restart
    uint64_t workflow_id = wf->workflow_id;
    workflow_destroy(wf);
    
    wf = workflow_load(workflow_id);
    assert(wf != NULL);
    
    result = workflow_restore_from_checkpoint(wf, checkpoint_id);
    assert(result == 0);
    assert(wf->current_node_id > 0);
    
    char* output = NULL;
    result = workflow_execute(wf, NULL, &output);
    assert(result == 0);
    assert(wf->status == WORKFLOW_STATUS_COMPLETED);
    
    free(output);
    workflow_destroy(wf);
}

// Test: Error handling (no silent failures)
void test_error_handling(void) {
    // Test NULL pointer handling
    int result = workflow_execute(NULL, "input", NULL);
    assert(result != 0);  // Should return error, not crash
    
    // Test invalid workflow state
    Workflow* wf = workflow_create("test", "test", entry_node);
    wf->status = WORKFLOW_STATUS_FAILED;
    
    result = workflow_execute(wf, "input", NULL);
    assert(result != 0);  // Should reject invalid state
    
    workflow_destroy(wf);
}

// Test: Memory leak detection
void test_memory_leaks(void) {
    // Run workflow multiple times, check for leaks
    for (int i = 0; i < 100; i++) {
        Workflow* wf = workflow_create("test", "test", entry_node);
        workflow_execute(wf, "input", NULL);
        workflow_destroy(wf);
    }
    
    // Address Sanitizer will detect leaks if present
}

// Test: Thread safety
void* test_thread_worker(void* arg) {
    for (int i = 0; i < 100; i++) {
        Workflow* wf = workflow_create("test", "test", entry_node);
        workflow_register(wf);
        workflow_get(wf->workflow_id);
        workflow_destroy(wf);
    }
    return NULL;
}

void test_thread_safety(void) {
    pthread_t threads[10];
    
    // Create multiple threads accessing workflow registry
    for (int i = 0; i < 10; i++) {
        pthread_create(&threads[i], NULL, test_thread_worker, NULL);
    }
    
    // Wait for all threads
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Thread Sanitizer will detect races if present
}

// Test: SQL injection prevention
void test_sql_injection_prevention(void) {
    Workflow* wf = workflow_create("test", "test", entry_node);
    
    // Attempt SQL injection
    char malicious_name[] = "test'; DROP TABLE workflows; --";
    int result = workflow_set_name(wf, malicious_name);
    
    // Should sanitize and prevent injection
    assert(result == 0);
    assert(strstr(wf->name, "DROP") == NULL);  // SQL removed
    
    // Verify table still exists
    assert(workflow_table_exists());
    
    workflow_destroy(wf);
}

// Test: Buffer overflow prevention
void test_buffer_overflow_prevention(void) {
    char large_input[10000];
    memset(large_input, 'A', sizeof(large_input) - 1);
    large_input[sizeof(large_input) - 1] = '\0';
    
    // Should handle gracefully (truncate or reject)
    Workflow* wf = workflow_create(large_input, "test", entry_node);
    
    if (wf) {
        // If created, name should be truncated
        assert(strlen(wf->name) <= MAX_WORKFLOW_NAME_LEN);
        workflow_destroy(wf);
    } else {
        // Or creation should fail gracefully
        assert(true);  // Acceptable behavior
    }
}
```

### Test Execution

**Run all tests:**
```bash
# Unit tests
make test

# Integration tests
make integration_test

# Fuzz tests
make fuzz_test_workflow

# Sanitizer tests
make DEBUG=1 SANITIZE=address,undefined test

# Coverage report
make coverage
```

### Test Coverage Requirements

- **Minimum coverage**: 80% for new code
- **Critical paths**: 100% coverage required
- **Security functions**: 100% coverage required
- **Error handling paths**: 100% coverage required

### Continuous Integration

**CI Pipeline Requirements:**
- All unit tests must pass
- All integration tests must pass
- All sanitizer tests must pass
- Coverage must meet minimum threshold
- Static analysis must pass
- No memory leaks detected

---

## Migration Path

### Phase 1: Additive Changes

**Strategy:** Add new workflow system alongside existing orchestrator

1. Implement workflow engine
2. Keep existing orchestrator unchanged
3. Add CLI commands for workflow management
4. Allow workflows to call existing orchestrator functions

### Phase 2: Gradual Migration

**Strategy:** Migrate existing patterns to workflows

1. Convert parallel execution to workflow pattern
2. Convert sequential pipelines to workflow
3. Convert critic loops to workflow
4. Keep backward compatibility

### Phase 3: Full Integration

**Strategy:** Make workflows the primary orchestration method

1. Update orchestrator to use workflows internally
2. Deprecate old hardcoded patterns
3. Provide migration tools
4. Update documentation

### Backward Compatibility

- Existing CLI commands continue to work
- Old execution plans still supported
- Gradual deprecation with warnings

---

## Performance Considerations

### Optimization Strategies

1. **Lazy State Loading**
   - Load workflow state on-demand
   - Cache frequently accessed state
   - Use SQLite WAL mode for concurrency

2. **Checkpoint Optimization**
   - Incremental checkpoints (only changed state)
   - Compress checkpoint data
   - Background checkpoint cleanup

3. **Memory Management**
   - Pool workflow objects
   - Reuse state structures
   - Efficient string handling

4. **GCD Integration**
   - Workflow execution on background queues
   - Parallel node execution where possible
   - Non-blocking checkpoint creation

### Performance Targets

| Operation | Target | Measurement |
|-----------|--------|-------------|
| Workflow creation | < 10ms | Time to create workflow |
| Node execution | < 100ms | Time per node (excluding agent call) |
| Checkpoint creation | < 50ms | Time to create checkpoint |
| Checkpoint restore | < 100ms | Time to restore from checkpoint |
| State update | < 1ms | Time to update state value |

---

## Security Considerations

### Security Architecture (Following Convergio Standards)

This implementation follows Convergio's rigorous security standards as documented in `SECURITY_AUDIT.md` and enforced across the codebase.

#### 1. SQL Injection Protection

**CRITICAL:** All SQL queries MUST use parameterized statements with bound parameters.

```c
// ✅ CORRECT: Parameterized query
int workflow_save(Workflow* wf) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO workflows (name, description, status, created_at) "
                      "VALUES (?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    // Bind parameters (prevents SQL injection)
    sqlite3_bind_text(stmt, 1, wf->name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, wf->description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, wf->status);
    sqlite3_bind_int64(stmt, 4, time(NULL));
    
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (result == SQLITE_DONE) ? 0 : -1;
}

// ❌ WRONG: String concatenation (vulnerable to SQL injection)
// NEVER DO THIS:
// sprintf(sql, "INSERT INTO workflows (name) VALUES ('%s')", wf->name);
```

**Enforcement:**
- All database operations in `src/workflow/workflow_persistence.c`
- Use `SQLITE_TRANSIENT` for all text bindings (prevents use-after-free)
- Code review checklist: verify no `sprintf`/`snprintf` in SQL queries
- Static analysis tooling to detect SQL injection patterns

#### 2. Input Validation & Sanitization

**Condition Expression Parser Security**

Condition expressions (e.g., `state.value == "approved"`) must be parsed safely:

```c
// Safe condition expression parser
typedef struct {
    char* key;
    char* operator;  // "==", "!=", ">", "<", "contains"
    char* value;
} ConditionExpr;

// Validate and parse condition expression
int condition_parse_safe(const char* expr, ConditionExpr* out) {
    if (!expr || !out) return -1;
    
    // Length limit (prevent buffer overflow)
    if (strlen(expr) > MAX_CONDITION_EXPR_LEN) {
        LOG_ERROR("Condition expression too long");
        return -1;
    }
    
    // Whitelist allowed operators
    const char* ALLOWED_OPS[] = {"==", "!=", ">", "<", ">=", "<=", "contains", "startsWith"};
    
    // Parse with bounds checking
    // ... safe parsing logic ...
    
    // Validate key name (alphanumeric + underscore only)
    if (!is_valid_identifier(out->key)) {
        LOG_ERROR("Invalid key name in condition");
        return -1;
    }
    
    return 0;
}

// Validate identifier (prevent injection)
static bool is_valid_identifier(const char* str) {
    if (!str || str[0] == '\0') return false;
    
    // Must start with letter or underscore
    if (!isalpha(str[0]) && str[0] != '_') return false;
    
    // Rest must be alphanumeric or underscore
    for (size_t i = 1; i < strlen(str); i++) {
        if (!isalnum(str[i]) && str[i] != '_') return false;
    }
    
    return true;
}
```

**State Value Sanitization**

```c
// Sanitize state values before storage
char* workflow_sanitize_state_value(const char* value) {
    if (!value) return NULL;
    
    // Length limit
    size_t len = strlen(value);
    if (len > MAX_STATE_VALUE_LEN) {
        LOG_WARN("State value truncated (exceeds MAX_STATE_VALUE_LEN)");
        len = MAX_STATE_VALUE_LEN;
    }
    
    // Allocate with bounds
    char* sanitized = calloc(len + 1, 1);
    if (!sanitized) return NULL;
    
    // Copy with bounds checking
    strncpy(sanitized, value, len);
    sanitized[len] = '\0';
    
    return sanitized;
}
```

#### 3. Buffer Overflow Protection

**All string operations MUST use bounds checking:**

```c
// ✅ CORRECT: Bounds-checked string operations
int workflow_set_state(Workflow* wf, const char* key, const char* value) {
    if (!wf || !key || !value) return -1;
    
    // Validate key length
    size_t key_len = strlen(key);
    if (key_len == 0 || key_len > MAX_STATE_KEY_LEN) {
        LOG_ERROR("Invalid state key length");
        return -1;
    }
    
    // Sanitize and validate value
    char* sanitized = workflow_sanitize_state_value(value);
    if (!sanitized) return -1;
    
    // Use safe string functions
    // ... implementation with bounds checking ...
    
    free(sanitized);
    return 0;
}

// ❌ WRONG: Unsafe string operations
// NEVER: strcpy(dest, src);  // No bounds checking
// NEVER: sprintf(buffer, "%s", value);  // No size limit
```

**Enforcement:**
- Use `strncpy` with explicit length limits
- Use `snprintf` with buffer size
- Allocate buffers dynamically with size checks
- Static analysis to detect unsafe string functions

#### 4. Command Injection Protection

**If workflows execute commands (via tools), sanitize all inputs:**

```c
// Workflow node execution with command sanitization
int workflow_execute_node(Workflow* wf, WorkflowNode* node, char** output) {
    if (node->type == NODE_TYPE_ACTION && node->action_prompt) {
        // If action involves tool execution, sanitize
        char* sanitized_prompt = tools_sanitize_prompt(node->action_prompt);
        if (!sanitized_prompt) {
            LOG_ERROR("Failed to sanitize action prompt");
            return -1;
        }
        
        // Execute with sanitized input
        // ... execution logic ...
        
        free(sanitized_prompt);
    }
    return 0;
}
```

**Reference:** Use existing `tools.c` sanitization functions:
- `tools_is_command_safe()` - Command blocklist checking
- `tools_sanitize_grep_pattern()` - Pattern sanitization
- `tools_normalize_command()` - Command normalization

#### 5. Path Traversal Protection

**If workflows access files, validate paths:**

```c
// Workflow file access with path validation
int workflow_read_file(Workflow* wf, const char* filepath, char** content) {
    if (!filepath) return -1;
    
    // Use existing path validation
    if (!tools_is_path_safe(filepath)) {
        LOG_ERROR("Unsafe file path: %s", filepath);
        return -1;
    }
    
    // Use safe file operations (TOCTOU protection)
    int fd = safe_open_read(filepath);
    if (fd < 0) return -1;
    
    // ... read with bounds checking ...
    
    close(fd);
    return 0;
}
```

**Reference:** Use existing `tools.c` path validation:
- `tools_is_path_safe()` - Path safety checking
- `safe_open_read()` / `safe_open_write()` - TOCTOU-safe file operations

#### 6. Thread Safety

**All global state MUST be protected with mutexes:**

```c
// Global workflow registry (thread-safe)
static Workflow** g_workflow_registry = NULL;
static size_t g_workflow_count = 0;
static size_t g_workflow_capacity = 0;
static pthread_mutex_t g_workflow_mutex = PTHREAD_MUTEX_INITIALIZER;

int workflow_register(Workflow* wf) {
    if (!wf) return -1;
    
    pthread_mutex_lock(&g_workflow_mutex);
    
    // Resize if needed
    if (g_workflow_count >= g_workflow_capacity) {
        // ... resize logic with error handling ...
    }
    
    g_workflow_registry[g_workflow_count++] = wf;
    
    pthread_mutex_unlock(&g_workflow_mutex);
    return 0;
}

Workflow* workflow_get(uint64_t workflow_id) {
    pthread_mutex_lock(&g_workflow_mutex);
    
    Workflow* result = NULL;
    for (size_t i = 0; i < g_workflow_count; i++) {
        if (g_workflow_registry[i]->workflow_id == workflow_id) {
            result = g_workflow_registry[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&g_workflow_mutex);
    return result;
}
```

**Enforcement:**
- All global state access protected by mutex
- No shared state without synchronization
- Deadlock prevention (consistent lock ordering)

#### 7. Memory Management

**Follow Convergio memory management standards:**

```c
// ✅ CORRECT: Proper memory management
Workflow* workflow_create(const char* name, const char* description, WorkflowNode* entry_node) {
    if (!name || !entry_node) return NULL;
    
    Workflow* wf = calloc(1, sizeof(Workflow));
    if (!wf) {
        LOG_ERROR("Failed to allocate Workflow");
        return NULL;
    }
    
    // Allocate strings with bounds checking
    size_t name_len = strlen(name);
    if (name_len > MAX_WORKFLOW_NAME_LEN) {
        LOG_ERROR("Workflow name too long");
        free(wf);
        return NULL;
    }
    
    wf->name = strdup(name);
    if (!wf->name) {
        LOG_ERROR("Failed to allocate workflow name");
        free(wf);
        return NULL;
    }
    
    // ... more initialization ...
    
    return wf;
}

void workflow_destroy(Workflow* wf) {
    if (!wf) return;
    
    // Free all allocated strings
    free(wf->name);
    free(wf->description);
    free(wf->error_message);
    
    // Free state
    if (wf->state) {
        workflow_state_destroy(wf->state);
    }
    
    // Free nodes (recursive cleanup)
    if (wf->entry_node) {
        workflow_node_destroy(wf->entry_node);
    }
    
    // NULL after free (prevent double-free)
    memset(wf, 0, sizeof(Workflow));
    free(wf);
}
```

**Enforcement:**
- All allocations checked for NULL
- All frees followed by NULL assignment (prevent double-free)
- No use-after-free (use `SQLITE_TRANSIENT` for SQLite bindings)
- Memory leak detection with Address Sanitizer

#### 8. Error Handling

**Follow Convergio error handling standards (no silent failures):**

```c
// ✅ CORRECT: Proper error handling
int workflow_execute(Workflow* wf, const char* input, char** output) {
    if (!wf || !input) {
        LOG_ERROR("Invalid arguments to workflow_execute");
        return -1;
    }
    
    // Check workflow state
    if (wf->status != WORKFLOW_STATUS_PENDING && 
        wf->status != WORKFLOW_STATUS_PAUSED) {
        LOG_ERROR("Workflow not in executable state: %d", wf->status);
        return -1;
    }
    
    // Execute with error propagation
    int result = workflow_execute_internal(wf, input, output);
    if (result != 0) {
        // Log error with context
        LOG_ERROR("Workflow execution failed: %s", wf->error_message ? wf->error_message : "Unknown error");
        return result;
    }
    
    return 0;
}

// ❌ WRONG: Silent error discarding
// NEVER: let _ = workflow_execute(...);  // Error ignored
// NEVER: if (result) { /* ignore */ }  // Silent failure
```

**Enforcement:**
- All errors logged with context
- No `let _ =` on fallible operations
- Error propagation with `?` or explicit handling
- User-visible error messages for UI layer

#### 9. Workflow Validation

**Validate workflow structure before execution:**

```c
// Validate workflow graph structure
int workflow_validate(Workflow* wf) {
    if (!wf) return -1;
    
    // Check for cycles (prevent infinite loops)
    if (workflow_has_cycles(wf)) {
        LOG_ERROR("Workflow contains cycles");
        return -1;
    }
    
    // Verify all agent IDs exist
    if (!workflow_verify_agents(wf)) {
        LOG_ERROR("Workflow references invalid agent IDs");
        return -1;
    }
    
    // Check node connectivity
    if (!workflow_check_connectivity(wf)) {
        LOG_ERROR("Workflow has disconnected nodes");
        return -1;
    }
    
    // Validate condition expressions
    if (!workflow_validate_conditions(wf)) {
        LOG_ERROR("Workflow has invalid condition expressions");
        return -1;
    }
    
    return 0;
}

// Cycle detection (DFS)
static bool workflow_has_cycles(Workflow* wf) {
    // Implement DFS cycle detection
    // ... implementation ...
    return false;
}
```

#### 10. Access Control & Authorization

**User-specific workflows and permissions:**

```c
// Workflow access control
typedef struct {
    uint64_t workflow_id;
    SemanticID owner_id;  // User who created workflow
    SemanticID* shared_with;  // Users with access
    size_t shared_count;
    WorkflowPermission permissions;  // READ, WRITE, EXECUTE
} WorkflowAccess;

int workflow_check_access(Workflow* wf, SemanticID user_id, WorkflowPermission required) {
    if (!wf) return -1;
    
    // Owner has full access
    if (wf->owner_id == user_id) return 0;
    
    // Check shared access
    // ... permission checking logic ...
    
    return -1;  // Access denied
}
```

#### 11. Cost Tracking Integration

**Integrate with existing cost management (ADR-003):**

```c
// Workflow execution with cost tracking
int workflow_execute_with_cost_tracking(Workflow* wf, const char* input, char** output) {
    // Check budget before execution
    if (!cost_check_budget()) {
        LOG_WARN("Budget exceeded, workflow execution blocked");
        return -1;
    }
    
    // Estimate cost
    double estimated_cost = workflow_estimate_cost(wf);
    double remaining = cost_get_remaining_budget();
    
    if (estimated_cost > remaining) {
        LOG_WARN("Estimated cost (%.2f) exceeds remaining budget (%.2f)", 
                 estimated_cost, remaining);
        // Ask for confirmation or abort
        return -1;
    }
    
    // Execute workflow
    int result = workflow_execute(wf, input, output);
    
    // Record actual cost
    if (result == 0) {
        cost_record_workflow_execution(wf, estimated_cost);
    }
    
    return result;
}
```

#### 12. Security Testing Requirements

**Mandatory security tests:**

```c
// Security test: SQL injection prevention
void test_sql_injection_prevention(void) {
    Workflow* wf = workflow_create("test", "test", entry_node);
    
    // Attempt SQL injection in workflow name
    char malicious_name[] = "test'; DROP TABLE workflows; --";
    
    // Should sanitize and prevent injection
    int result = workflow_set_name(wf, malicious_name);
    assert(result == 0);  // Should succeed (sanitized)
    
    // Verify table still exists
    assert(workflow_table_exists());
    
    workflow_destroy(wf);
}

// Security test: Buffer overflow prevention
void test_buffer_overflow_prevention(void) {
    char large_input[10000];
    memset(large_input, 'A', sizeof(large_input) - 1);
    large_input[sizeof(large_input) - 1] = '\0';
    
    // Should handle gracefully (truncate or reject)
    Workflow* wf = workflow_create(large_input, "test", entry_node);
    assert(wf == NULL || strlen(wf->name) <= MAX_WORKFLOW_NAME_LEN);
    
    if (wf) workflow_destroy(wf);
}

// Security test: Command injection prevention
void test_command_injection_prevention(void) {
    // Attempt command injection in condition expression
    char malicious_condition[] = "state.value == 'test' && system('rm -rf /')";
    
    ConditionExpr expr;
    int result = condition_parse_safe(malicious_condition, &expr);
    
    // Should reject or sanitize
    assert(result != 0 || expr.operator == NULL);
}
```

**Fuzz Testing Requirements:**
- Fuzz condition expression parser
- Fuzz workflow state values
- Fuzz checkpoint restoration
- Use Address Sanitizer and Undefined Behavior Sanitizer

#### 13. Security Checklist

**Pre-commit checklist:**

- [ ] All SQL queries use parameterized statements
- [ ] All string operations have bounds checking
- [ ] All global state protected by mutex
- [ ] All file operations use safe_open functions
- [ ] All command execution uses sanitization
- [ ] All errors are logged (no silent failures)
- [ ] All memory allocations checked for NULL
- [ ] All frees followed by NULL assignment
- [ ] Workflow validation before execution
- [ ] Cost tracking integrated
- [ ] Security tests pass
- [ ] Fuzz tests pass
- [ ] Address Sanitizer clean
- [ ] Static analysis clean

#### 14. Security Audit Integration

**This implementation must pass Convergio's security audit:**

- Reference: `SECURITY_AUDIT.md`
- All CRITICAL and HIGH severity issues must be addressed
- Code review by Luca (Security Expert) agent
- Security validation by Guardian (AI Security Validator) agent

**Known Security Patterns to Follow:**
- `tools.c:409-486` - Path safety implementation
- `tools.c:489-590` - Command safety implementation
- `tools.c:105-134` - TOCTOU prevention
- `oauth.m` - Constant-time comparison
- `persistence.c` - SQLite parameter binding

---

## Future Enhancements

### Potential Additions (Post-MVP)

1. **Visual Workflow Builder**
   - GUI for creating workflows
   - Drag-and-drop node editor
   - Workflow preview

2. **Workflow Marketplace**
   - Share workflows between users
   - Community-contributed patterns
   - Workflow ratings and reviews

3. **Advanced Patterns**
   - Machine learning-based routing
   - Adaptive workflows
   - Workflow optimization

4. **Monitoring & Analytics**
   - Workflow performance metrics
   - Bottleneck detection
   - Cost analysis per workflow

---

## Code Quality Standards

### Convergio Coding Guidelines Compliance

This implementation MUST follow all Convergio coding guidelines:

#### 1. Error Handling

**NEVER silently discard errors:**
```c
// ❌ WRONG: Silent error discarding
let _ = workflow_execute(wf, input, &output);

// ✅ CORRECT: Explicit error handling
int result = workflow_execute(wf, input, &output);
if (result != 0) {
    LOG_ERROR("Workflow execution failed: %s", wf->error_message);
    return result;
}
```

**Error Propagation:**
- Use return codes for error propagation
- Log all errors with context
- Never use `let _ =` on fallible operations
- All errors must be visible to UI layer

#### 2. Memory Management

**All allocations checked:**
```c
// ✅ CORRECT: NULL check after allocation
Workflow* wf = calloc(1, sizeof(Workflow));
if (!wf) {
    LOG_ERROR("Failed to allocate Workflow");
    return NULL;
}
```

**No use-after-free:**
```c
// ✅ CORRECT: NULL after free
free(wf->name);
wf->name = NULL;  // Prevent use-after-free
```

**No double-free:**
```c
// ✅ CORRECT: Check before free
if (wf) {
    free(wf->name);
    wf->name = NULL;
    free(wf);
    wf = NULL;  // Prevent double-free
}
```

#### 3. Bounds Checking

**All array/string operations:**
```c
// ✅ CORRECT: Bounds checking
if (index >= array_size) {
    LOG_ERROR("Index out of bounds: %zu >= %zu", index, array_size);
    return -1;
}
```

**No unsafe string functions:**
```c
// ❌ WRONG: Unsafe
strcpy(dest, src);

// ✅ CORRECT: Safe
strncpy(dest, src, dest_size - 1);
dest[dest_size - 1] = '\0';
```

#### 4. Thread Safety

**All global state protected:**
```c
// ✅ CORRECT: Mutex protection
pthread_mutex_lock(&g_workflow_mutex);
// ... access global state ...
pthread_mutex_unlock(&g_workflow_mutex);
```

**Consistent lock ordering** (prevent deadlocks):
- Always acquire locks in same order
- Document lock ordering in comments

#### 5. No Panic Functions

**Avoid functions that can panic:**
```c
// ❌ WRONG: Can panic
array[index] = value;

// ✅ CORRECT: Bounds check first
if (index >= array_size) return -1;
array[index] = value;
```

#### 6. Code Comments

**Comments explain "why", not "what":**
```c
// ✅ CORRECT: Explains why
// Use SQLITE_TRANSIENT to prevent use-after-free when SQLite
// frees the string after binding
sqlite3_bind_text(stmt, 1, value, -1, SQLITE_TRANSIENT);

// ❌ WRONG: Explains what (obvious from code)
// Bind the value to the first parameter
sqlite3_bind_text(stmt, 1, value, -1, SQLITE_TRANSIENT);
```

#### 7. Variable Naming

**Full words, no abbreviations:**
```c
// ✅ CORRECT: Full words
Workflow* workflow = workflow_create(...);
size_t workflow_count = 0;

// ❌ WRONG: Abbreviations
Workflow* wf = workflow_create(...);
size_t wc = 0;
```

### Static Analysis Requirements

**All code must pass:**
- `clang-tidy` with Convergio configuration
- `clang static analyzer`
- `cppcheck` (if applicable)

**Build with warnings:**
```bash
make DEBUG=1 WARNINGS=all
```

**No warnings allowed** in new code.

### Code Review Checklist

**Before submitting PR:**
- [ ] All tests pass (unit, integration, fuzz, sanitizer)
- [ ] Code coverage >= 80%
- [ ] No memory leaks (Address Sanitizer clean)
- [ ] No data races (Thread Sanitizer clean)
- [ ] No undefined behavior (UBSan clean)
- [ ] Static analysis clean
- [ ] Security checklist complete
- [ ] Error handling complete (no silent failures)
- [ ] Memory management correct (no leaks, no use-after-free)
- [ ] Thread safety verified
- [ ] Bounds checking on all array/string operations
- [ ] SQL queries parameterized
- [ ] Input validation complete
- [ ] Cost tracking integrated
- [ ] Documentation updated

---

## Implementation Compliance Matrix

| Requirement | Standard | Implementation | Status |
|-------------|----------|----------------|--------|
| **Security** | | | |
| SQL Injection Prevention | Parameterized queries only | All SQL uses `sqlite3_bind_*` | ✅ Required |
| Buffer Overflow Protection | Bounds checking on all operations | `strncpy`, `snprintf`, size limits | ✅ Required |
| Command Injection Prevention | Input sanitization | Use `tools_sanitize_*` functions | ✅ Required |
| Path Traversal Protection | Path validation | Use `tools_is_path_safe()` | ✅ Required |
| Thread Safety | Mutex protection | All global state protected | ✅ Required |
| **Error Handling** | | | |
| No Silent Failures | All errors logged | `LOG_ERROR` on all failures | ✅ Required |
| Error Propagation | Return codes | All functions return error codes | ✅ Required |
| User-Visible Errors | Error messages to UI | Errors propagated to UI layer | ✅ Required |
| **Memory Management** | | | |
| NULL Checks | All allocations checked | `if (!ptr) return NULL;` | ✅ Required |
| No Use-After-Free | NULL after free | `free(ptr); ptr = NULL;` | ✅ Required |
| No Double-Free | Check before free | `if (ptr) { free(ptr); ptr = NULL; }` | ✅ Required |
| **Testing** | | | |
| Unit Tests | All functions tested | `tests/test_workflow.c` | ✅ Required |
| Integration Tests | Cross-component tests | `tests/test_workflow_integration.c` | ✅ Required |
| Fuzz Tests | Security-critical paths | `tests/fuzz_workflow.c` | ✅ Required |
| Sanitizer Tests | Memory safety | ASan, UBSan, TSan | ✅ Required |
| Coverage | >= 80% minimum | Coverage report required | ✅ Required |
| **Code Quality** | | | |
| Static Analysis | clang-tidy, clang analyzer | No warnings allowed | ✅ Required |
| Code Review | Security checklist | All items verified | ✅ Required |
| Documentation | ADR, API docs | Complete documentation | ✅ Required |
| **Cost Management** | | | |
| Budget Checking | Before execution | `cost_check_budget()` | ✅ Required |
| Cost Tracking | Per workflow | `cost_record_workflow_execution()` | ✅ Required |
| Cost Estimation | Pre-execution | `workflow_estimate_cost()` | ✅ Required |

---

## Conclusion

This specification provides a complete roadmap for implementing advanced workflow orchestration in Convergio while maintaining its native C/Swift architecture and Apple Silicon optimizations.

**Key Benefits:**
- More sophisticated multi-agent coordination
- Workflow persistence and recovery
- Reusable workflow patterns
- Better debugging and observability
- Zero Python dependencies

**Compliance:**
- ✅ Full security standards compliance (SECURITY_AUDIT.md)
- ✅ Error handling standards (no silent failures)
- ✅ Memory management standards (no leaks, no use-after-free)
- ✅ Testing standards (unit, integration, fuzz, sanitizer)
- ✅ Code quality standards (static analysis, code review)
- ✅ Cost management integration (ADR-003)
- ✅ Thread safety standards (mutex protection)
- ✅ Input validation standards (sanitization, bounds checking)

**Key Benefits:**
- More sophisticated multi-agent coordination
- Workflow persistence and recovery
- Reusable workflow patterns
- Better debugging and observability
- Zero Python dependencies

**Next Steps:**
1. Review and approve this specification
2. Create implementation branch
3. Begin Phase 1 implementation
4. Regular progress reviews

---

**Document Status:** Ready for Implementation  
**Last Updated:** 2025-12-18  
**Version:** 1.0

