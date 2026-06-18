# Replay a Run

`vix replay` replays a previously recorded Vix execution.

Use this guide when you want to reproduce a run that failed, crashed, was interrupted, or behaved differently than expected.

Replay is useful because it keeps the execution context. You do not need to guess the command, the working directory, the runtime arguments, or the environment that was used.

> `vix run` does not record replay data by default.
> To create a replay record, run with `--replay`.

## The problem

When an app fails, the failure is not always only in the source code.

It can depend on:

```txt
the exact command
the current directory
the runtime arguments
the environment variables
the selected target
the executable path
the exit code
stdout and stderr output
whether the process crashed or was interrupted
```

Without replay data, reproducing the same run can become manual and fragile.

`vix replay` exists to make that workflow repeatable.

## What `vix replay` does

`vix replay` loads a recorded execution from:

```txt
.vix/runs/
```

Then it replays the stored command with the recorded context.

It can:

```txt
replay the latest recorded run
replay the latest failed run
replay a specific run by id
list previous recorded runs
show details about a run
append extra runtime arguments
add temporary environment variables
clean replay records
```

## Recording a run

Replay records are created only when `vix run` is executed with `--replay`.

```bash
vix run api --replay
vix run ./build-ninja/api --replay
vix run app.vix --replay
vix run main.cpp --replay
```

Without `--replay`, `vix run` runs normally and does not create replay data.

```bash
vix run api
vix run main.cpp
```

These commands do not create records under `.vix/runs/`.

## Basic usage

Replay the latest recorded run:

```bash
vix replay
```

This is equivalent to:

```bash
vix replay last
```

You can also use:

```bash
vix replay latest
```

Replay the latest failed run:

```bash
vix replay failed
```

Replay a specific run:

```bash
vix replay 2026-05-05-18-42-11-a91f
```

## Typical workflow

Record a run:

```bash
vix run api --replay
```

If the run fails, replay the latest failed run:

```bash
vix replay failed
```

For single-file C++:

```bash
vix run main.cpp --replay
vix replay last
```

For a built executable:

```bash
vix run ./build-ninja/api --replay
vix replay last
```

## Usage

```bash
vix replay [id|last|latest|failed|fail] [options] [-- app-args...]
vix replay list [options]
vix replay show [id|last|latest|failed|fail] [options]
vix replay clean [options]
```

## Replay targets

| Target   | Meaning                           |
| -------- | --------------------------------- |
| `last`   | Replay the latest recorded run    |
| `latest` | Alias for `last`                  |
| `failed` | Replay the latest failed run      |
| `fail`   | Alias for `failed`                |
| `<id>`   | Replay a specific recorded run id |

When no target is provided, Vix uses `last`.

```bash
vix replay
```

## Replay the latest run

```bash
vix replay last
```

Or simply:

```bash
vix replay
```

Use this when you want to rerun the most recent recorded execution.

## Replay the latest failed run

```bash
vix replay failed
```

This replays only the latest failed run.
It does not replay every failed run.

To see all failed runs:

```bash
vix replay list --failed
```

## Replay a specific run

Each recorded run has an id.

Example:

```bash
vix replay 2026-05-05-18-42-11-a91f
```

Run ids are stored under:

```txt
.vix/runs/
```

## List recorded runs

List recent runs:

```bash
vix replay list
```

Show only failed runs:

```bash
vix replay list --failed
```

Show only successful runs:

```bash
vix replay list --success
```

Show interrupted runs:

```bash
vix replay list --interrupted
```

Limit the number of results:

```bash
vix replay list --limit 10
```

## Show a recorded run

Show the latest run:

```bash
vix replay show last
```

Show the latest failed run:

```bash
vix replay show failed
```

Show a specific run:

```bash
vix replay show 2026-05-05-18-42-11-a91f
```

This is useful before replaying because it lets you inspect the stored metadata.

A recorded run can include:

```txt
run id
original command
working directory
status
exit code
duration
stdout log
stderr log
combined log
replay hint
```

## Dry run

Use `--dry-run` to print what would be replayed without executing it.

```bash
vix replay last --dry-run
```

This is useful when you want to inspect the replay command first.

## Print summary before replay

Use `--summary` to print the record summary before running it:

```bash
vix replay failed --summary
```

Disable summary output:

```bash
vix replay failed --no-summary
```

## Pass extra app arguments

Arguments after `--` are appended to the replayed command.

```bash
vix replay last -- --port 8080
```

Another example:

```bash
vix replay failed -- --debug --seed 42
```

The separator matters.

Replay options go before `--`.

App arguments go after `--`.

## Add environment variables

You can add temporary environment variables during replay.

```bash
vix replay last --env VIX_LOG_LEVEL=debug
```

Multiple variables:

```bash
vix replay last \
  --env VIX_LOG_LEVEL=debug \
  --env APP_ENV=local
```

This does not permanently modify `.env`.

It only affects the replayed process.

## Use another replay directory

By default, `vix replay` reads from `.vix/runs/` in the current directory.

Use `--dir` or `--cwd` to read replay data from another project:

```bash
vix replay last --dir ./api
```

Or:

```bash
vix replay list --cwd ./api
```

This is useful when you are not currently inside the project folder.

## Clean replay data

Remove recorded replay runs:

```bash
vix replay clean
```

This removes replay data from:

```txt
.vix/runs/
```

Use this when old records are no longer needed.

## Recorded files

A recorded run is stored like this:

```txt
.vix/runs/<id>/
  run.json
  stdout.log
  stderr.log
  combined.log
```

The latest marker stores the most recent run id:

```txt
.vix/runs/latest
```

## Options

| Option            | Description                                          |
| ----------------- | ---------------------------------------------------- |
| `--dry-run`       | Print the replay command without executing it        |
| `--summary`       | Print the record summary before replaying            |
| `--no-summary`    | Do not print the record summary                      |
| `--dir <path>`    | Use another directory containing `.vix/runs`         |
| `--cwd <path>`    | Alias-style working directory option for replay data |
| `--env KEY=VALUE` | Add an environment variable during replay            |
| `--`              | Append remaining arguments to the replayed command   |
| `-h, --help`      | Show command help                                    |

## Recording option on `vix run`

| Option     | Description                                        |
| ---------- | -------------------------------------------------- |
| `--replay` | Record this `vix run` execution under `.vix/runs/` |

Examples:

```bash
vix run api --replay
vix run ./build-ninja/api --replay
vix run app.vix --replay
vix run main.cpp --replay
```

## List options

| Option          | Description                                  |
| --------------- | -------------------------------------------- |
| `--all`         | Show all replay runs                         |
| `--failed`      | Show failed replay runs                      |
| `--fail`        | Alias for `--failed`                         |
| `--success`     | Show successful replay runs                  |
| `--ok`          | Alias for `--success`                        |
| `--interrupted` | Show interrupted replay runs                 |
| `--interrupt`   | Alias for `--interrupted`                    |
| `--limit <n>`   | Limit number of rows                         |
| `--dir <path>`  | Use another directory containing `.vix/runs` |
| `--cwd <path>`  | Use another directory containing `.vix/runs` |

## Difference between `vix run`, `vix dev`, and `vix replay`

| Command      | Best for                         | Replay behavior                       |
| ------------ | -------------------------------- | ------------------------------------- |
| `vix run`    | Manual execution                 | Records only when `--replay` is used  |
| `vix dev`    | Active development               | Focused on rebuild and rerun workflow |
| `vix replay` | Reproducing a recorded execution | Replays stored execution context      |

`vix replay` does not replace `vix run`.

It uses replay records created by replay-enabled runs.

## Example: replay a failed backend run

Record the run:

```bash
vix run api --replay
```

If it fails:

```bash
vix replay failed
```

Inspect before replaying:

```bash
vix replay show failed
```

Replay with debug logs:

```bash
vix replay failed --env VIX_LOG_LEVEL=debug
```

## Example: replay a single C++ file

Record:

```bash
vix run main.cpp --replay
```

Replay:

```bash
vix replay last
```

Replay with extra runtime arguments:

```bash
vix replay last -- --name Gaspard
```

## Example: inspect previous runs

List recent runs:

```bash
vix replay list
```

List only failures:

```bash
vix replay list --failed
```

Show a specific run:

```bash
vix replay show 2026-05-05-18-42-11-a91f
```

Replay it:

```bash
vix replay 2026-05-05-18-42-11-a91f
```

## Common workflows

```bash
# Record and replay a project run
vix run api --replay
vix replay last

# Record and replay a single-file C++ run
vix run main.cpp --replay
vix replay last

# Replay the latest failed run
vix replay failed

# Inspect the latest run before replaying
vix replay show last
vix replay last

# List failed runs
vix replay list --failed

# Replay with debug logs
vix replay failed --env VIX_LOG_LEVEL=debug

# Print the replay command only
vix replay last --dry-run

# Clean old replay data
vix replay clean
```

## Common mistakes

### Expecting `vix run` to record automatically

Wrong:

```bash
vix run api
vix replay last
```

Correct:

```bash
vix run api --replay
vix replay last
```

`vix run` does not create replay records by default.

Use `--replay` when you want the execution to be recorded.

### Running replay from the wrong directory

Wrong:

```bash
cd ..
vix replay last
```

Correct:

```bash
cd api
vix replay last
```

Or:

```bash
vix replay last --dir ./api
```

`vix replay` reads `.vix/runs/` from the current directory unless `--dir` or `--cwd` is provided.

### Thinking `failed` replays every failure

```bash
vix replay failed
```

This replays the latest failed run only.

To list all failures:

```bash
vix replay list --failed
```

### Passing app arguments without `--`

Wrong:

```bash
vix replay last --port 8080
```

Correct:

```bash
vix replay last -- --port 8080
```

The `--` separates replay options from arguments appended to the replayed command.

### Forgetting to inspect the run first

When you are unsure what will be replayed, use:

```bash
vix replay show last
vix replay last --dry-run
```

Then replay normally.

## Recommended debugging flow

```bash
# 1. Record the execution
vix run api --replay

# 2. If it fails, inspect the record
vix replay show failed

# 3. Replay the failure
vix replay failed

# 4. Replay with more logs
vix replay failed --env VIX_LOG_LEVEL=debug

# 5. Clean old records when done
vix replay clean
```

## Related commands

| Command      | Purpose                                              |
| ------------ | ---------------------------------------------------- |
| `vix run`    | Build and run manually                               |
| `vix dev`    | Run development mode with rebuild and rerun workflow |
| `vix build`  | Configure and compile                                |
| `vix check`  | Validate build, tests, runtime, and sanitizers       |
| `vix tests`  | Run tests                                            |
| `vix logs`   | Inspect production logs                              |
| `vix health` | Check local, public, and WebSocket health            |

## What you should remember

`vix replay` reproduces a recorded run.

Replay records are created only when you run:

```bash
vix run <target> --replay
```

The common workflow is:

```bash
vix run api --replay
vix replay failed
```

The replay data lives in:

```txt
.vix/runs/
```

Use:

```bash
vix replay show last
```

to inspect a record.

Use:

```bash
vix replay last --dry-run
```

to preview the replay command.

Use:

```bash
vix replay clean
```

to remove old replay data.

## Next step

Continue with development mode.

[Open the vix dev guide](/cli/dev)
