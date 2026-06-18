# Application Template

The application template is the simplest Vix project template.
Use it when you want a small runnable C++ application without starting from a large backend, web, Vue, game, or library structure.

It is the best template for:

- learning Vix projects
- creating a small HTTP app
- testing ideas
- building a simple service
- starting from a clean `vix.app` workflow
- growing gradually before moving to a larger architecture

## Create an application project

Create a project:

```bash
vix new hello --app
```

You can also use:

```bash
vix new hello
```

When no template is selected and no interactive choice is made, Vix uses the application template by default.

## Quick start

Enter the project:

```bash
cd hello
```

Create your local configuration:

```bash
cp .env.example .env
```

Build the project:

```bash
vix build
```

Run it:

```bash
vix run
```

Open:

```txt
http://localhost:8080
```

## Generated structure

A generated application project looks like this:

```txt
hello/
├── src/
│   └── main.cpp
├── tests/
│   └── test_basic.cpp
├── .env
├── .env.example
├── README.md
├── vix.app
└── vix.json
```

The structure is intentionally small.

The application template is not trying to impose a backend architecture. It gives you the minimum project foundation needed to build, run, test, configure, and evolve a Vix application.

## What each file does

| File or folder | Purpose                                               |
| -------------- | ----------------------------------------------------- |
| `src/main.cpp` | Main application source file.                         |
| `tests/`       | Basic project tests.                                  |
| `.env.example` | Shared example configuration.                         |
| `.env`         | Local configuration for your machine.                 |
| `README.md`    | Generated project documentation.                      |
| `vix.app`      | Application build manifest.                           |
| `vix.json`     | Project metadata, tasks, variables, and dependencies. |

## `src/main.cpp`

This is the entry point of the application.

A minimal application can look like this:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res) {
    res.text("Hello from Vix.cpp");
  });

  app.run();

  return 0;
}
```

The important part is:

```cpp
app.run();
```

This keeps runtime configuration outside the code.

The port should come from `.env`:

```dotenv
SERVER_PORT=8080
```

For normal applications, prefer:

```cpp
app.run();
```

instead of:

```cpp
app.run(8080);
```

This lets the same source code run in different environments.

## `.env.example`

`.env.example` documents the expected configuration.

It is safe to commit.

A simple example:

```dotenv
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
```

When another developer clones the project, they can run:

```bash
cp .env.example .env
```

Then adjust local values in `.env`.

## `.env`

`.env` contains local configuration.

It is the file you edit on your machine.

Example:

```dotenv
SERVER_PORT=8080
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
```

To change the port:

```dotenv
SERVER_PORT=3000
```

Then run:

```bash
vix run
```

The source code does not need to change.

## `vix.app`

`vix.app` is the application manifest.

It describes what Vix should build.

For simple projects, you do not need to write a `CMakeLists.txt` manually.

A generated application manifest has this shape:

```txt
name = "hello"
type = "executable"
standard = "c++20"

sources = [
  "src/main.cpp",
]

include_dirs = [
  "src",
]

defines = [
]

modules = [
]

deps = [
]

packages = [
  "vix",
]

links = [
  "vix::vix",
]

resources = [
  ".env=.env",
]

output_dir = "bin"
```

Vix reads this file and generates an internal CMake project under:

```txt
.vix/generated/app/
```

The build flow is:

```txt
vix.app
  -> Vix generates internal CMake
  -> vix build compiles the app
  -> vix run starts the app
```

## `modules`

The `modules` field is for internal application modules.

These are modules created inside your own project with:

```bash
vix modules init
vix modules add auth
```

A module is stored under:

```txt
modules/auth/
```

Then you can declare it in `vix.app`:

```txt
modules = [
  "auth",
]
```

Vix maps it to a CMake alias target using your project name.

For example, in a project named `hello`:

```txt
"auth" -> hello::auth
```

Use `modules` for code that belongs to your application.

Examples:

- `auth`
- `users`
- `orders`
- `billing`
- `notifications`

Do not use `modules` for registry packages.

## `deps`

The `deps` field is for external dependencies from the Vix Registry.

Example:

```txt
deps = [
  "gk/json@^1.0.0",
]
```

The usual workflow is:

```bash
vix registry sync
vix add gk/json@^1.0.0
vix install
```

After installation, Vix writes dependency information into:

```txt
.vix/vix_deps.cmake
```

Then the generated CMake project can load those dependencies.

Use `deps` for packages that come from the Vix Registry.

Do not use `deps` for internal application modules.

## `packages`

The `packages` field is for CMake packages that must be found with `find_package`.

The generated application template includes:

```txt
packages = [
  "vix",
]
```

This tells the generated CMake project to resolve the Vix SDK package.

## `links`

The `links` field is for CMake targets linked into your application.

The generated application template includes:

```txt
links = [
  "vix::vix",
]
```

This links your executable with the Vix SDK target.

When you add internal modules, Vix can link their generated targets.

When you add registry dependencies, their exported targets can also be linked here if needed.

## Dependency model

Use the right field for the right kind of dependency:

| Field      | Purpose                                         | Example            |
| ---------- | ----------------------------------------------- | ------------------ |
| `modules`  | Internal app modules created with `vix modules` | `"auth"`           |
| `deps`     | External packages from the Vix Registry         | `"gk/json@^1.0.0"` |
| `packages` | CMake packages resolved with `find_package`     | `"vix"`            |
| `links`    | CMake targets linked to your app                | `"vix::vix"`       |

A clean application can start with:

```txt
modules = [
]

deps = [
]

packages = [
  "vix",
]

links = [
  "vix::vix",
]
```

Then you add structure only when the project needs it.

## Why this design exists

The application template keeps the project small but still production-aware.

It avoids two common problems:

1. Starting with only a loose `main.cpp` and no project structure.
2. Starting with a large architecture before you need one.

The application template gives you the middle path:

```txt
simple source file
+ real project manifest
+ local configuration
+ tests
+ tasks
+ native C++ build
```

You can start small, then grow only when the application needs more structure.

## `vix.json`

`vix.json` stores project metadata, variables, tasks, and dependency information.

Generated application projects include useful task definitions such as:

```json
{
  "name": "hello",
  "deps": [],
  "vars": {
    "preset": "dev-ninja",
    "release_preset": "release",
    "log_level": "info"
  },
  "tasks": {
    "fmt": "vix fmt",
    "dev": {
      "description": "Start dev mode",
      "command": "vix dev"
    },
    "test": {
      "description": "Run project tests",
      "command": "vix tests --preset ${preset} --fail-fast"
    },
    "ci": {
      "description": "Local CI pipeline",
      "commands": [
        "vix check --preset ${preset} --tests",
        "vix tests --preset ${preset} --fail-fast"
      ]
    }
  }
}
```

You can run tasks with:

```bash
vix task dev
vix task test
vix task ci
```

Use tasks when you want repeatable project commands.

## Registry dependencies

The application template can use registry dependencies when the project needs external packages.
The workflow is:

```bash
vix registry sync
vix add <namespace>/<package>
vix install
```

Example:

```bash
vix add gk/json@^1.0.0
vix install
```

This updates project dependency metadata and installs packages into the project Vix workspace.
A lock file can keep installs reproducible:

```txt
vix.lock
```

Use registry dependencies for external packages.
Use internal modules when the code belongs to your own application.

## Build the application

Run:

```bash
vix build
```

This compiles the application without starting it.

For a release build:

```bash
vix build --preset release
```

For a clean rebuild:

```bash
vix build --clean
```

## Run the application

Run:

```bash
vix run
```

`vix run` builds when needed, resolves the executable, prepares the runtime environment, then starts the application.

## Development mode

Use:

```bash
vix dev
```

Use development mode while editing code.

Use `vix run` when you simply want to start the app directly.

## Tests

Run tests with:

```bash
vix tests
```

Or through the generated task:

```bash
vix task test
```

The application template includes a basic test file so the project has a testing path from the beginning.

## Formatting and checks

Format the project:

```bash
vix fmt
```

Run checks:

```bash
vix check
```

Or use tasks:

```bash
vix task ci
```

## When to use the application template

Use the application template when your project is still small.

Good examples:

- small HTTP app
- simple internal tool
- prototype
- CLI-style executable
- learning project
- small service
- test app for a Vix module
- app that may later become a backend

## When to move to another template

Move to the backend template when you need:

- controllers
- route registries
- middleware registries
- static files
- health endpoints
- storage
- migrations
- production diagnostics
- a stronger folder architecture

Use:

```bash
vix new api --template backend
```

Move to the web template when you need server-rendered HTML:

```bash
vix new site --template web
```

Move to the Vue template when you need a frontend app with a Vix backend:

```bash
vix new dashboard --template vue
```

Move to the game template when you need a game-oriented structure:

```bash
vix new platformer --template game
```

Move to the library template when you want reusable C++ code:

```bash
vix new mathlib --lib
```

## Suggested evolution path

Start with:

```txt
src/main.cpp
```

When routes grow, split them into functions:

```txt
src/main.cpp
src/routes.cpp
src/routes.hpp
```

When the app becomes a real backend, move to:

```txt
vix new api --template backend
```

The application template is for starting clean.
The backend template is for long-term backend structure.

## Common mistakes

### Treating `--app` like the backend template

The application template is intentionally small.
It does not create:

```txt
controllers/
routes/
middleware/
domain/
infrastructure/
migrations/
```

Use the backend template when you need those folders.

### Hardcoding local configuration

Avoid this:

```cpp
app.run(8080);
```

Prefer:

```cpp
app.run();
```

And configure the port in `.env`:

```dotenv
SERVER_PORT=8080
```

### Editing `.env.example` instead of `.env`

Use `.env.example` as documentation.

Use `.env` for local values.

### Adding many files without updating the manifest

If you add new source files, make sure `vix.app` includes them.

Example:

```txt
sources = [
  "src/main.cpp",
  "src/routes.cpp",
]
```

## What you should remember

Create an application:

```bash
vix new hello --app
cd hello
cp .env.example .env
```

Build it:

```bash
vix build
```

Run it:

```bash
vix run
```

Develop it:

```bash
vix dev
```

The application template is the smallest real Vix project.
It is the right place to start before choosing a larger architecture.

## Next steps

Continue with the other templates:

- [Backend template](/templates/backend)
- [Web template](/templates/web)
- [Vue template](/templates/vue)
- [Game template](/templates/game)
- [Library template](/templates/library)
