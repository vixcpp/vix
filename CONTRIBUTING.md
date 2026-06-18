# Contributing to Vix.cpp

Thank you for your interest in contributing to **Vix.cpp**.

Vix.cpp is a modular C++20 runtime. The project is built around clean APIs, high performance, strong tests, reproducible benchmarks, and strict CI.

This guide explains how to contribute code, tests, documentation, examples, benchmarks, and bug reports.

## Project Goals

Vix.cpp aims to provide:

- a modern C++20 runtime,
- modular components that can be used independently,
- fast HTTP and async runtime foundations,
- a strong CLI developer experience,
- reliable tests and CI,
- reproducible benchmark baselines,
- safe and maintainable C++ code.

Contributions should help move the project toward these goals.

## Code of Conduct

All contributors are expected to follow the project Code of Conduct.

Please read:

```txt
CODE_OF_CONDUCT.md
```

Respectful technical discussion is welcome. Personal attacks, harassment, or bad-faith behavior are not.

## Repository Structure

The repository is organized as an umbrella project with multiple modules.

Common directories:

```txt
modules/
  core/
  cli/
  json/
  async/
  threadpool/
  websocket/
  template/
  middleware/
  crypto/
  validation/
  conversion/
  db/
  orm/
  cache/
  kv/
  sync/
  p2p/
  game/
  fs/
  io/
  path/
  env/
  os/
  process/
  log/
  error/
  time/
  utils/

docs/
benchmarks/
.github/workflows/
```

Most modules have their own:

```txt
include/
src/
tests/
examples/
benchmarks/
CMakeLists.txt
```

Not every module has all of these directories.

## Branches

Use a feature branch for your work.

Example:

```bash
git checkout -b fix/threadpool-cancellation
```

Good branch names:

```txt
fix/threadpool-cancellation
test/core-session-errors
docs/json-readme
ci/static-analysis-scope
bench/core-router
```

Avoid working directly on release branches unless you are a maintainer preparing a release.

## Development Requirements

Recommended tools:

- C++20 compiler,
- CMake 3.20 or newer,
- Ninja or Make,
- Git,
- `vix` CLI,
- GoogleTest where required,
- cppcheck,
- clang-tidy,
- Valgrind,
- ASan / UBSan capable compiler.

On Ubuntu, useful packages include:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  git \
  pkg-config \
  cppcheck \
  clang-tidy \
  valgrind
```

Some modules may require optional dependencies such as:

```txt
OpenSSL
SQLite3
ZLIB
fmt
spdlog
SDL2
SDL2_image
OpenGL
MySQL Connector/C++
```

Only install optional dependencies when working on modules that need them.

## Build the Project

From the repository root:

```bash
vix build --build-target all
```

Or with CMake directly:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

For development builds:

```bash
vix build --build-target all -v
```

## Running Tests

Vix.cpp tests are usually stored inside each module, not only in the root `tests/` directory.

The root `tests/` directory may be empty. This is expected.

To enable broad test coverage:

```bash
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

Run tests:

```bash
ctest --test-dir build-ninja --output-on-failure --timeout 180
```

To run a specific test:

```bash
ctest --test-dir build-ninja --output-on-failure -R ThreadPoolTest
```

To run a GoogleTest binary directly:

```bash
./build-ninja/threadpool_build/tests/ExecutorTest
```

With a filter:

```bash
./build-ninja/threadpool_build/tests/ExecutorTest \
  --gtest_filter=ExecutorTest.ThreadPoolExecutorCanPostToThreadPool
```

## Sanitizer Builds

Use sanitizer builds when changing runtime, async, memory, lifecycle, threadpool, or low-level code.

```bash
vix build --clean --build-target all -v -- \
  -DBUILD_TESTING=ON \
  -DVIX_ENABLE_SANITIZERS=ON \
  -DVIX_BUILD_TESTS=ON
```

Run sanitizer tests:

```bash
ctest --test-dir build-san-tests --output-on-failure --timeout 180
```

If you add a new test executable that links against a sanitizer-enabled static library, make sure the test executable also links with sanitizer flags.

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

If the module uses a shared helper target:

```cmake
if (VIX_ENABLE_SANITIZERS AND TARGET vix_sanitizers)
  target_link_libraries(my_test_target PRIVATE vix_sanitizers)
endif()
```

## Valgrind

Use Valgrind for memory-sensitive changes.

```bash
ctest --test-dir build-valgrind --output-on-failure --timeout 300
```

Do not treat benchmark binaries as normal Valgrind test executables.

Valgrind jobs should focus on correctness and memory safety, not performance workloads.

## Static Analysis

The project uses static analysis tools such as:

- cppcheck,
- clang-tidy,
- compiler warnings.

Static analysis should focus on Vix project source code.

Do not run blocking static analysis on:

```txt
third_party/
_deps/
build/
tests/
examples/
benchmarks/
optional disabled backend files
```

For cppcheck, prefer analyzing implementation sources only:

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

For clang-tidy, prefer using the configured CMake compilation database:

```txt
build-analyze/compile_commands.json
```

This prevents analyzing source files that are not part of the current build profile.

---

## Handling Static Analysis False Positives

If a tool reports a false positive, prefer a narrow local suppression instead of weakening the whole workflow.

Example:

```cpp
if (!runs.empty())
{
  // cppcheck-suppress containerOutOfBounds
  return runs.front();
}
```

Only suppress when:

- the code is clearly safe,
- the warning is demonstrably false,
- the suppression is local,
- the suppression ID is specific,
- the reason is easy to understand from the surrounding code.

Do not add broad global suppressions for project code unless absolutely necessary.

## C++ Style

Use modern, readable C++20.

General rules:

- prefer clear names,
- keep functions focused,
- avoid unnecessary global state,
- avoid raw owning pointers,
- prefer RAII,
- avoid undefined behavior,
- avoid data races,
- avoid hidden lifetime assumptions,
- prefer `std::optional`, `std::variant`, and strong result types when useful,
- use `std::string_view` carefully and never return dangling views,
- make error paths explicit,
- keep APIs predictable.

## Formatting

Match the style of the existing file.

General style used across Vix.cpp:

```cpp
if (condition)
{
  do_work();
}
```

Use braces for control flow.

Prefer readable code over clever code.

## Headers

Public headers should live under:

```txt
modules/<module>/include/vix/<module>/
```

Implementation files should live under:

```txt
modules/<module>/src/
```

Tests should live under:

```txt
modules/<module>/tests/
```

Examples should live under:

```txt
modules/<module>/examples/
```

## CMake Rules

When adding a module target:

- provide a namespaced alias such as `vix::<module>`,
- set C++20 compile features,
- install public headers,
- support umbrella builds,
- support standalone builds where applicable,
- export targets correctly,
- keep optional dependencies behind options,
- do not force optional dependencies when a feature is disabled.

Example target naming:

```txt
vix_core
vix::core
```

For tests:

- register with `add_test`,
- link to the correct module target,
- attach sanitizer options when enabled,
- avoid merging plain test files that each define `main()` into one executable.

## GoogleTest and Plain Tests

Some modules use GoogleTest. Some modules use simple executable tests with their own `main()`.

Do not combine multiple plain test files that each define `main()` into one executable.

Bad:

```cmake
add_executable(vix_validation_tests
  validate_parsed_invalid.cpp
  validate_parsed_ok.cpp
  validate_string_smoke.cpp
)
```

Correct:

```cmake
foreach(src ${VIX_VALIDATION_TEST_SOURCES})
  get_filename_component(fname ${src} NAME_WE)
  set(tname "vix_validation_test_${fname}")

  add_executable(${tname} ${src})
  target_link_libraries(${tname} PRIVATE vix::validation)

  add_test(NAME ${tname} COMMAND ${tname})
endforeach()
```

## Threading and Async Code

Threading and async changes require extra care.

When changing threadpool, scheduler, executor, runtime, async TCP, or shutdown code:

- test normal builds,
- test sanitizer builds,
- test cancellation paths,
- test shutdown paths,
- ensure futures are always resolved,
- ensure promises are not left pending,
- avoid assuming async work completes immediately,
- avoid race-prone tests.

Bad test pattern:

```cpp
executor.post([&] {
  counter.fetch_add(1);
});

EXPECT_EQ(counter.load(), 1);
```

Better:

```cpp
executor.post([&] {
  counter.fetch_add(1);
});

ASSERT_TRUE(wait_until_true([&] {
  return counter.load() == 1;
}));
```

## Lifetime and Ownership

Avoid reading objects after ownership has moved or after a container clears owned objects.

Bad:

```cpp
auto system = std::make_unique<CountingSystem>();
auto *raw = system.get();

registry.add_system(std::move(system));
registry.clear();

EXPECT_EQ(raw->stopped_count, 1);
```

After `registry.clear()`, the system object may be destroyed.

Use an external counter:

```cpp
auto stopped_count = std::make_shared<int>(0);
```

## Benchmarks

Benchmarks must be reproducible.

Official benchmark numbers must come from Release builds, not debug or dev builds.

Good:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
```

Bad:

```bash
vix build
```

Development builds are useful for debugging, but they are not official performance numbers.

Benchmark reports should include:

- build type,
- compiler,
- platform,
- CPU,
- benchmark target,
- sample count,
- median time,
- mean time,
- operations per second.

## Documentation

Documentation contributions are welcome.

Good documentation should be:

- clear,
- accurate,
- tested when it contains commands,
- consistent with current APIs,
- free of outdated benchmark claims,
- honest about limitations.

When changing APIs, update the relevant documentation.

Common files:

```txt
README.md
README.ja.md
docs/
SECURITY_TESTS.md
CHANGELOG.md
```

## Commit Messages

Use short, clear commit messages.

Examples:

```txt
fix: resolve futures for queued cancellations
test: wait for threadpool executor task completion
ci: run clang-tidy on configured module sources
docs: update Japanese README
docs: add security test matrix
```

Common prefixes:

```txt
fix:
test:
docs:
ci:
bench:
refactor:
chore:
feat:
```

Avoid vague messages such as:

```txt
update
fix bug
changes
work
```

## Submodule Workflow

Some modules may be tracked as nested repositories or submodules.

When changing a module:

```bash
cd /vixcpp/vix/modules/<module>

git status
git add <files>
git commit -m "type: message"
git push origin dev
```

Then update the umbrella repository:

```bash
cd /vixcpp/vix

git status
git add modules/<module>
git commit -m "chore: update <module> module"
git push origin release/v2.6.3
```

Always commit the module first, then the umbrella pointer.

## Pull Request Checklist

Before opening a pull request, check:

- [ ] The code builds locally.
- [ ] Relevant module tests pass.
- [ ] New behavior has tests.
- [ ] Sanitizer build passes for low-level changes.
- [ ] No benchmark binary is treated as a normal test.
- [ ] Static analysis does not include vendored dependencies.
- [ ] Documentation is updated if APIs changed.
- [ ] Changelog is updated for user-facing changes.
- [ ] Commit messages are clear.
- [ ] The PR description explains what changed and why.

## Bug Reports

A good bug report should include:

- Vix.cpp version or commit,
- operating system,
- compiler version,
- command that failed,
- full error log,
- expected behavior,
- actual behavior,
- minimal reproduction if possible.

Example:

```txt
OS: Ubuntu 24.04
Compiler: GCC 13.3
Command: vix build --build-target all
Module: threadpool
Error: future.get() blocks after queued cancellation
```

## Feature Requests

A good feature request should explain:

- the problem,
- why the feature is useful,
- expected API shape if known,
- examples of usage,
- alternatives considered.

Feature requests should focus on the project goals.

## Performance Reports

Performance reports should include enough context to reproduce the result.

Include:

- build type,
- compiler,
- CPU,
- OS,
- command used,
- benchmark target,
- raw output,
- whether CPU affinity was used,
- whether logging was disabled,
- whether the result is warm or cold.

Avoid comparing debug builds against release builds.

## Release Notes

For user-facing changes, update:

```txt
CHANGELOG.md
```

Use sections such as:

```txt
Added
Changed
Fixed
Removed
Deprecated
Security
```

Keep release notes factual and specific.

## Security Issues

Do not open public issues for sensitive security vulnerabilities.

For security-sensitive reports, contact the maintainer privately:

```txt
adastrablockchain@gmail.com
```

Include:

- affected module,
- reproduction steps,
- impact,
- suggested fix if known.

## License

By contributing to Vix.cpp, you agree that your contributions are provided under the project license.

Vix.cpp is licensed under the MIT License.
