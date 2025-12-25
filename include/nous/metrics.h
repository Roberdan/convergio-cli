/**
 * CONVERGIO METRICS LAYER (REF-04)
 *
 * Adds metrics collection on top of existing telemetry:
 * - Counters (monotonic, e.g., request_count)
 * - Gauges (point-in-time, e.g., active_sessions)
 * - Histograms (distribution, e.g., response_time_ms)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef NOUS_METRICS_H
#define NOUS_METRICS_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// METRIC TYPES
// ============================================================================

typedef enum {
    METRIC_TYPE_COUNTER,      // Monotonically increasing
    METRIC_TYPE_GAUGE,        // Point-in-time value
    METRIC_TYPE_HISTOGRAM     // Distribution with buckets
} MetricType;

// ============================================================================
// METRIC LABELS
// ============================================================================

#define METRIC_MAX_LABELS 8
#define METRIC_MAX_LABEL_LEN 64

typedef struct {
    char key[METRIC_MAX_LABEL_LEN];
    char value[METRIC_MAX_LABEL_LEN];
} MetricLabel;

// ============================================================================
// HISTOGRAM BUCKETS
// ============================================================================

#define HISTOGRAM_BUCKET_COUNT 10

typedef struct {
    double upper_bound;
    uint64_t count;
} HistogramBucket;

// ============================================================================
// METRIC STRUCTURE
// ============================================================================

typedef struct {
    char name[128];
    char description[256];
    MetricType type;

    // Labels
    MetricLabel labels[METRIC_MAX_LABELS];
    int label_count;

    // Values (union based on type)
    union {
        uint64_t counter;
        double gauge;
        struct {
            HistogramBucket buckets[HISTOGRAM_BUCKET_COUNT];
            double sum;
            uint64_t count;
        } histogram;
    } value;
} Metric;

// ============================================================================
// METRICS API
// ============================================================================

/**
 * Initialize metrics subsystem
 */
int metrics_init(void);

/**
 * Shutdown metrics subsystem
 */
void metrics_shutdown(void);

/**
 * Counter operations
 */
void metrics_counter_inc(const char* name);
void metrics_counter_add(const char* name, uint64_t value);
uint64_t metrics_counter_get(const char* name);

/**
 * Gauge operations
 */
void metrics_gauge_set(const char* name, double value);
void metrics_gauge_inc(const char* name);
void metrics_gauge_dec(const char* name);
double metrics_gauge_get(const char* name);

/**
 * Histogram operations
 */
void metrics_histogram_observe(const char* name, double value);

/**
 * Register a new metric
 */
int metrics_register(const char* name, MetricType type, const char* description);

/**
 * Add label to metric
 */
int metrics_add_label(const char* name, const char* label_key, const char* label_value);

/**
 * Export metrics in Prometheus format
 */
char* metrics_export_prometheus(void);

/**
 * Export metrics as JSON
 */
char* metrics_export_json(void);

// ============================================================================
// CORRELATION IDS
// ============================================================================

/**
 * Generate new correlation ID for request tracing
 */
const char* metrics_new_correlation_id(void);

/**
 * Get current correlation ID (thread-local)
 */
const char* metrics_get_correlation_id(void);

/**
 * Set correlation ID (for propagation)
 */
void metrics_set_correlation_id(const char* id);

// ============================================================================
// AUTO-INSTRUMENTATION MACROS
// ============================================================================

#define METRICS_TIME_START() \
    struct timespec _metrics_start; \
    clock_gettime(CLOCK_MONOTONIC, &_metrics_start)

#define METRICS_TIME_END(metric_name) do { \
    struct timespec _metrics_end; \
    clock_gettime(CLOCK_MONOTONIC, &_metrics_end); \
    double _ms = (_metrics_end.tv_sec - _metrics_start.tv_sec) * 1000.0 + \
                 (_metrics_end.tv_nsec - _metrics_start.tv_nsec) / 1000000.0; \
    metrics_histogram_observe(metric_name, _ms); \
} while(0)

// ============================================================================
// WELL-KNOWN METRICS
// ============================================================================

#define METRIC_LLM_REQUESTS         "convergio_llm_requests_total"
#define METRIC_LLM_ERRORS           "convergio_llm_errors_total"
#define METRIC_LLM_LATENCY          "convergio_llm_latency_ms"
#define METRIC_TOKENS_INPUT         "convergio_tokens_input_total"
#define METRIC_TOKENS_OUTPUT        "convergio_tokens_output_total"
#define METRIC_ACTIVE_SESSIONS      "convergio_active_sessions"
#define METRIC_AGENT_INVOCATIONS    "convergio_agent_invocations_total"
#define METRIC_TOOL_EXECUTIONS      "convergio_tool_executions_total"

#endif // NOUS_METRICS_H
