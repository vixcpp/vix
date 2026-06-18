# vix upgrade

`vix upgrade` upgrades the Vix CLI or a globally installed package.

Use it when you want to update Vix itself from GitHub Releases, check the latest available CLI release, simulate an upgrade, or upgrade one package installed globally.

```bash
vix upgrade
```

## Overview

`vix upgrade` has two modes:

```txt
CLI upgrade mode
global package upgrade mode
```

CLI upgrade mode upgrades the Vix binary itself:

```bash
vix upgrade
```

Global package upgrade mode upgrades one globally installed registry package:

```bash
vix upgrade -g gk/jwt
```

These two modes are different.

CLI upgrade uses GitHub Releases.

Global package upgrade uses the Vix Registry and the global package manifest.

## Usage

```bash
vix upgrade
vix upgrade vX.Y.Z
vix upgrade --check
vix upgrade --dry-run
vix upgrade --json
vix upgrade -g [@]namespace/name[@version]
```

## Basic examples

```bash
# Upgrade Vix CLI to the latest release
vix upgrade

# Upgrade Vix CLI to a specific release
vix upgrade v2.6.0

# Check target release without downloading or installing
vix upgrade --check

# Simulate upgrade without downloading or installing
vix upgrade --dry-run

# Print machine-readable output
vix upgrade --json

# Upgrade a globally installed package
vix upgrade -g gk/jwt

# Upgrade a globally installed package to a specific version
vix upgrade -g gk/jwt@1.0.0

# Scoped-style package syntax
vix upgrade -g @gk/jwt
```

## Modes

| Mode                | Command                 | Purpose                                                |
| ------------------- | ----------------------- | ------------------------------------------------------ |
| CLI mode            | `vix upgrade`           | Upgrade the Vix CLI from GitHub Releases.              |
| CLI check mode      | `vix upgrade --check`   | Show target release and asset info without installing. |
| CLI dry-run mode    | `vix upgrade --dry-run` | Simulate upgrade without downloading or installing.    |
| Global package mode | `vix upgrade -g <pkg>`  | Upgrade one globally installed registry package.       |

## CLI upgrade mode

Run:

```bash
vix upgrade
```

Vix resolves the latest release from GitHub, downloads the correct asset for your OS and CPU architecture, verifies the archive, extracts the new binary, and replaces the installed CLI binary.

The default repository is:

```txt
vixcpp/vix
```

## Specific CLI version

To upgrade to a specific version, pass the full tag name:

```bash
vix upgrade v2.6.0
```

The version must include the `v` prefix.

Correct:

```bash
vix upgrade v2.6.0
```

Wrong:

```bash
vix upgrade 2.6.0
```

CLI releases are GitHub release tags, so the expected format is:

```txt
vX.Y.Z
```

## Latest CLI version

When no version is passed:

```bash
vix upgrade
```

Vix queries the latest GitHub release from:

```txt
https://api.github.com/repos/vixcpp/vix/releases/latest
```

Then it reads the release `tag_name`.

If Vix cannot resolve the latest tag, pass an explicit version:

```bash
vix upgrade v2.6.0
```

## Override release repository

Use `VIX_REPO` to override the GitHub repository used for CLI upgrades.

```bash
VIX_REPO=vixcpp/vix vix upgrade --check
```

Default:

```txt
vixcpp/vix
```

This is useful for testing release assets from another repository.

## Platform detection

`vix upgrade` detects:

```txt
os
arch
```

Supported OS values include:

```txt
linux
macos
windows
```

Supported CPU architecture values include:

```txt
x86_64
aarch64
```

If the architecture is unknown, Vix reports:

```txt
unsupported cpu arch for upgrade
```

## Release asset naming

Vix downloads a platform-specific release asset.

On Linux and macOS:

```txt
vix-<os>-<arch>.tar.gz
```

Examples:

```txt
vix-linux-x86_64.tar.gz
vix-linux-aarch64.tar.gz
vix-macos-x86_64.tar.gz
vix-macos-aarch64.tar.gz
```

On Windows:

```txt
vix-windows-<arch>.zip
```

Example:

```txt
vix-windows-x86_64.zip
```

## Download URL

For release:

```txt
v2.6.0
```

on Linux x86_64, Vix downloads:

```txt
https://github.com/vixcpp/vix/releases/download/v2.6.0/vix-linux-x86_64.tar.gz
```

The SHA256 file is expected at:

```txt
https://github.com/vixcpp/vix/releases/download/v2.6.0/vix-linux-x86_64.tar.gz.sha256
```

The optional minisign signature is expected at:

```txt
https://github.com/vixcpp/vix/releases/download/v2.6.0/vix-linux-x86_64.tar.gz.minisig
```

## Check mode

Use:

```bash
vix upgrade --check
```

Check mode resolves the target release and asset information but does not download or install anything.

It prints information such as:

```txt
repo
os
arch
exe
install_dir
target
asset
url
download_size
```

Example output shape:

```txt
Upgrade
repo: vixcpp/vix
os: linux
arch: x86_64
exe: vix
install_dir: /home/user/.local/bin
target: v2.6.0
asset: vix-linux-x86_64.tar.gz
url: https://github.com/vixcpp/vix/releases/download/v2.6.0/vix-linux-x86_64.tar.gz
download_size: 12.30 MB (12897433 bytes)
✔ check mode: no download, no install
```

## Dry run

Use:

```bash
vix upgrade --dry-run
```

Dry run is similar to check mode.

It resolves the target and asset information but does not download or install anything.

Example:

```bash
vix upgrade v2.6.0 --dry-run
```

Expected result:

```txt
dry-run: no download, no install
```

## JSON output

Use:

```bash
vix upgrade --json
```

Example output shape:

```json
{
  "command": "upgrade",
  "mode": "cli",
  "check": false,
  "dry_run": false,
  "repo": "vixcpp/vix",
  "os": "linux",
  "arch": "x86_64",
  "exe": "vix",
  "install_dir": "/home/user/.local/bin",
  "target": "v2.6.0",
  "asset": "vix-linux-x86_64.tar.gz",
  "url": "https://github.com/vixcpp/vix/releases/download/v2.6.0/vix-linux-x86_64.tar.gz",
  "download_size_bytes": 12897433,
  "minisign_verified": true,
  "status": "ok",
  "action": "upgrade",
  "installed": "v2.6.0",
  "message": "done"
}
```

Use JSON output for:

- scripts
- CI
- automation
- install dashboards
- release validation

## JSON check mode

```bash
vix upgrade --check --json
```

Example output shape:

```json
{
  "command": "upgrade",
  "mode": "cli",
  "check": true,
  "dry_run": false,
  "repo": "vixcpp/vix",
  "os": "linux",
  "arch": "x86_64",
  "target": "v2.6.0",
  "asset": "vix-linux-x86_64.tar.gz",
  "status": "ok",
  "action": "check",
  "message": "check mode: no download, no install"
}
```

## Current binary detection

Vix tries to determine the current executable path from:

```txt
VIX_CLI_PATH
```

If `VIX_CLI_PATH` is not set, it uses:

```txt
vix
```

or on Windows:

```txt
vix.exe
```

The install directory is guessed from the current executable path.

If needed, set:

```bash
VIX_CLI_PATH=/home/user/.local/bin/vix vix upgrade
```

## Install directory

Vix installs the upgraded binary into the detected install directory.

Default fallback on Linux/macOS:

```txt
~/.local/bin
```

Default fallback on Windows:

```txt
%LOCALAPPDATA%\Vix\bin
```

Before installing, Vix checks whether the install directory is writable.

If not writable, it reports:

```txt
install_dir is not writable: <path>
```

## Already installed behavior

If the installed version already matches the target version, Vix does not reinstall.

Example:

```txt
already installed: v2.6.0
```

It still updates installation metadata.

## Download verification

CLI upgrade verifies the downloaded archive.

The verification flow is:

```txt
download release archive
download .sha256 file
compute local sha256
compare expected and actual hash
optionally verify minisign if minisign is installed
```

SHA256 verification is required.

Minisign verification is best effort.

## SHA256 verification

Vix downloads:

```txt
<asset>.sha256
```

Then computes the local archive hash.

If the hashes do not match, Vix fails with an error like:

```txt
sha256 mismatch: expected <expected>, got <actual>
```

This protects against corrupted downloads.

## Minisign verification

If `minisign` is installed, Vix tries to verify:

```txt
<asset>.minisig
```

with the built-in public key.

If minisign succeeds, Vix prints:

```txt
minisign ok
```

If minisign is not available or the signature file cannot be verified, Vix warns:

```txt
minisig not found (sha256 already verified)
```

This warning does not fail the upgrade because SHA256 has already been checked.

## Extraction

On Linux and macOS, Vix extracts:

```txt
.tar.gz
```

using:

```txt
tar
```

On Windows, Vix extracts:

```txt
.zip
```

using PowerShell `Expand-Archive`.

If extraction fails, Vix reports:

```txt
failed to extract archive
```

## Binary replacement on Unix

On Linux and macOS, Vix stages the new binary and replaces the current binary.

The replacement flow is:

```txt
copy new binary to temporary file
chmod +x temporary file
rename temporary file over destination binary
fallback to copy if rename fails
```

After success, Vix prints:

```txt
installed: v2.6.0
done
```

## Binary replacement on Windows

On Windows, the running executable cannot always replace itself immediately.

Vix stages the new binary as:

```txt
vix.exe.new
```

Then schedules replacement after the current process exits.

Example output shape:

```txt
staged: C:\Users\user\AppData\Local\Vix\bin\vix.exe.new
Windows: replacing vix.exe after this process exits.
upgrade scheduled. reopen your terminal.
```

After that, reopen the terminal and run:

```powershell
vix --version
```

## Install metadata

After a successful CLI upgrade, Vix writes install metadata.

On Linux/macOS:

```txt
~/.local/share/vix/install.json
```

On Windows:

```txt
%LOCALAPPDATA%\Vix\install.json
```

The file stores information like:

```json
{
  "repo": "vixcpp/vix",
  "version": "v2.6.0",
  "installed_version": "v2.6.0",
  "installed_at": "2026-05-28T12:00:00Z",
  "os": "linux",
  "arch": "x86_64",
  "install_dir": "/home/user/.local/bin",
  "download_bytes": 12897433,
  "asset_url": "https://github.com/vixcpp/vix/releases/download/v2.6.0/vix-linux-x86_64.tar.gz"
}
```

## Global package upgrade

Use:

```bash
vix upgrade -g gk/jwt
```

or:

```bash
vix upgrade --global gk/jwt
```

Global package upgrade uses the local Vix Registry index and the global package manifest.

It does not upgrade project dependencies.

It does not rewrite `vix.lock`.

It only works for packages already installed globally.

## Global package examples

```bash
# Upgrade to latest registry version
vix upgrade -g gk/jwt

# Upgrade to specific version
vix upgrade -g gk/jwt@1.0.0

# Scoped-style syntax
vix upgrade -g @gk/jwt

# Check only
vix upgrade -g gk/jwt --check

# Dry run
vix upgrade -g gk/jwt --dry-run

# JSON output
vix upgrade -g gk/jwt --json
```

## Global package registry requirement

Global package upgrade requires the local registry index.

If it is missing, Vix reports:

```txt
registry not synced
Run: vix registry sync
```

Fix:

```bash
vix registry sync
vix upgrade -g gk/jwt
```

## Global package must already be installed

`vix upgrade -g <pkg>` upgrades an existing global package.

If the package is not installed globally, Vix reports:

```txt
global package is not installed: gk/jwt
```

Install it first:

```bash
vix install -g gk/jwt
```

Then upgrade later:

```bash
vix upgrade -g gk/jwt
```

## Global package manifest

Global packages are tracked in:

```txt
~/.vix/global/installed.json
```

Example shape:

```json
{
  "packages": [
    {
      "id": "gk/jwt",
      "version": "1.0.0",
      "repo": "https://github.com/gk/jwt",
      "tag": "v1.0.0",
      "commit": "...",
      "hash": "...",
      "type": "header-only",
      "include": "include",
      "installed_path": "/home/user/.vix/global/packages/gk.jwt/..."
    }
  ]
}
```

## Global package storage

Global packages are installed under:

```txt
~/.vix/global/packages/
```

Package source checkouts are stored under:

```txt
~/.vix/store/git/
```

For package:

```txt
gk/jwt
```

Vix uses a dot-safe folder form:

```txt
gk.jwt
```

## Global package upgrade flow

When you run:

```bash
vix upgrade -g gk/jwt
```

Vix does this:

1. Checks the registry index.
2. Resolves the package and target version.
3. Reads the global installed package manifest.
4. Confirms the package is already installed globally.
5. Compares current commit with target commit.
6. Fetches the package checkout if needed.
7. Verifies the dependency hash when available.
8. Reads package metadata from `vix.json`.
9. Installs the upgraded package under `~/.vix/global/packages/`.
10. Updates `~/.vix/global/installed.json`.
11. Removes the old installed path when replaced.

## Global package check mode

Use:

```bash
vix upgrade -g gk/jwt --check
```

Example output shape:

```txt
Upgrade global package
package: gk/jwt
current: 1.0.0
target: 1.1.0
✔ check mode: no install
```

## Global package dry run

Use:

```bash
vix upgrade -g gk/jwt --dry-run
```

Example output shape:

```txt
Upgrade global package
package: gk/jwt
current: 1.0.0
target: 1.1.0
✔ dry-run: no install
```

## Global package already up to date

If the currently installed global package commit equals the resolved target commit, Vix reports:

```txt
Global package already up to date
```

No reinstall is needed.

## Global package JSON output

```bash
vix upgrade -g gk/jwt --json
```

Example output shape:

```json
{
  "command": "upgrade",
  "mode": "global",
  "spec": "gk/jwt",
  "check": false,
  "dry_run": false,
  "package": "gk/jwt",
  "target_version": "1.1.0",
  "repo": "https://github.com/gk/jwt",
  "commit": "...",
  "current_version": "1.0.0",
  "current_commit": "...",
  "current_path": "/home/user/.vix/global/packages/gk.jwt/...",
  "status": "ok",
  "action": "upgrade",
  "installed_path": "/home/user/.vix/global/packages/gk.jwt/...",
  "installed_version": "1.1.0",
  "message": "done"
}
```

## Global package spec format

Accepted forms:

```txt
namespace/name
namespace/name@version
@namespace/name
@namespace/name@version
```

Examples:

```bash
vix upgrade -g gk/jwt
vix upgrade -g gk/jwt@1.0.0
vix upgrade -g @gk/jwt
```

## Missing global package spec

Wrong:

```bash
vix upgrade -g
```

Correct:

```bash
vix upgrade -g gk/jwt
```

If the package is missing, Vix reports:

```txt
missing package after -g
Example: vix upgrade -g @gk/jwt
```

## Difference between `vix upgrade` and `vix update`

| Command       | Purpose                                                |
| ------------- | ------------------------------------------------------ |
| `vix upgrade` | Upgrade the Vix CLI or one globally installed package. |
| `vix update`  | Update project dependencies and rewrite `vix.lock`.    |

Use `vix upgrade` for:

```txt
Vix CLI
global packages
```

Use `vix update` for:

```txt
project dependencies
```

## Difference between `vix upgrade -g` and `vix install -g`

| Command                | Purpose                                               |
| ---------------------- | ----------------------------------------------------- |
| `vix install -g <pkg>` | Install a global package.                             |
| `vix upgrade -g <pkg>` | Upgrade a package that is already installed globally. |

If the package is not installed globally, use:

```bash
vix install -g gk/jwt
```

## Difference between `vix upgrade` and `vix doctor --online`

| Command               | Purpose                                             |
| --------------------- | --------------------------------------------------- |
| `vix doctor --online` | Diagnose environment and check release information. |
| `vix upgrade --check` | Show the upgrade target and release asset info.     |
| `vix upgrade`         | Download, verify, extract, and install the new CLI. |

Use `doctor` for diagnostics.

Use `upgrade` to upgrade.

## Recommended CLI upgrade flow

```bash
vix upgrade --check
vix upgrade --dry-run
vix upgrade
vix --version
vix doctor
```

## Recommended global package upgrade flow

```bash
vix registry sync
vix list -g
vix upgrade -g gk/jwt --check
vix upgrade -g gk/jwt
vix list -g
```

## Options

| Option         | Description                                               |
| -------------- | --------------------------------------------------------- |
| `-g, --global` | Upgrade one globally installed package.                   |
| `--check`      | Show target version and download info without installing. |
| `--dry-run`    | Simulate the upgrade without installing.                  |
| `--json`       | Print machine-readable JSON output.                       |
| `-h, --help`   | Show command help.                                        |

## Environment variables

| Variable       | Description                                                  |
| -------------- | ------------------------------------------------------------ |
| `VIX_REPO`     | Override repository for CLI upgrades. Default: `vixcpp/vix`. |
| `VIX_CLI_PATH` | Override current Vix binary path detection.                  |

## Commands reference

| Command                       | Description                                 |
| ----------------------------- | ------------------------------------------- |
| `vix upgrade`                 | Upgrade Vix CLI to latest release.          |
| `vix upgrade v2.6.0`          | Upgrade Vix CLI to release `v2.6.0`.        |
| `vix upgrade --check`         | Check target release without installing.    |
| `vix upgrade --dry-run`       | Simulate without installing.                |
| `vix upgrade --json`          | Print JSON output.                          |
| `vix upgrade -g gk/jwt`       | Upgrade globally installed `gk/jwt`.        |
| `vix upgrade -g gk/jwt@1.0.0` | Upgrade global package to specific version. |

## Common workflows

### Check before upgrading CLI

```bash
vix upgrade --check
```

### Upgrade CLI to latest

```bash
vix upgrade
vix --version
```

### Upgrade CLI to specific version

```bash
vix upgrade v2.6.0
vix --version
```

### Use JSON output

```bash
vix upgrade --json
```

### Override repo

```bash
VIX_REPO=vixcpp/vix vix upgrade --check
```

### Upgrade global package

```bash
vix registry sync
vix upgrade -g gk/jwt
```

### Check global package upgrade

```bash
vix upgrade -g gk/jwt --check
```

## Common mistakes

### Using `vix upgrade` for project dependencies

Wrong:

```bash
vix upgrade
```

when you want to update project packages.

Correct:

```bash
vix update --install
```

### Forgetting `-g` for global packages

Wrong:

```bash
vix upgrade gk/jwt
```

This is interpreted as a CLI version argument, not a package.

Correct:

```bash
vix upgrade -g gk/jwt
```

### Passing CLI version without `v`

Wrong:

```bash
vix upgrade 2.6.0
```

Correct:

```bash
vix upgrade v2.6.0
```

### Running global upgrade before registry sync

Wrong:

```bash
vix upgrade -g gk/jwt
```

when the registry is not synced.

Correct:

```bash
vix registry sync
vix upgrade -g gk/jwt
```

### Trying to upgrade a global package that is not installed

Wrong:

```bash
vix upgrade -g gk/jwt
```

when `gk/jwt` is not globally installed.

Correct:

```bash
vix install -g gk/jwt
```

### Expecting `--check` to install

`--check` does not download or install.

Use normal upgrade when you want installation:

```bash
vix upgrade
```

### Expecting `--dry-run` to install

`--dry-run` does not download or install.

Use normal upgrade when ready:

```bash
vix upgrade
```

## Troubleshooting

### Could not resolve latest tag

Run with an explicit version:

```bash
vix upgrade v2.6.0
```

Also check internet access and GitHub availability.

### Need curl or wget

On Unix systems, Vix needs `curl` or `wget` for downloads.

Install one of them, then retry:

```bash
vix upgrade
```

### Tar is required

On Linux/macOS, Vix needs `tar` to extract `.tar.gz` assets.

Install `tar`, then retry.

### Install directory is not writable

If Vix reports:

```txt
install_dir is not writable
```

check where Vix is installed:

```bash
which vix
```

Then fix permissions or install Vix into a user-writable directory.

### SHA256 mismatch

Do not ignore SHA256 mismatch.

Retry once:

```bash
vix upgrade
```

If it continues, the release asset or checksum may be inconsistent.

### Archive does not contain binary

If Vix reports:

```txt
archive does not contain vix
```

or on Windows:

```txt
archive does not contain vix.exe
```

the release asset may be malformed.

### Windows replacement scheduled

On Windows, reopen the terminal after upgrade:

```powershell
vix --version
```

### Registry not synced for global package

Run:

```bash
vix registry sync
```

Then retry:

```bash
vix upgrade -g gk/jwt
```

### Invalid package spec or package not found

Use package format:

```txt
namespace/name
@namespace/name
namespace/name@version
```

Then sync registry:

```bash
vix registry sync
```

### Global package is not installed

Install it first:

```bash
vix install -g gk/jwt
```

Then later:

```bash
vix upgrade -g gk/jwt
```

### Integrity check failed for global package

Do not ignore it.

Run:

```bash
vix registry sync
vix upgrade -g gk/jwt
```

If it still fails, registry metadata or package content may be inconsistent.

## Best practices

Run `vix upgrade --check` before upgrading the CLI.

Use explicit release versions for reproducible upgrades.

Run `vix --version` after upgrading.

Run `vix doctor` after upgrading.

Use `vix registry sync` before upgrading global packages.

Use `vix list -g` before upgrading global packages.

Do not use `vix upgrade` for project dependency updates.

Use `vix update --install` for project dependencies.

Keep `curl`, `tar`, and SHA256 tools available on Unix systems.

Use JSON output for automation.

## Related commands

| Command               | Purpose                                           |
| --------------------- | ------------------------------------------------- |
| `vix doctor --online` | Check environment and latest release information. |
| `vix info`            | Show Vix paths and local state.                   |
| `vix update`          | Update project dependencies.                      |
| `vix install -g`      | Install a global package.                         |
| `vix list -g`         | List global packages.                             |
| `vix uninstall -g`    | Remove a global package.                          |
| `vix uninstall`       | Remove the Vix CLI.                               |
| `vix registry sync`   | Refresh local registry index for global packages. |
| `vix --version`       | Show installed Vix version.                       |

## Next step

Continue with uninstalling Vix or global packages.

[Open the vix uninstall guide](/cli/uninstall)
