# vix search

`vix search` searches packages in the local Vix Registry index.

Use it when you want to find packages by namespace, name, display name, description, or keywords before adding them to a project.

```bash
vix search json
```

## Overview

`vix search` is an offline registry search command.

It does not query GitHub directly.
It does not fetch package repositories.
It does not install anything.
It only reads the local registry index stored on your machine.
The local registry index is created and refreshed with:

```bash
vix registry sync
```

The search flow is:

```txt
local registry index
  -> scan package JSON entries
  -> score matching packages
  -> sort results
  -> print paginated output
```

## Usage

```bash
vix search <query> [--page N] [--limit N]
```

## Basic examples

```bash
# Search for JSON packages
vix search json

# Search by namespace
vix search softadastra

# Search by package name
vix search core

# Search with pagination
vix search softadastra --page 2

# Change result count per page
vix search softadastra --limit 10

# Page and limit together
vix search softadastra --page 2 --limit 5
```

## Registry requirement

`vix search` requires the local registry index.

If the registry has not been synced, Vix reports:

```txt
registry not synced
Run: vix registry sync
```

Fix:

```bash
vix registry sync
vix search json
```

## Registry location

`vix search` reads package entries from:

```txt
~/.vix/registry/index/index
```

Each package entry is a JSON file.

Example:

```txt
~/.vix/registry/index/index/softadastra.core.json
```

You can inspect the registry path with:

```bash
vix registry path
```

## What search matches

`vix search` can match a query against:

```txt
package id
package name
namespace
displayName
description
keywords
```

For a package entry like:

```json
{
  "namespace": "softadastra",
  "name": "core",
  "displayName": "Softadastra Core",
  "description": "Foundational primitives for Softadastra systems.",
  "keywords": ["core", "offline-first", "cpp"]
}
```

These can match:

```bash
vix search softadastra
vix search core
vix search offline-first
vix search primitives
```

## Search scoring

Results are ranked by a simple score.

The strongest matches come first.

Scoring uses this order:

| Match field  | Score |
| ------------ | ----: |
| package id   |   100 |
| name         |    60 |
| namespace    |    40 |
| display name |    25 |
| description  |    20 |
| keywords     |    15 |

This means an exact or partial match in the package id usually appears before a match only found in the description.

## Case-insensitive search

Search is case-insensitive.

These are equivalent:

```bash
vix search softadastra
vix search Softadastra
vix search SOFTADASTRA
```

## Multi-word queries

If you pass several words, Vix joins them into one query.

Example:

```bash
vix search offline sync
```

is treated as:

```txt
offline sync
```

Use this for description-style searches.

## Output

Example:

```bash
vix search softadastra
```

Output shape:

```txt
Search
  • query     : "softadastra"
  • page      : 1
  • limit     : 5

softadastra/softadastra  (latest: 0.1.7)
  Softadastra umbrella project for building core modules, optional apps, and top-level examples.
  repo: https://github.com/softadastra/softadastra

softadastra/cli  (latest: 0.4.2)
  Modular command-line interface engine for Softadastra systems: parsing, command registry, handlers, interactive REPL, and structured terminal I/O.
  repo: https://github.com/softadastra/cli

✔ Showing 1-5 of 17 result(s).
Page 1/4
Next: vix search "softadastra" --page 2 --limit 5
```

## Result fields

Each result can show:

| Field       | Meaning                                          |
| ----------- | ------------------------------------------------ |
| package id  | The package name used by `vix add`.              |
| latest      | Latest version detected from the registry entry. |
| description | Package description from registry metadata.      |
| repo        | Package source repository URL.                   |

Example package id:

```txt
softadastra/core
```

You can add it with:

```bash
vix add softadastra/core
```

## Latest version

`vix search` prints the latest version for each package when available.

It checks the registry entry in this order:

```txt
latest field
versions object
```

If `latest` exists, it uses that.

If `latest` is missing, it finds the latest SemVer version from the `versions` object.

## Pagination

By default, `vix search` shows:

```txt
page: 1
limit: 5
```

Use `--page` to move through results:

```bash
vix search softadastra --page 2
```

Use `--limit` to change how many results are shown per page:

```bash
vix search softadastra --limit 10
```

Use both together:

```bash
vix search softadastra --page 2 --limit 2
```

Example output shape:

```txt
Search
  • query     : "softadastra"
  • page      : 2
  • limit     : 2

softadastra/core  (latest: 1.7.0)
  Foundational primitives for Softadastra systems: types, errors, ids, time, hash, and config.
  repo: https://github.com/softadastra/core

softadastra/discovery  (latest: 0.5.0)
  Softadastra Discovery is a lightweight, offline-first discovery layer designed to find peers reliably across unstable networks.
  repo: https://github.com/softadastra/discovery

✔ Showing 3-4 of 17 result(s).
Page 2/9
Next: vix search "softadastra" --page 3 --limit 2
```

## Limit behavior

`--limit` must be a positive number.

Valid:

```bash
vix search json --limit 1
vix search json --limit 10
vix search json --limit 100
```

The maximum limit is clamped to:

```txt
100
```

So if you pass:

```bash
vix search json --limit 500
```

Vix uses:

```txt
100
```

## Page behavior

`--page` must be a positive number.

Valid:

```bash
vix search json --page 1
vix search json --page 2
```

Invalid:

```bash
vix search json --page 0
vix search json --page abc
```

If the page is too high, Vix reports:

```txt
page out of range
Total pages: N
```

## Supported option forms

Both forms are supported:

```bash
vix search json --page 2 --limit 5
```

and:

```bash
vix search json --page=2 --limit=5
```

## No results

If no package matches the query, Vix prints:

```txt
no results for "query"
Tip: search by namespace, name, description, or keywords
Example: vix search gaspardkirira
```

This is not a registry error.

It only means no local registry entry matched the query.

Try:

```bash
vix registry sync
vix search <query>
```

If it still returns no result, the package may not exist in the current registry index.

## Invalid arguments

If arguments are invalid, Vix prints:

```txt
invalid search arguments
Usage: vix search <query> [--page N] [--limit N]
Example: vix search json --page 2 --limit 5
```

Common invalid examples:

```bash
vix search
vix search json --page
vix search json --limit
vix search json --page 0
vix search json --limit 0
```

## Search before adding

A normal package discovery workflow is:

```bash
vix registry sync
vix search json
vix add gk/json
vix install
vix build
```

Search gives you the correct package id.

Then `vix add` adds it to the project.

Then `vix install` prepares dependencies for the project.

## Add a package from search results

If search returns:

```txt
softadastra/core  (latest: 1.7.0)
```

you can add it with:

```bash
vix add softadastra/core
```

or pin a version:

```bash
vix add softadastra/core@1.7.0
```

Then install:

```bash
vix install
```

## Search vs add

| Command              | Purpose                                          |
| -------------------- | ------------------------------------------------ |
| `vix search <query>` | Find packages in the local registry index.       |
| `vix add <pkg>`      | Add a package dependency to the current project. |

Use search when you do not know the exact package id.

Use add when you know the exact package id.

## Search vs registry sync

| Command              | Purpose                          |
| -------------------- | -------------------------------- |
| `vix registry sync`  | Refresh local registry metadata. |
| `vix search <query>` | Search that local metadata.      |

If search results are stale, run:

```bash
vix registry sync
```

then search again.

## Search vs install

| Command              | Purpose                                     |
| -------------------- | ------------------------------------------- |
| `vix search <query>` | Find package metadata.                      |
| `vix install`        | Install exact dependencies from `vix.lock`. |

Search does not install anything.

Install does not search the registry for new packages.

## Full workflow

```bash
# Refresh registry metadata
vix registry sync

# Search for a package
vix search json

# Add the exact package id
vix add gk/json@^1.0.0

# Install locked dependencies
vix install

# Build the project
vix build
```

## Options

| Option       | Description                                                    |
| ------------ | -------------------------------------------------------------- |
| `--page N`   | Show page `N`. Default: `1`.                                   |
| `--page=N`   | Same as `--page N`.                                            |
| `--limit N`  | Show up to `N` results per page. Default: `5`, maximum: `100`. |
| `--limit=N`  | Same as `--limit N`.                                           |
| `-h, --help` | Show command help.                                             |

## Commands reference

| Command                               | Description                           |
| ------------------------------------- | ------------------------------------- |
| `vix search json`                     | Search for packages matching `json`.  |
| `vix search softadastra`              | Search by namespace or metadata.      |
| `vix search json --page 2`            | Show page 2.                          |
| `vix search json --limit 20`          | Show 20 results per page.             |
| `vix search json --page 2 --limit 10` | Show page 2 with 10 results per page. |

## Common workflows

### Search after syncing registry

```bash
vix registry sync
vix search json
```

### Search by namespace

```bash
vix search softadastra
```

### Search with more results

```bash
vix search softadastra --limit 20
```

### Continue to next page

```bash
vix search softadastra --page 2 --limit 5
```

### Search and add

```bash
vix search core
vix add softadastra/core
vix install
```

### Search and pin latest shown version

```bash
vix search softadastra/core
vix add softadastra/core@1.7.0
vix install
```

## Common mistakes

### Forgetting registry sync

Wrong:

```bash
vix search json
```

when the registry has never been synced.

Correct:

```bash
vix registry sync
vix search json
```

### Expecting online search

`vix search` searches the local registry index.

It does not search GitHub live.

Refresh the local index with:

```bash
vix registry sync
```

### Expecting search to install packages

Wrong expectation:

```txt
vix search should install the package
```

Correct workflow:

```bash
vix search json
vix add gk/json
vix install
```

### Using a package description instead of package id with add

Search can match descriptions, but `vix add` needs the package id.

If search returns:

```txt
softadastra/core
```

use:

```bash
vix add softadastra/core
```

### Passing invalid pagination values

Wrong:

```bash
vix search json --page 0
vix search json --limit 0
vix search json --page abc
```

Correct:

```bash
vix search json --page 1 --limit 5
```

### Asking for a page that does not exist

If Vix says:

```txt
page out of range
Total pages: 4
```

use a page between `1` and `4`.

## Troubleshooting

### Registry not synced

Run:

```bash
vix registry sync
```

Then retry:

```bash
vix search json
```

### No results

Try a broader query:

```bash
vix search json
vix search core
vix search softadastra
```

Then sync and retry:

```bash
vix registry sync
vix search json
```

### Results look old

Refresh registry metadata:

```bash
vix registry sync
```

Then search again.

### Invalid search arguments

Check the expected format:

```bash
vix search <query> [--page N] [--limit N]
```

Example:

```bash
vix search softadastra --page 2 --limit 5
```

### Page out of range

Use the total page count printed by Vix.

Example:

```txt
Total pages: 4
```

Then run:

```bash
vix search softadastra --page 4
```

### Broken registry entry ignored

If a registry JSON file cannot be read, the current search implementation skips it.

Run:

```bash
vix registry sync
```

to refresh the local registry clone.

## Best practices

Run `vix registry sync` before searching if the registry may be stale.

Search by namespace when you know the author or organization.

Search by package name when you know the tool you need.

Search by keywords when you only know the category.

Use `--limit` when exploring many packages.

Copy the exact package id from the search result before running `vix add`.

Run `vix install` after adding dependencies.

Commit `vix.json` and `vix.lock` after dependency changes.

## Related commands

| Command             | Purpose                                                       |
| ------------------- | ------------------------------------------------------------- |
| `vix registry sync` | Refresh local registry metadata.                              |
| `vix registry path` | Show local registry path.                                     |
| `vix add`           | Add a registry package to a project.                          |
| `vix install`       | Install locked project dependencies.                          |
| `vix update`        | Re-resolve dependency versions.                               |
| `vix outdated`      | Check whether locked dependencies are behind registry latest. |
| `vix list`          | List project or global dependencies.                          |
| `vix publish`       | Publish a tagged package version to the registry.             |
| `vix unpublish`     | Remove a package entry from the registry.                     |
| `vix store`         | Manage local package source checkouts.                        |

## Next step

Add a package to your project.

[Open the vix add guide](/cli/add)
