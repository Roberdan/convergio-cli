# Implementation Plan: Semantic Graph Persistence

**Version:** 1.0
**Date:** 2024-12-13
**Branch:** `feature/semantic-graph-persistence`
**ADR:** [ADR-007](../adr/ADR-007-semantic-graph-persistence.md)

---

## Overview

This plan details the complete implementation of persistent semantic graph storage for Convergio, enabling agents to build and retain knowledge across sessions.

---

## Phase 1: Database Schema & Migration

### 1.1 Create Schema Migration

**File:** `src/memory/migrations/007_semantic_graph.sql`

```sql
-- Migration 007: Semantic Graph Tables
-- Date: 2024-12-13

-- Semantic nodes table
CREATE TABLE IF NOT EXISTS semantic_nodes (
    id INTEGER PRIMARY KEY,
    type INTEGER NOT NULL,
    essence TEXT NOT NULL,
    embedding BLOB,
    creator_id INTEGER,
    context_id INTEGER,
    importance REAL DEFAULT 0.5,
    access_count INTEGER DEFAULT 0,
    created_at INTEGER NOT NULL,
    last_accessed INTEGER,
    metadata_json TEXT,
    CONSTRAINT chk_importance CHECK (importance >= 0 AND importance <= 1)
);

-- Semantic relations table
CREATE TABLE IF NOT EXISTS semantic_relations (
    from_id INTEGER NOT NULL,
    to_id INTEGER NOT NULL,
    strength REAL DEFAULT 0.5,
    relation_type TEXT DEFAULT 'related',
    created_at INTEGER NOT NULL,
    updated_at INTEGER,
    PRIMARY KEY (from_id, to_id),
    FOREIGN KEY (from_id) REFERENCES semantic_nodes(id) ON DELETE CASCADE,
    FOREIGN KEY (to_id) REFERENCES semantic_nodes(id) ON DELETE CASCADE,
    CONSTRAINT chk_strength CHECK (strength >= 0 AND strength <= 1)
);

-- Performance indexes
CREATE INDEX IF NOT EXISTS idx_nodes_type ON semantic_nodes(type);
CREATE INDEX IF NOT EXISTS idx_nodes_importance ON semantic_nodes(importance DESC);
CREATE INDEX IF NOT EXISTS idx_nodes_created ON semantic_nodes(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_nodes_context ON semantic_nodes(context_id);
CREATE INDEX IF NOT EXISTS idx_relations_from ON semantic_relations(from_id);
CREATE INDEX IF NOT EXISTS idx_relations_to ON semantic_relations(to_id);
CREATE INDEX IF NOT EXISTS idx_relations_type ON semantic_relations(relation_type);

-- Migration tracking
INSERT OR REPLACE INTO schema_version (version, applied_at)
VALUES (7, datetime('now'));
```

### 1.2 Update persistence.c Schema

**File:** `src/memory/persistence.c`

**Changes:**
- Add schema version table
- Add migration runner
- Add new table definitions to SCHEMA_SQL

**Tasks:**
- [ ] Add `schema_version` table
- [ ] Add migration runner function
- [ ] Update `persistence_init()` to run migrations
- [ ] Test schema creation on fresh DB

### 1.3 Files to Modify

| File | Changes |
|------|---------|
| `src/memory/persistence.c` | Add schema, migration logic |
| `include/nous/persistence.h` | New function declarations |

---

## Phase 2: Persistence Functions

### 2.1 Node Persistence

**File:** `src/memory/semantic_persistence.c` (NEW)

```c
// ============================================================================
// SEMANTIC NODE PERSISTENCE
// ============================================================================

/**
 * Save a semantic node to SQLite
 * Called by nous_create_node() for write-through caching
 */
int persistence_save_semantic_node(
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
 * Load a semantic node from SQLite
 * Returns NULL if not found
 */
NousSemanticNode* persistence_load_semantic_node(SemanticID id);

/**
 * Update node access statistics
 */
int persistence_touch_node(SemanticID id);

/**
 * Delete a semantic node and its relations
 */
int persistence_delete_semantic_node(SemanticID id);

/**
 * Load nodes by type (for startup)
 */
SemanticID* persistence_load_nodes_by_type(
    SemanticType type,
    size_t limit,
    size_t* out_count
);

/**
 * Load most important nodes (for context)
 */
SemanticID* persistence_load_important_nodes(
    size_t limit,
    float min_importance,
    size_t* out_count
);
```

### 2.2 Relation Persistence

```c
// ============================================================================
// SEMANTIC RELATION PERSISTENCE
// ============================================================================

/**
 * Save a relation between nodes
 */
int persistence_save_relation(
    SemanticID from_id,
    SemanticID to_id,
    float strength,
    const char* relation_type
);

/**
 * Update relation strength
 */
int persistence_update_relation_strength(
    SemanticID from_id,
    SemanticID to_id,
    float new_strength
);

/**
 * Load all relations for a node
 */
typedef struct {
    SemanticID target_id;
    float strength;
    char* relation_type;
} SemanticRelation;

SemanticRelation* persistence_load_relations(
    SemanticID node_id,
    size_t* out_count
);

/**
 * Find nodes connected to given node
 */
SemanticID* persistence_find_connected(
    SemanticID node_id,
    size_t max_depth,
    size_t* out_count
);

/**
 * Delete a specific relation
 */
int persistence_delete_relation(SemanticID from_id, SemanticID to_id);
```

### 2.3 Graph Loading

```c
// ============================================================================
// GRAPH LOADING (STARTUP)
// ============================================================================

/**
 * Load semantic graph into fabric on startup
 * Strategy: Load high-importance nodes first, lazy-load rest
 */
int persistence_load_graph_into_fabric(size_t initial_node_limit);

/**
 * Sync in-memory changes to disk
 * Called periodically or on shutdown
 */
int persistence_sync_fabric_to_disk(void);

/**
 * Get graph statistics
 */
typedef struct {
    size_t total_nodes;
    size_t total_relations;
    size_t nodes_in_memory;
    size_t nodes_on_disk_only;
} GraphStats;

GraphStats persistence_get_graph_stats(void);
```

### 2.4 Tasks

- [ ] Create `src/memory/semantic_persistence.c`
- [ ] Create `include/nous/semantic_persistence.h`
- [ ] Implement `persistence_save_semantic_node()`
- [ ] Implement `persistence_load_semantic_node()`
- [ ] Implement `persistence_save_relation()`
- [ ] Implement `persistence_load_relations()`
- [ ] Implement `persistence_load_graph_into_fabric()`
- [ ] Implement `persistence_sync_fabric_to_disk()`
- [ ] Add to Makefile
- [ ] Unit tests for each function

---

## Phase 3: Fabric Integration

### 3.1 Modify nous_create_node()

**File:** `src/core/fabric.c`

```c
SemanticID nous_create_node(SemanticType type, const char* essence) {
    if (!nous_is_ready() || !essence) return SEMANTIC_ID_NULL;

    SemanticID id = generate_semantic_id(type);

    // ... existing in-memory allocation code ...

    // Generate embedding for semantic search
    size_t embed_dim = 0;
    float* embedding = semantic_embed_text(essence, &embed_dim);
    if (embedding) {
        memcpy(node->embedding.values, embedding,
               sizeof(float) * MIN(embed_dim, NOUS_EMBEDDING_DIM));
        node->embedding.dimensions = embed_dim;
        free(embedding);
    }

    // ... existing shard insertion code ...

    // NEW: Persist to SQLite (write-through)
    persistence_save_semantic_node(
        id, type, essence,
        node->embedding.values, node->embedding.dimensions,
        node->creator, node->context,
        0.5f  // default importance
    );

    return id;
}
```

### 3.2 Modify nous_connect()

```c
int nous_connect(SemanticID from, SemanticID to, float strength) {
    if (strength < 0.0f || strength > 1.0f) return -1;

    // ... existing in-memory relation code ...

    // NEW: Persist relation to SQLite
    persistence_save_relation(from, to, strength, "related");

    return 0;
}
```

### 3.3 Add nous_init() Graph Loading

```c
int nous_init(void) {
    // ... existing initialization ...

    // NEW: Load persisted graph on startup
    int loaded = persistence_load_graph_into_fabric(1000); // Top 1000 nodes
    if (loaded > 0) {
        LOG_INFO(LOG_CAT_CORE, "Loaded %d semantic nodes from persistence", loaded);
    }

    return 0;
}
```

### 3.4 Add nous_shutdown() Sync

```c
void nous_shutdown(void) {
    // NEW: Sync any pending changes
    persistence_sync_fabric_to_disk();

    // ... existing shutdown code ...
}
```

### 3.5 Tasks

- [ ] Modify `nous_create_node()` with persistence
- [ ] Modify `nous_connect()` with persistence
- [ ] Modify `nous_init()` with graph loading
- [ ] Modify `nous_shutdown()` with sync
- [ ] Add lazy loading for on-demand node access
- [ ] Integration tests

---

## Phase 4: Embedding System

### 4.1 Local Embedding Model

**File:** `src/ml/embeddings.c` (NEW)

```c
// ============================================================================
// LOCAL EMBEDDING GENERATION
// ============================================================================

/**
 * Initialize embedding model (e5-small-v2)
 * Downloads model on first run (~130MB)
 */
int embeddings_init(void);

/**
 * Generate embedding for text
 * Returns 384-dimensional float array
 */
float* semantic_embed_text(const char* text, size_t* out_dim);

/**
 * Batch embed multiple texts (more efficient)
 */
float** semantic_embed_batch(
    const char** texts,
    size_t count,
    size_t* out_dim
);

/**
 * Compute cosine similarity between embeddings
 */
float semantic_similarity(
    const float* a,
    const float* b,
    size_t dim
);

/**
 * Find most similar nodes to query
 */
typedef struct {
    SemanticID id;
    float similarity;
} SimilarityResult;

SimilarityResult* semantic_find_similar(
    const char* query,
    size_t max_results,
    float min_similarity,
    size_t* out_count
);

/**
 * Shutdown and free model
 */
void embeddings_shutdown(void);
```

### 4.2 Model Management

**File:** `src/ml/model_manager.c` (NEW)

```c
/**
 * Check if embedding model is downloaded
 */
bool model_is_available(const char* model_name);

/**
 * Download model from HuggingFace
 * Shows progress indicator
 */
int model_download(const char* model_name, void (*on_progress)(float));

/**
 * Get model path
 */
const char* model_get_path(const char* model_name);

// Model constants
#define EMBEDDING_MODEL_NAME "intfloat/e5-small-v2"
#define EMBEDDING_MODEL_SIZE_MB 130
#define EMBEDDING_DIM 384
```

### 4.3 Tasks

- [ ] Create `src/ml/embeddings.c`
- [ ] Create `src/ml/model_manager.c`
- [ ] Integrate with MLX framework
- [ ] Implement model download with progress
- [ ] Implement text embedding
- [ ] Implement similarity search
- [ ] Add to Makefile
- [ ] Test embedding quality

---

## Phase 5: User Commands

### 5.1 New Commands

**File:** `src/core/commands/memory_commands.c` (NEW)

| Command | Description | Implementation |
|---------|-------------|----------------|
| `/remember "text"` | Store a memory with high importance | `cmd_remember()` |
| `/recall "query"` | Search memories semantically | `cmd_recall()` |
| `/memories` | List recent/important memories | `cmd_memories()` |
| `/forget <id>` | Delete a specific memory | `cmd_forget()` |
| `/graph` | Show graph statistics | `cmd_graph()` |
| `/connections <node>` | Show node's connections | `cmd_connections()` |

### 5.2 Command Implementations

```c
// /remember "important thing to remember"
int cmd_remember(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: remember <text>\n");
        printf("Example: remember \"Roberto prefers clean code\"\n");
        return -1;
    }

    // Join args
    char* content = join_args(argv + 1, argc - 1);

    // Create semantic node with high importance
    SemanticID id = nous_create_node(SEMANTIC_TYPE_MEMORY, content);
    if (id == SEMANTIC_ID_NULL) {
        printf("Failed to store memory.\n");
        free(content);
        return -1;
    }

    // Set high importance
    persistence_update_node_importance(id, 0.9f);

    printf("Remembered: \"%s\"\n", content);
    printf("Memory ID: 0x%llx\n", (unsigned long long)id);

    free(content);
    return 0;
}

// /recall "what does Roberto prefer?"
int cmd_recall(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: recall <query>\n");
        printf("Example: recall \"Roberto preferences\"\n");
        return -1;
    }

    char* query = join_args(argv + 1, argc - 1);

    // Semantic search
    size_t count = 0;
    SimilarityResult* results = semantic_find_similar(query, 10, 0.5f, &count);

    if (count == 0) {
        printf("No relevant memories found.\n");
        free(query);
        return 0;
    }

    printf("\nFound %zu relevant memories:\n\n", count);
    for (size_t i = 0; i < count; i++) {
        NousSemanticNode* node = nous_get_node(results[i].id);
        if (node) {
            printf("  %.0f%% │ %s\n",
                   results[i].similarity * 100,
                   node->essence);
            nous_release_node(node);
        }
    }

    free(results);
    free(query);
    return 0;
}

// /memories [limit]
int cmd_memories(int argc, char** argv) {
    size_t limit = 20;
    if (argc >= 2) {
        limit = atoi(argv[1]);
    }

    size_t count = 0;
    SemanticID* nodes = persistence_load_important_nodes(limit, 0.3f, &count);

    printf("\n=== Stored Memories (%zu) ===\n\n", count);

    for (size_t i = 0; i < count; i++) {
        NousSemanticNode* node = nous_get_node(nodes[i]);
        if (node) {
            printf("  [%.1f] %s\n",
                   node->importance,
                   node->essence);
            nous_release_node(node);
        }
    }

    free(nodes);
    return 0;
}

// /graph
int cmd_graph(int argc, char** argv) {
    (void)argc; (void)argv;

    GraphStats stats = persistence_get_graph_stats();

    printf("\n=== Semantic Graph Statistics ===\n\n");
    printf("  Total nodes:      %zu\n", stats.total_nodes);
    printf("  Total relations:  %zu\n", stats.total_relations);
    printf("  In memory:        %zu\n", stats.nodes_in_memory);
    printf("  On disk only:     %zu\n", stats.nodes_on_disk_only);
    printf("\n");

    // Node type breakdown
    printf("  By type:\n");
    for (int t = 0; t < SEMANTIC_TYPE_MAX; t++) {
        size_t type_count = persistence_count_nodes_by_type(t);
        if (type_count > 0) {
            printf("    %-12s: %zu\n",
                   semantic_type_name(t),
                   type_count);
        }
    }

    return 0;
}

// /connections <query>
int cmd_connections(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: connections <node_query>\n");
        return -1;
    }

    // Find node by essence search
    char* query = join_args(argv + 1, argc - 1);
    size_t count = 0;
    SimilarityResult* results = semantic_find_similar(query, 1, 0.5f, &count);

    if (count == 0) {
        printf("No matching node found.\n");
        free(query);
        return 0;
    }

    SemanticID node_id = results[0].id;
    NousSemanticNode* node = nous_get_node(node_id);

    printf("\nConnections for: %s\n\n", node->essence);

    // Load relations
    size_t rel_count = 0;
    SemanticRelation* relations = persistence_load_relations(node_id, &rel_count);

    for (size_t i = 0; i < rel_count; i++) {
        NousSemanticNode* target = nous_get_node(relations[i].target_id);
        if (target) {
            printf("  -[%.2f %s]-> %s\n",
                   relations[i].strength,
                   relations[i].relation_type,
                   target->essence);
            nous_release_node(target);
        }
        free(relations[i].relation_type);
    }

    free(relations);
    nous_release_node(node);
    free(results);
    free(query);
    return 0;
}
```

### 5.3 Register Commands

**File:** `src/core/commands/commands.c`

```c
static const ReplCommand COMMANDS[] = {
    // ... existing commands ...

    // Memory commands
    {"remember",    "Store an important memory",         cmd_remember},
    {"recall",      "Search memories semantically",      cmd_recall},
    {"memories",    "List stored memories",              cmd_memories},
    {"forget",      "Delete a memory",                   cmd_forget},
    {"graph",       "Show semantic graph stats",         cmd_graph},
    {"connections", "Show node connections",             cmd_connections},

    // ... rest of commands ...
};
```

### 5.4 Tasks

- [ ] Create `src/core/commands/memory_commands.c`
- [ ] Implement `cmd_remember()`
- [ ] Implement `cmd_recall()`
- [ ] Implement `cmd_memories()`
- [ ] Implement `cmd_forget()`
- [ ] Implement `cmd_graph()`
- [ ] Implement `cmd_connections()`
- [ ] Register commands in `commands.c`
- [ ] Add to Makefile
- [ ] Test all commands

---

## Phase 6: Agent Integration

### 6.1 Auto-Memorization

**File:** `src/orchestrator/orchestrator.c`

Agents should automatically create semantic nodes when they learn important things.

```c
// In agent system prompt, add:
"## Semantic Memory\n"
"You have access to a persistent semantic graph for long-term memory.\n"
"When you learn something important:\n"
"1. Use memory_store with importance 0.7-1.0 for key facts\n"
"2. The system automatically creates relationships\n"
"3. Previous memories are retrieved in context\n\n"
"Relationship types:\n"
"- 'is_a': Category membership (Roberto is_a developer)\n"
"- 'prefers': User preferences (Roberto prefers clean_code)\n"
"- 'works_on': Projects (Roberto works_on ConvergioCLI)\n"
"- 'related_to': General association\n"
```

### 6.2 Enhanced Context Loading

```c
char* orchestrator_load_context(const char* user_input) {
    // ... existing code ...

    // NEW: Semantic graph context
    size_t sem_count = 0;
    SimilarityResult* semantic = semantic_find_similar(
        user_input, 5, 0.6f, &sem_count
    );

    if (semantic && sem_count > 0) {
        len += snprintf(context + len, capacity - len,
            "## Related Knowledge (from semantic graph)\n");

        for (size_t i = 0; i < sem_count; i++) {
            NousSemanticNode* node = nous_get_node(semantic[i].id);
            if (node) {
                // Include the node
                len += snprintf(context + len, capacity - len,
                    "- %s\n", node->essence);

                // Include connected nodes
                size_t rel_count = 0;
                SemanticRelation* rels = persistence_load_relations(
                    node->id, &rel_count
                );
                for (size_t j = 0; j < MIN(rel_count, 3); j++) {
                    NousSemanticNode* related = nous_get_node(rels[j].target_id);
                    if (related) {
                        len += snprintf(context + len, capacity - len,
                            "  └─ %s: %s\n",
                            rels[j].relation_type,
                            related->essence);
                        nous_release_node(related);
                    }
                }
                free(rels);
                nous_release_node(node);
            }
        }
        free(semantic);
        len += snprintf(context + len, capacity - len, "\n");
    }

    return context;
}
```

### 6.3 New Tool: graph_relate

```c
// Tool to create explicit relationships
ToolResult* tool_graph_relate(
    const char* from_essence,
    const char* to_essence,
    const char* relation_type,
    float strength
) {
    // Find or create 'from' node
    SemanticID from_id = find_or_create_node(from_essence);

    // Find or create 'to' node
    SemanticID to_id = find_or_create_node(to_essence);

    // Create relation
    nous_connect(from_id, to_id, strength);
    persistence_save_relation(from_id, to_id, strength, relation_type);

    return result_success("Relationship created");
}
```

### 6.4 Tasks

- [ ] Update Ali's system prompt with semantic memory instructions
- [ ] Enhance `orchestrator_load_context()` with graph search
- [ ] Add `graph_relate` tool
- [ ] Add `graph_query` tool for traversal
- [ ] Test agent memorization
- [ ] Test context retrieval

---

## Phase 7: Help & Documentation

### 7.1 Update Help Command

**File:** `src/core/commands/commands.c`

```c
// In cmd_help(), add new section:
printf("\033[1;33m🧠 SEMANTIC MEMORY\033[0m  \033[2m(persistent knowledge graph)\033[0m\n");
printf("   \033[36mremember \"text\"\033[0m     Store important information permanently\n");
printf("   \033[36mrecall \"query\"\033[0m      Search memories by meaning (semantic search)\n");
printf("   \033[36mmemories\033[0m            List stored memories by importance\n");
printf("   \033[36mgraph\033[0m               Show knowledge graph statistics\n");
printf("   \033[36mconnections \"node\"\033[0m  Show what a concept is connected to\n");
printf("   \033[2m   Agents auto-remember important facts about you\033[0m\n\n");
```

### 7.2 Detailed Help Entries

```c
static const CommandHelp DETAILED_HELP[] = {
    // ... existing entries ...

    {
        "remember",
        "remember <text>",
        "Store important information in the semantic graph",
        "Creates a persistent memory node with high importance.\n"
        "The system generates embeddings for semantic search.\n"
        "Agents can also create memories automatically.",
        "remember \"I prefer TypeScript over JavaScript\"\n"
        "remember \"Project deadline is December 20th\"\n"
        "remember \"API key stored in ~/.env\""
    },
    {
        "recall",
        "recall <query>",
        "Search memories using semantic similarity",
        "Finds memories related to your query by meaning, not just keywords.\n"
        "Uses vector embeddings for intelligent matching.\n"
        "Returns up to 10 results with similarity scores.",
        "recall \"my language preferences\"\n"
        "recall \"project deadlines\"\n"
        "recall \"where are credentials stored\""
    },
    {
        "graph",
        "graph",
        "Display semantic graph statistics",
        "Shows the current state of the knowledge graph:\n"
        "- Total nodes and relations\n"
        "- Breakdown by node type\n"
        "- Memory vs disk distribution",
        "graph"
    },
    {
        "connections",
        "connections <query>",
        "Show connections for a node",
        "Finds a node by query and displays all its relationships.\n"
        "Shows connection type and strength for each edge.",
        "connections \"Roberto\"\n"
        "connections \"ConvergioCLI project\""
    },
};
```

### 7.3 Update README

**File:** `README.md`

Add section:

```markdown
## Semantic Memory

Convergio includes a persistent semantic knowledge graph that grows over time.

### How It Works

- **Automatic Learning**: Agents remember important facts about you
- **Semantic Search**: Find memories by meaning, not just keywords
- **Relationship Graph**: Concepts are connected with weighted edges
- **Persistent**: Survives restarts, stored in SQLite

### Commands

| Command | Description |
|---------|-------------|
| `/remember "text"` | Store important information |
| `/recall "query"` | Search by semantic similarity |
| `/memories` | List stored memories |
| `/graph` | Show graph statistics |
| `/connections "node"` | Show concept relationships |

### Example

```
> /remember "I prefer functional programming"
Remembered: "I prefer functional programming"

> /remember "Working on the auth module this week"
Remembered: "Working on the auth module this week"

> /recall "what am I working on?"
Found 2 relevant memories:

  95% │ Working on the auth module this week
  62% │ I prefer functional programming

> /graph
=== Semantic Graph Statistics ===

  Total nodes:      247
  Total relations:  412
  In memory:        247
  On disk only:     0
```
```

### 7.4 Tasks

- [ ] Update `cmd_help()` with memory section
- [ ] Add detailed help for new commands
- [ ] Update README.md
- [ ] Add CHANGELOG entry
- [ ] Update man page (if exists)

---

## Phase 8: Testing

### 8.1 Unit Tests

**File:** `tests/test_semantic_persistence.c`

```c
void test_save_and_load_node(void) {
    // Create and save node
    SemanticID id = nous_create_node(SEMANTIC_TYPE_MEMORY, "Test memory");
    assert(id != SEMANTIC_ID_NULL);

    // Verify in database
    NousSemanticNode* loaded = persistence_load_semantic_node(id);
    assert(loaded != NULL);
    assert(strcmp(loaded->essence, "Test memory") == 0);

    nous_release_node(loaded);
}

void test_save_and_load_relation(void) {
    SemanticID a = nous_create_node(SEMANTIC_TYPE_CONCEPT, "A");
    SemanticID b = nous_create_node(SEMANTIC_TYPE_CONCEPT, "B");

    nous_connect(a, b, 0.8f);

    // Verify relation persisted
    size_t count = 0;
    SemanticRelation* rels = persistence_load_relations(a, &count);
    assert(count == 1);
    assert(rels[0].target_id == b);
    assert(fabs(rels[0].strength - 0.8f) < 0.01f);

    free(rels);
}

void test_graph_survives_restart(void) {
    // Create nodes
    SemanticID id = nous_create_node(SEMANTIC_TYPE_MEMORY, "Persistent");

    // Shutdown
    nous_shutdown();
    persistence_shutdown();

    // Reinitialize
    persistence_init(NULL);
    nous_init();

    // Verify node still exists
    NousSemanticNode* loaded = persistence_load_semantic_node(id);
    assert(loaded != NULL);
    assert(strcmp(loaded->essence, "Persistent") == 0);

    nous_release_node(loaded);
}

void test_semantic_search(void) {
    nous_create_node(SEMANTIC_TYPE_MEMORY, "I love pizza");
    nous_create_node(SEMANTIC_TYPE_MEMORY, "My favorite food is pasta");
    nous_create_node(SEMANTIC_TYPE_MEMORY, "I work as a programmer");

    size_t count = 0;
    SimilarityResult* results = semantic_find_similar(
        "what food do I like?", 5, 0.3f, &count
    );

    assert(count >= 2);
    // Food-related memories should rank higher

    free(results);
}
```

### 8.2 Integration Tests

**File:** `tests/test_memory_commands.c`

```c
void test_remember_recall_flow(void) {
    // Simulate /remember command
    char* argv_remember[] = {"remember", "Roberto", "likes", "clean", "code"};
    int result = cmd_remember(5, argv_remember);
    assert(result == 0);

    // Simulate /recall command
    char* argv_recall[] = {"recall", "coding", "preferences"};
    result = cmd_recall(3, argv_recall);
    assert(result == 0);
    // Should find the memory
}
```

### 8.3 Performance Tests

```c
void test_large_graph_performance(void) {
    clock_t start = clock();

    // Create 10,000 nodes
    for (int i = 0; i < 10000; i++) {
        char essence[64];
        snprintf(essence, sizeof(essence), "Node %d", i);
        nous_create_node(SEMANTIC_TYPE_CONCEPT, essence);
    }

    clock_t create_time = clock() - start;
    printf("Create 10k nodes: %.2f seconds\n",
           (double)create_time / CLOCKS_PER_SEC);

    // Search performance
    start = clock();
    for (int i = 0; i < 100; i++) {
        size_t count = 0;
        SimilarityResult* results = semantic_find_similar(
            "test query", 10, 0.3f, &count
        );
        free(results);
    }

    clock_t search_time = clock() - start;
    printf("100 searches: %.2f seconds\n",
           (double)search_time / CLOCKS_PER_SEC);

    // Startup performance (reload graph)
    persistence_shutdown();
    nous_shutdown();

    start = clock();
    persistence_init(NULL);
    nous_init();

    clock_t startup_time = clock() - start;
    printf("Startup with 10k nodes: %.2f seconds\n",
           (double)startup_time / CLOCKS_PER_SEC);

    assert(startup_time < 2 * CLOCKS_PER_SEC);  // Must be under 2 seconds
}
```

### 8.4 Tasks

- [ ] Create `tests/test_semantic_persistence.c`
- [ ] Create `tests/test_memory_commands.c`
- [ ] Create `tests/test_semantic_performance.c`
- [ ] Add tests to Makefile
- [ ] Run all tests
- [ ] Performance benchmarks

---

## Phase 9: Migration & Cleanup

### 9.1 Migrate Existing Memories

```c
/**
 * Migrate old `memories` table to `semantic_nodes`
 */
int migrate_memories_to_semantic_nodes(void) {
    const char* sql =
        "SELECT id, content, category, importance, created_at "
        "FROM memories";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);

    int migrated = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* content = (const char*)sqlite3_column_text(stmt, 1);
        const char* category = (const char*)sqlite3_column_text(stmt, 2);
        float importance = sqlite3_column_double(stmt, 3);

        // Create semantic node
        SemanticID id = nous_create_node(SEMANTIC_TYPE_MEMORY, content);
        if (id != SEMANTIC_ID_NULL) {
            persistence_update_node_importance(id, importance);

            // Add category as relation if present
            if (category && strlen(category) > 0) {
                SemanticID cat_id = find_or_create_node(category);
                nous_connect(id, cat_id, 0.8f);
            }

            migrated++;
        }
    }

    sqlite3_finalize(stmt);

    LOG_INFO(LOG_CAT_MEMORY, "Migrated %d memories to semantic graph", migrated);
    return migrated;
}
```

### 9.2 Update memory_store Tool

```c
// OLD: Direct SQLite insert
ToolResult* tool_memory_store(const char* content, ...) {
    persistence_save_memory(content, category, importance);
}

// NEW: Create semantic node
ToolResult* tool_memory_store(const char* content, ...) {
    SemanticID id = nous_create_node(SEMANTIC_TYPE_MEMORY, content);
    if (id == SEMANTIC_ID_NULL) {
        return result_error("Failed to store memory");
    }

    persistence_update_node_importance(id, importance);

    // Create category relation if provided
    if (category && strlen(category) > 0) {
        SemanticID cat_id = find_or_create_category_node(category);
        nous_connect(id, cat_id, 0.7f);
    }

    return result_success("Memory stored in semantic graph");
}
```

### 9.3 Tasks

- [ ] Create migration function
- [ ] Run migration on existing data
- [ ] Update `tool_memory_store()`
- [ ] Update `tool_memory_search()`
- [ ] Deprecate old `memories` table
- [ ] Test backward compatibility

---

## Execution Order

```
Week 1:
├── Phase 1: Schema & Migration          [2 days]
├── Phase 2: Persistence Functions       [3 days]

Week 2:
├── Phase 3: Fabric Integration          [2 days]
├── Phase 4: Embedding System            [3 days]

Week 3:
├── Phase 5: User Commands               [2 days]
├── Phase 6: Agent Integration           [2 days]
├── Phase 7: Help & Documentation        [1 day]

Week 4:
├── Phase 8: Testing                     [3 days]
├── Phase 9: Migration & Cleanup         [2 days]
```

---

## Files Summary

### New Files

| File | Lines (est.) | Purpose |
|------|--------------|---------|
| `src/memory/semantic_persistence.c` | ~600 | Node/relation persistence |
| `include/nous/semantic_persistence.h` | ~100 | Function declarations |
| `src/ml/embeddings.c` | ~400 | Local embedding generation |
| `src/ml/model_manager.c` | ~200 | Model download/management |
| `src/core/commands/memory_commands.c` | ~400 | User commands |
| `tests/test_semantic_persistence.c` | ~300 | Unit tests |
| `tests/test_memory_commands.c` | ~200 | Integration tests |
| `docs/adr/ADR-007-semantic-graph-persistence.md` | ~200 | Architecture decision |

### Modified Files

| File | Changes |
|------|---------|
| `src/memory/persistence.c` | Schema update, migration runner |
| `src/core/fabric.c` | Persistence calls in create/connect |
| `src/orchestrator/orchestrator.c` | Enhanced context loading |
| `src/tools/tools.c` | Update memory_store/search |
| `src/core/commands/commands.c` | Register new commands, update help |
| `README.md` | Semantic memory documentation |
| `CHANGELOG.md` | New feature entry |
| `Makefile` | New source files |

---

## Success Criteria

- [ ] All semantic nodes persist across restarts
- [ ] Relations persist with correct weights
- [ ] Semantic search returns relevant results (precision > 0.7)
- [ ] Startup time < 2 seconds with 10k nodes
- [ ] Memory usage < 500MB with 100k nodes
- [ ] All user commands work as documented
- [ ] Agents successfully use memory for context
- [ ] Migration completes without data loss
- [ ] All tests pass
- [ ] Documentation is complete
