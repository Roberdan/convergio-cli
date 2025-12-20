/**
 * CONVERGIO MLX LOCAL PROVIDER
 *
 * Apple Silicon native LLM inference using MLX framework.
 * Provides 100% offline operation with zero external dependencies.
 *
 * Architecture:
 * - Uses Metal Performance Shaders for GPU acceleration
 * - Uses Accelerate framework for SIMD operations
 * - Models stored in ~/.convergio/models/
 * - Tokenizer: SentencePiece compatible
 *
 * Copyright 2025 - Roberto D'Angelo & AI Team
 */

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <Accelerate/Accelerate.h>
#include "nous/mlx.h"
#include "nous/provider.h"
#include "nous/nous.h"
#include "nous/hardware.h"
#include "nous/telemetry.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <time.h>

// ============================================================================
// SWIFT BRIDGE DECLARATIONS
// These functions are implemented in Sources/ConvergioMLX/MLXBridge.swift
// and exposed via @_cdecl for C linkage
// ============================================================================

extern bool mlx_bridge_is_available(void);
extern bool mlx_bridge_load_model(const char* model_id, const char* cache_path);
extern void mlx_bridge_unload_model(void);
extern char* mlx_bridge_generate(
    const char* prompt,
    const char* system_prompt,
    int32_t max_tokens,
    float temperature,
    int32_t* out_token_count,
    float* out_tokens_per_sec,
    char** out_error
);
extern void mlx_bridge_clear_cache(void);
extern int64_t mlx_bridge_gpu_memory_used(void);

// Model management functions
extern char* mlx_bridge_download_model(const char* model_id, void (*progress_callback)(int32_t));
extern bool mlx_bridge_model_exists(const char* model_id);
extern int64_t mlx_bridge_model_size(const char* model_id);
extern bool mlx_bridge_delete_model(const char* model_id);
extern char* mlx_bridge_list_models(void);

// Flag to track if Swift bridge is available (set at runtime)
static bool g_swift_bridge_available = false;
static bool g_swift_bridge_checked = false;

// ============================================================================
// AVAILABLE MLX MODELS
// ============================================================================

static const MLXModelInfo g_mlx_model_info[] = {
    {
        .id = "llama-3.2-1b",
        .display_name = "Llama 3.2 1B",
        .huggingface_id = "mlx-community/Llama-3.2-1B-Instruct-4bit",
        .size_mb = 1500,
        .context_window = 131072,
        .min_ram_gb = 8,
        .supports_tools = true,
        .supports_italian = true,
        .best_for = "Fast responses, basic tasks, low RAM",
        .sha256 = NULL  // Set during download
    },
    {
        .id = "llama-3.2-3b",
        .display_name = "Llama 3.2 3B",
        .huggingface_id = "mlx-community/Llama-3.2-3B-Instruct-4bit",
        .size_mb = 3100,
        .context_window = 131072,
        .min_ram_gb = 8,
        .supports_tools = true,
        .supports_italian = true,
        .best_for = "Balanced speed and quality, recommended default",
        .sha256 = NULL
    },
    {
        .id = "phi-3-mini",
        .display_name = "Phi-3 Mini",
        .huggingface_id = "mlx-community/Phi-3-mini-4k-instruct-4bit",
        .size_mb = 2500,
        .context_window = 128000,
        .min_ram_gb = 8,
        .supports_tools = true,
        .supports_italian = false,
        .best_for = "Compact, good reasoning, Microsoft model",
        .sha256 = NULL
    },
    {
        .id = "mistral-7b-q4",
        .display_name = "Mistral 7B Q4",
        .huggingface_id = "mlx-community/Mistral-7B-Instruct-v0.3-4bit",
        .size_mb = 4500,
        .context_window = 32768,
        .min_ram_gb = 16,
        .supports_tools = true,
        .supports_italian = true,
        .best_for = "Powerful coding, needs 16GB RAM",
        .sha256 = NULL
    },
    {
        .id = "llama-3.1-8b-q4",
        .display_name = "Llama 3.1 8B Q4",
        .huggingface_id = "mlx-community/Meta-Llama-3.1-8B-Instruct-4bit",
        .size_mb = 5000,
        .context_window = 131072,
        .min_ram_gb = 16,
        .supports_tools = true,
        .supports_italian = true,
        .best_for = "Best quality local model, needs 16GB RAM",
        .sha256 = NULL
    },
    {
        .id = "deepseek-r1-1.5b",
        .display_name = "DeepSeek R1 Distill 1.5B",
        .huggingface_id = "mlx-community/DeepSeek-R1-Distill-Qwen-1.5B-4bit",
        .size_mb = 1200,
        .context_window = 64000,
        .min_ram_gb = 8,
        .supports_tools = true,
        .supports_italian = true,
        .best_for = "Fast reasoning, DeepSeek R1 distilled, low RAM",
        .sha256 = NULL
    },
    {
        .id = "deepseek-r1-7b",
        .display_name = "DeepSeek R1 Distill 7B",
        .huggingface_id = "mlx-community/DeepSeek-R1-Distill-Qwen-7B-4bit",
        .size_mb = 4500,
        .context_window = 64000,
        .min_ram_gb = 16,
        .supports_tools = true,
        .supports_italian = true,
        .best_for = "Strong reasoning, DeepSeek R1 quality, needs 16GB",
        .sha256 = NULL
    },
    {
        .id = "deepseek-r1-14b",
        .display_name = "DeepSeek R1 Distill 14B",
        .huggingface_id = "mlx-community/DeepSeek-R1-Distill-Qwen-14B-4bit",
        .size_mb = 8500,
        .context_window = 64000,
        .min_ram_gb = 24,
        .supports_tools = true,
        .supports_italian = true,
        .best_for = "Best reasoning, near R1 quality, needs 24GB+ RAM",
        .sha256 = NULL
    },
    {
        .id = "qwen2.5-coder-7b",
        .display_name = "Qwen 2.5 Coder 7B",
        .huggingface_id = "mlx-community/Qwen2.5-Coder-7B-Instruct-4bit",
        .size_mb = 4500,
        .context_window = 131072,
        .min_ram_gb = 16,
        .supports_tools = true,
        .supports_italian = false,
        .best_for = "Best local coding model, excellent for programming",
        .sha256 = NULL
    }
};
static const size_t g_mlx_model_count = sizeof(g_mlx_model_info) / sizeof(g_mlx_model_info[0]);

// ============================================================================
// MLX PROVIDER STATE
// ============================================================================

static MLXProviderData g_mlx_data = {0};
static pthread_mutex_t g_mlx_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_is_apple_silicon = false;
static bool g_hardware_checked = false;

// ============================================================================
// HARDWARE DETECTION
// ============================================================================

// Check if Swift bridge is linked and functional
static bool check_swift_bridge(void) {
    if (g_swift_bridge_checked) {
        return g_swift_bridge_available;
    }
    g_swift_bridge_checked = true;

    // Try to call the Swift bridge availability check
    // This will fail gracefully if the Swift library isn't linked
    @try {
        g_swift_bridge_available = mlx_bridge_is_available();
        if (g_swift_bridge_available) {
            LOG_INFO(LOG_CAT_SYSTEM, "MLX: Swift bridge available and functional");
        } else {
            LOG_DEBUG(LOG_CAT_SYSTEM, "MLX: Swift bridge reports unavailable (not macOS 14+ or not Apple Silicon)");
        }
    } @catch (NSException *exception) {
        g_swift_bridge_available = false;
        LOG_DEBUG(LOG_CAT_SYSTEM, "MLX: Swift bridge not linked or not available");
    }

    return g_swift_bridge_available;
}

static bool check_apple_silicon(void) {
    if (g_hardware_checked) {
        return g_is_apple_silicon;
    }

    g_hardware_checked = true;

    // Check if we're on Apple Silicon using global g_hardware
    if (convergio_detect_hardware() == 0 && convergio_hardware_detected()) {
        g_is_apple_silicon = (g_hardware.family != CHIP_FAMILY_UNKNOWN);
        if (g_is_apple_silicon) {
            size_t memory_gb = g_hardware.memory_bytes / (1024 * 1024 * 1024);
            LOG_INFO(LOG_CAT_SYSTEM, "MLX: Detected %s with %u GPU cores, %zu GB RAM",
                     convergio_chip_family_name(g_hardware.family),
                     g_hardware.gpu_cores, memory_gb);
        }
    } else {
        // Fallback: check for arm64 architecture
        #if defined(__arm64__) || defined(__aarch64__)
        g_is_apple_silicon = true;
        LOG_INFO(LOG_CAT_SYSTEM, "MLX: Running on Apple Silicon (arm64)");
        #else
        g_is_apple_silicon = false;
        LOG_WARN(LOG_CAT_SYSTEM, "MLX: Not running on Apple Silicon");
        #endif
    }

    return g_is_apple_silicon;
}

// ============================================================================
// PROGRESS BAR FOR DOWNLOADS
// ============================================================================

static int g_last_progress = -1;
static bool g_download_in_progress = false;

static void print_progress_bar(int percent) {
    if (!g_download_in_progress) return;

    // Only update if percent changed (avoid flicker)
    if (percent == g_last_progress) return;
    g_last_progress = percent;

    // Progress bar width
    const int bar_width = 40;
    int filled = (percent * bar_width) / 100;

    // ANSI escape codes for styling
    printf("\r\033[K");  // Clear line
    printf("  \033[36m⬇️  Downloading model:\033[0m [");

    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf("\033[32m█\033[0m");  // Green filled
        } else if (i == filled) {
            printf("\033[33m▓\033[0m");  // Yellow current
        } else {
            printf("\033[90m░\033[0m");  // Gray empty
        }
    }

    printf("] \033[1m%3d%%\033[0m", percent);
    fflush(stdout);

    if (percent >= 100) {
        printf("\n  \033[32m✓ Download complete!\033[0m\n");
        g_download_in_progress = false;
    }
}

// C callback for Swift bridge progress updates
static void download_progress_callback(int32_t percent) {
    print_progress_bar((int)percent);
}

// Download model with visible progress bar
MLXError mlx_download_model_with_progress(const char* huggingface_id) {
    if (!huggingface_id) return MLX_ERR_MODEL_NOT_FOUND;

    // Check if already downloaded
    if (mlx_bridge_model_exists(huggingface_id)) {
        LOG_INFO(LOG_CAT_SYSTEM, "MLX: Model already downloaded: %s", huggingface_id);
        return MLX_OK;
    }

    printf("\n  \033[36mPreparing to download: %s\033[0m\n", huggingface_id);
    printf("  \033[90mThis may take a few minutes depending on your connection...\033[0m\n\n");

    g_download_in_progress = true;
    g_last_progress = -1;

    // Call Swift bridge download with progress callback
    char* error = mlx_bridge_download_model(huggingface_id, download_progress_callback);

    g_download_in_progress = false;

    if (error) {
        printf("\n  \033[31m✗ Download failed: %s\033[0m\n", error);
        free(error);
        return MLX_ERR_LOAD_FAILED;
    }

    return MLX_OK;
}

// ============================================================================
// PATH UTILITIES
// ============================================================================

static char* get_models_dir(void) {
    const char* home = getenv("HOME");
    if (!home) return NULL;

    char* path = malloc(512);
    if (!path) return NULL;

    snprintf(path, 512, "%s/.convergio/models", home);
    return path;
}

static char* get_model_path(const char* model_id) {
    char* models_dir = get_models_dir();
    if (!models_dir) return NULL;

    char* path = malloc(512);
    if (!path) {
        free(models_dir);
        return NULL;
    }

    snprintf(path, 512, "%s/%s", models_dir, model_id);
    free(models_dir);

    return path;
}

// ============================================================================
// MODEL MANAGEMENT
// ============================================================================

bool mlx_is_available(void) {
    // Must be on Apple Silicon AND have Swift bridge available
    return check_apple_silicon() && check_swift_bridge();
}

const MLXModelInfo* mlx_get_available_models(size_t* out_count) {
    if (out_count) *out_count = g_mlx_model_count;
    return g_mlx_model_info;
}

static const MLXModelInfo* find_model_info(const char* model_id) {
    for (size_t i = 0; i < g_mlx_model_count; i++) {
        if (strcmp(g_mlx_model_info[i].id, model_id) == 0) {
            return &g_mlx_model_info[i];
        }
    }
    return NULL;
}

bool mlx_model_is_ready(const char* model_id) {
    if (!model_id) return false;

    char* path = get_model_path(model_id);
    if (!path) return false;

    // Check if model directory exists and has required files
    struct stat st;
    bool exists = (stat(path, &st) == 0 && S_ISDIR(st.st_mode));

    if (exists) {
        // Check for model weights file
        char weights_path[512];
        snprintf(weights_path, sizeof(weights_path), "%s/model.safetensors", path);
        exists = (stat(weights_path, &st) == 0);

        if (!exists) {
            // Try alternative weight file
            snprintf(weights_path, sizeof(weights_path), "%s/weights.npz", path);
            exists = (stat(weights_path, &st) == 0);
        }
    }

    free(path);
    return exists;
}

const char* mlx_get_model_path(const char* model_id) {
    if (!mlx_model_is_ready(model_id)) {
        return NULL;
    }

    // Return cached path
    static char cached_path[512];
    char* path = get_model_path(model_id);
    if (path) {
        strncpy(cached_path, path, sizeof(cached_path) - 1);
        cached_path[sizeof(cached_path) - 1] = '\0';
        free(path);
        return cached_path;
    }

    return NULL;
}

const char* mlx_recommend_model(size_t available_ram_gb) {
    if (available_ram_gb >= 16) {
        return "deepseek-r1-7b";   // Best reasoning, recommended default
    } else if (available_ram_gb >= 8) {
        return "deepseek-r1-1.5b"; // Fast reasoning, low RAM
    } else {
        return "llama-3.2-1b";     // Fastest, lowest RAM
    }
}

// ============================================================================
// MODEL LOADING (PLACEHOLDER - requires MLX integration)
// ============================================================================

MLXError mlx_load_model(const char* model_id) {
    pthread_mutex_lock(&g_mlx_mutex);

    if (!check_apple_silicon()) {
        pthread_mutex_unlock(&g_mlx_mutex);
        return MLX_ERR_NOT_APPLE_SILICON;
    }

    if (!check_swift_bridge()) {
        g_mlx_data.last_error = MLX_ERR_LOAD_FAILED;
        if (g_mlx_data.last_error_message) free(g_mlx_data.last_error_message);
        g_mlx_data.last_error_message = strdup("MLX Swift bridge not available. Rebuild with Swift support.");
        pthread_mutex_unlock(&g_mlx_mutex);
        return MLX_ERR_LOAD_FAILED;
    }

    // Strip "mlx/" prefix if present
    const char* actual_model_id = model_id;
    if (strncmp(model_id, "mlx/", 4) == 0) {
        actual_model_id = model_id + 4;
    }

    const MLXModelInfo* info = find_model_info(actual_model_id);
    if (!info) {
        pthread_mutex_unlock(&g_mlx_mutex);
        return MLX_ERR_MODEL_NOT_FOUND;
    }

    // Check RAM requirements (use global hardware info)
    if (convergio_hardware_detected()) {
        size_t available_gb = g_hardware.memory_bytes / (1024 * 1024 * 1024);
        if (available_gb < info->min_ram_gb) {
            g_mlx_data.last_error = MLX_ERR_OUT_OF_MEMORY;
            if (g_mlx_data.last_error_message) free(g_mlx_data.last_error_message);
            char msg[256];
            snprintf(msg, sizeof(msg), "Insufficient RAM: %s requires %zuGB, you have %zuGB",
                     info->display_name, info->min_ram_gb, available_gb);
            g_mlx_data.last_error_message = strdup(msg);
            pthread_mutex_unlock(&g_mlx_mutex);
            return MLX_ERR_OUT_OF_MEMORY;
        }
    }

    // Unload previous model if any
    if (g_mlx_data.model_loaded) {
        pthread_mutex_unlock(&g_mlx_mutex);
        mlx_unload_model();
        pthread_mutex_lock(&g_mlx_mutex);
    }

    // Get model cache directory
    char* models_dir = get_models_dir();
    if (!models_dir) {
        pthread_mutex_unlock(&g_mlx_mutex);
        return MLX_ERR_LOAD_FAILED;
    }

    // =========================================================================
    // STEP 1: Check if model needs downloading (with progress bar)
    // =========================================================================
    if (!mlx_bridge_model_exists(info->huggingface_id)) {
        pthread_mutex_unlock(&g_mlx_mutex);  // Release lock during download

        printf("\n  \033[36m📦 Model not found locally. Starting download...\033[0m\n");
        printf("  \033[90mModel: %s (%zu MB)\033[0m\n", info->display_name, info->size_mb);

        MLXError download_err = mlx_download_model_with_progress(info->huggingface_id);
        if (download_err != MLX_OK) {
            free(models_dir);
            return download_err;
        }

        pthread_mutex_lock(&g_mlx_mutex);  // Re-acquire lock
    } else {
        LOG_INFO(LOG_CAT_SYSTEM, "MLX: Model already cached: %s", info->huggingface_id);
    }

    // =========================================================================
    // STEP 2: Load the model into GPU memory
    // =========================================================================
    printf("\n  \033[36m🚀 Loading model into GPU memory...\033[0m\n");

    LOG_INFO(LOG_CAT_SYSTEM, "MLX: Loading model %s via Swift bridge", info->huggingface_id);

    // Call Swift bridge to load model
    // Use HuggingFace ID for downloading/caching
    bool success = mlx_bridge_load_model(info->huggingface_id, models_dir);

    if (!success) {
        g_mlx_data.last_error = MLX_ERR_LOAD_FAILED;
        if (g_mlx_data.last_error_message) free(g_mlx_data.last_error_message);
        g_mlx_data.last_error_message = strdup("Failed to load model via MLX Swift bridge");
        free(models_dir);
        pthread_mutex_unlock(&g_mlx_mutex);
        return MLX_ERR_LOAD_FAILED;
    }

    g_mlx_data.current_model_id = strdup(actual_model_id);
    g_mlx_data.model_path = models_dir;  // Keep models_dir for later cleanup
    g_mlx_data.model_loaded = true;
    g_mlx_data.context_used = 0;
    g_mlx_data.last_error = MLX_OK;

    pthread_mutex_unlock(&g_mlx_mutex);

    printf("  \033[32m✓ Model ready: %s\033[0m\n\n", info->display_name);
    LOG_INFO(LOG_CAT_SYSTEM, "MLX: Model %s loaded successfully (GPU memory: %lld bytes)",
             actual_model_id, mlx_bridge_gpu_memory_used());
    return MLX_OK;
}

void mlx_unload_model(void) {
    pthread_mutex_lock(&g_mlx_mutex);

    if (g_mlx_data.current_model_id) {
        LOG_INFO(LOG_CAT_SYSTEM, "MLX: Unloading model %s", g_mlx_data.current_model_id);
        free(g_mlx_data.current_model_id);
        g_mlx_data.current_model_id = NULL;
    }

    if (g_mlx_data.model_path) {
        free(g_mlx_data.model_path);
        g_mlx_data.model_path = NULL;
    }

    // Call Swift bridge to unload model and free GPU memory
    if (g_swift_bridge_available) {
        mlx_bridge_unload_model();
        mlx_bridge_clear_cache();
    }

    g_mlx_data.mlx_model = NULL;
    g_mlx_data.tokenizer = NULL;
    g_mlx_data.model_loaded = false;
    g_mlx_data.context_used = 0;

    pthread_mutex_unlock(&g_mlx_mutex);
}

const MLXModelInfo* mlx_get_current_model(void) {
    if (!g_mlx_data.model_loaded || !g_mlx_data.current_model_id) {
        return NULL;
    }
    return find_model_info(g_mlx_data.current_model_id);
}

// ============================================================================
// INFERENCE (via MLX Swift bridge)
// ============================================================================

MLXError mlx_generate(
    const char* prompt,
    const char* system,
    size_t max_tokens,
    float temperature,
    char** out_response
) {
    if (!prompt || !out_response) {
        return MLX_ERR_INFERENCE_FAILED;
    }

    pthread_mutex_lock(&g_mlx_mutex);

    if (!g_mlx_data.model_loaded) {
        pthread_mutex_unlock(&g_mlx_mutex);
        return MLX_ERR_MODEL_NOT_FOUND;
    }

    if (!g_swift_bridge_available) {
        pthread_mutex_unlock(&g_mlx_mutex);
        g_mlx_data.last_error = MLX_ERR_INFERENCE_FAILED;
        if (g_mlx_data.last_error_message) free(g_mlx_data.last_error_message);
        g_mlx_data.last_error_message = strdup("MLX Swift bridge not available");
        return MLX_ERR_INFERENCE_FAILED;
    }

    const MLXModelInfo* model = find_model_info(g_mlx_data.current_model_id);
    LOG_DEBUG(LOG_CAT_SYSTEM, "MLX: Generating response with %s (max_tokens=%zu, temp=%.2f)",
              model ? model->display_name : "unknown", max_tokens, (double)temperature);

    // Call Swift bridge for inference
    int32_t token_count = 0;
    float tokens_per_sec = 0.0f;
    char* error_msg = NULL;

    char* response = mlx_bridge_generate(
        prompt,
        system,
        (int32_t)max_tokens,
        temperature,
        &token_count,
        &tokens_per_sec,
        &error_msg
    );

    if (!response) {
        g_mlx_data.last_error = MLX_ERR_INFERENCE_FAILED;
        if (g_mlx_data.last_error_message) free(g_mlx_data.last_error_message);
        g_mlx_data.last_error_message = error_msg ? error_msg : strdup("Inference failed");
        pthread_mutex_unlock(&g_mlx_mutex);
        return MLX_ERR_INFERENCE_FAILED;
    }

    LOG_INFO(LOG_CAT_SYSTEM, "MLX: Generated %d tokens at %.1f tok/s",
             token_count, (double)tokens_per_sec);

    // Update context tracking
    g_mlx_data.context_used += mlx_estimate_tokens(prompt) + (size_t)token_count;

    *out_response = response;  // Caller must free()

    pthread_mutex_unlock(&g_mlx_mutex);
    return MLX_OK;
}

MLXError mlx_generate_stream(
    const char* prompt,
    const char* system,
    size_t max_tokens,
    float temperature,
    void (*on_token)(const char* token, void* ctx),
    void* user_ctx
) {
    if (!prompt || !on_token) {
        return MLX_ERR_INFERENCE_FAILED;
    }

    // For now, generate full response and stream word by word
    char* response = NULL;
    MLXError err = mlx_generate(prompt, system, max_tokens, temperature, &response);

    if (err != MLX_OK || !response) {
        return err;
    }

    // Simulate streaming by emitting words
    char* word = strtok(response, " \n");
    while (word) {
        on_token(word, user_ctx);
        on_token(" ", user_ctx);  // Add space
        word = strtok(NULL, " \n");
    }

    free(response);
    return MLX_OK;
}

size_t mlx_estimate_tokens(const char* text) {
    if (!text) return 0;

    // Simple estimate: ~4 characters per token on average
    // This is rough but works for most LLM tokenizers
    size_t len = strlen(text);
    return (len + 3) / 4;  // Round up
}

const char* mlx_error_message(MLXError error) {
    switch (error) {
        case MLX_OK:
            return "Success";
        case MLX_ERR_NOT_APPLE_SILICON:
            return "MLX requires Apple Silicon (M1/M2/M3/M4/M5)";
        case MLX_ERR_MODEL_NOT_FOUND:
            return "Model not found. Download it first with /setup → Local Models";
        case MLX_ERR_MODEL_CORRUPT:
            return "Model files are corrupt. Try re-downloading";
        case MLX_ERR_OUT_OF_MEMORY:
            return "Insufficient RAM for this model";
        case MLX_ERR_LOAD_FAILED:
            return "Failed to load model";
        case MLX_ERR_INFERENCE_FAILED:
            return "Inference failed";
        case MLX_ERR_TOKENIZER_FAILED:
            return "Tokenizer error";
        default:
            return "Unknown MLX error";
    }
}

// ============================================================================
// PROVIDER INTERFACE IMPLEMENTATION
// ============================================================================

static ProviderError mlx_provider_init(Provider* self) {
    (void)self;

    if (!check_apple_silicon()) {
        LOG_WARN(LOG_CAT_SYSTEM, "MLX provider not available: not Apple Silicon");
        return PROVIDER_ERR_NOT_INITIALIZED;
    }

    pthread_mutex_lock(&g_mlx_mutex);
    g_mlx_data.initialized = true;
    pthread_mutex_unlock(&g_mlx_mutex);

    LOG_INFO(LOG_CAT_SYSTEM, "MLX provider initialized");
    return PROVIDER_OK;
}

static void mlx_provider_shutdown(Provider* self) {
    (void)self;

    mlx_unload_model();

    pthread_mutex_lock(&g_mlx_mutex);

    if (g_mlx_data.last_error_message) {
        free(g_mlx_data.last_error_message);
        g_mlx_data.last_error_message = NULL;
    }

    g_mlx_data.initialized = false;
    pthread_mutex_unlock(&g_mlx_mutex);

    LOG_INFO(LOG_CAT_SYSTEM, "MLX provider shut down");
}

static bool mlx_provider_validate_key(Provider* self) {
    (void)self;
    // MLX doesn't need an API key - just check if we're on Apple Silicon
    return check_apple_silicon();
}

static char* mlx_provider_chat(
    Provider* self,
    const char* model,
    const char* system,
    const char* user,
    TokenUsage* usage
) {
    (void)self;

    if (!g_mlx_data.initialized) {
        return NULL;
    }

    // Load model if needed
    if (!g_mlx_data.model_loaded ||
        !g_mlx_data.current_model_id ||
        strcmp(g_mlx_data.current_model_id, model) != 0) {

        if (mlx_load_model(model) != MLX_OK) {
            telemetry_record_error("mlx_model_load_failed");
            return NULL;
        }
    }

    // Measure latency for telemetry
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    char* response = NULL;
    MLXError err = mlx_generate(user, system, 4096, 0.7f, &response);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double latency_ms = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) +
                        ((end_time.tv_nsec - start_time.tv_nsec) / 1000000.0);

    if (err != MLX_OK) {
        telemetry_record_error("mlx_inference_failed");
        return NULL;
    }

    // Update usage stats
    size_t tokens_input = 0;
    size_t tokens_output = 0;
    if (usage) {
        tokens_input = mlx_estimate_tokens(user);
        if (system) tokens_input += mlx_estimate_tokens(system);
        tokens_output = mlx_estimate_tokens(response);
        usage->input_tokens = tokens_input;
        usage->output_tokens = tokens_output;
        usage->cached_tokens = 0;
        usage->estimated_cost = 0.0;  // Local inference is free!
    }

    // Record successful API call in telemetry (local models, cost=0)
    telemetry_record_api_call("mlx", model ? model : "mlx", tokens_input, tokens_output, latency_ms);

    return response;
}

static size_t mlx_provider_estimate_tokens(Provider* self, const char* text) {
    (void)self;
    return mlx_estimate_tokens(text);
}

static ProviderErrorInfo g_mlx_last_error = {0};

static ProviderErrorInfo* mlx_provider_get_last_error(Provider* self) {
    (void)self;

    g_mlx_last_error.code = (g_mlx_data.last_error == MLX_OK) ? PROVIDER_OK : PROVIDER_ERR_UNKNOWN;
    g_mlx_last_error.message = g_mlx_data.last_error_message;
    g_mlx_last_error.is_retryable = false;

    return &g_mlx_last_error;
}

// ============================================================================
// CHAT WITH TOOLS (fallback to regular chat - MLX doesn't support native tool calling)
// ============================================================================

static char* mlx_provider_chat_with_tools(
    Provider* self,
    const char* model,
    const char* system,
    const char* user,
    ToolDefinition* tools,
    size_t tool_count,
    ToolCall** out_tool_calls,
    size_t* out_tool_count,
    TokenUsage* usage
) {
    // MLX local models don't support native tool calling yet
    // Fall back to regular chat - the model can still describe tool usage in text
    (void)tools;
    (void)tool_count;

    if (out_tool_calls) *out_tool_calls = NULL;
    if (out_tool_count) *out_tool_count = 0;

    LOG_DEBUG(LOG_CAT_SYSTEM, "MLX: tool calling not natively supported, falling back to chat");
    return mlx_provider_chat(self, model, system, user, usage);
}

// ============================================================================
// STREAMING CHAT (simulated - MLX bridge doesn't support true streaming yet)
// ============================================================================

static ProviderError mlx_provider_stream_chat(
    Provider* self,
    const char* model,
    const char* system,
    const char* user,
    StreamHandler* handler,
    TokenUsage* usage
) {
    if (!self || !user) return PROVIDER_ERR_INVALID_REQUEST;
    if (!g_mlx_data.initialized) return PROVIDER_ERR_NOT_INITIALIZED;

    // MLX Swift bridge doesn't support streaming callbacks yet
    // Fall back to regular generation and deliver as single chunk
    LOG_DEBUG(LOG_CAT_SYSTEM, "MLX: streaming not natively supported, using single-chunk delivery");

    char* response = mlx_provider_chat(self, model, system, user, usage);

    if (!response) {
        if (handler && handler->on_error) {
            handler->on_error(g_mlx_data.last_error_message ? g_mlx_data.last_error_message : "MLX generation failed",
                            handler->user_ctx);
        }
        return PROVIDER_ERR_UNKNOWN;
    }

    // Deliver the complete response as a single chunk
    if (handler) {
        if (handler->on_chunk) {
            handler->on_chunk(response, false, handler->user_ctx);
            handler->on_chunk("", true, handler->user_ctx);  // Signal completion
        }
        if (handler->on_complete) {
            handler->on_complete(response, handler->user_ctx);
        }
    }

    free(response);
    return PROVIDER_OK;
}

// ============================================================================
// LIST MODELS
// ============================================================================

static ProviderError mlx_provider_list_models(
    Provider* self,
    ModelConfig** out_models,
    size_t* out_count
) {
    (void)self;

    if (!out_models || !out_count) return PROVIDER_ERR_INVALID_REQUEST;

    // Return the static model info converted to ModelConfig format
    size_t count = sizeof(g_mlx_model_info) / sizeof(g_mlx_model_info[0]);

    ModelConfig* models = calloc(count, sizeof(ModelConfig));
    if (!models) return PROVIDER_ERR_UNKNOWN;

    for (size_t i = 0; i < count; i++) {
        models[i].id = g_mlx_model_info[i].id;
        models[i].display_name = g_mlx_model_info[i].display_name;
        models[i].provider = PROVIDER_MLX;
        models[i].input_cost_per_mtok = 0.0;   // Free - local inference
        models[i].output_cost_per_mtok = 0.0;  // Free - local inference
        models[i].thinking_cost_per_mtok = 0.0;
        models[i].context_window = g_mlx_model_info[i].context_window;
        models[i].max_output = 4096;  // Default max output
        models[i].supports_tools = g_mlx_model_info[i].supports_tools;
        models[i].supports_vision = false;  // MLX text models don't support vision yet
        models[i].supports_streaming = false;  // Not truly supported yet
        models[i].tier = COST_TIER_CHEAP;  // Free!
        models[i].released = "2024-09-25";  // Llama 3.2 release
        models[i].deprecated = false;
    }

    *out_models = models;
    *out_count = count;

    LOG_DEBUG(LOG_CAT_SYSTEM, "MLX: listed %zu available models", count);
    return PROVIDER_OK;
}

// ============================================================================
// PROVIDER FACTORY
// ============================================================================

Provider* mlx_provider_create(void) {
    // Check if we can use MLX before creating provider
    if (!check_apple_silicon()) {
        LOG_DEBUG(LOG_CAT_SYSTEM, "MLX provider not created: not Apple Silicon");
        return NULL;
    }

    Provider* provider = calloc(1, sizeof(Provider));
    if (!provider) {
        return NULL;
    }

    provider->type = PROVIDER_MLX;
    provider->name = "MLX (Apple Silicon)";
    provider->api_key_env = NULL;  // No API key needed
    provider->base_url = NULL;     // Local inference
    provider->initialized = false;

    // Core operations
    provider->init = mlx_provider_init;
    provider->shutdown = mlx_provider_shutdown;
    provider->validate_key = mlx_provider_validate_key;

    // Chat operations
    provider->chat = mlx_provider_chat;
    provider->chat_with_tools = mlx_provider_chat_with_tools;
    provider->stream_chat = mlx_provider_stream_chat;

    // Utilities
    provider->estimate_tokens = mlx_provider_estimate_tokens;
    provider->get_last_error = mlx_provider_get_last_error;
    provider->list_models = mlx_provider_list_models;

    provider->impl_data = &g_mlx_data;

    LOG_INFO(LOG_CAT_SYSTEM, "MLX provider created (Apple Silicon native)");
    return provider;
}
