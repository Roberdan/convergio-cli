# Privacy Policy - ConvergioCLI

**Last Updated:** 2025-12-21
**Version:** 1.0

## Overview

ConvergioCLI is a local-first AI assistant that prioritizes user privacy. This document explains how data is handled when using ConvergioCLI.

## Data Processing

### Local Processing
- **Conversation history** is stored locally in SQLite databases on your machine
- **Workflow checkpoints** are persisted locally for session recovery
- **Configuration files** (config.toml) remain on your local filesystem
- **No telemetry** is sent to ConvergioCLI developers

### API Credentials
- **API keys** can be provided via:
  - Environment variables (e.g., `ANTHROPIC_API_KEY`)
  - macOS Keychain (secure, encrypted storage)
  - OAuth 2.0 authentication flow
- **Credentials are never logged** or stored in plaintext
- **Keychain storage** uses Apple's secure enclave when available

## Third-Party Providers

When you use ConvergioCLI with AI providers, your prompts and responses are sent to those providers. Each provider has their own privacy policy:

| Provider | Privacy Policy |
|----------|---------------|
| Anthropic (Claude) | https://www.anthropic.com/privacy |
| OpenAI | https://openai.com/policies/privacy-policy |
| Google (Gemini) | https://policies.google.com/privacy |
| OpenRouter | https://openrouter.ai/privacy |
| Ollama (Local) | Fully local, no external transmission |

### Important Notes:
- **We do not control** how third-party providers process your data
- **API requests** contain your prompts and conversation context
- **Choose providers** based on your privacy requirements
- **Ollama option** keeps all processing local if privacy is critical

## What We DON'T Do

- **No usage tracking** - We don't track how you use ConvergioCLI
- **No analytics** - No Google Analytics, Mixpanel, or similar
- **No data selling** - We never sell or share your data
- **No cloud sync** - All data stays on your machine
- **No telemetry calls home** - The CLI doesn't phone home

## Data Retention

- **Local databases** persist until you delete them
- **Checkpoints** are stored locally and can be cleared manually
- **Logs** (if enabled) are stored locally in `~/.convergio/logs/`
- **You control** all data deletion

## Education Edition

The Education Edition includes additional safeguards:
- Content filtering for age-appropriate responses
- Cannot be switched to other editions at runtime
- Designed for educational environments

## Security Measures

- **Input validation** on all user inputs
- **Ethical guardrails** for multi-agent workflows
- **Human-in-the-loop** for sensitive operations
- **SQL injection protection** with parameterized queries
- **Path traversal prevention** for file operations

## User Rights

You have full control over your data:
- **Access**: All data is stored in `~/.convergio/`
- **Delete**: Remove the `.convergio` folder to delete all data
- **Portability**: Export conversations via CLI commands
- **Modification**: Edit config.toml for all settings

## Disclaimer

ConvergioCLI is provided "as is" without warranty. While we implement security measures, users are responsible for:
- Keeping API keys secure
- Understanding third-party provider policies
- Compliance with applicable laws and regulations
- Backing up important data

## Contact

For privacy concerns or questions:
- GitHub Issues: https://github.com/Roberdan/convergio-cli/issues
- Security Issues: Report via private disclosure

---

*This privacy policy applies to ConvergioCLI v5.4.0 and later versions.*
