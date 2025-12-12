# Provider Setup Guide

This guide explains how to configure each LLM provider for Convergio.

## Quick Start

Run `convergio setup` to interactively configure your providers:

```bash
convergio setup
```

Or set environment variables manually:

```bash
export ANTHROPIC_API_KEY="sk-ant-api03-..."
export OPENAI_API_KEY="sk-proj-..."
export GEMINI_API_KEY="AIza..."
```

---

## Anthropic (Claude)

### Available Models

| Model | Input Cost | Output Cost | Context | Best For |
|-------|------------|-------------|---------|----------|
| `claude-opus-4.5` | $15/MTok | $75/MTok | 200K | Complex reasoning, autonomous tasks |
| `claude-sonnet-4.5` | $3/MTok | $15/MTok | 1M | Coding, agents, best overall |
| `claude-sonnet-4` | $3/MTok | $15/MTok | 200K | General purpose, balanced |
| `claude-haiku-4.5` | $1/MTok | $5/MTok | 200K | Fast, cheap, classification |

### Setup

1. **Create Account**: Visit [console.anthropic.com](https://console.anthropic.com)

2. **Get API Key**:
   - Navigate to "API Keys" in the sidebar
   - Click "Create Key"
   - Copy the key (starts with `sk-ant-api03-`)

3. **Configure**:
   ```bash
   # Option 1: Environment variable
   export ANTHROPIC_API_KEY="sk-ant-api03-..."

   # Option 2: Interactive setup
   convergio setup
   ```

4. **Verify**:
   ```bash
   convergio providers test
   # Output: Anthropic ✓ Connected
   ```

### Authentication Methods

Convergio supports two authentication methods for Anthropic:

1. **API Key** (Recommended for development)
   - Set `ANTHROPIC_API_KEY` environment variable
   - Most common method for CLI usage

2. **Claude Max OAuth** (For Claude Max subscribers)
   - Run `convergio login`
   - Opens browser for OAuth authentication
   - Tokens stored securely in macOS Keychain

---

## OpenAI (GPT)

### Available Models

| Model | Input Cost | Output Cost | Context | Best For |
|-------|------------|-------------|---------|----------|
| `gpt-5.2-pro` | $5/MTok | $20/MTok | 400K | Most accurate, research |
| `gpt-5.2-thinking` | $2.50/MTok | $15/MTok | 400K | Coding, planning |
| `gpt-5.2-instant` | $1.25/MTok | $10/MTok | 400K | Fast writing |
| `gpt-5` | $1.25/MTok | $10/MTok | 256K | General flagship |
| `gpt-4o` | $5/MTok | $15/MTok | 128K | Multimodal, vision |
| `o3` | $10/MTok | $40/MTok | 128K | Deep reasoning |
| `o4-mini` | $0.15/MTok | $0.60/MTok | 128K | Efficient reasoning |
| `gpt-5-nano` | $0.05/MTok | $0.40/MTok | 128K | High-volume, simple |

### Setup

1. **Create Account**: Visit [platform.openai.com](https://platform.openai.com)

2. **Get API Key**:
   - Go to [platform.openai.com/api-keys](https://platform.openai.com/api-keys)
   - Click "Create new secret key"
   - Name it "convergio-cli"
   - Copy immediately (shown only once!)

3. **Configure**:
   ```bash
   export OPENAI_API_KEY="sk-proj-..."
   ```

4. **Verify**:
   ```bash
   convergio providers test
   # Output: OpenAI ✓ Connected
   ```

### Tier Requirements

Some models require higher spending tiers:

| Tier | Requirement | Models Available |
|------|-------------|------------------|
| Free | New account | gpt-4o-mini |
| Tier 1 | $5 spent | gpt-4o |
| Tier 2 | $50 spent | All models |
| Tier 3 | $100 spent | Higher rate limits |

---

## Google Gemini

### Available Models

| Model | Input Cost | Output Cost | Context | Best For |
|-------|------------|-------------|---------|----------|
| `gemini-3-pro` | $2/MTok | $12/MTok | 200K | General purpose, reasoning |
| `gemini-3-pro` (>200K) | $4/MTok | $18/MTok | 2M | Long context tasks |
| `gemini-3-ultra` | $7/MTok | $21/MTok | 2M | Enterprise, complex |
| `gemini-3-flash` | $0.075/MTok | $0.30/MTok | 1M | Fast, cost-effective |

### Setup

1. **Create Account**: Visit [aistudio.google.com](https://aistudio.google.com)

2. **Get API Key**:
   - Click "Get API Key" in the left sidebar
   - Select "Create API key in new project"
   - Copy the generated key (starts with `AIza`)

3. **Configure**:
   ```bash
   export GEMINI_API_KEY="AIza..."
   ```

4. **Verify**:
   ```bash
   convergio providers test
   # Output: Gemini ✓ Connected
   ```

### Free Tier

Gemini offers a generous free tier for development:

- **Gemini 3 Flash**: 15 RPM, 1M tokens/min
- **Gemini 3 Pro**: 2 RPM, 32K tokens/min

This is perfect for development and testing!

---

## Provider Comparison

### Cost Efficiency

| Task Type | Cheapest Option | Cost |
|-----------|----------------|------|
| Simple queries | Gemini 3 Flash | $0.075/$0.30 MTok |
| Routing/classification | GPT-5 Nano | $0.05/$0.40 MTok |
| Code generation | Claude Sonnet 4.5 | $3/$15 MTok |
| Deep reasoning | Claude Opus 4.5 | $15/$75 MTok |

### Feature Comparison

| Feature | Anthropic | OpenAI | Gemini |
|---------|-----------|--------|--------|
| Max Context | 1M | 400K | 2M |
| Tool Calling | ✓ | ✓ | ✓ |
| Vision | ✓ | ✓ | ✓ |
| Streaming | ✓ | ✓ | ✓ |
| Prompt Caching | ✓ | ✓ | ✓ |
| Batch API | ✓ | ✓ | ✓ |

---

## Troubleshooting

### "API key invalid"

- Ensure the key is copied completely (no spaces)
- Check the key hasn't expired
- Verify you're using the correct environment variable name

### "Rate limit exceeded"

- Convergio automatically retries with exponential backoff
- Consider upgrading your provider tier
- Use a fallback provider for redundancy

### "Model not found"

- Run `convergio models` to see available models
- Check model spelling (use exactly as shown)
- Some models require specific tier levels

### "Network error"

- Check your internet connection
- Verify firewall allows HTTPS to provider APIs
- Try `convergio providers test` for diagnostics

---

## Security Best Practices

1. **Never commit API keys** to version control
2. **Use environment variables** or secure storage (macOS Keychain)
3. **Rotate keys regularly**, especially if exposed
4. **Set budget limits** to prevent unexpected charges
5. **Monitor usage** via provider dashboards

---

## Support

- [Anthropic Documentation](https://docs.anthropic.com)
- [OpenAI Documentation](https://platform.openai.com/docs)
- [Gemini Documentation](https://ai.google.dev/gemini-api/docs)
- [Convergio Issues](https://github.com/Roberdan/convergio-cli/issues)
