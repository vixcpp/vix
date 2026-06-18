# Troubleshooting

This page lists common `vix.app` problems and how to fix them.

Use it when `vix build` or `vix run` does not behave as expected.

## Project not detected

You may see an error like:

```txt
Unable to determine the project directory.
```

or:

```txt
Missing CMakeLists.txt or vix.app.
```

This means Vix could not find a project root.

A `vix.app` project must have this structure:

```txt
myapp/
  vix.app
  src/
    main.cpp
```

Run the command from the project root:

```bash
cd myapp
vix build
```

Or pass the project directory explicitly:

```bash
vix build --dir ./myapp
```

## CMakeLists.txt is used instead of vix.app

Vix resolves projects in this order:

```txt
1. CMakeLists.txt
2. vix.app
```

If both files exist:

```txt
myapp/
  CMakeLists.txt
  vix.app
```

Vix uses:

```txt
CMakeLists.txt
```

To use `vix.app`, rename or remove the CMake file:

```bash
mv CMakeLists.txt CMakeLists.txt.bak
```

Then run:

```bash
vix build
```

## Missing name

A `vix.app` file must define a target name.

Incorrect:

```ini
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
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

The `name` field is used as the default build target and executable name.

## Invalid name

Target names should be simple.

Recommended:

```ini
name = hello
```

```ini
name = my_app
```

```ini
name = my-app
```

Avoid spaces or special characters:

```ini
name = "my app"
```

If the name is invalid, use only:

```txt
letters
numbers
_
-
```

Example:

```ini
name = my_app
```

## Missing sources

A `vix.app` target needs source files.

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

## Source file not found

You may see:

```txt
vix.app source file not found: src/main.cpp
```

This means the file listed in `sources` does not exist.

Check your project layout:

```txt
hello/
  vix.app
  src/
    main.cpp
```

Correct manifest:

```ini
sources = [
  src/main.cpp,
]
```

Paths are relative to the directory containing `vix.app`.

## Wrong relative path

If `vix.app` is in the project root:

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

## Wrong path inside tests

If `vix.app` is inside `tests/`:

```txt
mathlib/
  src/
    add.cpp
  tests/
    vix.app
    test_add.cpp
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

Paths are relative to `tests/`.

## Invalid target type

Supported target types are:

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

Incorrect:

```ini
type = exe
```

Correct:

```ini
type = executable
```

## Invalid C++ standard

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

Incorrect:

```ini
standard = cpp20
```

Correct:

```ini
standard = c++20
```

## Malformed array

Arrays can be inline:

```ini
sources = [src/main.cpp, src/app.cpp]
```

or multi-line:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
]
```

Incorrect:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
```

The closing `]` is missing.

Correct:

```ini
sources = [
  src/main.cpp,
  src/app.cpp,
]
```

## Unknown field

If you use a field that `vix.app` does not support, Vix may report an unknown field error.

Incorrect:

```ini
target = hello
```

Correct:

```ini
name = hello
```

Supported fields:

```txt
name
type
standard
sources
include_dirs
defines
links
compile_options
link_options
compile_features
packages
resources
output_dir
```

## Header not found

You may see a compiler error like:

```txt
fatal error: myapp/app.hpp: No such file or directory
```

Check your layout:

```txt
myapp/
  include/
    myapp/
      app.hpp
  src/
    main.cpp
```

If your code has:

```cpp
#include <myapp/app.hpp>
```

then your manifest needs:

```ini
include_dirs = [
  include,
]
```

## Header files listed as sources

Usually, headers do not need to be listed in `sources`.

Avoid this:

```ini
sources = [
  src/main.cpp,
  include/myapp/app.hpp,
]
```

Prefer this:

```ini
sources = [
  src/main.cpp,
]

include_dirs = [
  include,
]
```

## Duplicate main function

You may see a linker error about multiple definitions of `main`.

This often happens in tests.

Incorrect test manifest:

```ini
sources = [
  test_app.cpp,
  ../src/main.cpp,
  ../src/app.cpp,
]
```

If both `test_app.cpp` and `main.cpp` define `main()`, the linker fails.

Correct:

```ini
sources = [
  test_app.cpp,
  ../src/app.cpp,
]
```

Keep `main.cpp` thin and test the application logic from other source files.

## Undefined reference to main

You may see:

```txt
undefined reference to main
```

This usually means you declared an executable target without a `main()` function.

Incorrect:

```ini
name = mathlib
type = executable

sources = [
  src/add.cpp,
]
```

Correct for a library:

```ini
name = mathlib
type = static

sources = [
  src/add.cpp,
]
```

Correct for an app:

```ini
name = hello
type = executable

sources = [
  src/main.cpp,
]
```

and `src/main.cpp` should contain:

```cpp
int main()
{
  return 0;
}
```

## Package not found

You may see:

```txt
Could not find a package configuration file provided by "fmt"
```

Example manifest:

```ini
packages = [
  fmt:REQUIRED,
]

links = [
  fmt::fmt,
]
```

This means CMake could not find the package.

Possible fixes:

```txt
- install the package
- set CMAKE_PREFIX_PATH
- check the package name
- use CMakeLists.txt for custom dependency setup
```

Example with `CMAKE_PREFIX_PATH`:

```bash
vix build -- -DCMAKE_PREFIX_PATH=/path/to/fmt
```

## Package found but target not linked

`packages` only calls `find_package(...)`.

You still need to link the imported target:

```ini
packages = [fmt:REQUIRED]
links = [fmt::fmt]
```

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

## Imported target not found

You may see an error like:

```txt
Target "myapp" links to:
  fmt::fmt

but the target was not found.
```

Common causes:

```txt
- the package was not found
- the target name is wrong
- the package does not export that imported target
- links contains a target that does not exist
```

Check the package documentation.

For example, `fmt` commonly uses:

```ini
links = [
  fmt::fmt,
]
```

Boost components commonly use:

```ini
links = [
  Boost::system,
  Boost::filesystem,
]
```

## Boost package syntax error

If a package has components, quote the value.

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

The comma can be interpreted as an array separator if the value is not quoted.

## Linker flag in compile_options

Do not put linker options under `compile_options`.

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

## Compiler flag not supported

Some flags work only with specific compilers.

Example:

```ini
compile_options = [
  -Wall,
  -Wextra,
]
```

These are common with GCC and Clang, but not always valid for MSVC.

If portability matters, keep compiler-specific flags minimal.

## Resource not copied

Check that the resource path exists.

Project:

```txt
myapp/
  assets/
    logo.png
```

Correct:

```ini
resources = [
  assets,
]
```

Incorrect:

```ini
resources = [
  asset,
]
```

Resource paths are relative to the directory containing `vix.app`.

## Resource copied but app cannot find it

Resources are copied next to the built target.

If you use:

```ini
output_dir = bin
```

check:

```txt
build-ninja/bin/
```

not:

```txt
build-ninja/
```

Also remember that relative file access in C++ depends on the process working directory.

If your program does:

```cpp
std::ifstream file("config/app.json");
```

make sure the process is running from the expected directory, or resolve paths relative to the executable.

## Executable not found after build

`vix run` tries to find the built executable using the target name.

Check your manifest:

```ini
name = myapp
type = executable
```

Then check common output locations:

```txt
build-ninja/myapp
build-ninja/bin/myapp
build-release/bin/myapp
```

If your project is a library:

```ini
type = static
```

then there may be no executable to run.

Use:

```bash
vix build
```

or create a test executable under:

```txt
tests/vix.app
```

## vix run on a library

`vix run` is mainly for executable targets.

For a library:

```ini
name = mathlib
type = static
```

use:

```bash
vix build
```

To run tests:

```bash
cd tests
vix run
```

where `tests/vix.app` builds an executable test target.

## output_dir confusion

If you set:

```ini
output_dir = bin
```

the output is placed under the build directory:

```txt
build-ninja/bin/
```

not under:

```txt
project/bin/
```

`output_dir` is relative to the CMake build directory.

## Generated CMake file is missing

For `vix.app` projects, Vix generates:

```txt
.vix/generated/app/CMakeLists.txt
```

If it is missing, run:

```bash
vix build
```

If generation fails, check the manifest errors first.

## Do not edit generated CMake

Do not edit:

```txt
.vix/generated/app/CMakeLists.txt
```

Edit:

```txt
vix.app
```

The generated file can be overwritten by Vix.

## Need raw CMake output

Use:

```bash
vix build --cmake-verbose
```

This shows more CMake configure output.

You can also use:

```bash
vix build -v
```

for a more detailed Vix build summary.

## Need to pass CMake variables

Pass extra CMake arguments after `--`.

Example:

```bash
vix build -- -DCMAKE_PREFIX_PATH=/path/to/prefix
```

Another example:

```bash
vix build -- -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
```

For complex CMake variables, consider using a normal `CMakeLists.txt`.

## Build directory is stale

If the build behaves strangely, remove the build directory.

```bash
rm -rf build-ninja build-dev build-release
```

Then rebuild:

```bash
vix build
```

Or use:

```bash
vix build --clean
```

## .vix/generated is stale

If the generated CMake project looks stale, remove it:

```bash
rm -rf .vix/generated/app
```

Then rebuild:

```bash
vix build
```

## Using vix.app with complex CMake logic

`vix.app` is not meant to express every CMake feature.

Use `CMakeLists.txt` for:

```txt
- custom commands
- generated sources
- install rules
- CTest
- FetchContent
- CPM.cmake
- custom toolchains
- many targets in one project
- package export files
- advanced platform-specific logic
```

## Debug checklist

When a `vix.app` project fails, check this list:

```txt
1. Am I in the directory containing vix.app?
2. Does CMakeLists.txt exist? If yes, Vix uses it first.
3. Is name defined?
4. Is sources defined?
5. Do all source files exist?
6. Are include_dirs correct?
7. Is type correct?
8. Is standard written as c++17, c++20, or c++23?
9. Are packages and links both present when using imported targets?
10. Are resource paths correct?
11. Am I checking the right build directory?
```

## Minimal known-good vix.app

Use this to verify that `vix.app` works:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

With:

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
  vix::print("hello");
  return 0;
}
```

Run:

```bash
vix build
vix run
```

## Summary

Most `vix.app` issues come from:

```txt
- wrong relative paths
- missing sources
- missing include_dirs
- CMakeLists.txt taking priority
- packages not linked through links
- trying to use vix.app for complex CMake workflows
```

Keep the manifest simple and explicit.

When the project becomes complex, use `CMakeLists.txt`.

## Next steps

Continue with:

- [Best Practices](./best-practices.md)
- [Manifest Reference](./manifest-reference.md)
- [Packages and Links](./packages-and-links.md)
- [CMake Fallback](./cmake-fallback.md)
