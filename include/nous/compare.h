/**
 * CONVERGIO MODEL COMPARISON & BENCHMARK
 *
 * Execute prompts across multiple models/providers in parallel
 * and compare responses with metrics and diff analysis.
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_COMPARE_H
#define CONVERGIO_COMPARE_H

#include <stddef.h>
#include <stdbool.h>

// ============================================================================
// COMPARISON RESULT
// ============================================================================

typedef struct {
    char* model_id;              // Full model ID (e.g., "anthropic/claude-opus-4.5")
    char* response;              // Full response text
    double time_ms;              // Response time in milliseconds
    size_t tokens_in;            // Input tokens consumed
    size_t tokens_out;           // Output tokens generated
    double cost;                 // Total cost in USD
    bool success;                // Whether request succeeded
    char* error;                 // Error message if failed (NULL if success)
} CompareResult;

// ============================================================================
// COMPARISON MODES
// ============================================================================

typedef enum {
    COMPARE_MODE_PARALLEL,       // Execute all models in parallel (default)
    COMPARE_MODE_SEQUENTIAL      // Execute models one at a time
} CompareMode;

typedef struct {
    CompareMode mode;
    bool show_diff;              // Show text diff between responses
    bool show_metrics;           // Show performance metrics
    bool show_cost;              // Show cost breakdown
    const char* output_format;   // "table" or "json"
} CompareOptions;

// ============================================================================
// CORE COMPARISON FUNCTIONS
// ============================================================================

/**
 * Run comparison across multiple models
 * @param prompt The prompt to send to all models
 * @param system System prompt (can be NULL)
 * @param models Array of model IDs to compare
 * @param model_count Number of models
 * @param options Comparison options
 * @param results Output array of results (caller must free with compare_results_free)
 * @return 0 on success, -1 on error
 */
int compare_models(const char* prompt, const char* system,
                   const char** models, size_t model_count,
                   const CompareOptions* options,
                   CompareResult** results);

/**
 * Run benchmark: measure performance of a single model
 * @param prompt The prompt to benchmark
 * @param system System prompt (can be NULL)
 * @param model Model ID to benchmark
 * @param iterations Number of iterations to run
 * @param result Output result with average metrics (caller must free)
 * @return 0 on success, -1 on error
 */
int benchmark_model(const char* prompt, const char* system,
                    const char* model, size_t iterations,
                    CompareResult* result);

/**
 * Free comparison results
 * @param results Array of results to free
 * @param count Number of results
 */
void compare_results_free(CompareResult* results, size_t count);

// ============================================================================
// RENDERING FUNCTIONS
// ============================================================================

/**
 * Render comparison results as a formatted table
 * @param results Array of comparison results
 * @param count Number of results
 * @param options Comparison options (controls what to display)
 */
void render_comparison_table(const CompareResult* results, size_t count,
                              const CompareOptions* options);

/**
 * Render comparison results as JSON
 * @param results Array of comparison results
 * @param count Number of results
 * @return JSON string (caller must free)
 */
char* render_comparison_json(const CompareResult* results, size_t count);

/**
 * Render performance metrics chart (ASCII bar chart)
 * @param results Array of comparison results
 * @param count Number of results
 */
void render_metrics_chart(const CompareResult* results, size_t count);

// ============================================================================
// DIFF FUNCTIONS
// ============================================================================

/**
 * Generate line-by-line diff between two responses
 * @param response1 First response
 * @param response2 Second response
 * @param label1 Label for first response (e.g., model name)
 * @param label2 Label for second response
 * @return Diff string in unified diff format (caller must free)
 */
char* generate_response_diff(const char* response1, const char* response2,
                              const char* label1, const char* label2);

/**
 * Display diff between all pairs of responses
 * @param results Array of comparison results
 * @param count Number of results
 */
void display_all_diffs(const CompareResult* results, size_t count);

// ============================================================================
// PARALLEL EXECUTION
// ============================================================================

/**
 * Execute multiple model requests in parallel using pthreads
 * @param prompt The prompt to send
 * @param system System prompt (can be NULL)
 * @param models Array of model IDs
 * @param model_count Number of models
 * @param results Output array (caller must allocate with size model_count)
 * @return 0 on success, -1 on error
 */
int parallel_execute(const char* prompt, const char* system,
                     const char** models, size_t model_count,
                     CompareResult* results);

// ============================================================================
// DEFAULT OPTIONS
// ============================================================================

/**
 * Get default comparison options
 * @return Default options structure
 */
CompareOptions compare_options_default(void);

#endif // CONVERGIO_COMPARE_H
