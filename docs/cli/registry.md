# vix registry

`vix registry` manages the local Vix Registry index and package metadata.

Use it when you want to initialize a package manifest, refresh the local registry index, or inspect where the registry metadata is stored.

```bash
vix registry sync
```

## Overview

The Vix Registry is the package metadata layer used by Vix dependency commands.

It tells Vix:

```txt
which packages exist
which versions are available
where package repositories live
which Git tag and commit belong to each version
what metadata the package exposes
```

The registry is not the package source store.
The registry stores metadata.
The store keeps fetched package repositories.
The project lockfile pins exact versions.
The project install step prepares dependencies for the current project.

The full model is:

```txt
Registry index       -> package metadata
Store                -> fetched package Git checkouts
vix.json             -> declared project dependency requirements
vix.lock             -> exact resolved dependency versions
.vix/deps            -> project-local dependency links or copies
.vix/vix_deps.cmake  -> generated dependency integration
```

## Usage

```bash
vix registry <subcommand>
```

## Subcommands

| Subcommand | Purpose                                           |
| ---------- | ------------------------------------------------- |
| `init`     | Create a local `vix.json` manifest for a package. |
| `sync`     | Clone or refresh the local registry index.        |
| `path`     | Print the local registry index path.              |

## Basic examples

```bash
# Create a package manifest
vix registry init

# Overwrite existing vix.json
vix registry init --force

# Refresh local registry metadata
vix registry sync

# Show registry path
vix registry path
```

## Registry path

The local registry index is stored under:

```txt
~/.vix/registry/index
```

Registry package entries live inside:

```txt
~/.vix/registry/index/index
```

For package:

```txt
gk/json
```

the registry entry file is:

```txt
~/.vix/registry/index/index/gk.json.json
```

The filename format is:

```txt
<namespace>.<name>.json
```

## Show registry path

Run:

```bash
vix registry path
```

Example output shape:

```txt
Registry
path: /home/user/.vix/registry/index
```

Use this when you want to inspect the local registry clone manually.

## Registry repository

`vix registry sync` uses the official Vix Registry repository:

```txt
https://github.com/vixcpp/registry.git
```

The local clone is stored at:

```txt
~/.vix/registry/index
```

## Sync the registry

Run:

```bash
vix registry sync
```

If the registry does not exist locally, Vix clones it:

```txt
git clone --depth 1 https://github.com/vixcpp/registry.git ~/.vix/registry/index
```

Then Vix normalizes the worktree:

```txt
fetch origin --prune
checkout main from origin/main
reset --hard origin/main
```

If the registry already exists, Vix refreshes and resets it to `origin/main`.

## Why sync matters

The registry is local.

Commands like `vix add`, `vix update`, `vix outdated`, `vix publish`, and global package workflows read the local registry index.

If the local index is old, Vix may not see new packages or new versions.

Run:

```bash
vix registry sync
```

before dependency maintenance.

## When to run registry sync

Run `vix registry sync` before:

```txt
searching for packages
adding a new package
checking outdated dependencies
updating dependencies
installing a global package
upgrading a global package
publishing a package
unpublishing a package
```

Common workflow:

```bash
vix registry sync
vix search json
vix add gk/json
vix install
```

## What registry sync does not do

`vix registry sync` does not install dependencies.

It does not update `vix.lock`.

It does not create `.vix/deps`.

It does not generate `.vix/vix_deps.cmake`.

It only refreshes package metadata.

To install dependencies, use:

```bash
vix install
```

## Registry vs install

| Command             | Purpose                                     |
| ------------------- | ------------------------------------------- |
| `vix registry sync` | Refresh package metadata.                   |
| `vix install`       | Install exact dependencies from `vix.lock`. |

A normal project flow is:

```bash
vix registry sync
vix add gk/json
vix install
vix build
```

## Registry vs store

The registry and store are different.

| Area     | Path                    | Purpose                        |
| -------- | ----------------------- | ------------------------------ |
| Registry | `~/.vix/registry/index` | Package metadata index.        |
| Store    | `~/.vix/store/git`      | Fetched package Git checkouts. |

Use:

```bash
vix registry path
```

to see the registry location.

Use:

```bash
vix store path
```

to see the package store location.

## Initialize package metadata

Run:

```bash
vix registry init
```

This creates:

```txt
vix.json
```

in the current directory.

Use this when you are preparing a package that may later be published to the Vix Registry.

## Generated `vix.json`

`vix registry init` creates a starter manifest like this:

```json
{
  "name": "my-lib",
  "namespace": "your-namespace",
  "version": "0.1.0",
  "type": "header-only",
  "include": "include",
  "deps": [],
  "license": "MIT",
  "description": "A tiny header-only C++ library.",
  "keywords": ["cpp", "header-only", "vix"],
  "repository": "https://github.com/your-username/my-lib",
  "authors": [
    {
      "name": "Your Name",
      "github": "your-username"
    }
  ]
}
```

The default package name comes from the current folder name.

If the current folder is:

```txt
json
```

then the generated name is:

```json
{
  "name": "json"
}
```

## Overwrite existing manifest

If `vix.json` already exists, `vix registry init` fails by default.

Example output shape:

```txt
vix.json already exists
Use: vix registry init --force
```

To overwrite it:

```bash
vix registry init --force
```

Use `--force` carefully because it replaces the current manifest.

## Package identity

A registry package uses:

```txt
namespace/name
```

Example:

```txt
gk/json
```

In `vix.json`, that identity comes from:

```json
{
  "namespace": "gk",
  "name": "json"
}
```

This package id is used by commands like:

```bash
vix add gk/json
vix publish 0.2.0
vix unpublish gk/json
```

## Package versions

Published package versions are represented in registry entries.

A version points to a Git tag and commit.

Example shape:

```json
{
  "versions": {
    "0.2.0": {
      "tag": "v0.2.0",
      "commit": "8f3a9c4..."
    }
  }
}
```

This lets Vix resolve a package version to a stable Git commit.

## Registry entry shape

A registry entry can contain metadata such as:

```json
{
  "name": "json",
  "namespace": "gk",
  "displayName": "Vix JSON",
  "description": "Small JSON helpers for Vix projects.",
  "documentation": "https://github.com/gk/json#readme",
  "license": "MIT",
  "repo": {
    "url": "https://github.com/gk/json",
    "defaultBranch": "main"
  },
  "type": "header-only",
  "versions": {
    "0.2.0": {
      "tag": "v0.2.0",
      "commit": "8f3a9c4..."
    }
  }
}
```

Users normally do not edit registry entries directly.

Use:

```bash
vix publish
```

to create registry updates.

Use:

```bash
vix unpublish
```

to remove a package entry through the registry workflow.

## Dependency workflow

The registry is the first step in dependency resolution.

A typical dependency workflow is:

```bash
vix registry sync
vix search json
vix add gk/json@^1.0.0
vix install
vix build
```

What each command does:

| Command                  | Role                                                        |
| ------------------------ | ----------------------------------------------------------- |
| `vix registry sync`      | Refresh package metadata.                                   |
| `vix search json`        | Search local registry metadata.                             |
| `vix add gk/json@^1.0.0` | Add dependency requirement and rewrite `vix.lock`.          |
| `vix install`            | Install locked dependencies and generate integration files. |
| `vix build`              | Build the project.                                          |

## Add packages from the registry

After syncing:

```bash
vix add gk/json
```

or:

```bash
vix add gk/json@^1.0.0
```

Then install:

```bash
vix install
```

For `vix.app` projects, also link the dependency target when needed:

```ini
deps = [
  gk/json@^1.0.0,
]

links = [
  vix::vix,
  gk::json,
]
```

## Install locked dependencies

After dependencies are added or after cloning a project, run:

```bash
vix install
```

This uses:

```txt
vix.lock
```

and prepares:

```txt
.vix/deps
.vix/vix_deps.cmake
```

Use `vix install`, not `vix registry sync`, when you need project dependencies installed.

## Update dependencies

To check stale dependencies:

```bash
vix registry sync
vix outdated
```

To update locked versions:

```bash
vix update --install
```

A safe maintenance flow:

```bash
vix registry sync
vix outdated
vix update --dry-run
vix update --install
vix check --tests
```

## Global packages

The registry is also used for global package commands.

Install a global package:

```bash
vix registry sync
vix install -g gk/json
```

Upgrade a global package:

```bash
vix registry sync
vix upgrade -g gk/json
```

List global packages:

```bash
vix list -g
```

Remove a global package:

```bash
vix uninstall -g gk/json
```

Global packages are tracked under:

```txt
~/.vix/global/installed.json
```

## Publishing packages

To publish a package version, first prepare the package repository.

Recommended flow:

```bash
git status
vix fmt --check
vix check --tests
vix build --preset release

git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0

vix registry sync
vix publish 0.2.0 --dry-run
vix publish 0.2.0 --notes "Add helpers"
```

`vix publish` creates or updates a registry entry and opens a registry PR when possible.

The package becomes available after the registry PR is merged and users run:

```bash
vix registry sync
```

## Unpublishing packages

To remove a package from the registry index:

```bash
vix registry sync
vix unpublish namespace/name
```

Example:

```bash
vix unpublish gk/json
```

Current unpublish behavior removes the full package registry entry, not one specific version.

Use it carefully.

If only one release is broken, publishing a fixed version is usually safer:

```bash
git tag -a v0.2.1 -m "Release v0.2.1"
git push origin v0.2.1
vix publish 0.2.1
```

## Search packages

Use:

```bash
vix search <query>
```

Example:

```bash
vix search json
```

Search reads the local registry index.

If results look old or missing:

```bash
vix registry sync
vix search json
```

## Registry command map

Use this map to choose the right command.

| Goal                         | Command                          |
| ---------------------------- | -------------------------------- |
| Create package metadata      | `vix registry init`              |
| Refresh registry metadata    | `vix registry sync`              |
| Show registry location       | `vix registry path`              |
| Search packages              | `vix search <query>`             |
| Add package to a project     | `vix add <pkg>`                  |
| Install project dependencies | `vix install`                    |
| Check outdated dependencies  | `vix outdated`                   |
| Update dependencies          | `vix update`                     |
| Remove project dependency    | `vix remove <pkg>`               |
| List dependencies            | `vix list`                       |
| Publish package version      | `vix publish <version>`          |
| Unpublish package entry      | `vix unpublish <namespace/name>` |
| Inspect package store        | `vix store path`                 |
| Clean package store          | `vix store gc --project`         |

## Full project dependency workflow

For a new project:

```bash
vix registry sync
vix add gk/json@^1.0.0
vix install
vix build
vix tests
```

For a cloned project:

```bash
git clone https://github.com/example/api.git
cd api
vix registry sync
vix install
vix build
```

For dependency maintenance:

```bash
vix registry sync
vix outdated
vix update --dry-run
vix update --install
vix check --tests
```

## Full package publish workflow

```bash
# Create package metadata if needed
vix registry init

# Edit vix.json
# Add namespace, name, description, license, exports, constraints, maintainers

# Validate package
vix fmt --check
vix check --tests
vix build --preset release

# Tag release
git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0

# Publish through registry workflow
vix registry sync
vix publish 0.2.0 --dry-run
vix publish 0.2.0 --notes "Initial release"
```

## Full package consume workflow

After the publish PR merges:

```bash
vix registry sync
vix search json
vix add gk/json@0.2.0
vix install
vix build
```

## Options

`vix registry` has subcommand-specific options.

### `init` options

| Option       | Description                       |
| ------------ | --------------------------------- |
| `--force`    | Overwrite an existing `vix.json`. |
| `-h, --help` | Show command help.                |

### `sync` options

`vix registry sync` currently has no extra options.

### `path` options

`vix registry path` currently has no extra options.

## Commands reference

| Command                     | Description                                |
| --------------------------- | ------------------------------------------ |
| `vix registry init`         | Create a package `vix.json` manifest.      |
| `vix registry init --force` | Overwrite existing `vix.json`.             |
| `vix registry sync`         | Clone or refresh the local registry index. |
| `vix registry path`         | Print the local registry path.             |

## Common workflows

### Initialize a package manifest

```bash
vix registry init
```

### Overwrite a generated manifest

```bash
vix registry init --force
```

### Refresh before adding a package

```bash
vix registry sync
vix add gk/json
vix install
```

### Refresh before checking outdated dependencies

```bash
vix registry sync
vix outdated
```

### Refresh before publishing

```bash
vix registry sync
vix publish 0.2.0 --dry-run
vix publish 0.2.0
```

### Show registry location

```bash
vix registry path
```

### Inspect registry manually

```bash
vix registry path
cd ~/.vix/registry/index
git status
ls index
```

## Common mistakes

### Expecting sync to install dependencies

Wrong expectation:

```txt
vix registry sync should install packages
```

Correct model:

```txt
vix registry sync refreshes metadata only
```

Install project dependencies with:

```bash
vix install
```

### Forgetting to sync before adding

If a package is missing or results look stale:

```bash
vix registry sync
vix add namespace/name
```

### Editing registry entries manually

Avoid editing files under:

```txt
~/.vix/registry/index/index
```

Use:

```bash
vix publish
```

or:

```bash
vix unpublish
```

so the registry change goes through the expected Git branch and PR workflow.

### Confusing registry and store

Wrong:

```txt
registry = package source checkout cache
```

Correct:

```txt
registry = package metadata
store = fetched package source checkouts
```

### Using registry init for applications

`vix registry init` creates package metadata for packages intended for registry workflows.

For normal applications, start with:

```bash
vix new app --app
```

or:

```bash
vix new api --template backend
```

### Overwriting vix.json accidentally

`vix registry init --force` replaces the existing `vix.json`.

Use it only when you intentionally want to regenerate package metadata.

### Expecting publish to be immediately available

`vix publish` pushes a registry branch and may create a PR.

The package becomes available after:

```txt
registry PR is merged
user runs vix registry sync
```

## Troubleshooting

### Registry sync failed

If sync fails, Vix prints:

```txt
registry sync failed
Check network + git access, then retry: vix registry sync
```

Try:

```bash
vix registry sync
```

If needed, enable debug output:

```bash
VIX_DEBUG=1 vix registry sync
```

### Registry not synced

Some commands may report:

```txt
registry not synced
Run: vix registry sync
```

Fix:

```bash
vix registry sync
```

### Package not found

If `vix add` or `vix search` cannot find a package:

```bash
vix registry sync
vix search <name>
```

If it still cannot be found, the package may not exist in the current registry index.

### Existing vix.json during init

If `vix registry init` reports:

```txt
vix.json already exists
```

either keep the existing file or overwrite it intentionally:

```bash
vix registry init --force
```

### Local registry clone is broken

If the local registry worktree is broken, run:

```bash
vix registry sync
```

The command normalizes the worktree by fetching, checking out `main`, and resetting to `origin/main`.

### Git is missing

`vix registry sync` requires Git.

Install Git, then run:

```bash
vix registry sync
```

### Network failure

The registry sync needs network access to GitHub.

Check your connection and retry:

```bash
vix registry sync
```

## Best practices

Run `vix registry sync` before dependency maintenance.

Run `vix registry sync` before publishing.

Use `vix search` before adding a package if you do not know the exact package id.

Use `vix add` to add packages to a project.

Use `vix install` after adding dependencies.

Commit `vix.json` and `vix.lock`.

Do not commit `.vix/deps`.

Do not manually edit the local registry clone unless you are doing registry maintenance intentionally.

Use `vix publish --dry-run` before publishing.

Use `vix unpublish` carefully because current behavior removes the full package entry.

Use `vix store path` when you need package source cache location.

## Related commands

| Command            | Purpose                                                       |
| ------------------ | ------------------------------------------------------------- |
| `vix search`       | Search packages in the local registry index.                  |
| `vix add`          | Add a registry package to a project.                          |
| `vix install`      | Install locked project dependencies.                          |
| `vix update`       | Re-resolve dependency versions.                               |
| `vix outdated`     | Check whether locked dependencies are behind registry latest. |
| `vix remove`       | Remove a project dependency from the lockfile.                |
| `vix list`         | List project or global dependencies.                          |
| `vix publish`      | Publish a tagged package version to the registry.             |
| `vix unpublish`    | Remove a package entry from the registry.                     |
| `vix store`        | Manage the local package source store.                        |
| `vix upgrade -g`   | Upgrade a globally installed registry package.                |
| `vix uninstall -g` | Remove a globally installed package.                          |

## Next step

Search the registry for packages.

[Open the vix search guide](/cli/search)
