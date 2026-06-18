# vix build

`vix build` configures and builds a C++ project or a single C++ file with Vix.

Use it when you want to compile something without running it.

```bash
vix build
```

## Overview

`vix build` is the main build command in Vix.

It works with:

- CMake projects
- `vix.app` projects
- single C++ files
- named build targets
- cross-compilation targets
- cached builds
- artifact restoration
- object cache reuse
- target-aware graph execution
- warning checks
- build explanations
- exported binaries

The goal is not only to wrap CMake.

The goal is to give C++ a smarter build workflow:

```txt
fast when safe
correct by default
fallback when needed
```

Vix still uses CMake and Ninja for compatibility, but it adds build intelligence above them.

That includes:

- embedded presets
- target-aware builds
- fast no-op detection
- BuildState validation
- ArtifactCache
- ObjectCache
- BuildGraph execution when safe
- human-readable diagnostics
- ccache or sccache integration
- mold or lld integration
- CMake fallback when needed

## Usage

```bash
vix build [source.cpp] [options] -- [cmake args...]
```

## Basic examples

```bash
# Build the current project
vix build

# Build with detailed Vix output
vix build -v

# Build with a specific number of jobs
vix build -j 8

# Build from another directory
vix build --dir ./examples/blog

# Build a release version
vix build --preset release

# Build a specific target
vix build --build-target vix

# Build everything
vix build --build-target all

# Build a single C++ file
vix build main.cpp
```

## What `vix build` does

When you run:

```bash
vix build
```

Vix performs the needed steps:

1. Detect the project directory.
2. Resolve the project type.
3. Select a build preset.
4. Prepare the build directory.
5. Generate internal CMake for `vix.app` projects when needed.
6. Configure CMake when needed.
7. Import build metadata when available.
8. Check BuildState and cache state.
9. Restore artifacts when safe.
10. Use the target-aware graph executor when safe.
11. Fall back to CMake/Ninja when needed.
12. Write configure and build logs.
13. Store build metadata for future builds.

The normal output is intentionally compact:

```txt
Checking vix (dev)
  ✔ Up to date in 0.3s
```

or:

```txt
Compiling vix (dev)
  ✔ Configured
  ✔ Built
  ✔ Done in 1.6s
```

Use verbose mode for more details:

```bash
vix build -v
```

## Project types

`vix build` supports two project models:

```txt
CMakeLists.txt
vix.app
```

Project resolution order:

```txt
1. CMakeLists.txt
2. vix.app
```

If `CMakeLists.txt` exists, Vix uses it.

If no `CMakeLists.txt` exists but `vix.app` exists, Vix loads the `vix.app` manifest and generates an internal CMake project.

This keeps advanced CMake projects fully supported while giving simple Vix apps a cleaner manifest workflow.

## CMake project mode

For a CMake project:

```txt
myapp/
├── CMakeLists.txt
└── src/
    └── main.cpp
```

Run:

```bash
vix build
```

Vix uses the CMake compatibility path:

```txt
project resolution
-> build planning
-> CMake configure
-> CMake/Ninja build
```

Vix still improves the workflow with:

- presets
- cleaner output
- automatic build directories
- compiler launcher integration
- fast linker integration
- diagnostics
- logs
- target-aware shortcuts when safe

## vix.app project mode

For a `vix.app` project:

```txt
myapp/
├── vix.app
└── src/
    └── main.cpp
```

Run:

```bash
vix build
```

Vix loads the manifest and generates an internal CMake project under:

```txt
.vix/generated/app/
```

The flow is:

```txt
vix.app
-> parse manifest
-> validate manifest
-> generate internal CMake
-> configure
-> build
```

The user keeps a simple project manifest.

Vix keeps compatibility with the C++ build ecosystem.

## Single-file build mode

`vix build` can build a single C++ file:

```bash
vix build main.cpp
```

This compiles the file and produces an executable.

Examples:

```bash
vix build main.cpp
vix build main.cpp --bin
vix build main.cpp --out app
vix build main.cpp --with-sqlite --out app
vix build main.cpp --target x86_64-windows-gnu --out app.exe
```

For compiling and running a single file directly, use:

```bash
vix run main.cpp
```

Use `vix build main.cpp` when you only want the binary.

## Build target behavior

By default, `vix build` builds the main project target.

It does not build `all` by default.

This keeps normal development builds faster.

```bash
vix build
```

Build a specific target:

```bash
vix build --build-target vix
vix build --build-target api
vix build --build-target my_app
```

Build all targets:

```bash
vix build --build-target all
```

Use `all` when you need everything:

- examples
- tests
- libraries
- install targets
- generated utility targets

Example install workflow:

```bash
vix build --build-target all
sudo cmake --install build-ninja --prefix /usr/local
```

## Real target vs all target

Vix treats real targets and the `all` target differently.

```txt
real target -> graph target executor when safe
all target  -> CMake/Ninja path
```

A real target is usually an executable or library.

Examples:

```bash
vix build --build-target vix
vix build --build-target api
```

The `all` target can include many unrelated targets.

Examples:

```bash
vix build --build-target all
```

Because `all` is global and broad, Vix keeps it on the safer CMake/Ninja path.

## Presets

Vix provides embedded build presets.

| Preset      | Generator | Build type | Build directory |
| ----------- | --------- | ---------- | --------------- |
| `dev`       | Ninja     | Debug      | `build-dev`     |
| `dev-ninja` | Ninja     | Debug      | `build-ninja`   |
| `release`   | Ninja     | Release    | `build-release` |

The default preset is usually:

```txt
dev-ninja
```

Examples:

```bash
vix build
vix build --preset dev
vix build --preset dev-ninja
vix build --preset release
```

## Development build

Use the default build for daily work:

```bash
vix build
```

This normally uses:

```txt
preset: dev-ninja
build type: Debug
generator: Ninja
build dir: build-ninja
```

## Release build

Use release mode for optimized binaries:

```bash
vix build --preset release
```

Common release examples:

```bash
vix build --preset release
vix build --preset release --with-sqlite
vix build --preset release --with-mysql
vix build --preset release --static
vix build --preset release --launcher sccache --linker mold
```

## Build directories

Vix uses preset-specific build directories:

```txt
build-dev
build-ninja
build-release
```

For cross-compilation, the target triple is appended:

```txt
build-ninja-aarch64-linux-gnu
build-release-aarch64-linux-gnu
```

## Configure and build logs

`vix build` writes logs into the build directory.

Common logs:

```txt
build-dev/configure.log
build-dev/build.log
build-ninja/configure.log
build-ninja/build.log
build-release/configure.log
build-release/build.log
```

Use these logs when you need the full CMake, Ninja, compiler, or linker output.

Example:

```bash
cat build-ninja/build.log
```

## Verbose output

Use `-v` or `--verbose` to show detailed Vix output:

```bash
vix build -v
```

Verbose output can show:

```txt
project
preset
build directory
launcher
linker
jobs
configured state
build summary
cache behavior
```

It does not flood the terminal with raw CMake or Ninja logs.

## Raw CMake and Ninja output

Use `--cmake-verbose` when you need raw CMake or Ninja output:

```bash
vix build --cmake-verbose
```

Use this when debugging:

- CMake configuration
- generator behavior
- compiler commands
- linker commands
- low-level build output

Normal `-v` is for readable Vix output.

`--cmake-verbose` is for raw build system output.

## Quiet output

Use `-q` or `--quiet` to reduce output:

```bash
vix build --quiet
```

This is useful in scripts when you mostly care about the exit code.

## Parallel builds

Use `-j` or `--jobs` to control parallel build jobs:

```bash
vix build -j 8
vix build --jobs 16
vix build --jobs=8
```

If no job count is provided, Vix chooses a reasonable default based on the machine.

## Compiler launcher

Vix can use `sccache` or `ccache` to speed up repeated builds.

```bash
vix build --launcher auto
vix build --launcher sccache
vix build --launcher ccache
vix build --launcher none
```

| Mode      | Description                                     |
| --------- | ----------------------------------------------- |
| `auto`    | Prefer `sccache`, then `ccache` when available. |
| `sccache` | Use `sccache` if available.                     |
| `ccache`  | Use `ccache` if available.                      |
| `none`    | Disable compiler launcher.                      |

Examples:

```bash
vix build --launcher ccache
vix build --launcher=sccache
```

## Linker selection

Vix can use faster linkers when available.

```bash
vix build --linker auto
vix build --linker mold
vix build --linker lld
vix build --linker default
```

| Mode      | Description                               |
| --------- | ----------------------------------------- |
| `auto`    | Prefer `mold`, then `lld` when available. |
| `mold`    | Use `mold`.                               |
| `lld`     | Use `lld`.                                |
| `default` | Use the system default linker.            |

Examples:

```bash
vix build --linker mold
vix build --linker=lld
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

```txt
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

```txt
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

```txt
VIX_LINK_STATIC=ON
```

Static linking depends on your platform and available static libraries.

## Clean build

Use `--clean` to remove local build directories and configure again:

```bash
vix build --clean
```

This removes local build directories such as:

```txt
build-dev
build-ninja
build-release
```

Then Vix configures and builds again.

Use this when:

- CMake cache is stale
- the build directory is broken
- toolchain settings changed
- build flags changed in a confusing way
- you want a fresh local rebuild

`--clean` removes local build directories.

It does not remove global caches.

## Cache behavior

`vix build` uses several layers of build acceleration.

The important layers are:

```txt
BuildState
ArtifactCache
BuildGraph
ObjectCache
CMake/Ninja
```

The model is:

```txt
BuildState     -> fastest no-op validation
ArtifactCache  -> restore complete target output
BuildGraph     -> target-aware analysis
ObjectCache    -> restore compile outputs
CMake/Ninja    -> compatibility fallback
```

Vix is conservative.

A cache hit is only valid when Vix can prove the target identity still matches.

If Vix is unsure, it falls back.

## Disable cache shortcuts

Use `--no-cache` to disable Vix cache shortcuts:

```bash
vix build --no-cache
```

This keeps the build closer to the CMake/Ninja compatibility path.

## Fast no-op builds

Vix can detect clean builds quickly.

```bash
vix build --build-target vix
```

Expected no-op output:

```txt
Checking vix (dev)
  ✔ Up to date in 0.3s
```

`--fast` asks Vix to prefer the fast no-op path when possible:

```bash
vix build --fast
vix build --fast --build-target vix
```

Normal target builds can also be fast when Vix can prove the target is clean.

The rule is:

```txt
fast when safe
correct by default
fallback when needed
```

## BuildState validation

BuildState is the fastest no-op layer.

It can allow Vix to return quickly when the previous successful build still matches the current state.

A safe BuildState hit must validate things such as:

```txt
same build signature
same project fingerprint
same target
same preset
same build type
same compiler identity
same project inputs
last binary still exists
last binary is executable
artifact state is valid
```

A state hit is not enough by itself.

The output must still be valid.

If the final binary is missing, Vix must not say the target is up to date.

## ArtifactCache

ArtifactCache stores larger build outputs.

It can restore a final executable or library when the build identity matches.

Example clean rebuild flow:

```bash
vix build --build-target vix
rm -rf build-ninja
vix build --build-target vix
```

If the artifact exists and is valid, Vix can restore it instead of rebuilding.

The idea is:

```txt
same inputs
same build identity
same artifact
reuse safely
```

ArtifactCache is different from ObjectCache.

```txt
ObjectCache    -> restores .o and .d files
ArtifactCache  -> restores the final binary or library
```

## ObjectCache

ObjectCache works at the compile-task level.

It can reuse object files when the compile identity matches.

A compile identity includes:

```txt
source file content
header dependencies
compiler identity
compiler flags
include directories
defines
C++ standard
target triple
build type
toolchain
```

If any of those inputs change, the object should not be reused.

## BuildGraph

Vix can import build graph data from:

```txt
compile_commands.json
build.ninja
dependency files
object metadata
```

The BuildGraph helps Vix reason about:

```txt
sources
headers
objects
libraries
executables
compile tasks
link tasks
copy tasks
target closure
dirty state
```

This lets Vix avoid global rebuild assumptions when it can reason about the selected target.

## Graph target executor

The graph target executor is enabled by default when safe.

Disable it with:

```bash
VIX_GRAPH_EXECUTOR=0 vix build --build-target vix
```

It is not used for every build.

Vix avoids the graph executor when safety conditions are not met.

For example, it is not used when:

- graph executor is disabled
- cache is disabled
- `--clean` is used
- target is `all`
- cross-compilation is used
- raw CMake args are passed
- SQLite or MySQL build options are enabled
- static linking is requested
- compile commands are missing
- Ninja tasks are missing

In those cases, Vix falls back to CMake/Ninja.

## CMake/Ninja fallback

CMake and Ninja remain the compatibility path.

Vix falls back when that is safer.

This is important because CMake projects can include:

- generated sources
- custom commands
- custom targets
- install rules
- FetchContent
- toolchains
- package exports
- complex platform-specific logic

Vix should not guess when it cannot prove safety.

## Up-to-date detection

Vix can use Ninja dry-run behavior to detect whether a build is already up to date.

Disable it with:

```bash
vix build --no-up-to-date
```

Use this when debugging build behavior or when you want to avoid that shortcut.

## Status output

Vix sets Ninja status output automatically when possible.

Disable it with:

```bash
vix build --no-status
```

This can be useful in environments that do not handle terminal progress well.

## Build heartbeat

In CI systems, long builds with no output may look stuck.

Control heartbeat with:

```bash
VIX_BUILD_HEARTBEAT=1 vix build
```

Disable heartbeat with:

```bash
VIX_BUILD_HEARTBEAT=0 vix build
```

## Explain rebuilds

Use `--explain` to understand why Vix rebuilds something:

```bash
vix build --explain
vix build --explain --build-target vix
```

Example output:

```txt
Rebuilding BuildCommand.cpp
  reason: source file changed

Relinking vix
  reason: object file changed
```

If Vix cannot safely map a change to exact compile tasks, it should avoid lying.

It can delegate the target to Ninja instead of claiming no rebuild is required.

The principle is:

```txt
if Vix cannot prove the target is clean
it must not say the target is clean
```

## Warning listing

Use `--warnings` to show warnings from the last build log:

```bash
vix build --warnings
```

Paginate warnings:

```bash
vix build --warnings --page 2
vix build --warnings --limit 50
vix build --warnings --page 3 --limit 20
```

`--page` and `--limit` only work with `--warnings`.

Wrong:

```bash
vix build --page 2
```

Correct:

```bash
vix build --warnings --page 2
```

## Warning check

Use `--warning-check` to build with stronger compiler warnings enabled:

```bash
vix build --warning-check
```

A stricter check can be useful before release work:

```bash
vix build --warning-check --build-target all -v --clean
```

Vix can enable warning-related CMake variables and compiler warning flags for the build.

## Cross-compilation

Use `--target` to cross-compile:

```bash
vix build --target aarch64-linux-gnu
```

Release cross-build:

```bash
vix build --preset release --target aarch64-linux-gnu
```

Use a sysroot:

```bash
vix build --target aarch64-linux-gnu --sysroot /opt/sysroots/aarch64
```

Equivalent forms:

```bash
vix build --target=aarch64-linux-gnu
vix build --sysroot=/opt/sysroots/aarch64
```

Vix generates a CMake toolchain file in the build directory.

The expected compiler tools follow the target triple:

```txt
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

## Forward CMake arguments

Use `--` to pass raw arguments to CMake configuration:

```bash
vix build -- -DVIX_SYNC_BUILD_TESTS=ON
```

Release example:

```bash
vix build --preset release -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Everything after `--` is forwarded to CMake configure.

Raw CMake args can disable some fast Vix paths because Vix must stay conservative.

## Build from another directory

Use `--dir` or `-d` to build a project from another location:

```bash
vix build --dir ./examples/blog
vix build -d ./examples/blog
vix build --dir=./examples/blog
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
vix build --out=./dist/my_app
```

`--bin` and `--out` cannot be used together.

For single-file mode:

```bash
vix build main.cpp --bin
vix build main.cpp --out app
```

## Last binary metadata

When Vix exports or resolves a built binary, it can write metadata about the last binary.

This helps commands such as `vix run` find a recently built executable.

The metadata is stored under the Vix home directory.

## Build diagnostics

When compilation or linking fails, Vix tries to show a focused diagnostic instead of raw noisy logs.

Example shape:

```txt
✖ Link failed

message:
  Referenced by: BuildCommand.cpp

error:
  undefined symbol: vix::cli::build::print_build_header_full(...)

hint:
  The symbol is declared and used, but no linked object or library provides its definition.
```

The raw build output remains available in:

```txt
build-ninja/build.log
```

## Internal debug output

Use `VIX_LOG_LEVEL=debug` with `-v` to show deeper build details:

```bash
VIX_LOG_LEVEL=debug vix build -v
```

Use trace output for even deeper internal logs:

```bash
VIX_LOG_LEVEL=trace vix build -v
```

This is useful when debugging Vix itself.

It can expose details about:

- build graph state
- cache state
- artifact paths
- object cache decisions
- CMake variables
- command execution

## Options

| Option                  | Description                                                |
| ----------------------- | ---------------------------------------------------------- |
| `[source.cpp]`          | Build one C++ source file directly.                        |
| `-d, --dir <path>`      | Project directory.                                         |
| `--dir=<path>`          | Same as `--dir <path>`.                                    |
| `--preset <name>`       | Use a preset: `dev`, `dev-ninja`, or `release`.            |
| `--preset=<name>`       | Same as `--preset <name>`.                                 |
| `--build-target <name>` | Build a specific CMake target.                             |
| `--build-target=<name>` | Same as `--build-target <name>`.                           |
| `-j, --jobs <n>`        | Number of parallel build jobs.                             |
| `--jobs=<n>`            | Same as `--jobs <n>`.                                      |
| `--clean`               | Remove local build directories and configure again.        |
| `--fast`                | Use fast no-op detection when possible.                    |
| `--explain`             | Explain why files or targets rebuild.                      |
| `--warnings`            | Show warnings from the last build log.                     |
| `--warning-check`       | Build with strong compiler warnings enabled.               |
| `--page <n>`            | Warning page to display with `--warnings`. Default is `1`. |
| `--page=<n>`            | Same as `--page <n>`.                                      |
| `--limit <n>`           | Warnings per page with `--warnings`. Default is `10`.      |
| `--limit=<n>`           | Same as `--limit <n>`.                                     |
| `--no-cache`            | Disable Vix cache shortcuts.                               |
| `--no-status`           | Disable Ninja progress status.                             |
| `--no-up-to-date`       | Disable Ninja dry-run up-to-date detection.                |
| `--bin`                 | Export the built executable to the project root.           |
| `--out <path>`          | Export the built executable to a specific path.            |
| `--out=<path>`          | Same as `--out <path>`.                                    |
| `--launcher <mode>`     | Compiler launcher: `auto`, `none`, `sccache`, or `ccache`. |
| `--launcher=<mode>`     | Same as `--launcher <mode>`.                               |
| `--linker <mode>`       | Linker mode: `auto`, `default`, `mold`, or `lld`.          |
| `--linker=<mode>`       | Same as `--linker <mode>`.                                 |
| `--target <triple>`     | Cross-compilation target triple.                           |
| `--target=<triple>`     | Same as `--target <triple>`.                               |
| `--sysroot <path>`      | Sysroot for the cross toolchain.                           |
| `--sysroot=<path>`      | Same as `--sysroot <path>`.                                |
| `--targets`             | List detected cross toolchains on PATH.                    |
| `--static`              | Request static linking.                                    |
| `--with-sqlite`         | Enable SQLite support.                                     |
| `--with-mysql`          | Enable MySQL support.                                      |
| `-q, --quiet`           | Minimal output.                                            |
| `-v, --verbose`         | Show detailed build information.                           |
| `--cmake-verbose`       | Show raw CMake configure and build output.                 |
| `-h, --help`            | Show command help.                                         |
| `-- [cmake args...]`    | Pass extra arguments to CMake configure.                   |

## Environment variables

| Variable                | Description                                 |
| ----------------------- | ------------------------------------------- |
| `VIX_BUILD_HEARTBEAT=0` | Disable configure/build heartbeat.          |
| `VIX_BUILD_HEARTBEAT=1` | Force heartbeat when no output is produced. |
| `VIX_GRAPH_EXECUTOR=0`  | Disable graph target executor.              |
| `VIX_LOG_LEVEL=debug`   | Show deeper diagnostic output.              |
| `VIX_LOG_LEVEL=trace`   | Show trace-level diagnostic output.         |

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
vix build --build-target vix
```

### Clean rebuild

```bash
vix build --clean
```

### Fast no-op check

```bash
vix build --fast
```

### Explain rebuild decisions

```bash
vix build --explain --build-target vix
```

### Show warnings

```bash
vix build --warnings
```

### Run a strict warning check

```bash
vix build --warning-check --build-target all -v --clean
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

### Cross-compile

```bash
vix build --target aarch64-linux-gnu
```

### Export binary

```bash
vix build --out dist/app
```

### Build and install

```bash
vix build --build-target all
sudo cmake --install build-ninja --prefix /usr/local
```

### Disable graph executor

```bash
VIX_GRAPH_EXECUTOR=0 vix build --build-target vix
```

## Common mistakes

### Expecting `vix build` to run the app

`vix build` only builds.

It does not start the application.

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

### Expecting `vix build` to build every target

By default, `vix build` builds the main target.

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

### Passing runtime arguments to `vix build`

`vix build` does not run the program.

Wrong:

```bash
vix build --port 8080
```

Correct:

```bash
vix run --run --port 8080
```

### Using `-v` when you need raw CMake logs

`-v` shows a readable Vix summary.

For raw CMake and Ninja logs, use:

```bash
vix build --cmake-verbose
```

### Using `--page` without `--warnings`

Wrong:

```bash
vix build --page 2
```

Correct:

```bash
vix build --warnings --page 2
```

### Expecting `--clean` to remove global caches

`--clean` removes local build directories.

For broader cleanup, use:

```bash
vix clean
```

or:

```bash
vix reset
```

depending on what you want to remove.

## Troubleshooting

### Build says a target is missing

Build all targets:

```bash
vix build --build-target all
```

Or ask CMake for available targets:

```bash
cmake --build build-ninja --target help
```

### Install fails because a binary was not built

Build all targets before installing:

```bash
vix build --build-target all
sudo cmake --install build-ninja --prefix /usr/local
```

### Build is using stale configuration

Clean and rebuild:

```bash
vix build --clean
```

### Need raw build output

Use:

```bash
vix build --cmake-verbose
```

Or inspect:

```bash
cat build-ninja/build.log
```

### Need internal Vix build details

Use:

```bash
VIX_LOG_LEVEL=debug vix build -v
```

### Need to see why something rebuilt

Use:

```bash
vix build --explain
```

### Need to disable graph execution

Use:

```bash
VIX_GRAPH_EXECUTOR=0 vix build --build-target vix
```

### Need to disable cache shortcuts

Use:

```bash
vix build --no-cache
```

### Need less terminal progress

Use:

```bash
vix build --no-status
```

### Need CI heartbeat output

Use:

```bash
VIX_BUILD_HEARTBEAT=1 vix build
```

## Best practices

Use `vix build` when you only want to compile.

Use `vix run` when you want to build and run.

Use `vix dev` when you want a development loop.

Use `vix check` when you want validation.

Use `vix tests` when you want to run tests.

Use `vix build --build-target all` before install or release workflows.

Use `vix build --explain` when you want to understand rebuild decisions.

Use `vix build --warnings` to inspect warnings from previous builds.

Use `vix build --warning-check` before serious releases.

Use `VIX_GRAPH_EXECUTOR=0` only when you need to compare with the pure CMake/Ninja path.

## Difference between `vix build`, `vix run`, and `vix dev`

| Command           | Purpose                | Builds | Runs | Watches       | Restarts      |
| ----------------- | ---------------------- | ------ | ---- | ------------- | ------------- |
| `vix build`       | Compile only           | yes    | no   | no            | no            |
| `vix run`         | Build and run manually | yes    | yes  | no by default | no by default |
| `vix run --watch` | Build, run, watch      | yes    | yes  | yes           | yes           |
| `vix dev`         | Development loop       | yes    | yes  | yes           | yes           |

## Related commands

| Command     | Purpose                                                   |
| ----------- | --------------------------------------------------------- |
| `vix run`   | Build and run a project, file, binary, or runtime target. |
| `vix dev`   | Run with development reload.                              |
| `vix check` | Validate build, tests, runtime, and sanitizers.           |
| `vix tests` | Run tests.                                                |
| `vix clean` | Remove generated build artifacts.                         |
| `vix reset` | Reset generated Vix state.                                |

## Next step

Run the built application.

[Open the vix run guide](/cli/run)
