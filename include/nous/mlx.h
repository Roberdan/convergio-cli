/**
 * CONVERGIO MLX LOCAL PROVIDER
 *
 * Apple Silicon native LLM inference using MLX framework.
 * Provides 100% offline operation with zero external dependencies.
 *
 * Supported models:
 * - Llama 3.2 (1B, 3B)
 * - Phi-3 Mini
 * - Mistral 7B Q4
 * - Llama 3.1 8B Q4
 *
 * Requirements:
 * - Apple Silicon (M1/M2/M3/M4/M5)
 * - macOS Tahoe 26+ recommended
 * - 8GB+ unified memory (16GB+ for larger models)
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#ifndef CONVERGIO_MLX_H
#define CONVERGIO_MLX_H

#include "nous/provider.h"
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// MLX MODEL INFORMATION
// ============================================================================

typedef struct {
    const char* id;              // e.g., "llama-3.2-3b"
    const char* display_name;    // e.g., "Llama 3.2 3B"
    const char* huggingface_id;  // e.g., "mlx-community/Llama-3.2-3B-Instruct-4bit"
    size_t size_mb;              // Download size in MB
    size_t context_window;       // Max context in tokens
    size_t min_ram_gb;           // Minimum RAM required
    bool supports_tools;         // Tool calling support
    bool supports_italian;       // Italian language support
    const char* best_for;        // Use case description
    const char* sha256;          // Checksum for verification
} MLXModelInfo;

typedef enum {
    MLX_OK = 0,
    MLX_ERR_NOT_APPLE_SILICON,   // Not running on Apple Silicon
    MLX_ERR_MODEL_NOT_FOUND,     // Model not downloaded
    MLX_ERR_MODEL_CORRUPT,       // Model checksum mismatch
    MLX_ERR_OUT_OF_MEMORY,       // Insufficient RAM
    MLX_ERR_LOAD_FAILED,         // Failed to load model
    MLX_ERR_INFERENCE_FAILED,    // Inference error
    MLX_ERR_TOKENIZER_FAILED,    // Tokenizer error
    MLX_ERR_UNKNOWN
} MLXError;

// ============================================================================
// MLX PROVIDER STATE
// ============================================================================

typedef struct {
    bool initialized;
    bool model_loaded;
    char* current_model_id;
    char* model_path;
    size_t context_used;
    void* mlx_model;             // Opaque MLX model handle
    void* tokenizer;             // Opaque tokenizer handle
    MLXError last_error;
    char* last_error_message;
} MLXProviderData;

// ============================================================================
// MLX PROVIDER API
// ============================================================================

/**
 * Create MLX provider instance
 * @return Provider instance or NULL if not on Apple Silicon
 */
Provider* mlx_provider_create(void);

/**
 * Check if MLX is available on this system
 * @return true if running on Apple Silicon with MLX support
 */
bool mlx_is_available(void);

/**
 * Get list of available MLX models
 * @param out_count Number of models
 * @return Array of model info (do not free)
 */
const MLXModelInfo* mlx_get_available_models(size_t* out_count);

/**
 * Check if a model is downloaded and ready
 * @param model_id Model ID (e.g., "llama-3.2-3b")
 * @return true if model is ready to use
 */
bool mlx_model_is_ready(const char* model_id);

/**
 * Get model download path
 * @param model_id Model ID
 * @return Path to model directory or NULL if not found
 */
const char* mlx_get_model_path(const char* model_id);

/**
 * Download a model with visible progress bar
 * @param huggingface_id HuggingFace model ID (e.g., "mlx-community/Llama-3.2-3B-Instruct-4bit")
 * @return MLX_OK on success
 */
MLXError mlx_download_model_with_progress(const char* huggingface_id);

/**
 * Load a model for inference (auto-downloads if needed with progress)
 * @param model_id Model ID
 * @return MLX_OK on success
 */
MLXError mlx_load_model(const char* model_id);

/**
 * Unload current model to free memory
 */
void mlx_unload_model(void);

/**
 * Get current loaded model info
 * @return Model info or NULL if no model loaded
 */
const MLXModelInfo* mlx_get_current_model(void);

/**
 * Run inference with loaded model
 * @param prompt Input prompt
 * @param system System prompt (can be NULL)
 * @param max_tokens Maximum output tokens
 * @param temperature Sampling temperature (0.0-2.0)
 * @param out_response Output response (caller must free)
 * @return MLX_OK on success
 */
MLXError mlx_generate(
    const char* prompt,
    const char* system,
    size_t max_tokens,
    float temperature,
    char** out_response
);

/**
 * Run streaming inference
 * @param prompt Input prompt
 * @param system System prompt (can be NULL)
 * @param max_tokens Maximum output tokens
 * @param temperature Sampling temperature
 * @param on_token Callback for each token
 * @param user_ctx User context for callback
 * @return MLX_OK on success
 */
MLXError mlx_generate_stream(
    const char* prompt,
    const char* system,
    size_t max_tokens,
    float temperature,
    void (*on_token)(const char* token, void* ctx),
    void* user_ctx
);

/**
 * Estimate tokens for text
 * @param text Input text
 * @return Estimated token count
 */
size_t mlx_estimate_tokens(const char* text);

/**
 * Get human-readable error message
 * @param error Error code
 * @return Error message
 */
const char* mlx_error_message(MLXError error);

/**
 * Get recommended model based on available RAM
 * @param available_ram_gb Available RAM in GB
 * @return Recommended model ID or NULL
 */
const char* mlx_recommend_model(size_t available_ram_gb);

#endif // CONVERGIO_MLX_H
