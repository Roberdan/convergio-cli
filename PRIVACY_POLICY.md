# Privacy Policy

**Effective Date:** December 12, 2025
**Last Updated:** December 12, 2025

## Introduction

Convergio CLI ("Convergio," "the Software") is committed to protecting your privacy. This Privacy Policy explains how we handle your data when you use Convergio.

**TL;DR:** Convergio is privacy-first. Your data stays on your machine. We don't collect, store, or transmit your conversations or API keys.

---

## 1. Data We DO NOT Collect

Convergio does NOT collect:

- **Conversations**: Your prompts and AI responses stay on your machine
- **API Keys**: Stored locally in macOS Keychain or environment variables
- **Personal Information**: We don't ask for or store names, emails, etc.
- **Usage Telemetry**: No analytics or tracking by default
- **File Contents**: Files you work with are not transmitted to us
- **System Information**: We don't collect hardware/software details

---

## 2. Data Stored Locally

Convergio stores the following data **only on your machine**:

### 2.1 Configuration Files

Location: `~/.convergio/`

| File | Contents | Sensitive? |
|------|----------|------------|
| `config.json` | Preferences, settings | No |
| `agents/*.json` | Custom agent configurations | No |
| `models/registry.json` | Model pricing cache | No |
| `safety.json` | Safety settings | No |

### 2.2 API Keys

API keys are stored using:
- **macOS Keychain** (recommended, encrypted)
- **Environment Variables** (user's responsibility to secure)

**We never have access to your API keys.**

### 2.3 Audit Logs (Optional)

If you enable audit logging (`safety.json`):
- Logs are stored locally at `~/.convergio/logs/`
- You can view, export, or delete these logs at any time
- Logs are never transmitted to any server

---

## 3. Data Sent to Third Parties

### 3.1 LLM Providers

When you use Convergio, your prompts are sent **directly** to the LLM providers you've configured:

| Provider | Data Sent | Privacy Policy |
|----------|-----------|----------------|
| Anthropic | Prompts, system context | [anthropic.com/privacy](https://www.anthropic.com/privacy) |
| OpenAI | Prompts, system context | [openai.com/privacy](https://openai.com/policies/privacy-policy) |
| Google | Prompts, system context | [ai.google.dev/terms](https://ai.google.dev/gemini-api/terms) |

**Important:**
- Convergio acts as a client that sends requests to these providers
- Data handling by providers is governed by THEIR privacy policies
- We recommend reviewing each provider's data retention policies

### 3.2 Update Checks

When checking for updates (manual only):
- Convergio fetches `models.json` from GitHub
- No personal data is transmitted
- Standard GitHub CDN logging may apply

---

## 4. Data You Control

You have full control over your data:

### 4.1 View Your Data

```bash
# View configuration
cat ~/.convergio/config.json

# View audit logs (if enabled)
ls ~/.convergio/logs/
```

### 4.2 Delete Your Data

```bash
# Remove all Convergio data
rm -rf ~/.convergio/

# Remove from Keychain (macOS)
security delete-generic-password -s "convergio"
```

### 4.3 Export Your Data

```bash
# Export configuration
cp -r ~/.convergio/ ~/convergio-backup/
```

---

## 5. Children's Privacy

Convergio is not intended for use by children under 13 years of age. We do not knowingly collect data from children.

---

## 6. Security Measures

### 6.1 API Key Protection

- Keys stored in macOS Keychain (AES-256 encryption)
- Keys never logged or displayed in full
- Environment variables are user's responsibility

### 6.2 Local Data Protection

- Configuration files are stored in user home directory
- No sensitive data in configuration files
- Audit logs don't contain API keys

### 6.3 Network Security

- All API calls use HTTPS/TLS
- Certificate validation enabled
- No insecure fallbacks

---

## 7. GDPR Compliance (EU Users)

Under GDPR, you have the right to:

| Right | How Convergio Complies |
|-------|------------------------|
| Access | All data is local, you have full access |
| Rectification | Edit `~/.convergio/` files directly |
| Erasure | Delete `~/.convergio/` directory |
| Portability | Copy your config directory |
| Restriction | Disable features in settings |
| Object | Don't use features you object to |

**Data Controller:** Since Convergio doesn't collect your data, there is no data controller for Convergio. For data sent to LLM providers, each provider is their own data controller.

---

## 8. California Privacy Rights (CCPA)

California residents have the right to:

| Right | Convergio Response |
|-------|-------------------|
| Know what data is collected | No personal data collected |
| Delete personal data | No personal data to delete |
| Opt-out of sale | We don't sell any data |
| Non-discrimination | N/A - no data collected |

---

## 9. Changes to This Policy

We may update this Privacy Policy from time to time. Changes will be documented in:
- This file (version history at top)
- CHANGELOG.md
- Release notes

Your continued use of Convergio after changes constitutes acceptance of the updated policy.

---

## 10. Open Source Transparency

Convergio is open source. You can:
- Review our code: [github.com/Roberdan/convergio-cli](https://github.com/Roberdan/convergio-cli)
- Verify our privacy claims
- Build from source
- Report privacy concerns via issues

---

## 11. Contact

For privacy-related questions or concerns:

- **GitHub Issues**: [github.com/Roberdan/convergio-cli/issues](https://github.com/Roberdan/convergio-cli/issues)
- **Security Issues**: See [SECURITY.md](SECURITY.md)

---

## Summary

| Question | Answer |
|----------|--------|
| Do you collect my data? | No |
| Do you store my conversations? | No |
| Do you have my API keys? | No |
| Where is my data? | On your machine only |
| Who has my prompts? | Only the LLM provider you use |
| Can I delete everything? | Yes, delete `~/.convergio/` |

---

**Privacy by Design:** Convergio is built with privacy as a core principle. We believe your AI conversations are your business, not ours.
