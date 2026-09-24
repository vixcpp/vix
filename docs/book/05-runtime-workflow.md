# Runtime Workflow

The runtime workflow is the part of Vix.cpp that takes an input, resolves what it represents, prepares the environment, builds when needed, executes the target, and reports the result.

The input can be a single C++ file:

```bash
vix run main.cpp
```

It can be the current project:

```bash
vix run
```

It can also be an already built executable:

```bash
vix run ./build-ninja/api
```

The command stays simple, but the runtime workflow decides what must happen.

## The basic model

The runtime workflow follows this model:

```txt
input
  -> resolve target
  -> prepare environment
  -> build if needed
  -> execute
  -> report status
```

Vix.cpp does not run every target in the same way.

A single C++ file is different from a backend project. A `vix.app` application is different from a CMake project. An already built executable is different from a replayed run.

Vix.cpp first resolves the target, then chooses the correct execution path.

## What `vix run` means

When you run:

```bash
vix run
```

you are asking Vix.cpp to run the current application or project.

When you run:

```bash
vix run main.cpp
```

you are asking Vix.cpp to run a specific C++ file.

When you run:

```bash
vix run ./build-ninja/api
```

you are asking Vix.cpp to run an existing executable.

These commands belong to the same runtime family, but each target uses a different strategy.

## Runtime targets

Vix.cpp runtime can work with several target types.

| Target            | Example                     | Meaning                                                   |
| ----------------- | --------------------------- | --------------------------------------------------------- |
| Current project   | `vix run`                   | Resolve and run the application in the current folder.    |
| C++ file          | `vix run main.cpp`          | Compile and run one file.                                 |
| `vix.app` project | `vix run`                   | Generate the internal project if needed, build, then run. |
| CMake project     | `vix run`                   | Configure or build with the CMake workflow, then run.     |
| Executable        | `vix run ./build-ninja/api` | Run an already built binary.                              |
| Replay record     | `vix replay last`           | Re-run a recorded execution.                              |
| Docker workflow   | `vix run docker ...`        | Run through a Docker-aware runtime path when configured.  |

The runtime should not guess randomly. It should resolve the target from the input and project files, then explain failures clearly when resolution is impossible.

## Project resolution

For a project directory, Vix.cpp looks for project files.

Resolution order:

```txt
1. CMakeLists.txt
2. vix.app
```

If `CMakeLists.txt` exists, Vix.cpp uses the CMake project.

If there is no `CMakeLists.txt` and `vix.app` exists, Vix.cpp uses the `vix.app` application model.

This rule matters because it protects existing CMake projects while still giving simpler applications a manifest-based path.

## Running a `vix.app` application

A `vix.app` file describes the application.

Example:

```txt
name = api
type = executable
standard = c++23

sources = [
  src/main.cpp,
  src/app/AppFactory.cpp,
]

include_dirs = [
  src,
]

modules = [
  core,
  json,
  http,
]
```

When Vix.cpp runs this application, the workflow is:

```txt
read vix.app
  -> validate manifest
  -> generate internal CMake project
  -> configure if needed
  -> build target
  -> run target
```

The generated CMake project lives under:

```txt
.vix/generated/app/
```

The developer continues to edit `vix.app`. The generated build layer remains an implementation detail.

## Running a CMake project

If the project has `CMakeLists.txt`, Vix.cpp keeps the CMake path.

Typical flow:

```txt
read CMakeLists.txt
  -> select preset or default build configuration
  -> configure if needed
  -> build target
  -> run executable
```

Examples:

```bash
vix run
vix run --preset dev
vix run --preset release
```

This is the right path for projects that need direct CMake control.

## Running one C++ file

For a single file:

```bash
vix run main.cpp
```

Vix.cpp can use a direct compile path when the file is simple enough.

The simple path is:

```txt
main.cpp
  -> compile
  -> link
  -> run
```

This is useful for learning, testing ideas, writing small tools, running examples, and validating short programs without creating a full project.

## CMake fallback for scripts

Some single-file programs need more than direct compilation.

Examples include:

```txt
Vix runtime modules
registry dependencies
database support
sanitizers
special linker flags
project-level configuration
```

In those cases, Vix.cpp can use a CMake fallback.

The command remains the same:

```bash
vix run server.cpp
```

The strategy changes:

```txt
direct compile when enough
CMake fallback when safer
```

This lets a file grow from a small experiment into something more serious without forcing the developer to rewrite the workflow immediately.

## Running a built executable

You can run an existing executable:

```bash
vix run ./build-ninja/api
```

Other examples:

```bash
vix run ./build-ninja/api
vix run ./dist/server
```

In this case, Vix.cpp does not need to resolve a project target first. The binary is already the target.

This is useful when the application has already been built and you want Vix.cpp to handle the execution workflow around it.

## Runtime arguments

Application arguments go after `--`.

```bash
vix run -- --port 8080
```

The rule is:

```txt
before -- = Vix.cpp arguments
after --  = application arguments
```

Example:

```bash
vix run main.cpp -- --name gaspard --debug
```

Here:

```txt
main.cpp belongs to Vix.cpp
--name gaspard --debug belongs to the application
```

This distinction avoids confusion between runtime options and application options.

## Environment variables

Runtime behavior often depends on environment variables.

Example:

```bash
VIX_LOG_LEVEL=debug vix run
```

For application configuration, use `.env` when the application supports it.

Example:

```dotenv
APP_ENV=development
SERVER_HOST=127.0.0.1
SERVER_PORT=8080
VIX_LOG_LEVEL=info
DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=./data/app.db
```

Before running a project seriously, check the local environment:

```bash
vix env check
```

For production:

```bash
vix env check --production
```

Environment configuration should be explicit. A runtime workflow should not depend on invisible assumptions.

## Development runtime

`vix run` is for one execution.

`vix dev` is for active development.

```bash
vix dev
```

The development workflow is:

```txt
start application
  -> watch files
  -> classify changes
  -> rebuild or reconfigure
  -> restart when useful
```

Use `vix dev` while editing the application.

Use `vix run` when you want to start it directly.

## Replay-enabled runs

A normal run does not automatically record replay data.

To record a run:

```bash
vix run --replay
```

For one file:

```bash
vix run main.cpp --replay
```

Then replay it:

```bash
vix replay last
```

Replay the latest failed run:

```bash
vix replay failed
```

Replay records are stored under:

```txt
.vix/runs/
```

A recorded run can include:

```txt
command
working directory
arguments
environment additions
status
stdout
stderr
combined logs
duration
```

Replay exists so developers do not have to debug from memory when the exact run can be reproduced.

## Runtime output

Runtime output should show what matters.

For a server application, output can include:

```txt
application name
mode
HTTP URL
WebSocket URL
thread count
status
hint to stop
```

Example shape:

```txt
Vix.cpp READY
HTTP:    http://localhost:8080/
WS:      ws://localhost:9090/
Threads: 8/8
Mode:    run
Status:  ready
Hint:    Ctrl+C to stop the server
```

The output should make the runtime state visible. When the application is ready, the developer should see how to reach it. When it fails, the developer should see enough information to understand what happened next.

## Exit behavior

Runtime commands should return meaningful exit codes.

| Exit code | Meaning                                     |
| --------- | ------------------------------------------- |
| `0`       | Run completed successfully.                 |
| `1`       | Run failed.                                 |
| `130`     | Interrupted by user, usually with `Ctrl+C`. |

When a server is interrupted manually, that should be reported clearly.

Example:

```txt
Program interrupted by user (SIGINT).
```

An intentional interruption is not the same as a crash.

## Runtime errors

Runtime errors should be readable.

Unhelpful output:

```txt
error
```

Useful output:

```txt
error: failed to run target
target: api
reason: executable not found
fix: run vix build
```

The model is:

```txt
what failed
where it failed
why it failed
what to try next
```

A runtime and developer toolkit should not only run commands. It should help the developer recover when a command fails.

## Runtime and dependencies

If a project depends on registry packages, install them first:

```bash
vix install
```

After adding a dependency:

```bash
vix add softadastra/json
vix run
```

After cloning a project:

```bash
git clone https://github.com/example/api.git
cd api
vix install
vix run
```

Runtime should not silently ignore missing dependencies. If dependencies are missing, the fix should be clear.

## Runtime and modules

A `vix.app` project can declare modules:

```txt
modules = [
  core,
  json,
  http,
  db,
]
```

The runtime does not directly “run modules”, but modules affect the build and linking steps required before execution.

If a runtime target needs `db`, the build must include database support.

If a runtime target needs `http`, the build must link the correct HTTP module.

The runtime workflow depends on correct module composition.

## Runtime and health

For backend applications, runtime is connected to health checks.

A backend should expose:

```txt
GET /health
```

Then local runtime can be checked with:

```bash
vix health local
```

Public runtime can be checked with:

```bash
vix health public
```

A process that starts is not automatically healthy.

Runtime means the application is actually usable, not only that a process exists.

## Runtime and logs

When running locally, logs usually appear in the terminal.

In production, logs may come from systemd and Nginx.

Use:

```bash
vix logs app
vix logs proxy
vix logs errors
```

The runtime workflow should leave enough trace to debug failures.

A process that fails silently is not production-ready.

## Runtime and services

Production runtime usually runs through a service manager such as systemd.

A common model is:

```txt
systemd starts the Vix.cpp application
Nginx exposes it publicly
health checks verify it
logs explain it
deployment updates it
```

Commands:

```bash
vix service init
vix service status
vix service restart
```

The same application can run locally with:

```bash
vix run
```

and in production through systemd.

The application remains the same. The runtime environment changes.

## Runtime and Docker

Some workflows can involve Docker.

The important model is:

```txt
Vix.cpp owns the runtime intent.
Docker can be one execution backend when configured.
```

A Docker-aware runtime flow should still be visible and explainable.

The developer should know:

```txt
which image or container is used
which ports are exposed
which command is executed
which environment values are passed
```

Docker should not be hidden behind unclear behavior.

## Runtime and production

A production run should not rely on manual terminal sessions.

Local:

```bash
vix run
```

Development:

```bash
vix dev
```

Production:

```bash
vix service init
vix deploy
vix health
```

The production model is not:

```txt
ssh into a server and run random commands forever
```

The production model is:

```txt
configured service
repeatable deployment
checked health
available logs
```

## Common workflows

Run one file:

```bash
vix run main.cpp
```

Run the current project:

```bash
vix run
```

Run with application arguments:

```bash
vix run -- --port 8080
```

Record and replay:

```bash
vix run --replay
vix replay last
```

Run development mode:

```bash
vix dev
```

Run after dependency installation:

```bash
vix install
vix run
```

Build release, then run:

```bash
vix build --preset release
vix run
```

## Common mistakes

### Passing application arguments before `--`

Wrong:

```bash
vix run --port 8080
```

Correct:

```bash
vix run -- --port 8080
```

### Expecting replay without recording

Wrong:

```bash
vix run
vix replay last
```

Correct:

```bash
vix run --replay
vix replay last
```

### Running before installing dependencies

Wrong after cloning:

```bash
vix run
```

Correct:

```bash
vix install
vix run
```

### Using `vix run` for active development

This works:

```bash
vix run
```

But while editing, prefer:

```bash
vix dev
```

### Treating “process started” as “application healthy”

A backend can start and still be broken.

Check it:

```bash
vix health local
```

## Runtime checklist

Before trusting a runtime workflow, check:

```txt
Can the application be resolved?
Can dependencies be installed?
Can the target be built?
Can the executable be found?
Can application arguments be passed?
Can environment variables be loaded?
Can failures be diagnosed?
Can important runs be replayed?
Can health be checked?
Can logs be read?
```

If the answer is yes, the runtime workflow is usable.

## What you should remember

Runtime is not only execution.

Runtime is:

```txt
resolve target
prepare environment
build when needed
run with clear arguments
record when requested
report useful status
```

Use:

```bash
vix run main.cpp
```

for one file.

Use:

```bash
vix run
```

for the current application.

Use:

```bash
vix dev
```

while editing.

Use:

```bash
vix run --replay
vix replay failed
```

when reproduction matters.

The core model is:

```txt
same command
right target
safe strategy
clear output
```

## Next chapter

[Next: Build Workflow](/book/06-build-workflow)
