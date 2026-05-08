# Vix CLI

The Vix CLI is the command-line interface for Vix.cpp.

It gives C++ a modern runtime-like development experience. With one binary, you can create projects, run C++ files, build applications, manage dependencies, validate code, package artifacts, and inspect your local environment.

## What the CLI gives you

The `vix` command is designed to make C++ development faster and more direct.

It provides:

- an interactive REPL
- project creation
- C++ file generation
- script-like `.cpp` execution
- CMake-based project builds
- development mode with reload
- checks, tests, and sanitizers
- dependency management
- packaging and verification
- registry and local store tools
- environment diagnostics

## Start in seconds

Create a new application:

```bash
vix new api
cd api
vix install
vix dev
```

This creates a ready-to-run Vix project, installs dependencies, and starts the app in development mode.

## Default interactive mode

Running `vix` without a command starts the interactive REPL:

```bash
vix
```

The REPL lets you evaluate expressions, create variables, work with JSON, inspect the environment, and run Vix commands from an interactive shell.

You can also start it explicitly:

```bash
vix repl
```

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
```

## Command groups

The CLI is organized around a few major workflows.

### Project commands

| Command | Purpose |
|---------|---------|
| `vix new` | Create a new Vix project |
| `vix make` | Generate C++ files quickly |
| `vix dev` | Run an app in development mode with reload |
| `vix run` | Build and run a project, file, or manifest |
| `vix build` | Configure and build a CMake project |
| `vix check` | Validate a project or single C++ file |
| `vix tests` | Run project tests |
| `vix fmt` | Format C++ source files |
| `vix clean` | Remove local project cache directories |
| `vix reset` | Clean and reinstall the project |
| `vix task` | Run reusable project tasks |

### Dependency commands

| Command | Purpose |
|---------|---------|
| `vix add` | Add a package to the project |
| `vix install` | Install dependencies from `vix.lock` |
| `vix update` | Update dependencies |
| `vix outdated` | Check outdated dependencies |
| `vix remove` | Remove a dependency |
| `vix list` | List project or global dependencies |

Aliases:

| Alias | Equivalent command |
|-------|--------------------|
| `vix up` | `vix update` |
| `vix i` | `vix install` |
| `vix deps` | `vix install` |

### Packaging commands

| Command | Purpose |
|---------|---------|
| `vix pack` | Package a project into a distributable artifact |
| `vix verify` | Verify a package artifact |
| `vix cache` | Store a verified package locally |

### Advanced commands

| Command | Purpose |
|---------|---------|
| `vix registry` | Manage the local registry index |
| `vix store` | Manage the local package store |
| `vix orm` | Run database migration tooling |
| `vix p2p` | Run a peer-to-peer node |

### System commands

| Command | Purpose |
|---------|---------|
| `vix info` | Show Vix paths, caches, and local state |
| `vix doctor` | Check the local environment |
| `vix upgrade` | Upgrade Vix or a global package |
| `vix uninstall` | Remove Vix or a global package |
| `vix completion` | Generate shell completion scripts |
| `vix version` | Show the installed Vix version |

## Global options

Most commands support global logging and help options.

```bash
vix --help
vix --version
vix --verbose
vix --quiet
vix --log-level debug
```

| Option | Purpose |
|--------|---------|
| `-h, --help` | Show help |
| `-v, --version` | Show version |
| `--verbose` | Enable debug logs |
| `-q, --quiet` | Show only warnings and errors |
| `--log-level <level>` | Set log level |

Supported log levels: `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`

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

> Use `--run` for runtime arguments. Do not pass runtime arguments after `--`, because everything after `--` is forwarded to the compiler or linker in script mode.

### Build a project

```bash
vix build
```

### Build with SQLite support

```bash
vix build --with-sqlite
```

### Validate with sanitizers

```bash
vix check --san
```

For a single file:

```bash
vix check main.cpp --san
```

### Run tests

```bash
vix tests
```

### Format code

```bash
vix fmt
```

### Package a project

```bash
vix pack --name api --version 1.0.0
```

### Verify a package

```bash
vix verify --path ./dist/api@1.0.0
```

## When to use each command

- Use `vix` when you want an interactive shell.
- Use `vix new` when starting a new project.
- Use `vix make` when you want to generate C++ files such as classes, structs, enums, functions, tests, or config files.
- Use `vix dev` when you are actively developing and want automatic rebuilds.
- Use `vix run` when you want to build and run a project, a single `.cpp` file, or a `.vix` manifest.
- Use `vix build` when you only want to configure and compile.
- Use `vix check` when you want validation, tests, runtime checks, or sanitizer checks.
- Use `vix tests` when you only want to run tests.
- Use `vix fmt` before committing source code.
- Use `vix task` when your project defines reusable workflows in `vix.json`.
- Use `vix pack`, `vix verify`, and `vix cache` when preparing artifacts for sharing or deployment.
- Use `vix doctor` when something does not work and you want to inspect your environment.

## CLI design philosophy

The Vix CLI exists to make C++ feel direct without hiding what C++ is.

It does not replace CMake, compilers, linkers, or native tooling. Instead, it gives them a cleaner workflow:

1. write code
2. run it
3. check it
4. package it
5. ship it

Vix keeps C++ native, but makes the development loop faster and easier to understand.

## Next step

Continue with the [REPL guide](#).
