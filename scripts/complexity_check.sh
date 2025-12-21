#!/bin/bash
#
# Code Complexity Check Script
# Analyzes code complexity and reports files that exceed thresholds
#

set -e

# Thresholds
MAX_NESTING=5
MAX_FUNCTION_LENGTH=100
MAX_FILE_LENGTH=500

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "🔍 Analyzing code complexity..."

FAILED=0
FILES_CHECKED=0

# Check each C file
for file in $(find src -name "*.c" | grep -v ".build" | grep -v "test_stubs"); do
    FILES_CHECKED=$((FILES_CHECKED + 1))
    
    # Count lines
    LINES=$(wc -l < "$file" | tr -d ' ')
    
    # Count nesting depth (approximate: count consecutive {)
    NESTING=$(grep -o '{' "$file" | wc -l | tr -d ' ')
    # Rough estimate: divide by function count
    FUNC_COUNT=$(grep -c "^[a-zA-Z_].*(" "$file" || echo "1")
    AVG_NESTING=$((NESTING / FUNC_COUNT))
    
    # Count long functions (functions with > MAX_FUNCTION_LENGTH lines)
    LONG_FUNCS=$(awk '/^[a-zA-Z_].*\(/ {start=NR} /^}/ && start {if(NR-start > '"$MAX_FUNCTION_LENGTH"') count++; start=0} END {print count+0}' "$file")
    
    ISSUES=0
    
    if [ "$LINES" -gt "$MAX_FILE_LENGTH" ]; then
        echo "${YELLOW}⚠️  $file: Too long ($LINES lines, max $MAX_FILE_LENGTH)${NC}"
        ISSUES=$((ISSUES + 1))
    fi
    
    if [ "$AVG_NESTING" -gt "$MAX_NESTING" ]; then
        echo "${YELLOW}⚠️  $file: High nesting depth (avg $AVG_NESTING, max $MAX_NESTING)${NC}"
        ISSUES=$((ISSUES + 1))
    fi
    
    if [ "$LONG_FUNCS" -gt 0 ]; then
        echo "${YELLOW}⚠️  $file: $LONG_FUNCS functions exceed $MAX_FUNCTION_LENGTH lines${NC}"
        ISSUES=$((ISSUES + 1))
    fi
    
    if [ "$ISSUES" -gt 0 ]; then
        FAILED=$((FAILED + 1))
    fi
done

echo ""
if [ "$FAILED" -eq 0 ]; then
    echo "${GREEN}✅ PASSED: All $FILES_CHECKED files within complexity limits${NC}"
    exit 0
else
    echo "${YELLOW}⚠️  WARNING: $FAILED files exceed complexity thresholds${NC}"
    echo "   Consider refactoring to reduce complexity"
    exit 0  # Don't fail, just warn
fi

