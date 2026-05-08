# vix clean

`vix clean` removes local project cache directories.

Use it when you want to clean generated project state without deleting the global Vix cache.

## Usage

```bash
vix clean
```

## What it does

`vix clean` removes local build and cache directories from the current project — specifically `.vix/` and `build/`.

## What it does not remove

`vix clean` does not remove the global Vix directory (`~/.vix/`). Global packages, registry data, and global cache data are kept.

## Basic usage

```bash
cd api
vix clean
```

## Common workflow

```bash
vix clean
vix install
vix build
```

For this complete workflow in one command, use `vix reset`.

## Difference between `vix clean` and `vix reset`

| Command | What it does |
|---------|-------------|
| `vix clean` | Removes `.vix/` and `build/` |
| `vix reset` | Runs `vix clean`, then `vix install` |

## Local files affected

Common affected paths: `.vix/`, `build/`, `build-ninja/`, `build-release/`, `.vix/deps/`, `.vix/vix_deps.cmake`.

## Common workflows

```bash
# Clean after dependency issues
vix clean
vix install
vix build

# Clean before a fresh dev run
vix clean
vix install
vix dev

# Clean before validation
vix clean
vix install
vix check --tests
```

## Common mistakes

### Expecting `vix clean` to remove global cache

`vix clean` does not remove `~/.vix/`. For global store issues, use `vix store`, `vix registry`, or `vix info`.

### Forgetting to reinstall dependencies

After cleaning, run `vix install` before building.

### Running outside the project directory

```bash
# Wrong
cd ..
vix clean

# Correct
cd api
vix clean
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix reset` | Clean and reinstall project dependencies |
| `vix install` | Install dependencies from `vix.lock` |
| `vix build` | Configure and build project |
| `vix check` | Validate project |
| `vix info` | Show Vix paths and cache locations |
| `vix store` | Manage the local package store |

## Next step

Continue with project reset.

[Open the vix reset guide](/cli/reset)
