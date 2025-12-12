# Cost Optimization Guide

This guide explains how Convergio minimizes API costs while maintaining quality.

## Overview

AI API costs can add up quickly. Convergio implements multiple strategies to optimize costs:

1. **Smart Model Routing** - Use the right model for each task
2. **Prompt Caching** - Avoid redundant API calls
3. **Batch Processing** - Queue non-urgent requests
4. **Budget Management** - Automatic downgrading when budget is low
5. **Token Optimization** - Efficient prompt engineering

---

## Cost Strategies

### 1. Model Tiering

Different tasks need different model capabilities:

| Task Type | Recommended Model | Cost/1K tokens |
|-----------|------------------|----------------|
| Complex reasoning | Claude Opus 4.5 | $0.015 / $0.075 |
| General coding | Claude Sonnet 4.5 | $0.003 / $0.015 |
| Simple queries | GPT-5-nano | $0.00005 / $0.0004 |
| Long documents | Gemini 3 Flash | $0.000075 / $0.0003 |

**Savings Example:**
```
Using GPT-5-nano instead of Opus for routing:
- Opus: $0.09 per routing decision
- Nano: $0.00045 per routing decision
- Savings: 99.5%
```

### 2. Prompt Caching

Convergio caches prompt responses to avoid redundant API calls:

```
Request 1: "What is Python?"
→ API call → Response cached

Request 2: "What is Python?" (same user, same session)
→ Cache hit → No API call → $0.00
```

Cache invalidation:
- Session change
- 15-minute TTL
- Context change

### 3. Batch Processing

Non-urgent requests can be batched for lower costs:

```bash
# Queue batch request
convergio batch add "Generate 100 test cases"

# Process batch (often 50% cheaper)
convergio batch run
```

### 4. Budget Guards

Set spending limits to prevent runaway costs:

```bash
# Session budget
convergio --budget 5.00

# Daily budget (in config)
{
  "cost": {
    "daily_limit": 20.00,
    "monthly_limit": 500.00
  }
}
```

When budget is low, Convergio automatically:
1. Switches to cheaper models
2. Reduces context window
3. Batches non-critical requests

---

## Cost Monitoring

### Real-Time Status

The status bar shows current session cost:

```
◆ user ▶ project ▶ Sonnet 4.5 ▶ [default]    12.5K tokens
▶▶ 3 background tasks · $0.0847
```

### Cost Report

```bash
convergio cost report

Session Cost Summary
====================
Provider      | Calls | Tokens    | Cost
-------------|-------|-----------|--------
Anthropic    | 45    | 125,432   | $2.15
OpenAI       | 12    | 34,567    | $0.43
Gemini       | 8     | 89,012    | $0.12
-------------|-------|-----------|--------
Total        | 65    | 249,011   | $2.70

Daily trend: ↓15% vs yesterday
```

### Cost by Agent

```bash
convergio cost agents

Agent Cost Breakdown
====================
Agent    | Model           | Calls | Cost
---------|-----------------|-------|-------
Ali      | claude-opus-4.5 | 5     | $1.23
Marco    | claude-sonnet   | 28    | $0.89
Nina     | claude-haiku    | 32    | $0.45
Router   | gpt-5-nano      | 156   | $0.08
```

---

## Configuration

### Budget Settings

Create/edit `~/.convergio/config.json`:

```json
{
  "cost": {
    "session_budget": 5.00,
    "daily_budget": 20.00,
    "monthly_budget": 500.00,
    "warn_at_percent": 80,
    "pause_at_percent": 100
  }
}
```

### Model Preferences

Optimize for cost:

```json
{
  "routing": {
    "prefer_cheap": true,
    "max_cost_per_request": 0.10,
    "fallback_to_cheap_on_error": true
  }
}
```

### Caching Settings

```json
{
  "cache": {
    "enabled": true,
    "ttl_minutes": 15,
    "max_entries": 1000,
    "hash_prompts": true
  }
}
```

---

## Best Practices

### 1. Use the Right Model

Don't use Opus for simple tasks:

```bash
# ❌ Expensive for simple query
convergio --model claude-opus-4.5 "What's the capital of France?"

# ✓ Appropriate model
convergio --model gpt-5-nano "What's the capital of France?"
```

### 2. Batch Similar Requests

```bash
# ❌ Individual requests (more overhead)
for file in *.py; do
  convergio "review $file"
done

# ✓ Batch request (cheaper)
convergio "review all Python files in this directory"
```

### 3. Use System Prompts Efficiently

System prompts are included in every request. Keep them concise:

```bash
# ❌ Verbose system prompt (500 tokens each request)
"You are an expert software engineer with 20 years of experience..."

# ✓ Concise system prompt (50 tokens)
"Expert Python developer. Be concise."
```

### 4. Leverage Context

Use conversation history instead of repeating context:

```bash
# ❌ Repeating context
"Given the Python file I showed you earlier which contains..."

# ✓ Reference existing context
"In that file, fix the bug on line 42"
```

### 5. Set Budget Alerts

```bash
# Get notified at 80% budget
convergio config set cost.warn_at_percent 80
```

---

## Cost Comparison

### Scenario: Code Review (10K tokens input, 2K output)

| Model | Cost | Speed | Quality |
|-------|------|-------|---------|
| Claude Opus 4.5 | $0.30 | Slow | Excellent |
| Claude Sonnet 4.5 | $0.06 | Fast | Excellent |
| GPT-5.2-pro | $0.09 | Medium | Excellent |
| GPT-4o | $0.08 | Fast | Very Good |
| Gemini 3 Flash | $0.0015 | Very Fast | Good |

**Recommendation:** Use Sonnet for code review (best quality/cost ratio)

### Scenario: Quick Question (100 tokens in, 50 out)

| Model | Cost | Speed |
|-------|------|-------|
| Claude Opus 4.5 | $0.005 | Slow |
| GPT-5-nano | $0.00003 | Very Fast |
| Gemini 3 Flash | $0.00002 | Very Fast |

**Recommendation:** Use Nano/Flash for quick questions (166x cheaper)

---

## Tracking Spending

### Daily Summary Email

```json
{
  "notifications": {
    "daily_summary": true,
    "email": "you@example.com"
  }
}
```

### Export Cost Data

```bash
# Export to CSV
convergio cost export --format csv --output costs.csv

# Export to JSON
convergio cost export --format json --output costs.json
```

### Dashboard Integration

Convergio can export metrics to monitoring systems:

```bash
# Prometheus metrics
convergio metrics --format prometheus

# StatsD
convergio metrics --format statsd --host localhost:8125
```

---

## Troubleshooting

### "Budget exceeded"

```bash
# Check current usage
convergio cost status

# Increase budget
convergio config set cost.session_budget 10.00

# Or use cheaper model
convergio --model claude-haiku-4.5
```

### "Unexpected high costs"

```bash
# Review recent requests
convergio cost history --last 24h

# Check for expensive models
convergio cost agents

# Enable cost alerts
convergio config set cost.warn_at_percent 50
```

### "Cache not working"

```bash
# Check cache status
convergio cache status

# Clear cache
convergio cache clear

# Verify caching is enabled
convergio config get cache.enabled
```

---

## Reference

### Cost Constants

```
Claude Opus 4.5:    $15/$75 per MTok
Claude Sonnet 4.5:  $3/$15 per MTok
Claude Haiku 4.5:   $1/$5 per MTok
GPT-5.2-pro:        $5/$20 per MTok
GPT-5-nano:         $0.05/$0.40 per MTok
Gemini 3 Pro:       $2/$12 per MTok
Gemini 3 Flash:     $0.075/$0.30 per MTok
```

### API Documentation

- [Anthropic Pricing](https://anthropic.com/pricing)
- [OpenAI Pricing](https://openai.com/pricing)
- [Google AI Pricing](https://ai.google.dev/pricing)
