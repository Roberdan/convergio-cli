/**
 * CONVERGIO MODEL COMPARISON - Core Implementation
 *
 * Main comparison logic and result management
 */

#include "nous/compare.h"
#include "nous/provider.h"
#include "nous/nous.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// External parallel execution function
extern int parallel_execute(const char* prompt, const char* system,
                            const char** models, size_t model_count,
                            CompareResult* results);

// External rendering functions
extern void render_comparison_table(const CompareResult* results, size_t count,
                                    const CompareOptions* options);
extern char* render_comparison_json(const CompareResult* results, size_t count);
extern void display_all_diffs(const CompareResult* results, size_t count);

// ============================================================================
// DEFAULT OPTIONS
// ============================================================================

CompareOptions compare_options_default(void) {
    CompareOptions opts = {
        .mode = COMPARE_MODE_PARALLEL,
        .show_diff = true,
        .show_metrics = true,
        .show_cost = true,
        .output_format = "table"
    };
    return opts;
}

// ============================================================================
// RESULT MANAGEMENT
// ============================================================================

void compare_results_free(CompareResult* results, size_t count) {
    if (!results) return;

    for (size_t i = 0; i < count; i++) {
        free(results[i].model_id);
        free(results[i].response);
        free(results[i].error);
    }
    free(results);
}

// ============================================================================
// SEQUENTIAL EXECUTION (FALLBACK)
// ============================================================================

static int sequential_execute(const char* prompt, const char* system,
                              const char** models, size_t model_count,
                              CompareResult* results) {
    for (size_t i = 0; i < model_count; i++) {
        CompareResult* res = &results[i];

        // Initialize result
        res->model_id = strdup(models[i]);
        res->response = NULL;
        res->error = NULL;
        res->success = false;
        res->time_ms = 0.0;
        res->tokens_in = 0;
        res->tokens_out = 0;
        res->cost = 0.0;

        // Get model config
        const ModelConfig* model_cfg = model_get_config(models[i]);
        if (!model_cfg) {
            res->error = strdup("Model not found");
            LOG_WARN(LOG_CAT_SYSTEM, "Model not found: %s", models[i]);
            continue;
        }

        // Get provider and initialize if needed
        Provider* provider = provider_get(model_cfg->provider);
        if (!provider) {
            res->error = strdup("Provider not available");
            LOG_WARN(LOG_CAT_SYSTEM, "Provider not found for model: %s", models[i]);
            continue;
        }
        if (!provider->initialized && provider->init) {
            ProviderError err = provider->init(provider);
            if (err != PROVIDER_OK) {
                res->error = strdup("Provider initialization failed");
                LOG_WARN(LOG_CAT_SYSTEM, "Failed to init provider for model: %s", models[i]);
                continue;
            }
        }
        if (!provider->initialized) {
            res->error = strdup("Provider not initialized");
            LOG_WARN(LOG_CAT_SYSTEM, "Provider not initialized for model: %s", models[i]);
            continue;
        }

        // Execute request
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        TokenUsage usage = {0};
        char* response = provider->chat(provider, model_cfg->id, system, prompt, &usage);

        clock_gettime(CLOCK_MONOTONIC, &end);

        double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                           (end.tv_nsec - start.tv_nsec) / 1000000.0;

        if (response) {
            res->success = true;
            res->response = response;
            res->time_ms = elapsed_ms;
            res->tokens_in = usage.input_tokens;
            res->tokens_out = usage.output_tokens;
            res->cost = usage.estimated_cost;

            LOG_INFO(LOG_CAT_SYSTEM, "Completed: %s (%.2fms, $%.4f)",
                     models[i], elapsed_ms, usage.estimated_cost);
        } else {
            ProviderErrorInfo* err = provider->get_last_error(provider);
            if (err && err->message) {
                res->error = strdup(err->message);
            } else {
                res->error = strdup("Unknown error");
            }
            LOG_ERROR(LOG_CAT_SYSTEM, "Failed: %s - %s", models[i], res->error);
        }
    }

    return 0;
}

// ============================================================================
// MAIN COMPARISON FUNCTION
// ============================================================================

int compare_models(const char* prompt, const char* system,
                   const char** models, size_t model_count,
                   const CompareOptions* options,
                   CompareResult** results) {
    if (!prompt || !models || model_count == 0 || !results) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Invalid arguments to compare_models");
        return -1;
    }

    // Ensure provider registry is initialized
    ProviderError reg_err = provider_registry_init();
    if (reg_err != PROVIDER_OK) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Failed to initialize provider registry");
        return -1;
    }

    // Use default options if not provided
    CompareOptions opts = options ? *options : compare_options_default();

    // Allocate results array
    CompareResult* res = calloc(model_count, sizeof(CompareResult));
    if (!res) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Failed to allocate results array");
        return -1;
    }

    // Execute based on mode
    int ret;
    if (opts.mode == COMPARE_MODE_PARALLEL) {
        LOG_INFO(LOG_CAT_SYSTEM, "Starting parallel execution across %zu models", model_count);
        ret = parallel_execute(prompt, system, models, model_count, res);
    } else {
        LOG_INFO(LOG_CAT_SYSTEM, "Starting sequential execution across %zu models", model_count);
        ret = sequential_execute(prompt, system, models, model_count, res);
    }

    if (ret != 0) {
        compare_results_free(res, model_count);
        return -1;
    }

    *results = res;

    // Display results based on output format
    if (strcmp(opts.output_format, "json") == 0) {
        char* json = render_comparison_json(res, model_count);
        if (json) {
            printf("%s\n", json);
            free(json);
        }
    } else {
        render_comparison_table(res, model_count, &opts);

        if (opts.show_diff) {
            display_all_diffs(res, model_count);
        }
    }

    return 0;
}

// ============================================================================
// BENCHMARK FUNCTION
// ============================================================================

int benchmark_model(const char* prompt, const char* system,
                    const char* model, size_t iterations,
                    CompareResult* result) {
    if (!prompt || !model || iterations == 0 || !result) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Invalid arguments to benchmark_model");
        return -1;
    }

    // Ensure provider registry is initialized
    ProviderError reg_err = provider_registry_init();
    if (reg_err != PROVIDER_OK) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Failed to initialize provider registry");
        return -1;
    }

    // Initialize result
    memset(result, 0, sizeof(CompareResult));
    result->model_id = strdup(model);

    // Get model config
    const ModelConfig* model_cfg = model_get_config(model);
    if (!model_cfg) {
        result->error = strdup("Model not found");
        return -1;
    }

    // Get provider and initialize if needed
    Provider* provider = provider_get(model_cfg->provider);
    if (!provider) {
        result->error = strdup("Provider not available");
        return -1;
    }
    if (!provider->initialized && provider->init) {
        ProviderError err = provider->init(provider);
        if (err != PROVIDER_OK) {
            result->error = strdup("Provider initialization failed");
            return -1;
        }
    }
    if (!provider->initialized) {
        result->error = strdup("Provider not initialized");
        return -1;
    }

    // Run iterations
    double total_time_ms = 0.0;
    size_t total_tokens_in = 0;
    size_t total_tokens_out = 0;
    double total_cost = 0.0;
    size_t success_count = 0;

    LOG_INFO(LOG_CAT_SYSTEM, "Starting benchmark: %zu iterations of %s", iterations, model);

    for (size_t i = 0; i < iterations; i++) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        TokenUsage usage = {0};
        char* response = provider->chat(provider, model_cfg->id, system, prompt, &usage);

        clock_gettime(CLOCK_MONOTONIC, &end);

        double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                           (end.tv_nsec - start.tv_nsec) / 1000000.0;

        if (response) {
            success_count++;
            total_time_ms += elapsed_ms;
            total_tokens_in += usage.input_tokens;
            total_tokens_out += usage.output_tokens;
            total_cost += usage.estimated_cost;

            // Keep the last successful response
            if (result->response) {
                free(result->response);
            }
            result->response = response;

            LOG_DEBUG(LOG_CAT_SYSTEM, "Iteration %zu/%zu: %.2fms", i+1, iterations, elapsed_ms);
        } else {
            LOG_WARN(LOG_CAT_SYSTEM, "Iteration %zu/%zu failed", i+1, iterations);
        }
    }

    if (success_count == 0) {
        result->error = strdup("All iterations failed");
        return -1;
    }

    // Calculate averages
    result->success = true;
    result->time_ms = total_time_ms / success_count;
    result->tokens_in = total_tokens_in / success_count;
    result->tokens_out = total_tokens_out / success_count;
    result->cost = total_cost / success_count;

    LOG_INFO(LOG_CAT_SYSTEM, "Benchmark complete: %s (avg %.2fms, $%.4f)",
             model, result->time_ms, result->cost);

    // Display benchmark results
    printf("\n");
    printf("=== Benchmark Results: %s ===\n", model);
    printf("  Iterations:    %zu\n", iterations);
    printf("  Success rate:  %zu/%zu (%.1f%%)\n", success_count, iterations,
           (success_count * 100.0) / iterations);
    printf("  Avg time:      %.2f ms\n", result->time_ms);
    printf("  Avg tokens in: %zu\n", result->tokens_in);
    printf("  Avg tokens out: %zu\n", result->tokens_out);
    printf("  Avg cost:      $%.4f\n", result->cost);
    printf("\n");

    return 0;
}
