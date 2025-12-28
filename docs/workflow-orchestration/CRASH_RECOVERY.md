# Crash Recovery & Development Continuity Plan

**Date**: 2025-12-18  
**Status**: Mandatory Recovery Procedures

---

## Overview

This document defines procedures for recovering from crashes, interruptions, or needing to restart development work.

---

## Development State Persistence

### Git Strategy: Frequent Commits

**CRITICAL**: Commit early, commit often.

```bash
# After every small working change
git add src/workflow/workflow_engine.c
git commit -m "feat(workflow): implement workflow_execute function"

# After tests pass
git add tests/test_workflow_engine.c
git commit -m "test(workflow): add tests for workflow_execute"

# After each task completion
git commit -m "feat(workflow): complete F3 - Basic State Machine"
```

**Rule**: Never lose more than 1 hour of work. Commit at least every hour.

---

## Crash Recovery Procedures

### Scenario 1: Development Machine Crash

**Recovery Steps:**

1. **Check Git Status**
   ```bash
   cd /Users/roberdan/GitHub/ConvergioCLI-workflow
   git status
   ```

2. **Recover Uncommitted Work**
   ```bash
   # Check for uncommitted changes
   git diff
   git diff --staged
   
   # If changes exist, commit or stash
   git stash save "WIP: workflow engine implementation"
   # OR
   git commit -m "WIP: checkpoint before crash recovery"
   ```

3. **Verify Worktree State**
   ```bash
   # Check if worktree still exists
   git worktree list
   
   # If worktree lost, recreate
   git worktree add ../ConvergioCLI-workflow feature/workflow-orchestration
   ```

4. **Resume Development**
   ```bash
   # Restore stashed work (if stashed)
   git stash pop
   
   # Or continue from last commit
   git log --oneline -5  # Check last commits
   ```

---

### Scenario 2: Need to Restart Phase

**If a phase needs to be restarted:**

1. **Document What Went Wrong**
   ```bash
   # Create recovery document
   echo "Phase 1 restart - Issues: [describe]" > RECOVERY_LOG.md
   ```

2. **Create Recovery Branch**
   ```bash
   git checkout -b phase-1-foundation-v2
   # Or reset if needed
   git reset --hard phase-1-foundation-base
   ```

3. **Review Previous Work**
   ```bash
   # Check what was done
   git log --oneline phase-1-foundation-v2..phase-1-foundation
   
   # Review code
   git diff phase-1-foundation-v2..phase-1-foundation
   ```

4. **Apply Lessons Learned**
   - Update phase document with learnings
   - Adjust approach based on issues
   - Update test plan if needed

---

### Scenario 3: Database Corruption

**If database migration fails or corrupts:**

1. **Backup Current State**
   ```bash
   cp data/convergio.db data/convergio.db.backup
   ```

2. **Rollback Migration**
   ```sql
   -- In SQLite
   BEGIN TRANSACTION;
   DROP TABLE IF EXISTS workflow_checkpoints;
   DROP TABLE IF EXISTS workflow_state;
   DROP TABLE IF EXISTS workflow_edges;
   DROP TABLE IF EXISTS workflow_nodes;
   DROP TABLE IF EXISTS workflows;
   COMMIT;
   ```

3. **Fix Migration Script**
   - Identify issue in migration
   - Fix migration script
   - Test migration in isolation

4. **Re-run Migration**
   ```bash
   # Test migration first
   sqlite3 test.db < src/memory/migrations/016_workflow_engine.sql
   
   # If successful, apply to main DB
   sqlite3 data/convergio.db < src/memory/migrations/016_workflow_engine.sql
   ```

---

### Scenario 4: Test Suite Failure

**If test suite breaks and can't be fixed quickly:**

1. **Isolate Failing Test**
   ```bash
   # Run specific test
   ./build/bin/test_workflow --filter test_workflow_create
   ```

2. **Temporarily Disable (Last Resort)**
   ```c
   // In test file
   void test_workflow_create(void) {
       // TODO: Fix this test - disabled due to [reason]
       // TEST_SKIP("Temporarily disabled - issue #XXX");
       return;
   }
   ```

3. **Create Issue**
   - Document the failure
   - Create GitHub issue
   - Link issue in code comment

4. **Fix Immediately**
   - Do not proceed with other work
   - Fix test before continuing
   - Re-enable test

**ZERO TOLERANCE**: Tests cannot be permanently disabled. Must be fixed.

---

## Checkpoint System for Development

### Daily Checkpoints

**End of each day:**

```bash
# Commit all work
git add -A
git commit -m "WIP: End of day checkpoint - Phase 1, Day 3"

# Push to remote (backup)
git push origin phase-1-foundation

# Create checkpoint tag
git tag -a checkpoint/phase-1-day-3 -m "Checkpoint: Phase 1, Day 3 complete"
git push origin checkpoint/phase-1-day-3
```

### Task Completion Checkpoints

**After each task:**

```bash
# Task F1 complete
git commit -m "feat(workflow): F1 complete - Database schema"
git tag -a checkpoint/phase-1-f1 -m "Checkpoint: F1 complete"
git push origin phase-1-foundation checkpoint/phase-1-f1
```

### Phase Completion Checkpoints

**After each phase:**

```bash
# Phase 1 complete
git checkout feature/workflow-orchestration
git merge phase-1-foundation
git tag -a checkpoint/phase-1-complete -m "Phase 1 complete"
git push origin feature/workflow-orchestration checkpoint/phase-1-complete
```

---

## Recovery from Specific Failures

### Build Failure

```bash
# Clean build
make clean
rm -rf build/

# Rebuild
make

# If still fails, check dependencies
make DEBUG=1  # Check for missing dependencies
```

### Test Failure

```bash
# Run specific failing test
./build/bin/test_workflow --filter test_workflow_create

# Run with debug output
./build/bin/test_workflow --filter test_workflow_create --verbose

# Run with sanitizer to find issue
make DEBUG=1 SANITIZE=address test_workflow
```

### Memory Leak

```bash
# Run with Address Sanitizer
make DEBUG=1 SANITIZE=address test_workflow

# Use Instruments (macOS)
instruments -t "Leaks" ./build/bin/test_workflow
```

### Data Race

```bash
# Run with Thread Sanitizer
make DEBUG=1 SANITIZE=thread test_workflow

# Review output for race conditions
```

---

## Development Continuity Checklist

### Daily Start

- [ ] Check git status
- [ ] Pull latest changes
- [ ] Review yesterday's work
- [ ] Check for any issues/blockers
- [ ] Update phase status

### Daily End

- [ ] Commit all work
- [ ] Push to remote
- [ ] Create checkpoint tag
- [ ] Update phase document
- [ ] Document any issues

### Weekly Review

- [ ] Review phase progress
- [ ] Check test coverage
- [ ] Review code quality
- [ ] Update master plan
- [ ] Plan next week

---

## Emergency Procedures

### Complete Reset (Last Resort)

**If everything is broken and needs complete restart:**

```bash
# 1. Backup current state
git branch backup/before-reset-$(date +%Y%m%d)
git push origin backup/before-reset-$(date +%Y%m%d)

# 2. Reset to last known good state
git checkout feature/workflow-orchestration
git reset --hard checkpoint/phase-1-f1  # Or last good checkpoint

# 3. Clean build
make clean
rm -rf build/

# 4. Rebuild
make

# 5. Run tests
make test

# 6. Resume from checkpoint
```

---

## Prevention Strategies

### 1. Frequent Commits

**Rule**: Commit at least every hour, or after every test pass.

### 2. Automated Backups

**Git Hooks** (optional):
```bash
# .git/hooks/post-commit
#!/bin/sh
# Auto-push after commit (if on feature branch)
BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [[ "$BRANCH" == phase-* ]]; then
    git push origin "$BRANCH" --quiet
fi
```

### 3. Test-Driven Development

**Write tests first:**
- Write test
- See it fail
- Write code
- See test pass
- Commit

**Benefits:**
- Always have working tests
- Clear progress tracking
- Easy to resume after interruption

### 4. Incremental Development

**Small, testable changes:**
- Don't write 500 lines then test
- Write 50 lines, test, commit
- Repeat

---

## Recovery Documentation

### Recovery Log Template

**File**: `RECOVERY_LOG.md` (in worktree root)

```markdown
# Recovery Log

## 2025-12-18: Phase 1 Restart

### Issue
[Describe what went wrong]

### Recovery Actions
1. [Action taken]
2. [Action taken]

### Lessons Learned
- [Learning 1]
- [Learning 2]

### Prevention
- [How to prevent in future]
```

---

## Conclusion

**Key Principles:**
1. **Commit Frequently** - Never lose more than 1 hour of work
2. **Tag Checkpoints** - Easy to resume from known good state
3. **Test Continuously** - Always have working tests
4. **Document Issues** - Learn from problems
5. **Backup Regularly** - Push to remote daily

**Recovery is always possible if:**
- ✅ Work is committed frequently
- ✅ Tests are passing
- ✅ Checkpoints are tagged
- ✅ Remote is up to date











