# Vix-Owned Build State Contract

Status: FROZEN BUILD-STATE CONTRACT FOR CURRENT VIX

## 1. Ownership law

The source tree belongs to the user. Project-local build state belongs to Vix. Vix-owned project state lives under `.vix/`. Users do not manage build directories.

```text
backend requires a build tree
!=
user manages a build tree
```

There is no public build-directory concept. Vix selects and manages its backend state location.

## 2. Project layout

Canonical conceptual layout:

```text
project/
├── user sources/configuration
└── .vix/
    ├── build/
    │   └── <context-id>/
    │       ├── context.toml
    │       ├── source/       # only where Vix generates backend input
    │       └── backend/      # backend-owned build state and artifacts
    └── packages/             # separately materialized package content
```

Exact internal names may follow implementation conventions, but the ownership boundaries are frozen. Do not put package materialization inside a build context. Authoritative project configuration remains in user-owned project files, never solely in ignored `.vix/` state.

## 3. Build-context identity

A context isolates incompatible mutable build state. Its canonical identity includes at least:

- build-state schema version;
- project kind/backend;
- configuration;
- platform and ABI;
- compiler/toolchain identity;
- generator/backend identity;
- diagnostic mode (`none`, `san`, `asan`, `ubsan`, or `tsan`);
- external-CMake configure arguments, when applicable.

Target name is not part of the identity merely because a different target is selected. Targets in the same compatible build graph share one context.

The `<context-id>` directory component is internal and deterministic, derived from a canonical serialization of the BuildContext identity. It is opaque and is not public CLI syntax. Store readable identity data in `context.toml` for diagnostics and inspection; users are not expected to memorize context hashes.

## 4. CMake arguments and context isolation

External-CMake configure arguments supplied through `--cmake` are part of context identity. Preserve their sequence because argument ordering may be semantically relevant.

Thus:

```text
--cmake=-DFOO=ON
```

and:

```text
--cmake=-DFOO=OFF
```

select different contexts. Returning to an identical prior configuration may reuse its context. Arbitrary configure changes must not contaminate an unrelated cache.

## 5. Native generated backend input

Native generated CMake source must be context-specific when its contents or configuration can vary with context. The preferred format-1 layout is:

```text
.vix/build/<context-id>/source/
.vix/build/<context-id>/backend/
```

The first directory holds generated backend input where required; the second holds CMake/Ninja state, objects, compile database, and artifacts. Vix must not place authoritative user source or configuration there.

## 6. External CMake

For an external CMake project:

```text
source directory = user project root
binary directory = .vix/build/<context-id>/backend/
```

Vix does not use or take ownership of these directories merely because they exist:

```text
project/build/
project/build-debug/
project/build-release/
project/cmake-build-*/
project/out/
```

They remain user-owned and `--clean` never deletes them. CMake configure arguments control project configuration only; they do not choose Vix's binary directory.

## 7. Multi-configuration backends

Format 1 prioritizes context isolation and predictable behavior over minimizing backend tree count. Vix may use separate contexts for development and release even when a backend could technically host both configurations in one tree. This keeps context behavior consistent across single- and multi-configuration backends. Generator-specific multi-config details are not public CLI concepts.

## 8. `--clean`

`vix build --clean` identifies the requested context, removes only that Vix-owned context, recreates/configures it, and builds. It does not:

- delete all of `.vix/` or every build context;
- invoke an arbitrary backend clean target;
- delete package materialization, lock state, or global stores;
- delete source/configuration files;
- delete user-owned build directories.

Other valid Vix contexts remain available.

## 9. Artifacts

Physical artifact paths are internal implementation details. Project execution uses the project model's `Target`, `TargetCapabilities`, and `TargetArtifact`. For example, `vix run app` resolves the configured target artifact; it does not search conventional build directories or guess binary names. Users need not know artifact locations for normal build, run, or test operations.

## 10. Single-source mode

Standalone invocations:

```text
vix build file.cpp
vix run file.cpp
```

do not require creation of a project-local `.vix/`. Build/cache state may remain in Vix's existing fingerprinted user cache.

`vix build file.cpp` does not export an executable into the current directory by default. The resulting executable remains Vix-managed. Explicit export is:

```text
vix build file.cpp --out <path>
```

`vix run file.cpp` executes the Vix-managed artifact directly.

## 11. Compile database and tooling-active context

The authoritative compile database for a context may live at:

```text
.vix/build/<context-id>/backend/compile_commands.json
```

Vix tracks one tooling-active context through:

```text
.vix/build/active
```

It is the most recent successfully configured appropriate development context. Activation must happen only when configuration is successful; it must not point tooling at incomplete or failed configuration state.

## 12. Root `compile_commands.json` bridge

When project root does not contain a user-owned `compile_commands.json`, Vix may maintain a bridge to the active context database. A preferred POSIX representation is:

```text
project/compile_commands.json
    → .vix/build/active/backend/compile_commands.json
```

On platforms where symlinks are unavailable or inappropriate, Vix may use an equivalent atomically updated bridge. Vix must identify bridges it owns. A user-owned root `compile_commands.json` is never overwritten, deleted, or replaced; if one exists, Vix leaves it untouched.

## 13. Failed configuration/build

A failed new context does not replace the current tooling-active context. Only a successfully configured context may become active for tooling. Failed state may remain for diagnostics or be cleaned according to implementation policy, but tooling must not point at incomplete state.

## 14. Version control and `.vix/`

`.vix/` is generated/local state and should not normally be committed. Build correctness must not depend on Git or `.gitignore`. This contract does not authorize Vix to modify user Git configuration. Authoritative project configuration must not live only under `.vix/`.

## 15. Global cache boundaries

Keep these concepts separate:

```text
project build state                 → project/.vix/
standalone-source reusable cache    → Vix user cache
package content/materialization     → package-system state
```

Do not merge the package store into build contexts or automatically move project build state to a global cache.

## 16. No public build-directory flags

Do not introduce:

```text
--build-dir
--build-root
--binary-dir
--cmake-build-dir
```

Users do not select Vix's build-state location. `--cmake` configures the external CMake project; it does not override Vix's binary directory. If a real project cannot work without user control of that directory, report a contract contradiction rather than silently adding a flag.

## 17. Migration

Existing user-owned directories such as `build-dev`, `build-ninja`, and `build-release` are not automatically deleted, migrated, or reused. New Vix begins using `.vix/build/`. There is no destructive cleanup and no legacy build-tree discovery.

## 18. Build-state schema version

Build-state schema version is an internal identity used to invalidate or isolate contexts when Vix changes incompatible build-state semantics. It is not the `vix.toml` format, a package format, or the CLI version, and is not a normal CLI concept.

## 19. Public memory model

A user should only need to understand:

```text
vix build
vix run
vix test
vix build --clean
```

They should not need to know where CMake or Ninja put its binary tree, which build folder is active, which folder to delete, or where object files live.

> The developer manages sources. Vix manages build state.

## 20. Unchanged contracts

This contract does not change `vix.toml`, `vix.lock`, `vix.package.toml`, package formats, Registry formats, `ReleaseManifest`, `InstallReceipt`, CLI diagnostic modes, or Vix2.
