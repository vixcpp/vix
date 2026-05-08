# vix run

`vix run` builds and runs a Vix application, a single C++ file, or a `.vix` manifest.

Use it when you want one command that prepares a program and starts it immediately.

```bash
vix run
```

## Overview

`vix run` is the general execution command in Vix.

It can run projects, named targets, single C++ files, and `.vix` manifests.

It is useful when you want to start something manually without entering the continuous development loop of `vix dev`.

## Usage

```bash
vix run [name|file.cpp|manifest.vix] [options] [-- compiler/linker flags] [--run <args...>]
```

## Supported inputs

| Input | Mode | Example |
|-------|------|---------|
| No input | Project mode | `vix run` |
| Project or target name | Project mode | `vix run api` |
| Single `.cpp` file | Script mode | `vix run main.cpp` |
| `.vix` manifest file | Manifest mode | `vix run app.vix` |

## Basic examples

```bash
# Run the current project
vix run

# Run a named target or app
vix run api

# Run a single C++ file
vix run main.cpp

# Run a .vix manifest
vix run app.vix

# Run from a specific project directory
vix run --dir ./examples/blog
```

## What vix run does

When you run:

```bash
vix run
```

Vix performs the needed steps:

1. Detect the input type
2. Resolve the project, file, or manifest
3. Configure when needed
4. Build when needed
5. Start the executable
6. Stream the program output
7. Report build or runtime errors with Vix diagnostics

## Project mode

Project mode is used when Vix runs a CMake-based project.

```bash
vix run
```

Inside a project directory, Vix detects the project automatically.

```bash
cd api
vix run
```

You can also pass a named app or target:

```bash
vix run api
```

In project mode, Vix can configure the project, build the selected target, and run the resulting executable.

### Project directory

Use `--dir` when you want to run a project from another location.

```bash
vix run --dir ./examples/blog
vix run -d ./examples/blog
```

### Named target or app

```bash
vix run api
```

---

## Script mode

Script mode is used when the input is a single `.cpp` file.

```bash
vix run main.cpp
```

Vix treats the file like a runnable C++ script.
It can compile the file, cache the result, reuse the binary when possible, run the program, and show friendly diagnostics.

### Direct script compilation

For simple single-file programs, Vix can use a direct compile path, avoiding a full CMake project when it is not needed.

```bash
vix run main.cpp
```

### CMake fallback for scripts

Some scripts need more than direct compilation — for example, scripts that use Vix runtime features, special dependencies, database support, or sanitizer modes.
Vix handles this automatically. You still run the same command:

```bash
vix run server.cpp
```

Vix decides whether to use direct compilation or CMake fallback.

### Script cache

Vix caches script builds to make repeated runs faster.
The second run can reuse the cached result when nothing relevant changed.

### Local script cache

Use `--local-cache` when you want the script cache to be local to the current project or working directory.

```bash
vix run main.cpp --local-cache
```

## Manifest mode

Manifest mode is used when the input is a `.vix` file.

```bash
vix run app.vix
```

The manifest is loaded first, then CLI options are applied on top.
Depending on the manifest, Vix may behave like project mode or script mode.

## Runtime arguments

Runtime arguments are arguments passed to your program.

Use `--run` for script mode:

```bash
vix run main.cpp --run --port 8080
```

### Repeatable runtime arguments

```bash
vix run api --args --port --args 8080
vix run tool.cpp --args input.txt --args output.txt
```

### Important: `--` is not for runtime arguments in script mode

In script mode, `--` forwards flags to the compiler or linker.

Wrong:

```bash
vix run main.cpp -- --port 8080
```

Correct:

```bash
vix run main.cpp --run --port 8080
```

Use `--run` for runtime arguments. Use `--` for compiler or linker flags.

## Compiler and linker flags

In script mode, use `--` to pass compiler or linker flags.

```bash
# Optimization flags
vix run main.cpp -- -O2 -DNDEBUG

# Link with libraries
vix run main.cpp -- -lssl -lcrypto

# Add include paths
vix run main.cpp -- -I./include

# Add library paths
vix run main.cpp -- -L./lib -lmylib
```

### Compiler flags and runtime args together

```bash
vix run main.cpp -- -O2 -DNDEBUG --run hello 123
```

Here `-O2 -DNDEBUG` go to the compiler, and `hello 123` go to the program.

## Working directory

Use `--cwd` to run the program from a specific working directory.

```bash
vix run main.cpp --cwd ./data
vix run api --cwd ./runtime
```

## Environment variables

Use `--env` to pass environment variables to the running program.

```bash
vix run api --env APP_ENV=dev
vix run api --env APP_ENV=dev --env PORT=8080
vix run server.cpp --env PORT=8080
```

## Watch mode

Use `--watch` or `--reload` to rebuild and restart when files change.

```bash
vix run api --watch
vix run api --reload
vix run server.cpp --watch
```

For active development, prefer `vix dev`, which is optimized for the development loop.

## Server or script classification

Vix tries to decide whether the program is a long-running server or a short-lived script.
You can force the behavior.

### Force server mode

```bash
vix run server.cpp --force-server
vix run server.cpp --force-server --watch
```

### Force script mode

```bash
vix run tool.cpp --force-script
```

## Auto dependencies

Use `--auto-deps` to let Vix add include paths from local Vix dependencies.

```bash
vix run main.cpp --auto-deps
```

## Sanitizers

### AddressSanitizer + UBSan

```bash
vix run main.cpp --san
```

### UBSan only

```bash
vix run main.cpp --ubsan
```

### ThreadSanitizer

```bash
vix run main.cpp --tsan
```

## Database support

### SQLite support

```bash
vix run main.cpp --with-sqlite
```

### MySQL support

```bash
vix run main.cpp --with-mysql
```

## Presets

```bash
vix run api --preset dev-ninja
vix run api --preset dev-ninja --run-preset run-dev-ninja
```

## Parallel build jobs

```bash
vix run api -j 8
vix run api --jobs 16
```

## Terminal clearing

```bash
vix run api --clear=auto
vix run api --clear=always
vix run api --clear=never
vix run api --no-clear
```

`--no-clear` is equivalent to `--clear=never`.

## Automatic docs mode

```bash
vix run api --docs
vix run api --no-docs
```

You can also use the `VIX_DOCS` environment variable.

## Logging

```bash
vix run api --log-level debug
vix run api --verbose
vix run api --quiet
```

Supported levels: `trace`, `debug`, `info`, `warn`, `error`, `critical`

### Log format

```bash
vix run api --log-format kv
vix run api --log-format json
vix run api --log-format json-pretty
```

### Log color

```bash
vix run api --log-color auto
vix run api --log-color always
vix run api --log-color never
vix run api --no-color
```

## Replay recording

`vix run` can record executions for replay when replay support is enabled in the CLI.

Recorded runs can later be inspected with:

```bash
vix replay list
vix replay last
vix replay show <id>
vix replay failed
```

## Output behavior

`vix run` streams program output live.

For runtime crashes, compiler errors, linker errors, sanitizer reports, and common C++ mistakes, Vix tries to show a clearer diagnostic instead of raw noisy logs.

### Runtime errors

When a program fails at runtime, Vix can detect common issues such as: segmentation faults, abort, double free, invalid free, iterator invalidation, out-of-range access, uncaught exceptions, mutex errors, condition variable errors, thread errors, `string_view` lifetime issues, span access issues, and sanitizer reports.

### Build errors

When compilation fails, Vix formats the error clearly:

```
✖ Build failed

location:
  /path/to/file.cpp:line:column

error:
  message

code:
  line | source code
       | marker

hint:
  focused suggestion
```

## Project examples

```bash
# Run current project
vix run

# Run named target
vix run api

# Run with project directory
vix run --dir ./apps/api

# Run with arguments
vix run api --args --port --args 8080

# Run with environment variables
vix run api --env APP_ENV=dev --env PORT=8080

# Run with watch mode
vix run api --watch
```

## Script examples

```bash
# Run a file
vix run main.cpp

# Run with runtime arguments
vix run main.cpp --run input.txt output.txt

# Run with compiler flags
vix run main.cpp -- -O2 -DNDEBUG

# Run with include path
vix run main.cpp -- -I./include

# Run with libraries
vix run main.cpp -- -lssl -lcrypto

# Run with sanitizers
vix run main.cpp --san
vix run main.cpp --ubsan
vix run main.cpp --tsan

# Run with database support
vix run main.cpp --with-sqlite
vix run main.cpp --with-mysql

# Run with local cache
vix run main.cpp --local-cache

# Run with watch mode
vix run server.cpp --watch --force-server
```

## Manifest examples

```bash
# Run manifest
vix run app.vix

# Run manifest with environment
vix run app.vix --env APP_ENV=dev

# Run manifest with runtime args
vix run app.vix --args --port --args 8080
```

## Options

| Option | Description |
|--------|-------------|
| `-d, --dir <path>` | Project directory. Default is auto-detect. |
| `--preset <name>` | Configure preset. Default is usually `dev-ninja`. |
| `--run-preset <name>` | Build/run preset for target execution. |
| `-j, --jobs <n>` | Number of parallel build jobs. |
| `--clear <auto\|always\|never>` | Terminal clear behavior. |
| `--no-clear` | Alias for `--clear=never`. |
| `--cwd <path>` | Run the program with this working directory. |
| `--env <K=V>` | Add or override one environment variable. Repeatable. |
| `--args <value>` | Add one runtime argument. Repeatable. |
| `--run <args...>` | Runtime arguments for script mode. |
| `--watch` | Rebuild and restart on file changes. |
| `--reload` | Alias-style reload flag for watch behavior. |
| `--force-server` | Treat the program as a long-running server. |
| `--force-script` | Treat the program as a short-lived script. |
| `--auto-deps` | Add include paths from local Vix dependencies. |
| `--san` | Enable AddressSanitizer and UBSan where supported. |
| `--ubsan` | Enable UBSan only where supported. |
| `--tsan` | Enable ThreadSanitizer where supported. |
| `--with-sqlite` | Enable SQLite support for script mode. |
| `--with-mysql` | Enable MySQL support for script mode. |
| `--local-cache` | Use local script cache. |
| `--docs` | Enable automatic docs mode. |
| `--no-docs` | Disable automatic docs mode. |
| `--log-level <level>` | Set log level. |
| `--verbose` | Shortcut for debug logs. |
| `-q, --quiet` | Reduce output to warnings and errors. |
| `--log-format <format>` | Set log format. |
| `--log-color <mode>` | Control colored logs. |
| `--no-color` | Disable colored logs. |
| `-h, --help` | Show command help. |

## Environment variables

| Variable | Description |
|----------|-------------|
| `VIX_DOCS` | Enable or disable automatic docs mode. |
| `VIX_LOG_LEVEL` | Runtime log level. |
| `VIX_LOG_FORMAT` | Runtime log format. |
| `VIX_COLOR` | Color mode. |
| `NO_COLOR` | Disable colors. |
| `VIX_STDOUT_MODE` | Used by the CLI for smoother live output. |
| `VIX_CLI_CLEAR` | Terminal clear behavior. |
| `VIX_MODE` | May be set by Vix for runtime mode. |

## Common workflows

### Run the current project

```bash
vix run
```

### Run a named app

```bash
vix run api
```

### Run a project from another directory

```bash
vix run --dir ./apps/api
```

### Run a single C++ file

```bash
vix run main.cpp
```

### Run a server file

```bash
vix run server.cpp --force-server
```

### Run a CLI tool file

```bash
vix run tool.cpp --force-script
```

### Run with app arguments

Project:

```bash
vix run api --args --port --args 8080
```

Script:

```bash
vix run main.cpp --run --port 8080
```

### Run with compiler flags

```bash
vix run main.cpp -- -O2 -DNDEBUG
```

### Run with linked libraries

```bash
vix run main.cpp -- -lssl -lcrypto
```

### Run with watch mode

```bash
vix run api --watch
```

### Run with sanitizers

```bash
vix run main.cpp --san
```

### Run with debug logs

```bash
vix run api --log-level debug
VIX_LOG_LEVEL=debug vix run api
```

## Common mistakes

### Passing runtime args after `--` in script mode

Wrong:

```bash
vix run main.cpp -- --port 8080
```

Correct:

```bash
vix run main.cpp --run --port 8080
```

### Forgetting to enter the project directory

Wrong:

```bash
vix new api
vix run
```

Correct:

```bash
vix new api
cd api
vix run
```

### Using VIX_LOG_LEVEL=release

Wrong:

```bash
VIX_LOG_LEVEL=release vix run
```

Correct for release builds:

```bash
vix build --preset release
```

`VIX_LOG_LEVEL` controls logging, not build profile.

### Expecting vix run to behave like vix dev

`vix run` runs manually. For continuous reload during development, use:

```bash
vix dev
vix run --watch
```

### Passing Vix options after `--`

In script mode, arguments after `--` are treated as compiler or linker flags.

Wrong:

```bash
vix run main.cpp -- --verbose
```

Correct:

```bash
vix run main.cpp --verbose
```

## Troubleshooting

### The program does not receive arguments

```bash
vix run main.cpp --run arg1 arg2
vix run main.cpp --args arg1 --args arg2
```

### A compiler flag is treated as an app argument

```bash
vix run main.cpp -- -O2
```

### A runtime argument is treated as a compiler flag

```bash
vix run main.cpp --run --port 8080
```

### The app cannot find files

```bash
vix run api --cwd ./runtime
```

### Need to pass environment variables

```bash
vix run api --env APP_ENV=dev --env PORT=8080
```

### Need hot reload

```bash
vix run api --watch
vix dev
```

### Need more details

```bash
vix run api --verbose
VIX_LOG_LEVEL=debug vix run api
```

### Need less output

```bash
vix run api --quiet
```

## Best practices

Use `vix run` when you want to run something manually.
Use `vix dev` when actively editing code and you want automatic reload.
Use `vix build` when you only want to compile.
Use `vix check` when you want validation.
Use `vix replay` when you want to inspect previous runs.

## Difference between vix run, vix dev, and vix build

| Command | Purpose | Builds | Runs | Watches | Restarts |
|---------|---------|--------|------|---------|----------|
| `vix build` | Compile only | yes | no | no | no |
| `vix run` | Build and run manually | yes | yes | no by default | no by default |
| `vix run --watch` | Build, run, watch | yes | yes | yes | yes |
| `vix dev` | Development loop | yes | yes | yes | yes |

## Related commands

| Command | Purpose |
|---------|---------|
| `vix dev` | Run with development reload |
| `vix build` | Configure and compile |
| `vix check` | Validate build, tests, runtime, and sanitizers |
| `vix tests` | Run tests |
| `vix replay` | Inspect and replay previous Vix executions |
| `vix task` | Run reusable project tasks |

## Next step

Continue with development mode.

[Open the vix dev guide](/cli/dev)
