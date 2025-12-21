# Global Integration: Telemetry & Security

**Created**: 2025-12-20  
**Status**: ✅ Complete  
**Scope**: Global Convergio integration (all components)

---

## Overview

This document describes the global integration of telemetry and security systems across all Convergio components. All components must use these shared systems for consistency, observability, and security.

---

## Global Telemetry Integration

### Core Principles

1. **Privacy-First**: Opt-in only, no PII, anonymous aggregate metrics
2. **Consistent**: All components use the same telemetry system
3. **Comprehensive**: All important events are tracked
4. **User Control**: Full CLI management via `convergio telemetry`

### Integration Points

#### 1. Provider Calls (`src/providers/*.c`)

**Required**: Record all API calls to LLM providers

```c
#include "nous/telemetry.h"

// After successful API call
telemetry_record_api_call(
    "anthropic",           // provider
    "claude-sonnet-4",     // model
    tokens_input,          // input tokens
    tokens_output,         // output tokens
    latency_ms            // latency in milliseconds
);

// On error
telemetry_record_error("provider_api_error");
```

**Components to update**:
- `src/providers/anthropic.c`
- `src/providers/openai.c`
- `src/providers/gemini.c`
- `src/providers/ollama.c`
- `src/providers/openrouter.c`
- `src/providers/mlx.m`

#### 2. Orchestrator Events (`src/orchestrator/orchestrator.c`)

**Required**: Record delegation, planning, and convergence events

```c
// On agent delegation
telemetry_record_error("agent_delegation"); // Use error type for now, extend later

// On planning events
// (Add new event types as needed)
```

#### 3. Tool Execution (`src/tools/tools.c`)

**Required**: Record tool execution events (already partially done)

```c
// On tool execution success/failure
telemetry_record_error("tool_execution"); // Use error type for now, extend later
```

#### 4. Workflow Engine (`src/workflow/workflow_observability.c`)

**Status**: ✅ Already integrated

Uses workflow-specific telemetry events:
- `TELEMETRY_EVENT_WORKFLOW_START`
- `TELEMETRY_EVENT_WORKFLOW_END`
- `TELEMETRY_EVENT_WORKFLOW_NODE`
- `TELEMETRY_EVENT_WORKFLOW_ERROR`

---

## Global Security Integration

### Core Principles

1. **Defense in Depth**: Multiple layers of security
2. **Input Validation**: All inputs validated and sanitized
3. **Path Safety**: All file operations use safe path functions
4. **Command Safety**: All command execution uses sanitized inputs
5. **SQL Safety**: All SQL queries use parameterized statements

### Integration Points

#### 1. Path Operations

**Required**: Use `tools_is_path_safe()` for all file operations

```c
#include "nous/tools.h"

// Before any file operation
if (!tools_is_path_safe(file_path)) {
    LOG_ERROR(LOG_CAT_SYSTEM, "Path traversal attempt blocked: %s", file_path);
    return -1;
}
```

**Components to verify**:
- `src/tools/tools.c` ✅ (already uses it)
- `src/memory/persistence.c` (verify SQLite file paths)
- `src/workflow/checkpoint.c` (verify checkpoint file paths)
- `src/core/config.c` (verify config file paths)

#### 2. Command Execution

**Required**: Use `sanitize_grep_pattern()` and `normalize_command()` for all command execution

```c
#include "nous/tools.h"

// For grep patterns
char* sanitized = sanitize_grep_pattern(pattern);
if (!sanitized) {
    LOG_ERROR(LOG_CAT_TOOL, "Invalid grep pattern");
    return -1;
}

// For shell commands
char* normalized = normalize_command(command);
if (!normalized) {
    LOG_ERROR(LOG_CAT_TOOL, "Invalid command");
    return -1;
}
```

**Components to verify**:
- `src/tools/tools.c` ✅ (already uses it)
- `src/workflow/task_decomposer.c` (if executing commands)
- `src/orchestrator/orchestrator.c` (if executing commands)

#### 3. SQL Queries

**Required**: Use parameterized queries only

```c
// ✅ CORRECT: Parameterized query
sqlite3_stmt* stmt;
sqlite3_prepare_v2(db, "SELECT * FROM workflows WHERE id = ?", -1, &stmt, NULL);
sqlite3_bind_int64(stmt, 1, workflow_id);

// ❌ WRONG: String concatenation
char query[256];
snprintf(query, sizeof(query), "SELECT * FROM workflows WHERE id = %lu", workflow_id);
```

**Components to verify**:
- `src/memory/persistence.c` ✅ (already uses parameterized queries)
- `src/workflow/checkpoint.c` ✅ (already uses parameterized queries)
- `src/memory/semantic_persistence.c` (verify)

#### 4. Input Validation

**Required**: Validate all user inputs

```c
// Validate workflow names
if (!workflow_validate_name(name)) {
    LOG_ERROR(LOG_CAT_WORKFLOW, "Invalid workflow name: %s", name);
    return -1;
}

// Validate state keys
if (!workflow_validate_key(key)) {
    LOG_ERROR(LOG_CAT_WORKFLOW, "Invalid state key: %s", key);
    return -1;
}
```

**Components to verify**:
- `src/workflow/workflow_types.c` ✅ (already validates)
- `src/core/commands/workflow.c` (verify CLI input validation)
- `src/orchestrator/orchestrator.c` (verify user input validation)

---

## Global Logging Integration

### Core Principles

1. **Consistent Categories**: Use appropriate log categories
2. **Appropriate Levels**: Use appropriate log levels
3. **Structured**: Include context in log messages
4. **No Silent Failures**: All errors must be logged

### Log Categories

```c
LOG_CAT_SYSTEM,        // System/kernel operations
LOG_CAT_AGENT,         // Agent lifecycle & delegation
LOG_CAT_TOOL,          // Tool execution
LOG_CAT_API,           // Claude API calls
LOG_CAT_MEMORY,        // Memory/persistence operations
LOG_CAT_MSGBUS,        // Message bus communication
LOG_CAT_COST,          // Cost tracking
LOG_CAT_WORKFLOW       // Workflow orchestration
```

### Usage

```c
#include "nous/nous.h"

// Error logging
LOG_ERROR(LOG_CAT_SYSTEM, "Failed to initialize: %s", error_msg);

// Warning logging
LOG_WARN(LOG_CAT_AGENT, "Agent %s is taking longer than expected", agent_name);

// Info logging
LOG_INFO(LOG_CAT_WORKFLOW, "Workflow %s started", workflow_name);

// Debug logging
LOG_DEBUG(LOG_CAT_API, "API call to %s: %s tokens", provider, tokens);
```

---

## Integration Checklist

### Telemetry Integration

- [x] Core telemetry system (`src/telemetry/telemetry.c`)
- [x] CLI commands (`src/core/commands/telemetry.c`)
- [x] Main initialization (`src/core/main.c`)
- [x] Workflow engine (`src/workflow/workflow_observability.c`)
- [ ] Provider calls (`src/providers/*.c`) - **PENDING**
- [ ] Orchestrator events (`src/orchestrator/orchestrator.c`) - **PENDING**
- [ ] Tool execution (`src/tools/tools.c`) - **PARTIAL**

### Security Integration

- [x] Path safety functions (`src/tools/tools.c`)
- [x] Command sanitization (`src/tools/tools.c`)
- [x] SQL parameterization (`src/memory/persistence.c`, `src/workflow/checkpoint.c`)
- [x] Input validation (`src/workflow/workflow_types.c`)
- [ ] Verify all file operations use `tools_is_path_safe()` - **PENDING**
- [ ] Verify all command execution uses sanitization - **PENDING**
- [ ] Verify all SQL queries are parameterized - **PENDING**

### Logging Integration

- [x] Global logging system (`src/core/main.c`)
- [x] Log categories defined (`include/nous/nous.h`)
- [x] Workflow category added (`LOG_CAT_WORKFLOW`)
- [x] All components use `nous_log()` or macros
- [ ] Verify all errors are logged - **PENDING**
- [ ] Verify appropriate log levels used - **PENDING**

---

## Implementation Priority

### Phase 1: Critical Components (High Priority)

1. **Provider Telemetry** - Track all API calls
2. **Security Verification** - Verify all path/command/SQL operations
3. **Error Logging** - Ensure all errors are logged

### Phase 2: Important Components (Medium Priority)

1. **Orchestrator Telemetry** - Track delegation and planning events
2. **Tool Execution Telemetry** - Track tool usage
3. **Log Level Review** - Ensure appropriate log levels

### Phase 3: Enhancement (Low Priority)

1. **Extended Event Types** - Add more specific telemetry events
2. **Performance Metrics** - Add performance telemetry
3. **Security Audit Logging** - Enhanced security event logging

---

## Testing

### Telemetry Tests

- Verify telemetry events are recorded
- Verify telemetry can be disabled
- Verify telemetry data can be exported/deleted

### Security Tests

- Verify path traversal protection
- Verify command injection protection
- Verify SQL injection protection
- Verify input validation

### Logging Tests

- Verify log categories work correctly
- Verify log levels work correctly
- Verify all errors are logged

---

## Related Documents

- [OBSERVABILITY_INTEGRATION.md](workflow-orchestration/OBSERVABILITY_INTEGRATION.md) - Workflow-specific observability
- [SECURITY_AUDIT.md](../SECURITY_AUDIT.md) - Security audit report
- [ADR 002: Observability Integration](workflow-orchestration/ADR/002-observability-integration.md)
- [ADR 004: Security Validation](workflow-orchestration/ADR/004-security-validation.md)

---

## Summary

✅ **Telemetry**: Core system complete, CLI commands complete, workflow integration complete  
⏳ **Telemetry**: Provider integration pending, orchestrator integration pending  
✅ **Security**: Core functions complete, workflow validation complete  
⏳ **Security**: Global verification pending  
✅ **Logging**: Global system complete, all categories defined  
⏳ **Logging**: Verification pending  

The global integration of telemetry and security is in progress. Critical components (workflow engine) are fully integrated. Remaining components (providers, orchestrator) need telemetry integration.

