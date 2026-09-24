# vix list

`vix list` lists project dependencies or globally installed packages.

Use it when you want to inspect the dependencies locked by the current project or the packages installed globally in your Vix environment.

```bash
vix list
```

## Overview

`vix list` has two modes:

```txt
project mode
global mode
```

Project mode reads the current project lockfile:

```bash
vix list
```

Global mode reads the global package manifest:

```bash
vix list -g
```

or:

```bash
vix list --global
```

The command only lists package state.
It does not install anything.
It does not update anything.
It does not check whether packages are outdated.

## Usage

```bash
vix list
vix list -g
vix list --global
```

## Basic examples

```bash
# List project dependencies from vix.lock
vix list

# List globally installed packages
vix list -g

# Same as -g
vix list --global
```

## Modes

| Mode         | Command       | Reads from                     | Scope                  |
| ------------ | ------------- | ------------------------------ | ---------------------- |
| Project mode | `vix list`    | `./vix.lock`                   | Current project        |
| Global mode  | `vix list -g` | `~/.vix/global/installed.json` | Global Vix environment |

## Project dependencies

Run from inside a project folder:

```bash
vix list
```

Vix reads:

```txt
./vix.lock
```

Example output shape:

```txt
Project dependencies
lock: /home/user/api/vix.lock

gk/json  1.0.0
commit: 8f3a9c4...
repo: https://github.com/gk/json

gk/jwt  1.2.0
commit: 5d1b7c2...
repo: https://github.com/gk/jwt

✔ Found 2 dependency(ies).
```

## Project lockfile requirement

Project mode requires:

```txt
vix.lock
```

If the lockfile is missing, Vix reports:

```txt
missing lock file: /path/to/project/vix.lock
Tip: add a package with: vix add <pkg>@<version>
```

Fix it by adding or resolving dependencies:

```bash
vix registry sync
vix add gk/json
vix install
vix list
```

Or, if the project already has a committed lockfile, restore it from version control.

## Supported project lockfile shapes

`vix list` supports two project lockfile shapes.

Array shape:

```json
[
  {
    "id": "gk/json",
    "version": "1.0.0",
    "commit": "8f3a9c4..."
  }
]
```

Object shape:

```json
{
  "lockVersion": 1,
  "dependencies": [
    {
      "id": "gk/json",
      "version": "1.0.0",
      "repo": "https://github.com/gk/json",
      "tag": "v1.0.0",
      "commit": "8f3a9c4..."
    }
  ]
}
```

If the lockfile does not match one of these shapes, Vix reports:

```txt
invalid lock: expected array or { dependencies: [] }
```

## Displayed project fields

For each dependency, Vix can display:

| Field      | Source             |
| ---------- | ------------------ |
| package id | `id`               |
| version    | `version` or `tag` |
| commit     | `commit`           |
| repository | `repo`             |

If `version` is missing but `tag` exists, Vix derives the version from the tag.

Example:

```json
{
  "tag": "v1.0.0"
}
```

is displayed as:

```txt
1.0.0
```

## Empty project dependencies

If `vix.lock` exists but contains no dependencies, Vix prints:

```txt
No dependencies.
```

This means the project has no locked registry dependencies.

## Global packages

Run:

```bash
vix list -g
```

or:

```bash
vix list --global
```

Vix reads:

```txt
~/.vix/global/installed.json
```

Example output shape:

```txt
Global packages

gk/json  1.0.0
commit: 8f3a9c4...
repo: https://github.com/gk/json
type: header-only
include: include

✔ Found 1 global package(s).
```

## Global manifest path

Global packages are tracked in:

```txt
~/.vix/global/installed.json
```

The manifest has this shape:

```json
{
  "packages": [
    {
      "id": "gk/json",
      "version": "1.0.0",
      "repo": "https://github.com/gk/json",
      "tag": "v1.0.0",
      "commit": "8f3a9c4...",
      "type": "header-only",
      "include": "include",
      "installed_path": "/home/user/.vix/global/packages/gk.json/8f3a9c4..."
    }
  ]
}
```

## No global packages

If no global manifest exists, Vix prints:

```txt
No global packages installed.
```

If the manifest exists but the `packages` array is empty, Vix also prints:

```txt
No global packages installed.
```

## Invalid global manifest

Global mode expects:

```json
{
  "packages": []
}
```

If the manifest does not contain a valid `packages` array, Vix reports:

```txt
invalid global manifest: expected { packages: [] }
```

## Displayed global fields

For global packages, Vix displays the normal dependency fields:

```txt
id
version
commit
repo
```

It also displays:

```txt
type
include
```

when available.

Example:

```txt
type: header-only
include: include
```

These fields come from the package metadata read during global installation or upgrade.

## Project vs global packages

Project dependencies and global packages are different.

Project dependencies belong to the current project and are pinned in:

```txt
vix.lock
```

Global packages belong to the user’s Vix environment and are tracked in:

```txt
~/.vix/global/installed.json
```

Use project dependencies for normal application or library builds.

Use global packages for packages you intentionally install globally.

## Difference between `vix list` and `vix outdated`

| Command        | Purpose                                                               |
| -------------- | --------------------------------------------------------------------- |
| `vix list`     | Show what is currently locked or globally installed.                  |
| `vix outdated` | Compare locked project dependencies against registry latest versions. |

Use `vix list` to inspect current state.

Use `vix outdated` to check whether project dependencies are behind.

## Difference between `vix list` and `vix install`

| Command       | Purpose                                     |
| ------------- | ------------------------------------------- |
| `vix list`    | Read and display dependency state.          |
| `vix install` | Install exact dependencies from `vix.lock`. |

`vix list` does not install missing dependencies.

If dependency files are missing, run:

```bash
vix install
```

## Difference between `vix list` and `vix search`

| Command      | Purpose                                        |
| ------------ | ---------------------------------------------- |
| `vix list`   | Show dependencies already locked or installed. |
| `vix search` | Search packages in the local registry index.   |

Use `vix search` when you want to discover packages.

Use `vix list` when you want to inspect your current project or global state.

## Add and list workflow

```bash
vix registry sync
vix add gk/json
vix install
vix list
```

This shows the dependency after it has been added and locked.

## Update and list workflow

```bash
vix outdated
vix update --install
vix list
```

This lets you inspect the project after updating dependency versions.

## Remove and list workflow

```bash
vix remove gk/json
vix install
vix list
```

This lets you confirm the dependency is no longer present in the lockfile.

## Global package workflow

Install globally:

```bash
vix registry sync
vix install -g gk/json
```

List global packages:

```bash
vix list -g
```

Upgrade global package:

```bash
vix upgrade -g gk/json
```

Remove global package:

```bash
vix uninstall -g gk/json
```

List again:

```bash
vix list -g
```

## CI usage

In CI, `vix list` can be useful for debugging dependency state.

```bash
vix install
vix list
vix check --tests
```

For normal CI validation, `vix install` and `vix check --tests` matter more.

`vix list` is mainly informational.

## Options

| Option       | Description                       |
| ------------ | --------------------------------- |
| `-g`         | List globally installed packages. |
| `--global`   | Same as `-g`.                     |
| `-h, --help` | Show command help.                |

## Commands reference

| Command             | Description                                |
| ------------------- | ------------------------------------------ |
| `vix list`          | List project dependencies from `vix.lock`. |
| `vix list -g`       | List globally installed packages.          |
| `vix list --global` | Same as `vix list -g`.                     |

## Common workflows

### List project dependencies

```bash
vix list
```

### List global packages

```bash
vix list -g
```

### Add and list

```bash
vix add gk/json
vix install
vix list
```

### Update and list

```bash
vix update --install
vix list
```

### Remove and list

```bash
vix remove gk/json
vix install
vix list
```

### Install and inspect after clone

```bash
git clone https://github.com/example/api.git
cd api
vix install
vix list
```

## Common mistakes

### Running project list outside a project

Wrong:

```bash
cd /tmp
vix list
```

Correct:

```bash
cd /path/to/project
vix list
```

Project mode needs:

```txt
vix.lock
```

### Expecting `vix list` to check latest versions

`vix list` only shows current dependency state.

To check for newer versions:

```bash
vix registry sync
vix outdated
```

### Expecting `vix list` to install dependencies

`vix list` does not install anything.

To install locked dependencies:

```bash
vix install
```

### Confusing project and global packages

Project packages:

```bash
vix list
```

Global packages:

```bash
vix list -g
```

### Expecting global packages to appear in project list

Global packages are not project dependencies.

If a project needs a package, add it to the project:

```bash
vix add gk/json
vix install
```

### Expecting project dependencies to appear in global list

Project dependencies are not global installs.

Use:

```bash
vix list
```

inside the project.

## Troubleshooting

### Missing lock file

If Vix reports:

```txt
missing lock file
```

run the command from a Vix project folder or create dependency state:

```bash
vix add gk/json
vix install
```

### Failed to read lock

If Vix reports:

```txt
failed to read lock
```

the lockfile may be invalid JSON or unreadable.

Check:

```bash
cat vix.lock
```

Then regenerate or restore it.

### Invalid lock

If Vix reports:

```txt
invalid lock: expected array or { dependencies: [] }
```

make sure `vix.lock` has either an array or an object with a `dependencies` array.

### No dependencies

If Vix prints:

```txt
No dependencies.
```

the project lockfile exists but has no dependencies.

This is valid for projects that do not use registry packages.

### No global packages installed

If Vix prints:

```txt
No global packages installed.
```

there is no global package state yet.

Install a global package with:

```bash
vix install -g gk/json
```

### Failed to read global manifest

If Vix reports:

```txt
failed to read manifest
```

check:

```txt
~/.vix/global/installed.json
```

It may be unreadable or malformed.

### Invalid global manifest

The global manifest must contain:

```json
{
  "packages": []
}
```

If it is corrupted, reinstall or clean global package state intentionally.

## Best practices

Run `vix list` after adding, updating, or removing project dependencies.

Use `vix list -g` before upgrading or uninstalling global packages.

Use `vix outdated` when you need update information.

Use `vix install` after cloning a project.

Commit `vix.lock`.

Do not manually edit `vix.lock`.

Do not confuse global packages with project dependencies.

## Related commands

| Command             | Purpose                                |
| ------------------- | -------------------------------------- |
| `vix add`           | Add a project dependency.              |
| `vix install`       | Install dependencies from `vix.lock`.  |
| `vix update`        | Update project dependencies.           |
| `vix outdated`      | Check outdated project dependencies.   |
| `vix remove`        | Remove a project dependency.           |
| `vix search`        | Search packages in the local registry. |
| `vix registry sync` | Refresh local registry metadata.       |
| `vix install -g`    | Install a global package.              |
| `vix upgrade -g`    | Upgrade a global package.              |
| `vix uninstall -g`  | Remove a global package.               |

## Next step

Continue with packaging.

[Open the vix pack guide](/cli/pack)
