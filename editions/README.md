# Convergio Editions

**Convergio** is an AI-powered agent orchestration platform that brings together specialized AI agents to help you accomplish complex tasks. Unlike single-purpose AI tools, Convergio coordinates multiple expert agents that collaborate, share context, and deliver comprehensive solutions.

## Why Convergio?

### Multi-Agent Intelligence
Instead of a single AI that tries to do everything, Convergio deploys specialized agents that are experts in their domain. An architect agent designs systems, a security agent validates them, and a DevOps agent deploys them - all coordinated seamlessly.

### Persistent Memory
Convergio remembers context across sessions. Your projects, preferences, and decisions are preserved, allowing agents to build on previous work without starting from scratch.

### Enterprise-Ready
Built on a robust C kernel with provider abstraction, Convergio supports multiple AI backends (Anthropic Claude, OpenAI, local models) and integrates with your existing tools.

### Edition-Specific Focus
Each edition is optimized for specific use cases, reducing noise and providing agents tailored to your needs.

---

## Available Editions

| Edition | Agents | Best For | Key Value |
|---------|:------:|----------|-----------|
| **Master** | 60+ | Power users, enterprises | Complete AI workforce |
| **Education** | 18 | Students, teachers | Learn from history's greatest minds |
| **Business** | 10 | SMBs, sales teams | Close deals, delight customers |
| **Developer** | 11 | Dev teams, DevOps | Ship quality code faster |
| **Strategy** | 10 | Executives, consultants | Make better decisions |
| **Creative** | 6 | Designers, creatives | Breakthrough ideas and designs |

---

## Building Editions

```bash
# Build with default Master edition (all agents)
make

# Build specific edition
make EDITION=education   # Education pack
make EDITION=business    # Business pack
make EDITION=developer   # Developer pack
make EDITION=master      # Explicit master
```

---

## Core Agents (All Editions)

Every Convergio edition includes:

| Agent | Role | Superpower |
|-------|------|------------|
| **Ali** | Chief of Staff | Orchestrates all agents, makes complex decisions |
| **Anna** | Executive Assistant | Never miss a deadline, smart reminders |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         CONVERGIO CORE                               │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  Kernel: Memory, Context, Tool Orchestration, UI              │   │
│  └──────────────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │  Providers: Claude | GPT-4 | Local Models | Custom           │   │
│  └──────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
                                  │
         ┌────────────────────────┼────────────────────┐
         │                        │                    │
    ┌────▼────┐            ┌─────▼─────┐         ┌────▼────┐
    │EDUCATION│            │ BUSINESS  │         │DEVELOPER│
    │ 18 agents│            │ 10 agents │         │11 agents│
    └─────────┘            └───────────┘         └─────────┘
```

---

## Release & Distribution

Each edition is released as a separate binary:

| Edition | Binary | Installer |
|---------|--------|-----------|
| Master | `convergio` | `Convergio-5.3.1.dmg` |
| Education | `convergio-edu` | `Convergio-Education-5.3.1.dmg` |
| Business | `convergio-biz` | `Convergio-Business-5.3.1.dmg` |
| Developer | `convergio-dev` | `Convergio-Developer-5.3.1.dmg` |

### Release Process

1. **Tag**: `git tag v5.3.1-edu`
2. **Build**: `make EDITION=education`
3. **Test**: `make test EDITION=education`
4. **Package**: `make dmg EDITION=education`
5. **Release**: GitHub release with edition-specific assets

---

## Edition Details

- [Master Edition](README-master.md) - Complete AI workforce
- [Education Edition](README-education.md) - Virtual classroom with historical teachers
- [Business Edition](README-business.md) - Sales and customer success
- [Developer Edition](README-developer.md) - Code quality and DevOps

---

## Getting Started

```bash
# Clone the repository
git clone https://github.com/convergio/convergio-cli.git

# Build your preferred edition
cd convergio-cli
make EDITION=education

# Run
./build/bin/convergio
```

---

*Copyright (c) 2025 Convergio.io - All rights reserved*
