# vix add

Add a dependency from the Vix registry.

---

## Usage

```bash
vix add <namespace>/<name>@<version>
```

---

## Description

`vix add`:

- Adds a dependency to your project
- Requires an exact version (V1 registry model)
- Pins the resolved commit SHA in `vix.lock`
- Works with the local registry index

Before adding a dependency, you should sync the registry.

---

## Sync Registry

```bash
vix registry sync
```

---

## Example

```bash
vix add gaspardkirira/tree@0.1.0
```

---

## Versioning Rules

- Exact version required
- No version ranges in V1
- Version is resolved to a commit SHA
- The commit is stored in `vix.lock`

This ensures:

- Reproducible builds
- Deterministic dependency resolution
- No implicit upgrades

---

## What Happens Internally

1. Registry index is searched locally
2. Version is resolved to a commit
3. Entry is written to `vix.lock`
4. Dependencies are installed via:

```bash
vix deps
```

---

## Notes

- Requires prior `vix registry sync`
- Lockfile must be committed to version control
- Exact version required in current registry model

---

`vix add` is part of the deterministic dependency workflow in Vix.

