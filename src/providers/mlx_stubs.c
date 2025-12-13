/**
 * MLX Swift Bridge Stubs
 *
 * These stub implementations are linked when the Swift MLX library is not available
 * (e.g., on CI runners without Swift 6.0). They provide safe fallback behavior.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Stub: Always returns false (MLX not available)
bool mlx_bridge_is_available(void) {
    return false;
}

// Stub: Loading always fails
bool mlx_bridge_load_model(const char* model_id, const char* cache_path) {
    (void)model_id;
    (void)cache_path;
    return false;
}

// Stub: No-op
void mlx_bridge_unload_model(void) {
}

// Stub: Returns error message
char* mlx_bridge_generate(
    const char* prompt,
    const char* system_prompt,
    int32_t max_tokens,
    float temperature,
    int32_t* out_token_count,
    float* out_tokens_per_sec,
    char** out_error
) {
    (void)prompt;
    (void)system_prompt;
    (void)max_tokens;
    (void)temperature;
    if (out_token_count) *out_token_count = 0;
    if (out_tokens_per_sec) *out_tokens_per_sec = 0.0f;
    if (out_error) *out_error = strdup("MLX not available (Swift library not linked)");
    return NULL;
}

// Stub: No-op
void mlx_bridge_clear_cache(void) {
}

// Stub: Returns 0
int64_t mlx_bridge_gpu_memory_used(void) {
    return 0;
}

// Stub: Returns error
char* mlx_bridge_download_model(const char* model_id, void (*progress_callback)(int32_t)) {
    (void)model_id;
    (void)progress_callback;
    return strdup("MLX not available (Swift library not linked)");
}

// Stub: Always returns false
bool mlx_bridge_model_exists(const char* model_id) {
    (void)model_id;
    return false;
}

// Stub: Returns 0
int64_t mlx_bridge_model_size(const char* model_id) {
    (void)model_id;
    return 0;
}

// Stub: Always returns false
bool mlx_bridge_delete_model(const char* model_id) {
    (void)model_id;
    return false;
}

// Stub: Returns empty JSON array
char* mlx_bridge_list_models(void) {
    return strdup("[]");
}
