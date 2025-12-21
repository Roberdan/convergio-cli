# Workflow-Orchestration Merge Plan

**Created**: 2025-12-21
**Status**: 📋 READY FOR EXECUTION
**Priority**: HIGH - Blocks education-pack merge
**Estimated Effort**: 2-4 hours

---

## Executive Summary

The `feature/workflow-orchestration` branch introduces a comprehensive workflow engine (~157 files, +32K lines) but was developed **before** PR #73 (runtime edition switching). This creates a critical conflict: the workflow branch **DELETES** the edition system.

**Goal**: Merge workflow → main while preserving the edition system, then rebase education-pack.

---

## Current State

### Branches & Worktrees

| Worktree | Branch | Status |
|----------|--------|--------|
| `/Users/roberdan/GitHub/ConvergioCLI` | main | Has edition system (PR #73) |
| `/Users/roberdan/GitHub/ConvergioCLI-workflow` | feature/workflow-orchestration | Has workflow engine, MISSING edition |
| `/Users/roberdan/GitHub/ConvergioCLI-education` | feature/education-pack | Has education features + edition |

### Critical Conflicts

| File | Main (after PR #73) | Workflow Branch | Resolution |
|------|---------------------|-----------------|------------|
| `include/nous/edition.h` | ✅ Full edition API | ❌ DELETED | Restore from main |
| `src/core/edition.c` | ✅ Runtime switching | ❌ DELETED | Restore from main |
| `src/core/main.c` | ✅ Has `--edition` flag | ❌ No edition flag | Merge carefully |
| `Makefile` | ✅ EDITION support | ⚠️ Different structure | Manual merge |

---

## Pre-Merge Checklist

### 1. Verify Current State

```bash
# Check main has edition system
cd /Users/roberdan/GitHub/ConvergioCLI
git checkout main
git pull origin main
ls include/nous/edition.h  # Must exist
grep "edition_set" src/core/edition.c  # Must find function

# Check workflow branch status
cd /Users/roberdan/GitHub/ConvergioCLI-workflow
git status  # Should be clean
git log --oneline -5  # Check recent commits
```

### 2. Backup Critical Files

```bash
# Create backup of edition files from main
cd /Users/roberdan/GitHub/ConvergioCLI
mkdir -p /tmp/edition-backup
cp include/nous/edition.h /tmp/edition-backup/
cp src/core/edition.c /tmp/edition-backup/
cp src/core/main.c /tmp/edition-backup/main.c.backup
```

### 3. Verify Workflow Tests Pass

```bash
cd /Users/roberdan/GitHub/ConvergioCLI-workflow
make clean && make
make test  # All tests must pass
```

---

## Merge Execution Plan

### Phase 1: Prepare Workflow Branch

**Goal**: Add edition system to workflow branch before merging

```bash
cd /Users/roberdan/GitHub/ConvergioCLI-workflow

# 1. Fetch latest main
git fetch origin main

# 2. Create a merge preparation branch
git checkout -b prepare-workflow-merge

# 3. Cherry-pick or merge edition commits from main
# Option A: Merge main into workflow (recommended)
git merge origin/main --no-commit

# 4. Resolve conflicts
# - For edition.h: keep main's version (ours after merge is main's)
# - For edition.c: keep main's version
# - For main.c: carefully merge (keep both --edition flag AND workflow code)
# - For Makefile: manually merge EDITION support + workflow targets
```

### Phase 2: Resolve Conflicts

#### 2.1 edition.h Conflict

```bash
# If deleted in workflow, restore from main
git checkout origin/main -- include/nous/edition.h
```

#### 2.2 edition.c Conflict

```bash
# If deleted in workflow, restore from main
git checkout origin/main -- src/core/edition.c
```

#### 2.3 main.c Conflict

**Manual merge required**. Must keep BOTH:
- Edition handling from main (lines ~280-290 in main.c)
- Workflow integration from workflow branch

```c
// Keep from main:
} else if ((strcmp(argv[i], "--edition") == 0 || strcmp(argv[i], "-e") == 0) && i + 1 < argc) {
    const char* ed = argv[++i];
    if (strcmp(ed, "master") == 0) edition_set(EDITION_MASTER);
    else if (strcmp(ed, "business") == 0) edition_set(EDITION_BUSINESS);
    else if (strcmp(ed, "developer") == 0) edition_set(EDITION_DEVELOPER);
    else {
        fprintf(stderr, "Unknown edition: %s\n", ed);
        return 1;
    }
}

// Keep from workflow: any workflow-specific initialization
```

#### 2.4 Makefile Conflict

**Manual merge required**. Must keep BOTH:

```makefile
# Keep from main (top of file):
EDITION ?= master
ifeq ($(EDITION),education)
    EDITION_CFLAGS = -DCONVERGIO_EDITION_EDUCATION
    EDITION_SUFFIX = -edu
else ifeq ($(EDITION),business)
    EDITION_CFLAGS = -DCONVERGIO_EDITION_BUSINESS
    EDITION_SUFFIX = -biz
# ... etc

# Keep from workflow: all workflow targets
workflow_test:
    # ...

coverage_workflow:
    # ...

quality_gate:
    # ...
```

### Phase 3: Verify Merged Code

```bash
cd /Users/roberdan/GitHub/ConvergioCLI-workflow

# 1. Build all editions
make clean && make  # Default (master)
make clean && make EDITION=education  # Education
make clean && make EDITION=business  # Business

# 2. Run all tests
make test

# 3. Run workflow-specific tests
make workflow_test

# 4. Run quality gates
make quality_gate

# 5. Verify edition system works
./build/bin/convergio --edition business --help
./build/bin/convergio --version  # Should show edition
```

### Phase 4: Create PR

```bash
cd /Users/roberdan/GitHub/ConvergioCLI-workflow

# 1. Commit the merge
git add -A
git commit -m "$(cat <<'EOF'
feat(workflow): Advanced Workflow Orchestration with Edition System

Merges workflow orchestration feature while preserving runtime edition
switching from PR #73.

## Workflow Features
- State machine-based execution engine
- Checkpointing and recovery
- Task decomposition with LLM
- Group chat with consensus
- Conditional routing
- 9 pre-built workflow templates
- CLI commands: /workflow list, execute, resume

## Quality Standards
- Zero tolerance policy
- Pre-commit hooks
- Quality gates (coverage >= 80%)
- Security enforcement

## Files Added
- src/workflow/*.c (13 files, ~4500 lines)
- include/nous/workflow*.h (5 headers)
- tests/test_workflow*.c (9 test files, 80+ tests)
- src/workflow/templates/*.json (9 templates)
- docs/workflow-orchestration/*.md (40+ docs)

## Preserved from Main
- include/nous/edition.h
- src/core/edition.c
- --edition CLI flag
- EDITION Makefile support

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>
EOF
)"

# 2. Push
git push origin prepare-workflow-merge

# 3. Create PR
gh pr create \
  --title "feat(workflow): Advanced Workflow Orchestration (Edition-Compatible)" \
  --body "$(cat <<'EOF'
## Summary
Merges the comprehensive workflow orchestration system while preserving
the runtime edition switching from PR #73.

## What's New
- 📊 State machine workflow engine
- 💾 Checkpointing and recovery
- 🔄 Task decomposition
- 💬 Group chat with consensus
- 🔀 Conditional routing
- 📋 9 workflow templates (including class_council.json for education)
- 🛡️ Zero tolerance quality standards

## Conflict Resolution
- `edition.h` - Restored from main
- `edition.c` - Restored from main
- `main.c` - Merged (keeps --edition flag + workflow code)
- `Makefile` - Merged (keeps EDITION support + workflow targets)

## Test Plan
- [x] Build passes for all editions (master, education, business)
- [x] All existing tests pass
- [x] All workflow tests pass (80+ test cases)
- [x] Edition switching works at runtime
- [x] Quality gates pass

## Next Steps
After this merge:
1. Rebase feature/education-pack on new main
2. Merge feature/education-pack

🤖 Generated with [Claude Code](https://claude.com/claude-code)
EOF
)"
```

### Phase 5: Merge and Cleanup

```bash
# After PR approval and CI passes

# 1. Merge with merge commit (NEVER squash)
gh pr merge --merge

# 2. Update main
cd /Users/roberdan/GitHub/ConvergioCLI
git checkout main
git pull origin main

# 3. Verify
make clean && make
./build/bin/convergio --version
./build/bin/convergio --edition business --help
```

---

## Post-Merge: Education Pack Rebase

### Rebase Education on New Main

```bash
cd /Users/roberdan/GitHub/ConvergioCLI-education

# 1. Fetch updated main
git fetch origin main

# 2. Rebase
git rebase origin/main

# 3. Resolve any conflicts (should be minimal)
# - Makefile: may need to merge workflow + education targets
# - edition.c: should be clean (both have same version)

# 4. Verify
make clean && make EDITION=education
make education_test

# 5. Push (force required after rebase)
git push origin feature/education-pack --force-with-lease

# 6. Update PR #71
# PR should automatically update
```

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

## Verification Checklist

### After Workflow Merge

- [ ] `make clean && make` passes
- [ ] `make EDITION=education` passes
- [ ] `make EDITION=business` passes
- [ ] `make test` passes (all tests)
- [ ] `make workflow_test` passes
- [ ] `./build/bin/convergio --version` shows correct version
- [ ] `./build/bin/convergio --edition business` works
- [ ] `./build/bin/convergio-edu` locked to education (if built with EDITION=education)
- [ ] Quality gates pass: `make quality_gate`

### After Education Rebase

- [ ] `make EDITION=education` passes
- [ ] `make education_test` passes
- [ ] Ali Preside agent works
- [ ] Education agents load correctly
- [ ] Accessibility runtime works
- [ ] Libretto feature works

---

## Timeline

| Phase | Duration | Dependencies |
|-------|----------|--------------|
| Phase 1: Prepare | 30 min | None |
| Phase 2: Resolve Conflicts | 1 hour | Phase 1 |
| Phase 3: Verify | 30 min | Phase 2 |
| Phase 4: Create PR | 15 min | Phase 3 |
| Phase 5: Merge | 15 min | PR approval |
| Education Rebase | 30 min | Phase 5 |

**Total**: ~3 hours

---

## Key Files Reference

### Workflow Branch New Files

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

### Files Modified in Main

```
include/nous/edition.h     # Edition API (must preserve)
src/core/edition.c         # Runtime switching (must preserve)
src/core/main.c            # CLI flags (must merge carefully)
Makefile                   # Build system (must merge)
CMakeLists.txt             # CMake (add workflow sources)
```

---

## Notes

1. **NEVER squash commits** - Preserve full history
2. **NEVER force push to main** - Unless absolutely necessary with team agreement
3. **Always run tests** before pushing
4. **Create checkpoint tags** before risky operations: `git tag pre-workflow-merge`

---

**Document Owner**: Roberto
**Last Updated**: 2025-12-21
**Next Review**: Before merge execution
