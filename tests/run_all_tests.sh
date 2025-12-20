#!/bin/bash
#
# Convergio Comprehensive Test Suite
# Runs ALL tests for ALL editions with ZERO TOLERANCE for failures
#
# Usage: ./tests/run_all_tests.sh [--parallel] [--edition education|business|developer|full]
#
# Exit codes:
#   0 = All tests passed
#   1 = Tests failed
#   2 = Build failed
#   3 = Test script error
#

set -o pipefail

# Configuration
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RESULTS_DIR="/tmp/convergio_test_results_$$"
PARALLEL=false
EDITION="all"
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --parallel) PARALLEL=true; shift ;;
        --edition) EDITION="$2"; shift 2 ;;
        --verbose) VERBOSE=true; shift ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --parallel     Run independent test groups in parallel"
            echo "  --edition X    Test specific edition (education|business|developer|full|all)"
            echo "  --verbose      Show detailed output"
            echo "  -h, --help     Show this help"
            exit 0
            ;;
        *) echo "Unknown option: $1"; exit 3 ;;
    esac
done

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# Counters
TOTAL_PASSED=0
TOTAL_FAILED=0
TOTAL_WARNINGS=0
declare -a FAILED_TESTS=()
declare -a WARNINGS=()

# Create results directory
mkdir -p "$RESULTS_DIR"

# Logging
log() { echo -e "${CYAN}[$(date +%H:%M:%S)]${NC} $*"; }
log_pass() { echo -e "${GREEN}[PASS]${NC} $*"; ((TOTAL_PASSED++)); }
log_fail() {
    echo -e "${RED}[FAIL]${NC} $*"
    ((TOTAL_FAILED++))
    FAILED_TESTS+=("$*")
}
log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
    ((TOTAL_WARNINGS++))
    WARNINGS+=("$*")
}

# ============================================================================
# PHASE 1: BUILD ALL EDITIONS
# ============================================================================
build_edition() {
    local edition="$1"
    local binary_name="$2"
    local log_file="$RESULTS_DIR/build_${edition}.log"

    log "Building ${edition} edition..."
    cd "$PROJECT_ROOT"

    if [ "$edition" = "full" ]; then
        make clean >/dev/null 2>&1
        if make DEBUG=1 2>&1 | tee "$log_file"; then
            # Check for warnings
            local warnings=$(grep -c "warning:" "$log_file" 2>/dev/null || echo "0")
            if [ "$warnings" -gt 0 ]; then
                log_fail "Build $edition: $warnings compiler warnings found"
                grep "warning:" "$log_file" | head -10 >> "$RESULTS_DIR/all_issues.txt"
                return 1
            else
                log_pass "Build $edition: Success with 0 warnings"
                return 0
            fi
        else
            log_fail "Build $edition: Compilation failed"
            return 1
        fi
    else
        make clean >/dev/null 2>&1
        if make EDITION="$edition" DEBUG=1 2>&1 | tee "$log_file"; then
            local warnings=$(grep -c "warning:" "$log_file" 2>/dev/null || echo "0")
            if [ "$warnings" -gt 0 ]; then
                log_fail "Build $edition: $warnings compiler warnings found"
                grep "warning:" "$log_file" | head -10 >> "$RESULTS_DIR/all_issues.txt"
                return 1
            else
                log_pass "Build $edition: Success with 0 warnings"
                return 0
            fi
        else
            log_fail "Build $edition: Compilation failed"
            return 1
        fi
    fi
}

# ============================================================================
# PHASE 2: UNIT TESTS
# ============================================================================
run_unit_tests() {
    log "Running unit tests..."
    local log_file="$RESULTS_DIR/unit_tests.log"

    cd "$PROJECT_ROOT"
    if make test 2>&1 | tee "$log_file"; then
        local failures=$(grep -cE "FAIL|FAILED|Error" "$log_file" 2>/dev/null || echo "0")
        if [ "$failures" -gt 0 ]; then
            log_fail "Unit tests: $failures failures found"
            grep -E "FAIL|FAILED|Error" "$log_file" >> "$RESULTS_DIR/all_issues.txt"
            return 1
        else
            log_pass "Unit tests: All passed"
            return 0
        fi
    else
        log_fail "Unit tests: Execution failed"
        return 1
    fi
}

# ============================================================================
# PHASE 3: E2E TESTS PER EDITION
# ============================================================================
run_e2e_tests() {
    local edition="$1"
    log "Running E2E tests for $edition edition..."
    local log_file="$RESULTS_DIR/e2e_${edition}.log"

    cd "$PROJECT_ROOT"

    case "$edition" in
        education)
            if [ -x "tests/e2e_education_test.sh" ]; then
                if ./tests/e2e_education_test.sh 2>&1 | tee "$log_file"; then
                    # Parse results
                    local passed=$(grep -oE "PASSED.*[0-9]+" "$log_file" | grep -oE "[0-9]+" | tail -1 || echo "0")
                    local failed=$(grep -oE "FAILED.*[0-9]+" "$log_file" | grep -oE "[0-9]+" | tail -1 || echo "0")

                    if [ "$failed" -gt 0 ]; then
                        log_fail "E2E $edition: $failed tests failed"
                        grep -E "FAIL|❌" "$log_file" >> "$RESULTS_DIR/all_issues.txt"
                        return 1
                    else
                        log_pass "E2E $edition: $passed tests passed"
                        return 0
                    fi
                else
                    log_fail "E2E $edition: Test script failed"
                    return 1
                fi
            else
                log_warn "E2E $edition: Test script not found"
                return 0
            fi
            ;;
        full|master)
            if [ -x "tests/e2e_test.sh" ]; then
                if ./tests/e2e_test.sh 2>&1 | tee "$log_file"; then
                    local failed=$(grep -oE "FAILED:.*[0-9]+" "$log_file" | grep -oE "[0-9]+" | tail -1 || echo "0")

                    if [ "$failed" -gt 0 ]; then
                        log_fail "E2E $edition: $failed tests failed"
                        grep "FAIL" "$log_file" >> "$RESULTS_DIR/all_issues.txt"
                        return 1
                    else
                        log_pass "E2E $edition: All tests passed"
                        return 0
                    fi
                else
                    log_fail "E2E $edition: Test script failed"
                    return 1
                fi
            else
                log_warn "E2E $edition: Test script not found"
                return 0
            fi
            ;;
        *)
            log_warn "E2E $edition: No specific tests defined"
            return 0
            ;;
    esac
}

# ============================================================================
# PHASE 4: ACP TESTS (if applicable)
# ============================================================================
run_acp_tests() {
    log "Running ACP protocol tests..."
    local log_file="$RESULTS_DIR/acp_tests.log"

    cd "$PROJECT_ROOT"

    if [ -x "tests/test_acp_e2e.sh" ] && [ -x "build/bin/convergio-acp" ]; then
        if ./tests/test_acp_e2e.sh 2>&1 | tee "$log_file"; then
            local failed=$(grep -oE "[0-9]+ failed" "$log_file" | grep -oE "[0-9]+" | head -1 || echo "0")

            if [ "$failed" -gt 0 ]; then
                log_fail "ACP tests: $failed tests failed"
                grep "FAIL" "$log_file" >> "$RESULTS_DIR/all_issues.txt"
                return 1
            else
                log_pass "ACP tests: All passed"
                return 0
            fi
        else
            log_fail "ACP tests: Execution failed"
            return 1
        fi
    else
        log_warn "ACP tests: Skipped (binary not found or no test script)"
        return 0
    fi
}

# ============================================================================
# PHASE 5: CODE QUALITY CHECKS
# ============================================================================
run_code_quality() {
    log "Running code quality checks..."
    local issues=0

    cd "$PROJECT_ROOT"

    # Check for TODO/FIXME
    local todos=$(rg "TODO|FIXME|XXX|HACK" --type c --type objc src/ include/ 2>/dev/null | wc -l | tr -d ' ')
    if [ "$todos" -gt 0 ]; then
        log_fail "Code quality: $todos TODO/FIXME comments found"
        rg "TODO|FIXME|XXX|HACK" --type c --type objc src/ include/ 2>/dev/null | head -10 >> "$RESULTS_DIR/all_issues.txt"
        ((issues++))
    else
        log_pass "Code quality: No TODO/FIXME comments"
    fi

    # Check for debug prints (excluding debug_mutex and actual debug tools)
    local debug_prints=$(rg 'printf.*DEBUG|NSLog.*debug|fprintf.*stderr.*DEBUG' --type c --type objc src/ 2>/dev/null | grep -v "debug_mutex" | grep -v "src/core/main.c" | wc -l | tr -d ' ')
    if [ "$debug_prints" -gt 0 ]; then
        log_fail "Code quality: $debug_prints debug print statements found"
        ((issues++))
    else
        log_pass "Code quality: No debug print statements"
    fi

    # Check for commented-out code blocks
    local commented=$(rg "^//.*\{$|^//.*\}$" --type c src/ 2>/dev/null | wc -l | tr -d ' ')
    if [ "$commented" -gt 5 ]; then
        log_fail "Code quality: $commented commented-out code blocks found"
        ((issues++))
    else
        log_pass "Code quality: Minimal commented-out code"
    fi

    return $issues
}

# ============================================================================
# PHASE 6: SECURITY CHECKS
# ============================================================================
run_security_checks() {
    log "Running security checks..."
    local issues=0

    cd "$PROJECT_ROOT"

    # Check for hardcoded secrets
    local secrets=$(rg -i "password\s*=|secret\s*=|api.key\s*=|sk-ant-" --type c --type objc src/ 2>/dev/null | grep -v "example\|sample\|test\|demo" | wc -l | tr -d ' ')
    if [ "$secrets" -gt 0 ]; then
        log_fail "Security: $secrets potential hardcoded secrets found"
        ((issues++))
    else
        log_pass "Security: No hardcoded secrets detected"
    fi

    # Check for unsafe functions
    local unsafe=$(rg "strcpy\(|strcat\(|sprintf\(|gets\(" --type c src/ 2>/dev/null | wc -l | tr -d ' ')
    if [ "$unsafe" -gt 0 ]; then
        log_warn "Security: $unsafe unsafe C functions found (review recommended)"
    else
        log_pass "Security: No unsafe C functions"
    fi

    # Check .gitignore for sensitive files
    if ! grep -q "\.env" .gitignore 2>/dev/null; then
        log_warn "Security: .env not in .gitignore"
    fi

    return $issues
}

# ============================================================================
# PHASE 7: MEMORY SAFETY CHECKS
# ============================================================================
run_memory_checks() {
    log "Running memory safety checks..."
    local issues=0

    cd "$PROJECT_ROOT"

    # Check for missing NULL checks after malloc
    local missing_null=$(rg "malloc|calloc" -A1 --type c src/ 2>/dev/null | grep -v "if.*NULL\|if.*!\|== NULL" | grep -c "malloc\|calloc" || echo "0")
    if [ "$missing_null" -gt 5 ]; then
        log_warn "Memory: $missing_null allocations may be missing NULL checks"
    else
        log_pass "Memory: NULL checks look adequate"
    fi

    # Check for raw pthread usage
    local raw_pthread=$(rg "pthread_mutex_lock|pthread_mutex_unlock" --type c src/ 2>/dev/null | grep -v "debug_mutex" | wc -l | tr -d ' ')
    if [ "$raw_pthread" -gt 0 ]; then
        log_fail "Memory: $raw_pthread raw pthread calls (use debug_mutex wrappers)"
        ((issues++))
    else
        log_pass "Memory: Using debug_mutex wrappers correctly"
    fi

    return $issues
}

# ============================================================================
# MAIN EXECUTION
# ============================================================================
main() {
    echo ""
    echo "╔════════════════════════════════════════════════════════════════════════════╗"
    echo "║           CONVERGIO COMPREHENSIVE TEST SUITE - ZERO TOLERANCE              ║"
    echo "╠════════════════════════════════════════════════════════════════════════════╣"
    echo "║  Mode: $(printf '%-69s' "$($PARALLEL && echo 'PARALLEL' || echo 'SEQUENTIAL')")║"
    echo "║  Edition: $(printf '%-66s' "$EDITION")║"
    echo "║  Results: $(printf '%-66s' "$RESULTS_DIR")║"
    echo "╚════════════════════════════════════════════════════════════════════════════╝"
    echo ""

    START_TIME=$(date +%s)

    # Determine which editions to test
    local editions_to_test=()
    case "$EDITION" in
        all)
            editions_to_test=(full education business developer)
            ;;
        *)
            editions_to_test=("$EDITION")
            ;;
    esac

    # ========================================
    # PHASE 1: BUILD
    # ========================================
    echo ""
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${BLUE}  PHASE 1: BUILD ALL EDITIONS${NC}"
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    for ed in "${editions_to_test[@]}"; do
        build_edition "$ed" "convergio-${ed}"
    done

    # ========================================
    # PHASE 2: UNIT TESTS
    # ========================================
    echo ""
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${BLUE}  PHASE 2: UNIT TESTS${NC}"
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    run_unit_tests

    # ========================================
    # PHASE 3: E2E TESTS
    # ========================================
    echo ""
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${BLUE}  PHASE 3: E2E TESTS${NC}"
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    for ed in "${editions_to_test[@]}"; do
        # Build the edition first if needed
        if [ "$ed" != "full" ]; then
            make clean >/dev/null 2>&1
            make EDITION="$ed" >/dev/null 2>&1
        fi
        run_e2e_tests "$ed"
    done

    # ========================================
    # PHASE 4: ACP TESTS
    # ========================================
    echo ""
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${BLUE}  PHASE 4: ACP PROTOCOL TESTS${NC}"
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    run_acp_tests

    # ========================================
    # PHASE 5: CODE QUALITY
    # ========================================
    echo ""
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${BLUE}  PHASE 5: CODE QUALITY${NC}"
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    run_code_quality

    # ========================================
    # PHASE 6: SECURITY
    # ========================================
    echo ""
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${BLUE}  PHASE 6: SECURITY CHECKS${NC}"
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    run_security_checks

    # ========================================
    # PHASE 7: MEMORY SAFETY
    # ========================================
    echo ""
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${BLUE}  PHASE 7: MEMORY SAFETY${NC}"
    echo -e "${BOLD}${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    run_memory_checks

    # ========================================
    # FINAL REPORT
    # ========================================
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))

    echo ""
    echo "╔════════════════════════════════════════════════════════════════════════════╗"
    echo "║                           FINAL TEST REPORT                                ║"
    echo "╠════════════════════════════════════════════════════════════════════════════╣"
    printf "║  Duration: %-65s ║\n" "${DURATION}s"
    printf "║  ${GREEN}PASSED${NC}:  %-66s ║\n" "$TOTAL_PASSED"
    printf "║  ${RED}FAILED${NC}:  %-66s ║\n" "$TOTAL_FAILED"
    printf "║  ${YELLOW}WARNINGS${NC}: %-65s ║\n" "$TOTAL_WARNINGS"
    echo "╠════════════════════════════════════════════════════════════════════════════╣"

    if [ "$TOTAL_FAILED" -eq 0 ]; then
        echo "║                                                                            ║"
        echo -e "║                    ${GREEN}${BOLD}✅ ALL TESTS PASSED - READY FOR RELEASE${NC}                   ║"
        echo "║                                                                            ║"
        echo "╚════════════════════════════════════════════════════════════════════════════╝"

        # Generate success report
        {
            echo "# Convergio Test Report - $(date)"
            echo ""
            echo "## Summary: ✅ ALL TESTS PASSED"
            echo ""
            echo "- **Passed**: $TOTAL_PASSED"
            echo "- **Failed**: 0"
            echo "- **Warnings**: $TOTAL_WARNINGS"
            echo "- **Duration**: ${DURATION}s"
            echo ""
            if [ "$TOTAL_WARNINGS" -gt 0 ]; then
                echo "## Warnings (non-blocking)"
                for w in "${WARNINGS[@]}"; do
                    echo "- $w"
                done
            fi
        } > "$RESULTS_DIR/final_report.md"

        exit 0
    else
        echo "║                                                                            ║"
        echo -e "║                  ${RED}${BOLD}🔴 TESTS FAILED - RELEASE BLOCKED${NC}                       ║"
        echo "║                                                                            ║"
        echo "╠════════════════════════════════════════════════════════════════════════════╣"
        echo "║  Failed tests:                                                             ║"
        for test in "${FAILED_TESTS[@]}"; do
            printf "║    • %-70s ║\n" "${test:0:70}"
        done
        echo "╚════════════════════════════════════════════════════════════════════════════╝"

        # Generate failure report
        {
            echo "# Convergio Test Report - $(date)"
            echo ""
            echo "## Summary: 🔴 RELEASE BLOCKED"
            echo ""
            echo "- **Passed**: $TOTAL_PASSED"
            echo "- **Failed**: $TOTAL_FAILED"
            echo "- **Warnings**: $TOTAL_WARNINGS"
            echo "- **Duration**: ${DURATION}s"
            echo ""
            echo "## Failed Tests (BLOCKING)"
            for t in "${FAILED_TESTS[@]}"; do
                echo "- ❌ $t"
            done
            echo ""
            if [ "$TOTAL_WARNINGS" -gt 0 ]; then
                echo "## Warnings"
                for w in "${WARNINGS[@]}"; do
                    echo "- ⚠️ $w"
                done
            fi
            echo ""
            echo "## Detailed Issues"
            if [ -f "$RESULTS_DIR/all_issues.txt" ]; then
                echo '```'
                cat "$RESULTS_DIR/all_issues.txt"
                echo '```'
            fi
        } > "$RESULTS_DIR/final_report.md"

        echo ""
        echo -e "${RED}Full report: $RESULTS_DIR/final_report.md${NC}"
        exit 1
    fi
}

main "$@"
