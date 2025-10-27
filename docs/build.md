# Build & Packaging — Vix.cpp

This page explains the different build configurations, debugging tools, and packaging options for **Vix.cpp**.

---

## 🏗️ Build Modes

Vix.cpp uses **CMake ≥ 3.20** and supports multiple build types:

| Mode               | Description                                                    | Flags                               |
| ------------------ | -------------------------------------------------------------- | ----------------------------------- |
| **Debug**          | Includes debug symbols, no optimization. Best for development. | `-DCMAKE_BUILD_TYPE=Debug`          |
| **Release**        | Optimized for performance. Default for production.             | `-DCMAKE_BUILD_TYPE=Release`        |
| **RelWithDebInfo** | Mix of Release + debug symbols.                                | `-DCMAKE_BUILD_TYPE=RelWithDebInfo` |
| **MinSizeRel**     | Optimized for size.                                            | `-DCMAKE_BUILD_TYPE=MinSizeRel`     |

Example:

```bash
cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
```

---

## ⚙️ Sanitizers (ASan / UBSan)

Enable runtime memory and undefined behavior checks:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DVIX_ENABLE_SANITIZERS=ON
cmake --build build-asan -j
```

This automatically adds:

```
-fsanitize=address,undefined -O1 -g -fno-omit-frame-pointer
```

Use in **Debug only** — disable for Release builds.

---

## 🔍 Static Analysis

Vix.cpp integrates with common static analysis tools.

### Clang-Tidy

```bash
cmake -S . -B build -DVIX_ENABLE_CLANG_TIDY=ON
cmake --build build -j
```

### Cppcheck

```bash
cmake -S . -B build -DVIX_ENABLE_CPPCHECK=ON
```

---

## 🧩 Link-Time Optimization (LTO)

For higher performance, enable **LTO** in Release builds:

```bash
cmake -S . -B build-lto -DCMAKE_BUILD_TYPE=Release -DVIX_ENABLE_LTO=ON
cmake --build build-lto -j
```

LTO reduces binary size and improves runtime performance by optimizing across translation units.

---

## 🧪 Code Coverage (Developers)

For measuring test coverage in Debug builds:

```bash
cmake -S . -B build-cov -DCMAKE_BUILD_TYPE=Debug -DVIX_ENABLE_COVERAGE=ON
cmake --build build-cov -j
```

Run tests, then use `gcov`, `lcov`, or `llvm-cov` to generate reports.

---

## 🧱 Packaging / Installation

Build and install all umbrella modules:

```bash
cmake -S . -B build-pkg -DCMAKE_BUILD_TYPE=Release -DVIX_ENABLE_INSTALL=ON
cmake --build build-pkg -j
sudo cmake --install build-pkg --prefix /usr/local
```

Inspect installation layout:

```bash
cmake --install build-pkg --prefix /usr/local --dry-run
```

To uninstall manually, remove installed files or rebuild with a different prefix (e.g., `/opt/vixcpp`).

---

## 🧠 Developer Workflow

- Use **Ninja** for faster incremental builds:
  ```bash
  cmake -G Ninja -S . -B build
  ```
- Regenerate submodules:
  ```bash
  git submodule update --remote --merge
  ```
- Export compile commands for IDEs:
  ```bash
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  ```

---

## 🧩 Example Preset Configurations

You can define presets in `CMakePresets.json`:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "release",
      "generator": "Ninja",
      "binaryDir": "build-rel",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "VIX_ENABLE_LTO": "ON",
        "VIX_ENABLE_INSTALL": "ON"
      }
    },
    {
      "name": "debug-asan",
      "generator": "Ninja",
      "binaryDir": "build-asan",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "VIX_ENABLE_SANITIZERS": "ON"
      }
    }
  ]
}
```

Then run:

```bash
cmake --preset release
cmake --build --preset release
```

---

## 🧰 Troubleshooting Build Issues

| Problem                           | Cause                | Solution                                           |
| --------------------------------- | -------------------- | -------------------------------------------------- |
| `nlohmann/json.hpp not found`     | Missing dependency   | Install with `sudo apt install nlohmann-json3-dev` |
| `undefined reference to spdlog::` | Missing spdlog lib   | Install with `sudo apt install libspdlog-dev`      |
| `fatal error: boost/asio.hpp`     | Boost not installed  | `sudo apt install libboost-all-dev`                |
| `cmake: invalid preset`           | Missing CMake ≥ 3.20 | Upgrade CMake via `pip install cmake --upgrade`    |

---

## ✅ Next Steps

- [Quick Start](./quick-start.md)
- [Installation](./installation.md)
- [Benchmarks](./benchmarks.md)
- [CMake Options](./options.md)
- [Architecture](./architecture.md)
