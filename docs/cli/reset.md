# vix reset

`vix reset` resets the local project state.

It runs `vix clean`, then runs `vix install`.

Use it when you want to remove local generated project state and reinstall project dependencies in one command.

```bash
vix reset
```

## Overview

`vix reset` is the full project-local reset command.

It performs two steps:

```txt
1. vix clean
2. vix install
```

This is useful when your local project state is stale, broken, or needs to be regenerated from a clean dependency install.

It only affects the current project.

It does not remove the global Vix directory.

## Usage

```bash
vix reset
```

`vix reset` does not accept arguments in the current implementation.

If you pass any argument, Vix prints the help output.

## What it does

`vix reset` runs:

```bash
vix clean
```

then:

```bash
vix install
```

The clean step removes local project cache directories.

The install step reinstalls project dependencies from the project lockfile.

## What it removes

Because `vix reset` calls `vix clean`, it removes:

```txt
.vix/
build/
```

from the current project.

## What it reinstalls

Because `vix reset` calls `vix install`, it reinstalls dependencies from:

```txt
vix.lock
```

and regenerates project dependency integration state.

Typical generated files or folders may include:

```txt
.vix/
.vix/deps/
.vix/vix_deps.cmake
```

depending on the project and dependency setup.

## What it does not remove

`vix reset` does not remove global Vix state.

It does not remove:

```txt
~/.vix/
~/.vix/registry/
~/.vix/store/
~/.vix/global/
~/.vix/cache/
```

So global packages, registry metadata, global store data, and global caches remain available.

## Basic usage

Run it inside a Vix project:

```bash
cd api
vix reset
```

Output shape:

```txt
Reset
  cleaning project cache...

Clean
  removing .vix/
  removing build/

✔ Project cache cleaned

  reinstalling project dependencies...

...

✔ Project reset complete
```

If the clean step fails, reset stops.

If the install step fails, reset stops.

## Equivalent manual workflow

These are equivalent:

```bash
vix reset
```

and:

```bash
vix clean
vix install
```

Use `vix reset` when you want the full workflow in one command.

## When to use it

Use `vix reset` when:

```txt
dependency integration files are stale
.vix/ state is broken
a project install behaves strangely
you want a clean local dependency reinstall
you want to rebuild project state after changing dependency files
you want to recover from local generated-state issues
```

Common flow:

```bash
vix reset
vix build
```

or:

```bash
vix reset
vix dev
```

## Reset before development

```bash
vix reset
vix dev
```

Use this when development mode behaves strangely because local generated state may be stale.

## Reset before validation

```bash
vix reset
vix check --tests
```

Use this when you want to validate the project from a fresh local install state.

## Reset before a release build

```bash
vix reset
vix build --preset release
```

Use this when preparing a release build and you want dependency state to be regenerated first.

## Reset after dependency issues

```bash
vix registry sync
vix reset
vix build
```

Use this when dependency metadata may be outdated or local dependency state may be broken.

## Difference between `vix clean` and `vix reset`

| Command     | Purpose                                  |
| ----------- | ---------------------------------------- |
| `vix clean` | Remove local project cache directories.  |
| `vix reset` | Run `vix clean`, then run `vix install`. |

Use `vix clean` when you only want to remove generated project state.

Use `vix reset` when you want cleanup plus dependency reinstall.

## Difference between `vix reset` and `vix install`

| Command       | Purpose                                             |
| ------------- | --------------------------------------------------- |
| `vix install` | Install dependencies from the lockfile.             |
| `vix reset`   | Clean local state first, then install dependencies. |

Use `vix install` after cloning a clean project.

Use `vix reset` when the local generated state already exists and may be wrong.

## Difference between `vix reset` and global cleanup

`vix reset` is project-local.

It does not clean:

```txt
global registry index
global package store
global installed packages
global artifact cache
```

For global inspection, use:

```bash
vix info
```

For package store cleanup, use:

```bash
vix store path
vix store gc --project --dry-run
```

## Failure behavior

`vix reset` runs the clean step first.

If clean fails, Vix prints:

```txt
reset failed during clean
```

and returns the clean command exit code.

Then it runs install.

If install fails, Vix prints:

```txt
reset failed during install
```

and returns the install command exit code.

If both steps succeed, Vix prints:

```txt
Project reset complete
```

## Arguments

The current implementation does not accept extra arguments.

Correct:

```bash
vix reset
```

Wrong:

```bash
vix reset --force
vix reset --clean
vix reset api
```

If arguments are provided, Vix shows the help output.

## Options

`vix reset` currently has no options.

| Command            | Description                               |
| ------------------ | ----------------------------------------- |
| `vix reset`        | Clean and reinstall project dependencies. |
| `vix reset --help` | Show command help.                        |

## Commands reference

| Command       | Description                                   |
| ------------- | --------------------------------------------- |
| `vix reset`   | Run `vix clean`, then `vix install`.          |
| `vix clean`   | Remove `.vix/` and `build/`.                  |
| `vix install` | Install project dependencies from `vix.lock`. |
| `vix build`   | Build the project.                            |
| `vix dev`     | Run the project in development mode.          |
| `vix check`   | Validate project health.                      |
| `vix info`    | Inspect Vix paths, caches, and local state.   |

## Common workflows

### Reset project state

```bash
vix reset
```

### Reset before development

```bash
vix reset
vix dev
```

### Reset before validation

```bash
vix reset
vix check --tests
```

### Reset before release build

```bash
vix reset
vix build --preset release
```

### Reset after dependency problems

```bash
vix registry sync
vix reset
vix build
```

### Manual equivalent

```bash
vix clean
vix install
```

## Common mistakes

### Running outside the project directory

Wrong:

```bash
vix new api
vix reset
```

Correct:

```bash
vix new api
cd api
vix reset
```

`vix reset` works on the current directory.

### Using reset when only a rebuild is needed

If you only need to rebuild, use:

```bash
vix build
```

If you need a clean build directory, use:

```bash
vix clean
vix build
```

Use `vix reset` when dependency state also needs to be regenerated.

### Expecting reset to update dependency versions

`vix reset` runs `vix install`.

It installs the versions already pinned in:

```txt
vix.lock
```

It does not update dependencies to newer versions.

To update dependencies, use:

```bash
vix update --install
```

### Expecting reset to refresh the registry

`vix reset` does not run:

```bash
vix registry sync
```

If packages cannot be found or registry metadata is stale, run:

```bash
vix registry sync
vix reset
```

### Expecting reset to remove global cache

`vix reset` does not remove:

```txt
~/.vix/
```

Use `vix info` and `vix store` for global state inspection.

### Passing options to reset

Wrong:

```bash
vix reset --force
```

Correct:

```bash
vix reset
```

The current command has no options.

## Troubleshooting

### `reset failed during clean`

The clean step failed.

Check permissions on:

```txt
.vix/
build/
```

Then try:

```bash
vix clean
```

If needed, remove the folders manually.

### `reset failed during install`

The install step failed.

Common causes:

```txt
missing vix.lock
invalid lockfile
registry not synced
dependency not found
network or git issue
broken package metadata
```

Try:

```bash
vix registry sync
vix install
```

Then:

```bash
vix reset
```

### Missing lockfile

If the project does not have:

```txt
vix.lock
```

`vix install` may fail depending on the project setup.

Add dependencies with:

```bash
vix add <namespace>/<name>
```

or regenerate dependency state using the correct project workflow.

### Registry not synced

If install cannot find a package, run:

```bash
vix registry sync
vix reset
```

### Nothing was cleaned

If `.vix/` and `build/` do not exist, `vix clean` reports:

```txt
Nothing to clean
```

That is not an error.

`vix reset` can still continue to `vix install`.

### Project still fails after reset

Run:

```bash
vix doctor
vix info
vix check --tests
```

If the issue is dependency-related:

```bash
vix registry sync
vix reset
vix check --tests
```

## Best practices

Run `vix reset` from the project root.

Use it when dependency integration state looks broken.

Use `vix registry sync` before reset if package metadata may be outdated.

Use `vix install` instead of `vix reset` after a fresh clone when no generated state exists yet.

Use `vix clean` instead of `vix reset` when you only want to remove local build state.

Use `vix update --install` when you want newer dependency versions.

Use `vix info` before touching global cache or store data.

## Related commands

| Command             | Purpose                                                     |
| ------------------- | ----------------------------------------------------------- |
| `vix clean`         | Remove local project cache directories.                     |
| `vix install`       | Install dependencies from `vix.lock`.                       |
| `vix update`        | Resolve newer dependency versions and rewrite the lockfile. |
| `vix add`           | Add a dependency to the project.                            |
| `vix remove`        | Remove a dependency from the project.                       |
| `vix registry sync` | Refresh registry metadata.                                  |
| `vix build`         | Build the project.                                          |
| `vix dev`           | Run development mode.                                       |
| `vix check`         | Validate project health.                                    |
| `vix info`          | Show Vix paths and local state.                             |
| `vix doctor`        | Diagnose environment health.                                |

## Next step

Continue with dependency management.

[Open the vix add guide](/cli/add)
