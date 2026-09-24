# C++ Developer Toolkit

Most C++ projects eventually accumulate a collection of tools, scripts, conventions, and workflows around the source code itself.
The language solves the programming problem.
The surrounding tooling solves the development problem.

As projects grow, developers need ways to create projects, build them, run them, test them, format them, manage dependencies, inspect failures, package artifacts, and prepare releases.

This collection of tools and workflows is often called a developer toolkit.

Vix.cpp is designed as a modern C++ developer toolkit that provides a consistent workflow around native C++ applications.

## What is a developer toolkit?

A developer toolkit is not a compiler.

It is not a build system.

It is not a framework.

A developer toolkit provides the tools needed to work on a project throughout its lifecycle.

For example:

```txt
create
develop
run
test
debug
package
release
```

Most teams eventually build their own collection of scripts and commands to support these activities.

Vix.cpp attempts to provide a standard approach instead of forcing every project to reinvent the same operational layer.

## Why C++ needs better tooling

C++ is one of the most powerful programming languages available today.

It offers:

- native performance
- predictable execution
- direct memory control
- portability
- mature compilers
- a large ecosystem

However, the experience around the language is often fragmented.

A developer may use:

```txt
CMake
Ninja
CTest
clang-format
shell scripts
custom build scripts
package managers
deployment scripts
CI configuration
```

Each tool solves a useful problem.

The challenge is that developers frequently spend time switching between tools instead of focusing on the application itself.

Vix.cpp provides a unified workflow above these tools while remaining compatible with them.

## The Vix approach

The goal of Vix.cpp is not to replace everything.

The goal is to reduce operational friction.

A developer should be able to move from an idea to a working application with a predictable workflow.

For example:

```bash
vix new api
cd api
vix dev
```

A project should not require a long setup process before useful work can begin.

Likewise, common tasks should feel consistent:

```bash
vix build
vix run
vix tests
vix fmt
vix check
```

The command names remain simple because developers use them repeatedly throughout the life of a project.

## The project lifecycle

Most software projects follow a similar pattern.

They begin with a new project.

They evolve through development.

They are tested.

They are packaged.

Eventually they are released and maintained.

Vix.cpp provides commands that correspond to these stages.

```txt
Create
  -> vix new

Develop
  -> vix dev

Build
  -> vix build

Run
  -> vix run

Validate
  -> vix check

Test
  -> vix tests

Format
  -> vix fmt

Package
  -> vix pack

Release
  -> vix publish
```

The purpose is not to force a process.

The purpose is to provide a predictable command surface across projects.

## Project creation

A toolkit should make starting a project easy.

Create a project:

```bash
vix new api
```

Enter the project:

```bash
cd api
```

Start development:

```bash
vix dev
```

The developer can focus on the application instead of manually creating the initial structure.

## Running C++ directly

One of the simplest examples of the Vix toolkit philosophy is direct file execution.

Create:

```cpp
#include <vix.hpp>

int main()
{
  vix::print("Hello");
  return 0;
}
```

Run:

```bash
vix run main.cpp
```

This gives C++ a workflow that is useful for experiments, examples, prototypes, and learning.

As the project grows, the same developer can move into a full project structure without changing ecosystems.

## Build workflow

Building software is one of the most frequent development activities.

Vix.cpp provides:

```bash
vix build
```

This command becomes the standard entry point regardless of whether the project uses:

- CMake
- vix.app
- generated build files
- multiple targets

The goal is to reduce the need to remember project-specific build procedures.

## Development workflow

During active development, developers spend most of their time in a loop.

```txt
edit
build
run
observe
repeat
```

Vix.cpp uses:

```bash
vix dev
```

as the primary command for this workflow.

The exact implementation may evolve over time, but the purpose remains the same: provide a development-oriented entry point for the application.

## Testing

Reliable software requires tests.

Vix.cpp provides:

```bash
vix tests
```

This command acts as the standard way to execute project tests.

Developers should not need to remember a different command for every repository.

A consistent workflow lowers onboarding cost and makes automation easier.

## Validation

Testing is important, but many problems appear before tests even run.

Configuration issues, build issues, dependency issues, and environment problems often need to be detected earlier.

For this reason, Vix.cpp includes:

```bash
vix check
```

The role of this command is to validate the health of the project and identify common issues before they become larger problems.

## Formatting

Code formatting is one of the most common sources of unnecessary discussion inside teams.

Vix.cpp includes:

```bash
vix fmt
```

A standard formatting command allows teams to automate style consistency instead of debating formatting manually.

The goal is not to invent a new formatting standard.

The goal is to provide a predictable workflow.

## Dependency management

Modern applications rarely exist in isolation.

Projects often depend on external libraries.

Vix.cpp provides dependency commands such as:

```bash
vix add
vix install
vix update
vix remove
```

These commands give projects a consistent dependency workflow while remaining compatible with the broader C++ ecosystem.

## Packaging

Building software is not enough.

Eventually software must be distributed.

Vix.cpp includes packaging commands such as:

```bash
vix pack
vix verify
```

These commands help move a project from source code toward distributable artifacts.

## Diagnostics

A good toolkit should help developers understand failures.

Build systems and compilers can generate large amounts of output.

Vix.cpp attempts to surface useful diagnostics first while preserving access to the underlying logs.

The goal is not to hide information.

The goal is to reduce the time needed to understand what went wrong.

## Consistency across projects

One of the biggest advantages of a toolkit is consistency.

Imagine switching between ten repositories.

Without shared tooling, each project may use different commands.

```txt
build.sh
compile.sh
run.sh
dev.sh
test.sh
release.sh
```

Every repository becomes a new learning experience.

With a common toolkit, developers already know the basic workflow.

```bash
vix build
vix run
vix dev
vix tests
vix fmt
```

This reduces onboarding time and improves maintainability.

## Toolkit versus framework

Frameworks typically focus on a specific application model.

A web framework focuses on web applications.

A game framework focuses on games.

A desktop framework focuses on desktop applications.

A developer toolkit is broader.

Its purpose is to support the development process itself.

This is why Vix.cpp can be useful for:

- backend services
- CLI tools
- libraries
- AI agents
- WebSocket applications
- P2P systems
- game-oriented projects

The workflow remains familiar even when the application type changes.

## Toolkit versus runtime

The runtime and the toolkit are closely related.

The runtime provides the application foundation.

The toolkit provides the developer workflow.

```txt
Runtime:
  how the application operates

Toolkit:
  how the developer operates the application
```

Together they form the overall Vix.cpp experience.

## A practical example

A small application might follow this lifecycle:

Create it:

```bash
vix new blog
cd blog
```

Develop it:

```bash
vix dev
```

Test it:

```bash
vix tests
```

Validate it:

```bash
vix check
```

Build it:

```bash
vix build
```

Package it:

```bash
vix pack
```

Every stage uses the same toolkit.

The developer does not need to assemble a new workflow for every project.

## Mental model

A useful way to think about Vix.cpp is:

```txt
Language:
  C++

Build ecosystem:
  CMake, Ninja, compilers, linkers

Runtime:
  application foundation

Developer toolkit:
  commands, workflows, diagnostics, packaging, project lifecycle
```

Each layer has a different responsibility.

Vix.cpp focuses on the runtime and toolkit layers.

## Summary

A developer toolkit is the collection of tools and workflows used to build, run, test, validate, package, and maintain software.

Vix.cpp provides a modern C++ developer toolkit that gives projects a consistent operational workflow while remaining compatible with native C++ and the existing build ecosystem.

Its purpose is not to replace the language.

Its purpose is to make working with C++ applications more predictable, more repeatable, and easier to operate throughout the life of a project.

## Next step

Now that you understand the role of Vix.cpp as a runtime and developer toolkit, continue with the Vix Book to learn how applications are structured and built on top of that foundation.
