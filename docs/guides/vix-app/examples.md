# vix.app Examples

This page shows practical `vix.app` examples.

Use these examples as starting points for your own projects.

## Minimal executable

Project layout:

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

Build and run:

```bash
vix build
vix run
```

## Executable with headers

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

`include/hello/message.hpp`:

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

Build and run:

```bash
vix build
vix run
```

## Executable with multiple source files

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
  void run();
}
```

`src/app.cpp`:

```cpp
#include <vix.hpp>
#include <myapp/app.hpp>

namespace myapp
{
  void run()
  {
    vix::print("Running myapp");
  }
}
```

`src/main.cpp`:

```cpp
#include <myapp/app.hpp>

int main()
{
  myapp::run();
  return 0;
}
```

Build and run:

```bash
vix build
vix run
```

## Executable with compile definitions

Use `defines` to pass preprocessor definitions to the target.

`vix.app`:

```ini
name = config_app
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

defines = [
  APP_NAME="config_app",
  APP_VERSION="1.0.0",
  ENABLE_LOGGING=1,
]
```

`src/main.cpp`:

```cpp
#include <vix.hpp>

#ifndef APP_NAME
#define APP_NAME "unknown"
#endif

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

int main()
{
  vix::print(APP_NAME, APP_VERSION);
  return 0;
}
```

Build and run:

```bash
vix build
vix run
```

## Executable with compiler options

Use `compile_options` for compiler flags.

`vix.app`:

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

`src/main.cpp`:

```cpp
#include <vix.hpp>

int main()
{
  vix::print("Compiler options enabled");
  return 0;
}
```

Build:

```bash
vix build
```

Note: compiler options can be compiler-specific. GCC or Clang flags may not work with MSVC.

## Executable with output directory

Use `output_dir` to place the executable under a specific build output folder.

`vix.app`:

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

Build and run:

```bash
vix build
vix run
```

Manual run:

```bash
./build-ninja/bin/hello
```

## Executable with resources

Use `resources` to copy files or folders next to the built target after a successful build.

Project layout:

```txt
asset_app/
  vix.app
  src/
    main.cpp
  assets/
    message.txt
```

`vix.app`:

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

`src/main.cpp`:

```cpp
#include <vix.hpp>

int main()
{
  vix::print("Resource files are copied next to the executable.");
  return 0;
}
```

Build:

```bash
vix build
```

The `assets` directory is copied next to the built target.

## Resource with custom destination

You can copy a resource to a custom destination using:

```txt
src=dest
```

`vix.app`:

```ini
name = config_app
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]

resources = [
  "data/config.json=config/config.json",
  "data/icon.png=icon.png",
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

next to the target.

## Executable with Threads

Use `packages` to call `find_package(...)`.

Use `links` to link the imported target.

`vix.app`:

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

`src/main.cpp`:

```cpp
#include <vix.hpp>

#include <thread>

int main()
{
  std::thread worker([] {
    vix::print("Hello from a thread");
  });

  worker.join();
  return 0;
}
```

Build and run:

```bash
vix build
vix run
```

## Executable with fmt

This example assumes `fmt` is available through your system or CMake package paths.

`vix.app`:

```ini
name = fmt_app
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

packages = [
  fmt:REQUIRED,
]

links = [
  fmt::fmt,
]
```

`src/main.cpp`:

```cpp
#include <fmt/core.h>

int main()
{
  fmt::print("Hello from fmt\n");
  return 0;
}
```

Build and run:

```bash
vix build
vix run
```

Important:

```txt
packages only calls find_package(...).
links links the target.
```

So this is correct:

```ini
packages = [fmt:REQUIRED]
links = [fmt::fmt]
```

## Executable with Boost components

This example assumes Boost is available to CMake.

`vix.app`:

```ini
name = boost_app
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

packages = [
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]

links = [
  Boost::system,
  Boost::filesystem,
]
```

`src/main.cpp`:

```cpp
#include <vix.hpp>

#include <boost/filesystem.hpp>

int main()
{
  boost::filesystem::path p = ".";
  vix::print("Current path:", p.string());
  return 0;
}
```

Build and run:

```bash
vix build
vix run
```

## Static library

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

## Shared library

Project layout:

```txt
plugin/
  vix.app
  include/
    plugin/
      plugin.hpp
  src/
    plugin.cpp
```

`vix.app`:

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

## Library with tests

For `vix.app` V1, the recommended approach is one manifest per target.

Project layout:

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

`tests/test_add.cpp`:

```cpp
#include <vix.hpp>
#include <mathlib/math.hpp>

int main()
{
  if (mathlib::add(2, 3) != 5)
  {
    vix::print("test failed");
    return 1;
  }

  vix::print("test passed");
  return 0;
}
```

Build and run tests:

```bash
cd tests
vix build
vix run
```

## Advanced executable

This example shows most supported fields.

`vix.app`:

```ini
name = myapp
type = executable
standard = c++23
output_dir = bin

sources = [
  src/main.cpp,
  src/network/client.cpp,
  src/network/server.cpp,
  "src/with space.cpp",
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
  m,
]

compile_options = [
  -Wall,
  -Wextra,
  -Wpedantic,
  -O2,
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
  "data/icon.png=icon.png",
]
```

Build and run:

```bash
vix build
vix run
```

## Multiple applications

`vix.app` currently describes one target.

For multiple applications, keep one folder per target.

Example:

```txt
workspace/
  tools/
    server/
      vix.app
      src/
        main.cpp
    client/
      vix.app
      src/
        main.cpp
```

Build server:

```bash
cd tools/server
vix build
vix run
```

Build client:

```bash
cd tools/client
vix build
vix run
```

This keeps each manifest small and predictable.

## CMake fallback example

If your project needs custom CMake logic, use `CMakeLists.txt`.

Example:

```txt
advanced-project/
  CMakeLists.txt
  src/
    main.cpp
```

When both files exist:

```txt
CMakeLists.txt
vix.app
```

Vix uses `CMakeLists.txt`.

This allows you to start simple with `vix.app` and move to full CMake control later.

## Recommended project structures

### Application

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
  tests/
    vix.app
    test_math.cpp
```

### Example project

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

## Next steps

Continue with:

- [Packages and Links](./packages-and-links.md)
- [Tests](./tests.md)
- [Project Types](./project-types.md)
- [Best Practices](./best-practices.md)
