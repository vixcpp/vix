# Vix-Owned Build State — Research

Status: research; no migration contract is frozen by this document.

## 1. Current path inventory

| Current path or state | Classification | Observed use |
|---|---|---|
| Project source files, `vix.toml`, `CMakeLists.txt` | `USER_PROJECT_CONTENT` | Inputs to discovery, configuration, compilation, and target/test modeling. |
| Native generated CMake source: `<root>/.vix/build/native-source/` | `VIX_PROJECT_STATE` | `NativeDevelopmentRuntime` writes the generated `CMakeLists.txt` here. |
| Native backend tree: `<root>/.vix/build/<preset>[-<sanitizer>]/` | `VIX_PROJECT_STATE` | Native project configure/build state, CMake cache, File API data, Ninja files, compile database, and artifacts. The current preset path is based on preset build-directory names, with a sanitizer suffix. |
| General CMake/legacy build tree: `<root>/build-dev`, `build-ninja`, `build-release`, with optional sanitizer/platform suffixes | `VIX_PROJECT_STATE` in purpose, but currently visible root-level output | `ExecutionPlanning` derives this from the selected preset and optional target triple/sanitizer. CMake logs, signature/toolchain files, cache, File API data, generator files, compile database, graph/cache metadata, objects, and artifacts are placed there. |
| Native build graph objects: `<build-root>/.vix/obj/` | `VIX_PROJECT_STATE` | The CLI graph executor stores object files below its selected build root. |
| Package materialization: `<root>/.vix/packages/...` | `VIX_PROJECT_STATE`, not build state | Locked package content materialized for project consumption. A build clean must not remove it. |
| Package content store under the XDG/home cache | `GLOBAL_CACHE` | P10P durable package content is outside the build tree and is shared across projects. Exact path follows the package-store resolver. |
| Direct single-source cache: `~/.vix/cache/scripts/<fingerprint>/` (or platform home equivalent) | `GLOBAL_CACHE` | The direct compiler path stores fingerprinted binary and cache metadata here. `--local-cache` selects `<cwd>/.vix-scripts/` instead. |
| Single-source CMake fallback: `<scripts-root>/<script-key>/build-ninja/` | `GLOBAL_CACHE` by default; `VIX_PROJECT_STATE`-like local state with the local-cache option | A temporary generated CMake project and backend tree are keyed by source path. Default scripts root is `~/.vix/cache/scripts`; the fallback temp root is `<temp>/vix/cache/scripts`; local mode uses `.vix-scripts`. |
| Single-source exported executable | `FINAL_ARTIFACT` | `build file.cpp` exports the built executable to `--out`, or currently to the current directory by default. This is distinct from its cached build binary. |
| CMake configure/build logs and `.vix-config.sig` | `VIX_PROJECT_STATE` | Located in the current selected external CMake build tree. |
| CMake File API query/replies, `CMakeCache.txt`, `build.ninja`, `compile_commands.json` | `VIX_PROJECT_STATE` | Backend observations and state live under each backend build tree. File API query setup is performed against the selected build root. |
| Artifact/build state caches and graph metadata | `VIX_PROJECT_STATE` or `GLOBAL_CACHE`, depending on subsystem | Build-state files and graph data are under the selected build root; a separate engine artifact-cache root is based on the system temporary directory (`<temp>/vix/cache/build`). |
| Atomic-write/temporary staging files | `TEMPORARY` | Used transiently by file-writing helpers; these are not authoritative project inputs. |
| `<root>/build/`, `<root>/build-*`, `cmake-build-*`, `<root>/out/` created by users | `USER_PROJECT_CONTENT` | Vix must treat pre-existing directories as user-owned unless they are the exact Vix-selected state directory under the new ownership contract. |
| `modules/cli/build-runtime-evidence`, `modules/cli/build-distribution-smoke` and similar repository build trees | `LEGACY` / repository-development infrastructure | These are human verification/build trees, not runtime project layout. This research does not propose deleting or migrating them. |
| Old run/dev and application command build paths | `LEGACY` | Historical paths include project-root `build-dev` and guessed `build-ninja` outputs. They are not the desired current native project contract. |

The repository `.gitignore` ignores `build/`, `build-*/`, `compile_commands.json`, and `.vix/`. The current neutral `vix new` generator writes `vix.toml` and `src/main.cpp`; inspection found no generated `.gitignore`. The root repository ignore rules therefore do not prove that a newly generated project has its own ignore rule.

## 2. Native Vix project path

`NativeDevelopmentRuntime` currently chooses a generated source root at `<project>/.vix/build/native-source` and a backend root at `<project>/.vix/build/<preset-build-name>[-<sanitizer>]`. It writes generated CMake source, configures CMake with `-S <sourceRoot> -B <buildRoot>`, requests compile-command export, then observes targets/artifacts through the CMake File API. Package preparation reads the lock and materializes packages separately under `.vix/packages`; that state is not the build tree.

This already demonstrates that a backend can require CMake/Ninja structure while Vix owns it. Current gaps: the path uses preset implementation names, it does not visibly encode target platform/toolchain identity in the native runtime path, and the generated CMake source root is shared across native contexts. Context separation and possible concurrent configure/build races need to be resolved in migration.

## 3. External CMake path

The general execution-plan layout currently sets `buildDir` to the project root plus a preset-derived directory name. Built-in presets map to `build-dev`, `build-ninja`, and `build-release`; target triple and sanitizer suffixes may be appended. `BuildCommand` uses that directory for configure/build state, logs, CMake signature/toolchain files, File API observations, graph/object state, and artifact discovery. Run/test paths reconstruct the same plan and use the selected tree for target/test and artifact observations.

An out-of-source tree under `.vix/build/<context>` is compatible with ordinary CMake `-S source -B binary`, File API query/replies, `CMakeCache.txt`, generator files, CTest, and `compile_commands.json`; none inherently requires the binary tree to be adjacent to the source. Single- and multi-config generators differ in how configuration is selected and where configuration-specific artifacts are placed, so artifact lookup must continue to use File API target metadata rather than guessed paths. Existing code primarily resolves artifacts from the selected File API snapshot.

Custom `--cmake` configure arguments and `CMAKE_TOOLCHAIN_FILE` affect configured state. They should be represented in a canonical context/configuration signature; changing them must trigger reconfiguration or select an isolated context. CMake's own cache is scoped to the build tree. The user source `CMakeLists.txt` need not be rewritten to move the binary tree. CMake projects may contain `CMakePresets.json` with their own binary-directory policy; whether Vix honors presets, rejects conflicting preset binary directories, or deliberately invokes CMake without them is a migration decision to verify. Vix must not silently let a project preset redirect output outside Vix-owned state.

Vix can ignore external project `build/`, `cmake-build-debug/`, and `out/` and create a separate Vix-owned out-of-source tree. Those directories must not be cleaned, adopted, or treated as authoritative by default.

## 4. Single-source behavior

For a standalone source file, the direct compilation path computes a build fingerprint, places the binary and cache metadata under a global scripts cache, and uses that cached binary for execution. Its default root is the user home Vix cache; a temp-directory fallback exists where home is unavailable. The optional local-cache path uses `.vix-scripts` under the current directory. The alternate CMake fallback creates a generated per-source project under the same scripts root and uses its `build-ninja` child tree.

`vix build file.cpp` then exports a copy to `--out <path>` or currently to the current directory by default. `vix run file.cpp` consumes the cached binary. There is no guaranteed project root for an arbitrary standalone file, so moving its cache into a project's `.vix` is only well-defined when the invocation is intentionally associated with a discovered project. Recommended migration direction: retain a global fingerprinted cache for truly standalone sources; when `--dir` resolves a Vix project, consider project-local state only if the contract explicitly establishes that relationship. Avoid creating `.vix` beside an arbitrary source file without an ownership rule.

## 5. Minimum build-context identity

Distinct contexts must not share mutable backend caches when any setting can change compiler outputs or ABI. At minimum the identity/signature needs:

- build configuration (development/debug versus release);
- exact target platform/ABI when cross-compiling;
- compiler/toolchain identity, including compiler path/version and relevant toolchain file or SDK identity;
- backend/generator identity;
- diagnostic instrumentation mode;
- CMake configure arguments and relevant project/configuration inputs.

The selected target name normally should not be part of the context: multiple targets can be built in one configured project tree. CMake configure arguments need not all appear as readable path components. A stable opaque context key plus machine-readable context metadata/configuration signature is less fragile. Incompatible compiler/toolchain/platform contexts should be isolated; mutable CMake cache values that change within a context must be detected and reconfigured. The current preset+sanitizer naming is insufficient for all these dimensions, and the current target-triple suffix does not include compiler identity or CMake argument state.

## 6. Artifact behavior

Native runtime and external CMake target discovery use the CMake File API snapshot. `TargetArtifact` carries the exact artifact location observed for a target, and run resolves/executes that artifact rather than relying on a user-visible build directory or scanning arbitrary binaries. The build tree is an implementation detail; this model remains valid when relocated under `.vix/build`.

For single-source compilation, the cached binary is internal. `--out` is the existing explicit export mechanism; the current default export-to-working-directory behavior should be reviewed against the frozen CLI contract, but this research does not change it or propose a project-level artifact-export option.

## 7. `compile_commands.json` and editor integration

CMake is configured with compile-command export for native generated projects. The database is read from `<selected-build-root>/compile_commands.json` by build-graph integration; the engine's default helper returns that path. No reliable implementation that publishes or copies the active database to project-root `compile_commands.json` was found in the inspected paths. A message filter mentions “Copy compile_commands.json to project root,” but that is output filtering, not evidence of a current copy implementation.

For clangd/IDE convenience, a root-level symlink/bridge to the active context is a reasonable candidate because clangd commonly discovers `compile_commands.json` at project root, but it must never silently replace a user-owned file. A future policy should: create/update only a Vix-marked bridge or when the path is absent; refuse or leave an existing regular user file untouched; atomically redirect the bridge when the active context changes; and ensure the target database exists and is current. Multi-config and active configuration selection remain open. Copying the database risks stale duplicate data; a managed symlink/pointer is preferable on supported platforms, with an explicit fallback/discovery story where symlinks are unavailable.

## 8. Proposed `--clean` semantics

Recommended contract: `vix build --clean` removes only the selected Vix-owned context and then configures/builds it again. It must not remove sibling configuration/platform/toolchain contexts, user-owned root build directories, source files, `vix.toml`, `CMakeLists.txt`, `vix.lock`, `.vix/packages`, the global P10P content store, or unrelated caches. A root `compile_commands.json` bridge should be atomically refreshed after rebuilding; clean should not delete a user-owned file. If generated CMake source is context-shared, clean should either preserve it as project state or deterministically regenerate it, but should not remove shared data still needed by another context.

Current general `BuildCommand` cleanup recursively removes its entire selected `plan.buildDir`; this is safe only insofar as the current path is treated as Vix-generated. The new location under `.vix/build` makes ownership explicit. Current native execution selects its own context path, while single-source cache cleanup has separate behavior; command semantics across those paths should be made explicit during implementation.

## 9. Ignore rules and authoritative state

The repository's `.gitignore` already excludes `.vix/`, and `.vix` is local build/package materialization state in current native projects. Package identity/requirements and lock authority remain in root `vix.toml`/`vix.lock`, not ignored state. Materialized package content under `.vix/packages` is reconstructible from the lock/store and must survive build cleaning, though it need not be committed.

The neutral project generator currently does not emit `.gitignore`; consider adding a minimal generated ignore rule for `.vix/` as a project-creation policy in a later implementation task. This research pass does not edit the generator.

## 10. Migration risks

- Existing external CMake builds currently reside in visible root-level `build-*` trees. Moving to `.vix/build` will cause a fresh configure/build unless a safe, explicit migration is implemented; the conservative approach is to leave old trees untouched and start fresh.
- Never recursively delete a conventional user-owned `build/`, `cmake-build-*`, or `out/` directory. Clean must be scoped to a Vix-marked/derived context under `.vix`.
- Native generated CMake source is currently shared between contexts. Concurrent or differently configured builds may race or observe changed generated inputs.
- CMake arguments/toolchain changes and compiler upgrades can invalidate cached state. Context keys/signatures need to prevent stale target/artifact metadata from being reused.
- Multi-config generators require configuration-aware File API artifact resolution while sharing one CMake tree; the policy for coexisting debug/release and platform contexts needs exact definition.
- Root `compile_commands.json` may be user-owned. Never overwrite it to improve editor integration without a reversible ownership marker or explicit opt-in.
- The current repository ignore file is not automatically copied into newly generated projects.
- Standalone source compilation has a useful global fingerprint cache but does not naturally belong to a project-local `.vix` tree.
- Do not conflate repository verification builds (`build-runtime-evidence`, `build-distribution-smoke`) with end-user project state.

## 11. Proposed final layout

Candidate only; context spelling is intentionally not frozen:

```text
project/
├── source files
├── vix.toml OR CMakeLists.txt
└── .vix/
    ├── build/
    │   └── <opaque-context-id>/
    │       ├── context metadata/signature
    │       ├── generated CMake source (or context-local reference)
    │       ├── CMakeCache.txt / File API / backend files
    │       ├── compile_commands.json
    │       ├── objects and build metadata
    │       └── final target artifacts
    └── packages/              # independent materialized locked content
```

The context metadata should record human-readable dimensions and canonical configure inputs even if the directory key is opaque. Vix chooses the active context; `TargetArtifact` remains the public runtime route to artifacts. External user build trees remain untouched. Standalone `.cpp` invocations continue to use a global fingerprint cache unless explicitly tied to a discovered project by a future contract.

## 12. Unresolved decisions

1. Exact context-key algorithm and which compiler identity fields are stable enough to hash.
2. Whether native generated CMake source is context-local or shared immutable source under `.vix/generated`.
3. How native runtime will incorporate platform/toolchain/configure-argument dimensions while preserving current frozen project semantics.
4. Whether CMake project presets are honored, and how their `binaryDir` is prevented from escaping Vix-owned state.
5. How debug/release coexist for multi-config generators and what “active context” means for editor tooling.
6. The safe root `compile_commands.json` bridge policy on platforms without symlink support and when a user file already exists.
7. Whether `build file.cpp` should continue exporting to the current directory when `--out` is absent, or require an explicit output; this is separate from internal cache ownership.
8. Whether a project-local `.gitignore` containing `.vix/` is emitted by `new`, `init`, both, or neither.
9. Whether legacy root build directories receive any migration hint. Automatic adoption/deletion is not recommended without stronger ownership evidence.
10. Whether Vix build-context cleanup has a persistent ownership marker in addition to living under `.vix/build`.

## Ownership principle assessment

The proposed law is consistent with current native runtime behavior and with CMake's out-of-source model:

> The source tree belongs to the user. The project-local build state belongs to Vix and lives under `.vix/`. Backends may maintain their internal structure there. Final artifacts are discovered through the project model, not through user knowledge of build directories.

The current general external-CMake path and legacy single-source fallback do not yet consistently satisfy it. No reviewed evidence requires a public build-directory option; the migration can preserve backend control through the already-defined configure/toolchain escape hatch while keeping Vix's binary-tree location internal.
