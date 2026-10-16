# WASI Preview 2 integration status

This document supersedes the earlier WASI section in `PLATFORM_VALIDATION.md`
where it says wasi-libc sockets or the P2 SDK integration were absent. The
WASI Preview 2 selects a private wasi-libc socket/poll provider and requires
the public cooperative progress mode. Source implementation is present but
remains uncompiled and unverified until the pinned SDK and host tests run.

## Pins and provenance

Initial reproducible target:

* Target ABI: `wasm32-wasip2` (WASI Preview 2 Component Model).
* SDK: official wasi-sdk 34.0. The SDK release includes the P2 CMake toolchain
  at `share/cmake/wasi-sdk-p2.cmake` and a wasi-libc sysroot with P2 support.
* Linux x86_64 artifact:
  [official wasi-sdk 34 release](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-34),
  `wasi-sdk-34.0-x86_64-linux.tar.gz`; SHA-256
  `b761e3a0721dbae9c09a0059e5fdb2bf917d1b4a8a7b430fb3b5aafb0984b2c4`.
* Host: Wasmtime 49.0.3, [official immutable release](https://github.com/bytecodealliance/wasmtime/releases/tag/v49.0.3).
  The release page exposed no SHA-256 for the x86_64 Linux CLI asset in this
  audit. Before mirroring, verify its official release attestation/checksum.
* Component interfaces: WASI specification 0.2.12, including
  `wasi:sockets@0.2.12`, `wasi:io`, and clock interfaces. wasi-sdk's own
  sysroot/bindings and component linker are authoritative for the actual
  imports; Vix does not add WIT bindings or a second component toolchain.

This is not Preview 1 and not Preview 3. Browser WebAssembly remains outside
the raw TCP support contract.

## libc socket surface

The official wasi-libc source inventory includes Preview 2 implementations
for `socket`, `bind`, `connect`, `listen`, `accept`, `send`, `recv`,
`shutdown`, local/peer address lookup and `poll-wasip2.c`. The P2 `poll`
implementation bridges libc TCP descriptors to WASI socket pollables. This
provides a plausible private POSIX-compatible socket/readiness backend for
numeric IPv4/IPv6, connect/listen/accept, data transfer, half-shutdown, close,
and deadline polling. The C probe under `tests/wasi/` is specifically an ABI
probe; it does not prove the Vix Network API works.

Wasmtime exposes P2 TCP only with host permission. TCP must be enabled and the
host address policy must permit the requested address. Its documented context
defaults deny all addresses; a denied address must remain a meaningful failure
in the guest, never a simulated successful connection.

## Execution-model status after the cooperative progress contract

The public provider now has an explicit cooperative mode:

* `Provider::create_cooperative()` starts neither the background reactor
  thread nor the continuation worker pool.
* `Provider::poll()` performs a nonblocking progress turn, including native
  readiness, cancellation facts, deadlines, terminal completion, then queued
  coroutine resumption.
* The caller must keep driving the provider while operations are outstanding;
  cancellation and deadlines are observed on a subsequent progress turn.

The provider uses wasi-libc's Preview 2 socket descriptors and `poll()` bridge;
no direct WIT bindings or guest background threads are introduced. Its reactor
does only zero-timeout readiness polling, while `Provider::poll()` owns queued
commands, cancellation facts, deadlines, terminal arbitration and coroutine
resumption. `Provider::create()` returns an explicit unsupported-capability
error on WASI; callers must use `Provider::create_cooperative()`.

The implementation now needs validation against wasi-sdk 34 and Wasmtime 49:
nonblocking connect/accept/read/write, descriptor retirement, host-denied
capabilities, and deadline/cancellation arbitration remain runtime acceptance
criteria, not verified claims.

For domain CMake tests, set `VIX_WASMTIME_EXECUTABLE` to the pinned Wasmtime
49.0.3 binary. The registered public-consumer and denial tests do not grant
network access; the loopback test grants both TCP and inherited-network
capabilities explicitly. Missing host permissions fail the test rather than
being reported as a skip.

Domain validation entrypoint (not executed in this implementation phase):

```sh
cmake -S domains/network -B out/network-wasi-p2 \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/domains/network/wasi-sdk-p2-toolchain.cmake" \
  -DVIX_WASMTIME_EXECUTABLE=/opt/wasmtime-v49.0.3-x86_64-linux/wasmtime
cmake --build out/network-wasi-p2 -j2
ctest --test-dir out/network-wasi-p2 --output-on-failure
```

For the combined installed Standard/Files/Process/Network SDK test gate, use
the repository root and pass the same runtime path. The aggregate consumer
test installs the SDK, configures a separate consumer with the same toolchain,
builds it, and executes that consumer under Wasmtime:

```sh
cmake -S . -B out/vix-wasi-p2 \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/domains/network/wasi-sdk-p2-toolchain.cmake" \
  -DVIX_WASI_SDK_ROOT=/opt/wasi-sdk-34.0-x86_64-linux \
  -DVIX_WASMTIME_EXECUTABLE=/opt/wasmtime-v49.0.3-x86_64-linux/wasmtime \
  -DVIX_BUILD_TESTS=ON
cmake --build out/vix-wasi-p2 -j2
ctest --test-dir out/vix-wasi-p2 --output-on-failure
```

## Domain-local SDK and libc probe

The wrapper delegates to the exact upstream P2 toolchain and checks that the
SDK path is explicitly supplied:

```sh
export VIX_WASI_SDK_ROOT=/opt/wasi-sdk-34.0-x86_64-linux
cmake -S domains/network/tests/wasi -B out/network-wasi-p2 \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/domains/network/wasi-sdk-p2-toolchain.cmake" \
  -DWASMTIME_EXECUTABLE=/opt/wasmtime-v49.0.3-x86_64-linux/wasmtime
cmake --build out/network-wasi-p2 -j2
ctest --test-dir out/network-wasi-p2 --output-on-failure
```

The `denied` case uses `-W tcp=y` without granting addresses and expects
`EACCES`/`EPERM`. The `loopback` case uses `-W tcp=y -W inherit-network=y`
and checks bind/listen/connect/accept, send/receive, `poll`, half-close/EOF,
and close. Runtime flags are deliberately explicit because host policy is
part of the test. These probe commands are prepared for human execution and
were not run here.

## Provider acceptance still required

Before CMake selects WASI for `vix::network`, implement and test a genuine
single-thread-compatible operation engine or a supported threading strategy.
Then add public consumer and provider tests for permission denial, refusal,
multiple operations, cancellation/deadline arbitration, exactly-once
completion, resource destruction, EOF and shutdown. Distinguish separately:
SDK compilation, component instantiation, host-permitted TCP execution, and
Network API conformance. None is verified by the libc probe alone.
