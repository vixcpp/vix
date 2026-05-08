# vix publish

`vix publish` publishes a tagged version of the current package to the Vix registry.

Use it when your package is ready, tagged, pushed to origin, and should become available through the registry workflow.

## Usage

```bash
vix publish [version] [--notes "..."] [--dry-run]
```

## What it does

`vix publish` prepares and publishes a package version to the Vix registry. It verifies that the tag exists locally and on origin, attaches release notes, and can validate the publish flow without pushing changes.

## Requirements

Before publishing: be inside a Git repository, have a valid version tag locally and on origin, using the `v<version>` format.

```bash
# Required tag format
v0.2.0
```

## Basic usage

```bash
vix publish               # latest local SemVer tag
vix publish 0.2.0         # explicit version
vix publish --notes "Add helpers"
vix publish --dry-run
```

## Create a tag before publishing

```bash
git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0
vix publish 0.2.0
```

## Dry run

```bash
vix publish --dry-run
vix publish 0.2.0 --dry-run
```

## Cleanup

```bash
vix publish --cleanup
vix publish 0.2.0 --cleanup
```

## Options

| Option | Description |
|--------|-------------|
| `--notes "..."` | Attach release notes. |
| `--dry-run` | Validate without pushing changes. |
| `--cleanup` | Remove older local publish branches in the registry clone. |
| `-h, --help` | Show command help. |

## Recommended publish flow

```bash
git status
vix fmt --check
vix check --tests
vix build --preset release

git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0

vix publish 0.2.0 --dry-run
vix publish 0.2.0 --notes "Add helpers"
```

## Common mistakes

### Publishing before creating a tag

```bash
git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0
vix publish 0.2.0
```

### Creating the tag but not pushing it

`vix publish` requires the tag to exist both locally and on origin.

### Passing `v0.2.0` instead of `0.2.0`

The command accepts `0.2.0`. The Git tag uses the `v` prefix: `v0.2.0`.

## Related commands

| Command | Purpose |
|---------|---------|
| `vix pack` | Create a local package artifact |
| `vix verify` | Verify a package artifact |
| `vix cache` | Store a package locally |
| `vix registry` | Manage registry-related workflows |
| `vix add` | Add a published package to a project |

## Next step

Continue with registry management.

[Open the vix registry guide](/cli/registry)
