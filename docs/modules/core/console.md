# Vix Console (core)

Vix Console is a **Node.js-like runtime console** designed for **developer-facing output**: quick feedback, local debugging, and lightweight observability during development.

It is intentionally **not** a production logger. For structured logs, persistence, ingestion, and production-grade pipelines, use **`vix::utils::Logger`** (modules/utils).

---

## Goals (what Vix Console guarantees)

Vix Console is built around a “Dev-proof / Node-like trust contract”:

- **Zero-config**: usable immediately after `#include <vix/console.hpp>`.
- **Node-like DX**: `log/info/warn/error/debug` with familiar semantics.
- **Safe defaults**:
  - Default level = `info`
  - `debug` is **OFF** by default
  - `warn/error` → `stderr`
  - `log/info/debug` → `stdout`
- **Stable behavior**:
  - Environment variables are read **once** (startup / first use).
  - No “magical reconfiguration” mid-execution.
- **Performance-safe**:
  - **If a level is filtered, the call is near-zero cost** (no formatting, no timestamp, no lock, no I/O).
- **Thread-safe, readable output**:
  - Each call emits **one atomic line** (no interleaving between threads).
- **Minimalism**:
  - No files, no JSON by default, no heavy dependencies.

---

## Non-goals (what it is not)

Vix Console does **not** aim to:

- be a production logging system
- write to files
- provide JSON/KV structured output by default
- integrate with ingestion pipelines (ELK, OpenTelemetry, etc.)
- guarantee non-blocking I/O under all environments
- replace or compete with `vix::utils::Logger`

## What Vix Console is NOT (by design)

Vix Console intentionally does **not** replicate the full JavaScript `console.*` API
such as `console.table`, `console.group`, `console.time`, or `console.count`.

Those features require shared mutable state, heavy formatting, or implicit timing,
which are unsafe or unpredictable in a multi-threaded C++ runtime.

For advanced debugging, structured output, performance analysis, or production-grade
instrumentation, use **`vix::utils::Logger`** instead.

---

## Quick start

```cpp
#include <vix/console.hpp>
using namespace vix;

int main() {
  console.log("Hello");
  console.info("Server started on", 8080);
  console.warn("Deprecated API used:", "/v1/users");
  console.error("Crash detected:", "segfault");
  console.debug("x =", 42); // debug is OFF by default
}
```

Expected output format (stable):

```
HH:MM:SS [info]  Hello
HH:MM:SS [info]  Server started on 8080
HH:MM:SS [warn]  Deprecated API used: /v1/users
HH:MM:SS [error] Crash detected: segfault
```

---

## API and semantics

### Methods

- `console.log(...)`
- `console.info(...)`
- `console.warn(...)`
- `console.error(...)`
- `console.debug(...)`

### Level policy

- `log` **==** `info`
- Default visibility:
  - `info/warn/error`: **visible**
  - `debug`: **hidden**
- Stream routing:
  - `warn` + `error` → **stderr**
  - `log` + `info` + `debug` → **stdout**

This routing aligns with standard UNIX expectations and enables common redirection patterns.

---

## Environment variables (read once, stable)

Vix Console supports environment-driven configuration without code changes.

### `VIX_CONSOLE_LEVEL`

Controls the initial minimum level.
> **Note (default behavior):** `VIX_CONSOLE_LEVEL` is an advanced, manual override.
> When unset, `vix::console` follows `VIX_LOG_LEVEL`, which is typically set by the Vix CLI.
This keeps a single “verbosity control” for most users (`--log-level`), while still allowing expert overrides.

Supported values:

- `debug`
- `info`
- `warn`
- `error`
- `off`

Examples:

```bash
VIX_CONSOLE_LEVEL=debug ./app
VIX_CONSOLE_LEVEL=warn  ./app
VIX_CONSOLE_LEVEL=off   ./app
```

**Stability rule**: environment is read once (startup / first use).
Changing environment variables during runtime must not cause “mid-flight” behavior changes.

### Color control

Vix Console follows widely adopted conventions:

- `NO_COLOR`
  If set, colors are disabled unconditionally.
- `VIX_COLOR=auto|always|never`
  - `auto` (default): colors only when writing to a TTY
  - `always`: force colors
  - `never`: disable colors

Notes:
- When output is piped to files / CI log collectors, `auto` typically disables colors to preserve readability.

---

## Performance model (critical contract)

### The golden rule

**If a level is filtered, the call is near-zero cost.**

When a level is disabled (example: `debug` off by default), Vix Console guarantees:

- no message construction
- no heap allocation
- no concatenation
- no formatting stream (`ostringstream`)
- no timestamp generation
- no lock acquisition
- no I/O

This is the primary reason developers can safely leave `console.debug(...)` in code paths without fearing “silent performance cliffs”.

### Practical guidance

- `console.debug(...)` is designed to be “almost free” when disabled.
- `console.info/log(...)` in hot loops can still be expensive because:
  - it performs I/O
  - it may lock
  - output sinks can block (pipes, collectors)

For performance-sensitive paths, keep high-frequency logging behind explicit guards or prefer dedicated telemetry mechanisms.

---

## Thread safety and atomic lines

### Guarantee

Each call to `console.*` emits a **single atomic line**, preventing log interleaving across threads.

This improves readability in multi-threaded servers and avoids partial line corruption.

### Implication

- A mutex is used internally.
- The mutex duration is minimized by building the line outside the lock when possible.

---

## I/O behavior and constraints (important)

### Best-effort I/O

`stdout` and `stderr` are inherently environment-dependent:

- In TTY: typically buffered lightly
- In pipes / Docker / CI / journald: may be buffered differently and can block
- In overloaded collectors: can become slow

Vix Console follows a **best-effort** approach:

- avoids aggressive flushing by default
- writes one line at a time to reduce interleaving
- remains no-throw and never crashes an app intentionally

### Flush policy

Flushing after every line can create a severe performance cliff.

Default policy:

- **No flush per line** (to avoid synchronous penalties)
- Optional flush may be applied for `error` only (implementation-dependent) to increase visibility before crashes

---

## Ordering guarantees

- Ordering is guaranteed **within the same stream**:
  - `stdout` lines preserve order relative to other `stdout` lines
  - `stderr` lines preserve order relative to other `stderr` lines
- Ordering between `stdout` and `stderr` is **not guaranteed** due to independent buffering and collectors.

If strict ordering is required, route everything to one stream (or use a production logger with ordering semantics).

---

## Anti-spam protection (dev-proof)

### Why it exists

Many developers coming from Node.js/Python/PHP will naturally write:

```cpp
using namespace vix;
while (...) {
  console.log("tick");
}
```

In C++ server workloads, this can destroy throughput.

### Policy

Vix Console may apply a minimal **rate limit** for `log/info` when volume becomes abnormal.

- Affected: `log`, `info`
- Not affected: `warn`, `error` (never suppressed)
- Behavior when active:
  - excessive `log/info` lines are suppressed
  - at most **once per second**, Vix Console prints a summary line:

Example:

```
HH:MM:SS [warn] (console) suppressed 153 log/info lines (rate limit)
```

This preserves performance and signals misuse without turning console into a full logger.

---

## Reliability notes (crash/shutdown)

- Vix Console is **no-throw** (it must never throw exceptions).
- In a hard crash (SIGKILL, power loss, abrupt termination), some output can be lost.
- `error` lines may be more likely to be visible depending on the environment and flush policy.

---

## Recommended usage patterns

### Good

- quick startup messages
- “what’s happening” output in dev mode
- warnings about deprecated APIs or unexpected states
- error reporting when a subsystem fails

### Avoid

- high-frequency logs in hot loops
- using console as an audit trail
- using console for production observability pipelines

If your needs include retention, filtering, sampling, structured fields, or ingestion, switch to `vix::utils::Logger`.
If you find yourself wanting tables, counters, timers, or grouped output,
this is a signal to switch from `vix::console` to `vix::utils::Logger`.

---

## Integration

### Include

Use:

```cpp
#include <vix/console.hpp>
```

This exposes the global singleton:

```cpp
vix::console
```

### Umbrella headers

Vix may also include console in higher-level umbrella headers (e.g., `core.hpp` / `vix.hpp`) to make it available by default in Vix applications.

---

## FAQ

### Is Vix Console asynchronous?

No. Console output is fundamentally I/O-bound and depends on the output sink.
Vix Console minimizes overhead and avoids flush cliffs, but it does not attempt to make `stdout/stderr` non-blocking “by magic”.

### Why not auto-enable debug by default?

Because it would be a silent performance hazard in production-like workloads.
The default (`info`) preserves developer signal while protecting runtime performance.

### Why not structured JSON by default?

Because console is meant to be minimal and stable across environments.
Production logging and structured pipelines belong to `vix::utils::Logger`.

---

## Summary

Vix Console provides a **safe, stable, zero-config**, Node-like console designed for runtime/dev usage:

- trusted defaults
- filtered == near-free
- thread-safe atomic lines
- environment config read once
- minimal anti-spam guardrails

For production logging, use `vix::utils::Logger`.



