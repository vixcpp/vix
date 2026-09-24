# SECURITY_TESTS.md

# Vix.cpp Security & Stability Test Matrix

This document describes the security, stability, sanitizer, memory-check, and static-analysis test strategy used by Vix.cpp.

The goal of `SECURITY_CI` is not only to check that the project compiles. It also verifies that the Vix modules can be built, tested, analyzed, and packaged under stricter conditions.

---

## Goals

`SECURITY_CI` exists to protect Vix.cpp against:

- broken module test registration,
- missing sanitizer runtime links,
- undefined behavior detected by UBSan,
- memory issues detected by ASan or Valgrind,
- accidental analysis of vendored third-party code,
- unstable benchmark binaries being treated as normal tests,
- optional backend sources being analyzed when their features are disabled,
- CI jobs passing while module-level tests are actually not enabled.

---

## Scope

The security test workflow covers the Vix umbrella repository and its modules.

Covered areas include:

- normal module builds,
- module tests,
- sanitizer builds,
- Valgrind memory checks,
- static analysis,
- package export validation,
- CMake package configuration,
- benchmark build validation.

---

## Module Test Strategy

Vix.cpp modules keep their tests inside their own module directories.

The root `tests/` directory may be empty. This is expected.

Each module is responsible for registering its own tests through its own `tests/CMakeLists.txt` or module-level CMake configuration.

Examples:

```txt
modules/core/tests/
modules/json/tests/
modules/threadpool/tests/
modules/game/tests/
modules/validation/tests/
modules/conversion/tests/
modules/agent/tests/
```

`SECURITY_CI` must therefore enable module-specific test flags explicitly.

Example:

```bash
-DVIX_CORE_BUILD_TESTS=ON
-DVIX_JSON_BUILD_TESTS=ON
-DVIX_THREADPOOL_BUILD_TESTS=ON
-DVIX_GAME_BUILD_TESTS=ON
-DVIX_VALIDATION_BUILD_TESTS=ON
-DVIX_CONVERSION_BUILD_TESTS=ON
-DVIX_AI_AGENT_BUILD_TESTS=ON
```

---

## Full Local Test Build

To build Vix.cpp locally with broad test coverage enabled:

```bash
cd /vixcpp/vix

vix build --clean --build-target all -v -- \
  -DBUILD_TESTING=ON \
  -DVIX_BUILD_TESTS=ON \
  -DVIX_CORE_BUILD_TESTS=ON \
  -DVIX_ASYNC_BUILD_TESTS=ON \
  -DVIX_CACHE_BUILD_TESTS=ON \
  -DVIX_CONVERSION_BUILD_TESTS=ON \
  -DVIX_CRYPTO_BUILD_TESTS=ON \
  -DVIX_DB_BUILD_TESTS=ON \
  -DVIX_ENV_BUILD_TESTS=ON \
  -DVIX_ERROR_BUILD_TESTS=ON \
  -DVIX_FS_BUILD_TESTS=ON \
  -DVIX_GAME_BUILD_TESTS=ON \
  -DVIX_IO_BUILD_TESTS=ON \
  -DVIX_JSON_BUILD_TESTS=ON \
  -DVIX_KV_BUILD_TESTS=ON \
  -DVIX_LOG_BUILD_TESTS=ON \
  -DVIX_MIDDLEWARE_BUILD_TESTS=ON \
  -DVIX_OS_BUILD_TESTS=ON \
  -DVIX_PATH_BUILD_TESTS=ON \
  -DVIX_P2P_BUILD_TESTS=ON \
  -DVIX_PROCESS_BUILD_TESTS=ON \
  -DVIX_SYNC_BUILD_TESTS=ON \
  -DVIX_TEMPLATE_BUILD_TESTS=ON \
  -DVIX_TESTS_BUILD_TESTS=ON \
  -DVIX_THREADPOOL_BUILD_TESTS=ON \
  -DVIX_TIME_BUILD_TESTS=ON \
  -DVIX_VALIDATION_BUILD_TESTS=ON \
  -DVIX_WEBRPC_BUILD_TESTS=ON \
  -DVIX_WEBSOCKET_BUILD_TESTS=ON \
  -DVIX_AI_AGENT_BUILD_TESTS=ON
```

Run tests with:

```bash
ctest --test-dir build-ninja --output-on-failure --timeout 180
```

---

## Sanitizer Builds

Sanitizer builds enable ASan and UBSan where supported.

They are used to catch:

- invalid memory access,
- use-after-free,
- double-free,
- undefined behavior,
- signed integer overflow,
- missing sanitizer runtime linkage in test executables.

Example local sanitizer build:

```bash
cd /vixcpp/vix

vix build --clean --build-target all -v -- \
  -DBUILD_TESTING=ON \
  -DVIX_ENABLE_SANITIZERS=ON \
  -DVIX_BUILD_TESTS=ON \
  -DVIX_CORE_BUILD_TESTS=ON \
  -DVIX_ASYNC_BUILD_TESTS=ON \
  -DVIX_CACHE_BUILD_TESTS=ON \
  -DVIX_CONVERSION_BUILD_TESTS=ON \
  -DVIX_CRYPTO_BUILD_TESTS=ON \
  -DVIX_ENV_BUILD_TESTS=ON \
  -DVIX_ERROR_BUILD_TESTS=ON \
  -DVIX_FS_BUILD_TESTS=ON \
  -DVIX_GAME_BUILD_TESTS=ON \
  -DVIX_IO_BUILD_TESTS=ON \
  -DVIX_JSON_BUILD_TESTS=ON \
  -DVIX_KV_BUILD_TESTS=ON \
  -DVIX_LOG_BUILD_TESTS=ON \
  -DVIX_MIDDLEWARE_BUILD_TESTS=ON \
  -DVIX_OS_BUILD_TESTS=ON \
  -DVIX_PATH_BUILD_TESTS=ON \
  -DVIX_P2P_BUILD_TESTS=ON \
  -DVIX_PROCESS_BUILD_TESTS=ON \
  -DVIX_SYNC_BUILD_TESTS=ON \
  -DVIX_TEMPLATE_BUILD_TESTS=ON \
  -DVIX_TESTS_BUILD_TESTS=ON \
  -DVIX_THREADPOOL_BUILD_TESTS=ON \
  -DVIX_TIME_BUILD_TESTS=ON \
  -DVIX_VALIDATION_BUILD_TESTS=ON \
  -DVIX_WEBRPC_BUILD_TESTS=ON \
  -DVIX_WEBSOCKET_BUILD_TESTS=ON \
  -DVIX_AI_AGENT_BUILD_TESTS=ON
```

Run sanitizer tests with:

```bash
ctest --test-dir build-san-tests \
  --output-on-failure \
  --timeout 180
```

If a known timing-sensitive test is excluded temporarily, document it in the workflow with a comment explaining why.

Example:

```bash
ctest --test-dir build-san-tests \
  --output-on-failure \
  --timeout 180 \
  -E "^ShutdownTest$"
```

---

## Sanitizer Linking Rule

If a static library is compiled with sanitizer flags, every test executable linking against that library must also link with sanitizer flags.

Otherwise, the linker may fail with errors such as:

```txt
undefined reference to `__asan_*`
undefined reference to `__ubsan_*`
```

Each module test target should apply sanitizer compile and link options when `VIX_ENABLE_SANITIZERS=ON`.

Example:

```cmake
if (VIX_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(my_test_target
    PRIVATE
      -fno-omit-frame-pointer
      -fsanitize=address,undefined
  )

  target_link_options(my_test_target
    PRIVATE
      -fsanitize=address,undefined
  )
endif()
```

For modules that use shared helper targets such as `vix_sanitizers`, prefer linking the helper target when available.

Example:

```cmake
if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
  target_link_libraries(my_test_target PRIVATE vix_sanitizers)
endif()
```

---

## GoogleTest vs Plain Tests

Some modules use GoogleTest. Other modules use simple executable tests that define their own `main`.

Do not merge plain test files that each define `main()` into one GoogleTest executable.

Bad pattern:

```cmake
add_executable(vix_validation_tests
  validate_parsed_invalid.cpp
  validate_parsed_ok.cpp
  validate_parsed_smoke.cpp
  validate_string_smoke.cpp
)
```

If each file defines `main()`, this causes multiple-definition errors.

Correct pattern:

```cmake
foreach(src ${VIX_VALIDATION_TEST_SOURCES})
  get_filename_component(fname ${src} NAME_WE)
  set(tname "vix_validation_test_${fname}")

  add_executable(${tname} ${src})
  target_link_libraries(${tname} PRIVATE vix::validation)
  add_test(NAME ${tname} COMMAND ${tname})
endforeach()
```

---

## Valgrind Memory Checks

Valgrind is used to catch memory leaks and invalid memory operations.

Valgrind should run stable test binaries, not benchmark binaries.

Benchmark binaries are not normal correctness tests. They may run for too long or perform workloads that are not suitable for memory-check CI.

Valgrind jobs should avoid:

- benchmark executables,
- long-running stress benchmarks,
- timing-sensitive threadpool shutdown tests,
- tests known to be non-deterministic under heavy instrumentation.

Example:

```bash
ctest --test-dir build-valgrind \
  --output-on-failure \
  --timeout 300 \
  -E "^(ScopeTest|ShutdownTest|TaskCancellationTest|ThreadPoolTest)$"
```

Exclusions must be minimal and documented.

---

## Static Analysis

`SECURITY_CI` uses static analysis to catch possible issues before runtime.

The workflow may include:

- `cppcheck`,
- `clang-tidy`,
- compiler warnings,
- package export checks.

Static analysis should analyze project source files, not vendored third-party code.

Do not run blocking static analysis on:

- `third_party/`,
- `_deps/`,
- build directories,
- generated files,
- examples,
- benchmarks,
- tests,
- optional backend files disabled in the current CMake configuration.

---

## cppcheck Policy

Blocking `cppcheck` should focus on source implementation files.

Recommended scope:

```bash
find modules \
  -type f \
  -path '*/src/*.cpp' \
  ! -path '*/third_party/*' \
  ! -path '*/_deps/*' \
  ! -path '*/build/*' \
  ! -path '*/tests/*' \
  ! -path '*/test/*' \
  ! -path '*/examples/*' \
  ! -path '*/benchmarks/*' \
  ! -path '*/benchmark/*' \
  ! -path '*/backends/sdl/*'
```

Then pass the resulting file list to `cppcheck`.

Example:

```bash
cppcheck \
  --enable=warning,performance,portability \
  --std=c++20 \
  --inconclusive \
  --error-exitcode=2 \
  --suppress=missingIncludeSystem \
  --inline-suppr \
  -j "$(nproc)" \
  "${CPPCHECK_FILES[@]}"
```

Style-only findings may be reported separately and should not block the release unless intentionally promoted.

Example style report:

```bash
cppcheck \
  --enable=style \
  --std=c++20 \
  --inconclusive \
  --suppress=missingIncludeSystem \
  --inline-suppr \
  -j "$(nproc)" \
  "${CPPCHECK_STYLE_FILES[@]}" || true
```

---

## cppcheck False Positives

If `cppcheck` reports a false positive on code that is already guarded, prefer a local inline suppression rather than weakening the whole workflow.

Example:

```cpp
if (!runs.empty())
{
  // cppcheck-suppress containerOutOfBounds
  return runs.front();
}
```

This is accepted only when:

- the code is clearly guarded,
- the warning is demonstrably false,
- the suppression is local and specific,
- `--inline-suppr` is enabled in CI.

Do not add broad global suppressions for project code unless necessary.

---

## clang-tidy Policy

`clang-tidy` should analyze files that are part of the configured CMake build.

The safest source of truth is:

```txt
build-analyze/compile_commands.json
```

This avoids analyzing optional backends that are disabled by CMake.

For example, if the CI config uses:

```bash
-DVIX_GAME_ENABLE_SDL=OFF
-DVIX_GAME_ENABLE_SDL_OPENGL=OFF
```

then SDL backend files should not be analyzed in that job.

Recommended strategy:

- read `build-analyze/compile_commands.json`,
- keep only `.cpp` files under `modules/*/src/`,
- ignore third-party and build-generated paths,
- run `clang-tidy -p build-analyze` on that list.

---

## Optional Backends

Some modules contain optional backend implementations.

Examples:

```txt
modules/game/src/backends/sdl/
modules/db/src/mysql/
modules/websocket/src/SqliteMessageStore.cpp
```

Optional backend files should only be analyzed or built when their corresponding feature is enabled.

Examples:

```bash
-DVIX_GAME_ENABLE_SDL=ON
-DVIX_GAME_ENABLE_SDL_OPENGL=ON
-DVIX_DB_WITH_MYSQL=ON
-DVIX_WITH_SQLITE=ON
```

If a backend is disabled, static analysis should not treat it as part of the active build profile.

---

## Threadpool Stability Notes

Threadpool tests are sensitive to timing under sanitizers and Valgrind.

Tests must not assume that asynchronous work completes immediately after `post()`.

Bad pattern:

```cpp
executor.post([&] {
  counter.fetch_add(1);
});

EXPECT_EQ(counter.load(), 1);
```

Better pattern:

```cpp
executor.post([&] {
  counter.fetch_add(1);
});

ASSERT_TRUE(wait_until_true([&] {
  return counter.load() == 1;
}));
```

For future-returning tasks, cancellation must always resolve the associated future.

A queued task that is cancelled before execution must not leave its `Promise` unresolved.

---

## Game Registry Lifetime Notes

Tests must not read raw pointers to systems after `Registry::clear()`.

Bad pattern:

```cpp
auto system = std::make_unique<CountingSystem>();
auto *raw = system.get();

registry.add_system(std::move(system));
registry.clear();

EXPECT_EQ(raw->stopped_count, 1);
```

After `clear()`, the system object is destroyed.

Use an external counter instead:

```cpp
auto stopped_count = std::make_shared<int>(0);

class ClearCountingSystem final : public vix::game::System
{
public:
  explicit ClearCountingSystem(std::shared_ptr<int> stopped)
      : stopped_(std::move(stopped))
  {
  }

  void on_stop() override
  {
    ++(*stopped_);
    vix::game::System::on_stop();
  }

private:
  std::shared_ptr<int> stopped_;
};
```

---

## Time Module UBSan Notes

Tests must not rely on signed integer overflow.

Bad pattern:

```cpp
int sink = 0;

for (int i = 0; i < 100000; ++i)
{
  sink += i;
}
```

Better pattern:

```cpp
std::int64_t sink = 0;

for (std::int64_t i = 0; i < 100000; ++i)
{
  sink += i;
}
```

---

## Package Export Validation

The workflow should verify that exported CMake packages remain consumable.

This includes checking that generated config files resolve dependencies before loading exported targets.

Important dependency cases include:

- Threads,
- fmt,
- spdlog,
- OpenSSL,
- nlohmann_json,
- SQLite,
- ZLIB,
- MySQL Connector/C++,
- SDL2,
- SDL2_image,
- OpenGL.

Exported targets must not reference unavailable dependency targets.

---

## Recommended Local Debug Commands

Run all tests:

```bash
ctest --test-dir build-ninja --output-on-failure --timeout 180
```

Run sanitizer tests:

```bash
ctest --test-dir build-san-tests --output-on-failure --timeout 180
```

Run one failing test:

```bash
ctest --test-dir build-san-tests \
  --output-on-failure \
  -R ExecutorTest
```

Run one GoogleTest binary directly:

```bash
./build-san-tests/threadpool_build/tests/ExecutorTest
```

Run a specific GoogleTest case:

```bash
./build-san-tests/threadpool_build/tests/ExecutorTest \
  --gtest_filter=ExecutorTest.ThreadPoolExecutorCanPostToThreadPool
```

Run cppcheck source-only analysis locally:

```bash
mapfile -t CPPCHECK_FILES < <(
  find modules \
    -type f \
    -path '*/src/*.cpp' \
    ! -path '*/third_party/*' \
    ! -path '*/_deps/*' \
    ! -path '*/build/*' \
    ! -path '*/tests/*' \
    ! -path '*/test/*' \
    ! -path '*/examples/*' \
    ! -path '*/benchmarks/*' \
    ! -path '*/benchmark/*' \
    ! -path '*/backends/sdl/*' \
    | sort
)

cppcheck \
  --enable=warning,performance,portability \
  --std=c++20 \
  --inconclusive \
  --error-exitcode=2 \
  --suppress=missingIncludeSystem \
  --inline-suppr \
  -j "$(nproc)" \
  "${CPPCHECK_FILES[@]}"
```

---

## CI Design Rule

The security workflow should be strict, but it must stay focused.

The CI should fail for problems in Vix project code.

The CI should not fail because it analyzed:

- vendored third-party tests,
- optional disabled backend files,
- benchmark workloads as normal tests,
- plain test files merged into one binary,
- asynchronous tests that assume synchronous completion.

---

## Current Release Notes

For v2.6.3, `SECURITY_CI` was strengthened to cover broad module integration.

Major improvements include:

- module-level tests enabled in umbrella CI,
- sanitizer-ready test executables,
- fixed sanitizer runtime link errors,
- fixed plain-test CMake registration for validation and conversion modules,
- stabilized threadpool future cancellation behavior,
- stabilized threadpool executor posting tests,
- fixed game registry lifetime test,
- fixed UBSan integer overflow in time smoke test,
- restricted cppcheck to project source files,
- restricted clang-tidy to configured source files,
- excluded vendored third-party and optional disabled backend code from blocking analysis,
- separated style-only static analysis from blocking checks.

---

## Summary

`SECURITY_CI` is designed to answer four questions:

1. Can Vix compile with strict options?
2. Can module tests run when enabled from the umbrella repository?
3. Can sanitizer and memory-check tools run without false build-system failures?
4. Can static analysis focus on Vix source code instead of third-party or disabled code?

If all four answers are yes, the release is much safer to publish.
