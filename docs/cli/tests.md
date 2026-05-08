# vix tests

`vix tests` runs project tests.

Use it when you want to execute the test suite of a Vix or CMake project.

## Usage

```bash
vix tests [path] [options]
```

## What it does

`vix tests` is the test runner command of the Vix CLI. It can detect the current project, resolve the build directory from `CMakePresets.json`, run the native test runner when available, fall back to CTest when needed, forward raw CTest arguments, watch files and rerun tests on changes, and combine tests with runtime checks.

## Basic usage

```bash
# Run tests in the current project
vix tests

# Run tests in another project
vix tests ./examples/blog

# Run tests with a specific preset
vix tests --preset release
```

## Native runner and CTest

`vix tests` prefers the native Vix test runner when available. CTest is used when the native runner is not available, raw CTest arguments are passed after `--`, or the command needs to fall back.

Example with raw CTest arguments:

```bash
vix tests -- --output-on-failure -R MySuite
```

## Watch mode

```bash
vix tests --watch
```

## Run runtime checks after tests

```bash
vix tests --run
```

## List tests

```bash
vix tests --list
```

## Fail fast

```bash
vix tests --fail-fast
```

## Forward raw CTest arguments

```bash
vix tests -- --output-on-failure
vix tests -- -R MySuite
vix tests -- --output-on-failure -R Auth
```

## Presets

```bash
vix tests --preset release
```

If no preset is provided, Vix defaults to `dev-ninja`.

## When tests are not configured yet

If tests are not configured yet, `vix tests` uses `vix check --tests` to configure, build, and run the test suite through the validation workflow.

## Options

| Option | Description |
|--------|-------------|
| `--watch` | Watch files and rerun tests on changes. |
| `--run` | Run runtime check after tests. |
| `--list` | Forward to CTest test listing mode. |
| `--fail-fast` | Stop after the first failing test. |
| `--` | Pass raw arguments to CTest. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# Run all tests
vix tests

# Run tests while editing
vix tests --watch

# Run tests and runtime check
vix tests --run

# Run tests in release preset
vix tests --preset release

# Show tests
vix tests --list

# Stop on first failure
vix tests --fail-fast

# Show detailed CTest failures
vix tests -- --output-on-failure

# Run one test suite
vix tests -- -R MySuite
```

## Common mistakes

### Forgetting `--` before raw CTest arguments

Wrong:

```bash
vix tests -R MySuite
```

Correct:

```bash
vix tests -- -R MySuite
```

## Recommended validation flow

```bash
# During development
vix tests

# Before committing
vix fmt --check
vix check --tests

# For stricter validation
vix check --san --tests

# For release preparation
vix build --preset release
vix tests --preset release
vix check --san --full
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix check --tests` | Build and run tests as part of validation |
| `vix check --san --tests` | Run tests with sanitizers |
| `vix build` | Configure and build the project |
| `vix fmt` | Format source files |
| `vix dev` | Run the app in development mode |

## Next step

Continue with reusable project tasks.

[Open the vix task guide](/cli/task)
