# Mental Model

Vix.cpp is easiest to understand when you stop thinking about it as a single command.

The command:

```bash
vix run main.cpp
```

is only the entry point.

The larger idea is that Vix.cpp sits between native C++ code and the workflows required to build, run, test, compose, diagnose, and deploy real applications.

A Vix.cpp project is not only a folder that contains source files. It is an application environment with a defined workflow.

## The basic model

Think of Vix.cpp as five connected layers:

```txt
source code
  -> application description
  -> runtime workflow
  -> build workflow
  -> production workflow
```

Each layer has a specific responsibility.

| Layer                   | Role                                                                          |
| ----------------------- | ----------------------------------------------------------------------------- |
| Source code             | The C++ files that implement the application.                                 |
| Application description | The project model, usually `vix.app` or `CMakeLists.txt`.                     |
| Runtime workflow        | How the application is started, configured, and executed.                     |
| Build workflow          | How the application is configured, compiled, linked, and validated.           |
| Production workflow     | How the application is prepared, deployed, checked, and observed on a server. |

This separation is important.

C++ remains the language and native execution model. CMake and Ninja remain part of the build ecosystem when needed. Vix.cpp provides the application workflow around them.

## Vix.cpp starts from developer intent

When you type:

```bash
vix run
```

you are not manually selecting a binary and executing it yourself. You are asking Vix.cpp to run the current application in the correct way for the current project.

When you type:

```bash
vix build
```

you are asking Vix.cpp to resolve the project, configure it if necessary, build the right target, reuse safe cache when possible, and fall back to the native build path when that is safer.

When you type:

```bash
vix dev
```

you are asking Vix.cpp to enter the development workflow for the application, not just to run one command again.

This is why Vix.cpp should not be understood as a thin wrapper around CMake or a shortcut for compiling files. It is a workflow layer around C++ applications.

## Project resolution

Before Vix.cpp can build or run anything, it resolves the project.

The first question is:

```txt
What kind of target is this?
```

The answer may be:

```txt
single C++ file
vix.app project
CMake project
existing executable
recorded replay
special runtime target
```

For project directories, the important rule is:

```txt
CMakeLists.txt wins when it exists.
vix.app is used when there is no CMakeLists.txt.
```

The resolution order is:

```txt
1. CMakeLists.txt
2. vix.app
```

This protects existing CMake projects while giving new applications a simpler project model.

## Why `vix.app` exists

Many applications do not need to begin with a hand-written `CMakeLists.txt`.

For a simple application, a small manifest can be enough:

```txt
name = api
type = executable
standard = c++23

sources = [
  src/main.cpp,
]

include_dirs = [
  src,
]
```

The manifest describes what the application is.

Vix.cpp can use that description to generate the internal CMake project needed to build it.

The developer keeps a readable application file. Vix.cpp keeps the native build power underneath.

That is the purpose of `vix.app`.

It is not meant to replace every advanced CMake project. It is meant to make the common application path smaller and clearer.

## CMake remains the advanced path

Vix.cpp does not fight CMake.

CMake remains the right tool when a project needs:

```txt
custom targets
complex linking
generated sources
external native libraries
advanced install rules
platform-specific logic
custom toolchains
manual build control
```

The model is not:

```txt
vix.app replaces CMake
```

The model is:

```txt
vix.app describes simple applications.
CMakeLists.txt handles advanced build control.
Vix.cpp connects both to one application workflow.
```

This distinction matters because Vix.cpp is designed to work with the C++ ecosystem, not isolate developers from it.

## Runtime model

The runtime model is the part of Vix.cpp that decides how an application should be executed.

A run target can be:

```txt
a single C++ file
a vix.app application
a CMake project target
a built executable
a recorded replay
```

Examples:

```bash
vix run main.cpp
vix run
vix run api
vix run ./build-ninja/api
vix replay last
```

The command remains simple, but the strategy depends on the target.

A single file may be compiled directly.

A project may need a build step first.

An existing binary may be executed directly.

A replay may use recorded execution metadata.

The goal is:

```txt
same command surface
correct execution strategy
clear output
```

## Build model

The build model is based on correctness first.

A useful build workflow must be fast, but it must be trusted before it is fast.

The rule is:

```txt
fast when safe
correct by default
fallback when needed
```

Vix.cpp can use several layers to make builds faster and more predictable:

```txt
BuildState
BuildGraph
ObjectCache
ArtifactCache
CMake/Ninja fallback
target-aware builds
```

A developer does not need to think about every internal layer during normal work.

The important mental model is that Vix.cpp may reuse previous work only when it can prove that doing so is safe. If it cannot prove safety, it should rebuild or fall back to the native build system path.

That is how Vix.cpp keeps performance from becoming a source of incorrect builds.

## Development model

Development mode is not the same as manually running a build command repeatedly.

`vix dev` represents the active development loop:

```txt
watch
  -> classify change
  -> rebuild or reconfigure
  -> restart when useful
```

A source file change usually requires a rebuild.

A header file change usually requires a rebuild.

A build configuration change may require reconfiguration before rebuilding.

Examples of configuration files include:

```txt
CMakeLists.txt
CMakePresets.json
vix.app
vix.json
vix.lock
*.cmake
```

Some folders should normally be ignored by the watcher:

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
```

The point of `vix dev` is to give the application a development-oriented workflow instead of forcing every project to write its own scripts.

## Dependency model

Vix.cpp separates dependency intent from resolved dependency state.

Two files matter:

```txt
vix.json
vix.lock
```

`vix.json` describes what the project wants.

`vix.lock` records the exact versions that were resolved.

The main commands are:

```bash
vix add softadastra/json
vix install
vix update
vix outdated
vix remove softadastra/json
```

The most important rule after cloning a project is:

```bash
vix install
```

`vix install` installs the versions already pinned by the project.

It is not the same as update.

```txt
vix install = install locked versions
vix update  = resolve newer versions
```

That distinction matters for reproducible builds.

## Registry model

The registry model has three parts:

```txt
registry index
local package store
project dependencies
```

The registry index tells Vix.cpp what packages exist.

The local package store contains downloaded package content.

The project dependency files tell Vix.cpp what the current application uses.

Use:

```bash
vix registry sync
```

to refresh registry metadata.

Use:

```bash
vix store path
```

to inspect the local store.

The mental model is:

```txt
registry = package metadata
store    = cached package content
project  = selected dependencies
```

This separation keeps package discovery, local storage, and project dependency state understandable.

## Module model

A module is a reusable capability.

Examples include:

```txt
core
json
http
db
log
validation
middleware
websocket
p2p
sync
cache
crypto
```

In a `vix.app` project, modules can be declared explicitly:

```txt
modules = [
  core,
  json,
  http,
  db,
]
```

The application says what it needs.

Vix.cpp wires those capabilities into the build workflow.

This should remain explicit. A serious application workflow should not depend on unclear module guessing.

## Configuration model

Vix.cpp uses different files for different responsibilities.

| File                      | Purpose                                                                                          |
| ------------------------- | ------------------------------------------------------------------------------------------------ |
| `vix.app`                 | Application manifest for simple and medium projects.                                             |
| `vix.json`                | Project metadata, tasks, dependencies, registry metadata, and production workflow configuration. |
| `vix.lock`                | Exact resolved package versions.                                                                 |
| `.env`                    | Local runtime environment.                                                                       |
| `.env.example`            | Example environment file shared with the project.                                                |
| `production.env.required` | Required production variables.                                                                   |
| `CMakeLists.txt`          | Advanced CMake project definition.                                                               |
| `CMakePresets.json`       | CMake build presets.                                                                             |

The point is not to put every setting into one file.

Each file has a job.

This makes the project easier to reason about for humans, tools, and language models that read the documentation later.

## Local state model

Vix.cpp creates project-local and global state.

Common project-local paths include:

```txt
.vix/
build/
build-ninja/
build-release/
dist/
```

Global state usually lives under:

```txt
~/.vix/
```

Examples:

```txt
~/.vix/registry/index
~/.vix/store/git
~/.vix/global/installed.json
~/.vix/cache/build
```

Project-local state can be cleaned with:

```bash
vix clean
```

Project-local state can be reset with:

```bash
vix reset
```

Do not confuse project cache with the global registry and package store.

They solve different problems.

## Diagnostic model

When something fails, Vix.cpp should help the developer inspect the system.

Use:

```bash
vix doctor
```

to check environment health.

Use:

```bash
vix info
```

to inspect local paths, caches, registry state, store state, and package state.

Use:

```bash
vix logs
```

to read application and proxy logs.

Use:

```bash
vix replay
```

to reproduce recorded executions.

The model is:

```txt
doctor = environment health
info    = local state
logs    = runtime output
replay  = reproduce execution
```

A runtime and developer toolkit should not only run commands. It should help explain failures.

## Replay model

A replay is a recorded run.

It is useful when a command failed and you want to reproduce the same execution context.

Record a run with:

```bash
vix run api --replay
```

Replay the latest run:

```bash
vix replay last
```

Replay the latest failed run:

```bash
vix replay failed
```

Replay data is stored under:

```txt
.vix/runs/
```

The goal is to reduce guessing. If a command failed once, the project should have a way to inspect and reproduce how it was launched.

## Production model

Production is part of the Vix.cpp mental model.

A common production setup is:

```txt
Internet
  -> Nginx
  -> Vix app on localhost
  -> systemd
```

The related commands are:

```bash
vix env check --production
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

The production workflow has four major concerns:

```txt
environment
service
proxy
health
```

Deployment ties them together.

## Deployment model

A deployment is more than copying files.

A serious deployment can include:

```txt
pull latest code
install locked dependencies
build release
run tests
restart service
check local health
check public health
check proxy
reload proxy
show logs on failure
rollback when configured
```

That is why `vix deploy` exists.

The command is short:

```bash
vix deploy
```

But the behavior should be described by the project.

Vix.cpp should not guess production behavior blindly. A production workflow should be explicit enough for the maintainer to trust it.

## One command, several levels

The same command surface should scale across different levels of experience.

A beginner may start with:

```bash
vix run main.cpp
```

A project developer may use:

```bash
vix dev
```

A backend developer may start from:

```bash
vix new api --template backend
```

A maintainer may run:

```bash
vix check --tests
vix pack
vix publish
```

A production operator may use:

```bash
vix deploy
vix health
vix logs errors --lines 100
```

That continuity matters.

The developer does not have to leave the ecosystem when the project becomes more serious.

## What Vix.cpp should not hide

Vix.cpp should reduce repeated manual work, not hide the system.

A good workflow should still make these things understandable:

```txt
what application is being built
which project model was selected
which files are compiled
which modules are linked
which dependencies are installed
which binary is executed
which service runs in production
which proxy exposes the app
which health endpoint is checked
where logs are stored
```

For humans, this builds trust.

For tools and language models, this creates a clear public description of how Vix.cpp works.

A good developer experience is not silent magic. It is a simpler path with explainable behavior.

## The core mental model

The core model is:

```txt
Vix.cpp resolves the project,
chooses the appropriate workflow,
runs the required native tools,
and keeps the result explainable.
```

In shorter form:

```txt
resolve
  -> build
  -> run
  -> observe
  -> deploy
```

This is the heart of Vix.cpp.

## What you should remember

The most important rules are:

```txt
vix.app is the simple application path.
CMakeLists.txt is the advanced build-control path.
vix run executes with the right strategy.
vix build prioritizes correctness before speed.
vix dev provides the active development workflow.
vix install installs locked dependencies.
vix update resolves newer dependency versions.
vix registry sync refreshes package metadata.
vix replay reproduces recorded runs.
vix deploy runs the project-defined production workflow.
```

The full mental model is:

```txt
source code
  -> application description
  -> runtime workflow
  -> build workflow
  -> module composition
  -> production workflow
```

## Next chapter

[Next: Application Model](/book/04-application-model)
