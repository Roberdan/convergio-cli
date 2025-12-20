# Workflow Orchestration - Technical Documentation

## Overview

The workflow orchestration system provides a state machine-based engine for coordinating multi-agent workflows. It supports checkpointing, conditional routing, task decomposition, group chat, and comprehensive error handling.

## Architecture

### Core Components

1. **Workflow Engine** (`src/workflow/workflow_engine.c`)
   - State machine execution
   - Node execution (ACTION, DECISION, HUMAN_INPUT, etc.)
   - Workflow lifecycle management

2. **Workflow Types** (`src/workflow/workflow_types.c`)
   - Data structures (Workflow, WorkflowNode, WorkflowState)
   - Memory management
   - State operations

3. **Checkpoint Manager** (`src/workflow/checkpoint.c`)
   - Workflow persistence
   - Checkpoint creation and restoration
   - Database integration

4. **Task Decomposer** (`src/workflow/task_decomposer.c`)
   - Task decomposition via LLM
   - Dependency resolution
   - Execution planning

5. **Group Chat** (`src/workflow/group_chat.c`)
   - Multi-agent coordination
   - Turn-taking logic
   - Consensus detection

6. **Router** (`src/workflow/router.c`)
   - Conditional routing
   - Condition expression evaluation
   - State-based decisions

7. **Patterns** (`src/workflow/patterns.c`)
   - Workflow pattern library
   - Common patterns (review-refine, parallel analysis, etc.)

8. **Error Handling** (`src/workflow/error_handling.c`)
   - Comprehensive error handling
   - Timeout management
   - Network/LLM/file I/O error handling
   - Recovery strategies

9. **Observability** (`src/workflow/workflow_observability.c`)
   - Logging integration
   - Telemetry integration
   - Security validation
   - Audit trail

## Error Handling

### Error Types

- `WORKFLOW_ERROR_TIMEOUT`: Node execution exceeded timeout
- `WORKFLOW_ERROR_NETWORK`: Network connectivity issues
- `WORKFLOW_ERROR_FILE_IO`: File read/write errors
- `WORKFLOW_ERROR_CREDIT_EXHAUSTED`: API budget limit reached
- `WORKFLOW_ERROR_LLM_DOWN`: LLM provider unavailable
- `WORKFLOW_ERROR_TOOL_FAILED`: Tool execution failure
- `WORKFLOW_ERROR_AGENT_NOT_FOUND`: Agent not found
- `WORKFLOW_ERROR_PROVIDER_UNAVAILABLE`: Provider not available
- `WORKFLOW_ERROR_AUTHENTICATION`: Authentication failure
- `WORKFLOW_ERROR_RATE_LIMIT`: Rate limit exceeded
- `WORKFLOW_ERROR_UNKNOWN`: Unknown error

### Recovery Strategy

**Recoverable Errors** (workflow paused, can resume):
- Network errors
- LLM service downtime
- Rate limit errors

**Non-Recoverable Errors** (workflow failed, must restart):
- Credit/budget exhaustion
- File I/O errors
- Authentication errors
- Invalid input (security validation failed)

### Pre-Execution Checks

Before executing a workflow node:
1. Network connectivity check
2. Budget/credit availability check
3. LLM provider availability check
4. Timeout validation

## Security

### Input Validation

1. **Workflow Names**:
   - Allowed: alphanumeric, spaces, hyphens, underscores, dots
   - Max length: 256 characters
   - Validation: `workflow_validate_name_safe()`

2. **State Keys**:
   - Allowed: alphanumeric, underscores, dots, hyphens
   - Max length: 128 characters
   - Validation: `workflow_validate_key_safe()`

3. **State Values**:
   - Max length: 10KB
   - Sanitization: escape control characters, backslashes, quotes
   - Sanitization: `workflow_sanitize_value()`

4. **Condition Expressions**:
   - Max length: 1KB
   - Validation: check for dangerous patterns (exec, eval, system, etc.)
   - Validation: `workflow_validate_condition_safe()`

### Security Logging

All security events are logged at WARN level:
- Invalid input patterns
- Validation failures
- Suspicious activity

## Observability

### Logging

Structured logging with `LOG_CAT_WORKFLOW` category:
- Workflow start/end
- Node execution (start, completion, failure)
- Errors with context
- Security events

Example:
```c
LOG_INFO(LOG_CAT_WORKFLOW, "[workflow:bug_triage id:123] node:analyze_bug type:1 status:completed");
```

### Telemetry

Privacy-first telemetry (opt-in only):
- Workflow start/end events
- Node execution events (with latency)
- Error events
- Performance metrics

### Audit Trail

All workflow operations logged for compliance:
- Workflow lifecycle (start, end, completion)
- Node execution with timestamps
- Security events
- Error events

## Testing

### Test Coverage

- **Unit Tests**: Core functionality (types, engine, checkpoint, etc.)
- **Integration Tests**: Workflow execution with real agents
- **E2E Tests**: Complete workflow scenarios
- **Error Tests**: All error handling scenarios
- **Target Coverage**: >= 80%

### Test Files

- `tests/test_workflow_types.c`: Data structures
- `tests/test_workflow_engine.c`: State machine
- `tests/test_workflow_checkpoint.c`: Persistence
- `tests/test_workflow_e2e.c`: End-to-end scenarios
- `tests/test_workflow_error_handling.c`: Error handling
- `tests/test_task_decomposer.c`: Task decomposition
- `tests/test_group_chat.c`: Multi-agent coordination
- `tests/test_router.c`: Conditional routing
- `tests/test_patterns.c`: Workflow patterns

## API Reference

### Workflow Lifecycle

```c
// Create workflow
Workflow* workflow_create(const char* name, const char* description, WorkflowNode* entry_node);

// Execute workflow
int workflow_execute(Workflow* wf, const char* input, char** output);

// Resume from checkpoint
int workflow_resume(Workflow* wf, uint64_t checkpoint_id);

// Pause workflow
int workflow_pause(Workflow* wf);

// Cancel workflow
int workflow_cancel(Workflow* wf);

// Destroy workflow
void workflow_destroy(Workflow* wf);
```

### State Management

```c
// Set state value
int workflow_set_state(Workflow* wf, const char* key, const char* value);

// Get state value
const char* workflow_get_state_value(Workflow* wf, const char* key);

// Clear state
int workflow_clear_state(Workflow* wf);
```

### Checkpointing

```c
// Create checkpoint
uint64_t workflow_checkpoint(Workflow* wf, const char* node_name);

// Restore from checkpoint
int workflow_restore_from_checkpoint(Workflow* wf, uint64_t checkpoint_id);

// List checkpoints
Checkpoint* workflow_list_checkpoints(Workflow* wf, size_t* count);
```

### Error Handling

```c
// Check timeout
bool workflow_check_timeout(time_t start_time, int timeout_seconds);

// Check network
bool workflow_check_network(void);

// Check budget
bool workflow_check_budget(Workflow* wf);

// Handle error
bool workflow_handle_error(Workflow* wf, WorkflowNode* node, WorkflowErrorType error_type, const char* error_msg);
```

## Performance Considerations

1. **Timeout Management**: Default 5 minutes per node, configurable
2. **State Size**: Max 10KB per state value
3. **Checkpoint Frequency**: Configurable per workflow
4. **Parallel Execution**: Supported for PARALLEL nodes
5. **Caching**: Checkpoint data cached in memory

## Limitations

1. **Linear Workflows**: Phase 1 supports linear workflows only (conditional routing in Phase 4)
2. **Single Provider**: Currently uses Anthropic provider only
3. **No Streaming**: Workflow execution is synchronous
4. **Limited Retry**: Basic retry logic (full retry policy in future)

## Future Enhancements

1. **Parallel Execution**: Full support for parallel node execution
2. **Conditional Routing**: Advanced condition expression evaluation
3. **Streaming**: Support for streaming workflow execution
4. **Multi-Provider**: Support for multiple LLM providers
5. **Advanced Retry**: Configurable retry policies per node
6. **Workflow Visualization**: Mermaid diagram export
7. **Workflow Templates**: Library of common workflow patterns

## References

- [Master Plan](MASTER_PLAN.md)
- [User Guide](USER_GUIDE.md)
- [Use Cases](USE_CASES.md)
- [ADR-001: Error Handling Strategy](ADR/001-error-handling-strategy.md)
- [ADR-002: Observability Integration](ADR/002-observability-integration.md)
- [ADR-003: Test Strategy](ADR/003-test-strategy.md)
- [ADR-004: Security Validation](ADR/004-security-validation.md)

