# ADR-002: Observability Integration Strategy

**Status**: Accepted  
**Date**: 2025-01-XX  
**Deciders**: AI Team  
**Context**: Workflow orchestration needs integration with Convergio's logging, telemetry, security, and observability systems

## Context

Convergio has established systems for:
- Structured logging (`nous_log` with categories)
- Privacy-first telemetry (opt-in, anonymous metrics)
- Security validation and audit trails
- Performance monitoring

The workflow orchestration system must integrate with these systems to:
- Provide visibility into workflow execution
- Track performance metrics
- Ensure security compliance
- Enable debugging and troubleshooting

## Decision

We integrate workflow orchestration with Convergio's observability stack:

1. **Logging Integration**:
   - Add `LOG_CAT_WORKFLOW` category to `nous.h`
   - Structured logging for all workflow events (start, end, node execution, errors)
   - Context-rich log messages (workflow name, ID, node, status)

2. **Telemetry Integration**:
   - Add workflow event types to `telemetry.h` (start, end, node, error)
   - Record workflow execution metrics (latency, success/failure rates)
   - Privacy-first: only record if telemetry is enabled (opt-in)

3. **Security Integration**:
   - Input validation (workflow names, state keys, condition expressions)
   - Input sanitization (prevent injection attacks)
   - Security event logging (suspicious activity, validation failures)

4. **Audit Trail**:
   - Log all workflow operations for compliance
   - Track workflow lifecycle (start, end, completion, failures)
   - Record node execution with timestamps

## Consequences

### Positive
- Full visibility into workflow execution
- Security compliance (input validation, audit trails)
- Performance monitoring (latency tracking)
- Privacy-first approach (opt-in telemetry)

### Negative
- Additional code complexity
- Performance overhead (logging, telemetry)
- Storage requirements for audit logs

### Mitigation
- Conditional logging (only if log level permits)
- Telemetry only if enabled (opt-in)
- Audit logs stored locally (privacy-first)

## Implementation

- `src/workflow/workflow_observability.c`: Observability integration functions
- `include/nous/nous.h`: Added `LOG_CAT_WORKFLOW` category
- `include/nous/telemetry.h`: Added workflow event types
- `src/workflow/workflow_engine.c`: Integrated logging/telemetry in execution flow
- `src/workflow/workflow_types.c`: Security validation in state management

## References

- Convergio Privacy Policy (PRIVACY_POLICY.md)
- Convergio Telemetry System (include/nous/telemetry.h)
- Convergio Logging System (include/nous/nous.h)

