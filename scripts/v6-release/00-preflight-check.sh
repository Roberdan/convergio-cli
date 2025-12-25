#!/bin/bash
# V6 Release Preflight Check
# Validates all worktrees and branches before merge operations
set -e

echo "═══════════════════════════════════════════════════════════════"
echo "  CONVERGIO V6 PREFLIGHT CHECK"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════════════════════════════"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

ERRORS=0
WARNINGS=0

check_pass() {
    echo -e "${GREEN}✓${NC} $1"
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
    ((ERRORS++))
}

check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
    ((WARNINGS++))
}

# Worktree paths
MAIN_REPO="/Users/roberdan/GitHub/ConvergioCLI"
EDU_REPO="/Users/roberdan/GitHub/ConvergioCLI-education"
FEATURES_REPO="/Users/roberdan/GitHub/ConvergioCLI-features"
SCUOLA_REPO="/Users/roberdan/GitHub/ConvergioCLI/native-scuola-2026"
NATIVE_REPO="/Users/roberdan/GitHub/ConvergioNative"

echo ""
echo "1. WORKTREE VALIDATION"
echo "───────────────────────────────────────────────────────────────"

# Check each worktree exists
for repo in "$MAIN_REPO" "$EDU_REPO" "$FEATURES_REPO" "$SCUOLA_REPO" "$NATIVE_REPO"; do
    if [ -d "$repo" ]; then
        check_pass "Worktree exists: $repo"
    else
        check_fail "Worktree missing: $repo"
    fi
done

echo ""
echo "2. GIT STATUS CHECK (uncommitted changes)"
echo "───────────────────────────────────────────────────────────────"

for repo in "$MAIN_REPO" "$EDU_REPO" "$FEATURES_REPO" "$SCUOLA_REPO" "$NATIVE_REPO"; do
    if [ -d "$repo/.git" ] || [ -f "$repo/.git" ]; then
        status=$(cd "$repo" && git status --porcelain 2>/dev/null | wc -l | tr -d ' ')
        if [ "$status" -eq 0 ]; then
            check_pass "Clean working tree: $(basename $repo)"
        else
            check_warn "Uncommitted changes ($status files): $(basename $repo)"
        fi
    fi
done

echo ""
echo "3. BRANCH TRACKING CHECK"
echo "───────────────────────────────────────────────────────────────"

declare -A EXPECTED_BRANCHES
EXPECTED_BRANCHES["$MAIN_REPO"]="development"
EXPECTED_BRANCHES["$EDU_REPO"]="feature/education-pack"
EXPECTED_BRANCHES["$FEATURES_REPO"]="feature/convergio-enhancements"
EXPECTED_BRANCHES["$SCUOLA_REPO"]="feature/scuola-2026"
EXPECTED_BRANCHES["$NATIVE_REPO"]="feature/native-app"

for repo in "${!EXPECTED_BRANCHES[@]}"; do
    expected="${EXPECTED_BRANCHES[$repo]}"
    if [ -d "$repo" ]; then
        current=$(cd "$repo" && git branch --show-current 2>/dev/null)
        if [ "$current" = "$expected" ]; then
            check_pass "Correct branch: $current ($(basename $repo))"
        else
            check_warn "Expected branch '$expected', found '$current' ($(basename $repo))"
        fi
    fi
done

echo ""
echo "4. BUILD VALIDATION"
echo "───────────────────────────────────────────────────────────────"

if [ -f "$MAIN_REPO/Makefile" ]; then
    check_pass "Makefile found in main repo"
else
    check_fail "Makefile missing in main repo"
fi

echo ""
echo "5. CI/CD STATUS CHECK"
echo "───────────────────────────────────────────────────────────────"

if [ -d "$MAIN_REPO/.github/workflows" ]; then
    workflow_count=$(ls "$MAIN_REPO/.github/workflows"/*.yml 2>/dev/null | wc -l | tr -d ' ')
    check_pass "GitHub workflows found: $workflow_count"
else
    check_warn "No GitHub workflows directory found"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  PREFLIGHT SUMMARY"
echo "═══════════════════════════════════════════════════════════════"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}ALL CHECKS PASSED${NC}"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}PASSED WITH $WARNINGS WARNINGS${NC}"
    exit 0
else
    echo -e "${RED}FAILED: $ERRORS errors, $WARNINGS warnings${NC}"
    exit 1
fi
