# vix build

`vix build` configures and builds a CMake project using Vix presets.

Use it when you want to compile a project without running the application.

```bash
vix build
```

## Overview

`vix build` gives C++ projects a clean and fast build workflow.

It detects the current project, configures CMake, builds with Ninja, uses Vix presets, supports parallel builds, enables optional features such as SQLite or MySQL, uses compiler launchers such as ccache or sccache, can use fast linkers such as mold or lld, and writes build logs automatically.

By default, `vix build` builds the main project target, not the full `all` target.

This keeps normal development builds fast.

```bash
vix build
```

For a full build of every target, use:

```bash
vix build --build-target all
```

## Usage

```bash
vix build [options] -- [cmake args...]
```

## Basic usage

```bash
# Build the current project
vix build

# Build with a detailed Vix summary
vix build -v

# Build with a specific number of jobs
vix build -j 8

# Build from another directory
vix build --dir ./examples/blog

# Build a release version
vix build --preset release
```

## What vix build does

When you run:

```bash
vix build
```

Vix performs the following steps:

1. Detect the project directory
2. Select a build preset
3. Prepare the build directory
4. Generate CMake configuration when needed
5. Build the main project target
6. Write configure and build logs
7. Store build metadata for faster future decisions

The normal output is intentionally compact:

```
  build [============================] done
  ✔ Built
  ✔ Done in 0.1s
```

With verbose output:

```bash
vix build -v
```

you get a clearer build summary:

```
Configuring project-name (dev)
  ✔ Configured in 0.5s

Compiling project-name (dev)
  * launcher: ccache | linker: mold | jobs: 8
  build [============================] done
  ✔ Finished dev [unoptimized + debuginfo] in 10.6s
```

## Build target behavior

By default, `vix build` builds the main target of the project.

The default target name is the project directory name.

For example, inside:

```
~/vixcpp/vix
```

this command:

```bash
vix build
```

builds:

```
vix
```

not:

```
all
```

This avoids rebuilding examples, tests, and auxiliary executables during normal development.

### Build the main target

```bash
vix build
```

### Build a specific target

```bash
vix build --build-target vix
vix build --build-target project
vix build --build-target my_app
```

### Build everything

```bash
vix build --build-target all
```

Use `--build-target all` before install or release workflows when CMake install rules require extra binaries to exist.

Example:

```bash
vix build --build-target all
sudo cmake --install build-ninja --prefix /usr/local
```

## Presets

Vix provides embedded presets.

| Preset | Generator | Build type | Build directory |
|--------|-----------|------------|-----------------|
| `dev` | Ninja | Debug | `build-dev` |
| `dev-ninja` | Ninja | Debug | `build-ninja` |
| `release` | Ninja | Release | `build-release` |

The default preset is `dev-ninja`.

Examples:

```bash
# Use the default development preset
vix build

# Use the dev preset
vix build --preset dev

# Use the release preset
vix build --preset release
```

### Development build

Use the default build for daily work:

```bash
vix build
```

This uses:

- preset: `dev-ninja`
- build type: `Debug`
- generator: `Ninja`
- build dir: `build-ninja`

### Release build

Use `--preset release` for optimized builds:

```bash
vix build --preset release
```

This uses:

- build type: `Release`
- build dir: `build-release`

You can combine it with other options:

```bash
vix build --preset release --with-sqlite
vix build --preset release --static
vix build --preset release --launcher sccache --linker mold
```

## Verbose output

Use `-v` or `--verbose` to show a clean summary of the configure and build phases:

```bash
vix build -v
```

This shows useful information such as:

```
Configuring project (dev)
Compiling project (dev)
launcher: ccache
linker: mold
jobs: 8
```

It does not flood the terminal with raw CMake or Ninja logs.

## Raw CMake and Ninja output

Use `--cmake-verbose` when you need the raw CMake or Ninja output:

```bash
vix build --cmake-verbose
```

Use this when debugging CMake itself, generator behavior, linker commands, or full build output.

Normal `-v` is for human-readable Vix output. `--cmake-verbose` is for raw low-level output.

## Quiet output

Use `--quiet` to reduce output:

```bash
vix build --quiet
```

This is useful in scripts when you only need the exit code.

## Build logs

`vix build` writes logs into the build directory.

Common log files:

```
build-ninja/configure.log
build-ninja/build.log
build-dev/configure.log
build-dev/build.log
build-release/configure.log
build-release/build.log
```

Use these logs when you need the full CMake, Ninja, compiler, or linker output.

Example:

```bash
cat build-ninja/build.log
```

## Build progress

During a build, Vix shows compact progress:

```
  build [============----------------] 20/45
  › Building CXX object CMakeFiles/project.dir/src/http/RequestContext.cpp.o
```

At the end, Vix keeps the output readable:

```
  build [============================] done
  ✔ Built
  ✔ Done in 10.1s
```

If the build fails, Vix hides raw compiler or linker noise and prints a focused diagnostic.

Example:

```
  ✖ Link failed

  message:
    Referenced by: BuildCommand.cpp

  error:
    undefined symbol: vix::cli::build::print_build_header_full(...)

  hint:
    The symbol is declared and used, but no linked object or library provides its definition.
```

The raw build output is still available in:

```
build-ninja/build.log
```

## Parallel builds

Use `-j` or `--jobs` to control parallelism:

```bash
vix build -j 8
vix build --jobs 16
```

If no job count is provided, Vix uses the machine CPU count, clamped internally to avoid excessive values.

## Compiler launcher

Vix can use sccache or ccache to speed up repeated builds.

```bash
vix build --launcher auto
vix build --launcher sccache
vix build --launcher ccache
vix build --launcher none
```

| Mode | Description |
|------|-------------|
| `auto` | Prefer sccache, then ccache when available |
| `sccache` | Use sccache if available |
| `ccache` | Use ccache if available |
| `none` | Disable compiler launcher |

Example:

```bash
vix build --launcher ccache
```

## Linker selection

Vix can use faster linkers when available.

```bash
vix build --linker auto
vix build --linker mold
vix build --linker lld
vix build --linker default
```

| Mode | Description |
|------|-------------|
| `auto` | Prefer mold, then lld when available |
| `mold` | Use mold |
| `lld` | Use lld |
| `default` | Use the system default linker |

Example:

```bash
vix build --linker mold
```

## SQLite support

Use `--with-sqlite` to enable SQLite-related build options:

```bash
vix build --with-sqlite
```

Release example:

```bash
vix build --preset release --with-sqlite
```

This maps to CMake options such as:

```
VIX_ENABLE_DB=ON
VIX_DB_USE_SQLITE=ON
VIX_DB_REQUIRE_SQLITE=ON
```

## MySQL support

Use `--with-mysql` to enable MySQL-related build options:

```bash
vix build --with-mysql
```

Release example:

```bash
vix build --preset release --with-mysql
```

This maps to CMake options such as:

```
VIX_ENABLE_DB=ON
VIX_DB_USE_MYSQL=ON
VIX_DB_REQUIRE_MYSQL=ON
```

## Static linking

Use `--static` to request static linking:

```bash
vix build --static
```

Release example:

```bash
vix build --preset release --static
```

This maps to:

```
VIX_LINK_STATIC=ON
```

Static linking depends on your platform and available static libraries.

## Clean build

Use `--clean` to remove local build directories and reconfigure from scratch:

```bash
vix build --clean
```

This removes local build directories such as:

```
build-dev
build-ninja
build-release
```

Then it configures and builds again.

Use this when:

- CMake cache is stale
- the build directory is broken
- toolchain settings changed
- you want a fresh rebuild

## Cache behavior

Vix uses build metadata and signatures to avoid unnecessary setup work.
Normal builds still let Ninja decide whether source files must be recompiled.
This is important because Ninja remains the source of truth for file-level incremental compilation.

Use `--no-cache` to disable Vix cache shortcuts:

```bash
vix build --no-cache
```

## Fast no-op builds

Use `--fast` to let Vix check whether Ninja reports the project as up to date:

```bash
vix build --fast
```

If Ninja reports no work, Vix can exit quickly.
Use this in tight loops or CI checks where a no-op build should be as fast as possible.
You can disable Ninja dry-run up-to-date detection with:

```bash
vix build --no-up-to-date
```

## Status output

Vix sets Ninja status output automatically when possible.

Disable it with:

```bash
vix build --no-status
```

Use this if you want less progress output or if your environment does not handle terminal progress well.

## Build heartbeat

In CI systems, long builds with no output may look stuck.
Enable heartbeat output with:

```bash
VIX_BUILD_HEARTBEAT=1 vix build
```

This prints a heartbeat when the build is silent for several seconds.

## Debug build details

Normal verbose output hides internal details such as graph state, artifact cache paths, build state paths, and CMake variables.

To show internal build details, use:

```bash
VIX_LOG_LEVEL=debug vix build -v
```

or:

```bash
VIX_LOG_LEVEL=trace vix build -v
```

This is useful when debugging Vix itself.

## Cross-compilation

Use `--target` to cross-compile:

```bash
vix build --target aarch64-linux-gnu
```

Release cross-build:

```bash
vix build --preset release --target aarch64-linux-gnu
```

With sysroot:

```bash
vix build --target aarch64-linux-gnu --sysroot /opt/sysroots/aarch64
```

Vix generates a CMake toolchain file in the build directory.
The expected compiler tools follow the target triple:

```
aarch64-linux-gnu-gcc
aarch64-linux-gnu-g++
aarch64-linux-gnu-ar
aarch64-linux-gnu-ranlib
aarch64-linux-gnu-strip
```

List detected cross toolchains:

```bash
vix build --targets
```

---

## Forward CMake arguments

Use `--` to pass raw arguments to CMake:

```bash
vix build -- -DVIX_SYNC_BUILD_TESTS=ON
```

Another example:

```bash
vix build --preset release -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Everything after `--` is forwarded to CMake configuration.

## Build from another directory

Use `--dir` or `-d`:

```bash
vix build --dir ./examples/blog
vix build -d ./examples/blog
```

This is useful when you are outside the project root.

## Export the built binary

Use `--bin` to export the built executable to the project root:

```bash
vix build --bin
```

Use `--out` to export it to a specific path:

```bash
vix build --out ./dist/my_app
```

`--bin` and `--out` cannot be used together.

## Single C++ file build

`vix build` can also build one C++ source file:

```bash
vix build main.cpp
```

This builds the file and exports the produced executable.

For running a single file directly, use:

```bash
vix run main.cpp
```

## Install workflow

Because `vix build` builds the main target by default, install workflows should build all required install targets first:

```bash
vix build --build-target all
sudo cmake --install build-ninja --prefix /usr/local
```

If you only run:

```bash
vix build
sudo cmake --install build-ninja --prefix /usr/local
```

CMake install may fail if an install rule expects a binary that was not built by the main target.

## Options

| Option | Description |
|--------|-------------|
| `--preset <name>` | Preset to use: `dev`, `dev-ninja`, or `release`. |
| `--target <triple>` | Cross-compilation target triple. |
| `--sysroot <path>` | Sysroot for cross toolchain. |
| `--static` | Request static linking. |
| `--with-sqlite` | Enable SQLite support. |
| `--with-mysql` | Enable MySQL support. |
| `-j, --jobs <n>` | Number of parallel build jobs. |
| `--clean` | Remove local build directories and reconfigure from scratch. |
| `--no-cache` | Disable Vix build cache shortcuts. |
| `--fast` | Exit quickly if Ninja says the build is up to date. |
| `--linker <mode>` | Linker mode: `auto`, `default`, `mold`, or `lld`. |
| `--launcher <mode>` | Compiler launcher mode: `auto`, `none`, `sccache`, or `ccache`. |
| `--no-status` | Disable Ninja status progress format. |
| `--no-up-to-date` | Disable Ninja dry-run up-to-date detection. |
| `-d, --dir <path>` | Project directory. |
| `-q, --quiet` | Minimal output. |
| `-v, --verbose` | Show detailed Vix configure and build summary. |
| `--targets` | List detected cross toolchains on PATH. |
| `--cmake-verbose` | Show raw CMake and Ninja output. |
| `--build-target <name>` | Build a specific CMake target. Default is the project directory name. |
| `--bin` | Export the built executable to the project root. |
| `--out <path>` | Export the built executable to a specific path. |
| `-h, --help` | Show command help. |

## Environment variables

| Variable | Description |
|----------|-------------|
| `VIX_BUILD_HEARTBEAT=1` | Enable heartbeat when the build is silent for several seconds. |
| `VIX_LOG_LEVEL=debug` | Show internal build graph, cache, state, and CMake variable details. |
| `VIX_LOG_LEVEL=trace` | Show deeper internal build details. |
| `VIX_GRAPH_EXECUTOR=1` | Enable the experimental target-aware graph executor. |

## Experimental graph executor

Vix contains an internal build graph foundation.

It can import:

- `compile_commands.json`
- `build.ninja`
- dependency files
- object metadata

It is used to prepare future target-aware incremental builds.
The experimental graph executor is not enabled by default.

Enable it with:

```bash
VIX_GRAPH_EXECUTOR=1 vix build -v
```

For normal projects, the stable default path remains CMake/Ninja execution.

## Common workflows

### Normal development build

```bash
vix build
```

### Verbose development build

```bash
vix build -v
```

### Release build

```bash
vix build --preset release
```

### Full repository build

```bash
vix build --build-target all
```

### Build one target

```bash
vix build --build-target project
```

### Clean rebuild

```bash
vix build --clean
```

### Fast no-op check

```bash
vix build --fast
```

### Use mold

```bash
vix build --linker mold
```

### Use ccache

```bash
vix build --launcher ccache
```

### Release with SQLite

```bash
vix build --preset release --with-sqlite
```

### Release with MySQL

```bash
vix build --preset release --with-mysql
```

### Build and install

```bash
vix build --build-target all
sudo cmake --install build-ninja --prefix /usr/local
```

## Common mistakes

### Expecting vix build to run the app

`vix build` only builds the project. It does not start the application.

Use:

```bash
vix run
```

or:

```bash
vix dev
```

### Forgetting to enter the project directory

Wrong:

```bash
vix new api
vix build
```

Correct:

```bash
vix new api
cd api
vix build
```

### Expecting vix build to build every target

By default, `vix build` builds the main project target.

Use this for all targets:

```bash
vix build --build-target all
```

### Installing after only building the main target

Wrong:

```bash
vix build
sudo cmake --install build-ninja --prefix /usr/local
```

Correct:

```bash
vix build --build-target all
sudo cmake --install build-ninja --prefix /usr/local
```

### Passing runtime arguments to vix build

`vix build` does not run the program.

Wrong:

```bash
vix build --port 8080
```

Correct:

```bash
vix run --run --port 8080
```

### Using -v when you need raw CMake logs

`-v` shows a readable Vix summary.

For raw CMake and Ninja logs, use:

```bash
vix build --cmake-verbose
```

### Expecting --clean to remove global caches

`--clean` removes local build directories.

For broader cleanup, use commands such as:

```bash
vix clean
vix reset
```

depending on what you want to remove.

## Troubleshooting

### Build says a target is missing

Use:

```bash
vix build --build-target all
```

or check available CMake targets:

```bash
cmake --build build-ninja --target help
```

### Install fails because a binary is missing

Build all install-related targets first:

```bash
vix build --build-target all
sudo cmake --install build-ninja --prefix /usr/local
```

### CMake cache is stale

Use:

```bash
vix build --clean
```

### You need the raw linker command

Use:

```bash
vix build --cmake-verbose
```

or inspect:

```bash
cat build-ninja/build.log
```

### You need internal Vix build details

Use:

```bash
VIX_LOG_LEVEL=debug vix build -v
```

## When to use vix build

Use `vix build` when you want to:

- compile a project
- verify that a project builds
- produce a release binary
- build a specific CMake target
- use a fast C++ development loop
- inspect build errors without running the app
- prepare an install or packaging workflow

Do not use `vix build` when you want to run the app. Use `vix run` or `vix dev` instead.

## Related commands

| Command | Purpose |
|---------|---------|
| `vix run` | Build and run the app |
| `vix dev` | Run the app with reload |
| `vix check` | Validate build, tests, runtime, and sanitizers |
| `vix tests` | Run tests |
| `vix clean` | Remove local project cache directories |
| `vix reset` | Clean and reinstall project dependencies |

## Next step

Continue with validation.

[Open the vix check guide](/cli/check)


