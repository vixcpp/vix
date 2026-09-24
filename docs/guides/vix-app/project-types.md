# Project Types

`vix.app` supports the most common C++ target types.

A target type tells Vix what kind of build output should be generated.

```ini
type = executable
```

```ini
type = static
```

```ini
type = shared
```

```ini
type = library
```

For most projects, you will use either:

```txt
executable -> application
static     -> static library
shared     -> shared library
```

## Default type

If `type` is not provided, Vix uses:

```ini
type = executable
```

So this:

```ini
name = hello
standard = c++20

sources = [
  src/main.cpp,
]
```

is equivalent to:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

## Executable projects

Use `executable` when your project builds an application.

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

This creates an executable target named:

```txt
hello
```

Typical layout:

```txt
hello/
  vix.app
  src/
    main.cpp
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

Build and run:

```bash
vix build
vix run
```

## Executable with multiple files

A real application usually has more than one source file.

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

`vix.app`:

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

`include/myapp/app.hpp`:

```cpp
#pragma once

namespace myapp
{
  int run();
}
```

`src/app.cpp`:

```cpp
#include <vix.hpp>
#include <myapp/app.hpp>

namespace myapp
{
  int run()
  {
    vix::print("myapp running");
    return 0;
  }
}
```

`src/main.cpp`:

```cpp
#include <myapp/app.hpp>

int main()
{
  return myapp::run();
}
```

## Static library projects

Use `static` when your project builds a static library.

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

This creates a static library target named:

```txt
mathlib
```

Typical layout:

```txt
mathlib/
  vix.app
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
    mul.cpp
```

`include/mathlib/math.hpp`:

```cpp
#pragma once

namespace mathlib
{
  int add(int a, int b);
  int mul(int a, int b);
}
```

`src/add.cpp`:

```cpp
#include <mathlib/math.hpp>

namespace mathlib
{
  int add(int a, int b)
  {
    return a + b;
  }
}
```

`src/mul.cpp`:

```cpp
#include <mathlib/math.hpp>

namespace mathlib
{
  int mul(int a, int b)
  {
    return a * b;
  }
}
```

Build:

```bash
vix build
```

## Static library aliases

The following values are accepted for static libraries:

```ini
type = static
```

```ini
type = static-library
```

Use `static` for shorter manifests.

Recommended:

```ini
type = static
```

## Shared library projects

Use `shared` when your project builds a shared library.

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

This creates a shared library target named:

```txt
plugin
```

Typical layout:

```txt
plugin/
  vix.app
  include/
    plugin/
      plugin.hpp
  src/
    plugin.cpp
```

`include/plugin/plugin.hpp`:

```cpp
#pragma once

namespace plugin
{
  const char *name();
}
```

`src/plugin.cpp`:

```cpp
#include <plugin/plugin.hpp>

namespace plugin
{
  const char *name()
  {
    return "plugin";
  }
}
```

Build:

```bash
vix build
```

## Shared library aliases

The following values are accepted for shared libraries:

```ini
type = shared
```

```ini
type = shared-library
```

Use `shared` for shorter manifests.

Recommended:

```ini
type = shared
```

## Library projects

`library` is also supported.

```ini
name = core
type = library
standard = c++20

sources = [
  src/core.cpp,
]

include_dirs = [
  include,
]
```

Use `library` when you want the default library behavior supported by Vix.

For clearer intent, prefer one of these when possible:

```ini
type = static
```

or:

```ini
type = shared
```

## Choosing the right type

Use this rule:

```txt
executable -> the target has main()
static     -> reusable code linked into another target
shared     -> dynamic library loaded or linked at runtime
library    -> generic library target
```

## Executable vs library

An executable usually has a `main()` function.

Example:

```cpp
int main()
{
  return 0;
}
```

A library usually does not have `main()`.

Example:

```cpp
namespace mathlib
{
  int add(int a, int b)
  {
    return a + b;
  }
}
```

If you put `main()` in a static or shared library target, it usually means your project structure should be changed.

## Testing a library

For `vix.app` V1, use a separate test manifest.

Example:

```txt
mathlib/
  vix.app
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
  tests/
    vix.app
    test_add.cpp
```

Root `vix.app`:

```ini
name = mathlib
type = static
standard = c++20

sources = [
  src/add.cpp,
]

include_dirs = [
  include,
]
```

`tests/vix.app`:

```ini
name = mathlib_tests
type = executable
standard = c++20

sources = [
  test_add.cpp,
  ../src/add.cpp,
]

include_dirs = [
  ../include,
]
```

Run tests:

```bash
cd tests
vix run
```

## Application with internal library-style code

For applications, it is better to keep `main.cpp` small.

Recommended:

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

`vix.app`:

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

`src/main.cpp` should only start the application:

```cpp
#include <myapp/app.hpp>

int main()
{
  return myapp::run();
}
```

This makes your project easier to test.

## Examples as separate targets

Since `vix.app` describes one target, the recommended approach is one folder per example.

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
  resources/
    vix.app
    src/
      main.cpp
    assets/
      config.json
```

Build one example:

```bash
cd examples/hello
vix run
```

This keeps each example independent and simple.

## Multiple targets

`vix.app` is intentionally simple.

It describes one target.

For multiple targets, use one of these approaches:

```txt
- one folder with one vix.app per target
- a normal CMakeLists.txt for full multi-target control
```

Recommended for simple projects:

```txt
workspace/
  apps/
    server/
      vix.app
      src/
        main.cpp
    client/
      vix.app
      src/
        main.cpp
  libs/
    mathlib/
      vix.app
      src/
        add.cpp
      include/
        mathlib/
          math.hpp
```

For advanced multi-target projects, use `CMakeLists.txt`.

## Type and output name

The `name` field defines the target name.

The `type` field defines what kind of target is generated.

Example:

```ini
name = hello
type = executable
```

This creates an executable target named:

```txt
hello
```

Example:

```ini
name = mathlib
type = static
```

This creates a static library target named:

```txt
mathlib
```

## Type and vix run

`vix run` is mainly useful for executable targets.

For this manifest:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

you can run:

```bash
vix run
```

For this manifest:

```ini
name = mathlib
type = static
standard = c++20

sources = [
  src/add.cpp,
]
```

`vix build` is the main command:

```bash
vix build
```

If you want to run tests for a library, create a test executable under `tests/vix.app`.

## Type and resources

Resources are usually useful for executables.

Example:

```ini
name = asset_app
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

For libraries, resources are less common.

## Type and packages

All project types can use packages and links.

Executable:

```ini
name = threaded_app
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

Static library:

```ini
name = core
type = static
standard = c++20

sources = [
  src/core.cpp,
]

packages = [
  Threads:REQUIRED,
]

links = [
  Threads::Threads,
]
```

Shared library:

```ini
name = plugin
type = shared
standard = c++20

sources = [
  src/plugin.cpp,
]

packages = [
  Threads:REQUIRED,
]

links = [
  Threads::Threads,
]
```

## Common mistakes

### Using executable for a library

Incorrect:

```ini
name = mathlib
type = executable

sources = [
  src/add.cpp,
]
```

If `src/add.cpp` does not provide `main()`, the link step will fail.

Correct:

```ini
name = mathlib
type = static

sources = [
  src/add.cpp,
]
```

### Using static for an app

Incorrect:

```ini
name = hello
type = static

sources = [
  src/main.cpp,
]
```

If your target is meant to run, use:

```ini
type = executable
```

### Testing by adding main.cpp twice

Incorrect test manifest:

```ini
sources = [
  test_app.cpp,
  ../src/main.cpp,
  ../src/app.cpp,
]
```

This can cause duplicate `main()` errors.

Correct:

```ini
sources = [
  test_app.cpp,
  ../src/app.cpp,
]
```

## Recommended patterns

### CLI application

```txt
cli-tool/
  vix.app
  src/
    main.cpp
```

```ini
name = cli_tool
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

### Application with internal logic

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

### Static library

```txt
mathlib/
  vix.app
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
    mul.cpp
```

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

### Shared library

```txt
plugin/
  vix.app
  include/
    plugin/
      plugin.hpp
  src/
    plugin.cpp
```

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

## When to use CMakeLists.txt

Use `CMakeLists.txt` when you need:

```txt
- many targets in one project
- complex target dependencies
- generated source files
- custom commands
- CTest integration
- install rules
- advanced packaging
- platform-specific build logic
```

`vix.app` is best for one clear target.

CMake is still the compatibility path for advanced build systems.

## Summary

```txt
type = executable
  build an application

type = static
  build a static library

type = shared
  build a shared library

type = library
  build a generic library target
```

Recommended rule:

```txt
Use vix.app for one simple target.
Use CMakeLists.txt for complex multi-target projects.
```

## Next steps

Continue with:

- [Sources and Includes](./sources-and-includes.md)
- [Compile Options](./compile-options.md)
- [Libraries](./libraries.md)
- [Tests](./tests.md)
