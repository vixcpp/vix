# Vix CLI

The Vix CLI is the command-line interface for Vix.cpp.

It gives C++ a modern runtime-like development experience with one binary:

```txt
create
run
develop
build
test
format
package
deploy
inspect
upgrade
```

Vix does not try to replace C++.
It makes the normal C++ workflow faster, clearer, and easier to use.

## What the CLI gives you

The `vix` command is designed for real C++ applications.

It gives you:

- project creation
- C++ file and project execution
- development mode with reload
- fast builds with cache-aware behavior
- validation, tests, and sanitizers
- C++ file generation
- dependency management
- package registry workflows
- local package store tools
- packaging and verification
- database tools
- P2P tooling
- game project export
- local-first AI agent commands
- production service management
- reverse proxy generation
- health checks
- deployment workflow
- logs inspection
- environment checks
- CLI upgrade and uninstall tools

The goal is simple:

```txt
write C++
run it fast
validate it early
ship it safely
```

## Start in seconds

Create a new application:

```bash
vix new api
cd api
vix install
vix dev
```

This creates a ready-to-run project, installs dependencies, and starts the app in development mode.

## Run a single C++ file

```bash
vix run main.cpp
```

You can also run a C++ file directly:

```bash
vix main.cpp
```

Vix detects `.cpp` files and forwards them to `vix run`.

## Default interactive mode

Running `vix` without a command starts the interactive REPL:

```bash
vix
```

You can also start it explicitly:

```bash
vix repl
```

The REPL is useful for quick experiments, JSON work, expressions, and command-driven exploration.

## Core workflow

For a new project:

```bash
vix new api
cd api
vix install
vix dev
```

For an existing project:

```bash
vix install
vix dev
```

For a single C++ file:

```bash
vix run main.cpp
```

For validation:

```bash
vix check
vix tests
vix fmt --check
```

For release:

```bash
vix build --preset release
vix tests --preset release
vix pack --name api --version 1.0.0
vix verify --path ./dist/api@1.0.0
```

## Project commands

These commands are used while building and maintaining a Vix project.

| Command       | Purpose                                                  |
| ------------- | -------------------------------------------------------- |
| `vix new`     | Create a new Vix project.                                |
| `vix make`    | Generate C++ scaffolding.                                |
| `vix modules` | Manage application modules, including generated WebSocket modules. |
| `vix run`     | Build if needed, then run a project or file.             |
| `vix dev`     | Run development mode with reload.                        |
| `vix build`   | Configure and build a project or file.                   |
| `vix check`   | Validate build, runtime behavior, tests, and sanitizers. |
| `vix tests`   | Run tests.                                               |
| `vix test`    | Alias for `vix tests`.                                   |
| `vix fmt`     | Format C++ source files.                                 |
| `vix clean`   | Remove local project cache directories.                  |
| `vix reset`   | Run `vix clean`, then `vix install`.                     |
| `vix task`    | Run reusable project tasks.                              |
| `vix replay`  | Replay a previously recorded Vix execution.              |

## Runtime and development commands

Use these commands when you want to run code quickly.

| Command     | Use it when                                                    |
| ----------- | -------------------------------------------------------------- |
| `vix run`   | You want to run a project, a C++ file, or a manifest.          |
| `vix dev`   | You want a development loop with rebuilds and reload behavior. |
| `vix repl`  | You want an interactive Vix shell.                             |
| `vix check` | You want to validate before committing or shipping.            |

Example:

```bash
vix run main.cpp
vix dev
vix check --tests
```

## Build workflow

`vix build` is the command for compiling.

```bash
vix build
vix build --preset release
vix build --build-target vix
```

Vix keeps the C++ toolchain native, but gives the build process a cleaner command surface.

The build workflow can use:

```txt
CMake
Ninja
compiler flags
presets
target builds
artifact cache
object cache
build state
fallbacks
```

For a normal project:

```bash
vix build
```

For a release build:

```bash
vix build --preset release
```

For a target build:

```bash
vix build --build-target vix
```

## Development mode

`vix dev` is for active development.

```bash
vix dev
```

It watches project changes and reacts depending on what changed.

Typical behavior:

```txt
source file changed       -> rebuild
header file changed       -> rebuild
CMakeLists.txt changed    -> reconfigure and rebuild
vix.json changed          -> reconfigure and rebuild
vix.lock changed          -> reconfigure and rebuild
build output changed      -> ignore
.git changed              -> ignore
.vix changed              -> ignore
```

Use `vix dev` when you want the fastest edit-run loop.

## Code generation

`vix make` generates C++ files quickly.

```bash
vix make class User
vix make struct Claims
vix make enum Status
vix make function parse_token
vix make exception InvalidToken
vix make test AuthService
vix make config app
```

The shortcut form is also supported:

```bash
vix make:class User
vix make:struct Claims
vix make:test AuthService
```

Use `vix make` when a project already exists and you want to add files quickly.

Use `vix new` when you want to create a full project.

## Dependency commands

These commands manage project and global packages.

| Command        | Purpose                                                   |
| -------------- | --------------------------------------------------------- |
| `vix add`      | Add a package dependency to the current project.          |
| `vix install`  | Install exact project dependencies from `vix.lock`.       |
| `vix i`        | Alias for `vix install`.                                  |
| `vix update`   | Resolve newer dependency versions and rewrite `vix.lock`. |
| `vix up`       | Alias for `vix update`.                                   |
| `vix outdated` | Check which dependencies are behind the registry.         |
| `vix remove`   | Remove a project dependency.                              |
| `vix list`     | List project or global packages.                          |
| `vix search`   | Search packages in the local registry index.              |

Basic dependency workflow:

```bash
vix registry sync
vix add softadastra/json
vix install
vix list
```

Update workflow:

```bash
vix outdated
vix update --install
vix check --tests
```

After cloning an existing project:

```bash
vix install
```

Do not use update when you only want reproducible installs from `vix.lock`.

## Registry commands

The registry workflow is split into focused commands.

| Command              | Purpose                                                           |
| -------------------- | ----------------------------------------------------------------- |
| `vix registry init`  | Create a local `vix.json` package manifest.                       |
| `vix registry sync`  | Clone or refresh the local registry index.                        |
| `vix registry path`  | Print the registry index path.                                    |
| `vix search <query>` | Search the local registry index offline.                          |
| `vix publish`        | Publish a tagged package version through the registry workflow.   |
| `vix unpublish`      | Remove a package from the registry through the registry workflow. |
| `vix store`          | Manage the local package store.                                   |

Typical registry workflow:

```bash
vix registry sync
vix search json
vix add softadastra/json
vix install
```

Package publishing workflow:

```bash
vix registry init
vix check --tests
vix build --preset release

git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0

vix publish 0.2.0 --dry-run
vix publish 0.2.0
```

## Local store commands

`vix store` manages the local package store.

```bash
vix store path
vix store gc --project --dry-run
vix store gc --project
```

Use it when you want to inspect or clean package data stored under the local Vix environment.

## Packaging commands

Packaging commands are used to prepare artifacts for sharing, caching, verification, or deployment.

| Command      | Purpose                                                |
| ------------ | ------------------------------------------------------ |
| `vix pack`   | Create a distributable package folder or `.vixpkg`.    |
| `vix verify` | Verify a package against the `vix.manifest.v2` schema. |
| `vix cache`  | Store a verified package locally.                      |

Recommended package workflow:

```bash
vix build --preset release
vix check --tests
vix pack --name api --version 1.0.0
vix verify --path ./dist/api@1.0.0
vix cache --path ./dist/api@1.0.0
```

## Production commands

Vix includes production-oriented commands for local servers and Linux deployments.

| Command          | Purpose                                             |
| ---------------- | --------------------------------------------------- |
| `vix service`    | Install and manage a production `systemd` service.  |
| `vix proxy`      | Generate and validate reverse proxy configs.        |
| `vix health`     | Check local, public, and WebSocket app health.      |
| `vix deploy`     | Run the production deployment workflow.             |
| `vix logs`       | Show production app and proxy logs.                 |
| `vix env`        | Check project and production environment variables. |
| `vix production` | Show and validate production status.                |
| `vix ws`         | Check and diagnose WebSocket endpoints.             |

Typical production flow:

```bash
vix env check --production
vix service init
vix proxy nginx init
vix health
vix deploy
vix logs
```

Use these commands when your app is moving from local development to a real server.

## Database commands

Vix has two database command families.

| Command   | Purpose                                                              |
| --------- | -------------------------------------------------------------------- |
| `vix db`  | Inspect SQLite database and storage status.                          |
| `vix orm` | Run migration, rollback, status, and migration generation workflows. |

SQLite-oriented workflow:

```bash
vix db status
vix db migrate
vix db backup
```

ORM workflow:

```bash
vix orm makemigrations --new ./schema.new.json --dir ./migrations --name add_users
vix orm migrate --db blog_db --dir ./migrations
vix orm status --db blog_db
```

## Network and WebSocket commands

Vix includes tools for network-heavy applications.

| Command                | Purpose                                                    |
| ---------------------- | ---------------------------------------------------------- |
| `vix p2p`              | Run a P2P node with TCP, discovery, and bootstrap support. |
| `vix ws`               | Check and diagnose WebSocket endpoints.                    |
| `vix health websocket` | Check the configured WebSocket health target.              |

P2P example:

```bash
vix p2p --id A --listen 9001
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

WebSocket check:

```bash
vix ws check
vix health websocket
```

## Game commands

`vix game` is used for Vix game projects.

```bash
vix game export
```

Use it when you want to export and manage a game project built on the Vix game runtime.

## AI agent commands

`vix agent` runs the local-first Vix AI agent commands.

```bash
vix agent ask "Explain this project"
vix agent analyze
vix agent scan
```

Use it when you want AI-assisted project analysis, local model workflows, or workspace-aware agent behavior.

## System commands

These commands inspect, upgrade, or remove the Vix environment.

| Command          | Purpose                                                             |
| ---------------- | ------------------------------------------------------------------- |
| `vix info`       | Show Vix paths, caches, package counts, and local state.            |
| `vix doctor`     | Check environment health and optionally online release information. |
| `vix upgrade`    | Upgrade the Vix CLI or a globally installed package.                |
| `vix uninstall`  | Remove the Vix CLI or a globally installed package.                 |
| `vix completion` | Generate shell completion scripts.                                  |
| `vix version`    | Show the installed Vix version.                                     |

Environment inspection:

```bash
vix info
vix doctor
vix doctor --online
```

Upgrade:

```bash
vix upgrade --check
vix upgrade --dry-run
vix upgrade
```

## Global options

Global options are parsed before the command.

```bash
vix --help
vix --version
vix --verbose build
vix --quiet build
vix --log-level debug build
```

| Option                | Purpose                        |
| --------------------- | ------------------------------ |
| `-h, --help`          | Show global help.              |
| `-v, --version`       | Show version.                  |
| `--verbose`           | Enable debug logging.          |
| `-q, --quiet`         | Show only warnings and errors. |
| `--log-level <level>` | Set the log level.             |

Supported log levels:

```txt
trace
debug
info
warn
error
critical
```

`VIX_LOG_LEVEL` can also be used:

```bash
VIX_LOG_LEVEL=debug vix build
```

## Command help

Every command has its own help page.

```bash
vix help run
vix run --help

vix help build
vix build --help

vix help registry
vix registry --help
```

## Command shortcuts

| Shortcut              | Meaning                     |
| --------------------- | --------------------------- |
| `vix main.cpp`        | Runs `vix run main.cpp`.    |
| `vix app.vix`         | Runs `vix run app.vix`.     |
| `vix make:class User` | Runs `vix make class User`. |
| `vix test`            | Alias for `vix tests`.      |
| `vix up`              | Alias for `vix update`.     |
| `vix i`               | Alias for `vix install`.    |

`vix deps` still exists as a deprecated alias, but new documentation should use:

```bash
vix install
```

## Typical usage

### Create and run a new app

```bash
vix new api
cd api
vix install
vix dev
```

### Run a single C++ file

```bash
vix run main.cpp
```

### Pass runtime arguments to a C++ file

```bash
vix run main.cpp --run --port 8080
```

Use `--run` for runtime arguments.

Everything after `--` is forwarded to the compiler or linker in script mode.

### Build a project

```bash
vix build
```

### Build a target

```bash
vix build --build-target vix
```

### Validate with tests

```bash
vix check --tests
```

### Validate with sanitizers

```bash
vix check --san
```

### Run tests only

```bash
vix tests
```

### Format code

```bash
vix fmt
vix fmt --check
```

### Add a dependency

```bash
vix registry sync
vix add softadastra/json
vix install
```

### Package a project

```bash
vix pack --name api --version 1.0.0
```

### Verify a package

```bash
vix verify --path ./dist/api@1.0.0
```

### Deploy a production app

```bash
vix env check --production
vix deploy
vix health
vix logs
```

## When to use each command

Use `vix` when you want the interactive REPL.

Use `vix new` when starting a new project.

Use `vix make` when you want to generate C++ files inside an existing project.

Use `vix modules` when you want to manage optional project modules.

Use `vix run` when you want to build and run a project, a single `.cpp` file, or a `.vix` manifest.

Use `vix dev` when you are actively developing and want automatic rebuild behavior.

Use `vix build` when you only want to configure and compile.

Use `vix check` when you want validation before committing or shipping.

Use `vix tests` when you only want to run tests.

Use `vix fmt` before committing source code.

Use `vix task` when your project defines reusable workflows.

Use `vix add`, `vix install`, `vix update`, and `vix remove` for dependency management.

Use `vix registry`, `vix search`, `vix publish`, and `vix unpublish` for registry workflows.

Use `vix pack`, `vix verify`, and `vix cache` when preparing artifacts.

Use `vix service`, `vix proxy`, `vix deploy`, `vix health`, `vix logs`, `vix env`, `vix production`, and `vix ws` when moving toward production.

Use `vix info` when you want to inspect paths and caches.

Use `vix doctor` when something does not work and you want diagnostics.

Use `vix upgrade` when you want to update Vix itself.

## CLI design philosophy

The Vix CLI exists to make C++ feel direct without hiding what C++ is.

It keeps the native C++ toolchain:

```txt
compiler
linker
CMake
Ninja
system libraries
native binaries
```

But it gives the developer a simpler workflow:

```txt
vix new
vix install
vix dev
vix build
vix check
vix pack
vix deploy
```

Vix is not a replacement for C++.
It is a modern workflow around C++.

The principle is:

```txt
simple command
native output
clear diagnostics
safe defaults
```

## Next step

Continue with project creation.

[Open the vix new guide](/cli/new)
