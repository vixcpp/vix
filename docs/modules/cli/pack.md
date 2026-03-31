# vix pack

Package a Vix project into a distributable artifact.

---

## Usage

```bash
vix pack [options]
```

---

## Description

`vix pack`:

- Creates `dist/<name>@<version>`
- Generates a manifest v2 package
- Optionally creates a `.vixpkg` archive
- Optionally signs metadata using minisign

Default output:

```
<project>/dist/
```

---

## Options

```bash
-d, --dir <path>        Project directory (default: current directory)
--out <path>            Output directory (default: <project>/dist)
--name <name>           Package name (default: project folder name)
--version <ver>         Package version (default: 0.1.0)
--no-zip                Do not create .vixpkg (folder only)
--no-hash               Do not generate checksums.sha256
--verbose               Show copied files + signing output
--sign[=mode]           auto | never | required (default: auto)
-h, --help              Show help
```

---

## Basic Example

```bash
vix pack
```

Explicit name and version:

```bash
vix pack --name blog --version 1.0.0
```

Disable archive:

```bash
vix pack --no-zip
```

---

## Signing (Optional)

Environment:

```bash
VIX_MINISIGN_SECKEY=path
```

Example:

```bash
VIX_MINISIGN_SECKEY=./keys/vix-pack.key vix pack --sign
```

Modes:

- auto      Sign if key exists
- never     Disable signing
- required  Fail if signing is not possible

---

## What Gets Generated

Inside `dist/<name>@<version>`:

- Manifest v2 metadata
- Payload files
- Optional `checksums.sha256`
- Optional minisign signature

Optional archive:

```
dist/<name>@<version>.vixpkg
```

---

## Typical Workflow

```bash
vix build --preset release
vix pack --version 1.0.0
vix verify dist/blog@1.0.0
```

---

`vix pack` prepares your project for reproducible distribution.

