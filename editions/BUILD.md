# Building Convergio Editions

This document explains how to build different Convergio editions.

## Prerequisites

- macOS 14+ (Apple Silicon or Intel)
- Xcode Command Line Tools: `xcode-select --install`
- ccache (optional, for faster rebuilds): `brew install ccache`

## Quick Start

```bash
# Clone the repository
git clone https://github.com/convergio/convergio-cli.git
cd convergio-cli

# Build default (Master) edition
make

# Run
./build/bin/convergio
```

## Building Specific Editions

### Master Edition (Default)

All 70+ agents, full functionality.

```bash
make
# or explicitly:
make EDITION=master
```

### Education Edition

18 agents: 15 historical teachers + Ali, Anna, Jenny.

```bash
make EDITION=education
```

### Business Edition

10 agents: Sales, marketing, customer success, strategy.

```bash
make EDITION=business
```

### Developer Edition

11 agents: Code review, architecture, DevOps, security.

```bash
make EDITION=developer
```

## Build Process Overview

```
┌─────────────────────────────────────────────────────────────┐
│  1. EMBED AGENTS                                             │
│     scripts/embed_agents.sh                                  │
│     - Reads src/agents/definitions/*.md                      │
│     - Reads src/agents/definitions/education/*.md            │
│     - Generates src/agents/embedded_agents.c                 │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  2. COMPILE                                                  │
│     make EDITION=<name>                                      │
│     - Sets -DCONVERGIO_EDITION_<NAME> flag                   │
│     - Compiles all source files                              │
│     - Links binary                                           │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  3. EDITION FILTERING                                        │
│     At runtime, edition.c filters agents:                    │
│     - edition_has_agent() checks whitelist                   │
│     - Only whitelisted agents shown in /agents               │
│     - Non-whitelisted agents blocked                         │
└─────────────────────────────────────────────────────────────┘
```

## Regenerating Embedded Agents

If you add or modify agent definition files:

```bash
# Regenerate embedded agents
./scripts/embed_agents.sh

# Rebuild
make EDITION=<name>
```

## Clean Build

```bash
# Clean all build artifacts
make clean

# Full rebuild
make EDITION=education
```

## Debug Build

```bash
# Build with debug symbols and sanitizers
make EDITION=education DEBUG=1
```

## Release Build

```bash
# Optimized release build
make EDITION=education RELEASE=1
```

## Edition Configuration Files

| File | Purpose |
|------|---------|
| `src/core/edition.c` | Agent/feature/command whitelists |
| `include/nous/edition.h` | Edition types and API |
| `Makefile` | Build flags per edition |

## Adding a New Edition

1. Add edition type to `include/nous/edition.h`:
   ```c
   typedef enum {
       EDITION_MASTER = 0,
       EDITION_EDUCATION = 1,
       // Add new edition here
       EDITION_MYEDITION = 5
   } ConvergioEdition;
   ```

2. Add whitelist to `src/core/edition.c`:
   ```c
   static const char *MYEDITION_AGENTS[] = {
       "ali-chief-of-staff",
       "agent-1",
       "agent-2",
       NULL
   };
   ```

3. Add to EDITIONS array in `src/core/edition.c`

4. Add build flag to `Makefile`:
   ```makefile
   else ifeq ($(EDITION),myedition)
       CFLAGS += -DCONVERGIO_EDITION_MYEDITION
       EDITION_SUFFIX = -my
   ```

5. Create README: `editions/README-myedition.md`

## Troubleshooting

### "Agent not found" in Education Edition

Ensure agent IDs in `EDUCATION_AGENTS[]` match the embedded agent names:
- Check `src/agents/embedded_agents.c` for exact filenames
- Agent ID = filename without `.md` extension

### Build fails with "undefined reference"

Run embed script first:
```bash
./scripts/embed_agents.sh
make EDITION=education
```

### ccache not working

Verify ccache is installed:
```bash
which ccache
# Should output: /opt/homebrew/bin/ccache
```

## CI/CD

GitHub Actions builds all editions:

```yaml
jobs:
  build:
    strategy:
      matrix:
        edition: [master, education, business, developer]
    steps:
      - run: make EDITION=${{ matrix.edition }}
```

---

*See also: [ADR-003 Edition System Architecture](../docs/adr/ADR-003-edition-system-architecture.md)*
