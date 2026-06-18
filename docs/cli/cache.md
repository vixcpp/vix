# vix cache

`vix cache` stores a Vix package locally so it can be reused quickly.

Use it when you already have a package folder or `.vixpkg` artifact and want to place it in the local Vix package cache.

```bash
vix cache --path ./dist/blog@1.0.0
```

## Overview

`vix cache` takes an existing Vix package and stores it in a local cache layout.

It is useful after:

```bash
vix pack
vix verify
```

The command supports:

```txt
package folders
.vixpkg artifacts
custom store locations
safe overwrite with --force
verification before caching
signature verification
atomic cache writes
```

The cached package is stored by:

```txt
name
version
ABI tag
```

The default destination shape is:

```txt
<store>/packs/<name>/<version>/<os>-<arch>/
```

Example:

```txt
~/.local/share/vix/packs/blog/1.0.0/linux-x86_64/
```

## Usage

```bash
vix cache --path <folder|artifact.vixpkg> [options]
```

`--path` is required.

## Basic examples

```bash
# Cache a package folder
vix cache --path ./dist/blog@1.0.0

# Cache a .vixpkg artifact
vix cache --path ./dist/blog@1.0.0.vixpkg

# Overwrite an existing cached package
vix cache --path ./dist/blog@1.0.0 --force

# Show detailed copy operations
vix cache --path ./dist/blog@1.0.0 --verbose

# Cache into a custom store
vix cache --path ./dist/blog@1.0.0 --store ./local-store
```

## Required input

You must pass:

```bash
--path <folder|artifact.vixpkg>
```

If you run:

```bash
vix cache
```

Vix reports:

```txt
Missing --path. Try: vix cache --path <folder|artifact.vixpkg>
```

Correct:

```bash
vix cache --path ./dist/blog@1.0.0
```

or:

```bash
vix cache --path ./dist/blog@1.0.0.vixpkg
```

## What it accepts

`vix cache` accepts:

```txt
package folder
.vixpkg artifact
```

A package folder must contain:

```txt
manifest.json
```

A `.vixpkg` artifact is extracted first, then cached.

## `.vixpkg` extraction

On Linux and macOS, `.vixpkg` extraction uses:

```txt
unzip
```

Example:

```bash
vix cache --path ./dist/blog@1.0.0.vixpkg
```

If `unzip` is missing or extraction fails, Vix reports:

```txt
Unable to extract .vixpkg (need unzip).
```

In that case, cache the folder package instead:

```bash
vix cache --path ./dist/blog@1.0.0
```

## Single nested folder support

If a `.vixpkg` extracts to a single nested folder, Vix detects it.

Example extracted layout:

```txt
/tmp/vix_cache_1234/
└── blog@1.0.0/
    └── manifest.json
```

Vix uses:

```txt
/tmp/vix_cache_1234/blog@1.0.0/
```

as the package root.

## Source and pack root

When caching starts, Vix prints the original input and the resolved package root.

Example output shape:

```txt
vix cache

Source:
  dist/blog@1.0.0.vixpkg

Pack root:
  /tmp/vix_cache_1234/blog@1.0.0
```

For folder packages, the source and pack root are usually the same.

## Manifest requirement

The package root must contain:

```txt
manifest.json
```

If it is missing, Vix reports:

```txt
manifest.json is missing in pack.
Make sure you pass a valid package folder or .vixpkg artifact.
```

Fix:

```bash
vix pack --name blog --version 1.0.0
vix cache --path ./dist/blog@1.0.0
```

## Manifest format

`vix cache` expects a Vix manifest v2 package:

```json
{
  "schema": "vix.manifest.v2",
  "package": {
    "name": "blog",
    "version": "1.0.0"
  },
  "abi": {
    "os": "linux",
    "arch": "x86_64"
  },
  "payload": {
    "digest_algorithm": "sha256",
    "digest": "..."
  }
}
```

Required fields:

```txt
schema
package.name
package.version
abi.os
abi.arch
payload.digest_algorithm
payload.digest
```

## Package identity

The cache destination is computed from:

```txt
manifest.package.name
manifest.package.version
manifest.abi.os
manifest.abi.arch
```

Example:

```json
{
  "package": {
    "name": "blog",
    "version": "1.0.0"
  },
  "abi": {
    "os": "linux",
    "arch": "x86_64"
  }
}
```

Destination:

```txt
<store>/packs/blog/1.0.0/linux-x86_64/
```

The ABI tag is:

```txt
<os>-<arch>
```

Example:

```txt
linux-x86_64
```

## Default store location

If `--store` is not passed, Vix chooses the store root in this order.

On Linux and macOS:

```txt
1. VIX_STORE
2. XDG_DATA_HOME/vix
3. ~/.local/share/vix
4. temp/vix fallback
```

On Windows:

```txt
1. VIX_STORE
2. USERPROFILE/.vix
3. temp/vix fallback
```

So on a normal Linux machine, the default package cache root is usually:

```txt
~/.local/share/vix
```

The final cached package path becomes:

```txt
~/.local/share/vix/packs/<name>/<version>/<os>-<arch>/
```

## Custom store

Use:

```bash
vix cache --path ./dist/blog@1.0.0 --store ./local-store
```

This stores the package under:

```txt
./local-store/packs/<name>/<version>/<os>-<arch>/
```

Example:

```txt
./local-store/packs/blog/1.0.0/linux-x86_64/
```

## Verification by default

By default, `vix cache` verifies the package before storing it.

It checks:

```txt
manifest minimal validity
payload digest
checksums.sha256
signature when available or required
```

This makes caching safer because the package is validated before being copied into the store.

## Skip verification

Use:

```bash
vix cache --path ./dist/blog@1.0.0 --no-verify
```

This skips package verification.

Vix prints:

```txt
Verification skipped (--no-verify).
```

This is not recommended for release workflows.

Use it only for local experiments or when the package was already verified in a previous trusted step.

Recommended:

```bash
vix verify --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0
```

## Payload digest verification

On Linux and macOS, Vix verifies:

```txt
manifest.payload.digest
```

It rebuilds a stable SHA256 listing of the package payload and compares it to the manifest digest.

If the digest does not match, caching fails.

Example failure:

```txt
Payload digest mismatch (computed != manifest.payload.digest).
```

## Payload digest exclusions

These files are excluded from the payload digest:

```txt
manifest.json
checksums.sha256
meta/payload.digest
meta/payload.digest.minisig
```

Vix also reads additional excludes from:

```json
{
  "payload": {
    "excludes": []
  }
}
```

## `meta/payload.digest`

If present, Vix also checks:

```txt
meta/payload.digest
```

against:

```txt
manifest.payload.digest
```

If it differs, caching fails.

If it is missing, Vix warns:

```txt
meta/payload.digest missing.
```

## Checksums verification

On Linux and macOS, Vix checks:

```txt
checksums.sha256
```

If the file is present, Vix verifies every listed file.

It checks:

```txt
file exists
sha256 matches expected value
```

If a listed file is missing, caching fails.

If a listed file has a different hash, caching fails.

If `checksums.sha256` is missing, Vix warns:

```txt
checksums.sha256 is missing.
```

## Signature verification

`vix cache` can verify a minisign signature for:

```txt
meta/payload.digest
```

The signature file is:

```txt
meta/payload.digest.minisig
```

By default, if the signature is missing, Vix warns but does not fail.

To make signature verification mandatory, use:

```bash
vix cache \
  --path ./dist/blog@1.0.0 \
  --require-signature \
  --pubkey ./keys/vix-pack.pub
```

## Public key resolution

The public key can come from:

```txt
--pubkey <path>
VIX_MINISIGN_PUBKEY
~/.config/vix/keys/vix-pack.pub
~/keys/vix/vix-pack.pub
```

Resolution order:

```txt
1. --pubkey <path>
2. VIX_MINISIGN_PUBKEY
3. ~/.config/vix/keys/vix-pack.pub
4. ~/keys/vix/vix-pack.pub
```

Example with CLI option:

```bash
vix cache \
  --path ./dist/blog@1.0.0 \
  --require-signature \
  --pubkey ./keys/vix-pack.pub
```

Example with environment variable:

```bash
VIX_MINISIGN_PUBKEY=./keys/vix-pack.pub \
  vix cache --path ./dist/blog@1.0.0 --require-signature
```

## Missing signature

If the package is not signed and signature is not required, Vix warns:

```txt
meta/payload.digest.minisig missing (signature not verified).
```

If signature is required, Vix fails:

```txt
meta/payload.digest.minisig missing (signature required).
```

## Missing public key

If a signature exists but no public key is available, Vix warns by default:

```txt
No pubkey provided; skipping signature verification.
```

If signature is required, Vix fails:

```txt
--pubkey is required (or set VIX_MINISIGN_PUBKEY) to verify signature.
```

## Missing minisign

If `minisign` is missing and signature is not required, Vix warns:

```txt
minisign not available; skipping signature verification.
```

If signature is required, Vix fails:

```txt
minisign not available (signature required).
```

## Atomic cache write

`vix cache` writes the package atomically.

The flow is:

```txt
copy package to temporary destination
rename temporary destination to final destination
```

Example:

```txt
<store>/packs/blog/1.0.0/blog-x86_64.tmp.<pid>
<store>/packs/blog/1.0.0/linux-x86_64
```

This avoids leaving a broken final cache directory if copying fails.

If `rename()` fails, Vix falls back to copy and remove.

## Existing cached package

If the package is already cached, Vix refuses to overwrite it by default.

Example error:

```txt
Package already cached: <path> (use --force)
```

Use:

```bash
vix cache --path ./dist/blog@1.0.0 --force
```

This removes the existing cached package and writes the new one.

## Verbose mode

Use:

```bash
vix cache --path ./dist/blog@1.0.0 --verbose
```

Verbose mode shows detailed operations such as copied files and verification details.

Example output shape:

```txt
payload digest ok
digest: <sha256>
checksums ok: 12 file(s)
signature ok (minisign)
copied: manifest.json
copied: include/blog.hpp
copied: src/blog.cpp
```

## Successful output

Example:

```txt
vix cache

Source:
  dist/blog@1.0.0

Pack root:
  /home/user/project/dist/blog@1.0.0

Verifying:
  payload digest ok
  checksums ok: 12 file(s)

Target store:
  /home/user/.local/share/vix

Caching:
  blog@1.0.0 (linux-x86_64)
  /home/user/.local/share/vix/packs/blog/1.0.0/linux-x86_64

Package cached:
  /home/user/.local/share/vix/packs/blog/1.0.0/linux-x86_64
```

After caching, Vix prints:

```txt
Next: vix get blog@1.0.0 --out ./<folder> (coming soon)
```

`vix get` is not part of the current cache behavior yet. It is only shown as a future workflow hint.

## Windows behavior

On Windows, the current implementation performs minimal manifest validation when verification is enabled.

It prints:

```txt
Windows: only minimal manifest validation is available right now.
```

Full payload digest, checksums, `.vixpkg` extraction, and minisign verification are Unix-oriented in the current implementation.

## Recommended workflow

```bash
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0
```

If a `.vixpkg` archive exists:

```bash
vix cache --path ./dist/blog@1.0.0.vixpkg
```

## Signed cache workflow

Package with signing:

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key \
  vix pack --name blog --version 1.0.0 --sign=required
```

Cache with required signature:

```bash
vix cache \
  --path ./dist/blog@1.0.0 \
  --require-signature \
  --pubkey ./keys/vix-pack.pub
```

## Custom local store workflow

```bash
vix pack --name blog --version 1.0.0
vix cache --path ./dist/blog@1.0.0 --store ./local-store
```

Cached path:

```txt
./local-store/packs/blog/1.0.0/linux-x86_64/
```

## Force overwrite workflow

```bash
vix cache --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0 --force
```

Use `--force` when you intentionally want to replace the cached copy.

## CI usage

```bash
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0
```

For signed CI artifacts:

```bash
vix build --preset release
vix check --tests

VIX_MINISIGN_SECKEY=./keys/vix-pack.key \
  vix pack --name blog --version 1.0.0 --sign=required

vix cache \
  --path ./dist/blog@1.0.0 \
  --require-signature \
  --pubkey ./keys/vix-pack.pub
```

## Options

| Option                | Description                                               |
| --------------------- | --------------------------------------------------------- |
| `-p, --path <path>`   | Package folder or `.vixpkg` artifact. Required.           |
| `--store <dir>`       | Custom store location.                                    |
| `--force`             | Overwrite if already cached.                              |
| `--no-verify`         | Skip verification. Not recommended for release workflows. |
| `--verbose`           | Show detailed operations.                                 |
| `--require-signature` | Require a valid minisign signature.                       |
| `--pubkey <path>`     | Minisign public key.                                      |
| `-h, --help`          | Show command help.                                        |

## Environment variables

| Variable              | Description                                                   |
| --------------------- | ------------------------------------------------------------- |
| `VIX_STORE`           | Override the default local package cache root.                |
| `XDG_DATA_HOME`       | Used on Unix when `VIX_STORE` is not set.                     |
| `VIX_MINISIGN_PUBKEY` | Public key path used to verify minisign signatures.           |
| `HOME`                | Used on Unix for default store and default public key lookup. |
| `USERPROFILE`         | Used on Windows for default store fallback.                   |

## Commands reference

| Command                                                                               | Description                                    |
| ------------------------------------------------------------------------------------- | ---------------------------------------------- |
| `vix cache --path ./dist/blog@1.0.0`                                                  | Cache a package folder.                        |
| `vix cache --path ./dist/blog@1.0.0.vixpkg`                                           | Cache a package archive.                       |
| `vix cache --path ./dist/blog@1.0.0 --force`                                          | Overwrite existing cached package.             |
| `vix cache --path ./dist/blog@1.0.0 --no-verify`                                      | Cache without verification.                    |
| `vix cache --path ./dist/blog@1.0.0 --store ./local-store`                            | Cache into a custom store.                     |
| `vix cache --path ./dist/blog@1.0.0 --require-signature --pubkey ./keys/vix-pack.pub` | Cache only if signature verification succeeds. |

## Common workflows

### Pack, verify, cache

```bash
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0
```

### Cache a `.vixpkg`

```bash
vix cache --path ./dist/blog@1.0.0.vixpkg
```

### Cache with overwrite

```bash
vix cache --path ./dist/blog@1.0.0 --force
```

### Cache with required signature

```bash
vix cache \
  --path ./dist/blog@1.0.0.vixpkg \
  --require-signature \
  --pubkey ./keys/vix-pack.pub
```

### Cache into a custom store

```bash
vix cache \
  --path ./dist/blog@1.0.0 \
  --store ./local-store
```

### Cache without verification

```bash
vix cache \
  --path ./dist/blog@1.0.0 \
  --no-verify
```

## Common mistakes

### Forgetting `--path`

Wrong:

```bash
vix cache
```

Correct:

```bash
vix cache --path ./dist/blog@1.0.0
```

### Caching before packing

Wrong:

```bash
vix cache --path ./dist/blog@1.0.0
```

when the package does not exist.

Correct:

```bash
vix pack --name blog --version 1.0.0
vix cache --path ./dist/blog@1.0.0
```

### Passing a project root instead of a package root

Wrong:

```bash
vix cache --path .
```

from a normal project root without `manifest.json`.

Correct:

```bash
vix pack
vix cache --path ./dist/<name>@<version>
```

### Expecting cache to build the project

`vix cache` does not build.

Build first:

```bash
vix build --preset release
```

Then pack:

```bash
vix pack --name blog --version 1.0.0
```

Then cache:

```bash
vix cache --path ./dist/blog@1.0.0
```

### Expecting cache to create a package

`vix cache` stores an existing package.

It does not create the package.

Use:

```bash
vix pack
```

to create one.

### Skipping verification in release workflows

Avoid this for releases:

```bash
vix cache --path ./dist/blog@1.0.0 --no-verify
```

Prefer:

```bash
vix verify --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0
```

### Requiring signature without a public key

Wrong:

```bash
vix cache --path ./dist/blog@1.0.0 --require-signature
```

if no public key is configured.

Correct:

```bash
vix cache \
  --path ./dist/blog@1.0.0 \
  --require-signature \
  --pubkey ./keys/vix-pack.pub
```

or:

```bash
VIX_MINISIGN_PUBKEY=./keys/vix-pack.pub \
  vix cache --path ./dist/blog@1.0.0 --require-signature
```

### Re-caching without `--force`

If the package is already cached, use:

```bash
vix cache --path ./dist/blog@1.0.0 --force
```

only when you intentionally want to overwrite it.

## Troubleshooting

### `Missing --path`

Pass a package folder or `.vixpkg`:

```bash
vix cache --path ./dist/blog@1.0.0
```

### `Path not found or unsupported`

Check that the path exists:

```bash
ls ./dist
```

Then run:

```bash
vix cache --path ./dist/<name>@<version>
```

### `Unable to extract .vixpkg`

Install `unzip`, or cache the folder package:

```bash
vix cache --path ./dist/<name>@<version>
```

### `manifest.json is missing in pack`

You passed a folder that is not a package folder.

Run:

```bash
vix pack
```

Then cache the generated folder:

```bash
vix cache --path ./dist/<name>@<version>
```

### `manifest.json is invalid JSON`

Regenerate the package:

```bash
vix pack --name <name> --version <version>
```

Then cache again.

### `manifest.schema must be 'vix.manifest.v2'`

The package was not created with the expected Vix manifest v2 format.

Regenerate it:

```bash
vix pack
```

### `Payload digest mismatch`

The package contents changed after packing.

Regenerate and cache again:

```bash
vix pack --name <name> --version <version>
vix cache --path ./dist/<name>@<version> --force
```

### `SHA256 mismatch`

A file listed in `checksums.sha256` changed.

Regenerate the package:

```bash
vix pack
```

### `Missing file listed in checksums.sha256`

A file was removed after packing.

Regenerate the package or restore the missing file.

### `Package already cached`

Use:

```bash
vix cache --path ./dist/<name>@<version> --force
```

only if replacing the cached package is intentional.

### `cache failed`

Run with verbose output:

```bash
vix cache --path ./dist/<name>@<version> --verbose
```

Check permissions for the target store directory.

### `minisign verification failed`

Check that:

```txt
the package was signed with the matching private key
the public key is correct
meta/payload.digest was not modified
meta/payload.digest.minisig was not modified
```

Then repack and cache again.

## Best practices

Run `vix verify` before `vix cache`.

Keep verification enabled by default.

Use `--require-signature` for serious release artifacts.

Use `--force` only when intentionally replacing a cached package.

Use `--store` in CI when you want an isolated cache.

Use `VIX_STORE` when you want a stable shared cache path.

Do not edit package contents after `vix pack`.

If package contents change, run `vix pack` again.

Use `--verbose` when debugging cache failures.

## Related commands

| Command       | Purpose                                           |
| ------------- | ------------------------------------------------- |
| `vix pack`    | Create a package folder or `.vixpkg` artifact.    |
| `vix verify`  | Verify a package folder or artifact.              |
| `vix store`   | Manage the local dependency store.                |
| `vix build`   | Build before packaging.                           |
| `vix check`   | Validate before packaging.                        |
| `vix publish` | Publish a tagged package version to the registry. |

## Next step

Continue with registry management.

[Open the vix registry guide](/cli/registry)
