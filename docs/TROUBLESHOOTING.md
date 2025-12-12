# Troubleshooting Guide

Common issues and solutions for Convergio CLI.

---

## Quick Diagnostics

Run this first:
```bash
convergio diagnose
```

This checks:
- Configuration files
- API key validity
- Provider connectivity
- Model availability
- System resources

---

## Installation Issues

### "Command not found: convergio"

**Cause:** Binary not in PATH

**Solution:**
```bash
# macOS with Homebrew
brew link convergio

# Manual installation
export PATH="$PATH:/usr/local/bin"

# Verify
which convergio
```

### "Cannot load library"

**Cause:** Missing dependencies

**Solution:**
```bash
# macOS
brew install curl openssl readline

# Rebuild
cmake --build build --clean-first
```

### "Metal framework not found"

**Cause:** Running on non-Apple Silicon

**Solution:** Convergio requires Apple Silicon (M1/M2/M3). Run on compatible hardware or use Docker (limited features).

---

## API Key Issues

### "Invalid API key"

**Symptoms:**
- 401 Unauthorized errors
- "Authentication failed"

**Solutions:**

1. **Check key format:**
   ```bash
   # Anthropic keys start with sk-ant-
   echo $ANTHROPIC_API_KEY | head -c 10

   # OpenAI keys start with sk-proj- or sk-
   echo $OPENAI_API_KEY | head -c 8

   # Gemini keys start with AIza
   echo $GEMINI_API_KEY | head -c 4
   ```

2. **Verify key is set:**
   ```bash
   convergio providers test
   ```

3. **Re-enter key:**
   ```bash
   convergio setup
   ```

### "API key not found"

**Cause:** Environment variable not set

**Solutions:**

1. **Set in shell profile:**
   ```bash
   # ~/.zshrc or ~/.bashrc
   export ANTHROPIC_API_KEY="sk-ant-..."
   source ~/.zshrc
   ```

2. **Use keychain (recommended):**
   ```bash
   convergio setup  # Stores in macOS Keychain
   ```

### "Rate limit exceeded"

**Cause:** Too many requests

**Solutions:**

1. **Wait and retry:**
   Convergio automatically retries with backoff.

2. **Check your tier:**
   ```bash
   # OpenAI tiers require spending history
   # Anthropic has different rate limits per tier
   ```

3. **Use fallback provider:**
   Configure multiple providers for automatic failover.

---

## Model Issues

### "Model not found"

**Cause:** Invalid model name or unavailable

**Solutions:**

1. **Check available models:**
   ```bash
   convergio models list
   ```

2. **Use correct model name:**
   ```bash
   # Correct
   convergio --model claude-sonnet-4.5

   # Incorrect
   convergio --model claude-sonnet  # Missing version
   ```

3. **Check provider tier:**
   Some models require higher API tiers.

### "Context length exceeded"

**Cause:** Input too long for model

**Solutions:**

1. **Use model with larger context:**
   ```bash
   convergio --model claude-sonnet-4.5  # 1M tokens
   convergio --model gemini-3-pro       # 2M tokens
   ```

2. **Truncate input:**
   ```bash
   convergio "Summarize: $(head -c 50000 large_file.txt)"
   ```

3. **Use streaming:**
   ```bash
   convergio --stream "Long task..."
   ```

### "Output truncated"

**Cause:** max_tokens limit reached

**Solution:**
```bash
convergio --max-tokens 16384 "Generate long content..."
```

---

## Connection Issues

### "Network error"

**Symptoms:**
- Connection timeouts
- "Failed to connect"

**Solutions:**

1. **Check internet:**
   ```bash
   curl -I https://api.anthropic.com
   curl -I https://api.openai.com
   ```

2. **Check firewall:**
   Allow HTTPS to:
   - api.anthropic.com
   - api.openai.com
   - generativelanguage.googleapis.com

3. **Check proxy:**
   ```bash
   export HTTPS_PROXY="http://proxy:8080"
   ```

### "SSL certificate error"

**Cause:** Certificate validation failed

**Solutions:**

1. **Update CA certificates:**
   ```bash
   # macOS
   brew install ca-certificates
   ```

2. **Check system time:**
   ```bash
   date  # Should be current
   ```

### "Timeout waiting for response"

**Cause:** Slow response or network issues

**Solutions:**

1. **Increase timeout:**
   ```bash
   convergio --timeout 120 "Complex task..."
   ```

2. **Use streaming:**
   ```bash
   convergio --stream "Long task..."
   ```

3. **Try different provider:**
   ```bash
   convergio --provider gemini "Same task..."
   ```

---

## Cost Issues

### "Budget exceeded"

**Cause:** Session/daily budget limit reached

**Solutions:**

1. **Increase budget:**
   ```bash
   convergio --budget 10.00 "Continue..."
   ```

2. **Check current spend:**
   ```bash
   convergio cost status
   ```

3. **Use cheaper model:**
   ```bash
   convergio --model gpt-5-nano "Simple task"
   ```

### "Unexpected high costs"

**Solutions:**

1. **Review usage:**
   ```bash
   convergio cost history --last 24h
   ```

2. **Check agent costs:**
   ```bash
   convergio cost agents
   ```

3. **Enable budget limits:**
   ```json
   {
     "cost": {
       "session_budget": 5.00,
       "daily_budget": 20.00
     }
   }
   ```

---

## Agent Issues

### "Agent not responding"

**Solutions:**

1. **Check agent config:**
   ```bash
   convergio agents config <name>
   ```

2. **Verify model availability:**
   ```bash
   convergio providers test
   ```

3. **Check system prompt:**
   Ensure it's not causing refusals.

### "Wrong agent selected"

**Solutions:**

1. **Specify agent:**
   ```bash
   convergio --agent marco "Code task"
   ```

2. **Check routing rules:**
   ```bash
   convergio agents list
   ```

### "Agent loop / infinite delegation"

**Cause:** Agents delegating back and forth

**Solutions:**

1. **Check agent prompts:**
   Ensure they don't delegate unnecessarily.

2. **Limit delegation depth:**
   ```json
   {
     "orchestrator": {
       "max_delegation_depth": 3
     }
   }
   ```

---

## Performance Issues

### "Slow responses"

**Solutions:**

1. **Use faster model:**
   ```bash
   convergio --model claude-haiku-4.5 "Quick task"
   ```

2. **Enable streaming:**
   ```bash
   convergio --stream "Task..."
   ```

3. **Check network latency:**
   ```bash
   ping api.anthropic.com
   ```

### "High memory usage"

**Cause:** Large context or many agents

**Solutions:**

1. **Reduce context:**
   ```bash
   convergio --context-limit 50000
   ```

2. **Limit active agents:**
   ```bash
   convergio --max-agents 3
   ```

### "Terminal flickering"

**Cause:** Status bar refresh

**Solution:**
```bash
convergio --no-status "Task..."
```

---

## Configuration Issues

### "Config file not found"

**Solutions:**

1. **Create default config:**
   ```bash
   convergio init
   ```

2. **Check path:**
   ```bash
   ls ~/.convergio/config.json
   ```

### "Invalid config syntax"

**Solutions:**

1. **Validate JSON:**
   ```bash
   cat ~/.convergio/config.json | jq .
   ```

2. **Reset to defaults:**
   ```bash
   rm ~/.convergio/config.json
   convergio init
   ```

### "Settings not applied"

**Cause:** Config not reloaded

**Solution:**
```bash
convergio reload
# or restart convergio
```

---

## Terminal Issues

### "Garbled output"

**Cause:** Terminal doesn't support ANSI codes

**Solutions:**

1. **Use plain mode:**
   ```bash
   convergio --no-color "Task..."
   ```

2. **Check terminal:**
   ```bash
   echo $TERM
   # Should be xterm-256color or similar
   ```

### "Links not clickable"

**Cause:** Terminal doesn't support OSC 8

**Solution:**
- Use supported terminal: iTerm2, WezTerm, VS Code Terminal, Hyper
- Or disable hyperlinks:
  ```bash
  convergio --no-hyperlinks
  ```

### "Status bar overlapping"

**Cause:** Terminal resize not detected

**Solution:**
```bash
# Resize detection should be automatic
# If not working, try:
kill -WINCH $$
```

---

## Getting Help

### Logs

```bash
# View logs
cat ~/.convergio/logs/convergio.log

# Enable debug logging
convergio --debug "Task..."
```

### Diagnostics

```bash
# Full system check
convergio diagnose --verbose

# Export for bug report
convergio diagnose --export > diagnostics.txt
```

### Support

- **GitHub Issues:** [github.com/Roberdan/convergio-cli/issues](https://github.com/Roberdan/convergio-cli/issues)
- **Include:** Error message, `convergio diagnose` output, config (redact API keys)

---

## Common Error Messages

| Error | Cause | Solution |
|-------|-------|----------|
| `PROVIDER_ERR_AUTH` | Invalid API key | Check key format, re-run setup |
| `PROVIDER_ERR_RATE_LIMIT` | Too many requests | Wait, or use fallback |
| `PROVIDER_ERR_CONTEXT_LENGTH` | Input too long | Use larger model or truncate |
| `PROVIDER_ERR_CONTENT_FILTER` | Content blocked | Rephrase request |
| `PROVIDER_ERR_NETWORK` | Connection failed | Check internet, firewall |
| `PROVIDER_ERR_TIMEOUT` | Response too slow | Increase timeout, use streaming |
| `PROVIDER_ERR_INVALID_MODEL` | Model not found | Check model name |
