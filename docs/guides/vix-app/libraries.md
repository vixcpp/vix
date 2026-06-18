# Libraries

`vix.app` can build simple C++ libraries.

Supported library target types:

```ini
type = static
```

```ini
type = shared
```

```ini
type = library
```

For most projects, prefer explicit types:

```txt
static -> static library
shared -> shared library
```

## Static library

A static library is linked into another executable or library at build time.

Use:

```ini
type = static
```

Example:

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

## Static library layout

Recommended layout:

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

## Shared library

A shared library is loaded or linked dynamically at runtime.

Use:

```ini
type = shared
```

Example:

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

## Shared library layout

Recommended layout:

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

## Generic library

`library` is also supported:

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

Use `library` when you want Vix to use its default library behavior.

For clearer manifests, prefer:

```ini
type = static
```

or:

```ini
type = shared
```

## Header-only libraries

A header-only library usually does not need `sources`.

However, `vix.app` expects source files for a build target.

For a pure header-only library, the simplest V1 approach is usually:

```txt
- use it through include_dirs from another app
- or use a normal CMakeLists.txt for package-style library behavior
```

Example app using a header-only library:

```txt
myapp/
  vix.app
  third_party/
    tiny/
      include/
        tiny/
          tiny.hpp
  src/
    main.cpp
```

`vix.app`:

```ini
name = myapp
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

include_dirs = [
  third_party/tiny/include,
]
```

No `links` field is needed if the dependency is truly header-only.

## Library with public headers

For reusable libraries, keep public headers under `include/`.

Recommended:

```txt
mathlib/
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
```

Then users include:

```cpp
#include <mathlib/math.hpp>
```

Avoid putting public headers directly in `src/`.

`src/` should mainly contain implementation files and private headers.

## Library with private headers

You can use private headers under `src/`.

Example layout:

```txt
mathlib/
  vix.app
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
    detail/
      helpers.hpp
```

`vix.app`:

```ini
name = mathlib
type = static
standard = c++20

sources = [
  src/add.cpp,
]

include_dirs = [
  include,
  src,
]
```

Then `src/add.cpp` can include:

```cpp
#include <detail/helpers.hpp>
```

Public users should still include only:

```cpp
#include <mathlib/math.hpp>
```

## Library with compile definitions

You can add compile definitions to a library target.

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

defines = [
  MATHLIB_ENABLE_FAST_PATH=1,
]
```

C++ usage:

```cpp
#ifdef MATHLIB_ENABLE_FAST_PATH
// optimized implementation
#endif
```

## Library with compile options

You can add compiler options:

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

compile_options = [
  -Wall,
  -Wextra,
]
```

## Library with packages

Libraries can use packages and links.

Example with `Threads`:

```ini
name = workerlib
type = static
standard = c++20

sources = [
  src/worker.cpp,
]

include_dirs = [
  include,
]

packages = [
  Threads:REQUIRED,
]

links = [
  Threads::Threads,
]
```

Remember:

```txt
packages finds packages.
links links targets or libraries.
```

## Library with output_dir

Use `output_dir` to place the library under a predictable build folder.

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
```

After build, the library can be placed under:

```txt
build-ninja/lib/
```

The exact file name depends on the platform and toolchain.

Examples:

```txt
libmathlib.a
libmathlib.so
mathlib.lib
mathlib.dll
```

## Testing a library

For `vix.app` V1, use a separate test executable.

Recommended layout:

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

Root `vix.app`:

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

`tests/vix.app`:

```ini
name = mathlib_tests
type = executable
standard = c++20

sources = [
  test_math.cpp,
  ../src/add.cpp,
  ../src/mul.cpp,
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

## Why tests include library sources again

In `vix.app` V1, one manifest describes one target.

So the test executable can include the library implementation files directly:

```ini
sources = [
  test_math.cpp,
  ../src/add.cpp,
  ../src/mul.cpp,
]
```

This avoids introducing a complex multi-target syntax too early.

For advanced library and test setups, use `CMakeLists.txt`.

## Example library test

`tests/test_math.cpp`:

```cpp
#include <vix.hpp>
#include <mathlib/math.hpp>

int main()
{
  if (mathlib::add(2, 3) != 5)
  {
    vix::print("add failed");
    return 1;
  }

  if (mathlib::mul(4, 5) != 20)
  {
    vix::print("mul failed");
    return 1;
  }

  vix::print("all tests passed");
  return 0;
}
```

## Using a local library from an app

For simple V1 projects, the easiest approach is to include the library source files directly in the app target.

Example layout:

```txt
workspace/
  app/
    vix.app
    src/
      main.cpp
  libs/
    mathlib/
      include/
        mathlib/
          math.hpp
      src/
        add.cpp
```

`app/vix.app`:

```ini
name = calculator
type = executable
standard = c++20

sources = [
  src/main.cpp,
  ../libs/mathlib/src/add.cpp,
]

include_dirs = [
  ../libs/mathlib/include,
]
```

This is simple and works well for small projects.

For larger projects with several libraries, use a normal `CMakeLists.txt`.

## Shared library notes

Shared libraries are more platform-sensitive than static libraries.

Depending on the operating system, the output may be:

```txt
libplugin.so
libplugin.dylib
plugin.dll
```

Runtime loading and library search paths can also differ between platforms.

For simple shared libraries, `vix.app` is enough.

For advanced shared library packaging, use `CMakeLists.txt`.

## Library naming

The `name` field defines the target name.

Example:

```ini
name = mathlib
type = static
```

The target is named:

```txt
mathlib
```

The output file may include platform-specific prefixes or suffixes.

Examples:

```txt
libmathlib.a
libmathlib.so
mathlib.lib
```

## Library and vix run

`vix run` is mainly for executables.

For a library target:

```ini
name = mathlib
type = static
```

use:

```bash
vix build
```

To run something, create a test executable or example executable.

Example:

```txt
mathlib/
  examples/
    basic/
      vix.app
      src/
        main.cpp
```

`examples/basic/vix.app`:

```ini
name = mathlib_example
type = executable
standard = c++20

sources = [
  src/main.cpp,
  ../../src/add.cpp,
  ../../src/mul.cpp,
]

include_dirs = [
  ../../include,
]
```

Run:

```bash
cd examples/basic
vix run
```

## Examples for a library

Recommended layout:

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
  tests/
    vix.app
    test_math.cpp
```

This keeps each executable target separate:

```txt
root/vix.app              -> library
examples/basic/vix.app    -> example executable
tests/vix.app             -> test executable
```

## Example executable for a library

`examples/basic/vix.app`:

```ini
name = mathlib_example
type = executable
standard = c++20

sources = [
  src/main.cpp,
  ../../src/add.cpp,
  ../../src/mul.cpp,
]

include_dirs = [
  ../../include,
]
```

`examples/basic/src/main.cpp`:

```cpp
#include <vix.hpp>
#include <mathlib/math.hpp>

int main()
{
  vix::print("2 + 3 =", mathlib::add(2, 3));
  vix::print("4 * 5 =", mathlib::mul(4, 5));
  return 0;
}
```

Run:

```bash
cd examples/basic
vix run
```

## When to use CMakeLists.txt for libraries

Use `CMakeLists.txt` when your library needs:

```txt
- multiple targets in one project
- separate public/private dependencies
- install rules
- package export files
- CMake config generation
- FetchContent
- CTest integration
- complex examples
- complex dependency graphs
```

`vix.app` is best for simple libraries and small local workflows.

## Common mistakes

### Using executable for library code

Incorrect:

```ini
name = mathlib
type = executable

sources = [
  src/add.cpp,
]
```

If there is no `main()`, linking will fail.

Correct:

```ini
name = mathlib
type = static

sources = [
  src/add.cpp,
]
```

### Trying to run a library directly

This builds a library:

```ini
name = mathlib
type = static
```

Use:

```bash
vix build
```

not:

```bash
vix run
```

unless you have a test or example executable.

### Forgetting include_dirs

If your source uses:

```cpp
#include <mathlib/math.hpp>
```

and the file is here:

```txt
include/mathlib/math.hpp
```

you need:

```ini
include_dirs = [
  include,
]
```

### Including main.cpp in a library

A library target should usually not include `main.cpp`.

Keep `main.cpp` for executable targets.

## Recommended patterns

### Small static library

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

### Small shared library

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

### Library with tests

```txt
mathlib/
  vix.app
  include/
  src/
  tests/
    vix.app
    test_math.cpp
```

### Library with examples

```txt
mathlib/
  vix.app
  include/
  src/
  examples/
    basic/
      vix.app
      src/
        main.cpp
```

## Summary

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

Use:

```bash
vix build
```

to build library targets.

Use separate `vix.app` files for tests and examples:

```txt
tests/vix.app
examples/basic/vix.app
```

For complex multi-target library projects, use `CMakeLists.txt`.

## Next steps

Continue with:

- [Tests](./tests.md)
- [Project Types](./project-types.md)
- [Examples](./examples.md)
- [Best Practices](./best-practices.md)
