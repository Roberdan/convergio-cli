#!/bin/bash
# Convergio Build and Test Helper
# Usage:
#   ./scripts/convergio.sh build [edition]    # Build specific edition
#   ./scripts/convergio.sh test [edition]     # Test specific edition
#   ./scripts/convergio.sh all                # Build and test all editions
#
# Editions: education, business, developer, master (default)

set -e

cd "$(dirname "$0")/.."

# Colors
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

COMMAND="${1:-help}"
EDITION="${2:-master}"

show_help() {
    echo ""
    echo "Convergio Build and Test Helper"
    echo ""
    echo "Usage:"
    echo "  ./scripts/convergio.sh build [edition]    Build specific edition"
    echo "  ./scripts/convergio.sh test [edition]     Test specific edition"
    echo "  ./scripts/convergio.sh all                Build and test all editions"
    echo ""
    echo "Editions:"
    echo "  education   Education Edition (convergio-edu)"
    echo "  business    Business Edition (convergio-biz)"
    echo "  developer   Developer Edition (convergio-dev)"
    echo "  master      Master Edition (convergio)"
    echo ""
    echo "Examples:"
    echo "  ./scripts/convergio.sh build education"
    echo "  ./scripts/convergio.sh test education"
    echo "  ./scripts/convergio.sh test education --llm --verbose"
    echo ""
}

build_edition() {
    local edition="$1"
    echo -e "${CYAN}Building $edition edition...${NC}"
    make EDITION=$edition -j$(sysctl -n hw.ncpu)
    echo -e "${GREEN}Built successfully${NC}"
}

test_edition() {
    local edition="$1"
    shift
    local args="$@"

    echo -e "${CYAN}Testing $edition edition...${NC}"

    case $edition in
        education)
            if [ ! -x "./build/bin/convergio-edu" ]; then
                build_edition education
            fi
            ./tests/e2e_education_comprehensive_test.sh $args
            ;;
        business)
            if [ ! -x "./build/bin/convergio-biz" ]; then
                build_edition business
            fi
            if [ -f "./tests/e2e_business_test.sh" ]; then
                ./tests/e2e_business_test.sh $args
            else
                echo -e "${YELLOW}No business tests found${NC}"
            fi
            ;;
        developer)
            if [ ! -x "./build/bin/convergio-dev" ]; then
                build_edition developer
            fi
            if [ -f "./tests/e2e_developer_test.sh" ]; then
                ./tests/e2e_developer_test.sh $args
            else
                echo -e "${YELLOW}No developer tests found${NC}"
            fi
            ;;
        master)
            if [ ! -x "./build/bin/convergio" ]; then
                build_edition master
            fi
            if [ -f "./tests/e2e_master_test.sh" ]; then
                ./tests/e2e_master_test.sh $args
            else
                echo -e "${YELLOW}No master tests found${NC}"
            fi
            ;;
    esac
}

case $COMMAND in
    build)
        build_edition "$EDITION"
        ;;
    test)
        shift  # Remove 'test'
        shift  # Remove edition
        test_edition "$EDITION" "$@"
        ;;
    all)
        echo -e "${CYAN}Building and testing all editions...${NC}"
        for ed in education business developer master; do
            echo ""
            echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
            echo -e "${CYAN}  Edition: $ed${NC}"
            echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
            build_edition $ed
            test_edition $ed
        done
        ;;
    help|*)
        show_help
        ;;
esac
