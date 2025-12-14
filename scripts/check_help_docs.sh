#!/bin/bash
#
# check_help_docs.sh - Verify all REPL commands are documented in help system
#
# This script ensures every command in the COMMANDS[] array has corresponding
# documentation in the DETAILED_HELP[] array.
#
# Exit codes:
#   0 - All commands are documented
#   1 - Missing documentation for one or more commands
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
COMMANDS_FILE="$PROJECT_ROOT/src/core/commands/commands.c"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if commands.c exists
if [[ ! -f "$COMMANDS_FILE" ]]; then
    echo -e "${RED}Error: $COMMANDS_FILE not found${NC}"
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}REPL Command Documentation Check${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Extract command names from COMMANDS[] array
# Format: {"command_name", "description", cmd_function},
echo -e "${YELLOW}Extracting commands from COMMANDS[] array...${NC}"
COMMAND_NAMES=$(grep -A 100 'static const ReplCommand COMMANDS\[\]' "$COMMANDS_FILE" | \
    grep '{"' | \
    sed -n 's/.*{"\([^"]*\)".*/\1/p' | \
    grep -v '^$' || true)

if [[ -z "$COMMAND_NAMES" ]]; then
    echo -e "${RED}Error: Could not extract command names from COMMANDS[] array${NC}"
    exit 1
fi

# Count total commands
TOTAL_COMMANDS=$(echo "$COMMAND_NAMES" | wc -l | tr -d ' ')
echo -e "${GREEN}Found $TOTAL_COMMANDS commands in COMMANDS[] array${NC}"
echo ""

# Extract documented command names from DETAILED_HELP[] array
# Format: {"command_name", "usage", "description", ...},
echo -e "${YELLOW}Extracting documented commands from DETAILED_HELP[] array...${NC}"
DOCUMENTED_NAMES=$(grep -A 1000 'static const CommandHelp DETAILED_HELP\[\]' "$COMMANDS_FILE" | \
    grep '^ *{$' -A 1 | \
    grep '"' | \
    sed -n 's/.*"\([^"]*\)".*/\1/p' | \
    grep -v '^$' || true)

if [[ -z "$DOCUMENTED_NAMES" ]]; then
    echo -e "${RED}Error: Could not extract documented command names from DETAILED_HELP[] array${NC}"
    exit 1
fi

# Count documented commands
DOCUMENTED_COUNT=$(echo "$DOCUMENTED_NAMES" | wc -l | tr -d ' ')
echo -e "${GREEN}Found $DOCUMENTED_COUNT documented commands in DETAILED_HELP[] array${NC}"
echo ""

# Check each command is documented
MISSING_DOCS=()
DOCUMENTED_LIST=()
SKIP_COMMANDS=("exit")  # Commands that are aliases and don't need separate help

echo -e "${YELLOW}Checking documentation coverage...${NC}"
echo ""

while IFS= read -r cmd; do
    # Skip empty lines
    [[ -z "$cmd" ]] && continue

    # Skip commands that are aliases
    skip=false
    for skip_cmd in "${SKIP_COMMANDS[@]}"; do
        if [[ "$cmd" == "$skip_cmd" ]]; then
            skip=true
            break
        fi
    done

    if [[ "$skip" == true ]]; then
        echo -e "  ${BLUE}◦${NC} $cmd (alias - skipped)"
        continue
    fi

    # Check if documented
    if echo "$DOCUMENTED_NAMES" | grep -qx "$cmd"; then
        echo -e "  ${GREEN}✓${NC} $cmd"
        DOCUMENTED_LIST+=("$cmd")
    else
        echo -e "  ${RED}✗${NC} $cmd ${RED}(MISSING DOCUMENTATION)${NC}"
        MISSING_DOCS+=("$cmd")
    fi
done <<< "$COMMAND_NAMES"

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

CHECKED_COUNT=$((TOTAL_COMMANDS - ${#SKIP_COMMANDS[@]}))
FOUND_COUNT=${#DOCUMENTED_LIST[@]}
MISSING_COUNT=${#MISSING_DOCS[@]}

echo "Total commands:       $TOTAL_COMMANDS"
echo "Skipped (aliases):    ${#SKIP_COMMANDS[@]}"
echo "Checked:              $CHECKED_COUNT"
echo ""
echo -e "Documented:           ${GREEN}$FOUND_COUNT${NC}"
echo -e "Missing docs:         ${RED}$MISSING_COUNT${NC}"
echo ""

if [[ $MISSING_COUNT -eq 0 ]]; then
    echo -e "${GREEN}✓ All commands are properly documented!${NC}"
    echo ""
    exit 0
else
    echo -e "${RED}✗ The following commands need documentation in DETAILED_HELP[]:${NC}"
    echo ""
    for cmd in "${MISSING_DOCS[@]}"; do
        echo -e "  ${RED}•${NC} $cmd"
    done
    echo ""
    echo -e "${YELLOW}Action required:${NC}"
    echo "Add entries for these commands in the DETAILED_HELP[] array in:"
    echo "  $COMMANDS_FILE"
    echo ""
    exit 1
fi
