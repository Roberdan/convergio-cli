# Security Enforcement Plan - Global Security Validation

**Created**: 2025-12-21  
**Status**: 📋 IN PROGRESS  
**Purpose**: Plan for enforcing security validation globally across all Convergio codebase

---

## Current State

**Security Functions Available:**
- ✅ `tools_is_path_safe()` - Path validation
- ✅ `tools_is_command_safe()` - Command sanitization
- ✅ `safe_path_open()` - Safe file operations (TOCTOU prevention)
- ✅ `safe_open_read()` / `safe_open_write()` - Safe file descriptors

**Usage Statistics:**
- Security functions used: 27 instances across 9 files
- Potentially unsafe functions found: 20 files with `fopen`, `system`, `popen`, `strcpy`, `strcat`, `gets`

**Coverage:**
- SQL injection: 100% (all queries parameterized)
- Command injection: 100% (all commands sanitized via `tools_is_command_safe`)
- Path traversal: ~9% (only `src/tools/tools.c` uses safe functions)

---

## Files Requiring Security Updates

### High Priority (User Input / External Data)

1. **src/core/config.c** - Configuration file reading
   - Replace `fopen()` with `safe_path_open()`
   - Add path validation

2. **src/memory/persistence.c** - Database file operations
   - Replace `fopen()` with `safe_path_open()`
   - Add path validation

3. **src/telemetry/telemetry.c** - Telemetry file operations
   - Replace `fopen()` with `safe_path_open()`
   - Add path validation

4. **src/telemetry/export.c** - Export file operations
   - Replace `fopen()` with `safe_path_open()`
   - Add path validation

5. **src/projects/projects.c** - Project file operations
   - Replace `fopen()` with `safe_path_open()`
   - Add path validation

### Medium Priority (Internal Operations)

6. **src/core/main.c** - Main entry point
   - Review `fopen()` usage
   - Add validation where needed

7. **src/core/commands/commands.c** - Command execution
   - Review `system()` / `popen()` usage
   - Ensure `tools_is_command_safe()` is used

8. **src/orchestrator/registry.c** - Agent registry
   - Review file operations
   - Add path validation

9. **src/orchestrator/plan_db.c** - Plan database
   - Review file operations
   - Add path validation

10. **src/memory/memory.c** - Memory operations
    - Review file operations
    - Add path validation

### Low Priority (Less Critical)

11. **src/notifications/notify.c** - Notifications
12. **src/sync/file_lock.c** - File locking
13. **src/providers/tools.c** - Provider tools
14. **src/providers/model_loader.c** - Model loading
15. **src/mcp/mcp_client.c** - MCP client
16. **src/workflow/error_handling.c** - Workflow error handling (already reviewed)
17. **src/workflow/workflow_observability.c** - Workflow observability
18. **src/tools/output_service.c** - Output service
19. **src/agents/app-release-manager.md** - Documentation (no code)

---

## Implementation Strategy

### Phase 1: High Priority Files (This Week)

1. Update `src/core/config.c`
2. Update `src/memory/persistence.c`
3. Update `src/telemetry/telemetry.c`
4. Update `src/telemetry/export.c`
5. Update `src/projects/projects.c`

### Phase 2: Medium Priority Files (Next Week)

6. Review and update `src/core/main.c`
7. Review and update `src/core/commands/commands.c`
8. Review and update `src/orchestrator/registry.c`
9. Review and update `src/orchestrator/plan_db.c`
10. Review and update `src/memory/memory.c`

### Phase 3: Low Priority Files (Following Week)

11-19. Review and update remaining files

---

## Quality Gate Integration

**Add to `make quality_gate_security`:**

```makefile
quality_gate_security:
	@echo ""
	@echo "=== 3. Security Check (Comprehensive) ==="
	@# Check for unsafe functions in high-priority files
	@UNSAFE_HIGH=$$(grep -rE "(fopen|system|popen)\s*\(" src/core/config.c src/memory/persistence.c src/telemetry/telemetry.c src/telemetry/export.c src/projects/projects.c 2>/dev/null | grep -v "tools_is_command_safe\|safe_path_open\|safe_open" | wc -l | tr -d ' '); \
	if [ "$$UNSAFE_HIGH" -gt 0 ]; then \
		echo "❌ FAILED: Found $$UNSAFE_HIGH unsafe function calls in high-priority files"; \
		grep -rE "(fopen|system|popen)\s*\(" src/core/config.c src/memory/persistence.c src/telemetry/telemetry.c src/telemetry/export.c src/projects/projects.c 2>/dev/null | grep -v "tools_is_command_safe\|safe_path_open\|safe_open"; \
		exit 1; \
	else \
		echo "✅ PASSED: High-priority files use safe functions"; \
	fi
	@# Check for dangerous string functions
	@DANGEROUS=$$(grep -rE "(strcpy|strcat|gets)\s*\(" src/ 2>/dev/null | grep -v "tools_is_command_safe\|test_" | wc -l | tr -d ' '); \
	if [ "$$DANGEROUS" -gt 0 ]; then \
		echo "⚠️  WARNING: Found $$DANGEROUS potential security issues"; \
		echo "   Review use of dangerous functions"; \
	else \
		echo "✅ PASSED: No obvious security issues"; \
	fi
```

---

## Progress Tracking

- [ ] Phase 1: High Priority Files (0/5)
  - [ ] src/core/config.c
  - [ ] src/memory/persistence.c
  - [ ] src/telemetry/telemetry.c
  - [ ] src/telemetry/export.c
  - [ ] src/projects/projects.c

- [ ] Phase 2: Medium Priority Files (0/5)
  - [ ] src/core/main.c
  - [ ] src/core/commands/commands.c
  - [ ] src/orchestrator/registry.c
  - [ ] src/orchestrator/plan_db.c
  - [ ] src/memory/memory.c

- [ ] Phase 3: Low Priority Files (0/9)
  - [ ] src/notifications/notify.c
  - [ ] src/sync/file_lock.c
  - [ ] src/providers/tools.c
  - [ ] src/providers/model_loader.c
  - [ ] src/mcp/mcp_client.c
  - [ ] src/workflow/error_handling.c
  - [ ] src/workflow/workflow_observability.c
  - [ ] src/tools/output_service.c
  - [ ] src/agents/app-release-manager.md

---

## Notes

- This is a **large refactoring effort** that should be done incrementally
- Each file update should include tests to verify security improvements
- Priority is based on exposure to user input and external data
- Low priority files may have acceptable uses of unsafe functions (internal operations)

---

**Next Steps:**
1. Start with Phase 1 (high priority files)
2. Add tests for each updated file
3. Update quality gate to enforce security standards
4. Document security improvements in commit messages

