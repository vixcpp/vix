# Tests with vix.app

This guide explains how to organize tests in a `vix.app` project.

For `vix.app` V1, the recommended approach is simple:

```txt
one vix.app = one target
```

So for tests, the cleanest structure is:

```txt
one vix.app for the library or app
one vix.app for the test executable
```

This keeps the manifest simple and avoids turning `vix.app` into a complex multi-target build language.

## Recommended structure

For a small library with tests:

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

The root `vix.app` builds the library.

The `tests/vix.app` builds the test executable.

## Library manifest

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

This builds a static library target named `mathlib`.

## Library code

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

## Test manifest

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

This creates an executable target named `mathlib_tests`.

The test manifest includes the library source files directly:

```ini
sources = [
  test_math.cpp,
  ../src/add.cpp,
  ../src/mul.cpp,
]
```

This is the simplest approach for `vix.app` V1.

## Test code

`tests/test_math.cpp`:

```cpp
#include <vix.hpp>
#include <mathlib/math.hpp>

int main()
{
  if (mathlib::add(2, 3) != 5)
  {
    vix::print("add test failed");
    return 1;
  }

  if (mathlib::mul(4, 5) != 20)
  {
    vix::print("mul test failed");
    return 1;
  }

  vix::print("all tests passed");
  return 0;
}
```

## Build the library

From the project root:

```bash
vix build
```

This builds the root target:

```txt
mathlib
```

## Build and run the tests

Go to the test directory:

```bash
cd tests
```

Build the test executable:

```bash
vix build
```

Run the tests:

```bash
vix run
```

Expected output:

```txt
all tests passed
```

## Full project example

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

Commands:

```bash
vix build
cd tests
vix build
vix run
```

## App tests

You can also test application logic by placing most of your code outside `main.cpp`.

Recommended layout:

```txt
myapp/
  vix.app
  include/
    myapp/
      app.hpp
  src/
    app.cpp
    main.cpp
  tests/
    vix.app
    test_app.cpp
```

The app manifest:

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

The test manifest:

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

Notice that the test target includes `../src/app.cpp`, but not `../src/main.cpp`.

This avoids having two `main()` functions in the same test executable.

## Avoid testing main.cpp directly

For applications, avoid putting too much logic in `main.cpp`.

Prefer this:

```cpp
#include <myapp/app.hpp>

int main()
{
  return myapp::run();
}
```

Then put real logic in:

```txt
src/app.cpp
include/myapp/app.hpp
```

This makes the logic testable from `tests/test_app.cpp`.

## Example application code

`include/myapp/app.hpp`:

```cpp
#pragma once

namespace myapp
{
  int add(int a, int b);
  int run();
}
```

`src/app.cpp`:

```cpp
#include <vix.hpp>
#include <myapp/app.hpp>

namespace myapp
{
  int add(int a, int b)
  {
    return a + b;
  }

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

`tests/test_app.cpp`:

```cpp
#include <vix.hpp>
#include <myapp/app.hpp>

int main()
{
  if (myapp::add(10, 20) != 30)
  {
    vix::print("add test failed");
    return 1;
  }

  vix::print("app tests passed");
  return 0;
}
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

Run:

```bash
cd tests
vix run
```

## Multiple test files

You can split tests into multiple source files.

Project layout:

```txt
mathlib/
  tests/
    vix.app
    test_main.cpp
    test_add.cpp
    test_mul.cpp
```

`tests/vix.app`:

```ini
name = mathlib_tests
type = executable
standard = c++20

sources = [
  test_main.cpp,
  test_add.cpp,
  test_mul.cpp,
  ../src/add.cpp,
  ../src/mul.cpp,
]

include_dirs = [
  ../include,
]
```

Example `test_main.cpp`:

```cpp
#include <vix.hpp>

int run_add_tests();
int run_mul_tests();

int main()
{
  if (run_add_tests() != 0)
    return 1;

  if (run_mul_tests() != 0)
    return 1;

  vix::print("all tests passed");
  return 0;
}
```

Example `test_add.cpp`:

```cpp
#include <vix.hpp>
#include <mathlib/math.hpp>

int run_add_tests()
{
  if (mathlib::add(2, 3) != 5)
  {
    vix::print("add test failed");
    return 1;
  }

  return 0;
}
```

Example `test_mul.cpp`:

```cpp
#include <vix.hpp>
#include <mathlib/math.hpp>

int run_mul_tests()
{
  if (mathlib::mul(4, 5) != 20)
  {
    vix::print("mul test failed");
    return 1;
  }

  return 0;
}
```

## Tests with compile options

You can add warnings or test-specific compiler flags:

```ini
name = mathlib_tests
type = executable
standard = c++20

sources = [
  test_math.cpp,
  ../src/add.cpp,
]

include_dirs = [
  ../include,
]

compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
]
```

## Tests with defines

Use test-specific preprocessor definitions:

```ini
name = mathlib_tests
type = executable
standard = c++20

sources = [
  test_math.cpp,
  ../src/add.cpp,
]

include_dirs = [
  ../include,
]

defines = [
  MATHLIB_TESTING=1,
]
```

C++ usage:

```cpp
#ifdef MATHLIB_TESTING
// test-only code
#endif
```

## Tests with packages

You can use external test libraries if they are available through CMake packages.

Example with `Threads`:

```ini
name = threaded_tests
type = executable
standard = c++20

sources = [
  test_threads.cpp,
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
links links targets.
```

## Tests with resources

If tests need files, use `resources`.

Project layout:

```txt
myapp/
  tests/
    vix.app
    test_config.cpp
    data/
      config.json
```

`tests/vix.app`:

```ini
name = config_tests
type = executable
standard = c++20
output_dir = bin

sources = [
  test_config.cpp,
]

resources = [
  data,
]
```

After building, `data/` is copied next to the test executable.

## Running tests from CI

A simple CI command can be:

```bash
cd tests
vix build
vix run
```

For release mode:

```bash
cd tests
vix build --preset release
vix run
```

## Recommended test layout

For a library:

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
    test_main.cpp
    test_add.cpp
    test_mul.cpp
```

For an application:

```txt
myapp/
  vix.app
  include/
    myapp/
      app.hpp
  src/
    main.cpp
    app.cpp
  tests/
    vix.app
    test_app.cpp
```

## Why one manifest per test target

`vix.app` is intentionally simple.

For now, the clearest model is:

```txt
root/vix.app        -> app or library target
tests/vix.app       -> test executable target
examples/vix.app    -> example executable target
```

This avoids complex syntax like:

```txt
targets = [...]
tests = [...]
examples = [...]
```

Those features can exist later, but the V1 design stays small and predictable.

## When to use CMakeLists.txt for tests

Use a normal `CMakeLists.txt` when your test setup needs:

```txt
- many test targets
- CTest integration
- GoogleTest discovery
- FetchContent
- custom test fixtures
- generated test data
- install rules
- complex dependency setup
```

For simple tests, `vix.app` is enough.

For advanced test infrastructure, CMake remains the right compatibility path.

## Common mistakes

### Including main.cpp in tests

Incorrect:

```ini
sources = [
  test_app.cpp,
  ../src/main.cpp,
  ../src/app.cpp,
]
```

This can produce duplicate `main()` errors.

Correct:

```ini
sources = [
  test_app.cpp,
  ../src/app.cpp,
]
```

Keep `main.cpp` thin and test the real logic from other source files.

### Missing include directory

If your test includes:

```cpp
#include <mathlib/math.hpp>
```

you need:

```ini
include_dirs = [
  ../include,
]
```

### Source path is relative to tests/

Inside `tests/vix.app`, paths are relative to the `tests/` directory.

Correct:

```ini
sources = [
  test_math.cpp,
  ../src/add.cpp,
]
```

Incorrect:

```ini
sources = [
  tests/test_math.cpp,
  src/add.cpp,
]
```

when the manifest is already inside `tests/`.

### Package found but not linked

Incorrect:

```ini
packages = [
  fmt:REQUIRED,
]
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

## Summary

For tests with `vix.app`, use this pattern:

```txt
project/
  vix.app
  include/
  src/
  tests/
    vix.app
    test_*.cpp
```

Build and run:

```bash
cd tests
vix build
vix run
```

Keep test manifests small, explicit, and close to the test files.

## Next steps

Continue with:

- [Project Types](./project-types.md)
- [Sources and Includes](./sources-and-includes.md)
- [Libraries](./libraries.md)
- [Best Practices](./best-practices.md)
