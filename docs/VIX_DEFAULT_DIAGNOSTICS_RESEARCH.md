# Default Vix Diagnostics — Research

Status: DIAG-R2 DECISION FROZEN. This document records the evidence and rationale; `VIX_CLI_CONTRACT.md` is the implementation authority.

## 1. Current behavior

The shared `DiagnosticMode` aliases `vix::engine::SanitizerMode`. `RunDetail::Options` and test options default to `None`; native runtime entry points also default to `None`. `RunCommand` accepts an explicit mode and forwards it to native runtime or single-source compilation. `DevCommand` delegates to the watched Run path, so it currently inherits the same no-mode default. `TestsCommand` passes its selected mode into native test preparation and execution.

The current runtime manifest has `[build] san` values `off`, `ubsan`, and `asan_ubsan`, with `off` as the in-memory default. `RunManifestMerge` maps those legacy values to `None`, `Undefined`, and `AddressUndefined`. An explicit non-`None` CLI mode wins over the manifest. The manifest model does not represent ASan-only or TSan.

Explicit instrumentation is implemented in multiple backends. Native generated target/package CMake gets matching compile and link sanitizer flags. Direct single-source mode builds with the corresponding compiler sanitizer option and diagnostic flags. Sanitizer runtime options are installed before launch, and recognized sanitizer evidence can flow into Vix runtime diagnostics. External CMake run/test paths reject an explicit non-`None` Vix diagnostic mode because Vix cannot prove that an arbitrary project is instrumented.

Before DIAG-R2, ordinary run/test defaults did not instrument code. Human runtime evidence showed invalid programs could produce ordinary output without diagnostics, while explicit modes caught the specified memory, undefined-behavior, and race examples.

## 2. Frozen product principle

Candidate principle:

> A beginner should receive useful Vix runtime diagnostics without knowing what ASan, UBSan, or TSan are.

Vix diagnostics must not impose hidden production cost. `build` remains uninstrumented by default, and release execution receives no automatic instrumentation. Human benchmarks show a measurable first-run cost but small incremental/no-op development overhead, supporting the automatic development profile.

### Human benchmark evidence

| Workload | Normal | ASan + UBSan | Observation |
|---|---:|---:|---|
| Single-source first run | 2.48 s | 3.31 s | Instrumentation has measurable initial cost. |
| Single-source cached run | 0.22 s | 0.29 s | Warm-path overhead is small. |
| Single-source changed source | 3.02 s | 3.39 s | Incremental compile overhead is modest. |
| Native multi-file no-op | 0.16 s | 0.15 s | No meaningful no-op penalty observed. |
| Native one-source incremental | 1.06 s | 1.10 s | Small incremental difference. |
| Native incremental max RSS | 46,096 KB | 50,140 KB | Modest measured memory increase. |

Do not use the normal `vix build --clean` timing as evidence: human observation showed `ninja: no work to do`, contradicting the frozen clean contract. Track separately as a `BUILD-I1` implementation defect; DIAG-R2 does not fix it.

## 3. Frozen automatic model

Frozen defaults:

| Command/context | Candidate default |
|---|---|
| `vix build` | Diagnostics off unless an explicit mode is selected. |
| `vix run` | ASan + UBSan for development native Vix and standalone C++ unless overridden by explicit CLI/project policy. |
| `vix dev` | Same development default where Vix controls compilation; lifecycle support remains separately gated. |
| `vix test` | ASan + UBSan for development native Vix projects. |
| `--release` on run/dev | No automatic diagnostics; explicit mode still wins where supported. |
| External CMake | No automatic Vix instrumentation; explicit Vix modes fail closed unless generic support is proven. |
| Explicit modes | Override automatic policy; TSan remains explicit only. |
| `--diagnostics=off` | Explicit opt-out on run/dev/test. |

This split is coherent with BUILD-R2 because the resolved diagnostic mode participates in build-context identity. It preserves a fast, uninstrumented `build` route and does not mix incompatible objects. A first `run` after a plain `build` may create a separate diagnostic context; later runs reuse it, and both contexts may coexist. This is accepted rather than sharing incompatible objects.

## 4. Project modes

### Native Vix projects

Automatically select ASan + UBSan for development run/dev/test when Vix controls compilation. Native project generation propagates the resolved mode to target compile/link settings, and test execution passes it into preparation and execution. Native watched lifecycle remains separately unfinished.

### Standalone `.cpp`

`vix run file.cpp` automatically uses ASan + UBSan in development. The direct compiler path has mode-specific instrumentation, a fingerprinted cache, and Vix runtime evidence handling. Its diagnostic cache entry is distinct from an uninstrumented source build.

### External CMake

Vix does not know whether an arbitrary CMake project instruments its targets. Ordinary run/test preserves project-defined behavior; explicit Vix diagnostic selection fails closed. Automatic policy does not claim instrumentation for external CMake. This deliberate asymmetry should be stated plainly in help: automatic Vix diagnostics apply where Vix controls compilation.

## 5. Release behavior

`run --release` and `dev --release` have no implicit diagnostic mode. Explicit `--san` with release configuration is allowed where the selected toolchain supports it: optimization and instrumentation can coexist. The explicit mode overrides the release default; unsupported combinations fail clearly.

The build context must distinguish release+diagnostic from release+none. External CMake still cannot be claimed instrumented absent a proven generic mechanism.

## 6. Explicit modes and precedence

The four explicit modes remain:

```text
--san   AddressSanitizer + UndefinedBehaviorSanitizer
--asan  AddressSanitizer
--ubsan UndefinedBehaviorSanitizer
--tsan  ThreadSanitizer
```

Precedence is explicit CLI choice (including explicit off), then an explicit project/manifest setting, then the command automatic policy, then none. Explicit `--ubsan` means UBSan only, not the combined profile. TSan is never automatic and remains exclusive from ASan.

Keep one resolved `DiagnosticMode` type, but store request state separately so it distinguishes unspecified, explicit off, and each explicit mode. A request/policy representation must resolve before build-context selection; build/runtime layers receive one final mode. Do not overload `DiagnosticMode::None` as both unspecified and explicitly disabled.

## 7. Public opt-out

Frozen sole spelling:

```text
--diagnostics=off
```

This names user intent and is mutually exclusive with all explicit modes. `--no-san` and alternate negative spellings remain retired.

## 8. Manifest precedence and compatibility

The frozen policy order is:

```text
explicit CLI mode or explicit CLI off
> explicitly configured project/manifest diagnostic policy
> command default
> none
```

Preserve manifest field presence internally without changing syntax or format version: omitted means no project decision; explicit `san = "off"` opts out of the automatic policy; `ubsan` and `asan_ubsan` map to their existing modes. Do not add ASan-only or TSan manifest syntax. The current parser/model conflates omission and off, so implementation must retain presence at parse/merge time.

Legacy `ubsan` maps to `Undefined`; `asan_ubsan` maps to `AddressUndefined`. There is no legacy manifest-only ASan or TSan mode. CLI-only ASan/TSan can remain available independently if automatic policy is adopted.

## 9. Performance mechanics to measure

Static code shows these mechanisms; human timings/RSS are recorded in section 2. The supplied measurements do not include every metric below:

- Native target/package generation applies sanitizer flags at compile and link. ASan/UBSan modes also request debug/frame information; the combined and UBSan modes use no-recover behavior. TSan uses its own mode and separate options.
- Direct single-source mode builds with compiler sanitizer flags in the compile/link invocation; its cache fingerprint includes the selected mode, so switching modes produces distinct cache entries.
- Runtime launch sets the corresponding ASAN/UBSAN/TSAN environment options. This adds runtime work and changes report behavior, but static inspection cannot quantify it.
- Instrumented binaries have different code and runtime linkage. Exact output size and resident memory must be measured on representative workloads.
- No-op builds should avoid recompilation when the same context is already configured; whether the extra context selection/metadata path adds meaningful latency must be measured.

Benchmark wall time, user time, system time, maximum RSS, exit code, and artifact size. Distinguish first instrumented compile, one-source incremental rebuild, no-op rebuild, cold run (compile plus execution), and warm run (execution with cache hit). Record compiler, OS/host, Vix revision/build type, generator, and command line with every result.

## 10. Human benchmark methodology and commands

These commands document a reproducible HUMAN methodology; they were not run by the agent. Use an isolated temporary root and a fixed workload. Do not clear global caches. Keep all builds at `-j2` or lower. Use the same source bytes and toolchain across modes. For single-source cold measurements, make one copy of the fixture per mode under a unique temporary path so each mode/path begins with an isolated fingerprint key. Re-run the same command for the warm/no-op measurement.

Suggested reproducible workload: one source that allocates and repeatedly fills/scans a fixed-size vector and prints a deterministic checksum. It should be race-free, exercise memory accesses for sanitizers, avoid undefined behavior, and run long enough for timings to be stable. A single-thread workload avoids adding an implicit pthread-linking variable to the single-source path; TSan overhead can still be measured, while race-detection correctness is separately evidenced. Use identical source bytes in every mode and retain the source in the benchmark record. Example workload:

```cpp
#include <cstdint>
#include <iostream>
#include <vector>

int main()
{
  constexpr std::size_t count = 1'000'000;
  std::vector<std::uint64_t> values(count);
  std::uint64_t checksum = 0;
  for (std::uint64_t round = 0; round < 20; ++round)
  {
    for (std::size_t index = 0; index < count; ++index)
      values[index] = static_cast<std::uint64_t>(index) + round;
    for (const auto value : values)
      checksum += value;
  }
  std::cout << checksum << '\n';
}
```

Example shell setup on a POSIX host (substitute actual Vix and fixture paths):

```sh
VIX=/absolute/path/to/current/vix
FIXTURE=/absolute/path/to/diagnostic_workload.cpp
WORK="$(mktemp -d "${TMPDIR:-/tmp}/vix-diag-r1.XXXXXX")"
mkdir -p "$WORK/src"
for mode in normal san asan ubsan tsan; do
  mkdir -p "$WORK/src/$mode"
  cp "$FIXTURE" "$WORK/src/$mode/main.cpp"
done
```

Single-source cold build, repeat/no-op build, and warm run (record one row per mode; omit the diagnostic switch for `normal`):

```sh
/usr/bin/time -f 'wall_s=%e user_s=%U sys_s=%S max_rss_kb=%M exit=%x' \
  -o "$WORK/san-cold.txt" "$VIX" build "$WORK/src/san/main.cpp" \
  --out "$WORK/san-bin" -j2 --san
/usr/bin/time -f 'wall_s=%e user_s=%U sys_s=%S max_rss_kb=%M exit=%x' \
  -o "$WORK/san-noop.txt" "$VIX" build "$WORK/src/san/main.cpp" \
  --out "$WORK/san-bin" -j2 --san
/usr/bin/time -f 'wall_s=%e user_s=%U sys_s=%S max_rss_kb=%M exit=%x' \
  -o "$WORK/san-run-warm.txt" "$VIX" run "$WORK/src/san/main.cpp" --san
stat -c 'artifact_bytes=%s' "$WORK/san-bin"
```

Repeat with `--asan`, `--ubsan`, `--tsan`; normal omits the mode switch. If GNU `/usr/bin/time` is unavailable, shell `time` can collect wall/user/system time, but maximum RSS must be recorded with a platform resource monitor or marked unavailable. Use the same command forms for each mode. Avoid treating a `build` duration as a runtime duration; report cold `run` separately if desired.

For a small native project, create one neutral project with `vix new`, then add one test registration to its native manifest (for example a `smoke` test targeting its executable) and replace its source with the fixed workload. Keep that project in the isolated temporary root. Measure:

```sh
/usr/bin/time -f 'wall_s=%e user_s=%U sys_s=%S max_rss_kb=%M exit=%x' \
  -o "$WORK/native-normal-clean.txt" "$VIX" build --dir "$WORK/native" --clean -j2
/usr/bin/time -f 'wall_s=%e user_s=%U sys_s=%S max_rss_kb=%M exit=%x' \
  -o "$WORK/native-normal-noop.txt" "$VIX" build --dir "$WORK/native" -j2
touch "$WORK/native/src/main.cpp"
/usr/bin/time -f 'wall_s=%e user_s=%U sys_s=%S max_rss_kb=%M exit=%x' \
  -o "$WORK/native-normal-incremental.txt" "$VIX" build --dir "$WORK/native" -j2
/usr/bin/time -f 'wall_s=%e user_s=%U sys_s=%S max_rss_kb=%M exit=%x' \
  -o "$WORK/native-san-clean.txt" "$VIX" build --dir "$WORK/native" --clean -j2 --san
```

For accurate context-isolated clean/incremental comparisons, use separate copies of the same project for normal and `san`, since `--clean` intentionally targets only the selected context. Repeat no-op/incremental commands in each. Measure warm run and test using the matching explicit mode. Example test measurement:

```sh
CMAKE_BUILD_PARALLEL_LEVEL=2 /usr/bin/time \
  -f 'wall_s=%e user_s=%U sys_s=%S max_rss_kb=%M exit=%x' \
  -o "$WORK/native-test-san.txt" "$VIX" test --dir "$WORK/native-san" --san
```

Native `test` currently has no public jobs option; set `CMAKE_BUILD_PARALLEL_LEVEL=2` in the environment if the selected backend respects it, and verify observed backend concurrency. If concurrency cannot be constrained to two, omit that test-build measurement rather than violating the cap.

No suitable checked-in native Vix project fixture was found by the static inventory; the repository is primarily CMake-based. A representative medium native benchmark therefore remains optional and must not be replaced by timing the entire Vix repository.

## 11. Decision rationale

Use measured deltas, not arbitrary absolute thresholds. Consider:

- The measured single-source cold cost is noticeable, but cached and changed-source timings are close; the native no-op/incremental timings are also close. This supports automatic development diagnostics for run/dev/test.
- Separate contexts may make a first `run` build after a plain `build`; never mix incompatible objects to hide that cost.
- The evidence does not authorize automatic production/release instrumentation. Release defaults remain off.
- Compare ASan, UBSan, and combined behavior separately; do not infer combined cost as a simple sum.
- Keep TSan explicit; the frozen recommended automatic profile is exactly ASan + UBSan.
- Treat correctness evidence and performance as separate axes: a benchmark cannot negate the value of diagnostics, and a bug report alone cannot quantify default cost.

## 12. TSan source precision note

Human evidence says TSan correctly diagnosed the data race but Vix selected a loop line instead of the exact increment line. Current `find_best_runtime_location()` scans report lines in order, considers lines containing `#`, and returns the first usable source location. `DataRaceRule` uses this common location helper rather than first prioritizing the exact “Read/Write of size” access section or comparing both conflicting stack traces. Thus an earlier valid loop/caller frame can win over the exact access frame. This is a diagnostic precision improvement opportunity, not evidence that race detection or classification is incorrect. It is deliberately not fixed here.

## 13. Frozen decision and implementation follow-ups

Frozen: `build` defaults off; development `run`/`dev`/`test` defaults to ASan + UBSan where Vix controls compilation; release defaults off; external CMake receives no automatic Vix instrumentation; explicit CLI choice/off wins over manifest, which wins over command policy; TSan is explicit only; omission differs from manifest off; and `--diagnostics=off` is the sole opt-out.

Implementation follow-ups do not reopen the product decision: preserve manifest-field presence internally, resolve the request before context selection, complete native `dev` lifecycle verification separately, and fix the `BUILD-I1` `--clean` defect. The TSan frame precision improvement remains separate future work.
