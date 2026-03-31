# vix search

Search packages in the local registry index (offline).

---

## Usage

```bash
vix search <query>
```

---

## Description

`vix search` queries the local registry index.

- Works fully offline
- No network request required
- Uses the locally synced registry data

Before searching for the first time, you should sync the registry.

---

## Sync Registry

```bash
vix registry sync
```

---

## Examples

Search by package name:

```bash
vix search tree
```

Search by namespace or author:

```bash
vix search gaspardkirira
```

---

## Behavior Notes

- Results come from the local registry index.
- If no results are found, ensure you ran `vix registry sync`.
- Search is case-insensitive.

---

## Design Goals

- Fast local search
- Offline-first dependency model
- Deterministic package discovery

---

`vix search` is part of the Vix registry workflow.

