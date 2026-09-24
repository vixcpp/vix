# vix update

`vix update` updates project or global packages to newer versions.

Use it when you want to re-resolve dependency versions and rewrite `vix.lock`.

```bash
vix update
```

`vix up` is an alias for `vix update`.

```bash
vix up
```

## Overview

`vix update` is the command for refreshing dependency versions.

It can update:

- all project dependencies
- one project dependency
- several project dependencies
- one global package with `-g` or `--global`

In project mode, it reads dependencies from:

```txt
vix.json
```

Then it rewrites:

```txt
vix.lock
```

In global mode, it reuses the global install path:

```txt
vix install -g <package>
```

## Usage

```bash
vix update
vix up
vix update [@]namespace/name[@version]
vix up [@]namespace/name[@version]
vix update [options]
vix up [options]
vix update -g [@]namespace/name[@version]
```

## Basic examples

```bash
# Update all project dependencies
vix update

# Alias
vix up

# Update one dependency to latest
vix update gk/jwt

# Scoped-style syntax
vix update @gk/jwt

# Update one dependency to a new range
vix update gk/jwt@^1.2.0

# Update several dependencies
vix update gk/jwt gk/pdf

# Update and install immediately
vix update --install

# Preview without changing files
vix update --dry-run

# JSON output
vix update --json

# Update one global package
vix update -g gk/jwt
```

## What it does

In project mode, `vix update` performs this flow:

```txt
read vix.json
read vix.lock
select dependencies to update
resolve newer versions from the local registry
rewrite vix.lock
optionally run vix install
```

If you update a dependency with a new explicit range, Vix also updates that dependency requirement in `vix.json`.

Example:

```bash
vix update gk/jwt@^1.2.0
```

This can update:

```json
{
  "deps": [
    {
      "id": "gk/jwt",
      "version": "^1.2.0"
    }
  ]
}
```

Then Vix rewrites `vix.lock` with the exact resolved version.

## Project mode

Run:

```bash
vix update
```

This updates all dependencies already declared in `vix.json`.

Important rule:

```txt
vix update updates existing dependencies.
It does not add new dependencies.
```

If a dependency is not already in `vix.json`, use:

```bash
vix add <package>
```

first.

## Project dependency source

`vix update` reads declared dependencies from:

```txt
vix.json
```

Current format:

```json
{
  "deps": [
    {
      "id": "gk/jwt",
      "version": "^1.0.0"
    },
    {
      "id": "gk/pdf",
      "version": "1.0.0"
    }
  ]
}
```

Each dependency must have:

```txt
id
version
```

## Lockfile source

`vix update` also reads:

```txt
vix.lock
```

It uses the lockfile to know the previous resolved version.

If `vix.lock` is missing, Vix reports an error:

```txt
update failed: vix.lock not found
```

Fix:

```bash
vix install
```

or, if dependencies were never resolved:

```bash
vix add <package>
```

## Update all dependencies

Run:

```bash
vix update
```

When no package is specified, Vix updates every dependency from `vix.json`.

Important behavior:

```txt
vix update without explicit versions resolves latest available versions.
```

It does not simply reuse the old requested range when deciding the update target.

Example output shape:

```txt
Update
updating gk/jwt...
Add
id: gk/jwt
version: 1.3.0
tag: v1.3.0
commit: ...

resolving project dependencies...
✔ added: gk/jwt@1.3.0
✔ lock:  /home/user/api/vix.lock
✔ deps:  2

✔ gk/jwt: 1.2.0 -> 1.3.0
✔ processed 1 package(s), changed 1
⚠ Run: vix install to regenerate dependencies
```

## Update one dependency

Run:

```bash
vix update gk/jwt
```

This updates `gk/jwt` to the latest available version.

The dependency must already exist in `vix.json`.

If it does not, Vix reports:

```txt
dependency not found in vix.json: gk/jwt
```

Fix:

```bash
vix add gk/jwt
```

## Update one dependency to a new range

Run:

```bash
vix update gk/jwt@^1.2.0
```

This updates the requested version in `vix.json`, then rewrites `vix.lock`.

Use this when you intentionally want to change the version requirement.

## Update several dependencies

Run:

```bash
vix update gk/jwt gk/pdf
```

Vix updates only those dependencies.

Duplicate targets are de-duplicated internally.

## Scoped-style syntax

Both forms are accepted:

```bash
vix update gk/jwt
vix update @gk/jwt
```

Both refer to:

```txt
gk/jwt
```

You can also include a version range:

```bash
vix update @gk/jwt@^1.2.0
```

## Dry run

Use:

```bash
vix update --dry-run
```

Dry run shows what would be checked without changing `vix.json` or `vix.lock`.

Examples:

```bash
vix update --dry-run
vix update gk/jwt --dry-run
vix update gk/jwt gk/pdf --dry-run
```

Output shape:

```txt
Update
checking gk/jwt...
✔ gk/jwt: 1.2.0 -> latest
✔ processed 1 package(s), changed 0
```

Dry run does not resolve and write a new lockfile.

It is mainly a safe preview of the update targets.

## JSON output

Use:

```bash
vix update --json
```

Example output shape:

```json
{
  "command": "update",
  "dry_run": false,
  "install_after": false,
  "updated": [
    {
      "spec": "gk/jwt",
      "id": "gk/jwt",
      "before": "1.2.0",
      "after": "1.3.0",
      "changed": true
    }
  ]
}
```

Use JSON output for:

- scripts
- CI
- dashboards
- dependency reports
- automation

## JSON dry run

```bash
vix update --dry-run --json
```

Example shape:

```json
{
  "command": "update",
  "dry_run": true,
  "install_after": false,
  "updated": [
    {
      "spec": "gk/jwt",
      "id": "gk/jwt",
      "before": "1.2.0",
      "after": "1.2.0",
      "changed": false
    }
  ]
}
```

## Install after update

Use:

```bash
vix update --install
```

After rewriting the lockfile, Vix runs:

```bash
vix install
```

This regenerates installed dependency state and CMake integration.

Examples:

```bash
vix update --install
vix update gk/jwt --install
vix update gk/jwt gk/pdf --install
```

Without `--install`, Vix prints:

```txt
Run: vix install to regenerate dependencies
```

## Global update

Use `-g` or `--global`:

```bash
vix update -g gk/jwt
vix update --global gk/jwt
vix update -g @gk/jwt
vix update -g gk/jwt@1.0.0
```

Global update does not use `vix.lock`.

It reuses:

```bash
vix install -g <package>
```

So this:

```bash
vix update -g gk/jwt
```

behaves like a global reinstall/update from the registry.

## Global mode rules

Global mode requires one package spec.

Wrong:

```bash
vix update -g
```

Correct:

```bash
vix update -g gk/jwt
```

If the package is missing, Vix reports:

```txt
missing package spec
Example: vix update -g @gk/jwt
```

## Difference between `vix update` and `vix add`

| Command            | Purpose                                                      |
| ------------------ | ------------------------------------------------------------ |
| `vix add <pkg>`    | Add a new dependency to `vix.json` and rewrite `vix.lock`.   |
| `vix update <pkg>` | Update an existing dependency already present in `vix.json`. |

Use `vix add` when the dependency is new.

Use `vix update` when the dependency already exists.

## Difference between `vix update` and `vix install`

| Command       | Purpose                                              |
| ------------- | ---------------------------------------------------- |
| `vix update`  | Resolve newer versions and rewrite `vix.lock`.       |
| `vix install` | Install exact versions already pinned in `vix.lock`. |

After cloning a project, use:

```bash
vix install
```

not:

```bash
vix update
```

because install preserves the lockfile.

## Difference between `vix update` and `vix outdated`

| Command        | Purpose                                         |
| -------------- | ----------------------------------------------- |
| `vix outdated` | Report dependencies behind the registry latest. |
| `vix update`   | Re-resolve and rewrite dependency state.        |

Use `vix outdated` to inspect.

Use `vix update` to change files.

## Files changed

Project update can change:

```txt
vix.json
vix.lock
```

`vix.json` changes only when you pass an explicit version or range for a dependency.

Example:

```bash
vix update gk/jwt@^1.2.0
```

`vix.lock` is rewritten when update runs normally.

Dry run does not change files.

## Registry requirement

`vix update` resolves packages from the local registry index.

If the registry is not synced, update can fail during package resolution.

Run:

```bash
vix registry sync
vix update
```

## Full project update workflow

Check outdated packages:

```bash
vix registry sync
vix outdated
```

Update and install:

```bash
vix update --install
```

Validate:

```bash
vix build --build-target all
vix tests
```

or:

```bash
vix check --tests
```

## Safe update workflow

```bash
vix registry sync
vix outdated
vix update --dry-run
vix update --install
vix check --tests
```

For production apps:

```bash
vix registry sync
vix outdated
vix update --install
vix build --preset release
vix tests --preset release
```

## CI usage

A CI job should usually install locked dependencies:

```bash
vix install
vix check --tests
```

Use `vix update` in maintenance workflows, not normal build workflows.

Example dependency maintenance job:

```bash
vix registry sync
vix outdated --json
vix update --dry-run --json
```

If you intentionally want a bot or script to update:

```bash
vix registry sync
vix update --install
vix check --tests
```

## Options

| Option         | Description                                             |
| -------------- | ------------------------------------------------------- |
| `-g, --global` | Update one global package.                              |
| `--dry-run`    | Show what would be updated without changing `vix.lock`. |
| `--json`       | Print machine-readable JSON output.                     |
| `--install`    | Run `vix install` after update.                         |
| `-h, --help`   | Show command help.                                      |

## Commands reference

| Command                    | Description                                            |
| -------------------------- | ------------------------------------------------------ |
| `vix update`               | Update all project dependencies.                       |
| `vix up`                   | Alias for `vix update`.                                |
| `vix update <pkg>`         | Update one existing project dependency to latest.      |
| `vix update <pkg>@<range>` | Update one existing project dependency to a new range. |
| `vix update <pkg> <pkg>`   | Update several existing project dependencies.          |
| `vix update --dry-run`     | Preview update targets.                                |
| `vix update --json`        | Print JSON output.                                     |
| `vix update --install`     | Update, then install.                                  |
| `vix update -g <pkg>`      | Update one global package.                             |

## Common workflows

### Update all dependencies

```bash
vix registry sync
vix update
vix install
vix check --tests
```

### Update and install immediately

```bash
vix registry sync
vix update --install
vix check --tests
```

### Update one dependency

```bash
vix update gk/jwt
vix install
vix build
```

### Update one dependency to a new range

```bash
vix update gk/jwt@^1.2.0
vix install
vix check --tests
```

### Update several dependencies

```bash
vix update gk/jwt gk/pdf --install
vix check --tests
```

### Preview updates

```bash
vix update --dry-run
```

### Use JSON output

```bash
vix update --json
```

### Update a global package

```bash
vix registry sync
vix update -g gk/jwt
```

## Common mistakes

### Expecting update to add new dependencies

Wrong:

```bash
vix update gk/jwt
```

when `gk/jwt` is not in `vix.json`.

Correct:

```bash
vix add gk/jwt
```

### Running update after clone

Wrong:

```bash
git clone https://github.com/example/api.git
cd api
vix update
```

Correct:

```bash
git clone https://github.com/example/api.git
cd api
vix install
```

Use install after clone because it preserves exact locked versions.

### Forgetting to install after update

Wrong:

```bash
vix update
vix build
```

Better:

```bash
vix update --install
vix build
```

or:

```bash
vix update
vix install
vix build
```

### Updating blindly before release

Before a release, preview and validate:

```bash
vix outdated
vix update --dry-run
vix update --install
vix check --tests
```

### Expecting dry run to rewrite lockfile

`--dry-run` does not change files.

Use normal update when you want to rewrite `vix.lock`.

### Expecting global update to use project lockfile

Global update does not use:

```txt
vix.lock
```

It reuses global install logic:

```bash
vix install -g <package>
```

## Troubleshooting

### `vix.lock not found`

Create or restore the lockfile.

If the project has dependencies:

```bash
vix install
```

If dependencies were never added:

```bash
vix add <package>
```

### Dependency not found in `vix.json`

The package is not declared in the project manifest.

Add it first:

```bash
vix add gk/jwt
```

### Invalid package spec

Valid:

```bash
vix update gk/jwt
vix update @gk/jwt
vix update gk/jwt@^1.2.0
```

Invalid:

```bash
vix update jwt
vix update @/jwt
vix update gk/jwt@
```

### Registry not synced

Run:

```bash
vix registry sync
```

Then:

```bash
vix update
```

### No dependencies to update

If `vix.json` has no `deps`, Vix prints:

```txt
no dependencies to update
```

Add a dependency first:

```bash
vix add gk/jwt
```

### `vix update --json` shows empty list

This means no dependencies were selected or declared.

Check:

```bash
vix list
```

or inspect:

```txt
vix.json
```

### Install after update failed

Run install manually to see the failure:

```bash
vix install
```

Common causes:

- registry not synced
- package checkout failed
- integrity mismatch
- generated dependency integration failed

## Best practices

Use `vix outdated` before `vix update`.

Use `vix update --dry-run` before large dependency updates.

Use `vix update --install` when you want the project ready immediately after update.

Run tests after updating.

Commit both `vix.json` and `vix.lock`.

Do not manually edit `vix.lock`.

Use `vix add` for new dependencies.

Use `vix install` after cloning a project.

Use global update only for global packages.

## Related commands

| Command             | Purpose                                |
| ------------------- | -------------------------------------- |
| `vix outdated`      | Check which dependencies are outdated. |
| `vix install`       | Install exact locked dependencies.     |
| `vix add`           | Add a new dependency.                  |
| `vix remove`        | Remove a dependency.                   |
| `vix list`          | List dependencies.                     |
| `vix registry sync` | Refresh registry index.                |
| `vix check`         | Validate after updating.               |
| `vix build`         | Build after dependency changes.        |
| `vix tests`         | Run tests after dependency changes.    |

## Next step

Check outdated dependencies before updating.

[Open the vix outdated guide](/cli/outdated)
