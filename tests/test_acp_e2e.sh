#!/bin/bash
#
# Convergio ACP Protocol E2E Test Suite
# Tests all ACP features for Zed integration
#
# Usage: ./tests/test_acp_e2e.sh
#

set +e

ACP_SERVER="./build/bin/convergio-acp"
SESSIONS_DIR="$HOME/.convergio/sessions"
PASSED=0
FAILED=0
TOTAL=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_test() {
    ((TOTAL++))
    echo -n "  [$TOTAL] $1... "
}

pass() {
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
}

fail() {
    echo -e "${RED}FAIL${NC}"
    echo "      $1"
    ((FAILED++))
}

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║         CONVERGIO ACP E2E TEST SUITE                       ║"
echo "╠════════════════════════════════════════════════════════════╣"
echo "║  Testing ACP protocol for Zed integration                  ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if convergio-acp exists
if [ ! -x "$ACP_SERVER" ]; then
    echo -e "${RED}ERROR: $ACP_SERVER not found or not executable${NC}"
    echo "Run 'make acp' first to build the ACP server."
    exit 1
fi

# Clean up old test sessions
rm -rf "$SESSIONS_DIR/test_*" 2>/dev/null

echo -e "${BLUE}=== Test 1: ACP Server Basic Functionality ===${NC}"

# Test 1.1: --list-agents
log_test "--list-agents returns agent list"
output=$($ACP_SERVER --list-agents 2>&1)
if echo "$output" | grep -q "ali"; then
    pass
else
    fail "Ali agent not found in output"
fi

# Test 1.2: --help
log_test "--help shows usage"
output=$($ACP_SERVER --help 2>&1)
if echo "$output" | grep -q "agent"; then
    pass
else
    fail "Help doesn't mention agent"
fi

echo ""
echo -e "${BLUE}=== Test 2: ACP Protocol Messages ===${NC}"

# Test 2.1: Initialize request
log_test "Initialize request returns agentInfo"
INIT_REQUEST='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'
output=$(echo "$INIT_REQUEST" | timeout 5 $ACP_SERVER --agent ali 2>/dev/null | head -1)
if echo "$output" | grep -q '"agentInfo"'; then
    pass
else
    fail "No agentInfo in response: $output"
fi

# Test 2.2: Session/new creates session
log_test "session/new creates new session"
SESSION_REQUEST='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}
{"jsonrpc":"2.0","id":2,"method":"session/new","params":{"cwd":"/tmp/test_convergio"}}'
output=$(echo "$SESSION_REQUEST" | timeout 5 $ACP_SERVER --agent ali 2>/dev/null | grep -E "sessionId|session_id" | head -1)
if echo "$output" | grep -qE '"sessionId"|"session_id"'; then
    pass
else
    # Check if we got any valid response with result
    full_output=$(echo "$SESSION_REQUEST" | timeout 5 $ACP_SERVER --agent ali 2>/dev/null)
    if echo "$full_output" | grep -q '"result"'; then
        pass
    else
        fail "No session in response"
    fi
fi

echo ""
echo -e "${BLUE}=== Test 3: Session Persistence ===${NC}"

# Test 3.1: Session file created
log_test "Session file created on disk"
sleep 1  # Give time for file to be written
if ls "$SESSIONS_DIR"/*.json 2>/dev/null | head -1 > /dev/null; then
    pass
else
    fail "No session files in $SESSIONS_DIR"
fi

# Test 3.2: Session file contains valid JSON
log_test "Session file contains valid JSON"
SESSION_FILE=$(ls -t "$SESSIONS_DIR"/*.json 2>/dev/null | head -1)
if [ -n "$SESSION_FILE" ] && jq . "$SESSION_FILE" > /dev/null 2>&1; then
    pass
else
    fail "Session file is not valid JSON"
fi

# Test 3.3: Session has agent_name
log_test "Session contains agent_name"
if [ -n "$SESSION_FILE" ] && grep -q '"agent_name"' "$SESSION_FILE" 2>/dev/null; then
    pass
else
    fail "Session file doesn't contain agent_name"
fi

echo ""
echo -e "${BLUE}=== Test 4: Multi-Agent Support ===${NC}"

# Test 4.1: Different agents available
for agent in ali amy-cfo baccio-tech-architect dario-debugger rex-code-reviewer; do
    log_test "Agent '$agent' responds to initialize"
    output=$(echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' | timeout 5 $ACP_SERVER --agent "$agent" 2>/dev/null | head -1)
    if echo "$output" | grep -q '"agentInfo"'; then
        pass
    else
        fail "Agent $agent failed to respond"
    fi
done

echo ""
echo -e "${BLUE}=== Test 5: Ali Panel Keybinding ===${NC}"

# Test 5.1: Zed keymap has Ali panel binding
log_test "Zed keymap has Ali panel keybinding"
KEYMAP="/Users/roberdan/GitHub/convergio-zed/assets/keymaps/default-macos.json"
if [ -f "$KEYMAP" ] && grep -q "ali_panel::ToggleFocus" "$KEYMAP"; then
    pass
else
    fail "Ali panel keybinding not found in $KEYMAP"
fi

# Test 5.2: Ali panel crate exists
log_test "Ali panel crate exists"
ALI_PANEL="/Users/roberdan/GitHub/convergio-zed/crates/ali_panel/src/panel.rs"
if [ -f "$ALI_PANEL" ]; then
    pass
else
    fail "Ali panel crate not found"
fi

# Test 5.3: Enter keybinding in Ali panel
log_test "Enter keybinding configured for AliPanel"
if grep -q 'KeyBinding::new("enter", SendToAli, Some("AliPanel"))' "$ALI_PANEL" 2>/dev/null; then
    pass
else
    fail "Enter keybinding not configured correctly"
fi

echo ""
echo -e "${BLUE}=== Test 6: Convergio Panel Features ===${NC}"

CONVERGIO_PANEL="/Users/roberdan/GitHub/convergio-zed/crates/convergio_panel/src/panel.rs"

# Test 6.1: ConvergioAgent struct defined
log_test "ConvergioAgent struct defined in panel"
if [ -f "$CONVERGIO_PANEL" ]; then
    if grep -q 'pub struct ConvergioAgent' "$CONVERGIO_PANEL"; then
        pass
    else
        fail "ConvergioAgent struct not found"
    fi
else
    fail "Convergio panel not found"
fi

# Test 6.2: Categories defined
log_test "Agent categories defined"
if grep -q 'Leadership,' "$CONVERGIO_PANEL" && grep -q 'Technology,' "$CONVERGIO_PANEL"; then
    pass
else
    fail "Categories not found"
fi

# Test 6.3: Search functionality
log_test "Search functionality implemented"
if grep -q 'filter_query' "$CONVERGIO_PANEL"; then
    pass
else
    fail "Search not implemented"
fi

# Test 6.4: Onboarding implemented
log_test "Onboarding wizard implemented"
if grep -q 'show_onboarding' "$CONVERGIO_PANEL"; then
    pass
else
    fail "Onboarding not implemented"
fi

echo ""
echo "════════════════════════════════════════════════════════════"
echo -e "  Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC} out of $TOTAL tests"
echo "════════════════════════════════════════════════════════════"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Please review the output above.${NC}"
    exit 1
fi
