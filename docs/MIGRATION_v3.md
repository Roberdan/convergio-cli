# Migration Guide: Convergio v2.x to v3.0

This guide helps you migrate from Convergio v2.x (single-provider) to v3.0 (multi-provider).

## What's New in v3.0

### Major Features

1. **Multi-Provider Support**
   - Anthropic (Claude)
   - OpenAI (GPT)
   - Google (Gemini)
   - Ollama (Local)

2. **Intelligent Model Routing**
   - Automatic model selection per agent
   - Budget-aware downgrading
   - Provider fallback chains

3. **Enhanced Cost Management**
   - Real-time cost tracking
   - Budget enforcement
   - Prompt caching

4. **New UI Features**
   - Status bar with token/cost display
   - Clickable file links (OSC 8)
   - Terminal resize handling

---

## Breaking Changes

### Configuration

#### API Key Environment Variables

**v2.x:**
```bash
export ANTHROPIC_API_KEY="sk-..."
```

**v3.0:**
```bash
# Multiple providers supported
export ANTHROPIC_API_KEY="sk-ant-..."
export OPENAI_API_KEY="sk-proj-..."
export GEMINI_API_KEY="AIza..."
```

#### Configuration File Location

**v2.x:** `~/.nous/config.json`

**v3.0:** `~/.convergio/config.json`

Migration:
```bash
mv ~/.nous ~/.convergio
```

#### Configuration Schema

**v2.x:**
```json
{
  "model": "claude-sonnet-4",
  "max_tokens": 4096
}
```

**v3.0:**
```json
{
  "providers": {
    "default": "anthropic",
    "anthropic": {"enabled": true},
    "openai": {"enabled": true},
    "gemini": {"enabled": true}
  },
  "routing": {
    "default_model": "claude-sonnet-4.5",
    "budget_aware": true
  }
}
```

---

### API Changes

#### Provider Initialization

**v2.x:**
```c
claude_init();
```

**v3.0:**
```c
provider_registry_init();
Provider* anthropic = provider_get(PROVIDER_ANTHROPIC);
anthropic->init(anthropic);
```

#### Chat Function

**v2.x:**
```c
char* response = claude_chat(model, system, user);
```

**v3.0:**
```c
Provider* provider = provider_get(PROVIDER_ANTHROPIC);
TokenUsage usage;
char* response = provider->chat(provider, model, system, user, &usage);
```

#### Token Counting

**v2.x:** Manual estimation

**v3.0:**
```c
uint64_t tokens = tokens_estimate(text, PROVIDER_ANTHROPIC);
double cost = tokens_calculate_cost(input, output, model);
```

---

### Agent Configuration

#### Agent Definition Files

**v2.x:** `~/.nous/agents/marco.md`
```markdown
# Marco - The Coder

You are an expert software engineer...
```

**v3.0:** `~/.convergio/agents/marco.json`
```json
{
  "name": "marco",
  "description": "Expert coder",
  "role": "coder",
  "model": {
    "provider": "anthropic",
    "model_id": "claude-sonnet-4.5"
  },
  "system_prompt": "You are an expert software engineer..."
}
```

Migration script:
```bash
# Convert markdown agents to JSON
convergio migrate agents ~/.nous/agents
```

---

## Migration Steps

### 1. Update Installation

```bash
# Backup current installation
cp -r ~/.nous ~/.nous.backup

# Update Convergio
brew upgrade convergio
# or
git pull && cmake --build build
```

### 2. Migrate Configuration

```bash
# Create new config directory
mkdir -p ~/.convergio/agents

# Migrate config
convergio migrate config ~/.nous/config.json

# Or manually:
mv ~/.nous/config.json ~/.convergio/config.json
```

### 3. Add Provider API Keys

```bash
# Run setup wizard
convergio setup

# Or set manually
export ANTHROPIC_API_KEY="sk-ant-..."
export OPENAI_API_KEY="sk-proj-..."
export GEMINI_API_KEY="AIza..."
```

### 4. Migrate Agents

```bash
# Automatic migration
convergio migrate agents ~/.nous/agents/

# Verify
convergio agents list
```

### 5. Update Scripts

If you have scripts using Convergio:

**v2.x:**
```bash
nous "Hello world"
```

**v3.0:**
```bash
convergio "Hello world"
# Or with specific model
convergio --model gpt-4o "Hello world"
```

### 6. Test Installation

```bash
# Test all providers
convergio providers test

# Test specific provider
convergio providers test anthropic

# Run a test query
convergio "What is 2+2?"
```

---

## Feature Mapping

| v2.x Feature | v3.0 Equivalent |
|--------------|-----------------|
| `nous` command | `convergio` command |
| `~/.nous/` | `~/.convergio/` |
| Single model | Multi-provider routing |
| Manual cost tracking | Automatic cost tracking |
| Basic terminal output | Status bar + hyperlinks |

---

## Compatibility Mode

For gradual migration, enable v2 compatibility:

```json
{
  "compatibility": {
    "v2_mode": true,
    "legacy_paths": true
  }
}
```

This enables:
- `~/.nous/` path support
- Legacy config format
- Single-provider mode

---

## New Features to Explore

### 1. Model Selection

```bash
# Use specific model
convergio --model claude-opus-4.5 "Complex reasoning task"

# Use specific provider
convergio --provider openai "Generate code"

# Budget-limited session
convergio --budget 1.00 "Start session"
```

### 2. Cost Monitoring

```bash
# View current costs
convergio cost status

# View by agent
convergio cost agents

# Export cost report
convergio cost export --format csv
```

### 3. Provider Status

```bash
# Check all providers
convergio providers status

# Test connectivity
convergio providers test

# View available models
convergio models list
```

### 4. Agent Management

```bash
# List agents
convergio agents list

# View agent config
convergio agents config marco

# Update agent model
convergio agents set-model marco openai gpt-5.2-thinking
```

---

## Troubleshooting Migration

### "Command not found: convergio"

```bash
# Reinstall
brew reinstall convergio

# Or add to PATH
export PATH="$PATH:/usr/local/bin"
```

### "Config file not found"

```bash
# Create default config
convergio init

# Or migrate from v2
convergio migrate config ~/.nous/config.json
```

### "API key invalid"

```bash
# Verify key format
echo $ANTHROPIC_API_KEY  # Should start with sk-ant-

# Re-run setup
convergio setup
```

### "Model not available"

```bash
# Check available models
convergio models list

# Model names changed in v3
# v2: claude-sonnet-4
# v3: claude-sonnet-4.5
```

### "Agent not working"

```bash
# Check agent config
convergio agents config <name>

# Regenerate from template
convergio agents create --name <name> --role <role>
```

---

## Rollback

If you need to rollback to v2.x:

```bash
# Restore backup
rm -rf ~/.convergio
mv ~/.nous.backup ~/.nous

# Downgrade
brew install convergio@2.0.11
# or
git checkout v2.0.11
cmake --build build
```

---

## Getting Help

- Documentation: `/docs/` directory
- Issues: [github.com/Roberdan/convergio-cli/issues](https://github.com/Roberdan/convergio-cli/issues)
- Migration issues: Use label `migration`

---

## Summary Checklist

- [ ] Backup `~/.nous/` directory
- [ ] Update Convergio to v3.0
- [ ] Move config to `~/.convergio/`
- [ ] Add provider API keys
- [ ] Migrate agent definitions to JSON
- [ ] Update any scripts using `nous` → `convergio`
- [ ] Test all providers with `convergio providers test`
- [ ] Explore new features (cost tracking, model routing)
