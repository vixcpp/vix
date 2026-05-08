# vix store

`vix store` manages the local Vix package store.

Use it when you want to inspect the store location or garbage collect unused package data.

## Usage

```bash
vix store <subcommand> [options]
```

## Subcommands

| Subcommand | Purpose |
|------------|---------|
| `path` | Print the local store root path |
| `gc` | Garbage collect the store |

## Basic usage

```bash
vix store path
vix store gc
vix store gc --dry-run
vix store gc --project
```

## Garbage collection

```bash
vix store gc
```

### Dry run

```bash
vix store gc --dry-run
```

### Project-scoped GC

```bash
vix store gc --project
```

> **Warning:** `vix store gc --project` is destructive for other projects sharing the same store. Always use `--dry-run` first.

```bash
vix store gc --project --dry-run
vix store gc --project
```

## `gc` options

| Option | Description |
|--------|-------------|
| `--project` | Scope garbage collection to the current project using `vix.lock`. |
| `--dry-run` | List files that would be removed without deleting them. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# Show store path
vix store path

# Preview store cleanup
vix store gc --dry-run

# Clean unused store data
vix store gc

# Safe cleanup workflow
vix store path
vix store gc --dry-run
vix store gc

# Project-scoped cleanup
vix store path
vix store gc --project --dry-run
vix store gc --project
```

## Common mistakes

### Running project-scoped GC without previewing

```bash
# Always preview first
vix store gc --project --dry-run
```

### Expecting store GC to remove project build files

Use `vix clean` or `vix reset` for project-local build directories.

### Confusing registry and store

| Command | Purpose |
|---------|---------|
| `vix registry` | Manage registry metadata |
| `vix store` | Manage local package data |

## Related commands

| Command | Purpose |
|---------|---------|
| `vix registry` | Manage local registry index |
| `vix add` | Add a dependency |
| `vix install` | Install project dependencies |
| `vix clean` | Remove project-local cache directories |
| `vix reset` | Clean and reinstall the project |
| `vix info` | Show store, registry, cache, and environment paths |

## Next step

Continue with ORM tooling.

[Open the vix orm guide](/cli/orm)
