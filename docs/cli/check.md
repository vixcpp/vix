# vix check

`vix check` validates a Vix project or a single C++ file.

Use it when you want to verify that code builds correctly, tests pass, runtime execution works, and sanitizer checks are clean.

## Usage

```bash
vix check [path|file.cpp] [options]
```

## What it does

`vix check` is the validation command of the Vix CLI. It can detect a Vix or CMake project, configure the project, build the selected check profile, run tests with CTest, run the built executable, compile a single `.cpp` file, run sanitizer checks, validate runtime behavior, and use isolated build directories for check profiles.

## Basic usage

```bash
# Check the current project
vix check

# Check a project in another directory
vix check --dir ./examples/api

# Check a single C++ file
vix check main.cpp
```

## Project mode

In project mode, Vix can detect the project root, configure the selected preset, build the project, optionally run tests, optionally run the built executable, and optionally enable sanitizers.

## Script mode

Script mode is used when the input is a single `.cpp` file:

```bash
vix check main.cpp
```

Vix creates a temporary CMake project around the file, compiles it, and validates the result. With sanitizers enabled, Vix also runs the binary for runtime validation:

```bash
vix check main.cpp --san
```

## Run tests

```bash
vix check --tests
vix check --tests --run
vix check --san --tests
```

## Run the executable

```bash
vix check --run
vix check --run --run-timeout 20
```

## Sanitizer checks

```bash
# AddressSanitizer + UndefinedBehaviorSanitizer
vix check --san
vix check main.cpp --san

# UndefinedBehaviorSanitizer only
vix check --ubsan
vix check main.cpp --ubsan
```

## Smart sanitizer mode

By default, sanitizer mode is smart. For small projects, Vix checks the full project normally. For large or umbrella projects, Vix may switch to a reduced sanitizer configure to avoid unrelated failures.

## Full sanitizer mode

Use `--full` when you want the complete sanitizer configure:

```bash
vix check --san --full
```

Use it when preparing releases or debugging deeper build problems.

## Build presets

```bash
vix check --preset dev-ninja
vix check --build-preset build-ninja
```

## Parallel jobs

```bash
vix check -j 8
```

## CTest options

```bash
vix check --tests --ctest-preset test-dev
vix check --tests --ctest-arg --output-on-failure
vix check --tests \
  --ctest-arg --output-on-failure \
  --ctest-arg -R \
  --ctest-arg Auth
```

## Options

| Option | Description |
|--------|-------------|
| `-d, --dir <path>` | Explicit project directory. |
| `--preset <name>` | Configure preset. Default is `dev-ninja`. |
| `--build-preset <name>` | Build preset override. |
| `-j, --jobs <n>` | Number of parallel build jobs. |
| `--tests` | Run CTest after build. |
| `--ctest-preset <name>` | CTest preset override. |
| `--ctest-arg <arg>` | Extra argument forwarded to CTest. Repeatable. |
| `--run` | Run the built executable after build. |
| `--run-timeout <sec>` | Runtime timeout in seconds. |
| `-q, --quiet` | Minimal output. |
| `--verbose` | More verbose logging. |
| `--log-level <level>` | Set `VIX_LOG_LEVEL` for the check session. |
| `--san` | Enable AddressSanitizer and UndefinedBehaviorSanitizer. |
| `--ubsan` | Enable UndefinedBehaviorSanitizer only. |
| `--full` | Force the complete sanitizer check. |
| `-h, --help` | Show command help. |

## Build directories

Project checks use isolated build directories per profile: `build-ninja`, `build-ninja-san`, `build-ninja-ubsan`.

## Common workflows

```bash
# Basic project validation
vix check

# Build and run tests
vix check --tests

# Build and run the executable
vix check --run

# Full validation
vix check --tests --run

# Sanitizer validation
vix check --san

# Sanitizers with tests
vix check --san --tests

# Sanitizers with tests and runtime timeout
vix check --san --tests --run-timeout 20

# Strict full sanitizer check
vix check --san --full

# Single file validation
vix check main.cpp

# Single file sanitizer validation
vix check main.cpp --san
```

## Common mistakes

### Using `--full` for every local check

For daily work, prefer `vix check --san`. Use `--full` only when you intentionally want a complete project-level sanitizer configure.

## Related commands

| Command | Purpose |
|---------|---------|
| `vix build` | Configure and compile the project |
| `vix run` | Build and run the app |
| `vix dev` | Run the app with reload |
| `vix tests` | Run project tests |
| `vix fmt` | Format source files |

## Next step

Continue with tests.

[Open the vix tests guide](/cli/tests)
