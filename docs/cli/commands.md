# CLI Commands

This page is a complete overview of the Vix CLI command surface.

Use it when you want to quickly find the purpose of a command, its category, and the next detailed page to read.

## Command overview

```bash
vix <command> [options]
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

## Core workflow

Most Vix projects start with this flow:

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

## Project commands

| Command | Purpose | Guide |
|---------|---------|-------|
| `vix` | Start the interactive REPL | [REPL](/cli/repl) |
| `vix repl` | Start the REPL explicitly | [REPL](/cli/repl) |
| `vix new <name>` | Create a new Vix project | [vix new](/cli/new) |
| `vix make` | Generate C++ scaffolding | [vix make](/cli/make) |
| `vix dev` | Run an app in development mode with reload | [vix dev](/cli/dev) |
| `vix run` | Build and run a project, file, or manifest | [vix run](/cli/run) |
| `vix build` | Configure and build a CMake project | [vix build](/cli/build) |
| `vix check` | Validate build, tests, runtime, and sanitizers | [vix check](/cli/check) |
| `vix tests` | Run project tests | [vix tests](/cli/tests) |
| `vix fmt` | Format C++ source files | [vix fmt](/cli/fmt) |
| `vix clean` | Remove local project cache directories | [vix clean](/cli/clean) |
| `vix reset` | Clean and reinstall the project | [vix reset](/cli/reset) |
| `vix task` | Run reusable project tasks | [vix task](/cli/task) |

## Dependency commands

| Command | Purpose | Guide |
|---------|---------|-------|
| `vix add <pkg>` | Add a package to the project | [vix add](/cli/add) |
| `vix install` | Install dependencies from `vix.lock` | [vix install](/cli/install) |
| `vix update` | Update dependencies | [vix update](/cli/update) |
| `vix outdated` | Check outdated dependencies | [vix outdated](/cli/outdated) |
| `vix remove <pkg>` | Remove a dependency | [vix remove](/cli/remove) |
| `vix list` | List project or global packages | [vix list](/cli/list) |

### Dependency aliases

| Alias | Equivalent command |
|-------|--------------------|
| `vix up` | `vix update` |
| `vix i` | `vix install` |
| `vix deps` | `vix install` |

## Packaging commands

| Command | Purpose | Guide |
|---------|---------|-------|
| `vix pack` | Build a distributable package | [vix pack](/cli/pack) |
| `vix verify` | Verify package integrity | [vix verify](/cli/verify) |
| `vix cache` | Store a package locally | [vix cache](/cli/cache) |

## Advanced commands

| Command | Purpose | Guide |
|---------|---------|-------|
| `vix registry` | Manage the local registry index | [vix registry](/cli/registry) |
| `vix store` | Manage the local package store | [vix store](/cli/store) |
| `vix orm` | Run database migration tooling | [vix orm](/cli/orm) |
| `vix p2p` | Run a peer-to-peer node | [vix p2p](/cli/p2p) |

## System commands

| Command | Purpose | Guide |
|---------|---------|-------|
| `vix info` | Show Vix paths, caches, and local state | [vix info](/cli/info) |
| `vix doctor` | Check the local environment | [vix doctor](/cli/doctor) |
| `vix upgrade` | Upgrade Vix or a global package | [vix upgrade](/cli/upgrade) |
| `vix uninstall` | Remove Vix or a global package | [vix uninstall](/cli/uninstall) |
| `vix completion` | Generate shell completion scripts | [vix completion](/cli/completion) |
| `vix version` | Show the installed version | — |

## Help commands

| Command | Purpose |
|---------|---------|
| `vix help` | Show general help |
| `vix help <command>` | Show help for a specific command |
| `vix <command> --help` | Show help for a specific command |
| `vix -h` | Show general help |
| `vix --help` | Show general help |

Examples:

```bash
vix help run
vix run --help
vix new --help
```

## Global options

| Option | Purpose |
|--------|---------|
| `-h, --help` | Show help |
| `-v, --version` | Show version |
| `--verbose` | Enable debug logs |
| `-q, --quiet` | Show only warnings and errors |
| `--log-level <level>` | Set log level |

Supported log levels: `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`

## Project lifecycle map

| Stage | Command |
|-------|---------|
| Create project | `vix new` |
| Install dependencies | `vix install` |
| Develop | `vix dev` |
| Run manually | `vix run` |
| Validate | `vix check` |
| Test | `vix tests` |
| Format | `vix fmt` |
| Build | `vix build` |
| Package | `vix pack` |
| Verify artifact | `vix verify` |
| Cache artifact | `vix cache` |

## New project workflow

```bash
vix new api
cd api
vix install
vix dev
```

Validate before committing:

```bash
vix fmt --check
vix check --tests
```

Build for release:

```bash
vix build --preset release
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

## Choosing the right command

| Goal | Use |
|------|-----|
| Explore interactively | `vix` |
| Create a project | `vix new` |
| Generate a class or file | `vix make` |
| Run during development | `vix dev` |
| Run manually | `vix run` |
| Compile only | `vix build` |
| Validate deeply | `vix check` |
| Run tests only | `vix tests` |
| Format source files | `vix fmt` |
| Add dependencies | `vix add` |
| Install dependencies | `vix install` |
| Update dependencies | `vix update` |
| Package project | `vix pack` |
| Verify package | `vix verify` |
| Inspect environment | `vix info` |
| Diagnose setup | `vix doctor` |

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

For a fresh or cloned project, run `vix install` before `vix dev`.

## Recommended daily workflow

```bash
# Development
vix dev

# In another terminal
vix tests --watch

# Before committing
vix fmt --check
vix check --tests

# Before release
vix build --preset release
vix tests --preset release
vix check --san --full
vix pack --name api --version 1.0.0
vix verify --path ./dist/api@1.0.0
```

## Version

```bash
vix --version
```

Example output:
