# Artifact Cache

The artifact cache is the target-level cache layer in `vix build`.

Use this guide when you want to understand how Vix can reuse complete build outputs such as executables, libraries, compiled packages, and generated package artifacts.

## The problem

Object caching avoids recompiling individual source files.

But C++ builds often spend time beyond compilation.

A project may still need to:

```txt
archive static libraries
link executables
copy resources
prepare generated outputs
rebuild dependency packages
produce final artifacts
```

Even when object files are already available, the final target may still need work.
The artifact cache solves a larger question:

```txt
Have we already built this target or package with the same build identity?
```

If the answer is yes, Vix can restore or reuse the complete output instead of rebuilding it.

## What the artifact cache does

The artifact cache stores larger reusable build outputs.

Examples:

```txt
executable binary
static library
shared library
compiled package prefix
generated install prefix
final linked target
package artifact
```

For example:

```txt
build-ninja/vix
build-ninja/lib/libvix_core.a
dist/app@1.0.0.vixpkg
```

can be treated as artifacts when their full build identity is known.

## Object cache vs artifact cache

Object cache works at the source-file level:

```txt
src/server.cpp -> server.cpp.o
src/server.cpp -> server.cpp.d
```

Artifact cache works at the target or package level:

```txt
object files + libraries + link options -> executable
package source + compiler + options -> compiled package
```

The difference is:

```txt
ObjectCache
  reuses compile outputs

ArtifactCache
  reuses complete build outputs
```

A strong build system needs both.

## Why artifact caching matters

Artifact caching matters because repeated builds often rebuild outputs that are already known.

It helps with:

```txt
clean builds
CI builds
compiled dependencies
large libraries
final executables
local repeated builds
cross-project package reuse
```

The goal is not magic.

The goal is disciplined reuse:

```txt
same inputs
same build identity
same artifact
reuse safely
```

## The safety rule

The safety rule is:

```txt
reuse an artifact only when its full build identity matches
```

A target name is not enough.
A file path is not enough.
A previous successful build state is not enough.
Vix must know that the current build identity matches the cached artifact identity.
A false cache miss is acceptable.
A false cache hit is dangerous.

## What is an artifact?

An artifact is a reusable build output.

Examples:

```txt
binary executable
static library
shared library
compiled dependency
package install prefix
generated distribution folder
.vixpkg package
```

For example:

```txt
build-ninja/bin/server
```

is an artifact.

So is:

```txt
build-ninja/lib/libauth.a
```

A compiled registry package can also be an artifact.

## Artifact identity

An artifact needs a stable identity.

That identity can include:

```txt
package name
package version
target name
target type
compiler identity
linker identity
build type
target triple
source fingerprint
dependency fingerprint
build options
link options
resource fingerprint
```

A Debug binary is not the same as a Release binary.
A GCC build is not the same as a Clang build.
A Linux artifact is not the same as a Windows artifact.

## Fingerprint

A fingerprint is a compact representation of build-relevant inputs.
For an artifact, the fingerprint should include the inputs that can affect the final output.

Examples:

```txt
source file hashes
header file hashes
manifest hash
CMake configuration hash
compiler identity
compiler flags
linker identity
link options
target triple
build type
package versions
dependency artifact hashes
resources
```

If the fingerprint changes, the artifact must not be reused.

## Basic flow

The artifact cache flow is:

```txt
1. Resolve target or package identity
2. Compute artifact fingerprint
3. Check artifact cache
4. If hit, verify cached output
5. Restore or reuse artifact
6. If miss, build normally
7. Store artifact after successful build
```

The important part is verification.
A cache entry must be complete and valid before Vix uses it.

## Local build state vs artifact cache

Local build state and artifact cache answer different questions.
Local build state answers:

```txt
Is this target already valid in this build directory?
```

Artifact cache answers:

```txt
Do we have this complete output somewhere in the cache?
```

Both are useful.
Build state is fastest for no-op builds.
Artifact cache is useful after clean builds, across projects, and in CI.

## Relationship with fast target builds

Fast target builds use build state first.
If the target is already valid, Vix can return early:

```bash
vix build --build-target vix
```

Expected output shape:

```txt
Checking vix (dev)
  ✔ Up to date in 0.3s
```

But if the build directory is missing, build state alone cannot help.
That is where artifact cache matters.

## Clean build with warm artifact cache

Example:

```bash
vix build --build-target vix
rm -rf build-ninja
vix build --build-target vix
```

If the final target artifact is cached, Vix can restore it:

```txt
Restoring vix (dev)
  ✔ Artifact cache hit
  ✔ Done
```

This avoids recompiling and relinking when the final output is already known.

## Missing output safety

A state hit is not enough.
If the target output is missing, Vix must not say the target is up to date.

Example:

```bash
vix build --build-target vix
rm -f build-ninja/vix
vix build --fast --build-target vix
```

The correct behavior is:

```txt
target output missing
  rebuild or restore from artifact cache
```

Not:

```txt
Up to date
```

The final output must still exist and be valid.

## Layered build model

Artifact cache fits into the Vix build pipeline as a higher-level reuse layer.

The model is:

```txt
BuildState     -> fastest no-op validation
ArtifactCache  -> restore complete target
BuildGraph     -> target-aware analysis
ObjectCache    -> restore compile outputs
CMake/Ninja    -> compatibility fallback
```

Each layer has a job.

## BuildState

BuildState answers:

```txt
Can Vix prove this target is already valid right now?
```

If yes, Vix returns immediately.

This is the fastest path.

## ArtifactCache

ArtifactCache answers:

```txt
Can Vix restore the full target output?
```

If yes, Vix can avoid compile and link work.
This is especially useful after deleting the build directory.

## BuildGraph

BuildGraph answers:

```txt
What does this target depend on?
```

It gives Vix target awareness.
When the artifact cache misses, the graph helps Vix decide what work is necessary.

## ObjectCache

ObjectCache answers:

```txt
Can Vix restore individual compile outputs?
```

If the final artifact cannot be reused, object cache can still avoid recompiling files that have not changed.

## CMake and Ninja fallback

CMake and Ninja remain the compatibility fallback.
If Vix cannot prove that an artifact is safe, it should delegate.
The fallback is part of correctness.

```txt
if unsure, fallback
```

## Package artifacts

Package artifacts are one of the best use cases for artifact caching.
Packages are often reused across projects.

Example:

```txt
project A builds package softadastra/json
project B uses the same package
project B reuses the cached compiled artifact
```

This can avoid repeated dependency builds.

## Header-only packages

Header-only packages do not need compiled binaries.
For header-only packages, the artifact may only contain:

```txt
include/
manifest.json
metadata
```

No library output is needed.
This distinction matters.
A header-only package should not be treated like a compiled package.

## Compiled packages

Compiled packages can produce:

```txt
static libraries
shared libraries
headers
CMake config files
generated metadata
```

A compiled package artifact can be stored as a reusable prefix:

```txt
include/
lib/
share/
manifest.json
```

Then another project can reuse it when the identity matches.

## CMAKE_PREFIX_PATH integration

For compiled package artifacts, Vix can expose the cached prefix to CMake.

If an artifact contains:

```txt
lib/cmake/package/packageConfig.cmake
```

Vix can add the artifact prefix to:

```txt
CMAKE_PREFIX_PATH
```

This keeps artifact reuse compatible with normal CMake package discovery.

## Source fallback

If no compiled artifact exists, Vix can fall back to source integration.

The fallback model is:

```txt
artifact cache hit
  reuse compiled artifact

artifact cache miss
  build package from source
  store artifact after success
```

This gives immediate correctness and later speed.

## Final target artifacts

A final executable can also be cached.

Example:

```txt
build-ninja/vix
```

If the full target identity matches, Vix can restore the executable.
This is different from object cache.
Object cache restores `.o` files.
Artifact cache can restore the final binary.

## Artifact metadata

Each artifact should have metadata.

Example shape:

```json
{
  "name": "vix",
  "target": "vix",
  "type": "executable",
  "compiler": "g++",
  "compiler_version": "13.3.0",
  "linker": "mold",
  "build_type": "Debug",
  "target_triple": "x86_64-linux-gnu",
  "fingerprint": "...",
  "created_at": "...",
  "outputs": ["build-ninja/vix"]
}
```

The exact format can evolve.
The goal is explainability.
Vix should know what an artifact is and why it is valid.

## Cache layout

A possible artifact cache layout is:

```txt
~/.vix/cache/build/
  artifacts/
    <target-or-package>/
      <compiler>/
        <build-type>/
          <target-triple>/
            <fingerprint>/
              manifest.json
              outputs/
```

For package artifacts:

```txt
~/.vix/cache/build/
  packages/
    softadastra.json/
      gcc-13.3.0/
        release/
          x86_64-linux-gnu/
            <fingerprint>/
              include/
              lib/
              share/
              manifest.json
```

The exact layout can change.

The identity rule should not.

## Artifact verification

Before using a cached artifact, Vix should verify:

```txt
metadata exists
expected files exist
artifact type matches
fingerprint matches
compiler identity matches
build type matches
target triple matches
outputs are complete
```

If verification fails, treat it as a cache miss.
Do not use broken cache entries.

## Atomic restore

Artifact restoration should avoid partial outputs.

A safe restore can use this model:

```txt
restore into temporary location
verify expected files
move into final location
update build state
```

This prevents corrupted build directories when a restore fails halfway.

## Resource files

Resources can affect runtime output.

Example `vix.app` resources:

```txt
resources = [
  ".env=.env",
  "assets=assets",
]
```

If resources are part of the final artifact, their content should be included in the artifact identity.
If a resource changes, the artifact should not be reused without updating the restored output.

## Artifact cache and `vix.app`

`vix.app` is a strong input for artifact caching because it is structured.

It can describe:

```txt
name
type
sources
include_dirs
defines
modules
deps
packages
links
resources
output_dir
```

This makes it easier to compute a stable artifact fingerprint.
For `vix.app` projects, Vix has more direct knowledge than with arbitrary CMake.

## Artifact cache and generated CMake

When a `vix.app` project generates internal CMake, the artifact cache can still work.

The identity can include:

```txt
vix.app hash
generated CMake hash
project inputs
build options
compiler identity
link options
```

This allows artifact caching before a fully native build path exists.

## Artifact cache and CMake projects

For arbitrary CMake projects, Vix should be conservative.
CMake can contain custom logic.

So Vix should base decisions on generated build outputs such as:

```txt
compile_commands.json
build.ninja
CMakeCache.txt
dependency files
build state
```

If the artifact identity is incomplete, the cache should miss.

## Artifact cache and CI

CI often starts from a clean workspace.
That makes artifact caching useful.
A CI workflow can restore:

```txt
~/.vix/cache/build/
```

before running:

```bash
vix build --preset release
```

The CI cache key must include:

```txt
OS
compiler
compiler version
target triple
build type
dependency versions
Vix version
```

Unsafe CI cache keys can cause incorrect reuse.

## Artifact cache and cross-compilation

Cross-compilation requires separate artifact identities.

An artifact built for:

```txt
x86_64-linux-gnu
```

cannot be reused for:

```txt
aarch64-linux-gnu
```

Target triple must be part of the artifact key.
Sysroot and toolchain metadata may also need to be included.

## Artifact cache and build type

Debug and Release artifacts are different.
Debug may include:

```txt
debug symbols
assertions
different optimization level
```

Release may include:

```txt
optimization
NDEBUG
different link behavior
```

Build type must be part of the artifact identity.

## Artifact cache and compiler identity

Artifacts depend on compiler and ABI.

Important compiler identity fields include:

```txt
compiler name
compiler path
compiler version
standard library
target triple
ABI-relevant flags
```

For safety, Vix should separate artifacts by compiler identity.

## Artifact cache and linker identity

The linker can also affect final outputs.

For linked artifacts, the identity can include:

```txt
linker name
linker version
link options
library order
runtime paths
static or shared mode
```

If link identity changes, the final artifact should miss.

## Cache invalidation

Artifact cache invalidation happens naturally when fingerprints change.

Examples:

```txt
source changed
header changed
manifest changed
dependency changed
compiler changed
build type changed
target triple changed
link options changed
resource changed
```

The old artifact can remain in the cache.
It just will not match the new identity.

## Cache eviction

Artifact caches can become large.

Vix can eventually support eviction policies such as:

```txt
maximum cache size
least recently used
maximum age
per-target cleanup
per-package cleanup
manual clean
```

Eviction should never affect correctness.
It only affects whether future builds are cache hits or misses.

## Explainable cache behavior

Artifact cache decisions should be visible.

Useful messages include:

```txt
artifact cache hit: vix
artifact cache miss: source fingerprint changed
artifact cache miss: compiler changed
artifact cache miss: target output missing
artifact restored: build-ninja/vix
artifact stored: vix
```

A fast build that cannot be explained feels unsafe.

Vix should make cache behavior understandable.

## `--explain`

Use `--explain` to understand build decisions:

```bash
vix build --explain --build-target vix
```

Example output:

```txt
Restoring vix
  reason: artifact cache hit

Skipping compile tasks
  reason: final target restored
```

If the artifact cannot be reused:

```txt
Artifact cache miss
  reason: build fingerprint changed

Rebuilding BuildCommand.cpp
  reason: source file changed
```

This helps developers trust the result.

## Example workflow

Build a target:

```bash
vix build --build-target vix
```

Remove the build directory:

```bash
rm -rf build-ninja
```

Build the same target again:

```bash
vix build --build-target vix
```

If the artifact cache has a valid target artifact, Vix can restore it instead of rebuilding.

## Example output shape

```txt
Restoring vix (dev)
  ✔ Artifact cache hit
  ✔ Restored target output
  ✔ Done
```

If the artifact is not available:

```txt
Building vix (dev)
  i Artifact cache miss
  i Falling back to graph build
  ✔ Built
```

Both are valid.

A miss is not an error.

## Recommended workflow

Use normal builds:

```bash
vix build
```

Use target builds for specific outputs:

```bash
vix build --build-target vix
```

Use explain mode when diagnosing cache behavior:

```bash
vix build --explain --build-target vix
```

Use clean build only when needed:

```bash
vix build --clean
```

Use `vix info` to inspect cache paths:

```bash
vix info
```

## Common workflows

```bash
# Build normally
vix build

# Build one target
vix build --build-target vix

# Explain cache decisions
vix build --explain --build-target vix

# Test artifact restoration after build directory removal
vix build --build-target vix
rm -rf build-ninja
vix build --build-target vix

# Inspect cache state
vix info
```

## Common mistakes

### Thinking artifact cache replaces object cache

Artifact cache and object cache solve different problems.
Artifact cache reuses complete outputs.
Object cache reuses compile outputs.

### Trusting a target name only

The target name is not enough.

The full build identity must match.

### Reusing artifacts across compilers blindly

C++ artifacts are compiler and ABI sensitive.
Compiler identity must be part of the cache key.

### Ignoring resources

If resources affect the final output, they must be part of the artifact fingerprint.

### Treating cache misses as errors

A cache miss is normal.
It only means Vix must build normally.

## Good first version

A practical first artifact cache can support:

```txt
local build state
target artifact metadata
final binary restore
compiled package prefix reuse
basic fingerprinting
safe verification
clear hit and miss logs
```

It does not need remote caching on day one.
Correctness comes first.

## Later improvements

Later improvements can include:

```txt
remote artifact cache
link result cache
artifact signing
cache eviction
CI cache helpers
team cache sharing
explainable miss reasons
native vix.app integration
artifact trust policies
```

The system should grow in layers.

## Security considerations

A build cache stores executable code.
Local-only artifact cache has lower risk.
Remote artifact cache needs stronger safety.

Possible mechanisms:

```txt
content hashes
signatures
trusted cache sources
metadata verification
sandboxed extraction
```

For Vix, the best path is:

```txt
local correctness first
remote sharing later
trust before speed
```

## Related commands

| Command               | Purpose                                |
| --------------------- | -------------------------------------- |
| `vix build`           | Build a project or target              |
| `vix build --explain` | Explain rebuild and cache decisions    |
| `vix info`            | Show cache paths and local state       |
| `vix clean`           | Remove project-local generated state   |
| `vix reset`           | Clean and reinstall dependencies       |
| `vix pack`            | Create distributable package artifacts |
| `vix verify`          | Verify package artifacts               |
| `vix cache`           | Store verified packages locally        |

## What you should remember

The artifact cache is the large-output reuse layer.

It can restore:

```txt
executables
libraries
compiled packages
package artifacts
```

Its rule is:

```txt
reuse only when the full build identity matches
```

It sits above the object cache.
If the artifact cache hits, Vix can avoid compile and link work.
If it misses, Vix can still use the build graph and object cache.
If Vix is unsure, it falls back.
The principle remains:

```txt
fast when safe
correct by default
fallback when needed
```
