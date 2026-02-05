#!/bin/bash
# Run GRAIN unit tests via Standalone app (Debug build)
# Tests execute automatically on plugin load when JUCE_DEBUG is defined

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
XCODEPROJ="$PROJECT_DIR/Builds/MacOSX/GRAIN.xcodeproj"
APP_PATH="$PROJECT_DIR/Builds/MacOSX/build/Debug/GRAIN.app/Contents/MacOS/GRAIN"

echo "=== GRAIN Test Runner ==="
echo ""

# Check if Xcode project exists
if [ ! -d "$XCODEPROJ" ]; then
    echo "Error: Xcode project not found at $XCODEPROJ"
    echo "Run Projucer to generate it first:"
    echo "  ~/JUCE/Projucer.app/Contents/MacOS/Projucer --resave $PROJECT_DIR/GRAIN.jucer"
    exit 1
fi

# Build Standalone in Debug mode
echo "[1/2] Building Standalone (Debug)..."
xcodebuild -project "$XCODEPROJ" \
    -scheme "GRAIN - Standalone Plugin" \
    -configuration Debug \
    build \
    -quiet

if [ $? -ne 0 ]; then
    echo "Error: Build failed"
    exit 1
fi

echo "[2/2] Running tests..."
echo ""

# Run the app - tests execute on load and print to stdout
# The app will open a window, so we run it and capture output
# Use timeout to auto-close after tests complete
if command -v timeout &> /dev/null; then
    timeout 10 "$APP_PATH" 2>&1 || true
else
    # macOS doesn't have timeout by default, use a background process
    "$APP_PATH" &
    APP_PID=$!
    sleep 3
    kill $APP_PID 2>/dev/null || true
fi

echo ""
echo "=== Tests complete ==="
