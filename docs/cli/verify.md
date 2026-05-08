# vix verify

`vix verify` verifies a Vix package against the `vix.manifest.v2` schema.

Use it when you want to check that a package folder or `.vixpkg` artifact is valid, complete, and safe to reuse.

## Usage

```bash
vix verify [options]
vix verify --path <folder|artifact.vixpkg>
```

## What it does

`vix verify` checks a Vix package artifact. It can verify a package folder or `.vixpkg` artifact, validate the manifest, check integrity, verify checksums, verify minisign signatures, and auto-detect the latest package in `dist/`.

## Basic usage

```bash
vix verify                                      # auto-detect
vix verify --path ./dist/blog@1.0.0            # folder
vix verify --path ./dist/blog@1.0.0.vixpkg     # artifact
vix verify --verbose
```

## Auto-detection

When no `--path` is provided, Vix tries to detect the package automatically from the current directory or `dist/`.

## Strict mode

```bash
vix verify --strict
```

Missing optional security metadata becomes an error.

## Signature verification

```bash
vix verify --pubkey ./keys/vix-pack.pub
vix verify --path ./dist/blog@1.0.0 \
  --pubkey ./keys/vix-pack.pub \
  --require-signature
vix verify --no-sig   # skip signature verification
```

## Exit codes

| Exit code | Meaning |
|-----------|---------|
| `0` | Verification OK |
| `1` | Verification failed |

## Options

| Option | Description |
|--------|-------------|
| `-p, --path <path>` | Package folder or `.vixpkg` artifact. Default is auto-detect. |
| `--pubkey <path>` | Minisign public key. |
| `--verbose` | Print detailed checks and diagnostics. |
| `--strict` | Fail on missing optional security metadata. |
| `--require-signature` | Fail if signature is missing or cannot be verified. |
| `--no-sig` | Skip signature verification. |
| `--no-hash` | Skip `checksums.sha256` verification. |
| `-h, --help` | Show command help. |

## Environment variables

| Variable | Description |
|----------|-------------|
| `VIX_MINISIGN_PUBKEY` | Public key path used to verify minisign signatures. |

## Common workflows

```bash
# Pack and verify
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0

# Verify latest package automatically
vix verify

# Verify a .vixpkg
vix verify --path ./dist/blog@1.0.0.vixpkg

# Verify with required signature
vix verify \
  --path ./dist/blog@1.0.0 \
  --pubkey ./keys/vix-pack.pub \
  --require-signature

# CI usage
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0 --strict
```

## Common mistakes

### Verifying before packing

```bash
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
```

### Forgetting the public key for signature verification

```bash
vix verify --pubkey ./keys/vix-pack.pub --require-signature
# or
VIX_MINISIGN_PUBKEY=./keys/vix-pack.pub vix verify --require-signature
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix pack` | Create a package artifact |
| `vix cache` | Store a verified package locally |
| `vix build` | Build before packing |
| `vix check` | Validate before packing |

## Next step

Continue with package caching.

[Open the vix cache guide](/cli/cache)
