# ADR-004: MLX Integration for Local Embeddings

**Date**: 2024-12-10
**Status**: Proposed - Awaiting Roberto's Decision
**Author**: AI Team

## Context

Roberto asked about using MLX for maximum speed instead of cloud-based embeddings.

Current state:
- Using Claude API for all AI responses (cloud-based)
- Semantic Fabric exists with NEON-optimized similarity search
- No local embedding generation

## What is MLX?

**MLX** (Apple Machine Learning eXchange) is Apple's ML framework optimized for Apple Silicon:
- **Native M3 Max support**: Uses GPU + Neural Engine directly
- **Unified Memory**: Zero-copy between CPU/GPU
- **Python & Swift**: Easy model loading
- **Active ecosystem**: 40K+ stars on GitHub, frequent updates

## Proposed Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    USER INPUT                                    │
└─────────────────────────────────────────────────────────────────┘
                                │
                    ┌───────────┴───────────┐
                    ▼                       ▼
        ┌───────────────────┐   ┌───────────────────┐
        │  MLX EMBEDDINGS   │   │    CLAUDE API     │
        │  (Local, Free)    │   │  (Cloud, Paid)    │
        └───────────────────┘   └───────────────────┘
                    │                       │
                    ▼                       │
        ┌───────────────────┐               │
        │  SEMANTIC FABRIC  │←──────────────┘
        │  (NEON + Metal)   │
        └───────────────────┘
```

### Embedding Models (MLX-compatible)

| Model | Size | Dim | Use Case |
|-------|------|-----|----------|
| `all-MiniLM-L6-v2` | 22M | 384 | Fast, basic semantic search |
| `nomic-embed-text-v1.5` | 137M | 768 | High quality, balanced |
| `gte-large` | 335M | 1024 | Best quality, slower |

**Recommended**: `nomic-embed-text-v1.5` - best quality/speed tradeoff for M3 Max

### Implementation Options

#### Option A: Pure C with Metal Compute

```c
// src/neural/mlx_embeddings.m
#import <Metal/Metal.h>
#import <Accelerate/Accelerate.h>

// Load GGUF model directly, run inference on Metal
float* mlx_embed_text(const char* text, size_t* out_dim) {
    // 1. Tokenize (sentencepiece or BPE)
    // 2. Run transformer on Metal
    // 3. Pool embeddings
    // 4. Return float array
}
```

**Pro**: No dependencies, pure C/ObjC, fastest possible
**Con**: Complex, need to implement tokenizer + transformer

#### Option B: MLX-C Bridge (Swift/ObjC)

```swift
// Use mlx-swift package
import MLX

class EmbeddingGenerator {
    let model = try! loadModel("nomic-embed-text-v1.5")

    func embed(_ text: String) -> [Float] {
        return model.encode(text)
    }
}
```

**Pro**: Easy, well-tested, active community
**Con**: Swift dependency, larger binary

#### Option C: Python MLX via subprocess

```c
// Simple but adds latency
char* cmd = "python3 -c 'from mlx_embedding import embed; print(embed(...))'";
```

**Pro**: Simplest, uses existing Python ecosystem
**Con**: Subprocess overhead, requires Python

## Recommendation

**Option B (MLX-Swift Bridge)** with fallback to **Option C** for initial version:

1. **Phase 1**: Use Python MLX subprocess for prototype
2. **Phase 2**: Migrate to Swift MLX for performance
3. **Phase 3**: Pure Metal implementation if needed

## Benefits

1. **Zero API cost** for embeddings
2. **Faster** than network round-trip (~10ms local vs 100ms+ cloud)
3. **Privacy**: All data stays on device
4. **Offline capable**: Works without internet

## Tradeoffs

1. **First-run download**: ~200MB model download
2. **Memory**: Model uses ~400MB RAM when loaded
3. **Complexity**: Additional build dependencies

## Decision: Pure Metal/C

**Roberto's choice**: Option A - Pure Metal/C for maximum performance.

## Implementation

**Files created:**
- `src/neural/mlx_embed.h` - Header with model config (MiniLM-L6-v2 compatible)
- `src/neural/mlx_embed.m` - Full implementation (~700 lines)

**Architecture:**
```
Input Text
    │
    ▼
┌─────────────────┐
│   Tokenizer     │  Simple BPE-like tokenization
│   (CPU)         │  [CLS] + tokens + [SEP]
└─────────────────┘
    │
    ▼
┌─────────────────┐
│  Token + Pos    │  Embedding lookup
│  Embeddings     │  (NEON SIMD)
└─────────────────┘
    │
    ▼
┌─────────────────┐
│  6x Transformer │  Self-attention + FFN
│  Layers         │  (Accelerate BLAS + NEON)
└─────────────────┘
    │
    ▼
┌─────────────────┐
│  Mean Pooling   │  Sequence → Single vector
│  + L2 Norm      │  (NEON SIMD)
└─────────────────┘
    │
    ▼
384-dim embedding
```

**Optimizations:**
- NEON SIMD for layer norm, softmax, pooling
- Accelerate BLAS (cblas_sgemm) for matrix multiply
- Metal compute shaders available (currently CPU fallback)
- Xavier weight initialization

**Model Config:**
- Vocab size: 30,522 (BERT-compatible)
- Hidden dim: 384
- Layers: 6
- Attention heads: 12
- Max sequence: 512

## Current Alternative

Il Semantic Fabric usa già NEON SIMD per similarity search:

```c
// fabric.c - già implementato e funzionante
float nous_embedding_similarity_neon(const NousEmbedding* a, const NousEmbedding* b) {
    float16x8_t sum = vdupq_n_f16(0);
    for (size_t i = 0; i < NOUS_EMBEDDING_DIM; i += 8) {
        float16x8_t va = vld1q_f16(&a->values[i]);
        float16x8_t vb = vld1q_f16(&b->values[i]);
        sum = vfmaq_f16(sum, va, vb);
    }
    // ... normalize and return
}
```

Questo funziona per **search** ma non genera embeddings - quelli vengono da Claude API.

---

*Awaiting Roberto's decision on MLX integration priority and approach.*
