# vix tests

`vix tests` runs project tests.

Use it when you want to execute the test suite of a Vix or CMake project with a clean Vix-native output.

```bash
vix tests
```

## Overview

`vix tests` is the test runner command of the Vix CLI.

It detects the current project, resolves the build directory from the selected preset, runs the available test backend, summarizes results, and reports failures with a clean Vix diagnostic format.

Even when Vix uses CTest internally, the command stays a Vix command from the user's point of view.

## Usage

```bash
vix tests [path] [options] [-- raw-runner-args...]
```

## What it does

`vix tests` can:

- run all tests in the current project
- run tests in another project directory
- run tests from a selected preset
- run a single test or a filtered group of tests
- list tests
- stop on the first failure
- run tests in watch mode
- run runtime checks after tests
- show clean Vix test diagnostics
- show more structured details with `-v`
- show raw internal runner output with `--raw`
- pass raw internal runner arguments after `--`

## Basic usage

```bash
# Run tests in the current project
vix tests

# Run tests in another project
vix tests ./examples/blog

# Run tests with a specific preset
vix tests --preset release
```

## Output style

`vix tests` uses the same visual style as `vix build`.

Example success:

```bash
Testing all (dev)
  * engine: vix | jobs: 8
  tests [============================] done
  ✔ Passed 23 tests in 0.1s
```

Example failure:

```bash
Testing all (dev)
  * engine: vix | jobs: 8
  tests [============================] failed
  ✖ Failed 1 of 23 tests after 0.1s

  failed:
    kv_test_open

  error:
    FAILED: fast database should not be memory_only

  ➜ Run `vix tests -v` to show detailed Vix test output.
  ➜ Run `vix tests --raw` to show raw runner output.
```

By default, Vix does not dump the raw internal runner output. It extracts the useful failure information and presents it clearly.

## Run all tests

```bash
vix tests
```

This runs the test suite for the current project.

Vix resolves the build directory from the active preset. If no preset is provided, it uses `dev-ninja`.

## Run tests from another project

```bash
vix tests ./examples/blog
```

The first path-like argument is treated as the project directory.

## Presets

```bash
vix tests --preset release
```

If no preset is provided, Vix defaults to:

```bash
dev-ninja
```

Common presets:

```bash
vix tests --preset dev
vix tests --preset dev-ninja
vix tests --preset release
```

Preset mapping:

| Preset | Build directory |
|--------|-----------------|
| `dev` | `build-dev` |
| `dev-ninja` | `build-ninja` |
| `release` | `build-release` |

## Run one test

Use `--test` to run one test or tests matching a pattern.

```bash
vix tests --test kv_test_segment
```

You can also use `-R` as an alias:

```bash
vix tests -R kv_test_segment
```

This is the Vix-native way to do what you would normally do with:

```bash
ctest --test-dir build-ninja -R kv_test_segment --output-on-failure
```

But with `vix tests`, you do not need to think about the build directory or the internal runner.

Example output:

```bash
Testing kv_test_segment (dev)
  * engine: vix | jobs: 8
  tests [============================] done
  ✔ Passed 1 test in 0.1s
```

## Run tests matching a pattern

`--test` accepts a name or pattern.

```bash
vix tests --test kv_test
vix tests --test persistence
vix tests -R wal
```

This is useful when you want to focus on a specific area while developing.

## Verbose mode

Use `-v` or `--verbose` to show more Vix-formatted details.

```bash
vix tests -v
vix tests --verbose
```

`-v` should not expose raw internal runner output. It keeps the Vix interface and shows structured details.

Example:

```bash
Testing all (dev)
  * engine: vix | jobs: 8
  tests [============================] failed
  ✖ Failed 1 of 23 tests after 0.1s

  failed:
    kv_test_open

  error:
    FAILED: fast database should not be memory_only

  details:
    kv_test_open
      FAILED: fast database should not be memory_only
```

## Raw mode

Use `--raw` only when you want to inspect the raw output of the internal test runner.

```bash
vix tests --raw
```

Raw mode is useful when debugging the runner itself or when you need the exact backend output.

Example:

```bash
vix tests --test kv_test_segment --raw
```

Normal development should use:

```bash
vix tests
vix tests -v
vix tests --test name
```

## Jobs and parallel execution

`vix tests` runs tests with parallel execution when supported.

Example output:

```bash
Testing all (dev)
  * engine: vix | jobs: 8
  tests [============================] done
  ✔ Passed 23 tests in 0.1s
```

The `jobs` value is derived from available CPU threads, with a safe upper limit.

When Vix uses CTest internally, it passes parallel execution to the runner automatically.

## Watch mode

Use `--watch` to watch project files and re-run tests when files change.

```bash
vix tests --watch
```

Vix watches relevant project files such as:

- `.cpp`
- `.cc`
- `.cxx`
- `.hpp`
- `.hh`
- `.hxx`
- `.h`
- `.cmake`
- `CMakeLists.txt`
- `CMakePresets.json`

It ignores generated or unrelated directories such as:

- `.git`
- `.idea`
- `.vscode`
- build directories
- `dist`

## Run runtime checks after tests

Use `--run` to run runtime checks after tests pass.

```bash
vix tests --run
```

If tests fail, runtime checks are not executed.

This is useful when you want:

```text
tests first, runtime validation second
```

## List tests

```bash
vix tests --list
```

This lists available tests for the current project.

You can combine it with a pattern:

```bash
vix tests --list --test kv
vix tests --list -R wal
```

## Fail fast

Use `--fail-fast` to stop after the first failing test.

```bash
vix tests --fail-fast
```

This is useful when you want fast feedback during debugging.

## Raw runner arguments

Arguments after `--` are passed to the internal test runner.

```bash
vix tests -- --output-on-failure
vix tests -- -R MySuite
vix tests -- --output-on-failure -R Auth
```

For normal Vix usage, prefer the Vix-native options:

```bash
vix tests -v
vix tests --test MySuite
vix tests --raw
```

Use raw runner arguments only when you need a backend-specific option.

## Native runner and fallback

`vix tests` prefers a native test runner when available.

If no native runner is found, Vix falls back to the available project test backend.

The interface remains the same:

```bash
vix tests
```

Vix should not make the command feel like a direct CTest wrapper. Internal tools are implementation details.

## When tests are not configured yet

If the build directory does not exist or tests were not generated, Vix reports a clear message.

Example:

```text
✖ No build directory found for tests.
➜ Run `vix build --build-target all` first.
```

Recommended setup:

```bash
vix build --build-target all
vix tests
```

## Options

| Option | Description |
|--------|-------------|
| `--watch` | Watch files and rerun tests on changes. |
| `--run` | Run runtime checks after tests pass. |
| `--list` | List available tests. |
| `--test <name\|pattern>` | Run tests matching a name or pattern. |
| `-R <name\|pattern>` | Alias for `--test`. |
| `--fail-fast` | Stop after the first failing test. |
| `-v, --verbose` | Show detailed Vix-formatted test output. |
| `--raw` | Show raw internal test runner output. |
| `--preset <name>` | Use a specific preset. |
| `--` | Pass raw arguments to the internal test runner. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# Run all tests
vix tests

# Run tests while editing
vix tests --watch

# Run tests and runtime checks
vix tests --run

# Run tests in release preset
vix tests --preset release

# List tests
vix tests --list

# Stop on first failure
vix tests --fail-fast

# Show detailed Vix output
vix tests -v

# Show raw internal runner output
vix tests --raw

# Run one test
vix tests --test kv_test_segment

# Run one test with short syntax
vix tests -R kv_test_segment

# Run tests matching a pattern
vix tests --test wal

# Run one test in release mode
vix tests --preset release --test kv_test_segment
```

## Common mistakes

### Using raw runner syntax when Vix has a native option

Avoid this for normal usage:

```bash
vix tests -- -R kv_test_segment --output-on-failure
```

Prefer:

```bash
vix tests --test kv_test_segment
```

Or:

```bash
vix tests -R kv_test_segment
```

### Expecting `-v` to show raw backend output

`-v` shows detailed Vix-formatted output.

For raw backend output, use:

```bash
vix tests --raw
```

### Forgetting to build tests first

If tests were not generated yet, run:

```bash
vix build --build-target all
vix tests
```

### Passing raw runner options without `--`

Wrong:

```bash
vix tests --output-on-failure
```

Correct:

```bash
vix tests -- --output-on-failure
```

But for most cases, prefer:

```bash
vix tests -v
```

### Confusing build preset and log level

Wrong:

```bash
VIX_LOG_LEVEL=release vix tests
```

Correct:

```bash
vix tests --preset release
```

`VIX_LOG_LEVEL` controls logging. `--preset release` controls the build/test preset.

## Troubleshooting

### Need to see the failing test message

```bash
vix tests
```

Vix shows the first useful failure message automatically.

For more structured detail:

```bash
vix tests -v
```

For raw backend output:

```bash
vix tests --raw
```

### Need to run only one failing test

```bash
vix tests --test kv_test_open
```

Or:

```bash
vix tests -R kv_test_open
```

### Need to stop after the first failure

```bash
vix tests --fail-fast
```

### Need to list tests

```bash
vix tests --list
```

### Need to run tests from another folder

```bash
vix tests ./examples/blog
```

### Need to use release mode

```bash
vix build --preset release --build-target all
vix tests --preset release
```

## Recommended validation flow

During development:

```bash
vix tests
```

When debugging one failure:

```bash
vix tests --test failing_test_name
vix tests -v --test failing_test_name
```

Before committing:

```bash
vix fmt --check
vix build --build-target all
vix tests
```

For stricter validation:

```bash
vix check --san --tests
```

For release preparation:

```bash
vix build --preset release --build-target all
vix tests --preset release
vix check --san --full
```

## Difference between `vix tests`, `vix check --tests`, and raw runner usage

| Command | Purpose |
|---------|---------|
| `vix tests` | Run tests with clean Vix-native output. |
| `vix tests -v` | Run tests with detailed Vix-formatted output. |
| `vix tests --raw` | Show raw internal runner output. |
| `vix check --tests` | Run tests as part of the broader validation workflow. |
| `vix tests -- ...` | Pass backend-specific arguments to the internal runner. |

## Related commands

| Command | Purpose |
|---------|---------|
| `vix check --tests` | Build and run tests as part of validation. |
| `vix check --san --tests` | Run tests with sanitizers. |
| `vix build` | Configure and build the project. |
| `vix fmt` | Format source files. |
| `vix dev` | Run the app in development mode. |
| `vix run` | Build and run an application manually. |

## Next step

Continue with reusable project tasks.

[Open the vix task guide](/cli/task)
