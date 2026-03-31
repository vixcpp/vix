# vix remove

Remove a dependency from the project lockfile.

---

## Usage

```bash
vix remove <pkg>
vix remove <pkg>@<version>
```

---

## Description

`vix remove`:

- Removes a dependency entry from `vix.lock`
- Supports removing by package name only
- Or by package + specific version

The operation updates the lockfile only.
To fully apply changes, run:

```bash
vix deps
```

---

## Examples

Remove by package:

```bash
vix remove gaspardkirira/tree
```

Remove specific version:

```bash
vix remove gaspardkirira/tree@0.1.0
```

---

## Behavior Notes

- Works on the local `vix.lock`
- Does not modify registry
- Deterministic removal (no implicit upgrades/downgrades)
- Safe to commit after removal

---

## Typical Workflow

```bash
vix remove gaspardkirira/tree
vix deps
vix build
```

---

`vix remove` keeps dependency management explicit and reproducible.

