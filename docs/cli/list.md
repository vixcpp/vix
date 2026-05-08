# vix list

`vix list` lists project dependencies or globally installed packages.

Use it when you want to inspect what packages are currently installed for a project or globally in your Vix environment.

## Usage

```bash
vix list
vix list -g
```

## What it does

| Mode | Command | Purpose |
|------|---------|---------|
| Project mode | `vix list` | List project dependencies from `vix.lock` |
| Global mode | `vix list -g` | List globally installed packages |

## Project dependencies

```bash
cd api
vix list
```

Reads from `vix.lock`.

## Global packages

```bash
vix list -g
```

Reads from `~/.vix/global/installed.json`.

## Difference between project and global list

| Command | Reads from | Scope |
|---------|-----------|-------|
| `vix list` | `vix.lock` | Current project |
| `vix list -g` | `~/.vix/global/installed.json` | Global Vix environment |

## Options

| Option | Description |
|--------|-------------|
| `-g, --global` | List globally installed packages. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# List project dependencies
vix list

# List global packages
vix list -g

# Add and list
vix add gk/jwt
vix list

# Update and list
vix update --install
vix list

# Remove and list
vix remove gk/jwt
vix list
```

## CI usage

```bash
vix install
vix list
vix check --tests
```

## Common mistakes

### Running project list outside a project

```bash
# Wrong
cd ..
vix list

# Correct
cd api
vix list
```

### Expecting `vix list` to show outdated packages

Use `vix outdated` for that.

### Confusing project and global packages

```bash
vix list     # project packages
vix list -g  # global packages
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix add` | Add a dependency |
| `vix install` | Install dependencies |
| `vix update` | Update dependencies |
| `vix outdated` | Check outdated dependencies |
| `vix remove` | Remove a dependency |
| `vix install -g` | Install a global package |
| `vix upgrade -g` | Upgrade a global package |
| `vix uninstall -g` | Remove a global package |

## Next step

Continue with packaging.

[Open the vix pack guide](/cli/pack)
