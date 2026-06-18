# vix fmt

`vix fmt` formats C++ source files using `clang-format`.

Use it when you want consistent formatting across your project.

## Usage

```bash
vix fmt [options] [files...]
```

## What it does

`vix fmt` scans C++ files and formats them with `clang-format`.

It can format:

```txt
one file
multiple files
one directory
multiple directories
the default project folders
```

It can also check formatting without modifying files.

## Requirements

`vix fmt` requires `clang-format`.

If `clang-format` is not installed, Vix stops with an error:

```txt
clang-format not found
Install clang-format to use 'vix fmt'
```

Install `clang-format` before using this command.

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

# Ignore paths
vix fmt src --ignore=build --ignore=vendor
```

## Default behavior

If no files or directories are provided, `vix fmt` scans:

```txt
src/
include/
```

A normal project can usually be formatted with:

```bash
vix fmt
```

If neither `src/` nor `include/` contains C++ files, Vix prints:

```txt
No C++ files found
```

and exits successfully.

## Supported file extensions

`vix fmt` formats these file types:

```txt
.cpp
.hpp
.h
.cc
.cxx
.hh
.hxx
```

Other files are ignored.

## Format specific directories

```bash
vix fmt src
vix fmt include
vix fmt src include
```

When a directory is passed, Vix scans it recursively.

Example:

```bash
vix fmt src include
```

This formats C++ files inside both directories.

## Format specific files

```bash
vix fmt main.cpp
vix fmt src/main.cpp include/app.hpp
```

Only valid C++ files are formatted.

If a provided file does not exist or is not a supported C++ file, it is ignored.

## Check mode

Use `--check` when you only want to verify formatting:

```bash
vix fmt --check
vix fmt src include --check
vix fmt main.cpp --check
```

In check mode, Vix runs:

```bash
clang-format --dry-run --Werror <file>
```

It does not modify files.

If all files are formatted, Vix prints:

```txt
All files are properly formatted
```

If some files need formatting, Vix prints the file paths and exits with code `1`.

## Format mode

Without `--check`, Vix formats files in place.

It runs:

```bash
clang-format -i <file>
```

Example:

```bash
vix fmt src include
```

Vix prints each formatted file unless `--quiet` is enabled.

## Ignore paths

Use `--ignore` to skip a file, directory, or path pattern.

```bash
vix fmt src include --ignore=build
vix fmt src include --ignore=build --ignore=vendor
vix fmt src include --ignore .vix
```

`--ignore` is repeatable.

Common ignored folders:

```txt
build
build-ninja
build-release
vendor
third_party
.vix
dist
node_modules
```

Ignore matching is path-based. If the path contains the ignored value, it is skipped.

Example:

```bash
vix fmt . --ignore=build
```

This skips paths such as:

```txt
build/main.cpp
build-ninja/generated.cpp
```

## Quiet mode

Use `-q` or `--quiet` to suppress non-essential output:

```bash
vix fmt --quiet
vix fmt src include --quiet
vix fmt --check --quiet
```

Quiet mode is useful in scripts where only the exit code matters.

## Project formatting file

If a `.clang-format` file exists, `clang-format` uses it automatically.

`vix fmt` does not replace your formatting rules. It delegates formatting to `clang-format`.

Recommended project root:

```txt
.clang-format
src/
include/
```

## Options

| Option            | Description                                               |
| ----------------- | --------------------------------------------------------- |
| `--check`         | Check whether files are formatted without modifying them. |
| `--ignore <path>` | Ignore a file or path pattern. Repeatable.                |
| `--ignore=<path>` | Same as `--ignore <path>`.                                |
| `-q, --quiet`     | Suppress non-essential output.                            |
| `-h, --help`      | Show command help.                                        |

## Common workflows

### Format the current project

```bash
vix fmt
```

### Check formatting before commit

```bash
vix fmt --check
```

### Format source and headers

```bash
vix fmt src include
```

### Format one file

```bash
vix fmt src/main.cpp
```

### Format generated files after `vix make`

```bash
vix make class User --in src/domain --namespace app::domain
vix fmt src/domain
```

### Format while ignoring build outputs

```bash
vix fmt . \
  --ignore=build \
  --ignore=build-ninja \
  --ignore=.vix \
  --ignore=vendor
```

### Quiet CI check

```bash
vix fmt --check --quiet
```

## CI usage

In CI, use check mode:

```bash
vix fmt --check
vix check --tests
```

This prevents CI from modifying files.

A stricter CI workflow:

```bash
vix fmt --check
vix build --preset release
vix tests --preset release
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

Then run:

```bash
vix task fmt
vix task fmt:check
```

## Exit behavior

| Situation                             | Exit code |
| ------------------------------------- | --------- |
| All files formatted successfully      | `0`       |
| No C++ files found                    | `0`       |
| Check mode passes                     | `0`       |
| Check mode finds unformatted files    | `1`       |
| `clang-format` is missing             | `1`       |
| Formatting fails on at least one file | `1`       |
| Invalid option                        | `1`       |

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

Use ignore rules:

```bash
vix fmt . \
  --ignore=vendor \
  --ignore=third_party \
  --ignore=build \
  --ignore=.vix
```

### Expecting `vix fmt` to format every language

`vix fmt` formats C++ files only.

Supported extensions:

```txt
.cpp
.hpp
.h
.cc
.cxx
.hh
.hxx
```

### Running without `clang-format`

Install `clang-format` first.

Linux example:

```bash
sudo apt install clang-format
```

macOS example:

```bash
brew install clang-format
```

### Expecting Vix to define formatting rules

`vix fmt` runs `clang-format`.

Your formatting style should live in:

```txt
.clang-format
```

## Related commands

| Command     | Purpose                                         |
| ----------- | ----------------------------------------------- |
| `vix make`  | Generate C++ files.                             |
| `vix check` | Validate build, tests, runtime, and sanitizers. |
| `vix tests` | Run tests.                                      |
| `vix task`  | Run reusable project tasks.                     |
| `vix build` | Compile the project.                            |
| `vix clean` | Remove local project cache directories.         |

## Next step

Continue with project cleanup.

[Open the vix clean guide](/cli/clean)
