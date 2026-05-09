# vix fmt

`vix fmt` formats C++ source files using `clang-format`.

Use it when you want consistent formatting across your project.

## Usage

```bash
vix fmt [options] [files...]
```

## What it does

`vix fmt` scans files or directories and applies `clang-format`. It can format one file, multiple files, or directories, and check formatting without changing files.

## Basic usage

```bash
# Format the default project folders
vix fmt

# Format specific folders
vix fmt src include

# Format one file
vix fmt main.cpp

# Check whether a file is formatted
vix fmt main.cpp --check

# Ignore a path
vix fmt src --ignore=build --ignore=vendor
```

## Default behavior

If no files or directories are provided, `vix fmt` scans `src/` and `include/`. A normal project can usually be formatted with:

```bash
vix fmt
```

## Check mode

Use `--check` when you only want to verify formatting:

```bash
vix fmt --check
vix fmt src include --check
vix fmt main.cpp --check
```

This is useful before committing or in CI.

## Ignore paths

```bash
vix fmt src include --ignore=build
vix fmt src include --ignore=build --ignore=vendor
```

Common ignored folders: `build`, `vendor`, `third_party`, `.vix`, `dist`

## Project formatting file

If a `.clang-format` file exists, `clang-format` uses it automatically. Then `vix fmt` uses the formatting rules from `.clang-format`.

## Options

| Option | Description |
|--------|-------------|
| `--check` | Check if files are formatted without modifying them. |
| `--ignore <path>` | Ignore a file or path pattern. Repeatable. |
| `-q, --quiet` | Suppress non-essential output. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# Format the current project
vix fmt

# Check formatting before commit
vix fmt --check

# Format source and headers
vix fmt src include

# Format one file
vix fmt src/main.cpp

# Format while ignoring build outputs
vix fmt src include --ignore=build --ignore=.vix
```

## CI usage

In CI, use check mode:

```bash
vix fmt --check
vix check --tests
```

## Common mistakes

### Forgetting `--check` in CI

This modifies files:

```bash
vix fmt
```

In CI, prefer:

```bash
vix fmt --check
```

### Formatting generated or vendor files

```bash
vix fmt src include --ignore=vendor --ignore=build --ignore=.vix
```

## Recommended project task

```json
{
  "tasks": {
    "fmt": "vix fmt",
    "fmt:check": "vix fmt --check"
  }
}
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix make` | Generate C++ files |
| `vix check` | Validate build, tests, runtime, and sanitizers |
| `vix tests` | Run tests |
| `vix task` | Run reusable project tasks |
| `vix build` | Compile the project |

## Next step

Continue with project cleanup.

[Open the vix clean guide](/cli/clean)
