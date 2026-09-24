# Diagnostics

Diagnostics are the tools and habits you use to understand what Vix is doing.

Use this guide when a command fails, a build is slower than expected, a dependency is missing, a server does not start, a production deployment behaves incorrectly, or you need to explain what happened.

## The idea

Vix is designed to make failures visible.

A good diagnostic workflow should answer:

```txt
What command ran?
What did Vix detect?
What changed?
What failed?
What should I run next?
```

Diagnostics are not only for errors. They also help you understand build state, caches, dependency state, environment configuration, runtime behavior, production health, and logs.

## Start with the right command

Use this map first:

| Problem                                                       | Start with                           |
| ------------------------------------------------------------- | ------------------------------------ |
| Environment or tools look broken                              | `vix doctor`                         |
| You need paths, cache state, registry state, or package state | `vix info`                           |
| Build failed or rebuilt unexpectedly                          | `vix build --explain`                |
| Tests fail                                                    | `vix tests` or `vix check --tests`   |
| Runtime behavior is hard to reproduce                         | `vix run --replay` then `vix replay` |
| Environment variables may be wrong                            | `vix env check`                      |
| Production service is unhealthy                               | `vix health`                         |
| Production logs are needed                                    | `vix logs`                           |
| WebSocket endpoint is suspicious                              | `vix ws check`                       |
| Database state is suspicious                                  | `vix db status`                      |

## Basic diagnostic flow

A good local debugging flow is:

```bash
vix doctor
vix info
vix build --explain
vix check --tests
```

For a runtime issue:

```bash
vix run --replay
vix replay show last
vix replay last
```

For a production issue:

```bash
vix health
vix logs --errors
vix service status
vix proxy nginx check
```

## Environment diagnostics

Use `vix doctor` when you want to check whether your machine is ready for Vix.

```bash
vix doctor
```

Use online checks when you want to verify release information:

```bash
vix doctor --online
```

Use JSON output for scripts or CI:

```bash
vix doctor --json
vix doctor --json --online
```

Typical things to diagnose with `vix doctor`:

```txt
Vix CLI installation
compiler availability
CMake availability
Ninja availability
Git availability
required tools
upgrade readiness
latest release information
```

## Inspect Vix local state

Use `vix info` when you want to see where Vix stores things.

```bash
vix info
```

This can show:

```txt
Vix version
Vix root
registry path
store path
global package manifest
artifact cache path
store size
artifact cache size
package counts
cache state
```

Use it before cleanup commands:

```bash
vix info
vix store gc --dry-run
```

Use it to inspect a project dependency:

```bash
vix info softadastra/json
```

Use it to inspect a global package:

```bash
vix info -g softadastra/json
```

## Build diagnostics

Use `--explain` when a build rebuilds unexpectedly or when you want to understand why Vix chose a path.

```bash
vix build --explain
```

For a specific target:

```bash
vix build --build-target vix --explain
```

Useful cases:

```txt
source file changed
header file changed
CMakeLists.txt changed
compiler flags changed
target changed
artifact cache hit
artifact cache miss
fallback to Ninja
target is already up to date
```

Example output shape:

```txt
Rebuilding BuildCommand.cpp
  reason: source file changed

Relinking vix
  reason: object file changed
```

When Vix cannot prove the exact dependency relationship, it should not lie. It should fall back safely.

Example shape:

```txt
Project input changed
  reason: dependency changed, delegating target to Ninja

Relinking vix
  reason: target may depend on changed input
```

## Fast build diagnostics

For no-op target builds, check whether Vix can return early safely:

```bash
vix build --build-target vix
```

Expected shape when clean:

```txt
Checking vix (dev)
  ✔ Up to date in 0.3s
```

If the output is missing, Vix must rebuild or restore from cache.

```bash
rm -f build-ninja/vix
vix build --build-target vix
```

A state hit alone is not enough. The output must still be valid.

## Cache diagnostics

Vix has multiple cache layers.

```txt
BuildState     -> fast no-op validation
ArtifactCache  -> final binary or package restore
BuildGraph     -> target-aware analysis
ObjectCache    -> compile output reuse
CMake/Ninja    -> compatibility fallback
```

Use `vix info` to inspect cache locations:

```bash
vix info
```

Use store commands for package store diagnostics:

```bash
vix store path
vix store gc --dry-run
```

Use build explanations for build-cache behavior:

```bash
vix build --explain
```

Use verbose build output when you need more details:

```bash
vix build -v
```

## Dependency diagnostics

When packages are not found, start with the registry:

```bash
vix registry sync
```

Then inspect dependencies:

```bash
vix list
vix outdated
```

If local dependency state is broken:

```bash
vix reset
```

If you only need locked installs:

```bash
vix install
```

Use this flow:

```bash
vix registry sync
vix install
vix list
vix check --tests
```

## Registry diagnostics

Use:

```bash
vix registry path
```

to see where the local registry index is stored.

Use:

```bash
vix registry sync
```

to refresh it.

Use:

```bash
vix search json
```

to confirm the registry index is usable.

If `vix add`, `vix update`, `vix outdated`, or `vix publish` cannot find package metadata, run:

```bash
vix registry sync
```

## Runtime diagnostics

Use replay when runtime behavior must be reproduced.

Record a run:

```bash
vix run api --replay
```

Inspect the latest run:

```bash
vix replay show last
```

Replay it:

```bash
vix replay last
```

Replay the latest failed run:

```bash
vix replay failed
```

Replay with debug logging:

```bash
vix replay failed --env VIX_LOG_LEVEL=debug
```

This is useful when the problem depends on runtime arguments, environment, current directory, or the exact executable path.

## Runtime arguments diagnostics

If your app did not receive arguments, check the separator.

Wrong:

```bash
vix run main.cpp --name Gaspard
```

Correct:

```bash
vix run main.cpp -- --name Gaspard
```

Everything after `--` is passed to your app.

For replay:

```bash
vix replay last -- --debug
```

## Environment variable diagnostics

Use:

```bash
vix env check
```

For production requirements:

```bash
vix env check --production
```

Show values safely:

```bash
vix env check --production --show-values
```

Secrets are masked.

Use this when:

```txt
.env is missing
.env.example is incomplete
production required variables are missing
systemd environment does not match expected variables
configuration differs between local and production
```

## Logging diagnostics

For local runtime logging, use environment variables such as:

```dotenv
VIX_LOG_LEVEL=debug
VIX_LOG_FORMAT=kv
```

For one execution:

```bash
VIX_LOG_LEVEL=debug vix run
```

For replay:

```bash
vix replay failed --env VIX_LOG_LEVEL=debug
```

For production logs:

```bash
vix logs
vix logs app
vix logs proxy
vix logs errors
```

Follow logs:

```bash
vix logs app -f
```

Show recent errors:

```bash
vix logs errors --lines 100
```

## Production diagnostics

For production, start with health:

```bash
vix health
```

Check only the local application endpoint:

```bash
vix health local
```

Check the public endpoint:

```bash
vix health public
```

Check WebSocket:

```bash
vix health websocket
```

Then inspect logs:

```bash
vix logs --errors
```

Then inspect service state:

```bash
vix service status
```

Then inspect proxy state:

```bash
vix proxy nginx check
```

A practical production flow:

```bash
vix health
vix logs errors --lines 100
vix service status
vix proxy nginx check
```

## WebSocket diagnostics

Use:

```bash
vix ws check
```

Use a specific URL when needed:

```bash
vix ws check --url ws://127.0.0.1:9090/ws
```

Use ping when supported:

```bash
vix ws check --ping
```

Use verbose output:

```bash
vix ws check --verbose
```

This helps diagnose:

```txt
wrong WebSocket path
wrong port
proxy upgrade headers
timeout
public URL mismatch
local URL mismatch
heartbeat behavior
```

## Proxy diagnostics

Check the installed Nginx proxy config:

```bash
vix proxy nginx check
```

Validate and reload Nginx:

```bash
vix proxy nginx reload
```

Install generated config:

```bash
vix proxy nginx init
```

Issue or renew a certificate:

```bash
vix proxy nginx certbot
```

If the public endpoint fails but local health passes, check the proxy:

```bash
vix health local
vix health public
vix proxy nginx check
vix logs proxy
```

## Service diagnostics

Use service diagnostics when the app is managed by systemd.

```bash
vix service status
```

Restart the service:

```bash
vix service restart
```

Follow logs:

```bash
vix logs app -f
```

If deployment restarted the service but the app is not healthy:

```bash
vix service status
vix logs app --lines 100
vix health local
```

## Database diagnostics

Use:

```bash
vix db status
```

This checks database configuration and local SQLite state.

Use migrations:

```bash
vix db migrate
```

Back up SQLite:

```bash
vix db backup
```

For ORM migration status:

```bash
vix orm status --db blog_db --dir ./migrations
```

Use this when:

```txt
database file is missing
storage directory is missing
storage is not writable
WAL or SHM files are unexpected
migrations directory is missing
migration state is unclear
```

## Deployment diagnostics

Before deployment:

```bash
vix env check --production
vix check --tests
vix build --preset release
```

During deployment, use dry run:

```bash
vix deploy --dry-run
```

Verbose deployment:

```bash
vix deploy --verbose
```

If deployment fails:

```bash
vix logs errors --lines 100
vix service status
vix health
```

If deployment should not pull or run tests:

```bash
vix deploy --no-pull
vix deploy --no-tests
```

## Test diagnostics

Run tests:

```bash
vix tests
```

Fail fast:

```bash
vix tests --fail-fast
```

Run validation with tests:

```bash
vix check --tests
```

For release validation:

```bash
vix build --preset release
vix tests --preset release
```

## Formatting diagnostics

Check formatting without modifying files:

```bash
vix fmt --check
```

Format the project:

```bash
vix fmt
```

If generated files are not formatted:

```bash
vix make class User --in src/domain
vix fmt
```

## Cleanup diagnostics

If local generated state is broken, clean project state:

```bash
vix clean
```

Then reinstall:

```bash
vix install
```

Or use one command:

```bash
vix reset
```

Use `vix clean` for local project state.

Use `vix store` for global package store cleanup.

Use `vix info` before deleting global data.

## Common workflows

### Diagnose a broken local setup

```bash
vix doctor
vix info
vix registry sync
vix install
vix check --tests
```

### Diagnose a strange build

```bash
vix build --explain
vix build --build-target vix --explain
vix info
```

### Diagnose missing dependencies

```bash
vix registry sync
vix install
vix list
vix check --tests
```

### Diagnose a runtime failure

```bash
vix run api --replay
vix replay show failed
vix replay failed
vix replay failed --env VIX_LOG_LEVEL=debug
```

### Diagnose production failure

```bash
vix health
vix logs errors --lines 100
vix service status
vix proxy nginx check
```

### Diagnose WebSocket failure

```bash
vix health websocket
vix ws check --verbose
vix logs proxy --lines 100
```

### Diagnose deployment safely

```bash
vix env check --production
vix deploy --dry-run
vix deploy --verbose
vix health
```

## Common mistakes

### Running random cleanup first

Wrong:

```bash
rm -rf ~/.vix
```

Better:

```bash
vix info
vix doctor
vix store gc --dry-run
```

Do not delete global state before understanding what is broken.

### Using `vix clean` for global cache problems

`vix clean` only affects the current project.

Use:

```bash
vix info
vix store path
vix store gc --dry-run
```

for global package store diagnostics.

### Forgetting to sync the registry

If packages cannot be found:

```bash
vix registry sync
```

Then retry:

```bash
vix add namespace/name
vix install
```

### Forgetting `--replay`

Wrong:

```bash
vix run api
vix replay failed
```

Correct:

```bash
vix run api --replay
vix replay failed
```

Replay records are created only when recording is enabled.

### Debugging production without checking local health

Check local first:

```bash
vix health local
```

Then public:

```bash
vix health public
```

If local works but public fails, the issue is likely proxy, TLS, DNS, firewall, or public routing.

### Passing app arguments without `--`

Wrong:

```bash
vix run main.cpp --debug
```

Correct:

```bash
vix run main.cpp -- --debug
```

## Recommended diagnostic order

Use this order when you are not sure where the problem is:

```bash
vix doctor
vix info
vix env check
vix registry sync
vix install
vix build --explain
vix check --tests
```

For runtime:

```bash
vix run --replay
vix replay show last
vix replay last
```

For production:

```bash
vix env check --production
vix health
vix logs errors --lines 100
vix service status
vix proxy nginx check
```

## Related commands

| Command               | Purpose                                    |
| --------------------- | ------------------------------------------ |
| `vix doctor`          | Check environment health                   |
| `vix info`            | Show paths, caches, and local state        |
| `vix build --explain` | Explain build decisions                    |
| `vix check`           | Validate project health                    |
| `vix tests`           | Run tests                                  |
| `vix replay`          | Reproduce recorded runs                    |
| `vix env`             | Check environment files                    |
| `vix health`          | Check local, public, and WebSocket health  |
| `vix logs`            | Read app, proxy, and error logs            |
| `vix ws`              | Check WebSocket endpoint                   |
| `vix service`         | Manage systemd service                     |
| `vix proxy`           | Manage Nginx proxy                         |
| `vix db`              | Check, migrate, and back up database state |

## What you should remember

Start with information before cleanup:

```bash
vix doctor
vix info
```

Use explanations for builds:

```bash
vix build --explain
```

Use replay for runtime failures:

```bash
vix run api --replay
vix replay failed
```

Use health and logs for production:

```bash
vix health
vix logs errors --lines 100
```

Use environment checks when configuration may be wrong:

```bash
vix env check --production
```

The best diagnostic flow is simple:

```txt
inspect first
explain what changed
reproduce the failure
then fix
```

## Next step

Continue with production deployment.

[Open the production guide](/guides/production-nginx-systemd)
