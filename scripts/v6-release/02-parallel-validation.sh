#!/bin/bash
# V6 Release: Parallel validation across all worktrees
set -e

echo "═══════════════════════════════════════════════════════════════"
echo "  CONVERGIO V6 PARALLEL VALIDATION"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════════════════════════════"

MAIN_REPO="/Users/roberdan/GitHub/ConvergioCLI"
EDU_REPO="/Users/roberdan/GitHub/ConvergioCLI-education"
FEATURES_REPO="/Users/roberdan/GitHub/ConvergioCLI-features"
SCUOLA_REPO="/Users/roberdan/GitHub/ConvergioCLI/native-scuola-2026"
NATIVE_REPO="/Users/roberdan/GitHub/ConvergioNative"

RESULTS_DIR="$MAIN_REPO/scripts/v6-release/results"
mkdir -p "$RESULTS_DIR"

# Function to run build validation in background
validate_build() {
    local repo=$1
    local name=$(basename "$repo")
    local result_file="$RESULTS_DIR/$name-build.txt"

    echo "Starting build validation: $name"

    if [ -f "$repo/Makefile" ]; then
        (cd "$repo" && make clean 2>/dev/null; make 2>&1) > "$result_file" 2>&1
        if [ $? -eq 0 ]; then
            echo "PASS" >> "$result_file"
        else
            echo "FAIL" >> "$result_file"
        fi
    elif [ -f "$repo/Package.swift" ]; then
        (cd "$repo" && swift build 2>&1) > "$result_file" 2>&1
        if [ $? -eq 0 ]; then
            echo "PASS" >> "$result_file"
        else
            echo "FAIL" >> "$result_file"
        fi
    else
        echo "NO BUILD SYSTEM" > "$result_file"
        echo "SKIP" >> "$result_file"
    fi
}

# Function to run tests in background
validate_tests() {
    local repo=$1
    local name=$(basename "$repo")
    local result_file="$RESULTS_DIR/$name-tests.txt"

    echo "Starting test validation: $name"

    if [ -f "$repo/Makefile" ]; then
        (cd "$repo" && make test 2>&1) > "$result_file" 2>&1
        if [ $? -eq 0 ]; then
            echo "PASS" >> "$result_file"
        else
            echo "FAIL" >> "$result_file"
        fi
    elif [ -f "$repo/Package.swift" ]; then
        (cd "$repo" && swift test 2>&1) > "$result_file" 2>&1
        if [ $? -eq 0 ]; then
            echo "PASS" >> "$result_file"
        else
            echo "FAIL" >> "$result_file"
        fi
    else
        echo "NO TEST SYSTEM" > "$result_file"
        echo "SKIP" >> "$result_file"
    fi
}

# Function to check for warnings
validate_warnings() {
    local repo=$1
    local name=$(basename "$repo")
    local result_file="$RESULTS_DIR/$name-warnings.txt"

    echo "Checking for warnings: $name"

    if [ -f "$repo/Makefile" ]; then
        warnings=$(cd "$repo" && make 2>&1 | grep -c -E "warning:|Warning:" || true)
        echo "Warnings found: $warnings" > "$result_file"
        if [ "$warnings" -eq 0 ]; then
            echo "PASS" >> "$result_file"
        else
            echo "WARN" >> "$result_file"
        fi
    else
        echo "SKIP" > "$result_file"
    fi
}

echo ""
echo "Running validations in parallel..."
echo "───────────────────────────────────────────────────────────────"

# Run all validations in parallel
pids=()

for repo in "$MAIN_REPO" "$EDU_REPO" "$SCUOLA_REPO"; do
    if [ -d "$repo" ]; then
        validate_build "$repo" &
        pids+=($!)
    fi
done

# Wait for all background processes
for pid in "${pids[@]}"; do
    wait $pid 2>/dev/null || true
done

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  VALIDATION RESULTS"
echo "═══════════════════════════════════════════════════════════════"

TOTAL_PASS=0
TOTAL_FAIL=0
TOTAL_SKIP=0

for result_file in "$RESULTS_DIR"/*.txt; do
    if [ -f "$result_file" ]; then
        name=$(basename "$result_file" .txt)
        status=$(tail -1 "$result_file")

        case "$status" in
            PASS)
                echo "✓ $name: PASS"
                ((TOTAL_PASS++))
                ;;
            FAIL)
                echo "✗ $name: FAIL"
                ((TOTAL_FAIL++))
                ;;
            WARN)
                echo "⚠ $name: WARNINGS"
                ((TOTAL_FAIL++))
                ;;
            SKIP)
                echo "○ $name: SKIPPED"
                ((TOTAL_SKIP++))
                ;;
        esac
    fi
done

echo ""
echo "Summary: $TOTAL_PASS passed, $TOTAL_FAIL failed, $TOTAL_SKIP skipped"

if [ $TOTAL_FAIL -gt 0 ]; then
    echo ""
    echo "⚠️  VALIDATION FAILED - Review results in $RESULTS_DIR"
    exit 1
else
    echo ""
    echo "✓ ALL VALIDATIONS PASSED"
    exit 0
fi
