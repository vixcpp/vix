# Web Template

The web template creates a server-rendered Vix web application.

Use it when you want to build a web application where C++ renders HTML pages directly on the server.

Create a web project with:

```bash
vix new site --template web
```

## What this template is for

Use the web template when you want:

- HTML rendered by the Vix backend
- layouts and reusable partials
- pages generated from C++ route handlers
- static CSS and JavaScript
- simple dashboards
- internal tools
- admin pages
- status pages
- lightweight server-rendered websites

This template is not a Vue app.

This template is not only a JSON API.

It is a C++ web application where Vix handles:

```txt
HTTP routes
HTML templates
static files
middleware
health checks
server startup
```

## Design used by this template

The web template uses a simple **server-rendered MVC-style design**.

MVC means:

```txt
Model      -> data used by the page
View       -> HTML template
Controller -> route handler that prepares data and renders the view
```

In this template:

```txt
PageController
  -> creates template context
  -> calls res.render(...)
  -> returns HTML
```

The main flow is:

```txt
main.cpp
  -> AppBootstrap
      -> templates
      -> static files
      -> middleware
      -> routes
          -> PageController
              -> views/*.html
```

This design is easy to understand:

- `controllers/` decide what page to render.
- `views/` contain HTML templates.
- `public/` contains CSS and JavaScript.
- `routes/` register controllers.
- `middleware/` registers request middleware.
- `AppBootstrap` wires everything together.

## Quick start

Create the project:

```bash
vix new site --template web
```

Enter the project:

```bash
cd site
```

Create local configuration:

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

Open the dashboard:

```txt
http://127.0.0.1:8080/dashboard
```

Check health:

```bash
curl http://127.0.0.1:8080/health
```

## Generated structure

A web project generated with:

```bash
vix new site --template web
```

has this structure:

```txt
site/
├── include/
│   └── site/
│       ├── app/
│       │   └── AppBootstrap.hpp
│       └── presentation/
│           ├── controllers/
│           │   ├── PageController.hpp
│           │   └── HealthController.hpp
│           ├── middleware/
│           │   └── MiddlewareRegistry.hpp
│           └── routes/
│               └── RouteRegistry.hpp
├── src/
│   ├── main.cpp
│   └── site/
│       ├── app/
│       │   └── AppBootstrap.cpp
│       └── presentation/
│           ├── controllers/
│           │   ├── PageController.cpp
│           │   └── HealthController.cpp
│           ├── middleware/
│           │   └── MiddlewareRegistry.cpp
│           └── routes/
│               └── RouteRegistry.cpp
├── views/
│   ├── base.html
│   ├── header.html
│   ├── index.html
│   └── dashboard.html
├── public/
│   ├── app.css
│   └── app.js
├── storage/
├── tests/
├── .env.example
├── .env
├── vix.app
├── vix.json
└── README.md
```

## What each folder does

| File or folder              | Role                                                   |
| --------------------------- | ------------------------------------------------------ |
| `src/main.cpp`              | Minimal entry point.                                   |
| `app/`                      | Application startup and wiring.                        |
| `presentation/controllers/` | Page and health route handlers.                        |
| `presentation/routes/`      | Central route registration.                            |
| `presentation/middleware/`  | Central middleware registration.                       |
| `views/`                    | Server-rendered HTML templates.                        |
| `public/`                   | Static CSS, JavaScript, images, and assets.            |
| `storage/`                  | Runtime local storage.                                 |
| `tests/`                    | Generated test target.                                 |
| `.env.example`              | Documented environment variables.                      |
| `.env`                      | Local runtime configuration.                           |
| `vix.app`                   | Build manifest.                                        |
| `vix.json`                  | Project metadata, tasks, and production orchestration. |

## `main.cpp`

`main.cpp` stays intentionally small.

```cpp
#include <site/app/AppBootstrap.hpp>

int main()
{
  site::app::AppBootstrap bootstrap;
  return bootstrap.run();
}
```

Do not put all routes, template rendering, middleware, and startup logic in `main.cpp`.

The startup logic belongs in `AppBootstrap`.

## `AppBootstrap`

`AppBootstrap` owns the startup sequence.

It does four important things:

```txt
load .env
create vix::App
configure templates and public files
register middleware and routes
start the server
```

The generated bootstrap configures:

```cpp
app.templates("views");
app.static_dir("public", "/");
```

Then it registers:

```cpp
MiddlewareRegistry::register_all(app);
RouteRegistry::register_all(app);
```

Finally, it starts the server with configuration:

```cpp
app.run(cfg);
```

This means the port comes from `.env`, not from hardcoded source code.

## `presentation/`

The `presentation/` layer is the HTTP and rendering layer.

It contains:

```txt
controllers/
routes/
middleware/
```

Use it for:

- page routes
- template rendering
- HTTP health checks
- request middleware
- browser-facing logic

For a web template, this layer is where most of your page logic starts.

## `PageController`

`PageController` owns the browser-facing pages.

Generated routes:

```txt
GET /
GET /dashboard
```

The controller creates a template context:

```cpp
vix::template_::Context ctx;
ctx.set("title", "Home");
ctx.set("app_name", "site");
ctx.set("user", "Guest");
```

Then it renders a template:

```cpp
res.render("index.html", ctx);
```

For the dashboard, it also shows how to pass arrays to templates:

```cpp
vix::template_::Array features;
features.emplace_back("Server-rendered HTML");
features.emplace_back("Layouts with extends");
features.emplace_back("Partials with include");
features.emplace_back("Static assets");

ctx.set("features", features);
```

Then:

```cpp
res.render("dashboard.html", ctx);
```

This is the central idea of the web template:

```txt
C++ prepares data
HTML template displays data
```

## `HealthController`

`HealthController` owns the health endpoint.

Generated route:

```txt
GET /health
```

It returns JSON:

```json
{
  "ok": true,
  "status": "ok",
  "service": "site",
  "template": "web"
}
```

Use `/health` for:

- local checks
- deployment checks
- reverse proxy checks
- monitoring
- production diagnostics

## `RouteRegistry`

Routes are centralized in:

```txt
src/site/presentation/routes/RouteRegistry.cpp
```

The generated registry connects controllers to the app:

```cpp
controllers::PageController::register_routes(app);
controllers::HealthController::register_routes(app);
```

When you add a new page controller, register it here.

Example:

```cpp
controllers::BlogController::register_routes(app);
controllers::AdminController::register_routes(app);
```

This keeps `AppBootstrap` clean.

## `MiddlewareRegistry`

Middleware is centralized in:

```txt
src/site/presentation/middleware/MiddlewareRegistry.cpp
```

The generated middleware registry installs:

```txt
security headers
request logging
X-Web marker header
```

Recommended web middleware order:

```txt
CORS
-> rate limit
-> request logging
-> security headers
-> body limits
-> auth
-> routes
```

Use this folder when you add:

- authentication checks
- sessions
- rate limiting
- security headers
- request logging
- body limits
- custom web headers

## `views/`

The `views/` folder contains server-side HTML templates.

Generated files:

```txt
views/base.html
views/header.html
views/index.html
views/dashboard.html
```

The generated templates show:

```txt
layout inheritance
partials
variables
loops
blocks
```

## `base.html`

`base.html` is the layout.

It defines the shared HTML structure:

```html
<!doctype html>
<html lang="en">
  <head>
    <title>{{ title }} - {{ app_name }}</title>
    <link rel="stylesheet" href="/app.css" />
  </head>
  <body>
    {% include "header.html" %}

    <main class="page">{% block content %}{% endblock %}</main>

    <script src="/app.js"></script>
  </body>
</html>
```

Use this file for shared structure:

- `<head>`
- CSS links
- scripts
- global layout
- shared header/footer

## `header.html`

`header.html` is a partial.

It is included by `base.html`:

```html
{% include "header.html" %}
```

Use partials for repeated pieces:

- navbar
- footer
- sidebar
- alerts
- shared UI blocks

## `index.html`

`index.html` extends the base layout:

```html
{% extends "base.html" %} {% block content %} ... {% endblock %}
```

It receives data from `PageController`.

Example variables:

```txt
{{ title }}
{{ app_name }}
{{ user }}
```

## `dashboard.html`

`dashboard.html` shows a more advanced page.

It receives:

```txt
title
app_name
user
total_orders
features
```

It also demonstrates a loop:

```html
{% for feature in features %}
<li>{{ feature }}</li>
{% endfor %}
```

Use this page as a starting point for:

- admin dashboard
- internal panel
- status page
- reporting page
- customer portal

## `public/`

The `public/` folder contains static assets.

Generated files:

```txt
public/app.css
public/app.js
```

The generated app mounts it at `/`:

```cpp
app.static_dir("public", "/");
```

That means:

```txt
public/app.css -> http://127.0.0.1:8080/app.css
public/app.js  -> http://127.0.0.1:8080/app.js
```

Use `public/` for:

- CSS
- JavaScript
- images
- icons
- fonts
- static downloads
- browser assets

Do not put C++ source code in `public/`.

## `.env.example`

`.env.example` documents the expected configuration.

It includes values such as:

```dotenv
APP_NAME=site
APP_ENV=development
APP_TEMPLATE=web

SERVER_HOST=0.0.0.0
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=5000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=20

SERVER_TLS_ENABLED=false

VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv

PUBLIC_PATH=public
VIEWS_PATH=views
TEMPLATE_AUTO_ESCAPE_HTML=true
TEMPLATE_CACHE=true

STORAGE_PATH=storage

VIX_SERVICE_NAME=site
VIX_HEALTH_LOCAL=http://127.0.0.1:8080/health
VIX_HEALTH_PUBLIC=
VIX_PROXY_DOMAIN=
```

When someone clones the project, they should run:

```bash
cp .env.example .env
```

## `.env`

`.env` contains local runtime values.

Use it for:

- local port
- environment name
- logging level
- public directory
- views directory
- storage path
- production diagnostics values

To change the port:

```dotenv
SERVER_PORT=3000
```

Then run:

```bash
vix dev
```

The source code does not need to change.

## `vix.app`

`vix.app` is the build manifest.

It describes the executable web target.

The generated web manifest includes source files such as:

```txt
src/main.cpp
src/site/app/AppBootstrap.cpp
src/site/presentation/routes/RouteRegistry.cpp
src/site/presentation/middleware/MiddlewareRegistry.cpp
src/site/presentation/controllers/PageController.cpp
src/site/presentation/controllers/HealthController.cpp
```

It also includes runtime resources:

```txt
resources = [
  ".env=.env",
  "public=public",
  "views=views",
  "storage=storage",
]
```

That means the web app has access to its runtime files when built.

The build flow is:

```txt
vix.app
  -> Vix generates internal CMake
  -> vix build compiles the web app
  -> vix dev starts the app in development mode
```

Do not edit the generated CMake project manually.

Edit `vix.app`.

## `vix.json`

`vix.json` stores project metadata, tasks, and production orchestration.

The web template uses it for:

- project name
- version
- template type
- tasks
- production service settings
- proxy settings
- health checks
- deployment settings
- logs
- required environment variables
- web runtime defaults

Common tasks include:

```bash
vix task dev
vix task build
vix task test
vix task check
```

The generated production section prepares:

```txt
production.service
production.ports
production.proxy
production.health
production.deploy
production.logs
production.env
production.web
```

## Configuration model

The web template uses two main configuration files:

```txt
.env       -> runtime values
vix.json   -> project orchestration
```

Use `.env` for values that change per environment:

```txt
SERVER_PORT
VIX_LOG_LEVEL
PUBLIC_PATH
VIEWS_PATH
STORAGE_PATH
```

Use `vix.json` for project-level metadata:

```txt
tasks
service name
proxy configuration
health check URLs
deployment workflow
required environment variables
web defaults
```

Simple rule:

```txt
.env       = how the app runs here
vix.json   = how Vix manages the project
```

## Generated routes

The web template starts with:

```txt
GET /             HTML home page
GET /dashboard    HTML dashboard page
GET /health       JSON health check
```

Use:

```bash
curl http://127.0.0.1:8080/health
```

For HTML pages, open in the browser:

```txt
http://127.0.0.1:8080/
http://127.0.0.1:8080/dashboard
```

## How to add a new page

Example: add an About page.

Create a template:

```txt
views/about.html
```

Example:

```html
{% extends "base.html" %} {% block content %}
<section class="card">
  <p class="eyebrow">About</p>
  <h1>{{ title }}</h1>
  <p class="lead">This page is rendered by Vix.cpp.</p>
</section>
{% endblock %}
```

Add a route in `PageController.cpp`:

```cpp
app.get("/about", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::template_::Context ctx;
  ctx.set("title", "About");
  ctx.set("app_name", "site");

  res.render("about.html", ctx);
});
```

Then run:

```bash
vix dev
```

Open:

```txt
http://127.0.0.1:8080/about
```

## How to add a new controller

When pages grow, avoid putting every route in `PageController`.

Create a new controller:

```txt
include/site/presentation/controllers/BlogController.hpp
src/site/presentation/controllers/BlogController.cpp
```

Register it in:

```txt
src/site/presentation/routes/RouteRegistry.cpp
```

Example:

```cpp
controllers::BlogController::register_routes(app);
```

Add the `.cpp` file to `vix.app`:

```txt
sources = [
  "src/main.cpp",
  "src/site/app/AppBootstrap.cpp",
  "src/site/presentation/routes/RouteRegistry.cpp",
  "src/site/presentation/middleware/MiddlewareRegistry.cpp",
  "src/site/presentation/controllers/PageController.cpp",
  "src/site/presentation/controllers/HealthController.cpp",
  "src/site/presentation/controllers/BlogController.cpp",
]
```

Then rebuild:

```bash
vix build
```

## How to add static assets

Put static files in:

```txt
public/
```

Examples:

```txt
public/logo.svg
public/admin.css
public/dashboard.js
public/images/hero.png
```

Use them in templates:

```html
<link rel="stylesheet" href="/admin.css" />
<img src="/images/hero.png" alt="Hero" />
<script src="/dashboard.js"></script>
```

## How to add shared layouts

Use `base.html` for common layout.

Use partials for repeated sections.

Example:

```txt
views/footer.html
views/sidebar.html
views/flash.html
```

Include them:

```html
{% include "footer.html" %}
```

## How to add dynamic data

Dynamic data is passed from C++ to the template context.

Example:

```cpp
vix::template_::Context ctx;
ctx.set("title", "Dashboard");
ctx.set("user", "Gaspard");
ctx.set("total_orders", 42);

res.render("dashboard.html", ctx);
```

Then in the template:

```html
<h1>{{ title }}</h1>
<p>Welcome back, {{ user }}.</p>
<strong>{{ total_orders }}</strong>
```

For lists:

```cpp
vix::template_::Array features;
features.emplace_back("Fast C++ server");
features.emplace_back("Server-rendered HTML");

ctx.set("features", features);
```

Template:

```html
<ul>
  {% for feature in features %}
  <li>{{ feature }}</li>
  {% endfor %}
</ul>
```

## Web template vs backend template

Use the web template when the main output is HTML.

Use the backend template when the main output is JSON APIs and backend services.

| Need                                                             | Template  |
| ---------------------------------------------------------------- | --------- |
| HTML pages rendered by C++                                       | `web`     |
| JSON API service                                                 | `backend` |
| Static assets with server-rendered pages                         | `web`     |
| Controllers, middleware, health checks, production API structure | `backend` |
| Large frontend app with Vue                                      | `vue`     |

The web template can still return JSON for `/health`.

The backend template can still serve static files.

The difference is the main purpose.

## Web template vs Vue template

Use the web template when C++ renders the HTML.

Use the Vue template when Vue renders the UI in the browser.

| Need                                 | Template |
| ------------------------------------ | -------- |
| Server-rendered HTML                 | `web`    |
| Vue SPA frontend                     | `vue`    |
| Simple admin or internal pages       | `web`    |
| Complex interactive frontend         | `vue`    |
| Mostly C++ driven rendering          | `web`    |
| Mostly JavaScript frontend rendering | `vue`    |

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

The web template is production-oriented, but it is still a starter.

Before production, you will usually add:

- real error pages
- real CORS policy if needed
- real sessions or authentication
- CSRF protection for forms
- rate limiting
- template caching policy
- Nginx proxy
- TLS termination
- health checks
- service management
- logs
- deployment workflow

The template gives you the structure where these pieces belong.

## When not to use this template

Do not use the web template when you only want a tiny C++ experiment.

Use the application template:

```bash
vix new hello --app
```

Do not use the web template when your main product is a JSON API.

Use the backend template:

```bash
vix new api --template backend
```

Do not use the web template when your main frontend is Vue.

Use the Vue template:

```bash
vix new dashboard --template vue
```

## Common mistakes

### Thinking `web` means Vue

The web template is server-rendered.

The HTML is rendered by Vix on the server.

For Vue, use:

```bash
vix new dashboard --template vue
```

### Putting all pages in one controller forever

`PageController` is enough at the beginning.

When the app grows, create more controllers:

```txt
BlogController
AdminController
AuthController
SettingsController
```

### Forgetting to update `vix.app`

When you add a new `.cpp` file, add it to:

```txt
sources = [
]
```

in `vix.app`.

### Putting CSS inside templates forever

For maintainability, keep CSS in:

```txt
public/
```

Example:

```txt
public/app.css
public/admin.css
```

### Hardcoding runtime values

Avoid hardcoding ports and paths.

Use `.env`:

```dotenv
SERVER_PORT=8080
PUBLIC_PATH=public
VIEWS_PATH=views
```

## What you should remember

The web template is for server-rendered HTML with Vix.

The main flow is:

```txt
main.cpp
  -> AppBootstrap
      -> app.templates("views")
      -> app.static_dir("public", "/")
      -> MiddlewareRegistry
      -> RouteRegistry
          -> PageController
              -> res.render("template.html", ctx)
```

Use each folder for its role:

| Folder                      | Role                                       |
| --------------------------- | ------------------------------------------ |
| `app/`                      | Startup and wiring.                        |
| `presentation/controllers/` | Page and health handlers.                  |
| `presentation/routes/`      | Route registration.                        |
| `presentation/middleware/`  | Middleware registration.                   |
| `views/`                    | HTML templates.                            |
| `public/`                   | CSS, JavaScript, images, and static files. |
| `storage/`                  | Runtime local storage.                     |
| `tests/`                    | Tests.                                     |

Create a web app:

```bash
vix new site --template web
cd site
cp .env.example .env
vix dev
```

Open:

```txt
http://127.0.0.1:8080
http://127.0.0.1:8080/dashboard
```

## Next steps

Continue with:

- [Vue template](/templates/vue)
- [Backend template](/templates/backend)
- [Templates guide](/guides/templates)
- [Static files](/guides/static-files)
- [Routes](/book/04-routes)
- [Request and Response](/book/05-request-response)
