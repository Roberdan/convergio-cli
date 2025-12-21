/**
 * CONVERGIO UNIFIED ERROR HANDLING - Implementation
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>

// ============================================================================
// THREAD-LOCAL ERROR
// ============================================================================

static pthread_key_t g_error_key;
static pthread_once_t g_error_key_once = PTHREAD_ONCE_INIT;

static void error_key_destructor(void* ptr) {
    free(ptr);
}

static void error_key_create(void) {
    pthread_key_create(&g_error_key, error_key_destructor);
}

ConvergioError* error_get_last(void) {
    pthread_once(&g_error_key_once, error_key_create);
    ConvergioError* err = pthread_getspecific(g_error_key);
    if (!err) {
        err = calloc(1, sizeof(ConvergioError));
        pthread_setspecific(g_error_key, err);
    }
    return err;
}

void error_set_last(const ConvergioError* err) {
    if (!err) return;
    ConvergioError* last = error_get_last();
    if (last) {
        *last = *err;
    }
}

void error_clear_last(void) {
    ConvergioError* last = error_get_last();
    if (last) {
        error_init(last);
    }
}

// ============================================================================
// ERROR SETTERS
// ============================================================================

void error_set(ConvergioError* err, ErrorDomain domain, ConvergioErrorCode code,
               const char* message) {
    if (!err) return;
    err->domain = domain;
    err->code = code;
    err->domain_code = 0;
    err->is_retryable = false;
    err->retry_after_ms = 0;
    err->http_status = 0;

    if (message) {
        strncpy(err->message, message, sizeof(err->message) - 1);
        err->message[sizeof(err->message) - 1] = '\0';
    } else {
        err->message[0] = '\0';
    }
    err->details[0] = '\0';
}

void error_setf(ConvergioError* err, ErrorDomain domain, ConvergioErrorCode code,
                const char* fmt, ...) {
    if (!err || !fmt) return;
    err->domain = domain;
    err->code = code;
    err->domain_code = 0;
    err->is_retryable = false;
    err->retry_after_ms = 0;
    err->http_status = 0;

    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, args);
    va_end(args);

    err->details[0] = '\0';
}

// ============================================================================
// DOMAIN-SPECIFIC ERROR CONVERSION
// ============================================================================

// WorkflowErrorType mapping (from workflow.h)
void error_from_workflow(ConvergioError* err, int workflow_error_type,
                         const char* message) {
    if (!err) return;
    err->domain = ERROR_DOMAIN_WORKFLOW;
    err->domain_code = workflow_error_type;

    // Map workflow errors to generic codes
    switch (workflow_error_type) {
        case 0:  // WORKFLOW_ERROR_NONE
            err->code = CONVERGIO_OK;
            err->is_retryable = false;
            break;
        case 1:  // WORKFLOW_ERROR_TIMEOUT
            err->code = CONVERGIO_ERR_TIMEOUT;
            err->is_retryable = true;
            break;
        case 2:  // WORKFLOW_ERROR_NETWORK
            err->code = CONVERGIO_ERR_NETWORK;
            err->is_retryable = true;
            break;
        case 3:  // WORKFLOW_ERROR_FILE_IO
            err->code = CONVERGIO_ERR_FILE_READ;
            err->is_retryable = false;
            break;
        case 4:  // WORKFLOW_ERROR_CREDIT_EXHAUSTED
            err->code = CONVERGIO_ERR_CREDIT_EXHAUSTED;
            err->is_retryable = false;
            break;
        case 5:  // WORKFLOW_ERROR_LLM_DOWN
            err->code = CONVERGIO_ERR_PROVIDER_DOWN;
            err->is_retryable = true;
            break;
        case 6:  // WORKFLOW_ERROR_TOOL_FAILED
            err->code = CONVERGIO_ERR_UNKNOWN;
            err->is_retryable = false;
            break;
        case 7:  // WORKFLOW_ERROR_AGENT_NOT_FOUND
            err->code = CONVERGIO_ERR_NOT_FOUND;
            err->is_retryable = false;
            break;
        case 8:  // WORKFLOW_ERROR_PROVIDER_UNAVAILABLE
            err->code = CONVERGIO_ERR_PROVIDER_DOWN;
            err->is_retryable = true;
            break;
        case 9:  // WORKFLOW_ERROR_AUTHENTICATION
            err->code = CONVERGIO_ERR_AUTH_FAILED;
            err->is_retryable = false;
            break;
        case 10: // WORKFLOW_ERROR_RATE_LIMIT
            err->code = CONVERGIO_ERR_RATE_LIMITED;
            err->is_retryable = true;
            err->retry_after_ms = 1000;
            break;
        default: // WORKFLOW_ERROR_UNKNOWN
            err->code = CONVERGIO_ERR_UNKNOWN;
            err->is_retryable = false;
            break;
    }

    if (message) {
        strncpy(err->message, message, sizeof(err->message) - 1);
        err->message[sizeof(err->message) - 1] = '\0';
    }
}

// ProviderError mapping (from provider.h)
void error_from_provider(ConvergioError* err, int provider_error,
                         const char* message, int http_status, bool retryable) {
    if (!err) return;
    err->domain = ERROR_DOMAIN_PROVIDER;
    err->domain_code = provider_error;
    err->http_status = http_status;
    err->is_retryable = retryable;

    // Map provider errors to generic codes
    switch (provider_error) {
        case 0:  // PROVIDER_OK
            err->code = CONVERGIO_OK;
            break;
        case 1:  // PROVIDER_ERR_NETWORK
            err->code = CONVERGIO_ERR_NETWORK;
            break;
        case 2:  // PROVIDER_ERR_AUTH
            err->code = CONVERGIO_ERR_AUTH_FAILED;
            break;
        case 3:  // PROVIDER_ERR_RATE_LIMIT
            err->code = CONVERGIO_ERR_RATE_LIMITED;
            err->retry_after_ms = 1000;
            break;
        case 4:  // PROVIDER_ERR_INVALID_REQUEST
            err->code = CONVERGIO_ERR_INVALID_ARGUMENT;
            break;
        case 5:  // PROVIDER_ERR_SERVER
            err->code = CONVERGIO_ERR_PROVIDER_DOWN;
            break;
        case 6:  // PROVIDER_ERR_TIMEOUT
            err->code = CONVERGIO_ERR_TIMEOUT;
            break;
        case 7:  // PROVIDER_ERR_CONTENT_FILTER
            err->code = CONVERGIO_ERR_PERMISSION_DENIED;
            break;
        case 8:  // PROVIDER_ERR_MODEL_OVERLOADED
            err->code = CONVERGIO_ERR_PROVIDER_DOWN;
            err->is_retryable = true;
            break;
        case 9:  // PROVIDER_ERR_CREDIT_EXHAUSTED
            err->code = CONVERGIO_ERR_CREDIT_EXHAUSTED;
            break;
        case 10: // PROVIDER_ERR_NOT_INITIALIZED
            err->code = CONVERGIO_ERR_NOT_INITIALIZED;
            break;
        default: // PROVIDER_ERR_UNKNOWN
            err->code = CONVERGIO_ERR_UNKNOWN;
            break;
    }

    if (message) {
        strncpy(err->message, message, sizeof(err->message) - 1);
        err->message[sizeof(err->message) - 1] = '\0';
    }
}

// MLXError mapping (from mlx.h)
void error_from_mlx(ConvergioError* err, int mlx_error, const char* message) {
    if (!err) return;
    err->domain = ERROR_DOMAIN_MLX;
    err->domain_code = mlx_error;
    err->is_retryable = false;

    // Map MLX errors to generic codes
    switch (mlx_error) {
        case 0:  // MLX_OK
            err->code = CONVERGIO_OK;
            break;
        case 1:  // MLX_ERR_NOT_AVAILABLE
            err->code = CONVERGIO_ERR_NOT_INITIALIZED;
            break;
        case 2:  // MLX_ERR_MODEL_NOT_FOUND
            err->code = CONVERGIO_ERR_MODEL_NOT_FOUND;
            break;
        case 3:  // MLX_ERR_DOWNLOAD_FAILED
            err->code = CONVERGIO_ERR_NETWORK;
            err->is_retryable = true;
            break;
        case 4:  // MLX_ERR_OUT_OF_MEMORY
            err->code = CONVERGIO_ERR_OUT_OF_MEMORY;
            break;
        case 5:  // MLX_ERR_INFERENCE_FAILED
            err->code = CONVERGIO_ERR_UNKNOWN;
            break;
        case 6:  // MLX_ERR_CONTEXT_OVERFLOW
            err->code = CONVERGIO_ERR_INVALID_ARGUMENT;
            break;
        case 7:  // MLX_ERR_TOKENIZER_FAILED
            err->code = CONVERGIO_ERR_UNKNOWN;
            break;
        default: // MLX_ERR_UNKNOWN
            err->code = CONVERGIO_ERR_UNKNOWN;
            break;
    }

    if (message) {
        strncpy(err->message, message, sizeof(err->message) - 1);
        err->message[sizeof(err->message) - 1] = '\0';
    }
}

// ============================================================================
// ERROR STRING FUNCTIONS
// ============================================================================

const char* error_domain_name(ErrorDomain domain) {
    switch (domain) {
        case ERROR_DOMAIN_NONE:     return "none";
        case ERROR_DOMAIN_SYSTEM:   return "system";
        case ERROR_DOMAIN_WORKFLOW: return "workflow";
        case ERROR_DOMAIN_PROVIDER: return "provider";
        case ERROR_DOMAIN_MLX:      return "mlx";
        case ERROR_DOMAIN_NETWORK:  return "network";
        case ERROR_DOMAIN_DATABASE: return "database";
        case ERROR_DOMAIN_AUTH:     return "auth";
        case ERROR_DOMAIN_CONFIG:   return "config";
        default:                    return "unknown";
    }
}

const char* error_code_name(ConvergioErrorCode code) {
    switch (code) {
        case CONVERGIO_OK:                  return "OK";
        case CONVERGIO_ERR_UNKNOWN:         return "UNKNOWN";
        case CONVERGIO_ERR_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case CONVERGIO_ERR_OUT_OF_MEMORY:   return "OUT_OF_MEMORY";
        case CONVERGIO_ERR_NOT_INITIALIZED: return "NOT_INITIALIZED";
        case CONVERGIO_ERR_ALREADY_INITIALIZED: return "ALREADY_INITIALIZED";
        case CONVERGIO_ERR_NOT_FOUND:       return "NOT_FOUND";
        case CONVERGIO_ERR_ALREADY_EXISTS:  return "ALREADY_EXISTS";
        case CONVERGIO_ERR_PERMISSION_DENIED: return "PERMISSION_DENIED";
        case CONVERGIO_ERR_TIMEOUT:         return "TIMEOUT";
        case CONVERGIO_ERR_CANCELLED:       return "CANCELLED";
        case CONVERGIO_ERR_NETWORK:         return "NETWORK";
        case CONVERGIO_ERR_CONNECTION_FAILED: return "CONNECTION_FAILED";
        case CONVERGIO_ERR_DNS_FAILED:      return "DNS_FAILED";
        case CONVERGIO_ERR_SSL_ERROR:       return "SSL_ERROR";
        case CONVERGIO_ERR_RATE_LIMITED:    return "RATE_LIMITED";
        case CONVERGIO_ERR_CREDIT_EXHAUSTED: return "CREDIT_EXHAUSTED";
        case CONVERGIO_ERR_PROVIDER_DOWN:   return "PROVIDER_DOWN";
        case CONVERGIO_ERR_INVALID_RESPONSE: return "INVALID_RESPONSE";
        case CONVERGIO_ERR_MODEL_NOT_FOUND: return "MODEL_NOT_FOUND";
        case CONVERGIO_ERR_AUTH_FAILED:     return "AUTH_FAILED";
        case CONVERGIO_ERR_TOKEN_EXPIRED:   return "TOKEN_EXPIRED";
        case CONVERGIO_ERR_API_KEY_INVALID: return "API_KEY_INVALID";
        case CONVERGIO_ERR_FILE_NOT_FOUND:  return "FILE_NOT_FOUND";
        case CONVERGIO_ERR_FILE_READ:       return "FILE_READ";
        case CONVERGIO_ERR_FILE_WRITE:      return "FILE_WRITE";
        case CONVERGIO_ERR_PATH_TOO_LONG:   return "PATH_TOO_LONG";
        case CONVERGIO_ERR_DB_OPEN:         return "DB_OPEN";
        case CONVERGIO_ERR_DB_QUERY:        return "DB_QUERY";
        case CONVERGIO_ERR_DB_CONSTRAINT:   return "DB_CONSTRAINT";
        default:                            return "UNKNOWN";
    }
}

const char* error_to_string(const ConvergioError* err, char* buf, size_t buf_size) {
    if (!err || !buf || buf_size == 0) return "";

    snprintf(buf, buf_size, "[%s:%s] %s%s",
             error_domain_name(err->domain),
             error_code_name(err->code),
             err->message,
             err->is_retryable ? " (retryable)" : "");

    return buf;
}
