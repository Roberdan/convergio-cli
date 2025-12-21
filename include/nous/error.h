/**
 * CONVERGIO UNIFIED ERROR HANDLING
 *
 * Provides a unified error type that wraps domain-specific errors
 * (WorkflowErrorType, ProviderError, MLXError) with a common interface.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef NOUS_ERROR_H
#define NOUS_ERROR_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// ERROR DOMAINS
// ============================================================================

typedef enum {
    ERROR_DOMAIN_NONE = 0,
    ERROR_DOMAIN_SYSTEM,        // System/OS errors
    ERROR_DOMAIN_WORKFLOW,      // Workflow execution errors
    ERROR_DOMAIN_PROVIDER,      // LLM provider errors
    ERROR_DOMAIN_MLX,           // MLX local model errors
    ERROR_DOMAIN_NETWORK,       // Network/HTTP errors
    ERROR_DOMAIN_DATABASE,      // SQLite/persistence errors
    ERROR_DOMAIN_AUTH,          // Authentication errors
    ERROR_DOMAIN_CONFIG         // Configuration errors
} ErrorDomain;

// ============================================================================
// GENERIC ERROR CODES
// ============================================================================

typedef enum {
    // Success
    CONVERGIO_OK = 0,

    // Generic errors (0-99)
    CONVERGIO_ERR_UNKNOWN = 1,
    CONVERGIO_ERR_INVALID_ARGUMENT = 2,
    CONVERGIO_ERR_OUT_OF_MEMORY = 3,
    CONVERGIO_ERR_NOT_INITIALIZED = 4,
    CONVERGIO_ERR_ALREADY_INITIALIZED = 5,
    CONVERGIO_ERR_NOT_FOUND = 6,
    CONVERGIO_ERR_ALREADY_EXISTS = 7,
    CONVERGIO_ERR_PERMISSION_DENIED = 8,
    CONVERGIO_ERR_TIMEOUT = 9,
    CONVERGIO_ERR_CANCELLED = 10,

    // Network errors (100-199)
    CONVERGIO_ERR_NETWORK = 100,
    CONVERGIO_ERR_CONNECTION_FAILED = 101,
    CONVERGIO_ERR_DNS_FAILED = 102,
    CONVERGIO_ERR_SSL_ERROR = 103,

    // Provider errors (200-299)
    CONVERGIO_ERR_RATE_LIMITED = 200,
    CONVERGIO_ERR_CREDIT_EXHAUSTED = 201,
    CONVERGIO_ERR_PROVIDER_DOWN = 202,
    CONVERGIO_ERR_INVALID_RESPONSE = 203,
    CONVERGIO_ERR_MODEL_NOT_FOUND = 204,

    // Auth errors (300-399)
    CONVERGIO_ERR_AUTH_FAILED = 300,
    CONVERGIO_ERR_TOKEN_EXPIRED = 301,
    CONVERGIO_ERR_API_KEY_INVALID = 302,

    // File/IO errors (400-499)
    CONVERGIO_ERR_FILE_NOT_FOUND = 400,
    CONVERGIO_ERR_FILE_READ = 401,
    CONVERGIO_ERR_FILE_WRITE = 402,
    CONVERGIO_ERR_PATH_TOO_LONG = 403,

    // Database errors (500-599)
    CONVERGIO_ERR_DB_OPEN = 500,
    CONVERGIO_ERR_DB_QUERY = 501,
    CONVERGIO_ERR_DB_CONSTRAINT = 502
} ConvergioErrorCode;

// ============================================================================
// UNIFIED ERROR STRUCTURE
// ============================================================================

typedef struct {
    ErrorDomain domain;              // Which subsystem
    ConvergioErrorCode code;         // Generic error code
    int domain_code;                 // Domain-specific code (WorkflowErrorType, ProviderError, MLXError)
    char message[256];               // Human-readable message
    char details[512];               // Additional context/stack trace
    bool is_retryable;               // Can operation be retried?
    int retry_after_ms;              // Suggested retry delay (0 = immediate)
    int http_status;                 // HTTP status if applicable
} ConvergioError;

// ============================================================================
// ERROR HANDLING API
// ============================================================================

/**
 * Initialize an error structure
 */
static inline void error_init(ConvergioError* err) {
    if (!err) return;
    err->domain = ERROR_DOMAIN_NONE;
    err->code = CONVERGIO_OK;
    err->domain_code = 0;
    err->message[0] = '\0';
    err->details[0] = '\0';
    err->is_retryable = false;
    err->retry_after_ms = 0;
    err->http_status = 0;
}

/**
 * Check if error indicates success
 */
static inline bool error_is_ok(const ConvergioError* err) {
    return !err || err->code == CONVERGIO_OK;
}

/**
 * Check if error is retryable
 */
static inline bool error_is_retryable(const ConvergioError* err) {
    return err && err->is_retryable;
}

/**
 * Set error with message
 */
void error_set(ConvergioError* err, ErrorDomain domain, ConvergioErrorCode code,
               const char* message);

/**
 * Set error with formatted message
 */
void error_setf(ConvergioError* err, ErrorDomain domain, ConvergioErrorCode code,
                const char* fmt, ...);

/**
 * Set error from workflow error type
 */
void error_from_workflow(ConvergioError* err, int workflow_error_type,
                         const char* message);

/**
 * Set error from provider error
 */
void error_from_provider(ConvergioError* err, int provider_error,
                         const char* message, int http_status, bool retryable);

/**
 * Set error from MLX error
 */
void error_from_mlx(ConvergioError* err, int mlx_error, const char* message);

/**
 * Get error domain name
 */
const char* error_domain_name(ErrorDomain domain);

/**
 * Get generic error code name
 */
const char* error_code_name(ConvergioErrorCode code);

/**
 * Format error as string (for logging)
 */
const char* error_to_string(const ConvergioError* err, char* buf, size_t buf_size);

// ============================================================================
// THREAD-LOCAL ERROR
// ============================================================================

/**
 * Get thread-local error (for functions that can't return errors)
 */
ConvergioError* error_get_last(void);

/**
 * Set thread-local error
 */
void error_set_last(const ConvergioError* err);

/**
 * Clear thread-local error
 */
void error_clear_last(void);

#endif // NOUS_ERROR_H
