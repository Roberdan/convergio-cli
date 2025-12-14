/**
 * NOUS Semantic Fabric Implementation
 *
 * The living graph of meaning - optimized for Apple Silicon
 */

#include "nous/nous.h"
#include "nous/semantic_persistence.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>
#include <arm_neon.h>

// Flag to prevent recursive persistence calls during graph loading
static _Atomic bool g_loading_from_persistence = false;

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

static SemanticFabric* g_fabric = NULL;
static _Atomic bool g_initialized = false;

// Hash function optimized for SemanticID distribution
static inline uint32_t semantic_hash(SemanticID id) {
    // Use MurmurHash3 finalizer - excellent distribution
    uint64_t h = id;
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return (uint32_t)(h & (NOUS_FABRIC_SHARDS - 1));
}

// Generate unique SemanticID
static SemanticID generate_semantic_id(SemanticType type) {
    static _Atomic uint64_t counter = 0;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    // Pack: [timestamp_ns:40][type:8][counter:16]
    // - timestamp occupies bits 24-63
    // - type occupies bits 16-23 (see SEMANTIC_TYPE_MASK/SHIFT)
    // - counter occupies bits 0-15
    uint64_t time_part = ((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec) & 0xFFFFFFFFFFULL;
    uint64_t type_part = ((uint64_t)type & 0xFF) << 16;
    uint64_t count_part = (atomic_fetch_add(&counter, 1) & 0xFFFF);

    return (time_part << 24) | type_part | count_part;
}

// ============================================================================
// SIMD-OPTIMIZED EMBEDDING OPERATIONS (ARM NEON)
// ============================================================================

/**
 * Compute cosine similarity using NEON SIMD
 * Processes 8 float16 values per iteration
 */
float nous_embedding_similarity_neon(const NousEmbedding* a, const NousEmbedding* b) {
    float32x4_t dot_sum = vdupq_n_f32(0.0f);
    float32x4_t norm_a_sum = vdupq_n_f32(0.0f);
    float32x4_t norm_b_sum = vdupq_n_f32(0.0f);

    // Process 8 elements at a time (768 / 8 = 96 iterations)
    for (size_t i = 0; i < NOUS_EMBEDDING_DIM; i += 8) {
        // Load 8 float16 values and convert to float32
        float16x8_t va_f16 = vld1q_f16((const float16_t*)&a->values[i]);
        float16x8_t vb_f16 = vld1q_f16((const float16_t*)&b->values[i]);

        // Convert to float32 (low and high halves)
        float32x4_t va_lo = vcvt_f32_f16(vget_low_f16(va_f16));
        float32x4_t va_hi = vcvt_f32_f16(vget_high_f16(va_f16));
        float32x4_t vb_lo = vcvt_f32_f16(vget_low_f16(vb_f16));
        float32x4_t vb_hi = vcvt_f32_f16(vget_high_f16(vb_f16));

        // Accumulate dot product
        dot_sum = vfmaq_f32(dot_sum, va_lo, vb_lo);
        dot_sum = vfmaq_f32(dot_sum, va_hi, vb_hi);

        // Accumulate squared norms
        norm_a_sum = vfmaq_f32(norm_a_sum, va_lo, va_lo);
        norm_a_sum = vfmaq_f32(norm_a_sum, va_hi, va_hi);
        norm_b_sum = vfmaq_f32(norm_b_sum, vb_lo, vb_lo);
        norm_b_sum = vfmaq_f32(norm_b_sum, vb_hi, vb_hi);
    }

    // Horizontal reduction
    float dot = vaddvq_f32(dot_sum);
    float norm_a = vaddvq_f32(norm_a_sum);
    float norm_b = vaddvq_f32(norm_b_sum);

    // Cosine similarity with numerical stability
    float denom = sqrtf(norm_a) * sqrtf(norm_b);
    if (denom < 1e-8f) return 0.0f;

    return dot / denom;
}

/**
 * Batch embedding update using NEON
 * Used for incremental learning
 */
void nous_embedding_blend_neon(NousEmbedding* target,
                                const NousEmbedding* source,
                                float alpha) {
    float32x4_t valpha = vdupq_n_f32(alpha);
    float32x4_t v1_alpha = vdupq_n_f32(1.0f - alpha);

    for (size_t i = 0; i < NOUS_EMBEDDING_DIM; i += 8) {
        float16x8_t vt_f16 = vld1q_f16((const float16_t*)&target->values[i]);
        float16x8_t vs_f16 = vld1q_f16((const float16_t*)&source->values[i]);

        float32x4_t vt_lo = vcvt_f32_f16(vget_low_f16(vt_f16));
        float32x4_t vt_hi = vcvt_f32_f16(vget_high_f16(vt_f16));
        float32x4_t vs_lo = vcvt_f32_f16(vget_low_f16(vs_f16));
        float32x4_t vs_hi = vcvt_f32_f16(vget_high_f16(vs_f16));

        // Blend: target = target * (1-alpha) + source * alpha
        vt_lo = vfmaq_f32(vmulq_f32(vt_lo, v1_alpha), vs_lo, valpha);
        vt_hi = vfmaq_f32(vmulq_f32(vt_hi, v1_alpha), vs_hi, valpha);

        // Convert back to float16 and store
        float16x8_t result = vcombine_f16(vcvt_f16_f32(vt_lo), vcvt_f16_f32(vt_hi));
        vst1q_f16((float16_t*)&target->values[i], result);
    }
}

// ============================================================================
// FABRIC INITIALIZATION
// ============================================================================

int nous_init(void) {
    if (atomic_exchange(&g_initialized, true)) {
        return 0;  // Already initialized
    }

    // Use posix_memalign for portable aligned allocation
    void* ptr = NULL;
    if (posix_memalign(&ptr, NOUS_CACHE_LINE, sizeof(SemanticFabric)) != 0) {
        atomic_store(&g_initialized, false);
        return -1;
    }
    g_fabric = ptr;
    if (!g_fabric) {
        atomic_store(&g_initialized, false);
        return -1;
    }
    memset(g_fabric, 0, sizeof(SemanticFabric));

    // Initialize shards
    for (size_t i = 0; i < NOUS_FABRIC_SHARDS; i++) {
        FabricShard* shard = &g_fabric->shards[i];
        shard->capacity = NOUS_SHARD_INITIAL_CAP;
        shard->count = 0;
        shard->nodes = calloc(NOUS_SHARD_INITIAL_CAP, sizeof(NousSemanticNode*));
        shard->lock = OS_UNFAIR_LOCK_INIT;

        if (!shard->nodes) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                free(g_fabric->shards[j].nodes);
            }
            free(g_fabric);
            g_fabric = NULL;
            atomic_store(&g_initialized, false);
            return -1;
        }
    }

    // Create dispatch queues optimized for Apple Silicon topology
    dispatch_queue_attr_t p_attr = dispatch_queue_attr_make_with_qos_class(
        DISPATCH_QUEUE_CONCURRENT, QOS_CLASS_USER_INTERACTIVE, 0);
    g_fabric->p_core_queue = dispatch_queue_create("nous.p_cores", p_attr);

    dispatch_queue_attr_t e_attr = dispatch_queue_attr_make_with_qos_class(
        DISPATCH_QUEUE_CONCURRENT, QOS_CLASS_UTILITY, 0);
    g_fabric->e_core_queue = dispatch_queue_create("nous.e_cores", e_attr);

    dispatch_queue_attr_t gpu_attr = dispatch_queue_attr_make_with_qos_class(
        DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, 0);
    g_fabric->gpu_queue = dispatch_queue_create("nous.gpu", gpu_attr);

    // Load persisted graph on startup
    atomic_store(&g_loading_from_persistence, true);
    int loaded = sem_persist_load_graph(NOUS_MAX_LOADED_NODES);
    atomic_store(&g_loading_from_persistence, false);

    if (loaded > 0) {
        // Silently loaded nodes from persistence
    }

    return 0;
}

void nous_shutdown(void) {
    if (!atomic_exchange(&g_initialized, false)) {
        return;
    }

    if (!g_fabric) return;

    // Release all nodes
    for (size_t i = 0; i < NOUS_FABRIC_SHARDS; i++) {
        FabricShard* shard = &g_fabric->shards[i];
        for (size_t j = 0; j < shard->count; j++) {
            NousSemanticNode* node = shard->nodes[j];
            if (node) {
                free(node->essence);
                free(node->relations);
                free(node->relation_strengths);
                free(node);
            }
        }
        free(shard->nodes);
    }

    // Release queues
    if (g_fabric->p_core_queue) dispatch_release(g_fabric->p_core_queue);
    if (g_fabric->e_core_queue) dispatch_release(g_fabric->e_core_queue);
    if (g_fabric->gpu_queue) dispatch_release(g_fabric->gpu_queue);

    free(g_fabric);
    g_fabric = NULL;
}

bool nous_is_ready(void) {
    return atomic_load(&g_initialized) && g_fabric != NULL;
}

// ============================================================================
// NODE OPERATIONS
// ============================================================================

/**
 * Internal function to create a node with optional id_override.
 * Used by persistence layer to restore nodes with their original IDs.
 * Also accepts optional embedding, importance, and context/creator IDs.
 */
SemanticID nous_create_node_internal(
    SemanticType type,
    const char* essence,
    SemanticID id_override,
    const float* embedding,
    size_t embedding_dim,
    SemanticID creator_id,
    SemanticID context_id,
    float importance
) {
    if (!nous_is_ready() || !essence) return SEMANTIC_ID_NULL;

    // Use provided ID or generate new one
    SemanticID id = (id_override != SEMANTIC_ID_NULL) ? id_override : generate_semantic_id(type);
    uint32_t shard_idx = semantic_hash(id);
    FabricShard* shard = &g_fabric->shards[shard_idx];

    // Check if node already exists (when loading from persistence)
    if (id_override != SEMANTIC_ID_NULL) {
        os_unfair_lock_lock(&shard->lock);
        for (size_t i = 0; i < shard->count; i++) {
            if (shard->nodes[i]->id == id) {
                os_unfair_lock_unlock(&shard->lock);
                return id;  // Already loaded
            }
        }
        os_unfair_lock_unlock(&shard->lock);
    }

    // Allocate node (cache-line aligned)
    NousSemanticNode* node = aligned_alloc(NOUS_CACHE_LINE,
                                           sizeof(NousSemanticNode));
    if (!node) return SEMANTIC_ID_NULL;

    memset(node, 0, sizeof(NousSemanticNode));
    node->id = id;
    node->type = type;
    node->lock = OS_UNFAIR_LOCK_INIT;
    node->ref_count = 1;
    node->importance = importance;
    node->creator_id = creator_id;
    node->context_id = context_id;

    // Copy essence
    node->essence_len = strlen(essence);
    node->essence = malloc(node->essence_len + 1);
    if (!node->essence) {
        free(node);
        return SEMANTIC_ID_NULL;
    }
    memcpy(node->essence, essence, node->essence_len + 1);

    // Copy embedding if provided
    if (embedding && embedding_dim > 0 && embedding_dim <= NOUS_EMBEDDING_DIM) {
        for (size_t i = 0; i < embedding_dim; i++) {
            node->embedding.values[i] = (__fp16)embedding[i];
        }
    }

    // Initialize timestamps
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    node->created_at = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    node->last_accessed = node->created_at;

    // Insert into shard with lock
    os_unfair_lock_lock(&shard->lock);

    // Grow if needed
    if (shard->count >= shard->capacity) {
        size_t new_cap = shard->capacity * 2;
        NousSemanticNode** new_nodes = realloc(shard->nodes,
                                               new_cap * sizeof(NousSemanticNode*));
        if (!new_nodes) {
            os_unfair_lock_unlock(&shard->lock);
            free(node->essence);
            free(node);
            return SEMANTIC_ID_NULL;
        }
        shard->nodes = new_nodes;
        shard->capacity = new_cap;
    }

    shard->nodes[shard->count++] = node;
    os_unfair_lock_unlock(&shard->lock);

    atomic_fetch_add(&g_fabric->total_nodes, 1);

    return id;
}

SemanticID nous_create_node(SemanticType type, const char* essence) {
    if (!nous_is_ready() || !essence) return SEMANTIC_ID_NULL;

    SemanticID id = generate_semantic_id(type);
    uint32_t shard_idx = semantic_hash(id);
    FabricShard* shard = &g_fabric->shards[shard_idx];

    // Allocate node (cache-line aligned)
    NousSemanticNode* node = aligned_alloc(NOUS_CACHE_LINE,
                                           sizeof(NousSemanticNode));
    if (!node) return SEMANTIC_ID_NULL;

    memset(node, 0, sizeof(NousSemanticNode));
    node->id = id;
    node->type = type;
    node->lock = OS_UNFAIR_LOCK_INIT;
    node->ref_count = 1;
    node->importance = 0.5f;  // Default importance

    // Copy essence
    node->essence_len = strlen(essence);
    node->essence = malloc(node->essence_len + 1);
    if (!node->essence) {
        free(node);
        return SEMANTIC_ID_NULL;
    }
    memcpy(node->essence, essence, node->essence_len + 1);

    // Initialize timestamps
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    node->created_at = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    node->last_accessed = node->created_at;

    // Insert into shard with lock
    os_unfair_lock_lock(&shard->lock);

    // Grow if needed
    if (shard->count >= shard->capacity) {
        size_t new_cap = shard->capacity * 2;
        NousSemanticNode** new_nodes = realloc(shard->nodes,
                                               new_cap * sizeof(NousSemanticNode*));
        if (!new_nodes) {
            os_unfair_lock_unlock(&shard->lock);
            free(node->essence);
            free(node);
            return SEMANTIC_ID_NULL;
        }
        shard->nodes = new_nodes;
        shard->capacity = new_cap;
    }

    shard->nodes[shard->count++] = node;
    os_unfair_lock_unlock(&shard->lock);

    atomic_fetch_add(&g_fabric->total_nodes, 1);

    // Persist to SQLite (write-through) if not loading from persistence
    if (!atomic_load(&g_loading_from_persistence)) {
        sem_persist_save_node(
            id, type, essence,
            NULL, 0,  // No embedding yet
            SEMANTIC_ID_NULL, SEMANTIC_ID_NULL,  // No creator/context
            0.5f  // Default importance
        );
    }

    return id;
}

NousSemanticNode* nous_get_node(SemanticID id) {
    if (!nous_is_ready() || id == SEMANTIC_ID_NULL) return NULL;

    uint32_t shard_idx = semantic_hash(id);
    FabricShard* shard = &g_fabric->shards[shard_idx];

    os_unfair_lock_lock(&shard->lock);

    for (size_t i = 0; i < shard->count; i++) {
        if (shard->nodes[i]->id == id) {
            NousSemanticNode* node = shard->nodes[i];
            node->ref_count++;

            // Update access time (non-blocking on P-cores)
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            node->last_accessed = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
            node->access_count++;

            os_unfair_lock_unlock(&shard->lock);
            return node;
        }
    }

    os_unfair_lock_unlock(&shard->lock);

    // Node not in memory - try loading from persistence (on-demand)
    // Skip if we're already loading from persistence to prevent infinite recursion
    if (!atomic_load(&g_loading_from_persistence)) {
        atomic_store(&g_loading_from_persistence, true);
        int loaded = sem_persist_load_node(id);
        atomic_store(&g_loading_from_persistence, false);

        if (loaded == 0) {
            // Node was loaded, try to get it again
            os_unfair_lock_lock(&shard->lock);
            for (size_t i = 0; i < shard->count; i++) {
                if (shard->nodes[i]->id == id) {
                    NousSemanticNode* node = shard->nodes[i];
                    node->ref_count++;
                    os_unfair_lock_unlock(&shard->lock);
                    return node;
                }
            }
            os_unfair_lock_unlock(&shard->lock);
        }
    }

    return NULL;
}

void nous_release_node(NousSemanticNode* node) {
    if (!node) return;

    os_unfair_lock_lock(&node->lock);
    uint32_t new_count = --node->ref_count;
    os_unfair_lock_unlock(&node->lock);

    // Note: actual deletion is handled by garbage collection on E-cores
    (void)new_count;
}

int nous_delete_node(SemanticID id) {
    if (!nous_is_ready() || id == SEMANTIC_ID_NULL) return -1;

    uint32_t shard_idx = semantic_hash(id);
    FabricShard* shard = &g_fabric->shards[shard_idx];

    os_unfair_lock_lock(&shard->lock);

    for (size_t i = 0; i < shard->count; i++) {
        if (shard->nodes[i]->id == id) {
            NousSemanticNode* node = shard->nodes[i];

            // Check ref_count - don't delete if other threads are using it
            os_unfair_lock_lock(&node->lock);
            if (node->ref_count > 1) {
                os_unfair_lock_unlock(&node->lock);
                os_unfair_lock_unlock(&shard->lock);
                return -2;  // Node still in use
            }
            os_unfair_lock_unlock(&node->lock);

            // Free node resources
            free(node->essence);
            free(node->relations);
            free(node->relation_strengths);
            free(node);

            // Shift remaining nodes down
            for (size_t j = i; j < shard->count - 1; j++) {
                shard->nodes[j] = shard->nodes[j + 1];
            }
            shard->count--;

            os_unfair_lock_unlock(&shard->lock);

            atomic_fetch_sub(&g_fabric->total_nodes, 1);
            return 0;
        }
    }

    os_unfair_lock_unlock(&shard->lock);
    return -1;  // Not found in memory (might only be in DB)
}

// ============================================================================
// RELATION OPERATIONS
// ============================================================================

int nous_connect(SemanticID from, SemanticID to, float strength) {
    if (strength < 0.0f || strength > 1.0f) return -1;

    NousSemanticNode* node = nous_get_node(from);
    if (!node) return -1;

    os_unfair_lock_lock(&node->lock);

    // Check for existing relation
    for (size_t i = 0; i < node->relation_count; i++) {
        if (node->relations[i] == to) {
            // Update strength (exponential moving average)
            float new_strength = 0.7f * node->relation_strengths[i] + 0.3f * strength;
            node->relation_strengths[i] = new_strength;
            os_unfair_lock_unlock(&node->lock);
            nous_release_node(node);

            // Update persistence if not loading from persistence
            if (!atomic_load(&g_loading_from_persistence)) {
                sem_persist_update_relation(from, to, new_strength);
            }
            return 0;
        }
    }

    // Add new relation
    if (node->relation_count >= node->relation_capacity) {
        size_t new_cap = node->relation_capacity == 0 ? 8 : node->relation_capacity * 2;

        SemanticID* new_rels = realloc(node->relations, new_cap * sizeof(SemanticID));
        float* new_strengths = realloc(node->relation_strengths, new_cap * sizeof(float));

        if (!new_rels || !new_strengths) {
            os_unfair_lock_unlock(&node->lock);
            nous_release_node(node);
            return -1;
        }

        node->relations = new_rels;
        node->relation_strengths = new_strengths;
        node->relation_capacity = new_cap;
    }

    node->relations[node->relation_count] = to;
    node->relation_strengths[node->relation_count] = strength;
    node->relation_count++;

    os_unfair_lock_unlock(&node->lock);
    nous_release_node(node);

    atomic_fetch_add(&g_fabric->total_relations, 1);

    // Persist new relation if not loading from persistence
    if (!atomic_load(&g_loading_from_persistence)) {
        sem_persist_save_relation(from, to, strength, "related");
    }

    return 0;
}

// ============================================================================
// PARALLEL SIMILARITY SEARCH (P-CORES)
// ============================================================================

typedef struct {
    const NousEmbedding* query;
    SimilarityResult* results;
    size_t max_results;
    _Atomic size_t result_count;
    os_unfair_lock results_lock;
} SimilarityContext;

static void search_shard(void* ctx, size_t shard_idx) {
    SimilarityContext* search = ctx;
    FabricShard* shard = &g_fabric->shards[shard_idx];

    os_unfair_lock_lock(&shard->lock);

    for (size_t i = 0; i < shard->count; i++) {
        NousSemanticNode* node = shard->nodes[i];

        float sim = nous_embedding_similarity_neon(search->query, &node->embedding);

        // Insert into results if high enough
        os_unfair_lock_lock(&search->results_lock);

        size_t count = atomic_load(&search->result_count);
        if (count < search->max_results || sim > search->results[count - 1].similarity) {
            // Find insertion point
            size_t insert_at = 0;
            for (size_t j = 0; j < count; j++) {
                if (sim > search->results[j].similarity) {
                    insert_at = j;
                    break;
                }
                insert_at = j + 1;
            }

            if (insert_at < search->max_results) {
                // Shift lower elements
                size_t shift_count = count - insert_at;
                if (count >= search->max_results) shift_count--;
                if (shift_count > 0) {
                    memmove(&search->results[insert_at + 1],
                            &search->results[insert_at],
                            shift_count * sizeof(SimilarityResult));
                }

                search->results[insert_at].id = node->id;
                search->results[insert_at].similarity = sim;

                if (count < search->max_results) {
                    atomic_fetch_add(&search->result_count, 1);
                }
            }
        }

        os_unfair_lock_unlock(&search->results_lock);
    }

    os_unfair_lock_unlock(&shard->lock);
}

size_t nous_find_similar(const NousEmbedding* query,
                         size_t max_results,
                         SimilarityResult* results) {
    if (!nous_is_ready() || !query || !results || max_results == 0) return 0;

    SimilarityContext ctx = {
        .query = query,
        .results = results,
        .max_results = max_results,
        .result_count = 0,
        .results_lock = OS_UNFAIR_LOCK_INIT
    };

    // Parallel search across all shards using P-cores
    dispatch_apply(NOUS_FABRIC_SHARDS, g_fabric->p_core_queue, ^(size_t idx) {
        search_shard((void*)&ctx, idx);
    });

    atomic_fetch_add(&g_fabric->queries_processed, 1);

    return atomic_load(&ctx.result_count);
}

// ============================================================================
// STATISTICS
// ============================================================================

size_t nous_get_node_count(void) {
    if (!nous_is_ready()) return 0;
    return atomic_load(&g_fabric->total_nodes);
}
