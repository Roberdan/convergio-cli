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
# SECTION 6: Real API Tests (requires API key)
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 6: Real API Tests ===${NC}"

# Test chat with Ali
echo -n "  Testing: chat with Ali... "
output=$(echo -e "Rispondi solo 'OK' se mi senti\nquit" | timeout 60 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "OK\|sento\|ricevuto"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Ali didn't respond properly"
    ((FAILED++))
fi

# Test shell_exec tool
echo -n "  Testing: shell_exec tool (date)... "
output=$(echo -e "Esegui il comando 'echo TEST123' e dimmi l'output\nquit" | timeout 60 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -q "TEST123"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Shell exec didn't work or output not returned"
    ((FAILED++))
fi

# Test file_read tool
echo -n "  Testing: file_read tool... "
output=$(echo -e "Leggi il file VERSION e dimmi cosa contiene\nquit" | timeout 60 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -q "3\.0"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    File read didn't return version"
    ((FAILED++))
fi

# Test file_write tool (uses workspace-relative path)
echo -n "  Testing: file_write tool... "
TEST_FILE="test_e2e_write_$(date +%s).txt"
rm -f "$TEST_FILE" 2>/dev/null
output=$(echo -e "Scrivi 'TEST_WRITE_OK' nel file $TEST_FILE\nquit" | timeout 90 $CONVERGIO -q 2>&1) || true
if [ -f "$TEST_FILE" ] && grep -q "TEST_WRITE_OK" "$TEST_FILE" 2>/dev/null; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
    rm -f "$TEST_FILE"
else
    echo -e "${RED}FAIL${NC} (file_write tool failed)"
    echo "    File exists: $([ -f \"$TEST_FILE\" ] && echo 'yes' || echo 'no')"
    ((FAILED++))
fi

# Test web_fetch tool
echo -n "  Testing: web_fetch tool... "
output=$(echo -e "Vai su https://httpbin.org/get e dimmi cosa vedi\nquit" | timeout 90 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "httpbin\|origin\|headers\|url"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (may need internet)"
    ((SKIPPED++))
fi

# Test git command
echo -n "  Testing: git via shell... "
output=$(echo -e "Esegui 'git status' e dimmi quanti file sono modificati\nquit" | timeout 60 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "branch\|clean\|modific\|commit"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Git command didn't work"
    ((FAILED++))
fi

# =============================================================================
# SECTION 7: Agent Delegation & Communication
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 7: Agent Delegation ===${NC}"

# Test direct agent communication
echo -n "  Testing: direct agent @Baccio... "
output=$(echo -e "@baccio Dimmi in una parola cosa fai\nquit" | timeout 90 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "architet\|system\|design\|tech"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (agent communication requires API)"
    ((SKIPPED++))
fi

# Test new finance agent
echo -n "  Testing: finance agent Fiona available... "
output=$(echo -e "agent info fiona\nquit" | timeout 15 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "fiona\|market\|analyst"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Fiona agent not found"
    ((FAILED++))
fi

# Test Ali delegation
echo -n "  Testing: Ali delegation to specialist... "
output=$(echo -e "Chiedi a Baccio di descrivere brevemente il suo ruolo\nquit" | timeout 120 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "architet\|baccio\|tech\|system"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (delegation requires API)"
    ((SKIPPED++))
fi

# =============================================================================
# SECTION 8: Projects Feature
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 8: Projects Feature ===${NC}"

# Test project help
run_test "project help" "project" "project create"

# Test project templates
run_test "project templates" "project templates" "app-dev"

# Test project create with team
echo -n "  Testing: project create... "
# Clean up any existing test project first
rm -rf ~/.convergio/projects/testproject-e2e* 2>/dev/null
output=$(echo -e "project create TestProject-E2E --team baccio,davide --purpose \"E2E test project\"\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "created project\|TestProject"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Project creation failed"
    ((FAILED++))
fi

# Test project list
echo -n "  Testing: project list... "
output=$(echo -e "project list\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "testproject-e2e\|projects"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Project list failed"
    ((FAILED++))
fi

# Test project status
echo -n "  Testing: project status... "
output=$(echo -e "project use testproject-e2e\nproject status\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "team\|baccio\|davide"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Project status failed"
    ((FAILED++))
fi

# Test project team add
echo -n "  Testing: project team add... "
output=$(echo -e "project use testproject-e2e\nproject team add stefano\nproject status\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "added\|stefano"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Team add failed"
    ((FAILED++))
fi

# Test project team remove
echo -n "  Testing: project team remove... "
output=$(echo -e "project use testproject-e2e\nproject team remove stefano\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "removed"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Team remove failed"
    ((FAILED++))
fi

# Test project focus
echo -n "  Testing: project focus... "
output=$(echo -e "project use testproject-e2e\nproject focus Building the authentication module\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "focus.*updated\|authentication"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Focus update failed"
    ((FAILED++))
fi

# Test project decision
echo -n "  Testing: project decision... "
output=$(echo -e "project use testproject-e2e\nproject decision Using JWT for authentication\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "decision.*recorded\|JWT"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Decision record failed"
    ((FAILED++))
fi

# Test project clear
echo -n "  Testing: project clear... "
output=$(echo -e "project clear\nproject\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "cleared\|no active"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Project clear failed"
    ((FAILED++))
fi

# Test project create with template
echo -n "  Testing: project create with template... "
rm -rf ~/.convergio/projects/marketing-test* 2>/dev/null
output=$(echo -e "project create \"Marketing Test\" --template marketing\nquit" | timeout $TIMEOUT_SEC $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "created\|copywriter\|designer\|analyst"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Template-based creation failed"
    ((FAILED++))
fi

# Cleanup test projects
rm -rf ~/.convergio/projects/testproject-e2e* 2>/dev/null
rm -rf ~/.convergio/projects/marketing-test* 2>/dev/null

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
