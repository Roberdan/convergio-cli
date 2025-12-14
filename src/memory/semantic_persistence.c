/**
 * CONVERGIO - Semantic Graph Persistence Implementation
 *
 * SQLite-based persistence for the semantic knowledge graph.
 */

#include "nous/semantic_persistence.h"
#include "nous/nous.h"
#include "nous/debug_mutex.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// External database handle from persistence.c
extern sqlite3* g_db;
extern ConvergioMutex g_db_mutex;

// External fabric functions (declared in nous.h)
// nous_create_node_internal, nous_get_node, nous_release_node, nous_connect

// ============================================================================
// NODE PERSISTENCE
// ============================================================================

int sem_persist_save_node(
    SemanticID id,
    SemanticType type,
    const char* essence,
    const float* embedding,
    size_t embedding_dim,
    SemanticID creator_id,
    SemanticID context_id,
    float importance
) {
    if (!g_db || !essence) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "INSERT OR REPLACE INTO semantic_nodes "
        "(id, type, essence, embedding, creator_id, context_id, importance, "
        "access_count, created_at, last_accessed) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, 0, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    int64_t now = (int64_t)time(NULL);

    sqlite3_bind_int64(stmt, 1, (int64_t)id);
    sqlite3_bind_int(stmt, 2, (int)type);
    sqlite3_bind_text(stmt, 3, essence, -1, SQLITE_TRANSIENT);

    if (embedding && embedding_dim > 0) {
        sqlite3_bind_blob(stmt, 4, embedding, (int)(embedding_dim * sizeof(float)), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    sqlite3_bind_int64(stmt, 5, (int64_t)creator_id);
    sqlite3_bind_int64(stmt, 6, (int64_t)context_id);
    sqlite3_bind_double(stmt, 7, (double)importance);
    sqlite3_bind_int64(stmt, 8, now);
    sqlite3_bind_int64(stmt, 9, now);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int sem_persist_load_node(SemanticID id) {
    if (!g_db || id == SEMANTIC_ID_NULL) return -1;

    // Check if already in memory
    NousSemanticNode* existing = nous_get_node(id);
    if (existing) {
        nous_release_node(existing);
        return 0;  // Already loaded
    }

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "SELECT type, essence, embedding, creator_id, context_id, importance "
        "FROM semantic_nodes WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (int64_t)id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;  // Not found
    }

    SemanticType type = (SemanticType)sqlite3_column_int(stmt, 0);
    const char* essence = (const char*)sqlite3_column_text(stmt, 1);

    // Extract embedding if present
    const void* embedding_blob = sqlite3_column_blob(stmt, 2);
    int embedding_bytes = sqlite3_column_bytes(stmt, 2);
    size_t embedding_dim = (embedding_bytes > 0) ? (size_t)embedding_bytes / sizeof(float) : 0;

    // Extract other fields
    SemanticID creator_id = (SemanticID)sqlite3_column_int64(stmt, 3);
    SemanticID context_id = (SemanticID)sqlite3_column_int64(stmt, 4);
    float importance = (float)sqlite3_column_double(stmt, 5);

    // Create node in fabric with specific ID and all data
    SemanticID created = nous_create_node_internal(
        type, essence, id,
        (const float*)embedding_blob, embedding_dim,
        creator_id, context_id, importance
    );

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (created != SEMANTIC_ID_NULL) ? 0 : -1;
}

int sem_persist_update_importance(SemanticID id, float importance) {
    if (!g_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql = "UPDATE semantic_nodes SET importance = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_double(stmt, 1, (double)importance);
    sqlite3_bind_int64(stmt, 2, (int64_t)id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int sem_persist_touch_node(SemanticID id) {
    if (!g_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "UPDATE semantic_nodes SET access_count = access_count + 1, "
        "last_accessed = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (int64_t)time(NULL));
    sqlite3_bind_int64(stmt, 2, (int64_t)id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int sem_persist_delete_node(SemanticID id) {
    if (!g_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    // Delete relations first (CASCADE should handle this, but be explicit)
    const char* del_rel_sql =
        "DELETE FROM semantic_relations WHERE from_id = ? OR to_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, del_rel_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, (int64_t)id);
        sqlite3_bind_int64(stmt, 2, (int64_t)id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Delete node
    const char* del_node_sql = "DELETE FROM semantic_nodes WHERE id = ?";
    rc = sqlite3_prepare_v2(g_db, del_node_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (int64_t)id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

bool sem_persist_node_exists(SemanticID id) {
    if (!g_db) return false;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql = "SELECT 1 FROM semantic_nodes WHERE id = ? LIMIT 1";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return false;
    }

    sqlite3_bind_int64(stmt, 1, (int64_t)id);
    rc = sqlite3_step(stmt);
    bool exists = (rc == SQLITE_ROW);

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return exists;
}

// ============================================================================
// RELATION PERSISTENCE
// ============================================================================

int sem_persist_save_relation(
    SemanticID from_id,
    SemanticID to_id,
    float strength,
    const char* relation_type
) {
    if (!g_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "INSERT OR REPLACE INTO semantic_relations "
        "(from_id, to_id, strength, relation_type, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    int64_t now = (int64_t)time(NULL);

    sqlite3_bind_int64(stmt, 1, (int64_t)from_id);
    sqlite3_bind_int64(stmt, 2, (int64_t)to_id);
    sqlite3_bind_double(stmt, 3, (double)strength);
    sqlite3_bind_text(stmt, 4, relation_type ? relation_type : "related", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, now);
    sqlite3_bind_int64(stmt, 6, now);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int sem_persist_update_relation(
    SemanticID from_id,
    SemanticID to_id,
    float new_strength
) {
    if (!g_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "UPDATE semantic_relations SET strength = ?, updated_at = ? "
        "WHERE from_id = ? AND to_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_double(stmt, 1, (double)new_strength);
    sqlite3_bind_int64(stmt, 2, (int64_t)time(NULL));
    sqlite3_bind_int64(stmt, 3, (int64_t)from_id);
    sqlite3_bind_int64(stmt, 4, (int64_t)to_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

SemanticRelation* sem_persist_load_relations(SemanticID node_id, size_t* out_count) {
    if (!g_db || !out_count) return NULL;
    *out_count = 0;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "SELECT to_id, strength, relation_type FROM semantic_relations "
        "WHERE from_id = ? ORDER BY strength DESC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_int64(stmt, 1, (int64_t)node_id);

    // Count first
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) count++;
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    SemanticRelation* relations = calloc(count, sizeof(SemanticRelation));
    if (!relations) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        relations[i].target_id = (SemanticID)sqlite3_column_int64(stmt, 0);
        relations[i].strength = (float)sqlite3_column_double(stmt, 1);
        const char* type = (const char*)sqlite3_column_text(stmt, 2);
        relations[i].relation_type = type ? strdup(type) : strdup("related");

        // Check for strdup failure (memory exhaustion)
        if (!relations[i].relation_type) {
            // Clean up previously allocated strings
            for (size_t j = 0; j < i; j++) {
                free(relations[j].relation_type);
            }
            free(relations);
            sqlite3_finalize(stmt);
            CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
            *out_count = 0;
            return NULL;
        }
        i++;
    }

    *out_count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return relations;
}

int sem_persist_delete_relation(SemanticID from_id, SemanticID to_id) {
    if (!g_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "DELETE FROM semantic_relations WHERE from_id = ? AND to_id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (int64_t)from_id);
    sqlite3_bind_int64(stmt, 2, (int64_t)to_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// GRAPH OPERATIONS
// ============================================================================

int sem_persist_load_graph(size_t max_nodes) {
    if (!g_db) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    // Load nodes by importance (most important first)
    const char* sql =
        "SELECT id, type, essence, embedding, creator_id, context_id, importance "
        "FROM semantic_nodes "
        "ORDER BY importance DESC, access_count DESC LIMIT ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (int64_t)max_nodes);

    int loaded = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SemanticID id = (SemanticID)sqlite3_column_int64(stmt, 0);
        SemanticType type = (SemanticType)sqlite3_column_int(stmt, 1);
        const char* essence = (const char*)sqlite3_column_text(stmt, 2);

        // Extract embedding if present
        const void* embedding_blob = sqlite3_column_blob(stmt, 3);
        int embedding_bytes = sqlite3_column_bytes(stmt, 3);
        size_t embedding_dim = (embedding_bytes > 0) ? (size_t)embedding_bytes / sizeof(float) : 0;

        // Extract other fields
        SemanticID creator_id = (SemanticID)sqlite3_column_int64(stmt, 4);
        SemanticID context_id = (SemanticID)sqlite3_column_int64(stmt, 5);
        float importance = (float)sqlite3_column_double(stmt, 6);

        if (essence) {
            SemanticID created = nous_create_node_internal(
                type, essence, id,
                (const float*)embedding_blob, embedding_dim,
                creator_id, context_id, importance
            );
            if (created != SEMANTIC_ID_NULL) {
                loaded++;
            }
        }
    }

    sqlite3_finalize(stmt);

    // Load relations only for nodes that were loaded (using subquery for efficiency)
    // This avoids loading all relations and filtering in memory
    const char* rel_sql =
        "SELECT r.from_id, r.to_id, r.strength FROM semantic_relations r "
        "WHERE r.from_id IN (SELECT id FROM semantic_nodes ORDER BY importance DESC, access_count DESC LIMIT ?) "
        "AND r.to_id IN (SELECT id FROM semantic_nodes ORDER BY importance DESC, access_count DESC LIMIT ?)";

    rc = sqlite3_prepare_v2(g_db, rel_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, (int64_t)max_nodes);
        sqlite3_bind_int64(stmt, 2, (int64_t)max_nodes);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            SemanticID from = (SemanticID)sqlite3_column_int64(stmt, 0);
            SemanticID to = (SemanticID)sqlite3_column_int64(stmt, 1);
            float strength = (float)sqlite3_column_double(stmt, 2);

            nous_connect(from, to, strength);
        }
        sqlite3_finalize(stmt);
    }

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    LOG_INFO(LOG_CAT_MEMORY, "Loaded %d semantic nodes from persistence", loaded);
    return loaded;
}

GraphStats sem_persist_get_stats(void) {
    GraphStats stats = {0};

    if (!g_db) return stats;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    // Total nodes
    const char* sql = "SELECT COUNT(*) FROM semantic_nodes";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_nodes = (size_t)sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Total relations
    sql = "SELECT COUNT(*) FROM semantic_relations";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_relations = (size_t)sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    // Nodes by type
    sql = "SELECT type, COUNT(*) FROM semantic_nodes GROUP BY type";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int type = sqlite3_column_int(stmt, 0);
            if (type >= 0 && type < 16) {
                stats.nodes_by_type[type] = (size_t)sqlite3_column_int64(stmt, 1);
            }
        }
        sqlite3_finalize(stmt);
    }

    // In-memory count from fabric
    extern size_t nous_get_node_count(void);
    stats.nodes_in_memory = nous_get_node_count();

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return stats;
}

SemanticID* sem_persist_load_by_type(SemanticType type, size_t limit, size_t* out_count) {
    if (!g_db || !out_count) return NULL;
    *out_count = 0;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "SELECT id FROM semantic_nodes WHERE type = ? "
        "ORDER BY importance DESC LIMIT ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, (int)type);
    sqlite3_bind_int64(stmt, 2, (int64_t)limit);

    // Count first
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) count++;
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    SemanticID* ids = calloc(count, sizeof(SemanticID));
    if (!ids) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        ids[i++] = (SemanticID)sqlite3_column_int64(stmt, 0);
    }

    *out_count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return ids;
}

SemanticID* sem_persist_load_important(size_t limit, float min_importance, size_t* out_count) {
    if (!g_db || !out_count) return NULL;
    *out_count = 0;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "SELECT id FROM semantic_nodes WHERE importance >= ? "
        "ORDER BY importance DESC, access_count DESC LIMIT ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_double(stmt, 1, (double)min_importance);
    sqlite3_bind_int64(stmt, 2, (int64_t)limit);

    // Count first
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) count++;
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    SemanticID* ids = calloc(count, sizeof(SemanticID));
    if (!ids) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        ids[i++] = (SemanticID)sqlite3_column_int64(stmt, 0);
    }

    *out_count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return ids;
}

// Helper to escape SQL LIKE wildcards: %, _, and backslash
static void escape_like_pattern(const char* src, char* dest, size_t dest_size) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 1 < dest_size; ++i) {
        if (src[i] == '%' || src[i] == '_' || src[i] == '\\') {
            if (j + 2 >= dest_size) break;
            dest[j++] = '\\';
        }
        dest[j++] = src[i];
    }
    dest[j] = '\0';
}

SemanticID* sem_persist_search_essence(const char* query, size_t limit, size_t* out_count) {
    if (!g_db || !query || !out_count) return NULL;
    *out_count = 0;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    // Escape LIKE wildcards to prevent SQL injection via wildcard abuse
    char escaped_query[256];
    escape_like_pattern(query, escaped_query, sizeof(escaped_query));
    char search_pattern[512];
    snprintf(search_pattern, sizeof(search_pattern), "%%%s%%", escaped_query);

    const char* sql =
        "SELECT id FROM semantic_nodes WHERE essence LIKE ? ESCAPE '\\' "
        "ORDER BY importance DESC LIMIT ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, search_pattern, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, (int64_t)limit);

    // Count first
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) count++;
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    SemanticID* ids = calloc(count, sizeof(SemanticID));
    if (!ids) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        ids[i++] = (SemanticID)sqlite3_column_int64(stmt, 0);
    }

    *out_count = i;
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return ids;
}

// ============================================================================
// MIGRATION
// ============================================================================

// Structure to hold memory data for migration
typedef struct {
    char* content;
    char* category;
    float importance;
} MigrationMemory;

int sem_persist_migrate_memories(void) {
    if (!g_db) return -1;

    // Phase 1: Read all memory data while holding mutex (minimal blocking)
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    // Check if there are memories to migrate
    const char* count_sql = "SELECT COUNT(*) FROM memories";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, count_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    size_t total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = (size_t)sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (total == 0) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return 0;
    }

    // Allocate array to hold all memory data
    MigrationMemory* memories = calloc(total, sizeof(MigrationMemory));
    if (!memories) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    // Load all memories into array
    const char* sql =
        "SELECT content, category, importance FROM memories";

    rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(memories);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    size_t loaded = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && loaded < total) {
        const char* content = (const char*)sqlite3_column_text(stmt, 0);
        const char* category = (const char*)sqlite3_column_text(stmt, 1);

        if (content) {
            memories[loaded].content = strdup(content);
            memories[loaded].category = category ? strdup(category) : NULL;
            memories[loaded].importance = (float)sqlite3_column_double(stmt, 2);

            // Check for allocation failure
            if (!memories[loaded].content) {
                // Cleanup and abort
                for (size_t j = 0; j < loaded; j++) {
                    free(memories[j].content);
                    free(memories[j].category);
                }
                free(memories);
                sqlite3_finalize(stmt);
                CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
                return -1;
            }
            loaded++;
        }
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);  // Release mutex before node creation

    // Phase 2: Create semantic nodes (mutex released, no blocking)
    int migrated = 0;
    for (size_t i = 0; i < loaded; i++) {
        MigrationMemory* mem = &memories[i];

        SemanticID id = nous_create_node(SEMANTIC_TYPE_MEMORY, mem->content);
        if (id != SEMANTIC_ID_NULL) {
            // Update importance
            sem_persist_update_importance(id, mem->importance);

            // Create category relation if present
            if (mem->category && strlen(mem->category) > 0 &&
                strcmp(mem->category, "general") != 0) {
                SemanticID cat_id = nous_create_node(SEMANTIC_TYPE_CONCEPT, mem->category);
                if (cat_id != SEMANTIC_ID_NULL) {
                    nous_connect(id, cat_id, 0.7f);
                }
            }

            migrated++;
        }

        // Free memory data after processing
        free(mem->content);
        free(mem->category);
    }

    free(memories);

    LOG_INFO(LOG_CAT_MEMORY, "Migrated %d/%zu memories to semantic graph", migrated, total);
    return migrated;
}
