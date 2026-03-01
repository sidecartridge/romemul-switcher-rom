#!/bin/sh
set -u

MODE="${1:-check}"

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"

DEFAULT_CLANG_FORMAT="/usr/local/opt/llvm/bin/clang-format"
DEFAULT_CLANG_TIDY="/usr/local/opt/llvm/bin/clang-tidy"

if [ -x "$DEFAULT_CLANG_FORMAT" ]; then
  CLANG_FORMAT_BIN="${CLANG_FORMAT_BIN:-$DEFAULT_CLANG_FORMAT}"
else
  CLANG_FORMAT_BIN="${CLANG_FORMAT_BIN:-clang-format}"
fi

if [ -x "$DEFAULT_CLANG_TIDY" ]; then
  CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-$DEFAULT_CLANG_TIDY}"
else
  CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-clang-tidy}"
fi

if [ ! -f ".clang-format" ]; then
  echo "Missing .clang-format in project root" >&2
  exit 1
fi

TIDY_CONFIG_FILE=".clang-tidy"
if [ ! -f "$TIDY_CONFIG_FILE" ] && [ -f ".clang-tify" ]; then
  TIDY_CONFIG_FILE=".clang-tify"
fi

if [ ! -f "$TIDY_CONFIG_FILE" ]; then
  echo "Missing .clang-tidy (or .clang-tify) in project root" >&2
  exit 1
fi

if [ ! -x "$(command -v "$CLANG_FORMAT_BIN")" ]; then
  echo "clang-format not found: $CLANG_FORMAT_BIN" >&2
  exit 1
fi

if [ ! -x "$(command -v "$CLANG_TIDY_BIN")" ]; then
  echo "clang-tidy not found: $CLANG_TIDY_BIN" >&2
  exit 1
fi

IGNORE_PATTERNS="$(sed -n 's/^[[:space:]]*-[[:space:]]*src:[[:space:]]*"\(.*\)".*/\1/p' .clang-tidy-ignore 2>/dev/null || true)"

is_ignored() {
  file="$1"
  for pattern in $IGNORE_PATTERNS; do
    case "$file" in
      $pattern) return 0 ;;
    esac
  done
  return 1
}

collect_files() {
  rg --files src -g '*.c' -g '*.h' | while IFS= read -r file; do
    if ! is_ignored "$file"; then
      printf '%s\n' "$file"
    fi
  done
}

VERSION="0.0.0"
if [ -f "version.txt" ]; then
  VERSION="$(tr -d '\r\n' < version.txt)"
fi

TIDY_FLAGS="
  -std=gnu99
  -ffreestanding
  -DAPP_VERSION_STR=\"$VERSION\"
  -Isrc/st
  -Isrc/common
"

TMP_FILE_LIST="$(mktemp)"
trap 'rm -f "$TMP_FILE_LIST"' EXIT INT TERM
collect_files > "$TMP_FILE_LIST"

run_format() {
  while IFS= read -r file; do
    "$CLANG_FORMAT_BIN" -i "$file"
  done < "$TMP_FILE_LIST"
}

run_format_check() {
  failed=0
  while IFS= read -r file; do
    if ! "$CLANG_FORMAT_BIN" --dry-run --Werror "$file"; then
      failed=1
    fi
  done < "$TMP_FILE_LIST"
  return "$failed"
}

run_tidy() {
  failed=0
  while IFS= read -r file; do
    case "$file" in
      *.c)
        if ! "$CLANG_TIDY_BIN" --config-file="$TIDY_CONFIG_FILE" "$file" -- $TIDY_FLAGS; then
          failed=1
        fi
        ;;
    esac
  done < "$TMP_FILE_LIST"
  return "$failed"
}

case "$MODE" in
  format)
    run_format
    ;;
  format-check)
    run_format_check
    ;;
  tidy)
    run_tidy
    ;;
  check)
    failed=0
    run_format_check || failed=1
    run_tidy || failed=1
    exit "$failed"
    ;;
  *)
    echo "Usage: $0 [format|format-check|tidy|check]" >&2
    exit 2
    ;;
esac
