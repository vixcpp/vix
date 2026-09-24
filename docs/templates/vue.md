# Vue Template

The Vue template creates a project with two parts:

```txt
Vue frontend
Vix C++ backend
```

Use it when you want a modern browser UI powered by Vue, while keeping the backend in C++ with Vix.
Create a Vue project with:

```bash
vix new dashboard --template vue
```

## What this template is for

Use the Vue template when you want:

- a Vue frontend
- a Vix C++ backend
- API routes under `/api`
- Vite development server
- frontend hot reload
- C++ backend performance
- one project containing both frontend and backend
- dashboards, SaaS apps, admin panels, internal tools, or customer portals

This template is different from the web template.
The web template renders HTML on the server with Vix templates.
The Vue template renders the UI in the browser with Vue.

## Design used by this template

The Vue template uses a **frontend + backend architecture**.

The project is split into two clear parts:

```txt
frontend/      -> Vue application
src/main.cpp   -> Vix C++ backend
```

The development flow is:

```txt
browser
  -> Vite dev server
      -> Vue app
          -> fetch("/api/hello")
              -> Vite proxy
                  -> Vix backend on http://localhost:8080
```

This gives you a modern frontend workflow while keeping backend logic in C++.

## Quick start

Create the project:

```bash
vix new dashboard --template vue
```

Enter the project:

```bash
cd dashboard
```

Install frontend dependencies:

```bash
cd frontend
npm install
cd ..
```

Start development mode:

```bash
vix dev
```

Then open the Vue dev server URL shown by Vite.

If you want to run frontend and backend manually, start the backend first:

```bash
vix run
```

Then start Vue in another terminal:

```bash
cd frontend
npm run dev
```

## Generated structure

A Vue project generated with:

```bash
vix new dashboard --template vue
```

has this structure:

```txt
dashboard/
├── src/
│   └── main.cpp
├── frontend/
│   ├── index.html
│   ├── package.json
│   ├── vite.config.js
│   └── src/
│       ├── main.js
│       └── App.vue
├── tests/
├── vix.app
├── vix.json
└── README.md
```

Some projects may also include `.env` and `.env.example` depending on the selected options and current generator version.

## What each part does

| File or folder            | Role                                                               |
| ------------------------- | ------------------------------------------------------------------ |
| `src/main.cpp`            | Vix C++ backend entry point.                                       |
| `frontend/`               | Vue frontend application.                                          |
| `frontend/src/`           | Vue source files.                                                  |
| `frontend/src/App.vue`    | Main Vue component.                                                |
| `frontend/src/main.js`    | Vue application mount point.                                       |
| `frontend/vite.config.js` | Vite configuration and API proxy.                                  |
| `frontend/package.json`   | Frontend dependencies and scripts.                                 |
| `vix.app`                 | Vix backend build manifest.                                        |
| `vix.json`                | Project metadata, frontend config, tasks, and dependency metadata. |
| `tests/`                  | Backend tests.                                                     |

## Backend role

The backend is the Vix C++ part.

It handles API routes.

The generated frontend calls:

```txt
GET /api/hello
```

During development, the Vue app calls:

```js
fetch("/api/hello");
```

That request is proxied by Vite to:

```txt
http://localhost:8080
```

So the browser talks to Vue, and Vue talks to the Vix backend through `/api`.

## Frontend role

The frontend is the Vue application in:

```txt
frontend/
```

It owns:

- pages
- components
- browser UI
- frontend state
- CSS
- frontend routing if you add Vue Router later
- calls to the backend API

The generated `App.vue` loads a message from the backend:

```js
const response = await fetch("/api/hello");
const data = await response.json();
message.value = data.message || "Hello from Vix";
```

This confirms that the frontend and backend are connected.

## Vite proxy

The generated Vite config contains:

```js
export default defineConfig({
  clearScreen: false,
  plugins: [vue()],
  server: {
    host: "0.0.0.0",
    proxy: {
      "/api": "http://localhost:8080",
    },
  },
});
```

This means:

```txt
frontend request: /api/hello
real backend:     http://localhost:8080/api/hello
```

This avoids CORS issues during local development.

The browser sees one frontend origin.

Vite forwards API requests to the backend.

## Development mode

Use:

```bash
vix dev
```

For Vue projects, Vix can detect the Vue frontend from `vix.json` and `frontend/package.json`.

When detected, the dev session can start the Vue dev server and run the Vix backend together.

The project detection is based on:

```txt
vix.json contains "template": "vue"
vix.json contains "frontend"
frontend/package.json exists
```

If you prefer manual control, run two terminals:

Terminal 1:

```bash
vix run
```

Terminal 2:

```bash
cd frontend
npm run dev
```

## `frontend/package.json`

The generated frontend package contains:

```json
{
  "name": "dashboard-frontend",
  "private": true,
  "version": "0.1.0",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "vite build",
    "preview": "vite preview"
  },
  "dependencies": {
    "@vitejs/plugin-vue": "latest",
    "vite": "latest",
    "vue": "latest"
  },
  "devDependencies": {}
}
```

Useful commands:

```bash
cd frontend
npm install
npm run dev
npm run build
npm run preview
```

## `frontend/src/main.js`

This file mounts Vue into the browser page.

```js
import { createApp } from "vue";
import App from "./App.vue";

createApp(App).mount("#app");
```

The target element is in:

```txt
frontend/index.html
```

```html
<div id="app"></div>
```

## `frontend/src/App.vue`

`App.vue` is the first Vue component.

It shows the frontend and backend connection.

The generated component:

- creates a reactive `message`
- calls `/api/hello`
- displays the backend response
- shows a fallback if the backend cannot be reached

This is the first place to edit when building your UI.

## `src/main.cpp`

`src/main.cpp` is the Vix backend entry point.

It should expose API routes used by the Vue frontend.

A simple backend can look like this:

```cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/api/hello", [](Request &, Response &res) {
    res.json({
      "message", "Hello from the Vix backend"
    });
  });

  app.run();

  return 0;
}
```

Keep API routes under:

```txt
/api
```

This matches the Vite proxy and keeps frontend pages separate from backend APIs.

## `vix.json`

`vix.json` stores the Vue project metadata.

The generated file includes a `frontend` section:

```json
{
  "frontend": {
    "framework": "vue",
    "dir": "frontend",
    "dev": "npm run dev",
    "build": "npm run build",
    "dist": "frontend/dist"
  }
}
```

It also includes useful tasks:

```json
{
  "tasks": {
    "frontend:install": {
      "description": "Install Vue dependencies",
      "command": "npm install",
      "cwd": "frontend"
    },
    "frontend:dev": {
      "description": "Start Vue dev server",
      "command": "npm run dev",
      "cwd": "frontend"
    },
    "frontend:build": {
      "description": "Build Vue frontend",
      "command": "npm run build",
      "cwd": "frontend"
    },
    "backend:dev": {
      "description": "Start Vix backend",
      "command": "vix run"
    },
    "backend:build": {
      "description": "Build Vix backend",
      "command": "vix build --preset ${preset}"
    }
  }
}
```

Run tasks with:

```bash
vix task frontend:install
vix task frontend:dev
vix task frontend:build
vix task backend:dev
vix task backend:build
```

## `vix.app`

`vix.app` describes the backend target.

It is for the C++ backend, not the Vue frontend.

The Vue frontend is managed by:

```txt
frontend/package.json
frontend/vite.config.js
```

The backend is managed by:

```txt
vix.app
vix.json
src/main.cpp
```

Simple rule:

```txt
vix.app       -> C++ backend build
package.json  -> Vue frontend build
vix.json      -> project orchestration
```

## How frontend and backend talk

The generated app uses this path:

```txt
/api/hello
```

Frontend code:

```js
const response = await fetch("/api/hello");
```

Backend route:

```cpp
app.get("/api/hello", [](Request &, Response &res) {
  res.json({
    "message", "Hello from the Vix backend"
  });
});
```

During development:

```txt
Vue frontend -> Vite proxy -> Vix backend
```

In production, you usually serve the built frontend and backend behind the same domain or reverse proxy.

## Build the frontend

Run:

```bash
cd frontend
npm run build
```

This creates:

```txt
frontend/dist/
```

The `dist` folder contains the production frontend assets.

## Build the backend

From the project root:

```bash
vix build
```

For release:

```bash
vix build --preset release
```

## Run the backend

From the project root:

```bash
vix run
```

The backend should listen on the port configured by the environment.

Common default:

```txt
http://localhost:8080
```

## Recommended development workflow

For daily development:

```bash
vix dev
```

If automatic Vue dev mode is not available on your platform, use two terminals.

Terminal 1:

```bash
vix run
```

Terminal 2:

```bash
cd frontend
npm run dev
```

Then open the Vite dev server URL.

## Recommended production workflow

Build the frontend:

```bash
cd frontend
npm run build
cd ..
```

Build the backend:

```bash
vix build --preset release
```

Then deploy:

```txt
backend executable
frontend/dist
environment variables
reverse proxy configuration
```

A common production setup is:

```txt
Nginx
  -> serves frontend static files
  -> proxies /api to the Vix backend
```

This keeps the browser UI fast while Vix handles backend API requests.

## How to add a new API route

Add a route in `src/main.cpp`:

```cpp
app.get("/api/status", [](Request &, Response &res) {
  res.json({
    "ok", true,
    "service", "dashboard"
  });
});
```

Call it from Vue:

```js
const response = await fetch("/api/status");
const data = await response.json();
```

Because the path starts with `/api`, Vite will proxy it to the backend during development.

## How to add Vue pages

For a small project, you can continue editing:

```txt
frontend/src/App.vue
```

When the UI grows, add:

```txt
frontend/src/components/
frontend/src/pages/
frontend/src/api/
```

Recommended structure:

```txt
frontend/src/
├── api/
│   └── client.js
├── components/
│   └── AppCard.vue
├── pages/
│   └── DashboardPage.vue
├── App.vue
└── main.js
```

Use `frontend/src/api/` for functions that call the backend.

Example:

```js
export async function getStatus() {
  const response = await fetch("/api/status");
  return response.json();
}
```

## How to add frontend routing

When the UI grows, install Vue Router:

```bash
cd frontend
npm install vue-router
```

Then create:

```txt
frontend/src/router/
frontend/src/pages/
```

Use Vue Router for browser navigation.

Keep backend routes under `/api`.

## How to organize backend code

The generated Vue template starts with a simple backend.

That is intentional.

If the backend grows, you can split routes into files:

```txt
src/main.cpp
src/routes.cpp
src/routes.hpp
```

If the backend becomes a serious API, move to the backend template structure:

```bash
vix new api --template backend
```

Use the backend template when you need:

- controllers
- middleware registries
- route registries
- application layer
- domain layer
- infrastructure layer
- migrations
- production diagnostics

## Vue template vs web template

Use the Vue template when Vue renders the UI in the browser.

Use the web template when Vix renders HTML on the server.

| Need                           | Template |
| ------------------------------ | -------- |
| Vue SPA frontend               | `vue`    |
| Server-rendered HTML           | `web`    |
| Frontend hot reload            | `vue`    |
| HTML templates rendered by C++ | `web`    |
| Browser-side components        | `vue`    |
| Backend-rendered pages         | `web`    |

## Vue template vs backend template

Use the Vue template when the frontend is a major part of the product.

Use the backend template when the main product is a backend service or API.

| Need                                               | Template  |
| -------------------------------------------------- | --------- |
| Vue frontend + C++ backend                         | `vue`     |
| Production-oriented JSON API                       | `backend` |
| Vite frontend workflow                             | `vue`     |
| Controllers, middleware, health checks, migrations | `backend` |
| Browser UI first                                   | `vue`     |
| API/service first                                  | `backend` |

A Vue project can call a backend API.

A backend project can serve static files.

The difference is the main development strategy.

## Common mistakes

### Starting the frontend before installing dependencies

Run this first:

```bash
cd frontend
npm install
```

### Calling the backend without `/api`

During development, the proxy is configured for:

```txt
/api
```

Prefer API routes like:

```txt
/api/hello
/api/users
/api/orders
/api/status
```

### Hardcoding backend URLs in Vue

Avoid this in normal development:

```js
fetch("http://localhost:8080/api/hello");
```

Prefer:

```js
fetch("/api/hello");
```

This lets Vite proxy the request in development and makes production easier.

### Forgetting to start the backend

If Vue shows:

```txt
Could not reach the Vix backend
```

Start the backend:

```bash
vix run
```

or:

```bash
vix dev
```

### Mixing frontend build and backend build

Use the right command for each side:

```txt
npm run build  -> builds Vue frontend
vix build      -> builds Vix backend
```

### Putting backend logic in Vue

Vue should call APIs.

Backend logic belongs in C++.

Example:

```txt
Vue component
  -> fetch("/api/orders")
      -> Vix backend
          -> database or service logic
```

## What you should remember

The Vue template gives you:

```txt
Vue frontend
+ Vix C++ backend
+ Vite proxy for /api
+ project tasks in vix.json
```

The development flow is:

```txt
Vue app
  -> fetch("/api/...")
      -> Vite proxy
          -> Vix backend
```

Create a Vue project:

```bash
vix new dashboard --template vue
cd dashboard
cd frontend
npm install
cd ..
vix dev
```

Use:

```bash
vix task frontend:dev
vix task frontend:build
vix task backend:dev
vix task backend:build
```

when you want task-based workflows.

## Next steps

Continue with:

- [Backend template](/templates/backend)
- [Web template](/templates/web)
- [Build a REST API](/guides/build-rest-api)
- [Routes](/book/04-routes)
- [Request and Response](/book/05-request-response)
- [Realtime WebSocket](/book/11-realtime-websocket)
