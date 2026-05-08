# vix cache

`vix cache` stores a Vix package locally so it can be reused quickly.

Use it when you already have a package folder or `.vixpkg` artifact and want to place it in the local Vix cache.

## Usage

```bash
vix cache --path <folder|artifact.vixpkg> [options]
```

## What it does

`vix cache` takes an existing Vix package and stores it locally. It verifies the package before storing by default, and supports signature verification and custom store locations.

## Basic usage

```bash
vix cache --path ./dist/blog@1.0.0.vixpkg
vix cache --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0 --force
```

## Verification

By default, `vix cache` verifies the package before storing. Use `--no-verify` to skip:

```bash
vix cache --path ./dist/blog@1.0.0.vixpkg --no-verify
```

For safer workflows, prefer:

```bash
vix verify --path ./dist/blog@1.0.0.vixpkg
vix cache --path ./dist/blog@1.0.0.vixpkg
```

## Signature and public key

```bash
vix cache \
  --path ./dist/blog@1.0.0.vixpkg \
  --require-signature \
  --pubkey ./keys/vix-pack.pub
```

## Custom store location

```bash
vix cache \
  --path ./dist/blog@1.0.0.vixpkg \
  --store ./local-store
```

## Options

| Option | Description |
|--------|-------------|
| `-p, --path <path>` | Package folder or `.vixpkg` artifact. Required. |
| `--store <dir>` | Custom store location. |
| `--force` | Overwrite if already cached. |
| `--no-verify` | Skip verification. Not recommended for release workflows. |
| `--verbose` | Show detailed operations. |
| `--require-signature` | Require a valid signature. |
| `--pubkey <path>` | Minisign public key. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# Pack, verify, and cache
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0

# Cache a .vixpkg
vix cache --path ./dist/blog@1.0.0.vixpkg

# Cache with overwrite
vix cache --path ./dist/blog@1.0.0 --force

# Cache with required signature
vix cache \
  --path ./dist/blog@1.0.0.vixpkg \
  --require-signature \
  --pubkey ./keys/vix-pack.pub

# CI usage
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
vix cache --path ./dist/blog@1.0.0
```

## Common mistakes

### Forgetting `--path`

```bash
# Wrong
vix cache

# Correct
vix cache --path ./dist/blog@1.0.0.vixpkg
```

### Caching before packing

```bash
vix pack --name blog --version 1.0.0
vix cache --path ./dist/blog@1.0.0
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix pack` | Create a package artifact |
| `vix verify` | Verify a package artifact |
| `vix store` | Manage the local package store |
| `vix build` | Build before packing |

## Next step

Continue with registry management.

[Open the vix registry guide](/cli/registry)
