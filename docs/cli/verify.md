# vix verify

`vix verify` verifies a Vix package folder or `.vixpkg` artifact against the `vix.manifest.v2` package format.

Use it when you want to check that a package created by `vix pack` is valid, complete, and safe to reuse.

```bash
vix verify
```

## Overview

`vix verify` is the validation command for Vix package artifacts.

It can verify:

```txt
package folders
.vixpkg archives
latest package in dist/
current folder when it contains manifest.json
```

It checks:

```txt
manifest.json
vix.manifest.v2 schema fields
payload digest
checksums.sha256
minisign signature
required security metadata in strict mode
```

The command is read-only.

It does not modify the package.

## Usage

```bash
vix verify [options]
vix verify --path <folder|artifact.vixpkg>
```

## Basic examples

```bash
# Auto-detect latest package
vix verify

# Verify a package folder
vix verify --path ./dist/blog@1.0.0

# Verify a .vixpkg artifact
vix verify --path ./dist/blog@1.0.0.vixpkg

# Print detailed checks
vix verify --verbose

# Fail on missing optional security metadata
vix verify --strict
```

## Auto-detection

When no `--path` is provided, Vix tries to choose a package automatically.

Resolution order:

```txt
1. If current directory contains manifest.json, verify current directory.
2. Else if current directory contains CMakeLists.txt, verify latest dist/*/manifest.json.
3. Else if ./dist exists, verify latest dist/*/manifest.json.
4. Else verify current directory and fail with a useful error if no manifest exists.
```

Example:

```bash
vix pack --name blog --version 1.0.0
vix verify
```

If `dist/blog@1.0.0/manifest.json` is the latest package manifest, Vix verifies that package.

## Verify current package folder

If you are already inside a package folder:

```bash
cd dist/blog@1.0.0
vix verify
```

Vix detects:

```txt
manifest.json
```

and verifies the current directory.

## Verify explicit package folder

Use:

```bash
vix verify --path ./dist/blog@1.0.0
```

or:

```bash
vix verify -p ./dist/blog@1.0.0
```

This verifies the folder directly.

## Verify `.vixpkg`

On Linux and macOS, Vix can verify a `.vixpkg` archive:

```bash
vix verify --path ./dist/blog@1.0.0.vixpkg
```

The archive is extracted to a temporary directory.

Then Vix verifies the extracted package root.

If extraction produces a single nested folder containing `manifest.json`, Vix automatically uses that nested folder as the package root.

## `.vixpkg` requirement

Verifying `.vixpkg` requires:

```txt
unzip
```

If `unzip` is missing or extraction fails, Vix reports:

```txt
Unable to extract .vixpkg (need unzip).
```

Install `unzip`, or verify the folder package instead:

```bash
vix verify --path ./dist/blog@1.0.0
```

## Target output

Example:

```bash
vix verify --path ./dist/blog@1.0.0 --verbose
```

Output shape:

```txt
vix verify
Target:
  ./dist/blog@1.0.0

payload digest ok
digest: <sha256>
checksums ok: 12 file(s)
signature ok (minisign)

Verification OK.
```

If auto-detection is used with `--verbose`, Vix also prints the auto-detected package path.

## Manifest validation

`vix verify` requires:

```txt
manifest.json
```

The manifest must be valid JSON.

The root must be a JSON object.

The manifest schema must be:

```txt
vix.manifest.v2
```

Required minimal fields:

```txt
schema
package.name
package.version
abi.os
abi.arch
payload.digest_algorithm
payload.digest
```

## Minimal manifest shape

Example:

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

## Missing manifest

If `manifest.json` is missing, Vix fails:

```txt
manifest.json is missing.
```

If no explicit path was provided, Vix also prints hints:

```txt
If you are in a project folder, run: vix verify --path ./dist/<name>@<version>
Or run `vix pack` first.
```

Fix:

```bash
vix pack
vix verify
```

or:

```bash
vix verify --path ./dist/<name>@<version>
```

## Payload digest verification

On Linux and macOS, Vix verifies the payload digest.

The manifest digest is the authoritative value:

```json
{
  "payload": {
    "digest": "..."
  }
}
```

Vix rebuilds a stable SHA256 listing of the package payload, computes the digest, and compares it to:

```txt
manifest.payload.digest
```

If they differ, verification fails:

```txt
Payload digest mismatch (computed != manifest.payload.digest).
```

## Payload excludes

To avoid self-referential hashes, these files are excluded from the payload digest:

```txt
manifest.json
checksums.sha256
meta/payload.digest
meta/payload.digest.minisig
```

If the manifest has:

```json
{
  "payload": {
    "excludes": [
      "manifest.json",
      "checksums.sha256",
      "meta/payload.digest",
      "meta/payload.digest.minisig"
    ]
  }
}
```

Vix also applies those excludes.

## `meta/payload.digest`

If the package contains:

```txt
meta/payload.digest
```

Vix checks that it matches:

```txt
manifest.payload.digest
```

If it differs, verification fails:

```txt
Payload digest mismatch (meta/payload.digest != manifest.payload.digest).
```

If it is missing:

```txt
meta/payload.digest missing.
```

This is a warning by default.

In strict mode, it becomes an error.

## Checksums verification

On Linux and macOS, Vix verifies:

```txt
checksums.sha256
```

unless you pass:

```bash
vix verify --no-hash
```

For each file listed in `checksums.sha256`, Vix checks:

```txt
file exists
computed sha256 matches expected sha256
```

If a listed file is missing, verification fails.

If a hash differs, verification fails.

## Missing checksums

If `checksums.sha256` is missing, Vix warns by default:

```txt
checksums.sha256 is missing.
```

In strict mode, it becomes an error:

```txt
checksums.sha256 is missing (strict mode).
```

## Skip checksum verification

Use:

```bash
vix verify --no-hash
```

This skips `checksums.sha256` verification.

With `--verbose`, Vix prints:

```txt
checksums verification skipped (--no-hash)
```

## Signature verification

On Linux and macOS, Vix can verify a minisign signature for:

```txt
meta/payload.digest
```

The signature file is:

```txt
meta/payload.digest.minisig
```

The public key can come from:

```txt
--pubkey <path>
VIX_MINISIGN_PUBKEY
default key locations
```

## Public key resolution order

Vix resolves the public key in this order:

```txt
1. --pubkey <path>
2. VIX_MINISIGN_PUBKEY
3. ~/.config/vix/keys/vix-pack.pub
4. ~/keys/vix/vix-pack.pub
```

Example:

```bash
vix verify --pubkey ./keys/vix-pack.pub --require-signature
```

or:

```bash
VIX_MINISIGN_PUBKEY=./keys/vix-pack.pub vix verify --require-signature
```

## Require signature

Use:

```bash
vix verify --require-signature
```

This fails if the signature is missing or cannot be verified.

Recommended release verification:

```bash
vix verify \
  --path ./dist/blog@1.0.0 \
  --pubkey ./keys/vix-pack.pub \
  --require-signature
```

## Strict mode

Use:

```bash
vix verify --strict
```

Strict mode makes missing optional security metadata fail.

In strict mode, these become errors:

```txt
missing checksums.sha256
missing meta/payload.digest
missing meta/payload.digest.minisig
missing public key when signature exists or is required
missing minisign when signature is required
```

Strict mode is useful for CI and release workflows.

## Skip signature verification

Use:

```bash
vix verify --no-sig
```

This skips signature verification.

However, this cannot be combined safely with strict signature requirements.

If you use `--no-sig` with `--require-signature` or `--strict`, Vix fails:

```txt
--no-sig cannot be used with --require-signature/--strict.
```

## Missing signature

If the signature file is missing:

```txt
meta/payload.digest.minisig
```

Vix warns by default:

```txt
meta/payload.digest.minisig missing (signature not verified).
To generate it, set VIX_MINISIGN_SECKEY when running `vix pack`.
```

With `--require-signature` or `--strict`, this becomes an error:

```txt
meta/payload.digest.minisig missing (signature required).
```

## Signature verification tools

Signature verification requires:

```txt
minisign
```

If `minisign` is missing and signature is required, Vix fails.

If signature is not required, Vix warns and skips signature verification.

## Windows behavior

On Windows, the current implementation performs manifest checks only.

In strict mode, Vix warns:

```txt
Windows: only manifest checks are implemented for now.
```

Full payload, checksum, archive extraction, and minisign verification are currently implemented for Unix-like systems.

## Exit codes

| Exit code | Meaning              |
| --------- | -------------------- |
| `0`       | Verification OK.     |
| `1`       | Verification failed. |

## Recommended pack and verify flow

```bash
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
```

If a `.vixpkg` archive was created:

```bash
vix verify --path ./dist/blog@1.0.0.vixpkg
```

## Strict CI flow

```bash
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0 --strict
```

## Signed release flow

Pack with required signing:

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key \
  vix pack --name blog --version 1.0.0 --sign=required
```

Verify with required signature:

```bash
vix verify \
  --path ./dist/blog@1.0.0 \
  --pubkey ./keys/vix-pack.pub \
  --require-signature
```

or:

```bash
VIX_MINISIGN_PUBKEY=./keys/vix-pack.pub \
  vix verify --path ./dist/blog@1.0.0 --require-signature
```

## Auto-detect latest package flow

```bash
vix pack --name blog --version 1.0.0
vix verify
```

This verifies the latest package folder found in:

```txt
dist/
```

## Options

| Option                | Description                                                 |
| --------------------- | ----------------------------------------------------------- |
| `-p, --path <path>`   | Package folder or `.vixpkg` artifact. Default: auto-detect. |
| `--pubkey <path>`     | Minisign public key.                                        |
| `--verbose`           | Print detailed checks and diagnostics.                      |
| `--strict`            | Fail on missing optional security metadata.                 |
| `--require-signature` | Fail if signature is missing or cannot be verified.         |
| `--no-sig`            | Skip signature verification.                                |
| `--no-hash`           | Skip `checksums.sha256` verification.                       |
| `-h, --help`          | Show command help.                                          |

## Environment variables

| Variable              | Description                                         |
| --------------------- | --------------------------------------------------- |
| `VIX_MINISIGN_PUBKEY` | Public key path used to verify minisign signatures. |

## Commands reference

| Command                                      | Description                                 |
| -------------------------------------------- | ------------------------------------------- |
| `vix verify`                                 | Auto-detect and verify a package.           |
| `vix verify --verbose`                       | Auto-detect and print detailed checks.      |
| `vix verify --path ./dist/blog@1.0.0`        | Verify a package folder.                    |
| `vix verify --path ./dist/blog@1.0.0.vixpkg` | Verify a `.vixpkg` archive.                 |
| `vix verify --strict`                        | Fail on missing optional security metadata. |
| `vix verify --require-signature`             | Require minisign signature verification.    |
| `vix verify --no-sig`                        | Skip signature verification.                |
| `vix verify --no-hash`                       | Skip checksum verification.                 |

## Common workflows

### Pack and verify folder

```bash
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
```

### Pack and verify archive

```bash
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0.vixpkg
```

### Verify latest package automatically

```bash
vix verify
```

### Verify with detailed output

```bash
vix verify --verbose
```

### Verify without checksum check

```bash
vix verify --no-hash
```

### Verify without signature check

```bash
vix verify --no-sig
```

### Verify with required signature

```bash
vix verify \
  --path ./dist/blog@1.0.0 \
  --pubkey ./keys/vix-pack.pub \
  --require-signature
```

### Verify in CI

```bash
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0 --strict
```

## Common mistakes

### Verifying before packing

Wrong:

```bash
vix verify --path ./dist/blog@1.0.0
```

when the package does not exist.

Correct:

```bash
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
```

### Forgetting the public key

Wrong:

```bash
vix verify --require-signature
```

when no public key is configured.

Correct:

```bash
vix verify --pubkey ./keys/vix-pack.pub --require-signature
```

or:

```bash
VIX_MINISIGN_PUBKEY=./keys/vix-pack.pub vix verify --require-signature
```

### Using `--no-sig` with strict verification

Wrong:

```bash
vix verify --strict --no-sig
```

Correct:

```bash
vix verify --strict
```

or:

```bash
vix verify --no-sig
```

depending on whether you want strict signature enforcement.

### Expecting `.vixpkg` verification without `unzip`

Verifying a `.vixpkg` requires:

```txt
unzip
```

If `unzip` is unavailable, verify the package folder:

```bash
vix verify --path ./dist/blog@1.0.0
```

### Expecting signature verification without `minisign`

Signature verification requires:

```txt
minisign
```

If you do not require signature verification, Vix can still verify manifest, payload digest, and checksums.

### Expecting `--no-hash` to skip payload digest

`--no-hash` skips `checksums.sha256` verification.

It does not skip `manifest.payload.digest` verification.

### Expecting Windows to perform all checks

The current Windows implementation focuses on manifest checks.

Full payload digest, checksums, archive extraction, and minisign checks are Unix-oriented in the current implementation.

## Troubleshooting

### `manifest.json is missing`

Run:

```bash
vix pack
```

Then verify again:

```bash
vix verify
```

Or pass the correct package path:

```bash
vix verify --path ./dist/<name>@<version>
```

### `manifest.json is invalid JSON`

Open the manifest:

```bash
cat ./dist/<name>@<version>/manifest.json
```

Then regenerate the package:

```bash
vix pack --name <name> --version <version>
```

### `manifest.schema must be 'vix.manifest.v2'`

The package was not created with the expected Vix manifest v2 format.

Regenerate it:

```bash
vix pack
```

### `Payload digest mismatch`

The package contents changed after packing.

Regenerate the package:

```bash
vix pack --name <name> --version <version>
vix verify --path ./dist/<name>@<version>
```

### `SHA256 mismatch`

A file listed in `checksums.sha256` changed after packing.

Regenerate the package:

```bash
vix pack
```

### `Missing file listed in checksums.sha256`

A file was removed after packing.

Regenerate the package or restore the missing file.

### `meta/payload.digest.minisig missing`

The package was not signed.

For signed packages:

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key \
  vix pack --sign=required
```

Then verify:

```bash
vix verify --pubkey ./keys/vix-pack.pub --require-signature
```

### `--pubkey is required`

Provide a public key:

```bash
vix verify --pubkey ./keys/vix-pack.pub --require-signature
```

or set:

```bash
export VIX_MINISIGN_PUBKEY=./keys/vix-pack.pub
```

### `minisign verification failed`

Check that:

```txt
the package was signed with the matching private key
the public key is correct
meta/payload.digest was not modified
meta/payload.digest.minisig was not modified
```

Then repack and verify again.

### `Unable to extract .vixpkg`

Install `unzip`, or verify the package folder instead:

```bash
vix verify --path ./dist/<name>@<version>
```

## Best practices

Run `vix verify` after every release package.

Use `--strict` in CI.

Use `--require-signature` for serious release artifacts.

Use `--verbose` when debugging verification failures.

Do not edit package contents after `vix pack`.

If package contents change, run `vix pack` again.

Keep private signing keys secure.

Use `VIX_MINISIGN_PUBKEY` in CI when verifying signed packages.

Verify the folder package first, then verify the `.vixpkg` archive when it exists.

## Related commands

| Command       | Purpose                                           |
| ------------- | ------------------------------------------------- |
| `vix pack`    | Create a package folder or `.vixpkg` artifact.    |
| `vix cache`   | Store a verified package locally.                 |
| `vix build`   | Build before packaging.                           |
| `vix check`   | Validate before packaging.                        |
| `vix task`    | Automate release workflows.                       |
| `vix publish` | Publish a tagged package version to the registry. |

## Next step

Store a verified package locally.

[Open the vix cache guide](/cli/cache)
