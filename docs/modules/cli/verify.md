# vix verify

Verify a Vix package against the `vix.manifest.v2` schema.

---

## Usage

```bash
vix verify [options]
vix verify --path <folder|artifact.vixpkg>
```

---

## Description

`vix verify` validates:

- Manifest structure (v2 schema)
- Package integrity
- Optional checksums (`checksums.sha256`)
- Optional minisign signatures

By default, it auto-detects the package when run inside a project.

---

## Auto-detection Rules

When no `--path` is provided:

1. If current directory contains `manifest.json`
   → verify current directory

2. If current directory contains `CMakeLists.txt`
   → verify latest `dist/*/manifest.json`

3. If `./dist` exists
   → verify latest `dist/*/manifest.json`

---

## Options

```bash
-p, --path <path>          Package folder or .vixpkg artifact
--pubkey <path>            minisign public key
--verbose                  Detailed verification output
--strict                   Fail on missing optional security metadata
--require-signature        Require valid signature
--no-sig                   Skip signature verification
--no-hash                  Skip checksums verification
-h, --help                 Show help
```

---

## Signature Behavior

- If `meta/payload.digest.minisig` is missing:
  - Warning by default
  - Error with `--require-signature` or `--strict`

Public key resolution order:

1. `--pubkey`
2. `VIX_MINISIGN_PUBKEY`
3. Default key locations

---

## Exit Codes

```
0  Verification OK
1  Verification failed
```

---

## Examples

Verify auto-detected package:

```bash
vix verify
```

Verbose mode:

```bash
vix verify --verbose
```

Verify specific folder:

```bash
vix verify --path ./dist/blog@1.0.0
```

Require signature:

```bash
vix verify --require-signature
```

Verify archive:

```bash
vix verify --path ./dist/blog@1.0.0.vixpkg
```

Provide public key:

```bash
vix verify --pubkey ./keys/vix-pack.pub --require-signature
```

Using environment variable:

```bash
VIX_MINISIGN_PUBKEY=./keys/vix-pack.pub vix verify --strict
```

---

## Typical Workflow

```bash
vix pack --version 1.0.0
vix verify --require-signature
```

---

`vix verify` ensures package integrity and reproducible distribution.

