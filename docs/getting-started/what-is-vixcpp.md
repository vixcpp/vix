# What is Vix.cpp?

Vix.cpp is a modern C++ runtime and developer toolkit for building real applications with less friction around the development workflow.

It does not change the C++ language. It does not hide the native toolchain. It gives C++ projects a higher-level workflow for the tasks developers repeat every day: creating projects, running files, building targets, testing, formatting, managing dependencies, and preparing applications for production.

In practice, Vix.cpp sits above the usual C++ tooling and provides a consistent command surface around it.

```txt
C++ source code
  -> Vix.cpp workflow
  -> CMake/Ninja when needed
  -> native executable or library
```

The goal is not to make C++ look like another language. The goal is to make the application workflow around C++ more direct.

## Why Vix.cpp exists

C++ already gives developers performance, portability, control over memory, mature compilers, and access to a very large ecosystem. The difficult part is often not the language itself, but the amount of work required before a project feels like a usable application.

A typical C++ project can require several separate decisions before the first feature is even useful:

- how the project is structured
- how it is configured
- how it is built
- how it is run
- how tests are executed
- how dependencies are added
- how diagnostics are read
- how the same workflow is repeated across machines or CI

Experienced C++ developers know how to wire these pieces together, but the process is still repetitive. Newer developers often lose time before they reach the actual application logic.

Vix.cpp exists to provide a standard application workflow around native C++ without removing the underlying tools that make C++ powerful.

## The basic idea

With Vix.cpp, a single C++ file can be run directly:

```bash
vix run main.cpp
```

A project can be created, built, and run through one command surface:

```bash
vix new api
cd api
vix build
vix run
```

During development, the same project can be started with:

```bash
vix dev
```

Tests and formatting use the same CLI:

```bash
vix tests
vix fmt
```

The important part is consistency. The developer does not need to remember a different workflow for every stage of the project. Vix.cpp provides one entry point while still producing normal native C++ artifacts.

## What Vix.cpp provides

Vix.cpp combines three layers.

First, it provides a CLI for the development lifecycle. Commands such as `vix run`, `vix build`, `vix dev`, `vix tests`, `vix fmt`, and `vix check` give common C++ tasks a predictable interface.

Second, it provides a runtime foundation for applications. Vix.cpp includes modules for HTTP servers, routing, JSON APIs, WebSocket applications, async execution, logging, validation, templates, key-value storage, synchronization, P2P systems, AI agents, and game-oriented projects.

Third, it integrates with the native C++ build ecosystem. Vix.cpp can work with normal CMake projects, and for simpler projects it can use a `vix.app` manifest to generate an internal CMake project automatically.

This means Vix.cpp is not limited to one project style. A small example, a backend service, a CLI tool, a library, and a larger CMake-based codebase can all use the same general workflow where it makes sense.

## Vix.cpp and CMake

Vix.cpp does not replace CMake. It uses the existing C++ build ecosystem instead of pretending it does not exist.

For advanced projects, a normal `CMakeLists.txt` remains the right tool. Vix.cpp can detect it and use the CMake project directly.

For simpler projects, a `vix.app` file can describe the target, sources, include directories, compile options, libraries, packages, and output directory. Vix.cpp then generates the internal CMake project needed to build it.

```txt
Project with CMakeLists.txt
  -> Vix uses the existing CMake project

Project with vix.app
  -> Vix generates an internal CMake project
  -> Vix builds the target
```

This approach keeps compatibility with the C++ ecosystem while removing unnecessary setup work for projects that do not need custom CMake logic.

## Vix.cpp and native C++

A Vix.cpp project is still a C++ project.

The source files are C++ files. The compiler is a C++ compiler. The result is a native executable or library. Vix.cpp does not turn C++ into a scripting language and does not require developers to abandon the tools they already use.

The difference is the workflow around the project.

Instead of starting every project by manually deciding how to wire the build, commands, logs, tests, formatting, dependencies, and runtime behavior, Vix.cpp gives the project a standard operational layer from the beginning.

## Application-first workflow

Vix.cpp is designed around the idea that a C++ project should quickly become an application that can be run, tested, checked, and shipped.

For example, a backend project is not only a set of `.cpp` files. It usually needs routes, configuration, logs, diagnostics, tests, development commands, and production behavior.

A CLI tool is not only a `main()` function. It needs argument handling, output formatting, errors, packaging, and repeatable builds.

A library is not only headers and source files. It needs tests, examples, versioning, packaging, and dependency metadata.

Vix.cpp tries to bring these application concerns closer to the developer workflow without taking away the native C++ foundation.

## Common commands

| Command            | Purpose                                        |
| ------------------ | ---------------------------------------------- |
| `vix run main.cpp` | Run a single C++ file.                         |
| `vix new <name>`   | Create a new project.                          |
| `vix build`        | Build the current project.                     |
| `vix run`          | Build if needed, then run the current project. |
| `vix dev`          | Start the project in development mode.         |
| `vix tests`        | Run project tests.                             |
| `vix fmt`          | Format source files.                           |
| `vix check`        | Validate the project.                          |
| `vix add`          | Add a dependency.                              |
| `vix pack`         | Package a project artifact.                    |

These commands are meant to cover the normal development loop without forcing every project to reinvent its own scripts.

## What you can build

Vix.cpp can be used for several kinds of C++ applications:

- backend services
- HTTP servers
- JSON APIs
- WebSocket applications
- command-line tools
- reusable C++ libraries
- template-based web applications
- AI agent applications
- game-oriented projects
- P2P systems
- local-first and offline-first systems
- production services

The same project can also evolve over time. A simple `vix.app` project can later move to a full `CMakeLists.txt` when it needs advanced build control.

## Example: run one file

Create `main.cpp`:

```cpp
#include <vix.hpp>

int main()
{
  vix::print("Hello from Vix.cpp");
  return 0;
}
```

Run it:

```bash
vix run main.cpp
```

This is useful for experiments, small tools, examples, and learning.

## Example: create a project

Create a project:

```bash
vix new api
cd api
```

Build it:

```bash
vix build
```

Run it:

```bash
vix run
```

Use development mode while editing:

```bash
vix dev
```

The project keeps a normal C++ structure, but the daily workflow is handled through the Vix CLI.

## How to think about Vix.cpp

A useful way to understand Vix.cpp is to separate the responsibilities.

C++ is the language and native execution model.

CMake and Ninja provide build system compatibility.

Vix.cpp provides the runtime layer, command workflow, project conventions, modules, diagnostics, and developer experience around the application.

```txt
C++:
  language, performance, native binaries

CMake and Ninja:
  build system and ecosystem compatibility

Vix.cpp:
  application workflow, runtime modules, CLI, diagnostics, project lifecycle
```

Vix.cpp is strongest when the goal is not only to compile C++ code, but to build and maintain an application with a repeatable workflow.

## When Vix.cpp is useful

Vix.cpp is useful when you want a C++ project that can be created, built, run, tested, formatted, and packaged through a consistent interface.

It is especially useful for projects that need to move quickly from code to a runnable application: backend services, tools, prototypes, examples, internal systems, and production-oriented C++ applications.

For projects that already have complex CMake infrastructure, Vix.cpp can still be used as a cleaner command layer above the existing build. In that case, it does not need to own the whole project structure.

## Summary

Vix.cpp is a modern C++ runtime and developer toolkit.

It keeps the native C++ model, integrates with the existing build ecosystem, and gives developers a consistent workflow for building real applications.

The shortest way to experience it is:

```bash
vix run main.cpp
```

The normal project workflow is:

```bash
vix new api
cd api
vix build
vix run
vix dev
```

## Next step

Install Vix.cpp and run your first C++ file.

Next: [Installation](/getting-started/installation)
