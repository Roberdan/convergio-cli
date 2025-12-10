/**
 * CONVERGIO MLX EMBEDDINGS
 *
 * Pure Metal/C implementation for local text embeddings
 * Optimized for Apple M3 Max - Zero cloud costs
 *
 * Architecture: Simplified transformer encoder
 * - Tokenizer: BPE-based (compatible with MiniLM vocab)
 * - Encoder: 6 layers, 384 hidden, 12 heads
 * - Output: 384-dim embedding vector
 */

#ifndef CONVERGIO_MLX_EMBED_H
#define CONVERGIO_MLX_EMBED_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Model configuration (MiniLM-L6-v2 compatible)
#define MLX_VOCAB_SIZE      30522
#define MLX_HIDDEN_DIM      384
#define MLX_NUM_LAYERS      6
#define MLX_NUM_HEADS       12
#define MLX_HEAD_DIM        32      // HIDDEN_DIM / NUM_HEADS
#define MLX_MAX_SEQ_LEN     512
#define MLX_INTERMEDIATE    1536    // 4 * HIDDEN_DIM

// ============================================================================
// TYPES
// ============================================================================

typedef struct {
    // Token embeddings [VOCAB_SIZE, HIDDEN_DIM]
    float* token_embeddings;

    // Position embeddings [MAX_SEQ_LEN, HIDDEN_DIM]
    float* position_embeddings;

    // Layer norm params
    float* ln_gamma;  // [HIDDEN_DIM]
    float* ln_beta;   // [HIDDEN_DIM]

    // Transformer layers (6 layers)
    struct {
        // Self-attention
        float* q_weight;  // [HIDDEN_DIM, HIDDEN_DIM]
        float* k_weight;  // [HIDDEN_DIM, HIDDEN_DIM]
        float* v_weight;  // [HIDDEN_DIM, HIDDEN_DIM]
        float* o_weight;  // [HIDDEN_DIM, HIDDEN_DIM]
        float* attn_ln_gamma;
        float* attn_ln_beta;

        // FFN
        float* ffn_up;    // [HIDDEN_DIM, INTERMEDIATE]
        float* ffn_down;  // [INTERMEDIATE, HIDDEN_DIM]
        float* ffn_ln_gamma;
        float* ffn_ln_beta;
    } layers[MLX_NUM_LAYERS];

    // Final layer norm
    float* final_ln_gamma;
    float* final_ln_beta;

    // Pooler (for sentence embedding)
    float* pooler_weight;  // [HIDDEN_DIM, HIDDEN_DIM]
    float* pooler_bias;    // [HIDDEN_DIM]

    // Metal resources
    void* metal_device;
    void* metal_queue;
    void* compute_pipeline;
    void* weights_buffer;

    bool initialized;
    bool use_gpu;
} MLXEmbedModel;

typedef struct {
    int32_t* ids;
    size_t length;
} MLXTokens;

// ============================================================================
// API
// ============================================================================

// Lifecycle
int mlx_embed_init(const char* model_path);
void mlx_embed_shutdown(void);
bool mlx_embed_is_ready(void);

// Tokenization
MLXTokens* mlx_tokenize(const char* text);
void mlx_free_tokens(MLXTokens* tokens);

// Embedding generation
float* mlx_embed_text(const char* text, size_t* out_dim);
float* mlx_embed_tokens(const MLXTokens* tokens, size_t* out_dim);

// Batch processing (for efficiency)
float** mlx_embed_batch(const char** texts, size_t count, size_t* out_dim);
void mlx_free_embeddings(float** embeddings, size_t count);

// Similarity (uses NEON SIMD)
float mlx_cosine_similarity(const float* a, const float* b, size_t dim);

// Model info
size_t mlx_get_embedding_dim(void);
size_t mlx_get_vocab_size(void);
const char* mlx_get_model_name(void);

#endif // CONVERGIO_MLX_EMBED_H
