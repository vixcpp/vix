# Application Model

A Vix.cpp application is a native C++ project with a defined entry point, project description, runtime workflow, build workflow, and operational model.

Vix.cpp does not treat an application as only a collection of `.cpp` files. It treats it as a software unit that can be created, configured, built, run, tested, packaged, diagnosed, and deployed through a consistent workflow.

The application model can be summarized like this:

```txt
source code
  -> application description
  -> modules and dependencies
  -> configuration
  -> runtime workflow
  -> build output
  -> production workflow
```

This model is what allows a Vix.cpp project to move from a small local application to a production service without changing the basic way developers operate it.

## The application starts with intent

When a developer creates a project, the first question should not be only:

```txt
Where is the CMakeLists.txt?
```

The better first question is:

```txt
What kind of application is this?
```

A backend service, a CLI tool, a game, a reusable library, and a P2P node do not need the same starting structure.

Vix.cpp uses templates to give the project an initial shape:

```bash
vix new api --template backend
```

```bash
vix new tool --app
```

```bash
vix new mathlib --lib
```

The template does not finish the architecture for you. It gives the project a correct starting point so the application can grow from a clear foundation.

## The application root

The application root is the directory where Vix.cpp resolves the project.

A backend project may look like this:

```txt
api/
├── vix.app
├── vix.json
├── vix.lock
├── .env.example
├── production.env.required
├── src/
├── tests/
├── migrations/
├── public/
└── data/
```

From this directory, the main commands should feel natural:

```bash
vix dev
vix run
vix build
vix check --tests
vix deploy
```

The root matters because Vix.cpp reads project files from there and writes local state there.

Common project-local state includes:

```txt
.vix/
build/
build-ninja/
build-release/
dist/
```

These directories are generated state. They are not the source model of the application.

## Project inputs

Vix.cpp supports two main project inputs:

```txt
CMakeLists.txt
vix.app
```

The resolution order is:

```txt
1. CMakeLists.txt
2. vix.app
```

If `CMakeLists.txt` exists, Vix.cpp uses the existing CMake project.

If there is no `CMakeLists.txt` and `vix.app` exists, Vix.cpp uses the `vix.app` manifest.

This rule protects existing CMake projects while giving new applications a simpler path.

## `vix.app`

`vix.app` is the simple application manifest for Vix.cpp.

It describes what the application is and what needs to be built.

Example:

```txt
name = api
type = executable
standard = c++23

sources = [
  src/main.cpp,
  src/app/AppFactory.cpp,
  src/routes/HealthRoutes.cpp,
]

include_dirs = [
  src,
]

modules = [
  core,
  json,
  http,
]
```

This file answers basic build questions:

```txt
What is the application called?
What kind of target is it?
Which C++ standard does it use?
Which source files are compiled?
Which include directories are used?
Which Vix.cpp modules are required?
```

For many applications, this is enough to start.

Vix.cpp can generate the internal CMake project from this description.

The generated file lives under:

```txt
.vix/generated/app/CMakeLists.txt
```

Developers should edit `vix.app`, not the generated CMake file.

## Why `vix.app` matters

Many C++ applications do not need advanced CMake logic at the beginning.

A developer should be able to describe a normal application with a small manifest instead of starting from a full build system definition.

The difference is practical.

Instead of beginning with:

```txt
cmake_minimum_required(...)
project(...)
add_executable(...)
target_include_directories(...)
target_link_libraries(...)
```

a simple application can begin with:

```txt
name = api
type = executable
sources = [...]
modules = [...]
```

CMake is still available when needed. But it does not have to be the first wall every application developer hits.

## When to use CMake directly

Use `CMakeLists.txt` when the build itself needs advanced control.

Examples include:

```txt
custom targets
complex native dependencies
manual install rules
platform-specific build logic
advanced linking
custom code generation
large multi-target projects
exported CMake packages
toolchain-specific behavior
```

When `CMakeLists.txt` exists, Vix.cpp respects it.

The model is:

```txt
vix.app = simple application manifest
CMakeLists.txt = advanced build definition
Vix.cpp = one workflow over both
```

This is important for experienced C++ developers. Vix.cpp is not a closed ecosystem. It works with the native C++ build ecosystem.

## `vix.json`

`vix.app` describes the application build model.

`vix.json` describes the project workflow model.

It can contain project metadata, dependency information, tasks, registry configuration, and production workflow settings.

Example:

```json
{
  "name": "api",
  "version": "0.1.0",
  "type": "application",
  "deps": [],
  "tasks": {
    "dev": "vix dev",
    "build": "vix build",
    "test": "vix check --tests",
    "fmt": "vix fmt"
  }
}
```

For production, `vix.json` can also describe service and health behavior:

```json
{
  "production": {
    "service": {
      "name": "api",
      "user": "vix",
      "working_dir": "/home/vix/apps/api",
      "command": "vix run",
      "env_file": "/home/vix/apps/api/.env"
    },
    "health": {
      "local": "http://127.0.0.1:8080/health",
      "public": "https://api.example.com/health"
    }
  }
}
```

Keep the distinction clear:

```txt
vix.app  = application build model
vix.json = project workflow model
vix.lock = exact dependency model
```

## `vix.lock`

`vix.lock` records the exact dependency state of the project.

If `vix.json` describes what the project needs, `vix.lock` records what was actually resolved.

After cloning a project, use:

```bash
vix install
```

This installs the locked dependency versions.

Do not use update when the goal is reproducibility.

```txt
vix install = reproduce the locked state
vix update  = change the locked state
```

This distinction matters for teams, CI, release builds, and production systems.

## Source layout

A serious backend should not keep all logic inside `main.cpp`.

A maintainable backend can use a layout like this:

```txt
src/
├── main.cpp
├── app/
│   ├── AppFactory.hpp
│   └── AppFactory.cpp
├── config/
│   ├── Config.hpp
│   └── Config.cpp
├── routes/
│   ├── HealthRoutes.hpp
│   └── HealthRoutes.cpp
├── middleware/
├── validation/
├── services/
├── database/
└── errors/
```

The rule is simple:

```txt
main.cpp should start the application, not contain the whole application.
```

## Small `main.cpp`

A clean entry point can look like this:

```cpp
#include <vix.hpp>

#include "app/AppFactory.hpp"
#include "config/Config.hpp"

int main()
{
  api::Config config = api::Config::load(".env");

  vix::App app = api::create_app(config);

  app.run(config.server_port());

  return 0;
}
```

This keeps the responsibilities separate.

Configuration is loaded by the config layer.

The application is created by the app factory.

Routes are registered elsewhere.

`main.cpp` remains readable.

## Application factory

An application factory creates and configures the Vix app.

Header:

```cpp
#pragma once

#include <vix.hpp>

#include "config/Config.hpp"

namespace api
{
  vix::App create_app(const Config &config);
}
```

Implementation:

```cpp
#include "app/AppFactory.hpp"

#include "routes/HealthRoutes.hpp"

namespace api
{
  vix::App create_app(const Config &config)
  {
    vix::App app;

    register_health_routes(app, config);

    return app;
  }
}
```

This makes application setup easier to test and easier to extend.

## Routes

Routes should be grouped by feature.

Example:

```txt
routes/
├── HealthRoutes.hpp
├── AuthRoutes.hpp
├── UserRoutes.hpp
└── AdminRoutes.hpp
```

A health route should exist early:

```txt
GET /health
```

Example response:

```json
{
  "ok": true,
  "service": "api",
  "status": "healthy"
}
```

This route is useful locally, but it also matters in production:

```bash
vix health local
vix health public
vix deploy
```

A production workflow should be able to verify that the application is alive.

## API response shape

A backend should use a predictable response shape.

Success:

```json
{
  "ok": true,
  "data": {}
}
```

List:

```json
{
  "ok": true,
  "count": 3,
  "data": []
}
```

Error:

```json
{
  "ok": false,
  "error": "validation_failed",
  "message": "email is required"
}
```

A stable response shape helps clients, tests, logs, API documentation, and language models that need to understand examples from the documentation.

## Configuration

Configuration should come from the environment.

A backend template should include:

```txt
.env.example
production.env.required
```

Example `.env.example`:

```dotenv
APP_ENV=development

SERVER_HOST=127.0.0.1
SERVER_PORT=8080
SERVER_TLS_ENABLED=false

VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
VIX_COLOR=auto

DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=./data/app.db

JWT_SECRET=change-me
SESSION_SECRET=change-me
```

Check local configuration:

```bash
vix env check
```

Check production configuration:

```bash
vix env check --production
```

The application should not hardcode secrets, production paths, hostnames, or deployment-specific values.

## Modules

A Vix.cpp application is composed from modules.

Example:

```txt
modules = [
  core,
  json,
  http,
  validation,
  middleware,
  db,
  log,
]
```

Modules make the application explicit.

If the app uses JSON, declare JSON.

If the app uses HTTP, declare HTTP.

If the app uses database support, declare database support.

A good application model should avoid hidden guessing.

## Dependencies

Registry packages are different from built-in modules.

Use registry packages when the project needs reusable external packages.

Typical workflow:

```bash
vix registry sync
vix search json
vix add softadastra/json
vix install
```

After adding a dependency, project state can change in:

```txt
vix.json
vix.lock
.vix/deps/
.vix/vix_deps.cmake
```

After cloning the project, the expected command is:

```bash
vix install
```

That should reproduce the locked dependency state.

## Tasks

Tasks belong in `vix.json`.

Example:

```json
{
  "tasks": {
    "dev": "vix dev",
    "build": "vix build",
    "test": "vix check --tests",
    "fmt": "vix fmt",
    "release": "vix build --preset release && vix check --tests"
  }
}
```

Tasks are useful for repeated workflows.

They should not hide important behavior behind unclear names.

Good task names describe what the command does.

## Tests

A real application should include tests.

Basic structure:

```txt
tests/
├── test_health.cpp
├── test_validation.cpp
└── test_auth.cpp
```

Generate a test skeleton:

```bash
vix make test HealthRoutes
```

Run tests:

```bash
vix tests
```

Or run broader validation:

```bash
vix check --tests
```

The application model is incomplete without tests.

## Static files

Some applications serve public files.

Example:

```txt
public/
├── favicon.ico
├── robots.txt
└── assets/
```

Keep served files outside `src/`.

```txt
src/    = C++ source code
public/ = served public content
```

This keeps the layout understandable.

## Database files

For SQLite applications, keep database files outside source directories.

Example:

```txt
data/
└── app.db
```

Migrations belong in a dedicated folder:

```txt
migrations/
├── 2026_01_01_000001_create_users.up.sql
└── 2026_01_01_000001_create_users.down.sql
```

Database commands may include:

```bash
vix db status
vix db migrate
vix db backup
```

For ORM workflows:

```bash
vix orm status --db api --dir ./migrations
vix orm migrate --db api --dir ./migrations
```

## Generated files

Use `vix make` to generate files inside an existing project.

Examples:

```bash
vix make class User --in src/domain --namespace api::domain
vix make struct Claims --in src/auth --namespace api::auth
vix make enum Status --in src/domain
vix make function parse_token --in src/auth
vix make test AuthService
vix make config app --websocket --database
```

The model is:

```txt
vix new  = create a project
vix make = add files to a project
```

Do not confuse project creation with file generation.

## Runtime arguments

Application arguments are separated from Vix arguments with `--`.

Example:

```bash
vix run -- --port 8080
```

The first part belongs to Vix.cpp.

The second part belongs to the application.

```txt
vix run [vix options] -- [application arguments]
```

This distinction matters in scripts, CI, and replay workflows.

## Replay

If a run is important to reproduce, record it.

```bash
vix run --replay
```

Replay the latest run:

```bash
vix replay last
```

Replay the latest failed run:

```bash
vix replay failed
```

The application model includes replay because real applications fail.

When they fail, the exact run context matters.

## Development mode

Development mode watches files and rebuilds intelligently.

```bash
vix dev
```

Typical behavior:

```txt
source change -> rebuild
header change -> rebuild
config change -> reconfigure and rebuild
ignored path -> do nothing
```

Ignored paths include:

```txt
.git
.vix
build
build-dev
build-ninja
build-release
node_modules
.cache
.idea
.vscode
```

`vix dev` is for active development.

It is not the production process.

## Build output

Build output should not pollute the source model.

Common build folders:

```txt
build/
build-ninja/
build-release/
```

Generated Vix state:

```txt
.vix/
```

Package output:

```txt
dist/
```

Clean local project state:

```bash
vix clean
```

Reset local project state and reinstall dependencies:

```bash
vix reset
```

## Packaging

A project can be packed when it is ready to be distributed.

```bash
vix pack
```

Verify the package:

```bash
vix verify
```

Cache a verified package:

```bash
vix cache --path ./dist/api@1.0.0
```

For application developers, packaging may not be the first concern.

For libraries, SDKs, release artifacts, and distribution workflows, it becomes important.

## Production configuration

A production-ready application should have production configuration in `vix.json`.

Important sections include:

```txt
production.service
production.proxy
production.health
production.logs
production.deploy
```

These sections allow Vix.cpp to run commands such as:

```bash
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

Production should not be a separate manual world.

It should be part of the application model.

## Local vs production

Local development:

```bash
vix dev
```

Production service management:

```bash
vix service init
vix service status
```

Local health check:

```bash
vix health local
```

Public health check:

```bash
vix health public
```

Deployment:

```bash
vix deploy
```

The same application moves through different workflows.

The application model stays consistent.

## A complete backend template

A serious backend template should look like this:

```txt
api/
├── vix.app
├── vix.json
├── vix.lock
├── .env.example
├── production.env.required
├── README.md
├── src/
│   ├── main.cpp
│   ├── app/
│   ├── config/
│   ├── routes/
│   ├── middleware/
│   ├── validation/
│   ├── database/
│   ├── services/
│   └── errors/
├── migrations/
├── tests/
├── public/
└── data/
```

This is the kind of application Vix.cpp should make easy to create:

```bash
vix new api --template backend
```

## Application checklist

A Vix.cpp application should be able to answer these questions:

```txt
What is the application name?
What type of target is it?
What source files are compiled?
What modules are used?
What dependencies are installed?
What environment variables are required?
How is the application run locally?
How is it built?
How is it tested?
How is it checked?
How is it deployed?
How are logs read?
How is health checked?
```

If the project cannot answer these questions, the application model is incomplete.

These questions also make the project easier for humans, tools, CI systems, and language models to understand.

## What you should remember

A Vix.cpp application is not only source code.

It is:

```txt
source files
  -> application description
  -> modules
  -> dependencies
  -> runtime workflow
  -> build workflow
  -> tests
  -> production workflow
```

Use:

```bash
vix new api --template backend
```

for a real backend starting point.

Use `vix.app` for the simple application manifest.

Use `vix.json` for project workflow, dependencies, tasks, registry metadata, and production configuration.

Use:

```bash
vix install
```

to reproduce dependencies.

Use:

```bash
vix dev
```

to develop.

Use:

```bash
vix deploy
```

when the application is ready for production.

## Next chapter

[Next: Runtime Workflow](/book/05-runtime-workflow)
