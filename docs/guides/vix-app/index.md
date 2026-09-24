# vix.app

`vix.app` is the simple application manifest for Vix.cpp.

It gives C++ projects a small, readable project description without forcing every application to start with a hand-written `CMakeLists.txt`.

A `vix.app` file describes what the project is, which files it builds, which include directories it uses, which libraries it links, and which output it should produce.

Vix.cpp can then generate the internal CMake project needed to build the application.

```txt
vix.app
  -> generated CMake project
  -> vix build
  -> native executable or library
```

The result is still native C++. Vix.cpp does not replace the compiler, the linker, or the C++ build ecosystem. It provides a simpler entry point for projects that do not need full manual CMake control at the beginning.

## Why `vix.app` exists

C++ projects often start with build configuration before the developer can focus on the application itself.

For advanced projects, that control is useful.

For many simple and medium projects, the developer usually only needs to describe a few things:

```txt
what the project is called
what kind of target it builds
which C++ standard it uses
which source files are compiled
which include directories are available
which libraries or packages are linked
where the output goes
```

That information does not always need a full CMake file written by hand.

`vix.app` exists to make the common application path smaller:

```txt
describe the application
  -> let Vix.cpp generate the internal build project
  -> build and run with the Vix workflow
```

This keeps the early project experience focused on the application instead of build boilerplate.

## A minimal example

A minimal application can use this structure:

```txt
hello/
├── vix.app
└── src/
    └── main.cpp
```

`vix.app`:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

`src/main.cpp`:

```cpp
#include <vix.hpp>

int main()
{
  vix::print("Hello from vix.app");
  return 0;
}
```

Build the project:

```bash
vix build
```

Run it:

```bash
vix run
```

The project stays simple, but it still builds as a native C++ application.

## What `vix.app` describes

A `vix.app` file can describe the normal build properties of an application or library.

Common fields include:

| Field             | Purpose                                                              |
| ----------------- | -------------------------------------------------------------------- |
| `name`            | Target or application name.                                          |
| `type`            | Target type, such as `executable`, `static`, `shared`, or `library`. |
| `standard`        | C++ standard used by the target.                                     |
| `sources`         | Source files compiled into the target.                               |
| `include_dirs`    | Include directories available to the target.                         |
| `defines`         | Preprocessor definitions.                                            |
| `packages`        | CMake packages required by the target.                               |
| `links`           | Libraries or imported targets linked to the target.                  |
| `compile_options` | Extra compiler options.                                              |
| `link_options`    | Extra linker options.                                                |
| `resources`       | Files or directories copied with the target.                         |
| `modules`         | Internal application modules loaded from `modules/`.                 |
| `output_dir`      | Output directory for the generated artifact.                         |

Example:

```ini
name = myapp
type = executable
standard = c++23
output_dir = bin

sources = [
  src/main.cpp,
  src/app.cpp,
]

include_dirs = [
  include,
]

defines = [
  MYAPP_VERSION="1.0.0",
]

packages = [
  Threads:REQUIRED,
]

links = [
  Threads::Threads,
]

compile_options = [
  -Wall,
  -Wextra,
]
```

This is still a project description, not a new programming language. It is meant to cover common cases clearly.

## Project detection

Vix.cpp supports both existing CMake projects and `vix.app` projects.

The project resolution order is:

```txt
1. If CMakeLists.txt exists, Vix.cpp uses the CMake project.
2. If no CMakeLists.txt exists but vix.app exists, Vix.cpp uses vix.app.
```

This rule protects existing CMake projects.

A project with `CMakeLists.txt` continues to use its CMake build definition.

A project with only `vix.app` uses the manifest-based Vix.cpp workflow.

When Vix.cpp uses `vix.app`, it generates the internal CMake project under:

```txt
.vix/generated/app/CMakeLists.txt
```

Do not edit the generated file manually.

## Application modules

`vix.app` projects can load internal modules declared with the `modules` field.

```ini
modules = [
  auth,
  live_chat,
]
```

Create modules with the CLI:

```bash
vix modules init
vix modules add live_chat --websocket --workflow attached
```

The WebSocket module name is customizable. You can pass it positionally or with `--name`:

```bash
vix modules add --websocket --name notifications
```

Runtime WebSocket workflows are wired through the generated app integration. The `websocket.client` workflow is not selected as the app runtime.

Edit `vix.app` instead.

## Supported target types

`vix.app` supports common target types.

```ini
type = executable
```

Use this for applications and command-line tools.

```ini
type = static
```

Use this for static libraries.

```ini
type = shared
```

Use this for shared libraries.

```ini
type = library
```

Use this when you want the default library behavior supported by Vix.cpp.

The target type should match what the project is meant to produce.

## `vix.app` and CMake

`vix.app` does not remove CMake.

It gives a simpler entry point for projects that do not need custom CMake logic yet.

Internally, Vix.cpp can generate a CMake project from the manifest. That means the project still benefits from the native C++ build ecosystem.

```txt
Simple project:
  vix.app -> generated CMake -> native build

Advanced project:
  CMakeLists.txt -> normal CMake build
```

This separation lets a project start small and move to full CMake control later.

## When to use `vix.app`

Use `vix.app` when the project has a straightforward structure.

Good examples include:

- small applications
- command-line tools
- examples
- learning projects
- simple libraries
- prototypes
- demos
- applications with a few dependencies
- projects that do not need custom CMake logic

A typical layout is:

```txt
app/
├── vix.app
├── include/
└── src/
```

`vix.app` keeps this kind of project readable and easy to build.

## When to use `CMakeLists.txt`

Use a normal `CMakeLists.txt` when the build needs full CMake control.

Examples include:

- multiple complex targets
- custom code generation
- generated source files
- advanced install rules
- exported CMake packages
- complex toolchains
- platform-specific build logic
- advanced dependency discovery
- custom CMake functions
- large monorepos

The rule is practical:

```txt
Start with vix.app when the project is simple.
Move to CMakeLists.txt when the build needs advanced control.
```

Vix.cpp can work with both paths.

## What `vix.app` is not

`vix.app` is not meant to become a second CMake language.

It should not try to cover every advanced build case.

Its role is to describe common application and library projects clearly.

When the build becomes complex, use CMake directly.

The model is:

```txt
vix.app for clarity
CMake for full control
Vix.cpp connects both
```

## How it fits into Vix.cpp

`vix.app` is one part of the larger Vix.cpp application workflow.

A typical project may use:

```txt
vix.app  -> describes the application build
vix.json -> describes project metadata, tasks, dependencies, and workflow
vix.lock -> records exact dependency versions
.env     -> stores local runtime configuration
```

Each file has a separate job.

This keeps the project easier to understand for developers, CI systems, tooling, and language models that read the documentation.

## Next steps

Continue with:

- [Getting Started](./getting-started)
- [Manifest Reference](./manifest-reference)
- [Examples](./examples)
- [Packages and Links](./packages-and-links)
