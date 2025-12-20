# ADR-003: Edition System Architecture

**Status**: Accepted
**Date**: 2025-12-20
**Decision Makers**: Roberto + AI Agent Team

---

## Context

Convergio needs to support multiple "editions" (Education, Business, Developer, Master) with different agent sets, features, and target audiences. We need to decide between:

1. **Compile-time editions**: Separate binaries per edition
2. **Runtime editions**: Single binary with runtime switching
3. **Hybrid**: Combination of both approaches

---

## Decision

We adopt a **Hybrid Approach**:

### Education Edition: Separate Binary (Compile-Time)

- Built with `make EDITION=education`
- Produces `convergio-edu` binary
- Contains ONLY education-appropriate agents
- No possibility of accessing other agents (security for schools/children)

### Master/Business/Developer: Single Binary with Runtime Switching

- Built with `make` (default Master)
- Runtime edition selection via:
  - CLI flag: `convergio --edition business`
  - Environment: `CONVERGIO_EDITION=developer`
  - Config file: `~/.convergio/config.toml`
- License key validation for non-Master editions

---

## Rationale

### Why Separate Binary for Education?

1. **Child Safety**: Schools and parents need assurance that business/adult agents are not accessible
2. **Compliance**: Educational institutions may require software audits
3. **Smaller Footprint**: Education devices often have limited storage
4. **Simpler Licensing**: Per-school or per-student licensing without complex activation

### Why Runtime Switching for Professional Editions?

1. **Flexibility**: Professionals may need multiple editions
2. **Single Update Path**: One binary to update
3. **Trial/Upgrade Flow**: Easy to try Business, then upgrade to Master
4. **Enterprise Deployment**: IT can deploy one binary, configure per-user

---

## Technical Implementation

### Current (Phase 1): Compile-Time Only

```c
// edition.c - Compile-time selection
#if defined(CONVERGIO_EDITION_EDUCATION)
    #define CURRENT_EDITION EDITION_EDUCATION
#elif defined(CONVERGIO_EDITION_BUSINESS)
    #define CURRENT_EDITION EDITION_BUSINESS
// ...
#endif

// Whitelists are static const arrays
static const char *EDUCATION_AGENTS[] = { ... };
```

### Future (Phase 2): Runtime Switching

```c
// edition.c - Runtime selection
static ConvergioEdition g_current_edition = EDITION_MASTER;

void edition_set(ConvergioEdition edition) {
    if (edition == EDITION_EDUCATION) {
        // Reject - Education must use separate binary
        return;
    }
    if (!license_validate(edition)) {
        // Check license for non-Master editions
        return;
    }
    g_current_edition = edition;
    // Reload agent registry
    agent_registry_reload();
}
```

---

## Consequences

### Positive

- Education is secure and auditable
- Professional users have flexibility
- Single codebase for all editions
- Clear upgrade/upsell path

### Negative

- Two distribution channels (Education separate)
- Runtime switching requires licensing system
- More complex testing matrix

### Neutral

- Build system supports both approaches
- Documentation needed for both flows

---

## Build Commands

```bash
# Education (always separate binary)
make EDITION=education
# Output: build/bin/convergio-edu

# Master (default, all agents)
make
make EDITION=master
# Output: build/bin/convergio

# Business/Developer (compile-time for now, runtime later)
make EDITION=business
make EDITION=developer
```

---

## Migration Path

1. **Phase 1 (Current)**: All editions compile-time
2. **Phase 2**: Add runtime switching for Business/Developer
3. **Phase 3**: Add licensing system
4. **Phase 4**: Deprecate separate Business/Developer builds

---

## UI/UX Decisions (2025-12-20 Update)

### ASCII Branding per Edition

Each edition displays a distinctive ASCII banner under the main CONVERGIO logo:

```
CONVERGIO (main logo)
    └── EDUC (Education - green)
    └── BUSIN (Business - blue)
    └── DEVS (Developer - orange)
    └── (none for Master)
```

**Rationale**: Visual differentiation helps users immediately identify which edition they're using. Colors are chosen to match brand identity:
- Education: Green (growth, learning)
- Business: Blue (trust, professionalism)
- Developer: Orange (creativity, energy)

### Edition-Aware Help System

The `/help` command shows different content based on edition:

1. **Education Edition**: Shows 15 Maestri (teachers), study tools, language tools, progress tracking
2. **Business Edition**: Shows business agents, CRM tools, analytics (placeholder)
3. **Developer Edition**: Shows dev agents, code review tools, CI/CD (placeholder)
4. **Master Edition**: Shows all 53 agents and all features

**Command Filtering**:
- `edition_has_command()` validates commands at runtime
- Commands not in edition's whitelist display: "Command 'X' is not available in Convergio Education"
- Help for specific commands (`help quiz`) also checks availability

### Agent Filtering

The `/agents` command respects edition whitelists:
- Only shows agents available in current edition
- Education shows 18 agents (15 Maestri + Ali Principal + Anna + Jenny)
- Master shows all 53+ agents

---

## Related Documents

- [Phase 9: Verticalization](../education-pack/phases/phase-09-verticalization.md)
- [Editions README](../../editions/README.md)
- [Edition System Code](../../src/core/edition.c)
