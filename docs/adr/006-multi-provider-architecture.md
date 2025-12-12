# ADR 006: Multi-Provider Architecture

## Status
Accepted

## Date
2025-12-12

## Context
Convergio was initially designed as a Claude-only CLI tool. To provide users with flexibility, cost optimization, and resilience, we needed to support multiple LLM providers (Anthropic Claude, OpenAI GPT, Google Gemini) with intelligent routing and fallback capabilities.

## Decision
We implemented a multi-provider architecture with the following components:

### Provider Abstraction Layer
- `include/nous/provider.h` - Unified provider interface
- `src/providers/provider.c` - Provider registry and lifecycle management
- Individual provider adapters: `anthropic.c`, `openai.c`, `gemini.c`

### Model Router
- `src/router/model_router.c` - Intelligent model selection based on:
  - Agent requirements
  - Budget constraints
  - Provider availability
  - Cost optimization

### Cost Optimizer
- `src/router/cost_optimizer.c` - Budget tracking and cost-aware routing
- Session and daily budget limits
- Automatic downgrade to cheaper models when budget is low

### Test Infrastructure
- Mock provider framework (`tests/mock_provider.c/h`)
- Provider-specific mocks for testing error scenarios
- Unit tests for model router
- Integration tests for multi-provider scenarios

## Consequences

### Positive
- Users can leverage multiple AI providers
- Automatic fallback when primary provider is unavailable
- Cost optimization through intelligent model selection
- Better resilience through provider redundancy
- Easier testing through mock infrastructure

### Negative
- Increased complexity in provider management
- Need to maintain multiple API integrations
- Configuration complexity for end users

### Neutral
- Each provider requires its own API key
- Different providers have different capabilities and limitations

## Implementation Notes

### API Key Configuration (.env.example)
```bash
ANTHROPIC_API_KEY=sk-ant-api03-...
OPENAI_API_KEY=sk-...
GOOGLE_API_KEY=...
```

### Model Configuration (config/models.json)
Models are configured with provider prefix: `anthropic/claude-sonnet-4.5`, `openai/gpt-4o`, `gemini/gemini-2-flash`

### Budget Management
- Daily budget: Maximum spend per 24-hour period
- Session budget: Maximum spend per CLI session
- Automatic reset at midnight for daily budget

## References
- docs/PROVIDERS.md - Provider setup guide
- docs/MODEL_SELECTION.md - Model selection guide
- docs/COST_OPTIMIZATION.md - Cost optimization guide
