# vix update

`vix update` updates project or global packages to newer versions.

Use it when you want to refresh dependency versions and rewrite the lockfile.

## Usage

```bash
vix update
vix up
vix update [@]namespace/name[@version]
vix update -g [@]namespace/name[@version]
```

`vix up` is an alias for `vix update`.

## What it does

`vix update` resolves newer package versions from the registry. It can update all project dependencies, one project dependency, several project dependencies, or global packages with `-g`.

## Project update

```bash
vix update                      # all dependencies
vix update gk/jwt               # one dependency
vix update @gk/jwt              # scoped dependency
vix update gk/jwt@^1.2.0        # to a specific range
vix update gk/jwt gk/pdf        # several packages
```

## Global update

```bash
vix update -g gk/jwt
vix update -g gk/jwt@1.0.0
vix update -g @gk/jwt
```

Global update does not use `vix.lock`.

## Dry run

```bash
vix update --dry-run
vix update gk/jwt --dry-run
```

## Install after update

```bash
vix update --install
vix update gk/jwt --install
vix update gk/jwt gk/pdf --install
```

## Difference between `vix install` and `vix update`

| Command | Purpose |
|---------|---------|
| `vix install` | Install exact versions already pinned in `vix.lock` |
| `vix update` | Resolve newer versions and rewrite `vix.lock` |

## Options

| Option | Description |
|--------|-------------|
| `-g, --global` | Update a global package. |
| `--dry-run` | Show what would be updated without changing `vix.lock`. |
| `--json` | Print machine-readable JSON output. |
| `--install` | Run `vix install` after update. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
vix update                          # update project dependencies
vix update --install                # update and install immediately
vix update gk/jwt                   # update one dependency
vix update gk/jwt gk/pdf --install  # update several
vix update @gk/jwt                  # update a scoped dependency
vix update -g gk/jwt                # update a global package
vix up --install                    # use the alias
vix update --dry-run                # preview updates
vix update --json                   # machine-readable output
```

## Common mistakes

### Expecting update to add new dependencies

If `gk/jwt` is not already in `vix.json`, use `vix add gk/jwt` first.

### Running update when you only need install

After cloning a project, use `vix install` — it preserves exact locked versions.

### Forgetting to install after update

```bash
vix update --install
```

### Updating blindly before release

```bash
vix update --dry-run
vix update --install
vix check --tests
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix outdated` | Check which dependencies are outdated |
| `vix install` | Install exact locked dependencies |
| `vix add` | Add a new dependency |
| `vix remove` | Remove a dependency |
| `vix list` | List dependencies |
| `vix registry sync` | Refresh registry index |
| `vix check` | Validate after updating |

## Next step

Continue with outdated dependency checks.

[Open the vix outdated guide](/cli/outdated)
