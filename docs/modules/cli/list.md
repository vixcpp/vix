# vix list

List project dependencies from `vix.lock`.

---

## Usage

```bash
vix list
```

---

## Description

`vix list`:

- Reads the local `vix.lock`
- Displays all pinned dependencies
- Shows exact versions
- Shows resolved commit references

This command does not access the network.

---

## Example

```bash
vix list
```

Example output:

```
gaspardkirira/tree@0.1.0
adastra/json@0.3.2
```

---

## Behavior Notes

- Works offline
- Requires a `vix.lock` file
- Reflects the exact state of the project
- Useful for auditing dependency graph

---

## Typical Workflow

After adding or removing:

```bash
vix add gaspardkirira/tree@0.1.0
vix list
```

---

`vix list` provides a deterministic view of your dependency state.

