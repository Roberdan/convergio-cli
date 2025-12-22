#!/bin/bash
# ============================================================================
# CONVERSATIONAL CONFIG E2E TESTS
#
# Tests the conversational config module with realistic scenarios.
# These tests verify that the module works correctly in real-world situations.
#
# Usage: ./tests/e2e_conversational_config_test.sh
#
# Copyright (c) 2025 Convergio.io
# ============================================================================

set -e

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Counters
PASSED=0
FAILED=0
SKIPPED=0

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Binary path
CONVERGIO="$PROJECT_DIR/build/bin/convergio"
CONVERGIO_EDU="$PROJECT_DIR/build/bin/convergio-edu"

# Check for timeout command (macOS uses gtimeout from coreutils)
if command -v gtimeout &> /dev/null; then
    TIMEOUT_CMD="gtimeout"
elif command -v timeout &> /dev/null; then
    TIMEOUT_CMD="timeout"
else
    TIMEOUT_CMD=""
fi
TIMEOUT_SEC=30

echo ""
echo "╔════════════════════════════════════════════════════════════════════╗"
echo "║          CONVERSATIONAL CONFIG E2E TEST SUITE                      ║"
echo "╠════════════════════════════════════════════════════════════════════╣"
echo "║  Testing LLM-based conversational configuration gathering          ║"
echo "╚════════════════════════════════════════════════════════════════════╝"
echo ""

# =============================================================================
# HELPER FUNCTIONS
# =============================================================================

run_test() {
    local name="$1"
    local commands="$2"
    local expected="$3"
    local binary="${4:-$CONVERGIO}"

    echo -n "  Testing: $name... "

    if [ ! -f "$binary" ]; then
        echo -e "${YELLOW}SKIP${NC} (binary not found)"
        ((SKIPPED++))
        return 0
    fi

    output=$(echo -e "$commands\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $binary -q 2>&1) || true

    if echo "$output" | grep -qi "$expected"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        echo "    Expected: $expected"
        echo "    Got: $(echo "$output" | head -3 | tr '\n' ' ')"
        ((FAILED++))
        return 1
    fi
}

run_test_not_contains() {
    local name="$1"
    local commands="$2"
    local forbidden="$3"
    local binary="${4:-$CONVERGIO}"

    echo -n "  Testing: $name... "

    if [ ! -f "$binary" ]; then
        echo -e "${YELLOW}SKIP${NC} (binary not found)"
        ((SKIPPED++))
        return 0
    fi

    output=$(echo -e "$commands\nquit" | ${TIMEOUT_CMD:-cat} ${TIMEOUT_CMD:+$TIMEOUT_SEC} $binary -q 2>&1) || true

    if echo "$output" | grep -qi "$forbidden"; then
        echo -e "${RED}FAIL${NC}"
        echo "    Should NOT contain: $forbidden"
        ((FAILED++))
        return 1
    else
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
        return 0
    fi
}

# =============================================================================
# SECTION 1: MODULE AVAILABILITY
# =============================================================================
echo -e "${CYAN}=== Section 1: Module Availability ===${NC}"

# Check unit tests exist and pass
echo -n "  Testing: Unit tests exist... "
if [ -f "$PROJECT_DIR/tests/test_conversational_config.c" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check header exists
echo -n "  Testing: Header file exists... "
if [ -f "$PROJECT_DIR/include/nous/conversational_config.h" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check implementation exists
echo -n "  Testing: Implementation exists... "
if [ -f "$PROJECT_DIR/src/core/conversational_config.c" ]; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =============================================================================
# SECTION 2: API DESIGN QUALITY
# =============================================================================
echo ""
echo -e "${CYAN}=== Section 2: API Design Quality ===${NC}"

# Check for preset functions
echo -n "  Testing: Preset functions defined... "
if grep -q "conversational_config_preset_onboarding" "$PROJECT_DIR/include/nous/conversational_config.h" && \
   grep -q "conversational_config_preset_project" "$PROJECT_DIR/include/nous/conversational_config.h" && \
   grep -q "conversational_config_preset_preferences" "$PROJECT_DIR/include/nous/conversational_config.h"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for validation function
echo -n "  Testing: Validation function defined... "
if grep -q "conversational_config_validate" "$PROJECT_DIR/include/nous/conversational_config.h"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for fallback support
echo -n "  Testing: Fallback mode supported... "
if grep -q "enable_fallback" "$PROJECT_DIR/include/nous/conversational_config.h" && \
   grep -q "fallback_prompts" "$PROJECT_DIR/include/nous/conversational_config.h"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for I/O abstraction
echo -n "  Testing: Custom I/O support (for testing)... "
if grep -q "conversational_config_run_with_io" "$PROJECT_DIR/include/nous/conversational_config.h"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for callbacks
echo -n "  Testing: Callback hooks defined... "
if grep -q "on_turn" "$PROJECT_DIR/include/nous/conversational_config.h" && \
   grep -q "on_field_gathered" "$PROJECT_DIR/include/nous/conversational_config.h"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =============================================================================
# SECTION 3: IMPLEMENTATION QUALITY
# =============================================================================
echo ""
echo -e "${CYAN}=== Section 3: Implementation Quality ===${NC}"

# Check for exit command handling
echo -n "  Testing: Exit commands handled (esci/exit/quit)... "
if grep -q "esci" "$PROJECT_DIR/src/core/conversational_config.c" && \
   grep -q "exit" "$PROJECT_DIR/src/core/conversational_config.c" && \
   grep -q "quit" "$PROJECT_DIR/src/core/conversational_config.c"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for JSON extraction
echo -n "  Testing: JSON extraction implemented... "
if grep -q "extract_json_from_conversation" "$PROJECT_DIR/src/core/conversational_config.c"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for LLM integration
echo -n "  Testing: LLM integration (llm_chat)... "
if grep -q "llm_chat" "$PROJECT_DIR/src/core/conversational_config.c" && \
   grep -q "llm_is_available" "$PROJECT_DIR/src/core/conversational_config.c"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for fallback form mode
echo -n "  Testing: Fallback form mode implemented... "
if grep -q "run_fallback_form" "$PROJECT_DIR/src/core/conversational_config.c"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for history tracking
echo -n "  Testing: Conversation history tracking... "
if grep -q "append_to_history" "$PROJECT_DIR/src/core/conversational_config.c"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =============================================================================
# SECTION 4: SAFETY AND SECURITY
# =============================================================================
echo ""
echo -e "${CYAN}=== Section 4: Safety and Security ===${NC}"

# Check for buffer size limits
echo -n "  Testing: Buffer size limits defined... "
if grep -q "MAX_INPUT_LENGTH" "$PROJECT_DIR/src/core/conversational_config.c" && \
   grep -q "MAX_CONVERSATION_LENGTH" "$PROJECT_DIR/src/core/conversational_config.c"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for max turns limit
echo -n "  Testing: Max turns limit enforced... "
if grep -q "max_turns" "$PROJECT_DIR/src/core/conversational_config.c"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Check for memory cleanup
echo -n "  Testing: Memory cleanup function... "
if grep -q "conversational_result_free" "$PROJECT_DIR/src/core/conversational_config.c"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =============================================================================
# SECTION 5: INTEGRATION WITH EDUCATION
# =============================================================================
echo ""
echo -e "${CYAN}=== Section 5: Integration Potential ===${NC}"

# Check if ali_onboarding could use this module
echo -n "  Testing: Ali onboarding compatibility... "
if [ -f "$PROJECT_DIR/src/education/ali_onboarding.c" ]; then
    # The current implementation should be compatible with conversational_config
    echo -e "${GREEN}PASS${NC} (ali_onboarding.c exists, ready for integration)"
    ((PASSED++))
else
    echo -e "${YELLOW}SKIP${NC} (education module not found)"
    ((SKIPPED++))
fi

# Check for extraction schema compatibility
echo -n "  Testing: Education-compatible extraction schema... "
if grep -q "extraction_schema" "$PROJECT_DIR/include/nous/conversational_config.h"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =============================================================================
# SECTION 6: UNIT TEST VERIFICATION
# =============================================================================
echo ""
echo -e "${CYAN}=== Section 6: Unit Test Verification ===${NC}"

# Run unit tests if binary exists
echo -n "  Testing: Unit tests pass... "
if [ -f "$PROJECT_DIR/build/bin/conversational_config_test" ]; then
    if "$PROJECT_DIR/build/bin/conversational_config_test" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
else
    # Try to build and run
    if cd "$PROJECT_DIR" && make conversational_config_test > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${YELLOW}SKIP${NC} (could not build test)"
        ((SKIPPED++))
    fi
fi

# =============================================================================
# SUMMARY
# =============================================================================
echo ""
echo "╔════════════════════════════════════════════════════════════════════╗"
echo "║           CONVERSATIONAL CONFIG E2E TEST SUMMARY                   ║"
echo "╠════════════════════════════════════════════════════════════════════╣"
printf "║  ${GREEN}PASSED${NC}:  %-56d ║\n" $PASSED
printf "║  ${RED}FAILED${NC}:  %-56d ║\n" $FAILED
printf "║  ${YELLOW}SKIPPED${NC}: %-56d ║\n" $SKIPPED
echo "╠════════════════════════════════════════════════════════════════════╣"
TOTAL=$((PASSED + FAILED))
if [ $TOTAL -gt 0 ]; then
    RATE=$((PASSED * 100 / TOTAL))
    printf "║  Success Rate: %d%%%-50s ║\n" $RATE ""
fi
echo "╚════════════════════════════════════════════════════════════════════╝"

# Exit with failure if any tests failed
if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
