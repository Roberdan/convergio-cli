#!/bin/sh
#
# Convergio Pre-Commit Hook
# Enforces zero tolerance policy before every commit
#
# Installation: ln -s ../../scripts/pre-commit-hook.sh .git/hooks/pre-commit
#

set -e

echo "🔍 Running pre-commit checks..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track if any check fails
FAILED=0

# 1. Quick build check (fast feedback)
echo "  📦 Checking build..."
if ! make clean >/dev/null 2>&1; then
    echo "${RED}❌ Clean failed${NC}"
    FAILED=1
fi

if ! make >/dev/null 2>&1; then
    echo "${RED}❌ BUILD FAILED - Commit aborted${NC}"
    echo "   Fix compilation errors before committing."
    exit 1
fi

# 2. Warning check
echo "  ⚠️  Checking for warnings..."
if make 2>&1 | grep -i "warning:" >/dev/null; then
    echo "${RED}❌ WARNINGS FOUND - Commit aborted${NC}"
    echo "   Fix all warnings before committing."
    echo "   Warnings found:"
    make 2>&1 | grep -i "warning:" | head -5
    exit 1
fi

# 3. Quick test check (fast feedback - only critical tests)
echo "  🧪 Running quick tests..."
if command -v make >/dev/null 2>&1; then
    # Try to run quick tests if they exist
    if make help 2>/dev/null | grep -q "test_quick"; then
        if ! make test_quick >/dev/null 2>&1; then
            echo "${YELLOW}⚠️  Quick tests failed (non-blocking)${NC}"
            # Don't fail on quick tests, but warn
        fi
    fi
fi

# 4. Security scan (basic - check for dangerous functions)
echo "  🔒 Running security scan..."
DANGEROUS_FUNCS=$(git diff --cached --name-only --diff-filter=ACM | \
    grep '\.c$' | \
    xargs grep -lE "(strcpy|strcat|gets|sprintf)\s*\(" 2>/dev/null || true)

if [ -n "$DANGEROUS_FUNCS" ]; then
    echo "${RED}❌ DANGEROUS FUNCTIONS FOUND - Commit aborted${NC}"
    echo "   Use safe alternatives (strncpy, snprintf, etc.)"
    echo "   Files with dangerous functions:"
    echo "$DANGEROUS_FUNCS" | sed 's/^/   - /'
    exit 1
fi

# 5. Check for large files (prevent accidental commits)
echo "  📏 Checking file sizes..."
LARGE_FILES=$(git diff --cached --name-only --diff-filter=ACM | \
    xargs -I {} sh -c 'test -f {} && find {} -size +1M' 2>/dev/null || true)

if [ -n "$LARGE_FILES" ]; then
    echo "${YELLOW}⚠️  Large files detected (>1MB)${NC}"
    echo "$LARGE_FILES" | sed 's/^/   - /'
    echo "   Are you sure you want to commit these files?"
    read -p "   Continue? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 6. Check for merge conflicts markers
echo "  🔍 Checking for merge conflict markers..."
CONFLICT_MARKERS=$(git diff --cached | grep -E "^\+.*(<<<<<<|======|>>>>>>)" || true)

if [ -n "$CONFLICT_MARKERS" ]; then
    echo "${RED}❌ MERGE CONFLICT MARKERS FOUND - Commit aborted${NC}"
    echo "   Resolve all merge conflicts before committing."
    exit 1
fi

# All checks passed
if [ $FAILED -eq 0 ]; then
    echo "${GREEN}✅ Pre-commit checks passed${NC}"
    exit 0
else
    echo "${RED}❌ Pre-commit checks failed${NC}"
    exit 1
fi

