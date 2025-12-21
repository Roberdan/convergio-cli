# Convergio E2E Test Suite

Run comprehensive end-to-end tests on the Convergio CLI to verify all commands and functionality work correctly.

## Instructions

Execute the complete test suite for Convergio CLI and report results:

### Step 1: Run Unit Tests
```bash
make test
```
This runs: fuzz_test, unit_test, anna_test, compaction_test, plan_db_test, output_service_test, education_test

### Step 2: Run Education Pack Tests (BLOCKING)
```bash
make education_test
```
**MUST pass with 9/9 tests:**
- Mario (16, multi-disability: dyslexia + CP + dyscalculia)
- Sofia (14, ADHD combined)
- Luca (17, autism high-functioning)
- Giulia (15, baseline - no disabilities)
- Goal management
- Curriculum loading
- 14 Maestri verification

### Step 3: Run E2E Tests
```bash
./tests/e2e_test.sh
```

### Step 4: Run Education Edition E2E Tests
```bash
./tests/e2e_education_test.sh
```
**MUST pass with >= 95% success rate**

### Step 5: Run Education LLM Natural Language Tests (BLOCKING)
```bash
./tests/education_llm_test.sh
```
**Tests natural conversation quality:**
- Ali Preside identity (school principal, not corporate)
- Pedagogical approach (Maieutic method, growth mindset)
- Emotional support (handles frustration, anxiety)
- Accessibility adaptations (ADHD, dyslexia, visual learning)
- Safety guardrails (appropriate content, prompt injection)
- Multi-teacher delegation (correct subjects to correct maestri)
- Age-appropriate communication

### Step 6: Analyze results and report:
   - Number of tests passed/failed/skipped for each category
   - Any failing tests and their errors
   - Recommendations for fixes

### Step 5: If tests fail, investigate the root cause by:
   - Reading relevant source code
   - Checking recent changes
   - Identifying the specific failure point

## Test Categories

### Unit Tests (make test)
- Fuzz tests: Input fuzzing and edge cases
- Unit tests: Core functionality
- Anna tests: Assistant integration
- Compaction tests: Context management
- Plan DB tests: Planning database
- Output service tests: Output formatting
- **Education tests**: School scenarios with accessibility

### E2E Tests (e2e_test.sh)
- Basic commands (help, status, hardware, cost)
- Agent management (list, info, edit, reload)
- Tool system (check, install)
- Real API tests (chat, shell_exec, file operations)
- Agent delegation and communication

## Expected Output

Report should include:
- Unit test pass rate
- Education test results: MUST be 9/9 passed
- E2E test pass rate
- List of any failures with details
- Recommendations for fixes

## BLOCKING Conditions

❌ **NO RELEASE** if any of these fail:
- Education tests < 9/9 passed
- Any unit test failure
- Any E2E test failure
- Education E2E tests < 95% pass rate
- Education LLM tests with ANY failure
- Safety guardrail failures (prompt injection, inappropriate content)

## Integration with app-release-manager

When preparing a release, the app-release-manager agent MUST:
1. Run all test suites in order
2. Verify all blocking conditions pass
3. Generate test summary report
4. Block release if any critical test fails

The education_llm_test.sh script tests REAL conversations with Ali and the Maestri, evaluating responses for pedagogical quality, emotional support, and safety.
