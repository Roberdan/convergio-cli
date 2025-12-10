# ADR-001: Persistence Layer - SQLite vs Vector DB

**Date**: 2024-12-10
**Status**: Proposta - In discussione con Roberto
**Author**: AI Team

## Context

Il sistema Convergio Kernel necessita di:
1. **Memoria conversazionale** - Storia delle conversazioni
2. **Agent state** - Definizioni e stato degli agenti
3. **Cost tracking** - Costi per sessione/agente/totale
4. **RAG/Semantic search** - Ricerca semantica nei ricordi

## Decision Points

### Opzione A: SQLite (implementazione attuale)

**Pro:**
- Zero dependencies esterne (built-in su macOS)
- Semplice da deployare
- ACID compliant
- Ottimo per dati strutturati (costs, sessions, agent state)
- WAL mode per concurrency

**Contro:**
- Non ottimale per vector search
- Richiede estensione sqlite-vss per embeddings
- Non scala per miliardi di vettori

### Opzione B: Vector DB nativo (es. FAISS, Hnswlib)

**Pro:**
- Ottimizzato per similarity search
- SIMD/NEON acceleration
- In-memory per velocità massima

**Contro:**
- Dependency esterna
- Non ideale per dati relazionali
- Richiede snapshot/persistence custom

### Opzione C: Hybrid (SQLite + in-memory vector index)

**Pro:**
- Best of both worlds
- SQLite per dati strutturati
- Custom NEON-optimized vector store per embeddings
- Già abbiamo `nous_embedding_similarity_neon()` in fabric.c

**Contro:**
- Più complessità
- Due sistemi da sincronizzare

## Recommendation

**Opzione C - Hybrid approach:**

1. **SQLite** per:
   - Conversation history
   - Agent definitions
   - Cost tracking
   - Session management
   - User preferences

2. **Semantic Fabric (già esistente)** per:
   - Embedding storage (in-memory con persistence via mmap)
   - Vector similarity search (NEON SIMD optimized)
   - Semantic relationships

Questo sfrutta:
- Il codice `fabric.c` già scritto con NEON optimization
- `nous_find_similar()` per semantic search
- `nous_embedding_similarity_neon()` per cosine similarity veloce

## MLX Integration

### Proposta aggiuntiva: MLX per embeddings locali

MLX (Apple Machine Learning eXchange) permette:
- Generazione embeddings locale su M3 Max
- GPU acceleration nativa
- Zero cloud costs per embeddings

**Modelli suggeriti:**
- `all-MiniLM-L6-v2` (22M params, 384 dim) - veloce
- `nomic-embed-text-v1.5` (137M params, 768 dim) - qualità

**Implementazione:**
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

1. [ ] Conferma con Roberto: SQLite + Semantic Fabric ok?
2. [ ] Conferma con Roberto: Integrazione MLX per embeddings?
3. [ ] Se sì MLX: scegliere modello embedding
4. [ ] Documentare scelta finale

## Questions for Roberto

1. **SQLite**: È accettabile come dependency? È built-in su macOS quindi zero install.

2. **Vector DB**: Preferisci un vector DB dedicato (FAISS, Hnswlib) o sfruttiamo il Semantic Fabric già implementato con NEON?

3. **MLX**: Vuoi embeddings locali via MLX? Pro: zero costi cloud per embeddings. Contro: ~100MB modello da scaricare.

4. **Persistence format**: Per il vector store, preferisci:
   - mmap file (veloce, memory-mapped)
   - Binary dump (semplice)
   - SQLite blob (unificato)

---

*Waiting for Roberto's decision before proceeding.*
