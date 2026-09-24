# Compile Options

`vix.app` supports common compile-related settings for C++ projects.

The main fields are:

```ini
standard = c++20
```

```ini
defines = [
  MYAPP_VERSION="1.0.0",
]
```

```ini
compile_options = [
  -Wall,
  -Wextra,
]
```

```ini
compile_features = [
  cxx_std_20,
]
```

Use these fields to control the C++ standard, preprocessor definitions, compiler warnings, and CMake compile features.

## C++ standard

The `standard` field defines the C++ standard used by the target.

Example:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

Supported values include:

```ini
standard = c++17
```

```ini
standard = c++20
```

```ini
standard = c++23
```

Default:

```ini
standard = c++20
```

## Example with C++20

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

#include <span>
#include <vector>

int main()
{
  std::vector<int> values{1, 2, 3};
  std::span<int> view(values);

  vix::print("size:", view.size());
  return 0;
}
```

Build and run:

```bash
vix build
vix run
```

## Example with C++23

`vix.app`:

```ini
name = modern_app
type = executable
standard = c++23

sources = [
  src/main.cpp,
]
```

Use `c++23` when your compiler and standard library support the features you need.

## defines

The `defines` field adds preprocessor definitions to the target.

Example:

```ini
defines = [
  MYAPP_VERSION="1.0.0",
  MYAPP_ENABLE_LOGGING=1,
]
```

Complete manifest:

```ini
name = config_app
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

defines = [
  MYAPP_VERSION="1.0.0",
  MYAPP_ENABLE_LOGGING=1,
]
```

C++ usage:

```cpp
#include <vix.hpp>

#ifndef MYAPP_VERSION
#define MYAPP_VERSION "unknown"
#endif

int main()
{
  vix::print("version:", MYAPP_VERSION);

#ifdef MYAPP_ENABLE_LOGGING
  vix::print("logging enabled");
#endif

  return 0;
}
```

## Define values

Definitions can be simple flags:

```ini
defines = [
  MYAPP_DEBUG,
]
```

or key-value pairs:

```ini
defines = [
  MYAPP_VERSION="1.0.0",
  MYAPP_LEVEL=2,
]
```

## Quoting string defines

When a definition contains a string value, include the quotes as part of the value:

```ini
defines = [
  MYAPP_VERSION="1.0.0",
]
```

Then use it in C++:

```cpp
vix::print(MYAPP_VERSION);
```

## compile_options

The `compile_options` field adds compiler flags to the target.

Example:

```ini
compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]
```

Complete manifest:

```ini
name = warnings_app
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

## Common GCC and Clang warnings

For GCC or Clang, a useful starting point is:

```ini
compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]
```

For stricter builds:

```ini
compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
  -Wconversion,
  -Wshadow,
]
```

Use strict warnings carefully. Some flags can be noisy depending on your project.

## Optimization flags

You can pass optimization flags:

```ini
compile_options = [
  -O2,
]
```

or for debugging:

```ini
compile_options = [
  -g,
]
```

However, prefer Vix presets for normal debug and release workflows:

```bash
vix build
```

```bash
vix build --preset release
```

Use `compile_options` when you need target-specific flags.

## Platform-specific options

Compiler flags are not always portable.

For example:

```ini
compile_options = [
  -Wall,
  -Wextra,
]
```

works well with GCC and Clang, but not necessarily with MSVC.

For portable projects, avoid compiler-specific flags unless you control the build environment.

## link_options

Although this page focuses on compile options, linker options are related.

Use `link_options` for linker flags:

```ini
link_options = [
  "-Wl,--as-needed",
]
```

Do not put linker flags in `compile_options`.

Correct:

```ini
link_options = [
  "-Wl,--as-needed",
]
```

Incorrect:

```ini
compile_options = [
  "-Wl,--as-needed",
]
```

## compile_features

The `compile_features` field adds CMake compile features to the generated target.

Example:

```ini
compile_features = [
  cxx_std_20,
]
```

Complete manifest:

```ini
name = feature_app
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

## standard vs compile_features

For most projects, use:

```ini
standard = c++20
```

This is enough.

Use `compile_features` when you specifically need CMake-style feature declarations.

Example:

```ini
standard = c++20

compile_features = [
  cxx_std_20,
]
```

This is explicit, but often redundant.

## Recommended simple setup

For most Vix projects:

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

compile_options = [
  -Wall,
  -Wextra,
]
```

This gives a clean development setup without making the manifest too complex.

## Debug-style setup

For local development:

```ini
name = debug_app
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

defines = [
  DEBUG=1,
]

compile_options = [
  -Wall,
  -Wextra,
  -g,
]
```

Build:

```bash
vix build
```

## Release-style setup

For release builds, prefer:

```bash
vix build --preset release
```

You can still add target-specific optimization options:

```ini
compile_options = [
  -O2,
]
```

But avoid hardcoding release-only flags in the manifest if you want the same `vix.app` to work well for both debug and release.

## Sanitizers

For simple script mode, Vix has sanitizer flags such as:

```bash
vix run main.cpp --san
```

For `vix.app` projects, sanitizer integration should usually be handled through build configuration or CMake arguments.

Example:

```bash
vix build -- -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
```

For advanced sanitizer workflows, use a normal `CMakeLists.txt`.

## Warnings as errors

You can enable warnings as errors:

```ini
compile_options = [
  -Wall,
  -Wextra,
  -Werror,
]
```

Use this carefully.

It can be useful in CI, but it can make local development harder when compilers produce different warnings.

## Per-project flags

`compile_options` applies to the generated target.

Example:

```ini
name = server
type = executable
standard = c++20

sources = [
  src/main.cpp,
  src/server.cpp,
]

compile_options = [
  -Wall,
  -Wextra,
]
```

This is target-specific, not global to every CMake project on your machine.

## Inline syntax

For small projects, inline syntax is supported:

```ini
compile_options = [-Wall, -Wextra]
defines = [DEBUG=1]
compile_features = [cxx_std_20]
```

For larger projects, prefer multi-line syntax:

```ini
compile_options = [
  -Wall,
  -Wextra,
]

defines = [
  DEBUG=1,
]

compile_features = [
  cxx_std_20,
]
```

Multi-line syntax is easier to read and review.

## Complete example

```ini
name = myapp
type = executable
standard = c++23

sources = [
  src/main.cpp,
  src/app.cpp,
]

include_dirs = [
  include,
]

defines = [
  MYAPP_VERSION="1.2.3",
  MYAPP_ENABLE_LOGGING=1,
]

compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]

compile_features = [
  cxx_std_23,
]
```

## Common mistakes

### Using unsupported standard values

Incorrect:

```ini
standard = cpp20
```

Correct:

```ini
standard = c++20
```

### Putting linker flags in compile_options

Incorrect:

```ini
compile_options = [
  "-Wl,--as-needed",
]
```

Correct:

```ini
link_options = [
  "-Wl,--as-needed",
]
```

### Forgetting quotes for string defines

Incorrect:

```ini
defines = [
  MYAPP_VERSION=1.0.0,
]
```

This may not behave like a C++ string.

Correct:

```ini
defines = [
  MYAPP_VERSION="1.0.0",
]
```

### Using compiler-specific flags everywhere

This may work on GCC or Clang:

```ini
compile_options = [
  -Wall,
  -Wextra,
]
```

But it may not work on every compiler.

For portable projects, keep flags minimal.

## Recommended patterns

### Simple app

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

### App with warnings

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
]
```

### App with defines

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

defines = [
  HELLO_VERSION="1.0.0",
]
```

### App with explicit CMake feature

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

## Summary

Use:

```ini
standard = c++20
```

to select the C++ standard.

Use:

```ini
defines = [
  NAME=value,
]
```

for preprocessor definitions.

Use:

```ini
compile_options = [
  -Wall,
]
```

for compiler flags.

Use:

```ini
compile_features = [
  cxx_std_20,
]
```

only when you need explicit CMake compile features.

## Next steps

Continue with:

- [Resources](./resources.md)
- [Output Directory](./output-directory.md)
- [Packages and Links](./packages-and-links.md)
- [Best Practices](./best-practices.md)
