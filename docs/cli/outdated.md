# vix outdated

`vix outdated` checks whether project dependencies are behind the latest versions available in the local Vix Registry index.

Use it when you want to inspect dependency updates without changing your project files.

```bash
vix outdated
```

## Overview

`vix outdated` compares the versions pinned in:

```txt
vix.lock
```

with the latest versions known by the local registry index.

It does not update anything.
It does not install anything.
It does not rewrite `vix.json`.
It does not rewrite `vix.lock`.
It only reports dependency status.

Use it before deciding whether to run:

```bash
vix update
```

## Usage

```bash
vix outdated
vix outdated [@]namespace/name
vix outdated [@]namespace/name [@]namespace/name
vix outdated [options]
```

## Basic examples

```bash
# Check all locked dependencies
vix outdated

# Check one dependency
vix outdated gk/jwt

# Scoped-style syntax is accepted
vix outdated @gk/jwt

# Check several dependencies
vix outdated gk/jwt gk/pdf

# Machine-readable output
vix outdated --json

# Strict mode for CI
vix outdated --strict
```

## What it does

`vix outdated` performs this flow:

```txt
read vix.lock
read local registry index
collect locked dependencies
read latest registry version for each dependency
compare locked version with latest version
print current, latest, and status
```

The comparison is based on locked dependencies, not dependency ranges.

That means the source of truth is:

```txt
vix.lock
```

not:

```txt
vix.json
```

## Registry requirement

`vix outdated` needs the local registry index.

If the registry is not synced, Vix reports:

```txt
registry not synced
Run: vix registry sync
```

Fix it with:

```bash
vix registry sync
vix outdated
```

## Lockfile requirement

`vix outdated` reads:

```txt
vix.lock
```

If the file is missing, the command fails.

Typical fix:

```bash
vix registry sync
vix add gk/json@^1.0.0
vix install
vix outdated
```

or:

```bash
vix update
vix outdated
```

## Check all dependencies

Run:

```bash
vix outdated
```

Vix checks every dependency listed in:

```txt
vix.lock
```

Example output shape:

```txt
Outdated
Package   Current  Latest  Status
gk/json   1.0.0    1.1.0   outdated
gk/jwt    1.2.0    1.2.0   current

✔ checked 2 package(s), outdated 1
```

## Check one dependency

Run:

```bash
vix outdated gk/jwt
```

The package must exist in `vix.lock`.

If it does not, Vix reports:

```txt
dependency not found in vix.lock: gk/jwt
```

This is intentional.

`vix outdated` checks project dependencies.

It does not check arbitrary registry packages unless they are locked by the current project.

## Check several dependencies

Run:

```bash
vix outdated gk/jwt gk/pdf
```

Vix checks only those dependencies.

Each dependency must exist in `vix.lock`.

## Scoped-style package syntax

Both forms are accepted:

```bash
vix outdated gk/jwt
vix outdated @gk/jwt
```

Both refer to:

```txt
gk/jwt
```

Version suffixes are parsed, but outdated status is still based on the version pinned in `vix.lock`.

Example:

```bash
vix outdated @gk/jwt@1.x.x
```

This still checks the locked dependency:

```txt
gk/jwt
```

## Output table

The default output is a table.

Columns:

| Column    | Meaning                                           |
| --------- | ------------------------------------------------- |
| `Package` | Dependency id from `vix.lock`.                    |
| `Current` | Locked version from `vix.lock`.                   |
| `Latest`  | Latest version found in the local registry index. |
| `Status`  | `current`, `outdated`, or `missing`.              |

## Status values

| Status     | Meaning                                                               |
| ---------- | --------------------------------------------------------------------- |
| `current`  | Locked version equals the latest registry version.                    |
| `outdated` | Locked version differs from the latest registry version.              |
| `missing`  | Package was found in `vix.lock`, but not in the local registry index. |

## Missing packages

If a dependency exists in `vix.lock` but cannot be found in the local registry index, Vix marks it as:

```txt
missing
```

Example:

```txt
Package   Current  Latest  Status
gk/json   1.0.0    -       missing
```

Then it prints a warning:

```txt
1 package(s) missing from local registry index
```

Common fix:

```bash
vix registry sync
vix outdated
```

If it is still missing after sync, the package may not exist in the registry index anymore or the lockfile may point to an old package id.

## JSON output

Use:

```bash
vix outdated --json
```

Example output shape:

```json
{
  "command": "outdated",
  "packages": [
    {
      "spec": "gk/json",
      "id": "gk/json",
      "current": "1.0.0",
      "latest": "1.1.0",
      "outdated": true,
      "found_in_registry": true
    },
    {
      "spec": "gk/jwt",
      "id": "gk/jwt",
      "current": "1.2.0",
      "latest": "1.2.0",
      "outdated": false,
      "found_in_registry": true
    }
  ]
}
```

Use JSON output for:

- CI
- scripts
- dashboards
- maintenance reports
- dependency monitoring

## JSON with selected packages

```bash
vix outdated gk/json gk/jwt --json
```

The `spec` field keeps the user-provided package spec.

The `id` field is the normalized dependency id.

## Strict mode

Use:

```bash
vix outdated --strict
```

Strict mode changes the exit code.

| Result                          | Exit code |
| ------------------------------- | --------- |
| no outdated or missing packages | `0`       |
| at least one outdated package   | `1`       |
| at least one missing package    | `1`       |

This is useful in CI when you want stale dependencies to fail a maintenance job.

## Strict JSON mode

You can combine strict mode with JSON:

```bash
vix outdated --json --strict
```

This prints machine-readable output and still returns `1` if any dependency is outdated or missing.

## Exit codes

| Exit code | Meaning                                                                                                                                                                        |
| --------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `0`       | Command succeeded and strict mode did not find outdated or missing packages.                                                                                                   |
| `1`       | Registry missing, lockfile missing, invalid package spec, dependency missing from lockfile, outdated package in strict mode, missing package in strict mode, or another error. |

Without `--strict`, outdated dependencies do not make the command fail.

## Latest version source

Vix reads the latest version from the local registry entry.

It first checks:

```txt
latest
```

If that field is not available, it reads all available versions and selects the latest semver version.

That means the result depends on the local registry index.

Refresh it with:

```bash
vix registry sync
```

## Important limitation

`vix outdated` does not resolve version ranges.

It compares:

```txt
current locked version
```

against:

```txt
latest registry version
```

So if your `vix.json` says:

```json
{
  "deps": ["gk/json@^1.0.0"]
}
```

and your `vix.lock` pins:

```txt
gk/json 1.0.0
```

while the registry latest is:

```txt
1.1.0
```

`vix outdated` reports:

```txt
outdated
```

It does not decide whether the range allows the update.

Use:

```bash
vix update
```

to resolve and rewrite the lockfile.

## Difference between `vix outdated` and `vix update`

| Command        | Purpose                                                          |
| -------------- | ---------------------------------------------------------------- |
| `vix outdated` | Show whether locked dependencies are behind the registry latest. |
| `vix update`   | Resolve newer versions and rewrite `vix.lock`.                   |

Use `vix outdated` to inspect.

Use `vix update` to change the project.

## Difference between `vix outdated` and `vix install`

| Command        | Purpose                                                |
| -------------- | ------------------------------------------------------ |
| `vix outdated` | Compare locked versions with registry latest versions. |
| `vix install`  | Install exact versions already pinned in `vix.lock`.   |

`vix install` is for reproducible installation.

`vix outdated` is for dependency maintenance visibility.

## Difference between `vix outdated` and `vix registry sync`

| Command             | Purpose                                           |
| ------------------- | ------------------------------------------------- |
| `vix registry sync` | Refresh the local registry index.                 |
| `vix outdated`      | Compare `vix.lock` with the local registry index. |

A good maintenance workflow is:

```bash
vix registry sync
vix outdated
```

## Full maintenance workflow

Check current state:

```bash
vix registry sync
vix outdated
```

If something is outdated:

```bash
vix update
vix install
vix build --build-target all
vix tests
```

Or, when supported by your update workflow:

```bash
vix update --install
vix build --build-target all
vix tests
```

## CI usage

A CI job that only reports dependency status:

```bash
vix registry sync
vix outdated --json
```

A CI job that fails when dependencies are stale:

```bash
vix registry sync
vix outdated --strict
```

A CI job that produces JSON and fails when stale:

```bash
vix registry sync
vix outdated --json --strict
```

## Options

| Option       | Description                                                 |
| ------------ | ----------------------------------------------------------- |
| `--json`     | Print machine-readable JSON output.                         |
| `--strict`   | Return exit code `1` if any package is outdated or missing. |
| `-h, --help` | Show command help.                                          |

## Commands reference

| Command                    | Description                                    |
| -------------------------- | ---------------------------------------------- |
| `vix outdated`             | Check all locked dependencies.                 |
| `vix outdated <pkg>`       | Check one locked dependency.                   |
| `vix outdated <pkg> <pkg>` | Check several locked dependencies.             |
| `vix outdated --json`      | Print JSON output.                             |
| `vix outdated --strict`    | Fail if any dependency is outdated or missing. |

## Common workflows

### Check all dependencies

```bash
vix outdated
```

### Refresh registry, then check

```bash
vix registry sync
vix outdated
```

### Check one dependency

```bash
vix outdated gk/jwt
```

### Check several dependencies

```bash
vix outdated gk/jwt gk/pdf
```

### Use scoped syntax

```bash
vix outdated @gk/jwt
```

### Generate JSON report

```bash
vix outdated --json
```

### Strict CI check

```bash
vix outdated --strict
```

### Strict JSON CI check

```bash
vix outdated --json --strict
```

### Check before updating

```bash
vix registry sync
vix outdated
vix update
vix install
```

## Common mistakes

### Expecting `vix outdated` to update packages

Wrong expectation:

```txt
vix outdated should update vix.lock
```

Correct model:

```txt
vix outdated only reports status
```

To update:

```bash
vix update
vix install
```

### Forgetting to sync the registry

If results look old, refresh the registry:

```bash
vix registry sync
vix outdated
```

### Running without `vix.lock`

`vix outdated` needs locked project dependencies.

If `vix.lock` is missing, first add or update dependencies:

```bash
vix add gk/json@^1.0.0
vix install
```

### Checking a dependency not in the project

Wrong:

```bash
vix outdated gk/unknown
```

if `gk/unknown` is not in `vix.lock`.

Correct:

```bash
vix add gk/unknown
vix install
vix outdated gk/unknown
```

### Expecting range-aware update decisions

`vix outdated` compares locked version to registry latest.

It does not resolve ranges.

Use:

```bash
vix update
```

when you want version resolution.

### Confusing missing registry package with missing install

`missing` means the package was not found in the local registry index.

It does not necessarily mean the package is missing from `.vix/deps`.

Run:

```bash
vix registry sync
```

then check again.

## Troubleshooting

### Registry not synced

Run:

```bash
vix registry sync
```

Then:

```bash
vix outdated
```

### `vix.lock not found`

Create or update the lockfile:

```bash
vix add gk/json@^1.0.0
vix install
```

or:

```bash
vix update
```

### Invalid package spec

Valid package specs look like:

```txt
namespace/name
@namespace/name
namespace/name@version
@namespace/name@version
```

Example:

```bash
vix outdated gk/jwt
vix outdated @gk/jwt@1.x.x
```

Invalid:

```bash
vix outdated jwt
vix outdated gk
vix outdated @/jwt
```

### Dependency not found in lockfile

If Vix reports:

```txt
dependency not found in vix.lock: gk/jwt
```

the current project does not have that dependency pinned.

Check all dependencies:

```bash
vix list
```

or inspect:

```txt
vix.lock
```

Then add the dependency if needed:

```bash
vix add gk/jwt
vix install
```

### Package missing from local registry index

Run:

```bash
vix registry sync
vix outdated
```

If it is still missing, the package may not exist in the current registry index.

### JSON output is empty

If the project has no dependencies, JSON output is:

```json
{
  "command": "outdated",
  "packages": []
}
```

This is valid.

## Best practices

Run `vix registry sync` before dependency maintenance checks.

Use `vix outdated` before updating dependencies.

Use `vix outdated --json` for reports.

Use `vix outdated --strict` in CI only when you intentionally want stale dependencies to fail the job.

Commit `vix.lock`.

Do not manually edit `vix.lock`.

Use `vix update` to refresh locked versions.

Use `vix install` after updating the lockfile.

Run tests after updating dependencies.

## Related commands

| Command             | Purpose                                        |
| ------------------- | ---------------------------------------------- |
| `vix update`        | Resolve newer versions and rewrite `vix.lock`. |
| `vix install`       | Install exact locked dependencies.             |
| `vix add`           | Add a new dependency.                          |
| `vix remove`        | Remove a dependency.                           |
| `vix list`          | List locked or installed dependencies.         |
| `vix registry sync` | Refresh the local registry index.              |
| `vix reset`         | Clean and reinstall project dependency state.  |
| `vix build`         | Build after dependency changes.                |
| `vix tests`         | Run tests after dependency changes.            |

## Next step

Update dependency versions intentionally.

[Open the vix update guide](/cli/update)
