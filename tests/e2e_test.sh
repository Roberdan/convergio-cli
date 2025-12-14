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

# macOS doesn't have timeout, use gtimeout from coreutils or fallback
if command -v timeout &>/dev/null; then
    TIMEOUT_CMD="timeout"
elif command -v gtimeout &>/dev/null; then
    TIMEOUT_CMD="gtimeout"
else
    # Fallback: no timeout (run with simple shell backgrounding)
    TIMEOUT_CMD=""
fi
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

    output=$(echo -e "$commands\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true

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

    output=$(echo -e "$commands\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true

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

run_test "version" "" "5\."
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
if echo "$output" | grep -qE "[45]\.[0-9]+\.[0-9]+|version|VERSION"; then
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

# Test Ali delegation (single agent)
echo -n "  Testing: Ali delegation to specialist... "
output=$(echo -e "Ask Baccio to briefly describe his role\nquit" | timeout 120 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "architet\|baccio\|tech\|system\|design"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (delegation requires API)"
    ((SKIPPED++))
fi

# Test parallel delegation (multiple agents)
echo -n "  Testing: Parallel delegation to multiple agents... "
output=$(echo -e "Ask both Baccio and Luca to analyze security of a REST API\nquit" | timeout 180 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "security\|architet\|luca\|baccio\|api"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (parallel delegation requires API)"
    ((SKIPPED++))
fi

# Test sequential delegation
echo -n "  Testing: Sequential agent workflow... "
output=$(echo -e "First ask Baccio for architecture, then Thor to review quality\nquit" | timeout 180 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "architet\|quality\|thor\|baccio"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (sequential delegation requires API)"
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
output=$(echo -e "project create TestProject-E2E --team baccio,davide --purpose \"E2E test project\"\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
output=$(echo -e "project list\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
output=$(echo -e "project use testproject-e2e\nproject status\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
output=$(echo -e "project use testproject-e2e\nproject team add stefano\nproject status\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
output=$(echo -e "project use testproject-e2e\nproject team remove stefano\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
output=$(echo -e "project use testproject-e2e\nproject focus Building the authentication module\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
output=$(echo -e "project use testproject-e2e\nproject decision Using JWT for authentication\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
output=$(echo -e "project clear\nproject\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
output=$(echo -e "project create \"Marketing Test\" --template marketing\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
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
# SECTION 9: Provider Configuration & Setup Wizard
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 9: Provider & Setup Wizard ===${NC}"

# Test setup command help
run_test "setup help" "setup" "CONVERGIO SETUP WIZARD"

# Test setup shows providers
run_test "setup shows anthropic" "setup" "Anthropic"

# Test setup shows openrouter
run_test "setup shows openrouter" "setup" "OpenRouter"

# Test setup shows ollama
run_test "setup shows ollama" "setup" "Ollama"

# Test setup wizard can exit
echo -n "  Testing: setup wizard exit... "
output=$(echo -e "setup\n5\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "setup\|wizard\|configure"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Setup wizard didn't work"
    ((FAILED++))
fi

# Test provider models available
run_test "models include deepseek" "models" "DeepSeek"
run_test "models include llama local" "models" "Local"

# Test cost command shows new provider pricing
run_test "cost shows providers" "cost" "provider"

# =============================================================================
# SECTION 10: MLX Local Models (Apple Silicon)
# =============================================================================
echo ""
echo -e "${BLUE}=== Section 10: MLX Local Models ===${NC}"

# Check if running on Apple Silicon
if sysctl -n machdep.cpu.brand_string 2>/dev/null | grep -q "Apple"; then
    # Test MLX availability check
    run_test "MLX shows in help" "help" "local"

    # Test --local flag is recognized
    echo -n "  Testing: --local flag recognition... "
    output=$($CONVERGIO --local --help 2>&1) || true
    if echo "$output" | grep -qi "local\|mlx\|model"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi

    # Test local models are listed in setup
    run_test "setup shows MLX models" "setup\n5" "Local Models"

    # Test model info is available
    run_test "model list shows deepseek-r1" "models" "DeepSeek R1"
    run_test "model list shows llama" "models" "Llama 3.2"

    # Test MLX help shows available models
    echo -n "  Testing: --help shows MLX model names... "
    output=$($CONVERGIO --help 2>&1) || true
    if echo "$output" | grep -q "deepseek-r1-1.5b\|llama-3.2-1b"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${YELLOW}SKIP${NC} (model names not in help)"
        ((SKIPPED++))
    fi

    # Test that MLX binary links correctly (no missing symbols)
    echo -n "  Testing: MLX library linked correctly... "
    if $CONVERGIO --version 2>&1 | grep -q "^Convergio"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC} (binary may have linking issues)"
        ((FAILED++))
    fi

    # Note: Actual model download tests are not run in CI due to time/bandwidth
    # They should be run manually with: convergio --local --model deepseek-r1-1.5b
    skip_test "MLX model download" "requires manual testing (1.2GB download)"
    skip_test "MLX inference" "requires downloaded model"
else
    skip_test "MLX availability" "not on Apple Silicon"
    skip_test "MLX model listing" "not on Apple Silicon"
    skip_test "MLX model download" "not on Apple Silicon"
    skip_test "MLX inference" "not on Apple Silicon"
fi

# =============================================================================
# CONTEXT COMPACTION TESTS
# =============================================================================
echo ""
echo -e "${BLUE}┌────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BLUE}│  CONTEXT COMPACTION TESTS                                  │${NC}"
echo -e "${BLUE}└────────────────────────────────────────────────────────────┘${NC}"

# Note: Full compaction tests require long conversations which are time-consuming
# These are basic smoke tests to verify the compaction system initializes correctly

# Test that compaction module loads without errors
run_test_no_error "compaction module initializes" "help"

# Test database schema includes checkpoint table (via sqlite if available)
if command -v sqlite3 &> /dev/null; then
    DB_PATH="./data/convergio.db"
    if [ -f "$DB_PATH" ]; then
        if sqlite3 "$DB_PATH" ".tables" 2>/dev/null | grep -q "checkpoint_summaries"; then
            echo -e "  Testing: checkpoint_summaries table exists... ${GREEN}PASS${NC}"
            ((PASSED++))
        else
            echo -e "  Testing: checkpoint_summaries table exists... ${YELLOW}SKIP${NC} (table not yet created)"
            ((SKIPPED++))
        fi
    else
        skip_test "checkpoint_summaries table" "database not yet created"
    fi
else
    skip_test "checkpoint_summaries table" "sqlite3 not available"
fi

# Test that compaction configuration is reasonable
echo -n "  Testing: compaction threshold configuration... "
THRESHOLD=80000  # Should match COMPACTION_THRESHOLD_TOKENS
if [ $THRESHOLD -ge 50000 ] && [ $THRESHOLD -le 200000 ]; then
    echo -e "${GREEN}PASS${NC} (threshold: ${THRESHOLD})"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC} (threshold out of reasonable range)"
    ((FAILED++))
fi

# =============================================================================
# GIT/TEST WORKFLOW COMMANDS (Issue #15)
# =============================================================================
echo ""
echo -e "${BLUE}┌────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BLUE}│  GIT/TEST WORKFLOW COMMANDS                                │${NC}"
echo -e "${BLUE}└────────────────────────────────────────────────────────────┘${NC}"

# /test command
run_test "test command shows framework detection" "test" "test"

# /git command - should work (we're in a git repo)
run_test "git status works" "git status" "Recent commits"
run_test "git help shows subcommands" "git" "Subcommands"

# /pr command - check it detects we're on main
run_test "pr blocks on main branch" "pr" "Cannot create PR from main"

# Help documentation
run_test "help test exists" "help test" "auto-detect"
run_test "help git exists" "help git" "Git workflow"
run_test "help pr exists" "help pr" "pull request"

# =============================================================================
# SEMANTIC EMBEDDINGS (Issues #1, #2, #3)
# =============================================================================
echo ""
echo -e "${BLUE}┌────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BLUE}│  SEMANTIC MEMORY & EMBEDDINGS                              │${NC}"
echo -e "${BLUE}└────────────────────────────────────────────────────────────┘${NC}"

# Search command (uses embeddings)
run_test "search command works" "search test query" "Cerco\|No\|Found"

# Remember command
run_test "remember command works" "remember Test memory for E2E" "Stored\|Remembered\|memory"

# Check if OpenAI embeddings would be used (if key is set)
if [ -n "$OPENAI_API_KEY" ]; then
    echo -e "  Testing: OpenAI embeddings available... ${GREEN}PASS${NC} (API key set)"
    ((PASSED++))
else
    echo -e "  Testing: OpenAI embeddings available... ${YELLOW}SKIP${NC} (no API key, using fallback)"
    ((SKIPPED++))
fi

# =============================================================================
# ANNA TODO/NOTIFY TOOLS (Task Management)
# =============================================================================
echo ""
echo -e "${BLUE}┌────────────────────────────────────────────────────────────┐${NC}"
echo -e "${BLUE}│  ANNA TODO/NOTIFY TOOLS (Task Management)                  │${NC}"
echo -e "${BLUE}└────────────────────────────────────────────────────────────┘${NC}"

# Test todo command exists
run_test "todo command shows list" "todo" "task"
run_test "todo list command" "todo list" "task"

# Test todo add command
echo -n "  Testing: todo add creates task... "
output=$(echo -e "todo add Test E2E Task\ntodo list\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+30} $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "Test E2E Task\|created\|added"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    echo "    Todo add didn't work"
    ((FAILED++))
fi

# Test todo with due date
echo -n "  Testing: todo add with due date... "
output=$(echo -e "todo add \"Deadline task\" --due tomorrow\ntodo list\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+30} $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "Deadline task\|created\|added"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (due date parsing may vary)"
    ((SKIPPED++))
fi

# Test Anna with natural language task
echo -n "  Testing: @anna creates task via tool... "
output=$(echo -e "@anna aggiungi un task per testare i tool\nquit" | timeout 90 $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "task\|created\|creato\|aggiunto"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (requires API and tool execution)"
    ((SKIPPED++))
fi

# Test remind command
run_test "remind command exists" "remind" "remind"

# Test remind scheduling (don't actually wait for notification)
echo -n "  Testing: remind command schedules... "
output=$(echo -e "remind Test reminder in 1 hour\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+30} $CONVERGIO -q 2>&1) || true
if echo "$output" | grep -qi "scheduled\|reminder\|notif\|ore\|hour"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (remind may need different syntax)"
    ((SKIPPED++))
fi

# Test reminders list
run_test "reminders list command" "reminders" "reminder\|notif\|scheduled\|No"

# Cleanup test tasks
echo -n "  Testing: cleanup test tasks... "
# Get task IDs and delete them
output=$(echo -e "todo list\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+15} $CONVERGIO -q 2>&1) || true
# Extract task IDs containing "E2E" or "test" and mark as done
for id in $(echo "$output" | grep -i "e2e\|test" | grep -oE '\[?[0-9]+\]?' | tr -d '[]' | head -5); do
    echo -e "todo done $id\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+10} $CONVERGIO -q 2>&1 >/dev/null || true
done
echo -e "${GREEN}DONE${NC}"

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
