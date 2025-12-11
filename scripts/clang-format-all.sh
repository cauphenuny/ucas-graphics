#!/usr/bin/env bash
set -euo pipefail

# Format all .cc/.cpp/.h/.hpp files in this git repository recursively.
REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "Error: clang-format not found. Please install clang-format." >&2
  exit 1
fi

# Exclude common build/output directories and .git
find "$REPO_ROOT" \
  -type d \( -name .git -o -name build -o -name .build -o -name out -o -name dist -o -name external -o -path "*/external/*" \) -prune -o \
  -type f \( -name '*.cc' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
  -exec clang-format -i {} +

echo "clang-format applied."