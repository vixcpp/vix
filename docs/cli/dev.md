# vix dev

`vix dev` starts a Vix target in development mode.

It builds, runs, watches, rebuilds, and restarts automatically while you edit code.

Use it during active development.

```bash
vix dev
```

## Overview

`vix dev` is the development entrypoint for Vix.

It is designed for a fast feedback loop:

```txt
edit
save
detect change
rebuild
restart
continue
```

It works with:

- the current project
- a project directory or target name
- a `vix.app` project
- a CMake project
- a single C++ file
- a `.vix` manifest
- Vue fullstack projects
- long-running servers
- short-lived scripts
- apps that need runtime arguments
- apps that need environment variables
- apps that need SQLite, MySQL, or sanitizer modes

Internally, `vix dev` is development mode on top of `vix run`.

It automatically enables watch mode and dev mode.

Conceptually:

```txt
vix dev
  -> vix run --watch --dev-mode
```

## Usage

```bash
vix dev [target] [options] [-- compiler/linker flags] [--run <args...>]
```

## Basic examples

```bash
# Start the current project in dev mode
vix dev

# Start a named project or target
vix dev api

# Start a project from another directory
vix dev --dir ./examples/blog

# Start a single C++ file in dev mode
vix dev server.cpp

# Start a .vix manifest in dev mode
vix dev app.vix

# Pass runtime arguments to a script
vix dev server.cpp --run --port 8080

# Pass runtime arguments with repeatable args
vix dev api --args --port --args 8080

# Pass compiler flags to a script
vix dev server.cpp -- -O2 -DNDEBUG
```

## What `vix dev` does

When you run:

```bash
vix dev
```

Vix performs these steps:

1. Resolves the target.
2. Configures the project when needed.
3. Builds the target.
4. Finds the executable when possible.
5. Starts the app.
6. Watches relevant files.
7. Classifies file changes.
8. Rebuilds or reconfigures when needed.
9. Restarts the app automatically.
10. Stops cleanly on `Ctrl+C`.

The goal is simple:

```txt
keep the app running while you edit
rebuild only when relevant files change
restart safely when the app needs to restart
```

## Difference between `vix dev` and `vix run --watch`

`vix dev` is close to:

```bash
vix run --watch
```

But `vix dev` is the dedicated development command.

It automatically adds:

```txt
--watch
--dev-mode
```

Use `vix dev` when you are editing.

Use `vix run --watch` when you want manual control from the run command.

## Supported targets

| Target                           | Mode            | Example              |
| -------------------------------- | --------------- | -------------------- |
| No target                        | Current project | `vix dev`            |
| Project directory or target name | Project mode    | `vix dev api`        |
| Single C++ file                  | Script mode     | `vix dev server.cpp` |
| `.vix` manifest                  | Manifest mode   | `vix dev app.vix`    |

`vix dev` is focused on development sessions.

For runtime targets such as Docker, SSH, HTTP, or direct binary execution, prefer `vix run`.

## Project mode

Project mode is used when Vix runs a project.

A project can be based on:

```txt
CMakeLists.txt
vix.app
```

Run the current project:

```bash
vix dev
```

Run a named project or target:

```bash
vix dev api
```

Run from another directory:

```bash
vix dev --dir ./apps/api
```

Project mode can:

- resolve the project root
- configure when needed
- build the target
- run the app
- watch source and config files
- rebuild on source changes
- reconfigure on project config changes
- restart the app

## CMake and vix.app project resolution

Vix respects this project resolution order:

```txt
1. CMakeLists.txt
2. vix.app
```

If `CMakeLists.txt` exists, Vix uses the CMake project.

If no `CMakeLists.txt` exists but `vix.app` exists, Vix uses the `vix.app` project.

For `vix.app`, Vix can generate an internal CMake project and then run the development session.

## Script mode

Script mode is used when the target is a single C++ file.

```bash
vix dev server.cpp
```

This is useful for:

- prototypes
- demos
- quick servers
- small tools
- learning
- experiments
- testing Vix APIs without creating a full project

Force server behavior:

```bash
vix dev server.cpp --force-server
```

Force short-lived script behavior:

```bash
vix dev tool.cpp --force-script
```

## Manifest mode

Manifest mode is used when the target is a `.vix` file.

```bash
vix dev app.vix
```

The manifest is loaded first.

Then CLI options are applied on top.

Examples:

```bash
vix dev app.vix
vix dev app.vix --env APP_ENV=dev
vix dev app.vix --args --port --args 8080
```

## Watch mode

Watch mode is enabled by default in `vix dev`.

These are equivalent:

```bash
vix dev
vix dev --watch
vix dev --reload
```

When a relevant file changes, Vix:

1. waits for a small debounce delay
2. classifies the change
3. stops the running app
4. rebuilds or reconfigures
5. restarts the app

## File change classification

`vix dev` classifies changes into three kinds:

```txt
ignore
rebuild-only
reconfigure-and-rebuild
```

## Ignored changes

Some files and directories are ignored because they should not trigger a rebuild.

Ignored directories include:

```txt
.git
.vix
build
build-dev
build-ninja
build-release
node_modules
.cache
.idea
.vscode
docs
doc
dist
out
coverage
```

Ignored files include common non-build files such as:

```txt
README.md
CHANGELOG.md
LICENSE
.gitignore
```

Editing these files does not reload the app.

## Rebuild-only changes

Source and header changes trigger a rebuild and restart.

Source extensions:

```txt
.cpp
.cc
.cxx
.c
```

Header extensions:

```txt
.hpp
.hh
.hxx
.h
.ipp
.inl
```

Example:

```txt
changed: /home/user/api/src/main.cpp
```

Vix rebuilds and restarts the app.

## Reconfigure and rebuild changes

Project configuration changes trigger reconfigure and rebuild.

Config files include:

```txt
CMakeLists.txt
CMakePresets.json
vix.json
vix.toml
vix.lock
*.cmake
```

When one of these changes, Vix treats it as a configuration change.

This matters because targets, dependencies, build options, presets, or project layout may have changed.

## File index

`vix dev` uses a file index to detect changes.

The index tracks:

```txt
path
mtime
file size
change kind
```

This avoids unnecessary rebuilds and keeps watch mode fast.

## Debounce behavior

When a file changes, Vix waits briefly before rebuilding.

This avoids rebuilding multiple times while an editor is still writing files.

After the debounce delay, Vix selects the most relevant change.

If one change requires reconfigure and rebuild, that change wins over normal rebuild-only changes.

## Build behavior

`vix dev` uses the normal Vix build pipeline.

During dev rebuilds, Vix runs a fast build command internally.

The development rebuild command is based on:

```txt
vix build --build-target all --fast
```

with options such as:

```txt
-j
-v
--with-sqlite
--with-mysql
```

when they are provided.

This means dev mode can benefit from Vix build improvements such as fast no-op checks, cache behavior, and build diagnostics.

## Configure behavior

If the build directory has no CMake cache, Vix configures the project.

The configure step uses:

```txt
CMake
Ninja
Debug build type
compile_commands.json
```

When SQLite, MySQL, or sanitizers are enabled, Vix adds the relevant configuration flags.

If configure fails, Vix prints the error and waits for the next file change.

## Build directory

Project dev mode uses the development build directory.

Common build directory:

```txt
build-ninja
```

Example:

```txt
/home/user/api/build-ninja
```

## Target behavior

`vix dev` runs the resolved project target.

For a project named:

```txt
api
```

Vix expects a runnable target named:

```txt
api
```

When the executable exists, Vix starts it.

When no executable is found, Vix treats the project like a library project.

## Library projects

Some projects are libraries and do not produce a runnable executable.

If Vix builds successfully but cannot find a runnable executable, it prints a library-oriented message and keeps watching.

Example behavior:

```txt
Library built.
No runnable executable found. Watching for changes.
Use `vix tests` to run the test suite.
```

Use `vix tests` to run tests for library projects.

## Executable detection

After building, Vix looks for the executable in common locations:

```txt
build-ninja/<target>
build-ninja/bin/<target>
build-ninja/src/<target>
```

It can also scan the build directory recursively.

Test binaries are ignored when resolving the main app executable.

## Runtime process

When the executable is found, Vix starts it as a child process.

The child process receives:

```txt
VIX_MODE=dev
VIX_STDOUT_MODE=line
```

If SQLite mode is enabled, Vix sets:

```txt
VIX_DB_ENGINE=sqlite
VIX_ENABLE_DB=1
VIX_DB_USE_SQLITE=1
```

If MySQL mode is enabled, Vix sets:

```txt
VIX_DB_ENGINE=mysql
VIX_ENABLE_DB=1
VIX_DB_USE_MYSQL=1
```

Sanitizer environment is also applied when sanitizer options are enabled.

## Restart behavior

When a relevant file changes, Vix restarts the app.

The restart flow is:

```txt
detect change
debounce
print changed file
send SIGINT
wait briefly
send SIGTERM if needed
send SIGKILL if needed
rebuild
start new process
```

This makes restarts safer than killing immediately.

## Stopping dev mode

Press:

```txt
Ctrl+C
```

Vix handles `SIGINT` and `SIGTERM`.

It stops the dev session and terminates the child process.

Example output:

```txt
Stopping dev session...
```

## Vue fullstack projects

`vix dev` can detect Vue fullstack projects.

A Vue frontend is detected when:

```txt
vix.json contains "template"
vix.json contains "vue"
vix.json contains "frontend"
frontend/package.json exists
```

When detected, Vix starts the Vue dev server with:

```bash
npm run dev
```

from:

```txt
frontend/
```

It also sets:

```txt
VIX_FRONTEND=vue
```

When the dev session stops, Vix stops the Vue frontend process too.

This lets one `vix dev` session run both:

```txt
Vix C++ backend
Vue frontend
```

## Runtime arguments

Use `--run` for script runtime arguments:

```bash
vix dev server.cpp --run --port 8080
```

Use repeatable `--args` for runtime arguments:

```bash
vix dev api --args --port --args 8080
vix dev server.cpp --args --port --args 8080
```

## Important argument rule

In script mode, `--` is for compiler or linker flags.

Do not use it for runtime arguments.

Wrong:

```bash
vix dev server.cpp -- --port 8080
```

Correct:

```bash
vix dev server.cpp --run --port 8080
```

Or:

```bash
vix dev server.cpp --args --port --args 8080
```

## Compiler and linker flags

In script mode, everything after `--` is treated as compiler or linker flags.

```bash
vix dev server.cpp -- -O2 -DNDEBUG
```

Add include paths:

```bash
vix dev server.cpp -- -I./include
```

Link libraries:

```bash
vix dev server.cpp -- -lssl -lcrypto
```

## Working directory

Use `--cwd` to run the application from a specific working directory.

```bash
vix dev api --cwd ./runtime
vix dev server.cpp --cwd ./data
```

If no `--cwd` is provided, the child process runs from the project directory.

## Environment variables

Use `--env` to add or override environment variables for the running app.

```bash
vix dev api --env APP_ENV=dev
vix dev api --env APP_ENV=dev --env PORT=8080
vix dev server.cpp --env PORT=8080
```

Each `--env` accepts one `KEY=value` pair.

## Force server mode

Use `--force-server` when the target is a long-running app such as an HTTP server or WebSocket server.

```bash
vix dev server.cpp --force-server
```

This is useful for single-file servers.

## Force script mode

Use `--force-script` when the target is a short-lived CLI tool.

```bash
vix dev tool.cpp --force-script
```

This avoids treating a short-lived tool like a long-running server.

## Auto dependencies

Use `--auto-deps` to add include paths from installed local Vix dependencies.

```bash
vix dev main.cpp --auto-deps
```

Equivalent local form:

```bash
vix dev main.cpp --auto-deps=local
```

Search dependencies in parent folders too:

```bash
vix dev main.cpp --auto-deps=up
```

## Local script cache

Use `--local-cache` when you want the script cache to be local to the current working directory.

```bash
vix dev main.cpp --local-cache
```

## SQLite support

Enable SQLite support:

```bash
vix dev --with-sqlite
vix dev server.cpp --with-sqlite
```

When enabled, Vix can configure the build with SQLite-related options and set runtime environment variables for SQLite mode.

## MySQL support

Enable MySQL support:

```bash
vix dev --with-mysql
vix dev server.cpp --with-mysql
```

When enabled, Vix can configure the build with MySQL-related options and set runtime environment variables for MySQL mode.

## Sanitizers

Enable AddressSanitizer and UBSan:

```bash
vix dev server.cpp --san
```

Enable UBSan only:

```bash
vix dev server.cpp --ubsan
```

Enable ThreadSanitizer:

```bash
vix dev server.cpp --tsan
```

Use sanitizers when debugging:

- memory errors
- undefined behavior
- thread issues

## OpenAPI docs mode

`vix dev` supports the same docs mode flags as `vix run`.

Enable docs:

```bash
vix dev api --docs
```

Disable docs:

```bash
vix dev api --no-docs
```

Explicit values:

```bash
vix dev api --docs=true
vix dev api --docs=false
vix dev api --docs=1
vix dev api --docs=0
```

When enabled, Vix sets:

```txt
VIX_DOCS=1
```

When disabled, Vix sets:

```txt
VIX_DOCS=0
```

## Replay recording

Use `--replay` to record dev runs under `.vix/runs/`.

```bash
vix dev api --replay
vix dev server.cpp --replay
```

Recorded runs can later be inspected with:

```bash
vix replay list
vix replay last
vix replay failed
```

## Terminal clearing

Control terminal clearing:

```bash
vix dev api --clear=auto
vix dev api --clear=always
vix dev api --clear=never
```

Disable clearing:

```bash
vix dev api --no-clear
```

`--no-clear` is equivalent to:

```bash
--clear=never
```

## Logging

Set log level:

```bash
vix dev --log-level debug
```

Supported levels:

```txt
trace
debug
info
warn
error
critical
off
```

Use verbose mode:

```bash
vix dev --verbose
vix dev -v
```

Use quiet mode:

```bash
vix dev --quiet
vix dev -q
```

## Log format

Set log format:

```bash
vix dev --log-format kv
vix dev --log-format json
vix dev --log-format json-pretty
```

## Log color

Control colored output:

```bash
vix dev --log-color auto
vix dev --log-color always
vix dev --log-color never
```

Disable colors:

```bash
vix dev --no-color
```

## Dev output

Normal output is compact:

```txt
Dev api (dev)
  ✔ Configured
  ✔ Started pid=12345
```

Verbose output shows more detail:

```bash
vix dev -v
```

Example:

```txt
Dev api (dev)
  watching: /home/user/api
  target  : api
  build   : /home/user/api/build-ninja
  press Ctrl+C to stop
```

When a file changes:

```txt
Dev api (dev)
  changed: /home/user/api/src/main.cpp
```

When the app starts:

```txt
✔ Started pid=12389
```

When the app exits cleanly:

```txt
Dev app exited cleanly. Session stopped.
```

## Build failures in dev mode

If the build fails, Vix prints the build error and keeps the dev session alive.

It then waits for another relevant file change.

Example behavior:

```txt
Build failed in dev mode.
Fix the errors, save your files, and Vix will rebuild automatically.
```

This is important because a failed save should not force you to restart the whole dev session.

## Runtime exits in dev mode

If the child app exits with code `0`, Vix treats it as a clean exit and stops the session.

If it exits with a non-zero code, Vix reports the exit code.

For long-running servers, use:

```bash
vix dev server.cpp --force-server
```

## Platform support

The full `DevSession` implementation is Unix-oriented.

On Windows, the dev session can fall back or report that the current dev session implementation is not available yet.

For normal one-shot execution on Windows, use:

```bash
vix run
```

## Options

| Option                          | Description                                                |
| ------------------------------- | ---------------------------------------------------------- |
| `[target]`                      | Project, target, `.cpp` file, or `.vix` manifest.          |
| `-d, --dir <path>`              | Project directory.                                         |
| `--dir=<path>`                  | Same as `--dir <path>`.                                    |
| `--preset <name>`               | Configure or build preset. Default is usually `dev-ninja`. |
| `--preset=<name>`               | Same as `--preset <name>`.                                 |
| `--run-preset <name>`           | Run preset name.                                           |
| `--run-preset=<name>`           | Same as `--run-preset <name>`.                             |
| `-j, --jobs <n>`                | Number of parallel build jobs.                             |
| `--jobs=<n>`                    | Same as `--jobs <n>`.                                      |
| `--clean`                       | Clean or reconfigure before starting dev mode.             |
| `--replay`                      | Record runs under `.vix/runs/`.                            |
| `--cwd <path>`                  | Run the program from this working directory.               |
| `--cwd=<path>`                  | Same as `--cwd <path>`.                                    |
| `--env <K=V>`                   | Add or override one environment variable. Repeatable.      |
| `--env=<K=V>`                   | Same as `--env <K=V>`.                                     |
| `--args <value>`                | Add one runtime argument. Repeatable.                      |
| `--args=<value>`                | Same as `--args <value>`.                                  |
| `--run <args...>`               | Runtime arguments for script mode.                         |
| `--watch`                       | Watch files and restart. Enabled by default in `vix dev`.  |
| `--reload`                      | Alias-style flag for watch behavior.                       |
| `--force-server`                | Treat the program as a long-running server.                |
| `--force-script`                | Treat the program as a short-lived script.                 |
| `--auto-deps`                   | Auto-add includes from `.vix/deps/*/include`.              |
| `--auto-deps=local`             | Same as `--auto-deps`.                                     |
| `--auto-deps=up`                | Search dependencies in parent folders too.                 |
| `--san`                         | Enable AddressSanitizer and UBSan where supported.         |
| `--ubsan`                       | Enable UBSan only where supported.                         |
| `--tsan`                        | Enable ThreadSanitizer where supported.                    |
| `--with-sqlite`                 | Enable SQLite support.                                     |
| `--with-mysql`                  | Enable MySQL support.                                      |
| `--local-cache`                 | Use local script cache.                                    |
| `--docs`                        | Enable OpenAPI/docs for this run.                          |
| `--no-docs`                     | Disable OpenAPI/docs for this run.                         |
| `--docs=<0\|1\|true\|false>`    | Explicitly control OpenAPI/docs mode.                      |
| `--clear <auto\|always\|never>` | Clear terminal before runtime output.                      |
| `--clear=<auto\|always\|never>` | Same as `--clear <mode>`.                                  |
| `--no-clear`                    | Alias for `--clear=never`.                                 |
| `--log-level <level>`           | Set log level.                                             |
| `--log-level=<level>`           | Same as `--log-level <level>`.                             |
| `--verbose, -v`                 | Alias for debug-style output.                              |
| `--quiet, -q`                   | Reduce output to warnings and errors.                      |
| `--log-format <format>`         | Set log format: `kv`, `json`, or `json-pretty`.            |
| `--log-format=<format>`         | Same as `--log-format <format>`.                           |
| `--log-color <mode>`            | Set color mode: `auto`, `always`, or `never`.              |
| `--log-color=<mode>`            | Same as `--log-color <mode>`.                              |
| `--no-color`                    | Alias for `--log-color=never`.                             |
| `-h, --help`                    | Show command help.                                         |
| `-- <flags...>`                 | In script mode, pass compiler or linker flags.             |

## Environment variables

| Variable               | Description                                                    |
| ---------------------- | -------------------------------------------------------------- |
| `VIX_MODE=dev`         | Set automatically for the running app when not already set.    |
| `VIX_STDOUT_MODE=line` | Set automatically to make runtime output easier to stream.     |
| `VIX_DOCS`             | Enables or disables OpenAPI/docs mode.                         |
| `VIX_LOG_LEVEL`        | Runtime log level.                                             |
| `VIX_LOG_FORMAT`       | Runtime log format.                                            |
| `VIX_COLOR`            | Color mode.                                                    |
| `NO_COLOR`             | Disables colors.                                               |
| `VIX_CLI_CLEAR`        | Terminal clear behavior.                                       |
| `VIX_DB_ENGINE`        | Set to `sqlite` or `mysql` when database mode is enabled.      |
| `VIX_ENABLE_DB`        | Set when database mode is enabled.                             |
| `VIX_DB_USE_SQLITE`    | Set when SQLite mode is enabled.                               |
| `VIX_DB_USE_MYSQL`     | Set when MySQL mode is enabled.                                |
| `VIX_FRONTEND=vue`     | Set for the Vue frontend process.                              |
| `VIX_SHOW_ENV_HINT=1`  | Shows a hint when `.env.example` exists but `.env` is missing. |

## Common workflows

### Start a new app

```bash
vix new api
cd api
vix install
vix dev
```

### Run an existing app

```bash
cd api
vix install
vix dev
```

### Run from another directory

```bash
vix dev --dir ./apps/api
```

### Run a server file directly

```bash
vix dev server.cpp --force-server
```

### Run a CLI tool file directly

```bash
vix dev tool.cpp --force-script
```

### Pass runtime arguments

Project:

```bash
vix dev api --args --port --args 8080
```

Script:

```bash
vix dev server.cpp --run --port 8080
```

### Pass compiler flags to a script

```bash
vix dev server.cpp -- -O2 -DNDEBUG
```

### Use more build jobs

```bash
vix dev -j 8
```

### Enable SQLite

```bash
vix dev server.cpp --with-sqlite
```

### Enable MySQL

```bash
vix dev server.cpp --with-mysql
```

### Enable sanitizers

```bash
vix dev server.cpp --san
```

### Enable docs

```bash
vix dev api --docs
```

### Enable verbose dev output

```bash
vix dev -v
```

### Enable debug logs

```bash
VIX_LOG_LEVEL=debug vix dev
```

### Record a dev run

```bash
vix dev api --replay
```

## Common mistakes

### Running outside the project directory

Wrong:

```bash
vix new api
vix dev
```

Correct:

```bash
vix new api
cd api
vix dev
```

### Passing runtime args after `--` in script mode

In script mode, `--` is for compiler and linker flags.

Wrong:

```bash
vix dev server.cpp -- --port 8080
```

Correct:

```bash
vix dev server.cpp --run --port 8080
```

Or:

```bash
vix dev server.cpp --args --port --args 8080
```

### Expecting `vix dev` to build every target manually

`vix dev` is for development sessions.

For full repository validation or install preparation, use:

```bash
vix build --build-target all
```

### Using `VIX_LOG_LEVEL=release`

Wrong:

```bash
VIX_LOG_LEVEL=release vix dev
```

Correct for release builds:

```bash
vix build --preset release
```

`VIX_LOG_LEVEL` controls logging, not the build profile.

### Editing ignored files and expecting reload

Files such as `README.md`, `docs`, `.git`, `.vix`, build output, and `node_modules` are ignored by dev mode.

Editing them does not trigger reload.

### Expecting Windows dev sessions to behave like Unix dev sessions

The current full dev session orchestration is Unix-oriented.

On Windows, use one-shot commands such as:

```bash
vix run
vix build
```

when full dev session support is not available.

## Troubleshooting

### Nothing happens after saving

Make sure the file is watched.

Watched files include:

```txt
.cpp
.cc
.cxx
.c
.hpp
.hh
.hxx
.h
.ipp
.inl
.cmake
CMakeLists.txt
CMakePresets.json
vix.json
vix.toml
vix.lock
```

Ignored files and directories do not trigger reload.

### The app does not restart

Make sure the rebuild succeeds.

If the build fails, Vix waits for the next file change.

Fix the error, save again, and Vix will rebuild automatically.

### The executable cannot be found

Vix expects the resolved target executable to exist in the build directory.

Common locations:

```txt
build-ninja/<target>
build-ninja/bin/<target>
build-ninja/src/<target>
```

Make sure the project defines a runnable executable target.

For library projects, use:

```bash
vix tests
```

### The Vue frontend does not start

Make sure the project has:

```txt
frontend/package.json
```

and that the frontend can run:

```bash
cd frontend
npm run dev
```

### The app cannot find files

Set the runtime working directory:

```bash
vix dev api --cwd ./runtime
```

### Need environment variables

```bash
vix dev api --env APP_ENV=dev --env PORT=8080
```

### Need more details

```bash
vix dev -v
```

or:

```bash
VIX_LOG_LEVEL=debug vix dev
```

### Need less output

```bash
vix dev --quiet
```

### Need to restart after a failed build

Just fix the file and save again.

Vix keeps the dev session alive and rebuilds on the next relevant change.

## Best practices

Use `vix dev` while coding.

Use `vix build` before committing or packaging.

Use `vix build --build-target all` before install or full repository validation.

Use `vix check` when you want deeper validation.

Use `vix tests` for test suites.

Use `vix run` when you want a manual one-shot run.

Use `vix replay` when you need to inspect recorded runs.

Use `--run` for script runtime arguments.

Use `--` only for script compiler or linker flags.

Use `--force-server` for single-file servers.

Use `--force-script` for short-lived tools.

## Difference between `vix dev`, `vix run`, and `vix build`

| Command           | Purpose                | Builds | Runs | Watches       | Restarts      |
| ----------------- | ---------------------- | ------ | ---- | ------------- | ------------- |
| `vix build`       | Compile only           | yes    | no   | no            | no            |
| `vix run`         | Build and run manually | yes    | yes  | no by default | no by default |
| `vix run --watch` | Build, run, watch      | yes    | yes  | yes           | yes           |
| `vix dev`         | Development session    | yes    | yes  | yes           | yes           |

## Related commands

| Command      | Purpose                                         |
| ------------ | ----------------------------------------------- |
| `vix run`    | Build and run manually.                         |
| `vix build`  | Configure and compile.                          |
| `vix check`  | Validate build, tests, runtime, and sanitizers. |
| `vix tests`  | Run tests.                                      |
| `vix replay` | Inspect and replay previous Vix executions.     |
| `vix task`   | Run reusable project tasks.                     |

## Next step

Build the project without running it.

[Open the vix build guide](/cli/build)
