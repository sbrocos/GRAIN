#!/bin/bash
# Run GRAIN unit tests via the GRAINTests console application
# Returns exit code 0 on success, 1 on failure (CI-friendly)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PROJUCER="/Users/sbrocos/JUCE/Projucer.app/Contents/MacOS/Projucer"
TESTS_JUCER="$PROJECT_DIR/GRAINTests.jucer"
TESTS_XCODEPROJ="$PROJECT_DIR/Builds/MacOSX-Tests/GRAINTests.xcodeproj"
TESTS_BINARY="$PROJECT_DIR/Builds/MacOSX-Tests/build/Debug/GRAINTests"

echo "=== GRAIN Test Runner ==="
echo ""

# Regenerate Xcode project if needed
if [ ! -d "$TESTS_XCODEPROJ" ]; then
    echo "[0/2] Generating Xcode project..."
    if [ -x "$PROJUCER" ]; then
        "$PROJUCER" --resave "$TESTS_JUCER"
    else
        echo "Error: Projucer not found at $PROJUCER"
        echo "Generate the Xcode project manually first."
        exit 1
    fi
fi

# Build GRAINTests console app
echo "[1/2] Building GRAINTests (Debug)..."
xcodebuild -project "$TESTS_XCODEPROJ" \
    -scheme "GRAINTests - ConsoleApp" \
    -configuration Debug \
    build \
    -quiet

echo "[2/2] Running tests..."
echo ""

# Run tests — exit code reflects pass/fail
"$TESTS_BINARY"
EXIT_CODE=$?

echo ""
echo "=== Tests complete ==="
exit $EXIT_CODE
