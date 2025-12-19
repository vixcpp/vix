# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

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
