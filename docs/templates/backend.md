# Backend Template

The backend template creates a production-oriented Vix backend application.
Use it when you want more than a simple `main.cpp`.

It is designed for real backend services that need:

- API routes
- health checks
- middleware
- static files
- templates
- configuration
- storage
- database migrations
- production diagnostics
- deployment metadata
- a structure that can grow without becoming messy

Create a backend project with:

```bash
vix new api --template backend
```

## What this template is for

Use the backend template when you want to build:

- REST APIs
- JSON APIs
- backend services
- admin backends
- realtime backends
- WebSocket-ready services
- production services behind Nginx and systemd
- services with health checks and diagnostics
- APIs that may later need database, auth, logs, migrations, static files, or background logic

This template is not only a demo.

It gives you a serious starting structure for a backend application.

## Design used by this template

The backend template uses a **layered backend architecture inspired by Clean Architecture**.

The important idea is simple:

```txt
main.cpp
  -> app bootstrap
      -> middleware
      -> routes
          -> controllers
              -> application logic
                  -> domain logic
                      -> infrastructure
```

The generated project starts small, but the folders already prepare the application to grow.

This design separates responsibilities:

| Layer             | Role                                                                                              |
| ----------------- | ------------------------------------------------------------------------------------------------- |
| `app/`            | Starts and wires the application.                                                                 |
| `presentation/`   | Handles HTTP routes, controllers, middleware, and request/response concerns.                      |
| `application/`    | Contains use cases and application workflows.                                                     |
| `domain/`         | Contains business rules and core models.                                                          |
| `infrastructure/` | Contains database, file system, external services, repositories, clients, and technical adapters. |
| `support/`        | Contains shared helpers used by the backend.                                                      |

The goal is not to make the project complicated.

The goal is to prevent a backend from becoming one giant `main.cpp`.

## Quick start

Create the project:

```bash
vix new api --template backend
```

Enter the project:

```bash
cd api
```

Create your local configuration:

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
http://127.0.0.1:8080/status.html
```

The home page includes a WebSocket status panel. It is a browser-side probe that tries to connect to:

```txt
ws://<current-host>:9090/
```

To make that panel turn green, add a WebSocket module and run the app:

```bash
vix modules init
vix modules add live_chat --websocket --workflow attached
vix build
vix run
```

The module name is customizable. You can use `live_chat`, `notifications`, `presence`, or any valid module name. The explicit option form is:

```bash
vix modules add --websocket --name notifications
```

Check the API:

```bash
curl http://127.0.0.1:8080/api
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/api/health
```

## Generated structure

A backend project generated with:

```bash
vix new api --template backend
```

has this structure:

```txt
api/
├── include/
│   └── api/
│       ├── app/
│       │   └── AppBootstrap.hpp
│       ├── support/
│       │   └── HttpResponses.hpp
│       └── presentation/
│           ├── controllers/
│           ├── middleware/
│           └── routes/
├── src/
│   ├── main.cpp
│   └── api/
│       ├── app/
│       │   └── AppBootstrap.cpp
│       ├── application/
│       ├── domain/
│       ├── infrastructure/
│       ├── presentation/
│       │   ├── controllers/
│       │   │   ├── HomeController.hpp
│       │   │   ├── HomeController.cpp
│       │   │   ├── HealthController.hpp
│       │   │   └── HealthController.cpp
│       │   ├── middleware/
│       │   │   ├── MiddlewareRegistry.hpp
│       │   │   └── MiddlewareRegistry.cpp
│       │   └── routes/
│       │       ├── RouteRegistry.hpp
│       │       └── RouteRegistry.cpp
│       └── support/
│           └── HttpResponses.cpp
├── public/
│   ├── index.html
│   ├── app.css
│   ├── app.js
│   ├── status.html
│   ├── status.css
│   └── status.js
├── views/
├── storage/
├── migrations/
├── tests/
├── .env.example
├── .env
├── vix.app
├── vix.json
└── README.md
```

## Why this design exists

A backend usually starts simple.

At the beginning, you may only need:

```txt
GET /api
GET /health
```

But later, a real backend often needs:

- authentication
- users
- roles
- orders
- billing
- file uploads
- WebSocket events
- database access
- migrations
- background jobs
- logs
- health checks
- deployment checks
- static status pages
- reverse proxy configuration

If everything stays inside `main.cpp`, the project becomes hard to maintain.

The backend template avoids that problem by separating the project early, but without forcing you to fill every folder immediately.

You can start with the generated files and add logic progressively.

## Request flow

The generated backend follows this flow:

```txt
main.cpp
  -> AppBootstrap
      -> load .env
      -> create vix::App
      -> configure templates
      -> mount public files
      -> register middleware
      -> register routes
      -> start server
```

Then an HTTP request follows this flow:

```txt
client request
  -> middleware stack
  -> route registry
  -> controller
  -> response helper
  -> response sent to client
```

This flow gives you a clear place for each responsibility.

## `main.cpp`

`main.cpp` is intentionally small.

It only starts the application:

```cpp
#include <api/app/AppBootstrap.hpp>

int main()
{
  api::app::AppBootstrap bootstrap;
  return bootstrap.run();
}
```

Do not put all routes, middleware, database code, and business logic in `main.cpp`.

`main.cpp` should stay focused on startup.

The `app/` folder owns the application startup sequence.

Generated files:

```txt
include/api/app/AppBootstrap.hpp
src/api/app/AppBootstrap.cpp
```

`AppBootstrap` is responsible for:

- loading `.env`
- creating `vix::App`
- reading configuration
- configuring templates
- mounting static files
- enabling static compression when configured
- registering middleware
- registering routes
- starting the server

The generated bootstrap reads values such as:

```txt
templates.path
public.path
public.mount
public.index
public.cache_control
public.spa_fallback
public.compression
```

Then it configures:

```cpp
app.templates(viewsPath);
app.static_dir(publicPath, publicMount, publicIndex, ...);
```

Finally, it starts the application:

```cpp
app.run(cfg);
```

This is why the port and runtime settings belong in `.env`, not in the source code.

## `presentation/`

The `presentation/` layer is the HTTP layer.

It contains the code that talks to the outside world through HTTP.

Generated folders:

```txt
presentation/controllers/
presentation/routes/
presentation/middleware/
```

Use this layer for:

- HTTP routes
- request parsing
- response formatting
- middleware registration
- authentication middleware
- CORS
- rate limiting
- API controllers
- status endpoints

Do not put core business rules here.

Controllers should call application services when the project grows.

## `presentation/routes/`

Routes are centralized in:

```txt
src/api/presentation/routes/RouteRegistry.cpp
```

The generated `RouteRegistry` registers controller routes:

```cpp
controllers::HomeController::register_routes(app);
controllers::HealthController::register_routes(app);
```

This means `AppBootstrap` does not need to know every controller.

When you add a new controller, register it here.

Example:

```cpp
controllers::UserController::register_routes(app);
controllers::OrderController::register_routes(app);
```

This keeps route wiring clear.

## `presentation/controllers/`

Controllers own HTTP route handlers.

Generated controllers:

```txt
HomeController
HealthController
```

`HomeController` registers:

```txt
GET /api
```

`HealthController` registers:

```txt
GET /health
GET /api/health
```

A controller should handle HTTP concerns:

- reading route parameters
- reading query parameters
- validating request shape
- calling application services
- sending JSON responses

Example role:

```txt
UserController
  -> receives HTTP request
  -> validates input
  -> calls CreateUserUseCase
  -> returns JSON response
```

A controller should not contain heavy business logic.

## `presentation/middleware/`

Middleware is centralized in:

```txt
src/api/presentation/middleware/MiddlewareRegistry.cpp
```

The generated middleware registry installs:

- security headers
- request logging
- an `X-API: true` marker header for `/api` routes

The generated file also documents the recommended production order:

```txt
CORS -> rate limit -> request logging -> security headers -> body limits -> auth -> routes
```

This order matters.

For example:

- CORS should run early because browsers need it.
- Rate limiting should run before expensive logic.
- Request logging should observe the request.
- Security headers should apply to responses.
- Body limits should protect memory.
- Auth should run before protected routes.
- Routes should run after middleware is ready.

When your app grows, add middleware here instead of scattering it across `main.cpp`.

## `support/`

The `support/` folder contains shared backend helpers.

Generated files:

```txt
include/api/support/HttpResponses.hpp
src/api/support/HttpResponses.cpp
```

The generated helpers include:

```cpp
json_error(...)
json_ok(...)
json_message(...)
```

Use this folder for small reusable helpers that are not business logic.

Examples:

- JSON response helpers
- pagination helpers
- HTTP error helpers
- request parsing helpers
- shared constants

Do not put database repositories here.

Do not put business rules here.

Use `application/`, `domain/`, and `infrastructure/` for that.

## `application/`

The `application/` folder is empty at the beginning.

It is where you put use cases.

A use case is an application action.

Examples:

```txt
CreateUser
LoginUser
PlaceOrder
SendMessage
UploadFile
GenerateReport
```

When the project grows, the controller should not do everything.

Instead, the controller should call a use case.

Example flow:

```txt
POST /api/users
  -> UserController
      -> CreateUserUseCase
          -> UserRepository
          -> returns result
      -> JSON response
```

Use `application/` for orchestration.

It coordinates domain logic and infrastructure.

## `domain/`

The `domain/` folder is empty at the beginning.

It is where you put business rules.

Examples:

```txt
User
Order
Invoice
Payment
Role
Permission
Message
Conversation
```

Use this folder for logic that should not depend on HTTP, database, Vix, or external services.

Good domain code should be easy to test.

Example:

```txt
domain/User.hpp
domain/Order.hpp
domain/PermissionPolicy.hpp
```

The domain layer answers questions like:

```txt
Can this user perform this action?
Is this order valid?
Can this invoice be paid?
Is this message allowed?
```

## `infrastructure/`

The `infrastructure/` folder is empty at the beginning.

It is where you put technical adapters.

Examples:

```txt
MySqlUserRepository
SqliteOrderRepository
FileStorage
EmailClient
RedisCache
PaymentGatewayClient
WebSocketMessageStore
```

Use this folder for things that talk to the outside world:

- database
- file system
- Redis
- HTTP clients
- payment APIs
- email providers
- storage services
- external APIs

The application layer can depend on interfaces, while infrastructure provides concrete implementations.

## `public/`

The backend template includes `public/` on purpose.

This does not mean the backend is a frontend framework.

Static files are useful in a production backend.

Generated files include:

```txt
public/index.html
public/app.css
public/app.js
public/status.html
public/status.css
public/status.js
```

Use `public/` for:

- a simple landing page
- a backend status page
- a WebSocket test page
- a local diagnostics UI
- static assets
- deployment check pages
- a small admin/status interface

This is useful especially when the backend has WebSocket or realtime features.

For example, a backend may expose:

```txt
/status.html
```

to test:

- HTTP is running
- static files are served
- health endpoint responds
- WebSocket connection can be tested later

The generated `AppBootstrap` mounts `public/` at `/`.

That means:

```txt
public/index.html   -> http://127.0.0.1:8080/
public/status.html  -> http://127.0.0.1:8080/status.html
```

## `views/`

The `views/` folder is for server-side templates.

The backend template configures:

```cpp
app.templates(viewsPath);
```

The folder can stay empty if your backend only returns JSON.

Use `views/` when you want the backend to render HTML pages.

Examples:

- admin page
- status dashboard
- email preview page
- simple internal tool
- server-rendered documentation page

If your project is mainly server-rendered HTML, use the web template instead:

```bash
vix new site --template web
```

## `storage/`

The `storage/` folder is for runtime storage.

Examples:

```txt
storage/api.db
storage/uploads/
storage/cache/
storage/logs/
storage/tmp/
```

The generated `.env.example` can point SQLite to:

```dotenv
DATABASE_SQLITE_PATH=storage/api.db
```

Use `storage/` for local files that belong to the running service.

Do not put source code here.

## `migrations/`

The `migrations/` folder is for database migrations.

A migration is a database change saved as a file.

Examples:

```txt
001_create_users.sql
002_create_orders.sql
003_add_user_roles.sql
```

The backend template includes migration settings in `.env`:

```dotenv
VIX_ORM_DIR=migrations
```

Use this folder when your backend starts using a database.

## `tests/`

The `tests/` folder contains backend tests.

The generated backend includes a basic test executable.

Run tests with:

```bash
vix tests
```

Use tests for:

- domain rules
- use cases
- helpers
- controllers
- regression checks
- safety before deployment

As the backend grows, add tests close to the logic you care about.

## `.env.example`

`.env.example` documents the expected configuration.

It is safe to commit.

It contains values such as:

```dotenv
APP_NAME=api
APP_ENV=development

SERVER_HOST=0.0.0.0
SERVER_PORT=8080

PUBLIC_PATH=public
PUBLIC_MOUNT=/
PUBLIC_INDEX=index.html
PUBLIC_CACHE_CONTROL=public, max-age=3600
PUBLIC_SPA_FALLBACK=false
PUBLIC_COMPRESSION=false

DATABASE_ENGINE=sqlite
DATABASE_SQLITE_PATH=storage/api.db

WEBSOCKET_ENABLED=false
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
```

When someone clones the project, they should run:

```bash
cp .env.example .env
```

## `.env`

`.env` contains local runtime values.

Use `.env` for:

- local port
- local database path
- local logging level
- local WebSocket settings
- secrets
- machine-specific values

Do not hardcode these values in C++ source code.

## `vix.app`

`vix.app` is the build manifest.

It describes the backend executable target.

The generated backend manifest includes source files such as:

```txt
src/main.cpp
src/api/app/AppBootstrap.cpp
src/api/support/HttpResponses.cpp
src/api/presentation/routes/RouteRegistry.cpp
src/api/presentation/middleware/MiddlewareRegistry.cpp
src/api/presentation/controllers/HomeController.cpp
src/api/presentation/controllers/HealthController.cpp
```

It also includes resources:

```txt
resources = [
  ".env=.env",
  "public=public",
  "views=views",
  "storage=storage",
]
```

That means these runtime folders are copied or made available next to the built executable.

The build flow is:

```txt
vix.app
  -> Vix generates internal CMake
  -> vix build compiles the backend
  -> vix run starts the backend
```

Do not edit the generated CMake project manually.

Edit `vix.app`.

## `vix.json`

`vix.json` is the project metadata and orchestration file.

The backend template uses it for:

- project name
- version
- template type
- tasks
- production service settings
- proxy settings
- health checks
- deployment settings
- logs
- database defaults
- required environment variables

Common tasks include:

```bash
vix task dev
vix task build
vix task test
vix task check
```

The generated `vix.json` also prepares production-oriented sections such as:

```txt
production.service
production.ports
production.websocket
production.proxy
production.health
production.deploy
production.logs
production.env
production.database
```

This gives Vix enough metadata to help with production workflows.

## Configuration model

The backend template uses two kinds of configuration.

```txt
.env       -> runtime values
vix.json   -> project orchestration
```

Use `.env` for values that change per environment:

```txt
SERVER_PORT
DATABASE_SQLITE_PATH
WEBSOCKET_PORT
VIX_LOG_LEVEL
```

Use `vix.json` for project-level metadata:

```txt
tasks
production service name
proxy configuration
health check URLs
deployment workflow
required env variables
```

Do not duplicate everything everywhere.

The simple rule is:

```txt
.env       = how the app runs here
vix.json   = how Vix manages the project
```

## Generated routes

The backend starts with these routes:

```txt
GET /api
GET /health
GET /api/health
```

The `/` route is served from:

```txt
public/index.html
```

That means:

```txt
GET /       -> static HTML page
GET /api    -> JSON backend route
GET /health -> health check
```

This separation is useful:

- `/` can show a simple status or landing page.
- `/api` proves the API layer works.
- `/health` is for local checks, monitoring, service checks, and deployment scripts.

## Middleware stack

The generated middleware registry installs:

```txt
security headers
request logging
X-API marker for /api routes
```

Recommended production order:

```txt
CORS
-> rate limit
-> request logging
-> security headers
-> body limits
-> auth
-> routes
```

As your backend grows, add middleware in `MiddlewareRegistry`.

Examples:

```txt
CORS
rate limiting
auth
request body size limits
compression
sessions
API version headers
```

## WebSocket-ready configuration

The backend template includes WebSocket configuration in `.env.example` and `vix.json`.

Example:

```dotenv
WEBSOCKET_ENABLED=false
WEBSOCKET_HOST=0.0.0.0
WEBSOCKET_PORT=9090
WEBSOCKET_MAX_MESSAGE_SIZE=65536
WEBSOCKET_IDLE_TIMEOUT=60
WEBSOCKET_ENABLE_DEFLATE=true
WEBSOCKET_PING_INTERVAL=30
WEBSOCKET_AUTO_PING_PONG=true
```

This does not mean every backend starts a WebSocket server automatically.

It means the backend is prepared for realtime features.

When you add WebSocket later, you already have:

- the port convention
- environment variables
- production metadata
- proxy metadata
- a place for static status or test pages

This is why `public/status.html` is useful.

A backend can serve a small page that helps verify HTTP and later WebSocket behavior.

## How to add a new route

Example: add a users controller.

Create:

```txt
include/api/presentation/controllers/UserController.hpp
src/api/presentation/controllers/UserController.cpp
```

Register the controller in:

```txt
src/api/presentation/routes/RouteRegistry.cpp
```

Example:

```cpp
controllers::UserController::register_routes(app);
```

Add the new `.cpp` file to `vix.app`:

```txt
sources = [
  "src/main.cpp",
  "src/api/app/AppBootstrap.cpp",
  "src/api/support/HttpResponses.cpp",
  "src/api/presentation/routes/RouteRegistry.cpp",
  "src/api/presentation/middleware/MiddlewareRegistry.cpp",
  "src/api/presentation/controllers/HomeController.cpp",
  "src/api/presentation/controllers/HealthController.cpp",
  "src/api/presentation/controllers/UserController.cpp",
]
```

Then run:

```bash
vix build
vix run
```

## How to add business logic

Do not put business logic directly in the controller forever.

Start simple, then move logic into `application/`.

Example:

```txt
src/api/application/CreateUserUseCase.hpp
src/api/application/CreateUserUseCase.cpp
```

The controller should become:

```txt
HTTP request
  -> UserController
      -> CreateUserUseCase
          -> result
  -> JSON response
```

This keeps controllers thin.

## How to add domain logic

Put business rules in `domain/`.

Example:

```txt
src/api/domain/User.hpp
src/api/domain/UserPolicy.hpp
```

Use this layer for logic like:

```txt
validate user role
check if order can be cancelled
check if invoice can be paid
validate message policy
```

Domain logic should not know about HTTP.

## How to add database code

Put database code in `infrastructure/`.

Example:

```txt
src/api/infrastructure/database/SqliteConnection.hpp
src/api/infrastructure/repositories/UserRepository.hpp
src/api/infrastructure/repositories/SqliteUserRepository.cpp
```

Use `migrations/` for database schema changes.

Example:

```txt
migrations/001_create_users.sql
```

## How to add static files

Put static files in:

```txt
public/
```

Examples:

```txt
public/status.html
public/admin.html
public/app.css
public/app.js
public/ws-test.html
```

Then open:

```txt
http://127.0.0.1:8080/status.html
```

Use this for simple backend pages, not for large frontend applications.

For large frontend applications, use the Vue template:

```bash
vix new dashboard --template vue
```

## How to add templates

Put HTML templates in:

```txt
views/
```

Use templates when your backend should render HTML.

For a project mainly focused on server-rendered pages, prefer:

```bash
vix new site --template web
```

## Build and run

Start development mode:

```bash
vix dev
```

Build:

```bash
vix build
```

Run:

```bash
vix run
```

Run tests:

```bash
vix tests
```

Check the project:

```bash
vix check --tests --run
```

## Production direction

The backend template is production-oriented, but it is still a starter.

Before production, you will usually add:

- real logging policy
- real error handling
- CORS policy
- rate limiting
- authentication
- database migrations
- service user
- Nginx proxy
- TLS termination
- health checks
- deployment scripts
- monitoring
- backups

The template gives you the structure where these pieces belong.

## When not to use this template

Do not use the backend template when you only want a tiny experiment.

Use the application template instead:

```bash
vix new hello --app
```

Do not use the backend template when you mainly want server-rendered HTML pages.

Use the web template instead:

```bash
vix new site --template web
```

Do not use the backend template when you want a Vue frontend and a Vix backend together.

Use the Vue template instead:

```bash
vix new dashboard --template vue
```

## Common mistakes

### Putting everything in `main.cpp`

Avoid growing `main.cpp`.

Keep it small.

Use:

```txt
AppBootstrap
RouteRegistry
MiddlewareRegistry
controllers
```

### Putting business logic in controllers forever

A controller can start simple.

But when logic grows, move it to:

```txt
application/
domain/
```

### Putting database code in controllers

Database code belongs in:

```txt
infrastructure/
```

Controllers should not directly become database scripts.

### Forgetting to update `vix.app`

When you add new `.cpp` files, add them to:

```txt
sources = [
]
```

in `vix.app`.

### Confusing static files with frontend apps

`public/` is useful for status pages, simple assets, diagnostics, and small HTML pages.

For a full frontend app, use the Vue template.

### Hardcoding runtime values

Avoid hardcoding ports, paths, or secrets.

Use `.env`.

## What you should remember

The backend template uses a layered backend architecture inspired by Clean Architecture.

The main flow is:

```txt
main.cpp
  -> AppBootstrap
      -> MiddlewareRegistry
      -> RouteRegistry
          -> Controllers
              -> Application
                  -> Domain
                  -> Infrastructure
```

Use each folder for its role:

| Folder            | Role                                    |
| ----------------- | --------------------------------------- |
| `app/`            | Startup and wiring.                     |
| `presentation/`   | HTTP layer.                             |
| `application/`    | Use cases and workflows.                |
| `domain/`         | Business rules.                         |
| `infrastructure/` | Database and external systems.          |
| `support/`        | Shared backend helpers.                 |
| `public/`         | Static backend assets and status pages. |
| `views/`          | Server-rendered templates.              |
| `storage/`        | Runtime local storage.                  |
| `migrations/`     | Database migrations.                    |
| `tests/`          | Tests.                                  |

Create a backend:

```bash
vix new api --template backend
cd api
cp .env.example .env
vix dev
```

Check it:

```bash
curl http://127.0.0.1:8080/api
curl http://127.0.0.1:8080/health
curl http://127.0.0.1:8080/api/health
```

## Next steps

Continue with:

- [Web template](/templates/web)
- [Vue template](/templates/vue)
- [Build a REST API](/guides/build-rest-api)
- [Middleware](/book/07-middleware)
- [Database](/book/10-database)
- [Realtime WebSocket](/book/11-realtime-websocket)
