# ADR-001: Persistence Layer - SQLite vs Vector DB

**Date**: 2025-12-11
**Status**: Approved
**Author**: AI Team

## Context

The Convergio CLI system requires:
1. **Conversation memory** - Conversation history
2. **Agent state** - Agent definitions and state
3. **Cost tracking** - Costs per session/agent/total
4. **RAG/Semantic search** - Semantic search in memories

## Decision Points

### Option A: SQLite (current implementation)

**Pros:**
- Zero external dependencies (built-in on macOS)
- Simple to deploy
- ACID compliant
- Excellent for structured data (costs, sessions, agent state)
- WAL mode for concurrency

**Cons:**
- Not optimal for vector search
- Requires sqlite-vss extension for embeddings
- Does not scale for billions of vectors

### Option B: Native Vector DB (e.g., FAISS, Hnswlib)

**Pros:**
- Optimized for similarity search
- SIMD/NEON acceleration
- In-memory for maximum speed

**Cons:**
- External dependency
- Not ideal for relational data
- Requires custom snapshot/persistence

### Option C: Hybrid (SQLite + in-memory vector index)

**Pros:**
- Best of both worlds
- SQLite for structured data
- Custom NEON-optimized vector store for embeddings
- We already have `nous_embedding_similarity_neon()` in fabric.c

**Cons:**
- More complexity
- Two systems to synchronize

## Recommendation

**Option C - Hybrid approach:**

1. **SQLite** for:
   - Conversation history
   - Agent definitions
   - Cost tracking
   - Session management
   - User preferences

2. **Semantic Fabric (already existing)** for:
   - Embedding storage (in-memory with persistence via mmap)
   - Vector similarity search (NEON SIMD optimized)
   - Semantic relationships

This leverages:
- The `fabric.c` code already written with NEON optimization
- `nous_find_similar()` for semantic search
- `nous_embedding_similarity_neon()` for fast cosine similarity

## MLX Integration

### Additional proposal: MLX for local embeddings

MLX (Apple Machine Learning eXchange) enables:
- Local embedding generation on M3 Max
- Native GPU acceleration
- Zero cloud costs for embeddings

**Suggested models:**
- `all-MiniLM-L6-v2` (22M params, 384 dim) - fast
- `nomic-embed-text-v1.5` (137M params, 768 dim) - quality

**Implementation:**
```c
// src/neural/mlx_embeddings.m
#import <MLX/MLX.h>

float* mlx_generate_embedding(const char* text, size_t* out_dim) {
    // Load model once, cache
    // Run inference on M3 Max GPU
    // Return float array (768 dim)
}
```

## Action Items

1. [x] SQLite + Semantic Fabric confirmed
2. [x] MLX integration for embeddings confirmed
3. [x] MiniLM-L6-v2 model chosen
4. [x] Final choice documented

## Decision

**SQLite**: Built-in on macOS, zero installation required.

**Vector DB**: Using the Semantic Fabric already implemented with NEON.

**MLX**: Pure Metal/C implementation for local embeddings - zero cloud costs, ~100MB model.

**Persistence format**: mmap file (fast, memory-mapped) with SQLite for structured data.

---

*Decision made and implemented.*
