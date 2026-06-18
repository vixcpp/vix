# Sources and Includes

`vix.app` uses `sources` and `include_dirs` to describe the files that belong to your target.

These two fields are usually enough to describe the structure of a simple C++ project.

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
]

include_dirs = [
  include,
]
```

## Basic layout

A recommended project layout looks like this:

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

## sources

The `sources` field lists the source files that should be compiled.

```ini
sources = [
  src/main.cpp,
]
```

For multiple files:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
  src/server.cpp,
]
```

Inline arrays are also supported:

```ini
sources = [src/main.cpp, src/app.cpp]
```

## Source paths are relative to vix.app

All source paths are relative to the directory that contains `vix.app`.

Example:

```txt
myapp/
  vix.app
  src/
    main.cpp
```

Correct:

```ini
sources = [
  src/main.cpp,
]
```

Incorrect:

```ini
sources = [
  myapp/src/main.cpp,
]
```

because `vix.app` is already inside `myapp/`.

## Source file validation

Vix checks that source files exist before generating the internal CMake project.

If a source file does not exist, Vix reports an error like:

```txt
vix.app source file not found: src/main.cpp
```

Fix the path or create the missing file.

## include_dirs

The `include_dirs` field lists directories used by `#include`.

```ini
include_dirs = [
  include,
]
```

Example:

```txt
myapp/
  vix.app
  include/
    myapp/
      app.hpp
  src/
    main.cpp
```

`src/main.cpp`:

```cpp
#include <myapp/app.hpp>

int main()
{
  return 0;
}
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
  include,
]
```

## Include paths are also relative to vix.app

Example:

```txt
myapp/
  vix.app
  third_party/
    asio/
      include/
        asio.hpp
```

Correct:

```ini
include_dirs = [
  third_party/asio/include,
]
```

Then in C++:

```cpp
#include <asio.hpp>
```

## Header files do not need to be listed in sources

Usually, header files are not listed under `sources`.

Correct:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
]

include_dirs = [
  include,
]
```

You do not need:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
  include/myapp/app.hpp,
]
```

Headers are included through `include_dirs`.

## Example with a header

Project layout:

```txt
hello/
  vix.app
  include/
    hello/
      message.hpp
  src/
    main.cpp
```

`include/hello/message.hpp`:

```cpp
#pragma once

#include <string>

namespace hello
{
  inline std::string message()
  {
    return "Hello from vix.app";
  }
}
```

`src/main.cpp`:

```cpp
#include <vix.hpp>
#include <hello/message.hpp>

int main()
{
  vix::print(hello::message());
  return 0;
}
```

`vix.app`:

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

Build and run:

```bash
vix build
vix run
```

## Example with multiple source files

Project layout:

```txt
server/
  vix.app
  include/
    server/
      server.hpp
      router.hpp
  src/
    main.cpp
    server.cpp
    router.cpp
```

`vix.app`:

```ini
name = server
type = executable
standard = c++20

sources = [
  src/main.cpp,
  src/server.cpp,
  src/router.cpp,
]

include_dirs = [
  include,
]
```

`include/server/server.hpp`:

```cpp
#pragma once

namespace server
{
  int run();
}
```

`src/server.cpp`:

```cpp
#include <vix.hpp>
#include <server/server.hpp>

namespace server
{
  int run()
  {
    vix::print("server running");
    return 0;
  }
}
```

`src/main.cpp`:

```cpp
#include <server/server.hpp>

int main()
{
  return server::run();
}
```

## Using src as an include directory

Some projects include headers from `src/`.

Example:

```txt
myapp/
  vix.app
  src/
    main.cpp
    internal/
      config.hpp
```

`src/main.cpp`:

```cpp
#include <internal/config.hpp>
```

Then add `src` to `include_dirs`:

```ini
include_dirs = [
  src,
]
```

However, for public headers, prefer:

```txt
include/
```

Use `src/` for internal implementation headers.

## Recommended include structure

For reusable code, prefer this structure:

```txt
myapp/
  include/
    myapp/
      app.hpp
      config.hpp
  src/
    app.cpp
    main.cpp
```

Then include headers like this:

```cpp
#include <myapp/app.hpp>
```

This avoids name collisions with other libraries.

Recommended:

```txt
include/myapp/app.hpp
```

Avoid:

```txt
include/app.hpp
```

for larger projects.

## Quoted paths

Use quotes when a path contains spaces or special characters.

```ini
sources = [
  "src/with space.cpp",
]
```

```ini
include_dirs = [
  "third_party/my lib/include",
]
```

For clean projects, avoid spaces in file names when possible.

## Inline arrays

For very small projects, inline arrays are fine:

```ini
sources = [src/main.cpp]
include_dirs = [include]
```

For bigger projects, prefer multi-line arrays:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
  src/server.cpp,
]

include_dirs = [
  include,
  third_party/asio/include,
]
```

Multi-line arrays are easier to read and review in Git diffs.

## Static library example

Project layout:

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

`vix.app`:

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

## Test source paths

When a `vix.app` file is inside `tests/`, paths are relative to `tests/`.

Example:

```txt
mathlib/
  include/
    mathlib/
      math.hpp
  src/
    add.cpp
  tests/
    vix.app
    test_add.cpp
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

Correct:

```ini
sources = [
  test_add.cpp,
  ../src/add.cpp,
]
```

Incorrect:

```ini
sources = [
  tests/test_add.cpp,
  src/add.cpp,
]
```

because the manifest is already inside `tests/`.

## Example source paths

If `vix.app` is here:

```txt
project/vix.app
```

then these paths are resolved from `project/`:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
]
```

If `vix.app` is here:

```txt
project/tests/vix.app
```

then these paths are resolved from `project/tests/`:

```ini
sources = [
  test_app.cpp,
  ../src/app.cpp,
]
```

## Third-party headers

For header-only third-party libraries, add the include directory.

Example:

```txt
myapp/
  third_party/
    json/
      include/
        nlohmann/
          json.hpp
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
  third_party/json/include,
]
```

C++:

```cpp
#include <nlohmann/json.hpp>
```

No `links` entry is needed for a true header-only library.

## Generated files

If your project uses generated source files, `vix.app` may not be enough.

For example:

```txt
schema.proto -> generated .cpp/.hpp
```

or:

```txt
codegen tool -> generated source files
```

In that case, use one of these approaches:

```txt
- generate the files before running vix build
- commit generated files if appropriate
- use a normal CMakeLists.txt for full control
```

`vix.app` is designed for simple explicit source lists.

## Common mistakes

### Source path missing src/

Incorrect:

```ini
sources = [
  main.cpp,
]
```

Correct:

```ini
sources = [
  src/main.cpp,
]
```

when your file is located under `src/`.

### Missing include_dirs

If your code says:

```cpp
#include <myapp/app.hpp>
```

and your file is here:

```txt
include/myapp/app.hpp
```

you need:

```ini
include_dirs = [
  include,
]
```

### Listing headers as sources

Usually unnecessary:

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

### Wrong relative path in tests

If `tests/vix.app` contains:

```ini
sources = [
  src/add.cpp,
]
```

but the real file is:

```txt
../src/add.cpp
```

the build will fail.

Correct:

```ini
sources = [
  ../src/add.cpp,
]
```

### Including main.cpp in tests

Avoid this in test manifests:

```ini
sources = [
  test_app.cpp,
  ../src/main.cpp,
  ../src/app.cpp,
]
```

This can create duplicate `main()` errors.

Prefer:

```ini
sources = [
  test_app.cpp,
  ../src/app.cpp,
]
```

## Recommended patterns

### Small app

```txt
hello/
  vix.app
  src/
    main.cpp
```

```ini
sources = [
  src/main.cpp,
]
```

### App with reusable logic

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
sources = [
  src/main.cpp,
  src/app.cpp,
]

include_dirs = [
  include,
]
```

### Library

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
sources = [
  src/add.cpp,
  src/mul.cpp,
]

include_dirs = [
  include,
]
```

### Tests

```txt
mathlib/
  tests/
    vix.app
    test_math.cpp
```

```ini
sources = [
  test_math.cpp,
  ../src/add.cpp,
  ../src/mul.cpp,
]

include_dirs = [
  ../include,
]
```

## Summary

Use `sources` for files that must be compiled.

Use `include_dirs` for directories used by `#include`.

Recommended structure:

```txt
project/
  vix.app
  include/
    project/
      public.hpp
  src/
    main.cpp
    implementation.cpp
```

Recommended manifest:

```ini
name = project
type = executable
standard = c++20

sources = [
  src/main.cpp,
  src/implementation.cpp,
]

include_dirs = [
  include,
]
```

## Next steps

Continue with:

- [Compile Options](./compile-options.md)
- [Resources](./resources.md)
- [Tests](./tests.md)
- [Best Practices](./best-practices.md)
