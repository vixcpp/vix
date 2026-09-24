# vix publish

`vix publish` publishes a tagged version of the current package to the Vix Registry.

Use it when your package is ready, committed, tagged, pushed to origin, and should become available through the registry workflow.

```bash
vix publish
```

## Overview

`vix publish` prepares a registry entry for the current package.

It does not publish random local files.

It publishes a Git tag.

That means the package version must already exist as a Git tag locally and on the remote origin.

The command can:

- detect the current Git repository
- require a clean working tree
- resolve the publish version from a Git tag
- verify that the tag exists locally
- verify that the tag exists on `origin`
- resolve the commit behind the tag
- read package metadata from `vix.json`
- infer package namespace and name from Git remote when needed
- update the local registry clone
- create or update the registry package entry
- create a publish branch in the registry clone
- commit the registry update
- push the registry branch
- optionally create a GitHub pull request with `gh`
- run in dry-run mode
- attach release notes
- clean old local publish branches

The important model is:

```txt
package repo tag
  -> registry entry
  -> registry branch
  -> registry PR
  -> package available after PR merge
```

## Usage

```bash
vix publish [version] [--notes "..."] [--dry-run]
```

## Basic examples

```bash
# Publish latest local SemVer tag
vix publish

# Publish an explicit version
vix publish 0.2.0

# Publish with notes
vix publish 0.2.0 --notes "Add JSON helpers"

# Validate without writing or pushing
vix publish 0.2.0 --dry-run

# Remove older local publish branches in the registry clone
vix publish 0.2.0 --cleanup
```

## Required Git tag format

The Git tag must use this format:

```txt
v<version>
```

Example:

```txt
v0.2.0
```

When you run:

```bash
vix publish 0.2.0
```

Vix looks for:

```txt
v0.2.0
```

Do not pass the `v` prefix to `vix publish`.

Correct:

```bash
vix publish 0.2.0
```

Wrong:

```bash
vix publish v0.2.0
```

## Requirements

Before publishing, the package repository must satisfy these conditions:

```txt
inside a Git repository
working tree is clean
publish tag exists locally
publish tag exists on origin
local registry clone exists
registry index exists
```

The local registry clone comes from:

```bash
vix registry sync
```

## Recommended publish flow

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

After the registry PR is merged, users can refresh the registry and add the package:

```bash
vix registry sync
vix add namespace/name@0.2.0
vix install
```

## Version resolution

`vix publish` can resolve the version in two ways.

### Explicit version

```bash
vix publish 0.2.0
```

Vix checks:

```txt
local tag:  v0.2.0
remote tag: origin v0.2.0
```

If the local tag is missing, Vix fails.

If the remote tag is missing, Vix fails and tells you to push it.

### Latest local SemVer tag

```bash
vix publish
```

When no version is provided, Vix lists local Git tags and finds the latest SemVer tag.

Examples of publishable tags:

```txt
v0.1.0
v0.2.0
v1.0.0
```

Then Vix verifies that the selected tag exists on `origin`.

If the latest local tag has not been pushed, Vix fails.

## Clean working tree requirement

`vix publish` requires a clean working tree.

If your repository has uncommitted changes, Vix reports:

```txt
working tree is not clean
Commit your changes before publishing.
```

This is intentional.

A registry entry should point to a stable Git tag, not to a dirty local state.

Check:

```bash
git status
```

Commit or discard changes before publishing.

## Package identity

A package has this identity:

```txt
namespace/name
```

Example:

```txt
gk/json
```

Vix resolves the package identity from `vix.json` first.

Example:

```json
{
  "namespace": "gk",
  "name": "json",
  "description": "A small JSON helper package for Vix."
}
```

If `vix.json` contains `namespace` and `name`, Vix uses them.

Both values are normalized to lowercase.

## Required package fields in `vix.json`

When `vix.json` exists, `vix publish` requires:

```json
{
  "namespace": "gk",
  "name": "json"
}
```

If one is missing, Vix fails.

Example error:

```txt
invalid vix.json: missing string field `namespace`
```

or:

```txt
invalid vix.json: missing string field `name`
```

## Package name validation

`namespace` and `name` may contain:

```txt
letters
numbers
-
_
.
```

Valid:

```txt
gk
json
softadastra
http-client
json_utils
vix.core
```

Invalid:

```txt
gk/json
json helper
@scope
name!
```

## Inferring package identity from Git remote

If `vix.json` does not provide package identity, Vix tries to infer it from Git remote origin.

Example remote:

```txt
https://github.com/gk/json.git
```

or:

```txt
git@github.com:gk/json.git
```

Vix infers:

```txt
namespace: gk
name: json
id: gk/json
```

If Vix cannot infer the identity, it reports:

```txt
cannot infer package namespace/name
Fix: add { "namespace": "...", "name": "..." } in vix.json or ensure git remote origin is set.
```

## Registry location

`vix publish` expects the local registry clone at:

```txt
~/.vix/registry/index
```

Inside that repository, registry entries live under:

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

## Registry sync requirement

If the local registry clone is missing, Vix reports:

```txt
registry is not available locally: ~/.vix/registry/index
Run: vix registry sync
```

Fix:

```bash
vix registry sync
vix publish 0.2.0
```

## Existing registry entry

If the package already exists in the registry, Vix reads its registry entry.

The entry must contain:

```json
{
  "versions": {}
}
```

If the selected version already exists, Vix fails.

Example:

```txt
version already registered: gk/json@0.2.0
This tag/version is already present in the registry.
```

This prevents publishing the same package version twice.

## New registry entry

If the package does not exist yet, Vix creates a new registry entry.

It can use metadata from `vix.json`, including:

```txt
description
displayName
license
documentation
keywords
exports
constraints
dependencies
maintainers
```

If some fields are missing, Vix fills safe defaults.

## Generated registry metadata

For a new package, Vix can generate fields like:

```json
{
  "api": {
    "format": "vix-api-1",
    "generatedBy": "vix-cli",
    "path": "vix.api.json",
    "updatedAt": "2026-05-28T10:00:00Z"
  },
  "name": "json",
  "namespace": "gk",
  "displayName": "json",
  "description": "",
  "documentation": "https://github.com/gk/json#readme",
  "keywords": [],
  "license": "MIT",
  "manifestPath": "vix.json",
  "homepage": "https://github.com/gk/json",
  "repo": {
    "url": "https://github.com/gk/json",
    "defaultBranch": "main"
  },
  "type": "header-only",
  "versions": {}
}
```

## Version entry

For each published version, Vix adds:

```json
{
  "versions": {
    "0.2.0": {
      "tag": "v0.2.0",
      "commit": "..."
    }
  }
}
```

The package version points to:

```txt
tag
commit
```

This makes the registry version reproducible.

## Default package type

For new entries, the current generated default is:

```json
{
  "type": "header-only"
}
```

If your package needs more precise metadata, define the correct package metadata in `vix.json` or update the registry entry through the registry workflow.

## Exports

If `vix.json` defines `exports`, Vix uses it.

Example:

```json
{
  "exports": {
    "headers": ["json/json.hpp"],
    "modules": ["json/json.hpp"],
    "namespaces": ["gk::json"]
  }
}
```

If `exports` is missing, Vix tries a simple header convention.

For package name:

```txt
json
```

it looks for:

```txt
include/json/json.hpp
```

If the file exists, Vix adds:

```json
{
  "exports": {
    "headers": ["json/json.hpp"],
    "modules": ["json/json.hpp"],
    "namespaces": []
  }
}
```

## Constraints

If `vix.json` defines `constraints`, Vix uses them.

Example:

```json
{
  "constraints": {
    "minCppStandard": "c++20",
    "platforms": ["linux", "macos", "windows"]
  }
}
```

If missing, Vix uses defaults:

```json
{
  "minCppStandard": "c++17",
  "platforms": ["linux", "macos", "windows"]
}
```

## Dependencies metadata

If `vix.json` defines registry package dependencies for publication metadata, Vix can use them.

Example:

```json
{
  "dependencies": {
    "git": [],
    "registry": [],
    "system": []
  }
}
```

If missing, Vix creates:

```json
{
  "dependencies": {
    "git": [],
    "registry": [],
    "system": []
  }
}
```

This field is registry package metadata.

It is separate from project dependency management fields used by `vix add`, `vix update`, and `vix install`.

## Maintainers

If `vix.json` defines maintainers, Vix uses them.

Example:

```json
{
  "maintainers": [
    {
      "name": "Gaspard Kirira",
      "github": "gk"
    }
  ]
}
```

If missing, Vix creates a maintainer from the namespace and local Git user name when available.

## Quality metadata

Vix can generate basic quality metadata:

```json
{
  "quality": {
    "ci": [],
    "hasDocs": true,
    "hasExamples": true,
    "hasTests": true
  }
}
```

It detects examples and tests from folders such as:

```txt
examples
tests
test
unittests
```

## Dry run

Use:

```bash
vix publish 0.2.0 --dry-run
```

Dry run validates the publish flow and prints the registry entry that would be written.

It does not:

```txt
write registry entry
create registry branch
commit changes
push branch
create PR
```

Example:

```bash
vix publish 0.2.0 --dry-run
```

Output shape:

```txt
Publish
repo: /home/user/json
version: 0.2.0
tag: v0.2.0
commit: 8f3a...
id: gk/json
registry: /home/user/.vix/registry/index
entry: /home/user/.vix/registry/index/index/gk.json.json
dry-run: would update: /home/user/.vix/registry/index/index/gk.json.json
```

Then it prints the generated JSON.

## Publishing for real

Run:

```bash
vix publish 0.2.0
```

Vix then:

1. Writes or updates the registry entry.
2. Runs `git pull --ff-only` in the registry clone.
3. Creates a publish branch.
4. Adds the registry entry.
5. Commits the registry update.
6. Pushes the branch to origin.
7. Creates a GitHub PR when `gh` is installed and authenticated.

## Registry branch name

For package:

```txt
gk/json
```

version:

```txt
0.2.0
```

Vix creates a branch like:

```txt
publish-gk-json-0.2.0
```

Branch format:

```txt
publish-<namespace>-<name>-<version>
```

## Registry commit message

Vix commits with a message shaped like:

```txt
registry: gk/json v0.2.0
```

## GitHub PR creation

If GitHub CLI is installed and authenticated, Vix tries to create a PR automatically.

Requirements:

```txt
gh installed
gh authenticated for github.com
```

Check:

```bash
gh auth status -h github.com
```

If available, Vix runs a PR creation flow against:

```txt
vixcpp/registry
```

with:

```txt
base: main
head: publish branch
title: registry: add <package> v<version>
```

Example PR title:

```txt
registry: add gk/json v0.2.0
```

## PR body

The PR body includes:

```txt
package id
version
tag
commit
notes
```

Example:

```txt
Publish package `gk/json` version `0.2.0`.

- tag: `v0.2.0`
- commit: `8f3a...`

Notes:
Add helpers
```

## If GitHub PR creation fails

If `gh pr create` fails, Vix does not fail the whole publish.

It prints a warning and keeps the pushed branch.

You can create the PR manually.

Example output:

```txt
branch pushed: publish-gk-json-0.2.0
Create a PR on GitHub: vixcpp/registry <- publish-gk-json-0.2.0
Tip: install/auth gh to auto-create PR next time.
```

## Cleanup

Use:

```bash
vix publish 0.2.0 --cleanup
```

After pushing the current publish branch, Vix removes older local publish branches for the same package inside the registry clone.

It does not remove the current publish branch.

It does not delete remote branches.

This only cleans local registry clone branches.

## Notes

Use `--notes` to attach release notes to the generated PR body.

```bash
vix publish 0.2.0 --notes "Add helpers and improve diagnostics"
```

You can also use:

```bash
vix publish 0.2.0 --notes="Add helpers and improve diagnostics"
```

## Full `vix.json` example for a package

```json
{
  "namespace": "gk",
  "name": "json",
  "displayName": "Vix JSON",
  "description": "Small JSON helpers for Vix projects.",
  "license": "MIT",
  "documentation": "https://github.com/gk/json#readme",
  "keywords": ["json", "vix", "cpp"],
  "exports": {
    "headers": ["json/json.hpp"],
    "modules": ["json/json.hpp"],
    "namespaces": ["gk::json"]
  },
  "constraints": {
    "minCppStandard": "c++20",
    "platforms": ["linux", "macos", "windows"]
  },
  "dependencies": {
    "git": [],
    "registry": [],
    "system": []
  },
  "maintainers": [
    {
      "name": "Gaspard Kirira",
      "github": "gk"
    }
  ]
}
```

## Minimal `vix.json` example

```json
{
  "namespace": "gk",
  "name": "json",
  "description": "Small JSON helpers for Vix projects."
}
```

This is enough for Vix to resolve package identity and generate a registry entry.

## Full publish workflow

```bash
# Make sure the package is clean and valid
git status
vix fmt --check
vix check --tests
vix build --preset release

# Create and push the release tag
git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0

# Make sure registry clone exists locally
vix registry sync

# Preview registry entry
vix publish 0.2.0 --dry-run

# Publish registry branch and create PR if gh is available
vix publish 0.2.0 --notes "Add helpers"
```

## After the registry PR merges

Once the registry PR is merged, users can refresh their registry index:

```bash
vix registry sync
```

Then add the package:

```bash
vix add gk/json@0.2.0
vix install
```

For a `vix.app` project, use the package target in `links` when needed:

```ini
deps = [
  gk/json@0.2.0,
]

links = [
  vix::vix,
  gk::json,
]
```

Then build:

```bash
vix build
```

## Options

| Option          | Description                                                       |
| --------------- | ----------------------------------------------------------------- |
| `--notes "..."` | Attach release notes to the PR body.                              |
| `--notes=...`   | Same as `--notes "..."`.                                          |
| `--dry-run`     | Validate and print the registry entry without writing or pushing. |
| `--cleanup`     | Remove older local publish branches in the registry clone.        |
| `-h, --help`    | Show command help.                                                |

## Commands reference

| Command                           | Description                                    |
| --------------------------------- | ---------------------------------------------- |
| `vix publish`                     | Publish latest local SemVer tag.               |
| `vix publish 0.2.0`               | Publish explicit version using tag `v0.2.0`.   |
| `vix publish 0.2.0 --dry-run`     | Preview generated registry entry.              |
| `vix publish 0.2.0 --notes "..."` | Publish with notes for the PR body.            |
| `vix publish 0.2.0 --cleanup`     | Publish and remove old local publish branches. |

## Common workflows

### Publish latest tag

```bash
git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0
vix registry sync
vix publish
```

### Publish explicit version

```bash
git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0
vix registry sync
vix publish 0.2.0
```

### Dry run before publishing

```bash
vix publish 0.2.0 --dry-run
```

### Publish with notes

```bash
vix publish 0.2.0 --notes "Add helpers"
```

### Publish and clean old local branches

```bash
vix publish 0.2.0 --cleanup
```

### Add package after publish PR merges

```bash
vix registry sync
vix add gk/json@0.2.0
vix install
vix build
```

## Common mistakes

### Publishing with uncommitted changes

Wrong:

```bash
vix publish 0.2.0
```

while `git status` is not clean.

Correct:

```bash
git status
git add .
git commit -m "chore: prepare release"
vix publish 0.2.0
```

### Creating a tag but not pushing it

Wrong:

```bash
git tag -a v0.2.0 -m "Release v0.2.0"
vix publish 0.2.0
```

Correct:

```bash
git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0
vix publish 0.2.0
```

### Passing the version with `v`

Wrong:

```bash
vix publish v0.2.0
```

Correct:

```bash
vix publish 0.2.0
```

The Git tag uses `v0.2.0`.

The command argument uses `0.2.0`.

### Forgetting registry sync

Wrong:

```bash
vix publish 0.2.0
```

when the registry clone is missing.

Correct:

```bash
vix registry sync
vix publish 0.2.0
```

### Trying to publish an already registered version

If the version already exists in the registry entry, Vix refuses to publish it again.

Create a new version tag instead.

```bash
git tag -a v0.2.1 -m "Release v0.2.1"
git push origin v0.2.1
vix publish 0.2.1
```

### Expecting publish to merge the registry PR

`vix publish` can push a branch and create a PR.

It does not merge the PR.

The version becomes available after the registry PR is merged and users run:

```bash
vix registry sync
```

### Expecting publish to install the package

`vix publish` updates the registry workflow.

It does not install the package into a project.

Use:

```bash
vix add namespace/name@version
vix install
```

after the registry PR has been merged.

## Troubleshooting

### Not inside a Git repository

Run the command inside your package repository.

```bash
cd /path/to/package
vix publish 0.2.0
```

### Working tree is not clean

Check:

```bash
git status
```

Commit or discard changes.

### Tag not found locally

Create the tag:

```bash
git tag -a v0.2.0 -m "Release v0.2.0"
```

Then publish it to origin:

```bash
git push origin v0.2.0
```

### Tag not found on remote origin

Push the tag:

```bash
git push origin v0.2.0
```

or push all tags:

```bash
git push --tags
```

### No publishable Git tag found

Create and push a SemVer tag:

```bash
git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0
vix publish
```

### Cannot infer package namespace/name

Add package identity to `vix.json`:

```json
{
  "namespace": "gk",
  "name": "json"
}
```

or configure a valid Git remote origin.

### Invalid `vix.json`

Make sure `vix.json` is valid JSON and has the required fields:

```json
{
  "namespace": "gk",
  "name": "json"
}
```

### Registry is not available locally

Run:

```bash
vix registry sync
```

Then retry:

```bash
vix publish 0.2.0
```

### Failed to update local registry repo

Vix runs a fast-forward pull in the local registry clone.

If it fails, refresh the registry clone:

```bash
vix registry sync
```

Then retry.

### Failed to push branch

Check Git authentication for the registry remote.

You may need access to push a branch to the registry repository or fork workflow, depending on how the registry is configured.

### GitHub PR was not created

Check GitHub CLI:

```bash
gh --version
gh auth status -h github.com
```

If `gh` is missing or not authenticated, create the PR manually using the branch printed by Vix.

## Best practices

Keep the package repository clean before publishing.

Use annotated Git tags for releases.

Push the tag before publishing.

Run `vix publish --dry-run` before the real publish.

Define `namespace` and `name` explicitly in `vix.json`.

Add package metadata in `vix.json` before first publish.

Run `vix registry sync` before publishing.

Use `--notes` to make the registry PR easier to review.

Do not publish the same version twice.

After the PR merges, test the package from a clean project with:

```bash
vix registry sync
vix add namespace/name@version
vix install
vix build
```

## Related commands

| Command             | Purpose                                         |
| ------------------- | ----------------------------------------------- |
| `vix registry sync` | Clone or refresh the local registry index.      |
| `vix search`        | Search packages in the local registry index.    |
| `vix add`           | Add a published package to a project.           |
| `vix install`       | Install locked project dependencies.            |
| `vix update`        | Update dependency versions.                     |
| `vix outdated`      | Check whether locked dependencies are outdated. |
| `vix list`          | List project dependencies.                      |
| `vix pack`          | Create a local package artifact.                |
| `vix verify`        | Verify a package artifact.                      |

## Next step

Manage the local registry index.

[Open the vix registry guide](/cli/registry)
