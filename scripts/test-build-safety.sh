#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VIX_BIN="${VIX_BIN:-vix}"

PASS=0
FAIL=0

green() { printf "\033[32m%s\033[0m\n" "$1"; }
red() { printf "\033[31m%s\033[0m\n" "$1"; }
cyan() { printf "\033[36m%s\033[0m\n" "$1"; }

run_test() {
  local name="$1"
  shift

  cyan "==> $name"

  if "$@"; then
    green "PASS: $name"
    PASS=$((PASS + 1))
  else
    red "FAIL: $name"
    FAIL=$((FAIL + 1))
  fi

  echo
}

make_project() {
  local dir="$1"

  mkdir -p "$dir/src" "$dir/include"

  cat > "$dir/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.20)
project(vix_build_safety LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app src/main.cpp)
target_include_directories(app PRIVATE include)
target_compile_definitions(app PRIVATE APP_VALUE=42)

add_executable(tool src/tool.cpp)
EOF

  cat > "$dir/include/app.hpp" <<'EOF'
#pragma once

inline int app_value()
{
  return APP_VALUE;
}
EOF

  cat > "$dir/src/main.cpp" <<'EOF'
#include <iostream>
#include "app.hpp"

int main()
{
  std::cout << app_value() << "\n";
  return 0;
}
EOF

  cat > "$dir/src/tool.cpp" <<'EOF'
#include <iostream>

int main()
{
  std::cout << "tool\n";
  return 0;
}
EOF
}

assert_contains() {
  local file="$1"
  local expected="$2"

  grep -Fq "$expected" "$file"
}

assert_not_contains() {
  local file="$1"
  local unexpected="$2"

  ! grep -Fq "$unexpected" "$file"
}

assert_contains_any() {
  local file="$1"
  shift

  local expected
  for expected in "$@"; do
    if grep -Fq "$expected" "$file"; then
      return 0
    fi
  done

  return 1
}

test_build_target_all() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target all -v > out.log 2>&1
    assert_not_contains out.log "Graph target: all"
    assert_contains_any out.log "Compiling all" "Configuring all" "Finished"
  )
}

test_build_real_target() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app -v > out.log 2>&1
    assert_contains_any out.log \
      "Graph target: app" \
      "Compiling app" \
      "Checking app" \
      "Restoring app" \
      "Finished" \
      "Built"
  )
}

test_graph_executor_enabled() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app -v > out.log 2>&1
    assert_contains_any out.log \
      "Graph target: app" \
      "Compiling app" \
      "Checking app" \
      "Restoring app" \
      "Finished" \
      "Built"
  )
}

test_graph_executor_disabled() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    VIX_GRAPH_EXECUTOR=0 "$VIX_BIN" build --build-target app -v > out.log 2>&1
    assert_not_contains out.log "Graph target: app"
    assert_contains_any out.log "Compiling app" "Checking app" "Restoring app" "Finished"
  )
}

test_fast_state_hit() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app > /dev/null 2>&1
    "$VIX_BIN" build --fast --build-target app > out.log 2>&1
    assert_contains_any out.log "Checking app" "Restoring app"
    assert_contains_any out.log "Up to date" "Artifact cache hit"
  )
}

test_fast_fallback() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --fast --build-target app > out.log 2>&1
    assert_not_contains out.log "Checking app"
  )
}

test_missing_last_binary() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app > /dev/null 2>&1

    rm -f build-ninja/app
    rm -f build-ninja/bin/app
    rm -f build-ninja/src/app

    "$VIX_BIN" build --fast --build-target app > out.log 2>&1

    assert_not_contains out.log "Checking app"
  )
}

test_changed_source_file() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app > /dev/null 2>&1

    cat > src/main.cpp <<'EOF'
#include <iostream>
#include "app.hpp"

int main()
{
  std::cout << app_value() + 1 << "\n";
  return 0;
}
EOF

    "$VIX_BIN" build --build-target app > out.log 2>&1
    assert_not_contains out.log "Up to date"
  )
}

test_changed_header_file() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app > /dev/null 2>&1

    cat > include/app.hpp <<'EOF'
#pragma once

inline int app_value()
{
  return APP_VALUE + 10;
}
EOF

    "$VIX_BIN" build --build-target app > out.log 2>&1
    assert_not_contains out.log "Up to date"
  )
}

test_changed_cmakelists() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app > /dev/null 2>&1

    cat >> CMakeLists.txt <<'EOF'

target_compile_definitions(app PRIVATE APP_EXTRA_FLAG=1)
EOF

    "$VIX_BIN" build --fast --build-target app > out.log 2>&1
    assert_not_contains out.log "Checking app"
  )
}

test_changed_compiler_flags() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app -- -DAPP_TEST_FLAG=1 > /dev/null 2>&1
    "$VIX_BIN" build --fast --build-target app -- -DAPP_TEST_FLAG=2 > out.log 2>&1
    assert_not_contains out.log "Checking app"
  )
}

test_changed_build_target() {
  local tmp
  tmp="$(mktemp -d)"
  make_project "$tmp"

  (
    cd "$tmp"
    "$VIX_BIN" build --build-target app > /dev/null 2>&1
    "$VIX_BIN" build --fast --build-target tool > out.log 2>&1
    assert_not_contains out.log "Checking tool"
  )
}

main() {
  command -v "$VIX_BIN" >/dev/null 2>&1 || {
    red "vix binary not found: $VIX_BIN"
    echo "Set VIX_BIN=/path/to/vix and retry."
    exit 1
  }

  run_test "build target all" test_build_target_all
  run_test "build target real" test_build_real_target
  run_test "graph executor enabled" test_graph_executor_enabled
  run_test "graph executor disabled" test_graph_executor_disabled
  run_test "--fast state hit" test_fast_state_hit
  run_test "--fast fallback" test_fast_fallback
  run_test "missing last binary" test_missing_last_binary
  run_test "changed source file" test_changed_source_file
  run_test "changed header file" test_changed_header_file
  run_test "changed CMakeLists.txt" test_changed_cmakelists
  run_test "changed compiler flags" test_changed_compiler_flags
  run_test "changed build target" test_changed_build_target

  echo "Passed: $PASS"
  echo "Failed: $FAIL"

  if [[ "$FAIL" -ne 0 ]]; then
    exit 1
  fi
}

main "$@"
