# Why Vix Exists

Vix.cpp exists because C++ is still one of the strongest languages for building serious software, but the workflow around C++ applications is often heavier than it needs to be.

The problem is not that C++ lacks power. The problem is that moving from source code to a runnable, testable, maintainable, and deployable application usually requires many separate tools, scripts, conventions, and manual decisions.

Vix.cpp was created to reduce that friction while keeping the native C++ model intact.

## The real problem

A developer may start with a simple goal:

```txt
I want to build an application in C++.
```

But before the application becomes useful, the developer often has to deal with many surrounding tasks:

```txt
project structure
compiler flags
build directories
CMake configuration
dependency setup
tests
formatting
runtime arguments
logs
environment variables
service files
reverse proxy configuration
health checks
deployment steps
```

Each task matters. Serious applications need these pieces.

The issue is that every project often has to assemble them again. The first useful feature can be delayed by project setup, build wiring, scripts, and environment problems.

Vix.cpp exists to make this path more direct.

A single file can start with:

```bash
vix run main.cpp
```

A real project can start with:

```bash
vix new api --template backend
cd api
vix dev
```

The point is not to remove the professional parts of C++ development. The point is to give them a clearer workflow.

## C++ is not the weakness

C++ is used to build operating systems, browsers, databases, game engines, runtimes, network servers, trading systems, embedded systems, and high-performance services.

The language is capable.

The compilers are capable.

The ecosystem is capable.

The difficulty is that the normal application workflow can be fragmented. A developer can write valid C++ code and still get stuck on basic operational questions:

```txt
How do I build this project?
How do I run it?
How do I add dependencies?
How do I run tests?
How do I format the code?
How do I inspect failures?
How do I prepare it for production?
```

Vix.cpp focuses on those questions.

It does not try to make C++ less powerful. It tries to make the surrounding workflow more predictable.

## The Vix.cpp answer

Vix.cpp gives C++ an application-first workflow.

Instead of starting from build-system details, the developer can start from the action they want to perform.

Run a file:

```bash
vix run main.cpp
```

Create a backend:

```bash
vix new api --template backend
```

Start development mode:

```bash
vix dev
```

Build the project:

```bash
vix build
```

Validate the project:

```bash
vix check --tests
```

Move toward production:

```bash
vix deploy
```

The idea is simple: common application work should have common commands.

This does not remove advanced control. It gives everyday tasks a stable entry point.

## Why not only CMake?

CMake is powerful and important.

Vix.cpp does not try to erase it. CMake remains the right tool for complex build graphs, advanced target configuration, cross-platform projects, exported packages, custom toolchains, generated sources, and platform-specific build logic.

But many application developers do not want the first step of every project to be a visible `CMakeLists.txt`.

For simple and medium applications, Vix.cpp can use `vix.app`:

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

Vix.cpp can generate the internal CMake project from that manifest when needed.

The rule is straightforward:

```txt
Use vix.app for the simple application path.
Use CMakeLists.txt when the build needs direct CMake control.
```

This keeps the entry point simple without cutting the project off from the C++ build ecosystem.

## Why not only a build tool?

A build command solves only one part of the problem.

Real applications need more than compilation. They need development commands, runtime configuration, module composition, dependency management, tests, diagnostics, packaging, service setup, proxy configuration, health checks, logs, and deployment.

That is why Vix.cpp includes commands such as:

```bash
vix dev
vix run
vix build
vix check
vix tests
vix fmt
vix make
vix add
vix install
vix registry sync
vix replay
vix service
vix proxy nginx
vix health
vix logs
vix deploy
```

The goal is not to have many commands for appearance. The goal is to cover the real lifecycle of a C++ application.

## The traditional workflow

A traditional C++ application workflow can look like this:

```txt
create files manually
write CMake manually
configure the build manually
run the build manually
find the binary manually
run the binary manually
write helper scripts manually
write service configuration manually
write reverse proxy configuration manually
inspect logs manually
deploy manually
```

This approach gives full control, but it also creates repeated operational work.

Every project can end up with its own commands, scripts, directory layout, and debugging habits.

Vix.cpp tries to make the repeated parts explicit and reusable.

## The Vix.cpp workflow

A Vix.cpp workflow is organized around the application lifecycle:

```txt
create
  -> develop
  -> build
  -> check
  -> test
  -> package
  -> deploy
  -> inspect
```

With commands:

```bash
vix new api --template backend
vix dev
vix build
vix check --tests
vix pack
vix deploy
```

And when something fails:

```bash
vix doctor
vix info
vix logs errors --lines 100
vix replay failed
```

Vix.cpp does not remove debugging. It makes debugging more structured.

## Why application-first matters

Many C++ tools are build-system-first or library-first.

Vix.cpp is application-first.

That means it starts from questions such as:

```txt
How does the developer create an app?
How does the app run locally?
How does the app use configuration?
How does the app enter development mode?
How does the app compose modules?
How does the app install dependencies?
How does the app expose HTTP?
How does the app check health?
How does the app move to production?
```

This is why the Vix Book is organized around the application lifecycle:

```txt
Introduction
Why Vix Exists
Mental Model
Application Model
Runtime Workflow
Build Workflow
Modules and Composition
From Local to Production
Next Steps
```

The book is not just about a few commands. It explains how Vix.cpp thinks about C++ applications.

## Reducing accidental complexity

There are two kinds of complexity in software.

Essential complexity is the real problem your application must solve.

Accidental complexity is the extra work caused by tooling, setup, unclear workflows, inconsistent scripts, and avoidable operational friction.

C++ applications can have deep essential complexity. That is normal.

But developers should not lose unnecessary time on accidental complexity around:

```txt
project creation
build setup
run configuration
dependency installation
module composition
development loops
production setup
diagnostics
deployment
```

Vix.cpp tries to reduce that surrounding friction.

The application itself can still be complex. The path to run, test, package, and ship it should be clearer.

## Keeping explicit behavior

Vix.cpp should not become magic.

A good Vix command should be simple to run and possible to explain.

For example:

```bash
vix build
```

means that Vix.cpp resolves the project, configures it when needed, builds the selected target, uses cache only when safe, falls back when needed, and prints useful diagnostics.

And:

```bash
vix deploy
```

means that Vix.cpp runs the configured deployment workflow, builds the application, validates it, restarts the service, checks health, checks proxy behavior, and surfaces logs if something fails.

The command is short, but the behavior should remain understandable.

```txt
simple command
explicit behavior
clear diagnostics
```

That principle matters because serious developers need trust, not mystery.

## Safe fallback

Vix.cpp should be fast when it can be fast, but correctness comes first.

A single-file program can sometimes be compiled directly. Other cases need a safer path because they involve runtime modules, special dependencies, database support, sanitizer modes, project configuration, or generated build metadata.

In those cases, Vix.cpp can fall back to the appropriate build strategy.

The developer may still run:

```bash
vix run server.cpp
```

But Vix.cpp chooses the safest execution path.

The goal is:

```txt
same command
correct strategy
less manual work
```

## Dependencies

Real projects need packages.

That is why Vix.cpp includes registry and dependency workflows:

```bash
vix registry sync
vix search json
vix add softadastra/json
vix install
vix update
vix outdated
vix remove
```

A Vix.cpp project can use:

```txt
vix.json
vix.lock
.vix/deps/
.vix/vix_deps.cmake
local store
registry index
```

After cloning a project, the important command is:

```bash
vix install
```

It installs the versions pinned by the project.

For adding a package:

```bash
vix add namespace/name
```

For publishing a package:

```bash
vix publish 0.2.0
```

Vix.cpp exists because real applications need dependency workflows, not only compiler commands.

## Production

A tool that runs code locally is useful.

A tool that helps move a C++ application from local development to production is more valuable.

Vix.cpp includes production-oriented commands:

```bash
vix env check --production
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

A production Vix.cpp application can follow this model:

```txt
Internet
  -> Nginx
  -> Vix app on localhost
  -> systemd
```

The developer should not need to rewrite the same service and proxy workflow for every backend.

Vix.cpp makes that path repeatable.

## Diagnostics

A modern developer workflow needs diagnostics.

When something fails, the developer needs to know what command ran, what failed, where it failed, which logs matter, and what can be tried next.

Vix.cpp includes commands such as:

```bash
vix doctor
vix info
vix logs
vix replay
```

`vix doctor` checks the environment.

`vix info` shows local state, registry paths, store paths, caches, and packages.

`vix logs` reads application and proxy logs.

`vix replay` replays recorded runs.

The goal is not only to fail. The goal is to fail with enough information to continue.

## Learning path

Vix.cpp also makes C++ more approachable.

A beginner can start with:

```bash
vix run main.cpp
```

Then grow into:

```bash
vix new api --template backend
vix dev
vix build
vix check --tests
```

Later, the same workflow can move toward production:

```bash
vix service init
vix proxy nginx init
vix deploy
```

The same tool supports learning, development, maintenance, and deployment.

That continuity is important.

## What Vix.cpp should feel like

Vix.cpp should feel like a workflow where C++ remains powerful, but the surrounding operations are clearer.

The commands should be predictable. The project structure should be understandable. The build should be fast when safe. The diagnostics should help. Production should not feel like an unrelated manual process.

That is the experience Vix.cpp is trying to create.

## The core reason

Vix.cpp exists because this should be normal in C++:

```bash
vix new api --template backend
cd api
vix dev
```

And this should also be normal:

```bash
vix deploy
vix health
vix logs errors --lines 100
```

The same ecosystem should support local development and production operations.

## What you should remember

Vix.cpp exists to make real C++ application development smoother without changing C++ or removing advanced control.

It gives clear workflows for the tasks developers repeat every day:

```txt
run
create
develop
build
test
compose
package
deploy
debug
```

The shortest summary is:

```txt
Vix.cpp modernizes the workflow around C++.
```

## Next chapter

[Next: Mental Model](/book/03-mental-model)
