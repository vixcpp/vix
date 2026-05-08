# vix install

`vix install` installs project dependencies from `vix.lock`.

It can also install one package globally with `-g`.

## Usage

```bash
vix install
vix install -g [@]namespace/name[@version]
```

## What it does

| Mode | Command | Purpose |
|------|---------|---------|
| Project mode | `vix install` | Install exact project dependencies from `vix.lock` |
| Global mode | `vix install -g <pkg>` | Install one package globally |

## Project install

```bash
vix install
```

In project mode, Vix reads exact resolved dependencies from `vix.lock`, reuses cached packages when available, and generates CMake integration files.

Typical project outputs: `.vix/deps/`, `.vix/vix_deps.cmake`

## Global install

```bash
vix install -g gk/jwt
vix install -g gk/jwt@^1.0.0
vix install -g @gk/jwt
vix install -g @gk/jwt@~1.2.0
```

Typical global outputs: `~/.vix/global/packages/`, `~/.vix/global/installed.json`

## Project workflow

```bash
# New project
vix new api
cd api
vix install
vix dev

# Existing project
git clone https://github.com/example/api.git
cd api
vix install
vix dev
```

## Difference between `vix add` and `vix install`

| Command | Purpose |
|---------|---------|
| `vix add <pkg>` | Add a new dependency and update `vix.json` + `vix.lock` |
| `vix install` | Install dependencies already pinned in `vix.lock` |

## Difference between `vix update` and `vix install`

| Command | Purpose |
|---------|---------|
| `vix update` | Resolve newer versions and rewrite `vix.lock` |
| `vix install` | Install the exact versions already in `vix.lock` |

## Registry sync

If install fails because a package is not found:

```bash
vix registry sync
vix install
```

## Options

| Option | Description |
|--------|-------------|
| `-g, --global` | Install one package globally. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# Install project dependencies
vix install

# Install after clone
git clone https://github.com/example/api.git
cd api
vix install

# Install then run dev mode
vix install
vix dev

# Install a global package
vix install -g gk/jwt
vix install -g gk/jwt@^1.0.0
```

## CI usage

```bash
vix install
vix check --tests

# Release builds
vix install
vix build --preset release
vix tests --preset release
```

## Common mistakes

### Expecting `vix install` to update dependencies

`vix install` installs locked versions. Use `vix outdated` to check and `vix update` to update.

### Editing `vix.json` and expecting install to resolve ranges

Project install is strict. If you manually edit `vix.json`, use `vix update` or `vix add` to update `vix.lock` first.

## Related commands

| Command | Purpose |
|---------|---------|
| `vix add` | Add a new dependency |
| `vix update` | Update dependency versions |
| `vix outdated` | Check outdated dependencies |
| `vix remove` | Remove a dependency |
| `vix list` | List dependencies |
| `vix reset` | Clean and reinstall project dependencies |
| `vix registry sync` | Refresh registry index |

## Next step

Continue with dependency updates.

[Open the vix update guide](/cli/update)
