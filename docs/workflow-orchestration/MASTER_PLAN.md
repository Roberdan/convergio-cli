# Advanced Workflow Orchestration - Master Plan

**Created**: 2025-12-18  
**Last Updated**: 2025-12-20  
**Status**: 🔄 IN PROGRESS (Core Implementation Complete)  
**Version**: 1.2.0  
**Branch**: `feature/workflow-orchestration`

---

## QUICK STATUS

| Phase | Status | Tasks | Note |
|-------|--------|-------|------|
| [PHASE 1 - Foundation](phases/phase-1-foundation.md) | ✅ Core Complete | 4/4 | Workflow engine core implemented |
| [PHASE 2 - Task Decomposition](phases/phase-2-task-decomposition.md) | ✅ Core Complete | 3/3 | CrewAI-inspired patterns implemented |
| [PHASE 3 - Group Chat & Refinement](phases/phase-3-group-chat.md) | ✅ Core Complete | 3/3 | AutoGen-inspired patterns implemented |
| [PHASE 4 - Conditional Routing](phases/phase-4-conditional-routing.md) | ✅ Core Complete | 2/3 | LangGraph-inspired routing (Mermaid export pending) |
| [PHASE 5 - Integration & Polish](phases/phase-5-integration.md) | 🔄 Partial | 2/4 | CLI commands done, tests/docs pending |
| **TOTAL** | **🔄** | **14/17** | Core implementation complete, tests/docs in progress |

### Status Legend
- ✅ Done - Phase completed
- 🔄 In Progress - Currently working on
- ⏸️ Pending - Not started yet
- ❌ Blocked - Has blockers

---

## EXECUTIVE SUMMARY

### ⚠️ IMPORTANT: Compatibility & Risk Assessment

**✅ YES, si può fare tutto senza rompere Convergio attuale**

- **Phase 1-4**: Zero risk - puro codice nuovo, zero modifiche al codice esistente
- **Phase 5**: Low risk - integrazione graduale con fallback
- **Backward Compatibility**: 100% garantita
- **Usability**: Disponibile da subito per Ali e altri agenti (opzionale)

**📖 Leggi [COMPATIBILITY_ANALYSIS.md](COMPATIBILITY_ANALYSIS.md) per dettagli completi.**

### Expected Benefits

This implementation will deliver:

1. **Enhanced Multi-Agent Coordination**
   - State machine-based workflows enable complex multi-step agent collaboration
   - Conditional routing allows dynamic workflow adaptation based on results
   - Workflow templates provide reusable patterns for common scenarios
   - **Impact**: More sophisticated agent orchestration, reduced manual coordination

2. **Workflow Persistence & Recovery**
   - Checkpointing enables resume from any point in workflow execution
   - State persistence survives crashes and restarts
   - Workflow history for debugging and auditing
   - **Impact**: No lost work, better reliability, easier debugging

3. **Advanced Task Decomposition**
   - Hierarchical task breakdown with automatic dependency resolution
   - Role-based task assignment to appropriate agents
   - Parallel execution of independent tasks
   - **Impact**: Better task planning, optimal agent utilization

4. **Group Chat & Consensus Building**
   - Multi-agent discussions with structured turn-taking
   - Consensus detection for collaborative decisions
   - Iterative refinement loops for quality improvement
   - **Impact**: Better decision-making, higher quality outputs

5. **Improved Error Handling**
   - Retry logic with state machine support
   - Fallback strategies for failed nodes
   - Error state handling and recovery
   - **Impact**: More robust workflows, better failure recovery

6. **Enhanced Observability**
   - Workflow visualization (Mermaid diagrams)
   - Execution history and debugging
   - Performance metrics per workflow
   - **Impact**: Better understanding, easier troubleshooting

7. **Cost Optimization**
   - Cost estimation before workflow execution
   - Budget checking integrated with workflows
   - Per-workflow cost tracking
   - **Impact**: Better cost control, budget compliance

8. **Developer Experience**
   - Reusable workflow patterns
   - Workflow templates library
   - CLI commands for workflow management
   - **Impact**: Faster development, easier maintenance

### Key Differentiators

- **Zero Python Dependencies**: Pure C/Swift implementation maintains native performance
- **Apple Silicon Optimized**: GCD parallelization, Metal GPU support
- **Security First**: Full compliance with Convergio security standards
- **Cost Aware**: Integrated budget management and cost tracking
- **Production Ready**: Comprehensive testing, error handling, recovery

---

## DEFINITION OF DONE

- [x] All 5 phases core implementation completed
- [ ] All unit tests passing (>= 80% coverage) - **IN PROGRESS**
- [ ] All integration tests passing - **PENDING**
- [ ] All fuzz tests passing - **PENDING**
- [ ] All sanitizer tests passing (ASan, UBSan, TSan) - **PENDING**
- [ ] Security audit passed (Luca + Guardian agents) - **PENDING**
- [ ] Code review completed - **PENDING**
- [x] Documentation started (USER_GUIDE.md created)
- [ ] Performance targets met - **PENDING**
- [ ] Cost tracking integrated - **PENDING**
- [ ] PR merged to main - **PENDING**
- [ ] Release notes updated - **PENDING**

**Status**: Core implementation complete (14/17 tasks), testing and documentation in progress.

---

## IMPLEMENTATION STATUS (2025-12-20)

### ✅ Completed Components

**Phase 1 - Foundation:**
- ✅ Database schema migration (016_workflow_engine.sql)
- ✅ Workflow data structures (workflow.h, workflow_types.c)
- ✅ Basic state machine (workflow_engine.c)
- ✅ Checkpoint manager (checkpoint.c with SQLite)

**Phase 2 - Task Decomposition:**
- ✅ Task decomposer core (task_decomposer.c with LLM integration)
- ✅ Dependency resolution and topological sort
- ✅ Template library (code_review.json, product_launch.json)

**Phase 3 - Group Chat:**
- ✅ Group chat manager (group_chat.c)
- ✅ Consensus detection (voting mechanism)
- ✅ Turn-taking logic (round-robin, priority, consensus)

**Phase 4 - Conditional Routing:**
- ✅ Conditional router (router.c with condition evaluation)
- ✅ Pattern library (patterns.c: Review-Refine, Parallel Analysis, Sequential Planning, Consensus Building)

**Phase 5 - Integration:**
- ✅ CLI commands (workflow.c: list, show, execute, resume)
- ✅ Retry logic (retry.c)
- ✅ Basic tests (test_workflow_types.c)
- ✅ User guide (USER_GUIDE.md)

### ⏳ In Progress / Pending

**Testing:**
- ⏳ Complete test suite (coverage >= 80%)
- ⏳ Integration tests
- ⏳ Fuzz tests
- ⏳ Sanitizer tests (ASan, UBSan, TSan)

**Features:**
- ⏳ Workflow persistence layer (load/save from database)
- ⏳ Mermaid visualization export
- ⏳ Workflow execution history

**Documentation:**
- ⏳ Complete API documentation
- ⏳ README.md workflow section update
- ⏳ Help system integration

**Quality Gates:**
- ⏳ Code coverage >= 80%
- ⏳ Security audit
- ⏳ Performance benchmarks

### 📊 Statistics

- **Files Created**: 17 new files
- **Lines of Code**: ~3,500+ lines
- **Build Status**: ✅ Compiles successfully
- **Core Features**: ✅ All 5 phases core implementation complete

---

## DOCUMENTS

| Document | Description |
|----------|-------------|
| [Architecture Analysis](ARCHITECTURE_ANALYSIS.md) | **READ THIS FIRST** - Coherence with Convergio, scalability analysis, no unnecessary elements |
| [Compatibility Analysis](COMPATIBILITY_ANALYSIS.md) | **READ THIS** - Compatibility with existing code, risk assessment |
| [Integration Guide](INTEGRATION_GUIDE.md) | **READ THIS** - How to integrate workflows immediately, CLI commands, documentation updates |
| [Testing Plan](TESTING_PLAN.md) | **MANDATORY** - Complete test plan for every phase, zero tolerance policy |
| [Crash Recovery](CRASH_RECOVERY.md) | **MANDATORY** - How to recover from crashes, interruptions, restarts |
| [Parallel Development](PARALLEL_DEVELOPMENT.md) | **READ THIS** - Maximum parallelization strategy, time optimization |
| [Zero Tolerance Policy](ZERO_TOLERANCE_POLICY.md) | **MANDATORY** - Zero tolerance for errors, warnings, shortcuts |
| [Setup Checklist](SETUP_CHECKLIST.md) | **READ FIRST** - Pre-implementation setup for Core and Zed |
| [Review & Fixes](REVIEW_AND_FIXES.md) | **AUDIT** - Problems found and fixes applied during review |
| [Readiness Check](READINESS_CHECK.md) | **VERIFY** - Final readiness verification before starting |
| [Immediate Usage Examples](IMMEDIATE_USAGE_EXAMPLES.md) | **EXAMPLES** - Concrete examples of using workflows from Phase 1 |
| [README Update Template](README_UPDATE_TEMPLATE.md) | Template for updating README.md |
| [Architecture](architecture.md) | System architecture and design (to be created in Phase 1) |
| [ADR Workflow Engine](adr/018-workflow-orchestration.md) | Architecture decision record (to be created in Phase 1) |
| [Security Checklist](SECURITY_CHECKLIST.md) | **MANDATORY** - Security requirements checklist for all phases |

### Phase Details

- [PHASE 1 - Foundation](phases/phase-1-foundation.md) - Workflow engine core, checkpointing
- [PHASE 2 - Task Decomposition](phases/phase-2-task-decomposition.md) - CrewAI-inspired task breakdown
- [PHASE 3 - Group Chat & Refinement](phases/phase-3-group-chat.md) - AutoGen-inspired collaboration
- [PHASE 4 - Conditional Routing](phases/phase-4-conditional-routing.md) - LangGraph-inspired routing
- [PHASE 5 - Integration & Polish](phases/phase-5-integration.md) - Full integration and polish

---

## DEVELOPMENT WORKFLOW

### ⚠️ PRE-IMPLEMENTATION SETUP

**CRITICAL**: Complete setup checklist before starting development.

**📋 See [SETUP_CHECKLIST.md](SETUP_CHECKLIST.md) for complete setup instructions.**

**Quick Setup:**

#### Core Convergio (Main Repo)

```bash
# 1. Create feature branch
cd /Users/roberdan/GitHub/ConvergioCLI
git checkout -b feature/workflow-orchestration origin/main
git push -u origin feature/workflow-orchestration

# 2. Create worktree
git worktree add ../ConvergioCLI-workflow feature/workflow-orchestration
cd ../ConvergioCLI-workflow

# 3. Verify
git status  # Should be on feature/workflow-orchestration
make clean && make  # Should build
```

**Worktree Location**: `/Users/roberdan/GitHub/ConvergioCLI-workflow`

#### Convergio Zed (Integration Repo)

```bash
# 1. Create feature branch
cd /Users/roberdan/GitHub/convergio-zed
git checkout -b feature/zed-workflow-orchestration origin/main
git push -u origin feature/zed-workflow-orchestration

# 2. Create worktree
git worktree add ../convergio-zed-workflow feature/zed-workflow-orchestration
cd ../convergio-zed-workflow

# 3. Verify
git status  # Should be on feature/zed-workflow-orchestration
# Build according to Zed instructions
```

**Worktree Location**: `/Users/roberdan/GitHub/convergio-zed-workflow`

**📖 See [SETUP_CHECKLIST.md](SETUP_CHECKLIST.md) for complete verification steps.**

### Development Process

1. **Start Phase**
   ```bash
   # Ensure worktree is clean
   git status
   
   # Create phase branch
   git checkout -b phase-1-foundation
   ```

2. **Implement Phase**
   - Follow phase-specific implementation plan
   - **Write tests FIRST** (TDD approach - test before code)
   - Run tests frequently: `make test_workflow`
   - Check sanitizers: `make DEBUG=1 SANITIZE=address,undefined,thread test_workflow`
   - **Commit after every test pass** (never lose more than 1 hour of work)
   - **Tag checkpoints** after each task completion
   - See [CRASH_RECOVERY.md](CRASH_RECOVERY.md) for recovery procedures

3. **Pre-Commit Checks**
   ```bash
   # Quality gate (runs all checks)
   make quality_gate_workflow
   
   # If quality gate fails, fix issues before committing
   # Pre-commit hook will also run automatically
   
   # Quality gate checks:
   # - Build (zero warnings)
   # - All tests (unit, integration, fuzz, sanitizer)
   # - Coverage (>= 80%)
   # - Static analysis (zero issues)
   # - Security audit (zero vulnerabilities)
   # - Memory leaks (zero leaks)
   # - Data races (zero races)
   
   # See [ZERO_TOLERANCE_POLICY.md](ZERO_TOLERANCE_POLICY.md) for details
   ```

4. **Commit with Conventional Commits**
   ```bash
   git commit -m "feat(workflow): implement workflow engine core"
   git commit -m "test(workflow): add unit tests for workflow engine"
   git commit -m "fix(workflow): fix memory leak in workflow_destroy"
   ```

5. **Verify with App Release Manager**
   ```bash
   # Run app-release-manager agent
   convergio
   > @app-release-manager prepare release v5.4.0
   
   # Or manually verify:
   > @app-release-manager check quality gates
   ```

6. **Create PR for Phase**
   ```bash
   # Push phase branch
   git push origin phase-1-foundation
   
   # Create PR via GitHub CLI
   gh pr create \
     --title "feat(workflow): Phase 1 - Foundation" \
     --body "Implements workflow engine core and checkpointing.
     
     - [x] Workflow engine implemented
     - [x] Checkpoint manager implemented
     - [x] Unit tests passing
     - [x] Security checklist complete
     
     Closes #XXX"
   
   # Or create via GitHub web UI
   ```

7. **PR Review Process**
   - Wait for automated checks (CI)
   - Address review comments
   - Ensure all tests pass
   - Get approval from maintainer

8. **Merge PR**
   ```bash
   # Merge via GitHub CLI (NEVER squash - preserve history)
   gh pr merge phase-1-foundation --merge
   
   # Or merge via GitHub web UI
   ```

9. **Update Master Plan**
   - Mark phase as completed in this file
   - Update phase status
   - Document any deviations or learnings

### Quality Gates (Per Phase) - ZERO TOLERANCE

**MANDATORY before PR - NO EXCEPTIONS:**

**Automated Checks:**
- [ ] Build passes with **zero warnings** (`make clean && make 2>&1 | grep warning` = empty)
- [ ] All unit tests pass (`make test_workflow`)
- [ ] All integration tests pass (`make integration_test_workflow`)
- [ ] All fuzz tests pass (`make fuzz_test_workflow`)
- [ ] All sanitizer tests pass (`make DEBUG=1 SANITIZE=address,undefined,thread test_workflow`)
- [ ] Code coverage >= 80% (`make coverage_workflow` - verified, not estimated)
- [ ] Static analysis clean (`make lint` - zero issues)
- [ ] No memory leaks (Address Sanitizer - zero leaks)
- [ ] No data races (Thread Sanitizer - zero races)
- [ ] Security audit passed (`make security_audit_workflow`)

**Manual Checks:**
- [ ] **Security checklist complete** (see [SECURITY_CHECKLIST.md](SECURITY_CHECKLIST.md) - all items checked)
- [ ] Error handling complete (no silent failures, all errors logged)
- [ ] Memory management correct (no leaks, no use-after-free, NULL after free)
- [ ] Thread safety verified (all global state protected by mutex)
- [ ] SQL queries parameterized (no string concatenation in SQL)
- [ ] Input validation complete (all inputs validated and sanitized)
- [ ] Cost tracking integrated (if applicable)
- [ ] Documentation updated (README, USER_GUIDE, help system)
- [ ] **Quality gate script passes** (`make quality_gate_workflow`)
- [ ] **App-release-manager verification passed** (🟢 APPROVED, not 🔴 BLOCKED)

**See [ZERO_TOLERANCE_POLICY.md](ZERO_TOLERANCE_POLICY.md) for enforcement details.**

### App Release Manager Integration

**Before each PR, verify with app-release-manager:**

```bash
convergio
> @app-release-manager check quality gates
```

**Expected output:**
```
VERDICT: 🟢 APPROVE
✅ All tests passing
✅ Coverage >= 80%
✅ Security audit passed
✅ No memory leaks
✅ No data races
✅ Static analysis clean
✅ Zero warnings
```

**If BLOCKED:**
- **STOP** - Do not create PR
- Fix ALL violations
- Re-run verification
- **Only create PR when 🟢 APPROVED**
- Document all fixes in PR description

**ZERO TOLERANCE: PR will be rejected if app-release-manager returns 🔴 BLOCKED.**

### PR Template

```markdown
## Phase X: [Phase Name]

### Summary
Brief description of what this PR implements.

### Changes
- Implemented workflow engine core
- Added checkpoint manager
- Added unit tests

### Testing
- [x] Unit tests passing
- [x] Integration tests passing
- [x] Fuzz tests passing
- [x] Sanitizer tests passing
- [x] Coverage >= 80%

### Security
- [x] SQL queries parameterized
- [x] Input validation complete
- [x] Security checklist complete

### Checklist
- [x] Code follows Convergio standards
- [x] Error handling complete
- [x] Memory management correct
- [x] Thread safety verified
- [x] Documentation updated
- [x] App-release-manager verified

### Related
Closes #XXX
Part of workflow-orchestration master plan
```

---

## ARCHITECTURE OVERVIEW

See [architecture.md](architecture.md) for detailed architecture.

**Key Components:**
- `src/workflow/workflow_engine.c` - Core workflow execution
- `src/workflow/checkpoint.c` - Checkpoint management
- `src/workflow/task_decomposer.c` - Task decomposition
- `src/workflow/group_chat.c` - Group chat management
- `src/workflow/router.c` - Conditional routing
- `src/workflow/patterns.c` - Pattern library

**Database:**
- Migration: `src/memory/migrations/016_workflow_engine.sql`
- Tables: `workflows`, `workflow_nodes`, `workflow_edges`, `workflow_state`, `workflow_checkpoints`

---

## IMMEDIATE INTEGRATION (From Phase 1)

**Workflows are available immediately after Phase 1 completion:**

### User-Facing Features (Phase 1+)

- ✅ CLI commands: `/workflow list`, `/workflow execute`, `/workflow resume`
- ✅ Help system: `/help workflow` shows all commands
- ✅ README updated with workflow section
- ✅ User guide available

### Ali Integration (Phase 1+, Optional)

- ✅ Ali can suggest using workflows for complex tasks
- ✅ Ali can execute workflows via tool use
- ✅ Workflows are opt-in (backward compatible)

**See [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) for complete integration details.**

---

## NEXT STEPS

1. **Review and approve** this master plan
2. **Complete [SETUP_CHECKLIST.md](SETUP_CHECKLIST.md)** - Setup both Core and Zed worktrees
3. **Read mandatory documents** (see DOCUMENTS section above)
4. **Read [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)** for integration details
5. **Start Phase 1** implementation (includes CLI commands and docs)
6. **Set up CI/CD** for automated testing (if needed)

---

## SYNCHRONIZATION STRATEGY (Core ↔ Zed)

### Development Flow

**Phase 1-4: Develop in Core First**
- All development happens in `/Users/roberdan/GitHub/ConvergioCLI-workflow`
- Core Convergio is the source of truth
- After each phase completion, sync to Zed

**Phase 5: Integration in Both**
- Integrate in Core first
- Then port integration to Zed (adapting to Zed's architecture)

### Sync Process (After Each Phase)

```bash
# After Phase X completion in Core
cd /Users/roberdan/GitHub/ConvergioCLI-workflow
git checkout feature/workflow-orchestration
git merge phase-X-complete
git push origin feature/workflow-orchestration

# Sync to Zed (manual port or script)
cd /Users/roberdan/GitHub/convergio-zed-workflow
# Port changes from Core to Zed
# Adapt to Zed's architecture (Rust/GPUI instead of C)
# Commit and push
```

**Note**: Zed integration may require architectural adaptations (Rust instead of C, GPUI instead of CLI). See [COMPATIBILITY_ANALYSIS.md](COMPATIBILITY_ANALYSIS.md) for details.

---

**Document Status**: Ready for Implementation  
**Last Updated**: 2025-12-18  
**Version**: 1.0.0

## PARALLEL EXECUTION STRATEGY

**Reference**: [PARALLEL_DEVELOPMENT.md](PARALLEL_DEVELOPMENT.md)

### Core Convergio Parallelization

**Phase 1**: Sequential (due to dependencies)
- F1 → F2 → F3 → F4

**Phases 2-4**: Fully parallel (after Phase 1)
- Create 3 separate worktrees:
  ```bash
  git worktree add ../ConvergioCLI-phase2 phase-2-task-decomposition
  git worktree add ../ConvergioCLI-phase3 phase-3-group-chat
  git worktree add ../ConvergioCLI-phase4 phase-4-conditional-routing
  ```
- Develop all 3 phases simultaneously
- Merge all 3 before Phase 5

**Phase 5**: Sequential (depends on Phases 1-4)

### Multi-Agent/Process Execution

**If using multiple agents or processes:**

1. **Each agent/process gets its own worktree:**
   ```bash
   # Agent 1: Phase 2
   git worktree add ../ConvergioCLI-agent1 phase-2-task-decomposition
   
   # Agent 2: Phase 3
   git worktree add ../ConvergioCLI-agent2 phase-3-group-chat
   
   # Agent 3: Phase 4
   git worktree add ../ConvergioCLI-agent3 phase-4-conditional-routing
   ```

2. **Each agent follows same quality gates:**
   - All tests must pass
   - Zero warnings
   - Coverage >= 80%
   - See [ZERO_TOLERANCE_POLICY.md](ZERO_TOLERANCE_POLICY.md)

3. **Coordination:**
   - Daily sync: `git pull origin feature/workflow-orchestration`
   - After completion: Create PR from worktree branch
   - Merge coordinator: Merge all PRs before Phase 5

### Zed Integration Strategy

**Zed development follows Core:**
- Core Convergio is developed first (source of truth)
- After each phase, port to Zed (with architectural adaptations)
- Zed uses same phase structure and quality gates
- See [SYNCHRONIZATION STRATEGY](#synchronization-strategy-core--zed) above

**Time Savings:**
- Sequential: 44 days
- Parallel (Phases 2-4): 30 days
- **Savings: 14 days (32% faster)**
