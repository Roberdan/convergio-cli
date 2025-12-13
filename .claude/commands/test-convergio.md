# Convergio E2E Test Suite

Run comprehensive end-to-end tests on the Convergio CLI to verify all commands and functionality work correctly.

## Instructions

Execute the E2E test suite for Convergio CLI and report results:

1. First, run the automated test suite:
```bash
./tests/e2e_test.sh
```

2. Analyze the results and report:
   - Number of tests passed/failed/skipped
   - Any failing tests and their errors
   - Recommendations for fixes

3. If tests fail, investigate the root cause by:
   - Reading relevant source code
   - Checking recent changes
   - Identifying the specific failure point

## Test Categories

The E2E tests cover:
- Basic commands (help, status, hardware, cost)
- Agent management (list, info, edit, reload)
- Tool system (check, install)
- Real API tests (chat, shell_exec, file operations)
- Agent delegation and communication

## Expected Output

Report should include:
- Pass rate percentage
- List of any failures with details
- Recommendations for fixes
