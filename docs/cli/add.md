# vix add

`vix add` adds a package dependency to your project.

Use it when your project needs a package from the Vix registry.

## Usage

```bash
vix add [@]namespace/name[@version]
```

## What it does

`vix add` resolves a package, installs it, and updates the project dependency files. It updates both `vix.json` with the declared dependency requirement and `vix.lock` with the exact resolved version.

## Basic usage

```bash
vix add gk/jwt
vix add gk/jwt@^1.0.0
vix add @gk/jwt
vix add @gk/jwt@~1.2.0
```

## Package name format
namespace/name    →  gk/jwt
@namespace/name   →  @gk/jwt

## Version format

```bash
vix add gk/jwt            # latest version
vix add gk/jwt@1.0.0      # exact version
vix add gk/jwt@^1.0.0     # compatible range
vix add gk/jwt@~1.2.0     # patch-level range
vix add @gk/jwt@^1.0.0    # scoped with range
```

## What files are updated

`vix add` updates two files.

`vix.json` stores declared dependency requirements:

```json
{
  "dependencies": {
    "gk/jwt": "^1.0.0"
  }
}
```

`vix.lock` stores exact resolved versions for reproducible installs.

## After adding a dependency

```bash
vix add gk/jwt
vix build
vix dev
```

Typical local outputs: `.vix/deps/`, `.vix/vix_deps.cmake`

## Registry sync

If a package is not found, refresh the local registry index:

```bash
vix registry sync
vix add gk/jwt
```

## Common workflows

```bash
# Add latest package
vix add gk/jwt

# Add a package with compatible updates
vix add gk/jwt@^1.0.0

# Add a scoped package
vix add @gk/jwt

# Refresh registry before adding
vix registry sync
vix add gk/jwt

# Add and validate
vix add gk/jwt
vix check --tests
```

## Common mistakes

### Forgetting to sync the registry

```bash
vix registry sync
vix add gk/jwt
```

### Confusing `vix add` and `vix install`

| Command | Purpose |
|---------|---------|
| `vix add` | Add or change a dependency |
| `vix install` | Install exact locked dependencies |

### Expecting `vix add` to update all packages

Use `vix update` to update existing dependencies.

## Related commands

| Command | Purpose |
|---------|---------|
| `vix install` | Install dependencies from `vix.lock` |
| `vix update` | Update dependencies |
| `vix outdated` | Check outdated dependencies |
| `vix remove` | Remove dependencies |
| `vix list` | List dependencies |
| `vix registry sync` | Refresh local registry index |

## Next step

Continue with dependency installation.

[Open the vix install guide](/cli/install)
