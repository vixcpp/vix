# vix clean

`vix clean` removes local project cache directories.

Use it when you want to clean generated state from the current project without deleting the global Vix cache.

```bash
vix clean
```

## Overview

`vix clean` is a project-local cleanup command.

It removes only:

```txt
.vix/
build/
```

It does not remove global Vix data.

That means it keeps:

```txt
~/.vix/
~/.vix/registry/
~/.vix/store/
~/.vix/global/
~/.vix/cache/
```

Use `vix clean` when the project-local state is stale, broken, or no longer needed.

## Usage

```bash
vix clean
```

## What it removes

`vix clean` removes these directories from the current project:

```txt
.vix/
build/
```

Example:

```txt
my-app/
├── .vix/
├── build/
├── src/
├── vix.app
└── vix.json
```

After:

```bash
vix clean
```

the project becomes:

```txt
my-app/
├── src/
├── vix.app
└── vix.json
```

## What it does not remove

`vix clean` does not remove global Vix state.

It does not remove:

```txt
~/.vix/
~/.vix/registry/index/
~/.vix/store/git/
~/.vix/global/installed.json
~/.vix/cache/build/
```

So your registry index, global packages, and global store remain available.

## Basic usage

Run it inside a Vix project:

```bash
cd api
vix clean
```

Output shape:

```txt
Clean
  removing .vix/
  removing build/

✔ Project cache cleaned
```

If nothing exists to clean:

```txt
Clean

⚠ Nothing to clean
```

## When to use it

Use `vix clean` when:

```txt
local generated state is stale
the project cache is broken
you want a fresh local build directory
you want to remove generated project files
you want to retry dependency integration cleanly
```

Common cases:

```bash
vix clean
vix install
vix build
```

or:

```bash
vix clean
vix install
vix dev
```

## Clean before rebuilding

```bash
vix clean
vix build
```

Use this when you want to remove the default local build directory before building again.

## Clean before development mode

```bash
vix clean
vix install
vix dev
```

Use this when project-local generated state may be stale before running development mode.

## Clean after dependency issues

```bash
vix clean
vix install
vix check --tests
```

Use this when dependency integration files under `.vix/` may be outdated or broken.

## Difference between `vix clean` and `vix reset`

| Command     | Purpose                                                 |
| ----------- | ------------------------------------------------------- |
| `vix clean` | Remove `.vix/` and `build/` from the current project.   |
| `vix reset` | Run project cleanup and reinstall project dependencies. |

Use `vix clean` when you only want to delete local generated state.

Use `vix reset` when you want cleanup plus dependency reinstall.

## Clean workflow

Manual workflow:

```bash
vix clean
vix install
vix build
```

Equivalent higher-level workflow:

```bash
vix reset
vix build
```

## Important behavior

`vix clean` ignores extra arguments in the current implementation.

The intended command is:

```bash
vix clean
```

There are no command options in the current implementation.

## Options

`vix clean` currently has no options.

| Command            | Description                                           |
| ------------------ | ----------------------------------------------------- |
| `vix clean`        | Remove `.vix/` and `build/` from the current project. |
| `vix clean --help` | Show command help.                                    |

## Commands reference

| Command       | Description                               |
| ------------- | ----------------------------------------- |
| `vix clean`   | Clean project-local generated state.      |
| `vix reset`   | Clean and reinstall project dependencies. |
| `vix install` | Install dependencies from `vix.lock`.     |
| `vix build`   | Build the project.                        |
| `vix dev`     | Run the development workflow.             |
| `vix info`    | Inspect Vix paths and cache locations.    |
| `vix store`   | Manage the local package store.           |

## Common workflows

### Clean local project state

```bash
vix clean
```

### Clean and reinstall dependencies

```bash
vix clean
vix install
```

### Clean and rebuild

```bash
vix clean
vix install
vix build
```

### Clean before dev mode

```bash
vix clean
vix install
vix dev
```

### Clean before validation

```bash
vix clean
vix install
vix check --tests
```

## Common mistakes

### Expecting `vix clean` to remove global cache

`vix clean` does not remove:

```txt
~/.vix/
```

For global store cleanup, use:

```bash
vix store path
vix store gc --project --dry-run
```

For registry metadata, use:

```bash
vix registry path
vix registry sync
```

For environment inspection, use:

```bash
vix info
```

### Expecting `vix clean` to reinstall dependencies

`vix clean` only removes local directories.

After cleaning, run:

```bash
vix install
```

or use:

```bash
vix reset
```

when you want cleanup plus reinstall.

### Running outside the project directory

Wrong:

```bash
cd ..
vix clean
```

Correct:

```bash
cd api
vix clean
```

`vix clean` works relative to the current directory.

### Expecting it to remove every build directory

The current implementation removes:

```txt
.vix/
build/
```

It does not remove every possible custom build folder.

If your project uses a custom build folder such as:

```txt
build-ninja/
build-release/
```

remove it manually or use the command that created it if another workflow manages that directory.

### Expecting it to remove global packages

`vix clean` does not remove global packages.

Use:

```bash
vix list -g
vix uninstall -g <package>
```

for global packages.

## Troubleshooting

### `Nothing to clean`

This means neither `.vix/` nor `build/` exists in the current directory.

It is not an error.

You can continue with:

```bash
vix install
vix build
```

### Project still behaves strangely after clean

Run:

```bash
vix reset
```

or manually run:

```bash
vix clean
vix install
vix check --tests
```

### Dependency state still looks wrong

Refresh the registry and reinstall:

```bash
vix registry sync
vix clean
vix install
```

### Global cache issue remains

Inspect Vix global paths:

```bash
vix info
```

Then inspect the store:

```bash
vix store path
```

Preview cleanup before deleting anything:

```bash
vix store gc --project --dry-run
```

## Best practices

Run `vix clean` from the project root.

Use it for project-local generated state only.

Run `vix install` after cleaning when the project depends on registry packages.

Use `vix reset` when you want the full clean plus reinstall workflow.

Use `vix info` before touching global cache or store data.

Do not use `vix clean` when you only want to remove a dependency. Use `vix remove`.

Do not use `vix clean` when you want to remove Vix from the machine. Use `vix uninstall`.

## Related commands

| Command         | Purpose                                   |
| --------------- | ----------------------------------------- |
| `vix reset`     | Clean and reinstall project dependencies. |
| `vix install`   | Install dependencies from `vix.lock`.     |
| `vix build`     | Build the project.                        |
| `vix dev`       | Run the project in development mode.      |
| `vix check`     | Validate project health.                  |
| `vix info`      | Show Vix paths and cache locations.       |
| `vix store`     | Manage the local package store.           |
| `vix remove`    | Remove a project dependency.              |
| `vix uninstall` | Remove Vix or a global package.           |

## Next step

Continue with project reset.

[Open the vix reset guide](/cli/reset)
