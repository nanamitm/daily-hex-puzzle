#!/bin/sh
# Build script for Linux / macOS
CONFIG=${1:-Debug}
BUILD_DIR="build/$CONFIG"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG" && \
cmake --build "$BUILD_DIR" --config "$CONFIG" -j$(nproc 2>/dev/null || sysctl -n hw.logicalcpu) && \
echo "Build OK: $BUILD_DIR/DailyHexPuzzle"
