/**
 * CONVERGIO METRICS LAYER - Implementation (REF-04)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#include "nous/metrics.h"
#include "nous/nous.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uuid/uuid.h>

// ============================================================================
// METRICS STORAGE
// ============================================================================

#define MAX_METRICS 256

static Metric g_metrics[MAX_METRICS];
static int g_metric_count = 0;
static pthread_mutex_t g_metrics_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_metrics_initialized = false;

// Thread-local correlation ID
static pthread_key_t g_correlation_key;
static pthread_once_t g_correlation_once = PTHREAD_ONCE_INIT;

// Default histogram buckets (latency in ms)
static const double DEFAULT_BUCKETS[] = {1, 5, 10, 25, 50, 100, 250, 500, 1000, 5000};

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

static Metric* find_metric(const char* name) {
    for (int i = 0; i < g_metric_count; i++) {
        if (strcmp(g_metrics[i].name, name) == 0) {
            return &g_metrics[i];
        }
    }
    return NULL;
}

static void correlation_key_create(void) {
    pthread_key_create(&g_correlation_key, free);
}

// ============================================================================
// PUBLIC API - INIT/SHUTDOWN
// ============================================================================

int metrics_init(void) {
    if (g_metrics_initialized)
        return 0;

    pthread_once(&g_correlation_once, correlation_key_create);

    // Pre-register common metrics
    metrics_register(METRIC_LLM_REQUESTS, METRIC_TYPE_COUNTER, "Total LLM API requests");
    metrics_register(METRIC_LLM_ERRORS, METRIC_TYPE_COUNTER, "Total LLM API errors");
    metrics_register(METRIC_LLM_LATENCY, METRIC_TYPE_HISTOGRAM, "LLM request latency in ms");
    metrics_register(METRIC_TOKENS_INPUT, METRIC_TYPE_COUNTER, "Total input tokens");
    metrics_register(METRIC_TOKENS_OUTPUT, METRIC_TYPE_COUNTER, "Total output tokens");
    metrics_register(METRIC_ACTIVE_SESSIONS, METRIC_TYPE_GAUGE, "Currently active sessions");
    metrics_register(METRIC_AGENT_INVOCATIONS, METRIC_TYPE_COUNTER, "Total agent invocations");
    metrics_register(METRIC_TOOL_EXECUTIONS, METRIC_TYPE_COUNTER, "Total tool executions");

    g_metrics_initialized = true;
    LOG_INFO(LOG_CAT_SYSTEM, "Metrics subsystem initialized with %d metrics", g_metric_count);
    return 0;
}

void metrics_shutdown(void) {
    pthread_mutex_lock(&g_metrics_mutex);
    g_metric_count = 0;
    g_metrics_initialized = false;
    pthread_mutex_unlock(&g_metrics_mutex);
}

// ============================================================================
// PUBLIC API - REGISTRATION
// ============================================================================

int metrics_register(const char* name, MetricType type, const char* description) {
    pthread_mutex_lock(&g_metrics_mutex);

    if (g_metric_count >= MAX_METRICS) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return -1;
    }

    // Check if already exists
    if (find_metric(name)) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return 0; // Already registered
    }

    Metric* m = &g_metrics[g_metric_count++];
    memset(m, 0, sizeof(Metric));
    strncpy(m->name, name, sizeof(m->name) - 1);
    strncpy(m->description, description, sizeof(m->description) - 1);
    m->type = type;

    // Initialize histogram buckets
    if (type == METRIC_TYPE_HISTOGRAM) {
        for (int i = 0; i < HISTOGRAM_BUCKET_COUNT; i++) {
            m->value.histogram.buckets[i].upper_bound = DEFAULT_BUCKETS[i];
            m->value.histogram.buckets[i].count = 0;
        }
    }

    pthread_mutex_unlock(&g_metrics_mutex);
    return 0;
}

int metrics_add_label(const char* name, const char* label_key, const char* label_value) {
    pthread_mutex_lock(&g_metrics_mutex);

    Metric* m = find_metric(name);
    if (!m || m->label_count >= METRIC_MAX_LABELS) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return -1;
    }

    MetricLabel* label = &m->labels[m->label_count++];
    strncpy(label->key, label_key, METRIC_MAX_LABEL_LEN - 1);
    strncpy(label->value, label_value, METRIC_MAX_LABEL_LEN - 1);

    pthread_mutex_unlock(&g_metrics_mutex);
    return 0;
}

// ============================================================================
// PUBLIC API - COUNTERS
// ============================================================================

void metrics_counter_inc(const char* name) {
    metrics_counter_add(name, 1);
}

void metrics_counter_add(const char* name, uint64_t value) {
    pthread_mutex_lock(&g_metrics_mutex);
    Metric* m = find_metric(name);
    if (m && m->type == METRIC_TYPE_COUNTER) {
        m->value.counter += value;
    }
    pthread_mutex_unlock(&g_metrics_mutex);
}

uint64_t metrics_counter_get(const char* name) {
    pthread_mutex_lock(&g_metrics_mutex);
    Metric* m = find_metric(name);
    uint64_t val = (m && m->type == METRIC_TYPE_COUNTER) ? m->value.counter : 0;
    pthread_mutex_unlock(&g_metrics_mutex);
    return val;
}

// ============================================================================
// PUBLIC API - GAUGES
// ============================================================================

void metrics_gauge_set(const char* name, double value) {
    pthread_mutex_lock(&g_metrics_mutex);
    Metric* m = find_metric(name);
    if (m && m->type == METRIC_TYPE_GAUGE) {
        m->value.gauge = value;
    }
    pthread_mutex_unlock(&g_metrics_mutex);
}

void metrics_gauge_inc(const char* name) {
    pthread_mutex_lock(&g_metrics_mutex);
    Metric* m = find_metric(name);
    if (m && m->type == METRIC_TYPE_GAUGE) {
        m->value.gauge += 1.0;
    }
    pthread_mutex_unlock(&g_metrics_mutex);
}

void metrics_gauge_dec(const char* name) {
    pthread_mutex_lock(&g_metrics_mutex);
    Metric* m = find_metric(name);
    if (m && m->type == METRIC_TYPE_GAUGE) {
        m->value.gauge -= 1.0;
    }
    pthread_mutex_unlock(&g_metrics_mutex);
}

double metrics_gauge_get(const char* name) {
    pthread_mutex_lock(&g_metrics_mutex);
    Metric* m = find_metric(name);
    double val = (m && m->type == METRIC_TYPE_GAUGE) ? m->value.gauge : 0.0;
    pthread_mutex_unlock(&g_metrics_mutex);
    return val;
}

// ============================================================================
// PUBLIC API - HISTOGRAMS
// ============================================================================

void metrics_histogram_observe(const char* name, double value) {
    pthread_mutex_lock(&g_metrics_mutex);
    Metric* m = find_metric(name);
    if (m && m->type == METRIC_TYPE_HISTOGRAM) {
        m->value.histogram.sum += value;
        m->value.histogram.count++;

        // Update bucket counts
        for (int i = 0; i < HISTOGRAM_BUCKET_COUNT; i++) {
            if (value <= m->value.histogram.buckets[i].upper_bound) {
                m->value.histogram.buckets[i].count++;
            }
        }
    }
    pthread_mutex_unlock(&g_metrics_mutex);
}

// ============================================================================
// PUBLIC API - EXPORT
// ============================================================================

char* metrics_export_prometheus(void) {
    pthread_mutex_lock(&g_metrics_mutex);

    size_t buf_size = 16384;
    char* buf = malloc(buf_size);
    if (!buf) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return NULL;
    }

    size_t offset = 0;
    for (int i = 0; i < g_metric_count; i++) {
        Metric* m = &g_metrics[i];

        // Help line
        offset +=
            snprintf(buf + offset, buf_size - offset, "# HELP %s %s\n", m->name, m->description);

        // Type line
        const char* type_str = (m->type == METRIC_TYPE_COUNTER) ? "counter"
                               : (m->type == METRIC_TYPE_GAUGE) ? "gauge"
                                                                : "histogram";
        offset += snprintf(buf + offset, buf_size - offset, "# TYPE %s %s\n", m->name, type_str);

        // Value
        switch (m->type) {
        case METRIC_TYPE_COUNTER:
            offset += snprintf(buf + offset, buf_size - offset, "%s %llu\n", m->name,
                               (unsigned long long)m->value.counter);
            break;
        case METRIC_TYPE_GAUGE:
            offset +=
                snprintf(buf + offset, buf_size - offset, "%s %.2f\n", m->name, m->value.gauge);
            break;
        case METRIC_TYPE_HISTOGRAM:
            for (int j = 0; j < HISTOGRAM_BUCKET_COUNT; j++) {
                offset += snprintf(buf + offset, buf_size - offset, "%s_bucket{le=\"%.0f\"} %llu\n",
                                   m->name, m->value.histogram.buckets[j].upper_bound,
                                   (unsigned long long)m->value.histogram.buckets[j].count);
            }
            offset += snprintf(buf + offset, buf_size - offset, "%s_sum %.2f\n%s_count %llu\n",
                               m->name, m->value.histogram.sum, m->name,
                               (unsigned long long)m->value.histogram.count);
            break;
        }
    }

    pthread_mutex_unlock(&g_metrics_mutex);
    return buf;
}

char* metrics_export_json(void) {
    pthread_mutex_lock(&g_metrics_mutex);

    size_t buf_size = 16384;
    char* buf = malloc(buf_size);
    if (!buf) {
        pthread_mutex_unlock(&g_metrics_mutex);
        return NULL;
    }

    size_t offset = snprintf(buf, buf_size, "{\"metrics\":[");

    for (int i = 0; i < g_metric_count; i++) {
        Metric* m = &g_metrics[i];
        if (i > 0)
            offset += snprintf(buf + offset, buf_size - offset, ",");

        offset +=
            snprintf(buf + offset, buf_size - offset, "{\"name\":\"%s\",\"type\":\"%s\",", m->name,
                     (m->type == METRIC_TYPE_COUNTER) ? "counter"
                     : (m->type == METRIC_TYPE_GAUGE) ? "gauge"
                                                      : "histogram");

        switch (m->type) {
        case METRIC_TYPE_COUNTER:
            offset += snprintf(buf + offset, buf_size - offset, "\"value\":%llu}",
                               (unsigned long long)m->value.counter);
            break;
        case METRIC_TYPE_GAUGE:
            offset += snprintf(buf + offset, buf_size - offset, "\"value\":%.2f}", m->value.gauge);
            break;
        case METRIC_TYPE_HISTOGRAM:
            offset +=
                snprintf(buf + offset, buf_size - offset, "\"sum\":%.2f,\"count\":%llu}",
                         m->value.histogram.sum, (unsigned long long)m->value.histogram.count);
            break;
        }
    }

    snprintf(buf + offset, buf_size - offset, "]}");
    pthread_mutex_unlock(&g_metrics_mutex);
    return buf;
}

// ============================================================================
// PUBLIC API - CORRELATION IDS
// ============================================================================

const char* metrics_new_correlation_id(void) {
    char* id = malloc(37);
    if (!id)
        return NULL;

    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, id);

    // Store in thread-local
    char* old = pthread_getspecific(g_correlation_key);
    if (old)
        free(old);
    pthread_setspecific(g_correlation_key, id);

    return id;
}

const char* metrics_get_correlation_id(void) {
    return pthread_getspecific(g_correlation_key);
}

void metrics_set_correlation_id(const char* id) {
    if (!id)
        return;

    char* copy = strdup(id);
    char* old = pthread_getspecific(g_correlation_key);
    if (old)
        free(old);
    pthread_setspecific(g_correlation_key, copy);
}
