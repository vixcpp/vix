# vix store

`vix store` manages the local Vix package store.

Use it when you want to inspect where Vix stores fetched package repositories or garbage collect unused package checkouts.

```bash
vix store path
```

## Overview

The Vix store is the local package source cache.

It contains Git checkouts used by Vix Registry packages.

The store is shared between projects.

Typical path:

```txt
~/.vix/store/git
```

When a project installs dependencies, Vix can fetch package repositories into the store, then reuse those checkouts later instead of cloning the same package again.

## Usage

```bash
vix store <subcommand> [options]
```

## Subcommands

| Subcommand | Purpose                               |
| ---------- | ------------------------------------- |
| `path`     | Print the local Git store path.       |
| `gc`       | Garbage collect unused store entries. |

## Basic examples

```bash
# Show local store path
vix store path

# Preview project-scoped garbage collection
vix store gc --project --dry-run

# Run project-scoped garbage collection
vix store gc --project
```

## Store path

Run:

```bash
vix store path
```

Example output shape:

```txt
Store
root: /home/user/.vix/store/git
```

This is the root used for package Git checkouts.

## Store layout

The store uses package ids and commits.

A package id like:

```txt
gk/json
```

is stored with a dot-safe folder name:

```txt
gk.json
```

A checkout can look like:

```txt
~/.vix/store/git/gk.json/<commit>
```

Example:

```txt
~/.vix/store/git/
└── gk.json/
    └── 8f3a9c4...
```

This lets Vix keep multiple commits of the same package when different projects need different locked versions.

## Garbage collection

Run:

```bash
vix store gc --project
```

Current `vix store gc` requires project scope.

This means the command reads the current project lockfile:

```txt
./vix.lock
```

Then it keeps only the store entries referenced by that lockfile.

Everything else in the shared store can be removed.

## Important warning

`vix store gc --project` is destructive for other projects sharing the same store.

The store is shared globally.

If another Vix project uses a dependency checkout that is not referenced by the current project’s `vix.lock`, project-scoped GC may delete that checkout.

Always preview first:

```bash
vix store gc --project --dry-run
```

Then run the real cleanup only when the result is safe:

```bash
vix store gc --project
```

## Why `--project` is required

In the current version, GC must be scoped to the current project.

This is required explicitly:

```bash
vix store gc --project
```

If you run:

```bash
vix store gc
```

Vix reports:

```txt
GC requires --project scope in this version.
Use: vix store gc --project
```

This avoids accidental broad cleanup.

## Project-scoped GC

Project-scoped GC reads:

```txt
vix.lock
```

It keeps package commits referenced by the lockfile.

A dependency entry contains fields like:

```json
{
  "id": "gk/json",
  "commit": "8f3a9c4..."
}
```

Vix turns that into a keep key:

```txt
gk.json + 8f3a9c4...
```

Then it scans:

```txt
~/.vix/store/git
```

and removes commit folders that are not in the keep set.

## Dry run

Use:

```bash
vix store gc --project --dry-run
```

Dry run lists what would be removed without deleting anything.

Example output shape:

```txt
Store GC
scope: project
lock: /home/user/api/vix.lock
root: /home/user/.vix/store/git
mode: dry-run

WARNING: This will remove any store entries NOT used by THIS project.
If you have other Vix projects, their cached dependencies might be deleted.

would remove: gk.json [4a2b8f10...]

✔ GC dry-run finished.
would remove commits: 1
would remove packages: 0
would free: 12.40 MB
```

## Real GC

After previewing, run:

```bash
vix store gc --project
```

Example output shape:

```txt
Store GC
scope: project
lock: /home/user/api/vix.lock
root: /home/user/.vix/store/git

WARNING: This will remove any store entries NOT used by THIS project.
If you have other Vix projects, their cached dependencies might be deleted.

removed: gk.json [4a2b8f10...]

✔ GC finished.
removed commits: 1
removed packages: 1
freed: 12.40 MB
```

## What project GC keeps

Project GC keeps entries referenced by the current project lockfile.

Example `vix.lock`:

```json
{
  "lockVersion": 1,
  "dependencies": [
    {
      "id": "gk/json",
      "version": "1.0.0",
      "commit": "8f3a9c4..."
    }
  ]
}
```

This keeps:

```txt
~/.vix/store/git/gk.json/8f3a9c4...
```

Any other commit folder under `gk.json` can be removed if it is not referenced by the current project.

## Lockfile formats

`vix store gc --project` accepts two lockfile shapes.

Array shape:

```json
[
  {
    "id": "gk/json",
    "commit": "8f3a9c4..."
  }
]
```

Object shape:

```json
{
  "dependencies": [
    {
      "id": "gk/json",
      "commit": "8f3a9c4..."
    }
  ]
}
```

If the lockfile does not match these shapes, Vix reports:

```txt
invalid lock: expected array or { dependencies: [] }
```

## Missing lockfile

Project GC requires:

```txt
./vix.lock
```

If it is missing, Vix reports:

```txt
missing lock file: /path/to/project/vix.lock
Tip: run in a project folder containing vix.lock
```

Run the command from a Vix project folder that has a lockfile:

```bash
cd /path/to/project
vix store gc --project --dry-run
```

## What `vix store gc --project` removes

It removes unused commit directories under:

```txt
~/.vix/store/git/<package>/<commit>
```

Example:

```txt
~/.vix/store/git/gk.json/old-commit
```

After removing unused commits, it also removes empty package directories.

Example:

```txt
~/.vix/store/git/gk.json
```

if no commit folders remain.

## What it does not remove

`vix store gc --project` does not remove:

```txt
project build directories
project .vix/deps
project .vix/vix_deps.cmake
registry index
global package manifest
Vix CLI binary
```

For project-local generated files, use:

```bash
vix clean
```

For reinstalling project dependencies, use:

```bash
vix reset
```

For dependency installation, use:

```bash
vix install
```

## Difference between store, registry, and project deps

| Area                | Path                    | Purpose                                                         |
| ------------------- | ----------------------- | --------------------------------------------------------------- |
| Store               | `~/.vix/store/git`      | Shared package Git checkouts by commit.                         |
| Registry            | `~/.vix/registry/index` | Local registry metadata index.                                  |
| Project deps        | `.vix/deps`             | Project-local dependency links or copies.                       |
| Generated deps file | `.vix/vix_deps.cmake`   | Generated CMake integration for installed project dependencies. |

## Difference between `vix store gc` and `vix clean`

| Command                  | Purpose                                                                  |
| ------------------------ | ------------------------------------------------------------------------ |
| `vix store gc --project` | Clean shared package store entries not used by current project lockfile. |
| `vix clean`              | Clean project-local generated/build state.                               |

Use `vix store gc --project` for the shared package store.

Use `vix clean` for the current project.

## Difference between `vix store gc` and `vix reset`

| Command                  | Purpose                                                         |
| ------------------------ | --------------------------------------------------------------- |
| `vix store gc --project` | Remove unused shared store checkouts.                           |
| `vix reset`              | Clean and reinstall the current project dependency/build state. |

Use `vix reset` when a project dependency state is broken.

Use `vix store gc --project` when the global store is taking space.

## Safe cleanup workflow

Use this workflow:

```bash
vix store path
vix store gc --project --dry-run
vix store gc --project
```

The dry run is important because the store is shared.

## Project cleanup workflow

For cleaning a project and reinstalling dependencies:

```bash
vix clean
vix install
vix build
```

For deeper project dependency repair:

```bash
vix reset
vix install
vix build
```

Use `vix store gc --project` only when you specifically want to clean the shared package store.

## Options

| Option       | Description                                                                                          |
| ------------ | ---------------------------------------------------------------------------------------------------- |
| `--project`  | Scope garbage collection to the current project using `vix.lock`. Required for `gc` in this version. |
| `--dry-run`  | List store entries that would be removed without deleting them.                                      |
| `-h, --help` | Show command help.                                                                                   |

## Commands reference

| Command                            | Description                                                |
| ---------------------------------- | ---------------------------------------------------------- |
| `vix store path`                   | Print the local Git store path.                            |
| `vix store gc --project --dry-run` | Preview project-scoped store cleanup.                      |
| `vix store gc --project`           | Remove store commits not used by current project lockfile. |

## Common workflows

### Show store path

```bash
vix store path
```

### Preview cleanup

```bash
vix store gc --project --dry-run
```

### Run cleanup

```bash
vix store gc --project
```

### Safe cleanup

```bash
vix store path
vix store gc --project --dry-run
vix store gc --project
```

### Clean project state instead of store

```bash
vix clean
vix install
vix build
```

## Common mistakes

### Running GC without `--project`

Wrong:

```bash
vix store gc
```

Correct:

```bash
vix store gc --project
```

### Running project GC without previewing

Wrong:

```bash
vix store gc --project
```

as the first command.

Safer:

```bash
vix store gc --project --dry-run
vix store gc --project
```

### Running from the wrong folder

Wrong:

```bash
cd /tmp
vix store gc --project
```

when `/tmp` has no `vix.lock`.

Correct:

```bash
cd /path/to/vix/project
vix store gc --project --dry-run
```

### Expecting store GC to remove build files

`vix store gc --project` does not clean build folders.

Use:

```bash
vix clean
```

or:

```bash
vix reset
```

### Expecting store GC to regenerate dependency integration

`vix store gc --project` only removes store entries.

To install or regenerate project dependency integration, use:

```bash
vix install
```

### Confusing registry and store

The registry stores package metadata.

The store stores fetched package Git checkouts.

Use:

```bash
vix registry sync
```

for registry metadata.

Use:

```bash
vix store path
```

for the local package checkout store.

## Troubleshooting

### Missing lock file

Run the command inside a project folder with `vix.lock`.

```bash
cd /path/to/project
vix store gc --project --dry-run
```

If the project has dependencies but no lockfile, restore it or resolve dependencies again:

```bash
vix install
```

### Invalid lockfile

If Vix reports:

```txt
invalid lock: expected array or { dependencies: [] }
```

check `vix.lock`.

It should contain dependency entries with:

```txt
id
commit
```

### Store empty

If the store does not exist or is empty, Vix prints:

```txt
GC done (store empty).
```

There is nothing to clean.

### Permission denied

If Vix cannot inspect or remove some folders, check ownership:

```bash
ls -la ~/.vix/store/git
```

Fix permissions if needed.

### Cleanup removed a package needed by another project

Run install in that other project:

```bash
cd /path/to/other/project
vix install
```

Vix can fetch the missing package checkout again if the registry and lockfile are valid.

## Best practices

Use `vix store path` to inspect the store location.

Run `vix store gc --project --dry-run` before real cleanup.

Remember that the store is shared by all Vix projects.

Use project GC only when you accept that other projects may need to re-fetch dependencies.

Keep `vix.lock` committed so dependencies can be restored.

Use `vix install` to restore project dependencies.

Use `vix clean` for project-local cleanup.

Use `vix reset` for project dependency repair.

Do not manually delete random folders under `~/.vix/store/git` unless you understand which project uses them.

## Related commands

| Command             | Purpose                                             |
| ------------------- | --------------------------------------------------- |
| `vix registry sync` | Refresh local registry metadata.                    |
| `vix add`           | Add a dependency.                                   |
| `vix install`       | Install project dependencies from `vix.lock`.       |
| `vix clean`         | Remove project-local generated state.               |
| `vix reset`         | Clean and reinstall the project.                    |
| `vix info`          | Show store, registry, cache, and environment paths. |
| `vix list`          | List project dependencies.                          |

## Next step

Continue with ORM tooling.

[Open the vix orm guide](/cli/orm)
