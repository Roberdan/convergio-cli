/**
 * CONVERGIO MODEL COMPARISON - Parallel Execution
 *
 * Execute multiple model requests in parallel using pthreads
 */

#include "nous/compare.h"
#include "nous/nous.h"
#include "nous/provider.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// THREAD CONTEXT
// ============================================================================

typedef struct {
    const char* prompt;
    const char* system;
    const char* model_id;
    CompareResult* result;
    pthread_t thread;
} ThreadContext;

// ============================================================================
// THREAD WORKER FUNCTION
// ============================================================================

static void* execute_model_request(void* arg) {
    ThreadContext* ctx = (ThreadContext*)arg;
    CompareResult* res = ctx->result;

    // Initialize result
    res->model_id = strdup(ctx->model_id);
    res->response = NULL;
    res->error = NULL;
    res->success = false;
    res->time_ms = 0.0;
    res->tokens_in = 0;
    res->tokens_out = 0;
    res->cost = 0.0;

    // Get model config
    const ModelConfig* model_cfg = model_get_config(ctx->model_id);
    if (!model_cfg) {
        res->error = strdup("Model not found");
        LOG_WARN(LOG_CAT_SYSTEM, "Model not found: %s", ctx->model_id);
        return NULL;
    }

    // Get provider and initialize if needed
    Provider* provider = provider_get(model_cfg->provider);
    if (!provider) {
        res->error = strdup("Provider not available");
        LOG_WARN(LOG_CAT_SYSTEM, "Provider not found for model: %s", ctx->model_id);
        return NULL;
    }
    if (!provider->initialized && provider->init) {
        ProviderError err = provider->init(provider);
        if (err != PROVIDER_OK) {
            res->error = strdup("Provider initialization failed");
            LOG_WARN(LOG_CAT_SYSTEM, "Failed to init provider for model: %s", ctx->model_id);
            return NULL;
        }
    }
    if (!provider->initialized) {
        res->error = strdup("Provider not initialized");
        LOG_WARN(LOG_CAT_SYSTEM, "Provider not initialized for model: %s", ctx->model_id);
        return NULL;
    }

    // Execute request with timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    TokenUsage usage = {0};
    char* response = provider->chat(provider, model_cfg->id, ctx->system, ctx->prompt, &usage);

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate elapsed time
    double elapsed_ms =
        (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

    // Process result
    if (response) {
        res->success = true;
        res->response = response;
        res->time_ms = elapsed_ms;
        res->tokens_in = usage.input_tokens;
        res->tokens_out = usage.output_tokens;
        res->cost = usage.estimated_cost;

        LOG_INFO(LOG_CAT_SYSTEM, "Thread completed: %s (%.2fms, $%.4f)", ctx->model_id, elapsed_ms,
                 usage.estimated_cost);
    } else {
        // Get error from provider
        ProviderErrorInfo* err = provider->get_last_error(provider);
        if (err && err->message) {
            res->error = strdup(err->message);
        } else {
            res->error = strdup("Unknown error");
        }
        LOG_ERROR(LOG_CAT_SYSTEM, "Thread failed: %s - %s", ctx->model_id, res->error);
    }

    return NULL;
}

// ============================================================================
// PARALLEL EXECUTION
// ============================================================================

int parallel_execute(const char* prompt, const char* system, const char** models,
                     size_t model_count, CompareResult* results) {
    if (!prompt || !models || model_count == 0 || !results) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Invalid arguments to parallel_execute");
        return -1;
    }

    // Allocate thread contexts
    ThreadContext* contexts = calloc(model_count, sizeof(ThreadContext));
    if (!contexts) {
        LOG_ERROR(LOG_CAT_SYSTEM, "Failed to allocate thread contexts");
        return -1;
    }

    // Create threads
    LOG_INFO(LOG_CAT_SYSTEM, "Spawning %zu threads for parallel execution", model_count);

    for (size_t i = 0; i < model_count; i++) {
        contexts[i].prompt = prompt;
        contexts[i].system = system;
        contexts[i].model_id = models[i];
        contexts[i].result = &results[i];

        int ret = pthread_create(&contexts[i].thread, NULL, execute_model_request, &contexts[i]);
        if (ret != 0) {
            LOG_ERROR(LOG_CAT_SYSTEM, "Failed to create thread for model: %s", models[i]);
            // Mark as failed
            results[i].model_id = strdup(models[i]);
            results[i].error = strdup("Failed to create thread");
            results[i].success = false;
        } else {
            LOG_DEBUG(LOG_CAT_SYSTEM, "Thread spawned for: %s", models[i]);
        }
    }

    // Wait for all threads to complete
    LOG_INFO(LOG_CAT_SYSTEM, "Waiting for all threads to complete...");

    for (size_t i = 0; i < model_count; i++) {
        if (contexts[i].thread) {
            pthread_join(contexts[i].thread, NULL);
            LOG_DEBUG(LOG_CAT_SYSTEM, "Thread joined: %s", models[i]);
        }
    }

    free(contexts);

    LOG_INFO(LOG_CAT_SYSTEM, "All threads completed");

    // Count successes
    size_t success_count = 0;
    for (size_t i = 0; i < model_count; i++) {
        if (results[i].success) {
            success_count++;
        }
    }

    LOG_INFO(LOG_CAT_SYSTEM, "Success rate: %zu/%zu (%.1f%%)", success_count, model_count,
             (success_count * 100.0) / model_count);

    return 0;
}
