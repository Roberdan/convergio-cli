/**
 * NOUS Semantic Fabric Tests
 */

#include "nous/nous.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Testing %s...", #name); \
    fflush(stdout); \
    if (test_##name() == 0) { \
        printf(" OK\n"); \
        passed++; \
    } else { \
        printf(" FAILED\n"); \
        failed++; \
    } \
} while(0)

// ============================================================================
// TESTS
// ============================================================================

TEST(init_shutdown) {
    if (nous_init() != 0) return -1;
    if (!nous_is_ready()) return -1;
    nous_shutdown();
    if (nous_is_ready()) return -1;

    // Re-init for other tests
    return nous_init();
}

TEST(create_node) {
    SemanticID id = nous_create_node(SEMANTIC_TYPE_CONCEPT, "test concept");
    if (id == SEMANTIC_ID_NULL) return -1;

    NousSemanticNode* node = nous_get_node(id);
    if (!node) return -1;
    if (node->type != SEMANTIC_TYPE_CONCEPT) return -1;
    if (strcmp(node->essence, "test concept") != 0) return -1;

    nous_release_node(node);
    return 0;
}

TEST(create_many_nodes) {
    SemanticID ids[1000];

    for (int i = 0; i < 1000; i++) {
        char essence[64];
        snprintf(essence, sizeof(essence), "node %d", i);
        ids[i] = nous_create_node(SEMANTIC_TYPE_ENTITY, essence);
        if (ids[i] == SEMANTIC_ID_NULL) return -1;
    }

    // Verify all exist
    for (int i = 0; i < 1000; i++) {
        NousSemanticNode* node = nous_get_node(ids[i]);
        if (!node) return -1;
        nous_release_node(node);
    }

    return 0;
}

TEST(connect_nodes) {
    SemanticID a = nous_create_node(SEMANTIC_TYPE_CONCEPT, "source");
    SemanticID b = nous_create_node(SEMANTIC_TYPE_CONCEPT, "target");

    if (a == SEMANTIC_ID_NULL || b == SEMANTIC_ID_NULL) return -1;

    if (nous_connect(a, b, 0.8f) != 0) return -1;

    NousSemanticNode* node = nous_get_node(a);
    if (!node) return -1;
    if (node->relation_count != 1) return -1;
    if (node->relations[0] != b) return -1;
    if (node->relation_strengths[0] < 0.79f || node->relation_strengths[0] > 0.81f) return -1;

    nous_release_node(node);
    return 0;
}

TEST(update_connection_strength) {
    SemanticID a = nous_create_node(SEMANTIC_TYPE_CONCEPT, "source2");
    SemanticID b = nous_create_node(SEMANTIC_TYPE_CONCEPT, "target2");

    nous_connect(a, b, 0.5f);
    nous_connect(a, b, 1.0f);  // Should update, not duplicate

    NousSemanticNode* node = nous_get_node(a);
    if (!node) return -1;
    if (node->relation_count != 1) return -1;  // Should still be 1

    // Strength should be blended (0.7 * 0.5 + 0.3 * 1.0 = 0.65)
    if (node->relation_strengths[0] < 0.64f || node->relation_strengths[0] > 0.66f) return -1;

    nous_release_node(node);
    return 0;
}

TEST(semantic_id_types) {
    SemanticID concept = nous_create_node(SEMANTIC_TYPE_CONCEPT, "a concept");
    SemanticID entity = nous_create_node(SEMANTIC_TYPE_ENTITY, "an entity");
    SemanticID agent = nous_create_node(SEMANTIC_TYPE_AGENT, "an agent");

    SemanticType t1 = (concept & SEMANTIC_TYPE_MASK) >> SEMANTIC_TYPE_SHIFT;
    SemanticType t2 = (entity & SEMANTIC_TYPE_MASK) >> SEMANTIC_TYPE_SHIFT;
    SemanticType t3 = (agent & SEMANTIC_TYPE_MASK) >> SEMANTIC_TYPE_SHIFT;

    if (t1 != SEMANTIC_TYPE_CONCEPT) return -1;
    if (t2 != SEMANTIC_TYPE_ENTITY) return -1;
    if (t3 != SEMANTIC_TYPE_AGENT) return -1;

    return 0;
}

TEST(embedding_similarity_neon) {
    NousEmbedding a, b;

    // Initialize with known values
    for (int i = 0; i < NOUS_EMBEDDING_DIM; i++) {
        a.values[i] = (_Float16)(i % 10) / 10.0f;
        b.values[i] = (_Float16)(i % 10) / 10.0f;
    }

    // Identical vectors should have similarity ~1.0
    extern float nous_embedding_similarity_neon(const NousEmbedding*, const NousEmbedding*);
    float sim = nous_embedding_similarity_neon(&a, &b);

    if (sim < 0.99f) return -1;

    // Orthogonal-ish vectors should have low similarity
    for (int i = 0; i < NOUS_EMBEDDING_DIM; i++) {
        b.values[i] = (_Float16)((i + 5) % 10) / 10.0f;
    }

    sim = nous_embedding_similarity_neon(&a, &b);
    // Should still be reasonably high due to pattern
    if (sim < 0.5f || sim > 1.0f) return -1;

    return 0;
}

TEST(access_tracking) {
    SemanticID id = nous_create_node(SEMANTIC_TYPE_CONCEPT, "tracked");

    NousSemanticNode* node = nous_get_node(id);
    uint64_t first_access = node->last_accessed;
    uint64_t first_count = node->access_count;
    nous_release_node(node);

    // Small delay
    for (volatile int i = 0; i < 1000000; i++);

    node = nous_get_node(id);
    if (node->last_accessed <= first_access) return -1;
    if (node->access_count != first_count + 1) return -1;

    nous_release_node(node);
    return 0;
}

TEST(concurrent_access) {
    // Create shared node
    SemanticID shared = nous_create_node(SEMANTIC_TYPE_CONCEPT, "shared");

    // Spawn multiple threads accessing it
    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < 100; i++) {
        NousSemanticNode* node = nous_get_node(shared);
        if (node) {
            // Read essence
            volatile size_t len = strlen(node->essence);
            (void)len;
            nous_release_node(node);
        }
    }

    // Should not crash
    return 0;
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

static double measure_create_nodes(int count) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < count; i++) {
        char essence[32];
        snprintf(essence, sizeof(essence), "perf_node_%d", i);
        nous_create_node(SEMANTIC_TYPE_ENTITY, essence);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    return elapsed;
}

static double measure_similarity(int iterations) {
    NousEmbedding a, b;
    for (int i = 0; i < NOUS_EMBEDDING_DIM; i++) {
        a.values[i] = (_Float16)(rand() % 100) / 100.0f;
        b.values[i] = (_Float16)(rand() % 100) / 100.0f;
    }

    extern float nous_embedding_similarity_neon(const NousEmbedding*, const NousEmbedding*);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    volatile float sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += nous_embedding_similarity_neon(&a, &b);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    return elapsed;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("\n");
    printf("╔════════════════════════════════════╗\n");
    printf("║    NOUS Semantic Fabric Tests      ║\n");
    printf("╚════════════════════════════════════╝\n");
    printf("\n");

    int passed = 0, failed = 0;

    // Run tests
    RUN_TEST(init_shutdown);
    RUN_TEST(create_node);
    RUN_TEST(create_many_nodes);
    RUN_TEST(connect_nodes);
    RUN_TEST(update_connection_strength);
    RUN_TEST(semantic_id_types);
    RUN_TEST(embedding_similarity_neon);
    RUN_TEST(access_tracking);
    RUN_TEST(concurrent_access);

    printf("\n");
    printf("Results: %d passed, %d failed\n", passed, failed);

    // Performance benchmarks
    if (failed == 0) {
        printf("\n");
        printf("Performance benchmarks:\n");

        double t1 = measure_create_nodes(10000);
        printf("  Create 10K nodes: %.3f ms (%.0f nodes/sec)\n",
               t1 * 1000, 10000.0 / t1);

        double t2 = measure_similarity(100000);
        printf("  100K similarity ops: %.3f ms (%.2fM ops/sec)\n",
               t2 * 1000, 0.1 / t2);
    }

    printf("\n");

    nous_shutdown();

    return failed > 0 ? 1 : 0;
}
