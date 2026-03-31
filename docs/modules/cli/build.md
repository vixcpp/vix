# vix build

Configure and build a Vix project using embedded CMake presets.

---

## Usage

```bash
vix build [options] -- [cmake args...]
```

---

## Description

`vix build` wraps CMake + Ninja with:

- Embedded presets
- Strong signature cache (tool versions + CMake file hashes)
- Optional fast no-op exit
- Auto sccache / ccache detection
- Auto mold / lld detection
- Clean log separation

Designed for ultra-fast iteration loops.

---

# Presets

```
dev        -> Ninja + Debug   (build-dev)
dev-ninja  -> Ninja + Debug   (build-dev-ninja)
release    -> Ninja + Release (build-release)
```

Default preset: `dev`

---

# Options

```
--preset <name>       dev | dev-ninja | release
--target <triple>     Cross-compilation target triple
--sysroot <path>      Sysroot for cross toolchain
--static              Enable VIX_LINK_STATIC=ON
-j, --jobs <n>        Parallel jobs (default: CPU count)
--clean               Force reconfigure (ignore signature cache)
--no-cache            Disable signature cache
--fast                Exit immediately if up-to-date (Ninja dry-run)
--linker <mode>       auto | default | mold | lld
--launcher <mode>     auto | none | sccache | ccache
--no-status           Disable NINJA_STATUS progress formatting
--no-up-to-date       Disable Ninja dry-run check
-d, --dir <path>      Project directory
-q, --quiet           Minimal output
--targets             List detected cross toolchains
--cmake-verbose       Show full CMake output
--build-target <name> Build specific CMake target
-h, --help            Show help
```

---

# Environment Variables

```
VIX_BUILD_HEARTBEAT=1
```

Enables heartbeat output when build is silent for several seconds.

---

# Examples

Basic build:

```bash
vix build
```

Fast loop:

```bash
vix build --fast
```

Release:

```bash
vix build --preset release
```

Static release:

```bash
vix build --preset release --static
```

With launcher + linker:

```bash
vix build --launcher sccache --linker mold
```

Cross-compile:

```bash
vix build --target aarch64-linux-gnu
```

Pass raw CMake arguments:

```bash
vix build -- -DVIX_SYNC_BUILD_TESTS=ON
```

Parallel jobs:

```bash
vix build -j 8
```

---

# Logs

Each preset has its own directory:

```
build-dev*/configure.log
build-dev*/build.log
```

Logs are always written even if console output is minimal.

---

# Signature Cache

Vix calculates a strong signature based on:

- Compiler version
- CMake version
- Linker version
- Relevant CMake file hashes

If nothing changed, configure is skipped.

`--clean` bypasses this cache.

---

# Fast Mode

```
vix build --fast
```

If Ninja reports the project is already up-to-date, Vix exits immediately without full build processing.

Optimized for tight dev loops.

---

# Design Goals

- Deterministic builds
- Fast iteration
- Clean output
- Explicit configuration
- Cross-compilation ready
- Secure and reproducible artifacts

---

`vix build` is the foundation of the Vix development workflow.

