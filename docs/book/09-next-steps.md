# Next Steps

Vix is not only a command line tool.

It is a workflow for building, running, composing, testing, packaging, and deploying C++ applications with less friction.

At this point, the mental model should be clear:

```txt id="b8f1m2"
create app
  -> declare modules
  -> install dependencies
  -> build
  -> run
  -> develop
  -> check
  -> package
  -> deploy
```

The next step is to use that model on real projects.

## The full workflow

A complete Vix workflow looks like this:

```bash id="t4x81k"
vix new api --template backend
cd api

vix install
vix dev
```

Then before committing:

```bash id="d8o93a"
vix fmt --check
vix build
vix check --tests
```

Then before release:

```bash id="h2m7wc"
vix build --preset release
vix check --tests
vix pack
vix verify
```

Then for production:

```bash id="k91s0v"
vix env check --production
vix service init
vix proxy nginx init
vix health
vix deploy --dry-run
vix deploy
```

That is the serious path.

Not one isolated command.

A workflow.

## What to learn next

Do not try to learn everything at once.

The best order is:

```txt id="q8j6bb"
1. Project creation
2. Runtime workflow
3. Build workflow
4. Modules
5. Dependencies
6. Tests and checks
7. Packaging
8. Production
```

This order matters.

If the project model is not clear, production will feel confusing.

If dependencies are not clear, builds will feel random.

If health checks are missing, deploys will be blind.

## Start with a backend template

The most important next step is the backend template.

A backend template should not be an empty demo.

It should create a real foundation:

```txt id="x6z4oi"
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

The command should be:

```bash id="m3wkz9"
vix new api --template backend
```

Then:

```bash id="lmp0z6"
cd api
vix install
vix dev
```

The backend template is the bridge between Vix as a tool and Vix as a production workflow.

## What the backend template should prove

The backend template should prove that Vix can create a real application.

It should include:

```txt id="k5g8aa"
HTTP server
health route
config loading
env validation
basic middleware
basic validation
database-ready structure
test skeleton
production config
service config
proxy config
deploy config
```

The goal is not to generate a toy.

The goal is to generate a project a developer can extend immediately.

## Recommended backend layout

The backend layout should be clear:

```txt id="u7i5hr"
api/
├── vix.app
├── vix.json
├── vix.lock
├── .env.example
├── production.env.required
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
│   │   └── HealthRoutes.cpp
│   ├── middleware/
│   │   ├── RequestIdMiddleware.hpp
│   │   └── RequestIdMiddleware.cpp
│   ├── validation/
│   │   └── Validation.hpp
│   ├── services/
│   │   └── HealthService.hpp
│   ├── database/
│   │   └── Database.hpp
│   └── errors/
│       └── AppError.hpp
├── tests/
│   └── test_health.cpp
├── migrations/
├── public/
└── data/
```

This layout gives the app structure from day one.

## Keep `main.cpp` small

The generated `main.cpp` should not contain the whole app.

It should only do startup work:

```txt id="x1c48f"
load config
create app
start server
handle shutdown
```

The composition should happen in:

```txt id="e9r5l2"
src/app/AppFactory.cpp
```

That keeps the application easy to grow.

## Use `vix.app` first

For normal applications, the template should prioritize `vix.app`.

Example:

```txt id="y4qrvn"
name = "api"
type = "executable"
cpp_standard = "23"

sources = [
  "src/main.cpp",
  "src/app/AppFactory.cpp",
  "src/config/Config.cpp",
  "src/routes/HealthRoutes.cpp",
  "src/middleware/RequestIdMiddleware.cpp"
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
  "log"
]
```

CMake can still exist for advanced projects.

But for app templates, `vix.app` should be the first-class path.

## Use CMake when needed

CMake remains the advanced path.

Use it when the project needs:

```txt id="lxgoyf"
custom native dependencies
many targets
advanced platform rules
special install rules
deep compiler control
complex library composition
```

But a new backend app should not need to start there.

The rule:

```txt id="g2lq9k"
vix.app for normal apps
CMakeLists.txt for advanced control
```

## Build the first serious app

The first serious app should be small but complete.

Build this:

```txt id="p4u3bt"
GET /health
GET /version
GET /api/users
POST /api/users
```

It should include:

```txt id="uwb9sm"
config
validation
JSON responses
tests
database-ready structure
production health
```

Then run:

```bash id="utg4pw"
vix dev
```

Then validate:

```bash id="s22xw7"
vix check --tests
```

This proves the daily workflow.

## Add WebSocket after HTTP

Do not start with everything.

After HTTP is stable, add WebSocket:

```txt id="cfzb3e"
ws endpoint
connection lifecycle
heartbeat
basic message handling
health websocket
```

Then check:

```bash id="dgtn8d"
vix ws check
vix health websocket
```

This proves real-time runtime workflow.

## Add database after routing

After routes are stable, add database.

Start with SQLite:

```txt id="fsc877"
data/app.db
migrations/
repositories/
vix db status
vix db migrate
vix db backup
```

Commands:

```bash id="c5h59m"
vix db status
vix db migrate
vix db backup
```

Then move to ORM workflow when needed:

```bash id="mwt7cn"
vix orm status --db api --dir ./migrations
vix orm migrate --db api --dir ./migrations
```

Do not mix SQL everywhere.

Keep database access isolated.

## Add production last

Production should come after the app works locally.

The order:

```txt id="h766s4"
local run works
tests pass
env is clear
health route exists
release build works
service config exists
proxy config exists
deploy dry run is clean
```

Then deploy.

Do not skip straight from `vix dev` to production.

## Production setup path

For a server:

```bash id="z6425m"
vix doctor
vix install
vix env check --production
vix build --preset release
vix service init
vix proxy nginx init
vix health
vix deploy --dry-run
vix deploy
```

If something fails, inspect:

```bash id="fowx0u"
vix logs errors --lines 120
vix service status
vix health
```

Production must be observable.

## Documentation path

The docs should guide users in this order:

```txt id="v5b5gu"
Getting Started
CLI Commands
Templates
Guides
The Vix Book
Production
Registry
```

The CLI pages explain commands.

The guides explain workflows.

The book explains the mental model.

Do not mix all three into one page.

## CLI pages

Each CLI page should answer:

```txt id="y2mk4l"
What does this command do?
When should I use it?
What is the basic usage?
What are the options?
What files does it affect?
What are common workflows?
What are common mistakes?
What command comes next?
```

This keeps the docs consistent.

The CLI reference should be complete.

Every route in the sidebar should have a page.

## Guide pages

Guides should be practical.

A guide should build something.

Examples:

```txt id="nd66ad"
Build a REST API
WebSocket Chat
Static Files
Templates
Game
Validation
Authentication
Sessions
CORS
Rate Limiting
SQLite API
MySQL API
Fast Target Builds
Object Cache
Artifact Cache
Replay a Run
Runtime Arguments
Diagnostics
Nginx + systemd
```

A guide is not a command reference.

It should show a real workflow.

## Book pages

The book should explain why the system exists and how to think.

The book structure:

```txt id="prnyz9"
01 Introduction
02 Why Vix Exists
03 Mental Model
04 Application Model
05 Runtime Workflow
06 Build Workflow
07 Modules and Composition
08 From Local to Production
09 Next Steps
```

The book should stay human.

No vague marketing.

No artificial sentences.

No empty claims.

Every chapter should make the reader more capable.

## Registry path

The registry is important for ecosystem growth.

The core registry workflow:

```bash id="ccs4qv"
vix registry sync
vix search json
vix add softadastra/json
vix install
vix build
```

For library authors:

```bash id="s0l5ts"
vix registry init
vix fmt --check
vix check --tests
vix build --preset release

git tag -a v0.1.0 -m "Release v0.1.0"
git push origin v0.1.0

vix publish 0.1.0 --dry-run
vix publish 0.1.0
```

The registry should make sharing C++ packages less painful.

## Package path

A package workflow:

```bash id="s7x677"
vix pack
vix verify
vix cache --path ./dist/<name>@<version>
```

For release:

```bash id="u4hks2"
vix build --preset release
vix check --tests
vix pack --name api --version 1.0.0
vix verify --path ./dist/api@1.0.0 --strict
```

Packaging should prove that the project can be reused or distributed.

## Development path

The daily path should stay simple:

```bash id="v45wyn"
vix dev
```

When something breaks:

```bash id="ajh8nw"
vix build -v
vix doctor
vix info
vix reset
```

When a run must be reproduced:

```bash id="c3d3m5"
vix run --replay
vix replay failed
```

When formatting matters:

```bash id="ulnlrz"
vix fmt --check
```

These are the commands developers should remember.

## Build performance path

After the basics work, improve speed.

Study:

```txt id="r91hyh"
fast target builds
object cache
artifact cache
incremental builds
build graph
change classification
```

Useful commands and guides:

```txt id="ix0y0l"
vix build
vix build -v
vix dev
guides/fast-target-builds
guides/object-cache
guides/artifact-cache
```

Speed should never break correctness.

The rule remains:

```txt id="sf81rp"
correct first
fast second
clear always
```

## Diagnostics path

Diagnostics should become a strength of Vix.

A good error should show:

```txt id="ydwd1z"
what failed
where it failed
why it failed
what to try next
```

Commands involved:

```bash id="qe4ccj"
vix doctor
vix info
vix build -v
vix replay show last
vix logs errors
```

Do not accept useless errors.

A tool for C++ developers must explain failure well.

## Production path

Production docs should make one route clear:

```txt id="aywirt"
env
service
proxy
health
logs
deploy
```

Commands:

```bash id="qbp0aq"
vix env check --production
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

This is the path from local app to live backend.

## What to build first

The best first real project is:

```txt id="uv3skx"
a production-ready backend template
```

It should include:

```txt id="v8kvvn"
vix.app
HTTP server
health route
.env.example
production.env.required
tests
release build workflow
systemd config workflow
Nginx proxy workflow
deploy config workflow
```

This proves the Vix workflow better than a small toy.

## What to avoid

Avoid building features in the wrong order.

Do not start with:

```txt id="uqjkw4"
too many templates
too many registry features
complex P2P demos
complex AI agent workflows
production dashboards
```

Start with the path every developer understands:

```txt id="nvu3nh"
create backend
run backend
build backend
test backend
deploy backend
```

That path sells the whole workflow.

## The best next implementation order

Use this order:

```txt id="nrfdap"
1. Backend template
2. Backend guide
3. Runtime arguments guide
4. Diagnostics guide
5. Production Nginx + systemd guide
6. Registry overview polish
7. Packaging and verify polish
8. Fast build guides
9. WebSocket guide
10. Database guide
```

This order creates a complete developer story.

## The backend template contract

The backend template should guarantee:

```txt id="l5ib95"
vix install works
vix build works
vix run works
vix dev works
vix check --tests works
vix env check works
vix env check --production works after production env is configured
vix service init has enough config
vix proxy nginx init has enough config
vix health local has a route to check
```

If a generated backend cannot do these things, the template is not finished.

## Release workflow

Before releasing a new Vix version:

```bash id="w5gqj4"
vix fmt --check
vix check --tests
vix build --preset release
```

Then update docs and changelog.

Then follow the release branch workflow.

A release should prove:

```txt id="lki9qj"
Linux works
macOS works
Windows works
CLI commands match docs
templates match docs
examples match docs
```

Docs and CLI must not drift.

## Documentation checklist

Before merging docs:

```txt id="of86l3"
Every sidebar link exists
Every CLI command page exists
Examples use current commands
No old vix deps wording remains
vix install is used for installing dependencies
vix.app is first-class for apps
CMake is described as the advanced path
Next links are valid
Build has no dead links
```

This checklist prevents broken docs.

## Project checklist

Before calling a Vix app production-ready:

```txt id="xthnbi"
vix.app or CMakeLists.txt is clear
modules are declared
dependencies are locked
vix install works after clone
vix build works
vix dev works
vix check --tests works
.env.example exists
production.env.required exists
health route exists
logs are readable
service config exists
proxy config exists
deploy dry run works
```

That is the minimum.

## Mental model to keep

The whole book can be reduced to this:

```txt id="uyfij7"
Vix makes C++ application workflow explicit.
```

Not hidden.

Not magical.

Not disconnected from real production.

Explicit project model.

Explicit modules.

Explicit dependencies.

Explicit build.

Explicit runtime.

Explicit production.

## Final command map

For project creation:

```bash id="umpt16"
vix new api --template backend
```

For file generation:

```bash id="kxq9dj"
vix make class User --in src/domain --namespace app::domain
```

For dependencies:

```bash id="eoyqic"
vix registry sync
vix add softadastra/json
vix install
```

For development:

```bash id="l4tq9p"
vix dev
```

For running:

```bash id="th5z63"
vix run
vix run main.cpp
vix run -- --port 8080
```

For building:

```bash id="gibaqb"
vix build
vix build --preset release
```

For validation:

```bash id="s7z36p"
vix fmt --check
vix check --tests
```

For replay:

```bash id="h0yncb"
vix run --replay
vix replay failed
```

For package:

```bash id="b62sga"
vix pack
vix verify
```

For production:

```bash id="r0frt9"
vix env check --production
vix service init
vix proxy nginx init
vix health
vix logs
vix deploy
```

For cleanup:

```bash id="kw79a5"
vix clean
vix reset
vix store gc --project --dry-run
```

## What comes after the book

After this book, the next documentation should be practical guides.

Start with:

```txt id="h42ivh"
Build a REST API
Runtime Arguments
Diagnostics
Nginx + systemd
Fast Target Builds
Object Cache
Artifact Cache
WebSocket Chat
SQLite API
```

The book teaches the model.

The guides prove it in real code.

## Final idea

Vix should make C++ feel like a complete application platform.

Not by hiding C++.

Not by replacing C++.

But by giving C++ the workflow developers expect:

```txt id="k9r43s"
create
run
build
test
package
deploy
debug
repeat
```

The next step is clear:

```bash id="e7yd94"
vix new api --template backend
```

Then make that generated backend good enough that a developer can trust it.

That is the foundation.
