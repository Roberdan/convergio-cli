#!/bin/bash
# Run all Convergio Education Edition tests
# Usage: ./scripts/test-edu.sh [--llm] [--verbose] [--section N]

set -e

cd "$(dirname "$0")/.."

# Colors
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

echo ""
echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}     CONVERGIO EDUCATION EDITION - TEST RUNNER                  ${NC}"
echo -e "${CYAN}════════════════════════════════════════════════════════════════${NC}"
echo ""

# Check if binary exists
if [ ! -x "./build/bin/convergio-edu" ]; then
    echo "Building education edition first..."
    make EDITION=education -j$(sysctl -n hw.ncpu)
fi

# Parse arguments
RUN_LLM=false
VERBOSE=""
SECTION=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --llm)
            RUN_LLM=true
            shift
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            shift
            ;;
        --section|-s)
            SECTION="--section $2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

# Run comprehensive tests (static)
echo -e "${GREEN}Running comprehensive tests...${NC}"
./tests/e2e_education_comprehensive_test.sh $VERBOSE $SECTION

# Run LLM tests if requested
if [ "$RUN_LLM" = true ]; then
    echo ""
    echo -e "${GREEN}Running real LLM interaction tests...${NC}"
    ./tests/e2e_education_llm_test.sh $VERBOSE $SECTION
fi

echo ""
echo -e "${GREEN}All tests completed!${NC}"
