# Next steps

You have reached the end of the Vix book.

You started with one simple idea:

```txt
Run C++ code quickly.
```

Then you built the mental model step by step:

```txt
one file
  -> project
  -> backend
  -> modules
  -> data
  -> realtime
  -> production
```

Now the best next step is not just reading more.

The best next step is to build one real backend template with Vix.

## What you now understand

You now understand the core Vix workflow:

```bash
vix run main.cpp
vix new api --template backend
vix dev
vix build
vix check --tests
vix tests
```

You also understand that Vix is not only a command to compile C++.

Vix is a workflow for building real applications:

```txt
create
  -> run
  -> develop
  -> build
  -> test
  -> package
  -> deploy
```

## The path you completed

| Stage         | What you learned                                                              |
| ------------- | ----------------------------------------------------------------------------- |
| Start         | What Vix is and why it exists                                                 |
| CLI           | Running files, creating projects, building, testing, and checking code        |
| Project model | How `vix.app`, `vix.json`, `vix.lock`, and generated integration fit together |
| Runtime       | How Vix runs files, projects, apps, binaries, and replayable executions       |
| HTTP          | Building routes with `App`, `Request`, and `Response`                         |
| APIs          | Building JSON APIs                                                            |
| Layers        | Middleware, validation, errors, logging, and authentication                   |
| Data          | SQLite, MySQL, migrations, backups, and ORM tooling                           |
| Realtime      | WebSocket checks and realtime workflows                                       |
| Reliability   | Build state, object cache, artifact cache, and replay                         |
| Distributed   | P2P nodes, discovery, bootstrap, and peer communication                       |
| Production    | systemd, Nginx, TLS, logs, health checks, env validation, and deploy          |

The next step should combine these parts into one serious template.

## The best next project

Build a production-ready backend template.

Not just a small demo.

Not only one route.

A real backend base that can become the default starting point for serious Vix applications.

```bash
vix new api --template backend
cd api
vix dev
```

The backend template should contain everything a modern backend needs:

```txt
routing
validation
authentication
sessions
database
migrations
structured errors
structured logs
health checks
WebSocket-ready config
production config
systemd config support
Nginx proxy support
deployment workflow
tests
```

## Why a backend template matters

A backend template is important because it becomes the bridge between:

```txt
learning Vix
```

and:

```txt
building real production software
```

A developer should not finish the book and then ask:

```txt
What should I create now?
```

The answer should be obvious:

```txt
Create a backend.
Run it.
Extend it.
Deploy it.
```

That is why the next step is:

```bash
vix new api --template backend
```

## What the backend template should include

A strong backend template should include this structure:

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
│   │   ├── AppFactory.hpp
│   │   └── AppFactory.cpp
│   ├── config/
│   │   ├── Config.hpp
│   │   └── Config.cpp
│   ├── routes/
│   │   ├── HealthRoutes.hpp
│   │   ├── HealthRoutes.cpp
│   │   ├── UserRoutes.hpp
│   │   ├── UserRoutes.cpp
│   │   ├── AuthRoutes.hpp
│   │   └── AuthRoutes.cpp
│   ├── middleware/
│   │   ├── CorsMiddleware.hpp
│   │   ├── AuthMiddleware.hpp
│   │   └── ErrorMiddleware.hpp
│   ├── validation/
│   │   ├── Validator.hpp
│   │   └── UserValidator.hpp
│   ├── database/
│   │   ├── Database.hpp
│   │   ├── Database.cpp
│   │   ├── UserRepository.hpp
│   │   └── UserRepository.cpp
│   ├── services/
│   │   ├── AuthService.hpp
│   │   └── AuthService.cpp
│   └── errors/
│       ├── ApiError.hpp
│       └── ErrorResponse.hpp
├── migrations/
│   ├── 2026_01_01_000001_create_users.up.sql
│   └── 2026_01_01_000001_create_users.down.sql
├── tests/
│   ├── test_health.cpp
│   ├── test_validation.cpp
│   └── test_auth.cpp
└── public/
```

The exact structure can evolve, but the strategy is stable:

```txt
small main
clear modules
explicit config
testable layers
production-ready defaults
```

## What the backend should expose

The first backend template should provide a useful API surface:

```txt
GET  /
GET  /health
GET  /ready
GET  /users
GET  /users/{id}
POST /users
POST /auth/register
POST /auth/login
GET  /auth/me
POST /auth/logout
```

This is enough to teach real backend structure without becoming too large.

## Recommended first route

Start with health:

```txt
GET /health
```

It should return:

```json
{
  "ok": true,
  "service": "api",
  "status": "healthy"
}
```

This route matters because it is used by:

```bash
vix health local
vix health public
vix deploy
```

A production backend starts with health.

## Recommended response shape

Use a predictable API response shape.

Success object:

```json
{
  "ok": true,
  "data": {}
}
```

Success list:

```json
{
  "ok": true,
  "count": 2,
  "data": []
}
```

Error:

```json
{
  "ok": false,
  "error": "validation_failed",
  "message": "name is required"
}
```

This makes the backend easy to test, debug, document, and consume.

## Recommended `main.cpp`

Keep `main.cpp` small.

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

The goal is not to put all logic in `main.cpp`.

The goal is:

```txt
main.cpp starts the app
modules define behavior
tests validate behavior
production commands run it safely
```

## Recommended `vix.app`

The backend template should prefer `vix.app` for applications.

Example:

```txt
name = "api"
type = "executable"
cpp_standard = "23"

sources = [
  "src/main.cpp",
  "src/app/AppFactory.cpp",
  "src/config/Config.cpp",
  "src/routes/HealthRoutes.cpp",
  "src/routes/UserRoutes.cpp",
  "src/routes/AuthRoutes.cpp",
  "src/database/Database.cpp",
  "src/database/UserRepository.cpp",
  "src/services/AuthService.cpp"
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

For applications, `vix.app` should be the default simple path.

CMake remains the compatibility and advanced path.

## Recommended `vix.json`

The backend template should also include `vix.json` for dependency and production workflows.

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
    "fmt": "vix fmt",
    "deploy": "vix deploy"
  },
  "production": {
    "service": {
      "name": "api",
      "user": "vix",
      "working_dir": "/home/vix/apps/api",
      "command": "vix run",
      "env_file": "/home/vix/apps/api/.env"
    },
    "proxy": {
      "domain": "api.example.com",
      "http_port": 8080,
      "websocket": {
        "enabled": false,
        "path": "/ws",
        "port": 9090
      },
      "tls": {
        "enabled": true,
        "certificate": "/etc/letsencrypt/live/api.example.com/fullchain.pem",
        "certificate_key": "/etc/letsencrypt/live/api.example.com/privkey.pem"
      }
    },
    "health": {
      "service": "api",
      "local": "http://127.0.0.1:8080/health",
      "public": "https://api.example.com/health"
    },
    "logs": {
      "service": "api",
      "nginx_access": "/var/log/nginx/api.example.com.access.log",
      "nginx_error": "/var/log/nginx/api.example.com.error.log"
    },
    "deploy": {
      "pull": true,
      "branch": "main",
      "build": "vix build --preset release",
      "tests": true,
      "test_command": "vix check --tests",
      "service": "api",
      "health_local": true,
      "health_public": true,
      "proxy_check": true,
      "proxy_reload": true,
      "logs_on_failure": true,
      "log_lines": 100,
      "rollback": true
    }
  }
}
```

The template should teach production from the beginning.

## Recommended `.env.example`

The backend template should include:

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

For production, the user copies it:

```bash
cp .env.example .env
```

Then validates:

```bash
vix env check
vix env check --production
```

## Recommended production required env

Create:

```txt
production.env.required
```

Example:

```txt
APP_ENV
SERVER_HOST
SERVER_PORT
DATABASE_ENGINE
DATABASE_DEFAULT_NAME
JWT_SECRET
SESSION_SECRET
```

Then:

```bash
vix env check --production
```

## Recommended development workflow

After creating the backend:

```bash
vix new api --template backend
cd api
cp .env.example .env
vix install
vix dev
```

Then in another terminal:

```bash
curl -i http://127.0.0.1:8080/health
```

## Recommended build workflow

```bash
vix build
vix check --tests
vix tests
```

For release:

```bash
vix build --preset release
vix check --tests
```

## Recommended database workflow

For SQLite:

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

For schema changes:

```bash
vix orm makemigrations \
  --new ./schema.new.json \
  --snapshot ./schema.json \
  --dir ./migrations \
  --name add_users \
  --dialect mysql
```

## Recommended production workflow

On the server:

```bash
vix doctor
vix info
vix install
vix env check --production
vix build --preset release
vix check --tests
vix service init
vix proxy nginx init
vix health
```

Then normal deployment becomes:

```bash
vix deploy
vix health
```

If something fails:

```bash
vix logs errors --lines 100
vix service status
vix proxy nginx check
```

## Use the Guides section

The book gives you the story.

The guides help you solve specific problems.

Start with:

- [Build a REST API](/guides/build-rest-api)
- [Validation](/guides/validation)
- [Authentication](/guides/authentication)
- [Sessions](/guides/sessions)
- [SQLite API](/guides/database/sqlite)
- [MySQL API](/guides/database/mysql)
- [Fast Target Builds](/guides/fast-target-builds)
- [Object Cache](/guides/object-cache)
- [Artifact Cache](/guides/artifact-cache)
- [Replay a Run](/guides/replay)
- [Runtime Arguments](/guides/runtime-arguments)
- [Diagnostics](/guides/diagnostics)
- [Production: Nginx + systemd](/guides/production-nginx-systemd)

## Use the CLI reference

When you need exact command details, use the CLI reference.

Start here:

- [vix run](/cli/run)
- [vix new](/cli/new)
- [vix dev](/cli/dev)
- [vix build](/cli/build)
- [vix check](/cli/check)
- [vix tests](/cli/tests)
- [vix make](/cli/make)
- [vix db](/cli/db)
- [vix service](/cli/service)
- [vix proxy](/cli/proxy)
- [vix health](/cli/health)
- [vix logs](/cli/logs)
- [vix deploy](/cli/deploy)

## Learn by building the template

Use this order:

| Step | Goal                 | Command                                    |
| ---- | -------------------- | ------------------------------------------ |
| 1    | Create backend       | `vix new api --template backend`           |
| 2    | Run development mode | `vix dev`                                  |
| 3    | Add routes           | `vix make function register_health_routes` |
| 4    | Format code          | `vix fmt`                                  |
| 5    | Add tests            | `vix make test HealthRoutes`               |
| 6    | Validate             | `vix check --tests`                        |
| 7    | Build release        | `vix build --preset release`               |
| 8    | Configure production | `vix env check --production`               |
| 9    | Create service       | `vix service init`                         |
| 10   | Create proxy         | `vix proxy nginx init`                     |
| 11   | Check health         | `vix health`                               |
| 12   | Deploy               | `vix deploy`                               |

This is the real learning path after the book.

## What to add after the first backend

Once the backend template works, extend it in this order:

1. Add validation
2. Add structured errors
3. Add SQLite
4. Add authentication
5. Add sessions
6. Add rate limiting
7. Add WebSocket
8. Add replay-enabled debugging
9. Add production deployment
10. Add packaging and registry publishing if it becomes reusable

## Production checklist

Before calling a backend production-ready:

**App**

- [ ] `GET /health` exists
- [ ] `GET /ready` exists if readiness differs from health
- [ ] Error responses use one JSON shape
- [ ] Inputs are validated
- [ ] Logs are structured
- [ ] Secrets are never logged

**Build**

- [ ] `vix build` works
- [ ] `vix build --preset release` works
- [ ] `vix check --tests` passes
- [ ] Dependencies are installed with `vix install`

**Database**

- [ ] Database config comes from env
- [ ] Migrations exist
- [ ] Backup command works
- [ ] Credentials are not hardcoded

**Runtime**

- [ ] App runs as non-root user
- [ ] systemd service starts
- [ ] systemd restarts after failure
- [ ] Working directory is correct

**Network**

- [ ] App listens locally
- [ ] Nginx proxies public traffic
- [ ] HTTPS works
- [ ] WebSocket proxy works if enabled

**Deployment**

- [ ] `vix deploy --dry-run` looks correct
- [ ] `vix deploy` works
- [ ] `vix health` passes after deploy
- [ ] `vix logs errors --lines 100` is clean enough

## What makes a good Vix backend

A good Vix backend has these qualities:

```txt
small main
explicit config
clear routes
testable services
database isolated behind repositories
predictable JSON responses
production health checks
logs readable from CLI
deployment reproducible
```

This is the target.

## When to use each feature

| Feature                      | Use when                                                 |
| ---------------------------- | -------------------------------------------------------- |
| `vix run`                    | You need to run one file, one app, or one binary quickly |
| `vix dev`                    | You are actively developing                              |
| `vix new --template backend` | You need a serious backend starting point                |
| `vix make`                   | You want to generate files inside an existing project    |
| `vix build`                  | You want to compile the project                          |
| `vix check`                  | You want to validate build, tests, and runtime behavior  |
| `vix tests`                  | You want to run tests                                    |
| `vix db`                     | You need database status, migrations, and backups        |
| `vix orm`                    | You need migration tooling from schema changes           |
| `vix replay`                 | You need to reproduce a previous run                     |
| `vix service`                | You need systemd service management                      |
| `vix proxy`                  | You need Nginx reverse proxy setup                       |
| `vix health`                 | You need local, public, or WebSocket checks              |
| `vix logs`                   | You need production logs                                 |
| `vix deploy`                 | You need a safe deployment workflow                      |

## A final example direction

Build this:

```txt
Reliable Users API
```

Routes:

```txt
GET    /health
GET    /ready
POST   /auth/register
POST   /auth/login
GET    /auth/me
POST   /auth/logout
GET    /users
GET    /users/{id}
PATCH  /users/{id}
DELETE /users/{id}
```

Then add:

```txt
validation
SQLite
migrations
sessions
structured logs
rate limiting
WebSocket notifications
production deploy
```

This project uses almost everything you learned.

## What you should remember

The full Vix path is:

```txt
one C++ file
  -> Vix project
  -> backend template
  -> APIs
  -> validation
  -> database
  -> realtime
  -> production
```

The most important beginner command is:

```bash
vix run main.cpp
```

The most important project command is:

```bash
vix new api --template backend
```

The most important development command is:

```bash
vix dev
```

The most important production command is:

```bash
vix deploy
```

The final mental model is:

```txt
Vix is a modern C++ runtime and workflow for building fast, reliable applications.
```

The final practical step is:

```bash
vix new api --template backend
cd api
vix dev
```

Build the template completely.

Then deploy it for real.

_End of the Vix Book. You are ready to build real applications with Vix._
