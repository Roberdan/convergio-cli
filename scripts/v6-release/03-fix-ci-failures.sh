#!/bin/bash
# V6 Release: Fix known CI failures
# This script addresses the specific CI failures identified during analysis
set -e

echo "═══════════════════════════════════════════════════════════════"
echo "  CONVERGIO V6 CI FAILURE FIXES"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════════════════════════════"

EDU_REPO="/Users/roberdan/GitHub/ConvergioCLI-education"

echo ""
echo "1. EDUCATION-PACK: Fix unused variable warning"
echo "───────────────────────────────────────────────────────────────"

# The specific issue: src/orchestrator/registry.c:1448:22: warning: unused variable 'current_edition'
REGISTRY_FILE="$EDU_REPO/src/orchestrator/registry.c"

if [ -f "$REGISTRY_FILE" ]; then
    echo "Checking $REGISTRY_FILE..."

    # Check if the warning exists
    if grep -n "current_edition" "$REGISTRY_FILE" > /dev/null 2>&1; then
        echo "Found 'current_edition' variable at:"
        grep -n "current_edition" "$REGISTRY_FILE"

        echo ""
        echo "Options to fix:"
        echo "  1. Use the variable in the code"
        echo "  2. Mark it with __attribute__((unused))"
        echo "  3. Remove if truly unused"
        echo ""
        echo "Manual review required. Line around 1448."
    else
        echo "Variable 'current_edition' not found. May already be fixed."
    fi
else
    echo "Registry file not found at expected path."
    echo "Searching for registry.c..."
    find "$EDU_REPO" -name "registry.c" 2>/dev/null | head -5
fi

echo ""
echo "2. CHECKING OTHER COMMON CI ISSUES"
echo "───────────────────────────────────────────────────────────────"

# Check for other common warning patterns
for repo in "/Users/roberdan/GitHub/ConvergioCLI" "$EDU_REPO"; do
    if [ -d "$repo" ]; then
        echo ""
        echo "==> $(basename $repo)"

        # Look for unused variable patterns in C code
        echo "  Checking for unused variables..."
        find "$repo/src" -name "*.c" -exec grep -l "unused\|__attribute__" {} \; 2>/dev/null | head -5 || echo "  (none found)"

        # Check for missing includes that cause warnings
        echo "  Checking for potential missing includes..."
        find "$repo/src" -name "*.c" -exec grep -l "implicit declaration" {} \; 2>/dev/null | head -5 || echo "  (none found)"
    fi
done

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  CI FIX ANALYSIS COMPLETE"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Next steps:"
echo "  1. Review the identified issues above"
echo "  2. Apply fixes to the specific files"
echo "  3. Run 'make' to verify warnings are resolved"
echo "  4. Commit fixes with: git commit -m 'fix(ci): resolve unused variable warnings'"
