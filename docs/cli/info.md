# vix info

`vix info` shows Vix environment information, paths, caches, and local state.

Use it when you want to inspect where Vix stores data and understand the current local setup.

## Usage

```bash
vix info
```

## What it does

`vix info` prints useful information about your Vix installation and local environment, including: Vix version, root path, registry index path and state, store cache path and state, global packages manifest, build artifact cache path, disk usage, and package counts.

## Basic usage

```bash
vix info
```

## What it helps debug

Use `vix info` when debugging: dependency installation issues, registry sync issues, package store state, global package state, build artifact cache usage, disk usage problems, or unexpected cache behavior.

## Relationship with other commands

`vix info` is read-only. It does not modify your project or global Vix state. Use it before cleanup commands when you want to understand what exists.

```bash
vix info
vix store path
vix store gc --dry-run
```

## Common workflows

```bash
vix info
vix info && vix store gc --dry-run
vix install -g gk/jwt && vix info && vix list -g
vix registry sync && vix info
vix reset && vix info
```

## Common mistakes

### Expecting `vix info` to fix problems

`vix info` only prints information. Use `vix doctor` for environment checks, `vix clean` for project cleanup, `vix reset` for dependency reinstall.

### Expecting `vix info` to update the registry

```bash
vix registry sync  # to update registry metadata
```

### Expecting `vix info` to remove cache data

```bash
vix store gc        # to clean the store
vix store gc --dry-run  # preview first
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix doctor` | Check environment health |
| `vix registry path` | Print registry index path |
| `vix registry sync` | Refresh registry index |
| `vix store path` | Print local store path |
| `vix store gc` | Clean unused store data |
| `vix list -g` | List global packages |
| `vix clean` | Clean project-local generated state |
| `vix reset` | Clean and reinstall project dependencies |

## Next step

Continue with environment diagnostics.

[Open the vix doctor guide](/cli/doctor)
