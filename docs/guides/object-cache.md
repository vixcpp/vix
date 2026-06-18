# Object Cache

The object cache is the source-file cache layer in `vix build`.

Use this guide when you want to understand how Vix can avoid recompiling C++ files that were already compiled with the same inputs.

## The problem

C++ compilation is expensive because each source file can pull in many headers.

A small file like:

```txt
src/main.cpp
```

can indirectly depend on:

```txt
include/app/App.hpp
include/app/Routes.hpp
include/app/Config.hpp
include/vix.hpp
many standard library headers
many third-party headers
```

Without caching, the compiler may repeat work that has already been done before.
The goal of the object cache is simple:

```txt
same compile identity
same source inputs
same object output
reuse safely
```

## What the object cache does

The object cache stores and restores compile outputs.

A compile task usually produces:

```txt
source file -> object file
source file -> dependency file
```

For example:

```txt
src/server.cpp -> server.cpp.o
src/server.cpp -> server.cpp.d
```

The object cache can reuse those outputs when the compile identity has not changed.

## Object cache vs artifact cache

The object cache works at the source-file level.
The artifact cache works at the target level.

```txt
ObjectCache
  reuses .o and .d files

ArtifactCache
  reuses final binaries, libraries, or package outputs
```

They are complementary.

A strong Vix build can use both:

```txt
ArtifactCache hit
  restore final target
  skip compile and link

ArtifactCache miss
  check ObjectCache
  restore known compile outputs
  compile only missing or dirty files
  link target
```

## Why object caching matters

Object caching helps when:

```txt
a clean build directory was removed
a few source files changed
a target shares compile outputs with another build
a previous compile result is still valid
CI restored the cache
a dependency did not really change
```

It is especially useful for large C++ projects where many files compile independently.

## The safety rule

The safety rule is:

```txt
reuse an object file only when the full compile identity matches
```

A cached object file is not valid just because the source file name is the same.
The compile identity must include enough data to avoid false reuse.
A false cache miss is acceptable.
A false cache hit is dangerous.

## Compile identity

An object cache key should include the inputs that affect compilation.

Examples:

```txt
source file content
included header content
compiler path
compiler version
compiler flags
include directories
compile definitions
C++ standard
target triple
build type
precompiled configuration
dependency file state
```

If any important input changes, the object cache must miss.

## Source file fingerprint

The source file content is part of the cache identity.

If this file changes:

```txt
src/BuildCommand.cpp
```

then the cached object for that exact old content is not valid anymore.
Vix should rebuild that source file or restore a matching object from the cache if one exists.

## Header dependency fingerprint

Headers are part of the compile identity.

If a source file includes:

```cpp
#include <vix/cli/commands/BuildCommand.hpp>
```

and that header changes, the object that depends on it may no longer be valid.
That is why dependency files matter.

A compiler-generated dependency file can describe what headers were used by a compile task:

```txt
BuildCommand.cpp.o: BuildCommand.cpp BuildCommand.hpp BuildOptions.hpp
```

Vix can use this information to avoid guessing.

## Dependency files

The object cache should preserve dependency files together with object files.
For a compile output, Vix may store:

```txt
object file
dependency file
metadata
compile fingerprint
```

Example:

```txt
BuildCommand.cpp.o
BuildCommand.cpp.d
metadata.json
```

The `.d` file helps Vix understand which headers were part of the compile task.

## What gets restored

On an object cache hit, Vix can restore:

```txt
.o file
.d file
```

This means the build directory can regain the compile output without running the compiler again.
After that, the linker may still need to run if the final target is missing or dirty.

## Basic model

The object cache flow is:

```txt
1. Build compile task identity
2. Compute fingerprint
3. Check object cache
4. If hit, restore .o and .d
5. If miss, run compiler
6. Store .o and .d after successful compile
```

This is conservative and explainable.

## Relationship with BuildGraph

The BuildGraph tells Vix what compile tasks exist and which target needs them.
The object cache tells Vix whether each compile task can be reused.

Together:

```txt
BuildGraph
  selects compile tasks for the target

ObjectCache
  decides which selected compile tasks can be restored
```

This helps target-aware builds.

If you run:

```bash
vix build --build-target vix
```

Vix should focus on the compile tasks needed by the `vix` target.
It should not assume the whole project must rebuild.

## Relationship with BuildState

BuildState is faster than object cache.

BuildState answers:

```txt
Is this target already valid?
```

ObjectCache answers:

```txt
Can this compile output be reused?
```

So the order is:

```txt
BuildState hit
  return early

BuildState miss
  check ArtifactCache

ArtifactCache miss
  use BuildGraph and ObjectCache
```

## Relationship with ArtifactCache

ArtifactCache is above ObjectCache.

If the final binary is already cached, restoring the full artifact is better than restoring individual objects.

Example:

```txt
ArtifactCache hit
  restore build-ninja/vix
  done

ArtifactCache miss
  restore object files where possible
  compile remaining files
  link
```

Object cache is still valuable when the final artifact cannot be reused.

## Relationship with CMake and Ninja

Vix can use object caching while still preserving CMake and Ninja compatibility.
For CMake projects, Vix should be conservative.

It can read:

```txt
compile_commands.json
build.ninja
dependency files
CMakeCache.txt
```

Then it can build cache keys from real generated build data instead of guessing.

If Vix is unsure, it should delegate to Ninja.

## Cache key examples

A cache key can be built from data like:

```txt
compiler: /usr/bin/g++
compiler_version: GCC 13.3.0
standard: c++20
source_hash: ...
header_hashes: ...
compile_flags_hash: ...
include_dirs_hash: ...
defines_hash: ...
target_triple: x86_64-linux-gnu
build_type: Debug
```

The exact implementation can evolve.

The important rule stays stable:

```txt
same compile identity -> safe reuse
different compile identity -> cache miss
```

## Example workflow

Build a project:

```bash
vix build
```

Clean the build directory:

```bash
rm -rf build-ninja
```

Build again:

```bash
vix build
```

If the object cache is warm, Vix can restore compile outputs instead of recompiling every file.
The final link may still run if the target output is missing.

## Example output shape

A verbose build can show object cache behavior:

```txt
Object cache
  selected: 42
  hits: 39
  compiled: 3
  stored: 3
```

This means:

```txt
42 compile tasks were considered
39 were restored from cache
3 were compiled
3 new outputs were stored
```

## Why object cache hits can still relink

Restoring object files does not always mean the final binary exists.

For example:

```bash
rm -f build-ninja/vix
vix build --build-target vix
```

Even if all object files are restored, Vix may still need to link:

```txt
Relinking vix
  reason: target output missing
```

That is correct.

Object cache avoids recompilation.
Artifact cache can avoid relinking.

## When the object cache should hit

The object cache can hit when:

```txt
source file content is unchanged
headers used by the source are unchanged
compiler identity is unchanged
compile flags are unchanged
include directories are unchanged
definitions are unchanged
build type is unchanged
target platform is unchanged
```

## When the object cache should miss

The object cache should miss when:

```txt
source file changed
included header changed
compiler changed
C++ standard changed
compile flags changed
include directories changed
compile definitions changed
target architecture changed
build type changed
dependency data is missing or invalid
cache entry is incomplete
```

## Missing dependency data

If dependency data is missing, Vix should be careful.
For example, if Vix cannot know which headers affected a compile task, it should not pretend that the object is safe.

The safe behavior is:

```txt
dependency information missing
  cache miss or delegate
```

This avoids stale object reuse.

## Header changes

Header changes are one of the hardest parts of C++ caching.
A header can affect many source files.

Example:

```txt
include/vix/cli/BuildOptions.hpp
```

If this header changes, every source file that includes it directly or indirectly may need recompilation.
The object cache should rely on dependency data and fingerprints, not only timestamps.

## Timestamp is not enough

Timestamps are useful but not sufficient.
A file can have a different timestamp with the same content.
A file can also be restored in a way that changes timestamps.
A safer cache uses content fingerprints.

The principle is:

```txt
content identity beats timestamp identity
```

## Cache metadata

Each cached object entry should have metadata.

Example shape:

```json
{
  "source": "src/BuildCommand.cpp",
  "object": "BuildCommand.cpp.o",
  "deps": "BuildCommand.cpp.d",
  "compiler": "g++",
  "compiler_version": "13.3.0",
  "standard": "c++20",
  "build_type": "Debug",
  "target_triple": "x86_64-linux-gnu",
  "fingerprint": "..."
}
```

The exact format can change.
The goal is to make cache behavior inspectable and explainable.

## Store location

Object cache data can live under the Vix cache area.

A possible layout:

```txt
~/.vix/cache/build/objects/
  <compiler>/
    <target-triple>/
      <build-type>/
        <fingerprint>/
          object.o
          deps.d
          metadata.json
```

The exact layout can evolve.

The important part is stable identity.

## Local and global reuse

Object cache can help within one project.
It can also help across projects when the compile identity is identical.
Cross-project reuse must be conservative because paths, flags, includes, and dependencies can differ.
If path-sensitive compiler flags are used, they must be included in the key.

## CI usage

In CI, object cache can be useful if the cache directory is restored before build.

A CI workflow can restore:

```txt
~/.vix/cache/build/
```

Then run:

```bash
vix build --preset release
```

But the cache key must include:

```txt
OS
compiler
compiler version
target architecture
build type
dependency versions
```

Otherwise the cache can become unsafe.

## Explainable object cache

Vix should make cache decisions visible.

Useful messages include:

```txt
object cache hit: BuildCommand.cpp
object cache miss: source file changed
object cache miss: header changed
object cache miss: compiler flags changed
object cache miss: compiler changed
object cache restored: 39 object(s)
```

This helps developers trust the build system.

## `--explain`

Use `--explain` to understand rebuild decisions:

```bash
vix build --explain
```

For a source change:

```txt
Rebuilding BuildCommand.cpp
  reason: source file changed
```

For a header change:

```txt
Rebuilding BuildCommand.cpp
  reason: included header changed
```

If Vix cannot map the change precisely, it should say so:

```txt
Project input changed
  reason: dependency changed, delegating to Ninja
```

That is better than claiming false precision.

## Object cache and `vix.app`

`vix.app` makes object caching easier because the project inputs are structured.

It can explicitly describe:

```txt
sources
include_dirs
defines
links
compile_options
packages
resources
output_dir
```

This gives Vix a cleaner path to compute compile identities.
For generated CMake projects, Vix can still use the generated CMake and Ninja files as the compatibility layer.

## Object cache and CMake projects

For arbitrary CMake projects, Vix should avoid guessing.
CMake can run custom logic.
So Vix should use generated build outputs:

```txt
compile_commands.json
build.ninja
CMakeCache.txt
dependency files
```

This keeps caching compatible with existing C++ projects.

## Good first implementation

A practical object cache should start with:

```txt
compile command fingerprinting
source content hash
dependency file preservation
object file preservation
compiler identity
build type
target triple
safe restore
clear hit and miss logs
```

This is enough to provide real value without pretending to solve every C++ edge case on day one.

## Later improvements

Later improvements can include:

```txt
more precise header graph tracking
better explain output
remote object cache
cache eviction
content-addressed storage
cross-project reuse
toolchain-aware cache namespaces
debug info path normalization
```

The system should grow in layers.

## Common workflows

```bash
# Normal build
vix build

# Build a specific target
vix build --build-target vix

# Explain rebuild decisions
vix build --explain --build-target vix

# Force clean build when needed
vix build --clean

# Inspect Vix cache paths
vix info
```

## Common mistakes

### Thinking object cache avoids linking

Object cache avoids recompiling source files.
It does not always avoid linking.
Artifact cache is the layer that can restore final target outputs.

### Reusing objects without compiler identity

Object files depend on compiler and ABI details.
Compiler identity must be part of the cache key.

### Ignoring headers

In C++, headers are part of compilation.
A valid object cache must account for included headers.

### Trusting timestamps only

Timestamps are not strong enough for safe reuse.
Use content fingerprints and dependency information.

### Treating cache misses as failures

A cache miss is normal.
It only means Vix must compile normally.

## Related commands

| Command               | Purpose                              |
| --------------------- | ------------------------------------ |
| `vix build`           | Build the project                    |
| `vix build --explain` | Explain rebuild decisions            |
| `vix build --clean`   | Force a clean rebuild                |
| `vix info`            | Show cache and store paths           |
| `vix clean`           | Remove project-local generated state |
| `vix reset`           | Clean and reinstall dependencies     |

## What you should remember

The object cache is the compile-output reuse layer.

It stores and restores:

```txt
.o files
.d files
compile metadata
```

Its rule is:

```txt
reuse only when the full compile identity matches
```

It does not replace CMake or Ninja.
It works with the build graph, artifact cache, build state, and fallback path to make repeated C++ builds faster without becoming fragile.
