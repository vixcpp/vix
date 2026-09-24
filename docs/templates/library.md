# Library Template

The library template creates a reusable C++ library project.
Use it when you want to build code that can be imported by other C++ projects, published to the Vix Registry, versioned, tested, and reused.

Create a library project with:

```bash
vix new mathlib --lib
```

## What this template is for

Use the library template when you want:

- a reusable C++ library
- a header-only package
- a clean public `include/` API
- tests
- examples
- a stable CMake target
- registry-safe packaging
- a project that can be published with `vix publish`
- a project that can depend on other registry packages with `vix add`

This template is different from an application.
A library is not mainly something you run.
A library is something other projects include and link.

## Design used by this template

The library template uses a **registry-safe C++ library design**.

The important ideas are:

```txt
public headers live in include/
tests are separate
examples are separate
the library exposes a stable target
the project can be published to the registry
```

A library should be easy to consume.

That means:

- clear namespace
- clear include path
- stable target name
- tests that do not depend on examples
- examples that do not pollute consumers
- package metadata in `vix.json`
- reproducible dependency state through `vix.lock`

## Quick start

Create the library:

```bash
vix new mathlib --lib
```

Enter the project:

```bash
cd mathlib
```

Build it:

```bash
vix build
```

Run tests:

```bash
vix tests
```

Run examples if enabled by the generated project:

```bash
vix build --build-target all
```

## Generated structure

A generated library project usually looks like this:

```txt
mathlib/
├── include/
│   └── mathlib/
│       └── mathlib.hpp
├── tests/
│   └── test_basic.cpp
├── examples/
│   ├── basic.cpp
│   └── CMakeLists.txt
├── CMakeLists.txt
├── vix.json
└── README.md
```

The exact structure can evolve between Vix versions, but the strategy stays the same:

```txt
include/   -> public API
tests/     -> validation
examples/  -> usage examples
vix.json   -> package metadata, tasks, dependencies
```

## What each file does

| File or folder              | Role                                                      |
| --------------------------- | --------------------------------------------------------- |
| `include/<name>/<name>.hpp` | Public library header.                                    |
| `tests/`                    | Test programs for the library.                            |
| `examples/`                 | Example programs showing how to use the library.          |
| `CMakeLists.txt`            | Native build and package target definition.               |
| `vix.json`                  | Vix metadata, tasks, registry metadata, and dependencies. |
| `README.md`                 | Generated documentation for the library.                  |
| `vix.lock`                  | Created when registry dependencies are installed.         |

## Public include path

A library should expose a clean include path.

For a library named `mathlib`, users should include:

```cpp
#include <mathlib/mathlib.hpp>
```

This is why public headers live under:

```txt
include/mathlib/
```

Do not put public headers directly at the project root.

Do not force consumers to include files from `src/`.

## Public namespace

The generated library uses a namespace matching the package name.

Example:

```cpp
namespace mathlib
{
  // public API here
}
```

This keeps the library easy to consume:

```cpp
auto nodes = mathlib::make_chain(5);
```

For larger libraries, keep the public namespace stable.
Changing the namespace later is a breaking change.

## Example generated header

A simple generated header can look like this:

```cpp
#pragma once

#include <cstddef>
#include <vector>

namespace mathlib
{
  struct Node
  {
    std::size_t id{};
    std::vector<std::size_t> children{};
  };

  inline std::vector<Node> make_chain(std::size_t n)
  {
    std::vector<Node> nodes;
    nodes.reserve(n);

    for (std::size_t i = 0; i < n; ++i)
    {
      nodes.push_back(Node{i, {}});
    }

    for (std::size_t i = 0; i + 1 < n; ++i)
    {
      nodes[i].children.push_back(i + 1);
    }

    return nodes;
  }
}
```

The generated code is intentionally simple.

Its job is to prove that:

- the include path works
- the namespace works
- tests can include the library
- examples can include the library
- the project can be built and published

## Tests

The library template includes a basic test.

A simple test can look like this:

```cpp
#include <vix/tests/tests.hpp>
#include <mathlib/mathlib.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("mathlib basic test", [] {
    auto nodes = mathlib::make_chain(5);
    Assert::equal(nodes.size(), static_cast<std::size_t>(5));
  }));

  return TestRunner::run_all_and_exit();
}
```

Run tests with:

```bash
vix tests
```

Use tests to protect your public API.

Before publishing a new version, always run:

```bash
vix tests
```

## Examples

Examples show users how to use the library.

A basic example can look like this:

```cpp
#include <mathlib/mathlib.hpp>
#include <iostream>

int main()
{
  auto nodes = mathlib::make_chain(3);
  std::cout << "nodes=" << nodes.size() << "\n";
  return 0;
}
```

Examples are useful for:

- documentation
- smoke tests
- teaching
- verifying consumer usage

Keep examples small and focused.

## Stable target name

A library should expose a stable CMake target.

For a library named `mathlib`, the canonical target should be:

```cmake
mathlib::mathlib
```

This is important because consumers can depend on your library without guessing the internal target name.

Example consumer usage:

```cmake
target_link_libraries(app PRIVATE mathlib::mathlib)
```

A stable target name is part of the public API of a C++ library.

## Why tests and examples are separate

Tests and examples should not be forced on every consumer.

A user who installs your library usually wants the library target, not your test executables or example programs.

That is why a clean library design keeps:

```txt
library target
test targets
example targets
```

separate.

This avoids target collisions and keeps registry consumption safe.

## `vix.json`

`vix.json` is the Vix project and registry metadata file.

For a library, it should describe:

- package name
- version
- description
- license
- repository
- dependencies
- tasks
- registry metadata

Example shape:

```json
{
  "name": "mathlib",
  "version": "0.1.0",
  "type": "library",
  "description": "Small reusable C++ library.",
  "license": "MIT",
  "repo": "https://github.com/yourname/mathlib",
  "deps": [],
  "tasks": {
    "build": "vix build",
    "test": "vix tests",
    "check": "vix check --tests",
    "ci": ["vix build", "vix tests"]
  }
}
```

The exact generated shape can evolve, but the role stays the same:

```txt
vix.json = package metadata + tasks + dependency declarations
```

## Registry overview

The Vix Registry is used to discover, install, update, and publish reusable packages.

A library can be:

```txt
searched
added
installed
locked
updated
published
unpublished
```

The local registry index must be synced before package search and dependency resolution.

Sync the registry:

```bash
vix registry sync
```

Search packages:

```bash
vix search softadastra
```

Use pagination:

```bash
vix search softadastra --page 2 --limit 5
```

The registry already contains many packages.

At the time of this documentation work, your registry has around:

```txt
135 packages
```

## Search packages

Use `vix search` to find packages.

Example:

```bash
vix search softadastra
```

Expected output shape:

```txt
Search
  query : "softadastra"
  page  : 1
  limit : 5

softadastra/core  (latest: 1.7.0)
  Foundational primitives for Softadastra systems.
  repo: https://github.com/softadastra/core

softadastra/fs  (latest: 1.11.1)
  Filesystem observation and change detection layer.
  repo: https://github.com/softadastra/fs
```

Go to the next page:

```bash
vix search softadastra --page 2 --limit 5
```

Use search before adding a package.

It helps you confirm:

- package namespace
- package name
- latest version
- description
- repository URL

## Add a dependency

A library can use another library from the registry.

Example:

```bash
vix add softadastra/core@^1.7.0
```

Then install:

```bash
vix install
```

The normal dependency workflow is:

```bash
vix registry sync
vix add <namespace>/<name>[@version]
vix install
```

Examples:

```bash
vix add softadastra/core@^1.7.0
vix add softadastra/fs
vix add softadastra/json@0.3.0
```

When you run `vix add`, Vix updates dependency metadata.

If the project has `vix.json`, the dependency is added there.

If needed, Vix can create the dependency metadata structure.

Then `vix install` installs from the lock state.

## `vix.lock`

`vix.lock` records the installed dependency state.

It keeps installs reproducible.

The important rule is:

```txt
vix.json  -> what the project requests
vix.lock  -> what the project installed
```

Commit both files when building a reusable library:

```bash
git add vix.json vix.lock
git commit -m "chore: add registry dependencies"
```

This allows another developer to run:

```bash
vix install
```

and get the same dependency state.

## Install dependencies

Install project dependencies:

```bash
vix install
```

`vix install` uses `vix.lock`.

If `vix.lock` is missing, add a package first:

```bash
vix add <namespace>/<name>[@version]
```

Then run:

```bash
vix install
```

Installed dependencies are stored in the Vix project workspace and global Vix store.

## List dependencies

Show installed project dependencies:

```bash
vix list
```

This reads the project lock file and prints dependencies.

Use it after:

```bash
vix add
vix install
```

to verify what is installed.

## Check for new versions

Check if installed dependencies are behind the registry:

```bash
vix outdated
```

Check one package:

```bash
vix outdated softadastra/core
```

Machine-readable output:

```bash
vix outdated --json
```

Strict CI mode:

```bash
vix outdated --strict
```

Important rule:

```txt
vix outdated checks vix.lock
```

So it reports the installed state, not only what is written in `vix.json`.

## Update dependencies

Update dependencies:

```bash
vix update
```

Update and install:

```bash
vix update --install
```

Update one package:

```bash
vix update softadastra/core --install
```

Use this flow:

```bash
vix registry sync
vix outdated
vix update --install
vix tests
```

Then commit:

```bash
git add vix.json vix.lock
git commit -m "chore: update registry dependencies"
```

## Remove a dependency

Remove a package:

```bash
vix remove softadastra/core
```

Then reinstall remaining dependencies:

```bash
vix install
```

If needed, purge local dependency files for that package:

```bash
vix remove softadastra/core --purge
```

Use remove when the library no longer depends on a package.

## Reset project dependency state

Reset the local project state:

```bash
vix reset
```

This runs:

```txt
vix clean
vix install
```

Use this when you want to clean local build/cache state and reinstall project dependencies.

It only affects the current project.

It does not remove the global Vix directory.

## Store commands

The Vix store keeps cached dependency checkouts.

Show store path:

```bash
vix store path
```

Garbage collect unused store entries for the current project:

```bash
vix store gc
```

Use store cleanup carefully.

If other projects depend on cached entries, check what will be removed before deleting aggressively.

## Global packages

Some registry workflows can use global packages.

Install globally when supported by your package workflow:

```bash
vix add --global softadastra/core
```

List global packages:

```bash
vix list --global
```

For normal library development, prefer project dependencies.

Global packages are useful for tools or shared developer utilities.

## Publish a library

A library can be published directly to the Vix Registry.

The basic workflow is:

```bash
vix registry sync
vix publish
```

A safer release workflow is:

```bash
vix tests
git status
git add .
git commit -m "chore: prepare release"
git tag v0.1.0
vix registry sync
vix publish 0.1.0
```

`vix publish` expects a clean Git repository.

Commit your changes before publishing.

It resolves the release version, tag, commit, and package metadata.

## Dry-run publish

Before publishing for real, use:

```bash
vix publish 0.1.0 --dry-run
```

Use dry-run to check:

- package name
- version
- repository
- tag
- commit
- registry metadata

without changing the registry.

## Publish notes

You can include notes when publishing:

```bash
vix publish 0.1.0 --notes "Initial public release"
```

Use notes to explain what changed in the version.

## Cleanup after publish

If your publish workflow supports cleanup:

```bash
vix publish 0.1.0 --cleanup
```

Use cleanup when you want Vix to remove temporary registry work after publication.

## Unpublish

Unpublish is a dangerous operation.

Use it only when a package entry was published incorrectly.

Example:

```bash
vix unpublish softadastra/core@1.7.0
```

Before unpublishing, prefer publishing a fixed version when possible.

Registry consumers may already depend on the published version.

## Recommended library release workflow

Use this workflow for a normal library release:

```bash
vix registry sync
vix outdated
vix tests
vix build
git status
git add .
git commit -m "chore(release): prepare v0.1.0"
git tag v0.1.0
vix publish 0.1.0 --dry-run
vix publish 0.1.0
```

After publishing, verify:

```bash
vix registry sync
vix search mathlib
```

Then test consumption from another project:

```bash
mkdir -p /tmp/vix-consume-test
cd /tmp/vix-consume-test

vix new app --app
cd app

vix registry sync
vix add yourname/mathlib@0.1.0
vix install
vix build
```

## How another project uses your library

A consumer project runs:

```bash
vix registry sync
vix add yourname/mathlib@0.1.0
vix install
```

Then includes your header:

```cpp
#include <mathlib/mathlib.hpp>
```

And links your package target if needed by the generated build.

For a good library, the consumer should not need to know your internal folder structure.

## Versioning

Use semantic versions:

```txt
0.1.0
0.2.0
1.0.0
1.1.0
2.0.0
```

Suggested meaning:

| Version change | Use when                          |
| -------------- | --------------------------------- |
| Patch          | Bug fix, no public API break.     |
| Minor          | New feature, backward-compatible. |
| Major          | Breaking public API change.       |

Examples:

```bash
git tag v0.1.0
vix publish 0.1.0
```

```bash
git tag v0.2.0
vix publish 0.2.0
```

## Public API discipline

A library is judged by its public API.

Keep public headers stable.

Avoid exposing unnecessary internals.

Good public API:

```txt
include/mathlib/mathlib.hpp
include/mathlib/graph.hpp
include/mathlib/version.hpp
```

Avoid making consumers include:

```txt
src/internal/...
build/...
examples/...
tests/...
```

The public include directory is the contract.

## How to grow a library

Start simple:

```txt
include/mathlib/mathlib.hpp
```

When the library grows, split headers:

```txt
include/mathlib/
├── mathlib.hpp
├── graph.hpp
├── node.hpp
├── algorithms.hpp
└── version.hpp
```

Keep `mathlib.hpp` as the main convenience include:

```cpp
#pragma once

#include <mathlib/graph.hpp>
#include <mathlib/node.hpp>
#include <mathlib/algorithms.hpp>
#include <mathlib/version.hpp>
```

## Header-only vs compiled library

The generated scaffold starts as a header-only library.

That is the simplest registry-safe starting point.

Use header-only when:

- the library is small
- templates are important
- consumers should not link compiled objects
- the implementation is simple

Move to a compiled library when:

- compile time becomes too high
- implementation should be hidden
- the library has many `.cpp` files
- binary boundaries matter

For compiled libraries, keep public headers in `include/` and implementation in `src/`.

Example:

```txt
include/mathlib/mathlib.hpp
src/mathlib.cpp
```

## Using registry dependencies inside a library

A library can depend on another registry library.

Example:

```bash
vix registry sync
vix add softadastra/core@^1.7.0
vix install
```

Then document the dependency in your README.

If the dependency affects your public API, users must also receive it through the registry install workflow.

Keep dependencies intentional.

A library with too many unnecessary dependencies becomes harder to adopt.

## CI workflow

A simple CI workflow should run:

```bash
vix registry sync
vix install
vix build
vix tests
vix outdated --strict
```

For release branches, use:

```bash
vix publish <version> --dry-run
```

before publishing.

## Common mistakes

### Publishing without syncing the registry

Run:

```bash
vix registry sync
```

before searching, adding, updating, or publishing packages.

### Publishing with uncommitted changes

`vix publish` expects a clean Git repository.

Check:

```bash
git status
```

Commit before publishing.

### Forgetting to create a tag

Publish uses the release version and Git tag.

Create a tag:

```bash
git tag v0.1.0
```

Then publish:

```bash
vix publish 0.1.0
```

### Forgetting `vix.lock`

If your library uses registry dependencies, commit:

```txt
vix.json
vix.lock
```

`vix.json` records what you request.

`vix.lock` records what was installed.

### Breaking the include path

Do not move public headers randomly.

For a package named `mathlib`, this should stay stable:

```cpp
#include <mathlib/mathlib.hpp>
```

### Changing the namespace casually

Namespace is part of the public API.

Changing:

```cpp
namespace mathlib
```

to another namespace can break users.

### Adding dependencies without thinking

Every dependency becomes part of your library maintenance story.

Use `vix add` when the dependency is useful and intentional.

## What you should remember

Create a library:

```bash
vix new mathlib --lib
cd mathlib
vix build
vix tests
```

Search packages:

```bash
vix registry sync
vix search softadastra
```

Add a dependency:

```bash
vix add softadastra/core@^1.7.0
vix install
```

Check versions:

```bash
vix outdated
```

Update:

```bash
vix update --install
```

Publish:

```bash
git tag v0.1.0
vix registry sync
vix publish 0.1.0
```

The library template is the best place to learn the Vix Registry because libraries are meant to be reused, versioned, installed, and published.

## Next steps

Continue with:

- [Application template](/templates/application)
- [Backend template](/templates/backend)
- [vix.app](/guides/vix-app/)
- [Build and run](/cli/run)
- [CLI commands](/cli/)
