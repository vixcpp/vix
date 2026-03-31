# vix publish

Publish a package version to the Vix Registry.

---

## Usage

```bash
vix publish <version> [--notes "..."] [--dry-run]
```

---

## Description

`vix publish`:

- Publishes a new version of your library
- Updates the local registry clone (`~/.vix/registry/index`)
- Creates a new branch
- Commits the version metadata
- Pushes to the registry remote
- Opens a PR (if `gh` CLI is available)

Designed for clean, deterministic registry contributions.

---

## Requirements

Before publishing:

- You must be inside your library Git repository
- The Git tag must exist: `v<version>`
- The registry must be synced

```bash
vix registry sync
```

---

## Examples

Publish version:

```bash
vix publish 0.2.0
```

Publish with notes:

```bash
vix publish 0.2.0 --notes "Add count_leaves helper"
```

Dry run (no push):

```bash
vix publish 0.2.0 --dry-run
```

---

## Version Rules

- Version must match Git tag `v<version>`
- Tag must already exist
- Registry entry will reference the exact commit SHA

Example:

```
git tag v0.2.0
git push origin v0.2.0
vix publish 0.2.0
```

---

## What Happens Internally

1. Validate Git tag exists
2. Read tag commit SHA
3. Update registry JSON
4. Commit registry change
5. Push branch
6. Open Pull Request (if `gh` is installed)

---

## Notes

- `--dry-run` performs validation without modifying registry
- Publish is explicit, not automatic
- Lockfiles will pin the commit SHA of the published version

---

`vix publish` integrates your library into the Vix ecosystem.

