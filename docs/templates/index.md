# Project Templates

Vix project templates help you start with the right structure from the beginning.

Instead of creating the same folders, manifests, tests, configuration files, and project commands manually, you can generate a project that already matches your goal.

The main command is:

```bash
vix new <name>
```

Then you choose the right project shape:

```bash
vix new hello --app
vix new api --template backend
vix new site --template web
vix new dashboard --template vue
vix new mario --game
vix new mathlib --lib
```

## Why templates exist

C++ projects do not all need the same structure.
A small app should not start with a large backend architecture.
A backend service should not stay as one giant `main.cpp`.
A server-rendered website should not be organized like a Vue SPA.
A library should be designed for reuse, versioning, tests, examples, and registry publication.
A game project needs runtime concepts such as scenes, assets, frame updates, and game metadata.
Templates exist to give each project the right foundation.

## Available templates

## Available templates

| Template    | Command                            | Use when                                                             |
| ----------- | ---------------------------------- | -------------------------------------------------------------------- |
| Application | `vix new hello --app`              | You want a minimal runnable Vix application.                         |
| Backend     | `vix new api --template backend`   | You want a production-oriented API or backend service.               |
| Web         | `vix new site --template web`      | You want server-rendered HTML with Vix templates.                    |
| Vue         | `vix new dashboard --template vue` | You want a Vue frontend with a Vix C++ backend.                      |
| Game        | `vix new mario --game`             | You want a game, simulation, prototype, or custom engine foundation. |
| Library     | `vix new mathlib --lib`            | You want a reusable C++ library publishable to the Vix registry.     |

## Application template

The application template is the smallest real Vix project.

Use it for:

- learning Vix projects
- small HTTP apps
- prototypes
- simple tools
- experiments that need a real project structure

Create one:

```bash
vix new hello --app
cd hello
cp .env.example .env
vix build
vix run
```

It gives you a clean starting point without forcing a large architecture.

Read more: [Application template](/templates/application)

## Backend template

The backend template is for production-oriented services.

Use it for:

- JSON APIs
- REST APIs
- backend services
- WebSocket-ready services
- services with health checks
- services that may need auth, database, middleware, migrations, storage, or deployment metadata

Create one:

```bash
vix new api --template backend
cd api
cp .env.example .env
vix dev
```

It uses a layered backend architecture inspired by Clean Architecture:

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

Read more: [Backend template](/templates/backend)

## Web template

The web template is for server-rendered HTML.

Use it when C++ renders the pages directly with the Vix template engine.

Create one:

```bash
vix new site --template web
cd site
cp .env.example .env
vix dev
```

It gives you:

```txt
views/
public/
PageController
HealthController
RouteRegistry
MiddlewareRegistry
```

Use this template for:

- simple websites
- dashboards
- admin pages
- internal tools
- server-rendered pages
- HTML rendered by C++

Read more: [Web template](/templates/web)

## Vue template

The Vue template creates a frontend and backend project together.

Use it when you want:

```txt
Vue frontend
+ Vix C++ backend
```

Create one:

```bash
vix new dashboard --template vue
cd dashboard
cd frontend
npm install
cd ..
vix dev
```

The development flow is:

```txt
browser
  -> Vite dev server
      -> Vue app
          -> fetch("/api/...")
              -> Vite proxy
                  -> Vix backend
```

Use this template for:

- SaaS dashboards
- customer portals
- admin panels
- interactive web apps
- frontend-heavy products with a C++ backend

Read more: [Vue template](/templates/vue)

## Game template

The game template creates a Vix game project.

Use it for:

- small games
- simulations
- prototypes
- interactive tools
- real-time visualizations
- custom engine foundations

Create one:

```bash
vix new mario --game
cd mario
vix build
vix run
```

It gives you:

```txt
App
GameRuntime
Scene
SceneManager
Frame updates
assets/
game.package.json
vix.app
vix.json
```

`vix/game` is not a full game engine.

It is a modular runtime foundation for building games and interactive systems with C++.

Read more: [Game template](/templates/game)

For deeper examples, continue with: [Game guide](/guides/game)

## Library template

The library template creates a reusable C++ library.

Use it when you want:

- public headers
- tests
- examples
- a stable CMake target
- registry-safe structure
- a package that can be published with `vix publish`
- a package that can use dependencies with `vix add`

Create one:

```bash
vix new mathlib --lib
cd mathlib
vix build
vix tests
```

A library can also use the Vix Registry:

```bash
vix registry sync
vix search softadastra
vix add softadastra/core@^1.7.0
vix install
```

And it can be published:

```bash
git tag v0.1.0
vix registry sync
vix publish 0.1.0
```

Read more: [Library template](/templates/library)

## How to choose

Use this table when you are not sure which template to pick.

| Goal                                    | Best template  |
| --------------------------------------- | -------------- |
| I want to learn Vix projects            | Application    |
| I want a small runnable app             | Application    |
| I want a JSON API                       | Backend        |
| I want a production backend structure   | Backend        |
| I want server-rendered HTML             | Web            |
| I want a Vue frontend                   | Vue            |
| I want a C++ backend for a frontend app | Vue or Backend |
| I want a game or simulation             | Game           |
| I want a reusable package               | Library        |
| I want to publish to the Vix Registry   | Library        |

## Recommended learning path

If you are new to Vix, follow this order:

```txt
1. Application
2. Backend
3. Web
4. Vue
5. Library
6. Game
```

Why this order?

The application template teaches the smallest real project.

The backend template teaches production service structure.

The web template teaches server-rendered HTML.

The Vue template teaches frontend + backend integration.

The library template teaches reusable packages and registry workflows.

The game template teaches the game runtime foundation.

## Template responsibility

Each template has a different strategy.

| Template    | Main responsibility                                   |
| ----------- | ----------------------------------------------------- |
| Application | Start small with a real project.                      |
| Backend     | Organize APIs and backend services for growth.        |
| Web         | Render HTML pages from C++ with templates.            |
| Vue         | Combine a modern Vue frontend with a Vix backend.     |
| Game        | Build interactive real-time software with `vix/game`. |
| Library     | Build reusable packages for other projects.           |

Do not force one template to do everything.

Choose the template that matches the project you are building.

## Common mistakes

### Using the backend template for a tiny experiment

If you only want to test a small idea, use:

```bash
vix new hello --app
```

Use the backend template when you need a real service structure.

### Using the web template when you want Vue

The web template renders HTML on the server.

For Vue, use:

```bash
vix new dashboard --template vue
```

### Using the application template forever

The application template is a good starting point.

But when the app grows, move to a stronger structure.

Use:

```bash
vix new api --template backend
```

for serious backend services.

### Forgetting that libraries are reusable packages

A library should protect its public API.

Keep public headers under:

```txt
include/<name>/
```

and use the registry workflow when publishing.

### Treating `vix/game` like a complete engine

The game template gives you the foundation.

You build your own game systems on top.

## What you should remember

Use `vix new` to start with the right project shape:

```bash
vix new hello --app
vix new api --template backend
vix new site --template web
vix new dashboard --template vue
vix new mario --game
vix new mathlib --lib
```

The best template depends on the goal.

Start small with `--app`.

Use specialized templates when the project needs a clear strategy.

## Next steps

Continue with:

- [Application template](/templates/application)
- [Backend template](/templates/backend)
- [Web template](/templates/web)
- [Vue template](/templates/vue)
- [Game template](/templates/game)
- [Library template](/templates/library)
