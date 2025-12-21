# Workflow-Orchestration Merge Plan

**Created**: 2025-12-21
**Status**: WORKFLOW MERGED - EDUCATION IN PROGRESS
**Priority**: HIGH - Education pack merge pending
**Last Updated**: 2025-12-21 (aggiornato con stato reale)
**Location**: QUESTA E' L'UNICA COPIA AUTORITATIVA (in education worktree)

---

## IMPORTANT: Working Instructions

> **SINGLE SOURCE OF TRUTH**: Questo file in `/Users/roberdan/GitHub/ConvergioCLI-education/docs/plans/WORKFLOW_MERGE_PLAN.md` e' l'UNICA copia autoritativa del piano.

### Come lavorare:

1. **Lavora SOLO nel worktree education** (`/Users/roberdan/GitHub/ConvergioCLI-education`)
2. **Quando serve aggiornare da main**:
   ```bash
   cd /Users/roberdan/GitHub/ConvergioCLI-education
   git fetch origin main
   git merge origin/main  # Risolvi eventuali conflitti
   ```
3. **Fai tutte le modifiche qui** - codice, documentazione, test
4. **Testa tutto prima di proporre merge a main**:
   ```bash
   make clean && make EDITION=education
   make test
   make education_test
   ```
5. **Solo quando tutto funziona al 100%**, crea PR per merge a main

---

## Executive Summary

The `feature/workflow-orchestration` branch introduces a comprehensive workflow engine (~157 files, +32K lines) but was developed **before** PR #73 (runtime edition switching). This creates a critical conflict: the workflow branch **DELETES** the edition system.

**Goal**: Merge workflow -> main while preserving the edition system, then rebase education-pack.

---

## Current State

### Branches & Worktrees

| Worktree | Branch | Status |
|----------|--------|--------|
| `/Users/roberdan/GitHub/ConvergioCLI` | main | Has workflow + edition (PR #72 merged) |
| `/Users/roberdan/GitHub/ConvergioCLI-workflow` | feature/workflow-orchestration | MERGED to main |
| `/Users/roberdan/GitHub/ConvergioCLI-education` | feature/education-pack | IN PROGRESS - rebased on main, Makefile conflict resolved |

### Critical Conflicts (RESOLVED)

| File | Main (after PR #73) | Workflow Branch | Resolution |
|------|---------------------|-----------------|------------|
| `include/nous/edition.h` | Full edition API | DELETED | Restore from main |
| `src/core/edition.c` | Runtime switching | DELETED | Restore from main |
| `src/core/main.c` | Has `--edition` flag | No edition flag | Merge carefully |
| `Makefile` | EDITION support | Different structure | Manual merge - DONE |

---

## Verification Checklist

### After Workflow Merge (PR #72 - COMPLETED 2025-12-21)

- [x] `make clean && make` passes (verified during PR #72)
- [x] `make EDITION=education` passes
- [x] `make EDITION=business` passes
- [x] `make test` passes (all tests)
- [x] `make workflow_test` passes
- [x] `./build/bin/convergio --version` shows correct version
- [x] `./build/bin/convergio --edition business` works
- [x] `./build/bin/convergio-edu` locked to education (if built with EDITION=education)
- [x] Quality gates pass: `make quality_gate`

### After Education Rebase (IN PROGRESS)

- [ ] `make EDITION=education` passes - DA VERIFICARE
- [ ] `make education_test` passes - DA VERIFICARE
- [ ] Ali Preside agent works - DA VERIFICARE
- [ ] Education agents load correctly - DA VERIFICARE
- [ ] Accessibility runtime works - DA VERIFICARE
- [ ] Libretto feature works - DA VERIFICARE

---

## Post-Merge Code Analysis

> **STATUS**: GREP-BASED CHECK DONE (vedi CODE_ANALYSIS_REPORT.md in questo worktree)
> **NOTE**: Solo pattern matching, NON e' un audit completo

### Security Audit (grep-based preliminary)

- [x] SQL injection patterns: 0 trovati nel codice (solo in docs)
- [x] Buffer overflow (strcpy, sprintf): 0 trovati
- [x] Safe string functions: 1219 usi confermati
- [ ] Command injection: 15 system() calls - DA VALUTARE MANUALMENTE
- [ ] Analisi OWASP Top 10 con strumenti dedicati - NON FATTO
- [ ] Audit secrets management (API keys, tokens) - NON FATTO
- [ ] Review delle dipendenze per CVE note - NON FATTO

### Code Quality & Technical Debt

- [ ] Identificare codice duplicato tra moduli
- [ ] Verificare consistenza naming conventions
- [ ] Controllare complessita' ciclomatica delle funzioni
- [ ] Identificare funzioni troppo lunghe (>100 righe)
- [ ] Verificare error handling consistente
- [ ] Controllare coverage dei test (<80% = technical debt)
- [ ] Identificare magic numbers e stringhe hardcoded
- [x] Verificare tutti i TODO/FIXME nel codice
  - 3 TODOs reali trovati (102 occorrenze erano nomi variabili)
  - `src/orchestrator/workflow_integration.c:144` - Parse plan_output
  - `src/memory/persistence.c:230` - Manager tables per Anna
  - `src/education/anna_integration.c:730` - Session tracking

### Memory & Performance

- [ ] Eseguire Valgrind/AddressSanitizer su tutto il codebase
- [ ] Profiling con Instruments (macOS) per memory leaks
- [ ] Analisi performance hot paths (workflow engine, LLM calls)
- [ ] Verificare corretta deallocazione in tutti i percorsi di errore
- [ ] Controllare uso efficiente delle strutture dati
- [ ] Analisi latenza nelle chiamate API
- [ ] Verificare caching appropriato (agent definitions, config)

### Refactoring Opportunities

- [ ] **Registry Pattern**: Unificare agent/command/tool registries
- [ ] **Error Handling**: Centralizzare in `src/core/error_interpreter.c`
- [ ] **Config Loading**: Consolidare parsing config da varie fonti
- [ ] **Telemetry**: Unificare event tracking tra moduli
- [ ] **Logging**: Standardizzare livelli e formati log
- [ ] **Test Infrastructure**: Consolidare test utilities e mocks
- [ ] **Build System**: Pulire Makefile/CMakeLists duplicazioni

### Architecture Review

- [ ] Verificare separazione concerns tra layers
- [ ] Controllare dipendenze cicliche tra moduli
- [ ] Analizzare accoppiamento tra componenti
- [ ] Verificare che edition filtering sia consistente ovunque
- [ ] Controllare thread safety in codice concorrente
- [ ] Verificare gestione graceful shutdown
- [ ] Analizzare recovery da errori critici

### Specific Areas to Audit

| Area | Files | Focus |
|------|-------|-------|
| Workflow Engine | `src/workflow/*.c` | State machine correctness, memory |
| Checkpoint System | `checkpoint*.c` | Data integrity, recovery |
| LLM Integration | `src/providers/*.c` | Rate limiting, error handling |
| Agent Registry | `src/orchestrator/registry.c` | Thread safety, reload logic |
| Memory/DB | `src/memory/*.c` | SQL injection, data validation |
| Voice | `src/voice/*.c` | WebSocket handling, audio buffers |
| Education Tools | `src/education/tools/*.c` | Input sanitization |

### Tools to Use

```bash
# Static analysis
cppcheck --enable=all src/
clang-tidy src/**/*.c

# Memory analysis
valgrind --leak-check=full ./build/bin/convergio
leaks --atExit -- ./build/bin/convergio

# Coverage
make coverage
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# Complexity analysis
pmccabe src/**/*.c | sort -n -r | head -20

# Find TODOs and FIXMEs
rg -n "TODO|FIXME|XXX|HACK" src/
```

### Report Template

Dopo l'analisi, creare report in `docs/CODE_ANALYSIS_REPORT.md`:

```markdown
# Code Analysis Report - Post Workflow Merge

**Date**: YYYY-MM-DD
**Analyzer**: [Nome]

## Executive Summary
- Critical issues: X
- High priority: X
- Medium priority: X
- Low priority: X

## Critical Issues (Fix Immediately)
1. [Issue description, file, line]

## High Priority (Fix Before Release)
1. [Issue description]

## Medium Priority (Fix in Next Sprint)
1. [Issue description]

## Low Priority (Backlog)
1. [Issue description]

## Optimization Opportunities
1. [Opportunity description, estimated impact]

## Refactoring Recommendations
1. [Recommendation, effort estimate]
```

---

## Post-Merge Documentation Tasks

> **STATUS**: IN PROGRESS (working in education worktree)

### README.md Update

- [x] Aggiungere sezione **Workflow Engine** con features principali (GIA' FATTO in v5.4.0)
- [x] Documentare i nuovi comandi `/workflow list`, `/workflow execute`, `/workflow resume` (GIA' FATTO)
- [x] Aggiornare lista features con checkpointing, task decomposition, group chat (GIA' FATTO)
- [x] Documentare i 9 workflow templates disponibili (GIA' FATTO)
- [x] Verificare che tutti i badge/shields siano aggiornati (v5.4.0)
- [ ] Aggiungere sezione **Quality Standards** (Zero Tolerance Policy)
- [ ] Aggiornare sezione **Editions** con tutte le edition supportate

### Documentation Cleanup

- [ ] Rimuovere documentazione duplicata tra workflow e main (docs/workflow-orchestration/ADR vs docs/adr)
- [ ] Unificare docs/workflow-orchestration/ con docs/ principale
- [x] Aggiornare CHANGELOG.md con tutte le nuove features (v5.4.0 gia' documentato)
- [ ] Verificare che ADR siano tutti aggiornati e linkati
- [ ] Creare una pagina "What's New in v5.5" (o versione appropriata)

### Code Documentation

- [ ] Verificare che tutti i nuovi header abbiano documentazione Doxygen
- [ ] Aggiornare man page se presente
- [ ] Verificare `--help` output include nuovi comandi

---

## Notes

1. **NEVER squash commits** - Preserve full history
2. **NEVER force push to main** - Unless absolutely necessary with team agreement
3. **Always run tests** before pushing
4. **Create checkpoint tags** before risky operations: `git tag pre-workflow-merge`

---

## Rollback Plan

If merge fails or causes issues:

```bash
# 1. Revert the merge commit on main
cd /Users/roberdan/GitHub/ConvergioCLI
git checkout main
git revert -m 1 <merge-commit-hash>
git push origin main

# 2. Or hard reset (DANGEROUS - only if no one else pulled)
git reset --hard <commit-before-merge>
git push origin main --force-with-lease
```

---

## Key Files Reference

### Workflow Branch New Files (now on main)

```
src/workflow/
├── workflow_engine.c      # Core execution (551 lines)
├── workflow_types.c       # Data structures (394 lines)
├── task_decomposer.c      # LLM task breakdown (790 lines)
├── group_chat.c           # Multi-agent chat (294 lines)
├── router.c               # Conditional routing (195 lines)
├── patterns.c             # Reusable patterns (154 lines)
├── error_handling.c       # Error recovery (468 lines)
├── checkpoint.c           # Persistence (310 lines)
├── checkpoint_optimization.c
├── retry.c                # Retry logic (186 lines)
├── workflow_observability.c # Logging/telemetry (348 lines)
├── workflow_visualization.c # Mermaid export (308 lines)
└── templates/
    ├── class_council.json    # Education workflow!
    ├── code_review.json
    ├── incident_response.json
    └── ... (9 total)
```

### Education-Specific Files

```
src/education/
├── tools/                # Education-specific tools
├── anna_integration.c    # Anna assistant integration
├── accessibility.c       # Accessibility runtime
└── libretto.c            # Libretto feature
```

---

**Document Owner**: Roberto
**Last Updated**: 2025-12-21
**Single Authoritative Copy**: /Users/roberdan/GitHub/ConvergioCLI-education/docs/plans/WORKFLOW_MERGE_PLAN.md
