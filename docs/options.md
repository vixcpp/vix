# CMake Options — Vix.cpp

This page aggregates the common **build flags** and **options** you’ll use across modules.

---

## Core Options

| Option                          | Default | Description                                          |
| ------------------------------- | :-----: | ---------------------------------------------------- |
| `VIX_BUILD_EXAMPLES`            |  `ON`   | Build example programs under `/examples`.            |
| `VIX_BUILD_TESTS`               |  `ON`   | Build & run unit tests (GoogleTest if available).    |
| `VIX_ENABLE_SANITIZERS`         |  `OFF`  | Enable Address/Undefined sanitizers (Debug configs). |
| `VIX_ENABLE_LTO`                |  `OFF`  | Link Time Optimization (applies to Release).         |
| `CMAKE_EXPORT_COMPILE_COMMANDS` |  `ON`   | Export `compile_commands.json` for IDEs.             |
| `VIX_ENABLE_CLANG_TIDY`         |  `OFF`  | Run `clang-tidy` during builds (if available).       |
| `VIX_ENABLE_CPPCHECK`           |  `OFF`  | Run `cppcheck` during builds (if available).         |
| `VIX_ENABLE_COVERAGE`           |  `OFF`  | Enable coverage instrumentation in Debug builds.     |

### Example

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release -DVIX_ENABLE_LTO=ON
```

---

## ORM Options

| Option                   | Default | Description                                            |
| ------------------------ | :-----: | ------------------------------------------------------ |
| `VIX_ORM_USE_MYSQL`      |  `ON`   | Enable MySQL driver (mysql-connector-cpp).             |
| `VIX_ORM_USE_SQLITE`     |  `OFF`  | Enable SQLite3 backend.                                |
| `VIX_ORM_BUILD_EXAMPLES` |  `OFF`  | Build ORM example binaries.                            |
| `VIX_ORM_BUILD_TESTS`    |  `OFF`  | Build ORM test targets.                                |
| `VIX_ORM_REQUIRE_MYSQL`  |  `OFF`  | Fail configuration if MySQL was requested but missing. |

### Example

```bash
cmake -S modules/orm -B build -DVIX_ORM_USE_MYSQL=ON -DVIX_ORM_BUILD_EXAMPLES=ON
```

---

## Toolchain & Generator Tips

- Prefer **Ninja** for faster incremental builds:
  ```bash
  cmake -G Ninja -S . -B build
  cmake --build build -j
  ```
- Export compile commands for IDEs (VS Code/clangd):
  ```bash
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  ```
- Recommended Release flags (configure in your toolchain file/preset):
  ```text
  -O3 -DNDEBUG -march=native -flto
  ```

---

## Debugging & Analysis

- Enable sanitizers:
  ```bash
  cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DVIX_ENABLE_SANITIZERS=ON
  cmake --build build-asan -j
  ```
- Run static analysis:
  ```bash
  -DVIX_ENABLE_CLANG_TIDY=ON  -DVIX_ENABLE_CPPCHECK=ON
  ```

---

## Packaging & Install

```bash
cmake -S . -B build-pkg -DCMAKE_BUILD_TYPE=Release -DVIX_ENABLE_INSTALL=ON
cmake --build build-pkg -j
sudo cmake --install build-pkg --prefix /usr/local
```

Use `--dry-run` to inspect installed files:

```bash
cmake --install build-pkg --prefix /usr/local --dry-run
```
