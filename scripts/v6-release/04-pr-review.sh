#!/bin/bash
# V6 Release: GitHub PR Review and Cleanup
set -e

echo "═══════════════════════════════════════════════════════════════"
echo "  CONVERGIO V6 PR REVIEW & CLEANUP"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════════════════════════════"

REPO="Roberdan/convergio-cli"

echo ""
echo "1. OPEN & DRAFT PULL REQUESTS"
echo "───────────────────────────────────────────────────────────────"

echo ""
echo "Open PRs:"
gh pr list --repo "$REPO" --state open --json number,title,headRefName,createdAt --jq '.[] | "  #\(.number): \(.title) [\(.headRefName)]"' 2>/dev/null || echo "  (none or error)"

echo ""
echo "Draft PRs:"
gh pr list --repo "$REPO" --state open --json number,title,headRefName,isDraft --jq '.[] | select(.isDraft) | "  #\(.number): \(.title) [DRAFT]"' 2>/dev/null || echo "  (none or error)"

echo ""
echo "2. BRANCHES ANALYSIS"
echo "───────────────────────────────────────────────────────────────"

echo ""
echo "Remote feature branches:"
git branch -r | grep -E "feature/|release/" | head -20

echo ""
echo "3. PR STATUS CHECK"
echo "───────────────────────────────────────────────────────────────"

echo ""
echo "Checking PR #71 (education-pack)..."
gh pr view 71 --repo "$REPO" --json state,headRefName,mergeable,statusCheckRollup 2>/dev/null | jq '.' || echo "  Error or not found"

echo ""
echo "Checking PR #69 (Zed Integration)..."
gh pr view 69 --repo "$REPO" --json state,headRefName,mergeable,statusCheckRollup 2>/dev/null | jq '.' || echo "  Error or not found"

echo ""
echo "4. RECOMMENDED ACTIONS"
echo "───────────────────────────────────────────────────────────────"
echo ""
echo "Since V6 is being merged via development branch:"
echo ""
echo "  PR #71 (education-pack):"
echo "    → CLOSE after Phase 5 merge with comment:"
echo "    gh pr close 71 --comment \"Merged via development branch (V6 release)\""
echo ""
echo "  PR #69 (Zed Integration):"
echo "    → CHECK if included in scuola-2026, then:"
echo "    gh pr close 69 --comment \"Deferred to V7 or integrated via scuola-2026\""
echo ""
echo "  After V6 release:"
echo "    → Delete merged remote branches:"
echo "    git push origin --delete feature/education-pack"
echo "    git push origin --delete feature/convergio-enhancements"
echo "    git push origin --delete feature/scuola-2026"
echo "    git push origin --delete feature/native-app"
echo ""

echo "═══════════════════════════════════════════════════════════════"
echo "  REVIEW COMPLETE"
echo "═══════════════════════════════════════════════════════════════"
