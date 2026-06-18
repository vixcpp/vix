# Getting Started with vix.app

`vix.app` is the simplest way to create and build a C++ application with Vix.cpp.

Instead of starting with a handwritten `CMakeLists.txt`, you describe your application in a small manifest file and let Vix.cpp generate the internal build project automatically.

The result is still a native C++ application.

Your code is compiled by a normal C++ compiler and built through the native C++ toolchain. `vix.app` simply provides a more approachable project description for common application workflows.

A typical `vix.app` project looks like this:

```txt
hello/
├── vix.app
└── src/
    └── main.cpp
```

The manifest describes the application:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

Then Vix.cpp handles the build workflow:

```txt
vix.app
  -> generated CMake project
  -> native build
  -> executable
```

This approach is useful when you want to focus on building an application instead of writing build-system configuration.

For advanced projects that need full build control, you can still use a normal `CMakeLists.txt`.

## What you will build

In this guide you will learn how to:

- create a minimal `vix.app` project
- build and run an application
- add include directories
- use compile definitions
- add compiler options
- link external libraries
- copy resources
- understand how Vix.cpp resolves projects

By the end of the guide you will understand the core ideas behind the `vix.app` project model and when it should be used instead of a handwritten `CMakeLists.txt`.

## Create the project

Create a new folder:

```bash
mkdir hello-vix-app
cd hello-vix-app
```

Create the source directory:

```bash
mkdir -p src
```

Create the main file:

```bash
touch src/main.cpp
```

Add this code to `src/main.cpp`:

```cpp
#include <vix.hpp>

int main()
{
  vix::print("Hello from vix.app");
  return 0;
}
```

## Create vix.app

Create a file named `vix.app` in the project root:

```bash
touch vix.app
```

Add this content:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

Your project should now look like this:

```txt
hello-vix-app/
  vix.app
  src/
    main.cpp
```

## Build the project

Run:

```bash
vix build
```

Vix will detect `vix.app`, generate an internal CMake project, and build your application.

The generated CMake project is written under:

```txt
.vix/generated/app/CMakeLists.txt
```

You should not edit this generated file manually.

Edit `vix.app` instead.

## Run the project

Run:

```bash
vix run
```

Vix will build the project if needed, then run the executable.

Expected output:

```txt
Hello from vix.app
```

## Use an include directory

For a slightly more realistic project, add an `include/` directory:

```bash
mkdir -p include/hello
touch include/hello/message.hpp
```

Add this to `include/hello/message.hpp`:

```cpp
#pragma once

#include <string>

namespace hello
{
  inline std::string message()
  {
    return "Hello from a header file";
  }
}
```

Update `src/main.cpp`:

```cpp
#include <vix.hpp>
#include <hello/message.hpp>

int main()
{
  vix::print(hello::message());
  return 0;
}
```

Update `vix.app`:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

include_dirs = [
  include,
]
```

Build and run again:

```bash
vix build
vix run
```

## Add compile definitions

You can add preprocessor definitions with `defines`:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

include_dirs = [
  include,
]

defines = [
  HELLO_VERSION="1.0.0",
  HELLO_DEBUG=1,
]
```

Example usage in C++:

```cpp
#include <vix.hpp>

#ifndef HELLO_VERSION
#define HELLO_VERSION "unknown"
#endif

int main()
{
  vix::print("Version:", HELLO_VERSION);
  return 0;
}
```

## Add compiler options

You can add compiler flags with `compile_options`:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]
```

These options are passed to the generated target.

## Set an output directory

Use `output_dir` to place the built executable in a specific build output folder:

```ini
name = hello
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]
```

After building, the executable will be placed under:

```txt
build-ninja/bin/
```

Run it with:

```bash
vix run
```

or manually:

```bash
./build-ninja/bin/hello
```

## Use packages and links

`packages` is used for `find_package(...)`.

`links` is used for `target_link_libraries(...)`.

For example, to use `Threads`:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

packages = [
  Threads:REQUIRED,
]

links = [
  Threads::Threads,
]
```

Important rule:

```txt
packages finds packages.
links links targets or libraries.
```

`packages` does not automatically link imported targets.

## Add resources

Resources are copied next to the built target after a successful build.

Example:

```bash
mkdir -p assets
echo "hello config" > assets/config.txt
```

Update `vix.app`:

```ini
name = hello
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]

resources = [
  assets,
]
```

After building, `assets/` will be copied next to the executable.

## Build presets

`vix build` uses embedded Vix presets.

Common commands:

```bash
vix build
```

```bash
vix build --preset dev
```

```bash
vix build --preset release
```

Default behavior usually uses the development Ninja build directory:

```txt
build-ninja/
```

Release builds use:

```txt
build-release/
```

## Project detection rule

Vix resolves projects in this order:

```txt
1. CMakeLists.txt
2. vix.app
```

If both files exist, Vix uses `CMakeLists.txt`.

If you want Vix to use `vix.app`, do not keep a `CMakeLists.txt` in the same project root.

## Complete minimal example

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

`src/main.cpp`:

```cpp
#include <vix.hpp>

int main()
{
  vix::print("Hello from vix.app");
  return 0;
}
```

Commands:

```bash
vix build
vix run
```

## What to remember

`vix.app` is the simple application manifest for Vix.cpp.

It allows developers to describe a C++ application without writing a `CMakeLists.txt` manually.

Vix.cpp uses `vix.app` to generate an internal CMake project and build a native executable or library through the standard C++ toolchain.

For simple and medium projects, `vix.app` provides a cleaner application-oriented workflow.

For advanced build requirements, a normal `CMakeLists.txt` can still be used.

In short:

vix.app = application manifest
CMakeLists.txt = advanced build definition
Vix.cpp = workflow that supports both

## Next steps

Continue with:

- [Manifest Reference](./manifest-reference.md)
- [Examples](./examples.md)
- [Packages and Links](./packages-and-links.md)
- [Tests](./tests.md)
