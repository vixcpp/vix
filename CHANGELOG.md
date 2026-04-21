# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]
# v2.5.1

## Fixes

- core: close HTTP sessions on EOF instead of looping (prevents infinite loops and improves connection handling)
- async: suppress false-positive `-Wnull-dereference` warning around Asio headers using diagnostic push/pop

## Improvements

- websocket: remove unused `set_affinity` placeholder function (code cleanup)

## Notes

This release focuses on stability and code quality:

- Fixes a critical issue in HTTP session lifecycle that could cause infinite loops on EOF
- Cleans up compiler warnings to ensure a stricter and cleaner build
- Removes dead code in the websocket module for better maintainability

# v2.5.0

## Highlights

This release focuses on runtime stability, cleaner shutdown behavior, and better reliability in HTTP + WebSocket applications.

A major part of this work came from debugging real issues inside a large production-style C++ application.
The result is a much stronger Vix runtime, especially around shutdown flow, async cleanup, and WebSocket session writes.

## Fixed

### Core
- Stabilized `App::close()` shutdown flow
- Improved HTTP server stop and join behavior
- Fixed thread shutdown ordering in `HTTPServer`
- Restored multi-threaded HTTP I/O workers after temporary debugging constraints
- Removed temporary debug traces from core shutdown paths

### Async
- Improved scheduler shutdown behavior
- Stabilized async runtime stop flow
- Improved `asio_net_service` lifecycle management
- Added safer stop/join handling for the async network service

### WebSocket
- Fixed complex shutdown issues in mixed HTTP + WebSocket runtimes
- Improved `AttachedRuntime` shutdown coordination
- Stabilized low-level WebSocket server stop and join flow
- Fixed WebSocket session write flush lifecycle issues
- Fixed session lifetime issues during detached async write operations
- Restored missing write path pieces and cleaned up write queue handling
- Removed temporary debug traces from WebSocket runtime code

## Reliability improvements

This release significantly improves behavior in scenarios involving:
- repeated browser refreshes
- multiple WebSocket clients
- server shutdown with active connections
- async cancellation during shutdown
- thread join ordering
- WebSocket write flush safety

## Performance

- Restored multi-threaded HTTP I/O performance after debugging
- Kept the runtime stable while removing temporary bottlenecks introduced during bug isolation

## Internal notes

This version was heavily validated through real debugging sessions on a large C++ codebase with complex concurrency behavior.
Several difficult bugs were tracked down, including shutdown ordering problems, deadlocks, invalid session lifetime handling, and async cleanup issues.

## Upgrade notes

If you use Vix for HTTP + WebSocket applications, this release is recommended.

It contains important fixes for:
- shutdown stability
- session write handling
- async runtime cleanup
- mixed HTTP + WebSocket server lifecycle behavior

## Additional improvements

### Performance
- Added a dedicated `/bench` fast path for benchmark builds
- Improved the HTTP benchmark path in `VIX_BENCH_MODE`
- Benchmarked at ~98k req/s on the `/bench` route in release benchmark mode
- Kept benchmark-specific optimizations isolated from the standard HTTP runtime path

### Build and module reliability
- Fixed public Asio propagation from `vix::async` to dependent modules
- Improved umbrella build consistency for modules using async networking headers
- Cleaned up conditional compilation paths for benchmark-only helpers
- Reduced warning noise in core and platform-specific sources

### Process module
- Fixed POSIX platform warnings
- Improved non-Linux helper isolation in process spawning internals
- Cleaned up `noreturn`, unused parameter, and unused helper issues

# Vix.cpp v2.4.0

A major step forward for the Vix ecosystem.

This release expands the modular architecture, introduces native static file serving in the core runtime, adds a new testing system, improves environment handling, and standardizes the developer experience with umbrella headers and real-world examples.

---

## Core Runtime

- Introduce native `App::static_dir` (no middleware required)
- Static files are now served via router fallback (before 404)
- Fix root mount `/` to correctly serve nested paths (`/index.html`, `/style.css`, etc.)
- Remove legacy static handler bridge and middleware dependency
- Remove deprecated `vix/core.h`

---

## New Modules

The following modules are now fully integrated into the Vix ecosystem:

- `env`
- `error`
- `fs`
- `io`
- `log`
- `os`
- `path`
- `process`
- `tests`

These modules follow the Vix philosophy:

- minimal dependencies
- predictable behavior
- production-ready design
- standalone or umbrella compatible

---

## Umbrella Headers

All modules now expose a single entry point:

- `vix/async.hpp`
- `vix/cache.hpp`
- `vix/conversion.hpp`
- `vix/env.hpp`
- `vix/error.hpp`
- `vix/fs.hpp`
- `vix/io.hpp`
- `vix/json.hpp`
- `vix/log.hpp`
- `vix/middleware.hpp`
- `vix/net.hpp`
- `vix/orm.hpp`
- `vix/os.hpp`
- `vix/p2p.hpp`
- `vix/p2p_http.hpp`
- `vix/path.hpp`
- `vix/process.hpp`
- `vix/sync.hpp`
- `vix/template.hpp`
- `vix/tests.hpp`
- `vix/time.hpp`
- `vix/utils.hpp`
- `vix/validation.hpp`
- `vix/webrpc.hpp`
- `vix/websocket.hpp`

This provides:

- consistent includes
- cleaner public API
- better developer experience

---

## Native Testing Module

A new module `vix::tests` has been introduced.

Features:

- explicit test registration
- no macros
- no hidden magic
- exception-based failure handling
- clean API
- optional CTest integration

### Example

```cpp
auto &registry = TestRegistry::instance();

registry.add(TestCase("basic test", [] {
    Assert::equal(2 + 2, 4);
}));

return TestRunner::run_all_and_exit();
```

---

## Environment System

The environment system has been significantly expanded:

- `.env` file support
- layered configuration
- typed access (`bool`, `string`, etc.)
- production-ready configuration patterns

New examples included for:

- server configuration
- database configuration
- logging configuration
- websocket configuration

---

## Database Integration Improvements

New environment-driven database examples:

- MySQL
- SQLite
- connection pooling
- prepared queries
- transactions

Includes a full HTTP + DB environment showcase.

---

## Static Files

Static file serving is now part of the core runtime.

Examples:

- `01_basic_static.cpp`
- `02_static_directory.cpp`
- `03_static_with_cache.cpp`

Features:

- directory mounting via `app.static_dir`
- automatic `index.html` resolution
- cache control support
- safe path handling

---

## Examples Expansion

A large set of new examples has been added across modules.

#### Config
- server port
- database config
- logging config
- websocket config
- layered env

#### Env
- basic usage
- loading `.env` files
- layered configuration
- production setup

#### Error
- basic errors
- chaining
- exception handling
- result pattern

#### FS
- filesystem operations
- copy / move
- read / write
- directory listing

#### IO
- streams
- buffers
- stdin / stdout
- copy utilities

#### Log
- basic logging
- formatting
- contextual logging

#### OS
- system info
- user info
- sleep utilities

#### ORM
- environment-based HTTP ORM showcase

---

## Architecture

- Improved modular structure
- Consistent module boundaries
- Standardized CMake packaging
- Better separation between core and extensions
- Static serving moved to core runtime

---

## Improvements

- Updated documentation across `core` and `db` modules
- Improved examples consistency
- Cleanup of legacy configuration usage
- `.env.example` introduced as standard
- Removal of outdated documentation

---

## Notes

- `.env` is now treated as a local file and should not be committed
- Configuration is centered around `.env` and examples
- Modules can be used independently or via `vix.hpp`

---

## Upgrade Notes

- Review `.env` usage in your projects
- Update configuration patterns to the new env system
- Migrate tests to `vix::tests` if needed
- Static files no longer require middleware or bootstrap

---

## Summary

- Static serving is now **native** to the runtime
- Middleware is **no longer required** for static files
- New testing system with clean API
- Expanded environment system
- Full umbrella header support
- Stronger modular architecture

---

> Vix.cpp continues to evolve toward a production-ready C++ runtime with a strong focus on clarity, reliability, and real-world usability.
## [v2.3.1] - 2026-04-13

### Fixed
- Improve OpenSSL resolution in `VixConfig.cmake` for consumer projects (macOS / Homebrew)
- Avoid build failures when OpenSSL is installed but not on default CMake search paths

### Notes
- This is a patch release focused on packaging and cross-platform stability
- No changes to `vix run` behavior or runtime features

## [v2.3.0]

### Features

- feat(run): ultra-fast direct C++ execution
  Introduces a new execution mode that compiles and runs single C++ files instantly without CMake.

- feat(run): smart CMake fallback
  Automatically falls back to CMake when the script requires complex dependencies or project structure.

- feat(run,check): add --local-cache for script mode
  Enables local caching for faster repeated executions and unified cache behavior.

- feat(run,build): add SQLite/MySQL flags
  Adds support for database flags directly in run/build workflows.

### Improvements

- improve(run): global cache for script builds
  Script builds are now cached globally and keyed by absolute file path for better reuse.

- improve(run): better executable detection
  Resolves executables from actual build directories (`bin/`, scanned targets) instead of assuming project names.

- improve(run): reorganized execution pipeline
  Cleaner and more robust run flow with improved process handling.

- improve(config): enhance configuration system
  Improved config structure and handling in core module.

### Fixes

- fix(run): detect Vix runtime scripts correctly in fallback mode
  Prevents incorrect fallback behavior when running Vix-based scripts.

- fix(build): improve CMake error detection
  Fixes misleading build errors and improves project root handling.

- fix(utils): clean up configuration template
  Removes redundant content in `vix_utilsConfig.cmake`.

### Examples

- update(examples): improve run examples
  Updated examples for blocking, background, and dynamic port usage.

## [v2.2.0]

### Features

- feat(cli): introduce manifest + resolver + lockfile architecture
  New dependency system with manifest, deterministic resolution, and lockfile support.

- feat(cli): introduce semver resolver
  Full semver support across commands (`add`, `install`, `outdated`, etc.) with correct version selection.

- feat(make): add config generator with interactive prompts
  New `vix make:config` command to generate structured config files interactively.

### Improvements

- improve(resolver): support legacy vix.json dependencies
  Ensures backward compatibility with existing packages while enabling transitive resolution.

- improve(resolver): enable transitive dependency resolution
  Dependencies are now resolved recursively and correctly installed.

### Fixes

- fix(publish): reject duplicate versions
  Prevents publishing the same version multiple times.

- fix(publish): validate vix.json before publish
  Adds validation step to avoid invalid package releases.

- fix(publish): infer version from git tags
  Automatically detects version from git tags for consistency.

- fix(publish): restore correct build order
  Ensures dependencies are built in the correct sequence.

- fix(resolver): correct version resolution edge cases
  Fixes inconsistencies in dependency resolution across commands.

## [v2.1.19]

### Features

- feat(cli): structured CMake build error diagnostics
  Introduces a robust error detection system for CMake builds with clear, actionable diagnostics.

- feat(errors): dedicated detectors for build failures
  Adds specialized detectors for common build issues (linking, missing targets, compilation failures).

### Improvements

- improve(cli): better error readability
  Error messages are now cleaner and easier to understand, removing noisy lines like `ninja: build stopped`.

- improve(build): more reliable clean behavior
  `--clean` now correctly removes local build directories, ensuring a fresh build environment.

- improve(errors): smarter hint system
  Follow-up hints are only displayed when relevant, reducing confusion during debugging.

### Fixes

- fix(errors): detect missing CMake link targets
  Prevents silent failures when targets are not properly linked.

- fix(errors): gate hints on handled errors
  Avoids showing misleading suggestions when errors are already correctly handled.

- fix(build): ensure clean removes build artifacts
  Fixes incomplete cleanup when using `--clean`.

- fix(publish): handle null cwd and improve git detection
  Ensures publishing works reliably even in edge cases with missing working directory or git context.

## [v2.1.18]

### Fixes

- fix(cmake): make SQLite target normalization robust across CMake versions
  Replaced fragile SQLite alias normalization with imported compatibility targets so exported Vix packages work reliably with both `SQLite::SQLite3` and `SQLite3::SQLite3`.

- fix(run): resolve remaining `vix run` failures caused by exported websocket SQLite dependency handling

### Improvements

- improve(config): strengthen package compatibility for consumers using older and newer CMake versions

## [v2.1.17]

### Fixes

- fix(cmake): resolve SQLite target mismatch in exported websocket module
  Fixed an issue where `vix::websocket` exposed `SQLite3::SQLite3` but the target was not guaranteed to exist in consumer projects, causing CMake configuration failures.

- fix(config): ensure SQLite dependency is correctly resolved before loading Vix targets
  Added proper `find_dependency(SQLite3)` handling in `VixConfig.cmake` to prevent missing target errors during `find_package(Vix)`.

### Improvements

- improve(cmake): normalize SQLite target naming across modules and config
  Standardized on `SQLite3::SQLite3` as the canonical target and added compatibility aliases for `SQLite::SQLite3` and `sqlite3`.

- improve(websocket): strengthen SQLite target resolution logic
  Enhanced detection and aliasing to support multiple environments (vcpkg, system packages, manual installs).

- improve(dev experience): eliminate common `vix run` configuration failures
  Running simple programs no longer fails due to missing transitive dependencies.

### Notes

- This release fixes a critical developer experience issue when using `vix run` with modules that depend on SQLite.
- No breaking changes.

## [v2.1.16]

### Fixes

- fix(release): resolve Linux runtime failures due to shared library ABI mismatch
  Linux artifacts were previously built on Ubuntu 22.04, producing binaries linked against `libfmt.so.8` and incompatible with newer distributions (e.g. Ubuntu 24.04).
  Updated release workflows to build on Ubuntu 24.04 to align with modern Linux environments.

- fix(sdk): ensure SDK binaries run correctly on modern Linux systems
  SDK artifacts now use the same updated Linux build environment to avoid runtime failures after installation.

### Improvements

- improve(utils): prefer header-only fmt and spdlog targets
  Updated `vix_utils` to prioritize `fmt::fmt-header-only` and `spdlog::spdlog_header_only`, reducing reliance on system shared libraries and improving portability.

- improve(utils): avoid implicit shared library dependencies in Linux releases
  Added `SPDLOG_FMT_EXTERNAL=1` to ensure consistent integration with fmt and prevent unexpected linkage behavior.

- improve(ci): add runtime validation step for packaged Linux artifacts
  Added a post-build validation step to execute `vix --version` and inspect dependencies via `ldd`, preventing broken binaries from being published.

- improve(ci): unify Linux build environment across release and SDK workflows
  Standardized on Ubuntu 24.04 for consistency and improved compatibility.

### Notes

- Existing installations on older Linux systems may still require compatible runtime libraries.
- Future releases aim to provide fully self-contained Linux binaries.

## [v2.1.15]

### Fixes

- fix(cli): add relative RPATH for SDK binary
  Ensures the `vix` executable can locate its shared libraries after installation by using a relative runtime path (`$ORIGIN/../lib` on Unix, `@loader_path/../lib` on macOS).

- fix(install): handle same-file symlink in SDK mode
  Prevents the installer from attempting to create a symbolic link when the CLI is already installed at the target location.

### Improvements

- improve(install): enhance installer UX (Linux & macOS)
  Introduced a clearer, step-based output with styled messages (steps, success, warnings) for a more professional installation experience.

- improve(install): enhance Windows installer output
  Added structured logs and better feedback during installation, including clearer success and warning messages.

- improve(install): better installation summary
  Displays final location, version, and install kind after completion.

### Internal

- refactor(cli): make SDK runtime self-contained
  CLI now resolves its dependencies relative to its install prefix, improving portability and eliminating the need for manual environment configuration.

- refactor(install): cleaner install flow and logging
  Simplified installer logic and unified messaging across platforms.

## [v2.1.14]

### Fixes

- fix(cache): remove direct nlohmann_json dependency
  Replaced direct linkage to `nlohmann_json::nlohmann_json` with the internal facade `vix::json` to ensure a clean and consistent dependency model across modules.

- fix(build): resolve export error in VixTargets
  Fixed `install(EXPORT VixTargets ...)` failure caused by non-exported external dependencies leaking from the cache module.

### Improvements

- improve(module): enforce unified JSON abstraction
  All modules now rely on `vix::json` as the single JSON entry point, ensuring better modularity and portability.

- improve(build): cleaner umbrella dependency architecture
  Cache module no longer performs local JSON resolution or FetchContent logic. Dependency responsibility is delegated to the json module.

### Internal

- remove(cache): local nlohmann_json detection and fallback logic
  Simplifies the module and avoids duplicated dependency management.

- refactor(cache): align with umbrella build principles
  Ensures strict separation of responsibilities between modules and prevents accidental export of third-party targets.


## [v2.1.13]

### Fixes

- fix(cache): make nlohmann_json integration export-safe
  Removed invalid linkage to FetchContent targets and ensured header-only fallback does not break `install(EXPORT VixTargets)`.

- fix(cache): correct FetchContent usage order
  Fixed incorrect call sequence causing `No content details recorded for nlohmann_json` during configure.

- fix(cache): properly handle header-only fallback includes
  Ensured fallback JSON headers are only exposed at build time and not exported as external targets.

### Improvements

- improve SDK install reliability
  Cache module now installs correctly in both STATIC and HEADER-ONLY modes without missing dependencies.

- improve CMake export consistency
  All modules now follow a consistent pattern for handling external dependencies in export sets.

- improve fallback dependency handling
  JSON fallback is now correctly installed with the SDK when system package is not available.

### Notes

- v2.1.12 stabilized JSON and SQLite integration.
- v2.1.13 completes the fix for cache module and finalizes export-safe SDK packaging across all modules.

## [v2.1.12]

### Fixes

- fix(json): make nlohmann_json integration export-safe
  Removed direct linkage to non-exported targets and ensured header-only fallback works correctly with install/export.

- fix(websocket): normalize SQLite target usage
  Replaced deprecated `SQLite::SQLite3` with `SQLite3::SQLite3` and added compatibility aliases for cross-platform builds.

- fix(build): resolve CMake export errors in SDK builds
  Fixed `install(EXPORT VixTargets)` issues caused by non-exported dependencies.

### Improvements

- improve CMake package reliability
  Ensured all modules (json, websocket, etc.) are safe for `install(EXPORT ...)` and `find_package(Vix)` usage.

- improve SDK installability
  Generated SDK artifacts are now fully usable after installation without missing dependencies.

- improve cross-platform consistency
  Unified dependency handling across Linux, macOS, and Windows for SQLite and JSON backends.

### Notes

- v2.1.11 introduced SDK packaging.
- v2.1.12 stabilizes CMake exports and makes Vix fully installable as a reusable SDK.

## [v2.1.11]

### Features

- feat(sdk): introduce full SDK packaging across platforms
  Added dedicated SDK release workflow producing complete installable artifacts for Linux, macOS, and Windows.

### Fixes

- fix(sdk): correct Windows SDK packaging
  Added proper Windows build, install, and zip packaging using vcpkg and CMake toolchain.

- fix(ci): ensure consistent dependency installation in SDK builds
  Aligned Linux, macOS, and Windows environments to produce identical SDK layouts.

### Improvements

- improve installation experience
  SDK artifacts now contain full installation layout (bin, include, lib) instead of CLI-only binaries.

- improve cross-platform distribution
  SDK releases now available as:
  - `vix-sdk-linux-x86_64.tar.gz`
  - `vix-sdk-macos-aarch64.tar.gz`
  - `vix-sdk-windows-x86_64.zip`

- improve release architecture
  Separation between:
  - CLI releases (`release.yml`)
  - SDK releases (`sdk-release.yml`)

### Notes

- v2.1.10 stabilized builds across platforms.
- v2.1.11 introduces a complete SDK distribution model and prepares Vix for real-world integration workflows.

## [v2.1.10]

### Fixes

- fix(build): resolve Windows build failure due to missing spdlog and fmt
  Added spdlog and fmt to vcpkg manifest to ensure proper dependency resolution.

- fix(utils): ensure fmt is correctly linked across all platforms
  Confirmed fmt is available as a direct dependency for logging utilities.

### Improvements

- improve dependency consistency across platforms
  Unified dependency setup for Linux (apt), macOS (brew), and Windows (vcpkg).

- improve build reliability
  Eliminated platform-specific issues related to missing or inconsistent dependencies.

- refactor(deps): remove unnecessary boost dependencies
  Simplified project dependencies by removing boost-system and boost-beast.

### Notes

- v2.1.9 stabilized CMake and fmt integration but Windows builds were still missing dependencies.
- v2.1.10 completes full cross-platform stability across Linux, macOS, and Windows.

## [v2.1.9]

### Fixes

- fix(utils): resolve fmt header include failure on macOS
  Ensured `<fmt/ostream.h>` is properly resolved by linking fmt as a direct dependency.

- fix(utils): correct dependency propagation for logging module
  Exposed fmt as a PUBLIC dependency to make headers available to all consumers.

- fix(build): remove fragile spdlog fmt detection logic
  Eliminated conditional handling based on `SPDLOG_FMT_EXTERNAL` to simplify and stabilize builds.

### Improvements

- improve cross-platform build reliability
  Unified dependency handling for spdlog and fmt across Linux, macOS, and Windows.

- improve CMake clarity and maintainability
  Simplified dependency configuration with explicit and predictable linking.

### Notes

- v2.1.8 introduced the fmt migration but missed proper linkage in CMake.
- v2.1.9 completes the transition and provides fully stable builds on all platforms.


## [v2.1.8]

### Fixes

- fix(utils): resolve macOS build failure due to spdlog internal header
  Replaced usage of `spdlog/fmt/ostr.h` with `fmt/ostream.h` to ensure compatibility with newer spdlog and fmt versions.

- fix(ci): finalize cross-platform build stability
  Addressed remaining build issues affecting macOS environments.

### Improvements

- improve portability of logging module
  Removed reliance on spdlog internal headers and aligned with official fmt API.

- improve overall build consistency
  Ensured successful builds across all supported platforms (Linux, macOS, Windows).

### Notes

- v2.1.7 fixed CI dependency issues but exposed a compatibility issue with spdlog on macOS.
- v2.1.8 resolves this issue and is fully stable across all targets.

## [v2.1.7]

### Fixes

- fix(ci): resolve macOS build failure
  Added missing spdlog and fmt dependencies to macOS environment to fix CMake configuration error.

- fix(ci): complete cross-platform dependency setup
  Ensured all required system packages are installed across Linux (x86_64, aarch64) and macOS builds.

### Improvements

- improve CI stability across all targets
  Unified dependency installation strategy for consistent builds on Linux, macOS, and Windows.

- improve release reliability
  Eliminated remaining platform-specific issues affecting previous releases.

### Notes

- v2.1.6 fixed Linux builds but macOS builds were still incomplete.
- v2.1.7 is the first fully stable release across all supported platforms.

## [v2.1.6]

### Fixes

- fix(release): restore missing Linux x86_64 dependencies
  Reintroduced required system packages (spdlog, fmt, ssl, zlib, sqlite, brotli) for native Linux builds.

- fix(ci): correct aarch64 cross-compilation dependencies
  Added missing spdlog and fmt packages to the aarch64 environment to resolve CMake configuration failures.

- fix(ci): stabilize cross-compilation toolchain behavior
  Ensured proper package discovery with updated CMake toolchain configuration.

### Improvements

- improve CI consistency across platforms
  Unified dependency setup between Linux x86_64 and aarch64 builds.

- improve release reliability
  Fixed all known issues affecting Linux builds in previous releases.

### Notes

- v2.1.4 and v2.1.5 had CI issues affecting Linux builds.
- v2.1.6 is the first fully stable release across all targets (Linux, macOS, Windows).

## [v2.1.5]

### Fixes

- fix(release): correct Linux aarch64 build pipeline
  Fixed cross-compilation setup by updating CMake toolchain configuration to properly resolve system packages (spdlog, fmt).

- fix(ci): ensure dependency discovery in cross builds
  Updated CMAKE_FIND_ROOT_PATH_MODE_PACKAGE to allow both target and host package resolution.

- fix(ci): install missing dependencies for utils module
  Added spdlog and fmt to aarch64 build environment to prevent configuration failures.

### Improvements

- improve release reliability
  Stabilized CI pipeline across all targets (Linux, macOS, Windows).

- improve cross-compilation consistency
  Better alignment between native and cross builds for dependency resolution.

### Notes

- v2.1.4 was affected by a CI issue on Linux aarch64 and may not produce valid release artifacts.
- v2.1.5 supersedes v2.1.4 with a fully working release pipeline.

## [v2.1.4]

### Improvements

- improve codebase quality (zero warnings)
  Full cleanup across CLI, Core, and modules to ensure a warning-free build with strict compiler flags (`-Wall`, `-Wextra`, `-Wconversion`).

- improve build system consistency
  Standardized CMake configuration across modules with consistent `target_link_libraries` usage.

- improve third-party integration
  Better isolation of external warnings (Asio, spdlog, fmt) using `SYSTEM` includes.

- improve CLI behavior and consistency
  Cleaner command handling, improved REPL flow, and better internal structure.

- improve runtime safety
  Replaced unsafe patterns (`snprintf`, implicit conversions, redundant moves) with modern C++ constructs.

### Fixes

- fix(core): correct handling of `const char[]` vs `char*`
  Prevents template issues and removes warnings in Console.

- fix(session): remove redundant `std::move`
  Ensures cleaner and safer request construction.

- fix(cli): remove unused variables, functions, and parameters
  Eliminates warnings across CLI commands and REPL.

- fix(cli): guard platform-specific helpers
  Windows-only utilities are now properly isolated.

- fix(run): remove unused runtime variables
  Cleans dead code and ensures consistent execution.

- fix(repl): improve argument parsing and validation
  Fixes unused parameters and strengthens parsing logic.

- fix(cmake): resolve linking inconsistencies
  Fixes mixed signature usage and stabilizes builds.

### Documentation

- update CLI command reference
  README now reflects the complete `vix -h` command list.

### Developer Experience

- fully warning-free development environment
- cleaner, safer, and more maintainable codebase
- improved consistency across all modules

### Stability

- improved build reproducibility across environments
- improved reliability across CLI, core, and modules
- no breaking changes

## [v2.1.3]

### Features

- feat(run): clean runtime output
  Removed terminal clearing and spinner for a cleaner and more predictable output when running applications.

- fix(template): guard examples, tests, and benchmarks subdirectories

- feat(cli): task system and project utilities
  Introduces task execution via `vix.json`, along with new utilities for managing projects.

- feat(cli): fmt command
  Adds formatting support to improve code consistency.

- feat(cli): info command
  Displays environment details, cache statistics, and disk usage.

- feat(update): global update support
  Adds `-g` flag to update Vix globally.

### Improvements

- improved runtime readability
  Output is now stable, minimal, and production-friendly.

- improved REPL capabilities
  Better support for JSON access, array indexing, and typed literals.

- improved project scaffolding
  Generated projects now better align with the Vix build workflow.

### Fixes

- fix(run): remove spinner dependency from runtime output
  Ensures consistent output across environments.

- fix(utils): improve runtime log formatting
  Adds proper spacing between labels and values.

- fix(time): exclude benchmarks from library sources
  Prevents duplicate `main` symbol during compilation.

### Documentation

- add REPL documentation
  Introduces `docs/vix_repl.md` for better understanding of runtime interactions.

### Developer Experience

- cleaner CLI output and logs
- improved project generation and workflow consistency
- better visibility into runtime and environment state

### Stability

- improved reliability of runtime execution
- no breaking changes


## [v2.1.2]

### Features

- feat(cli): add shell completion command (bash)
  Generate auto-completion scripts for a smoother CLI experience.

- feat(search): add pagination support
  Introduces `--page` and `--limit` flags for better navigation in search results.

### Improvements

- fix(run): use PTY for live runtime stdout
  Improves real-time output when running applications with `vix run`.

- fix(cli): improve command suggestions
  Suggestions now rely on dispatcher entries for more accurate results.

### Documentation

- add shell completion guide
  New documentation explaining how to enable and use CLI auto-completion.

### Developer Experience

- smoother CLI interaction with better suggestions
- improved readability of runtime output
- enhanced usability for large search results

### Stability

- improved reliability of CLI runtime behavior
- no breaking changes

## [v2.1.1]

### Fixes

- fix(install): support `deps` fallback when loading package manifests
  `vix install` now correctly resolves dependencies declared using both `dependencies` and `deps` fields, ensuring proper topological ordering.

- fix(cmake): stabilize dependency loading order in `.vix/vix_deps.cmake`
  Prevents missing target errors when packages depend on each other (e.g. router → http).

### Improvements

- feat(cmake): registry-safe generated CMake for applications
  Generated `CMakeLists.txt` now:
  - safely includes `.vix/vix_deps.cmake` only when present
  - allows adding registry packages without breaking builds
  - introduces `vix_link_optional_targets(...)` helper for optional dependencies
  - provides clear inline guidance for linking registry libraries

- improve(cmake): better resilience for mixed package formats
  Ensures consistent behavior between `vix install`, `vix build`, and `vix run`.

### Notes

- This release improves reliability when using the Vix registry with CMake projects.
- Recommended to standardize on `"dependencies"` in package manifests, although `deps` remains supported.

---

Small release. Big stability upgrade.

## [v2.1.0]

This release focuses on performance, developer experience, and ecosystem maturity.

Vix is no longer just fast.
It is now structured, documented, and ready to be used in real projects.

---

### Performance

- optimized HTTP hot path in core runtime
- improved scheduler and task execution model
- reduced overhead in run queue processing
- optimized async coroutine fast-path
- removed redundant error handling in async layer

Result:
- lower latency
- better throughput
- more predictable execution under load

---

### Templates

- complete refactor of template examples
- introduction of real-world use cases (dashboard, blog, marketing pages)
- improved layout system (extends, includes, filters)

New examples include:
- shop dashboard
- blog home and post pages
- admin dashboards
- marketing landing pages

---

### Documentation

- massive addition of documentation across the entire ecosystem
- new structured docs for:
  - core modules
  - async
  - cache
  - p2p and sync
  - middleware and HTTP
  - database and ORM
- detailed real-world examples (auth, JWT, caching, rate limit, etc.)

Result:
- significantly improved onboarding
- clearer mental model of Vix architecture
- easier adoption for new developers

---

### Examples

#### Added
- structured template examples (01 → 12 progression)
- new real-world scenarios across modules
- improved consistency across all example categories

#### Removed
- removed outdated and legacy template examples
- cleaned old and unstructured demo files

---

### Core & Modules

- improvements across:
  - core
  - async
  - template
- better internal consistency and structure
- improved maintainability for future features

---

### Developer Experience

- examples now follow a progressive learning path
- documentation aligned with real usage patterns
- clearer separation between basic and advanced concepts

---

### Stability

- improved runtime reliability
- better performance under load
- no breaking changes

---

### Summary

v2.1.0 marks a major step toward making Vix a complete developer platform.

From performance to documentation to real-world examples,
Vix is now designed to be learned, used, and extended.

## [v2.0.0] - 2026-03-31

Vix.cpp 2.0 starts here.

This is the first official V2 release published through the release pipeline.

### Highlights

- major cleanup and reorganization across the Vix ecosystem
- improved examples across HTTP, cache, P2P, sync, templates, database, and ORM
- better developer onboarding with clearer and more progressive examples
- improved Windows/MSVC compatibility across CLI, core, and middleware
- cleaner internal consistency across multiple modules

### Examples

#### Added
- new cache examples
- new P2P examples
- new sync examples
- new template examples
- new database and ORM examples

#### Improved
- updated HTTP examples
- improved middleware demos
- refined security-related examples

#### Removed
- removed outdated and legacy examples
- cleaned older CRUD and deprecated demo layouts

### Stability

- fixed Windows build blockers
- improved cross-platform behavior
- reduced MSVC-specific issues and portability problems

### Notes

This release establishes the real V2 baseline for Vix.cpp.
Future patch and minor releases will build from here.

### Highlights

- massive overhaul of examples across the entire Vix ecosystem
- improved developer experience with clearer, structured, and progressive examples
- continued stabilization of cross-platform support (Windows/MSVC)

---

### Examples

#### Added

- new cache examples (memory, file persistence, LRU, offline handling, headers)
- new p2p examples (envelope, handshake, discovery, router, WAL, UDP, bootstrap)
- new sync examples (offline-first workflows and replication)
- new template examples
- new http cache examples
- new database and ORM examples

#### Updated

- improved HTTP examples (json_api, router params, tracing, etc.)
- updated middleware demos (mega middleware routes)
- improved security examples (JWT, CSRF, headers)

#### Removed

- removed legacy http_crud examples (replaced by cleaner and more modular examples)
- removed outdated cache demos
- removed old P2P demo assets

---

### Core & Modules

- improvements across cache, cli, core, p2p, p2p_http, sync, and websocket modules
- better consistency across modules and examples
- improved internal structure for long-term maintainability

---

### Fixes

- multiple internal fixes across modules
- improved stability of CLI and runtime behavior
- continued Windows/MSVC compatibility improvements

---

### Developer Experience

- examples now follow a more progressive learning path
- clearer separation between basic and advanced use cases
- better alignment between modules and real-world usage

---

### Stability

This release focuses on improving usability, clarity, and ecosystem consistency.

No breaking changes, but many improvements to structure and developer experience.

### Fixes

- fixed Windows build errors across CLI, core, and middleware modules
- restored missing join_for_log in CLI commands (Windows process handling)
- removed invalid OS checks causing compilation issues
- fixed ambiguous std::to_string usage in template module
- improved environment variable handling for cross-platform support

### Improvements

- added fallback for Unicode rendering in print (Windows compatibility)
- cleaned MSVC warnings across multiple modules
- improved formatting stability and safety in format module

### Stability

This release significantly improves cross-platform reliability,
especially on Windows/MSVC environments.

No breaking changes.

### Fixes

- template: fix ambiguous std::to_string call on macOS (libc++)
- template: ensure stable source signature generation using explicit int64_t cast

### Notes

This patch improves cross-platform compatibility of the template module,
specifically fixing a build failure on macOS with libc++.

No breaking changes.

## 2026-03-24

Major release of **Vix.cpp** introducing a new native HTTP stack, full async runtime improvements, and removal of Boost dependencies.

### Core Runtime

- Introduced **native HTTP stack (`vix::http`)**
- Removed dependency on **Boost.Beast**
- Improved **HTTP listener startup**
  - deterministic startup behavior
  - clean accept loop shutdown
- Added **async HTTP listener startup**
- Improved **runtime executor and threadpool shutdown safety**
- Fixed:
  - double shutdown issues in `App::close`
  - unsafe shutdown paths causing aborts
- Improved session handling by silencing normal EOF disconnect logs

### Async Module

- Refactored `tcp_listener`
  - asynchronous `listen()` implementation
  - asynchronous accept loop
- Improved shutdown safety
  - safe termination of `io_context`
  - proper worker thread cleanup
- Transitioned from synchronous startup to a fully asynchronous listener model

### WebSocket

- Migrated to **vix::http**
- Removed all **Boost dependencies**
- Introduced
  - async listener startup
  - improved runtime integration
- Fixed
  - shutdown race conditions
  - dangling callbacks in `AttachedRuntime`
  - connection refused issues during startup
- Improved
  - clean accept shutdown
  - lifecycle stability

### Middleware

- Full migration to **vix::http**
- Removed **Boost.Beast compatibility layer**
- Unified middleware behavior with the new HTTP stack

### CLI and Tooling

- Improved **auto-deps system**
  - support for compiled dependencies
  - correct handling of header-only packages
- Added deterministic dependency installation
- Improved CLI UX
  - cleaner output
  - better dependency detection
- Added internal caching improvements

### Architecture

- Unified runtime around
  - native HTTP layer
  - async execution model
  - modular system
- Cleaner separation between
  - runtime
  - transport
  - application layer

### Breaking Changes

- Removed Boost.Beast and related Boost dependencies
- Old HTTP stack is no longer supported
- Legacy executors were replaced by `RuntimeExecutor`
- Listener startup behavior changed to an async-first model

### Highlights

- Fully Boost-free runtime
- Native high-performance HTTP stack
- Async-first architecture
- Safer shutdown across modules
- Stronger production foundation for the Vix ecosystem

## [v1.47.1]

### CLI

**Fix header-only dependency handling in `vix run`**

- Fixed incorrect linking of header-only dependencies in auto-generated CMake

  The CLI no longer attempts to link all inferred dependency aliases blindly.

- Detect compiled dependencies using `CMakeLists.txt`

  Only dependencies that define build targets are treated as compiled libraries.

- Link only valid CMake targets

  Uses `if(TARGET ...)` to ensure safe and correct linking during build.

- Avoid linking header-only libraries

  Header-only packages without CMake targets are no longer passed to
  `target_link_libraries`, preventing build errors.

- Fixed runtime error
```cmake
target_link_libraries(... gk::pdf) target not found
```
This restores full support for header-only packages while maintaining
correct behavior for compiled dependencies.


## [v1.47.0]

### CLI

**Improved dependency handling for compiled packages**

- Fixed `vix run` dependency handling for compiled libraries

  Dependencies are now correctly integrated into CMake with proper
  `add_subdirectory` ordering, ensuring reliable builds.

- Load dependencies from `vix.lock`

  The CLI now uses the lockfile as the source of truth instead of relying
  on header-only detection, improving determinism and correctness.

- Support for `header-and-source` packages

  Packages that include both headers and source files are now treated
  as compiled libraries and handled correctly during build.

- Improved initialization order for dependencies

  Ensures correct build order for interdependent modules such as
  `core`, `wal`, `store`, and others.

- Enhanced `ScriptCMake` reliability

  Better handling of non header-only dependencies, making builds
  more robust across different package types.

## [v1.46.0]

### CLI

**Improved command UX and dependency detection**

- Standardized help output across commands (`add`, `remove`, `new`, `p2p`)

  All commands now follow a consistent structure with:
  - clear usage
  - practical examples
  - explanation of what happens

- Switched to product-style CLI help

  Help messages are now easier to read and more developer-friendly,
  improving onboarding and daily usage.

- Improved overall CLI readability and developer experience

  - cleaner output
  - more consistent formatting
  - better command interactions

- Fixed dependency install detection

  The `vix deps` command no longer incorrectly reports
  "up to date" on first install.

- Ensured consistent behavior across all CLI commands

## [v1.45.0]

### CLI

**Improved install experience and cache management**

- Simplified and cleaner output for `vix install`
- Reduced noise and improved readability during dependency installation

- New `vix cache` command

  Manage local cache directly from the CLI:

```bash
vix cache
```
This introduces better control over cached packages and improves developer workflows.

- General UX improvements across CLI commands
- More consistent output formatting
- Better feedback during operations
- Smoother command interactions

## [v1.44.0]

### Registry

**Deterministic dependency installation**

- Introduced deterministic dependency resolution for `vix install`

  Dependencies are now installed in a fully predictable and reproducible way,
  ensuring consistent builds across environments.

- Improved registry integration

  - Better handling of dependency graphs
  - More reliable installs across machines and CI environments

- Submodule synchronization improvements

  - Ensures correct versions are resolved and linked during installation

## [1.43.0] - 2026-03-14
### Added
- Add `vix.json` for all modules, enabling registry and package management.
- Introduce universal header-and-source support for core modules.
- New CLI utilities for publishing, syncing, and displaying tips in registry workflows.
- Low-level async runtime improvements (scheduler, coroutines, timers, thread pool).
- Enhanced HTTP caching engine with pluggable stores, policies, and keying strategies.
- Extended crypto primitives: AEAD, HMAC, KDF, signatures, and random utilities.
- DB module improvements: pool, drivers, transactions, migrations.
- WebSocket & P2P enhancements: client/server support, edge sync, discovery, transport layers.
- Middleware & P2P HTTP: routing, session, pipeline, and security modules.
- Validation header-only improvements: schemas, rules, and error handling.

### Changed
- CLI `Ui.hpp` improved tip display formatting.
- Core printing and reflection system refined for runtime debugging.
- JSON module: release v1.10.0 with registry integration.
- Minor API adjustments in ORM, sync, utils, and net modules.
- Bug fixes and header/source consistency across modules.

### Fixed
- Registry publish errors: local registry tips added when `pull --ff-only` fails.
- WebRPC and validation publishing conflicts resolved.
- Various module header/source mismatches corrected.

### Notes
- Version 1.43.0 consolidates registry, package management, and module release workflow.
- All modules now include a `vix.json` for Vix package system integration.
- Developers should run `vix registry sync` before publishing new versions to avoid pull conflicts.

## v1.42.0

### Core

**New universal printing system**

Added a powerful type-aware printing engine inspired by Python's `print()`.

The new `vix::print` module can render most common C++ types automatically:

- STL containers (`vector`, `map`, `set`, etc.)
- tuples and pairs
- `std::optional`
- `std::variant`
- smart pointers
- chrono types
- filesystem paths
- nested structures

Example:

```cpp
#include <vix/print.hpp>

vix::print(42, "hello", std::vector{1,2,3});
```

Additional utilities:

- `vix::pprint()` – multi-line pretty printing
- `vix::sprint()` – format values to string
- `vix::print_each()` – iterate and display ranges
- `vix::print_table()` – map-style table display
- `vix::print_stats()` – numeric container statistics
- `vix::print_diff()` / `vix::print_check()` – debugging helpers

The system is extensible via:

- `operator<<`
- ADL `vix_format(...)`
- `vix::formatter<T>` specialization

### Examples

Added new printing examples:

```
examples/print/
```

## v1.41.0

### CLI

**Improved interactive navigation**

Interactive menus now support both arrow keys and Vim-style navigation.

| Action | Keys |
|------|------|
| Up | `↑` or `k` |
| Down | `↓` or `j` |
| Confirm | `Enter` or `l` |
| Cancel | `Esc`, `h`, `q` |
| Toggle | `Space` |
| Toggle all | `a` |

This improves usability on compact keyboards and Vim-style workflows.

## v1.40.0

### CLI

- feat(cli): support `vix new .` (in-place project scaffold)

  You can now scaffold a Vix project directly in the current directory:

  ```bash
  vix new .
  ```

  If the directory is not empty, Vix prompts before overwriting template files.

- feat(cli): support npm-style scoped packages (`@namespace/name`)

  Vix now supports scoped package names similar to npm:

  ```bash
  vix add @vix/http
  vix add @gaspardkirira/api_app
  ```

  This enables clearer ecosystem organization and registry namespaces.
# [v1.39.0]

## CLI

### Self-healing vix tests

`vix tests` now automatically configures and builds the project when CTest is not ready.

---

### Changes

- Automatically runs `vix check --tests` if the build directory does not exist
- Automatically builds when `CTestTestfile.cmake` is missing
- Re-resolves the preset build directory after auto-build
- Preserves the selected preset to avoid mismatched build directories

---

### Impact

- `vix tests` works on fresh clones without requiring a manual `vix check`
- Removes unnecessary friction in the test workflow
- Improves overall CLI robustness

# [v1.38.0]

Vix now supports automatic version resolution when adding packages.
## You can run:

```bash
vix add namespace/name
```
and Vix will resolve the latest version automatically.
This release also improves transitive dependency installation and ensures vix.lock correctly pins the resolved commit for deterministic builds.

# [v1.37.0] 🚀 New: `--auto-deps` for single-file C++ (script mode)

You can now use libraries installed from the Vix registry directly inside a single `.cpp` file:

```bash
vix run server.cpp --auto-deps
```

- Automatically detects installed dependencies in `.vix/deps`
- Injects the correct `-I` include paths
- Avoids duplicate include flags
- No `CMakeLists.txt` required

---

# 🔼 New: `--auto-deps=up`

```bash
vix run src/server.cpp --auto-deps=up
```

In addition to the script directory, Vix can now search for `.vix/deps` in parent directories.
Useful for nested layouts and running scripts from subfolders.

---

# 🧠 Improvements

- Introduced `AutoDepsMode` (`None`, `Local`, `Up`)
- Proper validation and error handling for invalid `--auto-deps` values
- Improved `vix run --help` documentation
- Cleaner separation between runtime args (`--args`) and compiler flags (`--`)


## [v1.36.3] - 2026-02-19

### Improved

#### CLI – `vix publish`

- Generate a **rich registry entry** when publishing a new package:
  - Adds structured `api` metadata (`format`, `generatedBy`, `updatedAt`)
  - Includes `constraints`, `dependencies`, `exports`, `maintainers`, and `quality`
  - Derives defaults from `vix.json` when available
- Automatically refreshes `api.updatedAt` on every publish
- Keeps backward compatibility with existing registry entries
- Ensures `versions` is always a valid object before insertion

### Why this matters

Registry entries are now:
- More consistent
- Self-describing
- Future-proof for tooling and validation
- Closer to production-grade metadata from first publish

## v1.36.2

### Fix

- **publish**: always include published version (`tag` + `commit`) inside registry entry `versions` field

### Why

Registry validation requires a non-empty `versions` object.

Previously, `vix publish` created a registry entry with:

```json
{
  "versions": {}
}
```

This caused the `validate_registry_pr` workflow to fail before auto-merge.

### Result

- Registry PRs are now immediately valid
- Auto-merge works reliably without manual intervention
- No need to manually patch `index/*.json`
- `registry_index_from_tags` remains compatible

### Impact

Publishing a new library version is now fully automated and stable.
## v1.36.1

### Improvements

- CLI: improve `vix publish` messaging when triggering registry index updates
- Better feedback after tag push and registry workflow trigger
- Clearer output when version is already registered

### Registry

- Confirmed fully automatic tag-driven registry workflow
- Auto PR creation + auto-merge stabilized using bot token

### Internal

- Minor CLI output refinements
- No breaking changes
## v1.36.0

### ✨ Added

- `vix unpublish` command
  Allows package owners to remove an existing registry entry in a controlled way.
- Full CLI integration for unpublish flow (dispatch + command wiring).
- Tag-driven registry compatibility (CLI aligned with automated `index_from_tags` workflow).

### 🔄 Improved

- Registry sync stability (`vix registry sync`) with stricter reset and checkout handling.
- Publish / remove flows aligned with Git tag–based version indexing.
- CLI registry commands hardened for consistency with automated GitHub Actions pipeline.

### 🛠 Internal

- Registry index now derives owner/repo from `repo.url` instead of namespace.
- Improved compatibility with automated registry version generation from Git tags.
- CLI prepared for fully automated registry maintenance workflow.

---

This release strengthens the reliability of the Vix Registry and completes the tag-driven publication lifecycle.
## v1.35.3

### Changed

- **CLI (new)** : Generate modern `vix.json` for header-only libraries.
- Replace deprecated `repo` field with `repository`.
- Set `type` to `"header-only"` for `--lib` template.
- Add `include`, `keywords`, and `authors` fields to generated manifest.

### Notes

This is a patch release improving CLI scaffolding consistency with the Vix Registry specification.
No runtime behavior changes.

## v1.35.2

### Fixed
- CLI (Doctor): Fix MSVC build failure (`which_vix` not declared on Windows).
- Removed duplicate `<vix/utils/Env.hpp>` include.
- Added missing `<cstdio>` include for `_popen` / `popen` usage.
- Minor MSVC compatibility adjustments.

### Notes
This is a patch release focused on Windows/MSVC build stability.
No runtime behavior changes.

## [v1.35.1] - 2026-02-14

### Fixed
- Silenced MSVC warnings in CLI module:
  - C4505 (unreferenced static functions removed)
  - C4189 (unused local variable)
- Scoped Unix-only helpers under `#ifndef _WIN32` to avoid Windows-only compilation warnings.
- Removed deprecated `std::getenv` usage in CLI commands in favor of `vix::utils::vix_getenv` (MSVC C4996 clean).

### Improved
- Cleaner cross-platform compilation (Windows + Linux/macOS).
- More deterministic Windows builds (no internal linkage noise).
- Safer environment variable handling in `doctor`, `upgrade`, and `uninstall`.

### Internal
- Minor CLI refactoring to ensure functions are only compiled where used.
- No behavioral changes.

# v1.35.0

## CLI --- Install Lifecycle & Environment Diagnostics

This release introduces a complete install lifecycle for the Vix CLI,
including environment diagnostics, secure upgrades, uninstall logic, and
install metadata tracking.

## ✨ New

### `vix doctor`

Environment and install health checker.

-   Detects OS and architecture
-   Detects current binary location
-   Reads `install.json` metadata
-   Checks PATH consistency
-   Verifies required tools (curl/wget, tar, sha256, etc.)
-   Optional GitHub latest release check (`--online`)
-   JSON output mode (`--json`)
-   Proper exit codes for CI usage

Example:

``` bash
vix doctor
vix doctor --online
vix doctor --json --online
```

### `vix upgrade`

Secure binary upgrade mechanism.

-   Resolves latest tag from GitHub
-   Verifies sha256
-   Verifies minisign (when available)
-   Atomic install (tmp + move)
-   Writes structured `install.json` metadata
-   Preserves install directory

Example:

``` bash
vix upgrade
```

### `vix uninstall`

Robust uninstall command.

-   Removes current binary
-   Supports `--all` to remove all detected binaries in PATH
-   Supports `--purge` to delete local store/cache
-   Gracefully handles permission errors
-   Provides sudo hint when required
-   Post-check detects remaining binaries in PATH

Example:

``` bash
vix uninstall
vix uninstall --all --purge
```

## 🔐 Install Metadata (`install.json`)

Upgrades now generate structured metadata:

``` json
{
  "repo": "vixcpp/vix",
  "version": "v1.35.0",
  "installed_version": "v1.35.0",
  "installed_at": "...",
  "os": "linux",
  "arch": "x86_64",
  "install_dir": "...",
  "download_bytes": 0,
  "asset_url": "..."
}
```

This enables:

-   Reliable diagnostics (`doctor`)
-   PATH validation
-   Upgrade comparison
-   Future telemetry-safe tooling

## 🎯 Help Output Improvements

The CLI help output is now structured and linked:

-   Section-based layout
-   Quick start block
-   Install lifecycle commands surfaced
-   Documentation links included
-   Registry link added

Improves discoverability and developer experience.

## Stability

This release does not modify runtime modules.\
Changes are isolated to the CLI module.

Vix.cpp continues to move toward a fully self-managed, production-grade
runtime.

v1.22.13: sync cli/middleware/p2p and extend middleware example

- update submodules: cli, middleware, p2p
- tweak umbrella CMakeLists for module sync/build stability
- update examples/mega_middleware_routes.cpp

# Changelog

## v1.22.4

### Fixed
- Fixed release and CI failures caused by missing `spdlog::spdlog_header_only` on clean environments.
- Added a robust FetchContent fallback for header-only spdlog to ensure reproducible builds.
- Stabilized cross-compilation and GitHub Actions release workflows.

### Build
- Utils module now auto-fetches spdlog in header-only mode when not provided by the system.
- Preserved ABI safety by enforcing header-only logging dependencies.

### Notes
- No runtime behavior changes.
- No API breakage.
- This release only improves build reliability and release stability.

## v1.22.3 — 2026-01-27

### Fixed

- **cmake(umbrella):** fix Asio detection and fallback handling
  - Ensure standalone Asio is correctly detected from `third_party/asio`
  - Prevent hard failure when Asio is missing on CI runners
  - Centralize Asio include path via `vix::thirdparty_asio` interface target
  - Stabilize P2P build by fixing Asio integration at umbrella level

## v1.22.2 — 2026-01-27

### Fixed

- **p2p:** fix Asio integration
  - Asio is now strictly provided by the umbrella build
  - Remove implicit `<asio.hpp>` includes and non-deterministic paths
  - Reliable builds in local, CI, and downstream projects

- **core:** fix header guard in `Request.hpp`
  - Prevent collisions and undefined behavior in modular builds

### Build

- Clarify umbrella vs standalone responsibilities
- Deterministic and reproducible CMake configuration

---

## v1.22.1 — 2026-01-25

### Changed

- **core:** unify core modules and clarify responsibilities
- **build:** harmonize CMake and package configuration
- **modules:** sync cache, cli, db, net, p2p, and sync

### Added

- GitHub release workflow
- Security layout structure
- Advanced middleware routing example

### Docs

- Clarify scope and non-goals
- Document Vix Console contract and logging separation

## v1.22.0 — Runtime Console & DX Stabilization

**Release focus:** developer experience, runtime safety, and clear separation between dev console and production logging.

---

## ✨ New

## Vix Console (core)

Introduces a Node.js–inspired, **zero-config runtime console** available directly via:

```cpp
#include <vix/console.hpp>

vix::console.log("Hello");
```

Designed for **developer-facing output** with **strict runtime guarantees**.

---

## 🧠 Design & Runtime Guarantees

## Zero configuration by default

- Console works immediately after `#include`
- No init calls
- No setup required

## Safe defaults

- Default level: `info`
- `debug` **OFF** by default
- `log == info`
- `warn` / `error` → `stderr`
- `log` / `info` / `debug` → `stdout`

## Performance contract

- If a level is filtered, the call is **near-zero cost**
  - No message construction
  - No allocation
  - No lock
  - No I/O

## Thread-safe, atomic output

- Each console call emits a **single atomic line**

## Stable environment configuration

- Supports:
  - `VIX_CONSOLE_LEVEL`
  - `NO_COLOR`
  - `VIX_COLOR`
- Environment is read **once** (startup / first use)
- Behavior is fixed afterward

## Minimal anti-spam protection

- Prevents accidental performance collapse from excessive `log` / `info` usage
- `warn` and `error` are **never suppressed**

---

## 📖 Documentation

## New documentation: Vix Console

- Clearly defines:
  - Scope
  - Guarantees
  - Constraints
  - Non-goals

## Explicit separation

- `vix::console` → runtime / developer output
- `vix::utils::Logger` → production logging, structured output

## Clarified non-goals

- No `console.table`
- No `console.group`
- No `console.time`
- No `console.count`

Advanced debugging and observability belong to `vix::utils::Logger`.

---

## 🔒 Philosophy Clarification

- Vix Console is **intentionally not a production logger**
- Borrows **Node.js developer trust**
- Enforces **C++ runtime discipline**
- Designed to be safe **even if developers do not read documentation**

---

## 🔗 Commits

- `docs(core): document Vix Console contract and integrate core console module`
- `docs(console): clarify scope, non-goals, and logger separation`


## v1.21.1
- chore(cli): bump CLI submodule to v1.19.12 (cleaner sanitizer output, unified timeout logic, refined code frames)
- docs/examples: add `examples/vix_routes_showcase.cpp` (HTTP routes + query params showcase)

# Vix.cpp v1.21.0

This release stabilizes the new DB core module, improves CLI runtime output, and fixes several build/link issues across modules.

## Highlights
- DB/ORM separation is now fully aligned: `vix::db` is the low-level core layer, `vix::orm` remains optional sugar on top.
- CLI runtime output and error UX were refined to be clearer and less noisy.
- Improved reliability for MySQL detection/linking in diverse environments.

## CLI (modules/cli)
- Fix: prevent duplicate runtime logs in some failure paths.
- Improve: runtime error detectors and diagnostics formatting.
- Improve: UX cleanup for run/dev flows (clearer output, less noise).

## DB Core (modules/db)
- Fix: CMake/source/linkage issues across DB drivers.
- Fix: MySQL Connector/C++ discovery via fallback alias target (more robust CI/local setups).
- Improve: driver linkage consistency and feature flag reporting.

## Umbrella / Modules
- Introduced `vix::db` as a core module and decoupled ORM tooling/drivers accordingly.
- Synced submodules after DB/ORM compatibility fixes.

## Upgrade notes
- If you enable `ORM`, it automatically implies `DB`.
- If MySQL is enabled, ensure Connector/C++ is available (the fallback alias helps when CMake configs are missing).

## v1.20.1 — Improved CLI Error UX & Build Feedback

### ✨ CLI — Error reporting & diagnostics
- Introduce **Python-like compiler error rendering** with code frames
  `(file:line:column + surrounding context)`
- Highlight **only the faulty line** with a red caret (`^`) for instant readability
- Safely truncate very long lines with **UTF-8–aware ellipsis**
- Add **precise, Vix-style hints** for common mistakes
  (e.g. `Response::json()` misuse)
- Hide verbose alternative diagnostics unless log level is `debug` or `trace`
- Improve `run` / script error reporting for clearer, actionable diagnostics

### 🔨 CLI — Build output & Ninja integration
- Restore **live Ninja progress output** (`[x/y %]`) during builds
- Filter noisy compiler command output in live mode
- Keep **full raw build logs** in `build.log` for post-mortem diagnostics
- Let `ErrorHandler` display **structured, human-readable errors** on failure
- Introduce **progress-only build output mode**
- Fix unused variables and header warnings in build pipeline

### 🧩 Internal
- Minor internal cleanups to support improved error UX
- No breaking changes


## v1.20.0 — Modules, Registry & Dependency Workflow (Latest)

### ✨ Nouveautés majeures

#### CLI modules overhaul
Introduction d’un **layout app-first** pour les modules Vix :

- Includes explicites et sûrs
- Règles de linkage claires et déterministes
- Préparation à un écosystème modulaire stable et scalable

#### Workflow Registry & Dépendances amélioré

- Nouveau `vix deps` command pour inspecter et gérer les dépendances
- Correction du flux registry pour les dépendances transitives
- Meilleure cohérence entre registry local, store projet et modules

### 🧰 CLI

- Registry plus robuste pour les projets multi-modules
- Amélioration UX des commandes liées aux dépendances
- Base solide pour le futur workflow **registry publish / consume**

### 🧱 Architecture

Clarification nette entre :

- Modules applicatifs
- Modules système
- Dépendances registry

- Réduction des comportements implicites (includes / link automatiques)

---

## v1.19.2

### ✨ Nouveautés

- CLI : `deps` command
- Inspection claire des dépendances projet
- Support du registry dans le graphe de dépendances

### 🐛 Correctifs

- Correction du workflow de résolution des dépendances registry
- Stabilisation du store projet lors des ajouts / suppressions de deps

---

## v1.19.1

### ✨ Nouveautés

- **Registry publish workflow**
- Commande CLI dédiée pour publier vers le registry Vix
- Préparation du pipeline : développeur → registry → consommateurs

### 🔧 Maintenance

- Bump CLI pour intégrer le workflow registry publish
- Nettoyage interne du flux release

---

## v1.19.0

### ✨ Nouveautés majeures

#### Registry Git-based (V1)

- `sync`, `search`, `add`
- Fonctionnement **100 % offline-first** après synchronisation

#### Project-scoped store

- Dépendances isolées par projet
- Nettoyage et GC du store

#### Améliorations UI CLI

- Couleurs unifiées
- Helpers UI partagés
- Sorties plus lisibles et cohérentes

---

## v1.18.2

### 🐛 Correctifs

- Ajustements release et tooling
- Stabilisation du pipeline de versioning

---

## v1.18.1

### 🛠 Correctifs critiques

- Suivi correct des vendored CMake deps
- Correction des erreurs d’installation chez les développeurs
- Bump CLI associé


## v1.18.0 — Modular networking & secure P2P foundation

### Highlights

- Major architectural cleanup and modular extraction
- P2P stack now fully decoupled from core
- Cache, net, and sync promoted to first-class modules
- Clear dependency layering across the ecosystem

---

### 🧩 New Modules

- **net**
  Low-level networking primitives and reachability utilities
  (connectivity probing, network helpers).

- **cache**
  Standalone HTTP cache engine with pluggable stores
  (memory, LRU, file-based).

- **sync**
  Offline-first synchronization engine
  (WAL, outbox, retries, sync workers).

---

### 🧠 Core

- Removed embedded cache and sync implementations
- Core now focuses strictly on:
  - HTTP server
  - routing
  - request / response lifecycle
  - sessions & configuration
- Much cleaner public surface and faster compile times

---

### 🧱 Middleware

- Explicit dependency on cache (no implicit coupling)
- JSON and utils remain optional
- Scoped strictly to HTTP middleware pipeline
- Extensive smoke test coverage restored and stabilized

---

### 🌐 P2P

Integrated **v0.5.1** (latest)

#### P2P v0.5.1 — Latest

- Removed dependency on `vix::core`
- Requires `vix::net` only
- `sync`, `cache`, and `utils` are optional
- Cleaner, modular build
- Internal refactors (discovery, node, router)

#### P2P v0.5.0

- Complete secure transport layer (Phase 6)
- Encrypted envelopes with AEAD
- Per-peer session keys derived from handshake
- Replay protection using nonce counters
- Decrypt-before-dispatch logic
- Handshake & control messages kept plaintext
- Foundation for secure WAL / Outbox sync (Phase 6.6)

#### P2P v0.4.0

- AEAD support (encrypt + authenticate)
- Extended envelope format (nonce + auth tag)
- Session key storage per peer
- Secure channels with nonce tracking
- NullCrypto implementation for development

#### P2P v0.3.0

- HTTP bootstrap service for peer discovery
- Bootstrap.hpp and BootstrapHttp.cpp
- Extended node lifecycle for bootstrap integration
- Manual test registry added

net
├─ cache
├─ sync
└─ p2p
└─ (optional sync integration)

core
└─ middleware
└─ cache (required)

This release establishes a **clean, scalable foundation** for:

- offline-first sync
- secure edge routing
- future P2P + WAL integration
- long-term maintainability

---

### 📐 Architectural State (v1.18.0)

## v1.17.7 — Build & Run Reliability Improvements

### 🐛 Fixed

- **CLI (run):** Reuse the last successful build preset instead of defaulting to
  `dev-ninja`, ensuring consistent behavior between `vix build` and `vix run`.
- **CLI (run):** Cleaned up script build error handling to provide clearer and
  more actionable diagnostics.
- **CLI (run):** Improved logging of environment declarations during script
  execution for better debugging and transparency.

### 🛠 Improvements

- More predictable run flow after builds across presets.
- Smoother iterative workflows (build → run) with fewer surprises.

### ✅ Notes

No breaking changes. This release focuses on **stability and correctness** of the
CLI build/run pipeline.

## v1.17.6 — CLI Stability & Toolchain Fixes

### 🐛 Fixed

- **CLI:** Fixed `--linker lld|mold` integration.

  - Fast linker flags are now applied via `CMAKE_*_LINKER_FLAGS`
    (`EXE`, `SHARED`, `MODULE`) to ensure the selected linker is reliably
    used at link time.
  - Removed incorrect propagation of linker flags through
    `CMAKE_C_FLAGS` / `CMAKE_CXX_FLAGS`, preventing CMake
    “unused variables” warnings and inconsistent behavior.

- **CLI:** Improved `run` / `build` flow integration.
  - Unified preset and build-directory resolution across
    `RunDetail`, `RunFlow`, and `RunCommand`.
  - Fixed edge cases that could lead to missing helper symbols
    or mismatched build state during execution.

### 🛠 Internal

- Cleaner and more predictable linker configuration under CMake.
- Improved robustness of the CLI execution pipeline.
- No breaking changes.

### ✅ Notes

This release focuses on **CLI correctness**, **toolchain reliability**, and
a more stable execution model when using fast linkers and advanced build
configurations.

## v1.17.5 — Performance & CLI Stability Release

### 🚀 Performance

- **core:** remove logger context and console synchronization from HTTP access logs, reducing contention under load
- **utils:** snapshot `spdlog` logger instances to avoid global mutex contention and improve logging throughput

### 🖥️ CLI & Tooling

- **cli(run):** remove capture-based configure phase and switch to live execution for more predictable runtime behavior
- **cli:** clean and stabilize CMake output during configure while keeping build and run steps intact
- **cli:** remove dead helpers left after the build UX rewrite

### 🧹 Maintenance

- Internal cleanup across `cli`, `core`, and `utils` modules
- No breaking changes

### ✅ Notes

This release focuses on **runtime performance**, **log scalability**, and **CLI robustness**, especially under high load and iterative development workflows.

## v1.17.4 — 2026-01-02

### 🔧 Fixed

- **CMake / Packaging**

  - Fixed `find_package(Vix)` failures caused by missing `Boost::filesystem` / `Boost::system` targets.
  - Ensured all required dependencies are resolved **before** loading `VixTargets.cmake`.
  - Removed deprecated `FindBoost` behavior in exported configs (CMP0167-safe).
  - Stabilized consumer builds on macOS (AppleClang + Homebrew Boost) and Linux.

- **Core**

  - Removed `Boost::filesystem` from `vix::core` public link interface.
  - Restricted Boost dependency to `Boost::system` only (Asio / Beast).
  - Updated HTTP server internals and headers accordingly.
  - Prevented Boost symbols from leaking into consumer CMake targets.

- **WebSocket**
  - Cleaned `vix::websocket` exported link interface.
  - Fixed missing `Boost::system` target errors in downstream applications.
  - Improved module behavior with modern CMake dependency resolution.

### ✨ Improved

- More robust umbrella CMake configuration for multi-module installs.
- Clearer separation between internal dependencies and public API surface.
- Better cross-platform developer experience when using:
  ```bash
  find_package(Vix CONFIG REQUIRED)
  ```

## v1.17.3

### Added

- `vix build --target <triple>`: cross-compilation support with auto-generated CMake toolchain.

  - Automatically generates `vix-toolchain.cmake`
  - Passes `CMAKE_TOOLCHAIN_FILE` and `VIX_TARGET_TRIPLE`
  - Cache-safe: reuses build directories when configuration is unchanged
  - Compatible with `--preset`, `--static`, `--clean`

- Optional `--sysroot <path>` support for cross builds
  - Enables proper discovery of target libraries (e.g. zlib)
  - Designed for real cross environments (ARM, embedded, SBCs)

### Improved

- Smarter build directory reuse via configuration signatures
- More explicit error messages when cross toolchains or sysroots are missing

## v1.17.0

### Added

- `vix install` command to install/setup a Vix project or `.vixpkg` artifact.
- Middleware integrated into the default Vix runtime (works out-of-the-box).
- A full set of middleware-focused examples:
  - Auth (API key, JWT, RBAC)
  - CORS (basic/strict)
  - CSRF (strict + demos)
  - Rate limiting (servers + pipeline demos)
  - IP filtering (servers + pipeline demos)
  - Body limit
  - Compression
  - HTTP cache
  - ETag
  - Security headers
  - Static files
  - JSON/form/multipart parsing
  - Group builder examples

### Changed

- Updated CMake configuration and `VixConfig.cmake` to expose the middleware stack correctly for downstream projects.
- Updated README/docs to reflect the new install workflow and middleware usage.

### Notes

- No breaking changes expected.

## v1.16.1 — REPL Isolation & Stability

### ✨ Improvements

- Fully isolated the REPL from CLI commands (`build`, `run`, `check`, `tests`, `verify`)
- Disabled execution of CLI commands and flags inside the REPL
- Removed CLI flags from REPL autocompletion to avoid misleading suggestions
- Disabled `help <command>` and CLI command listing inside the REPL
- Improved REPL UX consistency with a sandboxed, Bun/Deno-like experience

### 🧠 Behavior Changes

- The REPL is now a **pure interactive sandbox** for:
  - math expressions
  - variables
  - JSON literals
  - `print / println`
  - `Vix` API (`cd`, `cwd`, `env`, `args`, `exit`, etc.)
- All project-related CLI commands must now be executed **outside** the REPL

### 🛠 Internal

- Removed remaining runtime dependencies between the REPL and the CLI dispatcher
- Simplified REPL command completion logic

---

## 1.16.0 — 2025-01-XX

### 🚀 Highlights

- **REPL is now the default mode**
  Running `vix` starts the interactive shell automatically
  (no more `vix repl`).

- **Modern runtime experience**
  Behavior aligned with Python, Node.js, and Deno.

---

### ✨ Added

- Default interactive REPL when running `vix`
- Dedicated `README_REPL.md` with:
  - Math expressions
  - Variables
  - JSON usage
  - `print` / `println`
  - Built-in `Vix` API examples
- Improved CLI documentation (`docs/modules/cli.md`)

---

### 🧠 Improved

- REPL argument evaluation:
  - Correct handling of string literals
  - Math expressions with variables
  - Mixed arguments (`println("x =", x+1)`)
- REPL execution flow stability
- Error messages clarity in interactive mode
- Overall CLI UX consistency

---

### 🧹 Changed

- Removed `vix repl` as a required entry point
- REPL is now the primary interaction mode
- CLI documentation updated to reflect new behavior

---

### 🔧 Internal

- REPL flow refactoring and cleanup
- Better separation between CLI dispatcher and REPL runtime
- Documentation structure improvements

---

### ⚠️ Notes

- JSON literals must be **strictly valid JSON**
  (`{"key":"value"}`, not `{key, value}`)

## [1.15.0] - 2025-12-20

### Added

-

### Changed

-

### Removed

-

## v1.15.0 — 2025-12-20

### Added

- **`vix check`**: validate a project or compile a single `.cpp` file without execution.
- **`vix tests`**: run project tests (alias of `vix check --tests`).
- **`vix verify`**: verify packaged artifacts and signatures.
- CLI reference documentation (`docs/vix-cli-help.md`).
- Pull Request template to standardize contributions.

### Changed

- Improved global CLI help output formatting and readability.
- Updated README with clearer CLI usage and script mode examples.
- Updated security testing documentation with realistic, tool-based practices.
- Improved contributing guidelines and code of conduct.

### Removed

- Deprecated `SECURITY.md` in favor of a more accurate security testing guide.

### Notes

- This release significantly improves **CLI usability**, **testing workflow**, and **project governance**.
- Recommended upgrade for all users relying on the Vix CLI.

## [1.14.7] - 2025-12-19

### Added

-

### Changed

-

### Removed

-

## v1.14.7 — CLI Packaging Stability Update

### Fixed

- Fixed `vix pack` potentially blocking when minisign required interactive input.
- Prevented unintended blocking behavior when signing is auto-detected.

### Added

- Integrated new CLI signing modes from vix-cli v1.9.2:
  - `--sign=auto` (default, non-blocking)
  - `--sign=never`
  - `--sign=required` (fail-fast, explicit)

### Improved

- More predictable and professional packaging workflow.
- Clear separation between optional and mandatory cryptographic signing.
- Improved developer experience for CI/CD and non-interactive environments.

## [1.14.6] - 2025-12-19

### Added

-

### Changed

-

### Removed

-

## 1.14.6 — 2025-XX-XX

### CLI

- Updated `modules/cli` to the latest version.
- Improved `vix pack` user experience:
  - Minisign password prompt is now visible when `--verbose` is enabled.
  - Prevents silent blocking during package signing.
- Added `vix verify` command:
  - Manifest v2 validation.
  - Payload digest verification.
  - Optional minisign signature verification.
  - Auto-detection of `dist/<name>@<version>` packages.
- Improved CLI help output:
  - Clearer command grouping.
  - Better descriptions for `pack` and `verify`.
  - More practical usage examples.

### Packaging & Security

- Clear separation between signed and unsigned packages.
- Environment-based key discovery:
  - `VIX_MINISIGN_SECKEY` for package signing.
  - `VIX_MINISIGN_PUBKEY` for signature verification.
- More robust verification feedback and strict modes.

### Developer Experience

- Smoother packaging and verification workflow.
- More predictable CLI behavior with explicit output.
- Improved diagnostics in verbose mode.

---

This release focuses on **polishing the CLI**, strengthening **package security**,
and delivering a **professional packaging & verification workflow** for Vix.

## [1.14.5] - 2025-12-19

### Added

-

### Changed

-

### Removed

-

## v1.14.5 — 2025-01-18

### Added

- New CLI command: `vix pack`.
- Package a Vix project into `dist/<name>@<version>/`.
- Optional `.vixpkg` archive generation.
- Manifest v2 (`vix.manifest.v2`) with:
  - Package metadata (name, version, kind, license).
  - ABI detection (OS, architecture).
  - Toolchain information (C++ compiler, standard, CMake).
  - Layout flags (include, src, lib, modules, README).
  - Exports and dependencies from `vix.toml`.

### Security

- Payload integrity via SHA256 content digest.
- Stable payload hashing excluding generated files.
- Optional Ed25519 signature using `minisign`:
  - `meta/payload.digest`
  - `meta/payload.digest.minisig`
  - Secret key via `VIX_MINISIGN_SECKEY`.

### Changed

- CLI help updated to include `vix pack`.
- Internal CLI and middleware refactoring to support packaging flow.

### Notes

- Signing is optional and enabled only when `minisign` is available.
- Manifest generation avoids self-referential hashing.

## [1.14.4] - 2025-12-17

### Added

-

### Changed

-

### Removed

-

## v1.14.4

### Fixed

- Fixed ORM migrator build after driver abstraction refactor.
- Replaced deprecated MySQL connection APIs with the factory-based `Connection` interface.
- Fixed CI failures caused by abstract `Connection` instantiation.
- Ensured the migrator compiles correctly when database drivers are disabled.

## [1.14.3] - 2025-12-17

### Added

-

### Changed

-

### Removed

-

## v1.14.3 — 2025-12-17

### Fixed

- Fixed JSON umbrella integration: `<vix.hpp>` now reliably exposes `vix::json` builders (`o`, `a`, `kv`).
- Fixed missing `OrderedJson` type in core `RequestHandler`.
- Fixed compilation errors in examples relying on implicit JSON availability.
- Fixed `vix_json` CMake include paths for both build-tree and install-tree usage.

### Internal

- Improved Core ↔ JSON module boundary consistency.
- Stabilized umbrella build (`vix::vix`) dependency propagation.

### Compatibility

- No breaking API changes.
- Existing applications continue to work without modification.

## [1.14.2] - 2025-12-17

### Added

-

### Changed

-

### Removed

-

## v1.14.2 — 2025-12-17

### Fixed

- **core/http**: fixed `RequestHandler` build failures by using the local `OrderedJson` alias and preventing name lookup from resolving to POSIX `::send()` (now uses `this->send()` internally).
- **json**: improved unified include header `vix/json.hpp` and ensured `OrderedJson` exposure is consistent across the JSON module.

## [1.14.1] - 2025-12-17

### Added

-

### Changed

-

### Removed

-

## v1.14.1 — Stability, Cache & Build Hygiene Release

**Release date:** 2025-12-17

### ✨ Highlights

- Finalized **HTTP GET cache** system across core and middleware.
- Integrated cache as a **first-class middleware** in the request pipeline.
- Added comprehensive **cache and middleware tests**.
- Achieved **strict warning-free builds** (`-Weffc++`) across all modules.

---

### 🧱 Core & Middleware

- Completed HTTP cache implementation (Memory, File, LRU stores).
- Introduced cache utilities (`CacheKey`, HTTP header helpers).
- Clean integration with `HTTPServer`, `Router`, and `Session`.
- Added HTTP cache middleware for transparent GET caching.
- Added periodic middleware support for timed/background tasks.
- Middleware pipeline refined and stabilized.

---

### 🛠️ Build & Quality

- Enforced **strict warning hygiene** with `-Weffc++`.
- Fixed:
  - sign-conversion warnings
  - ignored return values (`chdir`, `setenv`, etc.)
  - `[[nodiscard]]` API misuse in examples
- Isolated GCC/libstdc++ false positives (`std::regex`) without masking project warnings.
- Improved and unified **CMake configuration** across all modules.

---

### 🧪 Tests

- Added and stabilized HTTP cache smoke tests.
- Added middleware-level tests validating cache behavior.
- Improved test CMake configuration for consistency.

---

### 📦 Modules Updated

- `core`
- `middleware`
- `cli`
- `websocket`
- `orm`
- `utils`
- `json`

---

### 🔒 Stability & Compatibility

- No breaking API changes.
- Focus on correctness, performance, and long-term maintainability.
- Stable foundation for upcoming **offline-first** and **sync/WAL** features.

## [1.14.0] - 2025-12-14

### Added

-

### Changed

-

### Removed

-

## ORM Migrations & CLI Stabilization

### Added

- New ORM migrations engine with file-based `.up.sql` / `.down.sql` support
- Internal migrator tool (`vix_orm_migrator`) used by the CLI
- `vix orm` command with subcommands:
  - `migrate`
  - `rollback`
  - `status`
- Support for CLI flags:
  - `--db`, `--dir`, `--host`, `--user`, `--pass`, `--steps`
- Environment-based configuration via `VIX_ORM_*`
- Automatic project root and migrations directory detection
- New ORM documentation:
  - Migrations overview
  - Batch insert
  - Repository & QueryBuilder examples
- Application-level ORM workflow (no longer tied to ORM module paths)

### Changed

- ORM examples moved from source tree to documentation
- ORM usage clarified to always target application databases
- CLI help improved with full ORM usage examples
- README updated to reflect new ORM & CLI capabilities

### Fixed

- Logger initialization issues (`static auto& logger`) across:
  - core
  - utils
  - middleware
  - websocket
- CMake linkage inconsistencies between modules
- Incorrect default database usage during migrations (`vixdb` vs app DB)
- Rollback failures caused by mismatched migration directories

### Removed

- Legacy ORM example source files
- Deprecated ORM documentation pages
- Old migrate_init example binaries

### Notes

- Databases must be created manually before running migrations
- Migrations are now strictly application-scoped

## [1.13.3] - 2025-12-12

### Added

-

### Changed

-

### Removed

-

## v1.13.3 — Patch Release

### Fixed

- Fixed SQLite integration in WebSocket module (missing includes, symbols, WAL usage).
- Fixed `SqliteMessageStore` compilation errors with proper `sqlite3.h` usage.
- Split WebSocket router implementation into `.cpp` (no implementation in headers).
- Fixed Long Polling and WebSocket bridge consistency.

### Build / CMake

- ORM examples are now **automatically disabled** when MySQL support is OFF.
- Prevented MySQL-dependent ORM examples from breaking default builds.
- Improved umbrella CMake example filtering logic.
- Safer linking of optional modules (ORM, WebSocket).

### Internal

- Reduced header coupling and improved compile hygiene.
- More robust optional-feature handling (SQLite / MySQL / ORM).

## [1.13.2] - 2025-12-12

### Added

-

### Changed

-

### Removed

-

### WebSocket

- Add SQLite-based message store with WAL support (`SqliteMessageStore`)
- Introduce long-polling fallback for unstable or offline-first environments
- Improve WebSocket client lifecycle (reconnect, heartbeat, write queue)
- Fix header/source separation issues (remove cpp logic from headers)
- Normalize message payload handling using typed JSON `{ type, payload }`
- Improve router and server API consistency
- Internal cleanup and error-handling improvements

### ORM

- Fix CMake integration issues when used from umbrella Vix project
- Ensure ORM targets are correctly exported and linkable by Vix apps
- Minor build-system cleanups

### Internal

- Synchronize `websocket` and `orm` modules with umbrella repository
- Improve build stability and warning cleanup

## [1.13.1] - 2025-12-12

### Added

-

### Changed

-

### Removed

-

## [1.13.0] - 2025-12-12

### Added

-

### Changed

-

### Removed

-

### 🚀 Highlights

- **Stable ORM support**: `vix::orm` is now cleanly exported and linkable from external applications.
- **Simplified application CMake**: apps only link what they use (`vix::vix`, `vix::orm`).
- **Improved developer experience** with `vix new`, `vix build`, and `vix dev`.

---

### ✨ Added

- Exported `vix::orm` target via the main `VixTargets` umbrella.
- Robust MySQL Connector/C++ detection with fallback imported target.
- Cleaner README template generated by `vix new`.
- Explicit ORM opt-in in application CMake (`target_link_libraries(app PRIVATE vix::orm)`).

---

### 🔧 Changed

- ORM no longer forces MySQL dependencies on core-only applications.
- Application CMake templates are now **minimal and predictable**.
- CLI scaffolding aligns with real-world usage (`vix dev`, ORM optional).
- Internal CMake logic cleaned and normalized across modules.

---

### 🐛 Fixed

- Undefined references to `sql::mysql::get_driver_instance()`.
- Broken CMake detection when MySQL is installed without config files.
- Incorrect or obsolete ORM CMake config exports.
- Inconsistent ORM behavior between installed and local builds.

---

### 🧹 Removed

- Obsolete `VixOrmConfig.cmake.in`.
- Redundant MySQL pre-seeding logic in generated applications.

## [1.12.5] - 2025-12-11

### Added

-

### Changed

-

### Removed

-

feat(orm): full ORM integration into Vix build system for v1.12.4

- Connected vix::orm to the top-level CMakeLists for correct package export.
- Updated VixConfig.cmake.in to export ORM targets (vix::orm, drivers, includes).
- Refreshed ORM examples: batch_insert_tx, transactions, migrations, querybuilder, error handling.
- Updated CLI generator (vix new) to support ORM-ready application templates.
- Synced new commits across modules/orm and modules/cli to match the 1.12.4 API.

## [1.12.4] - 2025-12-11

### Added

-

### Changed

-

### Removed

-

v1.12.4: major examples overhaul + core/cli/websocket sync

• Examples refactor: - Removed outdated examples (CRUD, ORM demos, legacy error-handling, trace_route, etc.) - Introduced new structured example layout:
examples/http/
examples/http_crud/
examples/http_ws/
examples/orm/
examples/websocket/ - Added EXAMPLES_README.md and WebSocket example readme - Updated main.cpp with new Request/Response API

• WebSocket module: - Updated advanced + simple examples to new Runtime API - Added chat_room, simple_server, simple_client - Updated examples/websocket CMakeLists

• Core updates: - Improved Request/Response facade - Updated App.cpp, RequestHandler, Config, and umbrella headers - Synced module changes with the new HTTP DX

• CLI updates: - Integrated core/websocket changes for dev/run modes

• Root project: - Updated top-level CMakeLists.txt - Cleaned README for v1.12.4

This version brings a unified example structure and prepares Vix for the new DX-first API.

## [1.12.3] - 2025-12-10

### Added

-

### Changed

-

### Removed

-

## [v1.12.3] – Added Dev Mode for Projects

### Added

- New **`vix dev`** workflow for full applications (not only single-file scripts).
- Automatic **project hot-reload** using `run_project_watch` for CMake-based Vix apps.
- Brand-new **`DevCommand`** (`vix dev`) integrated into the CLI.
- Spinner-based progress indicators for Configure / Build steps.
- Improved detection between script mode and project mode.

### Changed

- `RunCommand` routing now supports watch mode for full project runs.
- Unified dev experience: `vix dev` now forwards to the correct execution flow with consistent UX.
- Cleaner CLI output and improved developer hints/messages.

### Fixed

- Watcher not triggering correctly for some CMake-based projects.
- Missing includes and namespace issues inside the CLI module.
- Several small UX inconsistencies (e.g., duplicate “Ctrl+C” messages).

## [1.12.2] - 2025-12-09

### Added

-

### Changed

-

### Removed

-

chore: prepare v1.12.2 (stdout auto-config, CLI refinements, submodule bumps)

This release improves stdout behavior, developer experience, and aligns
all submodules with the latest work.

🧩 Core

- Added StdoutConfig (automatic stdout line-buffering via VIX_STDOUT_MODE)
- Apps launched via Vix runtime now default to line-buffered stdout
- Ensures consistent server log flushing across all HTTP/WebSocket apps

🧩 CLI

- Improved RunCommand: unified stdout export for presets/fallback/script mode
- More robust command runner with filtered live output + proper ErrorHandler usage
- Updated README and build configuration
- Ensures deterministic behavior for all build/run workflows

🔧 Umbrella (root)

- Updated CMakeLists.txt and README.md
- Bumped submodules:
  • modules/cli → latest dev commit
  • modules/core → latest dev commit
- Synced submodule pointers to match development branches

Version: v1.12.2

## [1.12.1] - 2025-12-08

### Added

- Improved CLI UX for `vix run`: cleaner output flow, removal of unwanted help banner at the end.
- More robust app detection during `vix run` to correctly handle root-level and nested applications.
- Additional CMake configuration flags exported for external consumers (`VIX_WITH_JSON`, `VIX_WITH_OPENSSL`, `VIX_WITH_BOOST_FS`, `VIX_WITH_MYSQL`, `VIX_HAS_ORM`).

### Changed

- Major cleanup and restructuring of `cmake/VixConfig.cmake.in`:
  - `find_dependency(...)` now used instead of leaking unknown imported targets.
  - Exported targets now properly grouped under `vix::vix` for external linking.
  - Installation layout aligned with modern CMake packaging flow.
- Updated root `CMakeLists.txt` to correctly install umbrella exports, avoiding missing-target errors during `find_package(Vix CONFIG)`.
- Synchronized all submodules to the latest development commits (core, utils, cli).

### Fixed

- Fixed application linking errors caused by missing transitive exports in v1.12.0.
- Fixed CLI module not exporting correct symbols for app execution.
- Fixed a regression where Vix runtime installs failed when JSON/backend options changed.
- Fixed configuration issues where external apps (using the generated config package) could not resolve dependencies.

### Removed

- Deprecated internal export macros that are no longer necessary with the new generated config.

## [1.12.0] - 2025-12-08

### Added

-

### Changed

-

### Removed

-

## [1.10.5] - 2025-12-06

### Added

-

### Changed

-

### Removed

-

## [1.10.4] - 2025-12-06

### Added

-

### Changed

-

### Removed

-

## [1.10.2] - 2025-12-06

### Added

-

### Changed

-

### Removed

-

## [0.10.2] - 2025-11-07

### Added

-

### Changed

-

### Removed

-

## [1.10.1] - 2025-11-01

### Added

-

### Changed

-

### Removed

-

## [1.10.0] - 2025-11-01

### Added

-

### Changed

-

### Removed

-

## [1.9.3] - 2025-10-27

### Added

-

### Changed

-

### Removed

-

## [1.9.2] - 2025-10-27

### Added

-

### Changed

-

### Removed

-

## [1.9.1] - 2025-10-27

### Added

-

### Changed

-

### Removed

-

## [1.9.0] - 2025-10-26

### Added

-

### Changed

-

### Removed

-

## [1.8.4] - 2025-10-26

### Added

-

### Changed

-

### Removed

-

## [1.8.3] - 2025-10-26

### Added

-

### Changed

-

### Removed

-

## [1.8.2] - 2025-10-26

### Added

-

### Changed

-

### Removed

-

## [1.8.0] - 2025-10-25

### Added

-

### Changed

-

### Removed

-

## [1.7.1] - 2025-10-23

### Added

-

### Changed

-

### Removed

-

## [1.7.0] - 2025-10-13

### Added

-

### Changed

-

### Removed

-

## [1.6.0] - 2025-10-12

### Added

-

### Changed

-

### Removed

-

## [1.5.1] - 2025-10-11

### Added

-

### Changed

-

### Removed

-

## [1.5.0] - 2025-10-10

### Added

-

### Changed

-

### Removed

-

## [1.4.0] - 2025-10-10

### Added

-

### Changed

-

### Removed

-

## [1.3.3] - 2025-10-09

### Added

-

### Changed

-

### Removed

-

## [1.3.2] - 2025-10-09

### Added

-

### Changed

-

### Removed

-

## [1.3.1] - 2025-10-09

### Added

-

### Changed

-

### Removed

-

## [1.3.0] - 2025-10-09

### Added

-

### Changed

-

### Removed

-

## [1.2.0] - 2025-10-08

### Added

-

### Changed

-

### Removed

-

## [1.1.1] - 2025-10-07

### Added

-

### Changed

-

### Removed

-

## [1.1.0] - 2025-10-07

### Added

-

### Changed

-

### Removed

-

## [1.0.2] - 2025-10-06

### Added

-

### Changed

-

### Removed

-

## [1.0.1] - 2025-10-06

### Added

-

### Changed

-

### Removed

-

## [1.0.0] - 2025-10-06

### Added

-

### Changed

-

### Removed

-

## [0.1.2] - 2025-10-06

### Added

-

### Changed

-

### Removed

-

## [0.1.1] - 2025-10-06

### Added

-

### Changed

-

### Removed

-

### Added

- Modular C++ framework structure with `core`, `orm`, `cli`, `docs`, `middleware`, `websocket`, `devtools`, `examples`.
- `App` class for simplified HTTP server setup.
- Router system supporting dynamic route parameters (`/users/{id}` style).
- JSON response wrapper using `nlohmann::json`.
- Middleware system for request handling.
- Example endpoints `/hello`, `/ping`, and `/users/{id}`.
- Thread-safe signal handling for graceful shutdown.
- Basic configuration system (`Config` class) to manage JSON config files.

### Changed

- Logger integrated using `spdlog` with configurable log levels.
- Improved request parameter extraction for performance.

### Fixed

- Path parameter extraction to correctly handle `string_view` types.
- Fixed default response for unmatched routes (`404` JSON message).

---

## [0.1.0] - 2025-10-06

### Added

- Initial release of core module with working HTTP server.
- Basic routing system and request handlers.
- Simple example endpoints demonstrating JSON and text responses.
- Thread-safe server shutdown handling.
- Integration of performance measurement scripts (FlameGraph ready).

### Changed

- Optimized route parameter parsing to avoid `boost::regex` overhead.

### Fixed

- Compilation errors due to `string_view` mismatch in request handler.
- Minor bug fixes in App initialization and signal handling.

---

## [0.0.1] - Draft

- Project skeleton created.
- Basic CMake setup and folder structure.
- Placeholder modules for `core`, `orm`, and `examples`.
