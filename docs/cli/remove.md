# vix remove

`vix remove` removes a package dependency from your project.

Use it when your project no longer needs a package.

## Usage

```bash
vix remove [@]namespace/name[@version]
vix remove [@]namespace/name[@version] --purge [-y]
```

## What it does

`vix remove` removes a dependency from the current project. By default, it keeps cached package files. Use `--purge` to also delete local package files.

## Basic usage

```bash
vix remove gk/jwt
vix remove @gk/jwt
vix remove gk/jwt@1.0.0
vix remove @gk/jwt@1.0.0
```

## Remove and purge files

```bash
vix remove @gk/jwt --purge
vix remove @gk/jwt --purge -y
```

## What files are updated

`vix remove` updates `vix.lock` and project dependency links. Typical affected paths: `.vix/deps/`, `.vix/vix_deps.cmake`.

## After removing a package

```bash
vix remove gk/jwt
vix install
vix check --tests
```

## Difference between `vix remove` and `vix uninstall`

| Command | Purpose |
|---------|---------|
| `vix remove` | Remove a dependency from the current project |
| `vix uninstall -g <pkg>` | Remove a globally installed package |

## Options

| Option | Description |
|--------|-------------|
| `--purge` | Delete local package files after removing the dependency. |
| `-y` | Skip confirmation when using `--purge`. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
vix remove gk/jwt                   # remove a dependency
vix remove @gk/jwt                  # remove a scoped dependency
vix remove @gk/jwt --purge -y       # remove and purge local files
vix remove gk/jwt && vix install    # remove then reinstall state
vix remove gk/jwt && vix check --tests  # remove and validate
```

## Common mistakes

### Expecting remove to unpublish a package

`vix remove` only affects the current project, not the registry.

### Forgetting to update source code

After removing a dependency, remove any `#include` references to it, then run `vix check --tests`.

### Confusing project remove with global uninstall

```bash
vix remove gk/jwt        # project dependency
vix uninstall -g gk/jwt  # global package
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix add` | Add a dependency |
| `vix install` | Install dependencies from `vix.lock` |
| `vix update` | Update dependencies |
| `vix outdated` | Check outdated dependencies |
| `vix list` | List dependencies |
| `vix uninstall` | Remove Vix or a global package |
| `vix check` | Validate after removing dependencies |

## Next step

Continue with dependency listing.

[Open the vix list guide](/cli/list)
