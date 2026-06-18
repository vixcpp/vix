# Vix.cpp vs CMake

Vix.cpp and CMake solve different problems.

CMake is a build system generator. It describes how a C++ project is configured, how targets are created, which source files are compiled, which libraries are linked, and how the final build system is generated.

Vix.cpp is a modern C++ runtime and developer toolkit. It gives C++ projects a higher-level workflow for creating, running, building, testing, formatting, packaging, and working with applications.

In other words, Vix.cpp does not compete with CMake at the same layer.

```txt
CMake:
  build configuration and build system generation

Vix.cpp:
  application workflow, CLI, runtime modules, diagnostics, project lifecycle
```

Vix.cpp can use CMake internally when that is the right tool. It can also work directly with an existing `CMakeLists.txt` project.

## The short answer

Use CMake when you need full control over the build.

Use Vix.cpp when you want a complete application workflow around your C++ project.

For many projects, the two work together.

```txt
Vix.cpp -> project workflow -> CMake/Ninja -> native binary
```

The output is still native C++. The compiler is still a C++ compiler. The build can still go through CMake and Ninja. Vix.cpp adds the workflow layer around that process.

## What CMake is responsible for

CMake is responsible for describing and generating the build system.

A typical `CMakeLists.txt` defines things such as:

- project name and version
- targets
- source files
- include directories
- compiler features
- compile options
- link options
- libraries
- install rules
- package discovery
- generated files
- platform-specific build logic

Example:

```cmake
cmake_minimum_required(VERSION 3.20)

project(hello LANGUAGES CXX)

add_executable(hello
  src/main.cpp
)

target_compile_features(hello PRIVATE cxx_std_20)
```

CMake is powerful because it can describe very complex build graphs. Large C++ projects, libraries, SDKs, embedded systems, and cross-platform applications often need that level of control.

## What Vix.cpp is responsible for

Vix.cpp is responsible for making the project easier to operate as an application.

It provides commands such as:

```bash
vix new api
vix build
vix run
vix dev
vix tests
vix fmt
vix check
vix add
vix pack
```

These commands are not only build instructions. They represent the daily workflow around the project.

For example:

```bash
vix build
```

means “build this project using the project model Vix detected.”

```bash
vix run
```

means “build if needed, resolve the runnable target, then start the app.”

```bash
vix dev
```

means “start the project in a development-oriented workflow.”

That is different from CMake, which mainly describes and generates the build system.

## Why Vix.cpp still uses CMake

C++ already has a mature build ecosystem. CMake is widely used, supported by many libraries, supported by IDEs, and understood by CI systems and package managers.

Vix.cpp does not try to throw that away.

Instead, Vix.cpp uses CMake where CMake is strong, then adds a more direct workflow above it.

```txt
CMake gives build compatibility.
Vix.cpp gives application workflow.
```

This is important because real C++ projects often need access to existing CMake packages, target exports, toolchains, platform-specific behavior, and advanced build logic.

Vix.cpp can benefit from that ecosystem while giving developers a simpler entry point for everyday work.

## Existing CMake projects

If a project already has a `CMakeLists.txt`, Vix.cpp can use it directly.

```txt
my-project/
  CMakeLists.txt
  src/
    main.cpp
```

From the project directory:

```bash
vix build
```

Vix.cpp detects the CMake project and builds through the native build path.

You can still pass CMake arguments when needed:

```bash
vix build -- -DCMAKE_BUILD_TYPE=Release
```

You can also build a specific target:

```bash
vix build --build-target my_app
```

This lets Vix.cpp act as a cleaner command layer above an existing CMake project.

## vix.app projects

Not every C++ project needs a hand-written `CMakeLists.txt`.

For simple and medium projects, Vix.cpp can use a `vix.app` manifest.

```txt
hello/
  vix.app
  src/
    main.cpp
```

Example:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

Then:

```bash
vix build
vix run
```

In this mode, Vix.cpp reads `vix.app`, validates it, and generates an internal CMake project.

The developer keeps a small manifest. Vix.cpp keeps the native build path underneath.

```txt
vix.app
  -> generated CMake project
  -> Ninja build
  -> native executable
```

This is useful when a project needs normal C++ compilation but does not need custom CMake logic yet.

## Project detection

Vix.cpp uses a simple resolution order.

```txt
1. If CMakeLists.txt exists, use the CMake project.
2. If no CMakeLists.txt exists but vix.app exists, use the vix.app project.
```

This means Vix.cpp does not break existing CMake projects.

If a project has a `CMakeLists.txt`, Vix.cpp respects it.

If a project only has a `vix.app`, Vix.cpp generates the internal build files needed to compile the app.

## When to use CMake directly

Use a normal `CMakeLists.txt` when the build needs full control.

Examples include:

- multiple complex targets
- custom code generation
- advanced install rules
- exported CMake packages
- advanced dependency discovery
- platform-specific build logic
- custom toolchains
- generated sources
- complex monorepos
- deeply customized compiler or linker configuration

In those cases, CMake is the correct layer for the build description.

Vix.cpp can still be useful above it as the command workflow.

## When to use vix.app

Use `vix.app` when the project structure is simple enough to describe in a small manifest.

Good examples include:

- small applications
- command-line tools
- examples
- prototypes
- demos
- simple libraries
- learning projects
- backend services with a straightforward layout

The practical rule is:

```txt
Start with vix.app when the project is simple.
Move to CMakeLists.txt when the build itself becomes complex.
```

This keeps the beginning of a project lightweight without blocking the project from becoming more advanced later.

## What Vix.cpp adds above CMake

Vix.cpp adds a project workflow that CMake does not try to provide directly.

For example, Vix.cpp can provide:

- a single CLI for common project operations
- project creation through templates
- single-file execution with `vix run main.cpp`
- development mode with `vix dev`
- cleaner diagnostics
- test commands
- formatting commands
- dependency commands
- package commands
- runtime-oriented modules
- production-oriented commands
- a consistent experience across project types

CMake can build a project, but it does not define the full developer workflow around running, testing, formatting, packaging, and operating the application.

That is the layer Vix.cpp focuses on.

## Example: same project, two workflows

A CMake-only workflow might look like this:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/hello
ctest --test-dir build
```

A Vix workflow can be:

```bash
vix build
vix run
vix tests
```

The Vix workflow can still use CMake and Ninja internally. The difference is that the developer interacts with a higher-level command surface.

## Example: build a CMake project with Vix

Project:

```txt
hello/
  CMakeLists.txt
  src/
    main.cpp
```

Build:

```bash
vix build
```

Run:

```bash
vix run
```

Run tests:

```bash
vix tests
```

In this case, Vix.cpp does not need to replace the `CMakeLists.txt`. It uses the existing project and gives it a consistent workflow.

## Example: build a vix.app project

Project:

```txt
hello/
  vix.app
  src/
    main.cpp
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

Build:

```bash
vix build
```

Run:

```bash
vix run
```

Vix.cpp generates the internal CMake project automatically.

## For experienced C++ developers

Experienced C++ developers do not need Vix.cpp because they cannot write CMake. They may use Vix.cpp because it reduces repeated operational work around projects.

A mature C++ project often accumulates scripts for common tasks:

```txt
scripts/build.sh
scripts/run.sh
scripts/test.sh
scripts/format.sh
scripts/package.sh
scripts/dev.sh
```

Vix.cpp tries to standardize these common operations without forcing the project to abandon CMake.

For teams, this can reduce onboarding cost. For maintainers, it can reduce duplicated project infrastructure. For tools, it creates a more predictable surface to integrate with.

## For new C++ developers

Newer C++ developers often meet build system complexity before they understand the language well.

Vix.cpp gives them a smoother starting point:

```bash
vix run main.cpp
```

Then:

```bash
vix new hello
cd hello
vix dev
```

They can still learn CMake later when they need it, but they do not need to start every project by writing build configuration manually.

## Comparison table

| Area                     | CMake                                | Vix.cpp                                          |
| ------------------------ | ------------------------------------ | ------------------------------------------------ |
| Main role                | Build system generator               | Runtime and developer toolkit                    |
| Main file                | `CMakeLists.txt`                     | `vix.app`, `vix.json`, or existing project files |
| Main command style       | `cmake -S`, `cmake --build`, `ctest` | `vix build`, `vix run`, `vix dev`, `vix tests`   |
| Best at                  | Describing complex builds            | Operating C++ applications                       |
| Output                   | Build files and targets              | Workflow around native C++ targets               |
| Existing ecosystem       | Very large                           | Built to integrate with it                       |
| Learning curve           | Can be high                          | Designed to be direct                            |
| Replacement relationship | Not replaced                         | Uses or complements CMake                        |

## Common misunderstanding

Vix.cpp should not be understood as “CMake but shorter.”

That is not the point.

CMake describes the build.

Vix.cpp operates the project.

A project can use both:

```txt
CMakeLists.txt:
  describes targets and build rules

Vix.cpp:
  provides the daily commands around the project
```

This separation is important. It allows Vix.cpp to be useful without pretending that every build problem should be solved by a new build language.

## Summary

CMake remains the right tool for describing complex C++ builds.

Vix.cpp provides the application workflow around C++ projects: create, build, run, develop, test, format, check, package, and operate.

For simple projects, Vix.cpp can use `vix.app` to avoid writing CMake manually.

For advanced projects, Vix.cpp can work with an existing `CMakeLists.txt`.

The relationship is not:

```txt
Vix.cpp instead of CMake
```

The better model is:

```txt
Vix.cpp above CMake when CMake is needed
Vix.cpp without manual CMake when the project is simple
```

## Next step

Learn how Vix.cpp thinks about the C++ application workflow.

Next: [C++ Runtime](/guides/cpp-runtime)
