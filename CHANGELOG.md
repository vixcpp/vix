# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

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
