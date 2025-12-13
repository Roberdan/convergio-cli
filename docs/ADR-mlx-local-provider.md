# ADR: MLX Local Provider for Convergio

## Status
**Accepted** - 2025-12-13

## Context

Convergio CLI needs the ability to operate 100% offline without external API dependencies. This is critical for:
- Privacy-sensitive environments where data cannot leave the device
- Air-gapped systems without internet access
- Cost reduction for high-volume usage
- Low-latency requirements (no network roundtrip)

Apple Silicon Macs (M1/M2/M3/M4/M5) have powerful Neural Engines and unified memory that enable efficient local LLM inference through Apple's MLX framework.

## Decision

We will implement a new **MLX Local Provider** that runs AI models natively on Apple Silicon using the MLX framework.

### Key Design Decisions

#### 1. Provider Architecture
- MLX provider follows the same `Provider` interface as cloud providers
- Factory function `mlx_provider_create()` returns a Provider instance
- All operations are local - no network calls required
- Graceful degradation: if not on Apple Silicon, MLX provider is unavailable

#### 2. Model Selection
We will support 4-bit quantized models from HuggingFace mlx-community:

| Model | Size | RAM | Use Case |
|-------|------|-----|----------|
| Llama 3.2 1B | 800MB | 4GB | Fast, lightweight |
| Llama 3.2 3B | 1.8GB | 8GB | Balanced |
| DeepSeek R1 Distill 1.5B | 1GB | 4GB | Reasoning, coding |
| DeepSeek R1 Distill 7B | 4.5GB | 8GB | Better reasoning |
| DeepSeek R1 Distill 14B | 8.5GB | 16GB | Best reasoning |
| Qwen 2.5 Coder 7B | 4.2GB | 8GB | Code generation |
| Phi-3 Mini | 2.2GB | 6GB | Fast, efficient |
| Mistral 7B Q4 | 4.1GB | 8GB | Multilingual |
| Llama 3.1 8B Q4 | 4.6GB | 8GB | Best quality |

**Rationale**: 4-bit quantization provides optimal balance between quality and RAM usage on consumer hardware.

#### 3. Model Storage
Models are stored in `~/.convergio/models/mlx/<model-id>/`:
- Downloaded from HuggingFace using `huggingface_hub` Python package
- Each model is a directory containing safetensors, tokenizer, and config
- Checksum verification for integrity

#### 4. Dynamic Context Limits
Context compaction thresholds are dynamically adjusted based on model capabilities:
- Cloud models (200K context): 80K threshold
- Local models (32K-131K context): 60% of context window
- Minimum threshold: 2K tokens
- Maximum threshold: 200K tokens

#### 5. Hardware Detection
Use existing `convergio_detect_hardware()` to:
- Verify Apple Silicon (M1+)
- Check available unified memory
- Warn if RAM insufficient for selected model

#### 6. Setup Integration
The `/setup` wizard includes a "Local Models" menu:
- List available models with status (downloaded/not downloaded)
- Download models with progress indication
- Delete installed models to free space
- Show RAM requirements and recommendations

### Alternatives Considered

#### Alternative 1: Ollama Integration
**Rejected** because:
- Requires separate Ollama daemon running
- Additional dependency to install and maintain
- Less tight integration with Convergio

#### Alternative 2: llama.cpp Direct Integration
**Rejected** because:
- Would require significant additional C/C++ code
- MLX is Apple's native solution, better optimized
- MLX models are more readily available

#### Alternative 3: PyTorch/ONNX Runtime
**Rejected** because:
- Not optimized for Apple Silicon
- Larger runtime dependencies
- Slower inference compared to MLX

## Consequences

### Positive
- **100% offline operation** - No internet or API keys required
- **Privacy** - Data never leaves the device
- **Cost savings** - No API charges for local inference
- **Low latency** - No network roundtrip
- **Apple Silicon optimized** - Leverages Neural Engine and unified memory

### Negative
- **Apple Silicon only** - Doesn't work on Intel Macs or other platforms
- **Quality tradeoff** - Local models less capable than Claude for complex tasks
- **Model storage** - Requires 1-9GB per model on disk
- **RAM requirements** - Larger models need 16GB+ unified memory
- **Tool calling** - Less reliable than cloud models

### Neutral
- **Additional code** - ~2900 lines added across 8 work packages
- **HuggingFace dependency** - Python package needed for model downloads
- **Maintenance** - Model registry may need updates as new models release

## Implementation

The implementation is divided into 8 work packages:
1. **WP1**: Core MLX Provider (mlx.m, mlx.h)
2. **WP2**: Model Manager (download/delete)
3. **WP3**: Setup Wizard Enhancement
4. **WP4**: Dynamic Context Limits
5. **WP5**: Provider Registry Update
6. **WP6**: Model Registry (models.json)
7. **WP7**: Help & Documentation
8. **WP8**: CLI Arguments

See `docs/plans/PLAN-mlx-local-provider.md` for detailed implementation plan.

## References
- [Apple MLX Framework](https://github.com/ml-explore/mlx)
- [MLX Community Models](https://huggingface.co/mlx-community)
- [DeepSeek R1 Distill](https://huggingface.co/deepseek-ai)
- [Convergio Provider Architecture](../src/providers/provider.c)
