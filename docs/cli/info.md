# vix info

`vix info` shows Vix environment information, package details, paths, caches, and local state.

Use it when you want to inspect where Vix stores data, verify what exists locally, or understand the current Vix setup before debugging or cleaning anything.

```bash
vix info
```

## Overview

`vix info` is a read-only inspection command.

It does not modify your project.
It does not sync the registry.
It does not install dependencies.
It does not clean caches.
It only prints information about the current Vix environment or a specific package.

`vix info` can show:

```txt
Vix CLI version
home directory
Vix root path
registry index path and state
package store path and state
global package manifest path and state
build artifact cache path and state
store package count
store commit count
global package count
store disk usage
artifact cache disk usage
project package details
global package details
```

## Usage

```bash
vix info
vix info [@]namespace/name
vix info -g [@]namespace/name
vix info --global [@]namespace/name
```

## Basic examples

```bash
# Show local Vix environment
vix info

# Show a project dependency from vix.lock
vix info softadastra/json

# Scoped-style syntax
vix info @softadastra/json

# Show a globally installed package
vix info -g softadastra/json

# Same as -g
vix info --global softadastra/json
```

## Modes

| Mode                 | Command             | Reads from                     | Purpose                                     |
| -------------------- | ------------------- | ------------------------------ | ------------------------------------------- |
| Environment mode     | `vix info`          | local Vix paths                | Show environment, paths, caches, and state. |
| Project package mode | `vix info <pkg>`    | `./vix.lock`                   | Show one project dependency.                |
| Global package mode  | `vix info -g <pkg>` | `~/.vix/global/installed.json` | Show one globally installed package.        |

## Environment mode

Run:

```bash
vix info
```

Example output shape:

```txt
Info
version: 2.6.0
home: /home/user
root: /home/user/.vix
root state: present

Environment
vix root: /home/user/.vix [present]
registry: /home/user/.vix/registry/index [present]
store: /home/user/.vix/store/git [present]
global root: /home/user/.vix/global [present]
global manifest: /home/user/.vix/global/installed.json [present]

Caches
artifact cache: /home/user/.vix/cache/build [present]
store packages: 12
store commits: 28
global packages: 3
store size: 240.52 MB
artifact size: 18.33 MB

Status
registry index: present (directory)
store cache: present (directory)
global root: present (directory)
global manifest: present (file)
artifact cache: present (directory)
globals usable: yes
registry usable: yes
store usable: yes
artifacts usable: yes

✔ Vix environment detected
```

## Environment paths

`vix info` prints the main Vix paths.

| Path                           | Purpose                                          |
| ------------------------------ | ------------------------------------------------ |
| `~/.vix`                       | Vix root directory.                              |
| `~/.vix/registry/index`        | Local registry metadata index.                   |
| `~/.vix/store/git`             | Shared Git checkout store for registry packages. |
| `~/.vix/global`                | Global package area.                             |
| `~/.vix/global/installed.json` | Global package manifest.                         |
| `~/.vix/cache/build`           | Build artifact cache root.                       |

## Vix root

The Vix root is:

```txt
~/.vix
```

On Unix-like systems, it is based on:

```txt
HOME
```

On Windows, it is based on:

```txt
USERPROFILE
```

If the home directory cannot be detected, Vix falls back to:

```txt
.vix
```

relative to the current directory.

## Registry state

The registry path is:

```txt
~/.vix/registry/index
```

`vix info` shows whether it is present or missing.

If the registry is missing, run:

```bash
vix registry sync
```

The registry is used by commands like:

```bash
vix search
vix add
vix update
vix outdated
vix publish
vix unpublish
```

## Store state

The package store path is:

```txt
~/.vix/store/git
```

This is where Vix keeps fetched package Git checkouts.

`vix info` counts:

```txt
store packages
store commits
store size
```

A package like:

```txt
softadastra/core
```

can be stored as:

```txt
~/.vix/store/git/softadastra.core/<commit>
```

## Global package state

Global package data lives under:

```txt
~/.vix/global
```

The global package manifest is:

```txt
~/.vix/global/installed.json
```

`vix info` counts packages from this manifest.

If no manifest exists, the global package count is:

```txt
0
```

Use:

```bash
vix list -g
```

to list global packages.

## Artifact cache state

The build artifact cache path is:

```txt
~/.vix/cache/build
```

`vix info` shows:

```txt
artifact cache path
artifact cache state
artifact size
artifacts usable
```

This is useful when debugging cache behavior or disk usage.

## Disk usage

`vix info` computes disk usage for:

```txt
~/.vix/store/git
~/.vix/cache/build
```

It prints human-readable sizes such as:

```txt
240.52 MB
18.33 MB
```

If a path is missing, its size is shown as:

```txt
0 B
```

## Status section

The status section shows whether each path is present and what type it is.

Example:

```txt
registry index: present (directory)
global manifest: present (file)
artifact cache: missing (path)
```

Possible type labels include:

```txt
directory
file
path
```

## Usable flags

`vix info` prints simple yes/no state:

```txt
globals usable
registry usable
store usable
artifacts usable
```

These are based on path existence.

Example:

```txt
registry usable: yes
store usable: no
```

If `registry usable` is `no`, run:

```bash
vix registry sync
```

If `store usable` is `no`, package commands may create it when needed.

## Project package info

Use:

```bash
vix info softadastra/json
```

This reads package details from the current project lockfile:

```txt
./vix.lock
```

Example output shape:

```txt
Project package
id: softadastra/json
version: 1.0.0
commit: 8f3a9c4...
repo: https://github.com/softadastra/json
type: header-only
include: include

✔ Package info loaded
```

## Project package requirement

Project package info requires:

```txt
vix.lock
```

in the current directory.

If it is missing, Vix reports:

```txt
project lock file not found
```

Run from the project directory:

```bash
cd /path/to/project
vix info softadastra/json
```

If the project has dependencies but no lockfile, resolve them:

```bash
vix registry sync
vix add softadastra/json
vix install
```

## Supported project lockfile shapes

`vix info <package>` can read package arrays from these shapes.

Array shape:

```json
[
  {
    "id": "softadastra/json",
    "version": "1.0.0",
    "commit": "8f3a9c4..."
  }
]
```

Object shape with `dependencies`:

```json
{
  "dependencies": [
    {
      "id": "softadastra/json",
      "version": "1.0.0",
      "repo": "https://github.com/softadastra/json",
      "tag": "v1.0.0",
      "commit": "8f3a9c4..."
    }
  ]
}
```

Object shape with `packages` is also accepted by the shared package reader.

## Global package info

Use:

```bash
vix info -g softadastra/json
```

or:

```bash
vix info --global softadastra/json
```

This reads package details from:

```txt
~/.vix/global/installed.json
```

Example output shape:

```txt
Global package
id: softadastra/json
version: 1.0.0
commit: 8f3a9c4...
repo: https://github.com/softadastra/json
type: header-only
include: include
path: /home/user/.vix/global/packages/softadastra.json/8f3a9c4...

✔ Package info loaded
```

## Package fields

For package info, Vix can print:

| Field     | Meaning                                     |
| --------- | ------------------------------------------- |
| `id`      | Package id, for example `softadastra/json`. |
| `version` | Package version.                            |
| `commit`  | Resolved Git commit.                        |
| `repo`    | Source repository.                          |
| `type`    | Package type when available.                |
| `include` | Include directory when available.           |
| `path`    | Installed path when available.              |

The `path` field usually appears for global packages because global installs store an `installed_path`.

## Version fallback

If a package does not have a `version` field but has a `tag`, Vix derives the version from the tag.

Example:

```json
{
  "tag": "v1.2.0"
}
```

is displayed as:

```txt
1.2.0
```

## Repository field

The repository can be stored as a string:

```json
{
  "repo": "https://github.com/softadastra/json"
}
```

or as an object:

```json
{
  "repo": {
    "url": "https://github.com/softadastra/json"
  }
}
```

`vix info` supports both.

## Scoped-style package ids

The help allows scoped-style input:

```bash
vix info @softadastra/json
vix info -g @softadastra/json
```

The stored package id is still normally:

```txt
softadastra/json
```

Use the package id exactly as it appears in `vix.lock` or `vix list` if lookup fails.

## Read-only behavior

`vix info` is safe to run.

It does not:

```txt
edit vix.json
edit vix.lock
create .vix/deps
sync the registry
install packages
delete cache files
```

Use it before cleanup commands when you want to understand the current state.

## Difference between `vix info` and `vix doctor`

| Command      | Purpose                                               |
| ------------ | ----------------------------------------------------- |
| `vix info`   | Show paths, caches, counts, and package details.      |
| `vix doctor` | Check environment health and diagnose setup problems. |

Use `vix info` to inspect.

Use `vix doctor` to diagnose.

## Difference between `vix info` and `vix list`

| Command    | Purpose                                         |
| ---------- | ----------------------------------------------- |
| `vix info` | Show environment details or one package detail. |
| `vix list` | List all project or global packages.            |

Use `vix list` when you want the full dependency list.

Use `vix info <pkg>` when you want details for one package.

## Difference between `vix info` and `vix registry path`

| Command             | Purpose                             |
| ------------------- | ----------------------------------- |
| `vix info`          | Show all main Vix paths and states. |
| `vix registry path` | Show only the registry path.        |

Use `vix info` for a broad overview.

Use `vix registry path` for the registry location only.

## Difference between `vix info` and `vix store path`

| Command          | Purpose                                   |
| ---------------- | ----------------------------------------- |
| `vix info`       | Show store path, state, size, and counts. |
| `vix store path` | Show only the store path.                 |

Use `vix info` when you need store size and state.

Use `vix store path` when you only need the path.

## Debugging workflows

### Inspect environment

```bash
vix info
```

### Registry problem

```bash
vix info
vix registry sync
vix search json
```

### Dependency install problem

```bash
vix info
vix list
vix install
vix check --tests
```

### Global package problem

```bash
vix info
vix list -g
vix info -g softadastra/json
```

### Disk usage problem

```bash
vix info
vix store path
vix store gc --project --dry-run
```

### Cache behavior problem

```bash
vix info
vix build --build-target all
vix info
```

## Common workflows

### Show local environment

```bash
vix info
```

### Show a project package

```bash
vix list
vix info softadastra/json
```

### Show a global package

```bash
vix list -g
vix info -g softadastra/json
```

### Inspect before cleanup

```bash
vix info
vix store gc --project --dry-run
```

### Inspect after registry sync

```bash
vix registry sync
vix info
```

### Inspect after global install

```bash
vix install -g softadastra/json
vix info
vix list -g
```

## Options

| Option       | Description                                         |
| ------------ | --------------------------------------------------- |
| `-g`         | Read package info from the global package manifest. |
| `--global`   | Same as `-g`.                                       |
| `-h, --help` | Show command help.                                  |

## Commands reference

| Command                              | Description                                              |
| ------------------------------------ | -------------------------------------------------------- |
| `vix info`                           | Show Vix environment, paths, caches, counts, and status. |
| `vix info softadastra/json`          | Show a project package from `./vix.lock`.                |
| `vix info -g softadastra/json`       | Show a globally installed package.                       |
| `vix info --global softadastra/json` | Same as `vix info -g softadastra/json`.                  |

## Common mistakes

### Expecting info to fix problems

`vix info` only prints information.

For environment diagnostics, use:

```bash
vix doctor
```

For project cleanup, use:

```bash
vix clean
```

For dependency reinstall, use:

```bash
vix reset
```

### Expecting info to sync the registry

`vix info` does not update registry metadata.

Use:

```bash
vix registry sync
```

### Expecting info to install dependencies

`vix info` does not install packages.

Use:

```bash
vix install
```

### Expecting info to remove cache data

`vix info` does not clean the store or artifact cache.

Preview store cleanup with:

```bash
vix store gc --project --dry-run
```

### Running package info outside a project

Wrong:

```bash
cd /tmp
vix info softadastra/json
```

Correct:

```bash
cd /path/to/project
vix info softadastra/json
```

Project package info reads:

```txt
./vix.lock
```

### Forgetting `-g` for global package info

Wrong:

```bash
vix info softadastra/json
```

when the package is installed globally but not in the current project.

Correct:

```bash
vix info -g softadastra/json
```

### Expecting `vix info` to list all packages

Use:

```bash
vix list
```

or:

```bash
vix list -g
```

to list packages.

Use `vix info <pkg>` for one package.

## Troubleshooting

### Project lock file not found

Run from the project directory:

```bash
cd /path/to/project
vix info softadastra/json
```

If the project needs dependencies, create the lockfile:

```bash
vix add softadastra/json
vix install
```

### Global package manifest not found

If you run:

```bash
vix info -g softadastra/json
```

and Vix reports that the global package manifest is not found, no global packages are installed yet.

Install one:

```bash
vix registry sync
vix install -g softadastra/json
```

### Invalid project lock file

If the lockfile is malformed, Vix reports:

```txt
invalid project lock file
```

Check:

```bash
cat vix.lock
```

Then regenerate or restore it.

### Invalid global manifest

If the global manifest is malformed, Vix reports:

```txt
invalid global manifest
```

Check:

```txt
~/.vix/global/installed.json
```

Then reinstall or intentionally clean the global package state.

### Package not found

If Vix reports:

```txt
package not found: softadastra/json
```

check the package id:

```bash
vix list
```

or for global packages:

```bash
vix list -g
```

Then run `vix info` with the exact id shown.

### Vix root is missing

If Vix prints:

```txt
Vix root is missing. Some commands may create it on demand.
```

that means:

```txt
~/.vix
```

does not exist yet.

Commands like registry sync or install can create it when needed:

```bash
vix registry sync
```

## Best practices

Run `vix info` before deleting cache or store data.

Use `vix info` when debugging path problems.

Use `vix info <pkg>` after `vix list` when you need one package detail.

Use `vix info -g <pkg>` after `vix list -g` for global package details.

Use `vix doctor` after `vix info` when the environment looks wrong.

Use `vix registry sync` when registry state is missing or stale.

Use `vix install` when project dependencies are missing.

Use `vix store gc --project --dry-run` before store cleanup.

## Related commands

| Command                  | Purpose                                            |
| ------------------------ | -------------------------------------------------- |
| `vix doctor`             | Check environment health.                          |
| `vix registry path`      | Print registry index path.                         |
| `vix registry sync`      | Refresh registry index.                            |
| `vix store path`         | Print local store path.                            |
| `vix store gc --project` | Clean unused store data for current project scope. |
| `vix list`               | List project dependencies.                         |
| `vix list -g`            | List global packages.                              |
| `vix clean`              | Clean project-local generated state.               |
| `vix reset`              | Clean and reinstall project dependencies.          |
| `vix install`            | Install locked project dependencies.               |
| `vix install -g`         | Install a global package.                          |

## Next step

Diagnose the environment.

[Open the vix doctor guide](/cli/doctor)
