# Phase 9 - Verticalization System

**Status**: TODO
**Progress**: 0%
**Last Updated**: 2025-12-20

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
| V01 | Edition configuration system | [ ] | P0 | JSON/TOML to define contents |
| V02 | Build flag per edition | [ ] | P0 | `make EDITION=education` |
| V03 | Agent whitelist per edition | [ ] | P0 | Only teachers in Education |
| V04 | Feature flags per edition | [ ] | P0 | Toolkit only in Education |
| V05 | Branding per edition | [ ] | P1 | Name, icon, splash |
| V06 | ACP server per-edition | [ ] | P0 | `convergio-acp-edu` |
| V07 | Zed extension per-edition | [ ] | P0 | Separate extensions |
| V08 | Installer per-edition | [ ] | P1 | Separate DMG/PKG |
| V09 | Distribution channels | [ ] | P1 | GitHub releases |
| V10 | Edition-specific prompts | [ ] | P0 | Different system prompts |

---

## Edition Definitions

### Convergio Education Edition

- **Agents**: ED01-ED15 (Teachers), Ali (principal), Anna (reminders), Jenny (A11y)
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
| VT01 | Build education edition test | [ ] | `make EDITION=education` |
| VT02 | Agent whitelist test | [ ] | Only ED01-ED15 |
| VT03 | ACP education test | [ ] | Works in Zed |
| VT04 | Feature isolation test | [ ] | Business features unavailable |

---

## Acceptance Criteria

- [ ] Separate build per edition
- [ ] Agents filtered correctly
- [ ] ACP server per-edition
- [ ] Zed extension per-edition
- [ ] Separate installers

---

## Result

Not yet started. Requires architecture and implementation.
