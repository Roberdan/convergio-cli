# ADR-007: Semantic Graph Persistence

**Status:** Implemented
**Date:** 2025-12-13
**Author:** Roberto + AI Team
**Deciders:** Roberto

## Context

Convergio has two separate memory systems that are not integrated:

1. **Simple Memory Store** (`memories` table in SQLite)
   - Flat key-value storage with importance scores
   - Used by `memory_store` and `memory_search` tools
   - Persisted to disk ✅
   - No relationships between memories

2. **Semantic Graph** (Nous Kernel in `fabric.c`)
   - Full knowledge graph with `SemanticID`, nodes, weighted relations
   - Sophisticated in-memory structure with sharding and lock-free reads
   - **NOT persisted to disk** ❌
   - All data lost on restart

The semantic graph is architecturally superior but currently useless because nothing survives a restart.

## Decision

We will **persist the semantic graph to SQLite** and integrate it with the existing memory system, creating a unified knowledge layer that:

1. Survives restarts
2. Enables relationship-based queries
3. Supports semantic similarity search
4. Grows incrementally across sessions

### Key Design Decisions

#### 1. Storage Backend: SQLite (same DB)

**Rationale:**
- Already have SQLite infrastructure
- Single database file for all persistence
- ACID transactions for graph consistency
- WAL mode already enabled for concurrency

**Alternative considered:** Separate graph database (Neo4j, etc.)
- Rejected: Too much complexity for current scale

#### 2. Lazy Loading with Write-Through Cache

**Rationale:**
- Load frequently-accessed nodes on startup
- Load remaining nodes on-demand
- Write-through: every `nous_create_node()` persists immediately
- Periodic background sync for relation strength updates

**Alternative considered:** Full graph in memory
- Rejected: Won't scale, high startup time

#### 3. Local Embeddings with e5-small-v2

**Rationale:**
- 33M parameters, ~130MB model
- Runs on Apple Silicon MLX
- No API costs
- Latency: ~10ms per embedding
- Quality: Good enough for semantic search

**Alternative considered:** API-based embeddings (OpenAI, Voyage)
- Rejected: Adds latency, cost, and external dependency

#### 4. Unified Memory Interface

**Rationale:**
- `memory_store` becomes a wrapper around semantic node creation
- `memory_search` uses graph traversal + vector similarity
- Backward compatible with existing tools

## Technical Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         APPLICATION LAYER                           │
├─────────────────────────────────────────────────────────────────────┤
│  Commands: /remember, /search, /memories, /forget, /graph           │
│  Tools: memory_store, memory_search, graph_query                    │
│  Agents: Auto-memorize, context retrieval                           │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         SEMANTIC LAYER                              │
├─────────────────────────────────────────────────────────────────────┤
│  nous_create_node()  →  In-Memory + SQLite                          │
│  nous_connect()      →  In-Memory + SQLite                          │
│  nous_query()        →  Graph traversal + Vector similarity         │
│  nous_find_related() →  BFS/DFS on relation graph                   │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         PERSISTENCE LAYER                           │
├─────────────────────────────────────────────────────────────────────┤
│  semantic_nodes table    │  semantic_relations table                │
│  - id (SemanticID)       │  - from_id, to_id                        │
│  - type, essence         │  - strength, relation_type               │
│  - embedding (BLOB)      │  - created_at                            │
│  - metadata              │                                          │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         EMBEDDING LAYER                             │
├─────────────────────────────────────────────────────────────────────┤
│  MLX e5-small-v2  →  384-dim embeddings                             │
│  Cosine similarity for semantic search                              │
│  Cached embeddings in SQLite BLOB                                   │
└─────────────────────────────────────────────────────────────────────┘
```

## Schema

```sql
-- Semantic nodes (replaces/extends memories table)
CREATE TABLE semantic_nodes (
    id INTEGER PRIMARY KEY,              -- SemanticID (64-bit)
    type INTEGER NOT NULL,               -- SemanticType enum
    essence TEXT NOT NULL,               -- Human-readable content
    embedding BLOB,                      -- Float32 array (384 dims)
    creator_id INTEGER,                  -- Who created this node
    context_id INTEGER,                  -- Space/project context
    importance REAL DEFAULT 0.5,         -- [0,1] for retrieval ranking
    access_count INTEGER DEFAULT 0,
    created_at INTEGER NOT NULL,         -- Unix timestamp (ns)
    last_accessed INTEGER,
    metadata_json TEXT                   -- Extensible metadata
);

-- Relations between nodes (weighted edges)
CREATE TABLE semantic_relations (
    from_id INTEGER NOT NULL,
    to_id INTEGER NOT NULL,
    strength REAL DEFAULT 0.5,           -- [0,1] edge weight
    relation_type TEXT DEFAULT 'related', -- Semantic relation type
    created_at INTEGER NOT NULL,
    updated_at INTEGER,
    PRIMARY KEY (from_id, to_id),
    FOREIGN KEY (from_id) REFERENCES semantic_nodes(id) ON DELETE CASCADE,
    FOREIGN KEY (to_id) REFERENCES semantic_nodes(id) ON DELETE CASCADE
);

-- Indexes for performance
CREATE INDEX idx_nodes_type ON semantic_nodes(type);
CREATE INDEX idx_nodes_importance ON semantic_nodes(importance DESC);
CREATE INDEX idx_nodes_created ON semantic_nodes(created_at DESC);
CREATE INDEX idx_relations_from ON semantic_relations(from_id);
CREATE INDEX idx_relations_to ON semantic_relations(to_id);
```

## Consequences

### Positive
- Knowledge persists across sessions
- Relationship-based reasoning becomes possible
- Semantic search improves with graph context
- Foundation for advanced agent memory

### Negative
- Increased SQLite I/O
- Startup time increases (graph loading)
- Embedding model adds ~130MB to installation

### Risks
- Graph can grow unbounded → Need pruning strategy
- Embedding drift over time → Need re-embedding capability
- Complex queries may be slow → Need query optimization

## Migration Strategy

1. **Phase 1:** Add new tables, don't modify existing behavior
2. **Phase 2:** Migrate existing `memories` to `semantic_nodes`
3. **Phase 3:** Update `memory_store`/`memory_search` to use new system
4. **Phase 4:** Deprecate old `memories` table

## Success Metrics

- [x] All semantic nodes survive restart
- [x] Relations persist and load correctly
- [x] Semantic search returns relevant results (>0.7 precision)
- [x] Startup time < 2 seconds with 10k nodes
- [ ] Memory usage < 500MB with 100k nodes (not yet tested at scale)
