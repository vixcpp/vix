# Best Practices

This page gives practical recommendations for writing clean `vix.app` projects.

The main idea is simple:

```txt
Keep vix.app small, explicit, and predictable.
```

`vix.app` is designed to describe one clear C++ target without forcing users to write a full `CMakeLists.txt`.

## Use one manifest per target

`vix.app` is intentionally simple.

Recommended:

```txt
one vix.app = one target
```

Good structure:

```txt
myapp/
  vix.app
  src/
    main.cpp
```

For tests:

```txt
myapp/
  tests/
    vix.app
    test_app.cpp
```

For examples:

```txt
myapp/
  examples/
    basic/
      vix.app
      src/
        main.cpp
```

This keeps each target easy to understand.

## Keep the manifest readable

Prefer multi-line arrays for real projects.

Recommended:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
  src/server.cpp,
]

include_dirs = [
  include,
]
```

Avoid very long inline arrays:

```ini
sources = [src/main.cpp, src/app.cpp, src/server.cpp, src/router.cpp, src/db.cpp]
```

Inline arrays are fine for very small projects:

```ini
sources = [src/main.cpp]
```

## Use a stable project layout

Recommended application layout:

```txt
myapp/
  vix.app
  include/
    myapp/
      app.hpp
  src/
    main.cpp
    app.cpp
  assets/
    config.json
```

Recommended library layout:

```txt
mathlib/
  vix.app
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
    mul.cpp
  tests/
    vix.app
    test_math.cpp
```

Recommended examples layout:

```txt
mathlib/
  examples/
    basic/
      vix.app
      src/
        main.cpp
```

## Keep main.cpp small

For applications, avoid putting all logic in `main.cpp`.

Prefer:

```cpp
#include <myapp/app.hpp>

int main()
{
  return myapp::run();
}
```

Then put the real logic in:

```txt
src/app.cpp
include/myapp/app.hpp
```

This makes the project easier to test.

## Use include/project_name/ for public headers

Recommended:

```txt
include/
  myapp/
    app.hpp
    config.hpp
```

Then include headers like this:

```cpp
#include <myapp/app.hpp>
```

Avoid this for larger projects:

```txt
include/
  app.hpp
```

Namespaced include paths reduce collisions with other libraries.

## Use src/ for implementation

Recommended:

```txt
src/
  main.cpp
  app.cpp
  server.cpp
```

Public headers should usually go under:

```txt
include/
```

Private implementation headers can go under:

```txt
src/
```

Example:

```txt
src/
  detail/
    parser.hpp
```

If your source includes private headers from `src/`, add:

```ini
include_dirs = [
  include,
  src,
]
```

## Do not list headers as sources

Usually, do not do this:

```ini
sources = [
  src/main.cpp,
  include/myapp/app.hpp,
]
```

Prefer:

```ini
sources = [
  src/main.cpp,
]

include_dirs = [
  include,
]
```

Headers are included by the compiler through `include_dirs`.

## Use clear target names

The `name` field should be stable and simple.

Recommended:

```ini
name = myapp
```

```ini
name = mathlib
```

```ini
name = my_app
```

Avoid:

```ini
name = "my app"
```

Use only simple characters:

```txt
letters
numbers
_
-
```

The target name is used by `vix build` and `vix run`.

## Choose the correct target type

Use:

```ini
type = executable
```

for applications.

Use:

```ini
type = static
```

for static libraries.

Use:

```ini
type = shared
```

for shared libraries.

Recommended rule:

```txt
If it has main(), use executable.
If it is reusable code, use static or shared.
```

## Prefer explicit type

Even though `executable` can be the default, it is clearer to write it explicitly:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

This makes the manifest easier to read.

## Use standard for the C++ version

Prefer:

```ini
standard = c++20
```

or:

```ini
standard = c++23
```

For most projects, this is enough.

Use `compile_features` only when you need explicit CMake compile features:

```ini
compile_features = [
  cxx_std_20,
]
```

## Keep compile options minimal

A good starting point for GCC and Clang:

```ini
compile_options = [
  -Wall,
  -Wextra,
]
```

For stricter projects:

```ini
compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]
```

Avoid adding too many compiler-specific flags in a project meant to be portable.

## Do not put linker flags in compile_options

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

Use:

```ini
compile_options
```

for compiler flags.

Use:

```ini
link_options
```

for linker flags.

## Use packages and links together

Remember the rule:

```txt
packages -> find_package(...)
links    -> target_link_libraries(...)
```

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

`packages` finds the package, but it does not link the imported target automatically.

## Quote package values with commas

Correct:

```ini
packages = [
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]
```

Incorrect:

```ini
packages = [
  Boost:COMPONENTS=system,filesystem:REQUIRED,
]
```

Commas can split array items, so quote package values that contain commas.

## Use output_dir for apps

For executable projects, this is a good default:

```ini
output_dir = bin
```

Example:

```ini
name = myapp
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]
```

This gives a predictable output location:

```txt
build-ninja/bin/myapp
```

## Use output_dir for libraries when needed

For libraries, this can be useful:

```ini
output_dir = lib
```

Example:

```ini
name = mathlib
type = static
standard = c++20
output_dir = lib

sources = [
  src/add.cpp,
]
```

Output can be placed under:

```txt
build-ninja/lib/
```

## Keep resources close to the app

Recommended:

```txt
myapp/
  assets/
  config/
  public/
```

Manifest:

```ini
resources = [
  assets,
  config,
]
```

If you use:

```ini
output_dir = bin
```

resources are copied next to the executable under:

```txt
build-ninja/bin/
```

## Use custom resource destinations when needed

Example:

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

## Avoid absolute paths

Prefer relative paths:

```ini
sources = [
  src/main.cpp,
]

include_dirs = [
  include,
]

resources = [
  assets,
]
```

Avoid:

```ini
sources = [
  /home/user/project/src/main.cpp,
]
```

Relative paths make projects easier to move, share, and build on other machines.

## Quote paths with spaces

If a path contains spaces, quote it:

```ini
sources = [
  "src/with space.cpp",
]
```

```ini
resources = [
  "my assets",
]
```

Better practice: avoid spaces in source and resource paths when possible.

## Keep tests separate

Recommended:

```txt
myapp/
  vix.app
  tests/
    vix.app
    test_app.cpp
```

Root `vix.app`:

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

`tests/vix.app`:

```ini
name = myapp_tests
type = executable
standard = c++20

sources = [
  test_app.cpp,
  ../src/app.cpp,
]

include_dirs = [
  ../include,
]
```

This avoids complex multi-target syntax.

## Do not include main.cpp in tests

Incorrect:

```ini
sources = [
  test_app.cpp,
  ../src/main.cpp,
  ../src/app.cpp,
]
```

Correct:

```ini
sources = [
  test_app.cpp,
  ../src/app.cpp,
]
```

This avoids duplicate `main()` errors.

## Keep examples separate

Recommended:

```txt
examples/
  hello/
    vix.app
    src/
      main.cpp
  threads/
    vix.app
    src/
      main.cpp
```

Each example can be built and run independently:

```bash
cd examples/hello
vix run
```

## Use CMakeLists.txt for complex projects

Use `vix.app` for simple and medium projects.

Use `CMakeLists.txt` when you need full build-system control.

Examples:

```txt
- multiple targets in one project
- generated source files
- custom commands
- install rules
- CTest
- FetchContent
- CPM.cmake
- custom toolchains
- package export files
- advanced platform-specific logic
```

Do not force complex CMake logic into `vix.app`.

## Do not edit generated CMake

For `vix.app` projects, Vix generates:

```txt
.vix/generated/app/CMakeLists.txt
```

Do not edit this file manually.

Edit:

```txt
vix.app
```

The generated CMake file can be overwritten by Vix.

## Do not commit generated files

Usually, commit:

```txt
vix.app
```

Do not commit:

```txt
.vix/generated/app/CMakeLists.txt
```

Recommended `.gitignore`:

```txt
.vix/generated/
build-dev/
build-ninja/
build-release/
```

## Start simple

A good first `vix.app` should look like this:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

Add fields only when needed.

For example, do not add `packages`, `links`, `resources`, or `output_dir` until the project actually needs them.

## Recommended full app manifest

```ini
name = myapp
type = executable
standard = c++20
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

compile_options = [
  -Wall,
  -Wextra,
]

resources = [
  assets,
  config,
]
```

## Recommended full library manifest

```ini
name = mathlib
type = static
standard = c++20
output_dir = lib

sources = [
  src/add.cpp,
  src/mul.cpp,
]

include_dirs = [
  include,
]

compile_options = [
  -Wall,
  -Wextra,
]
```

## Recommended test manifest

```ini
name = mathlib_tests
type = executable
standard = c++20
output_dir = bin

sources = [
  test_math.cpp,
  ../src/add.cpp,
  ../src/mul.cpp,
]

include_dirs = [
  ../include,
]

compile_options = [
  -Wall,
  -Wextra,
]
```

## Recommended package pattern

```ini
packages = [
  Threads:REQUIRED,
  fmt:REQUIRED,
]

links = [
  Threads::Threads,
  fmt::fmt,
]
```

Keep package discovery and linking explicit.

## Recommended project templates

### Simple app

```txt
hello/
  vix.app
  src/
    main.cpp
```

### App with logic

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

### App with resources

```txt
myapp/
  vix.app
  include/
    myapp/
      app.hpp
  src/
    main.cpp
    app.cpp
  assets/
  config/
```

### Library with tests

```txt
mathlib/
  vix.app
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
    mul.cpp
  tests/
    vix.app
    test_math.cpp
```

### Library with examples

```txt
mathlib/
  vix.app
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
    mul.cpp
  examples/
    basic/
      vix.app
      src/
        main.cpp
```

## Build commands

Common commands:

```bash
vix build
```

```bash
vix run
```

Release build:

```bash
vix build --preset release
```

Clean build:

```bash
vix build --clean
```

Verbose build:

```bash
vix build -v
```

Raw CMake configure output:

```bash
vix build --cmake-verbose
```

Pass extra CMake variables:

```bash
vix build -- -DCMAKE_PREFIX_PATH=/path/to/prefix
```

## Review checklist

Before committing a `vix.app`, check:

```txt
1. Is name simple and stable?
2. Is type correct?
3. Is standard correct?
4. Do all sources exist?
5. Are include_dirs correct?
6. Are packages and links both present when using imported targets?
7. Are resources needed and correctly named?
8. Is output_dir useful for this target?
9. Are tests in a separate manifest?
10. Is CMakeLists.txt absent if you expect Vix to use vix.app?
```

## Summary

Best practices:

```txt
- keep one target per vix.app
- use src/ and include/
- keep main.cpp small
- use tests/vix.app for tests
- use examples/<name>/vix.app for examples
- keep packages and links separate
- use output_dir = bin for apps
- do not edit generated CMake
- use CMakeLists.txt for advanced builds
```

`vix.app` should stay simple.

CMake remains available when the project needs full control.

## Next steps

Continue with:

- [Getting Started](./getting-started.md)
- [Examples](./examples.md)
- [Troubleshooting](./troubleshooting.md)
- [CMake Fallback](./cmake-fallback.md)
