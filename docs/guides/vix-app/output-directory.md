# Output Directory

`vix.app` supports `output_dir` to control where the built target is placed inside the build tree.

This is useful when you want a predictable output layout such as:

```txt
build-ninja/bin/myapp
```

## Basic usage

```ini
name = hello
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]
```

Build:

```bash
vix build
```

The executable can be placed under:

```txt
build-ninja/bin/hello
```

Run:

```bash
vix run
```

## What output_dir means

`output_dir` is relative to the CMake build directory.

Example:

```ini
output_dir = bin
```

means:

```txt
${CMAKE_BINARY_DIR}/bin
```

So with the default Vix build directory:

```txt
build-ninja/
```

the final output becomes:

```txt
build-ninja/bin/
```

## Default output location

If you do not set `output_dir`, the output location depends on the generated CMake project and the selected build system.

Example without `output_dir`:

```ini
name = hello
type = executable
standard = c++20

sources = [
  src/main.cpp,
]
```

The executable may be produced under:

```txt
build-ninja/hello
```

or another CMake-managed location depending on the target type and generator.

For predictable output, use:

```ini
output_dir = bin
```

## Recommended app layout

Project:

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
output_dir = bin

sources = [
  src/main.cpp,
]
```

After build:

```txt
hello/
  build-ninja/
    bin/
      hello
```

## output_dir with resources

`resources` are copied next to the built target.

So if you use:

```ini
output_dir = bin
```

and:

```ini
resources = [
  assets,
]
```

the output can look like this:

```txt
build-ninja/
  bin/
    myapp
    assets/
```

Complete example:

```ini
name = myapp
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

## Example with config files

Project layout:

```txt
config_app/
  vix.app
  src/
    main.cpp
  config/
    app.json
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
  config,
]
```

After build:

```txt
build-ninja/
  bin/
    config_app
    config/
      app.json
```

## Running manually

If you want to run the executable manually:

```bash
./build-ninja/bin/config_app
```

If your program reads files using relative paths, remember that the current working directory matters.

For example:

```cpp
std::ifstream file("config/app.json");
```

will look for `config/app.json` relative to the process working directory, not always relative to the executable file.

## vix run and output_dir

`vix run` tries to resolve the built executable automatically.

For a manifest like this:

```ini
name = myapp
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]
```

`vix run` can find common executable locations such as:

```txt
build-ninja/myapp
build-ninja/bin/myapp
build-ninja/src/myapp
```

So you can usually run:

```bash
vix run
```

without manually typing the executable path.

## output_dir for static libraries

You can also use `output_dir` with static libraries.

Example:

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

The static library output can be placed under:

```txt
build-ninja/lib/
```

## output_dir for shared libraries

For shared libraries:

```ini
name = plugin
type = shared
standard = c++20
output_dir = lib

sources = [
  src/plugin.cpp,
]

include_dirs = [
  include,
]
```

The shared library output can be placed under:

```txt
build-ninja/lib/
```

Depending on the platform, the final file may look like:

```txt
libplugin.so
```

```txt
libplugin.dylib
```

```txt
plugin.dll
```

## Multiple presets

Vix uses different build directories depending on the preset.

Default development build:

```bash
vix build
```

usually uses:

```txt
build-ninja/
```

Release build:

```bash
vix build --preset release
```

uses:

```txt
build-release/
```

So with:

```ini
output_dir = bin
```

you can get:

```txt
build-ninja/bin/myapp
```

and:

```txt
build-release/bin/myapp
```

## output_dir is not an install directory

`output_dir` controls where the target is placed inside the build directory.

It is not the same as an install prefix.

This:

```ini
output_dir = bin
```

does not install your application globally.

It only affects the build output layout.

For advanced install rules, use a normal `CMakeLists.txt`.

## Avoid absolute output paths

Prefer relative paths:

```ini
output_dir = bin
```

Avoid absolute paths:

```ini
output_dir = /usr/local/bin
```

`vix.app` is designed for local project builds.

Use install tools or CMake install rules when you need system-wide installation.

## Recommended values

For applications:

```ini
output_dir = bin
```

For libraries:

```ini
output_dir = lib
```

For tests:

```ini
output_dir = bin
```

Examples:

```ini
name = myapp
type = executable
output_dir = bin
```

```ini
name = mathlib
type = static
output_dir = lib
```

```ini
name = mathlib_tests
type = executable
output_dir = bin
```

## Complete executable example

```ini
name = server
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
  src/server.cpp,
]

include_dirs = [
  include,
]

resources = [
  public,
  config,
]
```

After build:

```txt
build-ninja/
  bin/
    server
    public/
    config/
```

## Complete library example

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

After build:

```txt
build-ninja/
  lib/
    libmathlib.a
```

The exact library file name depends on the platform and toolchain.

## Complete test example

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

resources = [
  data,
]
```

After build from the `tests/` folder:

```txt
tests/
  build-ninja/
    bin/
      mathlib_tests
      data/
```

Run:

```bash
vix run
```

## Common mistakes

### Expecting output_dir to be relative to the source directory

Incorrect expectation:

```txt
output_dir = bin
```

creates:

```txt
project/bin/myapp
```

Actual behavior:

```txt
project/build-ninja/bin/myapp
```

`output_dir` is relative to the build directory.

### Expecting output_dir to install the binary

`output_dir` does not install binaries into system paths.

It only controls the build output location.

Use CMake install rules for installation workflows.

### Forgetting resources move with output_dir

If you set:

```ini
output_dir = bin
```

and:

```ini
resources = [
  assets,
]
```

then check:

```txt
build-ninja/bin/assets/
```

not:

```txt
build-ninja/assets/
```

### Running manually from the wrong directory

If your app expects:

```txt
config/app.json
```

and the file is under:

```txt
build-ninja/bin/config/app.json
```

then running from the project root may not find it unless your app resolves paths relative to the executable.

## Troubleshooting

### Executable not found

Check the target name:

```ini
name = myapp
```

Then check common output locations:

```txt
build-ninja/myapp
build-ninja/bin/myapp
build-release/bin/myapp
```

### Resource not found

Check whether you set `output_dir`.

With:

```ini
output_dir = bin
```

resources are copied near:

```txt
build-ninja/bin/
```

### Wrong preset directory

If you built with:

```bash
vix build --preset release
```

check:

```txt
build-release/
```

not:

```txt
build-ninja/
```

## Summary

Use `output_dir` when you want predictable build outputs.

For applications:

```ini
output_dir = bin
```

For libraries:

```ini
output_dir = lib
```

Remember:

```txt
output_dir is relative to the build directory.
resources are copied next to the built target.
vix run can usually find the executable automatically.
```

## Next steps

Continue with:

- [Resources](./resources.md)
- [Examples](./examples.md)
- [Libraries](./libraries.md)
- [Troubleshooting](./troubleshooting.md)
