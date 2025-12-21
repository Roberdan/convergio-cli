# Workflow-Orchestration Merge Plan

**Last Updated**: 2025-12-21 | **Status**: 🔴 PHASE 2 - SECURITY AUDIT

---

## Dashboard

| Stato | Tasks |
|-------|-------|
| ✅ Completati | 12 |
| ⬚ Da fare | 39 |
| **Totale** | **51** |

```
✅ PHASE 1: MERGE & BUILD ████████████████████ 100% (5/5)
🔴 PHASE 2: SECURITY      ████████░░░░░░░░░░░░  43% (3/7)
🟠 PHASE 3: QUALITY       █░░░░░░░░░░░░░░░░░░░   8% (1/13)
🟡 PHASE 4: DOCS          ██████░░░░░░░░░░░░░░  25% (3/12)
🟢 PHASE 5: REFACTORING   ░░░░░░░░░░░░░░░░░░░░   0% (0/14)
```

---

## ✅ Completati (12 tasks)

| ID | Phase | Task | Data |
|----|-------|------|------|
| ✅ | 1 | Workflow merge PR #72 | 2025-12-21 |
| ✅ | 1 | Education rebase | 2025-12-21 |
| ✅ | 1 | Build EDITION=education | 2025-12-21 |
| ✅ | 1 | All tests passing (39/39) | 2025-12-21 |
| ✅ | 1 | Ali Preside, Accessibility, Libretto verified | 2025-12-21 |
| ✅ | 2 | SQL injection check (0 found) | 2025-12-21 |
| ✅ | 2 | Buffer overflow check (0 found) | 2025-12-21 |
| ✅ | 2 | Safe string functions (1219 confirmed) | 2025-12-21 |
| ✅ | 3 | TODO/FIXME audit (3 real TODOs found) | 2025-12-21 |
| ✅ | 4 | README workflow section | 2025-12-21 |
| ✅ | 4 | Workflow commands documented | 2025-12-21 |
| ✅ | 4 | CHANGELOG v5.4.0 | 2025-12-21 |

---

## ⬚ Da Fare (39 tasks)

| Phase | Tasks | Priority |
|-------|-------|----------|
| 🔴 2 Security | 4 tasks (SEC-01 to SEC-04) | CRITICAL |
| 🟠 3 Quality | 12 tasks (QA-01 to QA-12) | HIGH |
| 🟡 4 Docs | 9 tasks (DOC-01 to DOC-09) | MEDIUM |
| 🟢 5 Refactor | 14 tasks (ARCH + REF) | BACKLOG |

---

## Working Instructions

> **SINGLE SOURCE OF TRUTH**: `/Users/roberdan/GitHub/ConvergioCLI-education/docs/plans/WORKFLOW_MERGE_PLAN.md`

```bash
# Lavora SOLO qui
cd /Users/roberdan/GitHub/ConvergioCLI-education

# Aggiornare da main
git fetch origin main && git merge origin/main

# Test prima di PR
make clean && make EDITION=education && make test && make education_test
```

---

## 🔴 PHASE 2: Security Audit (PRIORITY: CRITICAL)

### Batch 2A - Parallel (3 agents)

| Task ID | Task | Agent | Status |
|---------|------|-------|--------|
| 🔒 SEC-01 | Valutare 15 system() calls per command injection | luca-security-expert | ⬚ |
| 🔒 SEC-02 | Audit secrets management (API keys, tokens, .env) | luca-security-expert | ⬚ |
| 🔒 SEC-03 | Review dipendenze per CVE (check package versions) | luca-security-expert | ⬚ |

```bash
# SEC-01: Find system() calls
rg "system\s*\(" /Users/roberdan/GitHub/ConvergioCLI-education/src/ -n

# SEC-02: Find potential secrets
rg -i "(api.?key|token|secret|password|credential)" /Users/roberdan/GitHub/ConvergioCLI-education/src/ -n

# SEC-03: Check dependencies
cat /Users/roberdan/GitHub/ConvergioCLI-education/Package.swift 2>/dev/null || echo "No Swift deps"
```

### Batch 2B - Sequential (requires 2A)

| Task ID | Task | Agent | Status |
|---------|------|-------|--------|
| 🔒 SEC-04 | OWASP Top 10 analysis con cppcheck/clang-tidy | luca-security-expert | ⬚ |

```bash
# Run static analysis
cd /Users/roberdan/GitHub/ConvergioCLI-education
cppcheck --enable=all --error-exitcode=1 src/ 2>&1 | head -100
```

---

## 🟠 PHASE 3: Code Quality (PRIORITY: HIGH)

### Batch 3A - Parallel (4 agents, independent scans)

| Task ID | Task | Command | Status |
|---------|------|---------|--------|
| 🧪 QA-01 | Funzioni >100 righe | `wc -l src/**/*.c \| awk '$1>100'` | ⬚ |
| 🧪 QA-02 | Complessita ciclomatica | `pmccabe src/**/*.c \| sort -rn \| head -20` | ⬚ |
| 🧪 QA-03 | Magic numbers | `rg "[^0-9][0-9]{3,}[^0-9]" src/ --type c` | ⬚ |
| 🧪 QA-04 | Naming conventions | `rg "^[a-z]+_[a-z]+" src/ -o \| sort \| uniq -c \| sort -rn` | ⬚ |

### Batch 3B - Parallel (2 agents, heavy analysis)

| Task ID | Task | Command | Status |
|---------|------|---------|--------|
| 🧪 QA-05 | Test coverage | `make coverage && lcov ...` | ⬚ |
| 🧪 QA-06 | Codice duplicato | `jscpd src/ --min-lines 10` | ⬚ |

### Batch 3C - Memory (sequential, needs build)

| Task ID | Task | Command | Status |
|---------|------|---------|--------|
| 🧪 QA-07 | Valgrind leak check | `valgrind --leak-check=full ./build/bin/convergio-edu --help` | ⬚ |
| 🧪 QA-08 | macOS leaks | `leaks --atExit -- ./build/bin/convergio-edu --help` | ⬚ |
| 🧪 QA-09 | AddressSanitizer build | `make clean && CFLAGS="-fsanitize=address" make` | ⬚ |

### Batch 3D - Error handling (sequential review)

| Task ID | Task | Focus | Status |
|---------|------|-------|--------|
| 🧪 QA-10 | Error handling consistente | Review error paths in src/workflow/ | ⬚ |
| 🧪 QA-11 | Deallocazione in error paths | Check free() on all error returns | ⬚ |
| 🧪 QA-12 | Graceful shutdown | Verify cleanup in signal handlers | ⬚ |

---

## 🟡 PHASE 4: Documentation (PRIORITY: MEDIUM)

### Batch 4A - Parallel (3 agents, independent docs)

| Task ID | Task | File | Status |
|---------|------|------|--------|
| 📄 DOC-01 | Quality Standards section | README.md | ⬚ |
| 📄 DOC-02 | Editions section update | README.md | ⬚ |
| 📄 DOC-03 | What's New in v5.5 page | docs/WHATS_NEW_v5.5.md | ⬚ |

### Batch 4B - Parallel (cleanup)

| Task ID | Task | Action | Status |
|---------|------|--------|--------|
| 📄 DOC-04 | Unificare docs/workflow-orchestration/ | Move to docs/ | ⬚ |
| 📄 DOC-05 | Rimuovere ADR duplicati | Delete workflow-orchestration/ADR | ⬚ |

### Batch 4C - Sequential (verification)

| Task ID | Task | Check | Status |
|---------|------|-------|--------|
| 📄 DOC-06 | Doxygen headers | All new .h files | ⬚ |
| 📄 DOC-07 | Man page update | man/convergio.1 | ⬚ |
| 📄 DOC-08 | --help output verification | Run and check | ⬚ |
| 📄 DOC-09 | ADR linkage | Check all ADR are linked | ⬚ |

---

## 🟢 PHASE 5: Refactoring (PRIORITY: LOW - BACKLOG)

> Questi sono miglioramenti futuri, non bloccanti per il merge.

### Architecture Review (before refactoring)

| Task ID | Task | Status |
|---------|------|--------|
| 🔧 ARCH-01 | Separazione concerns tra layers | ⬚ |
| 🔧 ARCH-02 | Dipendenze cicliche tra moduli | ⬚ |
| 🔧 ARCH-03 | Accoppiamento componenti | ⬚ |
| 🔧 ARCH-04 | Edition filtering consistente | ⬚ |
| 🔧 ARCH-05 | Thread safety | ⬚ |
| 🔧 ARCH-06 | Recovery errori critici | ⬚ |

### Refactoring Opportunities (future sprints)

| Task ID | Task | Effort | Status |
|---------|------|--------|--------|
| 🔧 REF-01 | Registry Pattern unificato | Medium | ⬚ |
| 🔧 REF-02 | Error Handling centralizzato | Medium | ⬚ |
| 🔧 REF-03 | Config Loading consolidato | Low | ⬚ |
| 🔧 REF-04 | Telemetry unificato | Medium | ⬚ |
| 🔧 REF-05 | Logging standardizzato | Low | ⬚ |
| 🔧 REF-06 | Test infrastructure | Medium | ⬚ |
| 🔧 REF-07 | Build system cleanup | Low | ⬚ |
| 🔧 REF-08 | Caching agent definitions | Low | ⬚ |

---

## Known TODOs in Code

| File | Line | Description |
|------|------|-------------|
| `src/orchestrator/workflow_integration.c` | 144 | Parse plan_output |
| `src/memory/persistence.c` | 230 | Manager tables per Anna |
| `src/education/anna_integration.c` | 730 | Session tracking |

---

## Execution Parallelization Map

```
TIME ──────────────────────────────────────────────────────►

PHASE 2A: [SEC-01]──┐
          [SEC-02]──┼──► PHASE 2B: [SEC-04]
          [SEC-03]──┘

PHASE 3A: [QA-01]──┐
          [QA-02]──┤
          [QA-03]──┼──► PHASE 3C: [QA-07]──[QA-08]──[QA-09]
          [QA-04]──┤              │
                   │              ▼
PHASE 3B: [QA-05]──┤    PHASE 3D: [QA-10]──[QA-11]──[QA-12]
          [QA-06]──┘

PHASE 4A: [DOC-01]──┐
          [DOC-02]──┼──► PHASE 4C: [DOC-06]──[DOC-07]──[DOC-08]──[DOC-09]
          [DOC-03]──┤
                    │
PHASE 4B: [DOC-04]──┤
          [DOC-05]──┘

(PHASE 5 is backlog - execute only after merge)
```

---

## Quick Commands Reference

```bash
# Security scans (run in parallel)
rg "system\s*\(" src/ -n &
rg -i "api.?key|token|secret" src/ -n &
cppcheck --enable=all src/ 2>&1 | head -50 &
wait

# Quality scans (run in parallel)
pmccabe src/**/*.c 2>/dev/null | sort -rn | head -20 &
rg "[^0-9][0-9]{4,}[^0-9]" src/ --type c &
wait

# Memory analysis
valgrind --leak-check=full ./build/bin/convergio-edu --help 2>&1 | tail -30
leaks --atExit -- ./build/bin/convergio-edu --help 2>&1 | tail -20
```

---

## Rollback Plan

```bash
# Revert merge on main
cd /Users/roberdan/GitHub/ConvergioCLI
git checkout main
git revert -m 1 <merge-commit-hash>
git push origin main
```

---

## Notes

1. **NEVER squash commits** - Preserve full history
2. **NEVER force push to main**
3. **Always run tests** before pushing
4. **Update this dashboard** after completing tasks

---

## Legenda

| Icona | Significato |
|-------|-------------|
| ✅ | Completato |
| ⏳ | In corso |
| ⬚ | Da fare |
| 🔴 | CRITICAL priority |
| 🟠 | HIGH priority |
| 🟡 | MEDIUM priority |
| 🟢 | LOW/BACKLOG |
| 🔒 | Security task |
| 🧪 | Quality/Test task |
| 📄 | Documentation task |
| 🔧 | Refactoring task |

---

**Document Owner**: Roberto
**Single Authoritative Copy**: `/Users/roberdan/GitHub/ConvergioCLI-education/docs/plans/WORKFLOW_MERGE_PLAN.md`
