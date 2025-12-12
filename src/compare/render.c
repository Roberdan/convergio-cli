/**
 * CONVERGIO MODEL COMPARISON - Rendering
 *
 * Table and chart rendering for comparison results
 */

#include "nous/compare.h"
#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static void print_separator(size_t width) {
    for (size_t i = 0; i < width; i++) {
        printf("─");
    }
    printf("\n");
}

static const char* truncate_string(const char* str, size_t max_len, char* buffer, size_t buf_size) {
    if (!str) return "(null)";

    size_t len = strlen(str);
    if (len <= max_len) {
        return str;
    }

    // Truncate and add ellipsis
    size_t copy_len = (max_len - 3 < buf_size - 4) ? max_len - 3 : buf_size - 4;
    strncpy(buffer, str, copy_len);
    buffer[copy_len] = '\0';
    strcat(buffer, "...");
    return buffer;
}

// ============================================================================
// TABLE RENDERING
// ============================================================================

void render_comparison_table(const CompareResult* results, size_t count,
                              const CompareOptions* options) {
    if (!results || count == 0) {
        printf("No results to display.\n");
        return;
    }

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                       MODEL COMPARISON RESULTS                            ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Print summary table
    printf("┌────────────────────────────────┬─────────┬──────────┬──────────┬──────────┐\n");
    printf("│ Model                          │ Status  │ Time(ms) │ Tokens   │ Cost($)  │\n");
    printf("├────────────────────────────────┼─────────┼──────────┼──────────┼──────────┤\n");

    for (size_t i = 0; i < count; i++) {
        const CompareResult* res = &results[i];

        char model_buf[32];
        const char* model_name = truncate_string(res->model_id, 28, model_buf, sizeof(model_buf));

        if (res->success) {
            printf("│ %-30s │ \033[32m✓\033[0m      │ %8.1f │ %8zu │ %8.4f │\n",
                   model_name,
                   res->time_ms,
                   res->tokens_out,
                   res->cost);
        } else {
            char err_buf[32];
            const char* err_msg = truncate_string(res->error ? res->error : "Unknown", 28, err_buf, sizeof(err_buf));
            printf("│ %-30s │ \033[31m✗\033[0m      │ %8s │ %8s │ %8s │\n",
                   model_name, "-", "-", "-");
            printf("│   Error: %-51s │\n", err_msg);
        }
    }

    printf("└────────────────────────────────┴─────────┴──────────┴──────────┴──────────┘\n");

    // Show detailed metrics if requested
    if (options && options->show_metrics) {
        printf("\n");
        render_metrics_chart(results, count);
    }

    // Print responses
    printf("\n");
    printf("═══ RESPONSES ═══\n");
    printf("\n");

    for (size_t i = 0; i < count; i++) {
        const CompareResult* res = &results[i];

        if (res->success && res->response) {
            printf("\033[1m► %s\033[0m\n", res->model_id);
            print_separator(78);
            printf("%s\n", res->response);
            print_separator(78);
            printf("\n");
        }
    }
}

// ============================================================================
// METRICS CHART
// ============================================================================

void render_metrics_chart(const CompareResult* results, size_t count) {
    if (!results || count == 0) return;

    // Find max values for scaling
    double max_time = 0.0;
    double max_cost = 0.0;

    for (size_t i = 0; i < count; i++) {
        if (results[i].success) {
            if (results[i].time_ms > max_time) max_time = results[i].time_ms;
            if (results[i].cost > max_cost) max_cost = results[i].cost;
        }
    }

    if (max_time == 0.0 && max_cost == 0.0) {
        printf("No metrics to display.\n");
        return;
    }

    printf("Performance Metrics:\n");
    printf("\n");

    // Response time chart
    if (max_time > 0.0) {
        printf("Response Time (ms):\n");
        for (size_t i = 0; i < count; i++) {
            if (results[i].success) {
                char model_buf[24];
                const char* model_name = truncate_string(results[i].model_id, 20, model_buf, sizeof(model_buf));
                printf("  %-22s ", model_name);

                int bar_width = (int)((results[i].time_ms / max_time) * 40);
                for (int j = 0; j < bar_width; j++) {
                    printf("█");
                }
                printf(" %.1f ms\n", results[i].time_ms);
            }
        }
        printf("\n");
    }

    // Cost chart
    if (max_cost > 0.0) {
        printf("Cost ($):\n");
        for (size_t i = 0; i < count; i++) {
            if (results[i].success) {
                char model_buf[24];
                const char* model_name = truncate_string(results[i].model_id, 20, model_buf, sizeof(model_buf));
                printf("  %-22s ", model_name);

                int bar_width = (int)((results[i].cost / max_cost) * 40);
                for (int j = 0; j < bar_width; j++) {
                    printf("█");
                }
                printf(" $%.4f\n", results[i].cost);
            }
        }
        printf("\n");
    }
}

// ============================================================================
// JSON RENDERING
// ============================================================================

static void json_escape_string(const char* str, char* out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; str[i] && j < out_size - 2; i++) {
        switch (str[i]) {
            case '"':  out[j++] = '\\'; out[j++] = '"'; break;
            case '\\': out[j++] = '\\'; out[j++] = '\\'; break;
            case '\n': out[j++] = '\\'; out[j++] = 'n'; break;
            case '\r': out[j++] = '\\'; out[j++] = 'r'; break;
            case '\t': out[j++] = '\\'; out[j++] = 't'; break;
            default:   out[j++] = str[i]; break;
        }
    }
    out[j] = '\0';
}

char* render_comparison_json(const CompareResult* results, size_t count) {
    if (!results || count == 0) return NULL;

    // Estimate size (rough calculation)
    size_t size = 1024 + (count * 4096);
    char* json = malloc(size);
    if (!json) return NULL;

    size_t pos = 0;
    pos += snprintf(json + pos, size - pos, "{\n  \"results\": [\n");

    for (size_t i = 0; i < count; i++) {
        const CompareResult* res = &results[i];

        pos += snprintf(json + pos, size - pos, "    {\n");
        pos += snprintf(json + pos, size - pos, "      \"model\": \"%s\",\n", res->model_id ? res->model_id : "");
        pos += snprintf(json + pos, size - pos, "      \"success\": %s,\n", res->success ? "true" : "false");

        if (res->success) {
            pos += snprintf(json + pos, size - pos, "      \"time_ms\": %.2f,\n", res->time_ms);
            pos += snprintf(json + pos, size - pos, "      \"tokens_in\": %zu,\n", res->tokens_in);
            pos += snprintf(json + pos, size - pos, "      \"tokens_out\": %zu,\n", res->tokens_out);
            pos += snprintf(json + pos, size - pos, "      \"cost\": %.4f,\n", res->cost);

            if (res->response) {
                char escaped[4096];
                json_escape_string(res->response, escaped, sizeof(escaped));
                pos += snprintf(json + pos, size - pos, "      \"response\": \"%s\"\n", escaped);
            } else {
                pos += snprintf(json + pos, size - pos, "      \"response\": null\n");
            }
        } else {
            if (res->error) {
                char escaped[512];
                json_escape_string(res->error, escaped, sizeof(escaped));
                pos += snprintf(json + pos, size - pos, "      \"error\": \"%s\"\n", escaped);
            } else {
                pos += snprintf(json + pos, size - pos, "      \"error\": \"Unknown error\"\n");
            }
        }

        pos += snprintf(json + pos, size - pos, "    }%s\n", (i < count - 1) ? "," : "");
    }

    pos += snprintf(json + pos, size - pos, "  ]\n}\n");

    return json;
}
