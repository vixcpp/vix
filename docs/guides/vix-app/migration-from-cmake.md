# Migrating from CMake to vix.app

This guide shows how to migrate a simple `CMakeLists.txt` project to `vix.app`.

`vix.app` is not a replacement for every CMake project.

It is a simpler entry point for projects that do not need custom CMake logic.

## When migration makes sense

Migrating to `vix.app` makes sense when your CMake project is simple.

Good cases:

```txt
- one executable
- one static library
- one shared library
- simple source list
- simple include directories
- simple compile definitions
- simple package discovery
- simple target links
- simple resources
```

For example:

```txt
hello/
  CMakeLists.txt
  src/
    main.cpp
```

can usually become:

```txt
hello/
  vix.app
  src/
    main.cpp
```

## When to keep CMake

Keep `CMakeLists.txt` when your project needs advanced build logic.

Examples:

```txt
- many targets in one file
- custom commands
- generated sources
- install rules
- CTest integration
- FetchContent
- CPM.cmake
- custom toolchains
- advanced dependency discovery
- platform-specific build logic
- package export files
```

The rule is simple:

```txt
Use vix.app for simple targets.
Use CMakeLists.txt for full control.
```

## Important resolution rule

Vix resolves projects in this order:

```txt
1. CMakeLists.txt
2. vix.app
```

If both files exist, Vix uses `CMakeLists.txt`.

So if you want Vix to use `vix.app`, remove or rename the `CMakeLists.txt`.

Example:

```bash
mv CMakeLists.txt CMakeLists.txt.bak
```

Then run:

```bash
vix build
vix run
```

## Minimal executable migration

Original project:

```txt
hello/
  CMakeLists.txt
  src/
    main.cpp
```

Original `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)

project(hello LANGUAGES CXX)

add_executable(hello
  src/main.cpp
)

target_compile_features(hello PRIVATE cxx_std_20)
```

Equivalent `vix.app`:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

Then remove or rename `CMakeLists.txt`:

```bash
mv CMakeLists.txt CMakeLists.txt.bak
```

Build and run:

```bash
vix build
vix run
```

## Migrating include directories

Original `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)

project(myapp LANGUAGES CXX)

add_executable(myapp
  src/main.cpp
  src/app.cpp
)

target_include_directories(myapp PRIVATE
  include
)
```

Equivalent `vix.app`:

```ini
name = myapp
type = executable
standard = c++20

sources = [
  src/main.cpp,
  src/app.cpp,
]

include_dirs = [
  include,
]
```

Project layout:

```txt
myapp/
  vix.app
  include/
    myapp/
      app.hpp
  src/
    main.cpp
    app.cpp
```

## Migrating compile definitions

Original CMake:

```cmake
target_compile_definitions(myapp PRIVATE
  MYAPP_VERSION="1.0.0"
  MYAPP_ENABLE_LOGGING=1
)
```

Equivalent `vix.app`:

```ini
defines = [
  MYAPP_VERSION="1.0.0",
  MYAPP_ENABLE_LOGGING=1,
]
```

Complete manifest:

```ini
name = myapp
type = executable
standard = c++20

sources = [
  src/main.cpp,
  src/app.cpp,
]

include_dirs = [
  include,
]

defines = [
  MYAPP_VERSION="1.0.0",
  MYAPP_ENABLE_LOGGING=1,
]
```

## Migrating compile options

Original CMake:

```cmake
target_compile_options(myapp PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)
```

Equivalent `vix.app`:

```ini
compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]
```

Complete manifest:

```ini
name = myapp
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

## Migrating link options

Original CMake:

```cmake
target_link_options(myapp PRIVATE
  -Wl,--as-needed
)
```

Equivalent `vix.app`:

```ini
link_options = [
  "-Wl,--as-needed",
]
```

Quote values that contain commas.

## Migrating target links

Original CMake:

```cmake
target_link_libraries(myapp PRIVATE
  m
)
```

Equivalent `vix.app`:

```ini
links = [
  m,
]
```

Complete manifest:

```ini
name = myapp
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

links = [
  m,
]
```

## Migrating find_package

Original CMake:

```cmake
find_package(Threads REQUIRED)

target_link_libraries(myapp PRIVATE
  Threads::Threads
)
```

Equivalent `vix.app`:

```ini
packages = [
  Threads:REQUIRED,
]

links = [
  Threads::Threads,
]
```

Important:

```txt
packages -> find_package(...)
links    -> target_link_libraries(...)
```

`packages` does not link targets automatically.

## Migrating fmt

Original CMake:

```cmake
find_package(fmt REQUIRED)

target_link_libraries(myapp PRIVATE
  fmt::fmt
)
```

Equivalent `vix.app`:

```ini
packages = [
  fmt:REQUIRED,
]

links = [
  fmt::fmt,
]
```

## Migrating Boost components

Original CMake:

```cmake
find_package(Boost REQUIRED COMPONENTS system filesystem)

target_link_libraries(myapp PRIVATE
  Boost::system
  Boost::filesystem
)
```

Equivalent `vix.app`:

```ini
packages = [
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]

links = [
  Boost::system,
  Boost::filesystem,
]
```

Quote package values that contain commas.

## Migrating a static library

Original CMake:

```cmake
cmake_minimum_required(VERSION 3.24)

project(mathlib LANGUAGES CXX)

add_library(mathlib STATIC
  src/add.cpp
  src/mul.cpp
)

target_include_directories(mathlib PUBLIC
  include
)

target_compile_features(mathlib PUBLIC cxx_std_20)
```

Equivalent `vix.app`:

```ini
name = mathlib
type = static
standard = c++20

sources = [
  src/add.cpp,
  src/mul.cpp,
]

include_dirs = [
  include,
]
```

Build:

```bash
vix build
```

## Migrating a shared library

Original CMake:

```cmake
cmake_minimum_required(VERSION 3.24)

project(plugin LANGUAGES CXX)

add_library(plugin SHARED
  src/plugin.cpp
)

target_include_directories(plugin PUBLIC
  include
)

target_compile_features(plugin PUBLIC cxx_std_20)
```

Equivalent `vix.app`:

```ini
name = plugin
type = shared
standard = c++20

sources = [
  src/plugin.cpp,
]

include_dirs = [
  include,
]
```

Build:

```bash
vix build
```

## Migrating output directories

Original CMake:

```cmake
set_target_properties(myapp PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
```

Equivalent `vix.app`:

```ini
output_dir = bin
```

Complete example:

```ini
name = myapp
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]
```

The output can be placed under:

```txt
build-ninja/bin/myapp
```

## Migrating copied resources

Original CMake:

```cmake
add_custom_command(TARGET myapp POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:myapp>/assets
)
```

Equivalent `vix.app`:

```ini
resources = [
  assets,
]
```

Complete example:

```ini
name = myapp
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

## Migrating copied config files

Original CMake:

```cmake
add_custom_command(TARGET myapp POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/data/config.json
          $<TARGET_FILE_DIR:myapp>/config/config.json
)
```

Equivalent `vix.app`:

```ini
resources = [
  "data/config.json=config/config.json",
]
```

## Full migration example

Original project:

```txt
server/
  CMakeLists.txt
  include/
    server/
      server.hpp
  src/
    main.cpp
    server.cpp
  public/
    index.html
```

Original `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)

project(server LANGUAGES CXX)

find_package(Threads REQUIRED)

add_executable(server
  src/main.cpp
  src/server.cpp
)

target_compile_features(server PRIVATE cxx_std_20)

target_include_directories(server PRIVATE
  include
)

target_compile_definitions(server PRIVATE
  SERVER_VERSION="1.0.0"
)

target_compile_options(server PRIVATE
  -Wall
  -Wextra
)

target_link_libraries(server PRIVATE
  Threads::Threads
)

set_target_properties(server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(TARGET server POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/public
          $<TARGET_FILE_DIR:server>/public
)
```

Equivalent `vix.app`:

```ini
name = server
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
  src/server.cpp,
]

include_dirs = [
  include,
]

defines = [
  SERVER_VERSION="1.0.0",
]

compile_options = [
  -Wall,
  -Wextra,
]

packages = [
  Threads:REQUIRED,
]

links = [
  Threads::Threads,
]

resources = [
  public,
]
```

Then rename the old CMake file:

```bash
mv CMakeLists.txt CMakeLists.txt.bak
```

Build and run:

```bash
vix build
vix run
```

## CMake to vix.app mapping

| CMake | vix.app |
| --- | --- |
| `project(myapp)` | `name = myapp` |
| `add_executable(myapp ...)` | `type = executable` |
| `add_library(name STATIC ...)` | `type = static` |
| `add_library(name SHARED ...)` | `type = shared` |
| source list | `sources = [...]` |
| `target_include_directories` | `include_dirs = [...]` |
| `target_compile_definitions` | `defines = [...]` |
| `target_compile_options` | `compile_options = [...]` |
| `target_link_options` | `link_options = [...]` |
| `target_compile_features` | `compile_features = [...]` |
| `find_package(...)` | `packages = [...]` |
| `target_link_libraries(...)` | `links = [...]` |
| post-build copy | `resources = [...]` |
| target output directory | `output_dir = ...` |

## Things that do not map directly

Some CMake features do not have a direct `vix.app` equivalent.

Examples:

```txt
- custom commands
- generated sources
- install rules
- package export files
- FetchContent
- CTest
- toolchain-specific branches
- many targets in one file
- generator expressions
- custom functions and macros
```

If your project depends on these, keep `CMakeLists.txt`.

## Step-by-step migration checklist

1. Identify the main target.

```txt
add_executable(...)
add_library(...)
```

2. Copy the target name to `name`.

```ini
name = myapp
```

3. Convert the target type.

```ini
type = executable
```

or:

```ini
type = static
```

or:

```ini
type = shared
```

4. Convert source files.

```ini
sources = [
  src/main.cpp,
]
```

5. Convert include directories.

```ini
include_dirs = [
  include,
]
```

6. Convert definitions.

```ini
defines = [
  MYAPP_VERSION="1.0.0",
]
```

7. Convert compile options.

```ini
compile_options = [
  -Wall,
  -Wextra,
]
```

8. Convert packages.

```ini
packages = [
  Threads:REQUIRED,
]
```

9. Convert linked libraries.

```ini
links = [
  Threads::Threads,
]
```

10. Convert resources if needed.

```ini
resources = [
  assets,
]
```

11. Rename `CMakeLists.txt`.

```bash
mv CMakeLists.txt CMakeLists.txt.bak
```

12. Build.

```bash
vix build
```

13. Run.

```bash
vix run
```

## Verify the generated CMake

For `vix.app` projects, Vix generates an internal CMake project under:

```txt
.vix/generated/app/CMakeLists.txt
```

You can inspect it when debugging.

Do not edit it manually.

If you need to change the build, edit:

```txt
vix.app
```

## Debugging migration issues

Use verbose output:

```bash
vix build -v
```

Use raw CMake configure output:

```bash
vix build --cmake-verbose
```

Pass extra CMake options:

```bash
vix build -- -DCMAKE_PREFIX_PATH=/path/to/prefix
```

## Common migration mistakes

### Keeping CMakeLists.txt

If both files exist:

```txt
CMakeLists.txt
vix.app
```

Vix uses `CMakeLists.txt`.

To test `vix.app`, rename the CMake file:

```bash
mv CMakeLists.txt CMakeLists.txt.bak
```

### Forgetting links

This is not enough:

```ini
packages = [
  fmt:REQUIRED,
]
```

You also need:

```ini
links = [
  fmt::fmt,
]
```

### Missing include directories

If your source includes:

```cpp
#include <myapp/app.hpp>
```

and the file is under:

```txt
include/myapp/app.hpp
```

you need:

```ini
include_dirs = [
  include,
]
```

### Including generated sources

If your CMake project generates source files during the build, `vix.app` may not be the right fit.

Use CMake for generated-source workflows.

### Migrating too much too early

Do not try to migrate a large complex CMake project all at once.

Start with small apps, examples, demos, or libraries.

## Recommended migration targets

Good first migration targets:

```txt
- examples
- demos
- simple CLI tools
- small libraries
- test apps
- prototype projects
```

Risky migration targets:

```txt
- monorepos
- SDKs with install rules
- projects using FetchContent heavily
- projects with many generated files
- projects with many platform-specific branches
```

## Summary

`vix.app` can replace simple CMake files.

It maps common CMake patterns to a small manifest:

```ini
name = myapp
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

include_dirs = [
  include,
]
```

But it should not replace complex CMake projects.

Use this rule:

```txt
Start simple with vix.app.
Keep CMake when you need full control.
```

## Next steps

Continue with:

- [CMake Fallback](./cmake-fallback.md)
- [Manifest Reference](./manifest-reference.md)
- [Troubleshooting](./troubleshooting.md)
- [Best Practices](./best-practices.md)
