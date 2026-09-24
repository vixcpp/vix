# Introduction

Welcome to the Vix Book.

This book explains how Vix.cpp helps developers build real C++ applications with a clearer workflow around the native C++ ecosystem.

Vix.cpp does not change the language. It does not replace the compiler, the linker, or CMake. It adds an application-oriented layer above the tools C++ developers already use.

The path of this book is simple:

```txt
single C++ file
  -> structured application
  -> runtime workflow
  -> build workflow
  -> modules
  -> production
```

The goal is to understand how Vix.cpp turns C++ code into applications that can be created, built, run, tested, packaged, and deployed through a consistent workflow.

## Why this book exists

C++ is powerful, mature, and widely used for systems where performance and control matter.

But the workflow around C++ applications is often fragmented. Before an application feels usable, a developer may need to configure the build system, choose a project layout, prepare compiler flags, wire dependencies, set up tests, define scripts, configure logs, manage runtime arguments, and prepare deployment steps.

None of these tasks are unusual. They are part of serious software development. The problem is that every project often has to assemble them again.

Vix.cpp exists to make that application workflow more direct.

The first step can be as small as:

```bash
vix run main.cpp
```

Then the same workflow can grow into a project:

```bash
vix new api --template backend
cd api
vix dev
```

That progression is the foundation of this book.

## What Vix.cpp is

Vix.cpp is a modern C++ runtime and developer toolkit.

It provides a command workflow for common development tasks:

```bash
vix run
vix dev
vix build
vix check
vix tests
vix fmt
vix pack
vix deploy
```

It also provides runtime modules for application development, including HTTP, JSON, WebSocket, async execution, logging, validation, caching, P2P, sync, AI agents, games, and production-oriented workflows.

A useful way to think about Vix.cpp is:

```txt
C++:
  language, performance, native binaries

CMake and Ninja:
  build system compatibility

Vix.cpp:
  runtime, CLI, modules, diagnostics, project workflow
```

Vix.cpp works around C++. It does not replace it.

## The first command

The smallest useful Vix workflow is:

```bash
vix run main.cpp
```

Example:

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

This gives fast feedback without creating a full project first.

For experiments, examples, small tools, and learning, this is enough.

## From file to application

A real application needs structure.

At some point, one file is not enough. You need source folders, configuration, tests, dependencies, logs, runtime behavior, and a repeatable development command.

For that, Vix.cpp provides project templates:

```bash
vix new api --template backend
cd api
vix dev
```

A backend project can include:

```txt
routes
configuration
validation
database access
tests
health checks
production configuration
deployment workflow
```

This is the shift from running code to building an application.

Vix.cpp is designed for that shift.

## Book structure

This book is organized into nine chapters:

```txt
01. Introduction
02. Why Vix Exists
03. Mental Model
04. Application Model
05. Runtime Workflow
06. Build Workflow
07. Modules and Composition
08. From Local to Production
09. Next Steps
```

Each chapter answers one important question.

| Chapter                  | Main question                                             |
| ------------------------ | --------------------------------------------------------- |
| Introduction             | What path does this book follow?                          |
| Why Vix Exists           | What problem does Vix.cpp solve for C++ developers?       |
| Mental Model             | How should you think about Vix.cpp as a system?           |
| Application Model        | How is a Vix application structured?                      |
| Runtime Workflow         | How does Vix run files, projects, binaries, and commands? |
| Build Workflow           | How does Vix build safely and efficiently?                |
| Modules and Composition  | How do modules and dependencies fit together?             |
| From Local to Production | How does a Vix app move from development to deployment?   |
| Next Steps               | What should you build after learning the core model?      |

The structure is intentionally focused.

It follows the lifecycle of a C++ application instead of listing features randomly.

## The Vix.cpp path

The practical path looks like this:

```bash
vix run main.cpp
vix new api --template backend
cd api
vix dev
vix build
vix check --tests
vix service init
vix proxy nginx init
vix health
vix deploy
```

Each command belongs to a stage:

```txt
try code
  -> create project
  -> develop
  -> build
  -> validate
  -> prepare service
  -> configure proxy
  -> check health
  -> deploy
```

The same project can move from a local experiment to a production service without changing the overall workflow model.

## Application model

Vix.cpp supports two main project models.

For simple and medium applications, Vix.cpp can use `vix.app`.

A `vix.app` file describes the application in a small manifest:

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

Vix.cpp can use that manifest to generate the internal CMake project needed to build the application.

For advanced projects, a normal `CMakeLists.txt` is still supported.

The rule is:

```txt
Use vix.app for a simple application workflow.
Use CMakeLists.txt when the build needs advanced control.
```

If `CMakeLists.txt` exists, Vix.cpp uses it.

If there is no `CMakeLists.txt` and `vix.app` exists, Vix.cpp uses `vix.app`.

This keeps the simple path simple while preserving compatibility with the existing C++ build ecosystem.

## Runtime workflow

Vix.cpp can run different kinds of targets:

```txt
single C++ files
project applications
vix.app manifests
built binaries
recorded replay runs
```

The common command is:

```bash
vix run
```

or:

```bash
vix run main.cpp
```

Under the hood, Vix.cpp selects the appropriate strategy.

A single file may use direct compilation.

A project may require a build step first.

A CMake project may go through the CMake/Ninja path.

The goal is not magic. The goal is a consistent command with clear behavior.

```txt
same command
correct strategy
clear output
```

## Build workflow

Vix.cpp also cares about build performance, but correctness comes first.

The build principle is:

```txt
fast when safe
correct by default
fallback when needed
```

Vix.cpp can use build state, object cache, artifact cache, target-aware builds, and CMake/Ninja integration.

But if a shortcut cannot be proven safe, Vix.cpp should rebuild or fall back to the native build system path.

A build tool must be trusted before it is fast.

## Modules and composition

Real applications need features beyond `main()`.

Vix.cpp applications can compose modules such as:

```txt
core
json
http
db
validation
middleware
log
websocket
p2p
sync
cache
crypto
```

Dependencies can also be managed through the registry workflow:

```bash
vix registry sync
vix search json
vix add softadastra/json
vix install
```

After cloning a project, the important dependency command is:

```bash
vix install
```

It installs the exact versions pinned by the project.

## From local to production

Vix.cpp should not stop at local development.

A C++ application eventually needs to run somewhere.

A typical production setup can look like this:

```txt
Internet
  -> Nginx
  -> Vix app on localhost
  -> systemd
```

Vix.cpp provides commands for this workflow:

```bash
vix env check --production
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

The purpose is not to replace every deployment platform.

The purpose is to make production part of the application workflow instead of treating it as an unrelated manual process.

## What this book is not

This book is not a full C++ language course.

It will not teach every C++ feature, memory rule, template technique, concurrency model, or performance detail.

Those topics matter, but they belong to C++ itself.

This book focuses on Vix.cpp: how to use it, how to understand its model, and how to build applications with it.

## What you should expect

By the end of this book, you should understand:

- why Vix.cpp exists
- how Vix.cpp fits into the C++ ecosystem
- how `vix.app` fits into the project model
- how `vix run` works conceptually
- how `vix build` relates to CMake and generated projects
- how modules are composed
- how dependencies are installed
- how local development moves toward production

You should be able to move from:

```bash
vix run main.cpp
```

to:

```bash
vix new api --template backend
cd api
vix dev
```

and later to:

```bash
vix deploy
```

## The core mental model

The core mental model is:

```txt
Vix.cpp is an application workflow around native C++.
```

That workflow includes:

```txt
create
run
develop
build
test
compose
package
deploy
debug
```

This is why Vix.cpp exists.

This is what the book explains.

## What you should remember

Vix.cpp starts small:

```bash
vix run main.cpp
```

It becomes useful for applications:

```bash
vix new api --template backend
cd api
vix dev
```

It becomes serious in production:

```bash
vix deploy
vix health
vix logs errors --lines 100
```

The path of the book is:

```txt
one file
  -> application model
  -> runtime workflow
  -> build workflow
  -> module composition
  -> production
```

The final goal is clear:

```txt
Build real C++ applications with a modern workflow.
```

## Next chapter

[Next: Why Vix Exists](/book/02-why-vix)
