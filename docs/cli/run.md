# vix run

`vix run` builds and runs something with Vix.

It can run a Vix project, a single C++ file, a `.vix` manifest, a compiled binary, a container image, an SSH target, or an HTTP target.

Use it when you want one command that prepares a target and starts it immediately.

```bash
vix run
```

## Overview

`vix run` is the general execution command in Vix.

It is not limited to web applications.

It can run:

- the current project
- a named project or target
- a `vix.app` project
- a CMake project
- a single C++ file
- a `.vix` manifest
- a compiled binary
- a Docker image
- a container image
- an SSH target
- an HTTP or HTTPS target
- Vix umbrella examples

It is useful when you want to start something manually without entering the continuous development loop of `vix dev`.

For active development with automatic reload, use `vix dev` or `vix run --watch`.

## Usage

```bash
vix run [target] [options] [-- compiler/linker flags] [--run <args...>]
```

The target is optional.

If no target is provided, Vix tries to run the current project.

```bash
vix run
```

## Supported targets

| Target                           | Mode            | Example                       |
| -------------------------------- | --------------- | ----------------------------- |
| No target                        | Current project | `vix run`                     |
| Project directory or target name | Project mode    | `vix run api`                 |
| Single C++ file                  | Script mode     | `vix run main.cpp`            |
| `.vix` manifest                  | Manifest mode   | `vix run app.vix`             |
| Binary                           | Binary mode     | `vix run ./app`               |
| Docker image                     | Runtime target  | `vix run docker://nginx`      |
| Container image                  | Runtime target  | `vix run container://nginx`   |
| SSH target                       | Runtime target  | `vix run ssh://user@host`     |
| HTTP target                      | Runtime target  | `vix run https://example.com` |
| Vix umbrella example             | Example mode    | `vix run example main`        |

## Basic examples

```bash
# Run the current project
vix run

# Run a named project or target
vix run api

# Run a project from another directory
vix run --dir ./examples/blog

# Run a single C++ file
vix run main.cpp

# Run a .vix manifest
vix run app.vix

# Run a compiled binary
vix run ./app

# Run a Docker image
vix run docker://nginx -p 8080:80

# Run a container image
vix run container://nginx -p 8080:80

# Run a remote SSH command
vix run ssh://localhost echo hello

# Fetch an HTTP target
vix run https://example.com
```

## What `vix run` does

When you run:

```bash
vix run
```

Vix performs the needed steps:

1. Parse the command.
2. Detect the target type.
3. Resolve the project, file, manifest, binary, or runtime target.
4. Configure the project when needed.
5. Build the target when needed.
6. Resolve the executable when needed.
7. Apply runtime environment variables.
8. Start the target.
9. Stream output live.
10. Capture build or runtime failures.
11. Print Vix diagnostics when possible.

The goal is simple:

```txt
one command
correct target resolution
clear runtime behavior
better diagnostics when something fails
```

## Target resolution

`vix run` resolves targets in a practical order.

If a target is provided and starts with one of these prefixes, Vix treats it as a runtime target:

```txt
docker://
container://
ssh://
http://
https://
```

If a target is a file, Vix checks whether it is:

- a `.cpp` file
- an executable binary
- a `.vix` manifest

If a target is a directory, Vix treats it as a project directory.

If no target is provided, Vix tries to use:

```txt
.vix/meta.json last_binary
CMakeLists.txt
vix.app
local executable in the current folder
```

If none of these can be resolved, Vix reports that it cannot determine what to run.

## Project mode

Project mode is used when Vix runs a project.

A project can be based on:

```txt
CMakeLists.txt
vix.app
```

Run the current project:

```bash
vix run
```

Run a named target or project:

```bash
vix run api
```

Run from another directory:

```bash
vix run --dir ./apps/api
```

Project mode can:

- resolve the project root
- detect `CMakeLists.txt` or `vix.app`
- configure the project when needed
- build the selected target
- find the resulting executable
- run it with runtime arguments and environment variables

## CMake and vix.app project resolution

Vix respects this project resolution order:

```txt
1. CMakeLists.txt
2. vix.app
```

If both files exist, Vix uses `CMakeLists.txt`.

That keeps full CMake projects in control.

If no `CMakeLists.txt` exists but `vix.app` exists, Vix uses the `vix.app` project.

For `vix.app`, Vix can generate an internal CMake project and then build it.

The user still runs:

```bash
vix run
```

## Preset-based project runs

For projects with CMake presets, Vix can use the preset flow.

```bash
vix run api --preset dev-ninja
vix run api --preset release
```

You can also select a run preset:

```bash
vix run api --run-preset run-dev-ninja
```

## Clean project run

Use `--clean` when you want to clean or reconfigure before running.

```bash
vix run --clean
vix run api --clean
```

## Parallel build jobs

Use `-j` or `--jobs` to control build parallelism.

```bash
vix run api -j 8
vix run api --jobs 16
```

## Script mode

Script mode is used when the target is a single `.cpp` file.

```bash
vix run main.cpp
```

Vix treats the file as a runnable C++ script.

This is useful for:

- quick experiments
- small examples
- learning
- temporary tools
- small HTTP servers
- local scripts
- testing Vix APIs without creating a full project

## Direct script compilation

For simple scripts, Vix can use direct compilation.

```bash
vix run main.cpp
```

The idea is:

```txt
main.cpp
  -> compile
  -> link
  -> run
```

The user does not need to create:

```txt
CMakeLists.txt
vix.app
```

## CMake fallback for scripts

Some scripts need more than direct compilation, for example scripts that use Vix runtime features, special dependencies, database support, or sanitizer modes.

Vix handles this automatically. You still run the same command:

```bash
vix run server.cpp
```

Vix decides whether to use direct compilation or CMake fallback.

## Script cache

Vix can cache script builds.

If nothing relevant changed, repeated runs can reuse the previous result.

Use local script cache when you want the cache to stay local to the current working directory:

```bash
vix run main.cpp --local-cache
```

## Auto dependencies

Use `--auto-deps` to add include paths from installed local Vix dependencies.

```bash
vix run main.cpp --auto-deps
```

Equivalent local form:

```bash
vix run main.cpp --auto-deps=local
```

Search dependencies in parent folders too:

```bash
vix run main.cpp --auto-deps=up
```

This is useful when a script depends on packages installed under `.vix/deps`.

## Script compiler and linker flags

In script mode, everything after `--` is treated as compiler or linker flags.

```bash
vix run main.cpp -- -O2 -DNDEBUG
```

Add include paths:

```bash
vix run main.cpp -- -I./include
```

Add library paths:

```bash
vix run main.cpp -- -L./lib
```

Link libraries:

```bash
vix run main.cpp -- -lssl -lcrypto
```

Combine flags:

```bash
vix run main.cpp -- -O2 -I./include -L./lib -lmylib
```

## Runtime arguments

Runtime arguments are arguments passed to the program being executed.

For project mode, use repeated `--args`:

```bash
vix run api --args --port --args 8080
```

For script mode, prefer `--run`:

```bash
vix run main.cpp --run --port 8080
```

You can also use repeated `--args` in script mode:

```bash
vix run main.cpp --args input.txt --args output.txt
```

## Important script argument rule

In script mode, `--` is for compiler or linker flags.

Do not use it for runtime arguments.

Wrong:

```bash
vix run main.cpp -- --port 8080
```

Correct:

```bash
vix run main.cpp --run --port 8080
```

Or:

```bash
vix run main.cpp --args --port --args 8080
```

## Compiler flags and runtime args together

Use `--` for compiler or linker flags, then `--run` for program arguments.

```bash
vix run main.cpp -- -O2 -DNDEBUG --run hello 123
```

Here:

```txt
-O2 -DNDEBUG
```

go to the compiler.

```txt
hello 123
```

go to the program.

## Manifest mode

Manifest mode is used when the target is a `.vix` manifest.

```bash
vix run app.vix
```

The manifest is loaded first.

Then command-line options are merged on top.

Depending on the manifest, Vix may behave like script mode or project mode.

Examples:

```bash
vix run app.vix
vix run app.vix --env APP_ENV=dev
vix run app.vix --args --port --args 8080
```

## Binary mode

If the target is an executable binary, Vix can run it directly.

```bash
vix run ./app
```

You can pass runtime arguments:

```bash
vix run ./app --args --port --args 8080
```

You can pass environment variables:

```bash
vix run ./app --env APP_ENV=dev
```

You can set the working directory:

```bash
vix run ./app --cwd ./runtime
```

Vix still applies its runtime environment handling and diagnostics where possible.

## Docker and container targets

`vix run` can run container targets on supported Unix-like systems.

Docker image:

```bash
vix run docker://nginx
```

Container image:

```bash
vix run container://nginx
```

Pass Docker runtime arguments after the target:

```bash
vix run docker://nginx -p 8080:80
```

This maps to a Docker run flow similar to:

```bash
docker run -it --rm -p 8080:80 nginx
```

Container runtime targets are not supported on Windows yet.

## SSH targets

`vix run` can execute an SSH target.

```bash
vix run ssh://user@host
```

Pass a remote command:

```bash
vix run ssh://localhost echo hello
```

This maps to an SSH command similar to:

```bash
ssh localhost echo hello
```

Use this when you want Vix to dispatch a simple runtime target through SSH.

## HTTP and HTTPS targets

`vix run` can run an HTTP or HTTPS target.

```bash
vix run http://example.com
vix run https://example.com
```

On supported systems, this uses a `curl -L` style request.

You can pass additional arguments:

```bash
vix run https://example.com -I
```

This is useful for quick HTTP checks from the same runtime command surface.

## Vix umbrella examples

Inside the Vix umbrella repository, examples can be run with:

```bash
vix run example main
vix run example now_server
```

If the example binary exists in the build directory, Vix runs it.

If it does not exist, Vix reports that the example binary was not found and tells you to make sure the example is enabled and built.

## Library projects and test binaries

Some projects are libraries and do not produce a main executable.

When Vix cannot find a main executable, it can detect a single test binary in the build directory and run it.

This allows a library project to still have a useful `vix run` behavior when a single test binary is available.

## Working directory

Use `--cwd` to run the program from a specific working directory.

```bash
vix run api --cwd ./runtime
vix run main.cpp --cwd ./data
vix run ./app --cwd ./runtime
```

This is useful when your program expects files relative to a specific runtime folder.

## Environment variables

Use `--env` to add or override environment variables for the running program.

```bash
vix run api --env APP_ENV=dev
vix run api --env APP_ENV=dev --env PORT=8080
vix run server.cpp --env PORT=8080
```

Each `--env` accepts one `KEY=value` pair.

## Watch mode

Use `--watch` to rebuild and restart when files change.

```bash
vix run api --watch
vix run server.cpp --watch
```

`--reload` is an alias-style flag for watch behavior:

```bash
vix run api --reload
```

Force server behavior:

```bash
vix run server.cpp --force-server --watch
```

Force script behavior:

```bash
vix run tool.cpp --force-script
```

For active development, prefer:

```bash
vix dev
```

`vix dev` is optimized for the development loop.

## Server and script classification

Vix tries to decide whether a program is a long-running server or a short-lived script.

You can force the classification.

Treat the program as a server:

```bash
vix run server.cpp --force-server
```

Treat the program as a short-lived script:

```bash
vix run tool.cpp --force-script
```

This matters for watch mode, restart behavior, and runtime output handling.

## Sanitizers

Vix can run script builds with sanitizers.

Enable AddressSanitizer and UBSan:

```bash
vix run main.cpp --san
```

Enable UBSan only:

```bash
vix run main.cpp --ubsan
```

Enable ThreadSanitizer:

```bash
vix run main.cpp --tsan
```

Sanitizers are useful for detecting memory errors, undefined behavior, and threading issues.

## Database support in script mode

Enable SQLite support:

```bash
vix run main.cpp --with-sqlite
```

Enable MySQL support:

```bash
vix run main.cpp --with-mysql
```

These options are useful when a single-file script needs database-related Vix support.

## OpenAPI docs mode

`vix run` keeps OpenAPI/docs disabled by default.

```bash
vix run api
```

This sets:

```bash
VIX_DOCS=0
```

Enable docs for one run:

```bash
vix run api --docs
```

This sets:

```bash
VIX_DOCS=1
```

Disable docs explicitly:

```bash
vix run api --no-docs
```

You can also pass an explicit value:

```bash
vix run api --docs=true
vix run api --docs=false
vix run api --docs=1
vix run api --docs=0
```

The CLI option is preferred because it makes the current run explicit.

You can also use the environment variable:

```bash
VIX_DOCS=1 vix run api
VIX_DOCS=0 vix run api
```

## Replay recording

Use `--replay` to record a run under `.vix/runs/`.

```bash
vix run api --replay
vix run main.cpp --replay
vix run ./app --replay
```

Recorded runs can later be inspected or replayed with:

```bash
vix replay list
vix replay last
vix replay show <id>
vix replay failed
```

Use replay when you want to reproduce a failed, crashed, or unexpected execution without guessing the original command.

## Logging

Set the log level:

```bash
vix run api --log-level debug
```

Use verbose mode:

```bash
vix run api --verbose
```

Use quiet mode:

```bash
vix run api --quiet
```

Supported log levels:

```txt
trace
debug
info
warn
error
critical
off
```

`--verbose` is an alias for debug-style output.

`--quiet` reduces output to warnings and errors.

## Log format

Set the log format:

```bash
vix run api --log-format kv
vix run api --log-format json
vix run api --log-format json-pretty
```

## Log color

Control colored output:

```bash
vix run api --log-color auto
vix run api --log-color always
vix run api --log-color never
```

Disable colors:

```bash
vix run api --no-color
```

`--no-color` is an alias for:

```bash
--log-color=never
```

## Terminal clearing

Control terminal clearing:

```bash
vix run api --clear=auto
vix run api --clear=always
vix run api --clear=never
```

Disable clearing:

```bash
vix run api --no-clear
```

`--no-clear` is equivalent to:

```bash
--clear=never
```

## Output behavior

`vix run` streams program output live.

When possible, it also improves errors from:

- compiler failures
- linker failures
- CMake configure failures
- runtime crashes
- sanitizer reports
- uncaught exceptions
- memory errors
- thread errors
- known C++ runtime mistakes

Instead of showing only raw logs, Vix tries to show:

```txt
what failed
where it failed
the relevant code frame
a focused hint
the original output when needed
```

## Runtime diagnostics

When a program fails at runtime, Vix can detect common failure families such as:

- segmentation faults
- aborts
- uncaught exceptions
- double free
- invalid free
- use after free
- buffer overflow
- null pointer errors
- out-of-range access
- iterator invalidation
- mutex errors
- condition variable errors
- thread lifecycle errors
- `string_view` lifetime issues
- span access issues
- sanitizer reports

The goal is not to hide C++.

The goal is to make C++ failures easier to understand.

## Build diagnostics

When compilation fails, Vix tries to format the error clearly.

Example shape:

```txt
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

If Vix cannot classify the error, it still shows the raw compiler or build output.

## Environment variables

| Variable            | Description                                                                   |
| ------------------- | ----------------------------------------------------------------------------- |
| `VIX_DOCS`          | Enables or disables OpenAPI/docs mode.                                        |
| `VIX_LOG_LEVEL`     | Runtime log level.                                                            |
| `VIX_LOG_FORMAT`    | Runtime log format.                                                           |
| `VIX_COLOR`         | Color mode.                                                                   |
| `NO_COLOR`          | Disables colors.                                                              |
| `VIX_STDOUT_MODE`   | Used by Vix for smoother live output.                                         |
| `VIX_CLI_CLEAR`     | Terminal clear behavior.                                                      |
| `VIX_MODE`          | Runtime mode set by Vix when not already set.                                 |
| `VIX_SHOW_ENV_HINT` | When set to `1`, shows a hint if `.env.example` exists but `.env` is missing. |
| `VIX_RUN_UI`        | Enables the run progress UI when set.                                         |

## Options

| Option                          | Description                                                |
| ------------------------------- | ---------------------------------------------------------- |
| `-d, --dir <path>`              | Project directory.                                         |
| `--dir=<path>`                  | Same as `--dir <path>`.                                    |
| `--preset <name>`               | Configure or build preset. Default is usually `dev-ninja`. |
| `--preset=<name>`               | Same as `--preset <name>`.                                 |
| `--run-preset <name>`           | Run preset name.                                           |
| `--run-preset=<name>`           | Same as `--run-preset <name>`.                             |
| `-j, --jobs <n>`                | Number of parallel build jobs.                             |
| `--jobs=<n>`                    | Same as `--jobs <n>`.                                      |
| `--clean`                       | Clean or reconfigure before running.                       |
| `--replay`                      | Record this run under `.vix/runs/`.                        |
| `--cwd <path>`                  | Run the program from this working directory.               |
| `--cwd=<path>`                  | Same as `--cwd <path>`.                                    |
| `--env <K=V>`                   | Add or override one environment variable. Repeatable.      |
| `--env=<K=V>`                   | Same as `--env <K=V>`.                                     |
| `--args <value>`                | Add one runtime argument. Repeatable.                      |
| `--args=<value>`                | Same as `--args <value>`.                                  |
| `--run <args...>`               | Runtime arguments for script mode.                         |
| `--watch`                       | Rebuild and restart on file changes.                       |
| `--reload`                      | Alias-style flag for watch behavior.                       |
| `--force-server`                | Treat the program as a long-running server.                |
| `--force-script`                | Treat the program as a short-lived script.                 |
| `--auto-deps`                   | Auto-add includes from `.vix/deps/*/include`.              |
| `--auto-deps=local`             | Same as `--auto-deps`.                                     |
| `--auto-deps=up`                | Search dependencies in parent folders too.                 |
| `--san`                         | Enable AddressSanitizer and UBSan where supported.         |
| `--ubsan`                       | Enable UBSan only where supported.                         |
| `--tsan`                        | Enable ThreadSanitizer where supported.                    |
| `--with-sqlite`                 | Enable SQLite support for script mode.                     |
| `--with-mysql`                  | Enable MySQL support for script mode.                      |
| `--local-cache`                 | Use local script cache.                                    |
| `--docs`                        | Enable OpenAPI/docs for this run.                          |
| `--no-docs`                     | Disable OpenAPI/docs for this run.                         |
| `--docs=<0\|1\|true\|false>`    | Explicitly control OpenAPI/docs mode.                      |
| `--clear <auto\|always\|never>` | Clear terminal before runtime output.                      |
| `--clear=<auto\|always\|never>` | Same as `--clear <mode>`.                                  |
| `--no-clear`                    | Alias for `--clear=never`.                                 |
| `--log-level <level>`           | Set log level.                                             |
| `--log-level=<level>`           | Same as `--log-level <level>`.                             |
| `--verbose`                     | Alias for debug logging.                                   |
| `-q, --quiet`                   | Reduce output to warnings and errors.                      |
| `--log-format <format>`         | Set log format: `kv`, `json`, or `json-pretty`.            |
| `--log-format=<format>`         | Same as `--log-format <format>`.                           |
| `--log-color <mode>`            | Set color mode: `auto`, `always`, or `never`.              |
| `--log-color=<mode>`            | Same as `--log-color <mode>`.                              |
| `--no-color`                    | Alias for `--log-color=never`.                             |
| `-h, --help`                    | Show command help.                                         |
| `-- <flags...>`                 | In script mode, pass compiler or linker flags.             |

## Project examples

```bash
# Run current project
vix run

# Run named target
vix run api

# Run with project directory
vix run --dir ./apps/api

# Run with preset
vix run api --preset release

# Run with clean configure/build
vix run api --clean

# Run with parallel jobs
vix run api -j 8

# Run with runtime arguments
vix run api --args --port --args 8080

# Run with environment variables
vix run api --env APP_ENV=dev --env PORT=8080

# Run from a specific working directory
vix run api --cwd ./runtime

# Run with watch mode
vix run api --watch

# Record the run
vix run api --replay
```

## Script examples

```bash
# Run a single C++ file
vix run main.cpp

# Run with runtime arguments
vix run main.cpp --run input.txt output.txt

# Run with repeatable runtime arguments
vix run main.cpp --args input.txt --args output.txt

# Run with compiler flags
vix run main.cpp -- -O2 -DNDEBUG

# Run with include path
vix run main.cpp -- -I./include

# Run with linked libraries
vix run main.cpp -- -lssl -lcrypto

# Run with auto dependencies
vix run main.cpp --auto-deps

# Run with local cache
vix run main.cpp --local-cache

# Run with sanitizers
vix run main.cpp --san
vix run main.cpp --ubsan
vix run main.cpp --tsan

# Run with database support
vix run main.cpp --with-sqlite
vix run main.cpp --with-mysql

# Run as a server
vix run server.cpp --force-server

# Run as a server with watch mode
vix run server.cpp --force-server --watch

# Run as a short-lived script
vix run tool.cpp --force-script
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

## Binary examples

```bash
# Run a binary
vix run ./app

# Run a binary with arguments
vix run ./app --args --port --args 8080

# Run a binary with environment
vix run ./app --env APP_ENV=dev
```

## Runtime target examples

```bash
# Run Docker image
vix run docker://nginx

# Run Docker image with port mapping
vix run docker://nginx -p 8080:80

# Run container image
vix run container://nginx

# Run SSH target
vix run ssh://localhost echo hello

# Run HTTP target
vix run http://example.com

# Run HTTPS target
vix run https://example.com
```

## Common workflows

### Run the current project

```bash
vix run
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

### Run with OpenAPI docs enabled

```bash
vix run api --docs
```

By default, OpenAPI/docs are disabled:

```bash
vix run api
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

### Using `VIX_LOG_LEVEL=release`

Wrong:

```bash
VIX_LOG_LEVEL=release vix run
```

Correct for release builds:

```bash
vix run --preset release
```

`VIX_LOG_LEVEL` controls logging, not the build profile.

### Expecting `vix run` to behave exactly like `vix dev`

`vix run` runs manually.

For continuous reload during development, use:

```bash
vix dev
```

Or:

```bash
vix run --watch
```

### Passing script compiler flags as runtime args

Wrong:

```bash
vix run main.cpp --args -O2
```

Correct:

```bash
vix run main.cpp -- -O2
```

### Passing script runtime args as compiler flags

Wrong:

```bash
vix run main.cpp -- --name Gaspard
```

Correct:

```bash
vix run main.cpp --run --name Gaspard
```

## Troubleshooting

### The program does not receive arguments

For script mode:

```bash
vix run main.cpp --run arg1 arg2
```

Or:

```bash
vix run main.cpp --args arg1 --args arg2
```

For project mode:

```bash
vix run api --args arg1 --args arg2
```

### A compiler flag is treated as an app argument

Use `--` in script mode:

```bash
vix run main.cpp -- -O2
```

### A runtime argument is treated as a compiler flag

Use `--run` in script mode:

```bash
vix run main.cpp --run --port 8080
```

### The app cannot find files

Set the runtime working directory:

```bash
vix run api --cwd ./runtime
```

### Need environment variables

```bash
vix run api --env APP_ENV=dev --env PORT=8080
```

### Need hot reload

```bash
vix run api --watch
```

Or:

```bash
vix dev
```

### Need more details

```bash
vix run api --verbose
```

Or:

```bash
VIX_LOG_LEVEL=debug vix run api
```

### Need less output

```bash
vix run api --quiet
```

### `.env` is missing

If `.env.example` exists and you want Vix to show a hint when `.env` is missing, enable:

```bash
VIX_SHOW_ENV_HINT=1 vix run
```

Then create your local environment file:

```bash
cp .env.example .env
```

## Best practices

Use `vix run` when you want to build and run something manually.

Use `vix dev` when you are actively editing code and want automatic reload.

Use `vix build` when you only want to compile.

Use `vix check` when you want validation.

Use `vix tests` when you want to run tests.

Use `vix replay` when you want to inspect or reproduce recorded runs.

Use `--run` for script runtime arguments.

Use `--` for script compiler and linker flags.

Use `--env` for run-specific environment variables.

Use `--cwd` when your program depends on runtime files.

Use `--replay` when the run may need to be reproduced later.

## Difference between `vix run`, `vix dev`, and `vix build`

| Command           | Purpose                | Builds | Runs | Watches       | Restarts      |
| ----------------- | ---------------------- | ------ | ---- | ------------- | ------------- |
| `vix build`       | Compile only           | yes    | no   | no            | no            |
| `vix run`         | Build and run manually | yes    | yes  | no by default | no by default |
| `vix run --watch` | Build, run, watch      | yes    | yes  | yes           | yes           |
| `vix dev`         | Development loop       | yes    | yes  | yes           | yes           |

## Related commands

| Command      | Purpose                                         |
| ------------ | ----------------------------------------------- |
| `vix dev`    | Run with development reload.                    |
| `vix build`  | Configure and compile.                          |
| `vix check`  | Validate build, tests, runtime, and sanitizers. |
| `vix tests`  | Run tests.                                      |
| `vix replay` | Inspect and replay previous Vix executions.     |
| `vix task`   | Run reusable project tasks.                     |

## Next step

Continue with development mode.

[Open the vix dev guide](/cli/dev)
