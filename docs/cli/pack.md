# vix pack

`vix pack` packages a Vix project into a distributable artifact.

Use it when you want to prepare a project for sharing, caching, verification, or deployment workflows.

## Usage

```bash
vix pack [options]
```

## What it does

`vix pack` creates a package from a Vix project. By default, it packages the project into `dist/<name>@<version>` and optionally creates a `dist/<name>@<version>.vixpkg` artifact using the Vix manifest v2 format.

## Basic usage

```bash
vix pack
vix pack --name blog --version 1.0.0
vix pack --verbose
vix pack --no-zip
```

## Output directory

Default output: `dist/`. Use `--out` to choose another:

```bash
vix pack --out ./artifacts
```

## Options

| Option | Description |
|--------|-------------|
| `-d, --dir <path>` | Project directory. Default is the current directory. |
| `--out <path>` | Output directory. Default is `<project>/dist`. |
| `--name <name>` | Package name. Default is the project folder name. |
| `--version <ver>` | Package version. Default is `0.1.0`. |
| `--no-zip` | Do not create a `.vixpkg` artifact. |
| `--no-hash` | Do not generate `checksums.sha256`. |
| `--verbose` | Show copied files and signing output. |
| `--sign[=mode]` | Signing mode: `auto`, `never`, or `required`. |
| `-h, --help` | Show command help. |

## Environment variables

| Variable | Description |
|----------|-------------|
| `VIX_MINISIGN_SECKEY` | Secret key path used to sign package metadata. |

## Common workflows

```bash
vix pack
vix pack --name blog --version 1.0.0
vix pack --no-zip
vix pack --no-hash
vix pack --verbose
vix pack --sign=never
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign=required
vix pack --out ./artifacts --name blog --version 1.0.0
```

## Recommended release flow

```bash
vix build --preset release
vix check --tests
vix pack --name blog --version 1.0.0
vix verify --path ./dist/blog@1.0.0
```

## Common mistakes

### Packing before building

For release workflows, build first:

```bash
vix build --preset release
vix pack --name blog --version 1.0.0
```

### Forgetting to verify

```bash
vix verify --path ./dist/blog@1.0.0
```

### Requiring signing without a key

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign=required
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix verify` | Verify a package artifact |
| `vix cache` | Store a package locally |
| `vix build` | Build before packaging |
| `vix check` | Validate before packaging |
| `vix task` | Automate release workflows |

## Next step

Continue with package verification.

[Open the vix verify guide](/cli/verify)
