# ADR-018: Workflow Orchestration System

**Status**: Accepted  
**Date**: 2025-12-20  
**Deciders**: AI Team  
**Context**: Convergio needs a state machine-based workflow orchestration system for complex multi-agent coordination

---

## Context

Convergio's orchestrator provides basic agent delegation and coordination, but lacks:
- State machine-based execution with checkpointing
- Complex conditional routing based on execution state
- Task decomposition for breaking down large tasks
- Multi-agent group discussions with consensus building
- Workflow persistence and recovery
- Reusable workflow patterns

Users need to coordinate multiple agents for complex tasks (code review, product launch, security audits) with:
- Ability to pause and resume workflows
- Conditional routing based on results
- Task breakdown and parallel execution
- Consensus building among agents
- Error recovery and retry logic

## Decision

We implement a comprehensive workflow orchestration system with:

1. **State Machine Engine**: Core workflow execution with state transitions
2. **Checkpointing**: Workflow persistence and recovery at any point
3. **Task Decomposition**: LLM-based task breakdown with dependency resolution
4. **Group Chat**: Multi-agent discussions with consensus detection
5. **Conditional Routing**: State-based dynamic workflow routing
6. **Pattern Library**: Reusable workflow patterns (review-refine, parallel analysis, etc.)
7. **Error Handling**: Comprehensive error recovery with retry and fallback
8. **Observability**: Full integration with logging, telemetry, and security

**Architecture**:
- Pure C implementation (no Python dependencies)
- SQLite for persistence (existing Convergio database)
- Integration with existing orchestrator (no breaking changes)
- Backward compatible (existing orchestrator still works)

**Inspired By**:
- LangGraph (conditional routing, state machines)
- CrewAI (task decomposition, role-based execution)
- AutoGen (group chat, consensus building)

## Consequences

### Positive

1. **Enhanced Multi-Agent Coordination**
   - State machine enables complex multi-step workflows
   - Conditional routing allows dynamic adaptation
   - Workflow templates provide reusable patterns

2. **Workflow Persistence & Recovery**
   - Checkpointing enables resume from any point
   - State persistence survives crashes
   - Workflow history for debugging

3. **Advanced Task Decomposition**
   - Hierarchical task breakdown
   - Automatic dependency resolution
   - Parallel execution of independent tasks

4. **Group Chat & Consensus**
   - Multi-agent discussions
   - Consensus detection
   - Iterative refinement loops

5. **Improved Error Handling**
   - Retry logic with state machine support
   - Fallback strategies
   - Error state handling

6. **Enhanced Observability**
   - Workflow visualization (Mermaid)
   - Execution history
   - Performance metrics

7. **Cost Optimization**
   - Cost estimation before execution
   - Budget checking integrated
   - Per-workflow cost tracking

### Negative

1. **Complexity**
   - Additional codebase (~10,000 lines)
   - More state to manage
   - Learning curve for users

2. **Performance**
   - Checkpointing overhead
   - State serialization cost
   - Database I/O for persistence

3. **Maintenance**
   - More code to maintain
   - More test cases
   - More documentation

### Mitigation

1. **Complexity**: Comprehensive documentation, examples, templates
2. **Performance**: Incremental checkpoints (future), binary serialization (future)
3. **Maintenance**: Extensive test coverage, clear architecture, ADRs

## Implementation

### Phase 1: Foundation
- Database schema migration (016_workflow_engine.sql)
- Workflow data structures (workflow.h, workflow_types.c)
- Basic state machine (workflow_engine.c)
- Checkpoint manager (checkpoint.c)

### Phase 2: Task Decomposition
- Task decomposer (task_decomposer.c)
- Dependency resolution
- Template library

### Phase 3: Group Chat
- Group chat manager (group_chat.c)
- Consensus detection
- Turn-taking logic

### Phase 4: Conditional Routing
- Conditional router (router.c)
- Pattern library (patterns.c)
- Mermaid visualization (workflow_visualization.c)

### Phase 5: Integration
- CLI commands (workflow.c)
- Error handling (error_handling.c)
- Observability (workflow_observability.c)
- Security validation
- Complete test suite

## Alternatives Considered

### Alternative 1: Python-based Workflow Engine
**Rejected**: Would require Python runtime, breaking "zero Python dependencies" principle

### Alternative 2: External Workflow Engine (Temporal, Airflow)
**Rejected**: Would require external dependencies, breaking self-contained architecture

### Alternative 3: Simple Sequential Execution
**Rejected**: Doesn't support conditional routing, checkpointing, or complex patterns

### Alternative 4: Extend Existing Orchestrator
**Rejected**: Would break backward compatibility, orchestrator is too tightly coupled

## References

- [Architecture Documentation](workflow-orchestration/architecture.md)
- [Technical Documentation](workflow-orchestration/TECHNICAL_DOCUMENTATION.md)
- [User Guide](workflow-orchestration/USER_GUIDE.md)
- [Master Plan](workflow-orchestration/MASTER_PLAN.md)
- [Compatibility Analysis](workflow-orchestration/COMPATIBILITY_ANALYSIS.md)

---

**Last Updated**: 2025-12-20  
**Related ADRs**: 
- ADR-001: Persistence Layer (SQLite)
- ADR-002: Agent Execution Model
- ADR-006: Multi-Provider Architecture

