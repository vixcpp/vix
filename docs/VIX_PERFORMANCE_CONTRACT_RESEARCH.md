# Vix Development Latency Contract Research

Status: research for BUILD-I1; no performance contract is frozen by this document.

## Performance principle

Candidate foundational law:

> Vix prefers predictable high performance. The common development loop should feel immediate. Safety and diagnostics do not justify avoidable latency.

The primary user metric is **edit → `vix run` → program starts or a diagnostic appears**. Compiler duration alone misses project discovery, dependency checks, context selection, backend startup, artifact lookup, PTY/capture, and diagnostic presentation.

Keep the DIAG-R2 defaults: `run`, `dev`, and native `test` automatically use the recommended ASan + UBSan profile in development. Performance work must remove redundant orchestration and reuse compatible diagnostic contexts, not disable safety. `--diagnostics=off` remains an explicit choice.

## Human timing evidence

These measurements were supplied from human runs; Codex did not execute benchmarks.

| Workflow | Normal | ASan + UBSan |
|---|---:|---:|
| Single-source first run | 2.48 s | 3.31 s |
| Single-source cached run | 0.22 s | 0.29 s |
| Single-source changed source | 3.02 s | 3.39 s |
| Native multi-file no-op | 0.16 s | 0.15 s |
| Native one-translation-unit incremental | 1.06 s | 1.10 s |
| Native incremental max RSS | 46,096 KB | 50,140 KB |

The no-op native values are evidence of low measured latency, not proof that no configure/build work occurred. The current native run path does perform those operations (described below). Human evidence also found `vix build --clean` could print `ninja: no work to do`; that remains the separate BUILD-I1 correctness defect, not an optimization target.

Conclusion supported by these measurements: sanitizer instrumentation adds measurable first-build cost, while incremental/no-op development overhead is small. Remaining experience work is chiefly avoiding unnecessary or repeated orchestration. Do not extrapolate universal latency or memory guarantees from these fixtures.

## Current standalone-source `run` path

For a `.cpp` input, `RunCommand` resolves the mode, then `run_single_cpp` in `src/commands/run/RunScript.cpp` prepares options and calls `probe_single_cpp_script` (`src/commands/run/detail/ScriptProbe.cpp`). If the source and its detected requirements qualify, it uses the direct compiler path; otherwise it falls back to a generated CMake project.

The direct path currently performs the following on each invocation, including a cache hit:

1. Probe source features and flags, inspect matching project/global package include configuration, and decide direct-vs-CMake strategy.
2. Resolve the compiler executable; obtain compiler version and target triple; read source content and mtime; collect dependency and header fingerprints. Header discovery can run the compiler in dependency-query (`-MM`) mode and hash discovered headers. The cache key includes the Vix/compiler identity, target, language standard, build mode, source, flags, dependencies, headers, and sanitizer mode.
3. Load and validate cache metadata, then compile/link only if the cached fingerprint or artifact is stale.
4. Set the sanitizer runtime environment, create a PTY/process session, run through the process capture/filter path, and retain/replay output for diagnostics. POSIX PTY output merges child stdout/stderr; failures invoke runtime evidence detection, while successful output is replayed to the user.

The cache root is normally `$HOME/.vix/cache/scripts/<key>` (or the corresponding `USERPROFILE` location); the internal local-cache option uses a project-local `.vix-scripts` path. This is separate from BUILD-R2 project `.vix/build` state.

The existing `VIX_PERF_TRACE=1` instrumentation in `DirectScriptRunner.cpp` can report `dependency_discovery`, `fingerprint`, `cache_validation`, `compiler_and_linker`, and `cache_persistence`. It does not separately time CLI startup, option parsing, `ScriptProbe`, compiler identity subprocesses, PTY setup, process startup, output filtering, or diagnostic interpretation. A cached 290 ms run therefore cannot be attributed to one cost from the current trace alone. Likely measurable candidates include probing, compiler identity/target queries, source/header hashing and dependency discovery, and PTY/shell/process setup; these are hypotheses until profiled.

### Standalone fast-path opportunity

The direct-source cache already launches its Vix-managed executable without compiling when the fingerprint is valid. This is the strongest existing fast path. The next research/measurement step is to determine which fingerprint inputs can be validated cheaply without losing correctness. In particular, compiler identity checks and dependency/header discovery appear on the cache-validation path; any persisted metadata shortcut must still detect relevant compiler, source, header, dependency, option, and diagnostic-mode changes.

Do not add a second dependency graph if compiler depfiles or existing cache metadata can answer freshness correctly. Avoid whole-tree scans unless a measured correctness need requires them.

## Current native Vix project `run` path

`run_native_project` in `src/project/native/NativeDevelopmentRuntime.cpp` calls `prepare` on each run. `prepare` currently:

- reparses and validates `vix.toml`;
- reads and validates `vix.lock` when required;
- checks locked package content and materialization state;
- regenerates the native package CMake graph and writes generated CMake input;
- selects a build root based on preset and sanitizer suffix;
- prepares the CMake File API query and invokes CMake configure;
- loads a target/artifact snapshot.

The resulting `build_prepared` invokes `cmake --build` every run, then reloads target/artifact metadata. Ninja/CMake may make this cheap for a no-op, but the Vix call chain still incurs the backend process and metadata work. The current native path has no persisted configuration-signature decision equivalent to the one in the general BuildCommand path.

Native generated locations today are approximately:

```text
project/.vix/build/native-source/CMakeLists.txt
project/.vix/build/<preset>[ -<sanitizer-suffix> ]/   # CMake/Ninja build state
```

This is `.vix`-owned state, but it is not yet the frozen BUILD-R2 `<context-id>/{context.toml,source/,backend/}` layout. `native-source` is shared mutable generated input, and the build directory naming does not yet identify every BUILD-R2 dimension (for example complete toolchain/platform/configure arguments). BUILD-I1 must implement that contract rather than layering an unrelated cache over this layout.

The current native execution uses the observed `TargetArtifact`, which is the right user-facing model. Metadata lookup is nevertheless repeated during preparation/build. A verified artifact may be launched directly only after a reliable context/freshness check proves its build inputs unchanged. Backend freshness remains authoritative for incremental compilation; Vix should avoid reimplementing Ninja's dependency graph.

## External CMake and general build behavior

The general `BuildCommand` computes a configuration signature and asks the engine `ConfigureDecision` whether configuration is needed. The signature includes the build plan, project fingerprint, preset/build type, target/platform, CMake variables/raw CMake arguments, toolchain content, launcher/linker choices, and (after diagnostics integration) sanitizer mode. Project configure regeneration is therefore gated in this path.

For ordinary builds the command delegates incremental compile/link freshness to CMake/Ninja. Its optional internal fast no-op path scans stored project inputs and CMake glob state before skipping backend invocation, but the canonical CLI should not expose internal cache strategy switches solely to achieve speed. Measure whether backend no-op startup is material before adding a separate Vix-side freshness mechanism.

External CMake projects can keep their own CMake cache/build graph inside the Vix-owned binary directory when BUILD-R2 is implemented. Existing user-owned `build/`, `cmake-build-*`, `out/`, and similar directories remain untouched. A `.cpp` content edit should not change configure identity; CMake/Ninja depfiles should trigger only affected compile/link work.

## Configure and no-op policy recommendation

For each Vix build context:

- configure only when context identity or actual configure inputs change;
- do not reconfigure for ordinary source/header content edits;
- invoke the backend for incremental work unless measurements justify a proven-safe direct no-op shortcut;
- delegate compile/link dependency freshness to CMake/Ninja;
- treat a clean request as actual removal/recreation of the selected Vix context (the current `--clean` discrepancy is a correctness blocker for BUILD-I1).

Relevant configure inputs should be the BUILD-R2 identity: schema version, project/backend kind, configuration, platform/ABI, compiler/toolchain identity, generator/backend, resolved diagnostic mode, and ordered external-CMake configure arguments. Target name is not context identity where targets share a compatible graph. A source edit belongs to backend freshness, not context identity.

## Diagnostic-mode interaction

Diagnostic mode is already included in the standalone script fingerprint and native sanitizer build suffix, so normal and instrumented artifacts are separated. BUILD-R2 strengthens this into an explicit build-context identity. Switching modes should reuse each exact context when returning to it, while never reusing incompatible object files. Automatic diagnostics remain on for the frozen development `run/dev/test` policy.

The sanitizer runtime environment and PTY/capture/interpretation path remain necessary on a diagnostic run. Static code confirms these layers, but the provided timings do not isolate their fixed cost. The single-source perf trace can separate some fingerprint and compiler phases; later human profiling should separately time command startup and process launch. Do not trade away captured evidence or visible program output to reduce latency.

## Persisted metadata needed for BUILD-I1

Persist only what removes repeated work while preserving correctness:

- canonical context identity plus readable context metadata, including resolved diagnostic mode;
- a successful-configuration marker/signature covering configure inputs and backend identity;
- backend-owned build graph/state and artifact metadata;
- tooling-active context only after successful configuration, as required by BUILD-R2.

The CMake cache/File API and Ninja depfiles should remain the authorities for configured targets, artifacts, and translation-unit freshness. Do not persist a second project dependency graph or move package content into build contexts. Package lock/content checks remain package-system state, not compile cache state.

## Human profiling plan (not run by Codex)

Use an already-built CLI and existing project/source fixtures; keep parallelism at no more than `-j2` if compilation is triggered. Do not clear global caches. For a cold-ish single-source case, use a fresh temporary source/cache root or isolated `HOME`/`XDG_CACHE_HOME`, rather than deleting a user's cache. Compare the same source/options in normal and automatic diagnostic mode, then repeat unchanged and after changing one source line.

Example timing form (repeat for the listed cases):

```sh
/usr/bin/time -f 'wall=%e user=%U sys=%S maxrss_kb=%M exit=%x' \
  vix run /path/to/fixture.cpp
```

Measure separately:

1. cached standalone `vix run fixture.cpp` (automatic diagnostics);
2. cached standalone run with explicit `--diagnostics=off` for comparison only;
3. native no-op `vix build` and native no-op `vix run`;
4. native one-translation-unit incremental `vix run` after touching/changing only that source;
5. if available, the same command with `VIX_PERF_TRACE=1` to collect the direct-run internal stage timings.

For optional system-call attribution, if `strace` is already installed, an expert may use:

```sh
strace -f -c -o /tmp/vix-run.strace \
  vix run /path/to/fixture.cpp
```

Do not install profiling tools for this gate. Record wall/user/system time, max RSS, exit code, whether configure/compiler/linker ran, and artifact size when applicable. Keep output and diagnostic behavior checked alongside timing.

Reason about relative evidence: a larger first instrumented compile can be acceptable if incremental and no-op loops stay close; repeated expensive configuration or compiler identity/dependency scans on cached runs are stronger UX targets; large repeated recompilation when returning to a known context indicates context/cache separation failure. No universal millisecond threshold is proposed here.

## Recommendations for BUILD-I1

1. Implement the frozen `.vix/build/<context-id>/{context.toml,source/,backend/}` ownership model and make the context key include diagnostic mode and all relevant configure identity.
2. Add a persisted successful configure-signature check to the native runtime before launching CMake configure; generated graph/content changes should invalidate it, while `.cpp` changes should not.
3. Keep Ninja/CMake as compile/link freshness authority. Avoid an eager Vix full-project source scan on every run.
4. Reuse exact contexts when switching back to a previous configuration or diagnostic mode.
5. After a successful context selection/configuration, obtain the runnable `TargetArtifact` without duplicative discovery where the existing CMake File API snapshot remains valid. Relaunch only after freshness is established.
6. Make `--clean` remove only the selected Vix-owned context and prove it configures/builds again; do not remove packages, lock state, global stores, or user build trees.
7. Re-profile cached and incremental paths using the same human fixtures before considering additional caching. Keep editor compile database activation atomic and independent from failed contexts.

## Unresolved questions

- How much of the 290 ms cached standalone run is source probing/fingerprinting versus CLI/process/PTY overhead? Existing trace does not split all those phases.
- Does direct compiler identity/target querying occur on every cache hit, and can it be replaced by a safe persisted identity check without masking a compiler upgrade or `CXX` change?
- What exact native configure-signature inputs can be serialized canonically without hashing ordinary source content into configure identity?
- Can native CMake File API target/artifact data be safely reused across a successful unchanged context, or should CMake be asked to regenerate only its metadata while retaining Ninja state?
- Which timestamp/stat strategy is dependable across supported filesystems, network mounts, and coarse timestamp resolution? BUILD-I1 should prefer backend depfiles unless evidence demonstrates a gap.
- How should Vix measure orchestration independently from compiler/linker time without making profiling output part of the public CLI contract?

## Evidence inspected

Static inspection covered `modules/cli/src/commands/RunCommand.cpp`, `modules/cli/src/commands/run/RunScript.cpp`, `modules/cli/src/commands/run/detail/DirectScriptRunner.cpp`, `modules/cli/src/commands/run/detail/ScriptProbe.cpp`, `modules/cli/src/commands/run/RunProcess.cpp`, `modules/cli/src/project/native/NativeDevelopmentRuntime.cpp`, `modules/cli/src/commands/BuildCommand.cpp`, and the engine `ConfigureDecision` implementation. No production source or tests were modified for PERF-R1.
