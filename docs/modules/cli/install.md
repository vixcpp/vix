# vix install

Install a Vix package into the local store.

---

## Usage

```bash
vix install --path <folder|artifact.vixpkg> [options]
```

---

## Description

`vix install`:

- Installs a packaged project into the local Vix store
- Verifies payload digest and checksums
- Verifies signature (if present)
- Copies package into deterministic store layout

By default, verification is enabled.

---

## Options

```bash
-p, --path <path>          Package folder or .vixpkg artifact (required)
--store <dir>              Override store root
--force                    Overwrite if already installed
--no-verify                Skip verification (NOT recommended)
--verbose                  Print detailed checks and copied files
--require-signature        Fail if signature missing or invalid
--pubkey <path>            minisign public key
-h, --help                 Show help
```

---

## Store Location

Default resolution order:

1. `VIX_STORE`
2. `XDG_DATA_HOME/vix`
3. `~/.local/share/vix`

Final layout:

```
<store>/packs/<name>/<version>/<os>-<arch>/
```

---

## Examples

Install archive:

```bash
vix install --path ./dist/blog@1.0.0.vixpkg
```

Install folder package:

```bash
vix install --path ./dist/blog@1.0.0
```

Force overwrite:

```bash
vix install --path ./dist/blog@1.0.0 --force
```

Require signature verification:

```bash
vix install --path ./blog@1.0.0.vixpkg --require-signature --pubkey ./keys/vix-pack.pub
```

---

## Security Notes

- Verification runs by default
- `--no-verify` disables integrity checks (not recommended)
- `--require-signature` enforces cryptographic verification
- Public key resolution:
  - `--pubkey`
  - `VIX_MINISIGN_PUBKEY`

---

## Typical Workflow

```bash
vix pack --version 1.0.0
vix verify --require-signature
vix install --path ./dist/blog@1.0.0.vixpkg
```

---

`vix install` moves verified packages into the reproducible Vix store.

