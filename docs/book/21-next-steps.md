# Next steps

You have reached the end of the Vix book.
You started with one simple idea:

```txt
Run C++ code quickly.
```

Then you built the mental model step by step:

```txt
CLI → Runtime → Application → Modules → Production
```

## What you now understand

The main Vix workflow:

```bash
vix run main.cpp
vix new api
vix dev
vix build
vix check
vix tests
```

The main application model:

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res){
    res.json({"message", "Hello from Vix"});
  });

  app.run(8080);
  return 0;
}
```

## The path you completed

| Stage       | What you learned                                                        |
|-------------|-------------------------------------------------------------------------|
| Start       | What Vix is and why it exists.                                          |
| CLI         | Running files, creating projects, building, testing, and checking code. |
| HTTP        | Building routes with `App`, `Request`, and `Response`.                  |
| APIs        | Building JSON APIs.                                                     |
| Layers      | Adding middleware, validation, errors, and logging.                     |
| Data        | Using SQLite, MySQL, and database access.                               |
| Realtime    | Using WebSocket and the async runtime.                                  |
| Reliability | Using cache and offline-first synchronization.                          |
| Distributed | Building P2P features.                                                  |
| Production  | Deploying with Nginx, systemd, TLS, logs, and health checks.            |

## What to build next

The best next step is to build one real app from start to finish.

**A good first real Vix app:**

```txt
GET  /
GET  /health
GET  /users
GET  /users/{id}
POST /users
POST /auth/register
POST /auth/login
GET  /auth/me
```

With:
- validation,
- SQLite storage,
- structured errors,
- logs,
- production deployment.

## Recommended project

```bash
vix new users-api
cd users-api
vix dev
```

Build step by step:

1. Add `/health`
2. Add `/users`
3. Add JSON responses
4. Add validation
5. Add SQLite
6. Add structured errors
7. Add logs
8. Build release
9. Deploy behind Nginx and systemd

## Use the Guides section

The book teaches the story.
The guides help you solve specific problems:

- [Build a REST API](/guides/build-rest-api)
- [Validation](/guides/validation)
- [Authentication](/guides/authentication)
- [Sessions](/guides/sessions)
- [CORS](/guides/cors)
- [Rate limiting](/guides/rate-limiting)
- [SQLite API](/guides/sqlite-api)
- [MySQL API](/guides/mysql-api)
- [WebSocket chat](/guides/websocket-chat)
- [Static files](/guides/static-files)
- [Templates](/guides/templates)
- [Production: Nginx + systemd](/guides/production-nginx-systemd)

## Use the CLI reference

When you need command details:

```bash
vix run main.cpp --run --port 8080   # runtime args
vix run main.cpp -- -O2 -DNDEBUG    # compiler flags
vix build --preset release
vix build --with-sqlite
```

Remember: `--` = compiler/linker flags, `--run` = runtime arguments to your program.

## Learn by layers

| Layer | What to learn |
|-------|--------------|
| 1 | `vix run main.cpp` — run C++ files |
| 2 | `App`, routes, `Request`, `Response` — HTTP APIs |
| 3 | Middleware, validation, errors, logging |
| 4 | SQLite or MySQL |
| 5 | WebSocket for realtime |
| 6 | Cache and sync for reliability |
| 7 | P2P for distributed behavior |
| 8 | Release build, systemd, Nginx, HTTPS |

## Recommended learning order after the book

1. Build a REST API
2. Add validation and structured errors
3. Add SQLite
4. Add authentication
5. Deploy with Nginx and systemd
6. Add WebSocket
7. Add cache
8. Add offline-first sync
9. Add P2P
10. Study internals and performance

## Production checklist

**App:**
- [ ] Health route exists
- [ ] Errors use consistent JSON shape
- [ ] Input is validated
- [ ] Logs are structured
- [ ] Secrets are not logged

**Build:**
- [ ] Release build works
- [ ] Required flags enabled (`--with-sqlite`, `--with-mysql`)
- [ ] Dependencies installed

**Runtime:**
- [ ] App runs as non-root user
- [ ] systemd restarts it
- [ ] Working directory is correct

**Network:**
- [ ] App listens locally
- [ ] Nginx proxies public traffic
- [ ] HTTPS enabled
- [ ] WebSocket upgrade headers configured if needed

**Data:**
- [ ] Database path is stable
- [ ] Credentials in environment
- [ ] Backups exist

**Security:**
- [ ] Admin routes protected
- [ ] P2P control routes protected
- [ ] CORS configured correctly
- [ ] Rate limiting enabled where needed

## What makes a good Vix application

Keep `main()` small:

```cpp
int main()
{
  config::Config cfg{".env"};
  App app;
  configure_middlewares(app);
  register_public_routes(app);
  register_user_routes(app);
  app.run(cfg.getServerPort());
  return 0;
}
```

Use predictable response shapes:

```json
{ "ok": true, "data": {} }
{ "ok": true, "count": 2, "data": [] }
{ "ok": false, "error": "validation_failed", "message": "name is required" }
```

## When to use each runtime feature

| Feature      | Use when                                           |
|--------------|----------------------------------------------------|
| `vix run`    | You need to run one file quickly.                  |
| `vix new`    | You want to start a real project.                  |
| HTTP         | You are building APIs or web routes.               |
| JSON         | You need structured API responses.                 |
| Middleware   | You need shared request behavior.                  |
| Validation   | You are accepting user input.                      |
| Database     | You need durable application state.                |
| WebSocket    | You need realtime client updates.                  |
| Async        | You need timers, signals, or non-blocking I/O.     |
| Cache        | You need speed or stale data under failure.        |
| Sync         | You must not lose local operations.                |
| P2P          | You need nodes to discover, connect, or replicate. |
| Production   | Your app must run as a service.                    |

## A final example direction

**Reliable Notes API:**

```txt
POST   /notes
GET    /notes
GET    /notes/{id}
PATCH  /notes/{id}
DELETE /notes/{id}
GET    /health
```

Grow it:
authentication → WebSocket updates → offline-first outbox → P2P sync → production deployment.

This kind of project uses almost everything you learned.

## What you should remember

The full Vix path:

```txt
one C++ file → Vix project → HTTP API → professional API layers
→ database → realtime → async runtime → cache → offline-first sync
→ P2P → production deployment
```

The most important command is still:

```bash
vix run main.cpp
```

The most important production command is:

```bash
vix build --preset release
```

The most important mental model is:

```txt
Vix is a modern C++ runtime for building fast and reliable applications.
```

The final idea: **start simple, build progressively, deploy for real.**

*End of the Vix Book. You are ready to build real applications with Vix. Choose one project and build it completely.*
