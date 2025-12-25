#!/bin/bash
# Build Convergio Education Edition
# Usage: ./scripts/build-edu.sh [clean]

set -e

cd "$(dirname "$0")/.."

if [ "$1" = "clean" ]; then
    echo "Cleaning..."
    make clean
fi

echo "Building Convergio Education Edition..."
make EDITION=education -j$(sysctl -n hw.ncpu)

echo ""
echo "Build complete: ./build/bin/convergio-edu"
