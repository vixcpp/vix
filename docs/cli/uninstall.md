# vix uninstall

`vix uninstall` removes the Vix CLI or a globally installed package.

Use it when you want to remove Vix from your machine, remove a global package, or clean up local Vix data.

## Usage

```bash
vix uninstall [options]
vix uninstall -g <pkg>
```

## What it does

| Mode | Command | Purpose |
|------|---------|---------|
| CLI mode | `vix uninstall` | Remove the Vix CLI binary |
| Global package mode | `vix uninstall -g <pkg>` | Remove a globally installed package |

## Basic usage

```bash
vix uninstall                   # remove detected CLI
vix uninstall --purge           # remove CLI + local store/cache data
vix uninstall -g gk/jwt         # remove a global package
```

## Uninstall options

```bash
vix uninstall --all             # remove every detected Vix binary
vix uninstall --all --system    # include system locations
vix uninstall --prefix /usr/local   # remove <prefix>/bin/vix
vix uninstall --path /usr/local/bin/vix  # remove explicit binary
```

## Difference between CLI uninstall and package uninstall

| Command | Removes |
|---------|---------|
| `vix uninstall` | Vix CLI binary |
| `vix uninstall --purge` | Vix CLI binary + local store/cache data |
| `vix uninstall -g <pkg>` | One globally installed package |

## Difference between `vix remove` and `vix uninstall`

| Command | Purpose |
|---------|---------|
| `vix remove <pkg>` | Remove a dependency from the current project |
| `vix uninstall -g <pkg>` | Remove a globally installed package |
| `vix uninstall` | Remove the Vix CLI |

## Options

| Option | Description |
|--------|-------------|
| `-g, --global` | Remove a globally installed package. |
| `--purge` | Remove local store/cache as well. |
| `--all` | Try to remove every detected Vix binary in common locations. |
| `--system` | Include system locations such as `/usr/local/bin` and `/usr/bin`. |
| `--prefix <dir>` | Remove `<dir>/bin/vix`. |
| `--path <file>` | Remove the binary at an explicit path. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
vix uninstall
vix uninstall --purge
vix uninstall --all
vix uninstall --all --system
vix uninstall --prefix /usr/local
vix uninstall --path /usr/local/bin/vix
vix uninstall -g gk/jwt
vix uninstall -g @gk/jwt
```

## Before uninstalling

```bash
vix info       # inspect current setup
vix doctor     # check environment health
vix clean      # remove project-local files only
vix remove gk/jwt  # remove a project dependency only
```

## Common mistakes

### Using uninstall for project dependencies

```bash
vix remove gk/jwt          # project dependency
vix uninstall -g gk/jwt    # global package
```

### Using `--purge` when you only want to remove the binary

```bash
vix uninstall          # only removes the CLI binary
vix uninstall --purge  # also removes local store/cache data
```

### Removing the wrong binary

```bash
which vix
vix --version
vix uninstall --path /usr/local/bin/vix
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix upgrade` | Upgrade the Vix CLI or a global package |
| `vix install -g` | Install a global package |
| `vix list -g` | List global packages |
| `vix remove` | Remove a project dependency |
| `vix clean` | Clean project-local generated state |
| `vix reset` | Clean and reinstall project dependencies |
| `vix info` | Inspect Vix paths and caches |
| `vix doctor` | Diagnose environment health |

## Next step

Continue with shell completion.

[Open the vix completion guide](/cli/completion)
