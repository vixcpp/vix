# vix upgrade

`vix upgrade` upgrades the Vix CLI or a globally installed package.

Use it when you want to update Vix itself, check for a newer CLI release, or upgrade one package installed globally.

## Usage

```bash
vix upgrade
vix upgrade vX.Y.Z
vix upgrade --check
vix upgrade --dry-run
vix upgrade --json
vix upgrade -g [@]namespace/name[@version]
```

## What it does

| Mode | Command | Purpose |
|------|---------|---------|
| CLI mode | `vix upgrade` | Upgrade the Vix CLI from GitHub releases |
| Global package mode | `vix upgrade -g <pkg>` | Upgrade a globally installed package |

## Basic usage

```bash
vix upgrade               # latest version
vix upgrade v2.0.1        # specific version
vix upgrade --check       # check without installing
vix upgrade --dry-run     # simulate
vix upgrade --json        # machine-readable output
```

## Upgrade a global package

```bash
vix upgrade -g gk/jwt
vix upgrade -g gk/jwt@1.0.0
vix upgrade -g @gk/jwt
```

## Override the CLI release repository

```bash
VIX_REPO=vixcpp/vix vix upgrade --check
```

## Difference between `vix upgrade` and `vix update`

| Command | Purpose |
|---------|---------|
| `vix upgrade` | Upgrade the Vix CLI or a global package |
| `vix update` | Update project dependencies and rewrite `vix.lock` |

## Options

| Option | Description |
|--------|-------------|
| `-g, --global` | Upgrade a globally installed package. |
| `--check` | Show target version and download info without installing. |
| `--dry-run` | Simulate the upgrade without installing. |
| `--json` | Print machine-readable JSON output. |
| `-h, --help` | Show command help. |

## Environment variables

| Variable | Description |
|----------|-------------|
| `VIX_REPO` | Override repository for CLI upgrades. Default is `vixcpp/vix`. |

## Recommended upgrade flow

```bash
vix upgrade --check
vix upgrade --dry-run
vix upgrade
vix --version
vix doctor
```

## Common mistakes

### Using `vix upgrade` for project dependencies

```bash
# Wrong for project packages
vix upgrade

# Correct
vix update --install
```

### Forgetting `-g` for global package upgrade

```bash
# Upgrades the CLI, not a package
vix upgrade gk/jwt

# Correct
vix upgrade -g gk/jwt
```

### Passing a version without the `v` prefix

```bash
vix upgrade v2.0.1   # correct
vix upgrade 2.0.1    # wrong
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix doctor --online` | Check environment and latest release information |
| `vix info` | Show Vix paths and local state |
| `vix update` | Update project dependencies |
| `vix install -g` | Install a global package |
| `vix list -g` | List global packages |
| `vix uninstall -g` | Remove a global package |
| `vix --version` | Show installed Vix version |

## Next step

Continue with uninstalling Vix or global packages.

[Open the vix uninstall guide](/cli/uninstall)
