#!/bin/bash
set -e
cd "$(dirname "$0")/.."

echo "Regenerando compile_commands.json..."
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj -scheme "GRAIN - All" clean
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj -scheme "GRAIN - All" build 2>&1 | tee build.log
compiledb --parse build.log
rm build.log

echo "✓ Listo"
