# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)  
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

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
