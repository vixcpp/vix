# Current-generation host distributions

The current release-qualification pipeline assembles the Linux x86_64 GNU
candidate as `vix-x86_64-linux-gnu-<version>.tar.gz` and a separate bootstrap
archive as `vix-installer-x86_64-linux-gnu-<version>.tar.gz`. The first archive
contains the Vix CLI and relocatable canonical CMake SDK. The bootstrap archive
is only for the separately managed host-installation flow; it is not required
for manual extraction.

For a qualified Linux x86_64 candidate archive, install it into a dedicated
versioned directory and use its bundled executable:

```sh
version=2.9.0
prefix="$HOME/.local/opt/vix/$version"
mkdir -p "$prefix"
tar -xzf "vix-x86_64-linux-gnu-$version.tar.gz" -C "$prefix"
"$prefix/bin/vix" --version
export PATH="$prefix/bin:$PATH"
```

An external CMake project can consume the SDK without referring to the Vix
source or build trees:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="$prefix"
cmake --build build
```

The exact archive/runtime dependency list and external consumer are qualified
by the release workflow. Do not overlay two versions or replace files owned by
an OS package manager. Package-manager installations remain owned by that
manager; `vix upgrade` does not replace them. On platforms where a managed
upgrade path is unsupported, install a newer archive side-by-side and update
the caller-controlled PATH or package-manager installation.

Current worktree qualification produces only the Linux x86_64 GNU host archive.
Linux ARM64 GNU, macOS x86_64/ARM64, and Windows x86_64/MSVC do not yet have
release archives qualified by this pipeline. Windows managed self-upgrade is
unsupported. No native Android or WASI CLI host is implied by target SDK
artifacts.
