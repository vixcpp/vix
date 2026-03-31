# vix run

Configure, build, and run a Vix.cpp application.

---

## Usage

```bash
vix run [name|file.cpp|manifest.vix] [options] [-- compiler/linker flags]
```

---

# Modes

## 1) Project Mode

Runs a CMake-based project (auto-detected).

```bash
vix run
vix run api
vix run --dir ./examples/blog
```

---

## 2) Script Mode (.cpp)

Compile and run a single file.

```bash
vix run main.cpp
```

Supports:

- ASan / UBSan
- Custom linker flags
- Fast iteration

---

## 3) Manifest Mode (.vix)

Run using a `.vix` manifest.

```bash
vix run api.vix
```

Manifest kinds:

- `script`  → entry = .cpp file
- `project` → entry = src/main.cpp

CLI flags override manifest values.

---

# Options

```
-d, --dir <path>              Project directory
--preset <name>               Configure preset (default: dev-ninja)
--run-preset <name>           Build preset for 'run' target
-j, --jobs <n>                Parallel jobs
--clear <auto|always|never>   Clear terminal before output
--no-clear                    Alias for --clear=never
```

---

# Runtime Options

```
--cwd <path>                  Working directory
--env <K=V>                   Environment variable (repeatable)
--args <value>                Runtime argument (repeatable)
```

Important:

- Use `--args` for runtime arguments
- Use `--` for compiler/linker flags (script mode only)

---

# Watch Mode

```
--watch
--reload
--force-server
--force-script
```

- `--watch` rebuilds and restarts on file changes
- `--force-server` treats app as long-lived
- `--force-script` treats app as short-lived

---

# Script Mode Flags

```
--san       Enable ASan + UBSan
--ubsan     Enable UBSan only
```

---

# Documentation (OpenAPI)

```
--docs
--no-docs
--docs=0|1|true|false
```

Maps to:

```
VIX_DOCS
```

---

# Logging

```
--log-level <level>
--verbose
--quiet
--log-format <kv|json|json-pretty>
--log-color <auto|always|never>
--no-color
```

Environment mappings:

```
VIX_LOG_LEVEL
VIX_LOG_FORMAT
VIX_COLOR
```

---

# Compiler / Linker Flags (Script Mode)

Use `--` separator:

```bash
vix run main.cpp -- -lssl -lcrypto
vix run main.cpp -- -DDEBUG
```

Everything after `--` is forwarded to the compiler.

---

# Examples

Script with runtime args:

```bash
vix run main.cpp --cwd ./data --args --config --args config.json
```

Disable auto docs:

```bash
vix run api.cpp --no-docs
```

Watch mode:

```bash
vix run --watch api
```

Cross preset:

```bash
vix run api --preset dev-ninja --run-preset run-dev-ninja
```

---

# Environment Variables

```
VIX_DOCS
VIX_LOG_LEVEL
VIX_LOG_FORMAT
VIX_COLOR
VIX_STDOUT_MODE
```

---

# Design Goals

- One command for build + run
- Script and project symmetry
- Manifest support
- Explicit runtime arguments
- Clean logging model
- Watch mode built-in

---

`vix run` is the main execution entry point of the Vix workflow.

