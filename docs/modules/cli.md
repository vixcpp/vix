# 🧩 Vix.cpp — CLI Module

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![MIT](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Module-Stable-success)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)

> The **Vix CLI** (`vix`) is the official command-line interface for  
> **Vix.cpp — the offline-first, ultra-fast C++ runtime**.
>
> It provides a clean developer experience similar to _Cargo_, _Deno_, or _Vue CLI_,  
> but natively powered by **C++20** and **fast incremental CMake builds**.

---

# 🚀 Overview

The CLI is designed to offer a **zero-friction workflow**:

- create a new project in one command
- build it instantly
- run it with auto-compile and execution
- support direct `.cpp` execution (script mode)

Whether you're building a microservice, a full backend, or experimenting locally,  
the Vix CLI is the fastest way to work with Vix.cpp.

---

# ⚙️ Features

### 🏗 Project Generation

`vix new <name>` scaffolds a new Vix application with:

- CMakeLists.txt
- src/main.cpp
- README.md
- .gitignore

### ⚡ Smart Build System

`vix build` automatically:

- detects CMake presets
- performs incremental builds
- handles compile_commands.json
- avoids unnecessary rebuilds

### 🚀 Automatic Run

`vix run`:

- builds if needed
- selects the correct executable
- passes your runtime arguments
- handles script mode (.cpp execution)

### 🔧 Unified Logging

Supports:

```
--log-level trace|debug|info|warn|error|critical
--quiet
--verbose
```

---

# 🧰 Commands

```
vix [GLOBAL OPTIONS] <COMMAND> [ARGS...]
```

## Project Commands

| Command      | Description                |
| ------------ | -------------------------- |
| `new <name>` | Scaffold a new Vix project |

## Development Commands

| Command               | Description                                |
| --------------------- | ------------------------------------------ |
| `build [name]`        | Configure and build a project              |
| `run [name] [--args]` | Build if needed, then run                  |
| `run file.cpp`        | Script mode: compile & run a single `.cpp` |

## Info Commands

| Command   | Description       |
| --------- | ----------------- |
| `help`    | Show help         |
| `version` | Show version info |

---

# 🌍 Global Options

| Option                | Description          |
| --------------------- | -------------------- |
| `--log-level <level>` | Set log level        |
| `--verbose`           | Debug-level logs     |
| `--quiet`             | Only warnings/errors |
| `-h`, `--help`        | Show CLI help        |
| `-v`, `--version`     | Show CLI version     |

---

# 🌱 Environment Variables

| Variable        | Description       |
| --------------- | ----------------- |
| `VIX_LOG_LEVEL` | Default log level |

---

# 🧪 Examples

### Create

```bash
vix new api
```

### Build

```bash
vix build
```

### Run

```bash
vix run -- --port 8080
```

### Script mode

```bash
vix run server.cpp
```

---

# 🧩 Architecture

### Internal Components

- **CLI** — dispatches commands
- **RunFlow** — orchestrates build → run
- **RunScript** — compiles .cpp files in temporary environments

### Directory Structure

```csharp
modules/cli/
  include/vix/cli/CLI.hpp
  src/CLI.cpp
  src/commands/
  src/commands/run/
  CMakeLists.txt
```

---

# 🧾 Actual Help Output

```csharp
Vix.cpp — Modern C++ backend runtime
Version: v1.5.1

Usage:
  vix [GLOBAL OPTIONS] <COMMAND> [ARGS...]

Commands:
  Project:
    new <name>             Scaffold a new Vix project in ./<name>

  Development:
    build [name]           Configure and build a project (root or app)
    run [name] [--args]    Build (if needed) and run a project or app

  Info:
    help [command]         Show this help or help for a specific command
    version                Show CLI version information

Examples:
  vix new api
  vix build
  vix run api -- --port 8080
```

---

# 🪪 License

MIT License — Vix.cpp Authors
