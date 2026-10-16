# Network platform validation

This document records source-level provider selection and the verification
needed before any platform is called supported. Source selection is not a
verification result.

## Android / Termux

The Android NDK selects the Linux-family provider, using Bionic's socket APIs
and the kernel `epoll`, `eventfd`, and `accept4` interfaces. The implementation
uses no GNU-libc-only API. The minimum supported NDK platform is Android API
21, and the configured ABI must be `arm64-v8a` or `x86_64`. CMake rejects a
lower API level or another ABI instead of implying support.

Cross-compile the standalone Network project with the NDK:

```sh
cmake -S domains/network -B out/network-android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DVIX_BUILD_TESTS=ON
cmake --build out/network-android-arm64 --target \
  vix_network_public_consumer_test vix_network_transport_contract_test \
  vix_network_continuations_lifecycle_test -j2
```

Repeat with `-B out/network-android-x86_64 -DANDROID_ABI=x86_64`. Cross-built
CTest executables are not host-runnable. Run them on an Android emulator or
device (or from a native Termux build), for example after deploying the three
executables to `/data/local/tmp/vix-network`:

```sh
adb shell mkdir -p /data/local/tmp/vix-network
adb push out/network-android-arm64/tests/vix_network_public_consumer_test \
  out/network-android-arm64/tests/vix_network_transport_contract_test \
  out/network-android-arm64/tests/vix_network_continuations_lifecycle_test \
  /data/local/tmp/vix-network/
adb shell chmod 755 /data/local/tmp/vix-network/vix_network_public_consumer_test \
  /data/local/tmp/vix-network/vix_network_transport_contract_test \
  /data/local/tmp/vix-network/vix_network_continuations_lifecycle_test
adb shell /data/local/tmp/vix-network/vix_network_public_consumer_test
adb shell /data/local/tmp/vix-network/vix_network_transport_contract_test
adb shell /data/local/tmp/vix-network/vix_network_continuations_lifecycle_test concurrent-startup
adb shell /data/local/tmp/vix-network/vix_network_continuations_lifecycle_test drain-on-process-exit
```

For native Termux validation, configure and run the tests on-device rather than
cross-running a host CTest binary:

```sh
cmake -S "$VIX_SOURCE/domains/network" -B "$PREFIX/tmp/vix-network" -DVIX_BUILD_TESTS=ON
cmake --build "$PREFIX/tmp/vix-network" -j2
ctest --test-dir "$PREFIX/tmp/vix-network" --output-on-failure
```

Run the `--ipv6-only` transport test separately where the device enables IPv6
loopback. A capability-specific skip is not evidence that IPv6 passed. Repeat
runtime tests on both ABIs; a cross-compile alone does not establish Bionic,
emulator, or Termux runtime behavior.

## WASI and browser WebAssembly

The source-level wasi-libc Preview 2 provider and cooperative poll adapter are
now described in [`WASI_P2_INTEGRATION.md`](WASI_P2_INTEGRATION.md). They are
selected only for the pinned `wasm32-wasip2` target and are not yet compiled or
runtime verified. Emscripten and Preview 1 remain unsupported.

WASI Preview 1, WASI 0.2 Component Model interfaces, and emerging WASI 0.3
interfaces must not be treated as interchangeable. Browser WebAssembly
remains unsupported for arbitrary raw TCP.
consumer and host-runtime integration tests. Until then WASI is unsupported,
not a provider that compiled or a capability inferred from a CMake target.

## Required verification matrix

| Platform                     | Build evidence                                                     | Runtime evidence required                                                                             |
| ---------------------------- | ------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------- |
| Linux x86_64, glibc          | Native build and public/transport/lifecycle CTest                  | Native loopback suite                                                                                 |
| Linux ARM64, glibc           | Native ARM64 build, or cross-build plus ARM64 runner               | Native ARM64 loopback suite; cross-build alone is insufficient                                        |
| Linux x86_64 and ARM64, musl | Build with musl toolchain for each architecture                    | Execute in matching musl environment                                                                  |
| Android `arm64-v8a`, API 21+ | NDK cross-build of library and registered consumer/transport tests | Emulator/device runtime plus Termux validation                                                        |
| Android `x86_64`, API 21+    | NDK cross-build of library and registered consumer/transport tests | x86_64 emulator/device runtime                                                                        |
| macOS x86_64                 | Native build and all registered Network tests                      | Native TCP, cancellation, deadline, shutdown suite                                                    |
| macOS Apple Silicon          | Native ARM64 build and all registered Network tests                | Native TCP and lifecycle suite                                                                        |
| Windows x86_64, MSVC         | Native build and all registered Network tests                      | Native Winsock transport, race, wakeup, and lifecycle suite                                           |
| Windows ARM64, MSVC          | Native ARM64 build and all registered Network tests                | Native ARM64 Winsock runtime suite                                                                    |
| WASI                         | Not currently configured                                           | No verification until a concrete ABI, binding pipeline, and host runtime are selected and implemented |
| Browser WebAssembly          | Unsupported                                                        | No raw-TCP claim or verification                                                                      |

For every native CI entry, run the exported public-consumer test and all
provider-appropriate transport/lifecycle tests. Keep IPv6 capability checks
separate and report skips distinctly. Cross-compilation, emulation, and native
execution must be reported as different evidence classes.
