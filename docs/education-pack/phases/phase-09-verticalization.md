# Phase 9 - Verticalization System

**Status**: ✅ COMPLETE
**Progress**: 100%
**Last Updated**: 2025-12-20
**ADR**: [ADR-003-edition-system-architecture](../../adr/ADR-003-edition-system-architecture.md)

---

## Objective

Create a "vertical editions" system that allows distributing Convergio in specialized versions (Education, Business, Developer) with different agents and features, maintaining a single codebase.

---

## Vision

```
+-------------------------------------------------------------------+
|                    CONVERGIO CORE CODEBASE                         |
|  (kernel, providers, orchestrator, memory, tools, UI, ACP)        |
+-------------------------------------------------------------------+
|                         BUILD SYSTEM                               |
|            make all | make EDITION=education                       |
+-------------+---------------------+-------------------------------+
|  EDUCATION  |      BUSINESS       |        DEVELOPER              |
|  Edition    |      Edition        |        Edition                |
+-------------+---------------------+-------------------------------+
| Agents:     | Agents:             | Agents:                       |
| - Teachers  | - Ali               | - Rex                         |
| - Ali       | - Fabio             | - Paolo                       |
| - Anna      | - Andrea            | - Baccio                      |
| - Jenny     | - Sofia             | - Dario                       |
+-------------+---------------------+-------------------------------+
| Features:   | Features:           | Features:                     |
| - Toolkit   | - CRM               | - Code Review                 |
| - Gradebook | - Pipeline          | - Architecture                |
| - Quiz      | - Analytics         | - CI/CD                       |
+-------------+---------------------+-------------------------------+
```

---

## Tasks

| ID | Task | Status | Priority | Note |
|----|------|--------|----------|------|
| V01 | Edition configuration system | [x] | P0 | Hardcoded in edition.c (no external config needed) |
| V02 | Build flag per edition | [x] | P0 | `make EDITION=education` works |
| V03 | Agent whitelist per edition | [x] | P0 | 20 agents for Education (17 maestri + Ali, Anna, Jenny) |
| V04 | Feature flags per edition | [x] | P0 | edition_has_feature() API |
| V05 | Branding per edition | [x] | P1 | Edition name in header |
| V06 | ACP server per-edition | [ ] | P1 | Future: convergio-acp-edu |
| V07 | Zed extension per-edition | [ ] | P1 | Future: Separate extensions |
| V08 | Installer per-edition | [ ] | P2 | Future: Separate DMG/PKG |
| V09 | Distribution channels | [ ] | P2 | Future: GitHub releases |
| V10 | Edition-specific prompts | [x] | P0 | EDUCATION_SYSTEM_PROMPT defined |

---

## Edition Definitions

### Convergio Education Edition

- **Agents**: ED01-ED17 (Teachers), Ali (principal), Anna (reminders), Jenny (A11y)
- **Features**: Educational toolkit, Gradebook, Quiz, Flashcards, Interactive HTML
- **Target**: Students 6-19, parents, teachers
- **ACP**: `convergio-acp-edu` with school context

### Convergio Business Edition (future)

- **Agents**: Ali, Fabio (Sales), Andrea (Customer Success), Sofia (Marketing)
- **Features**: CRM integration, Sales pipeline, Customer analytics
- **Target**: SMB, startups, sales teams
- **ACP**: `convergio-acp-biz` with business context

### Convergio Developer Edition (future)

- **Agents**: Rex (Code Review), Paolo (Best Practices), Baccio (Architect), Dario (Debugger)
- **Features**: Code analysis, Architecture suggestions, CI/CD integration
- **Target**: Developers, DevOps, Tech Leads
- **ACP**: `convergio-acp-dev` with development context

---

## Implementation Approach

```c
// edition.h
typedef enum {
    EDITION_FULL = 0,        // All agents
    EDITION_EDUCATION = 1,   // Teachers + Education tools
    EDITION_BUSINESS = 2,    // Business agents
    EDITION_DEVELOPER = 3    // Developer agents
} ConvergioEdition;

// Current edition (set at compile time)
extern ConvergioEdition g_current_edition;

// Check if agent is available in current edition
bool edition_has_agent(const char* agent_id);

// Check if feature is available
bool edition_has_feature(const char* feature_id);
```

---

## Zed Integration per Edition

| Edition | ACP Binary | Zed Extension ID | Marketplace |
|---------|------------|------------------|-------------|
| Education | convergio-acp-edu | convergio.education | Zed Extensions |
| Business | convergio-acp-biz | convergio.business | Zed Extensions |
| Developer | convergio-acp-dev | convergio.developer | Zed Extensions |

---

## Files to Create

- `src/edition/edition.h`
- `src/edition/edition.c`
- `editions/education.toml`
- `editions/business.toml`
- `editions/developer.toml`
- `Makefile` (edition targets)

---

## Tests

| ID | Test | Status | Note |
|----|------|--------|------|
| VT01 | Build education edition test | [x] | `make EDITION=education` builds successfully |
| VT02 | Agent whitelist test | [x] | Only 20 education agents shown in /agents |
| VT03 | ACP education test | [ ] | Future: ACP integration |
| VT04 | Feature isolation test | [x] | edition_has_feature() API working |

---

## Acceptance Criteria

- [x] Separate build per edition (`make EDITION=education`)
- [x] Agents filtered correctly (20 education agents)
- [ ] ACP server per-edition (future)
- [ ] Zed extension per-edition (future)
- [ ] Separate installers (future)

---

## Result

**COMPLETE** - Core verticalization system implemented:

- `include/nous/edition.h` - Types and API (EDITION_MASTER as default)
- `src/core/edition.c` - Full implementation with whitelists
- Agent filtering integrated into registry.c
- Edition-specific system prompts defined
- Build with `make EDITION=education` produces filtered binary
- Ali included in ALL editions (Chief of Staff)
- Anna included in ALL editions (Executive Assistant)
- Edition READMEs created in `/editions/` directory

### Architecture Decision

**Hybrid approach adopted** (see ADR-003):
- Education: Always separate binary (security for schools)
- Master/Business/Developer: Will support runtime switching (Phase 2)

### Files Created

- `editions/README.md` - Overview of all editions
- `editions/README-master.md` - Master edition details
- `editions/README-education.md` - Education edition details
- `editions/README-business.md` - Business edition details
- `editions/README-developer.md` - Developer edition details

Future work (P1/P2): Runtime edition switching, licensing system, ACP per-edition, Zed extensions, installers.
