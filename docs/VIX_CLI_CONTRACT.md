# Vix CLI Contract

Status: FROZEN CLI CONTRACT FOR CURRENT VIX

This document is the implementation authority for the current Vix CLI public language. It supersedes research recommendations in `VIX_CLI_CONTRACT_RESEARCH.md` where they differ. It does not freeze internal engine design. `command = user intention`; `option = generic modification of that intention`; internal engine capability does not imply public capability; backend-specific capability uses an explicit named escape hatch; one concept has one canonical spelling.

Vix diagnostic modes are first-class C++ development controls. They request diagnostic behavior from Vix; they are not public compiler-flag aliases.

Default diagnostic policy is frozen by DIAG-R2: `build` is uninstrumented by default; development `run`, `dev`, and native `test` automatically use ASan + UBSan where Vix controls compilation; `--release` has no automatic diagnostics. Explicit CLI choice (including `--diagnostics=off`) overrides an explicit project/manifest setting, which overrides the command default. The resolved mode is selected before build-context selection and remains part of build-context identity. `DiagnosticMode::None` is a resolved mode, not a substitute for an unspecified request.

## 1. Canonical command surface

These are the only public commands:

```text
new
init

build
run
dev
test

add
remove
update
install
publish

upgrade
```

Infrastructure syntax is `vix help <command>`, `-h/--help`, and `-v/--version`. Remove the historical top-level `vix file.cpp` shortcut. Single-file work is explicit: `vix build file.cpp` or `vix run file.cpp`.

No other commands, aliases, hidden routes, or top-level shortcuts are public.

## 2. Global options

Exactly:

```text
-h, --help
-v, --version
--verbose
-q, --quiet
```

`--verbose` and `--quiet` are mutually exclusive and have consistent meaning across commands. There is no public `--log-level`, color option, or internal debug option. Logging environment hooks do not become CLI syntax.

## 3. `vix new`

Canonical syntax:

```text
vix new <path>
```

Examples: `vix new hello`, `vix new projects/hello`.

The command creates a new directory/project at exactly the supplied path. Project name is derived from the destination basename. It creates the minimal native Vix project using the existing format-1 model/serializer. It does not overwrite conflicting existing content and does not scaffold into a non-empty destination.

There are no public options in format 1. In particular, no `--force`, `--dir`, template selection, application type, or CMake generation. The generated project has no `CMakeLists.txt`, `vix.json`, `vix.app`, or unnecessary `vix.lock`.

## 4. `vix init`

Canonical syntax:

```text
vix init
```

No arguments or options. It adopts the current directory without moving, reorganizing, or guessing at existing files. It creates a zero-target native format-1 project. It refuses to overwrite an existing `vix.toml`. It refuses a CMake-only root to avoid creating same-root authority ambiguity. Historical `vix.json` and `vix.app` are ignored, not parsed or migrated.

## 5. `vix build`

Canonical forms:

```text
vix build
vix build <target>
vix build <source.cpp>
```

Generic public options:

```text
-d, --dir <path>
-j, --jobs <n>
--release
--clean
--platform <triple>
--verbose
-q, --quiet
--san
--asan
--ubsan
--tsan
```

`build` has no automatic diagnostics. Explicit diagnostic modes are first-class C++ development controls and mutually exclusive. `--san` selects AddressSanitizer + UndefinedBehaviorSanitizer; `--asan`, `--ubsan`, and `--tsan` select those individual modes.

`--dir` selects the project root used by ProjectDiscovery. The default build configuration is development/debug. `--release` selects the optimized release configuration. There are no public presets. For CMake projects, Vix maps these generic modes to the backend configuration.

The positional `<target>` selects a project Target identity/name. `--platform <triple>` selects output platform/ABI. Do not use `--target` to mean a platform.

`--clean` removes Vix-owned build state and then configures/builds again. It does not invoke an arbitrary backend clean target.

Implementation note: human benchmarking observed `vix build --clean` returning `ninja: no work to do`. This violates the contract and is tracked separately as a `BUILD-I1` implementation defect; it does not change the required semantics above.

`--out <path>` is single-source-only and selects the single-file executable output. Project targets retain their authoritative TargetArtifact output.

Single-file-only options:

```text
--out <path>
--compiler-arg <value>
--compiler-arg=<value>
--linker-arg <value>
--linker-arg=<value>
```

Each compiler/linker option is repeatable and passes exactly one argument to its named tool stage. Examples include include paths, definitions, language standard, pthread, library paths, and libraries. There is no raw `--` compiler-argument tail.

External-CMake-only advanced option:

```text
--cmake <configure-argument>
--cmake=<configure-argument>
```

`--cmake` is repeatable and forwards exactly one CMake **configure** argument per occurrence. For example:

```text
vix build app --cmake=-DENABLE_TLS=ON --cmake=-DBUILD_TESTS=ON
vix build --cmake=-DCMAKE_TOOLCHAIN_FILE=/opt/toolchains/arm.cmake
```

CMake cache values follow normal CMake cache semantics and persist in the configured build tree until configuration changes or Vix-owned build state is cleaned. This option does not forward Ninja arguments, `cmake --build` arguments, or CTest arguments. Using it on a native Vix project or in single-source mode is an error.

Invalid combinations are rejected, never ignored:

- `--cmake` with a native Vix project or single-source input;
- `--compiler-arg` or `--linker-arg` with project-target mode;
- `--out` with project-target mode;
- `--platform` values that cannot be supported by the selected build/toolchain context;
- `--verbose` with `--quiet`.

With development configuration and no explicit CLI/project diagnostic decision, native Vix projects and standalone `.cpp` runs use the recommended ASan + UBSan profile. `--diagnostics=off` disables that automatic choice. Explicit modes override the automatic choice (`--ubsan` means UBSan only). A resolved mode instruments compilation and linking; single-source `run` also configures the sanitizer runtime environment and routes usable evidence through Vix diagnostics. `--release` defaults to no diagnostics, though an explicit supported mode still applies. External CMake receives no automatic Vix instrumentation, and explicit Vix diagnostic selection is rejected with guidance to use the project's own CMake configuration.

## 6. `vix run`

Canonical forms:

```text
vix run
vix run <target>
vix run <source.cpp>
vix run <target-or-source> -- <program-args...>
```

There is no separate arbitrary-binary-path execution feature; users can execute a binary directly through their shell.

Public options:

```text
-d, --dir <path>
--release
--cwd <path>
--env <KEY=VALUE>
--verbose
-q, --quiet
--san
--asan
--ubsan
--tsan
--diagnostics=off
```

The four diagnostic modes have the shared meanings defined under `build` and are mutually exclusive. In development, absent explicit CLI/project policy, `run` selects ASan + UBSan where Vix controls compilation. `--diagnostics=off` opts out; an explicit mode selects exactly that mode. `--release` defaults to none, but explicit selection still wins where supported. `run` builds with the resolved mode, configures its runtime environment, and preserves usable sanitizer evidence for Vix diagnostics. External CMake gets no automatic instrumentation and rejects explicit Vix diagnostic modes.

For single-source mode, repeatable `--compiler-arg` and `--linker-arg` have exactly the build meanings. For an external CMake project, repeatable `--cmake` has exactly the configure-only build meaning. These mode-specific options are invalid for incompatible inputs.

After the first `--`, every remaining token belongs to the program. It never supplies compiler flags, linker flags, CMake arguments, or more Vix options. For example:

```text
vix run server -- --port 8080
vix run hello.cpp --compiler-arg=-Iinclude --linker-arg=-pthread -- input.txt
```

Without an explicit target, exactly one runnable target is selected. Zero runnable targets produce a structured error. Multiple runnable targets produce a structured ambiguity error listing the runnable names. Selection uses TargetCapabilities and TargetArtifact, not binary-name/path guessing.

`--dir` selects the project root. `--cwd` selects the launched process working directory; a relative value resolves against the discovered project root. `--env` is repeatable, requires a valid `KEY=VALUE` assignment, preserves inherited environment, and overrides the inherited value for the explicit key.

Invalid combinations include incompatible backend/single-source options, project-target `--out`, `--platform` on run, malformed environment assignments, and verbose plus quiet. Reject `--cmake` on native projects or source-file mode; reject compiler/linker arguments on project-target mode.

No container/remote execution inputs, server/script classification, replay, OpenAPI/docs, database switches, historical UI/cache options, or `run --watch` are part of the public language.

## 7. `vix dev`

Intent: build and execute a selected C++ program, then rebuild/restart it when relevant project inputs change.

Canonical forms:

```text
vix dev
vix dev <target>
vix dev <target> -- <program-args...>
```

Single-source `dev` is not part of format 1 unless watcher semantics have already been verified. Public runtime/build-context controls mirror `run` where applicable:

```text
-d, --dir <path>
--release
--cwd <path>
--env <KEY=VALUE>
--verbose
-q, --quiet
--cmake <configure-argument>   # external CMake only, same semantics as build
--san
--asan
--ubsan
--tsan
--diagnostics=off
```

In development, absent explicit CLI/project policy, `dev` selects ASan + UBSan where Vix controls compilation. `--diagnostics=off` opts out; explicit modes select exactly the requested mode. `--release` defaults to none, with explicit supported selection taking precedence. Diagnostic selection remains stable across development iterations. Native development has the supported watch/rebuild/restart lifecycle on POSIX. External CMake projects currently execute once through `dev`; automatic watching/restart is not implemented. External CMake is not automatically instrumented and rejects explicit Vix modes.

Program arguments use `--`; watch/restart is the command intention. There is no `run --watch`, `dev --watch`, or `--reload` option.

Release readiness requires verified behavior for native Vix and external CMake projects, rebuild after change, termination of the old process, start of the new process, and exit/failure handling. Do not promise lifecycle guarantees stronger than verified implementation evidence.

## 8. `vix test`

Canonical forms:

```text
vix test
vix test <filter>
vix test --list
```

Public options:

```text
-d, --dir <path>
--list
--watch
--fail-fast
--verbose
-q, --quiet
--san
--asan
--ubsan
--tsan
--diagnostics=off
```

Native tests in development automatically resolve to ASan + UBSan absent explicit CLI/project policy. `--diagnostics=off` opts out; explicit modes select exactly that mode. Native test targets are built with the resolved mode, which is passed to test execution so runtime configuration/evidence is retained. External CMake receives no automatic instrumentation and rejects explicit Vix diagnostic modes. In format 1, diagnostic controls cannot be combined with terminal `--ctest` mode.

In format 1, `<filter>` is a literal, case-sensitive substring of the canonical test name. It is not a regex or glob and carries no hidden CTest interpretation.

Advanced external-CMake-only mode:

```text
vix test --ctest <raw-ctest-arguments...>
```

`--ctest` is terminal: everything after it belongs to CTest. Project selection and verbosity controls may precede it (`-d/--dir`, `--verbose`, `--quiet`). The mode rejects a Vix filter and `--list`, `--watch`, or `--fail-fast`. It is invalid on native projects. Ordinary CTest execution remains an internal implementation detail.

No `--exact-test`, `-R`, `--test`, `--raw`, `--preset`, `--build-target`, or generic `--` CTest tail is public.

## 9. Interactive C++ REPL (excluded in Vix 3.0)

Vix 3.0 does not expose `vix repl`. The historical Reply evaluator is not a C++ REPL and is intentionally unavailable through the stable public command surface. The implementation may remain in the source tree for unrelated internal or historical use, but it is not part of the Vix 3.0 CLI contract.

## 10. Package commands

### `vix add`

Registry package:

```text
vix add <publisher>/<package>
vix add <publisher>/<package> --registry <logical-registry-id>
vix add <publisher>/<package> --version exact=<version>
vix add <publisher>/<package> --version compatible=<version>
```

Git package:

```text
vix add <git-url>
vix add <git-url> --branch <name>
vix add <git-url> --tag <name>
vix add <git-url> --commit <full-commit-id>
```

Registry bare-add discovers the highest verified stable release, records a compatible requirement, resolves, and writes the lock. Git bare-add discovers the remote default branch, records branch intent, and locks the exact commit. Git revision selectors are mutually exclusive. Registry and Git options cannot be mixed. There is no global installation semantic.

### `vix remove`

```text
vix remove <publisher>/<package> [--registry <id>]
vix remove <git-url>
```

No revision selectors. It removes a root requirement by Registry identity or Git source.

### `vix update`

Exactly `vix update`, with no arguments. It re-resolves the complete declared root dependency graph. Package-specific update is future research.

### `vix install`

```text
vix install
vix install --offline
```

No package argument. It materializes exactly the current valid `vix.lock`; it does not resolve. An empty/no-requirement project remains a successful no-op under the frozen package contract.

`--offline` is strict: no network, metadata acquisition, resolution, or remote source access. Only durable local content may satisfy the lock. Missing locked bytes produce a structured offline-missing-content error; there is no silent network fallback. `--offline` is valid only for install.

### `vix publish`

Exactly `vix publish`, no public options in format 1. Identity comes from `vix.package.toml`, publication routing from publishing configuration, and authentication from the configured environment-variable indirection. No Registry override, endpoint/token/password option, or dry-run. A future dry-run requires a separately frozen no-network/output guarantee.

### `vix upgrade`

Exactly `vix upgrade`, no public options in format 1. Managed installation upgrades through the verified distribution runtime. Archive/manual installs refuse self-upgrade; external installs report the owner. Generation crossing is forbidden. There is no `--check`, `--version`, `--channel`, or `--rollback` option.

## 11. Cross-command laws

### Project root

For `build`, `run`, `dev`, and `test`, `-d/--dir <path>` always means the project root used by ProjectDiscovery.

### Verbosity

Exactly `--verbose` and `-q/--quiet`; mutually exclusive with the same semantics for every command.

### Release configuration

`--release` is available only on commands that build before acting: build, run, dev. It selects optimized release configuration. Automatic diagnostics are off for release execution; an explicit CLI mode still wins if supported. Do not add it to test in format 1; users may build explicitly first or use the contracted external-CMake mode.

### Diagnostic request and resolution

The request state must distinguish unspecified, explicit off, and each explicit mode while retaining exactly one `DiagnosticMode` type for the resolved result. Resolve before choosing a build context. Precedence is:

```text
explicit CLI mode or --diagnostics=off
> explicit project/manifest setting
> command default
> none
```

The runtime manifest's existing `san` values remain `off`, `ubsan`, and `asan_ubsan`. Omission is not a project decision; explicit `san = "off"` is an opt-out; the other values map to their existing modes. Preserve this distinction internally without changing manifest syntax or format. ASan-only and TSan manifest settings are not added. `--diagnostics=off` is supported only by `run`, `dev`, and `test`; `build` already defaults off.

### Platform and toolchain

`--platform <triple>` belongs to build only. It does not imply cross-run semantics. There is no public generic `--toolchain`, `--sysroot`, `--linker`, or `--launcher` in format 1. Generic toolchain identity/resolution is not mature enough. External CMake users may use the CMake configure escape hatch for `CMAKE_TOOLCHAIN_FILE`; single-file users may use compiler/linker argument channels. A first-class Vix Toolchain CLI is future research.

### Argument channels

The meanings are disjoint:

| Channel | Syntax | Meaning |
|---|---|---|
| Vix options | Before terminal backend/runtime delimiter | Vix command controls |
| CMake configure | Repeatable `--cmake <one-arg>` | One CMake configure argument |
| CTest backend | Terminal `--ctest ...` | All remaining tokens belong to CTest |
| Compiler | Repeatable `--compiler-arg <one-arg>` | One compiler argument, single-source only |
| Linker | Repeatable `--linker-arg <one-arg>` | One linker argument, single-source only |
| Runtime program | `run`/`dev` `-- ...` | All remaining tokens belong to the program |

No generic arbitrary-backend passthrough exists. In particular, bare `--` is never a compiler, linker, CMake, or CTest channel.

## 12. Invalid-combination matrix

All combinations below must fail clearly and before execution; the CLI must not silently ignore an option:

| Combination | Required result |
|---|---|
| `--cmake` on native Vix project | Reject: CMake configure arguments apply only to external CMake projects |
| `--cmake` on single-source build/run | Reject: incompatible input mode |
| `--compiler-arg` on project-target mode | Reject: single `.cpp` only |
| `--linker-arg` on project-target mode | Reject: single `.cpp` only |
| `--out` on project-target mode | Reject: project TargetArtifact owns output |
| `--ctest` on native Vix project | Reject: CTest mode is external-CMake only |
| `--ctest` mixed with filter | Reject: terminal backend mode cannot combine with Vix filter |
| `--ctest` mixed with `--list` | Reject |
| `--ctest` mixed with `--watch` | Reject |
| `--ctest` mixed with `--fail-fast` | Reject |
| Any two diagnostic mode switches, including a repeated switch | Reject; select exactly one of `--san`, `--asan`, `--ubsan`, `--tsan` |
| `--diagnostics=off` mixed with any diagnostic mode | Reject: explicit off and explicit mode are mutually exclusive |
| A diagnostic mode with external CMake or terminal `--ctest` | Reject unless generic instrumentation is explicitly supported |
| `--offline` outside install | Reject as unknown/invalid option |
| `--platform` on run/dev/test | Reject as unsupported command option |
| `--verbose` with `--quiet` | Reject as mutually exclusive |

## 13. Explicitly retired public syntax

The following are not public current-Vix syntax. Their useful engine behavior may remain internal when needed.

- Top-level `vix file.cpp` shortcut.
- Build presets; raw ambiguous build `--` tail; `--build-target`; platform use of `--target`; `--sysroot`; `--launcher`; `--linker`; redundant sanitizer spellings `--sanitize`, `--sanitize=<mode>`, and `--no-san`; database toggles; Cloud report; managed-SDK profile; graph executor; heartbeat; cache/status controls; warning pagination; debug-log internals.
- Remote/container run inputs (`docker://`, `container://`, `ssh://`, `http://`); server/script classification; run watch/reload; replay; OpenAPI/docs controls; environment hints; runtime cache/fingerprint controls; run logging-format/color controls.
- CTest parser vocabulary in ordinary Vix testing: exact-test, `-R`, `--test`, raw mode, presets, build-target, and generic `--` CTest tail. Advanced CTest arguments exist only after terminal `--ctest` in external-CMake mode.
- `vix repl`; interactive C++ execution is outside the Vix 3.0 release scope.
- Global log-level, color, and internal debug options.
- `new --force`, `new --dir`, template/application selection; `init` options; `publish` options; `upgrade` options described above.

Removal from the public language does not require deleting a useful engine capability. Preserve external CMake and CTest support behind the explicit contracts above.

## 14. Help requirements

Each command help is concise and follows:

```text
Usage
one-sentence description
Arguments
Options
Advanced   # only when an escape hatch exists
Examples   # at most a few useful examples
```

Normal help does not require knowledge of Ninja, CTest, or CMake. Backend terminology appears only in explicitly labeled advanced sections. Help must not describe internal architecture or historical compatibility.

## 15. Memory test

```text
new      create
init     adopt

build    compile
run      execute once
dev      rebuild/restart while developing
test     test

add      add dependency
remove   remove dependency
update   refresh dependency graph
install  materialize lock
publish  publish package

upgrade  upgrade Vix
```

Advanced escape hatches are exactly:

```text
--cmake
--ctest
--compiler-arg
--linker-arg
```

Any implementation need that appears to require another public concept must be reported as a contract contradiction for review, not silently added to the CLI.
