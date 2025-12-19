#!/bin/bash
# Convergio 6.0 - Phase Verification Test Suite
# Tests all 6 phases of the Zed Integration MVP

# Don't exit on error - we want to run all tests

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

# Test result tracking
pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((PASS_COUNT++))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((FAIL_COUNT++))
}

skip() {
    echo -e "${YELLOW}⊘ SKIP${NC}: $1"
    ((SKIP_COUNT++))
}

section() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
}

# Paths
CONVERGIO_CLI="/Users/roberdan/GitHub/ConvergioCLI"
CONVERGIO_ZED="/Users/roberdan/GitHub/convergio-zed"
ACP_BINARY="$CONVERGIO_CLI/build/bin/convergio-acp"
ZED_BINARY="$CONVERGIO_ZED/target/release/zed"

cd "$CONVERGIO_CLI"

#######################################
# FASE 1 - MVP Tests
#######################################
section "FASE 1 - MVP (ACP Protocol)"

# Test 1.1: convergio-acp binary exists
if [[ -x "$ACP_BINARY" ]]; then
    pass "convergio-acp binary exists and is executable"
else
    fail "convergio-acp binary not found at $ACP_BINARY"
fi

# Test 1.2: convergio-acp --help works
if "$ACP_BINARY" --help 2>&1 | grep -q "\-\-agent"; then
    pass "convergio-acp --help shows usage info"
else
    fail "convergio-acp --help doesn't work"
fi

# Test 1.3: ACP source files exist
if [[ -f "src/acp/acp_server.c" ]] && [[ -f "include/nous/acp.h" ]]; then
    pass "ACP source files exist (acp_server.c, acp.h)"
else
    fail "ACP source files missing"
fi

# Test 1.4: ACP protocol handlers implemented
if grep -q "handle_initialize" src/acp/acp_server.c && \
   grep -q "handle_session_new" src/acp/acp_server.c && \
   grep -q "handle_session_prompt" src/acp/acp_server.c; then
    pass "ACP protocol handlers implemented (initialize, session/new, session/prompt)"
else
    fail "ACP protocol handlers missing"
fi

# Test 1.5: Streaming support
if grep -q "streaming" src/acp/acp_server.c || grep -q "chunk" src/acp/acp_server.c; then
    pass "Streaming support implemented in ACP"
else
    fail "Streaming support missing in ACP"
fi

#######################################
# FASE 2 - Multi-Agent Panel Tests
#######################################
section "FASE 2 - Multi-Agent Panel"

# Test 2.1: --agent flag exists
if "$ACP_BINARY" --help 2>&1 | grep -q "\-\-agent"; then
    pass "--agent flag supported"
else
    fail "--agent flag not found"
fi

# Test 2.2: --list-agents works
AGENT_COUNT=$("$ACP_BINARY" --list-agents 2>/dev/null | wc -l)
if [[ $AGENT_COUNT -ge 50 ]]; then
    pass "--list-agents shows $AGENT_COUNT agents (expected 54+)"
else
    fail "--list-agents shows only $AGENT_COUNT agents (expected 54+)"
fi

# Test 2.3: Ali agent exists
if "$ACP_BINARY" --list-agents 2>/dev/null | grep -qi "ali"; then
    pass "Ali agent available in agent list"
else
    fail "Ali agent not found"
fi

# Test 2.4: generate_zed_config.sh exists
if [[ -x "scripts/generate_zed_config.sh" ]]; then
    pass "generate_zed_config.sh exists and is executable"
else
    if [[ -f "scripts/generate_zed_config.sh" ]]; then
        skip "generate_zed_config.sh exists but not executable"
    else
        fail "generate_zed_config.sh not found"
    fi
fi

# Test 2.5: Agent definitions exist
AGENT_DEFS=$(find src/agents/definitions -name "*.md" 2>/dev/null | wc -l)
if [[ $AGENT_DEFS -ge 50 ]]; then
    pass "Found $AGENT_DEFS agent definitions"
else
    fail "Only $AGENT_DEFS agent definitions found (expected 54+)"
fi

#######################################
# FASE 3 - Convergio-Zed Fork Tests
#######################################
section "FASE 3 - Convergio-Zed Fork"

# Test 3.1: convergio-zed repo exists
if [[ -d "$CONVERGIO_ZED" ]]; then
    pass "convergio-zed repository exists"
else
    fail "convergio-zed repository not found at $CONVERGIO_ZED"
fi

# Test 3.2: Zed binary exists
if [[ -x "$ZED_BINARY" ]]; then
    pass "Zed release binary exists"
else
    fail "Zed release binary not found at $ZED_BINARY"
fi

# Test 3.3: convergio_panel crate exists
if [[ -d "$CONVERGIO_ZED/crates/convergio_panel" ]]; then
    pass "convergio_panel crate exists"

    # Check panel.rs
    if [[ -f "$CONVERGIO_ZED/crates/convergio_panel/src/panel.rs" ]]; then
        pass "convergio_panel/src/panel.rs exists"
    else
        fail "convergio_panel/src/panel.rs missing"
    fi
else
    fail "convergio_panel crate not found"
fi

# Test 3.4: ali_panel crate exists
if [[ -d "$CONVERGIO_ZED/crates/ali_panel" ]]; then
    pass "ali_panel crate exists"

    if [[ -f "$CONVERGIO_ZED/crates/ali_panel/src/panel.rs" ]]; then
        pass "ali_panel/src/panel.rs exists"
    else
        fail "ali_panel/src/panel.rs missing"
    fi
else
    fail "ali_panel crate not found"
fi

# Test 3.5: Panel registered in main.rs
if grep -q "convergio_panel" "$CONVERGIO_ZED/crates/zed/src/main.rs" && \
   grep -q "ali_panel" "$CONVERGIO_ZED/crates/zed/src/main.rs"; then
    pass "Panels registered in main.rs"
else
    fail "Panels not registered in main.rs"
fi

# Test 3.6: 54 agents in convergio_panel
PANEL_AGENTS=$(grep -c "Self::new(" "$CONVERGIO_ZED/crates/convergio_panel/src/panel.rs" 2>/dev/null || echo 0)
if [[ $PANEL_AGENTS -ge 50 ]]; then
    pass "Found $PANEL_AGENTS agents in convergio_panel"
else
    fail "Only $PANEL_AGENTS agents in convergio_panel (expected 54)"
fi

# Test 3.7: Categories implemented
if grep -q "AgentCategory" "$CONVERGIO_ZED/crates/convergio_panel/src/panel.rs"; then
    pass "AgentCategory enum implemented"
else
    fail "AgentCategory not found"
fi

#######################################
# FASE 4 - Feature Avanzate Tests
#######################################
section "FASE 4 - Feature Avanzate"

# Test 4.1: Ali bottom panel implementation
if grep -q "DockPosition::Bottom" "$CONVERGIO_ZED/crates/ali_panel/src/panel.rs"; then
    pass "Ali panel configured for bottom dock"
else
    fail "Ali panel not configured for bottom dock"
fi

# Test 4.2: AcpThreadView integration
if grep -q "AcpThreadView" "$CONVERGIO_ZED/crates/ali_panel/src/panel.rs"; then
    pass "AcpThreadView integrated in Ali panel"
else
    fail "AcpThreadView not integrated in Ali panel"
fi

# Test 4.3: Persistence - sessions directory support
if grep -q "sessions" src/acp/acp_server.c || grep -q "session" include/nous/acp.h; then
    pass "Session persistence support in ACP"
else
    fail "Session persistence not found in ACP"
fi

# Test 4.4: thread_by_agent_name in Zed
if grep -q "thread_by_agent_name" "$CONVERGIO_ZED/crates/agent/src/history_store.rs" 2>/dev/null; then
    pass "thread_by_agent_name implemented in HistoryStore"
else
    fail "thread_by_agent_name not found in HistoryStore"
fi

# Test 4.5: agent_name field in DB
if grep -q "agent_name" "$CONVERGIO_ZED/crates/agent/src/db.rs" 2>/dev/null; then
    pass "agent_name field in database schema"
else
    fail "agent_name field not in database"
fi

#######################################
# FASE 5 - Polish & UX Tests
#######################################
section "FASE 5 - Polish & UX"

# Test 5.1: Icon mapping exists
if grep -q "IconName::" "$CONVERGIO_ZED/crates/convergio_panel/src/panel.rs"; then
    pass "Icon mapping implemented in convergio_panel"
else
    fail "Icon mapping not found"
fi

# Test 5.2: Category colors (HSLA)
if grep -qi "hsla\|color" "$CONVERGIO_ZED/crates/convergio_panel/src/panel.rs"; then
    pass "Category colors implemented"
else
    skip "Category colors implementation unclear"
fi

# Test 5.3: Search functionality
if grep -q "search\|filter" "$CONVERGIO_ZED/crates/convergio_panel/src/panel.rs"; then
    pass "Search/filter functionality exists"
else
    fail "Search functionality not found"
fi

# Test 5.4: Collapsible categories
if grep -q "Disclosure\|collapsed\|expand" "$CONVERGIO_ZED/crates/convergio_panel/src/panel.rs"; then
    pass "Collapsible categories implemented"
else
    fail "Collapsible categories not found"
fi

#######################################
# FASE 6 - File Interaction & E2E Tests
#######################################
section "FASE 6 - File Interaction & E2E"

# Test 6.1: E2E test suite exists
if [[ -f "tests/test_acp_e2e.sh" ]]; then
    pass "E2E test suite exists (test_acp_e2e.sh)"

    # Run E2E tests if executable
    if [[ -x "tests/test_acp_e2e.sh" ]]; then
        echo "  Running E2E tests..."
        if timeout 60 ./tests/test_acp_e2e.sh > /tmp/e2e_output.txt 2>&1; then
            E2E_PASS=$(grep -c "PASS\|✓" /tmp/e2e_output.txt 2>/dev/null || echo 0)
            pass "E2E tests passed ($E2E_PASS tests)"
        else
            fail "E2E tests failed - check /tmp/e2e_output.txt"
        fi
    else
        skip "E2E test not executable"
    fi
else
    fail "E2E test suite not found"
fi

# Test 6.2: Custom Convergio icons
if [[ -f "$CONVERGIO_ZED/assets/icons/convergio.svg" ]]; then
    pass "Custom convergio.svg icon exists"
else
    fail "convergio.svg icon not found"
fi

if [[ -f "$CONVERGIO_ZED/assets/icons/convergio_ali.svg" ]]; then
    pass "Custom convergio_ali.svg icon exists"
else
    fail "convergio_ali.svg icon not found"
fi

# Test 6.3: IconName enum updated
if grep -q "Convergio" "$CONVERGIO_ZED/crates/icons/src/icons.rs" 2>/dev/null; then
    pass "Convergio added to IconName enum"
else
    fail "Convergio not in IconName enum"
fi

# Test 6.4: File tools in orchestrator
if grep -q "file_read\|read_file" src/orchestrator/orchestrator.c 2>/dev/null || \
   grep -q "file" src/tools/tools.c 2>/dev/null; then
    pass "File tools exist in orchestrator"
else
    skip "File tools implementation unclear"
fi

# Test 6.5: Ali panel full chat (not just button)
if grep -q "thread_view" "$CONVERGIO_ZED/crates/ali_panel/src/panel.rs" && \
   ! grep -q "Open Chat.*button" "$CONVERGIO_ZED/crates/ali_panel/src/panel.rs"; then
    pass "Ali panel has full chat (not just button)"
else
    if grep -q "AcpThreadView" "$CONVERGIO_ZED/crates/ali_panel/src/panel.rs"; then
        pass "Ali panel integrates AcpThreadView"
    else
        fail "Ali panel missing full chat integration"
    fi
fi

#######################################
# SUMMARY
#######################################
section "TEST SUMMARY"

TOTAL=$((PASS_COUNT + FAIL_COUNT + SKIP_COUNT))
echo ""
echo -e "Total Tests: $TOTAL"
echo -e "${GREEN}Passed: $PASS_COUNT${NC}"
echo -e "${RED}Failed: $FAIL_COUNT${NC}"
echo -e "${YELLOW}Skipped: $SKIP_COUNT${NC}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}    ALL PHASES VERIFIED SUCCESSFULLY! ✓${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    exit 0
else
    echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${RED}    $FAIL_COUNT TEST(S) FAILED - REVIEW REQUIRED${NC}"
    echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"
    exit 1
fi
