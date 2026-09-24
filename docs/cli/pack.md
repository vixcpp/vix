# vix pack

`vix pack` packages a Vix project into a distributable folder and, on Unix-like systems, optionally creates a `.vixpkg` archive.

Use it when you want to prepare a project for sharing, verification, caching, release workflows, or deployment pipelines.

```bash
vix pack
```

## Overview

`vix pack` creates a package under:

```txt
dist/<name>@<version>
```

On Linux and macOS, it can also create:

```txt
dist/<name>@<version>.vixpkg
```

The generated package uses the Vix manifest v2 format:

```txt
manifest.json
```

The package can include:

```txt
include/
src/
lib/
modules/
tests/
meta/
README.md
CMakeLists.txt
CMakePresets.json
vix.toml
LICENSE
checksums.sha256
manifest.json
```

## Usage

```bash
vix pack [options]
```

## Basic examples

```bash
# Package the current project
vix pack

# Package with explicit name and version
vix pack --name blog --version 1.0.0

# Package another project directory
vix pack --dir ./my-lib

# Write output to another directory
vix pack --out ./artifacts

# Print copied files and signing details
vix pack --verbose

# Create only the folder package
vix pack --no-zip

# Skip checksums.sha256
vix pack --no-hash

# Disable signing
vix pack --sign=never

# Require signing
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign=required
```

## Project requirement

`vix pack` currently requires a project with:

```txt
CMakeLists.txt
```

If no `CMakeLists.txt` exists, Vix reports:

```txt
No CMakeLists.txt found in: <project>
Run from a Vix project folder or use: vix pack --dir <path>
```

This means `vix pack` currently packages CMake-backed Vix projects.

## Output layout

Default output directory:

```txt
<project>/dist
```

Default package folder:

```txt
<project>/dist/<name>@<version>
```

Default archive on Linux/macOS:

```txt
<project>/dist/<name>@<version>.vixpkg
```

Example:

```bash
vix pack --name blog --version 1.0.0
```

Output:

```txt
dist/
├── blog@1.0.0/
│   ├── include/
│   ├── src/
│   ├── meta/
│   ├── CMakeLists.txt
│   ├── manifest.json
│   └── checksums.sha256
└── blog@1.0.0.vixpkg
```

## Default name and version

If no name is passed, Vix uses the project folder name.

Example:

```txt
/home/user/projects/blog
```

Default package name:

```txt
blog
```

If no version is passed, Vix uses:

```txt
0.1.0
```

So:

```bash
vix pack
```

can create:

```txt
dist/blog@0.1.0
```

## Custom output directory

Use:

```bash
vix pack --out ./artifacts
```

Example:

```bash
vix pack --out ./artifacts --name blog --version 1.0.0
```

Output:

```txt
artifacts/blog@1.0.0
artifacts/blog@1.0.0.vixpkg
```

## Custom project directory

Use:

```bash
vix pack --dir ./my-lib
```

or:

```bash
vix pack -d ./my-lib
```

This packages another project without changing your current shell directory.

## What gets copied

`vix pack` copies these directories when they exist:

```txt
include/
src/
lib/
modules/
tests/
```

It also copies these root files when they exist:

```txt
README.md
CMakeLists.txt
CMakePresets.json
vix.toml
LICENSE
```

`README.md` is copied into:

```txt
meta/README.md
```

`CMakeLists.txt`, `CMakePresets.json`, `vix.toml`, and `LICENSE` are copied to the package root.

## Verbose copy output

Use:

```bash
vix pack --verbose
```

This prints copied files and signing output when signing is used.

Example output shape:

```txt
vix pack
Project:
  /home/user/blog
Output:
  /home/user/blog/dist
Packaging:
  blog@1.0.0
copied: include/blog.hpp
copied: src/blog.cpp
copied: CMakeLists.txt
```

## Existing package folder

If the target package folder already exists, Vix removes it before creating the new package.

Example:

```txt
dist/blog@1.0.0
```

is removed and recreated when you run:

```bash
vix pack --name blog --version 1.0.0
```

## Manifest v2

`vix pack` writes:

```txt
manifest.json
```

The manifest schema is:

```txt
vix.manifest.v2
```

The manifest contains package metadata, ABI info, exports, dependencies, toolchain info, layout information, payload digest, signature metadata, and checksums.

Example shape:

```json
{
  "schema": "vix.manifest.v2",
  "package": {
    "name": "blog",
    "version": "1.0.0",
    "kind": "package",
    "license": "MIT"
  },
  "abi": {
    "os": "linux",
    "arch": "x86_64"
  },
  "exports": ["blog"],
  "dependencies": {},
  "toolchain": {
    "cxx": {
      "path": "c++",
      "version": "c++ (Ubuntu 13.3.0) ...",
      "standard": "c++23"
    },
    "cmake": {
      "version": "cmake version ...",
      "generator": "presets"
    }
  },
  "layout": {
    "include": true,
    "src": true,
    "lib": false,
    "modules": false,
    "readme": true
  }
}
```

## Metadata from `vix.toml`

If the project has:

```txt
vix.toml
```

`vix pack` reads package metadata from it.

Supported sections include:

```txt
[package]
[exports]
[dependencies]
[toolchain]
```

## `[package]`

Example:

```toml
[package]
name = "blog"
version = "1.0.0"
kind = "library"
license = "MIT"
```

Supported fields:

| Field     | Purpose          |
| --------- | ---------------- |
| `name`    | Package name.    |
| `version` | Package version. |
| `kind`    | Package kind.    |
| `license` | Package license. |

CLI options can still provide name and version defaults, but `vix.toml` is used by the manifest writer when present.

## `[exports]`

Example:

```toml
[exports]
items = ["blog", "blog/http"]
```

If no exports are defined, Vix uses the package name as the default export.

## `[dependencies]`

Example:

```toml
[dependencies]
softadastra.core = "^1.0.0"
```

Dependencies are written into `manifest.json`.

## `[toolchain]`

Example:

```toml
[toolchain]
cxx_standard = "c++23"
cmake_generator = "Ninja"
```

If no C++ standard is provided, Vix uses:

```txt
c++23
```

If `CMakePresets.json` exists and no generator is set, Vix can mark the generator as:

```txt
presets
```

## ABI metadata

The manifest includes ABI information.

OS values include:

```txt
linux
macos
windows
```

Architecture values include:

```txt
x86_64
arm64
x86
unknown
```

Example:

```json
{
  "abi": {
    "os": "linux",
    "arch": "x86_64"
  }
}
```

## Payload digest

On Linux and macOS, `vix pack` creates:

```txt
meta/payload.digest
```

The payload digest is computed from a stable SHA256 listing of package files.

These files are excluded from the payload digest to avoid self-referential hashes:

```txt
manifest.json
checksums.sha256
meta/payload.digest
meta/payload.digest.minisig
```

The digest is also recorded in `manifest.json`.

Example:

```json
{
  "payload": {
    "digest_algorithm": "sha256",
    "digest": "...",
    "digest_available": true,
    "excludes": [
      "manifest.json",
      "checksums.sha256",
      "meta/payload.digest",
      "meta/payload.digest.minisig"
    ]
  }
}
```

## Checksums

On Linux and macOS, `vix pack` writes:

```txt
checksums.sha256
```

unless you pass:

```bash
vix pack --no-hash
```

The checksum file excludes:

```txt
checksums.sha256
manifest.json
```

This avoids self-referential checksum problems.

## Disable checksums

Use:

```bash
vix pack --no-hash
```

This skips `checksums.sha256`.

The package still gets `manifest.json`.

## Archive creation

On Linux and macOS, Vix tries to create:

```txt
dist/<name>@<version>.vixpkg
```

using the `zip` command.

If `zip` is missing or fails, Vix keeps the folder package and prints a hint.

Example:

```txt
zip tool not available (or zip failed). Keeping folder package instead.
You can install zip, or use: vix pack --no-zip
```

## Folder-only package

Use:

```bash
vix pack --no-zip
```

This creates only:

```txt
dist/<name>@<version>
```

No `.vixpkg` archive is created.

## Windows behavior

On Windows, `vix pack` currently creates the folder package only.

It writes:

```txt
manifest.json
```

but does not create the `.vixpkg` archive in the current implementation.

Example output:

```txt
Package folder created:
dist/blog@1.0.0
```

## Signing

Signing is optional.

On Linux and macOS, `vix pack` can sign:

```txt
meta/payload.digest
```

with `minisign`.

The signature file is:

```txt
meta/payload.digest.minisig
```

The manifest records signature metadata but does not embed the signature content.

## Signing modes

`--sign` supports:

```txt
auto
never
required
```

| Mode       | Behavior                                                                         |
| ---------- | -------------------------------------------------------------------------------- |
| `auto`     | Sign if `minisign` and a signing key are available. Never blocks for passphrase. |
| `never`    | Do not sign.                                                                     |
| `required` | Signing must succeed or packaging fails.                                         |

Default:

```txt
auto
```

## `--sign`

Passing:

```bash
vix pack --sign
```

is the same as required signing.

It requires `minisign` and a signing key.

## `--sign=auto`

Use:

```bash
vix pack --sign=auto
```

In auto mode, Vix signs only when possible.

Auto mode does not block for an interactive passphrase.

If signing needs a passphrase, signing is skipped in auto mode.

With `--verbose`, Vix may print:

```txt
Signing skipped (auto): needs passphrase or minisign failed.
```

## `--sign=never`

Use:

```bash
vix pack --sign=never
```

This disables signing.

The manifest records signature as disabled.

## `--sign=required`

Use:

```bash
vix pack --sign=required
```

This requires signing.

If `minisign` is missing or no key is available, Vix fails.

Example error shape:

```txt
pack: signing required but unavailable: minisign not found
Install minisign and/or set VIX_MINISIGN_SECKEY=/path/to/key
```

## Signing key

Set the signing key with:

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign=required
```

Environment variable:

```txt
VIX_MINISIGN_SECKEY
```

On Unix-like systems, auto mode can also look for default keys:

```txt
~/.config/vix/keys/vix-pack.key
~/keys/vix/vix-pack.key
```

## Required signing output

Example:

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign=required
```

Output shape:

```txt
Signing:
  mode: required
  tool: minisign (ed25519)
  key: ./keys/vix-pack.key
  file: meta/payload.digest
  minisign may prompt for the private key passphrase.
```

If signing succeeds, the package includes:

```txt
meta/payload.digest.minisig
```

## Manifest signature section

Example:

```json
{
  "signature": {
    "enabled": true,
    "algorithm": "ed25519",
    "tool": "minisign",
    "signed": "meta/payload.digest",
    "file": "meta/payload.digest.minisig"
  }
}
```

If signing is disabled or skipped:

```json
{
  "signature": {
    "enabled": false,
    "algorithm": "ed25519",
    "tool": "minisign",
    "signed": "meta/payload.digest",
    "file": "meta/payload.digest.minisig"
  }
}
```

## Required tools on Unix

For full Unix packaging behavior, these tools are useful:

```txt
sha256sum
zip
minisign
```

Required for checksums:

```txt
sha256sum
```

Required for `.vixpkg` archive:

```txt
zip
```

Required for signing:

```txt
minisign
```

If `zip` is missing, Vix can still create the folder package.

If `minisign` is missing in auto mode, Vix can still package without signing.

If `minisign` is missing in required mode, Vix fails.

## Recommended release flow

```bash
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
```

If an archive was created:

```bash
vix verify --path ./dist/blog@1.0.0.vixpkg
```

## Release flow with signing

```bash
vix build --preset release
vix check --tests
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --name blog --version 1.0.0 --sign=required
vix verify --path ./dist/blog@1.0.0
```

## Package folder flow

```bash
vix pack --no-zip
vix verify --path ./dist/blog@0.1.0
```

Use this when you want an unpacked package folder.

## Package archive flow

```bash
vix pack
vix verify --path ./dist/blog@0.1.0.vixpkg
```

Use this when you want a single distributable `.vixpkg` file.

## Options

| Option             | Description                                          |
| ------------------ | ---------------------------------------------------- |
| `-d, --dir <path>` | Project directory. Default: current directory.       |
| `--out <path>`     | Output directory. Default: `<project>/dist`.         |
| `--name <name>`    | Package name. Default: project folder name.          |
| `--version <ver>`  | Package version. Default: `0.1.0`.                   |
| `--no-zip`         | Do not create `.vixpkg`; create folder package only. |
| `--no-hash`        | Do not generate `checksums.sha256`.                  |
| `--verbose`        | Show copied files and minisign output when used.     |
| `--sign`           | Require signing. Same as `--sign=required`.          |
| `--sign=auto`      | Sign when possible without blocking. Default mode.   |
| `--sign=never`     | Disable signing.                                     |
| `--sign=required`  | Fail if signing cannot be completed.                 |
| `-h, --help`       | Show command help.                                   |

## Environment variables

| Variable              | Description                                             |
| --------------------- | ------------------------------------------------------- |
| `VIX_MINISIGN_SECKEY` | Secret key path used to sign `meta/payload.digest`.     |
| `CXX`                 | Compiler path recorded in manifest toolchain metadata.  |
| `CMAKE_GENERATOR`     | CMake generator recorded in manifest metadata when set. |

## Commands reference

| Command                                | Description                                            |
| -------------------------------------- | ------------------------------------------------------ |
| `vix pack`                             | Package current project with default name and version. |
| `vix pack --name blog --version 1.0.0` | Package as `blog@1.0.0`.                               |
| `vix pack --dir ./my-lib`              | Package another project directory.                     |
| `vix pack --out ./artifacts`           | Write package output to `./artifacts`.                 |
| `vix pack --no-zip`                    | Create folder package only.                            |
| `vix pack --no-hash`                   | Skip `checksums.sha256`.                               |
| `vix pack --verbose`                   | Show copied files and signing details.                 |
| `vix pack --sign=never`                | Package without signing.                               |
| `vix pack --sign=required`             | Require minisign signing.                              |

## Common workflows

### Package current project

```bash
vix pack
```

### Package with explicit name and version

```bash
vix pack --name blog --version 1.0.0
```

### Package to artifacts folder

```bash
vix pack --out ./artifacts --name blog --version 1.0.0
```

### Package another project

```bash
vix pack --dir ../my-lib --name my-lib --version 1.0.0
```

### Package folder only

```bash
vix pack --no-zip
```

### Package without signing

```bash
vix pack --sign=never
```

### Package with required signing

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign=required
```

### Package and verify

```bash
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
```

## Common mistakes

### Running outside a CMake-backed project

Wrong:

```bash
cd /tmp
vix pack
```

Correct:

```bash
cd /path/to/project
vix pack
```

The project must contain:

```txt
CMakeLists.txt
```

### Packing before building

For release workflows, build first:

```bash
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
```

### Forgetting to verify

After packing, verify the output:

```bash
vix verify --path ./dist/blog@1.0.0
```

or:

```bash
vix verify --path ./dist/blog@1.0.0.vixpkg
```

### Requiring signing without a key

Wrong:

```bash
vix pack --sign=required
```

when no key is configured.

Correct:

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign=required
```

### Expecting auto signing to prompt for a passphrase

Auto mode does not block for a passphrase.

Use required signing when you want interactive signing:

```bash
vix pack --sign=required
```

### Expecting `.vixpkg` without `zip`

On Linux/macOS, `.vixpkg` creation needs `zip`.

If `zip` is missing, Vix keeps the folder package.

Install `zip` or use:

```bash
vix pack --no-zip
```

### Expecting `.vixpkg` on Windows

The current Windows path creates a folder package only.

Use the generated folder:

```txt
dist/<name>@<version>
```

### Expecting `--no-hash` to remove manifest integrity metadata

`--no-hash` skips `checksums.sha256`.

The manifest is still generated.

The payload digest can still exist on Unix-like systems.

## Troubleshooting

### No CMakeLists.txt found

Run from the project root or pass `--dir`:

```bash
vix pack --dir /path/to/project
```

### Unable to create output directory

Check permissions for the output path:

```bash
vix pack --out ./artifacts
```

### Failed to copy file

Check file permissions and whether the destination is writable.

Retry with:

```bash
vix pack --verbose
```

to see which file was being copied.

### Zip tool not available

Install `zip` or use:

```bash
vix pack --no-zip
```

### Signing required but minisign not found

Install `minisign`, then retry:

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign=required
```

### Signing required but no key found

Set:

```bash
VIX_MINISIGN_SECKEY=/path/to/key
```

Then run:

```bash
vix pack --sign=required
```

### Minisign failed

Check that the key path is correct and that you entered the correct passphrase.

Use:

```bash
vix pack --sign=required --verbose
```

### checksums.sha256 missing

If you passed:

```bash
vix pack --no-hash
```

then `checksums.sha256` is intentionally skipped.

If you did not pass `--no-hash`, make sure `sha256sum` is available on Unix-like systems.

## Best practices

Build before packing release artifacts.

Run tests before packing.

Use explicit `--name` and `--version` for releases.

Use `--out ./artifacts` in CI.

Use `--verbose` when debugging packaging.

Use `--sign=required` for serious release artifacts.

Use `--sign=never` for local test packages when signing is not needed.

Run `vix verify` after `vix pack`.

Keep `vix.toml` accurate so `manifest.json` contains useful metadata.

Do not edit generated `manifest.json` manually.

## Related commands

| Command       | Purpose                                           |
| ------------- | ------------------------------------------------- |
| `vix verify`  | Verify a package folder or artifact.              |
| `vix cache`   | Store a package locally.                          |
| `vix build`   | Build before packaging.                           |
| `vix check`   | Validate before packaging.                        |
| `vix task`    | Automate release workflows.                       |
| `vix publish` | Publish a tagged package version to the registry. |

## Next step

Verify a package artifact.

[Open the vix verify guide](/cli/verify)
