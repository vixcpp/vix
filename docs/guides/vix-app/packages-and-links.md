# Packages and Links

`vix.app` separates package discovery from linking.

This is important.

```txt
packages -> find_package(...)
links    -> target_link_libraries(...)
```

`packages` does not automatically link anything.

You use `packages` to ask CMake to find a package.

You use `links` to choose what targets or libraries your target should link against.

## Basic rule

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

The incorrect version finds `fmt`, but it does not link `fmt::fmt`.

## packages

The `packages` field generates `find_package(...)` calls in the internal CMake project.

Example:

```ini
packages = [
  Threads:REQUIRED,
  fmt:REQUIRED,
]
```

This means Vix generates something equivalent to:

```cmake
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
```

## links

The `links` field generates `target_link_libraries(...)`.

Example:

```ini
links = [
  Threads::Threads,
  fmt::fmt,
]
```

This means Vix generates something equivalent to:

```cmake
target_link_libraries(myapp PRIVATE
  Threads::Threads
  fmt::fmt
)
```

## Why packages and links are separate

CMake packages can expose different target names.

For example, a package can be found with:

```cmake
find_package(fmt REQUIRED)
```

but the target to link is usually:

```cmake
fmt::fmt
```

Another package can be found with:

```cmake
find_package(Boost REQUIRED COMPONENTS system filesystem)
```

but the targets to link can be:

```cmake
Boost::system
Boost::filesystem
```

Because package names and target names are not always the same, `vix.app` keeps them separate.

## Supported package syntax

`packages` supports these forms:

```txt
<name>
<name>:REQUIRED
<name>:COMPONENTS=a,b
<name>:COMPONENTS=a,b:REQUIRED
```

## Simple package

```ini
packages = [
  Threads,
]
```

Generates:

```cmake
find_package(Threads)
```

## Required package

```ini
packages = [
  Threads:REQUIRED,
]
```

Generates:

```cmake
find_package(Threads REQUIRED)
```

## Package with components

```ini
packages = [
  "Boost:COMPONENTS=system,filesystem",
]
```

Generates:

```cmake
find_package(Boost COMPONENTS system filesystem)
```

## Required package with components

```ini
packages = [
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]
```

Generates:

```cmake
find_package(Boost REQUIRED COMPONENTS system filesystem)
```

## Why some package values are quoted

Values that contain commas should be quoted.

Correct:

```ini
packages = [
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]
```

If you do not quote a value containing commas, the manifest parser may treat it as multiple array items.

## Threads example

Project layout:

```txt
threaded-app/
  vix.app
  src/
    main.cpp
```

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

## fmt example

This example assumes `fmt` is available to CMake on your system.

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

## Boost example

This example assumes Boost is installed and available to CMake.

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
  boost::filesystem::path path = ".";
  vix::print("Current path:", path.string());
  return 0;
}
```

Build and run:

```bash
vix build
vix run
```

## System libraries

You can link simple system libraries directly with `links`.

Example on Linux:

```ini
links = [
  m,
]
```

This links the math library.

Complete example:

```ini
name = math_app
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

links = [
  m,
]
```

`src/main.cpp`:

```cpp
#include <vix.hpp>

#include <cmath>

int main()
{
  vix::print("sqrt(25) =", std::sqrt(25.0));
  return 0;
}
```

## Local CMake targets

If your generated project has access to a local target, you can list it under `links`.

Example:

```ini
links = [
  my_local_lib,
]
```

For `vix.app` V1, the recommended approach is still one manifest per target.

For multiple related targets, use either:

```txt
- one vix.app project per target
- or a normal CMakeLists.txt for full control
```

## Header-only libraries

Header-only libraries usually do not need linking.

They usually only need an include directory.

Example:

```ini
name = header_app
type = executable
standard = c++20

sources = [
  src/main.cpp,
]

include_dirs = [
  third_party/some_header_lib/include,
]
```

No `links` field is needed if the library is truly header-only.

## Package found but target not linked

A common mistake is to write:

```ini
packages = [
  fmt:REQUIRED,
]
```

and expect the target to link automatically.

This is not enough.

You still need:

```ini
links = [
  fmt::fmt,
]
```

Correct complete version:

```ini
packages = [
  fmt:REQUIRED,
]

links = [
  fmt::fmt,
]
```

## Imported target not found

If the build fails with an error similar to:

```txt
Target "myapp" links to:
  fmt::fmt

but the target was not found.
```

It usually means one of these things:

```txt
- the package was not found
- the package was found but does not export that target name
- the target name is different on your system
- you forgot to add the package under packages
```

Check the package documentation to confirm the imported target name.

## Package not found

If CMake cannot find a package, you may see an error like:

```txt
Could not find a package configuration file provided by "fmt"
```

This means CMake cannot locate the package.

Possible fixes:

```txt
- install the package
- set CMAKE_PREFIX_PATH
- use your system package manager
- use a normal CMakeLists.txt if the dependency needs custom setup
```

You can pass extra CMake arguments after `--`:

```bash
vix build -- -DCMAKE_PREFIX_PATH=/path/to/prefix
```

## Using CMAKE_PREFIX_PATH

Some packages are installed in custom locations.

Example:

```bash
vix build -- -DCMAKE_PREFIX_PATH=/opt/fmt
```

or:

```bash
vix build -- -DCMAKE_PREFIX_PATH=$HOME/local
```

This helps CMake find packages installed outside the default system paths.

## Multiple packages

You can list several packages:

```ini
packages = [
  Threads:REQUIRED,
  fmt:REQUIRED,
  "Boost:COMPONENTS=system,filesystem:REQUIRED",
]
```

And link their imported targets:

```ini
links = [
  Threads::Threads,
  fmt::fmt,
  Boost::system,
  Boost::filesystem,
]
```

Complete example:

```ini
name = network_app
type = executable
standard = c++20

sources = [
  src/main.cpp,
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
```

## Using link options

`link_options` is different from `links`.

Use `links` for libraries and targets:

```ini
links = [
  fmt::fmt,
  Threads::Threads,
]
```

Use `link_options` for linker flags:

```ini
link_options = [
  "-Wl,--as-needed",
]
```

Complete example:

```ini
name = linked_app
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

link_options = [
  "-Wl,--as-needed",
]
```

## Recommended pattern

For every external dependency, think in two steps.

Step 1: find the package.

```ini
packages = [
  fmt:REQUIRED,
]
```

Step 2: link the target.

```ini
links = [
  fmt::fmt,
]
```

Final:

```ini
packages = [
  fmt:REQUIRED,
]

links = [
  fmt::fmt,
]
```

## When to use CMakeLists.txt instead

Use a normal `CMakeLists.txt` when dependency setup requires complex CMake logic.

Examples:

```txt
- FetchContent
- CPM.cmake
- custom find modules
- generated dependency targets
- platform-specific package logic
- optional dependency graphs
- multiple dependency variants
```

`vix.app` is designed to keep common cases simple.

For advanced dependency control, CMake is still the right tool.

## Summary

```txt
packages:
  finds packages with find_package(...)

links:
  links libraries or imported targets with target_link_libraries(...)

compile_options:
  compiler flags

link_options:
  linker flags
```

The safest pattern is:

```ini
packages = [
  SomePackage:REQUIRED,
]

links = [
  SomePackage::SomeTarget,
]
```

## Next steps

Continue with:

- [Tests](./tests.md)
- [Project Types](./project-types.md)
- [Troubleshooting](./troubleshooting.md)
- [Best Practices](./best-practices.md)
