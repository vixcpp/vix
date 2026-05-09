# vix reset

`vix reset` resets the local project state.

It runs `vix clean`, then runs `vix install`.

Use it when you want to remove local generated project state and reinstall dependencies in one command.

## Usage

```bash
vix reset
```

## What it does

`vix reset` performs two steps: `vix clean` then `vix install`. It removes local project cache directories and build output, then reinstalls dependencies from the lockfile and regenerates local dependency integration files.

## What it does not affect

`vix reset` does not remove the global Vix directory (`~/.vix/`). Global packages, registry indexes, global stores, and global caches remain available.

## Basic usage

```bash
cd api
vix reset
```

## Equivalent manual workflow

```bash
# These two are equivalent
vix reset

vix clean
vix install
```

## Common workflow

```bash
vix reset
vix build
vix dev
```

## Difference between `vix clean` and `vix reset`

| Command | What it does |
|---------|-------------|
| `vix clean` | Removes local project cache directories |
| `vix reset` | Runs `vix clean`, then `vix install` |

## Common workflows

```bash
# Reset before development
vix reset
vix dev

# Reset before validation
vix reset
vix check --tests

# Reset before release build
vix reset
vix build --preset release

# Reset after dependency problems
vix reset
vix build
```

## Common mistakes

### Running outside the project directory

```bash
# Wrong
vix new api
vix reset

# Correct
vix new api
cd api
vix reset
```

### Using `vix reset` when only a rebuild is needed

```bash
# For rebuild only
vix build --clean
# or
vix clean
vix build
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix clean` | Remove local project cache directories |
| `vix install` | Install dependencies from `vix.lock` |
| `vix build` | Configure and build project |
| `vix check` | Validate project |
| `vix dev` | Run the app in development mode |
| `vix info` | Show Vix paths and cache locations |

## Next step

Continue with dependency management.

[Open the vix add guide](/cli/add)
