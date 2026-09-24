# Manifest Reference

This page documents every field supported by `vix.app`.

A `vix.app` file is a simple manifest placed at the root of a project.

Example:

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

## File format

`vix.app` uses a simple key-value format.

Supported forms:

```ini
key = value
```

```ini
key = [value1, value2, value3]
```

```ini
key = [
  value1,
  value2,
  value3,
]
```

Comments are supported with `#`:

```ini
# This is a comment
name = hello # inline comment
```

Values can be quoted when needed:

```ini
sources = [
  "src/with space.cpp",
]
```

Use quotes when a value contains spaces, commas, or special characters.

## Resolution rule

Vix resolves project files in this order:

```txt
1. CMakeLists.txt
2. vix.app
```

If `CMakeLists.txt` exists, Vix uses it directly.

If no `CMakeLists.txt` exists but `vix.app` exists, Vix uses `vix.app`.

## Supported fields

| Field | Required | Description |
| --- | --- | --- |
| `name` | Yes | Target name |
| `type` | No | Target type |
| `standard` | No | C++ standard |
| `sources` | Yes | Source files |
| `include_dirs` | No | Include directories |
| `defines` | No | Preprocessor definitions |
| `links` | No | Libraries or CMake targets to link |
| `compile_options` | No | Compiler flags |
| `link_options` | No | Linker flags |
| `compile_features` | No | CMake compile features |
| `packages` | No | Packages passed to `find_package(...)` |
| `resources` | No | Files or directories copied after build |
| `modules` | No | Internal application modules under `modules/` |
| `output_dir` | No | Output directory inside the build tree |

## name

Required.

The `name` field defines the target name.

```ini
name = hello
```

Vix uses this name as the default build target.

For example:

```ini
name = hello
type = executable
```

generates a target named `hello`.

This name is also used by `vix run` to find the executable after building.

Recommended rules:

```txt
- Use letters, numbers, `_`, or `-`
- Avoid spaces
- Keep the name short and stable
```

Good examples:

```ini
name = hello
```

```ini
name = mathlib
```

```ini
name = softadastra_server
```

Avoid:

```ini
name = "my app"
```

## type

Optional.

Default:

```ini
type = executable
```

The `type` field defines what kind of target Vix should generate.

Supported values:

```ini
type = executable
```

```ini
type = static
```

```ini
type = static-library
```

```ini
type = shared
```

```ini
type = shared-library
```

```ini
type = library
```

### executable

Creates an executable application.

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

### static

Creates a static library.

```ini
name = mathlib
type = static
standard = c++20

sources = [
  src/add.cpp,
  src/mul.cpp,
]
```

### shared

Creates a shared library.

```ini
name = plugin
type = shared
standard = c++20

sources = [
  src/plugin.cpp,
]
```

### library

Creates a library target using the default library behavior supported by Vix.

```ini
name = core
type = library
standard = c++20

sources = [
  src/core.cpp,
]
```

## standard

Optional.

Default:

```ini
standard = c++20
```

The `standard` field defines the C++ language standard.

Supported values:

```ini
standard = c++17
```

```ini
standard = c++20
```

```ini
standard = c++23
```

Example:

```ini
name = hello
type = executable
standard = c++23

sources = [
  src/main.cpp,
]
```

This sets the generated CMake target to use the selected C++ standard.

## sources

Required.

The `sources` field lists the source files used by the target.

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
]
```

Source paths are relative to the directory containing `vix.app`.

Example layout:

```txt
myapp/
  vix.app
  src/
    main.cpp
    app.cpp
```

Manifest:

```ini
name = myapp
type = executable
standard = c++20

sources = [
  src/main.cpp,
  src/app.cpp,
]
```

Inline array syntax is also supported:

```ini
sources = [src/main.cpp, src/app.cpp]
```

If a path contains spaces, quote it:

```ini
sources = [
  "src/my file.cpp",
]
```

If a source file does not exist, Vix reports an error before generating the CMake project.

## include_dirs

Optional.

The `include_dirs` field lists include directories.

```ini
include_dirs = [
  include,
  third_party/asio/include,
]
```

Example layout:

```txt
myapp/
  vix.app
  include/
    myapp/
      app.hpp
  src/
    main.cpp
```

Manifest:

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

C++ usage:

```cpp
#include <myapp/app.hpp>
```

Include paths are relative to the directory containing `vix.app`.

## defines

Optional.

The `defines` field adds preprocessor definitions.

```ini
defines = [
  MYAPP_VERSION="1.0.0",
  MYAPP_ENABLE_LOGGING=1,
]
```

Example:

```ini
name = myapp
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

defines = [
  MYAPP_VERSION="1.0.0",
  MYAPP_DEBUG=1,
]
```

C++ usage:

```cpp
#ifndef MYAPP_VERSION
#define MYAPP_VERSION "unknown"
#endif
```

Definitions are passed to the generated target.

## links

Optional.

The `links` field lists libraries or CMake targets to link.

```ini
links = [
  Threads::Threads,
  fmt::fmt,
  m,
]
```

Use `links` for:

```txt
- CMake imported targets
- system libraries
- local library targets
- simple linker library names
```

Example:

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

Important:

```txt
packages finds packages.
links links targets or libraries.
```

`packages` does not link automatically.

## modules

Optional.

The `modules` field lists internal application modules created with `vix modules`.

```ini
modules = [
  auth,
  live_chat,
]
```

Each name maps to a folder under `modules/`:

```txt
modules/auth
modules/live_chat
```

When Vix generates the internal app CMake project, it includes `cmake/vix_modules.cmake`, loads the enabled modules, and links their aliases to the application target.

For a project named `api`, these modules expose aliases like:

```txt
api::auth
api::live_chat
```

Do not use `modules` for registry packages. Use `deps` for registry packages, `packages` for `find_package(...)`, and `links` for CMake targets.

### WebSocket modules in `vix.app`

WebSocket application modules are generated with:

```bash
vix modules add live_chat --websocket --workflow attached
```

or with an explicit name option:

```bash
vix modules add --websocket --name live_chat
```

The generated module has a `vix.module` manifest. Vix reads this manifest to detect module metadata such as:

```ini
workflow = "websocket.attached"
runtime = true
```

Runtime WebSocket workflows are wired through the generated `vix_app_modules.cpp` file. Vix supports one runtime application module at a time.

The `websocket.client` workflow is not treated as the app runtime. It can register lightweight routes and support code while the normal application runtime remains in control.

## compile_options

Optional.

The `compile_options` field adds compiler options to the target.

```ini
compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]
```

Example:

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

You can also write:

```ini
compile_options = [-Wall, -Wextra, -Wpedantic]
```

Compiler options can be platform-specific.

For example, GCC or Clang options may not work with MSVC.

## link_options

Optional.

The `link_options` field adds linker options to the target.

```ini
link_options = [
  "-Wl,--as-needed",
]
```

Quote values that contain commas:

```ini
link_options = [
  "-Wl,--as-needed",
]
```

Example:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

link_options = [
  "-Wl,--as-needed",
]
```

Linker options are toolchain-specific.

## compile_features

Optional.

The `compile_features` field adds CMake compile features to the target.

```ini
compile_features = [
  cxx_std_20,
]
```

Example:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

compile_features = [
  cxx_std_20,
]
```

Common values:

```txt
cxx_std_17
cxx_std_20
cxx_std_23
```

In most projects, `standard` is enough.

Use `compile_features` when you need explicit CMake feature declarations.

## packages

Optional.

The `packages` field generates `find_package(...)` calls in the internal CMake project.

```ini
packages = [
  Threads:REQUIRED,
  fmt:REQUIRED,
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]
```

Supported forms:

```txt
<name>
<name>:REQUIRED
<name>:COMPONENTS=a,b
<name>:COMPONENTS=a,b:REQUIRED
```

Examples:

```ini
packages = [
  Threads,
]
```

```ini
packages = [
  Threads:REQUIRED,
]
```

```ini
packages = [
  "Boost:COMPONENTS=system,filesystem",
]
```

```ini
packages = [
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]
```

Important:

```txt
packages only calls find_package(...).
It does not link the imported targets automatically.
```

You must link imported targets through `links`.

Correct:

```ini
packages = [
  fmt:REQUIRED,
]

links = [
  fmt::fmt,
]
```

Incorrect:

```ini
packages = [
  fmt:REQUIRED,
]
```

The incorrect example finds `fmt`, but does not link `fmt::fmt`.

## resources

Optional.

The `resources` field copies files or directories next to the built target after a successful build.

```ini
resources = [
  assets,
  "data/config.json=config/config.json",
]
```

Supported forms:

```txt
<src>
<src>=<dest>
```

### Copy with basename

```ini
resources = [
  assets,
]
```

This copies `assets` next to the built target.

### Copy with custom destination

```ini
resources = [
  "data/config.json=config/config.json",
]
```

This copies:

```txt
data/config.json
```

to:

```txt
config/config.json
```

next to the built target.

Common use cases:

```txt
- assets
- config files
- templates
- public files
- development certificates
```

## output_dir

Optional.

The `output_dir` field controls where the built target is placed inside the build tree.

```ini
output_dir = bin
```

Example:

```ini
name = hello
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]
```

After building, the executable can be placed under:

```txt
build-ninja/bin/hello
```

If `output_dir` is relative, Vix resolves it relative to the CMake build directory.

For example:

```ini
output_dir = bin
```

means:

```txt
${CMAKE_BINARY_DIR}/bin
```

## Complete example

```ini
name = myapp
type = executable
standard = c++23
output_dir = bin

sources = [
  src/main.cpp,
  src/app.cpp,
  src/network/client.cpp,
]

include_dirs = [
  include,
  third_party/asio/include,
]

defines = [
  MYAPP_VERSION="1.2.3",
  MYAPP_ENABLE_LOGGING=1,
]

packages = [
  Threads:REQUIRED,
  fmt:REQUIRED,
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]

links = [
  Threads::Threads,
  fmt::fmt,
  Boost::system,
  Boost::filesystem,
]

compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]

link_options = [
  "-Wl,--as-needed",
]

compile_features = [
  cxx_std_23,
]

resources = [
  assets,
  "data/config.json=config/config.json",
]
```

## Minimal example

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

## Static library example

```ini
name = mathlib
type = static
standard = c++17

sources = [
  src/add.cpp,
  src/mul.cpp,
]

include_dirs = [
  include,
]
```

## Shared library example

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

## Common mistakes

### Missing sources

Incorrect:

```ini
name = hello
type = executable
standard = c++20
```

Correct:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

### Package found but target not linked

`packages` only calls `find_package(...)`.

You still need to link the imported target:

```ini
packages = [fmt:REQUIRED]
links = [fmt::fmt]
```

### Source path is wrong

Incorrect:

```ini
sources = [
  main.cpp,
]
```

when your file is actually here:

```txt
src/main.cpp
```

Correct:

```ini
sources = [
  src/main.cpp,
]
```

### CMakeLists.txt exists

If your project has both:

```txt
CMakeLists.txt
vix.app
```

Vix uses `CMakeLists.txt`.

To use `vix.app`, remove or rename `CMakeLists.txt`.

## Next steps

Continue with:

- [Examples](./examples.md)
- [Packages and Links](./packages-and-links.md)
- [Sources and Includes](./sources-and-includes.md)
- [Troubleshooting](./troubleshooting.md)
