# vix uninstall

`vix uninstall` removes the Vix CLI or a globally installed Vix package.

Use it when you want to remove the Vix binary from your machine, remove a global package, or purge local Vix store and cache data.

```bash
vix uninstall
```

## Overview

`vix uninstall` has two modes:

```txt
CLI uninstall mode
global package uninstall mode
```

CLI uninstall mode removes the Vix binary:

```bash
vix uninstall
```

Global package uninstall mode removes one globally installed package:

```bash
vix uninstall -g gk/jwt
```

The command can also remove local Vix store/cache data with:

```bash
vix uninstall --purge
```

## Usage

```bash
vix uninstall [options]
vix uninstall -g <package>
```

## Basic examples

```bash
# Remove detected Vix CLI binary
vix uninstall

# Remove Vix CLI binary and local store/cache
vix uninstall --purge

# Remove every detected Vix binary
vix uninstall --all

# Include system locations such as /usr/local/bin and /usr/bin
vix uninstall --all --system

# Remove binary from a custom prefix
vix uninstall --prefix /usr/local

# Remove an explicit binary path
vix uninstall --path /usr/local/bin/vix

# Remove a globally installed package
vix uninstall -g gk/jwt
```

## Modes

| Mode                | Command                  | Purpose                                           |
| ------------------- | ------------------------ | ------------------------------------------------- |
| CLI mode            | `vix uninstall`          | Remove the detected Vix CLI binary.               |
| CLI purge mode      | `vix uninstall --purge`  | Remove Vix CLI binary and local store/cache data. |
| Global package mode | `vix uninstall -g <pkg>` | Remove one globally installed package.            |

## CLI uninstall mode

Run:

```bash
vix uninstall
```

Vix tries to find the installed CLI binary, remove it, remove its install metadata, then print a shell cache tip.

Example output shape:

```txt
Uninstall
✔ Removed binary: /home/user/.local/bin/vix
✔ Removed install.json
ℹ You may need to run: hash -r (bash/zsh)
✔ Uninstall complete.
⚠ Tip: run: hash -r (bash/zsh) or restart your terminal.
```

## How Vix finds the CLI binary

Vix builds a list of candidate binary paths.

It can use:

```txt
explicit --path
install.json install_dir
VIX_CLI_PATH
command -v vix
explicit --prefix
system paths when --system or --all is used
```

Then it removes the first matching binary by default.

If `--all` is used, it tries to remove every detected candidate.

## Candidate path order

The candidate order is:

```txt
1. --path <file>
2. install.json install_dir + /vix
3. VIX_CLI_PATH
4. command -v vix
5. --prefix <dir>/bin/vix
6. system paths when --system or --all is used
```

On Linux, system paths include:

```txt
/usr/local/bin/vix
/usr/bin/vix
```

## Explicit path uninstall

Use `--path` when you know the exact binary to remove.

```bash
vix uninstall --path /usr/local/bin/vix
```

This removes only that file.

Vix refuses to remove directories.

If the path is a directory, it reports:

```txt
refusing to remove directory: /path
```

## Prefix uninstall

Use `--prefix` when Vix was installed under a custom prefix.

```bash
vix uninstall --prefix /usr/local
```

This targets:

```txt
/usr/local/bin/vix
```

On Windows, the binary name is:

```txt
vix.exe
```

## Remove all detected binaries

Use:

```bash
vix uninstall --all
```

This tells Vix to keep trying candidate paths instead of stopping after the first removed binary.

Use this when you have multiple Vix binaries in different locations.

Example:

```bash
vix uninstall --all
```

## Include system locations

Use:

```bash
vix uninstall --all --system
```

This includes common system locations such as:

```txt
/usr/local/bin/vix
/usr/bin/vix
```

If Vix cannot remove a system binary because of permissions, it can suggest a command such as:

```bash
sudo rm -f /usr/local/bin/vix
```

## install.json

Vix removes install metadata when uninstalling the CLI.

On Linux, the install metadata path is:

```txt
~/.local/share/vix/install.json
```

On Windows, the install metadata path is under:

```txt
%LOCALAPPDATA%\Vix\install.json
```

This file can contain the install directory used to detect the CLI binary.

## Purge local store and cache

Use:

```bash
vix uninstall --purge
```

This removes the CLI binary and the local Vix store/cache.

On Linux, the store/cache root is:

```txt
~/.vix
```

On Windows, the store/cache root is:

```txt
%LOCALAPPDATA%\Vix\store
```

Example output shape:

```txt
Uninstall
✔ Removed binary: /home/user/.local/bin/vix
✔ Removed install.json
✔ Purged local store/cache
ℹ You may need to run: hash -r (bash/zsh)
✔ Uninstall complete.
```

## What `--purge` removes

`--purge` removes local Vix data such as:

```txt
registry index
global packages
package store
cache data
local Vix metadata
```

On Linux, this means:

```txt
~/.vix
```

Use `--purge` only when you want to remove Vix data, not just the binary.

## What normal uninstall removes

Normal uninstall removes:

```txt
Vix CLI binary
install.json
```

It does not remove:

```txt
~/.vix
global packages
registry index
package store/cache
```

Use `--purge` when you want those removed too.

## Shell command cache

After removing the binary, your shell may still remember the old path.

Vix prints:

```bash
hash -r
```

Run it in Bash or Zsh:

```bash
hash -r
```

Or restart your terminal.

This clears the shell command cache.

## PATH post-check

After uninstalling, Vix checks whether `vix` is still found in `PATH`.

If it still exists, Vix warns:

```txt
Still found in PATH: /some/path/vix
```

That means another Vix binary still exists.

Use:

```bash
which vix
```

or:

```bash
command -v vix
```

Then remove the remaining binary:

```bash
vix uninstall --path /some/path/vix
```

or:

```bash
vix uninstall --all --system
```

## Global package uninstall mode

Use:

```bash
vix uninstall -g gk/jwt
```

or:

```bash
vix uninstall --global gk/jwt
```

This removes one globally installed package.

It reads the global manifest:

```txt
~/.vix/global/installed.json
```

Then it removes the package entry and deletes the installed package path.

## Global package manifest

Global packages are recorded in:

```txt
~/.vix/global/installed.json
```

The manifest contains a `packages` array.

Example shape:

```json
{
  "packages": [
    {
      "id": "gk/jwt",
      "version": "1.0.0",
      "installed_path": "/home/user/.vix/global/packages/gk.jwt"
    }
  ]
}
```

When you run:

```bash
vix uninstall -g gk/jwt
```

Vix finds the package with:

```txt
id = gk/jwt
```

Then it removes:

```txt
installed_path
```

and updates the manifest.

## Global package output

Example:

```txt
✔ Removed: /home/user/.vix/global/packages/gk.jwt
✔ Uninstalled gk/jwt
```

If no global manifest exists:

```txt
No global packages installed.
```

If the package is not found:

```txt
Package not found: gk/jwt
```

## Global package id format

Use the package id stored in the global manifest.

Example:

```txt
gk/jwt
```

Run:

```bash
vix list -g
```

to inspect globally installed packages before uninstalling.

## Difference between CLI uninstall and global package uninstall

| Command                  | Removes                                    |
| ------------------------ | ------------------------------------------ |
| `vix uninstall`          | Vix CLI binary.                            |
| `vix uninstall --purge`  | Vix CLI binary and local store/cache data. |
| `vix uninstall -g <pkg>` | One globally installed package.            |

## Difference between `vix remove` and `vix uninstall`

| Command                  | Purpose                                                |
| ------------------------ | ------------------------------------------------------ |
| `vix remove <pkg>`       | Remove a dependency from the current project lockfile. |
| `vix uninstall -g <pkg>` | Remove a globally installed package.                   |
| `vix uninstall`          | Remove the Vix CLI binary.                             |

Use `vix remove` for project dependencies.

Use `vix uninstall -g` for global packages.

Use `vix uninstall` for the CLI itself.

## Full CLI uninstall workflow

Inspect current setup:

```bash
vix info
which vix
vix --version
```

Uninstall:

```bash
vix uninstall
```

Clear shell cache:

```bash
hash -r
```

Verify:

```bash
command -v vix
```

If still found:

```bash
vix uninstall --all --system
hash -r
```

## Full purge workflow

Use this when you want to remove Vix and local Vix data:

```bash
vix uninstall --purge
hash -r
```

This removes the CLI binary, install metadata, and store/cache data.

## Remove custom installation

If Vix was installed under a custom prefix:

```bash
vix uninstall --prefix /opt/vix
```

This targets:

```txt
/opt/vix/bin/vix
```

If you know the exact binary:

```bash
vix uninstall --path /opt/vix/bin/vix
```

## Remove system installation

If Vix is installed in a system path:

```bash
vix uninstall --all --system
```

If permission is denied, Vix may suggest:

```bash
sudo rm -f /usr/local/bin/vix
```

Then clear shell cache:

```bash
hash -r
```

## Remove global package

```bash
vix uninstall -g gk/jwt
```

Check:

```bash
vix list -g
```

If the package is gone, uninstall succeeded.

## Options

| Option           | Description                                                       |
| ---------------- | ----------------------------------------------------------------- |
| `-g, --global`   | Remove a globally installed package.                              |
| `--purge`        | Remove local store/cache as well.                                 |
| `--all`          | Try to remove every detected Vix binary in candidate paths.       |
| `--system`       | Include system locations such as `/usr/local/bin` and `/usr/bin`. |
| `--prefix <dir>` | Remove `<dir>/bin/vix`.                                           |
| `--path <file>`  | Remove the binary at an explicit path.                            |
| `-h, --help`     | Show command help.                                                |

## Commands reference

| Command                                   | Description                                             |
| ----------------------------------------- | ------------------------------------------------------- |
| `vix uninstall`                           | Remove detected Vix CLI binary.                         |
| `vix uninstall --purge`                   | Remove CLI and local store/cache.                       |
| `vix uninstall --all`                     | Remove every detected Vix binary candidate.             |
| `vix uninstall --all --system`            | Remove every detected candidate including system paths. |
| `vix uninstall --prefix /usr/local`       | Remove `/usr/local/bin/vix`.                            |
| `vix uninstall --path /usr/local/bin/vix` | Remove explicit binary path.                            |
| `vix uninstall -g gk/jwt`                 | Remove one global package.                              |

## Common workflows

### Remove Vix CLI

```bash
vix uninstall
hash -r
```

### Remove Vix CLI and cache/store

```bash
vix uninstall --purge
hash -r
```

### Remove every detected binary

```bash
vix uninstall --all
hash -r
```

### Remove system binaries too

```bash
vix uninstall --all --system
hash -r
```

### Remove a custom prefix install

```bash
vix uninstall --prefix /usr/local
hash -r
```

### Remove an explicit binary

```bash
vix uninstall --path /usr/local/bin/vix
hash -r
```

### Remove a global package

```bash
vix uninstall -g gk/jwt
```

## Common mistakes

### Using uninstall for project dependencies

Wrong:

```bash
vix uninstall -g gk/jwt
```

when you mean the current project dependency.

Correct:

```bash
vix remove gk/jwt
vix install
```

### Using remove for global packages

Wrong:

```bash
vix remove gk/jwt
```

when you mean a global package.

Correct:

```bash
vix uninstall -g gk/jwt
```

### Using `--purge` when you only want to remove the binary

Wrong:

```bash
vix uninstall --purge
```

if you want to keep registry, cache, and global packages.

Correct:

```bash
vix uninstall
```

### Forgetting shell cache

After uninstalling, run:

```bash
hash -r
```

or restart your terminal.

### Removing only one binary when multiple exist

If Vix says:

```txt
Still found in PATH: /some/path/vix
```

remove the remaining binary:

```bash
vix uninstall --all --system
```

or:

```bash
vix uninstall --path /some/path/vix
```

### Expecting `--system` to work without permissions

System paths may require elevated permissions.

If Vix cannot remove the file, use the suggested `sudo rm -f ...` command.

### Passing `-g` without a package

Wrong:

```bash
vix uninstall -g
```

Correct:

```bash
vix uninstall -g gk/jwt
```

## Troubleshooting

### No candidate paths found

Vix could not detect the CLI binary.

Try:

```bash
which vix
command -v vix
```

If you know the path:

```bash
vix uninstall --path /path/to/vix
```

### Uninstall finished but nothing removed

This means Vix did not find a removable binary at the candidate paths.

Check:

```bash
command -v vix
```

If a path appears, remove it explicitly:

```bash
vix uninstall --path /that/path
```

### Still found in PATH

Run:

```bash
command -v vix
```

Then remove the returned path:

```bash
vix uninstall --path /returned/path
```

Clear shell cache:

```bash
hash -r
```

### Permission denied

If the binary is in a system path, use:

```bash
sudo rm -f /usr/local/bin/vix
```

Then:

```bash
hash -r
```

### Missing HOME or LOCALAPPDATA

Vix needs a home directory to locate install metadata and store paths.

On Linux/macOS, make sure:

```bash
echo "$HOME"
```

is set.

On Windows, make sure:

```powershell
echo $env:LOCALAPPDATA
```

is set.

### No global packages installed

If:

```bash
vix uninstall -g gk/jwt
```

prints:

```txt
No global packages installed.
```

then the global manifest does not exist.

There is nothing to remove globally.

### Invalid global manifest

If Vix reports:

```txt
Invalid global manifest.
```

the global install file is malformed.

Check:

```txt
~/.vix/global/installed.json
```

You may need to reinstall or manually clean the corrupted global state.

### Global package not found

If Vix reports:

```txt
Package not found: gk/jwt
```

check installed global packages:

```bash
vix list -g
```

Then use the exact package id shown there.

## Best practices

Run `vix info` before uninstalling if you are unsure where Vix is installed.

Use `vix uninstall` when you only want to remove the binary.

Use `vix uninstall --purge` only when you want to remove local Vix data too.

Use `vix uninstall --all --system` when multiple binaries exist.

Use `--path` when you know the exact binary path.

Run `hash -r` after uninstalling on Bash or Zsh.

Use `vix uninstall -g <pkg>` only for global packages.

Use `vix remove <pkg>` for project dependencies.

## Related commands

| Command          | Purpose                                                          |
| ---------------- | ---------------------------------------------------------------- |
| `vix upgrade`    | Upgrade the Vix CLI or a global package.                         |
| `vix install -g` | Install a global package.                                        |
| `vix list -g`    | List global packages.                                            |
| `vix remove`     | Remove a project dependency.                                     |
| `vix clean`      | Clean project-local generated state.                             |
| `vix reset`      | Clean and reinstall project dependencies.                        |
| `vix info`       | Inspect Vix paths and caches.                                    |
| `vix doctor`     | Diagnose environment health.                                     |
| `vix install`    | Reinstall project dependencies after project dependency changes. |

## Next step

Continue with shell completion.

[Open the vix completion guide](/cli/completion)
