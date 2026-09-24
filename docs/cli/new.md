# vix new

`vix new` creates a new Vix project.

Use it when you want to start an application, backend service, web project, Vue fullstack project, game project, or reusable C++ library.

```bash
vix new api
```

## Overview

`vix new` is the project creation command in Vix.

It creates a ready-to-use project with:

- source files
- project configuration
- Vix metadata
- generated tasks
- build setup
- runtime configuration when needed
- template-specific structure

It is the recommended way to start a Vix project because it creates a structure that works immediately with:

```bash
vix build
vix run
vix dev
vix tests
vix check
vix task
```

## Usage

```bash
vix new <name|path> [options]
```

## Basic examples

```bash
# Create a default application
vix new api

# Create a project in the current directory
vix new .

# Create a backend service
vix new api --template backend

# Create a server-rendered web project
vix new blog --template web

# Create a Vue fullstack project
vix new dashboard --template vue

# Create a game project
vix new mario --game

# Create a game project with template syntax
vix new platformer --template game

# Create a header-only library
vix new tree --lib

# Create inside another directory
vix new blog -d ./projects

# Overwrite an existing directory
vix new api --force
```

## What `vix new` can create

| Type        | Command                            | Use when                                                |
| ----------- | ---------------------------------- | ------------------------------------------------------- |
| Application | `vix new api`                      | You want a small runnable C++ app.                      |
| Application | `vix new api --app`                | You want to be explicit about the default app template. |
| Backend     | `vix new api --template backend`   | You want a production-oriented API or backend service.  |
| Web         | `vix new blog --template web`      | You want server-rendered HTML with Vix.                 |
| Vue         | `vix new dashboard --template vue` | You want a Vue frontend with a Vix C++ backend.         |
| Game        | `vix new mario --game`             | You want a Vix game project.                            |
| Game        | `vix new mario --template game`    | Same idea using template syntax.                        |
| Library     | `vix new tree --lib`               | You want a reusable header-only C++ library.            |

## Default behavior

By default, `vix new` creates an application project.

```bash
vix new api
```

This is equivalent to:

```bash
vix new api --app
```

Use the default application template when you want:

- a small executable app
- a simple HTTP app
- a local tool
- a demo
- a prototype
- a project that can grow later

## Application template

Create an application:

```bash
vix new hello
```

or:

```bash
vix new hello --app
```

Enter the project:

```bash
cd hello
```

Build it:

```bash
vix build
```

Run it:

```bash
vix run
```

Start development mode:

```bash
vix dev
```

The application template is the smallest real Vix project.

It is useful when you want to start clean without a large backend, web, Vue, game, or library structure.

## Backend template

Create a backend project:

```bash
vix new api --template backend
```

Enter the project:

```bash
cd api
```

Create your local environment file:

```bash
cp .env.example .env
```

Start development mode:

```bash
vix dev
```

Test the health endpoint:

```bash
curl http://127.0.0.1:8080/health
```

Use the backend template when you need:

- API routes
- production-oriented folders
- config files
- health endpoints
- middleware-ready structure
- database-ready structure
- a stronger backend foundation than the default app template

## Web template

Create a web project:

```bash
vix new blog --template web
```

Enter the project:

```bash
cd blog
```

Create your local environment file:

```bash
cp .env.example .env
```

Start development mode:

```bash
vix dev
```

Open:

```txt
http://127.0.0.1:8080
```

Use the web template when you want:

- server-rendered HTML
- static assets
- Vix templates
- a simple web app structure
- an app that does not need a separate frontend framework

## Vue template

Create a Vue fullstack project:

```bash
vix new dashboard --template vue
```

Enter the project:

```bash
cd dashboard
```

Install dependencies when needed:

```bash
vix install
```

Start development mode:

```bash
vix dev
```

Use the Vue template when you want:

- a Vue frontend
- a Vix C++ backend
- one fullstack project
- frontend and backend development together

In dev mode, Vix can detect the Vue frontend and start the frontend dev server when the project contains the expected frontend structure.

## Game template

Create a game project:

```bash
vix new mario --game
```

or:

```bash
vix new mario --template game
```

Enter the project:

```bash
cd mario
```

Build it:

```bash
vix build
```

Run it:

```bash
vix run
```

Use the game template when you want:

- a game-oriented project structure
- a simulation
- a real-time prototype
- a custom engine foundation
- a project based on the Vix game runtime

## Library template

Create a header-only library:

```bash
vix new tree --lib
```

Enter the project:

```bash
cd tree
```

Build the generated project:

```bash
vix build --build-target all
```

Enable tests:

```bash
vix build --build-target all -- -Dtree_BUILD_TESTS=ON
```

Run tests:

```bash
vix tests
```

Use the library template when you want:

- reusable C++ code
- a header-only package
- an `include/` directory
- tests
- examples
- a future package for the Vix Registry

## Why libraries use `--build-target all`

`vix build` builds the main project target by default.

For an application named `api`, this works because the generated project creates an executable target named:

```txt
api
```

For a header-only library named `tree`, the project usually exposes an interface target and optional tests or examples.

So for generated libraries, use:

```bash
vix build --build-target all
```

This asks CMake and Ninja to build all enabled generated targets.

## Template selection

You can select a template with a flag:

```bash
vix new api --app
vix new tree --lib
vix new mario --game
```

You can also use `--template`:

```bash
vix new api --template backend
vix new blog --template web
vix new dashboard --template vue
vix new platformer --template game
```

Supported `--template` values:

```txt
backend
web
vue
game
```

## Interactive mode

When Vix can interact with the terminal and you do not provide a specific template, it can ask you to choose a template interactively.

This is useful when you run:

```bash
vix new myproject
```

and want to choose the project type from a menu.

In scripts or CI, use explicit flags or non-interactive mode.

## Non-interactive mode

Use `VIX_NONINTERACTIVE=1` to disable prompts:

```bash
VIX_NONINTERACTIVE=1 vix new api
```

Create a library without prompts:

```bash
VIX_NONINTERACTIVE=1 vix new tree --lib
```

CI also disables prompts:

```bash
CI=1 vix new api
```

Use non-interactive mode when running `vix new` from scripts, CI jobs, or automated setup commands.

## Create in the current directory

Use `.` to initialize the current directory:

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

If the current directory is not empty, Vix may ask for confirmation in interactive mode.

In non-interactive mode, use `--force` if you intentionally want to write template files into the current directory.

```bash
vix new . --force
```

## Create inside another directory

Use `-d` or `--dir` to choose the base directory:

```bash
vix new blog -d ./projects
```

This creates:

```txt
./projects/blog
```

Equivalent form:

```bash
vix new blog --dir ./projects
```

## Overwrite an existing directory

By default, Vix avoids overwriting existing project files.

Use `--force` only when you intentionally want to overwrite an existing directory:

```bash
vix new api --force
```

Use this carefully.

If the destination exists and is not empty, files may be replaced.

## Conflicting options

Choose only one project type.

Wrong:

```bash
vix new api --app --lib
```

Wrong:

```bash
vix new api --lib --template backend
```

Wrong:

```bash
vix new game --game --template vue
```

Correct:

```bash
vix new api --app
```

or:

```bash
vix new api --template backend
```

or:

```bash
vix new tree --lib
```

or:

```bash
vix new mario --game
```

## Generated application structure

A generated application has a structure similar to:

```txt
api/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── api.vix
├── vix.json
├── .env
├── .env.example
├── src/
│   └── main.cpp
└── tests/
    └── test_basic.cpp
```

The executable target matches the project name.

For example:

```bash
vix new api
cd api
vix build
```

This builds the `api` target.

## Generated backend structure

A generated backend project has a production-oriented structure.

It can include folders for routes, handlers, middleware, configuration, and application code.

The exact structure can evolve with Vix versions, but the purpose stays the same:

```txt
backend template
-> production-oriented C++ API foundation
```

Use it when the project is meant to become a real backend service.

## Generated web structure

A generated web project is designed for server-rendered web apps.

It can include:

```txt
src/
templates/
public/
.env
.env.example
vix.json
```

Use it when you want Vix to serve HTML, templates, and static files.

## Generated Vue structure

A generated Vue project is a fullstack project.

It can include:

```txt
backend C++ app
frontend/
frontend/package.json
vix.json
.env
.env.example
```

Use `vix dev` to run the development workflow.

## Generated game structure

A generated game project is designed for Vix game workflows.

It can include:

```txt
src/
assets/
CMakeLists.txt
CMakePresets.json
vix.json
README.md
```

Use:

```bash
vix build
vix run
```

to build and start the game.

## Generated library structure

A generated header-only library has a structure similar to:

```txt
tree/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── tree.vix
├── vix.json
├── include/
│   └── tree/
│       └── tree.hpp
├── examples/
│   ├── CMakeLists.txt
│   └── basic.cpp
└── tests/
    └── test_basic.cpp
```

Build it with:

```bash
vix build --build-target all
```

Enable tests with:

```bash
vix build --build-target all -- -Dtree_BUILD_TESTS=ON
```

Run tests:

```bash
vix tests
```

## Generated manifest files

A new project can include:

```txt
vix.json
<project>.vix
vix.app
```

The exact files depend on the template.

## `vix.json`

`vix.json` stores project metadata, dependencies, variables, and reusable tasks.

It is used by commands such as:

```bash
vix task
vix add
vix install
vix check
```

## `<project>.vix`

Some templates generate a `.vix` manifest file.

It can be used with commands such as:

```bash
vix run app.vix
vix dev app.vix
```

## `vix.app`

Some templates use the `vix.app` workflow.

`vix.app` describes a simple Vix application target.

It can be used by:

```bash
vix build
vix run
vix dev
```

without requiring the user to manually write a `CMakeLists.txt` for simple apps.

## Generated tasks

Generated projects can include reusable tasks in `vix.json`.

Examples:

```bash
vix task dev
vix task test
vix task ci
```

Tasks make project workflows repeatable.

The exact generated tasks depend on the template.

## Feature selection

For some templates, interactive mode can ask which features you want.

Feature selection can apply to application, backend, or Vue projects.

Examples of feature categories can include:

- database support
- static runtime options
- full static options
- backend-related project features

The exact available features can evolve with Vix versions.

In non-interactive mode, choose the template explicitly and configure features later.

## Application workflow

Create:

```bash
vix new api
```

Enter:

```bash
cd api
```

Build:

```bash
vix build
```

Run:

```bash
vix run
```

Develop:

```bash
vix dev
```

## Backend workflow

Create:

```bash
vix new api --template backend
```

Enter:

```bash
cd api
```

Create local env:

```bash
cp .env.example .env
```

Start dev mode:

```bash
vix dev
```

Test health:

```bash
curl http://127.0.0.1:8080/health
```

## Web workflow

Create:

```bash
vix new blog --template web
```

Enter:

```bash
cd blog
```

Create local env:

```bash
cp .env.example .env
```

Start dev mode:

```bash
vix dev
```

Open:

```txt
http://127.0.0.1:8080
```

## Vue workflow

Create:

```bash
vix new dashboard --template vue
```

Enter:

```bash
cd dashboard
```

Install dependencies when needed:

```bash
vix install
```

Start dev mode:

```bash
vix dev
```

## Game workflow

Create:

```bash
vix new mario --game
```

Enter:

```bash
cd mario
```

Build:

```bash
vix build
```

Run:

```bash
vix run
```

## Library workflow

Create:

```bash
vix new tree --lib
```

Enter:

```bash
cd tree
```

Build all generated targets:

```bash
vix build --build-target all
```

Enable tests:

```bash
vix build --build-target all -- -Dtree_BUILD_TESTS=ON
```

Run tests:

```bash
vix tests
```

## After project creation

Common next commands:

```bash
cd <project>
vix build
vix run
vix dev
```

If the project has dependencies:

```bash
vix install
```

If you want validation:

```bash
vix check
```

If you want tests:

```bash
vix tests
```

If you want to run generated tasks:

```bash
vix task <name>
```

## Options

| Option               | Description                                                                  |
| -------------------- | ---------------------------------------------------------------------------- |
| `<name\|path>`       | Project name or destination path.                                            |
| `--app`              | Generate an application project. This is the default.                        |
| `--application`      | Alias-style application option.                                              |
| `--type=app`         | Generate an application project.                                             |
| `--type=application` | Generate an application project.                                             |
| `--lib`              | Generate a header-only library project.                                      |
| `--library`          | Alias-style library option.                                                  |
| `--type=lib`         | Generate a library project.                                                  |
| `--type=library`     | Generate a library project.                                                  |
| `--game`             | Generate a game project.                                                     |
| `--type=game`        | Generate a game project.                                                     |
| `--template <name>`  | Generate a template project: `backend`, `web`, `vue`, or `game`.             |
| `--template=<name>`  | Same as `--template <name>`.                                                 |
| `-d, --dir <path>`   | Base directory for project creation.                                         |
| `--dir=<path>`       | Same as `--dir <path>`.                                                      |
| `--force`            | Overwrite an existing directory or write into a non-empty current directory. |
| `-h, --help`         | Show command help.                                                           |

## Environment variables

| Variable               | Description                                     |
| ---------------------- | ----------------------------------------------- |
| `VIX_NONINTERACTIVE=1` | Disable interactive prompts.                    |
| `CI=1`                 | Disable interactive prompts in CI environments. |

Examples:

```bash
VIX_NONINTERACTIVE=1 vix new api
VIX_NONINTERACTIVE=1 vix new tree --lib
CI=1 vix new api --template backend
```

## Common mistakes

### Running commands outside the generated project

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

### Creating a library when you need an app

Wrong for a backend service:

```bash
vix new api --lib
```

Correct:

```bash
vix new api
```

or:

```bash
vix new api --template backend
```

### Using `vix build` directly in a header-only library

Wrong:

```bash
vix new tree --lib
cd tree
vix build
```

Correct:

```bash
vix new tree --lib
cd tree
vix build --build-target all
```

### Running library tests before enabling them

Wrong:

```bash
vix new tree --lib
cd tree
vix tests
```

Correct:

```bash
vix new tree --lib
cd tree
vix build --build-target all -- -Dtree_BUILD_TESTS=ON
vix tests
```

### Combining incompatible project types

Wrong:

```bash
vix new api --lib --template backend
```

Correct:

```bash
vix new api --template backend
```

### Using `--force` too early

Avoid this unless you really want to overwrite files:

```bash
vix new api --force
```

Prefer creating a clean folder first.

## Troubleshooting

### Missing project name

Wrong:

```bash
vix new --template backend
```

Correct:

```bash
vix new api --template backend
```

### Unknown template

Wrong:

```bash
vix new app --template desktop
```

Supported templates:

```txt
backend
web
vue
game
```

Correct:

```bash
vix new app --template backend
```

### Directory is not empty

If the target directory exists and is not empty, Vix will avoid overwriting by default.

Use a clean directory or pass:

```bash
vix new api --force
```

only when you intentionally want to overwrite.

### Base directory is invalid

Wrong:

```bash
vix new blog --dir ./missing-folder
```

Create the base directory first:

```bash
mkdir -p ./projects
vix new blog --dir ./projects
```

### Build target not found in a library

You are probably in a generated header-only library.

Use:

```bash
vix build --build-target all
```

### No tests available in a library

Enable tests first:

```bash
vix build --build-target all -- -Dtree_BUILD_TESTS=ON
```

Then run:

```bash
vix tests
```

Replace `tree` with your project name.

## When to use each template

Use the application template when you want the smallest runnable app.
Use the backend template when you want a production-oriented API or backend service.
Use the web template when you want server-rendered HTML.
Use the Vue template when you want a frontend and backend in one project.
Use the game template when you want a game, simulation, prototype, or custom engine foundation.
Use the library template when you want reusable C++ code that can later be packaged and shared.

## Related commands

| Command       | Purpose                             |
| ------------- | ----------------------------------- |
| `vix build`   | Build the generated project.        |
| `vix run`     | Build and run the generated app.    |
| `vix dev`     | Start development mode with reload. |
| `vix tests`   | Run tests.                          |
| `vix check`   | Validate the project.               |
| `vix install` | Install project dependencies.       |
| `vix add`     | Add dependencies.                   |
| `vix task`    | Run generated or custom tasks.      |

## Next step

Build the generated project.

[Open the vix build guide](/cli/build)
