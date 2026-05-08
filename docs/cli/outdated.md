# vix outdated

`vix outdated` checks whether project dependencies are behind the latest versions available in the local registry index.

Use it when you want to inspect dependency updates without changing your project files.

## Usage

```bash
vix outdated
vix outdated [@]namespace/name
vix outdated [@]namespace/name [@]namespace/name
vix outdated [options]
```

## What it does

`vix outdated` compares locked project dependencies (`vix.lock`) with the latest versions known by the local registry index. It does not update anything — it only reports what is outdated or missing.

## Basic usage

```bash
vix outdated              # all dependencies
vix outdated gk/jwt       # one dependency
vix outdated @gk/jwt      # scoped dependency
vix outdated gk/jwt gk/pdf  # several dependencies
```

## JSON output

```bash
vix outdated --json
```

## Strict mode

```bash
vix outdated --strict
```

Exit code `0` = no outdated dependencies. Exit code `1` = at least one is outdated or missing.

## Registry sync

```bash
vix registry sync
vix outdated
```

## Difference between `vix outdated` and `vix update`

| Command | Purpose |
|---------|---------|
| `vix outdated` | Show what is behind |
| `vix update` | Resolve newer versions and rewrite `vix.lock` |

## Options

| Option | Description |
|--------|-------------|
| `--json` | Print machine-readable JSON output. |
| `--strict` | Return exit code 1 if any package is outdated or missing. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
vix outdated                  # check all dependencies
vix registry sync             # refresh registry
vix outdated                  # check after sync
vix outdated gk/jwt           # check one dependency
vix outdated --json           # machine-readable output
vix outdated --strict         # CI strict check
vix outdated                  # check then update
vix update --install
```

## CI usage

```bash
# Maintenance workflow
vix registry sync
vix outdated --json

# Fail on stale dependencies
vix outdated --strict
```

## Common mistakes

### Expecting `vix outdated` to update packages

```bash
# Only checks
vix outdated

# To update
vix update --install
```

### Forgetting to sync the registry

```bash
vix registry sync
vix outdated
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix update` | Update dependency versions |
| `vix install` | Install exact locked dependencies |
| `vix add` | Add a new dependency |
| `vix remove` | Remove a dependency |
| `vix list` | List dependencies |
| `vix registry sync` | Refresh registry index |

## Next step

Continue with dependency removal.

[Open the vix remove guide](/cli/remove)
