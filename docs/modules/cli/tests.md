# vix tests

Run project tests using CTest.

---

## Usage

```bash
vix tests [path] [options]
```

---

## Description

`vix tests`:

- Resolves build directory from CMakePresets.json
- Uses the preset (default: dev-ninja)
- Executes CTest
- Integrates with Vix build system

It is equivalent to:

```
vix check --tests
```

But provides a cleaner developer interface.

---

# Test Flags

```
--watch         Watch files and re-run tests on changes
--list          List tests only (ctest --show-only)
--fail-fast     Stop on first failure
--run           Run runtime check after tests
```

---

# CTest Passthrough

Use `--` to pass raw arguments directly to CTest:

```bash
vix tests -- --output-on-failure
```

Run specific test suite:

```bash
vix tests -- --output-on-failure -R MySuite
```

---

# Preset Behavior

- Preset is taken from forwarded args (e.g. --preset release)
- Default preset: dev-ninja

Example:

```bash
vix tests --preset release
```

---

# Examples

Basic run:

```bash
vix tests
```

Watch mode:

```bash
vix tests --watch
```

List tests:

```bash
vix tests --list
```

Stop on first failure:

```bash
vix tests --fail-fast
```

Run tests + runtime:

```bash
vix tests --run
```

Specify project path:

```bash
vix tests ./examples/blog
```

---

# Notes

- Build directory is derived from CMakePresets.json (binaryDir).
- Other `vix check` options may be forwarded.
- Designed for clean CI integration.

---

`vix tests` integrates seamlessly into the Vix development workflow.

