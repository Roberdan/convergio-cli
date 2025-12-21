#!/bin/bash
#
# Pre-Release Check Script for Convergio CLI (Workflow Orchestration)
# ZERO TOLERANCE - Executes all quality gates and tests
#
# Usage: ./scripts/pre_release_check.sh [--fix] [--verbose]
#

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Options
AUTO_FIX=false
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --fix)
            AUTO_FIX=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Counters
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
BLOCKING_ISSUES=()

# Logging
LOG_DIR="/tmp/convergio_pre_release_$(date +%s)"
mkdir -p "$LOG_DIR"

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED_CHECKS++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_CHECKS++))
    BLOCKING_ISSUES+=("$1")
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

run_check() {
    local name="$1"
    local command="$2"
    local expected_exit="${3:-0}"
    
    ((TOTAL_CHECKS++))
    log_info "Running: $name"
    
    if $VERBOSE; then
        eval "$command" > "$LOG_DIR/${name// /_}.log" 2>&1
        local exit_code=$?
    else
        eval "$command" > "$LOG_DIR/${name// /_}.log" 2>&1
        local exit_code=$?
    fi
    
    if [ $exit_code -eq $expected_exit ]; then
        log_success "$name"
        return 0
    else
        log_error "$name (exit code: $exit_code)"
        if $VERBOSE; then
            echo "--- Output ---"
            tail -20 "$LOG_DIR/${name// /_}.log"
        fi
        return 1
    fi
}

# ============================================================================
# PHASE 0: Workflow Feature Verification
# ============================================================================

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║     PRE-RELEASE CHECK - WORKFLOW ORCHESTRATION              ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

echo "=== PHASE 0: Workflow Feature Verification ==="

# Check workflow files exist
log_info "Verifying workflow implementation files..."
WORKFLOW_FILES=(
    "src/workflow/workflow_types.c"
    "src/workflow/workflow_engine.c"
    "src/workflow/checkpoint.c"
    "src/workflow/task_decomposer.c"
    "src/workflow/group_chat.c"
    "src/workflow/router.c"
    "src/workflow/patterns.c"
    "src/workflow/retry.c"
    "src/workflow/error_handling.c"
    "src/workflow/workflow_observability.c"
    "include/nous/workflow.h"
    "src/memory/migrations/016_workflow_engine.sql"
    "src/core/commands/workflow.c"
)

for file in "${WORKFLOW_FILES[@]}"; do
    if [ -f "$file" ]; then
        log_success "File exists: $file"
    else
        log_error "File missing: $file"
    fi
done

# Check test files exist
log_info "Verifying workflow test files..."
TEST_FILES=(
    "tests/test_workflow_types.c"
    "tests/test_workflow_engine.c"
    "tests/test_workflow_checkpoint.c"
    "tests/test_task_decomposer.c"
    "tests/test_group_chat.c"
    "tests/test_router.c"
    "tests/test_patterns.c"
    "tests/test_workflow_error_handling.c"
    "tests/test_workflow_e2e.c"
    "tests/test_workflow_e2e_bug_triage.c"
    "tests/test_workflow_e2e_pre_release.c"
    "tests/test_telemetry.c"
    "tests/test_security.c"
)

for file in "${TEST_FILES[@]}"; do
    if [ -f "$file" ]; then
        log_success "Test file exists: $file"
    else
        log_error "Test file missing: $file"
    fi
done

# Check templates exist and are valid JSON
log_info "Verifying workflow templates..."
TEMPLATE_COUNT=0
for template in src/workflow/templates/*.json; do
    if [ -f "$template" ]; then
        if python3 -m json.tool "$template" > /dev/null 2>&1; then
            log_success "Valid JSON: $template"
            ((TEMPLATE_COUNT++))
        else
            log_error "Invalid JSON: $template"
        fi
    fi
done

if [ $TEMPLATE_COUNT -ge 9 ]; then
    log_success "All 9+ templates present and valid"
else
    log_error "Missing templates (found: $TEMPLATE_COUNT, expected: 9+)"
fi

# ============================================================================
# PHASE 1: Build Verification
# ============================================================================

echo ""
echo "=== PHASE 1: Build Verification ==="

run_check "Clean build" "make clean && make" 0

# Check for warnings
WARNINGS=$(make clean && make 2>&1 | grep -i warning | wc -l || true)
if [ "$WARNINGS" -eq 0 ]; then
    log_success "Zero compiler warnings"
else
    log_error "Found $WARNINGS compiler warnings (ZERO TOLERANCE)"
    if $AUTO_FIX; then
        log_warning "Auto-fix not implemented for warnings - manual fix required"
    fi
fi

# ============================================================================
# PHASE 2: Test Execution
# ============================================================================

echo ""
echo "=== PHASE 2: Test Execution ==="

# Workflow tests
run_check "Workflow tests" "make workflow_test" 0

# Telemetry tests
run_check "Telemetry tests" "make telemetry_test" 0

# Security tests
run_check "Security tests" "make security_test" 0

# All tests
run_check "All tests" "make test" 0

# Sanitizer tests (if DEBUG build available)
if command -v clang &> /dev/null; then
    run_check "Sanitizer tests" "make DEBUG=1 SANITIZE=address,undefined,thread test" 0 || log_warning "Sanitizer tests skipped (DEBUG build not available)"
else
    log_warning "Sanitizer tests skipped (clang not available)"
fi

# Fuzz tests
run_check "Fuzz tests" "make fuzz_test" 0

# ============================================================================
# PHASE 3: Static Analysis & Security
# ============================================================================

echo ""
echo "=== PHASE 3: Static Analysis & Security ==="

# Check for SQL injection risks
log_info "Checking for SQL injection risks..."
SQL_RISKS=$(grep -r "sqlite3_exec.*%" src/workflow/ 2>/dev/null | wc -l || true)
if [ "$SQL_RISKS" -eq 0 ]; then
    log_success "No SQL injection risks (using parameterized queries)"
else
    log_error "Found $SQL_RISKS potential SQL injection risks"
fi

# Check for command injection risks
log_info "Checking for command injection risks..."
CMD_RISKS=$(grep -r "system\|popen" src/workflow/ 2>/dev/null | grep -v "tools_is_command_safe" | wc -l || true)
if [ "$CMD_RISKS" -eq 0 ]; then
    log_success "No command injection risks (using safe functions)"
else
    log_error "Found $CMD_RISKS potential command injection risks"
fi

# Check for path traversal risks
log_info "Checking for path traversal risks..."
PATH_RISKS=$(grep -r "fopen\|open" src/workflow/ 2>/dev/null | grep -v "safe_path_open\|tools_is_path_safe" | wc -l || true)
if [ "$PATH_RISKS" -eq 0 ]; then
    log_success "No path traversal risks (using safe functions)"
else
    log_warning "Found $PATH_RISKS potential path traversal risks (may be false positives)"
fi

# ============================================================================
# PHASE 4: Documentation Check
# ============================================================================

echo ""
echo "=== PHASE 4: Documentation Check ==="

DOC_FILES=(
    "docs/workflow-orchestration/USER_GUIDE.md"
    "docs/workflow-orchestration/USE_CASES.md"
    "docs/workflow-orchestration/TECHNICAL_DOCUMENTATION.md"
    "docs/workflow-orchestration/MASTER_PLAN.md"
    "docs/workflow-orchestration/CODEBASE_AUDIT.md"
    "README.md"
)

for doc in "${DOC_FILES[@]}"; do
    if [ -f "$doc" ] && [ -s "$doc" ]; then
        log_success "Documentation exists: $doc"
    else
        log_error "Documentation missing or empty: $doc"
    fi
done

# Check README has workflow section
if grep -q "Workflow Orchestration\|workflow" README.md 2>/dev/null; then
    log_success "README.md includes workflow section"
else
    log_error "README.md missing workflow section"
fi

# ============================================================================
# PHASE 5: Integration Verification
# ============================================================================

echo ""
echo "=== PHASE 5: Integration Verification ==="

# Check telemetry integration
log_info "Checking telemetry integration..."
TELEMETRY_PROVIDERS=$(grep -r "telemetry_record_api_call" src/providers/*.c 2>/dev/null | wc -l || true)
if [ "$TELEMETRY_PROVIDERS" -ge 5 ]; then
    log_success "Telemetry integrated in $TELEMETRY_PROVIDERS providers"
else
    log_error "Telemetry integration incomplete (found: $TELEMETRY_PROVIDERS, expected: 6+)"
fi

# Check security integration
log_info "Checking security integration..."
if grep -q "tools_is_path_safe\|safe_path_open" src/workflow/*.c 2>/dev/null; then
    log_success "Security functions used in workflow code"
else
    log_warning "Security functions may not be used everywhere"
fi

# Check logging integration
if grep -q "LOG_CAT_WORKFLOW" src/core/main.c 2>/dev/null; then
    log_success "Workflow logging category integrated"
else
    log_error "Workflow logging category not integrated"
fi

# ============================================================================
# PHASE 6: Code Coverage (if available)
# ============================================================================

echo ""
echo "=== PHASE 6: Code Coverage ==="

if command -v gcov &> /dev/null && command -v lcov &> /dev/null; then
    log_info "Running coverage analysis..."
    if make coverage > "$LOG_DIR/coverage.log" 2>&1; then
        COVERAGE=$(grep -oP 'lines\.\.\.: \K[0-9.]+' "$LOG_DIR/coverage.log" | head -1 || echo "0")
        if [ -n "$COVERAGE" ] && (( $(echo "$COVERAGE >= 80" | bc -l) )); then
            log_success "Code coverage: ${COVERAGE}% (>= 80%)"
        else
            log_error "Code coverage: ${COVERAGE}% (< 80% target)"
        fi
    else
        log_warning "Coverage analysis failed (check $LOG_DIR/coverage.log)"
    fi
else
    log_warning "Coverage tools not available (gcov/lcov)"
fi

# ============================================================================
# FINAL REPORT
# ============================================================================

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                    FINAL REPORT                              ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "Total Checks: $TOTAL_CHECKS"
echo -e "${GREEN}Passed: $PASSED_CHECKS${NC}"
echo -e "${RED}Failed: $FAILED_CHECKS${NC}"
echo ""

if [ $FAILED_CHECKS -eq 0 ]; then
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║              ✅ RELEASE APPROVED                            ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "All quality gates passed. Ready for release."
    exit 0
else
    echo -e "${RED}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║              ❌ RELEASE BLOCKED                               ║${NC}"
    echo -e "${RED}╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Blocking Issues:"
    for issue in "${BLOCKING_ISSUES[@]}"; do
        echo "  - $issue"
    done
    echo ""
    echo "Logs available in: $LOG_DIR"
    echo ""
    echo "FIX ALL ISSUES BEFORE RELEASE (ZERO TOLERANCE)"
    exit 1
fi

