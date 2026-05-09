# vix registry

`vix registry` manages the local Vix registry index.

Use it when you need to initialize package metadata, refresh the registry index, or inspect the registry path.

## Usage

```bash
vix registry <subcommand>
```

## Subcommands

| Subcommand | Purpose |
|------------|---------|
| `init` | Create a local `vix.json` manifest for a package |
| `sync` | Update the local registry index |
| `path` | Print the local registry index path |

## Basic usage

```bash
vix registry init
vix registry sync
vix registry path
```

## Initialize package metadata

```bash
vix registry init
vix registry init --force   # overwrite existing vix.json
```

## Sync the registry index

```bash
vix registry sync
```

Run this before `vix add`, `vix update`, `vix outdated`, or `vix publish` to ensure fresh package metadata.

## Registry init options

| Option | Description |
|--------|-------------|
| `--force` | Overwrite an existing `vix.json` file. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# Initialize a package manifest
vix registry init

# Refresh registry before adding a package
vix registry sync
vix add gk/jwt

# Refresh registry before checking outdated packages
vix registry sync
vix outdated

# Show registry location
vix registry path

# Publish then sync
vix publish 0.2.0
vix registry sync
```

## Common mistakes

### Forgetting to sync before adding a new package

```bash
vix registry sync
vix add namespace/name
```

### Expecting registry sync to install dependencies

`vix registry sync` only refreshes metadata. Use `vix install` to install dependencies.

## Related commands

| Command | Purpose |
|---------|---------|
| `vix publish` | Publish package versions |
| `vix add` | Add packages to a project |
| `vix install` | Install locked dependencies |
| `vix update` | Update dependencies |
| `vix outdated` | Check outdated dependencies |
| `vix store` | Manage local package store |

## Next step

Continue with the local store.

[Open the vix store guide](/cli/store)
