#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VIX_BIN="${VIX_BIN:-vix}"
TARGET="${TARGET:-vix}"
RUNS="${RUNS:-20}"
WARMUP="${WARMUP:-5}"

BENCH_DIR="${ROOT_DIR}/.vix/bench"
REPORT_DIR="${BENCH_DIR}/reports"
REPORT_FILE="${REPORT_DIR}/build-benchmark.md"

mkdir -p "${REPORT_DIR}"

require_command() {
  local cmd="$1"

  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "error: required command not found: $cmd" >&2
    exit 1
  fi
}

count_files() {
  local pattern="$1"

  find "${ROOT_DIR}" \
    -path "${ROOT_DIR}/.git" -prune -o \
    -path "${ROOT_DIR}/build*" -prune -o \
    -path "${ROOT_DIR}/.vix" -prune -o \
    -type f \
    \( ${pattern} \) \
    -print | wc -l | tr -d ' '
}

detect_cpu() {
  if command -v lscpu >/dev/null 2>&1; then
    lscpu | awk -F: '/Model name/ { gsub(/^[ \t]+/, "", $2); print $2; exit }'
  else
    uname -m
  fi
}

detect_ram() {
  if command -v free >/dev/null 2>&1; then
    free -h | awk '/Mem:/ { print $2; exit }'
  else
    echo "unknown"
  fi
}

detect_os() {
  if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    echo "${PRETTY_NAME:-unknown}"
  else
    uname -a
  fi
}

detect_compiler() {
  if command -v c++ >/dev/null 2>&1; then
    c++ --version | head -n 1
  else
    echo "unknown"
  fi
}

detect_linker() {
  if command -v mold >/dev/null 2>&1; then
    echo "mold"
    return
  fi

  if command -v ld.lld >/dev/null 2>&1; then
    echo "lld"
    return
  fi

  if command -v ld >/dev/null 2>&1; then
    ld --version | head -n 1
    return
  fi

  echo "unknown"
}

detect_launcher() {
  if command -v sccache >/dev/null 2>&1; then
    echo "sccache"
    return
  fi

  if command -v ccache >/dev/null 2>&1; then
    echo "ccache"
    return
  fi

  echo "none"
}

detect_vix_version() {
  "$VIX_BIN" --version 2>/dev/null || "$VIX_BIN" version 2>/dev/null || echo "unknown"
}

count_compile_tasks() {
  local compile_commands="${ROOT_DIR}/build-ninja/compile_commands.json"

  if [[ ! -f "$compile_commands" ]]; then
    echo "0"
    return
  fi

  python3 - <<PY
import json
from pathlib import Path

path = Path("${compile_commands}")
try:
    data = json.loads(path.read_text())
    print(len(data) if isinstance(data, list) else 0)
except Exception:
    print(0)
PY
}

write_metadata() {
  local source_count="$1"
  local header_count="$2"
  local compile_task_count="$3"

  {
    echo "# Vix Build Benchmark"
    echo
    echo "## Environment"
    echo
    echo "| Field | Value |"
    echo "| --- | --- |"
    echo "| CPU | $(detect_cpu) |"
    echo "| RAM | $(detect_ram) |"
    echo "| OS | $(detect_os) |"
    echo "| Compiler | $(detect_compiler) |"
    echo "| Linker | $(detect_linker) |"
    echo "| Launcher | $(detect_launcher) |"
    echo "| Vix version | $(detect_vix_version) |"
    echo "| Target | ${TARGET} |"
    echo "| Runs | ${RUNS} |"
    echo "| Warmup | ${WARMUP} |"
    echo
    echo "## Project"
    echo
    echo "| Field | Value |"
    echo "| --- | --- |"
    echo "| Source files | ${source_count} |"
    echo "| Header files | ${header_count} |"
    echo "| Compile tasks | ${compile_task_count} |"
    echo "| Object cache | \$HOME/.vix/cache/objects |"
    echo "| Artifact cache | \$HOME/.vix/cache/build |"
    echo
    echo "## Benchmark"
    echo
  } > "$REPORT_FILE"
}

run_hyperfine() {
  hyperfine \
    --warmup "$WARMUP" \
    --runs "$RUNS" \
    --export-markdown "${REPORT_DIR}/hyperfine.md" \
    "${VIX_BIN} build --fast --build-target ${TARGET}" \
    "${VIX_BIN} build --build-target ${TARGET}" \
    "VIX_GRAPH_EXECUTOR=0 ${VIX_BIN} build --build-target ${TARGET}"

  cat "${REPORT_DIR}/hyperfine.md" >> "$REPORT_FILE"
}

run_scenario() {
  local name="$1"
  shift

  echo "## ${name}" >> "$REPORT_FILE"
  echo >> "$REPORT_FILE"

  {
    echo '```txt'
    "$@"
    echo '```'
  } >> "$REPORT_FILE" 2>&1

  echo >> "$REPORT_FILE"
}

main() {
  require_command "$VIX_BIN"
  require_command hyperfine
  require_command python3

  cd "$ROOT_DIR"

  echo "Preparing baseline build..."
  "$VIX_BIN" build --build-target "$TARGET" >/dev/null

  local source_count
  local header_count
  local compile_task_count

  source_count="$(count_files '-name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.c"')"
  header_count="$(count_files '-name "*.hpp" -o -name "*.hh" -o -name "*.hxx" -o -name "*.h" -o -name "*.ipp"')"
  compile_task_count="$(count_compile_tasks)"

  write_metadata "$source_count" "$header_count" "$compile_task_count"

  echo "Running standard benchmark..."
  run_hyperfine

  echo "Running official scenarios..."

  run_scenario "no-op build" \
    "$VIX_BIN" build --build-target "$TARGET"

  run_scenario "fast path hit" \
    "$VIX_BIN" build --fast --build-target "$TARGET"

  run_scenario "target build vs all build: target" \
    "$VIX_BIN" build --build-target "$TARGET"

  run_scenario "target build vs all build: all" \
    "$VIX_BIN" build --build-target all

  echo "Report written to:"
  echo "  ${REPORT_FILE}"
}

main "$@"
