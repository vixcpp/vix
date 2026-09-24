# Fast Target Builds

Fast target builds explain how Vix makes repeated C++ builds faster without losing correctness.
Use this guide when you want to understand why commands such as:

```bash
vix build --build-target vix
```

can return quickly when nothing changed, while still rebuilding safely when something did change.

## The problem

C++ builds can become slow even when you only want to check one target.
A normal build tool often needs to:

```txt
load project metadata
load packages
scan files
load compile_commands.json
load build.ninja
read dependency files
build the graph
resolve dirty state
delegate to Ninja
```

That is safe, but it can be expensive when the target is already clean.
The goal of Vix is not to skip work blindly.

The goal is:

```txt
fast when safe
correct by default
fallback when needed
```

## What fast target builds do

A fast target build lets Vix return early when it can prove that a target is already up to date.

Example:

```bash
vix build --build-target vix
```

Expected output shape:

```txt
Checking vix (dev)
  ✔ Up to date in 0.3s
```

This is different from pretending that nothing changed.

Vix only takes the fast path when the build state proves that the target is still valid.

## The important idea

The important idea is simple:

```txt
a target is up to date only when Vix can prove it
```

A cached build state is not enough by itself.
The final output must still be valid.
For an executable target, this means the binary must still exist and be executable.
For a library target, the expected library output must still exist.
If the output is missing, Vix must rebuild or restore it from the artifact cache.

## What Vix checks

For a no-op target build, Vix can return early only when important build identity fields still match.

Examples:

```txt
same build signature
same project fingerprint
same target
same preset
same build type
same compiler identity
same project inputs
last target output exists
last target output is valid
artifact state is valid
```

If one of these changes, Vix should not trust the previous state.
It should rebuild, restore, or fall back to the normal build path.

## Basic usage

Build one target:

```bash
vix build --build-target vix
```

Build again without changing anything:

```bash
vix build --build-target vix
```

If the target is clean, Vix can return quickly:

```txt
Checking vix (dev)
  ✔ Up to date in 0.3s
```

## Compare with `--fast`

Vix also supports the explicit fast path:

```bash
vix build --fast --build-target vix
```

The important improvement is that normal target builds can now become fast too.

Before, the fastest no-op path was usually:

```bash
vix build --fast --build-target vix
```

Now, when the state is safe, this can also be fast:

```bash
vix build --build-target vix
```

That matters because the default command becomes smarter.

The user should not always need to think:

```txt
Should I use --fast here?
```

For clean targets, Vix can make the normal command fast when correctness is proven.

## Safety rule

The safety rule is:

```txt
state hit is not enough
the target output must still be valid
```

For example:

```bash
vix build --build-target vix
rm -f build-ninja/vix
vix build --fast --build-target vix
```

Vix must not say that the target is up to date if the binary is missing.

It must rebuild or restore the target.

## Artifact cache relationship

Fast target builds are part of a layered build model.
The local build state is the fastest no-op validation layer.
The artifact cache is the layer that can restore complete outputs.
The object cache is the layer that can reuse compiled objects.
The fallback is CMake and Ninja.

The model looks like this:

```txt
BuildState     -> fastest no-op validation
ArtifactCache  -> restore complete target
BuildGraph     -> target-aware analysis
ObjectCache    -> restore compile outputs
CMake/Ninja    -> compatibility fallback
```

Each layer has a different role.

## BuildState

`BuildState` answers:

```txt
Is this target already valid in this build directory?
```

If the answer is yes, Vix can return quickly.
This is the fastest path because there is no need to walk the full graph.

## ArtifactCache

`ArtifactCache` answers:

```txt
Do we already have this final target output in the cache?
```

For example, after deleting the build directory:

```bash
vix build --build-target vix
rm -rf build-ninja
vix build --build-target vix
```

Vix may restore the target:

```txt
Restoring vix (dev)
  ✔ Artifact cache hit
  ✔ Done
```

That avoids recompiling when the full target artifact is already known.

## BuildGraph

`BuildGraph` gives Vix target awareness.

It helps Vix understand:

```txt
which compile tasks belong to the requested target
which object files affect the target
which dependencies affect the target
which link step produces the target
```

When the project changes, this matters more than the no-op path.
A clean no-op build may finish before the graph executor is needed.
But when files change, the graph executor helps Vix rebuild only the necessary target work.

## ObjectCache

`ObjectCache` works at the source-file level.

It can restore compile outputs such as:

```txt
BuildCommand.cpp -> BuildCommand.cpp.o
BuildCommand.cpp -> BuildCommand.cpp.d
```

If the final artifact cannot be reused, object cache can still reduce compilation work.

## CMake and Ninja fallback

Vix still keeps CMake and Ninja as the compatibility fallback.
If Vix cannot prove that a target is clean, it should delegate safely.
The fallback is not a failure.

It is part of the correctness model.

```txt
if unsure, delegate
```

## Example benchmark

On one Vix.cpp benchmark setup, the result looked like this:

```txt
vix build --fast --build-target vix                 313 ms
vix build --build-target vix                        315 ms
VIX_GRAPH_EXECUTOR=0 vix build --build-target vix   309 ms
```

The important conclusion is not only that `--fast` is quick.

The important conclusion is:

```txt
normal no-op target builds are fast by default
```

That is better for daily development.

## Why Graph Executor still matters

When the benchmark shows similar no-op times with and without the graph executor, it does not mean the graph executor is useless.
It means the clean no-op path is being solved earlier.
For unchanged targets, the fastest layer wins.
For changed projects, the graph executor still matters because it gives Vix target intelligence.

It can help with:

```txt
target resolution
target closure
dirty compile task detection
safe partial rebuilds
safe delegation
```

## Explain mode

Use `--explain` when you want to know why Vix rebuilds something:

```bash
vix build --explain --build-target vix
```

Example output for a source change:

```txt
Rebuilding BuildCommand.cpp
  reason: source file changed

Relinking vix
  reason: object file changed
```

For some header changes, Vix may not yet be able to map the header precisely to selected compile tasks.
In that case, it should avoid lying.

A safe explanation can look like this:

```txt
Project input changed
  reason: dependency changed, delegating target to Ninja

Relinking vix
  reason: target may depend on changed input
```

That is the right behavior.

If Vix cannot prove that the target is clean, it must not say that it is clean.

## When Vix should return early

Vix can return early when:

```txt
the build state matches
the requested target matches
the preset matches
the build type matches
the compiler identity matches
the project fingerprint matches
the target output still exists
the target output is valid
```

This is the ideal no-op path.

## When Vix should rebuild

Vix should rebuild when:

```txt
a source file changed
a header file changed
CMakeLists.txt changed
vix.app changed
vix.json changed
vix.lock changed
compiler flags changed
the requested target changed
the build preset changed
the compiler changed
the target output is missing
```

A rebuild can still be optimized by the build graph, object cache, and artifact cache.

## When Vix should fall back

Vix should fall back when it cannot prove safety.

Examples:

```txt
unknown generated input
ambiguous header dependency
unsupported build graph shape
missing dependency metadata
incomplete state file
invalid cache entry
```

The fallback should be conservative.
A false cache miss is acceptable.
A false cache hit is dangerous.

## Recommended workflow

Use normal target builds first:

```bash
vix build --build-target vix
```

Use `--explain` when investigating rebuild behavior:

```bash
vix build --explain --build-target vix
```

Use `--fast` when you explicitly want the fastest state-based check:

```bash
vix build --fast --build-target vix
```

Use a clean rebuild only when needed:

```bash
vix build --clean --build-target vix
```

## Common workflows

```bash
# Build one target
vix build --build-target vix

# Repeat the same target build
vix build --build-target vix

# Explain why Vix rebuilds
vix build --explain --build-target vix

# Use the explicit fast path
vix build --fast --build-target vix

# Force a clean build
vix build --clean --build-target vix
```

## Common mistakes

### Thinking fast means unsafe

Fast target builds are only fast when Vix can prove that the target is valid.

The rule is:

```txt
fast only after validation
```

### Trusting state without checking outputs

A build state hit is not enough.

The final output must still exist and be valid.

### Expecting every changed build to finish in 300 ms

The 300 ms path is for clean no-op target builds.
If source, headers, flags, manifests, or build configuration changed, Vix must do more work.

### Disabling fallback

Fallback is part of correctness.
If Vix is unsure, delegating to Ninja is the safe behavior.

## Related commands

| Command               | Purpose                               |
| --------------------- | ------------------------------------- |
| `vix build`           | Build a project or target             |
| `vix build --explain` | Explain rebuild decisions             |
| `vix build --fast`    | Use the explicit fast validation path |
| `vix clean`           | Remove local generated state          |
| `vix reset`           | Clean and reinstall dependencies      |
| `vix info`            | Inspect cache and store paths         |

## What you should remember

Fast target builds are not about skipping work blindly.
They are about proving that work is unnecessary.

The principle is:

```txt
fast when safe
correct by default
fallback when needed
```

For developers, this makes the ordinary command smarter:

```bash
vix build --build-target vix
```

If nothing changed, it can return quickly.
If something changed, it rebuilds.
If Vix is unsure, it falls back.
