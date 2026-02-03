#!/bin/bash
# GRAIN - Code Quality Tools
# Usage: ./scripts/format.sh [check|fix|tidy]

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="$PROJECT_ROOT/Source"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Find source files
find_sources() {
  find "$SOURCE_DIR" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null || true
}

# Check if tools are installed
check_tools() {
  local missing=()

  if ! command -v clang-format &>/dev/null; then
    missing+=("clang-format")
  fi

  if ! command -v clang-tidy &>/dev/null; then
    missing+=("clang-tidy")
  fi

  if [ ${#missing[@]} -ne 0 ]; then
    echo -e "${RED}Error: Missing tools: ${missing[*]}${NC}"
    echo ""
    echo "Install on macOS with Homebrew:"
    echo "  brew install llvm"
    echo "  # Add to PATH: export PATH=\"/opt/homebrew/opt/llvm/bin:\$PATH\""
    echo ""
    exit 1
  fi
}

# Format check (dry-run)
format_check() {
  echo -e "${YELLOW}Checking code formatting...${NC}"

  local files
  files=$(find_sources)

  if [ -z "$files" ]; then
    echo -e "${YELLOW}No source files found in $SOURCE_DIR${NC}"
    exit 0
  fi

  local failed=0

  for file in $files; do
    if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
      echo -e "${RED}  ✗ $file${NC}"
      failed=1
    else
      echo -e "${GREEN}  ✓ $file${NC}"
    fi
  done

  if [ $failed -eq 1 ]; then
    echo ""
    echo -e "${RED}Formatting issues found. Run './scripts/format.sh fix' to auto-fix.${NC}"
    exit 1
  fi

  echo -e "${GREEN}All files properly formatted!${NC}"
}

# Format fix (in-place)
format_fix() {
  echo -e "${YELLOW}Formatting source files...${NC}"

  local files
  files=$(find_sources)

  if [ -z "$files" ]; then
    echo -e "${YELLOW}No source files found in $SOURCE_DIR${NC}"
    exit 0
  fi

  for file in $files; do
    clang-format -i "$file"
    echo -e "${GREEN}  ✓ $file${NC}"
  done

  echo -e "${GREEN}Formatting complete!${NC}"
}

# Run clang-tidy
run_tidy() {
  echo -e "${YELLOW}Running clang-tidy...${NC}"

  local files
  files=$(find_sources)

  if [ -z "$files" ]; then
    echo -e "${YELLOW}No source files found in $SOURCE_DIR${NC}"
    exit 0
  fi

  # Check for compile_commands.json
  local compile_db=""
  if [ -f "$PROJECT_ROOT/build/compile_commands.json" ]; then
    compile_db="-p $PROJECT_ROOT/build"
  elif [ -f "$PROJECT_ROOT/Builds/compile_commands.json" ]; then
    compile_db="-p $PROJECT_ROOT/Builds"
  else
    echo -e "${YELLOW}Warning: No compile_commands.json found.${NC}"
    echo "  For best results, generate it with CMake:"
    echo "  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build"
    echo ""
  fi

  local failed=0

  for file in $files; do
    echo -e "${YELLOW}Analyzing: $file${NC}"
    if ! clang-tidy $compile_db "$file" -- \
      -std=c++17 \
      -I"$SOURCE_DIR" \
      -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
      2>&1 | grep -v "^$"; then
      failed=1
    fi
  done

  if [ $failed -eq 1 ]; then
    echo -e "${RED}clang-tidy found issues.${NC}"
    exit 1
  fi

  echo -e "${GREEN}clang-tidy analysis complete!${NC}"
}

# Show usage
usage() {
  echo "GRAIN Code Quality Tools"
  echo ""
  echo "Usage: $0 [command]"
  echo ""
  echo "Commands:"
  echo "  check   Check formatting (dry-run, no changes)"
  echo "  fix     Auto-fix formatting issues"
  echo "  tidy    Run clang-tidy static analysis"
  echo "  all     Run format check + clang-tidy"
  echo ""
  echo "Examples:"
  echo "  $0 check    # CI: verify formatting"
  echo "  $0 fix      # Dev: auto-format before commit"
  echo "  $0 tidy     # Dev: find potential bugs"
}

# Main
main() {
  check_tools

  case "${1:-}" in
  check)
    format_check
    ;;
  fix)
    format_fix
    ;;
  tidy)
    run_tidy
    ;;
  all)
    format_check
    echo ""
    run_tidy
    ;;
  *)
    usage
    exit 1
    ;;
  esac
}

main "$@"
