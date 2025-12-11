# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.x     | :white_check_mark: |

## Reporting a Vulnerability

We take security seriously. If you discover a security vulnerability in Convergio CLI, please report it responsibly.

### How to Report

**Please do NOT report security vulnerabilities through public GitHub issues.**

Instead, please send an email to: **roberdan@fightthestroke.org**

Include the following information:

1. **Description** of the vulnerability
2. **Steps to reproduce** the issue
3. **Potential impact** of the vulnerability
4. **Suggested fix** (if any)

### What to Expect

- **Acknowledgment**: We will acknowledge receipt of your report within 48 hours
- **Assessment**: We will investigate and assess the vulnerability within 7 days
- **Resolution**: We aim to resolve critical vulnerabilities within 30 days
- **Disclosure**: We will coordinate with you on public disclosure timing

### Security Considerations

Convergio CLI includes several features that interact with the system:

#### Tool Execution
- `file_read`/`file_write`: Blocked for system paths (`/etc`, `/System`, etc.)
- `shell_exec`: Dangerous commands are blocked (`rm -rf`, `sudo`, etc.)
- `web_fetch`: Standard HTTP/HTTPS only

#### API Keys
- The `ANTHROPIC_API_KEY` is read from environment variables
- Never commit API keys to the repository
- Use `.env` files (which are gitignored)

#### Data Storage
- SQLite database stored in `data/` directory
- Contains conversation history and memories
- The `data/` directory is gitignored by default

### Best Practices for Users

1. **Protect your API key**: Never share or commit your Anthropic API key
2. **Review tool usage**: Monitor what tools Ali is using via debug mode
3. **Backup data**: The `data/` directory contains your conversation history
4. **Update regularly**: Pull the latest version for security patches

## Acknowledgments

We appreciate the security research community's efforts in responsibly disclosing vulnerabilities.

Thank you for helping keep Convergio CLI secure!
