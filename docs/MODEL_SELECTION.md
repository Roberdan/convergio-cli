# Model Selection Guide

This guide explains how Convergio selects and routes models for different agents and tasks.

## Overview

Convergio uses an intelligent model routing system that:
- Assigns optimal models to each agent based on their role
- Respects budget constraints
- Handles provider failures with automatic fallback
- Optimizes cost while maintaining quality

---

## Agent-Model Mapping

Each agent in Convergio is pre-configured with a default model optimized for their role:

| Agent | Default Model | Fallback Model | Reasoning |
|-------|--------------|----------------|-----------|
| **Ali** (Orchestrator) | Claude Opus 4.5 | Claude Sonnet 4.5 | Complex reasoning and coordination |
| **Marco** (Coder) | Claude Sonnet 4.5 | GPT-5.2-thinking | Best coding performance |
| **Sara** (Writer) | Claude Sonnet 4.5 | Gemini 3 Pro | High-quality content generation |
| **Leo** (Analyst) | GPT-5.2-pro | Claude Sonnet 4.5 | Deep research and analysis |
| **Nina** (Critic) | Claude Haiku 4.5 | GPT-5-nano | Fast validation and review |
| **Router** | GPT-5-nano | Gemini 3 Flash | Ultra-fast task routing |

---

## How Model Selection Works

### 1. Agent Configuration (Highest Priority)

If an agent has a model explicitly configured in its definition:

```json
{
  "name": "marco",
  "model": {
    "provider": "anthropic",
    "model_id": "claude-sonnet-4.5",
    "fallback": {
      "provider": "openai",
      "model_id": "gpt-5.2-thinking"
    }
  }
}
```

### 2. Budget Constraints

If the remaining budget is low, the system automatically downgrades:

| Budget Level | Model Tier Selection |
|--------------|---------------------|
| > $1.00 | Premium models (Opus, GPT-5.2-pro) |
| $0.50 - $1.00 | Standard models (Sonnet, GPT-4o) |
| $0.10 - $0.50 | Economy models (Haiku, Nano) |
| < $0.10 | Cheapest available (Flash, Nano) |

### 3. Provider Availability

If the primary provider fails:
1. Check fallback model
2. Try equivalent model on another provider
3. Fall back to cheapest available model

### 4. Task Complexity

For simple tasks, the router may downgrade automatically:

```
Task: "What time is it in Tokyo?"
→ Simple query → Use GPT-5-nano instead of Opus
```

---

## Model Tiers

### Tier 1: Premium (Complex Tasks)

Best for: Complex reasoning, long-form content, autonomous agents

| Model | Provider | Cost (In/Out per MTok) |
|-------|----------|------------------------|
| Claude Opus 4.5 | Anthropic | $15 / $75 |
| GPT-5.2-pro | OpenAI | $5 / $20 |
| Gemini 3 Ultra | Google | $7 / $21 |

### Tier 2: Standard (General Use)

Best for: Coding, writing, general conversation

| Model | Provider | Cost (In/Out per MTok) |
|-------|----------|------------------------|
| Claude Sonnet 4.5 | Anthropic | $3 / $15 |
| GPT-5 | OpenAI | $1.25 / $10 |
| Gemini 3 Pro | Google | $2 / $12 |

### Tier 3: Economy (Simple Tasks)

Best for: Classification, routing, simple Q&A

| Model | Provider | Cost (In/Out per MTok) |
|-------|----------|------------------------|
| Claude Haiku 4.5 | Anthropic | $1 / $5 |
| GPT-5-nano | OpenAI | $0.05 / $0.40 |
| Gemini 3 Flash | Google | $0.075 / $0.30 |

---

## Customizing Model Selection

### Per-Session Override

```bash
# Use specific model for entire session
convergio --model claude-opus-4.5

# Use specific provider
convergio --provider openai
```

### Per-Agent Configuration

Create `~/.convergio/agents/marco.json`:

```json
{
  "name": "marco",
  "role": "coder",
  "model": {
    "provider": "openai",
    "model_id": "gpt-5.2-thinking"
  }
}
```

### Environment Variables

```bash
# Set default model
export CONVERGIO_DEFAULT_MODEL="claude-sonnet-4.5"

# Set default provider
export CONVERGIO_DEFAULT_PROVIDER="anthropic"
```

---

## Fallback Chain

When a model request fails, Convergio follows this fallback chain:

```
Primary Model (configured)
    ↓ (if fails)
Fallback Model (configured)
    ↓ (if fails)
Equivalent Model (different provider)
    ↓ (if fails)
Economy Model (cheapest available)
    ↓ (if fails)
Error (all providers unavailable)
```

### Example Fallback Scenarios

**Scenario 1: API Rate Limited**
```
claude-sonnet-4.5 → rate limited
    ↓
gpt-5 → success
```

**Scenario 2: Budget Exhausted**
```
claude-opus-4.5 → budget exceeded
    ↓
claude-haiku-4.5 → success (cheaper)
```

**Scenario 3: Provider Down**
```
All Anthropic models → provider error
    ↓
OpenAI models → success
```

---

## Model Capabilities Matrix

| Capability | Claude Opus | Claude Sonnet | GPT-5.2-pro | Gemini 3 Pro |
|------------|-------------|---------------|-------------|--------------|
| Complex Reasoning | ★★★★★ | ★★★★ | ★★★★★ | ★★★★ |
| Code Generation | ★★★★★ | ★★★★★ | ★★★★ | ★★★★ |
| Creative Writing | ★★★★★ | ★★★★ | ★★★★ | ★★★★ |
| Tool Calling | ★★★★★ | ★★★★★ | ★★★★★ | ★★★★★ |
| Long Context | ★★★★ (200K) | ★★★★★ (1M) | ★★★★ (400K) | ★★★★★ (2M) |
| Speed | ★★★ | ★★★★ | ★★★★ | ★★★★★ |
| Cost Efficiency | ★★ | ★★★★ | ★★★ | ★★★★ |

---

## Best Practices

### 1. Let Convergio Choose

In most cases, the default model selection is optimal. Trust the system.

### 2. Set a Budget

```bash
convergio --budget 5.00  # $5 session limit
```

This enables smart downgrading and prevents unexpected costs.

### 3. Use Task Hints

Include complexity hints in your prompts:

```
"[SIMPLE] What's 2+2?"     → Routes to cheap model
"[COMPLEX] Design a distributed system" → Routes to premium model
```

### 4. Monitor Usage

```bash
convergio cost status
```

Review which models are consuming budget.

### 5. Configure for Your Workflow

If you primarily do coding:
```json
{
  "default_model": "claude-sonnet-4.5",
  "fallback_model": "gpt-5.2-thinking"
}
```

---

## Troubleshooting

### "Model not available"

The requested model may require a specific API tier:

```bash
# Check your OpenAI tier
convergio providers test openai

# Use an alternative
convergio --model gpt-4o
```

### "Budget exceeded"

The system automatically downgrades, but you can:

```bash
# Increase budget
convergio --budget 20.00

# Or use cheaper models explicitly
convergio --model claude-haiku-4.5
```

### "All providers failed"

Check your API keys:

```bash
convergio providers test
```

---

## Reference

### Supported Models

See [PROVIDERS.md](PROVIDERS.md) for full model list and pricing.

### Configuration Schema

See [config/models.json](../config/models.json) for model definitions.
