/**
 * NOUS Metal Shaders for M3 Max GPU
 *
 * Optimized for 30 GPU cores with dynamic caching
 * Uses Metal 4 features for maximum throughput
 */

#include <metal_stdlib>
using namespace metal;

// ============================================================================
// CONSTANTS
// ============================================================================

constant uint EMBEDDING_DIM = 768;
constant uint TILE_SIZE = 256;  // Optimal for M3 Max threadgroup size
constant uint VECTORS_PER_THREAD = 4;

// ============================================================================
// BATCH COSINE SIMILARITY
// ============================================================================

/**
 * Computes cosine similarity between a query and N candidate embeddings
 *
 * Uses:
 * - Threadgroup shared memory for query caching
 * - SIMD group operations for efficient reduction
 * - Fused multiply-add for numerical precision
 */
kernel void batch_cosine_similarity(
    device const half* query [[buffer(0)]],           // [EMBEDDING_DIM]
    device const half* candidates [[buffer(1)]],      // [N x EMBEDDING_DIM]
    device float* similarities [[buffer(2)]],          // [N]
    constant uint& candidate_count [[buffer(3)]],
    uint tid [[thread_position_in_threadgroup]],
    uint gid [[thread_position_in_grid]],
    uint tg_size [[threads_per_threadgroup]],
    uint simd_lane [[thread_index_in_simdgroup]],
    uint simd_group [[simdgroup_index_in_threadgroup]])
{
    // Cache query in threadgroup memory (shared across threads)
    threadgroup half shared_query[EMBEDDING_DIM];

    // Cooperatively load query into shared memory
    for (uint i = tid; i < EMBEDDING_DIM; i += tg_size) {
        shared_query[i] = query[i];
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Each thread processes one candidate
    if (gid >= candidate_count) return;

    device const half* candidate = candidates + gid * EMBEDDING_DIM;

    // Compute dot product and norms using SIMD
    float dot = 0.0f;
    float norm_q = 0.0f;
    float norm_c = 0.0f;

    // Process 8 elements at a time for optimal half-precision throughput
    for (uint i = 0; i < EMBEDDING_DIM; i += 8) {
        half8 q = *reinterpret_cast<threadgroup const half8*>(&shared_query[i]);
        half8 c = *reinterpret_cast<device const half8*>(&candidate[i]);

        // Convert to float for accumulation
        float4 q_lo = float4(q.lo);
        float4 q_hi = float4(q.hi);
        float4 c_lo = float4(c.lo);
        float4 c_hi = float4(c.hi);

        dot += dot(q_lo, c_lo) + dot(q_hi, c_hi);
        norm_q += dot(q_lo, q_lo) + dot(q_hi, q_hi);
        norm_c += dot(c_lo, c_lo) + dot(c_hi, c_hi);
    }

    // Compute cosine similarity
    float denom = sqrt(norm_q) * sqrt(norm_c);
    similarities[gid] = (denom > 1e-8f) ? (dot / denom) : 0.0f;
}

// ============================================================================
// TOP-K SIMILARITY SEARCH
// ============================================================================

/**
 * Finds top K most similar embeddings using parallel reduction
 *
 * Two-phase approach:
 * 1. Each threadgroup finds local top-K
 * 2. Final reduction combines results
 */

struct SimilarityPair {
    uint index;
    float similarity;
};

kernel void top_k_similarity(
    device const half* query [[buffer(0)]],
    device const half* embeddings [[buffer(1)]],
    device float* all_similarities [[buffer(2)]],  // Pre-computed
    device SimilarityPair* results [[buffer(3)]],
    constant uint& embedding_count [[buffer(4)]],
    constant uint& k [[buffer(5)]],
    uint tid [[thread_position_in_threadgroup]],
    uint gid [[thread_position_in_grid]],
    uint tgid [[threadgroup_position_in_grid]],
    uint tg_size [[threads_per_threadgroup]])
{
    // Shared memory for local top-K
    threadgroup SimilarityPair local_top[32];  // Max K = 32

    uint start = tgid * tg_size;
    uint end = min(start + tg_size, embedding_count);

    // Initialize local top-K with minimum values
    if (tid < k) {
        local_top[tid] = {UINT_MAX, -1.0f};
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Each thread checks its element against local top-K
    if (start + tid < end) {
        float sim = all_similarities[start + tid];

        // Try to insert into local top-K
        for (uint i = 0; i < k; i++) {
            if (sim > local_top[i].similarity) {
                // Atomic compare-and-swap simulation with barrier
                threadgroup_barrier(mem_flags::mem_threadgroup);
                if (sim > local_top[i].similarity) {
                    // Shift down
                    for (uint j = k - 1; j > i; j--) {
                        local_top[j] = local_top[j - 1];
                    }
                    local_top[i] = {start + tid, sim};
                }
                break;
            }
        }
    }

    threadgroup_barrier(mem_flags::mem_threadgroup);

    // Thread 0 writes results for this threadgroup
    if (tid < k) {
        results[tgid * k + tid] = local_top[tid];
    }
}

// ============================================================================
// EMBEDDING GENERATION (Neural-style forward pass)
// ============================================================================

/**
 * Simple embedding projection layer
 * Input: tokenized text (indices)
 * Output: pooled embedding
 */
kernel void embed_tokens(
    device const uint* tokens [[buffer(0)]],          // [seq_len]
    device const half* token_embeddings [[buffer(1)]], // [vocab_size x EMBEDDING_DIM]
    device half* output [[buffer(2)]],                 // [EMBEDDING_DIM]
    constant uint& seq_len [[buffer(3)]],
    constant uint& vocab_size [[buffer(4)]],
    uint tid [[thread_position_in_grid]])
{
    if (tid >= EMBEDDING_DIM) return;

    float sum = 0.0f;

    // Mean pooling over sequence
    for (uint i = 0; i < seq_len; i++) {
        uint token = tokens[i];
        if (token < vocab_size) {
            sum += float(token_embeddings[token * EMBEDDING_DIM + tid]);
        }
    }

    output[tid] = half(sum / float(seq_len));
}

// ============================================================================
// RELATION GRAPH OPERATIONS
// ============================================================================

/**
 * Propagate activation through semantic graph
 * Used for spreading activation search
 */
kernel void propagate_activation(
    device const uint* adjacency_offsets [[buffer(0)]],  // [N+1] CSR format
    device const uint* adjacency_indices [[buffer(1)]],
    device const half* edge_weights [[buffer(2)]],
    device float* activations [[buffer(3)]],             // [N]
    device float* new_activations [[buffer(4)]],         // [N]
    constant float& decay [[buffer(5)]],
    constant uint& node_count [[buffer(6)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= node_count) return;

    uint start = adjacency_offsets[gid];
    uint end = adjacency_offsets[gid + 1];

    float incoming = 0.0f;

    // Sum weighted activations from neighbors
    for (uint i = start; i < end; i++) {
        uint neighbor = adjacency_indices[i];
        float weight = float(edge_weights[i]);
        incoming += activations[neighbor] * weight;
    }

    // Apply decay and add incoming activation
    new_activations[gid] = activations[gid] * decay + incoming;
}

// ============================================================================
// EMBEDDING CLUSTERING (K-Means step)
// ============================================================================

/**
 * Assign embeddings to nearest centroid
 */
kernel void kmeans_assign(
    device const half* embeddings [[buffer(0)]],   // [N x EMBEDDING_DIM]
    device const half* centroids [[buffer(1)]],    // [K x EMBEDDING_DIM]
    device uint* assignments [[buffer(2)]],         // [N]
    constant uint& embedding_count [[buffer(3)]],
    constant uint& centroid_count [[buffer(4)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= embedding_count) return;

    device const half* emb = embeddings + gid * EMBEDDING_DIM;

    float min_dist = INFINITY;
    uint best_centroid = 0;

    for (uint c = 0; c < centroid_count; c++) {
        device const half* cent = centroids + c * EMBEDDING_DIM;

        float dist = 0.0f;
        for (uint i = 0; i < EMBEDDING_DIM; i += 8) {
            half8 e = *reinterpret_cast<device const half8*>(&emb[i]);
            half8 ce = *reinterpret_cast<device const half8*>(&cent[i]);
            half8 diff = e - ce;

            float4 d_lo = float4(diff.lo);
            float4 d_hi = float4(diff.hi);
            dist += dot(d_lo, d_lo) + dot(d_hi, d_hi);
        }

        if (dist < min_dist) {
            min_dist = dist;
            best_centroid = c;
        }
    }

    assignments[gid] = best_centroid;
}
