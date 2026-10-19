# Experimental target SDK archive qualification

Target SDK archives are separate from host CLI distributions. They contain an
installed, relocatable Vix CMake SDK for one target; they do not contain the
compiler SDK or host runtime needed to build or execute that target.

The release-qualification workflow produces candidate archives for NDK ABI
`arm64-v8a` or `x86_64` at API 21 or newer. They are not an advertised or
publicly available release until the protected signed-release workflow is
explicitly enabled and the target-component installer contract is approved.
The release component identifiers are `android-arm64-v8a-api21` and
`android-x86_64-api21` respectively.
Install the matching Android NDK separately, then configure a
consumer with the NDK's `android.toolchain.cmake`, matching `ANDROID_ABI`,
`ANDROID_PLATFORM=android-21`, and `CMAKE_PREFIX_PATH` pointing to the extracted
Vix target SDK. Cross-compilation is qualified separately from Android
emulator/device execution; the latter is not claimed by this archive.

The WASI candidate is only for `wasm32-wasip2` (WASI Preview 2). Install wasi-sdk
34.0 separately, use `domains/network/wasi-sdk-p2-toolchain.cmake` or the
upstream P2 toolchain, and set `VIX_WASI_SDK_ROOT`. The Vix Network provider is
cooperative: create it with `Provider::create_cooperative()` and drive
`Provider::poll()`. Socket access also depends on Wasmtime host permissions.
This is not WASI Preview 1 or browser WebAssembly.

For a qualification artifact, extract the versioned
`vix-sdk-<target>-<version>.tar.gz` into a dedicated directory. For example:

```sh
target=wasm32-wasip2
version=2.9.0
prefix="$HOME/.local/opt/vix-sdk/$target/$version"
mkdir -p "$prefix"
tar -xzf "vix-sdk-$target-$version.tar.gz" -C "$prefix"
cmake -S . -B build-wasi \
  -DCMAKE_TOOLCHAIN_FILE="$VIX_WASI_SDK_ROOT/share/wasi-sysroot/../cmake/wasi-sdk.cmake" \
  -DCMAKE_PREFIX_PATH="$prefix"
```

For Android, select the matching NDK `android.toolchain.cmake`, `ANDROID_ABI`,
and `ANDROID_PLATFORM=android-21`; set `CMAKE_PREFIX_PATH` to that SDK's
extracted prefix. The archive's `share/vix/target-sdk.toml` records its target,
version, and external toolchain prerequisite. Do not overlay different targets
into one prefix; keep each target SDK in its own directory.

The distribution runtime can install one explicitly selected, signed target
SDK into the managed Vix home under `target-sdks/<target>/<version>`. Each
target/version is immutable; a new version is installed side-by-side, so an
older SDK remains available for rollback. A same-version different digest or
component identity is rejected. It does not add a `vix` CLI command; no public
target-installation command is approved in this release slice. `vix upgrade`
remains host-only and `vix install` remains project-only.

Current clients verify the whole signed manifest, select only the exact host
artifact for host upgrades, and leave target components untouched. Older
clients that do not understand the component format fail closed rather than
silently skipping component metadata.

These archives are target SDK assets, not native `vix` executables. Their
presence does not imply that the Vix CLI runs on Android or WASI, nor that
Android runtime behavior or unsupported WASI capabilities are qualified.
