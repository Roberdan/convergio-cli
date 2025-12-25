#!/bin/bash
# V6 Release: Find all TODOs, FIXMEs, stubs, and technical debt
set -e

echo "═══════════════════════════════════════════════════════════════"
echo "  CONVERGIO V6 TECHNICAL DEBT SCANNER"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════════════════════════════"

# Worktree paths
REPOS=(
    "/Users/roberdan/GitHub/ConvergioCLI"
    "/Users/roberdan/GitHub/ConvergioCLI-education"
    "/Users/roberdan/GitHub/ConvergioCLI-features"
    "/Users/roberdan/GitHub/ConvergioCLI/native-scuola-2026"
    "/Users/roberdan/GitHub/ConvergioNative"
)

OUTPUT_FILE="/Users/roberdan/GitHub/ConvergioCLI/scripts/v6-release/debt-report.txt"

> "$OUTPUT_FILE"

echo "" | tee -a "$OUTPUT_FILE"
echo "1. TODO ITEMS" | tee -a "$OUTPUT_FILE"
echo "───────────────────────────────────────────────────────────────" | tee -a "$OUTPUT_FILE"

for repo in "${REPOS[@]}"; do
    if [ -d "$repo" ]; then
        echo "" | tee -a "$OUTPUT_FILE"
        echo "==> $(basename $repo)" | tee -a "$OUTPUT_FILE"
        cd "$repo"
        rg -i "TODO|FIXME|XXX|HACK" \
            --type c --type swift --type python --type js --type ts \
            -g '!node_modules/*' -g '!*.lock' -g '!build/*' \
            --line-number 2>/dev/null | head -50 | tee -a "$OUTPUT_FILE" || echo "  (none found)"
    fi
done

echo "" | tee -a "$OUTPUT_FILE"
echo "2. STUB IMPLEMENTATIONS" | tee -a "$OUTPUT_FILE"
echo "───────────────────────────────────────────────────────────────" | tee -a "$OUTPUT_FILE"

for repo in "${REPOS[@]}"; do
    if [ -d "$repo" ]; then
        echo "" | tee -a "$OUTPUT_FILE"
        echo "==> $(basename $repo)" | tee -a "$OUTPUT_FILE"
        cd "$repo"
        rg -i "(stub|placeholder|not implemented|unimplemented|fatalError|preconditionFailure)" \
            --type c --type swift --type python --type js --type ts \
            -g '!node_modules/*' -g '!*.lock' -g '!build/*' \
            --line-number 2>/dev/null | head -30 | tee -a "$OUTPUT_FILE" || echo "  (none found)"
    fi
done

echo "" | tee -a "$OUTPUT_FILE"
echo "3. DEPRECATED CODE" | tee -a "$OUTPUT_FILE"
echo "───────────────────────────────────────────────────────────────" | tee -a "$OUTPUT_FILE"

for repo in "${REPOS[@]}"; do
    if [ -d "$repo" ]; then
        echo "" | tee -a "$OUTPUT_FILE"
        echo "==> $(basename $repo)" | tee -a "$OUTPUT_FILE"
        cd "$repo"
        rg -i "deprecated|obsolete" \
            --type c --type swift --type python --type js --type ts \
            -g '!node_modules/*' -g '!*.lock' -g '!build/*' \
            --line-number 2>/dev/null | head -20 | tee -a "$OUTPUT_FILE" || echo "  (none found)"
    fi
done

echo "" | tee -a "$OUTPUT_FILE"
echo "4. HARDCODED VALUES (potential config issues)" | tee -a "$OUTPUT_FILE"
echo "───────────────────────────────────────────────────────────────" | tee -a "$OUTPUT_FILE"

for repo in "${REPOS[@]}"; do
    if [ -d "$repo" ]; then
        echo "" | tee -a "$OUTPUT_FILE"
        echo "==> $(basename $repo)" | tee -a "$OUTPUT_FILE"
        cd "$repo"
        rg -i "hardcoded|magic number|temp|temporary" \
            --type c --type swift --type python --type js --type ts \
            -g '!node_modules/*' -g '!*.lock' -g '!build/*' \
            --line-number 2>/dev/null | head -20 | tee -a "$OUTPUT_FILE" || echo "  (none found)"
    fi
done

echo "" | tee -a "$OUTPUT_FILE"
echo "5. COMMENTED OUT CODE" | tee -a "$OUTPUT_FILE"
echo "───────────────────────────────────────────────────────────────" | tee -a "$OUTPUT_FILE"

for repo in "${REPOS[@]}"; do
    if [ -d "$repo" ]; then
        echo "" | tee -a "$OUTPUT_FILE"
        echo "==> $(basename $repo)" | tee -a "$OUTPUT_FILE"
        cd "$repo"
        # Look for patterns that suggest commented code
        rg "^[[:space:]]*(//|#|/\*)[[:space:]]*(if|for|while|return|func|def|class|struct)" \
            --type c --type swift --type python --type js --type ts \
            -g '!node_modules/*' -g '!*.lock' -g '!build/*' \
            --line-number 2>/dev/null | head -20 | tee -a "$OUTPUT_FILE" || echo "  (none found)"
    fi
done

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  SCAN COMPLETE"
echo "  Full report: $OUTPUT_FILE"
echo "═══════════════════════════════════════════════════════════════"

# Count issues
TODO_COUNT=$(rg -c -i "TODO|FIXME|XXX|HACK" "${REPOS[@]}" --type c --type swift --type python 2>/dev/null | awk -F: '{sum+=$2} END {print sum}' || echo "0")
echo "Total TODO/FIXME items: $TODO_COUNT"

if [ "${TODO_COUNT:-0}" -gt 0 ]; then
    echo ""
    echo "⚠️  WARNING: Technical debt items found. Review before V6 release."
    exit 1
else
    echo ""
    echo "✓ No critical technical debt items found."
    exit 0
fi
