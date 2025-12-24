/**
 * CONVERGIO WORKFLOW OBSERVABILITY
 *
 * Integration with Convergio's logging, telemetry, and security systems:
 * - Structured logging for all workflow operations
 * - Telemetry events for workflow execution
 * - Security validation and input sanitization
 * - Audit trail for workflow operations
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/nous.h"
#include "nous/telemetry.h"
#include "nous/workflow.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// LOGGING INTEGRATION
// ============================================================================

/**
 * @brief Log workflow event with structured information
 */
void workflow_log_event(LogLevel level, const char* event, const char* workflow_name,
                        uint64_t workflow_id, const char* details) {
    if (workflow_name) {
        if (details) {
            nous_log(level, LOG_CAT_WORKFLOW, "[workflow:%s id:%llu] %s: %s", workflow_name,
                     (unsigned long long)workflow_id, event, details);
        } else {
            nous_log(level, LOG_CAT_WORKFLOW, "[workflow:%s id:%llu] %s", workflow_name,
                     (unsigned long long)workflow_id, event);
        }
    } else {
        if (details) {
            nous_log(level, LOG_CAT_WORKFLOW, "[workflow id:%llu] %s: %s",
                     (unsigned long long)workflow_id, event, details);
        } else {
            nous_log(level, LOG_CAT_WORKFLOW, "[workflow id:%llu] %s",
                     (unsigned long long)workflow_id, event);
        }
    }
}

/**
 * @brief Log workflow node execution
 */
void workflow_log_node_execution(const Workflow* wf, const WorkflowNode* node, const char* status,
                                 const char* details) {
    if (!wf || !node) {
        return;
    }

    const char* workflow_name = wf->name ? wf->name : "unnamed";
    const char* node_name = node->name ? node->name : "unnamed";

    if (details) {
        LOG_INFO(LOG_CAT_WORKFLOW, "[workflow:%s id:%llu] node:%s type:%d status:%s details:%s",
                 workflow_name, (unsigned long long)wf->workflow_id, node_name, (int)node->type,
                 status, details);
    } else {
        LOG_INFO(LOG_CAT_WORKFLOW, "[workflow:%s id:%llu] node:%s type:%d status:%s", workflow_name,
                 (unsigned long long)wf->workflow_id, node_name, (int)node->type, status);
    }
}

/**
 * @brief Log workflow error
 */
void workflow_log_error(const Workflow* wf, const char* error_type, const char* error_message) {
    if (!wf) {
        return;
    }

    const char* workflow_name = wf->name ? wf->name : "unnamed";

    if (error_message) {
        LOG_ERROR(LOG_CAT_WORKFLOW, "[workflow:%s id:%llu] error_type:%s error:%s", workflow_name,
                  (unsigned long long)wf->workflow_id, error_type, error_message);
    } else {
        LOG_ERROR(LOG_CAT_WORKFLOW, "[workflow:%s id:%llu] error_type:%s", workflow_name,
                  (unsigned long long)wf->workflow_id, error_type);
    }
}

// ============================================================================
// TELEMETRY INTEGRATION
// ============================================================================

/**
 * @brief Record workflow start event
 */
void workflow_telemetry_start(const Workflow* wf) {
    if (!wf || !telemetry_is_enabled()) {
        return;
    }

    // Record workflow start via telemetry system
    // Note: Telemetry system doesn't have direct workflow events yet,
    // so we use error event type with special error_type
    telemetry_record_error("workflow_start");

    workflow_log_event(LOG_LEVEL_INFO, "workflow_start", wf->name, wf->workflow_id, NULL);
}

/**
 * @brief Record workflow end event
 */
void workflow_telemetry_end(const Workflow* wf, bool success) {
    if (!wf || !telemetry_is_enabled()) {
        return;
    }

    // Record workflow end
    telemetry_record_error(success ? "workflow_end_success" : "workflow_end_failure");

    const char* status = success ? "success" : "failure";
    workflow_log_event(LOG_LEVEL_INFO, "workflow_end", wf->name, wf->workflow_id, status);
}

/**
 * @brief Record workflow node execution event
 */
void workflow_telemetry_node(const Workflow* wf, const WorkflowNode* node, bool success,
                             double latency_ms) {
    if (!wf || !node || !telemetry_is_enabled()) {
        return;
    }

    // Record node execution (using error event type with special error_type)
    char event_type[128];
    snprintf(event_type, sizeof(event_type), "workflow_node_%s_%s", success ? "success" : "failure",
             node->name ? node->name : "unnamed");
    telemetry_record_error(event_type);

    workflow_log_node_execution(wf, node, success ? "success" : "failure", NULL);
}

/**
 * @brief Record workflow error event
 */
void workflow_telemetry_error(const Workflow* wf, const char* error_type) {
    if (!wf || !telemetry_is_enabled()) {
        return;
    }

    // Record workflow error
    char event_type[128];
    snprintf(event_type, sizeof(event_type), "workflow_error_%s",
             error_type ? error_type : "unknown");
    telemetry_record_error(event_type);

    workflow_log_error(wf, error_type, NULL);
}

// ============================================================================
// SECURITY: INPUT VALIDATION & SANITIZATION
// ============================================================================

/**
 * @brief Validate workflow name (security: prevent injection)
 * @return true if valid, false otherwise
 */
bool workflow_validate_name_safe(const char* name) {
    if (!name) {
        return false;
    }

    // Check length (prevent buffer overflow)
    size_t len = strlen(name);
    if (len == 0 || len > 256) {
        return false;
    }

    // Check for dangerous characters (prevent injection)
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        // Allow alphanumeric, spaces, hyphens, underscores, dots
        if (!isalnum(c) && c != ' ' && c != '-' && c != '_' && c != '.') {
            return false;
        }
    }

    return true;
}

/**
 * @brief Validate workflow state key (security: prevent injection)
 * @return true if valid, false otherwise
 */
bool workflow_validate_key_safe(const char* key) {
    if (!key) {
        return false;
    }

    // Check length
    size_t len = strlen(key);
    if (len == 0 || len > 128) {
        return false;
    }

    // Check for dangerous characters
    for (size_t i = 0; i < len; i++) {
        char c = key[i];
        // Allow alphanumeric, underscores, dots, hyphens
        if (!isalnum(c) && c != '_' && c != '.' && c != '-') {
            return false;
        }
    }

    return true;
}

/**
 * @brief Sanitize workflow state value (security: prevent injection)
 * @return Sanitized string (caller must free) or NULL on failure
 */
char* workflow_sanitize_value(const char* value) {
    if (!value) {
        return NULL;
    }

    size_t len = strlen(value);
    if (len > 10240) { // Max 10KB per value
        return NULL;
    }

    // Allocate output buffer
    char* sanitized = malloc(len + 1);
    if (!sanitized) {
        return NULL;
    }

    // Copy and escape dangerous characters
    size_t out_idx = 0;
    for (size_t i = 0; i < len && out_idx < len; i++) {
        char c = value[i];

        // Escape control characters and dangerous sequences
        if (c < 0x20 && c != '\n' && c != '\r' && c != '\t') {
            // Skip control characters (except newline, carriage return, tab)
            continue;
        }

        // Escape backslashes and quotes (prevent JSON injection)
        if (c == '\\' || c == '"' || c == '\'') {
            if (out_idx + 1 < len) {
                sanitized[out_idx++] = '\\';
                sanitized[out_idx++] = c;
            }
        } else {
            sanitized[out_idx++] = c;
        }
    }

    sanitized[out_idx] = '\0';
    return sanitized;
}

/**
 * @brief Validate condition expression (security: prevent code injection)
 * @return true if valid, false otherwise
 */
bool workflow_validate_condition_safe(const char* condition) {
    if (!condition) {
        return true; // NULL condition is valid (no condition)
    }

    size_t len = strlen(condition);
    if (len > 1024) { // Max 1KB per condition
        return false;
    }

    // Check for dangerous patterns (prevent code injection)
    const char* dangerous_patterns[] = {
        "exec(",   "eval(",       "system(",  "popen(",   "fork(",
        "execve(", "import ",     "require ", "include ", "#include",
        "<script", "javascript:", "onerror=", "onload=",  NULL};

    for (int i = 0; dangerous_patterns[i]; i++) {
        if (strstr(condition, dangerous_patterns[i])) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// AUDIT TRAIL
// ============================================================================

/**
 * @brief Record audit event for workflow operation
 */
void workflow_audit_log(const Workflow* wf, const char* operation, const char* details) {
    if (!wf) {
        return;
    }

    const char* workflow_name = wf->name ? wf->name : "unnamed";

    // Log to audit trail (using INFO level for audit events)
    if (details) {
        LOG_INFO(LOG_CAT_WORKFLOW, "[AUDIT workflow:%s id:%llu] operation:%s details:%s",
                 workflow_name, (unsigned long long)wf->workflow_id, operation, details);
    } else {
        LOG_INFO(LOG_CAT_WORKFLOW, "[AUDIT workflow:%s id:%llu] operation:%s", workflow_name,
                 (unsigned long long)wf->workflow_id, operation);
    }

    // Also record in telemetry if enabled
    if (telemetry_is_enabled()) {
        char event_type[128];
        snprintf(event_type, sizeof(event_type), "workflow_audit_%s", operation);
        telemetry_record_error(event_type);
    }
}

/**
 * @brief Record security event (suspicious activity, validation failures, etc.)
 */
void workflow_security_log(const Workflow* wf, const char* security_event, const char* details) {
    if (!wf) {
        return;
    }

    const char* workflow_name = wf->name ? wf->name : "unnamed";

    // Log security events at WARN level (they're important)
    if (details) {
        LOG_WARN(LOG_CAT_WORKFLOW, "[SECURITY workflow:%s id:%llu] event:%s details:%s",
                 workflow_name, (unsigned long long)wf->workflow_id, security_event, details);
    } else {
        LOG_WARN(LOG_CAT_WORKFLOW, "[SECURITY workflow:%s id:%llu] event:%s", workflow_name,
                 (unsigned long long)wf->workflow_id, security_event);
    }

    // Record in telemetry
    if (telemetry_is_enabled()) {
        char event_type[128];
        snprintf(event_type, sizeof(event_type), "workflow_security_%s", security_event);
        telemetry_record_error(event_type);
    }
}
