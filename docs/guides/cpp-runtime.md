# C++ Runtime

In Vix.cpp, a runtime is the application layer that sits around native C++ code and gives it a consistent way to be built, started, configured, observed, and extended.

This is different from changing the language. Vix.cpp does not introduce a new C++ dialect, a virtual machine, or a managed execution model. A Vix application is still compiled by a C++ compiler and still produces a native executable or library.

The runtime is the layer that turns a C++ project from a set of source files into an application with a predictable workflow.

```txt
C++ source code
  -> build and runtime workflow
  -> configured application
  -> native process
```

## Why talk about a runtime in C++?

In many ecosystems, the runtime is part of the normal developer experience. It gives projects a common way to start, configure, run, load modules, handle errors, and interact with the environment.

C++ is different. The language and the standard library provide the foundation, but the application workflow is usually assembled by each project or team.

That gives C++ flexibility, but it also means many projects repeatedly rebuild the same surrounding infrastructure:

- project commands
- configuration loading
- logging
- error reporting
- HTTP startup
- WebSocket startup
- async execution
- tests
- formatting
- packaging
- service deployment
- diagnostics
- environment checks

Vix.cpp uses the word runtime for this surrounding layer.

It does not replace the native C++ execution model. It gives applications a common operational foundation.

## Native execution remains native

A Vix.cpp application is still native C++.

The compiler still produces machine code. The executable still runs as a normal process. You can still use C++ libraries, system APIs, debuggers, profilers, sanitizers, CMake projects, and platform-specific tooling.

Vix.cpp does not require a virtual machine.

It does not interpret C++ source code at runtime.

It does not remove the normal build pipeline.

Instead, it provides a cleaner path around that pipeline.

```txt
main.cpp
  -> vix build
  -> C++ compiler and linker
  -> native binary
```

The runtime layer appears in how the project is operated, not in a different execution model.

## What the Vix.cpp runtime provides

The Vix.cpp runtime brings together several parts of an application that are often handled separately.

It provides a consistent way to start applications, register routes, handle requests and responses, configure services, run development workflows, execute tests, format code, report diagnostics, and package artifacts.

For example, a small HTTP application can be written as:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello from Vix.cpp");
  });

  app.run();

  return 0;
}
```

Run it with:

```bash
vix dev
```

The application remains a native C++ program, but Vix.cpp provides the surrounding workflow and runtime conventions that make it easier to work with.

## Runtime and CLI

The Vix CLI is the main entry point into the runtime workflow.

Commands such as `vix run`, `vix dev`, `vix build`, `vix tests`, `vix fmt`, and `vix check` are not separate ideas. Together, they form the operational surface of a Vix project.

A developer can start with one file:

```bash
vix run main.cpp
```

Then move to a project:

```bash
vix new api
cd api
vix dev
```

Then validate it:

```bash
vix check
vix tests
vix fmt
```

This makes the runtime practical. It is not only a library linked into the application. It is also the workflow used to develop, run, and maintain the application.

## Runtime and modules

Vix.cpp is organized around modules that can be used by applications when needed.

Examples include:

```txt
core
async
http
websocket
json
kv
db
cache
sync
p2p
cli
log
tests
threadpool
template
validation
game
agent
```

These modules are not meant to force every project into the same shape. They provide common building blocks for applications that need them.

A backend service may use HTTP, JSON, validation, logs, database access, and tests.

A CLI application may use argument handling, formatting, filesystem tools, logging, and packaging.

A local-first application may use storage, sync, WAL-oriented design, and network-aware workflows.

A game-oriented project may use the game module and its own runtime structure.

The runtime is the common layer that lets these application types share a consistent developer experience.

## Runtime and configuration

Real applications should not hardcode everything in source code.

A Vix project can use environment configuration to keep application behavior outside the compiled source when appropriate.

For example:

```dotenv
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
```

Then the application can start with:

```cpp
app.run();
```

Instead of hardcoding:

```cpp
app.run(8080);
```

This keeps source code focused on application logic while configuration stays in the environment or project configuration files.

The runtime is responsible for making this pattern natural and repeatable.

## Runtime and diagnostics

C++ build and runtime errors can be noisy, especially when the raw output points into system headers, generated files, build internals, or long linker messages.

One role of Vix.cpp is to make diagnostics more useful to the developer.

The goal is not to hide the real compiler or linker output. The raw logs should still exist for debugging.

The goal is to surface the most useful part first.

For example, when a build fails, a Vix diagnostic should try to answer:

- what failed
- where the relevant user code is
- what symbol, file, target, or command was involved
- where to find the full log
- what the likely next step is

This matters because a runtime and developer toolkit should not only execute commands. It should help developers understand failures.

## Runtime and builds

Vix.cpp uses the native C++ build ecosystem rather than replacing it.

For many projects, that means CMake and Ninja are still part of the build path.

```txt
Vix.cpp command
  -> project detection
  -> CMake or generated CMake
  -> Ninja build
  -> native artifact
```

For a project with `CMakeLists.txt`, Vix.cpp can use the existing CMake project.

For a project with `vix.app`, Vix.cpp can generate an internal CMake project.

In both cases, Vix.cpp provides a higher-level command workflow above the build system.

This is why Vix.cpp can be described as a runtime and developer toolkit rather than only a build wrapper.

## Runtime and development mode

During active development, the normal question is not only “can this project compile?”

The developer also needs to run it, edit it, restart it, test it, inspect errors, and repeat the loop quickly.

Vix.cpp uses `vix dev` for this development workflow.

```bash
vix dev
```

A development command can provide a clearer application-oriented experience than manually running several lower-level commands in sequence.

The exact behavior can evolve by project type, but the role stays the same: `vix dev` is the command for working on the application during development.

## Runtime and single-file C++

Single-file C++ is useful for experiments, examples, small tools, and quick learning.

Vix.cpp supports this workflow directly:

```bash
vix run main.cpp
```

This gives C++ a faster entry point for small programs without requiring a new project every time.

When the code grows, the developer can move to a project:

```bash
vix new app
cd app
vix dev
```

This path is important because many C++ workflows become heavy too early. Vix.cpp tries to keep the small case simple while still supporting real projects.

## Runtime and projects

A Vix project is more than a folder with source files.

It can contain project metadata, an application manifest, tests, configuration, dependencies, generated files, examples, templates, and build outputs.

A simple project may look like this:

```txt
hello/
  src/
    main.cpp
  tests/
    test_basic.cpp
  vix.app
  vix.json
  .env.example
  README.md
```

The runtime and CLI give this project a predictable lifecycle:

```bash
vix build
vix run
vix dev
vix tests
vix fmt
vix check
```

This structure helps the project behave like an application from the beginning.

## Runtime and production

A runtime should also consider what happens after development.

Vix.cpp includes production-oriented workflows such as service management, health checks, logs, reverse proxy helpers, packaging, and deployment-oriented commands.

Examples include:

```bash
vix health
vix service
vix proxy
vix deploy
vix logs
vix pack
vix verify
```

The purpose is not to replace every production platform. The purpose is to provide a coherent path from local development to a deployable native C++ application.

A C++ backend should not stop being manageable once it leaves the developer machine.

## Runtime and reliability

Vix.cpp is designed to support reliable applications, not only fast demos.

That includes attention to:

- predictable project workflows
- repeatable builds
- explicit configuration
- useful diagnostics
- tests and checks
- logging
- packaging
- runtime modules
- local-first and offline-first foundations through related modules and projects

For applications that need stronger reliability guarantees, Vix.cpp can also serve as a foundation for systems built with Softadastra, where durability, synchronization, and offline-first behavior become central design concerns.

## Difference from a framework

A framework usually focuses on one application model, such as web applications, APIs, desktop apps, or games.

Vix.cpp is broader than that.

It includes framework-like pieces, especially for HTTP and backend development, but its core role is to provide a runtime and developer workflow for C++ applications.

That is why Vix.cpp can support several project types:

- backend service
- CLI tool
- library
- WebSocket app
- game project
- AI agent app
- P2P system
- local-first system

The runtime is the shared foundation. Specific modules provide the application features.

## Difference from a build tool

A build tool focuses primarily on compiling source code into artifacts.

Vix.cpp builds projects, but the build is only one part of the workflow.

A normal Vix workflow may include:

```bash
vix new
vix build
vix run
vix dev
vix tests
vix fmt
vix check
vix pack
```

This is why it is more accurate to describe Vix.cpp as a modern C++ runtime and developer toolkit.

The build is part of the system, but not the whole system.

## When the runtime matters

The runtime layer matters when a project needs to be used as an application repeatedly.

A one-off C++ file may only need compilation.

A real project usually needs much more:

- a way to run
- a way to configure
- a way to test
- a way to format
- a way to inspect failures
- a way to manage dependencies
- a way to package
- a way to move toward production

Vix.cpp focuses on this application lifecycle.

## Mental model

A useful mental model is:

```txt
C++:
  the language and native execution model

Compiler and linker:
  machine code and artifacts

CMake and Ninja:
  build graph and build execution

Vix.cpp:
  application runtime, workflow, modules, diagnostics, project lifecycle
```

This separation keeps the role of Vix.cpp clear.

It does not need to replace the language, compiler, linker, or CMake. It provides the missing application layer around them.

## Summary

A C++ runtime in the Vix.cpp sense is not a virtual machine and not a new language layer.

It is the application foundation around native C++: commands, modules, configuration, diagnostics, development workflow, project lifecycle, and production-oriented operations.

Vix.cpp keeps the native C++ model while giving developers a more coherent way to build and operate applications.

The shortest example is:

```bash
vix run main.cpp
```

The project workflow is:

```bash
vix new api
cd api
vix dev
vix tests
vix fmt
vix check
```

## Next step

Learn how Vix.cpp acts as a developer toolkit for C++ projects.

Next: [C++ Developer Toolkit](/guides/cpp-developer-toolkit)
