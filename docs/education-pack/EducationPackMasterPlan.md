# Education Pack - Master Plan

**Created**: 2025-12-19
**Last Updated**: 2025-12-20
**Status**: 12/13 Phases Complete (Phase 13 Localization pending)
**Version**: 2.4 (stubs being replaced with real implementations)
**Branch**: `feature/education-pack`
**Worktree**: `/Users/roberdan/GitHub/ConvergioCLI-education`

---

## Vision

A virtual classroom council with the greatest historical teachers, equipped with advanced educational toolkit, coordinated by Ali (principal), who adapt to each student's specific needs.

**Pedagogical Principles**: Challenging but Achievable | Maieutics | Storytelling | Multimodal | Accessibility

---

## Quick Status

| # | Phase | Status | Progress | Blocking Issues | Link |
|---|-------|:------:|----------|-----------------|------|
| 1 | System Setup | ✅ | 100% | All P0 complete | [phase-01-setup.md](phases/phase-01-setup.md) |
| 2 | 15 Historical Teachers | ✅ | 100% | - | [phase-02-maestri.md](phases/phase-02-maestri.md) |
| 3 | Educational Toolkit | ✅ | 100% P0 | TKT01-06 tests done | [phase-03-toolkit.md](phases/phase-03-toolkit.md) |
| 4 | Italian Curricula | ✅ | 100% | JSON parser working | [phase-04-curriculum.md](phases/phase-04-curriculum.md) |
| 5 | Educational Features | ✅ | 100% | All fixed | [phase-05-features.md](phases/phase-05-features.md) |
| 6 | Accessibility | ✅ | 100% | AT04-09 tests done | [phase-06-accessibility.md](phases/phase-06-accessibility.md) |
| 7 | Coordination | ✅ | 100% | Ali fixed with 15 maestri | [phase-07-coordination.md](phases/phase-07-coordination.md) |
| 8 | Testing | ✅ | 100% | 36 education tests | [phase-08-testing.md](phases/phase-08-testing.md) |
| 9 | Verticalization | ✅ | 100% | Edition system complete | [phase-09-verticalization.md](phases/phase-09-verticalization.md) |
| 10 | Voice Interaction | ✅ | 100% | A11y profile loads | [phase-10-voice.md](phases/phase-10-voice.md) |
| 11 | Learning Science | ✅ | 100% | FSRS + Mastery done | [phase-11-learning-science.md](phases/phase-11-learning-science.md) |
| 12 | Storytelling | ✅ | 100% | All strings in English | [phase-12-storytelling.md](phases/phase-12-storytelling.md) |
| 13 | Localization | ⬜ | 0% | Architecture needed | [phase-13-localization.md](phases/phase-13-localization.md) |

**Legend**: ✅ Done | 🔄 In Progress | ⬜ Not Started

**Build**: Clean (0 errors, 0 warnings) | **Tests**: 132 total test functions (36 education-specific)

---

## Critical Issues (from Codex Audit)

| # | Issue | Status | Priority |
|---|-------|--------|----------|
| 1 | Curriculum parser is stub - doesn't load real JSON | ✅ Fixed | P0 |
| 2 | Wizard shows 15 curricula but only 8 JSON files exist | ✅ Fixed | P0 |
| 3 | Maestro IDs inconsistent (need slug as canonical) | ✅ Fixed | P0 |
| 4 | Ali Preside hardcodes wrong maestro IDs | ✅ Fixed | P0 |
| 5 | Voice mode doesn't load accessibility profile | ✅ Fixed | P1 |
| 6 | /xp persistence is fake | ✅ Fixed | P1 |
| 7 | /video command not implemented | ✅ Fixed | P2 |
| 8 | Story hooks had wrong IDs | ✅ Fixed | - |
| 9 | Italian strings in code | ✅ Fixed | - |

---

## Definition of Done

- [x] 15 teachers operational and tested
- [x] P0 Toolkit complete (mindmaps, quiz, flashcards, audio, calc)
- [x] Setup wizard working
- [x] 8 curricula complete (JSON files)
- [x] All A11y conditions supported (P0)
- [x] Anna integration working
- [x] Ali principal operational
- [x] Voice interaction working with all teachers
- [x] Mastery learning system active (FSRS + 80% threshold)
- [x] All code and strings in English
- [x] Curriculum parser loads real JSON data
- [ ] Test with 5+ real students
- [ ] Feedback >4/5 from users with disabilities

---

## Audit Log

### 2025-12-20 - Phase 9 Verticalization Complete + Architecture Decision

**Edition system implemented:**
- `include/nous/edition.h` - Types and public API (EDITION_MASTER as default)
- `src/core/edition.c` - Full implementation with agent/feature/command whitelists
- Agent filtering integrated into `registry.c`
- Edition-specific system prompts for Education, Business, Developer
- Build with `make EDITION=education` produces filtered binary
- 18 education agents (15 maestri + Ali, Anna, Jenny)
- Ali (Chief of Staff) included in ALL editions
- Anna (Executive Assistant) included in ALL editions
- Translated Italian strings in registry.c to English

**Architecture Decision (ADR-003):**
- Hybrid approach: Education = separate binary (security), others = runtime switching (future)
- See `docs/adr/ADR-003-edition-system-architecture.md`

**Edition Documentation Created:**
- `editions/README.md` - Overview with value propositions
- `editions/README-master.md` - Master edition (all 60+ agents)
- `editions/README-education.md` - Education pack (18 agents)
- `editions/README-business.md` - Business pack (10 agents)
- `editions/README-developer.md` - Developer pack (11 agents)

**Build optimization:**
- ccache support added to Makefile (incremental builds <1 second)

### 2025-12-20 - Full English Translation Complete

**All Italian strings translated to English across the codebase:**
- `education_commands.c` - /study, /homework, /quiz, /flashcards, /mindmap examples; entire /libretto command
- `commands.c` - help text for libretto and mindmap commands
- `setup_wizard.c` - accessibility step, profile sharing messages
- `education_db.c` - LLM error message, default system prompt
- `quiz.c` - grade comments (Excellent, Good work, Passing, Needs review)
- `mastery.c` - skill status labels (Mastered, Proficient, Familiar, In Progress, Not Started)
- `accessibility_runtime.c` - place values, celebration messages, metaphors

**Build optimization added:**
- ccache support for faster rebuilds (incremental builds <1 second)
- Parallel compilation with -j8

**All 9 critical issues from Codex audit now fixed.**

### 2025-12-20 - Stub Elimination Session

**Findings from Codex Analysis**:
1. Curriculum parser was a stub - doesn't parse real JSON subjects/topics
2. Wizard shows 15 curricula but only 8 JSON files exist
3. Maestro IDs inconsistent across system (need slug as canonical)
4. Ali Preside hardcodes wrong maestro IDs
5. Voice mode doesn't load accessibility profile
6. /xp persistence is fake
7. /video command not implemented
8. Story hooks used wrong IDs
9. Many strings still in Italian

**Corrections Applied**:
- Translated all Italian strings to English in storytelling.c
- Fixed story hooks to use correct English slug IDs (socrates-philosophy, etc.)
- Added Humboldt and Chris hooks (was missing)
- Now 15 maestri have story hooks with correct IDs
- Curriculum parser now loads real JSON data

### 2025-12-20 - Brutal Reality Check

**Findings**:
1. Multiple phases marked 100% had incomplete tasks
2. Test count was 253 but actual is 454+
3. Teacher test verifies 14/15 (missing chris-storytelling)
4. Many unit tests marked [ ] in phase files but integration tests pass
5. Compiler warnings present (should be 0)

**Corrections Applied**:
- Phase 1-6 status corrected from ✅ to 🔄
- Test count updated to 454
- Added "Blocking Issues" column to status table
- P0/P1 task counts recalculated

---

## Documents

| Document | Description |
|----------|-------------|
| [Architecture](architecture.md) | System diagrams and structure |
| [Execution Log](execution-log.md) | Event timeline and decisions |
| [Education README](../education/README.md) | Feature overview |
| [Editions Overview](../../editions/README.md) | All editions with value propositions |
| [ADR-001 HTML Generator](../adr/ADR-001-html-generator-llm-approach.md) | LLM vs Templates |
| [ADR-002 Voice](../adr/ADR-002-voice-interaction-architecture.md) | Voice architecture |
| [ADR-003 Edition System](../adr/ADR-003-edition-system-architecture.md) | Edition architecture |
| [Voice Setup](../voice/VOICE_SETUP.md) | Setup guide |

---

## Request Management

### How to track new requests

1. **Classification**: Bug Fix (BF), Enhancement (EN), New Feature (NF)
2. **ID Format**: `REQ-YYYYMMDD-NN` (e.g., REQ-20251220-01)
3. **Tracking**: Add to table below + appropriate phase file

### Active Requests

| ID | Type | Description | Phase | Status |
|----|------|-------------|-------|--------|
| REQ-20251220-16 | NF | Localization system (EN master, IT) | 13 | New |

### Completed Requests

| ID | Type | Description | Phase | Date |
|----|------|-------------|-------|------|
| REQ-20251219-01 | NF | LLM-based Interactive HTML | 3 | 2025-12-19 |
| REQ-20251220-01 | NF | Voice CLI + Audio | 10 | 2025-12-20 |
| REQ-20251220-02 | BF | Fix all compiler warnings | ALL | 2025-12-20 |
| REQ-20251220-03 | BF | Fix teacher test 14->15 | 2 | 2025-12-20 |
| REQ-20251220-04 | EN | Implement S18 Adaptive Learning API | 1 | 2025-12-20 |
| REQ-20251220-05 | EN | Add maieutic/a11y tests (MT02, MT03) | 2 | 2025-12-20 |
| REQ-20251220-06 | EN | Add ITE Commerciale curriculum | 4 | 2025-12-20 |
| REQ-20251220-11 | EN | Voice mode load a11y profile | 10 | 2025-12-20 |
| REQ-20251220-12 | EN | Implement real /xp persistence | 5 | 2025-12-20 |
| REQ-20251220-13 | BF | Fix /video command | 5 | 2025-12-20 |
| REQ-20251220-14 | BF | Translate Italian strings to English | 12 | 2025-12-20 |
| REQ-20251220-15 | BF | Fix story hooks IDs | 12 | 2025-12-20 |
| REQ-20251220-17 | EN | Full codebase English translation | ALL | 2025-12-20 |
| REQ-20251220-18 | EN | Build optimization (ccache) | - | 2025-12-20 |

---

## Instructions

> - Update Quick Status after each completed task
> - Each teacher reads accessibility profile before responding
> - Essential tools (P0) first, nice-to-have (P1/P2) after
> - **VERIFY IT COMPILES WITH 0 WARNINGS** before marking as DONE
> - **RUN TESTS** before marking as DONE
> - Parallelize as much as indicated in phase files
> - **ALL CODE AND STRINGS MUST BE IN ENGLISH**

---

## Stats

- **Total tasks**: 193
- **P0 tasks completed**: 101/101 (100%)
- **P1/P2 tasks completed**: ~85/92 (92%)
- **Max parallel threads**: 10
- **LOC education**: ~12000+
- **Test suites**: 10 (fuzz, unit, anna, compaction, plan_db, output, education, tools, web_search, help)
- **Total test functions**: 132
- **Education tests**: 36
- **Phases completed**: 12/13 (Phase 13 Localization pending)
- **Curricula JSON files**: 8 (in `curricula/it/`)

---

## Roadmap

### Phase 14: Additional Editions (P1)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| E01 | Design Strategy Edition | [ ] | P1 | Domik, Satya, Antonio, Ethan, Evan, Angela |
| E02 | Design Creative Edition | [ ] | P1 | Jony, Sara, Stefano, Riccardo |
| E03 | Design Compliance Edition | [ ] | P2 | Elena, Dr. Enzo, Sophia, Guardian, Luca |
| E04 | Design HR Edition | [ ] | P2 | Giulia, Coach, Dave, Behice |
| E05 | Add whitelists to edition.c | [ ] | P1 | After E01-E04 design |
| E06 | Create README for each new edition | [ ] | P1 | Following existing pattern |

**Proposed Editions:**

| Edition | Agents | Target Audience |
|---------|--------|-----------------|
| Strategy | Ali, Domik, Satya, Antonio, Ethan, Evan, Angela, Anna | C-suite, consultants |
| Creative | Ali, Jony, Sara, Stefano, Riccardo, Anna | Designers, agencies |
| Compliance | Ali, Elena, Dr. Enzo, Sophia, Guardian, Luca, Anna | Legal, healthcare, gov |
| HR | Ali, Giulia, Coach, Dave, Behice, Anna | HR teams, people ops |

### Phase 15: Runtime Edition Switching (P2)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| R01 | Refactor edition selection to runtime | [ ] | P2 | Keep compile-time for Education |
| R02 | Add `--edition` CLI flag | [ ] | P2 | `convergio --edition business` |
| R03 | Add `CONVERGIO_EDITION` env var | [ ] | P2 | Alternative to CLI flag |
| R04 | Add edition to config.toml | [ ] | P2 | Persistent setting |
| R05 | Hot-reload agent registry on switch | [ ] | P2 | Without restart |
| R06 | Update documentation | [ ] | P2 | CLI help, man page |

**Architecture**: See [ADR-003](../adr/ADR-003-edition-system-architecture.md)

### Phase 16: Licensing System (P3)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| L01 | Design license key format | [ ] | P3 | Edition + expiry + features |
| L02 | Implement license validation | [ ] | P3 | Local + optional server check |
| L03 | Add license file storage | [ ] | P3 | `~/.convergio/license.key` |
| L04 | Grace period for expired licenses | [ ] | P3 | 7 days warning |
| L05 | Trial mode implementation | [ ] | P3 | 14 days full access |
| L06 | License UI in setup wizard | [ ] | P3 | Enter/validate key |

### Phase 17: Distribution & Installers (P3)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| D01 | macOS DMG per edition | [ ] | P3 | Convergio-Education-5.3.1.dmg |
| D02 | macOS PKG installer | [ ] | P3 | For enterprise deployment |
| D03 | Homebrew formula | [ ] | P3 | `brew install convergio` |
| D04 | Linux AppImage | [ ] | P3 | Portable Linux binary |
| D05 | GitHub Actions release workflow | [ ] | P3 | Automated per-edition releases |
| D06 | Code signing (macOS) | [ ] | P3 | Apple Developer certificate |

### Phase 18: ACP & IDE Integration (P3)

| ID | Task | Status | Priority | Notes |
|----|------|--------|----------|-------|
| A01 | ACP server per edition | [ ] | P3 | convergio-acp-edu, -biz, -dev |
| A02 | Zed extension per edition | [ ] | P3 | Marketplace submissions |
| A03 | VS Code extension | [ ] | P3 | Alternative to Zed |
| A04 | Edition-aware context in ACP | [ ] | P3 | Only show relevant agents |

---

## Roadmap Timeline (Tentative)

```
2025 Q1: Phase 14 (Additional Editions)
2025 Q2: Phase 15 (Runtime Switching)
2025 Q3: Phase 16 (Licensing) + Phase 17 (Distribution)
2025 Q4: Phase 18 (ACP & IDE Integration)
```

---

*Author: Roberto with AI agent team support*
