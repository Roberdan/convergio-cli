/**
 * CONVERGIO PERSISTENT MEMORY
 *
 * SQLite-based storage for:
 * - Conversation history
 * - Agent definitions and state
 * - User preferences
 * - Cost tracking over time
 */

#include "nous/orchestrator.h"
#include "nous/config.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>
#include "nous/debug_mutex.h"

// Database handle
static sqlite3* g_db = NULL;
CONVERGIO_MUTEX_DECLARE(g_db_mutex);

// Get DB path from config, fallback to default
static const char* get_db_path(void) {
    const char* path = convergio_config_get("db_path");
    if (path && strlen(path) > 0) {
        return path;
    }
    // Fallback for when config not initialized
    return "data/convergio.db";
}

// Ensure parent directory exists
static int ensure_db_directory(const char* db_path) {
    if (!db_path) return -1;

    // Find last slash to get directory
    char dir[512];
    strncpy(dir, db_path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';

    char* last_slash = strrchr(dir, '/');
    if (!last_slash) return 0;  // No directory component

    *last_slash = '\0';  // Truncate to get directory only

    // Create directory recursively
    struct stat st;
    if (stat(dir, &st) == 0) {
        return 0;  // Already exists
    }

    // Try to create it
    if (mkdir(dir, 0755) == 0) {
        return 0;
    }

    // Try parent directory first
    char* parent_slash = strrchr(dir, '/');
    if (parent_slash) {
        *parent_slash = '\0';
        if (strlen(dir) > 0) {
            mkdir(dir, 0755);  // Create parent
        }
        *parent_slash = '/';
        mkdir(dir, 0755);  // Try again
    }

    return 0;
}

// ============================================================================
// SCHEMA
// ============================================================================

static const char* SCHEMA_SQL =
    // Messages table
    "CREATE TABLE IF NOT EXISTS messages ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  session_id TEXT NOT NULL,"
    "  type INTEGER NOT NULL,"
    "  sender_id INTEGER,"
    "  sender_name TEXT,"
    "  recipient_id INTEGER,"
    "  content TEXT NOT NULL,"
    "  metadata_json TEXT,"
    "  input_tokens INTEGER DEFAULT 0,"
    "  output_tokens INTEGER DEFAULT 0,"
    "  cost_usd REAL DEFAULT 0,"
    "  parent_id INTEGER,"
    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");"

    // Agent definitions
    "CREATE TABLE IF NOT EXISTS agents ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  name TEXT UNIQUE NOT NULL,"
    "  role INTEGER NOT NULL,"
    "  system_prompt TEXT NOT NULL,"
    "  specialized_context TEXT,"
    "  color TEXT,"
    "  tools_json TEXT,"
    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");"

    // Agent usage statistics
    "CREATE TABLE IF NOT EXISTS agent_usage ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  agent_name TEXT NOT NULL,"
    "  session_id TEXT NOT NULL,"
    "  input_tokens INTEGER DEFAULT 0,"
    "  output_tokens INTEGER DEFAULT 0,"
    "  cost_usd REAL DEFAULT 0,"
    "  api_calls INTEGER DEFAULT 0,"
    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");"

    // User preferences
    "CREATE TABLE IF NOT EXISTS user_prefs ("
    "  key TEXT PRIMARY KEY,"
    "  value TEXT NOT NULL,"
    "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");"

    // Session tracking
    "CREATE TABLE IF NOT EXISTS sessions ("
    "  id TEXT PRIMARY KEY,"
    "  user_name TEXT,"
    "  total_cost REAL DEFAULT 0,"
    "  total_messages INTEGER DEFAULT 0,"
    "  started_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "  ended_at DATETIME"
    ");"

    // Cost history (daily aggregates)
    "CREATE TABLE IF NOT EXISTS cost_history ("
    "  date TEXT PRIMARY KEY,"
    "  input_tokens INTEGER DEFAULT 0,"
    "  output_tokens INTEGER DEFAULT 0,"
    "  total_cost REAL DEFAULT 0,"
    "  api_calls INTEGER DEFAULT 0"
    ");"

    // Semantic memories (for future RAG)
    "CREATE TABLE IF NOT EXISTS memories ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  content TEXT NOT NULL,"
    "  category TEXT DEFAULT 'general',"
    "  embedding BLOB,"
    "  importance REAL DEFAULT 0.5,"
    "  access_count INTEGER DEFAULT 0,"
    "  last_accessed DATETIME,"
    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");"

    // Indexes for performance
    "CREATE INDEX IF NOT EXISTS idx_messages_session ON messages(session_id);"
    "CREATE INDEX IF NOT EXISTS idx_messages_created ON messages(created_at);"
    "CREATE INDEX IF NOT EXISTS idx_agent_usage_agent ON agent_usage(agent_name);"
    "CREATE INDEX IF NOT EXISTS idx_memories_importance ON memories(importance DESC);";

// ============================================================================
// INITIALIZATION
// ============================================================================

int persistence_init(const char* db_path) {
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    if (g_db != NULL) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return 0;  // Already initialized
    }

    // Use provided path, or get from config, or fallback to default
    const char* path = db_path ? db_path : get_db_path();

    // Ensure the directory exists before opening
    ensure_db_directory(path);

    int rc = sqlite3_open(path, &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Convergio: Cannot open database '%s': %s\n", path, sqlite3_errmsg(g_db));
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    // Enable WAL mode for better concurrency
    char* err_msg = NULL;
    rc = sqlite3_exec(g_db, "PRAGMA journal_mode=WAL;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
    }

    // Create schema
    rc = sqlite3_exec(g_db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Convergio: Schema creation failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_db);
        g_db = NULL;
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
    return 0;
}

void persistence_shutdown(void) {
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
}

// ============================================================================
// MESSAGE PERSISTENCE
// ============================================================================

int persistence_save_message(const char* session_id, Message* msg) {
    if (!g_db || !session_id || !msg) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "INSERT INTO messages (session_id, type, sender_id, content, metadata_json, "
        "input_tokens, output_tokens, cost_usd, parent_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, msg->type);
    sqlite3_bind_int64(stmt, 3, msg->sender);
    sqlite3_bind_text(stmt, 4, msg->content, -1, SQLITE_STATIC);
    // Guard against NULL metadata_json (would crash strlen in sqlite3_bind_text)
    if (msg->metadata_json) {
        sqlite3_bind_text(stmt, 5, msg->metadata_json, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 5);
    }
    sqlite3_bind_int64(stmt, 6, msg->tokens_used.input_tokens);
    sqlite3_bind_int64(stmt, 7, msg->tokens_used.output_tokens);
    sqlite3_bind_double(stmt, 8, msg->tokens_used.estimated_cost);
    sqlite3_bind_int64(stmt, 9, msg->parent_id);

    rc = sqlite3_step(stmt);
    int64_t new_id = sqlite3_last_insert_rowid(g_db);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    if (rc == SQLITE_DONE) {
        msg->id = new_id;
        return 0;
    }
    return -1;
}

// Load recent messages for context
Message** persistence_load_recent_messages(const char* session_id, size_t limit, size_t* out_count) {
    if (!g_db || !session_id || !out_count) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "SELECT id, type, sender_id, content, metadata_json, "
        "input_tokens, output_tokens, cost_usd, parent_id, created_at "
        "FROM messages WHERE session_id = ? "
        "ORDER BY created_at DESC LIMIT ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)limit);

    // Count results first
    Message** messages = malloc(sizeof(Message*) * limit);
    if (!messages) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < limit) {
        Message* msg = calloc(1, sizeof(Message));
        if (!msg) break;

        msg->id = sqlite3_column_int64(stmt, 0);
        msg->type = sqlite3_column_int(stmt, 1);
        msg->sender = sqlite3_column_int64(stmt, 2);

        const char* content = (const char*)sqlite3_column_text(stmt, 3);
        msg->content = content ? strdup(content) : NULL;

        const char* metadata = (const char*)sqlite3_column_text(stmt, 4);
        msg->metadata_json = metadata ? strdup(metadata) : NULL;

        msg->tokens_used.input_tokens = sqlite3_column_int64(stmt, 5);
        msg->tokens_used.output_tokens = sqlite3_column_int64(stmt, 6);
        msg->tokens_used.estimated_cost = sqlite3_column_double(stmt, 7);
        msg->parent_id = sqlite3_column_int64(stmt, 8);

        messages[count++] = msg;
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    *out_count = count;
    return messages;
}

// ============================================================================
// AGENT PERSISTENCE
// ============================================================================

int persistence_save_agent(const char* name, AgentRole role, const char* system_prompt,
                           const char* context, const char* color, const char* tools_json) {
    if (!g_db || !name || !system_prompt) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "INSERT OR REPLACE INTO agents (name, role, system_prompt, specialized_context, color, tools_json, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, role);
    sqlite3_bind_text(stmt, 3, system_prompt, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, context, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, color, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, tools_json, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Load agent definition
char* persistence_load_agent_prompt(const char* name) {
    if (!g_db || !name) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql = "SELECT system_prompt FROM agents WHERE name = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    char* prompt = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) prompt = strdup(text);
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return prompt;
}

// ============================================================================
// USER PREFERENCES
// ============================================================================

int persistence_set_pref(const char* key, const char* value) {
    if (!g_db || !key || !value) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "INSERT OR REPLACE INTO user_prefs (key, value, updated_at) "
        "VALUES (?, ?, CURRENT_TIMESTAMP)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

char* persistence_get_pref(const char* key) {
    if (!g_db || !key) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql = "SELECT value FROM user_prefs WHERE key = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

    char* value = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) value = strdup(text);
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return value;
}

// ============================================================================
// COST PERSISTENCE
// ============================================================================

int persistence_save_cost_daily(const char* date, uint64_t input_tokens,
                                 uint64_t output_tokens, double cost, uint32_t calls) {
    if (!g_db || !date) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "INSERT INTO cost_history (date, input_tokens, output_tokens, total_cost, api_calls) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON CONFLICT(date) DO UPDATE SET "
        "input_tokens = input_tokens + excluded.input_tokens, "
        "output_tokens = output_tokens + excluded.output_tokens, "
        "total_cost = total_cost + excluded.total_cost, "
        "api_calls = api_calls + excluded.api_calls";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, date, -1, SQLITE_STATIC);
    // Clamp uint64_t to int64_t max to prevent overflow (extremely unlikely in practice)
    int64_t safe_input = (input_tokens > INT64_MAX) ? INT64_MAX : (int64_t)input_tokens;
    int64_t safe_output = (output_tokens > INT64_MAX) ? INT64_MAX : (int64_t)output_tokens;
    sqlite3_bind_int64(stmt, 2, safe_input);
    sqlite3_bind_int64(stmt, 3, safe_output);
    sqlite3_bind_double(stmt, 4, cost);
    sqlite3_bind_int(stmt, 5, calls);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Get total historical cost
double persistence_get_total_cost(void) {
    if (!g_db) return 0.0;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql = "SELECT SUM(total_cost) FROM cost_history";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return 0.0;
    }

    double total = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_double(stmt, 0);
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return total;
}

// ============================================================================
// MEMORY/RAG WITH SEMANTIC SEARCH
// ============================================================================

// Forward declaration of MLX embedding functions
extern float* mlx_embed_text(const char* text, size_t* out_dim);
extern float mlx_cosine_similarity(const float* a, const float* b, size_t dim);
extern size_t mlx_get_embedding_dim(void);

int persistence_save_memory(const char* content, const char* category, float importance) {
    if (!g_db || !content) return -1;

    // Generate embedding using MLX local transformer
    size_t embed_dim = 0;
    float* embedding = mlx_embed_text(content, &embed_dim);

    // Default category if not provided
    const char* cat = (category && category[0]) ? category : "general";

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "INSERT INTO memories (content, category, embedding, importance) VALUES (?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        if (embedding) free(embedding);
        return -1;
    }

    // Use SQLITE_TRANSIENT to make SQLite copy the data immediately
    // This prevents use-after-free if the data is freed before finalize
    sqlite3_bind_text(stmt, 1, content, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, cat, -1, SQLITE_TRANSIENT);

    if (embedding && embed_dim > 0) {
        sqlite3_bind_blob(stmt, 3, embedding, (int)(embed_dim * sizeof(float)), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    sqlite3_bind_double(stmt, 4, importance);

    rc = sqlite3_step(stmt);
    int64_t new_id = sqlite3_last_insert_rowid(g_db);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    if (embedding) free(embedding);

    return (rc == SQLITE_DONE) ? (int)new_id : -1;
}

// Search memories by importance
char** persistence_get_important_memories(size_t limit, size_t* out_count) {
    if (!g_db || !out_count) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "SELECT content FROM memories "
        "ORDER BY importance DESC, access_count DESC LIMIT ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, (int)limit);

    char** memories = malloc(sizeof(char*) * limit);
    if (!memories) {
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < limit) {
        const char* text = (const char*)sqlite3_column_text(stmt, 0);
        memories[count++] = text ? strdup(text) : NULL;
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    *out_count = count;
    return memories;
}

// Semantic search using embeddings
typedef struct {
    char* content;
    float similarity;
} MemoryMatch;

static int compare_matches(const void* a, const void* b) {
    const MemoryMatch* ma = (const MemoryMatch*)a;
    const MemoryMatch* mb = (const MemoryMatch*)b;
    if (mb->similarity > ma->similarity) return 1;
    if (mb->similarity < ma->similarity) return -1;
    return 0;
}

char** persistence_search_memories(const char* query, size_t max_results,
                                    float min_similarity, size_t* out_count) {
    if (!g_db || !query || !out_count) return NULL;

    // Generate query embedding
    // Simple keyword search (MLX embeddings unreliable without pre-trained weights)
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    // Use LIKE search for keywords
    char search_sql[512];
    snprintf(search_sql, sizeof(search_sql),
        "SELECT content FROM memories WHERE content LIKE '%%%s%%' "
        "ORDER BY importance DESC LIMIT %zu", query, max_results);

    sqlite3_stmt* keyword_stmt;
    int kw_rc = sqlite3_prepare_v2(g_db, search_sql, -1, &keyword_stmt, NULL);
    if (kw_rc == SQLITE_OK) {
        char** results = malloc(sizeof(char*) * max_results);
        size_t count = 0;

        while (sqlite3_step(keyword_stmt) == SQLITE_ROW && count < max_results) {
            const char* text = (const char*)sqlite3_column_text(keyword_stmt, 0);
            if (text) results[count++] = strdup(text);
        }
        sqlite3_finalize(keyword_stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

        if (count > 0) {
            *out_count = count;
            return results;
        }
        free(results);
    } else {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
    }

    // Fallback to returning all important memories
    return persistence_get_important_memories(max_results, out_count);
}

// Semantic search using MLX embeddings (unused until pre-trained weights available)
__attribute__((unused))
static char** persistence_search_memories_semantic(const char* query, size_t max_results,
                                                    float min_similarity, size_t* out_count) {
    if (!g_db || !query || !out_count) return NULL;

    // Generate query embedding
    size_t query_dim = 0;
    float* query_embedding = mlx_embed_text(query, &query_dim);
    if (!query_embedding || query_dim == 0) {
        return persistence_get_important_memories(max_results, out_count);
    }

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    // Load all memories with embeddings
    const char* sql = "SELECT id, content, embedding FROM memories WHERE embedding IS NOT NULL";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        free(query_embedding);
        return NULL;
    }

    // Collect and score matches
    size_t capacity = 64;
    MemoryMatch* matches = malloc(sizeof(MemoryMatch) * capacity);
    size_t match_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* content = (const char*)sqlite3_column_text(stmt, 1);
        const void* blob = sqlite3_column_blob(stmt, 2);
        int blob_size = sqlite3_column_bytes(stmt, 2);

        if (!blob || blob_size != (int)(query_dim * sizeof(float))) continue;

        // Calculate similarity
        const float* mem_embedding = (const float*)blob;
        float similarity = mlx_cosine_similarity(query_embedding, mem_embedding, query_dim);

        if (similarity >= min_similarity) {
            if (match_count >= capacity) {
                capacity *= 2;
                matches = realloc(matches, sizeof(MemoryMatch) * capacity);
            }
            matches[match_count].content = strdup(content);
            matches[match_count].similarity = similarity;
            match_count++;
        }
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
    free(query_embedding);

    if (match_count == 0) {
        free(matches);
        *out_count = 0;
        return NULL;
    }

    // Sort by similarity (descending)
    qsort(matches, match_count, sizeof(MemoryMatch), compare_matches);

    // Return top results
    size_t result_count = match_count < max_results ? match_count : max_results;
    char** results = malloc(sizeof(char*) * result_count);

    for (size_t i = 0; i < result_count; i++) {
        results[i] = matches[i].content;
    }

    // Free remaining matches not returned
    for (size_t i = result_count; i < match_count; i++) {
        free(matches[i].content);
    }
    free(matches);

    // Update access counts for returned memories
    CONVERGIO_MUTEX_LOCK(&g_db_mutex);
    const char* update_sql = "UPDATE memories SET access_count = access_count + 1, "
                             "last_accessed = CURRENT_TIMESTAMP WHERE content = ?";

    for (size_t i = 0; i < result_count; i++) {
        sqlite3_stmt* update_stmt;
        if (sqlite3_prepare_v2(g_db, update_sql, -1, &update_stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(update_stmt, 1, results[i], -1, SQLITE_STATIC);
            sqlite3_step(update_stmt);
            sqlite3_finalize(update_stmt);
        }
    }
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    *out_count = result_count;
    return results;
}

// ============================================================================
// SESSION MANAGEMENT
// ============================================================================

char* persistence_create_session(const char* user_name) {
    if (!g_db) return NULL;

    // Generate session ID
    char session_id[64];
    snprintf(session_id, sizeof(session_id), "session_%ld_%d",
             (long)time(NULL), rand() % 10000);

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql = "INSERT INTO sessions (id, user_name) VALUES (?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? strdup(session_id) : NULL;
}

int persistence_end_session(const char* session_id, double total_cost, int total_messages) {
    if (!g_db || !session_id) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "UPDATE sessions SET ended_at = CURRENT_TIMESTAMP, "
        "total_cost = ?, total_messages = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    sqlite3_bind_double(stmt, 1, total_cost);
    sqlite3_bind_int(stmt, 2, total_messages);
    sqlite3_bind_text(stmt, 3, session_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// ============================================================================
// CONVERSATION HISTORY
// ============================================================================

// Save a conversation turn (user message + assistant response)
int persistence_save_conversation(const char* session_id, const char* role,
                                   const char* content, int tokens) {
    if (!g_db || !session_id || !role || !content) return -1;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "INSERT INTO messages (session_id, type, sender_name, content, input_tokens) "
        "VALUES (?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return -1;
    }

    int msg_type = (strcmp(role, "user") == 0) ? 1 : 2;  // 1=user, 2=assistant

    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, msg_type);
    sqlite3_bind_text(stmt, 3, role, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, content, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, tokens);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Load conversation history for context (most recent first, reversed for chronological order)
char* persistence_load_conversation_context(const char* session_id, size_t max_messages) {
    if (!g_db || !session_id) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "SELECT sender_name, content FROM ("
        "  SELECT sender_name, content, created_at FROM messages "
        "  WHERE session_id = ? ORDER BY created_at DESC LIMIT ?"
        ") ORDER BY created_at ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)max_messages);

    // Build conversation string
    size_t capacity = 16384;
    char* context = malloc(capacity);
    context[0] = '\0';
    size_t len = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* role = (const char*)sqlite3_column_text(stmt, 0);
        const char* content = (const char*)sqlite3_column_text(stmt, 1);

        if (!role || !content) continue;

        // Format: [role]: content\n\n
        size_t needed = strlen(role) + strlen(content) + 8;
        if (len + needed >= capacity) {
            capacity *= 2;
            context = realloc(context, capacity);
        }

        len += snprintf(context + len, capacity - len, "[%s]: %s\n\n", role, content);
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    if (len == 0) {
        free(context);
        return NULL;
    }

    return context;
}

// Load recent conversations across all sessions for long-term memory
char* persistence_load_recent_context(size_t max_messages) {
    if (!g_db) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    const char* sql =
        "SELECT sender_name, content, session_id, date(created_at) as day FROM ("
        "  SELECT sender_name, content, session_id, created_at FROM messages "
        "  ORDER BY created_at DESC LIMIT ?"
        ") ORDER BY created_at ASC";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, (int)max_messages);

    size_t capacity = 32768;
    char* context = malloc(capacity);
    context[0] = '\0';
    size_t len = 0;
    char last_session[64] = "";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* role = (const char*)sqlite3_column_text(stmt, 0);
        const char* content = (const char*)sqlite3_column_text(stmt, 1);
        const char* session = (const char*)sqlite3_column_text(stmt, 2);
        const char* day = (const char*)sqlite3_column_text(stmt, 3);

        if (!role || !content) continue;

        // Add session separator if new session
        if (session && strcmp(session, last_session) != 0) {
            size_t needed = 64;
            if (len + needed >= capacity) {
                capacity *= 2;
                context = realloc(context, capacity);
            }
            len += snprintf(context + len, capacity - len,
                "\n--- Session %s ---\n", day ? day : "unknown");
            strncpy(last_session, session, sizeof(last_session) - 1);
        }

        size_t needed = strlen(role) + strlen(content) + 8;
        if (len + needed >= capacity) {
            capacity *= 2;
            context = realloc(context, capacity);
        }

        len += snprintf(context + len, capacity - len, "[%s]: %s\n\n", role, content);
    }

    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    if (len == 0) {
        free(context);
        return NULL;
    }

    return context;
}

// Get last session ID or create new one
char* persistence_get_or_create_session(void) {
    if (!g_db) return NULL;

    CONVERGIO_MUTEX_LOCK(&g_db_mutex);

    // Try to get most recent active session (from today)
    const char* sql =
        "SELECT id FROM sessions "
        "WHERE date(started_at) = date('now') AND ended_at IS NULL "
        "ORDER BY started_at DESC LIMIT 1";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        const char* id = (const char*)sqlite3_column_text(stmt, 0);
        char* session_id = strdup(id);
        sqlite3_finalize(stmt);
        CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);
        return session_id;
    }
    sqlite3_finalize(stmt);
    CONVERGIO_MUTEX_UNLOCK(&g_db_mutex);

    // No active session, create new one
    return persistence_create_session("default");
}
