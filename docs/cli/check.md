# vix check

`vix check` validates a Vix project, a CMake project, or a single C++ file.

Use it when you want to verify that code builds correctly, tests pass, runtime execution works, and sanitizer checks are clean.

## Usage

```bash
vix check [path|file.cpp] [options]
```

## Overview

`vix check` is the validation command of the Vix CLI.

It can:

- detect a Vix or CMake project
- configure the selected check profile
- build the project
- run tests when requested
- run the built executable when requested
- validate a single `.cpp` file
- run sanitizer checks
- use isolated build directories for check profiles
- keep normal output clean
- show deeper details with `--verbose`

## Output style

`vix check` uses the same visual style as `vix build` and `vix tests`.

Example project check:

```bash
vix check
```

```text
Checking kv (dev)
  * profile: default | tests: off | runtime: off | jobs: 8
  build [============================] done
  ✔ Project check OK (build) in 0.1s
```

Example project check with tests:

```bash
vix check --tests
```

```text
Checking kv (dev)
  * profile: default | tests: on | runtime: off | jobs: 8
  build [============================] done
Testing all (dev)
  * engine: vix | jobs: 8
  tests [============================] done
  ✔ Passed 23 tests in 0.2s
  ✔ Project check OK (build, tests) in 0.3s
```

Example script check:

```bash
vix check main.cpp
```

```text
Checking main.cpp (script)
  * profile: default | runtime: off | jobs: 8
  build [============================] done
  ✔ Script check OK (build) in 0.6s
```

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

Project mode is used when Vix detects a `CMakeLists.txt`.

```bash
vix check
```

In project mode, Vix can:

- detect the project root
- select the configure preset
- prepare the check profile
- build the project
- optionally run tests
- optionally run the built executable
- optionally use sanitizers

By default, project mode builds the project only.

```bash
vix check
```

This does not run tests or runtime validation unless you ask for them.

## Script mode

Script mode is used when the input is a single `.cpp` file.

```bash
vix check main.cpp
```

Vix creates a temporary CMake project around the file, compiles it, and validates the result.

Example:

```text
Checking main.cpp (script)
  * profile: default | runtime: off | jobs: 8
  build [============================] done
  ✔ Script check OK (build) in 0.6s
```

With sanitizers enabled, Vix also runs the binary for runtime validation:

```bash
vix check main.cpp --san
```

Example:

```text
Checking main.cpp (script)
  * profile: asan+ubsan | runtime: on | jobs: 8
  configure [============================] done
  build [============================] done
  runtime [============================] done
  ✔ Script check OK (build, runtime, asan+ubsan) in 0.8s
```

## Project sanitizers

Use `--san` to validate a project with AddressSanitizer and UndefinedBehaviorSanitizer.

```bash
vix check --san
```

In project mode, `--san` validates the sanitizer build profile.

It does not automatically run a runtime executable, because many projects are libraries and do not produce an executable with the project directory name.

Example:

```text
Checking kv (dev)
  * profile: asan+ubsan | tests: off | runtime: off | jobs: 8
  build [============================] done
  ✔ Project check OK (build, asan+ubsan) in 0.4s
```

To run the executable after the sanitizer build, explicitly add `--run`:

```bash
vix check --san --run
```

## Script sanitizers

For a single `.cpp` file, sanitizer checks compile and run the binary.

```bash
vix check main.cpp --san
```

This is useful because a single file usually represents a runnable program.

## Run tests

Use `--tests` to run tests after the build.

```bash
vix check --tests
```

`vix check --tests` reuses the `vix tests` experience instead of exposing the internal test runner directly.

Example:

```text
Checking kv (dev)
  * profile: default | tests: on | runtime: off | jobs: 8
  build [============================] done
Testing all (dev)
  * engine: vix | jobs: 8
  tests [============================] done
  ✔ Passed 23 tests in 0.2s
  ✔ Project check OK (build, tests) in 0.3s
```

You can combine tests with sanitizers:

```bash
vix check --san --tests
```

## Run the executable

Use `--run` to run the built executable after the build.

```bash
vix check --run
```

Use a timeout when the program may stay alive, such as an HTTP server:

```bash
vix check --run --run-timeout 20
```

For projects, `--run` expects a runnable binary matching the project directory name.

For example, in a project named `api`, Vix looks for a runnable binary named `api` in the build output.

## Full validation

```bash
vix check --tests --run
```

This runs:

1. build
2. tests
3. runtime validation

If tests fail, runtime validation is not executed.

## Sanitizer checks

```bash
# AddressSanitizer + UndefinedBehaviorSanitizer
vix check --san
vix check main.cpp --san

# UndefinedBehaviorSanitizer only
vix check --ubsan
vix check main.cpp --ubsan

# ThreadSanitizer
vix check --tsan
vix check main.cpp --tsan
```

## Smart sanitizer mode

By default, sanitizer mode is smart.

For small projects, Vix checks the project normally.

For large or umbrella projects, Vix may switch to a reduced sanitizer configure to avoid unrelated packaging, export, install, or auxiliary target failures.

This keeps day-to-day sanitizer validation focused on useful feedback.

## Full sanitizer mode

Use `--full` when you want the complete sanitizer configure.

```bash
vix check --san --full
```

Use it when preparing releases or debugging deeper build problems.

## Presets

```bash
vix check --preset dev-ninja
vix check --preset release
```

`--preset` controls the configure preset.

If no preset is provided, Vix defaults to:

```bash
dev-ninja
```

You can also override the build preset:

```bash
vix check --build-preset build-ninja
```

## Parallel jobs

```bash
vix check -j 8
vix check --jobs 8
```

If no job count is provided, Vix chooses a safe default based on available CPU threads.

The active job count is shown in the header:

```text
* profile: default | tests: off | runtime: off | jobs: 8
```

## Test options

For normal usage, prefer `vix tests` directly when you need advanced test selection.

```bash
vix tests --test kv_test_segment
vix tests -R kv_test_segment
vix tests -v
vix tests --raw
```

For `vix check`, use `--tests` for the standard validation flow:

```bash
vix check --tests
```

You can still forward internal test runner arguments when needed:

```bash
vix check --tests --ctest-preset test-dev
vix check --tests --ctest-arg --output-on-failure
vix check --tests \
  --ctest-arg --output-on-failure \
  --ctest-arg -R \
  --ctest-arg Auth
```

## Verbose mode

Use `--verbose` to show more internal details.

```bash
vix check --verbose
vix check main.cpp --verbose
```

Example script verbose output:

```text
Check resolution
  • mode      : script
  • script    : /home/user/tmp/main.cpp

Checking main.cpp (script)
  * profile: default | runtime: off | jobs: 8

Check details
  • script    : /home/user/tmp/main.cpp
  • project dir: /home/user/.vix/cache/scripts/main
  • build dir : /home/user/.vix/cache/scripts/main/build
  • profile   : default
  • runtime   : disabled

✔ Matching cache detected for this script profile.
  • build dir : /home/user/.vix/cache/scripts/main/build

• Starting build.
  • target    : main

build [============================] done
✔ Script check OK (build) in 0.1s
```

Normal mode keeps these details hidden.

## Quiet mode

Use `--quiet` for minimal output.

```bash
vix check --quiet
vix check main.cpp --quiet
```

## Build directories

Project checks use isolated build directories per profile.

Common examples:

| Profile             | Build directory     |
| ------------------- | ------------------- |
| default dev-ninja   | `build-ninja`       |
| sanitizer dev-ninja | `build-ninja-san`   |
| UBSan dev-ninja     | `build-ninja-ubsan` |
| release             | `build-release`     |

Script checks use the Vix script cache.

Example:

```text
~/.vix/cache/scripts/main/
```

With local cache enabled:

```bash
vix check main.cpp --local-cache
```

Vix uses a local script cache instead of the global one.

## Options

| Option                  | Description                                                       |
| ----------------------- | ----------------------------------------------------------------- |
| `-d, --dir <path>`      | Explicit project directory.                                       |
| `--preset <name>`       | Configure preset. Default is `dev-ninja`.                         |
| `--build-preset <name>` | Build preset override.                                            |
| `-j, --jobs <n>`        | Number of parallel build jobs.                                    |
| `--tests`               | Run tests after build.                                            |
| `--ctest-preset <name>` | Internal test preset override.                                    |
| `--ctest-arg <arg>`     | Extra argument forwarded to the internal test runner. Repeatable. |
| `--run`                 | Run the built executable after build.                             |
| `--run-timeout <sec>`   | Runtime timeout in seconds.                                       |
| `-q, --quiet`           | Minimal output.                                                   |
| `--verbose`             | More verbose logging.                                             |
| `--log-level <level>`   | Set `VIX_LOG_LEVEL` for the check session.                        |
| `--san`                 | Enable AddressSanitizer and UndefinedBehaviorSanitizer.           |
| `--ubsan`               | Enable UndefinedBehaviorSanitizer only.                           |
| `--tsan`                | Enable ThreadSanitizer.                                           |
| `--full`                | Force the complete sanitizer check.                               |
| `--with-sqlite`         | Enable SQLite backend for single-file checks when supported.      |
| `--with-mysql`          | Enable MySQL backend for single-file checks when supported.       |
| `--local-cache`         | Use a local script cache for single-file checks.                  |
| `-h, --help`            | Show command help.                                                |

## Common workflows

```bash
# Basic project validation
vix check

# Build and run tests
vix check --tests

# Build and run the executable
vix check --run

# Full project validation
vix check --tests --run

# Project sanitizer build validation
vix check --san

# Project sanitizer build plus tests
vix check --san --tests

# Project sanitizer build plus runtime
vix check --san --run

# Strict full sanitizer check
vix check --san --full

# Single-file validation
vix check main.cpp

# Single-file sanitizer validation
vix check main.cpp --san

# Single-file ThreadSanitizer validation
vix check main.cpp --tsan

# Verbose diagnostics
vix check --verbose

# Validate another project
vix check --dir ./examples/api
```

## Common mistakes

### Expecting project `--san` to run automatically

In project mode, `--san` validates the sanitizer build profile.

It does not automatically run an executable.

Use:

```bash
vix check --san --run
```

when you really want runtime validation too.

### Using `--run` on a library project

Library projects may not produce an executable matching the project folder name.

For libraries, prefer:

```bash
vix check
vix check --tests
vix check --san
```

Use `--run` only for application projects.

### Using `--full` for every local check

For daily work, prefer:

```bash
vix check --san
```

Use `--full` when you intentionally want a complete project-level sanitizer configure.

### Expecting `--tests` to behave like raw CTest

`vix check --tests` uses the Vix test experience.

For advanced test usage, prefer:

```bash
vix tests --test name
vix tests -v
vix tests --raw
```

### Passing many test runner options through `vix check`

This works:

```bash
vix check --tests --ctest-arg -R --ctest-arg Auth
```

But this is clearer:

```bash
vix tests -R Auth
```

Use `vix check --tests` for validation workflows. Use `vix tests` for test-focused workflows.

## Recommended validation flow

During development:

```bash
vix check
vix tests
```

Before committing:

```bash
vix fmt --check
vix check --tests
```

For sanitizer validation:

```bash
vix check --san
```

For application runtime validation:

```bash
vix check --run
```

For release preparation:

```bash
vix build --preset release --build-target all
vix tests --preset release
vix check --san --full
```

## Difference between `vix check`, `vix build`, and `vix tests`

| Command     | Purpose                                                                   |
| ----------- | ------------------------------------------------------------------------- |
| `vix build` | Configure and compile the project.                                        |
| `vix tests` | Run project tests with a Vix-native test output.                          |
| `vix check` | Validate build, optional tests, optional runtime, and sanitizer profiles. |

## Related commands

| Command     | Purpose                            |
| ----------- | ---------------------------------- |
| `vix build` | Configure and compile the project. |
| `vix run`   | Build and run the app manually.    |
| `vix dev`   | Run the app with reload.           |
| `vix tests` | Run project tests.                 |
| `vix fmt`   | Format source files.               |

## Next step

Continue with tests.

[Open the vix tests guide](/cli/tests)
