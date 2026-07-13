# Modules and Composition

A Vix application grows by composition.

You start with a small app.

Then you add what the app needs:

```txt id="p41m7z"
core
json
http
validation
middleware
database
websocket
p2p
sync
cache
crypto
```

The important rule is simple:

```txt id="l12k6y"
the application should declare what it uses
```

No hidden guessing.

No unclear magic.

A module should be visible in the project model.

## The base idea

A Vix app is not one giant block.

It is composed from smaller capabilities.

The model:

```txt id="f7x0t3"
application
  -> modules
  -> dependencies
  -> configuration
  -> runtime
```

A backend can use HTTP.

A real-time app can use WebSocket.

A database app can use SQLite or MySQL.

A distributed app can use P2P.

The application stays one app, but its capabilities are composed.

## What a module means

A module is a reusable capability.

Examples:

| Module       | Role                                       |
| ------------ | ------------------------------------------ |
| `core`       | base types, runtime primitives, utilities  |
| `json`       | JSON parsing, serialization, values        |
| `http`       | HTTP server, routes, request, response     |
| `validation` | request and data validation                |
| `middleware` | request pipeline behavior                  |
| `db`         | database access and migrations             |
| `log`        | logging and structured output              |
| `websocket`  | real-time connections                      |
| `p2p`        | peer-to-peer networking                    |
| `sync`       | offline-first synchronization              |
| `cache`      | memory, file, HTTP, or build-related cache |
| `crypto`     | hashing, signatures, encryption helpers    |

A module should have a clear job.

If it does too many unrelated things, composition becomes hard.

## Application modules in `vix.app`

For simple apps, modules should be declared in `vix.app`.

Example:

```txt id="d09w9m"
name = "api"
type = "executable"
cpp_standard = "23"

sources = [
  "src/main.cpp",
  "src/app/AppFactory.cpp",
  "src/routes/HealthRoutes.cpp"
]

include_dirs = [
  "src"
]

modules = [
  "core",
  "json",
  "http",
  "validation",
  "middleware",
  "db",
  "log"
]
```

This tells Vix what the application needs.

The build workflow can then wire the right targets.

The runtime workflow can then run an app that was built with the correct capabilities.

### Generated application modules

`vix modules` can create internal application modules under `modules/`.

```bash
vix modules init
vix modules add auth
vix modules add live_chat --websocket --workflow attached
```

The module name is yours. `live_chat`, `notifications`, `presence`, and `events` are all valid names. You can also use the explicit option form:

```bash
vix modules add --websocket --name live_chat
```

A WebSocket module named `live_chat` generates:

```txt
modules/live_chat/
  include/live_chat/LiveChatModule.hpp
  src/LiveChatModule.cpp
  vix.module
```

The module manifest records whether the module is a runtime workflow:

```ini
workflow = "websocket.attached"
runtime = true
```

For runtime workflows, the generated app integration calls the module `run(...)` entry point. For `websocket.client`, the module is not used as the app runtime.

The current generated runtime model supports one runtime application module at a time.

## Why modules must be explicit

Hidden modules create confusion.

Bad model:

```txt id="sfsk03"
Vix guesses what the app needs.
```

Better model:

```txt id="zt94qa"
The app declares what it needs.
Vix wires it correctly.
```

This matters when debugging.

If the app uses JSON, it should be visible.

If the app uses database support, it should be visible.

If the app needs WebSocket, it should be visible.

Explicit modules make the app easier to understand.

## Built-in modules vs registry packages

There are two different ideas:

```txt id="f7br04"
Vix modules
registry packages
```

They are not the same.

| Item             | Meaning                                                         |
| ---------------- | --------------------------------------------------------------- |
| Vix module       | A capability provided by the Vix ecosystem or runtime           |
| Registry package | A versioned external package installed through the Vix registry |

Example module declaration:

```txt id="t0vrzd"
modules = [
  "core",
  "json",
  "http"
]
```

Example registry dependency:

```bash id="q88v27"
vix add softadastra/json
```

The mental model:

```txt id="jceqse"
modules = capabilities used by the app
packages = versioned dependencies installed into the project
```

## Dependency files

Registry dependencies are tracked by two files:

```txt id="ovchgt"
vix.json
vix.lock
```

`vix.json` stores declared dependency requirements.

`vix.lock` stores exact resolved versions.

The workflow:

```bash id="nayl3f"
vix registry sync
vix add softadastra/json
vix install
```

After that, the project has a reproducible dependency state.

After cloning:

```bash id="niwv5r"
vix install
```

That is the command that matters.

Do not use `vix update` when you only want to reproduce the project.

## `vix install` vs `vix update`

This distinction is important.

```txt id="v6w3js"
vix install = install what is already locked
vix update = resolve newer versions and change the lockfile
```

After cloning a project:

```bash id="ojdqld"
vix install
```

When you intentionally want newer versions:

```bash id="zkpf8g"
vix update --install
```

A serious project should not randomly update dependencies during normal setup.

Reproducibility starts with `vix.lock`.

## Adding a dependency

Use `vix add` when the project needs a new package.

```bash id="w5vhj9"
vix add softadastra/json
```

With a version:

```bash id="ga1hfk"
vix add softadastra/json@1.0.0
```

With a range:

```bash id="xf0d00"
vix add softadastra/json@^1.0.0
```

What changes:

```txt id="dig9y2"
vix.json
vix.lock
.vix/deps/
.vix/vix_deps.cmake
```

Then validate:

```bash id="x4qbi2"
vix build
vix check --tests
```

## Removing a dependency

Use:

```bash id="a55utb"
vix remove softadastra/json
```

Then reinstall project state:

```bash id="d87viq"
vix install
```

Then validate:

```bash id="dwdzy2"
vix check --tests
```

Removing a dependency is not only a CLI action.

You must also remove source code that depends on it:

```txt id="u7oquv"
#include statements
types
function calls
CMake references when using CMake directly
```

## Checking outdated dependencies

Use:

```bash id="pma40c"
vix outdated
```

Refresh the registry first:

```bash id="zyo3nl"
vix registry sync
vix outdated
```

For CI:

```bash id="f4x8ex"
vix outdated --strict
```

This checks whether packages are outdated.

It does not update them.

That is the correct behavior.

## Searching packages

Use:

```bash id="odsb16"
vix search json
```

Search uses the local registry index.

So sync first:

```bash id="sxpvxc"
vix registry sync
vix search softadastra
```

Example:

```txt id="g1tu4t"
softadastra/json  (latest: 1.0.0)
  JSON support for Softadastra and Vix applications.
  repo: https://github.com/softadastra/json
```

Search is offline after the registry index exists locally.

## Registry model

The registry model has three parts:

```txt id="oaczhx"
remote registry repository
local registry index
local package store
```

`vix registry sync` updates local metadata.

```bash id="cg9i7f"
vix registry sync
```

`vix store` manages local package content.

```bash id="w8qdsl"
vix store path
vix store gc --project --dry-run
```

The model:

```txt id="jzv91t"
registry index = package metadata
store = cached package content
project = selected dependencies
```

## Publishing packages

A reusable library can be published to the registry.

Basic flow:

```bash id="dmyar2"
vix fmt --check
vix check --tests
vix build --preset release

git tag -a v0.2.0 -m "Release v0.2.0"
git push origin v0.2.0

vix publish 0.2.0 --dry-run
vix publish 0.2.0 --notes "Add helpers"
```

Publishing expects:

```txt id="ck2k1y"
clean Git working tree
local tag
tag pushed to origin
registry index synced
package metadata available
```

If registry metadata is missing locally:

```bash id="z2s54u"
vix registry sync
```

## Package metadata

A package should have clear metadata.

Example `vix.json` for a library:

```json id="fm6567"
{
  "namespace": "softadastra",
  "name": "json",
  "version": "1.0.0",
  "type": "header-only",
  "include": "include",
  "license": "MIT",
  "description": "JSON support for Softadastra and Vix applications.",
  "keywords": ["cpp", "json", "vix"],
  "deps": []
}
```

Important fields:

| Field         | Role                          |
| ------------- | ----------------------------- |
| `namespace`   | package owner or organization |
| `name`        | package name                  |
| `version`     | package version               |
| `type`        | package type                  |
| `include`     | include root                  |
| `license`     | license                       |
| `description` | package description           |
| `keywords`    | search keywords               |
| `deps`        | package dependencies          |

Keep metadata boring and accurate.

That is better than marketing text.

## Package identity

A package id uses this form:

```txt id="ov9tud"
namespace/name
```

Example:

```txt id="t69fuj"
softadastra/json
softadastra/core
softadastra/fs
```

Scoped form is also accepted by package commands:

```txt id="tbwdo3"
@softadastra/json
```

But the identity is still:

```txt id="n8tlk5"
softadastra/json
```

Use stable names.

A package name should not change every week.

## Version model

Package versions should follow SemVer-style versions.

Example:

```txt id="kdb0ip"
0.1.0
1.0.0
1.2.3
```

Git tags use the `v` prefix:

```txt id="ymd3zu"
v0.1.0
v1.0.0
v1.2.3
```

Commands usually accept the version without the `v` in publish:

```bash id="z576p3"
vix publish 1.0.0
```

The Git tag is:

```txt id="p2bsdc"
v1.0.0
```

Keep this distinction clear.

## Lockfile model

A lockfile should contain exact resolved versions.

Conceptually:

```json id="u6qfjm"
{
  "lockVersion": 1,
  "dependencies": [
    {
      "id": "softadastra/json",
      "requested": "^1.0.0",
      "version": "1.0.3",
      "repo": "https://github.com/softadastra/json",
      "tag": "v1.0.3",
      "commit": "abc123",
      "hash": "..."
    }
  ]
}
```

The exact fields can evolve.

The purpose does not change:

```txt id="icx1d1"
same lockfile
  -> same dependency versions
  -> reproducible project setup
```

## Store model

The local store keeps package checkouts.

Common path:

```txt id="l466dk"
~/.vix/store/git
```

Inspect it:

```bash id="nw1m2a"
vix store path
```

Preview cleanup:

```bash id="xqpv3c"
vix store gc --project --dry-run
```

Run cleanup:

```bash id="hmkkas"
vix store gc --project
```

Be careful with project-scoped cleanup.

It keeps dependencies referenced by the current project lockfile and may remove other cached entries.

Preview first.

## Global packages

Some packages can be installed globally:

```bash id="sttfcw"
vix install -g softadastra/json
```

List global packages:

```bash id="rx5pf8"
vix list -g
```

Upgrade a global package:

```bash id="ilx0oc"
vix upgrade -g softadastra/json
```

Uninstall a global package:

```bash id="aizs5t"
vix uninstall -g softadastra/json
```

Global packages are stored outside the current project.

They are not the same as project dependencies.

## Project dependencies vs global packages

Keep this distinction:

| Scope   | Commands                                                              | Storage                     |
| ------- | --------------------------------------------------------------------- | --------------------------- |
| Project | `vix add`, `vix install`, `vix update`, `vix remove`, `vix list`      | current project and `.vix/` |
| Global  | `vix install -g`, `vix upgrade -g`, `vix uninstall -g`, `vix list -g` | `~/.vix/global/`            |

Use project dependencies for application builds.

Use global packages only when the package is meant to be available globally.

## Composition inside the source tree

Modules and packages should not create a messy source tree.

A backend can stay organized like this:

```txt id="k5e7tc"
src/
├── main.cpp
├── app/
├── config/
├── routes/
├── middleware/
├── validation/
├── database/
├── services/
└── errors/
```

Each folder should have one reason to exist.

Do not put all module usage into `main.cpp`.

Keep `main.cpp` small.

## Composition through an app factory

A clean app can be composed in an app factory.

Header:

```cpp id="kdqh8d"
#pragma once

#include <vix.hpp>

#include "config/Config.hpp"

namespace api
{
  vix::App create_app(const Config &config);
}
```

Implementation:

```cpp id="uc4eb7"
#include "app/AppFactory.hpp"

#include "routes/HealthRoutes.hpp"
#include "routes/UserRoutes.hpp"
#include "middleware/RequestIdMiddleware.hpp"

namespace api
{
  vix::App create_app(const Config &config)
  {
    vix::App app;

    register_request_id_middleware(app);
    register_health_routes(app, config);
    register_user_routes(app, config);

    return app;
  }
}
```

The application is composed in one clear place.

## Route composition

Routes should be grouped by feature.

```txt id="x4p4hn"
routes/
├── HealthRoutes.hpp
├── AuthRoutes.hpp
├── UserRoutes.hpp
└── AdminRoutes.hpp
```

Each group exposes one registration function.

Example:

```cpp id="lyw3yw"
#pragma once

#include <vix.hpp>

#include "config/Config.hpp"

namespace api
{
  void register_health_routes(vix::App &app, const Config &config);
}
```

This gives the app factory a simple job:

```cpp id="hvex1t"
register_health_routes(app, config);
```

No giant route file.

No giant `main.cpp`.

## Middleware composition

Middleware belongs in its own folder.

```txt id="uwggp6"
middleware/
├── RequestIdMiddleware.hpp
├── CorsMiddleware.hpp
├── AuthMiddleware.hpp
└── RateLimitMiddleware.hpp
```

Middleware should be explicit.

Example:

```cpp id="b8a48l"
register_request_id_middleware(app);
register_cors_middleware(app, config);
register_auth_middleware(app, config);
```

The order matters.

So the app factory should show the order.

## Validation composition

Validation should not be hidden inside handlers.

A backend can have:

```txt id="j6r5l2"
validation/
├── AuthValidation.hpp
├── UserValidation.hpp
└── CommonRules.hpp
```

Route handlers become easier to read.

The model:

```txt id="p4lvi7"
parse request
  -> validate input
  -> call service
  -> return response
```

Validation is a module-level capability, but the app should decide where validation rules live.

## Database composition

Database code should not be scattered everywhere.

A backend can use:

```txt id="smzwdc"
database/
├── Database.hpp
├── Database.cpp
├── migrations/
└── repositories/
```

Or:

```txt id="uycn24"
repositories/
├── UserRepository.hpp
└── SessionRepository.hpp
```

Migration commands:

```bash id="lh08r0"
vix db status
vix db migrate
vix db backup
```

ORM tooling:

```bash id="tsr5w9"
vix orm migrate --db api --dir ./migrations
vix orm status --db api --dir ./migrations
```

The app should not know every SQL detail in route handlers.

## WebSocket composition

A WebSocket app should keep real-time logic separate.

Example:

```txt id="qjbg1x"
websocket/
├── ChatGateway.hpp
├── PresenceGateway.hpp
└── WsEvents.hpp
```

Vix commands for WebSocket checks:

```bash id="qp9yqy"
vix ws check
vix health websocket
```

If an app uses WebSocket, the app model should show:

```txt id="vf06sh"
configured WebSocket URL
local WebSocket endpoint
public WebSocket endpoint
heartbeat behavior
timeout behavior
```

This belongs in configuration, not random code.

## P2P composition

P2P apps need node identity, listening port, discovery, and bootstrap behavior.

Example command:

```bash id="qzlwt2"
vix p2p --id A --listen 9001
```

With bootstrap:

```bash id="w06s2b"
vix p2p --id A \
  --listen 9001 \
  --bootstrap on \
  --registry http://127.0.0.1:8080 \
  --announce on
```

For source layout:

```txt id="y4ksek"
p2p/
├── NodeConfig.hpp
├── PeerHandlers.hpp
└── BootstrapConfig.hpp
```

The module gives the capability.

The app controls the topology.

## Sync composition

Offline-first systems need sync as a separate concern.

The sync model can include:

```txt id="axma97"
outbox
WAL
retry policy
conflict handling
network probe
sync engine
```

Do not mix sync logic randomly into handlers.

A clean app can have:

```txt id="wuxlsa"
sync/
├── SyncEngineFactory.hpp
├── OutboxHandlers.hpp
└── ConflictPolicy.hpp
```

The important model:

```txt id="n4kjuy"
local write first
durable operation
sync later
converge safely
```

This is a serious composition layer.

## Cache composition

Cache should be explicit.

There are different cache types:

```txt id="qq63sz"
HTTP cache
application data cache
build artifact cache
object cache
registry package store
```

Do not mix them.

In an app, cache code can live in:

```txt id="o84ity"
cache/
├── UserCache.hpp
├── ResponseCache.hpp
└── CachePolicy.hpp
```

For Vix local state, inspect cache through:

```bash id="x2pbvj"
vix info
vix store path
vix store gc --project --dry-run
```

Build cache is not the same as application cache.

## Crypto composition

Crypto should be isolated and reviewed carefully.

Example layout:

```txt id="x88xgb"
crypto/
├── PasswordHash.hpp
├── TokenSigner.hpp
└── KeyLoader.hpp
```

Do not scatter crypto logic across routes.

Do not hardcode secrets.

Use environment variables and config.

Check env:

```bash id="jvp4e5"
vix env check
vix env check --production
```

A crypto module gives primitives.

The app must still use them correctly.

## Logging composition

Logging should be available everywhere, but configured in one place.

Example config:

```dotenv id="v6d0ri"
VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
VIX_COLOR=auto
```

Production logs:

```bash id="v07fzw"
vix logs app
vix logs proxy
vix logs errors
```

The app should log enough to debug:

```txt id="gq1b94"
startup
configuration summary
route errors
database errors
external calls
shutdown
```

Do not log secrets.

## Environment composition

Modules often need environment variables.

Database:

```dotenv id="dy2wqg"
DATABASE_ENGINE=sqlite
DATABASE_DEFAULT_NAME=./data/app.db
```

Server:

```dotenv id="pqlrsu"
SERVER_HOST=127.0.0.1
SERVER_PORT=8080
```

Security:

```dotenv id="po8ogv"
JWT_SECRET=change-me
SESSION_SECRET=change-me
```

Production required variables:

```txt id="r2e97y"
production.env.required
```

Check them:

```bash id="mz1qg7"
vix env check --production
```

Composition is not only code.

It includes configuration.

## Production composition

A production app composes several layers:

```txt id="uzc031"
Vix app
systemd service
Nginx proxy
health checks
logs
deploy workflow
```

Commands:

```bash id="rd4pxl"
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

Production config belongs in `vix.json`.

The app should not require manual server steps that are impossible to repeat.

## Template composition

Templates should create the first composition.

A backend template should include:

```txt id="sm9xuq"
vix.app
vix.json
.env.example
production.env.required
src/main.cpp
src/app/
src/config/
src/routes/
src/middleware/
src/validation/
src/database/
src/services/
src/errors/
tests/
migrations/
public/
data/
```

The command:

```bash id="g2c9xi"
vix new api --template backend
```

should create a real backend foundation.

Not an empty toy.

## Library composition

A library is different from an app.

For a library:

```bash id="ojvijj"
vix new mathlib --lib
```

A library should focus on:

```txt id="n83zrs"
public headers
source files when needed
tests
package metadata
versioning
registry publishing
```

Typical layout:

```txt id="u652mq"
include/
src/
tests/
vix.json
README.md
LICENSE
```

A library can be packed and published.

```bash id="reb70o"
vix pack
vix verify
vix publish 0.1.0
```

## Application composition vs library composition

The difference:

| Project     | Main concern                       |
| ----------- | ---------------------------------- |
| Application | run, configure, serve, deploy      |
| Library     | expose API, test, package, publish |

An application uses modules and packages to provide behavior.

A library exposes reusable behavior to other projects.

Do not design them the same way.

## Composition checklist

For any Vix app, check:

```txt id="wkcbmg"
Are modules declared?
Are registry dependencies locked?
Is vix install enough after clone?
Is the source tree separated by feature?
Is main.cpp small?
Are routes grouped?
Is middleware order visible?
Is validation separated?
Is database access isolated?
Is env config documented?
Are health checks present?
Are production commands configured?
```

If not, the app is not yet well composed.

## Common workflows

Add a package:

```bash id="rmsbei"
vix registry sync
vix add softadastra/json
vix build
vix check --tests
```

Install after clone:

```bash id="xlzb8a"
vix install
vix build
```

Check outdated packages:

```bash id="zy7mzg"
vix registry sync
vix outdated
```

Update intentionally:

```bash id="okjhk5"
vix update --install
vix check --tests
```

Remove a dependency:

```bash id="hrnmst"
vix remove softadastra/json
vix install
vix check --tests
```

Publish a library:

```bash id="puopgo"
vix fmt --check
vix check --tests
vix build --preset release
git tag -a v0.1.0 -m "Release v0.1.0"
git push origin v0.1.0
vix publish 0.1.0
```

## Common mistakes

### Treating modules and packages as the same thing

Wrong model:

```txt id="el6z2y"
modules and packages are identical
```

Correct model:

```txt id="kl79mh"
modules are capabilities
packages are versioned dependencies
```

### Updating dependencies after clone

Wrong:

```bash id="kzxdmj"
vix update
```

Correct:

```bash id="brgg1a"
vix install
```

### Hiding everything in `main.cpp`

Wrong:

```txt id="rcv8kb"
main.cpp contains routes, config, database, middleware, and startup
```

Correct:

```txt id="zfx08z"
main.cpp starts the app
app factory composes the app
features live in focused folders
```

### Forgetting registry sync

If a package is not found:

```bash id="om9pzo"
vix registry sync
vix add namespace/name
```

### Forgetting validation after dependency changes

After changing dependencies:

```bash id="myoh5u"
vix build
vix check --tests
```

## What you should remember

Composition is how a Vix app grows.

The model:

```txt id="p8nxup"
app
  -> modules
  -> packages
  -> config
  -> runtime
  -> production
```

Use `modules = [...]` in `vix.app` for app capabilities.

Use `vix add` for registry dependencies.

Use `vix install` to reproduce locked dependency state.

Use `vix update` only when you intentionally want newer versions.

Keep source code separated by responsibility.

Keep `main.cpp` small.

The core rule:

```txt id="xqyvnh"
declare what the app uses
compose it in clear places
keep the workflow reproducible
```

## Next chapter

[Next: From Local to Production](/book/08-local-to-production)
