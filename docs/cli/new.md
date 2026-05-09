# vix new

`vix new` creates a new Vix project.

Use it when you want to start a new application or initialize a Vix project in an existing folder.

## Usage

```bash
vix new <name|path> [options]
```

## Examples

Create a new project named `api`:

```bash
vix new api
```

Create a project in the current directory:

```bash
vix new .
```

Create a header-only library:

```bash
vix new tree --lib
```

Create a project inside another directory:

```bash
vix new blog -d ./projects
```

Overwrite an existing directory:

```bash
vix new api --force
```

## What it creates

For an application, `vix new` generates a ready-to-run Vix project with:

- a CMake project
- source structure
- config files
- a `vix.json` manifest
- default project tasks
- an empty dependency list

For a library, it generates package metadata and an empty dependency list.

## Basic workflow

```bash
vix new api
cd api
vix install
vix dev
```

## Application project

By default, `vix new` creates an application:

```bash
vix new api
```

This is equivalent to:

```bash
vix new api --app
```

Use this when you want to build an executable application, API server, backend service, tool, or demo.

## Library project

Use `--lib` when you want to create a header-only library package:

```bash
vix new tree --lib
```

This is useful when you want to create reusable C++ code that can later be packaged and shared.

## Initialize the current directory

Use `.` to create the project in the current folder:

```bash
vix new .
```

This is useful when you already created the repository manually.

Example:

```bash
mkdir api
cd api
git init
vix new .
```

## Create inside another directory

Use `-d` or `--dir` to choose the base directory where the project should be created:

```bash
vix new blog -d ./projects
```

This creates `./projects/blog`.

## Overwrite an existing directory

By default, Vix avoids overwriting existing project files.

Use `--force` only when you intentionally want to overwrite an existing directory:

```bash
vix new api --force
```

Use this carefully, because existing files may be replaced.

## Options

| Option | Description |
|--------|-------------|
| `--app` | Generate an application project. This is the default. |
| `--lib` | Generate a header-only library project. |
| `-d, --dir <path>` | Base directory for project creation. |
| `--force` | Overwrite an existing directory. |
| `-h, --help` | Show command help. |

## Environment variables

| Variable | Description |
|----------|-------------|
| `VIX_NONINTERACTIVE=1` | Disable interactive prompts. |
| `CI=1` | Disable interactive prompts in CI environments. |

Use non-interactive mode in scripts and CI pipelines:

```bash
VIX_NONINTERACTIVE=1 vix new api
```

## Generated manifest

A new project includes a `vix.json` manifest. The manifest describes the project, its type, its dependencies, and its reusable tasks.

A typical application manifest may include:

```json
{
  "name": "api",
  "version": "0.1.0",
  "type": "app",
  "dependencies": {},
  "tasks": {
    "dev": "vix dev",
    "run": "vix run",
    "build": "vix build",
    "check": "vix check --tests",
    "fmt": "vix fmt"
  }
}
```

The exact generated content may evolve with Vix versions, but the role stays the same: `vix.json` is the project manifest.

## After project creation

After creating a project, the usual next commands are:

```bash
cd api
vix install
vix dev
```

- Use `vix install` to install dependencies from the lockfile.
- Use `vix dev` to run the application in development mode.
- Use `vix run` when you want to build and run manually.
- Use `vix build` when you only want to compile.
- Use `vix check` when you want to validate the project.

## Common mistakes

### Running commands outside the project

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

After creating a project, enter the generated directory before running project commands.

### Using `--force` too early

Avoid this unless you really want to overwrite files:

```bash
vix new api --force
```

Prefer creating a clean folder first.

### Creating a library when you need an app

This creates a library:

```bash
vix new api --lib
```

For a backend service or executable app, use the default:

```bash
vix new api
```

## When to use `vix new`

Use `vix new` when:

- starting a new Vix application
- creating a reusable Vix library package
- initializing an existing empty repository
- preparing a project for dependency management
- creating a project that should work with `vix dev`, `vix run`, `vix build`, and `vix check`

## Related commands

| Command | Purpose |
|---------|---------|
| `vix install` | Install project dependencies |
| `vix dev` | Run the app in development mode |
| `vix run` | Build and run the project |
| `vix build` | Build the project |
| `vix check` | Validate the project |
| `vix task` | Run generated or custom project tasks |
| `vix add` | Add dependencies |

## Next step

Continue with file generation.

[Open the vix make guide](/cli/make)
