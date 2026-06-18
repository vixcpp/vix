# vix unpublish

`vix unpublish` removes a package from the Vix Registry.

Use it when a package should no longer be available from the registry index.

```bash
vix unpublish namespace/name
```

## Overview

`vix unpublish` removes the package registry entry from the local Vix Registry clone, then creates a registry branch and pushes it for review.

It does not remove a single version.

It removes the package entry file.

For package:

```txt
gaspardkirira/strings
```

Vix deletes:

```txt
index/gaspardkirira.strings.json
```

The model is:

```txt
registry entry file
  -> delete package entry
  -> create unpublish branch
  -> commit deletion
  -> push branch
  -> create GitHub PR when gh is available
```

## Usage

```bash
vix unpublish <namespace/name> [-y|--yes]
```

## Basic examples

```bash
# Unpublish a package
vix unpublish gaspardkirira/strings

# Skip confirmation
vix unpublish gaspardkirira/strings --yes

# Same as --yes
vix unpublish gaspardkirira/strings -y
```

## What it does

When you run:

```bash
vix unpublish gaspardkirira/strings
```

Vix does this:

1. Parses the package id.
2. Validates the format `namespace/name`.
3. Locates the local registry clone.
4. Checks that the registry entry exists.
5. Asks for confirmation unless `-y` or `--yes` is used.
6. Pulls the local registry clone with `git pull --ff-only`.
7. Creates a new unpublish branch.
8. Deletes the package registry entry file.
9. Stages the deletion with `git add -A`.
10. Commits the registry update.
11. Pushes the branch to origin.
12. Tries to create a GitHub PR with `gh`.

## Package format

The package id must use this format:

```txt
namespace/name
```

Valid:

```bash
vix unpublish gaspardkirira/strings
vix unpublish gk/json
vix unpublish softadastra/http
```

Invalid:

```bash
vix unpublish strings
vix unpublish gaspardkirira
vix unpublish gaspardkirira/
vix unpublish /strings
vix unpublish gk/json@1.0.0
```

Current `vix unpublish` expects a package id, not a versioned package spec.

## Important behavior

`vix unpublish` removes the full package entry from the registry.

It does not remove only one version.

This means if the package entry contains:

```json
{
  "versions": {
    "0.1.0": {
      "tag": "v0.1.0"
    },
    "0.2.0": {
      "tag": "v0.2.0"
    }
  }
}
```

running:

```bash
vix unpublish gaspardkirira/strings
```

removes the whole registry entry file.

## What it does not do

`vix unpublish` does not remove:

```txt
the package GitHub repository
Git tags in the package repository
already installed packages
users' vix.lock files
local project .vix/deps folders
global package caches
```

It only removes the package entry from the Vix Registry index through a registry branch and PR.

## Registry location

`vix unpublish` expects the local registry clone at:

```txt
~/.vix/registry/index
```

Registry entries live under:

```txt
~/.vix/registry/index/index
```

For package:

```txt
gaspardkirira/strings
```

the registry entry file is:

```txt
~/.vix/registry/index/index/gaspardkirira.strings.json
```

The filename format is:

```txt
<namespace>.<name>.json
```

## Registry sync requirement

Before unpublishing, the local registry must exist.

Create or refresh it with:

```bash
vix registry sync
```

Then run:

```bash
vix unpublish gaspardkirira/strings
```

If the registry is missing, Vix reports:

```txt
registry is not available locally
Run: vix registry sync
```

## Confirmation

By default, `vix unpublish` asks for confirmation.

It prints:

```txt
This will remove the package from the Vix registry.
Changes are auto-merged after validation.
Package: gaspardkirira/strings
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

## Skip confirmation

Use `-y` or `--yes` to skip the confirmation prompt.

```bash
vix unpublish gaspardkirira/strings --yes
```

or:

```bash
vix unpublish gaspardkirira/strings -y
```

Use this only when you are sure the package entry should be removed.

## Branch name

`vix unpublish` creates a branch with this shape:

```txt
unpublish-<namespace>-<name>-<timestamp>
```

Example:

```txt
unpublish-gaspardkirira-strings-20260528T104512Z
```

The timestamp is in compact UTC format:

```txt
YYYYMMDDTHHMMSSZ
```

## Git workflow

After confirmation, Vix runs Git commands in the local registry clone.

It first updates the registry clone:

```bash
git -C ~/.vix/registry/index pull -q --ff-only
```

Then creates the branch:

```bash
git -C ~/.vix/registry/index checkout -B <branch> -q
```

Then stages the deletion:

```bash
git -C ~/.vix/registry/index add -A
```

Then commits:

```bash
git -C ~/.vix/registry/index commit -q -m "registry: unpublish namespace/name"
```

Then pushes:

```bash
git -C ~/.vix/registry/index push -u origin <branch>
```

## Commit message

The commit message uses this shape:

```txt
registry: unpublish namespace/name
```

Example:

```txt
registry: unpublish gaspardkirira/strings
```

## GitHub PR creation

After pushing the branch, Vix tries to create a PR with GitHub CLI.

It checks:

```bash
gh --version
gh auth status -h github.com
```

If `gh` is installed and authenticated, Vix creates a PR against:

```txt
vixcpp/registry
```

with:

```txt
base: main
head: <unpublish branch>
title: registry: unpublish namespace/name
```

Example title:

```txt
registry: unpublish gaspardkirira/strings
```

## PR body

The generated PR body has this shape:

```txt
Removes gaspardkirira/strings from the Vix registry.

Deleted file:
- index/gaspardkirira.strings.json
```

## If `gh` is missing

If GitHub CLI is not installed, Vix still pushes the branch.

Then it prints:

```txt
gh not found, skipping PR creation
Create PR manually: vixcpp/registry <- <branch>
```

Create the PR manually from the printed branch.

## If `gh` is not authenticated

If GitHub CLI is installed but not authenticated, Vix prints:

```txt
gh is installed but not authenticated
Run: gh auth login
Then create PR: vixcpp/registry <- <branch>
```

Authenticate with:

```bash
gh auth login
```

Then create the PR manually if needed.

## If PR creation fails

If `gh pr create` fails, Vix does not undo the pushed branch.

It prints:

```txt
gh pr create failed, continuing
Create PR manually: vixcpp/registry <- <branch>
```

The registry branch is still available.

## After the PR merges

After the registry PR is merged, users must refresh the local registry index:

```bash
vix registry sync
```

Then the unpublished package should no longer be found in the local registry index.

Example:

```bash
vix search strings
```

or:

```bash
vix add gaspardkirira/strings
```

should no longer find or resolve the package if the registry entry was removed.

## Full workflow

```bash
# Refresh registry clone
vix registry sync

# Unpublish package
vix unpublish gaspardkirira/strings

# Type DELETE when prompted

# After PR merge
vix registry sync
vix search strings
```

## Non-interactive workflow

```bash
vix registry sync
vix unpublish gaspardkirira/strings --yes
```

Use this only when the package removal is intentional.

## Safer release workflow

If only one release is broken, publishing a fixed version is usually safer than unpublishing the entire package.

Recommended:

```bash
git tag -a v0.2.1 -m "Release v0.2.1"
git push origin v0.2.1
vix publish 0.2.1
```

Use `vix unpublish` when the whole package entry should be removed from the registry.

## Options

| Option       | Description               |
| ------------ | ------------------------- |
| `-y`         | Skip confirmation prompt. |
| `--yes`      | Same as `-y`.             |
| `-h, --help` | Show command help.        |

## Commands reference

| Command                              | Description                               |
| ------------------------------------ | ----------------------------------------- |
| `vix unpublish namespace/name`       | Remove a package entry from the registry. |
| `vix unpublish namespace/name --yes` | Remove without confirmation prompt.       |
| `vix unpublish namespace/name -y`    | Same as `--yes`.                          |
| `vix unpublish --help`               | Show help.                                |

## Common workflows

### Unpublish a package

```bash
vix registry sync
vix unpublish gaspardkirira/strings
```

### Skip confirmation

```bash
vix registry sync
vix unpublish gaspardkirira/strings --yes
```

### Create PR manually when `gh` is missing

```bash
vix unpublish gaspardkirira/strings
```

Then use the printed branch:

```txt
vixcpp/registry <- unpublish-gaspardkirira-strings-20260528T104512Z
```

### Verify after merge

```bash
vix registry sync
vix search strings
```

## Common mistakes

### Passing a version

Wrong:

```bash
vix unpublish gk/json@0.2.0
```

Correct:

```bash
vix unpublish gk/json
```

The current command removes the package entry, not a single version.

### Using an invalid package id

Wrong:

```bash
vix unpublish json
```

Correct:

```bash
vix unpublish gk/json
```

### Forgetting registry sync

Wrong:

```bash
vix unpublish gk/json
```

when the registry clone does not exist locally.

Correct:

```bash
vix registry sync
vix unpublish gk/json
```

### Typing the wrong confirmation text

The confirmation must be exactly:

```txt
DELETE
```

Anything else cancels the command.

### Expecting unpublish to delete GitHub tags

`vix unpublish` does not delete package repository tags.

If you need to delete a tag, do it explicitly in the package repository.

### Expecting unpublish to remove installed copies

Users who already installed a package may still have it in local caches, project lockfiles, or project dependency folders.

Unpublish only removes the package from the registry index going forward.

### Expecting immediate registry removal

The removal becomes visible after:

```txt
registry branch pushed
PR created or opened manually
PR merged
users run vix registry sync
```

## Troubleshooting

### Missing package id

If you run:

```bash
vix unpublish
```

Vix reports:

```txt
unpublish failed: missing package id. Try: vix unpublish namespace/name
```

Use:

```bash
vix unpublish namespace/name
```

### Invalid id format

If Vix reports:

```txt
invalid id format, expected namespace/name
```

use a valid package id:

```bash
vix unpublish gk/json
```

### Registry is not available locally

Run:

```bash
vix registry sync
```

Then retry:

```bash
vix unpublish gk/json
```

### Registry entry not found locally

If Vix reports:

```txt
registry entry not found locally
Run: vix registry sync
```

refresh the registry:

```bash
vix registry sync
```

If the error remains, the package is probably not present in the local registry index.

### Cancelled

If you did not type `DELETE`, Vix cancels.

Run again and type:

```txt
DELETE
```

or use:

```bash
vix unpublish gk/json --yes
```

### Failed to update local registry repo

Vix runs:

```bash
git pull --ff-only
```

inside the registry clone.

If it fails, refresh the registry clone:

```bash
vix registry sync
```

Then retry.

### Failed to create branch

Check the local registry clone state:

```bash
cd ~/.vix/registry/index
git status
```

Then run:

```bash
vix registry sync
```

### Failed to delete entry file

Check file permissions in:

```txt
~/.vix/registry/index/index
```

Then retry after syncing the registry.

### Failed to commit

Check registry clone state:

```bash
cd ~/.vix/registry/index
git status
```

If there are no changes, the entry may already be removed.

### Failed to push branch

Check Git authentication and repository permissions.

You need permission to push a branch to the registry remote.

### `gh` not found

Install GitHub CLI or create the PR manually.

The branch has already been pushed.

### `gh` is not authenticated

Run:

```bash
gh auth login
```

Then create the PR manually if the command already pushed the branch.

## Best practices

Run `vix registry sync` before unpublishing.

Use `vix search <name>` before unpublishing to confirm the package exists.

Understand that current `vix unpublish` removes the package entry, not a single version.

Prefer publishing a fixed version when only one release is broken.

Use `--yes` only in trusted automation.

Keep the registry PR small and clear.

After merge, run `vix registry sync` and verify the package is gone.

Do not expect unpublish to remove user caches, tags, or old lockfiles.

## Related commands

| Command             | Purpose                                           |
| ------------------- | ------------------------------------------------- |
| `vix publish`       | Publish a tagged package version to the registry. |
| `vix registry sync` | Clone or refresh the local registry index.        |
| `vix search`        | Search packages in the local registry index.      |
| `vix add`           | Add a registry package to a project.              |
| `vix install`       | Install locked project dependencies.              |
| `vix remove`        | Remove a project dependency from the lockfile.    |
| `vix list`          | List project dependencies.                        |

## Next step

Manage the local registry index.

[Open the vix registry guide](/cli/registry)
