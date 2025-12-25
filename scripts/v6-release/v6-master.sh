#!/bin/bash
# V6 Release Master Orchestration Script
# This script coordinates the entire V6 release process
set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║           CONVERGIO V6 RELEASE ORCHESTRATION                  ║"
echo "║           $(date '+%Y-%m-%d %H:%M:%S')                               ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAIN_REPO="/Users/roberdan/GitHub/ConvergioCLI"
LOG_FILE="$SCRIPT_DIR/v6-release-$(date '+%Y%m%d-%H%M%S').log"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() {
    echo -e "$1" | tee -a "$LOG_FILE"
}

phase_header() {
    log ""
    log "${BLUE}╔═══════════════════════════════════════════════════════════════╗${NC}"
    log "${BLUE}║  PHASE $1: $2${NC}"
    log "${BLUE}╚═══════════════════════════════════════════════════════════════╝${NC}"
    log ""
}

check_result() {
    if [ $1 -eq 0 ]; then
        log "${GREEN}✓ $2 completed successfully${NC}"
    else
        log "${RED}✗ $2 failed${NC}"
        if [ "$3" = "critical" ]; then
            log "${RED}CRITICAL FAILURE - Aborting release process${NC}"
            exit 1
        fi
    fi
}

# ═══════════════════════════════════════════════════════════════
# PHASE 0: PREFLIGHT CHECKS
# ═══════════════════════════════════════════════════════════════
phase_header "0" "PREFLIGHT CHECKS"

log "Running preflight validation..."
"$SCRIPT_DIR/00-preflight-check.sh" 2>&1 | tee -a "$LOG_FILE"
check_result $? "Preflight checks" "critical"

# ═══════════════════════════════════════════════════════════════
# PHASE 1: TECHNICAL DEBT SCAN
# ═══════════════════════════════════════════════════════════════
phase_header "1" "TECHNICAL DEBT SCAN"

log "Scanning for TODOs, stubs, and technical debt..."
"$SCRIPT_DIR/01-find-stubs-todos.sh" 2>&1 | tee -a "$LOG_FILE" || true
# Note: We continue even if debt is found, but log it

# ═══════════════════════════════════════════════════════════════
# PHASE 2: CI FIXES
# ═══════════════════════════════════════════════════════════════
phase_header "2" "CI FAILURE ANALYSIS"

log "Analyzing CI failures..."
"$SCRIPT_DIR/03-fix-ci-failures.sh" 2>&1 | tee -a "$LOG_FILE" || true

# ═══════════════════════════════════════════════════════════════
# PHASE 3: PARALLEL VALIDATION
# ═══════════════════════════════════════════════════════════════
phase_header "3" "PARALLEL BUILD VALIDATION"

log "Running parallel build and test validation..."
"$SCRIPT_DIR/02-parallel-validation.sh" 2>&1 | tee -a "$LOG_FILE"
check_result $? "Parallel validation" "warn"

# ═══════════════════════════════════════════════════════════════
# SUMMARY
# ═══════════════════════════════════════════════════════════════
log ""
log "╔═══════════════════════════════════════════════════════════════╗"
log "║           V6 RELEASE PREPARATION COMPLETE                     ║"
log "╚═══════════════════════════════════════════════════════════════╝"
log ""
log "Log file: $LOG_FILE"
log ""
log "Next Steps:"
log "  1. Review the log file for any warnings or issues"
log "  2. Fix any CI failures identified in Phase 2"
log "  3. Proceed with merge operations as per V6ReleasePlan.md"
log ""
log "Merge Order (from V6ReleasePlan.md):"
log "  1. feature/convergio-enhancements → development"
log "  2. feature/scuola-2026 → development"
log "  3. Cherry-pick unique commits from feature/native-app"
log "  4. feature/education-pack → development (CAREFUL - 100+ conflicts)"
log ""
log "Use: git merge --no-ff <branch> for each merge"
