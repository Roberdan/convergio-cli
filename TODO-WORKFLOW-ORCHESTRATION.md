# Workflow Orchestration - Remaining Tasks

**Branch:** `feature/workflow-orchestration`
**PR:** #72
**Last Updated:** 2025-12-21
**Status:** Security Audit Complete - Fixes Required Before Merge

---

## Executive Summary

The workflow orchestration feature is functionally complete but requires **critical security fixes** before merging to main. Two independent security audits (Luca Security Expert + Guardian AI Validator) identified multiple vulnerabilities.

---

## P0 - Critical Security Fixes (Block Merge)

### From Luca Security Audit

| ID | Issue | File | Severity |
|----|-------|------|----------|
| CRITICAL-01 | SQL Injection in Checkpoint Persistence | `src/memory/persistence.c` | CRITICAL |
| HIGH-01 | Path Traversal in Workflow Loading | `src/workflow/workflow.c` | HIGH |
| HIGH-02 | Unsafe Condition Evaluation | `src/workflow/router.c` | HIGH |

### From Guardian AI Audit

| ID | Issue | File | Severity |
|----|-------|------|----------|
| P0-1 | No Input Sanitization in Router | `src/workflow/router.c` | CRITICAL |
| P0-2 | LLM Output Validation Missing | `src/workflow/task_decomposer.c` | CRITICAL |
| P0-3 | Unsafe Expression Evaluation | `src/workflow/router.c` | CRITICAL |
| P0-4 | Buffer Overflow Risk | `src/workflow/task_decomposer.c` | CRITICAL |

### Required Actions Before Merge

- [ ] Implement parameterized queries for ALL checkpoint operations
- [ ] Add workflow directory whitelist validation
- [ ] Implement safe condition parser with field whitelisting
- [ ] Add JSON schema validation for LLM outputs
- [ ] Replace fixed buffers with dynamic allocation
- [ ] Add `is_safe_string()` and `sanitize_string()` utilities
- [ ] Add security unit tests for injection prevention

---

## P1 - High Priority (Post-Merge Sprint)

### Security Improvements

- [ ] MEDIUM-01: Fix memory leak in workflow state management (`src/workflow/state.c`)
- [ ] MEDIUM-02: Add input validation limits for node configuration (`src/workflow/graph.c`)
- [ ] MEDIUM-03: Add thread safety for concurrent workflow execution (`src/workflow/executor.c`)
- [ ] Implement fair agent selection with bias prevention (`src/workflow/group_chat.c`)
- [ ] Add ethical guardrails for multi-agent coordination
- [ ] Implement human-in-the-loop for sensitive operations

### Quality Assurance

- [ ] Add sanitizer tests to CI pipeline (ASan, UBSan, TSan)
- [ ] Add memory profiling under load
- [ ] Concurrent execution stress testing
- [ ] Fuzz testing for JSON parsing and condition evaluation

---

## P2 - Medium Priority (Backlog)

### From Security Audits

- [ ] LOW-01: Add security metadata to workflow schema
- [ ] LOW-02: Implement comprehensive security event logging
- [ ] Add workflow signature verification
- [ ] Security monitoring dashboard
- [ ] Automated security scanning in CI/CD

### From MASTER_PLAN.md

- [ ] Performance benchmarks vs Claude Code
- [ ] Codebase audit for remaining TODOs
- [ ] Documentation improvements

### Future Enhancements

- [ ] Workflow history UI visualization
- [ ] Advanced debugging tools for workflows
- [ ] Workflow version control and rollback
- [ ] Agent capability scoring system

---

## Completed Tasks

- [x] Core workflow engine implementation
- [x] DAG-based workflow definition
- [x] State machine with checkpointing
- [x] Multi-agent patterns (Router, Group Chat, Handoff)
- [x] Error handling with retry policies
- [x] Telemetry integration
- [x] Safe path operations (Phase 1 & 2)
- [x] Database migration for workflow tables
- [x] PR #72 review comments resolved
- [x] Sanitizer tests passed (23/23, 8/8, 18/18, 10/10)
- [x] Security audits completed (Luca + Guardian)
- [x] Extended telemetry events added
- [x] CMake build support added

---

## Security Audit Reports

Full security audit reports are available:

1. **Luca Security Audit**: `docs/security/WORKFLOW_SECURITY_AUDIT.md`
   - 1 CRITICAL, 2 HIGH, 3 MEDIUM, 2 LOW findings
   - OWASP Top 10 compliance assessment
   - CWE coverage analysis
   - Phased remediation roadmap

2. **Guardian AI Validation**: `docs/security/workflow-security-validation.md`
   - 4 CRITICAL, 3 HIGH, 2 MEDIUM findings
   - Responsible AI standards evaluation
   - Ethical concerns assessment
   - Testing recommendations

3. **Required Fixes**: `docs/security/SECURITY-FIXES-REQUIRED.md`
   - P0 implementation code examples
   - Security test suite requirements
   - Implementation checklist

---

## Merge Criteria

The PR can be merged when:

1. [ ] All P0 security fixes implemented
2. [ ] Security unit tests added and passing
3. [ ] Penetration testing for injection vulnerabilities
4. [ ] Re-validation by security audit
5. [ ] CI green (build + tests + sanitizers)

---

## Post-Merge: Execute Merge Plan

After security fixes are complete and PR #72 is ready, execute the merge plan:

**Document:** `/Users/roberdan/GitHub/ConvergioCLI/docs/plans/WORKFLOW_MERGE_PLAN.md`

**Why is this needed?** The workflow branch was developed BEFORE PR #73 (runtime edition switching), creating a critical conflict: the workflow branch DELETES the edition system. The merge plan ensures:

1. Edition system is preserved from main
2. Workflow features are properly integrated
3. All conflicts in main.c and Makefile are correctly resolved
4. Education-pack branch is rebased after merge

**Estimated Effort:** 2-4 hours

**Phases:**
1. Prepare workflow branch (merge main into it)
2. Resolve conflicts (edition.h, edition.c, main.c, Makefile)
3. Verify merged code (build all editions, run tests)
4. Create PR and merge
5. Rebase education-pack on new main

---

## Notes

- The feature branch is at `/Users/roberdan/GitHub/ConvergioCLI-workflow`
- Worktree was created for parallel development
- All workflow tests currently passing (sanitizer clean)
- Security audits recommend NOT deploying until P0 fixes complete

---

**Contact:** Roberto (with AI agent team)
**Last Audit:** 2025-12-21
