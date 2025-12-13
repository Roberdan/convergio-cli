/**
 * CONVERGIO - Semantic Graph Persistence
 *
 * Persistent storage for the semantic knowledge graph.
 * Nodes and relations are stored in SQLite and loaded into
 * the in-memory fabric on startup.
 */

#ifndef NOUS_SEMANTIC_PERSISTENCE_H
#define NOUS_SEMANTIC_PERSISTENCE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "nous.h"

// ============================================================================
// TYPES
// ============================================================================

typedef struct {
    SemanticID target_id;
    float strength;
    char* relation_type;
} SemanticRelation;

typedef struct {
    size_t total_nodes;
    size_t total_relations;
    size_t nodes_in_memory;
    size_t nodes_by_type[16];  // Count per SemanticType
} GraphStats;

// ============================================================================
// NODE PERSISTENCE
// ============================================================================

/**
 * Save a semantic node to SQLite
 * Called by nous_create_node() for write-through caching
 */
int sem_persist_save_node(
    SemanticID id,
    SemanticType type,
    const char* essence,
    const float* embedding,
    size_t embedding_dim,
    SemanticID creator_id,
    SemanticID context_id,
    float importance
);

/**
 * Load a semantic node from SQLite into fabric
 * Returns 0 on success, -1 if not found
 */
int sem_persist_load_node(SemanticID id);

/**
 * Update node importance
 */
int sem_persist_update_importance(SemanticID id, float importance);

/**
 * Update node access statistics
 */
int sem_persist_touch_node(SemanticID id);

/**
 * Delete a semantic node and its relations
 */
int sem_persist_delete_node(SemanticID id);

/**
 * Check if node exists in database
 */
bool sem_persist_node_exists(SemanticID id);

// ============================================================================
// RELATION PERSISTENCE
// ============================================================================

/**
 * Save a relation between nodes
 */
int sem_persist_save_relation(
    SemanticID from_id,
    SemanticID to_id,
    float strength,
    const char* relation_type
);

/**
 * Update relation strength
 */
int sem_persist_update_relation(
    SemanticID from_id,
    SemanticID to_id,
    float new_strength
);

/**
 * Load all relations for a node
 * Caller must free results and relation_type strings
 */
SemanticRelation* sem_persist_load_relations(
    SemanticID node_id,
    size_t* out_count
);

/**
 * Delete a specific relation
 */
int sem_persist_delete_relation(SemanticID from_id, SemanticID to_id);

// ============================================================================
// GRAPH OPERATIONS
// ============================================================================

/**
 * Load high-priority nodes into fabric on startup
 * Returns number of nodes loaded
 */
int sem_persist_load_graph(size_t max_nodes);

/**
 * Get graph statistics
 */
GraphStats sem_persist_get_stats(void);

/**
 * Load nodes by type
 * Caller must free the returned array
 */
SemanticID* sem_persist_load_by_type(
    SemanticType type,
    size_t limit,
    size_t* out_count
);

/**
 * Load most important nodes
 * Caller must free the returned array
 */
SemanticID* sem_persist_load_important(
    size_t limit,
    float min_importance,
    size_t* out_count
);

/**
 * Search nodes by essence (keyword search)
 * Caller must free the returned array
 */
SemanticID* sem_persist_search_essence(
    const char* query,
    size_t limit,
    size_t* out_count
);

// ============================================================================
// MIGRATION
// ============================================================================

/**
 * Migrate old memories table to semantic_nodes
 * Returns number of memories migrated
 */
int sem_persist_migrate_memories(void);

#endif // NOUS_SEMANTIC_PERSISTENCE_H
