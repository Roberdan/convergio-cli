# Global Observability & Telemetry Integration

**Created**: 2025-12-20  
**Status**: ✅ Complete  
**Scope**: Global Convergio integration (not just workflow engine)

---

## Overview

This document describes the complete integration of observability and telemetry systems at the global level for Convergio. The workflow orchestration feature is fully integrated with these global systems, ensuring consistent logging, telemetry, and security auditing across all Convergio components.

---

## Global Logging System

### Implementation

The global logging system is implemented in `src/core/main.c` with the following components:

#### Log Categories

```c
typedef enum {
    LOG_CAT_SYSTEM,        // System/kernel operations
    LOG_CAT_AGENT,         // Agent lifecycle & delegation
    LOG_CAT_TOOL,          // Tool execution
    LOG_CAT_API,           // Claude API calls
    LOG_CAT_MEMORY,        // Memory/persistence operations
    LOG_CAT_MSGBUS,        // Message bus communication
    LOG_CAT_COST,          // Cost tracking
    LOG_CAT_WORKFLOW       // Workflow orchestration (NEW)
} LogCategory;
```

#### Log Levels

```c
typedef enum {
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE
} LogLevel;
```

#### Integration

- **Workflow Engine**: Uses `LOG_CAT_WORKFLOW` for all workflow-related logging
- **Color Coding**: `LOG_CAT_WORKFLOW` uses bright yellow (`\033[93m`) for visual distinction
- **Consistency**: All workflow components use the same logging system as other Convergio components

### Usage Example

```c
// In workflow_observability.c
workflow_log_event(LOG_LEVEL_INFO, "workflow_started", workflow->name, workflow->workflow_id, "Workflow execution started");
```

This integrates with the global `nous_log()` function:

```c
void nous_log(LogLevel level, LogCategory cat, const char* fmt, ...);
```

---

## Global Telemetry System

### Implementation

The global telemetry system is implemented in `src/telemetry/telemetry.c` with privacy-first, opt-in design.

#### Telemetry Event Types

```c
typedef enum {
    TELEMETRY_EVENT_API_CALL,       // API call to a provider
    TELEMETRY_EVENT_ERROR,          // Error occurred (type only, no content)
    TELEMETRY_EVENT_FALLBACK,       // Provider fallback triggered
    TELEMETRY_EVENT_SESSION_START,  // Session started
    TELEMETRY_EVENT_SESSION_END,    // Session ended
    TELEMETRY_EVENT_WORKFLOW_START, // Workflow execution started (NEW)
    TELEMETRY_EVENT_WORKFLOW_END,   // Workflow execution ended (NEW)
    TELEMETRY_EVENT_WORKFLOW_NODE,  // Workflow node executed (NEW)
    TELEMETRY_EVENT_WORKFLOW_ERROR, // Workflow error occurred (NEW)
} TelemetryEventType;
```

#### Privacy Principles

- **OPT-IN ONLY**: Telemetry is disabled by default
- **Privacy-First**: No PII, anonymous aggregate metrics only
- **User Control**: View/export/delete at any time via CLI commands

#### Initialization

Telemetry is initialized in `src/core/main.c`:

```c
// Initialize telemetry system (privacy-first, opt-in)
if (telemetry_init() != 0) {
    fprintf(stderr, "  \033[33m⚠ Telemetry initialization failed (non-critical)\033[0m\n");
} else {
    // Record session start in telemetry
    telemetry_record_session_start();
}
```

And shut down at the end:

```c
// Record session end in telemetry
telemetry_record_session_end();

// Shutdown telemetry (flushes pending events)
telemetry_shutdown();
```

### Workflow Integration

The workflow engine uses the global telemetry system via `workflow_observability.c`:

```c
void workflow_telemetry_start(Workflow* wf) {
    if (!telemetry_is_enabled()) return;
    
    // Record workflow start event
    // Uses global telemetry_record_* functions
}

void workflow_telemetry_node(Workflow* wf, WorkflowNode* node, bool success, double latency_ms) {
    if (!telemetry_is_enabled()) return;
    
    // Record node execution metrics
    // Uses global telemetry system
}
```

---

## CLI Commands

### Telemetry Management

Full telemetry management is available via the `convergio telemetry` command:

```bash
# Check telemetry status
convergio telemetry status

# View telemetry information
convergio telemetry info

# Enable telemetry (opt-in)
convergio telemetry enable

# Disable telemetry (opt-out)
convergio telemetry disable

# View collected telemetry data
convergio telemetry view

# Export telemetry data as JSON
convergio telemetry export

# Delete all telemetry data
convergio telemetry delete
```

### Implementation

The CLI commands are implemented in `src/core/commands/telemetry.c` and registered in `src/core/commands/commands.c`.

---

## Security Audit Integration

### Security Events

The workflow engine integrates with Convergio's security audit system:

```c
void workflow_security_audit_event(Workflow* wf, const char* event_type, const char* description, bool success);
```

This function:
- Records security-relevant events (authentication, authorization, input validation, etc.)
- Uses the global logging system with appropriate log levels
- Integrates with Convergio's security audit trail

### Audit Trail

All security events are logged with:
- Timestamp
- Event type
- Workflow context
- Success/failure status
- Description

---

## Integration Points

### 1. Workflow Engine → Global Logging

**File**: `src/workflow/workflow_observability.c`

```c
void workflow_log_event(LogLevel level, const char* event_name, const char* workflow_name, uint64_t workflow_id, const char* message) {
    nous_log(level, LOG_CAT_WORKFLOW, "[%s:%lu] %s: %s", workflow_name, workflow_id, event_name, message);
}
```

### 2. Workflow Engine → Global Telemetry

**File**: `src/workflow/workflow_observability.c`

```c
void workflow_telemetry_start(Workflow* wf) {
    if (!telemetry_is_enabled()) return;
    // Uses global telemetry system
    // Records workflow start event
}
```

### 3. Main Initialization

**File**: `src/core/main.c`

- Initializes telemetry system during startup
- Records session start event
- Records session end event on shutdown
- Shuts down telemetry system gracefully

### 4. Log Category Registration

**File**: `src/core/main.c`

```c
static const char* LOG_CAT_NAMES[] = {
    "SYSTEM", "AGENT", "TOOL", "API", "MEMORY", "MSGBUS", "COST", "WORKFLOW"
};

static const char* LOG_CAT_COLORS[] = {
    "\033[36m",   // Cyan - SYSTEM
    "\033[33m",   // Yellow - AGENT
    "\033[32m",   // Green - TOOL
    "\033[35m",   // Magenta - API
    "\033[34m",   // Blue - MEMORY
    "\033[37m",   // White - MSGBUS
    "\033[31m",   // Red - COST
    "\033[93m"    // Bright Yellow - WORKFLOW
};
```

---

## Benefits

### 1. Consistency

- All Convergio components use the same logging system
- Consistent log format across all components
- Unified telemetry collection

### 2. Observability

- Complete visibility into workflow execution
- Performance metrics per workflow and node
- Error tracking and analysis

### 3. Security

- Security audit trail for all workflow operations
- Input validation logging
- Security event tracking

### 4. User Control

- Privacy-first design (opt-in telemetry)
- Full user control via CLI commands
- Ability to view, export, and delete telemetry data

### 5. Debugging

- Structured logging with categories and levels
- Workflow-specific log category for easy filtering
- Telemetry data for performance analysis

---

## Testing

### Logging Tests

- Verify `LOG_CAT_WORKFLOW` appears in log output
- Verify log levels work correctly
- Verify log formatting is consistent

### Telemetry Tests

- Verify telemetry initialization in `main.c`
- Verify workflow events are recorded
- Verify CLI commands work correctly
- Verify privacy settings (opt-in/opt-out)

### Integration Tests

- Verify workflow engine uses global logging
- Verify workflow engine uses global telemetry
- Verify security audit events are logged

---

## Future Enhancements

### Potential Additions

1. **Log Rotation**: Automatic log file rotation to prevent disk space issues
2. **Log Aggregation**: Centralized log collection for distributed deployments
3. **Metrics Dashboard**: Visual dashboard for telemetry metrics
4. **Alerting**: Alerts for critical errors or performance issues
5. **Distributed Tracing**: Trace workflow execution across multiple processes/nodes

---

## Related Documents

- [ADR 002: Observability Integration](ADR/002-observability-integration.md)
- [Technical Documentation](TECHNICAL_DOCUMENTATION.md) - Section on Observability
- [User Guide](USER_GUIDE.md) - Section on Telemetry Management

---

## Summary

✅ **Global Logging**: Fully integrated with `LOG_CAT_WORKFLOW` category  
✅ **Global Telemetry**: Fully integrated with workflow-specific event types  
✅ **CLI Commands**: Complete telemetry management via `convergio telemetry`  
✅ **Security Audit**: Integrated with Convergio's security audit system  
✅ **Privacy-First**: Opt-in telemetry with full user control  
✅ **Consistency**: All components use the same observability systems  

The workflow orchestration feature is fully integrated with Convergio's global observability and telemetry systems, ensuring consistent logging, telemetry collection, and security auditing across all components.

