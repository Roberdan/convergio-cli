#!/bin/bash
#
# Convergio CLI E2E Test Suite
# Tests all commands simulating technical and business users
#
# Usage: ./tests/e2e_test.sh
#

# Don't exit on error - we handle errors ourselves
set +e

CONVERGIO="./build/bin/convergio"
TIMEOUT_SEC=15
PASSED=0
FAILED=0
SKIPPED=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test helper function
run_test() {
    local name="$1"
    local commands="$2"
    local expected="$3"

    echo -n "  Testing: $name... "

    output=$(echo -e "$commands\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true

    if echo "$output" | grep -q "$expected"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        echo "    Expected: $expected"
        echo "    Got: $(echo "$output" | head -5 | tr '\n' ' ')"
        ((FAILED++))
        return 1
    fi
}

# Test for absence of error (ignore "Debug mode:" messages)
run_test_no_error() {
    local name="$1"
    local commands="$2"

    echo -n "  Testing: $name... "

    output=$(echo -e "$commands\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true

    # Filter out "Debug mode:" lines before checking for errors
    filtered=$(echo "$output" | grep -v "Debug mode:")
    if echo "$filtered" | grep -qi "error:\|failed\|not found\|crash"; then
        echo -e "${RED}FAIL${NC}"
        echo "    Got error: $(echo "$filtered" | grep -i "error:\|failed\|not found" | head -2)"
        ((FAILED++))
        return 1
    else
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
        return 0
    fi
}

# Skip test
skip_test() {
    local name="$1"
    local reason="$2"
    echo -e "  Testing: $name... ${YELLOW}SKIP${NC} ($reason)"
    ((SKIPPED++))
}

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║         CONVERGIO CLI E2E TEST SUITE                       ║"
echo "╠════════════════════════════════════════════════════════════╣"
echo "║  Simulating Technical User and Business User scenarios     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if convergio exists
if [ ! -x "$CONVERGIO" ]; then
    echo -e "${RED}ERROR: $CONVERGIO not found or not executable${NC}"
    echo "Run 'make' first to build the project."
    exit 1
fi

# =============================================================================
# SECTION 1: Basic Commands (Both Users)
# =============================================================================
echo -e "${BLUE}=== Section 1: Basic Commands ===${NC}"

run_test "version" "" "3.0"
run_test "help shows commands" "help" "Available commands"
run_test "status shows kernel" "status" "NOUS System Status"
run_test "hardware shows chip" "hardware" "Apple"
run_test "cost shows budget" "cost" "BUDGET"

# =============================================================================
# SECTION 2: Technical User Scenarios
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 2: Technical User Scenarios ===${NC}"

# Agent management
run_test "agents list" "agents" "agenti specialistici"
run_test "agent help" "agent" "Sottocomandi"
run_test "agent list subcommand" "agent list" "agenti"

# Tools
run_test "tools help" "tools" "Command: tools"
run_test "tools check" "tools check" "installed"

# Debug/development
run_test "debug help" "debug" "Debug"
run_test "stream help" "stream" "Streaming"
run_test "theme help" "theme" "theme"

# Model comparison (just help, don't run actual API calls)
run_test "compare help" "compare" "Compare models"
run_test "benchmark help" "benchmark" "Benchmark"

# Updates
run_test "update check" "update check" "version"

# News/changelog
run_test "news shows release" "news" "Release"

# =============================================================================
# SECTION 3: Business User Scenarios
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 3: Business User Scenarios ===${NC}"

# Simple queries (would need API keys to actually run)
skip_test "chat with Ali" "Requires API key"
skip_test "ask about costs" "Requires API key"

# Cost management
run_test "cost report" "cost report" "COST REPORT"
run_test "cost shows spending" "cost" "spent"

# Auth status
run_test "auth status" "auth" "Authentication"

# =============================================================================
# SECTION 4: Edge Cases and Error Handling
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 4: Edge Cases ===${NC}"

# Invalid commands - they get passed to Ali as natural language input
skip_test "invalid command" "Passed to Ali as natural language"

# Empty inputs handled gracefully
run_test_no_error "empty command" ""

# Agent with partial name (fixed!)
run_test "agent info partial name" "agent info baccio" "baccio-tech-architect"

# =============================================================================
# SECTION 5: Command Argument Handling
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 5: Argument Handling ===${NC}"

run_test "debug with level" "debug info" "Debug"
run_test "theme ocean" "theme ocean" "Theme"
run_test "stream on" "stream on" "Streaming"

# =============================================================================
# SUMMARY
# =============================================================================
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    TEST SUMMARY                            ║"
echo "╠════════════════════════════════════════════════════════════╣"
printf "║  ${GREEN}PASSED${NC}:  %-47d ║\n" $PASSED
printf "║  ${RED}FAILED${NC}:  %-47d ║\n" $FAILED
printf "║  ${YELLOW}SKIPPED${NC}: %-47d ║\n" $SKIPPED
echo "╠════════════════════════════════════════════════════════════╣"
TOTAL=$((PASSED + FAILED))
if [ $TOTAL -gt 0 ]; then
    PCT=$((PASSED * 100 / TOTAL))
    printf "║  Success Rate: %d%%                                        ║\n" $PCT
fi
echo "╚════════════════════════════════════════════════════════════╝"

# Notes
if [ $SKIPPED -gt 0 ]; then
    echo ""
    echo "Notes:"
    echo "  - Some tests skipped (require API keys or are by design)"
    echo ""
fi

# Exit with error if any tests failed
if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
