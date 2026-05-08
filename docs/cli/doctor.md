# vix doctor

`vix doctor` checks the local environment for running and upgrading Vix.

Use it when you want to diagnose setup problems, inspect missing tools, or verify whether your Vix installation can work correctly.

## Usage

```bash
vix doctor [options]
```

## What it does

`vix doctor` inspects the local environment. It can check whether Vix is installed correctly, inspect required development tools, check environment readiness, check local upgrade information, optionally check the latest release online, and print a JSON summary for scripts or CI.

## Basic usage

```bash
vix doctor
vix doctor --online
vix doctor --json
vix doctor --json --online
vix doctor --online --repo vixcpp/vix
```

## Options

| Option | Description |
|--------|-------------|
| `--json` | Print a JSON summary at the end. |
| `--online` | Also check the latest release on GitHub. |
| `--repo <owner/name>` | Repository to check when using `--online`. Default is `vixcpp/vix`. |
| `-h, --help` | Show command help. |

## When to run it

Run `vix doctor` when: `vix build` fails unexpectedly, `vix run` cannot find tools, `vix install` behaves strangely, your environment changed, you installed Vix on a new machine, or you want to verify setup before using Vix seriously.

## Suggested debugging flow

```bash
vix doctor
vix info
vix registry sync     # if package resolution fails
vix reset             # if dependency state is broken
vix doctor --online   # if Vix may be outdated
```

## Difference between `vix doctor` and `vix info`

| Command | Purpose |
|---------|---------|
| `vix doctor` | Check environment health |
| `vix info` | Show paths, caches, and local state |

## Common mistakes

### Expecting doctor to fix problems automatically

`vix doctor` diagnoses problems — it does not repair them. After diagnosis, use the appropriate command (`vix registry sync`, `vix reset`, `vix upgrade`).

### Using `--repo` without `--online`

```bash
vix doctor --online --repo vixcpp/vix
```

## Recommended troubleshooting order

```bash
vix doctor
vix info
vix registry sync
vix reset
vix check --tests

# If issue may be caused by an old version
vix doctor --online
vix upgrade --check
```

## Related commands

| Command | Purpose |
|---------|---------|
| `vix info` | Show paths, caches, and local state |
| `vix upgrade --check` | Check upgrade target and download info |
| `vix upgrade` | Upgrade Vix |
| `vix registry sync` | Refresh registry metadata |
| `vix reset` | Clean and reinstall project dependencies |
| `vix check` | Validate project health |

## Next step

Continue with upgrades.

[Open the vix upgrade guide](/cli/upgrade)
