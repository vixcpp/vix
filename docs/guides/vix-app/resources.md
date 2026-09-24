# Resources

`vix.app` supports copying resource files or directories next to the built target.

Use the `resources` field when your application needs files at runtime.

Common examples:

```txt
assets/
config.json
templates/
public/
data/
```

## Basic example

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

Build:

```bash
vix build
```

The `assets` directory is copied next to the built target.

If the executable is placed under:

```txt
build-ninja/bin/asset_app
```

then the resources are copied near it:

```txt
build-ninja/bin/assets/
```

## Why resources exist

C++ applications often need runtime files.

For example:

```txt
- images
- JSON config files
- templates
- static web files
- certificates for local development
- test data
- language files
```

Without resource copying, you may build the executable successfully but fail at runtime because files are missing.

`resources` solves this by copying required files after a successful build.

## Copy a directory

To copy a whole directory, list it under `resources`:

```ini
resources = [
  assets,
]
```

Example layout:

```txt
myapp/
  vix.app
  src/
    main.cpp
  assets/
    logo.png
    config.json
```

After build:

```txt
build-ninja/
  bin/
    myapp
    assets/
      logo.png
      config.json
```

## Copy a single file

You can copy a single file:

```ini
resources = [
  config.json,
]
```

Example layout:

```txt
myapp/
  vix.app
  src/
    main.cpp
  config.json
```

After build:

```txt
build-ninja/
  myapp
  config.json
```

or, when `output_dir = bin` is used:

```txt
build-ninja/
  bin/
    myapp
    config.json
```

## Copy with a custom destination

You can copy a resource to a custom destination using:

```txt
source=destination
```

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

Example output:

```txt
build-ninja/
  bin/
    myapp
    config/
      config.json
```

## Multiple resources

You can copy several resources:

```ini
resources = [
  assets,
  public,
  "data/config.json=config/config.json",
  "data/icon.png=icon.png",
]
```

This is useful for applications with several runtime directories.

## Complete example

Project layout:

```txt
myapp/
  vix.app
  src/
    main.cpp
  assets/
    logo.txt
  data/
    config.json
```

`vix.app`:

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
  "data/config.json=config/config.json",
]
```

`src/main.cpp`:

```cpp
#include <vix.hpp>

#include <fstream>
#include <string>

int main()
{
  std::ifstream file("config/config.json");

  if (!file)
  {
    vix::print("config file not found");
    return 1;
  }

  std::string content;
  std::getline(file, content);

  vix::print("config:", content);
  return 0;
}
```

Build and run:

```bash
vix build
vix run
```

## Resources and output_dir

`resources` are copied next to the built target.

If you do not set `output_dir`, the target is usually built under the build directory:

```txt
build-ninja/
```

If you set:

```ini
output_dir = bin
```

then the target is placed under:

```txt
build-ninja/bin/
```

and resources are copied there.

Example:

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

Output:

```txt
build-ninja/
  bin/
    myapp
    assets/
```

## Resources and vix run

`vix run` runs the built executable.

Because resources are copied next to the executable, your app can read them using paths relative to the executable location or runtime working directory, depending on how your app handles paths.

For simple apps, keep resource access predictable.

Example:

```txt
config/config.json
assets/logo.png
```

Avoid hardcoding absolute paths.

## Resource paths are relative to vix.app

Resource source paths are relative to the directory containing `vix.app`.

Example:

```txt
myapp/
  vix.app
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
  myapp/assets,
]
```

because `vix.app` is already inside `myapp/`.

## Resources inside tests

Tests can also use resources.

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

Build and run:

```bash
cd tests
vix run
```

The test data is copied next to the test executable.

## Custom destination for tests

Example:

```ini
resources = [
  "data/config.json=config/config.json",
]
```

This lets the test read:

```txt
config/config.json
```

instead of:

```txt
data/config.json
```

## Resource syntax

Supported forms:

```txt
<src>
<src>=<dest>
```

### Form 1: source only

```ini
resources = [
  assets,
]
```

This copies `assets` using its original basename.

### Form 2: source and destination

```ini
resources = [
  "data/config.json=config/config.json",
]
```

This copies the source to a custom destination next to the built target.

## Quoting resources

Quote resource values when they contain special characters.

Recommended:

```ini
resources = [
  "data/config.json=config/config.json",
]
```

Also quote paths with spaces:

```ini
resources = [
  "my assets",
  "data/my config.json=config/config.json",
]
```

For clean projects, avoid spaces in resource paths when possible.

## Directory resources

When the source is a directory, Vix copies the directory.

Example:

```ini
resources = [
  assets,
]
```

Input:

```txt
assets/
  logo.png
  style.css
```

Output:

```txt
assets/
  logo.png
  style.css
```

## File resources

When the source is a file, Vix copies the file.

Example:

```ini
resources = [
  config.json,
]
```

Input:

```txt
config.json
```

Output:

```txt
config.json
```

## Renaming files

Use `src=dest` to rename a file.

```ini
resources = [
  "data/dev.json=config.json",
]
```

This copies:

```txt
data/dev.json
```

as:

```txt
config.json
```

next to the built target.

## Nesting files

Use `src=dest` to place a file inside a directory.

```ini
resources = [
  "data/dev.json=config/dev.json",
]
```

Output:

```txt
config/
  dev.json
```

## Recommended structure

For applications:

```txt
myapp/
  vix.app
  src/
    main.cpp
  include/
    myapp/
      app.hpp
  assets/
    logo.png
  config/
    app.json
```

Manifest:

```ini
name = myapp
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]

include_dirs = [
  include,
]

resources = [
  assets,
  config,
]
```

## Web server example

For a small web server, you may want to copy static files.

Project layout:

```txt
webapp/
  vix.app
  src/
    main.cpp
  public/
    index.html
    app.css
```

`vix.app`:

```ini
name = webapp
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]

resources = [
  public,
]
```

After build:

```txt
build-ninja/
  bin/
    webapp
    public/
      index.html
      app.css
```

## Config file example

Project layout:

```txt
config_app/
  vix.app
  src/
    main.cpp
  data/
    config.json
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
]
```

After build:

```txt
build-ninja/
  bin/
    config_app
    config/
      config.json
```

## What resources should not be used for

Do not use `resources` for files that need to be compiled.

For source files, use:

```ini
sources = [
  src/main.cpp,
]
```

For include directories, use:

```ini
include_dirs = [
  include,
]
```

For runtime files, use:

```ini
resources = [
  assets,
]
```

## Generated resources

If your resources are generated before build, make sure they exist before running:

```bash
vix build
```

For complex generated resources, use a normal `CMakeLists.txt`.

Examples that may need CMake:

```txt
- generated assets
- code generation
- resource compression
- embedding binary files
- custom copy rules
```

## Common mistakes

### Resource path does not exist

Incorrect:

```ini
resources = [
  asset,
]
```

when the real directory is:

```txt
assets/
```

Correct:

```ini
resources = [
  assets,
]
```

### Expecting resources to compile

Incorrect:

```ini
resources = [
  src/main.cpp,
]
```

Correct:

```ini
sources = [
  src/main.cpp,
]
```

### Forgetting output_dir location

If you use:

```ini
output_dir = bin
```

your executable and resources are under:

```txt
build-ninja/bin/
```

not directly under:

```txt
build-ninja/
```

### Using absolute runtime paths

Avoid code like:

```cpp
std::ifstream file("/home/user/project/config.json");
```

Prefer relative resource paths or application-controlled runtime paths.

## Troubleshooting

### Config file not found at runtime

Check where the executable is built.

If you use:

```ini
output_dir = bin
```

look under:

```txt
build-ninja/bin/
```

Check that the resource was copied there.

### Resource copied but app still cannot find it

Your app may be running with a different working directory.

Either:

```txt
- open files relative to the executable location
- run from the output directory
- use a runtime config path
- pass a path through command-line arguments
```

### Directory copied but content missing

Check that the files exist before build.

Also check that your path is relative to the directory containing `vix.app`.

## Summary

Use `resources` for runtime files.

```ini
resources = [
  assets,
  "data/config.json=config/config.json",
]
```

Use `output_dir` when you want a predictable output folder:

```ini
output_dir = bin
```

Recommended app pattern:

```txt
myapp/
  vix.app
  src/
  include/
  assets/
  config/
```

Recommended manifest:

```ini
name = myapp
type = executable
standard = c++20
output_dir = bin

sources = [
  src/main.cpp,
]

include_dirs = [
  include,
]

resources = [
  assets,
  config,
]
```

## Next steps

Continue with:

- [Output Directory](./output-directory.md)
- [Examples](./examples.md)
- [Tests](./tests.md)
- [Troubleshooting](./troubleshooting.md)
