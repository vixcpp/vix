# Vix CLI contract research (CLI-R1)

Status: research, not a frozen contract. This document records the CLI surface and implementation evidence inspected for this pass, then proposes a small candidate language for human review. No production behavior, parser, help, or test was changed for this research task.

## 1. Principles

The public CLI should name user intentions, not internal architecture. A normal path should be memorable (`new`, `build`, `run`, `test`, `add`, `install`). Generic C++ controls are appropriate when they describe stable concepts such as target selection, configuration, jobs, environment, toolchain, platform, or test filtering. Backend-specific controls belong behind explicit, named advanced boundaries. Graph scheduling, heartbeat, cache implementation, UI, and compiler-fingerprint details are not user intentions. Application and service domains do not belong in a foundational C++ CLI.

These categories are deliberately distinct:

- **A — SIMPLE VIX INTENT:** common action and ordinary inputs.
- **B — GENERIC C++ CONTROL:** broadly useful project/build/runtime/testing/package control.
- **C — EXPLICIT BACKEND ESCAPE HATCH:** opt-in access to an underlying ecosystem's exceptional controls.
- **D — INTERNAL IMPLEMENTATION KNOB:** engine detail, not public language.
- **E — APPLICATION-SPECIFIC/HISTORICAL:** domain behavior outside foundational C++ tooling.

The current command registration/help advertises `new`, `init`, `build`, `run`, `test`, `dev`, `repl`, `add`, `remove`, `update`, `install`, `publish`, and `upgrade`. Infrastructure syntax includes `vix <file.cpp>`, `vix help <command>`, `-h/--help`, and `-v/--version`. Top-level verbosity is `--verbose` and `-q/--quiet`; `VIX_LOG_LEVEL` is also read from the environment. Parser acceptance and per-command help are not fully uniform, so the classifications below distinguish public front-door behavior from deeper parser residue.

## 2. Command-by-command analysis

### COMMAND: new

Intent:
  Create a minimal usable native Vix C++ project in a new directory.

Primary inputs:
  A project name or destination path; optionally a base directory and a non-empty destination policy.

Simple syntax:
  `vix new hello`.

Generic C++ controls:
  None are necessary for the minimal executable scaffold. Executable/library choice can be expressed in the project model when a real user need and stable target contract exist, rather than growing a template menu prematurely.

Backend escape hatches required:
  None. New native projects should not be generated as CMake projects.

Invalid combinations:
  Multiple project names; a file destination; overlaying conflicting authority files; silently overwriting a non-empty destination.

Capabilities that should remain internal:
  File creation, name/path validation, native model serialization.

Application-specific/historical concepts to remove:
  Web/server/app templates, framework bootstrapping, `vix.json`, `vix.app`, and user-root `CMakeLists.txt` generation.

Open questions:
  Should the CLI ever offer named templates? Evidence is insufficient; any future template would need to remain neutral C++ and must not create a second manifest grammar. Should `--force` mean safe scaffold into a non-empty directory or merely force overwrite? Current behavior adds a scaffold only when it does not conflict; the name may overstate its destructive effect.

Representative workflows:
  `vix new hello`; `cd hello`; `vix build`; `vix run`.

### COMMAND: init

Intent:
  Adopt the current directory as a native Vix project without reorganizing it.

Primary inputs:
  Current directory and existing project-authority files.

Simple syntax:
  `vix init`.

Generic C++ controls:
  None are currently justified; current implementation adopts the current directory, derives its name, and writes a zero-target native manifest.

Backend escape hatches required:
  None. A CMake-only directory is already an external CMake project and must not be converted implicitly.

Invalid combinations:
  Existing `vix.toml` (must not overwrite); existing `CMakeLists.txt` without `vix.toml` (must refuse to create ambiguous authority); no command-line source scanning or architecture guessing.

Capabilities that should remain internal:
  Project discovery/classification and format serialization.

Application-specific/historical concepts to remove:
  Legacy-manifest parsing/migration and automatic conversion of application/CMake layouts.

Open questions:
  None for a zero-target format-1 project; the inspected implementation documents that format 1 allows zero targets. Whether future `init` supports an explicit directory argument should be decided only if a consistent `--dir` convention is adopted across project commands.

Representative workflows:
  Empty directory: `vix init`; directory with arbitrary files: `vix init` leaves them untouched; existing CMake project: refuse; existing native project: report already initialized.

### COMMAND: build

Intent:
  Build a native project target, an external CMake project, or one C++ source file.

Primary inputs:
  Current/project directory, optional target name or source path, project model, dependency lock/materialized packages, selected host/toolchain context.

Simple syntax:
  `vix build`; `vix build app`; `vix build src/tool.cpp`.

Generic C++ controls:
  Target selection, debug/release configuration, parallel jobs, toolchain/compiler choice, target platform, sysroot, output path, clean/reconfigure behavior, and diagnostics/verbosity are broadly meaningful. A target name and target platform are distinct dimensions: the former selects a build graph node, the latter selects the output ABI/platform. `--platform <triple>` is clearer than overloading `--target`.

Backend escape hatches required:
  External CMake needs a way to set configure/cache variables and a toolchain file; a named `--cmake <arg>` or grouped CMake configure boundary is a candidate. Single-file mode needs explicit repeatable compiler and linker argument channels (e.g. `--compiler-arg` and `--linker-arg`) for `-I`, `-D`, `-std=`, `-pthread`, `-W`, `-L`, `-l`, and target-specific flags. Do not use one ambiguous `--` tail for CMake, compiler, linker, and runtime arguments. Raw build-tool/Ninja control is not yet shown to be necessary.

Invalid combinations:
  Project target selection with a source-file compilation request; backend CMake arguments on native projects unless a documented no-op/error policy exists; platform selection without a compatible toolchain; build output path that conflicts with a project target artifact; multiple selected targets if the command contract is singular.

Capabilities that should remain internal:
  Build graph executor, Ninja invocation details, heartbeat, up-to-date implementation, cache policy/fingerprint, status UI, CMake generator selection unless intentionally part of a generic configuration contract.

Application-specific/historical concepts to remove:
  SQLite/MySQL switches, Cloud reporting, managed-SDK profiles tied to products, application-specific server/script behavior, and internal graph/cache switches.

Open questions:
  How does a project define named configurations consistently across native and CMake backends? Is `--release` sufficient as a first-class common mode, with custom configurations available via project/CMake escape hatch? Should clean mean delete/reconfigure Vix-owned build state, or clean the backend build? Which toolchain facts can Vix model generically versus delegate to CMake? Should explicit compiler selection be a first-class generic option or a toolchain-file concern? Single-file compiler arguments currently pass an internal `--` tail; a distinct compiler/linker syntax is needed before presenting this as a stable expert contract.

Representative workflows:
  Beginner: `vix build`; target: `vix build app`; release: `vix build app --release`; cross: `vix build app --platform aarch64-linux-gnu`; custom CMake configuration/toolchain: named CMake escape hatch; one-file compile: explicit compiler and linker argument channels.

### COMMAND: run

Intent:
  Build if needed, then execute one selected program once.

Primary inputs:
  Current project, target/source/binary input, runtime arguments, working directory, environment overrides.

Simple syntax:
  `vix run`; `vix run app -- --port 8080`; `vix run src/main.cpp -- <program-args>`.

Generic C++ controls:
  Target selection; working directory; repeated `KEY=VALUE` environment entries; program arguments after a separator; potentially configuration/platform forwarded as build context when consistently available.

Backend escape hatches required:
  CMake project configuration belongs to the shared build escape hatch, not a run-specific CMake syntax. Single-file compiler/linker controls must be distinct from runtime arguments. A direct binary input can be useful but needs a clear policy about whether Vix builds anything or merely executes it.

Invalid combinations:
  More than one target/source input; program arguments before the separator; build-only flags accepted as runtime flags; a selected non-runnable target; container/remote URL treated as an ordinary local target without a separately designed execution feature.

Capabilities that should remain internal:
  Artifact lookup, build-if-needed decision, process creation, executable selection, environment merge mechanics.

Application-specific/historical concepts to remove:
  Docker/container/SSH/HTTP execution endpoints, server/script classification, replay UI, environment-file hints, app-specific docs/OpenAPI, database toggles, and historical run UI controls.

Open questions:
  Should a raw binary be accepted by `run`, and if so how is its ownership/working directory handled? How should Vix expose build configuration/platform options to run without duplicating syntax? Exact behavior of repeated `--env` and overriding inherited environment needs a concise contract.

Representative workflows:
  `vix run`; `vix run app -- --port 8080`; `vix run --cwd data --env MODE=debug app -- input.txt`; single source with separate compile/linker options and program-argument delimiter.

### COMMAND: dev

Intent:
  Repeatedly run a C++ target while relevant project inputs change.

Primary inputs:
  Same executable selection and program arguments as `run`; watched project inputs.

Simple syntax:
  `vix dev`; `vix dev app -- --verbose`.

Generic C++ controls:
  Share target, working directory, environment, runtime arguments, build configuration, platform, and verbosity semantics with `run`/`build` where applicable.

Backend escape hatches required:
  No separate watcher/backend arguments are justified yet. CMake build customization should use the shared explicit CMake boundary.

Invalid combinations:
  Watch mode on a non-project input unless file-watch semantics are defined; run-only arguments accidentally consumed by Vix; claiming guaranteed process restart where the process abstraction cannot stop/wait/restart safely.

Capabilities that should remain internal:
  Watch backend, polling/debounce, event filtering, lifecycle mechanics.

Application-specific/historical concepts to remove:
  Server/frontend/web/app classification and framework-specific restart hooks.

Open questions:
  Current dev delegates to watched-run behavior, but native `vix.toml` watch behavior is reported unavailable; current contract therefore has a capability gap. Is `dev` public only when native watch and managed process lifecycle are verified? What is the exact watched-input set? Which restart guarantees can be truthfully promised?

Representative workflows:
  `vix dev`; `vix dev app -- --config dev.toml`; change source → rebuild/re-run, without application-specific interpretation.

### COMMAND: test

Intent:
  Discover, select, and run project tests through the project's supported test model.

Primary inputs:
  Native `Test` registrations or external CMake test discovery; optional test filter.

Simple syntax:
  `vix test`; `vix test network`; `vix test --list`.

Generic C++ controls:
  Stable filter semantics (exact, substring, or glob/regex must be selected), list-only, watch, fail-fast, verbosity, and potentially generic parallelism/timeout controls.

Backend escape hatches required:
  External CMake users may have legitimate advanced CTest controls. A named `--ctest <arg>` channel, or an explicit raw backend mode, is a candidate if common test controls cannot model real workflows. It must not be confused with test filters or program arguments. No evidence yet establishes which CTest flags are indispensable; this remains an expert-workflow validation item.

Invalid combinations:
  Conflicting exact and pattern filters; raw CTest options on native projects; watch combined with a one-shot raw backend invocation unless semantics are explicit; filter supplied more than once if only one filter is supported.

Capabilities that should remain internal:
  CTest discovery metadata/protocol, CTest command construction, native test-to-target build mapping.

Application-specific/historical concepts to remove:
  App test metadata, legacy test binary guessing, runtime-check mode unrelated to registered tests.

Open questions:
  Filter contract (exact/substring/regex/tags); whether `--watch` is supported for native and CMake projects equally; list output identity; whether fail-fast has meaning when the backend executes tests in parallel; which CTest controls warrant an explicit escape hatch.

Representative workflows:
  `vix test`; `vix test parser`; `vix test --list`; CMake project with an advanced CTest selection/timeout using an explicit backend boundary.

### COMMAND: repl

Intent:
  Start the interactive C++ REPL.

Primary inputs:
  Interactive input; optional arguments intentionally passed into the REPL.

Simple syntax:
  `vix repl`.

Generic C++ controls:
  C++ standard, include paths, definitions, package/project context, compiler/toolchain may matter for compiling entered expressions, but the current command has no corresponding public options.

Backend escape hatches required:
  Avoid turning the REPL into a raw compiler driver. If compiler flags are needed, prefer inheriting discovered project/toolchain context or a small explicit compiler-options mechanism after real workflows are validated.

Invalid combinations:
  Ambiguous `--` use: current REPL forwards the tail to `Vix.args()`; it is not a compiler/backend argument separator.

Capabilities that should remain internal:
  REPL parser/evaluator, history, line editing, temporary compilation details.

Application-specific/historical concepts to remove:
  Network-server examples/options as the central REPL explanation; application runtime bootstrapping.

Open questions:
  Should REPL discover a project and reuse package/toolchain context, or remain a standalone C++ shell? What is its supported language standard and compilation model? Current help calls it “Vix Reply REPL” and shows app-like `--port` arguments, which muddies the C++ intention.

Representative workflows:
  `vix repl`; optionally a documented REPL argument tail for REPL-level arguments only.

### COMMAND: add

Intent:
  Add a Registry or Git package requirement to the current native project and resolve/update its lock.

Primary inputs:
  Registry `publisher/package` or Git URL; optional version/revision information.

Simple syntax:
  `vix add publisher/package`; `vix add https://host/path/repo.git`.

Generic C++ controls:
  Version constraints and Registry selection are dependency controls; selecting a specific Git ref is an expert source pin.

Backend escape hatches required:
  Git URL plus mutually exclusive `--branch`, `--tag`, or full `--commit` are source-specific controls rather than a general Vix build backend escape. Registry `--registry` selects a logical Registry, not a host URL.

Invalid combinations:
  Git revision flags on Registry packages; Registry/version flags on Git URL; more than one Git revision selector; malformed identity or commit ID.

Capabilities that should remain internal:
  Resolver, metadata sources, resolution catalog, lock serialization, materialization.

Application-specific/historical concepts to remove:
  Old module installer, global installation, application plug-in semantics.

Open questions:
  Current bare Registry identity selects the latest stable release before creating a compatible constraint; is this beginner behavior sufficiently clear and reproducible? Does explicit exact/compatible syntax belong in help as advanced detail? Whether shorthand `publisher/package@constraint` is desirable is unresolved and must not change frozen resolution semantics in this research phase.

Representative workflows:
  `vix add acme/json`; `vix add acme/json --registry community --version compatible=1.2.0`; `vix add <git-url> --tag v1.0`.

### COMMAND: remove

Intent:
  Remove one root dependency identified by Registry identity or Git source.

Primary inputs:
  `publisher/package` with optional Registry ID, or Git URL.

Simple syntax:
  `vix remove publisher/package`; `vix remove <git-url>`.

Generic C++ controls:
  Optional `--registry <id>` to disambiguate/select the logical Registry.

Backend escape hatches required:
  None beyond the source identity forms; exact Git URL normalization is an implementation concern.

Invalid combinations:
  Registry flag on Git URL; ambiguous identity if package declarations share identity/source form; nonexistent root requirement.

Capabilities that should remain internal:
  Requirement matching, transitive graph recomputation, lock transaction.

Application-specific/historical concepts to remove:
  Module removal or filesystem package deletion semantics.

Open questions:
  Is a concise alias/identity syntax needed to remove two Git sources with equivalent normalized URL? Current public model is intentionally symmetric with add at the identity level but not revision-specific.

Representative workflows:
  `vix remove acme/json`; `vix remove community/json --registry community`; `vix remove <git-url>`.

### COMMAND: update

Intent:
  Re-resolve all declared root dependencies and write the resulting lock state.

Primary inputs:
  Current native manifest, Registry/Git metadata, existing lock state.

Simple syntax:
  `vix update`.

Generic C++ controls:
  None currently; all-root update is the frozen format-1 behavior.

Backend escape hatches required:
  None.

Invalid combinations:
  Package selector arguments are currently rejected; unsupported stale/invalid manifest/lock state fails rather than being silently ignored.

Capabilities that should remain internal:
  Resolver/catalog/source selection and transaction semantics.

Application-specific/historical concepts to remove:
  Historical “upgrade Vix” meaning; self-upgrade is the separate `upgrade` command.

Open questions:
  Future package-specific update may be useful, but current format/runtime intentionally updates the whole root graph. It is not a current option and must not be implied by a positional argument.

Representative workflows:
  `vix update` after changing constraints or when intentionally refreshing the complete dependency graph.

### COMMAND: install

Intent:
  Acquire/materialize exactly the package graph already recorded by `vix.lock`.

Primary inputs:
  Current `vix.toml`, current valid lock, package-content stores and configured sources.

Simple syntax:
  `vix install`.

Generic C++ controls:
  Offline operation is a generic capability worth exposing if the implementation can guarantee no network/content-source call and distinguish missing local content. Cache/store completeness is not itself a user mode; it is a precondition/result.

Backend escape hatches required:
  None.

Invalid combinations:
  Package selectors/URLs; missing, stale, invalid, or unnecessary lock when requirements demand a current lock; offline mode with unavailable locked bytes.

Capabilities that should remain internal:
  Store layout, digest directories, acquisition adapters, materialization paths.

Application-specific/historical concepts to remove:
  `install <package/url>`, global package installation, resolution/latest selection.

Open questions:
  Does the current acquisition API have a strict offline mode and a precise missing-content error? The inspected `install` parser accepts no arguments and help says exact locked graph, but no user-facing offline option is evident. Is no-package install a no-op (currently it reports nothing to install)?

Representative workflows:
  Online: `vix install`; CI/offline preparation: prefill content online, then run a strict `vix install --offline` candidate, only if strict semantics are implemented and tested.

### COMMAND: publish

Intent:
  Submit the current package release described by `vix.package.toml` to its configured Registry authority.

Primary inputs:
  Package contract, `registry/publisher/package/version` identity, P10O content, publishing configuration, Registry-specific token environment variable.

Simple syntax:
  `vix publish`.

Generic C++ controls:
  Package identity/config are manifest-owned; per-Registry endpoint and credential source are configuration/environment-owned. A dry-run or validate-only mode could be useful but should only be exposed if it has a stable, meaningful guarantee (e.g. no network and exact computed digest shown).

Backend escape hatches required:
  None; HTTPS is the current adapter but not a user-facing backend flag. Registry-neutral transport/configuration remains internal.

Invalid combinations:
  Git-only package without Registry identity; missing Registry publication target/credential; identity override that disagrees with the package manifest; successful reporting before authority acceptance.

Capabilities that should remain internal:
  Publication JSON/envelope construction, bearer handling, P10O collection, HTTP transport, authority signing/admission.

Application-specific/historical concepts to remove:
  Application deployment, Git tags as publication authority, Git pushes/PRs/cloud deploy, global Vix login.

Open questions:
  Should `--registry` be an override? Current package identity includes a registry and publication routes by it; allowing a CLI override creates mismatch risk and should likely be prohibited unless it means a non-authoritative target override with explicit validation. A safe `--dry-run` could prepare/validate and show identity/digest without credentials/network, but this is not implemented and its output contract is open. Authentication should remain configured per Registry via environment indirection, not CLI token flags.

Representative workflows:
  Author manifest → `vix add` dependencies → `vix install`/build → `vix publish`; repeat same identity/version/content idempotently; changed content at same version must be rejected by the authority.

### COMMAND: upgrade

Intent:
  Upgrade the current Vix installation when that installation is managed by Vix.

Primary inputs:
  Install receipt, current host, selected stable channel/source config, trusted release key.

Simple syntax:
  `vix upgrade`.

Generic C++ controls:
  None currently needed for the ordinary managed installation. Installation owner controls whether Vix may replace files.

Backend escape hatches required:
  None; source URLs and transport are distribution configuration, not arbitrary command-line network choices.

Invalid combinations:
  External/archive installation self-replacement; another generation; unsupported host; invalid signature/digest; component-bearing unsupported release.

Capabilities that should remain internal:
  Release source, signature verification, exact host selection, archive validation, staging, activation, rollback.

Application-specific/historical concepts to remove:
  Project dependency upgrade, package Registry use, Vix2 generation crossing.

Open questions:
  Should `--check` or channel/version override ever exist? These are plausible but not necessary for the core contract. A version override can bypass the configured release channel unless still constrained by a signed manifest and explicit generation/channel policy. Rollback is an installation operation but no public command is currently specified; preserve internal rollback until a tested UX is designed.

Representative workflows:
  Managed current install: `vix upgrade`; external owner: report owner and direct user to its manager; archive install: explain original installation method.

## 3. Cross-command concepts

### Project directory

Current build/run/test/dev all accept `-d/--dir` in their concise public help (run/dev also have `--cwd` for process working directory). The project directory must always mean the root from which ProjectDiscovery operates; runtime cwd is distinct. `new --dir` currently means a base/destination directory, not a project root. This semantic difference is legitimate but merits clear command-local wording. `init` currently acts only on cwd. A future universal `--dir` should not be assumed until these meanings are named distinctly.

### Target versus platform

Target name selects a `Project.targets` entry. Platform triple selects compilation output ABI/OS. Prefer positional target plus a separately named `--platform`; avoid `--target` for both meanings. Toolchain choice may be a generic build concept (compiler family/target/sysroot) while CMake toolchain-file syntax is backend-specific.

### Configuration

The candidate simple model is a common debug/release configuration, with debug as default if that matches both native and external-CMake behavior. CMake has arbitrary named configurations/cache state; those should be expressed by project configuration or the explicit CMake escape hatch rather than carrying historical preset vocabulary into every command. Current build help advertises `--release`; current internal build parser also knows `--preset`, and test internals select `dev-ninja`. Native/CMake parity and persistence need confirmation before freezing.

### Argument channels

There are at least five separate streams: Vix CLI options; CMake configure/cache arguments; compiler arguments for one-file compilation; linker arguments for one-file compilation; runtime program arguments; and optionally CTest runner arguments. One `--` cannot safely mean all of them. `run`'s separator should mean program arguments only. Backend channels should be explicitly named and only accepted in their matching input/backend mode.

### Verbosity/environment

`--verbose`, `-q/--quiet`, `-h/--help`, `-v/--version` are the current global vocabulary. A consistent precedence rule is needed if global verbosity and command-local aliases are accepted together. `--cwd` and `--env KEY=VALUE` are generic process controls; avoid environment-file/application hints as hidden behavior. `--log-level` is not in current top-level help; `VIX_LOG_LEVEL` remains an environment implementation hook and need not be public absent a user need.

## 4. Backend escape-hatch analysis

| Backend/capability | Simple Vix abstraction | Expert escape hatch | Needed now? |
|---|---|---|---|
| CMake build/configure | `vix build [target]`, common configuration/jobs/platform | Explicit CMake configure-argument channel, e.g. a named `--cmake` boundary; toolchain file may be a first-class generic toolchain choice or a CMake-specific argument | Yes for preserving expert external-CMake configuration; exact syntax/open policy unresolved |
| CTest | `vix test [filter]`, `--list`, generic watch/fail-fast/filter | Named CTest-only argument boundary if real workflows require controls not represented generically | Likely useful for external CMake compatibility, but required flags need evidence |
| Compiler, one-file mode | `vix build source.cpp` | Repeatable explicit compiler-argument channel for include/define/standard/warning flags | Yes, ordinary expert C++ one-file workflows need these |
| Linker, one-file mode | Vix links source program using selected toolchain | Separate linker-argument channel for library paths/libraries/linker flags | Yes for linking non-default libraries; avoid conflation with compiler args |
| Toolchain | Generic toolchain/platform/sysroot selection | Backend-specific toolchain file (CMake) under explicit CMake boundary, or a shared `--toolchain` reference if the model is mature | Generic toolchain concept yes; exact interface unresolved |
| Ninja/build tool | Vix build/jobs/output | No public passthrough by default; possibly CMake escape hatch only for configure settings, not arbitrary Ninja invocation | No evidence for public Ninja control; preserve engine capability internally |

The key distinction is control over the project/toolchain versus control over Vix's backend execution. CMake cache values and toolchain selection change the configured C++ project and are legitimate. Ninja executor details, heartbeat, graph executor, and cache implementation do not. Arbitrary CMake build-tool arguments should not be silently folded into configure arguments; if later required, give them a separate explicitly named boundary.

## 5. Current-option classification

Labels follow the requested vocabulary exactly. “Advertised/accepted” describes the inspected current front door; “internal parser residue” means lower-level code contains the option, but the public argument normalizer/help does not currently admit it. This is a classification inventory, not a change request.

### Build

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| `[target]`, positional target | Help + public normalizer | KEEP_SIMPLE |
| `[source.cpp]` | Help + public normalizer | KEEP_SIMPLE |
| `-d`, `--dir` | Help + normalizer | KEEP_SIMPLE |
| `-j`, `--jobs` | Help + normalizer | KEEP_GENERIC_ADVANCED |
| `--clean` | Help + normalizer | KEEP_GENERIC_ADVANCED |
| `--platform <triple>` (mapped internally to target triple) | Help + normalizer | KEEP_GENERIC_ADVANCED |
| `--out <path>` | Help + normalizer | KEEP_GENERIC_ADVANCED |
| `--release` | Help + normalizer, mapped to release preset | KEEP_GENERIC_ADVANCED |
| `-v`, `--verbose` | Help + normalizer | KEEP_SIMPLE |
| `-q`, `--quiet` | Help + normalizer | KEEP_SIMPLE |
| `-h`, `--help` | Help + normalizer | KEEP_SIMPLE |
| `--` raw single-file compiler flags | Public normalizer permits only after `.cpp`; meaning is broad/underspecified | REPLACE_WITH_EXPLICIT_ESCAPE_HATCH |
| `--debug`, `--debug-log`, `--log` | Lower parser residue; not in concise help/front-door allowlist | MAKE_INTERNAL |
| `--heartbeat`, `--no-heartbeat` | Lower parser residue | MAKE_INTERNAL |
| `--graph-executor` | Lower parser residue | MAKE_INTERNAL |
| `--explain`, `--page`, `--limit`, `--report` | Lower parser residue/diagnostic UI controls | MAKE_INTERNAL |
| `--watch` | Lower parser residue; belongs to `dev`, not build | MAKE_INTERNAL |
| `--warnings`, `--warning-check` | Lower parser residue; possible generic diagnostics policy needs separate research | UNRESOLVED |
| `--sanitize`, `--san`, `--asan`, `--ubsan`, `--tsan` | Lower parser residue, many aliases for instrumentation | KEEP_GENERIC_ADVANCED |
| `--managed-sdk` | Lower parser residue/profile-specific | REMOVE_SPECIALIZED |
| `--preset` | Lower parser residue/custom backend configuration | REPLACE_WITH_EXPLICIT_ESCAPE_HATCH |
| `--with-sqlite`, `--with-mysql` | Lower parser residue | REMOVE_SPECIALIZED |
| `--target <triple>` | Lower parser spelling collides conceptually with build target; platform equivalent is clearer | REPLACE_WITH_EXPLICIT_ESCAPE_HATCH |
| `--build-target <name>` | Lower parser residue duplicates positional target | MAKE_INTERNAL |
| `--require-project-metadata` | Internal dispatch guard | MAKE_INTERNAL |
| `--targets` | Lower parser residue/target-list operation | MAKE_INTERNAL |
| `--cmake-verbose` | Backend detail | REPLACE_WITH_EXPLICIT_ESCAPE_HATCH |
| `--sysroot` | Lower parser residue, legitimate cross toolchain concept | KEEP_GENERIC_ADVANCED |
| `--static` | Lower parser residue; generic linkage mode may be useful but has platform implications | UNRESOLVED |
| `--bin` | Lower parser output mode duplicate | MAKE_INTERNAL |
| `--fast` | Lower parser strategy alias | MAKE_INTERNAL |
| `--no-cache` | Cache policy internal unless concrete reproducibility contract is established | MAKE_INTERNAL |
| `--linker` | Lower parser linker implementation choice | KEEP_GENERIC_ADVANCED |
| `--launcher` | Lower parser toolchain detail | MAKE_INTERNAL |
| `--no-status`, `--no-up-to-date` | UI/cache implementation behavior | MAKE_INTERNAL |
| `-- [cmake args...]` | Lower-level raw CMake tail if available in backend path; ambiguous with compiler/runtime separators | REPLACE_WITH_EXPLICIT_ESCAPE_HATCH |

### Run

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| `[target]`, `[source.cpp]`, optional direct binary | Help + normalizer | KEEP_SIMPLE |
| `-d`, `--dir` | Help + normalizer | KEEP_SIMPLE |
| `--cwd` | Help + normalizer | KEEP_GENERIC_ADVANCED |
| `--env KEY=VALUE` | Help + normalizer | KEEP_GENERIC_ADVANCED |
| `--verbose`, `-q/--quiet`, `-h/--help` | Help + normalizer | KEEP_SIMPLE |
| `-- <program args...>` | Help + normalizer; run-only runtime argument separator | KEEP_SIMPLE |
| `--watch` | Rejected by run public normalizer; dev uses internal watched entry | MAKE_INTERNAL |
| `--preset`, `--jobs`, `--clean` | Historical/lower parser options blocked by public normalizer | MAKE_INTERNAL |
| `--check` | Historical lower parser option, duplicates build intent | MAKE_INTERNAL |
| `--replay` | Historical replay implementation | REMOVE_SPECIALIZED |
| `--reload` | Watch alias/history | MAKE_INTERNAL |
| `--force-server`, `--force-script` | Application classification | REMOVE_SPECIALIZED |
| `--ui`, `--no-ui` | Historical UI controls | MAKE_INTERNAL |
| `--env-hint`, `--no-env-hint` | Application/environment presentation | REMOVE_SPECIALIZED |
| `--trace-cache`, `--no-trace-cache`, `--compiler-fingerprint` | Cache internals | MAKE_INTERNAL |
| `--auto-deps[=local|up]` | Historical dependency automation conflicts with explicit package model | REMOVE_SPECIALIZED |
| `--san`, `--no-san`, `--ubsan`, `--tsan` | Sanitizer modes are legitimate generic C++ controls, but run-specific aliases should be unified with build instrumentation | KEEP_GENERIC_ADVANCED |
| `--with-sqlite`, `--with-mysql`, `--local-cache` | Domain/cache modes | REMOVE_SPECIALIZED |
| `--docs`, `--no-docs`, `--docs=<value>` | Application documentation behavior | REMOVE_SPECIALIZED |
| `--clear`, `--no-clear` | Terminal/UI behavior | MAKE_INTERNAL |
| `--loglevel`, `--log-format`, `--log-color`, `--no-color` | Logging presentation aliases/configuration | MAKE_INTERNAL |
| `docker://`, `container://`, `ssh://`, `http://` inputs | Historical remote/application execution modes | REMOVE_SPECIALIZED |

### Dev

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| `[target]`, `[source.cpp]`, `-- <program args...>` | Help; forwards to watched run | KEEP_SIMPLE |
| `-d`, `--dir`, `--cwd`, `--env`, `--verbose`, `-q/--quiet` | Help and shared run normalizer | KEEP_GENERIC_ADVANCED |
| Watch mode itself | Implicit command intention, not a flag | KEEP_SIMPLE |
| `run --watch` | Not admitted publicly; internal watched call exists | MAKE_INTERNAL |
| Watch backend, debounce, restart details | Engine capability | MAKE_INTERNAL |
| `--reload`/historical app watch aliases | No canonical help; application-era | REMOVE_SPECIALIZED |

### Test

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| `[filter]` | Help/parser; current semantics need clarification (CMake maps to regex) | UNRESOLVED |
| `-d`, `--dir` | Help/parser | KEEP_SIMPLE |
| `--list` | Help/parser | KEEP_SIMPLE |
| `--watch` | Help/parser; watcher reruns tests | KEEP_GENERIC_ADVANCED |
| `--fail-fast` | Help/parser | KEEP_GENERIC_ADVANCED |
| `--verbose`, `-q/--quiet` | Help/parser | KEEP_SIMPLE |
| `--exact-test`, `--exact-test=<name>` | Lower parser/legacy variant | REPLACE_WITH_EXPLICIT_ESCAPE_HATCH |
| `--test <regex>`, `-R <regex>`, `--tests-regex` | CTest/raw pattern spellings | REPLACE_WITH_EXPLICIT_ESCAPE_HATCH |
| `--raw`, `--run` | Legacy runner modes | REMOVE_SPECIALIZED |
| `--preset[=]`, `--build-target[=]` | CMake/build implementation controls | MAKE_INTERNAL |
| `-- [ctest args...]` | Historical/raw backend tail, not normal help | REPLACE_WITH_EXPLICIT_ESCAPE_HATCH |
| `-j`, `--parallel`, `--test-load`, `--timeout` | CTest lower parser/backend controls; generic parallelism/timeout may deserve stable Vix forms | UNRESOLVED |
| Internal `-N`, `-R`, `--parallel`, `--timeout` forwarding | Backend implementation | MAKE_INTERNAL |

### Repl

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| No REPL-specific flags | Current help shows no options | KEEP_SIMPLE |
| `-- <args...>` | Passed to `Vix.args()` inside REPL | KEEP_GENERIC_ADVANCED |
| C++ standard/include/define/toolchain controls | No current CLI support; should inherit project context if designed | UNRESOLVED |

### Add

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| `<publisher>/<package>` | Help/parser | KEEP_SIMPLE |
| `<git-url>` | Help/parser | KEEP_SIMPLE |
| `--registry <id>` | Help/parser; logical Registry selection | KEEP_GENERIC_ADVANCED |
| `--version exact=...|compatible=...` | Help/parser; version constraints | KEEP_GENERIC_ADVANCED |
| `--branch`, `--tag`, `--commit` | Help/parser; mutually exclusive Git source pin | KEEP_GENERIC_ADVANCED |
| Bare Registry package latest-stable inference / Git default branch discovery | Implicit behavior, no flag | UNRESOLVED |
| Old global/module install switches (if any downstream implementation remnants) | Not in current parser | REMOVE_SPECIALIZED |

### Remove

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| `<publisher>/<package>` | Help/parser | KEEP_SIMPLE |
| `<git-url>` | Help/parser | KEEP_SIMPLE |
| `--registry <id>` | Help/parser | KEEP_GENERIC_ADVANCED |
| Package filesystem deletion or global removal | Not part of current command parser | REMOVE_SPECIALIZED |

### Update

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| No option; updates all root requirements | Help/parser | KEEP_SIMPLE |
| Positional package selector | Explicitly rejected; future selective update is not current behavior | UNRESOLVED |

### Install

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| No arguments; exact lock materialization | Help/parser | KEEP_SIMPLE |
| `--offline` | Not currently accepted/evidenced | KEEP_GENERIC_ADVANCED |
| package/URL selectors | Explicitly rejected; historical semantics | REMOVE_SPECIALIZED |

### Publish

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| No arguments | Help/parser | KEEP_SIMPLE |
| Package Registry identity in manifest | Required semantic input, not a CLI option | KEEP_SIMPLE |
| `--registry` override | Not supported; could conflict with manifest authority | UNRESOLVED |
| `--dry-run`/validation-only | Not supported; potentially useful if semantics are precise | UNRESOLVED |
| CLI token/password option | Not supported; credentials use configured environment variable | MAKE_INTERNAL |
| Git tag/PR/cloud deployment options | Removed historical behavior | REMOVE_SPECIALIZED |

### Upgrade

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| No arguments | Help/parser | KEEP_SIMPLE |
| `--check` | Not currently accepted | UNRESOLVED |
| `--version`, `--channel` | Not currently accepted; must remain within signed generation/channel policy if ever added | UNRESOLVED |
| rollback option/command | No public option; internal activation rollback exists | MAKE_INTERNAL |
| project dependency selectors | Not part of this command | REMOVE_SPECIALIZED |

### Global

| Current spelling/capability | Status/evidence | Classification |
|---|---|---|
| `-h`, `--help`; `vix help <command>` | Global dispatch | KEEP_SIMPLE |
| `-v`, `--version` | Global dispatch | KEEP_SIMPLE |
| `--verbose` | Global parser/logging | KEEP_SIMPLE |
| `-q`, `--quiet` | Global parser/logging | KEEP_SIMPLE |
| `--log-level` | Not in top-level help; environment `VIX_LOG_LEVEL` is internal/user environment hook | MAKE_INTERNAL |
| Color controls | Not canonical global help; legacy run internals only | MAKE_INTERNAL |

## 6. Real-world workflow validation

### Beginner native Vix

Candidate flow is expressible: `vix new hello`; `vix build`; `vix run`; `vix test`; `vix add acme/json`; `vix update`. Package addition semantics are less obvious than the build flow because bare Registry add discovers latest stable then records a compatible constraint; help should explain this in plain language.

### Advanced native Vix

Target selection is positional. Release mode is currently `--release`. Cross compilation has `--platform <triple>` and implementation supports some target strings, but custom toolchain selection/sysroot behavior must be confirmed. Offline install is not exposed despite being a natural workflow. A custom native toolchain contract is not fully evidenced.

### Existing CMake project

Discovery preserves CMake-only projects and rejects same-root native/CMake ambiguity. Build target selection can be positional. Public CMake configure values/toolchain-file access are missing from the current concise public parser/help, even though an expert CMake project often needs `-D` values and `CMAKE_TOOLCHAIN_FILE`. CTest remains an internal external-CMake implementation; ordinary filter/list exists, but raw advanced CTest control lacks a named public boundary. This workflow is not complete for advanced CMake users until escape-hatch semantics are chosen.

### Single C++ file

`vix build source.cpp` and `vix run source.cpp -- args` express compile/run. The current build separator allows arbitrary source compiler flags, but a single undifferentiated tail does not distinguish compiler versus linker arguments and conflicts conceptually with runtime `--`. Explicit compiler/linker channels are needed to make this an understandable expert workflow.

### Package author

Manifest dependencies → `vix add` → lock update → `vix install`/build → `vix publish` is supported. Registry identity/version is manifest-owned; per-registry publishing endpoint/token selection comes from publishing configuration and environment. No CLI override is needed for the baseline workflow.

### Managed Vix installation

`vix upgrade` supports managed installation and refuses external/archive ownership. No check-only/version/channel selector is needed to make this baseline usable. External-owner refusal is an intentional boundary.

## 7. Unresolved decisions

1. Define shared configuration names/semantics for native and external CMake builds; decide whether `--release` is the only common profile or whether project-defined configurations are needed.
2. Freeze CMake escape-hatch argument grouping and persistence (configure-time cache values versus per-invocation build-tool arguments).
3. Decide whether CMake toolchain files are a generic `--toolchain` choice or a CMake-only escape hatch.
4. Define explicit compiler and linker argument channels for single-file mode, including ordering and repeated values.
5. Define native `dev` support and truthful process restart/watch guarantees; current native watch is reported unavailable.
6. Define test filter semantics (exact, substring, regex/glob) and advanced CTest escape-hatch scope.
7. Decide which sanitizers/warning policy/timeout/parallelism are generic public C++ controls and unify aliases across build/run/test.
8. Decide offline install guarantee and observable missing-content behavior; no CLI flag is currently present.
9. Decide whether `publish --dry-run` has a strong no-network guarantee and whether a Registry override is ever legitimate when manifest identity already names the destination.
10. Clarify REPL project/package/toolchain context and whether `--` denotes REPL arguments only.
11. Clarify the boundary for direct binary execution in `run` and consistent project-directory semantics across `new`, `init`, build, run, and test.
12. Treat upgrade check/version/channel/rollback controls as future possibilities; do not expose them without a concrete signed-release policy and tested semantics.

## 8. Proposed minimal candidate surface

This is a review candidate, not a final syntax freeze:

```text
vix new <name> [--dir <base>] [--force]
vix init

vix build [target | source.cpp]
  -d/--dir, -j/--jobs, --release, --platform, --clean, --out
  --cmake <configure-arg> (CMake projects only; exact grouping unresolved)
  --compiler-arg <arg>, --linker-arg <arg> (single-file mode)

vix run [target | source.cpp | binary] [-- runtime-args...]
  -d/--dir, --cwd, --env, --verbose, -q/--quiet
  shared build-context options only after consistency is defined

vix dev [target | source.cpp] [-- runtime-args...]
  shares run controls; watch is command intent, not a public run flag

vix test [filter]
  -d/--dir, --list, --watch, --fail-fast
  --ctest <arg> only if external-CMake needs justify a named escape hatch

vix repl [REPL arguments only]

vix add <publisher>/<package> [--registry <id>] [--version <constraint>]
vix add <git-url> [--branch <name> | --tag <name> | --commit <oid>]
vix remove <publisher>/<package> [--registry <id>] | <git-url>
vix update
vix install [--offline]  # candidate only; only with strict offline semantics
vix publish              # identity/config/auth from package + publishing config
vix upgrade

Global: -h/--help, -v/--version, --verbose, -q/--quiet
```

The core candidate keeps normal commands short, preserves generic C++ controls, and gives backend-specific controls explicit names. Decisions marked unresolved must be settled before this candidate is called complete.
