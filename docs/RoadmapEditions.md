# Convergio Editions Roadmap

**Created**: 2025-12-21
**Status**: Active Discussion
**Branch**: `feature/runtime-edition-switching`

---

## Current State (v5.3.1)

### Edition System
- **Runtime switching** for Master/Business/Developer via CLI, env var, or config
- **Compile-time locking** for Education edition (child safety)
- Priority: CLI flag > CONVERGIO_EDITION env var > config.toml
- PR #73 implements this system

### Editions Available
| Edition | Binary | Use Case |
|---------|--------|----------|
| Master | `convergio` | All 60+ agents (default) |
| Education | `convergio-edu` | 14 historical Maestri + accessibility |
| Business | `convergio` | Sales, marketing, finance agents |
| Developer | `convergio` | Code review, DevOps, security agents |

---

## Open Discussion: Multi-Repository Architecture

### The Challenge
As editions grow (more verticals like Education, Healthcare, Legal), maintaining everything in a single repository becomes complex:
- Contributors who only care about one vertical must understand the whole codebase
- Dependency updates affect all editions
- Release cycles are coupled
- Testing overhead grows

### Proposed Solution: Core + Plugins

```
convergio-cli/                    # Core kernel (main repo)
├── src/core/                     # Orchestrator, config, CLI
├── src/providers/                # API providers
├── src/agentic/                  # Tool system
└── plugin-api/                   # Plugin interface for editions

convergio-education/              # Education plugin repo
├── agents/                       # 14 Maestri definitions
├── features/                     # Quiz, flashcards, study sessions
└── curricula/                    # Italian, UK, US curricula

convergio-business/               # Business plugin repo
├── agents/                       # Business agents
└── features/                     # CRM, pipeline, analytics

convergio-developer/              # Developer plugin repo
├── agents/                       # Code review, security agents
└── features/                     # Git, PR, test integrations
```

### Benefits
1. **Focused contributions**: Contributors can work on single verticals
2. **Independent release cycles**: Education can release without affecting core
3. **Reduced dependencies**: Each plugin only has what it needs
4. **Simpler testing**: Test one vertical at a time
5. **Clearer ownership**: Different teams can own different plugins

### Technical Requirements

#### Plugin API Design
```c
// plugin-api/convergio_plugin.h
typedef struct {
    const char *id;              // "education", "business"
    const char *version;
    const char **agents;         // NULL-terminated list
    const char **features;
    const char **commands;

    // Callbacks
    int (*init)(void);
    int (*shutdown)(void);
    int (*on_agent_switch)(const char *agent_id);
} ConvergioPlugin;

// Core discovers plugins in ~/.convergio/plugins/
int convergio_load_plugins(void);
```

#### Distribution Options
1. **Homebrew formulas**: `brew install convergio-education`
2. **Built-in plugin manager**: `convergio plugin install education`
3. **Compile-time bundling**: Still needed for Education (child safety)

### Migration Path
1. **Phase 1** (Current): Single repo with edition system
2. **Phase 2**: Extract plugin API, keep editions internal
3. **Phase 3**: Move Education to separate repo (still compile-time)
4. **Phase 4**: Move Business/Developer to plugins (runtime-loadable)

---

## Pending Features

### Localization (Phase 13)
- [ ] Multi-language support for UI strings
- [ ] Education Maestri speak student's language
- [ ] RTL support for Arabic/Hebrew

### Hot-Reload (Deferred)
- Hot-reload agent registry on edition switch
- Currently requires restart (acceptable per user feedback)

### Education-Specific
- [ ] 14 Maestri with full personality
- [ ] Accessibility profiles (dyslexia, ADHD, etc.)
- [ ] Curriculum engine (Italian school system)
- [ ] Quiz and study session system
- [ ] Anna reminder integration

---

## Architecture Decision

**ADR-003 Decision**: Education edition MUST remain compile-time locked.
- Reason: Child safety compliance for schools
- Parents/schools can trust the binary is "locked"
- Cannot be circumvented by environment variables or config

---

## Questions to Resolve

1. **Plugin discovery**: ~/.convergio/plugins/ or bundled?
2. **Version compatibility**: How to ensure plugin works with core version?
3. **Feature flags**: Should features be plugin-level or global?
4. **Agent override**: Can plugins override core agents?

---

## Next Steps

1. Merge PR #73 (runtime edition switching)
2. Merge `feature/education-pack` into main
3. Design plugin API specification
4. Create ADR for plugin architecture
5. Prototype with Education as first plugin

---

**Last Updated**: 2025-12-21
**Author**: Roberto with AI team support
