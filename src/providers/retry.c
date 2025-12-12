/**
 * CONVERGIO RETRY HANDLER
 *
 * Exponential backoff and retry logic for API calls:
 * - Configurable retry policies
 * - Jittered exponential backoff
 * - Rate limit handling
 * - Circuit breaker pattern
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

typedef struct {
    int max_retries;           // Maximum retry attempts
    int base_delay_ms;         // Initial delay (milliseconds)
    int max_delay_ms;          // Maximum delay cap
    double backoff_multiplier; // Exponential multiplier
    double jitter_factor;      // Random jitter (0.0 - 1.0)
    bool retry_on_timeout;     // Retry timeout errors
    bool retry_on_rate_limit;  // Retry rate limit errors
    bool retry_on_server_error;// Retry 5xx errors
} RetryPolicy;

// Default policy
static RetryPolicy g_default_policy = {
    .max_retries = 3,
    .base_delay_ms = 1000,
    .max_delay_ms = 60000,
    .backoff_multiplier = 2.0,
    .jitter_factor = 0.25,
    .retry_on_timeout = true,
    .retry_on_rate_limit = true,
    .retry_on_server_error = true,
};

// Per-provider policies
static RetryPolicy g_provider_policies[4];
static bool g_policies_initialized = false;
static pthread_mutex_t g_retry_mutex = PTHREAD_MUTEX_INITIALIZER;

// ============================================================================
// CIRCUIT BREAKER
// ============================================================================

typedef enum {
    CIRCUIT_CLOSED,    // Normal operation
    CIRCUIT_OPEN,      // Failing, reject requests
    CIRCUIT_HALF_OPEN, // Testing if recovered
} CircuitState;

typedef struct {
    CircuitState state;
    int failure_count;
    int success_count;
    time_t last_failure;
    time_t opened_at;
    int failure_threshold;    // Failures before opening
    int success_threshold;    // Successes to close from half-open
    int open_duration_sec;    // Time before trying half-open
} CircuitBreaker;

static CircuitBreaker g_circuits[4] = {0};  // One per provider

// ============================================================================
// STATISTICS
// ============================================================================

typedef struct {
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    uint64_t retried_requests;
    uint64_t total_retries;
    uint64_t circuit_rejections;
    double total_delay_ms;
} RetryStats;

static RetryStats g_stats[4] = {0};

// ============================================================================
// HELPERS
// ============================================================================

static double random_jitter(double jitter_factor) {
    // Random value between -jitter_factor and +jitter_factor
    double r = (double)rand() / RAND_MAX;
    return (r * 2.0 - 1.0) * jitter_factor;
}

static int calculate_delay(RetryPolicy* policy, int attempt) {
    // Exponential backoff with jitter
    double delay = policy->base_delay_ms * pow(policy->backoff_multiplier, attempt);

    // Apply jitter
    double jitter = random_jitter(policy->jitter_factor);
    delay *= (1.0 + jitter);

    // Cap at max delay
    if (delay > policy->max_delay_ms) {
        delay = policy->max_delay_ms;
    }

    return (int)delay;
}

static bool should_retry(RetryPolicy* policy, ProviderError error) {
    switch (error) {
        case PROVIDER_ERR_TIMEOUT:
            return policy->retry_on_timeout;
        case PROVIDER_ERR_RATE_LIMIT:
            return policy->retry_on_rate_limit;
        case PROVIDER_ERR_NETWORK:
        case PROVIDER_ERR_UNKNOWN:
            return policy->retry_on_server_error;
        case PROVIDER_ERR_AUTH:
        case PROVIDER_ERR_MODEL_NOT_FOUND:
        case PROVIDER_ERR_CONTENT_FILTER:
        case PROVIDER_ERR_CONTEXT_LENGTH:
            return false;  // Don't retry these
        default:
            return false;
    }
}

// ============================================================================
// CIRCUIT BREAKER LOGIC
// ============================================================================

static void circuit_init(CircuitBreaker* cb) {
    cb->state = CIRCUIT_CLOSED;
    cb->failure_count = 0;
    cb->success_count = 0;
    cb->last_failure = 0;
    cb->opened_at = 0;
    cb->failure_threshold = 5;
    cb->success_threshold = 2;
    cb->open_duration_sec = 30;
}

static bool circuit_allow_request(CircuitBreaker* cb) {
    pthread_mutex_lock(&g_retry_mutex);

    time_t now = time(NULL);

    switch (cb->state) {
        case CIRCUIT_CLOSED:
            pthread_mutex_unlock(&g_retry_mutex);
            return true;

        case CIRCUIT_OPEN:
            // Check if we should transition to half-open
            if (now - cb->opened_at >= cb->open_duration_sec) {
                cb->state = CIRCUIT_HALF_OPEN;
                cb->success_count = 0;
                pthread_mutex_unlock(&g_retry_mutex);
                return true;
            }
            pthread_mutex_unlock(&g_retry_mutex);
            return false;

        case CIRCUIT_HALF_OPEN:
            pthread_mutex_unlock(&g_retry_mutex);
            return true;
    }

    pthread_mutex_unlock(&g_retry_mutex);
    return false;
}

static void circuit_record_success(CircuitBreaker* cb) {
    pthread_mutex_lock(&g_retry_mutex);

    switch (cb->state) {
        case CIRCUIT_CLOSED:
            cb->failure_count = 0;
            break;

        case CIRCUIT_HALF_OPEN:
            cb->success_count++;
            if (cb->success_count >= cb->success_threshold) {
                cb->state = CIRCUIT_CLOSED;
                cb->failure_count = 0;
            }
            break;

        case CIRCUIT_OPEN:
            // Shouldn't happen
            break;
    }

    pthread_mutex_unlock(&g_retry_mutex);
}

static void circuit_record_failure(CircuitBreaker* cb) {
    pthread_mutex_lock(&g_retry_mutex);

    cb->last_failure = time(NULL);

    switch (cb->state) {
        case CIRCUIT_CLOSED:
            cb->failure_count++;
            if (cb->failure_count >= cb->failure_threshold) {
                cb->state = CIRCUIT_OPEN;
                cb->opened_at = time(NULL);
            }
            break;

        case CIRCUIT_HALF_OPEN:
            // Failed while testing, go back to open
            cb->state = CIRCUIT_OPEN;
            cb->opened_at = time(NULL);
            break;

        case CIRCUIT_OPEN:
            // Already open
            break;
    }

    pthread_mutex_unlock(&g_retry_mutex);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

int retry_init(void) {
    pthread_mutex_lock(&g_retry_mutex);

    if (g_policies_initialized) {
        pthread_mutex_unlock(&g_retry_mutex);
        return 0;
    }

    srand((unsigned int)time(NULL));

    // Initialize per-provider policies
    for (int i = 0; i < 4; i++) {
        g_provider_policies[i] = g_default_policy;
        circuit_init(&g_circuits[i]);
        memset(&g_stats[i], 0, sizeof(RetryStats));
    }

    // Provider-specific adjustments
    // Anthropic: Higher rate limits, longer delays
    g_provider_policies[PROVIDER_ANTHROPIC].max_delay_ms = 120000;

    // OpenAI: Aggressive retry on rate limits
    g_provider_policies[PROVIDER_OPENAI].max_retries = 5;

    // Gemini: Free tier has lower limits
    g_provider_policies[PROVIDER_GEMINI].base_delay_ms = 2000;

    g_policies_initialized = true;
    pthread_mutex_unlock(&g_retry_mutex);

    return 0;
}

void retry_shutdown(void) {
    pthread_mutex_lock(&g_retry_mutex);
    g_policies_initialized = false;
    pthread_mutex_unlock(&g_retry_mutex);
}

// ============================================================================
// RETRY EXECUTION
// ============================================================================

typedef char* (*RetryableFunc)(void* ctx, ProviderError* out_error);

/**
 * Execute a function with retry logic
 */
char* retry_execute(ProviderType provider, RetryableFunc func, void* ctx,
                    ProviderError* out_error) {
    if (!g_policies_initialized) {
        retry_init();
    }

    RetryPolicy* policy = &g_provider_policies[provider];
    CircuitBreaker* circuit = &g_circuits[provider];
    RetryStats* stats = &g_stats[provider];

    stats->total_requests++;

    // Check circuit breaker
    if (!circuit_allow_request(circuit)) {
        stats->circuit_rejections++;
        if (out_error) *out_error = PROVIDER_ERR_RATE_LIMIT;
        return NULL;
    }

    ProviderError last_error = PROVIDER_OK;
    char* result = NULL;

    for (int attempt = 0; attempt <= policy->max_retries; attempt++) {
        // Execute the function
        result = func(ctx, &last_error);

        if (result != NULL || last_error == PROVIDER_OK) {
            // Success
            stats->successful_requests++;
            circuit_record_success(circuit);
            if (out_error) *out_error = PROVIDER_OK;
            return result;
        }

        // Check if we should retry
        if (attempt >= policy->max_retries || !should_retry(policy, last_error)) {
            break;
        }

        // Calculate delay
        int delay_ms = calculate_delay(policy, attempt);
        stats->total_delay_ms += delay_ms;
        stats->total_retries++;

        // Special handling for rate limit errors
        if (last_error == PROVIDER_ERR_RATE_LIMIT) {
            // Use longer delay for rate limits
            delay_ms = delay_ms * 2;
            if (delay_ms > policy->max_delay_ms) {
                delay_ms = policy->max_delay_ms;
            }
        }

        // Sleep before retry
        usleep(delay_ms * 1000);

        if (attempt == 0) {
            stats->retried_requests++;
        }
    }

    // All retries failed
    stats->failed_requests++;
    circuit_record_failure(circuit);
    if (out_error) *out_error = last_error;
    return NULL;
}

// ============================================================================
// SIMPLE RETRY WRAPPER
// ============================================================================

typedef struct {
    Provider* provider;
    const char* model;
    const char* system;
    const char* user;
    TokenUsage* usage;
} ChatContext;

static char* chat_wrapper(void* ctx, ProviderError* out_error) {
    ChatContext* c = (ChatContext*)ctx;

    char* result = c->provider->chat(c->provider, c->model, c->system,
                                      c->user, c->usage);

    if (!result) {
        // Map to error based on usage or other signals
        *out_error = PROVIDER_ERR_UNKNOWN;
    } else {
        *out_error = PROVIDER_OK;
    }

    return result;
}

/**
 * Chat with automatic retry
 */
char* retry_chat(Provider* provider, const char* model, const char* system,
                 const char* user, TokenUsage* usage, ProviderError* out_error) {
    ChatContext ctx = {
        .provider = provider,
        .model = model,
        .system = system,
        .user = user,
        .usage = usage
    };

    return retry_execute(provider->type, chat_wrapper, &ctx, out_error);
}

// ============================================================================
// POLICY CONFIGURATION
// ============================================================================

void retry_set_policy(ProviderType provider, RetryPolicy* policy) {
    if (provider >= 4) return;

    pthread_mutex_lock(&g_retry_mutex);
    g_provider_policies[provider] = *policy;
    pthread_mutex_unlock(&g_retry_mutex);
}

RetryPolicy* retry_get_policy(ProviderType provider) {
    if (provider >= 4) return NULL;
    return &g_provider_policies[provider];
}

void retry_set_max_retries(ProviderType provider, int max_retries) {
    if (provider >= 4) return;

    pthread_mutex_lock(&g_retry_mutex);
    g_provider_policies[provider].max_retries = max_retries;
    pthread_mutex_unlock(&g_retry_mutex);
}

void retry_set_base_delay(ProviderType provider, int base_delay_ms) {
    if (provider >= 4) return;

    pthread_mutex_lock(&g_retry_mutex);
    g_provider_policies[provider].base_delay_ms = base_delay_ms;
    pthread_mutex_unlock(&g_retry_mutex);
}

// ============================================================================
// CIRCUIT BREAKER CONFIGURATION
// ============================================================================

void retry_set_circuit_threshold(ProviderType provider, int failure_threshold,
                                  int success_threshold) {
    if (provider >= 4) return;

    pthread_mutex_lock(&g_retry_mutex);
    g_circuits[provider].failure_threshold = failure_threshold;
    g_circuits[provider].success_threshold = success_threshold;
    pthread_mutex_unlock(&g_retry_mutex);
}

void retry_reset_circuit(ProviderType provider) {
    if (provider >= 4) return;

    pthread_mutex_lock(&g_retry_mutex);
    circuit_init(&g_circuits[provider]);
    pthread_mutex_unlock(&g_retry_mutex);
}

CircuitState retry_get_circuit_state(ProviderType provider) {
    if (provider >= 4) return CIRCUIT_CLOSED;

    pthread_mutex_lock(&g_retry_mutex);
    CircuitState state = g_circuits[provider].state;
    pthread_mutex_unlock(&g_retry_mutex);

    return state;
}

// ============================================================================
// STATISTICS
// ============================================================================

char* retry_stats_json(ProviderType provider) {
    if (provider >= 4) return NULL;

    pthread_mutex_lock(&g_retry_mutex);

    RetryStats* stats = &g_stats[provider];
    CircuitBreaker* circuit = &g_circuits[provider];

    char* json = malloc(512);
    if (!json) {
        pthread_mutex_unlock(&g_retry_mutex);
        return NULL;
    }

    const char* state_str;
    switch (circuit->state) {
        case CIRCUIT_CLOSED: state_str = "closed"; break;
        case CIRCUIT_OPEN: state_str = "open"; break;
        case CIRCUIT_HALF_OPEN: state_str = "half_open"; break;
        default: state_str = "unknown";
    }

    double success_rate = stats->total_requests > 0
        ? (double)stats->successful_requests / stats->total_requests * 100.0
        : 0.0;

    double avg_delay = stats->total_retries > 0
        ? stats->total_delay_ms / stats->total_retries
        : 0.0;

    snprintf(json, 512,
        "{"
        "\"total_requests\":%llu,"
        "\"successful\":%llu,"
        "\"failed\":%llu,"
        "\"retried\":%llu,"
        "\"total_retries\":%llu,"
        "\"circuit_rejections\":%llu,"
        "\"success_rate\":%.2f,"
        "\"avg_retry_delay_ms\":%.2f,"
        "\"circuit_state\":\"%s\""
        "}",
        (unsigned long long)stats->total_requests,
        (unsigned long long)stats->successful_requests,
        (unsigned long long)stats->failed_requests,
        (unsigned long long)stats->retried_requests,
        (unsigned long long)stats->total_retries,
        (unsigned long long)stats->circuit_rejections,
        success_rate,
        avg_delay,
        state_str);

    pthread_mutex_unlock(&g_retry_mutex);
    return json;
}

void retry_reset_stats(ProviderType provider) {
    if (provider >= 4) return;

    pthread_mutex_lock(&g_retry_mutex);
    memset(&g_stats[provider], 0, sizeof(RetryStats));
    pthread_mutex_unlock(&g_retry_mutex);
}
