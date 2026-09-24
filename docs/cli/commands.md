# CLI Commands

This page is the command map for the Vix CLI.

Use it when you want to quickly understand what each command does, which workflow it belongs to, and which detailed guide to read next.

## Command format

```bash
vix <command> [options]
```

Examples:

```bash
vix new api
vix install
vix dev
vix build
vix check --tests
```

Running `vix` without a command starts the interactive REPL:

```bash
vix
```

Show global help:

```bash
vix --help
```

Show the installed version:

```bash
vix --version
```

## The main idea

Vix gives C++ a modern command workflow.

Instead of manually wiring every step, you get one command surface for:

```txt
create
generate
install
run
develop
build
check
test
format
package
verify
cache
publish
deploy
inspect
upgrade
```

Vix keeps native C++.
It simplifies the workflow around it.

## Core project workflow

Most new Vix projects start like this:

```bash
vix new api
cd api
vix install
vix dev
```

Existing projects usually start like this:

```bash
vix install
vix dev
```

Single-file C++ usage:

```bash
vix run main.cpp
```

You can also run a `.cpp` file directly:

```bash
vix main.cpp
```

## Project commands

These commands are used to create, run, build, validate, and maintain a project.

| Command          | Purpose                                                  | Guide                       |
| ---------------- | -------------------------------------------------------- | --------------------------- |
| `vix`            | Start the interactive REPL.                              | [REPL](/cli/repl)           |
| `vix repl`       | Start the REPL explicitly.                               | [vix repl](/cli/repl)       |
| `vix new <name>` | Create a new Vix project.                                | [vix new](/cli/new)         |
| `vix make`       | Generate C++ scaffolding.                                | [vix make](/cli/make)       |
| `vix modules`    | Manage optional project modules.                         | [vix modules](/cli/modules) |
| `vix run`        | Build if needed, then run a project, file, or manifest.  | [vix run](/cli/run)         |
| `vix dev`        | Run the app in development mode with reload behavior.    | [vix dev](/cli/dev)         |
| `vix build`      | Configure and build a project or file.                   | [vix build](/cli/build)     |
| `vix check`      | Validate build, tests, runtime behavior, and sanitizers. | [vix check](/cli/check)     |
| `vix tests`      | Run project tests.                                       | [vix tests](/cli/tests)     |
| `vix test`       | Alias for `vix tests`.                                   | [vix tests](/cli/tests)     |
| `vix fmt`        | Format C++ source files.                                 | [vix fmt](/cli/fmt)         |
| `vix clean`      | Remove local project cache directories.                  | [vix clean](/cli/clean)     |
| `vix reset`      | Run `vix clean`, then `vix install`.                     | [vix reset](/cli/reset)     |
| `vix task`       | Run reusable project tasks.                              | [vix task](/cli/task)       |
| `vix replay`     | Replay a previously recorded Vix execution.              | [vix replay](/cli/replay)   |

## Dependency and registry commands

These commands manage packages, the registry index, project dependencies, and global packages.

| Command              | Purpose                                                   | Guide                           |
| -------------------- | --------------------------------------------------------- | ------------------------------- |
| `vix registry`       | Manage the local registry index.                          | [vix registry](/cli/registry)   |
| `vix registry init`  | Create a local `vix.json` package manifest.               | [vix registry](/cli/registry)   |
| `vix registry sync`  | Clone or refresh the local registry index.                | [vix registry](/cli/registry)   |
| `vix registry path`  | Print the local registry index path.                      | [vix registry](/cli/registry)   |
| `vix search <query>` | Search packages in the local registry index.              | [vix search](/cli/search)       |
| `vix add <pkg>`      | Add a package dependency to the project.                  | [vix add](/cli/add)             |
| `vix install`        | Install exact dependencies from `vix.lock`.               | [vix install](/cli/install)     |
| `vix update`         | Resolve newer dependency versions and rewrite `vix.lock`. | [vix update](/cli/update)       |
| `vix outdated`       | Check which dependencies are behind the registry.         | [vix outdated](/cli/outdated)   |
| `vix remove <pkg>`   | Remove a project dependency.                              | [vix remove](/cli/remove)       |
| `vix list`           | List project or global packages.                          | [vix list](/cli/list)           |
| `vix store`          | Manage the local package store.                           | [vix store](/cli/store)         |
| `vix publish`        | Publish a tagged package version.                         | [vix publish](/cli/publish)     |
| `vix unpublish`      | Remove a package from the registry workflow.              | [vix unpublish](/cli/unpublish) |

## Dependency aliases

| Alias      | Equivalent command | Status     |
| ---------- | ------------------ | ---------- |
| `vix i`    | `vix install`      | Current    |
| `vix up`   | `vix update`       | Current    |
| `vix deps` | `vix install`      | Deprecated |

Use this in new documentation:

```bash
vix install
```

Do not recommend this in new pages:

```bash
vix deps
```

## Packaging commands

These commands are used to create, verify, and cache distributable artifacts.

| Command      | Purpose                                        | Guide                     |
| ------------ | ---------------------------------------------- | ------------------------- |
| `vix pack`   | Create a package folder or `.vixpkg` artifact. | [vix pack](/cli/pack)     |
| `vix verify` | Verify a package against `vix.manifest.v2`.    | [vix verify](/cli/verify) |
| `vix cache`  | Store a verified package locally.              | [vix cache](/cli/cache)   |

Recommended flow:

```bash
vix build --preset release
vix check --tests
vix pack --name api --version 1.0.0
vix verify --path ./dist/api@1.0.0
vix cache --path ./dist/api@1.0.0
```

## Production commands

These commands are used when an app is moving toward a server or production environment.

| Command          | Purpose                                             | Guide                                                   |
| ---------------- | --------------------------------------------------- | ------------------------------------------------------- |
| `vix service`    | Install and manage a production `systemd` service.  | [vix service](/cli/service)                             |
| `vix proxy`      | Generate and validate reverse proxy configs.        | [vix proxy](/cli/proxy)                                 |
| `vix health`     | Check local, public, and WebSocket health targets.  | [vix health](/cli/health)                               |
| `vix deploy`     | Run the production deployment workflow.             | [vix deploy](/cli/deploy)                               |
| `vix logs`       | Show production app and proxy logs.                 | [vix logs](/cli/logs)                                   |
| `vix env`        | Check project and production environment variables. | [vix env](/cli/env)                                     |
| `vix production` | Show and validate production status.                | [Production workflow](/guides/production-nginx-systemd) |
| `vix ws`         | Check and diagnose WebSocket endpoints.             | [vix ws](/cli/ws)                                       |

Typical production flow:

```bash
vix env check --production
vix service init
vix proxy nginx init
vix health
vix deploy
vix logs
```

## Database commands

Vix has two database command families.

| Command   | Purpose                                                              | Guide               |
| --------- | -------------------------------------------------------------------- | ------------------- |
| `vix db`  | Inspect SQLite database state, storage, migrations, and backups.     | [vix db](/cli/db)   |
| `vix orm` | Run migration, rollback, status, and migration generation workflows. | [vix orm](/cli/orm) |

SQLite-focused workflow:

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

## Network commands

These commands are useful for networked, real-time, or peer-to-peer apps.

| Command                | Purpose                                                    | Guide                     |
| ---------------------- | ---------------------------------------------------------- | ------------------------- |
| `vix p2p`              | Run a P2P node with TCP, discovery, and bootstrap support. | [vix p2p](/cli/p2p)       |
| `vix ws`               | Check and diagnose WebSocket endpoints.                    | [vix ws](/cli/ws)         |
| `vix health websocket` | Check the configured WebSocket health target.              | [vix health](/cli/health) |

P2P example:

```bash
vix p2p --id A --listen 9001
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

WebSocket example:

```bash
vix ws check
vix health websocket
```

## Game commands

| Command    | Purpose                              | Guide                 |
| ---------- | ------------------------------------ | --------------------- |
| `vix game` | Export and manage Vix game projects. | [vix game](/cli/game) |

Example:

```bash
vix game export
```

Use this when working with projects built on the Vix game runtime.

## AI commands

| Command     | Purpose                                | Guide                   |
| ----------- | -------------------------------------- | ----------------------- |
| `vix agent` | Run local-first Vix AI agent commands. | [vix agent](/cli/agent) |

Examples:

```bash
vix agent ask "Explain this project"
vix agent analyze
vix agent scan
```

## System commands

These commands inspect, repair, upgrade, or remove the Vix environment.

| Command          | Purpose                                                     | Guide                             |
| ---------------- | ----------------------------------------------------------- | --------------------------------- |
| `vix info`       | Show Vix paths, caches, package counts, and local state.    | [vix info](/cli/info)             |
| `vix doctor`     | Check environment health and optional online release state. | [vix doctor](/cli/doctor)         |
| `vix upgrade`    | Upgrade the Vix CLI or a global package.                    | [vix upgrade](/cli/upgrade)       |
| `vix uninstall`  | Remove the Vix CLI or a global package.                     | [vix uninstall](/cli/uninstall)   |
| `vix completion` | Generate shell completion scripts.                          | [vix completion](/cli/completion) |
| `vix version`    | Show the installed Vix version.                             | [vix version](/cli/)              |

Environment inspection:

```bash
vix info
vix doctor
vix doctor --online
```

Upgrade flow:

```bash
vix upgrade --check
vix upgrade --dry-run
vix upgrade
vix --version
vix doctor
```

## Help commands

| Command                | Purpose                           |
| ---------------------- | --------------------------------- |
| `vix help`             | Show general help.                |
| `vix help <command>`   | Show help for a specific command. |
| `vix <command> --help` | Show help for a specific command. |
| `vix -h`               | Show general help.                |
| `vix --help`           | Show general help.                |

Examples:

```bash
vix help run
vix run --help
vix build --help
vix registry --help
```

## Global options

Global options are parsed before the command.

| Option                | Purpose                        |
| --------------------- | ------------------------------ |
| `-h, --help`          | Show help.                     |
| `-v, --version`       | Show version.                  |
| `--verbose`           | Enable debug logs.             |
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

Example:

```bash
vix --log-level debug build
```

You can also use the environment variable:

```bash
VIX_LOG_LEVEL=debug vix build
```

## Command shortcuts

| Shortcut              | Meaning                     |
| --------------------- | --------------------------- |
| `vix main.cpp`        | Runs `vix run main.cpp`.    |
| `vix app.vix`         | Runs `vix run app.vix`.     |
| `vix make:class User` | Runs `vix make class User`. |
| `vix test`            | Runs `vix tests`.           |
| `vix i`               | Runs `vix install`.         |
| `vix up`              | Runs `vix update`.          |

## Project lifecycle map

| Stage                | Command       |
| -------------------- | ------------- |
| Create project       | `vix new`     |
| Generate files       | `vix make`    |
| Add modules          | `vix modules` |
| Install dependencies | `vix install` |
| Develop              | `vix dev`     |
| Run manually         | `vix run`     |
| Format               | `vix fmt`     |
| Validate             | `vix check`   |
| Test                 | `vix tests`   |
| Build                | `vix build`   |
| Package              | `vix pack`    |
| Verify artifact      | `vix verify`  |
| Cache artifact       | `vix cache`   |
| Deploy               | `vix deploy`  |
| Inspect              | `vix info`    |
| Diagnose             | `vix doctor`  |

## New project workflow

```bash
vix new api
cd api
vix install
vix dev
```

Before committing:

```bash
vix fmt --check
vix check --tests
```

Before release:

```bash
vix build --preset release
vix tests --preset release
vix pack --name api --version 1.0.0
vix verify --path ./dist/api@1.0.0
```

## Existing project workflow

```bash
git clone https://github.com/example/api.git
cd api
vix install
vix dev
vix check --tests
```

## Single-file workflow

```bash
vix run main.cpp
vix run main.cpp --run --port 8080
vix run main.cpp -- -O2 -DNDEBUG
vix run main.cpp --san
vix check main.cpp --san
```

Use `--run` for runtime arguments.

Everything after `--` is forwarded to the compiler or linker in script mode.

## Dependency workflow

Add a dependency:

```bash
vix registry sync
vix add softadastra/json
vix install
```

Check dependencies:

```bash
vix list
vix outdated
```

Update dependencies:

```bash
vix update --install
vix check --tests
```

Remove a dependency:

```bash
vix remove softadastra/json
vix install
vix check --tests
```

## Registry workflow

Search packages:

```bash
vix registry sync
vix search softadastra
```

Initialize package metadata:

```bash
vix registry init
```

Publish a package:

```bash
vix check --tests
vix build --preset release

git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0

vix publish 0.2.0 --dry-run
vix publish 0.2.0
```

## Production workflow

Prepare environment:

```bash
vix env check --production
```

Install service:

```bash
vix service init
vix service status
```

Install proxy:

```bash
vix proxy nginx init
vix proxy nginx check
```

Check health:

```bash
vix health
vix health local
vix health public
vix health websocket
```

Deploy:

```bash
vix deploy
```

Inspect logs:

```bash
vix logs
vix logs --errors
```

## Choosing the right command

| Goal                                              | Use            |
| ------------------------------------------------- | -------------- |
| Explore interactively                             | `vix`          |
| Create a project                                  | `vix new`      |
| Generate a class, test, enum, function, or config | `vix make`     |
| Add optional modules                              | `vix modules`  |
| Run during development                            | `vix dev`      |
| Run manually                                      | `vix run`      |
| Compile only                                      | `vix build`    |
| Validate deeply                                   | `vix check`    |
| Run tests only                                    | `vix tests`    |
| Format source files                               | `vix fmt`      |
| Add dependencies                                  | `vix add`      |
| Install dependencies                              | `vix install`  |
| Update dependencies                               | `vix update`   |
| Check outdated dependencies                       | `vix outdated` |
| Search registry packages                          | `vix search`   |
| Package project                                   | `vix pack`     |
| Verify package                                    | `vix verify`   |
| Cache package                                     | `vix cache`    |
| Publish package                                   | `vix publish`  |
| Inspect environment                               | `vix info`     |
| Diagnose setup                                    | `vix doctor`   |
| Manage production service                         | `vix service`  |
| Configure reverse proxy                           | `vix proxy`    |
| Deploy production app                             | `vix deploy`   |
| View production logs                              | `vix logs`     |
| Check WebSocket endpoint                          | `vix ws`       |
| Run a P2P node                                    | `vix p2p`      |
| Export a game project                             | `vix game`     |
| Run AI agent commands                             | `vix agent`    |

## Common mistakes

### Passing runtime arguments after `--` in script mode

Wrong:

```bash
vix run main.cpp -- --port 8080
```

Correct:

```bash
vix run main.cpp --run --port 8080
```

### Running project commands outside the project directory

Wrong:

```bash
vix new api
vix dev
```

Correct:

```bash
vix new api
cd api
vix dev
```

### Forgetting to install dependencies

For a fresh or cloned project, run:

```bash
vix install
```

Then start development:

```bash
vix dev
```

### Using update when you only need install

After cloning a project, use:

```bash
vix install
```

Use `vix update` only when you want to resolve newer dependency versions.

### Using `vix deps` in new documentation

`vix deps` is a deprecated alias.

Use:

```bash
vix install
```

## Recommended daily workflow

```bash
# Start development
vix dev

# Run tests
vix tests

# Before committing
vix fmt --check
vix check --tests

# Before release
vix build --preset release
vix tests --preset release
vix pack --name api --version 1.0.0
vix verify --path ./dist/api@1.0.0
```

## Full command list

Each entry below is used as:

```bash
vix <command>

add
agent
build
cache
check
clean
completion
db
deploy
dev
doctor
env
fmt
game
health
info
install
list
logs
make
modules
new
orm
outdated
p2p
pack
production
proxy
publish
registry
remove
replay
repl
reset
run
search
service
store
task
test
tests
uninstall
unpublish
update
upgrade
verify
version
ws
```

## Next step

Start with project creation.

[Open the vix new guide](/cli/new)
