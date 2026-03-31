# vix registry

Manage the Vix Registry (Git-based package index).

---

## Usage

```bash
vix registry <subcommand>
```

---

## Subcommands

```bash
sync        Clone or update the registry index (git-based)
path        Print local registry index path
```

---

## Description

The Vix Registry is:

- A Git repository
- Fully offline after sync
- Serverless (no API, no authentication)
- Deterministic and commit-pinned

It maps:

```
<namespace>/<name>@<version> -> immutable git commit SHA
```

This guarantees reproducible builds.

---

## First Step

Before searching or adding packages:

```bash
vix registry sync
```

This clones or updates the local registry index.

---

## Examples

Sync registry:

```bash
vix registry sync
```

Print local registry path:

```bash
vix registry path
```

Search locally:

```bash
vix search tree
```

Add dependency:

```bash
vix add gaspardkirira/tree@0.1.0
```

---

## Publishing a Package

Publishing updates the registry by creating a branch and opening a PR.

```bash
vix publish 0.2.0
```

Requirements:

- You must be inside your library Git repo
- Tag must exist: `v<version>`
- Registry must be synced

```bash
git tag v0.2.0
git push origin v0.2.0
vix registry sync
vix publish 0.2.0
```

---

## Registry Design (V1)

- Pure Git model
- No centralized service
- No runtime dependency on remote APIs
- All searches are local after sync
- Lockfiles pin exact commit SHAs

This keeps dependency resolution transparent and reproducible.

---

## Related Commands

```bash
vix search <query>
vix add <pkg>@<version>
vix list
vix remove <pkg>
vix publish <version>
```

---

The Vix registry is minimal, deterministic, and Git-native by design.

