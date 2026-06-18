# vix remove

`vix remove` removes a package dependency from the current project lockfile.

Use it when your project no longer needs a locked package dependency.

```bash
vix remove gk/jwt
```

## Overview

`vix remove` removes one dependency from:

```txt
vix.lock
```

It can also delete the project-local dependency folder under:

```txt
.vix/deps/
```

when `--purge` is used.

It does not remove a package from the Vix Registry.

It does not uninstall a global package.

It does not currently rewrite `vix.json`.

## Usage

```bash
vix remove [@]namespace/name[@version]
vix remove [@]namespace/name[@version] --purge [-y]
```

## Basic examples

```bash
# Remove a locked dependency
vix remove gk/jwt

# Scoped-style syntax
vix remove @gk/jwt

# Remove only if the locked version matches
vix remove gk/jwt@1.0.0

# Scoped-style syntax with version
vix remove @gk/jwt@1.0.0

# Remove and delete project-local dependency files
vix remove @gk/jwt --purge

# Remove and purge without confirmation
vix remove @gk/jwt --purge -y
```

## What it does

When you run:

```bash
vix remove gk/jwt
```

Vix does this:

1. Parses the package target.
2. Reads `vix.lock`.
3. Finds the matching dependency in the lockfile.
4. Removes the first matching dependency entry.
5. Writes the updated `vix.lock`.
6. Optionally deletes `.vix/deps/<namespace>.<name>` when `--purge` is used.
7. Prints a tip to regenerate dependency integration if needed.

## Important behavior

`vix remove` currently removes from:

```txt
vix.lock
```

It does not remove the dependency declaration from:

```txt
vix.json
```

So after removing a dependency, check whether `vix.json` still contains it.

If it is still declared there, remove it manually or use the dependency workflow that matches your project.

## Package format

A package target uses this format:

```txt
namespace/name
```

Examples:

```txt
gk/jwt
gk/json
```

Scoped-style syntax is also accepted:

```txt
@namespace/name
```

Example:

```txt
@gk/jwt
```

Both forms refer to the same dependency id:

```txt
gk/jwt
```

## Version matching

You can pass a version:

```bash
vix remove gk/jwt@1.0.0
```

When a version is provided, Vix removes the dependency only if the locked dependency matches that version.

Vix checks:

```txt
version
tag
```

If the lockfile uses a tag like:

```txt
v1.0.0
```

then this still matches:

```bash
vix remove gk/jwt@1.0.0
```

## Lockfile requirement

`vix remove` requires:

```txt
vix.lock
```

If the lockfile is missing, Vix reports:

```txt
missing lock file: /path/to/project/vix.lock
Run: vix add <pkg>@<version> first
```

This command is lockfile-based.

## Lockfile format

`vix remove` expects a lockfile with:

```json
{
  "dependencies": [
    {
      "id": "gk/jwt",
      "version": "1.0.0"
    }
  ]
}
```

If the lockfile does not contain a `dependencies` array, Vix reports:

```txt
invalid lock: missing 'dependencies' array
Tip: regenerate lock by re-adding dependencies
```

## Remove from lockfile

Run:

```bash
vix remove gk/jwt
```

Example output shape:

```txt
Remove
id: gk/jwt
✔ removed from vix.lock: gk/jwt
✔ lock:  /home/user/api/vix.lock
⚠ Tip: run 'vix install' to regenerate .vix/vix_deps.cmake if needed.
```

## Remove with version

Run:

```bash
vix remove gk/jwt@1.0.0
```

Example output shape:

```txt
Remove
id: gk/jwt
version: 1.0.0
✔ removed from vix.lock: gk/jwt
✔ lock:  /home/user/api/vix.lock
⚠ Tip: run 'vix install' to regenerate .vix/vix_deps.cmake if needed.
```

If the dependency exists but the version does not match, Vix reports:

```txt
dependency not found in lock: gk/jwt
Tip: use 'vix list' to check current deps
```

## Purge project-local files

Use `--purge` to delete the project-local dependency folder.

```bash
vix remove gk/jwt --purge
```

For package:

```txt
gk/jwt
```

Vix deletes:

```txt
.vix/deps/gk.jwt
```

The folder format is:

```txt
.vix/deps/<namespace>.<name>
```

Examples:

| Package        | Project-local dependency folder |
| -------------- | ------------------------------- |
| `gk/jwt`       | `.vix/deps/gk.jwt`              |
| `gk/json`      | `.vix/deps/gk.json`             |
| `adastra/http` | `.vix/deps/adastra.http`        |

## Purge confirmation

When `--purge` is used, Vix asks for confirmation before deleting files.

Example:

```txt
This will also delete files from this project:
/home/user/api/.vix/deps/gk.jwt
Type DELETE to confirm:
```

You must type:

```txt
DELETE
```

If you type anything else, Vix cancels:

```txt
cancelled
```

## Skip purge confirmation

Use `-y` or `--yes`:

```bash
vix remove gk/jwt --purge -y
```

or:

```bash
vix remove gk/jwt --purge --yes
```

This skips confirmation and deletes the project-local dependency folder if it exists.

## What `--purge` does not delete

`--purge` deletes the project-local dependency folder under:

```txt
.vix/deps/
```

It does not delete the global Git store.

It does not delete registry metadata.

It does not delete a globally installed package.

For global packages, use the global uninstall workflow.

## What files are affected

`vix remove` can affect:

```txt
vix.lock
.vix/deps/<namespace>.<name>   # only with --purge
```

It may require regenerating:

```txt
.vix/vix_deps.cmake
```

That is why Vix prints:

```bash
vix install
```

after removal.

## After removing a package

After removing a dependency, validate the project.

```bash
vix install
vix build
vix check --tests
```

If your source code still includes headers from the removed package, the build will fail.

Remove those includes and target links.

## Using remove with vix.app

For `vix.app` projects, check these places after removal:

```txt
vix.lock
vix.json
vix.app
```

If the package is still declared in `vix.app`, remove it from:

```ini
deps = [
  gk/jwt@^1.0.0,
]

links = [
  gk::jwt,
]
```

Example cleanup:

```ini
deps = [
]

links = [
  vix::vix,
]
```

Then run:

```bash
vix install
vix build
```

## Using remove with CMake

For CMake projects, remove the dependency link if the target is no longer used.

Example before:

```cmake
target_link_libraries(api PRIVATE
  gk::jwt
)
```

After:

```cmake
target_link_libraries(api PRIVATE
)
```

Then regenerate dependency integration if needed:

```bash
vix install
vix build
```

## Difference between `vix remove` and `vix uninstall`

| Command                  | Purpose                                              |
| ------------------------ | ---------------------------------------------------- |
| `vix remove <pkg>`       | Remove a locked dependency from the current project. |
| `vix uninstall -g <pkg>` | Remove a globally installed package.                 |

Use `vix remove` for project dependencies.

Use `vix uninstall -g` for global packages.

## Difference between `vix remove` and manual cleanup

| Action                         | Result                                                         |
| ------------------------------ | -------------------------------------------------------------- |
| `vix remove <pkg>`             | Updates `vix.lock`.                                            |
| `vix remove <pkg> --purge`     | Updates `vix.lock` and deletes `.vix/deps/<namespace>.<name>`. |
| manual delete from `.vix/deps` | Deletes files but leaves lockfile unchanged.                   |
| manual edit of `vix.lock`      | Not recommended.                                               |

Prefer `vix remove`.

## Full workflow

Remove the dependency:

```bash
vix remove gk/jwt
```

Regenerate dependency integration:

```bash
vix install
```

Remove usage from source code, `vix.app`, or CMake.

Validate:

```bash
vix build
vix check --tests
```

## Full purge workflow

```bash
vix remove gk/jwt --purge -y
vix install
vix build
vix check --tests
```

Use `--purge` when you want to clean project-local dependency files too.

## Options

| Option       | Description                                                      |
| ------------ | ---------------------------------------------------------------- |
| `--purge`    | Delete local package files under `.vix/deps/<namespace>.<name>`. |
| `-y`         | Skip confirmation when using `--purge`.                          |
| `--yes`      | Same as `-y`.                                                    |
| `-h, --help` | Show command help.                                               |

## Commands reference

| Command                         | Description                                                 |
| ------------------------------- | ----------------------------------------------------------- |
| `vix remove gk/jwt`             | Remove `gk/jwt` from `vix.lock`.                            |
| `vix remove @gk/jwt`            | Same using scoped-style syntax.                             |
| `vix remove gk/jwt@1.0.0`       | Remove only if version matches.                             |
| `vix remove @gk/jwt --purge`    | Remove and ask before deleting project-local files.         |
| `vix remove @gk/jwt --purge -y` | Remove and delete project-local files without confirmation. |

## Common workflows

### Remove a dependency

```bash
vix remove gk/jwt
vix install
vix build
```

### Remove a scoped dependency

```bash
vix remove @gk/jwt
vix install
vix build
```

### Remove a specific version

```bash
vix remove gk/jwt@1.0.0
vix install
vix build
```

### Remove and purge files

```bash
vix remove @gk/jwt --purge -y
vix install
vix build
```

### Remove and validate

```bash
vix remove gk/jwt
vix install
vix check --tests
```

## Common mistakes

### Expecting remove to unpublish a package

`vix remove` only affects the current project.

It does not remove anything from the registry.

### Expecting remove to uninstall global packages

Wrong:

```bash
vix remove gk/jwt
```

when you mean a global install.

Use the global uninstall command instead:

```bash
vix uninstall -g gk/jwt
```

### Expecting remove to update `vix.json`

The current command removes from `vix.lock`.

Check `vix.json` after removal.

If the dependency is still declared there, remove or update it intentionally.

### Forgetting to update `vix.app`

For `vix.app` projects, also remove unused entries from:

```txt
deps
links
```

### Forgetting to update CMake

For CMake projects, remove unused target links such as:

```cmake
target_link_libraries(api PRIVATE gk::jwt)
```

### Forgetting to update source code

After removing a dependency, remove includes such as:

```cpp
#include <jwt/api.hpp>
```

Then run:

```bash
vix check --tests
```

### Forgetting to regenerate dependency integration

After changing dependency state, run:

```bash
vix install
```

This regenerates:

```txt
.vix/vix_deps.cmake
```

### Using wrong package format

Wrong:

```bash
vix remove jwt
```

Correct:

```bash
vix remove gk/jwt
```

## Troubleshooting

### Missing package id

If you run:

```bash
vix remove
```

Vix reports a missing package id and shows help.

Use:

```bash
vix remove namespace/name
```

Example:

```bash
vix remove gk/jwt
```

### Package id cannot be empty

Use a valid package id:

```txt
namespace/name
```

Valid:

```bash
vix remove gk/jwt
vix remove @gk/jwt
```

Invalid:

```bash
vix remove jwt
vix remove @/jwt
vix remove gk/
```

### Missing lock file

If Vix reports:

```txt
missing lock file
```

the project has no `vix.lock`.

Check the current directory.

If this is a Vix project with dependencies, recreate dependency state:

```bash
vix add <pkg>
```

or restore `vix.lock` from version control.

### Invalid lockfile

If Vix reports:

```txt
invalid lock: missing 'dependencies' array
```

regenerate the lockfile by re-adding dependencies or restoring a valid lockfile.

### Dependency not found in lock

If Vix reports:

```txt
dependency not found in lock: gk/jwt
```

check current dependencies:

```bash
vix list
```

The package may already be removed, or the id may be different.

### Purge cancelled

When using `--purge`, type exactly:

```txt
DELETE
```

or pass:

```bash
-y
```

if you intentionally want to skip confirmation.

### Failed to delete project-local files

If Vix reports:

```txt
failed to delete: .vix/deps/gk.jwt
```

check permissions:

```bash
ls -ld .vix/deps/gk.jwt
```

Then fix ownership or remove manually if needed.

## Best practices

Use `vix list` before removing dependencies.

Remove the dependency from source code before or after running `vix remove`.

For `vix.app`, remove unused package specs from `deps`.

For `vix.app`, remove unused CMake aliases from `links`.

For CMake, remove unused `target_link_libraries` entries.

Run `vix install` after removing dependencies.

Run `vix build` after removal.

Run `vix check --tests` before committing.

Use `--purge` when you want to clean `.vix/deps`.

Do not manually edit `vix.lock`.

Do not confuse project dependencies with global packages.

## Related commands

| Command         | Purpose                                  |
| --------------- | ---------------------------------------- |
| `vix add`       | Add a project dependency.                |
| `vix install`   | Install dependencies from `vix.lock`.    |
| `vix update`    | Update dependency versions.              |
| `vix outdated`  | Check outdated dependencies.             |
| `vix list`      | List project dependencies.               |
| `vix install`   | Regenerate dependency integration files. |
| `vix uninstall` | Remove Vix or a global package.          |
| `vix check`     | Validate after removing dependencies.    |

## Next step

List project dependencies.

[Open the vix list guide](/cli/list)
