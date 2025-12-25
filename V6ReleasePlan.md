# CONVERGIO V6 RELEASE PLAN

**Version**: 6.0.0
**Date**: 2025-12-25
**Author**: AI Team with Roberto
**Status**: PLANNING

---

## EXECUTIVE SUMMARY

This plan consolidates **4 feature branches** into a stable V6 release using a **development staging branch**. The goal is to merge all work without losing any features while maintaining maximum quality.

### Current State Overview

| Branch | Location | Commits Ahead | C Code Changes | CI Status | Merge Order |
|--------|----------|---------------|----------------|-----------|-------------|
| `main` | Root | - | - | PASSING | Base |
| `feature/convergio-enhancements` | ConvergioCLI-features | 4 | Minimal | Unknown | **1st** (easy) |
| `feature/education-pack` | ConvergioCLI-education | 538 | **175 files, 91K lines** | FIXED | **2nd** (core CLI) |
| `feature/scuola-2026` | native-scuola-2026 | 307 | 1 file (swift_bridge) | Unknown | **3rd** (native app) |
| `feature/native-app` | ConvergioNative | 283 | 1 file | Unknown | **4th** (cherry-pick ProviderManager) |

**IMPORTANT**: education-pack must be merged BEFORE scuola-2026 because:
- education-pack has the core CLI changes (edition system, education features, 175 C files)
- scuola-2026 is the Swift native app that depends on the CLI

---

## 📋 EXECUTION CHECKLIST (Quick Overview)

> Usa questa tabella per tracciare l'avanzamento a colpo d'occhio.

### Phase 0: Preparation (30 min)
| # | Task | Status |
|---|------|--------|
| 0.1 | Create `development` branch from main | [ ] |
| 0.2 | Create backup branches for all features | [ ] |
| 0.3 | Verify all worktrees clean, gh auth OK | [ ] |
| 0.4 | Review PRs: close #71, defer #69 to V7 | [ ] |

### Phase 1: Merge Enhancements (1 hour)
| # | Task | Status |
|---|------|--------|
| 1.1 | Merge `feature/convergio-enhancements` to development | [ ] |
| 1.2 | Build & test pass | [ ] |
| 1.3 | Push to remote | [ ] |

### Phase 2: Merge Education-Pack (8-16 hours) ⚠️ CRITICAL
| # | Task | Status |
|---|------|--------|
| 2.1 | Verify education-pack CI fixed (153695f) | [x] |
| 2.2 | Rebase/merge education-pack to development | [ ] |
| 2.3 | Resolve ~100 conflicts (see Phase 5 guide) | [ ] |
| 2.4 | Build & all 499+ tests pass | [ ] |
| 2.5 | Verify Azure-only for EDU edition | [ ] |
| 2.6 | Push to remote | [ ] |

### Phase 3: Merge Scuola-2026 (2-3 hours)
| # | Task | Status |
|---|------|--------|
| 3.1 | Merge `feature/scuola-2026` to development | [ ] |
| 3.2 | Resolve Makefile/CI conflicts | [ ] |
| 3.3 | Verify 17 maestri images present | [ ] |
| 3.4 | Verify native app builds (`make native`) | [ ] |
| 3.5 | Push to remote | [ ] |

### Phase 4: Cherry-pick Native-App (30 min)
| # | Task | Status |
|---|------|--------|
| 4.1 | Cherry-pick ProviderManager from `b6382b2` | [ ] |
| 4.2 | Verify ProviderManager.swift present | [ ] |
| 4.3 | Build & test pass | [ ] |
| 4.4 | Archive native-app branch with tag | [ ] |

### Phase 6: Quality Gates (4-8 hours)
| # | Task | Status |
|---|------|--------|
| 6.1 | Fix HP-1: Remove shell=True subprocess | [ ] |
| 6.2 | Fix HP-2: Replace random with secrets | [ ] |
| 6.3 | Create .editorconfig, .clang-format | [ ] |
| 6.4 | Update README.md, CHANGELOG.md | [ ] |
| 6.5 | Full test suite: 100% pass, 0 warnings | [ ] |

### Phase 7: Release Preparation (2-4 hours)
| # | Task | Status |
|---|------|--------|
| 7.1 | Bump version to 6.0.0 everywhere | [ ] |
| 7.2 | Create PR: development → main | [ ] |
| 7.3 | Wait for CI, get approvals | [ ] |
| 7.4 | Merge to main (merge commit, NOT squash) | [ ] |
| 7.5 | Tag v6.0.0 and push | [ ] |
| 7.6 | Create GitHub Release | [ ] |

### Phase 8: Cleanup (1 hour)
| # | Task | Status |
|---|------|--------|
| 8.1 | Remove all worktrees | [ ] |
| 8.2 | Delete local merged branches | [ ] |
| 8.3 | Close PR #71 with comment | [ ] |
| 8.4 | Move V6ReleasePlan.md to docs/releases/ | [ ] |

### Phase 9: Final Review (4-8 hours) ⚠️ MANDATORY
| # | Task | Status |
|---|------|--------|
| 9.1 | Verify Ollama `qwen2.5:0.5b` working | [x] |
| 9.2 | Zero TODO/FIXME in production code | [ ] |
| 9.3 | Zero Anthropic references in EDU code | [ ] |
| 9.4 | App release manager check passed | [ ] |
| 9.5 | Website updated for V6 | [ ] |
| 9.6 | README.md complete and English | [ ] |
| 9.7 | All tests pass with Ollama local | [ ] |
| 9.8 | EDU tests pass with Azure | [ ] |
| 9.9 | Version 6.0.0 consistent everywhere | [ ] |
| 9.10 | Final sign-off by Roberto | [ ] |

### Summary
| Phase | Tasks | Effort | Status |
|-------|-------|--------|--------|
| 0 | 4 | 30 min | [ ] |
| 1 | 3 | 1 hour | [ ] |
| 2 | 6 | 8-16 hours | [ ] |
| 3 | 5 | 2-3 hours | [ ] |
| 4 | 4 | 30 min | [ ] |
| 6 | 5 | 4-8 hours | [ ] |
| 7 | 6 | 2-4 hours | [ ] |
| 8 | 4 | 1 hour | [ ] |
| 9 | 10 | 4-8 hours | [ ] |
| **TOTAL** | **47** | **24-44 hours** | **[ ]** |

---

## 🧪 TESTING STRATEGY: Ollama Local vs Production Providers

### Obiettivo
Separare chiaramente i **test di sviluppo/integrazione** dai **test di produzione**:

| Fase | Provider | Modello | Scopo |
|------|----------|---------|-------|
| **Sviluppo & CI** | Ollama (locale) | `qwen2.5:0.5b` (397MB) | Verificare che il codice funzioni |
| **Pre-Release EDU** | Azure OpenAI | `gpt-4o` via Azure | Verificare GDPR compliance |
| **Produzione EDU** | Azure OpenAI ONLY | `gpt-4o` via Azure | GDPR compliance scuole |
| **Produzione FULL** | Multi-provider | Anthropic/OpenAI/Azure | Flessibilità utente |

> ⚠️ **IMPORTANTE**: Usare SOLO `qwen2.5:0.5b` locale per i test, MAI il modello cloud che ha limitazioni.

### Perché Ollama per i Test di Sviluppo?

1. **Zero costi API** - Nessun consumo di crediti durante sviluppo
2. **Velocità** - Modelli piccoli rispondono in millisecondi
3. **Offline** - Funziona senza connessione internet
4. **Riproducibilità** - Stesso modello = stesso comportamento
5. **CI/CD friendly** - Può girare in GitHub Actions con self-hosted runner

### Setup Ollama per Test

```bash
# Installare Ollama (se non già presente)
brew install ollama

# Scaricare modello locale leggero (USARE SOLO QUESTO!)
ollama pull qwen2.5:0.5b    # 397MB, velocissimo, locale

# Verificare che Ollama sia attivo
ollama list
curl http://localhost:11434/api/tags

# Test rapido
echo '{"model": "qwen2.5:0.5b", "prompt": "Hello", "stream": false}' | \
  curl -s http://localhost:11434/api/generate -d @-
```

> ⚠️ NON usare modelli cloud (es. gpt-oss:120b-cloud) per i test - hanno limitazioni!

### Configurazione Test con Ollama

```bash
# Per i test di sviluppo, impostare:
export CONVERGIO_TEST_PROVIDER=ollama
export CONVERGIO_TEST_MODEL=qwen2.5:0.5b
export OLLAMA_HOST=http://localhost:11434

# Eseguire test
make test                    # Unit test (no LLM)
make test_llm PROVIDER=ollama  # Test LLM con Ollama
make test_e2e PROVIDER=ollama  # E2E con Ollama
```

### Configurazione Pre-Release (Provider Reali)

```bash
# Prima del release, verificare con provider reali:
export CONVERGIO_TEST_PROVIDER=azure  # Per EDU
export AZURE_OPENAI_ENDPOINT=https://xxx.openai.azure.com
export AZURE_OPENAI_KEY=xxx
export AZURE_OPENAI_DEPLOYMENT=gpt-4o

# Test con Azure
make test_llm PROVIDER=azure
make test_e2e PROVIDER=azure
```

### ⚠️ Regola Fondamentale per Education Edition

```
╔══════════════════════════════════════════════════════════════════╗
║  EDUCATION EDITION IN PRODUZIONE = SOLO AZURE OPENAI             ║
║                                                                   ║
║  • Ollama OK per TEST di sviluppo                                ║
║  • Azure OBBLIGATORIO per RELEASE e PRODUZIONE                   ║
║  • MAI Anthropic/OpenAI diretto in EDU                           ║
║  • GDPR richiede data residency EU (Azure West Europe)           ║
╚══════════════════════════════════════════════════════════════════╝
```

### Implementazione nel Codice

Il provider selector deve rispettare questa logica:

```c
// In src/providers/provider_selector.c
Provider* select_provider(ConvergioEdition edition, bool is_test_mode) {
    // In test mode, allow Ollama for any edition
    if (is_test_mode && getenv("CONVERGIO_TEST_PROVIDER")) {
        return provider_from_env("CONVERGIO_TEST_PROVIDER");
    }

    // In production, EDU edition MUST use Azure
    if (edition == EDITION_EDUCATION) {
        return provider_azure_openai();  // ONLY Azure for EDU
    }

    // Other editions can use configured provider
    return provider_from_config();
}
```

### Checklist Test Pre-Release

- [ ] `make test` passa (unit test, no LLM)
- [ ] `make test_llm PROVIDER=ollama` passa (verifica funzionamento base)
- [ ] `make test_e2e PROVIDER=ollama` passa (verifica flusso completo)
- [ ] `make test_llm PROVIDER=azure` passa (verifica Azure per EDU)
- [ ] Nessun riferimento hardcoded a Anthropic in codice EDU
- [ ] Provider selector rispetta edition restrictions

---

## PHASE 0: PREPARATION
**Status**: [ ] NOT STARTED

### 0.1 Create Development Branch
```bash
git checkout main
git pull origin main
git checkout -b development
git push -u origin development
```
- [ ] Create `development` branch from `main`
- [ ] Push to remote
- [ ] Protect branch (require PR reviews)

### 0.2 Backup All Branches
```bash
git branch backup-main-pre-v6 main
git branch backup-education-pack-pre-v6 feature/education-pack
git branch backup-scuola-2026-pre-v6 feature/scuola-2026
git branch backup-native-app-pre-v6 feature/native-app
git branch backup-enhancements-pre-v6 feature/convergio-enhancements
```
- [ ] Create backup branches for all feature branches
- [ ] Verify backups exist: `git branch -a | grep backup`

### 0.3 Pre-Flight Checks
- [ ] All worktrees accessible and clean
- [ ] No uncommitted changes in any worktree
- [ ] GitHub CLI authenticated: `gh auth status`
- [ ] Test suite available: `make test`

### 0.4 GitHub PR Review & Cleanup
**Run**: `scripts/v6-release/04-pr-review.sh`

**IMPORTANT**: Since we're merging all branches via `development` branch (not PRs), existing PRs become obsolete.

**Current Open/Draft PRs (as of 2025-12-25):**

| # | Title | Branch | Status | Action |
|---|-------|--------|--------|--------|
| #71 | Education Pack Multi-Edition System | feature/education-pack | OPEN | **CLOSE with comment** - merged via development |
| #69 | Zed Integration Phases 1-10 | feature/acp-zed-integration | DRAFT | **DEFER to V7** - keep for next version |

**PR Cleanup Commands:**
```bash
# Close PR #71 with explanation
gh pr close 71 --comment "Merged via development branch as part of V6 release. See commit history in development -> main."

# Keep PR #69 open for V7 - just add a comment
gh pr comment 69 --body "Deferred to V7 release. Keeping open for future integration."
```

**Actions Required:**
- [ ] Close PR #71 after education-pack merge (Phase 5)
- [ ] Add V7 milestone label to PR #69 (Zed Integration)
- [ ] Delete merged feature branches from remote
- [ ] Create new V6 release PR from development -> main (Phase 7)

---

## PHASE 1: MERGE CONVERGIO-ENHANCEMENTS
**Status**: [ ] NOT STARTED
**Risk**: LOW (0 conflicts)
**Estimate**: 1 hour

### Features to Preserve
- Website implementation (21 files)
- Voice history functions
- Apple Foundation Models Swift bridge

### 1.1 Merge to Development
```bash
git checkout development
git merge feature/convergio-enhancements --no-ff -m "Merge feature/convergio-enhancements: website, voice history, Foundation Models"
```
- [ ] Merge enhancements branch
- [ ] Verify no conflicts

### 1.2 Validation
- [ ] Build passes: `make clean && make`
- [ ] Tests pass: `make test`
- [ ] Website files present: `ls website/`
- [ ] Voice history code present: `ls src/voice/` or equivalent

### 1.3 Commit and Push
```bash
git push origin development
```
- [ ] Push to remote
- [ ] Create PR for review: `gh pr create --base main --head development --title "V6: Phase 1 - Convergio Enhancements"`
- [ ] Wait for CI to pass
- [ ] DO NOT merge to main yet

---

## PHASE 2: MERGE EDUCATION-PACK (CORE CLI - DO THIS BEFORE SCUOLA!)
**Status**: [ ] NOT STARTED
**Risk**: HIGH (~100 conflicts, 175 C files, 91K+ lines)
**Estimate**: 8-16 hours
**Strategy**: INCREMENTAL MERGE

> ⚠️ **IMPORTANT**: This phase was previously Phase 5, but MUST be done BEFORE scuola-2026
> because education-pack contains the core CLI changes that the native app depends on.

See section "PHASE 5 (LEGACY): EDUCATION-PACK DETAILS" below for full conflict resolution guide.

---

## PHASE 3: MERGE SCUOLA-2026 (NATIVE APP - AFTER EDUCATION-PACK!)
**Status**: [ ] NOT STARTED
**Risk**: LOW (education-pack already merged, only 1 C file change)
**Estimate**: 2-3 hours

### Features to Preserve (CRITICAL - DO NOT LOSE)
- [ ] 17 Historical Maestri portraits (images in assets)
- [ ] Azure OpenAI GDPR enforcement for EDU edition
- [ ] Student Profile settings tab
- [ ] Global hotkey (Cmd+Shift+Space)
- [ ] Voice waveform visualization
- [ ] OpenDyslexic font for accessibility
- [ ] WebSocket reconnection with exponential backoff
- [ ] Grozio maestro for International Law
- [ ] Theme system fixes
- [ ] All Swift native app code

### 3.1 Pre-Merge Checks
```bash
cd /Users/roberdan/GitHub/ConvergioCLI/native-scuola-2026
git status
git log --oneline -5
```
- [ ] Verify worktree is clean
- [ ] Document last commit: _______________

### 3.2 Merge to Development
```bash
git checkout development
git merge feature/scuola-2026 --no-ff --no-commit
```
- [ ] Initiate merge

### 3.3 Resolve Conflicts
**Expected Conflicts**:
1. `.github/workflows/ci.yml`
2. `Makefile`

**Resolution Strategy**:
- [ ] `ci.yml`: Keep scuola-2026 version (has native app CI)
- [ ] `Makefile`: Keep scuola-2026 version (has native build targets)

```bash
git checkout --theirs .github/workflows/ci.yml
git checkout --theirs Makefile
git add .github/workflows/ci.yml Makefile
```

### 3.4 Validation Checklist
- [ ] Build C code: `make clean && make`
- [ ] Build Swift app: `make native` or `xcodebuild`
- [ ] Tests pass: `make test`
- [ ] Verify maestri images: `ls ConvergioApp/ConvergioApp/Assets.xcassets/Maestri/`
- [ ] Verify Azure provider: `ls ConvergioApp/ConvergioApp/Services/AzureOpenAIProvider.swift`
- [ ] Verify Student Profile: `grep -l "StudentProfile" ConvergioApp/`
- [ ] Verify Hotkey: `grep -l "HotkeyManager" ConvergioApp/`
- [ ] Verify Voice waveform: `grep -l "waveform\|Waveform" ConvergioApp/`
- [ ] Verify OpenDyslexic: `grep -l "OpenDyslexic" ConvergioApp/`
- [ ] Verify Grozio: `grep -l "Grozio" ConvergioApp/` or `ls **/grozio*`

### 3.5 Commit and Push
```bash
git commit -m "Merge feature/scuola-2026: EDU native app with 17 maestri, GDPR, accessibility"
git push origin development
```
- [ ] Commit merge
- [ ] Push to remote
- [ ] Verify CI passes

---

## PHASE 4: VERIFY & CHERRY-PICK FROM NATIVE-APP
**Status**: [ ] NOT STARTED
**Risk**: LOW (scuola-2026 is superset, only 1 unique commit)
**Estimate**: 30 minutes

### Analysis
`feature/native-app` is **almost entirely contained** in scuola-2026:
- scuola-2026 has 25 commits not in native-app (superset)
- native-app has only **1 unique commit** (`b6382b2`)

**Unique commit adds:**
- `ProviderManager` for AI provider selection and fallback chain
- Minor FlashcardDeckView swipe improvements (already in scuola as different impl)

**Verification**: Files that exist in both branches:
- ✓ `FlashcardDeckView.swift` - exists in scuola-2026
- ✓ `FSRSManager.swift` - exists in scuola-2026
- ✗ `ProviderManager.swift` - NOT in scuola-2026 (optional feature)

### 4.1 Action: Cherry-pick ProviderManager
**Required**: Cherry-pick the unique commit to preserve ProviderManager feature.

```bash
git checkout development
git cherry-pick b6382b2 --no-commit
# Selectively stage ProviderManager-related files
git add ConvergioApp/ConvergioApp/Services/ProviderManager.swift
git commit -m "Cherry-pick from native-app: ProviderManager for AI provider selection"
```

### 4.2 Verification Checklist
- [ ] Confirm FlashcardDeckView.swift in development: `ls ConvergioApp/ConvergioApp/Views/Education/FlashcardDeckView.swift`
- [ ] Confirm FSRSManager.swift in development: `ls ConvergioApp/ConvergioApp/Services/FSRSManager.swift`
- [ ] Confirm ProviderManager.swift cherry-picked: `ls ConvergioApp/ConvergioApp/Services/ProviderManager.swift`
- [ ] Mark native-app as superseded by scuola-2026

### 4.3 Build Validation
- [ ] Build Swift app: `make native`
- [ ] Tests pass: `make test`
- [ ] Verify Provider Manager: `grep -l "ProviderManager" ConvergioApp/`
- [ ] Verify Flashcard swipe: `grep "swipe\|Swipe" ConvergioApp/**/*.swift`
- [ ] Verify FSRS scheduling: `grep "FSRS\|fsrs" ConvergioApp/**/*.swift`

### 4.4 Commit and Push
```bash
git push origin development
```
- [ ] Push to remote
- [ ] Verify CI passes

### 4.5 Archive Native-App Branch
```bash
git tag archive/feature-native-app-v6 feature/native-app
git push origin archive/feature-native-app-v6
```
- [ ] Tag for archive
- [ ] DO NOT delete yet (keep until V6 stable)

---

## PHASE 4b: CI FIX (ALREADY DONE)
**Status**: [x] COMPLETED
**Commit**: `153695f` on feature/education-pack

### Fix Applied
**Error was**: `src/orchestrator/registry.c:1448:22: warning: unused variable 'current_edition'`
**Fix**: Removed unused variable declaration (edition_has_agent() internally uses edition_current())

---

## PHASE 5: EDUCATION-PACK DETAILED CONFLICT RESOLUTION GUIDE
> ⚠️ **REFERENCE SECTION** - This is NOT a separate phase. It's the detailed conflict resolution guide for Phase 2 (education-pack merge). Follow these instructions during Phase 2 execution.

**Status**: Follow during Phase 2
**Risk**: HIGH (~100 conflicts)
**Estimate**: 8-16 hours
**Strategy**: INCREMENTAL MERGE

### Features to Preserve (CRITICAL)
- [ ] Azure OpenAI verification system
- [ ] 17 Historical Teachers integration in C code
- [ ] LLM test improvements
- [ ] Mastery gate and visualization
- [ ] Safety tests (SAF01-SAF10, 25 tests)
- [ ] Sanitizer fixes (use-after-free)
- [ ] Phase 1-5 education features
- [ ] Workflow orchestration enhancements
- [ ] Multi-edition system
- [ ] All tests that pass (499+ tests)

### ⚠️⚠️⚠️ CRITICAL: Azure OpenAI ONLY for Education Edition ⚠️⚠️⚠️
**In EDUCATION edition, we MUST use ONLY Azure OpenAI, NEVER Anthropic!**
- GDPR compliance requires Azure's EU data residency
- Schools require Azure for institutional compliance
- All LLM calls in EDU edition MUST go through Azure OpenAI

**Verification Commands:**
```bash
# Should return NOTHING in education code paths:
grep -r "anthropic\|ANTHROPIC" src/education/ src/providers/anthropic*

# Should show Azure configuration:
grep -r "azure\|AZURE" src/providers/azure* src/education/

# Verify provider selection in EDU mode:
grep -r "edition.*EDUCATION.*azure\|azure.*edition.*EDUCATION" src/
```

**Code Requirement:**
- `src/providers/` must check `edition_current() == EDITION_EDUCATION`
- If EDU edition, ONLY allow Azure provider, reject Anthropic/OpenAI

### 5.1 Pre-Merge Preparation
```bash
cd /Users/roberdan/GitHub/ConvergioCLI-education
git fetch origin
git log --oneline main..HEAD | wc -l  # Should be 538
```
- [ ] Verify education-pack is up to date
- [ ] Verify CI is passing after Phase 4 fix
- [ ] Document current test count: _____ tests passing

### 5.2 Rebase Strategy (RECOMMENDED)
```bash
git checkout feature/education-pack
git rebase development
# Resolve conflicts iteratively
```
- [ ] Start rebase onto development
- [ ] For each conflict, follow resolution guide below

### 5.3 Conflict Resolution Guide

#### Category 1: Documentation Files (~15 files)
**Strategy**: Keep education-pack versions (more complete)
- [ ] `docs/workflow-orchestration/*.md` - Keep education
- [ ] `docs/Convergio6MasterPlan.md` - Merge manually
- [ ] `README.md` - Merge manually (add education features)

#### Category 2: C Source Files (~40 files)
**Strategy**: Careful line-by-line merge
- [ ] `src/orchestrator/*.c` - Keep education improvements
- [ ] `src/workflow/*.c` - Keep education features
- [ ] `src/providers/*.c` - Merge carefully (both may have changes)
- [ ] `src/core/*.c` - Keep education edition support
- [ ] `src/agents/*.c` - Keep education maestri

#### Category 3: Header Files (~10 files)
**Strategy**: Include all new definitions
- [ ] `include/nous/edition.h` - Include multi-edition support
- [ ] `include/nous/orchestrator.h` - Merge all new functions
- [ ] `include/nous/tools.h` - Include all tools

#### Category 4: Test Files (~5 files)
**Strategy**: Keep education tests (more comprehensive)
- [ ] `tests/test_stubs.c` - Merge all stubs
- [ ] `tests/test_workflow_e2e.c` - Keep education version

#### Category 5: Configuration (~5 files)
**Strategy**: Merge carefully
- [ ] `Makefile` - Merge all targets (education + native)
- [ ] `.env.example` - Include all education variables
- [ ] `.clang-format` - Keep education version if exists
- [ ] `.github/workflows/ci.yml` - Merge native + education CI

### 5.4 Validation After Each Conflict Set
After resolving each category:
```bash
make clean && make
make test
```
- [ ] Build passes after doc conflicts
- [ ] Build passes after C source conflicts
- [ ] Build passes after header conflicts
- [ ] Build passes after test conflicts
- [ ] Build passes after config conflicts

### 5.5 Complete Rebase
```bash
git rebase --continue
# Repeat until complete
```
- [ ] Rebase completes without errors
- [ ] Push rebased branch: `git push origin feature/education-pack --force-with-lease`

### 5.6 Final Merge to Development
```bash
git checkout development
git merge feature/education-pack --no-ff -m "Merge feature/education-pack: Multi-edition system, 17 maestri, workflow orchestration"
git push origin development
```
- [ ] Merge education-pack to development
- [ ] Push to remote

### 5.7 Comprehensive Validation
- [ ] Full build: `make clean && make all`
- [ ] All tests pass: `make test` (expect 499+ tests)
- [ ] Native build: `make native`
- [ ] Safety tests: `make test-safety` (25 tests)
- [ ] E2E tests: `make test-e2e`
- [ ] CI pipeline passes

---

## PHASE 6: QUALITY GATES
**Status**: [ ] NOT STARTED
**Estimate**: 4-8 hours

### 6.1 Security Fixes (from Luca's audit)
- [ ] **HP-1**: Remove/secure `shell=True` subprocess calls
- [ ] **HP-2**: Replace `random` with `secrets` for security tokens
- [ ] **MP-1**: Implement CORS policy for ACP server
- [ ] **MP-3**: Add input validation to API endpoints

### 6.2 Best Practices Fixes (from Paolo's review)
- [ ] Create `.editorconfig` file
- [ ] Create `.clang-format` configuration
- [ ] Create `.swiftformat` configuration
- [ ] Ensure all error handling follows standard

### 6.3 Documentation Completion
- [ ] Update `README.md` with V6 features
- [ ] Verify `AGENTS.md` is complete
- [ ] Update `docs/PROVIDERS.md`
- [ ] Create `CHANGELOG.md` for V6

### 6.4 Final Test Suite
```bash
make clean
make all
make test
make test-native
make test-e2e
make test-safety
```
- [ ] 100% of tests pass
- [ ] No compiler warnings (zero tolerance)
- [ ] No memory leaks (valgrind/ASan clean)
- [ ] Performance benchmarks within limits

---

## PHASE 7: RELEASE PREPARATION
**Status**: [ ] NOT STARTED
**Estimate**: 2-4 hours

### 7.1 Version Bump
- [ ] Update version in `include/nous/version.h` to 6.0.0
- [ ] Update version in `ConvergioApp/project.yml`
- [ ] Update version in `Package.swift` if exists

### 7.2 Final Development -> Main PR
```bash
gh pr create \
  --base main \
  --head development \
  --title "V6.0.0 Release: Multi-Edition System + Education Pack + Native App" \
  --body "## Summary
- Multi-edition architecture (Education, Business, Developer)
- 17 Historical Maestri with portraits
- Native macOS app with SwiftUI
- GDPR-compliant Azure OpenAI integration
- Workflow orchestration system
- 499+ tests passing
- Zero compiler warnings
- Security audit passed

## Breaking Changes
- None

## Migration Guide
- See docs/MIGRATION_V5_V6.md

Generated with Claude Code assistance."
```
- [ ] Create release PR
- [ ] Request reviews
- [ ] Wait for CI to pass
- [ ] Get approvals
- [ ] Merge to main with merge commit (NOT squash)

### 7.3 Post-Merge
```bash
git checkout main
git pull origin main
git tag -a v6.0.0 -m "Convergio V6.0.0 - Multi-Edition Release"
git push origin v6.0.0
```
- [ ] Tag release
- [ ] Push tag

### 7.4 GitHub Release
```bash
gh release create v6.0.0 \
  --title "Convergio V6.0.0" \
  --notes-file CHANGELOG.md \
  --latest
```
- [ ] Create GitHub release
- [ ] Upload binaries if applicable

---

## PHASE 8: CLEANUP
**Status**: [ ] NOT STARTED
**Estimate**: 1 hour

### 8.1 Remove Worktrees
```bash
git worktree remove /Users/roberdan/GitHub/ConvergioCLI-education
git worktree remove /Users/roberdan/GitHub/ConvergioCLI-features
git worktree remove /Users/roberdan/GitHub/ConvergioCLI/native-scuola-2026
git worktree remove /Users/roberdan/GitHub/ConvergioNative
```
- [ ] Remove all worktrees

### 8.2 Delete Merged Branches
```bash
git branch -d feature/education-pack
git branch -d feature/convergio-enhancements
git branch -d feature/scuola-2026
git branch -d feature/native-app
git branch -d development
```
- [ ] Delete local branches

### 8.3 Clean Remote Branches (optional, keep for 30 days)
```bash
# Only after 30 days of stable V6
git push origin --delete feature/education-pack
git push origin --delete feature/convergio-enhancements
git push origin --delete feature/scuola-2026
git push origin --delete feature/native-app
git push origin --delete development
```
- [ ] Schedule remote cleanup for 30 days post-release

### 8.4 Archive Documentation
- [ ] Move `V6ReleasePlan.md` to `docs/releases/V6ReleasePlan.md`
- [ ] Update `docs/WORKTREE_MERGE_PLAN.md` status to COMPLETED

---

## PHASE 9: FINAL COMPREHENSIVE REVIEW (MANDATORY)
**Status**: [ ] NOT STARTED
**Estimate**: 4-8 hours
**Criticality**: BLOCKING - Do not release without completing this phase

> ⚠️ Questa fase è OBBLIGATORIA prima del release. Verifica che tutto sia coerente e funzionante.

### 9.1 Verifica Ollama Locale (Pre-Test)

```bash
# Verificare che Ollama sia attivo e configurato
ollama list
# Deve mostrare: qwen2.5:0.5b (397MB, locale)

# Se non c'è, scaricarlo:
ollama pull qwen2.5:0.5b

# Verificare che risponda:
curl http://localhost:11434/api/tags

# Test rapido:
echo '{"model": "qwen2.5:0.5b", "prompt": "Hello", "stream": false}' | \
  curl -s http://localhost:11434/api/generate -d @-
```

> ⚠️ Usare SOLO `qwen2.5:0.5b` locale, MAI modelli cloud!

- [ ] Ollama running e responsive
- [ ] Modello leggero disponibile per test
- [ ] Convergio può connettersi a Ollama

### 9.2 Code Review Completo

```bash
# Cercare TODO, FIXME, HACK residui
rg -i "TODO|FIXME|HACK|XXX" src/ include/ --type c | grep -v test

# Cercare stub o placeholder
rg -i "stub|placeholder|not.?implemented" src/ include/ --type c

# Verificare nessun riferimento Anthropic in codice EDU
rg -i "anthropic" src/education/ src/providers/ --type c

# Verificare nessun hardcoded secret
rg -i "api.?key.*=.*['\"]" src/ --type c
```

- [ ] Zero TODO/FIXME in production code
- [ ] Zero stub o placeholder
- [ ] Zero riferimenti Anthropic in codice EDU
- [ ] Zero hardcoded secrets

### 9.3 App Release Manager Check

```bash
# Eseguire app-release-manager per quality gates
convergio
> @app-release-manager check all

# Oppure script automatico:
./scripts/v6-release/v6-master.sh
```

- [ ] Security audit passed (HP-1, HP-2 fixed)
- [ ] Code quality score > 80%
- [ ] Test coverage > 90%
- [ ] Zero compiler warnings
- [ ] All 499+ tests pass

### 9.4 Website Update

```bash
# Verificare che il sito sia aggiornato
ls website/

# Controllare versione nel sito
grep -r "version\|Version\|6.0" website/
```

- [ ] Homepage aggiornata con V6 features
- [ ] Education section presente
- [ ] Native app section presente
- [ ] Comparison matrix aggiornata
- [ ] Download links corretti

### 9.5 README e Documentation

```bash
# Verificare README principale
head -100 README.md

# Verificare che tutti i nuovi comandi siano documentati
grep -E "^##|^###" README.md

# Verificare CHANGELOG
tail -50 CHANGELOG.md
```

- [ ] README.md aggiornato per V6
- [ ] Tutti i nuovi comandi documentati
- [ ] Sezione Education Edition presente
- [ ] CHANGELOG.md aggiornato
- [ ] Versione 6.0.0 in tutti i file

### 9.6 Consistency Check

```bash
# Verificare versione coerente in tutti i file
grep -r "6\.0\.0\|v6\|V6" --include="*.md" --include="*.json" --include="VERSION"

# Verificare che i nomi siano coerenti
grep -ri "convergio" --include="*.md" | grep -i "spelling\|typo"
```

- [ ] Versione 6.0.0 coerente ovunque
- [ ] Naming coerente (Convergio, non ConvergioCLI ovunque)
- [ ] Dates aggiornate nei file
- [ ] Author/credits corretti

### 9.7 Test Completi con Ollama

```bash
# Setup test environment (SOLO modello locale!)
export CONVERGIO_TEST_PROVIDER=ollama
export CONVERGIO_TEST_MODEL=qwen2.5:0.5b
export OLLAMA_HOST=http://localhost:11434

# Run all tests
make clean && make
make test                      # Unit tests
make test_llm PROVIDER=ollama  # LLM integration tests
make test_e2e PROVIDER=ollama  # End-to-end tests
```

- [ ] `make test` - 100% pass
- [ ] `make test_llm PROVIDER=ollama` - 100% pass
- [ ] `make test_e2e PROVIDER=ollama` - 100% pass
- [ ] Zero memory leaks (valgrind/sanitizer)
- [ ] Zero race conditions (thread sanitizer)

### 9.8 Test Pre-Release con Azure (EDU Only)

```bash
# Solo per validare EDU edition
export CONVERGIO_TEST_PROVIDER=azure
export AZURE_OPENAI_ENDPOINT=$AZURE_OPENAI_ENDPOINT
export AZURE_OPENAI_KEY=$AZURE_OPENAI_KEY

# Test EDU specifici
make test_edu PROVIDER=azure
```

- [ ] EDU edition usa SOLO Azure
- [ ] GDPR compliance verificata
- [ ] Azure endpoint EU West Europe

### 9.9 Final Checklist

| Item | Status | Verified By |
|------|--------|-------------|
| All branches merged | [ ] | |
| Zero warnings | [ ] | |
| All tests pass | [ ] | |
| Ollama tests pass | [ ] | |
| Azure EDU tests pass | [ ] | |
| README updated | [ ] | |
| CHANGELOG updated | [ ] | |
| Website updated | [ ] | |
| Version 6.0.0 everywhere | [ ] | |
| Security audit clean | [ ] | |
| No TODO/FIXME in prod | [ ] | |
| Documentation English | [ ] | |

### 9.10 Sign-Off

```
╔══════════════════════════════════════════════════════════════════╗
║                    V6 RELEASE APPROVAL                            ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                   ║
║  All Phase 9 checks MUST be complete before release.             ║
║                                                                   ║
║  Reviewed by: _________________________                          ║
║  Date: _________________________                                  ║
║  Signature: _________________________                             ║
║                                                                   ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## RISK MITIGATION

### Rollback Procedures

#### If Phase 1-3 fails:
```bash
git checkout main
git branch -D development
git checkout -b development
# Restart from Phase 1
```

#### If Phase 4-5 fails:
```bash
git checkout development
git reset --hard backup-development-pre-education
# Restart Phase 5
```

#### If V6 release has critical bugs:
```bash
git checkout main
git revert -m 1 <merge-commit-sha>
git push origin main
gh release delete v6.0.0
git tag -d v6.0.0
git push origin :refs/tags/v6.0.0
```

### Daily Backup During Merge
```bash
# Run daily during merge process
git checkout development
git branch backup-development-$(date +%Y%m%d) development
```

---

## OPEN TASKS CONSOLIDATED (FROM ALL WORKTREES)

### BLOCKERS - Must Fix Before V6

| Source | Task | Worktree | Status |
|--------|------|----------|--------|
| CI | `registry.c:1448` unused variable `current_edition` | education-pack | ✅ FIXED (153695f) |
| EduRelease | clang-tidy not installed | education-pack | BLOCKED |
| EduRelease | clang-format not installed | education-pack | BLOCKED |
| EduRelease | `/agents` shows 73 instead of 20 (count bug) | education-pack | BUG |
| Voice | Microphone returns zeros (Mac reboot needed) | scuola-2026 | BLOCKED |

---

### From VOICE_IMPLEMENTATION_PLAN.md (scuola-2026)

| ID | Task | Priority | Status |
|----|------|----------|--------|
| V-P0-1 | Test voice session end-to-end | P0 | Pending (after reboot) |
| V-P0-2 | Verify microphone captures audio | P0 | Pending (after reboot) |
| V-P0-3 | Verify AI responses come through | P0 | Pending (after reboot) |
| V-P0-4 | Test emotion detection from voice | P0 | Pending |
| V-P2-1 | Verify all 18 maestri images display | P2 | Pending |
| V-P2-2 | Replace Manzoni placeholder | P2 | Pending |
| V-P3-1 | Add voice transcription to chat history | P3 | Pending |
| V-P3-2 | Save voice sessions to student progress | P3 | Pending |
| V-P3-3 | Test global hotkey from background | P3 | Pending |
| V-P4-1 | Verify OpenDyslexic font in dyslexia mode | P4 | Pending |
| V-P4-2 | Test all accessibility settings together | P4 | Pending |
| V-P4-3 | Edit student profile inline | P4 | Pending |

---

### From ACCESSIBILITY_INTEGRATION.md (scuola-2026)

| ID | Task | Priority | Status |
|----|------|----------|--------|
| A-1 | Screen reader optimized navigation | P3 | Future |
| A-2 | Custom color themes | P3 | Future |
| A-3 | Bionic reading mode | P3 | Future |
| A-4 | Reading ruler/guide | P3 | Future |
| A-5 | Word spacing adjustments | P3 | Future |
| A-6 | Custom TTS voices | P3 | Future |
| A-7 | Session analytics dashboard | P3 | Future |
| A-8 | Multi-language support | P2 | Future |
| A-9 | Cloud sync for settings | P3 | Future |

---

### From RELEASE_PLAN.md (scuola-2026 - CLI v1.0.0)

| Block | Task | Priority | Status |
|-------|------|----------|--------|
| A | VERSION file, headers, CI/CD workflows | P1 | Pending |
| B | Hardware detection implementation | P1 | Pending |
| C | Config parser TOML, Keychain | P1 | Pending |
| D | Updater implementation | P2 | Pending |
| E | fabric.c, gpu.m, scheduler.c, mlx updates | P1 | Pending |
| F | main.c integration | P1 | Pending |
| G | README, CHANGELOG, docs | P2 | Pending |
| H | Homebrew release | P1 | Pending |

---

### From EduReleasePlanDec22.md (education-pack)

| ID | Task | Priority | Status |
|----|------|----------|--------|
| 6.1-6.5 | Static analysis with clang-tidy | P2 | BLOCKED (need brew install llvm) |
| 6.6 | Code formatting with clang-format | P2 | BLOCKED |

---

### From EducationPackMasterPlan.md (education-pack)

| ID | Task | Priority | Status |
|----|------|----------|--------|
| E01 | Design Strategy Edition | P1 | Pending |
| E02 | Design Creative Edition | P1 | Pending |
| E03 | Design Compliance Edition | P2 | Pending |
| E04 | Design HR Edition | P2 | Pending |
| E05 | Add whitelists to edition.c | P1 | Pending (after E01-E04) |
| E06 | Create README for each edition | P1 | Pending |
| R01 | Refactor edition selection to runtime | P2 | Pending |
| R02 | Add `--edition` CLI flag | P2 | Pending |
| R03 | Add `CONVERGIO_EDITION` env var | P2 | Pending |
| R04 | Add edition to config.toml | P2 | Pending |
| R05 | Hot-reload agent registry on switch | P2 | Pending |
| R06 | Update documentation | P2 | Pending |
| L01-L06 | License system | P3 | Future |
| D01-D06 | Distribution (DMG, PKG, Homebrew) | P3 | Future |
| Phase 13 | Localization | P1 | Pending |
| - | Test with 5+ real students | P1 | Pending |
| - | Feedback >4/5 from disability users | P1 | Pending |

---

### From WebsitePlan.md (convergio-enhancements)

| ID | Task | Priority | Status |
|----|------|----------|--------|
| W5 | Add Italian language support | P2 | Pending |
| W6 | Update hero for all editions | P2 | Pending |
| W7 | Add testimonials section | P3 | Pending |
| W8 | SEO optimization | P3 | Pending |
| - | Document Education Manifesto | P2 | Pending |
| - | Document Technical Architecture | P2 | Pending |
| - | Document Teacher Manifesto | P2 | Pending |

---

### From TODO-WORKFLOW-ORCHESTRATION.md (main)

| ID | Task | Priority | Status |
|----|------|----------|--------|
| LOW-01 | Add security metadata to workflow schema | P3 | Pending |
| LOW-02 | Security event logging | P3 | Pending |
| - | Workflow signature verification | P3 | Pending |
| - | Security monitoring dashboard | P3 | Pending |
| - | Automated security scanning in CI/CD | P2 | Pending |
| - | Performance benchmarks vs Claude Code | P2 | Pending |
| - | Codebase audit for remaining TODOs | P2 | Pending |
| - | Documentation improvements | P2 | Pending |
| - | Workflow history UI visualization | P3 | Pending |
| - | Advanced debugging tools for workflows | P3 | Pending |
| - | Workflow version control and rollback | P3 | Pending |
| - | Agent capability scoring system | P3 | Pending |

---

### From MULTIMODEL_ROADMAP.md (main)

| ID | Task | Priority | Status |
|----|------|----------|--------|
| WS-F | Telemetry system (7 subtasks) | P3 | Pending (user consent needed) |
| - | Inter-agent communication protocol | P3 | Future |
| - | State synchronization | P3 | Future |
| - | CI/CD integration | P2 | Future |
| - | Compare command implementation | P2 | Pending |
| - | Benchmark command implementation | P2 | Pending |
| - | Model pricing API fetch | P2 | Pending |
| - | Verify model IDs still valid | P2 | Pending |
| - | Check deprecated models | P2 | Pending |
| - | Update config/models.json | P2 | Pending |

---

### From KillerApp2026.md (scuola-2026)

| ID | Task | Priority | Status |
|----|------|----------|--------|
| KA-1 | Zero P0 bugs in production | P1 | Pending |
| KA-2 | App launch < 2 seconds | P1 | Pending |
| KA-3 | Memory footprint < 500MB | P1 | Pending |
| KA-4 | Battery impact: "Low" in Activity Monitor | P1 | Pending |

---

### SUMMARY: Task Counts by Priority

| Priority | Count | Category |
|----------|-------|----------|
| **P0 (Blocker)** | 5 | CI fix, microphone, clang tools |
| **P1 (Must Have)** | 25 | Core features, editions, tests |
| **P2 (Should Have)** | 30 | Enhancements, docs, tools |
| **P3 (Nice to Have)** | 35 | Future, polish, optimization |

**Total Open Tasks**: ~95

---

## SUCCESS CRITERIA

### Must Have (V6 Release Gate)
- [ ] All 3 branches merged + 1 cherry-pick (enhancements, education-pack, scuola-2026 + native-app ProviderManager)
- [ ] Zero compiler warnings
- [ ] All tests pass (499+ expected)
- [ ] CI pipeline green
- [ ] Security audit items HP-1, HP-2 fixed
- [ ] README updated
- [ ] Version bumped to 6.0.0

### Should Have
- [ ] Performance benchmarks documented
- [ ] Migration guide created
- [ ] All documentation in English
- [ ] ADRs for major decisions

### Nice to Have
- [ ] Automated dependency scanning
- [ ] Security headers in ACP
- [ ] Complete OWASP compliance

---

## TIMELINE ESTIMATE

| Phase | Description | Effort | Dependencies |
|-------|-------------|--------|--------------|
| Phase 0 | Preparation & Backups | 30 min | None |
| Phase 1 | Merge convergio-enhancements | 1 hour | Phase 0 |
| Phase 2 | Merge education-pack (CORE) | 8-16 hours | Phase 1 |
| Phase 3 | Merge scuola-2026 | 2-3 hours | Phase 2 |
| Phase 4 | Cherry-pick from native-app | 30 min | Phase 3 |
| Phase 5 | Reference (see Phase 2) | - | - |
| Phase 6 | Quality Gates | 4-8 hours | Phase 3 |
| Phase 7 | Release Preparation | 2-4 hours | Phase 6 |
| Phase 8 | Cleanup | 1 hour | Phase 7 |
| Phase 9 | Final Comprehensive Review | 4-8 hours | Phase 8 |

**Total Estimate**: 24-44 hours of work

---

## APPROVAL

- [ ] Plan reviewed by Roberto
- [ ] Merge order confirmed
- [ ] Risk mitigation approved
- [ ] Ready to execute

**Signature**: ________________________
**Date**: ________________________
