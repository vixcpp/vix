# vix replay

`vix replay` replays a previously recorded Vix execution.

It helps you reproduce a run that failed, crashed, or behaved unexpectedly. Use it when you want to rerun the same command without guessing what happened.

## Usage

```bash
vix replay [id|last|failed] [options] [-- app-args...]
```

## What it does

`vix replay` loads a recorded execution from `.vix/runs/` and runs it again with the stored context.

It can:

- replay the latest recorded run
- replay the latest failed run
- replay a specific run by id
- show recorded run details
- list previous runs
- clean recorded replay data
- append extra runtime arguments
- add temporary environment variables during replay

## Basic usage

```bash
# Replay the latest recorded run
vix replay

# Replay the latest run explicitly
vix replay last

# Replay the latest failed run
vix replay failed

# Replay a specific run
vix replay 2026-05-05-18-42-11-a91f
```

## When to use it

Use `vix replay` when an execution fails and you want to reproduce it quickly.

**Typical workflow:**

```bash
vix dev
# something fails

vix replay failed
```

Or:

```bash
vix run server.cpp
# unexpected result

vix replay last
```

## Why it matters

Without replay, you often need to remember: which command was used, which directory it ran from, which arguments were passed, which environment variables were active, which output was produced, and whether the run failed, crashed, or was interrupted.

`vix replay` keeps that context in a recorded run.

## Difference between `vix run`, `vix dev`, and `vix replay`

| Command      | Best for                        | Records/replays context              |
|--------------|---------------------------------|--------------------------------------|
| `vix run`    | manual run                      | records execution                    |
| `vix dev`    | active development              | records execution with watch mode    |
| `vix replay` | reproducing a previous run      | replays recorded execution           |

> `vix replay` does not replace `vix run` or `vix dev`. It uses their recorded executions.

## Replay the latest run

```bash
vix replay
```

Equivalent to:

```bash
vix replay last
```

You can also use:

```bash
vix replay latest
```

## Replay the latest failed run

```bash
vix replay failed
```

You can also use:

```bash
vix replay fail
```

This is useful when several successful runs happened before or after a failure.

## Replay a specific run

Each recorded run has an id:

```bash
vix replay 2026-05-05-18-42-11-a91f
```

Run ids are stored under `.vix/runs/`.

## List recorded runs

```bash
vix replay list
```

```bash
# Show only failed runs
vix replay list --failed

# Show only successful runs
vix replay list --success

# Show only interrupted runs
vix replay list --interrupted

# Limit the number of results
vix replay list --limit 10
```

## Show a recorded run

```bash
# Show the latest run
vix replay show last

# Show the latest failed run
vix replay show failed

# Show a specific run
vix replay show 2026-05-05-18-42-11-a91f
```

This prints the recorded metadata, command, status, duration, logs, and replay hint.

## Dry run

Use `--dry-run` to print the replay command without executing it:

```bash
vix replay last --dry-run
```

Useful when you want to inspect what will be replayed before running it.

## Pass extra app arguments

Arguments after `--` are appended to the replay command:

```bash
vix replay last -- --port 8080
```

## Add environment variables

```bash
vix replay last --env VIX_LOG_LEVEL=debug
```

Multiple variables:

```bash
vix replay last --env VIX_LOG_LEVEL=debug --env APP_ENV=local
```

## Use another replay directory

By default, `vix replay` reads from `.vix/runs/` in the current directory.

Use `--dir` or `--cwd` to read from another location:

```bash
vix replay last --dir ./api
vix replay list --cwd ./api
```

## Clean replay data

```bash
vix replay clean
```

Removes recorded replay runs from `.vix/runs/`.

## Recorded files

A recorded run is stored as:

```text
.vix/runs/<id>/
  run.json
  stdout.log
  stderr.log
  combined.log
```

The latest marker stores the most recent run id:

```text
.vix/runs/latest
```

## Options

| Option               | Description                                            |
|----------------------|--------------------------------------------------------|
| `--dry-run`          | Print the replay command without executing it          |
| `--summary`          | Print the record summary before replaying              |
| `--no-summary`       | Do not print the record summary                        |
| `--dir`, `--cwd`     | Use another directory containing `.vix/runs`           |
| `--env KEY=VALUE`    | Add an environment variable during replay              |
| `--`                 | Append remaining arguments to the replay command       |
| `-h`, `--help`       | Show command help                                      |

### List options

| Option                        | Description                                     |
|-------------------------------|-------------------------------------------------|
| `--all`                       | Show all replay runs                            |
| `--failed`, `--fail`          | Show failed replay runs                         |
| `--success`, `--ok`           | Show successful replay runs                     |
| `--interrupted`, `--interrupt`| Show interrupted replay runs                    |
| `--limit <n>`                 | Limit number of rows                            |
| `--dir`, `--cwd <path>`       | Use another directory containing `.vix/runs`    |

---

## Common workflows

```bash
# Replay the last run
vix replay

# Replay the latest failed run
vix replay failed

# Inspect the latest run before replaying
vix replay show last
vix replay last

# List failures
vix replay list --failed

# Replay with debug logs
vix replay failed --env VIX_LOG_LEVEL=debug

# Print the replay command only
vix replay last --dry-run

# Clean old replay data
vix replay clean
```

## Common mistakes

### Running replay from the wrong directory

**Wrong:**

```bash
cd ..
vix replay last
```

**Correct:**

```bash
cd api
vix replay last
```

Or:

```bash
vix replay last --dir ./api
```

> `vix replay` reads `.vix/runs/` from the current directory unless `--dir` or `--cwd` is provided.

### Forgetting that `failed` means the latest failed run

```bash
vix replay failed
```

This does not replay every failed run. It replays only the latest failed run.

To list all failures:

```bash
vix replay list --failed
```

### Passing extra arguments without `--`

**Wrong:**

```bash
vix replay last --port 8080
```

**Correct:**

```bash
vix replay last -- --port 8080
```

> The `--` separates replay options from arguments appended to the replayed command.

## Related commands

| Command      | Purpose                                          |
|--------------|--------------------------------------------------|
| `vix run`    | Build and run manually                           |
| `vix dev`    | Start dev server with auto-reload                |
| `vix build`  | Configure and compile                            |
| `vix check`  | Validate build, tests, runtime, and sanitizers   |
| `vix tests`  | Run tests                                        |

## Next step

Continue with development mode.

[Open the vix dev guide](/cli/dev)
