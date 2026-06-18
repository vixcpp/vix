# Production Bootstrap

This example shows a production-style Vix application structure.

The goal is to keep `main()` small and move startup wiring into an application bootstrap layer.

This is useful when your app needs:

```txt id="ik1a8v"
configuration from .env
server settings
public files
SPA fallback
optional static compression
API middleware
route registration
clean startup structure
```

This example is not a framework.

It is a clean way to organize a real Vix backend.

## What this example builds

The app exposes:

```txt id="sjy3za"
GET  /
GET  /api/health
GET  /api/products
POST /api/products
```

It also serves static files from:

```txt id="r882lw"
public/
```

The app is organized like this:

```txt id="ig3ts0"
main.cpp
  calls AppBootstrap::run()

AppBootstrap
  loads configuration
  creates vix::App
  configures static files
  configures optional static compression
  installs middleware
  registers routes
  starts the server

MiddlewareRegistry
  installs API middleware

RouteRegistry
  registers application routes
```

## Project structure

Create this structure:

```txt id="q3ddc8"
production_bootstrap_demo/
├── .env
├── CMakeLists.txt
├── main.cpp
├── public/
│   ├── index.html
│   ├── app.js
│   └── style.css
└── src/
    ├── AppBootstrap.hpp
    ├── AppBootstrap.cpp
    ├── MiddlewareRegistry.hpp
    ├── MiddlewareRegistry.cpp
    ├── RouteRegistry.hpp
    └── RouteRegistry.cpp
```

For a larger project, you can move these into namespaces such as:

```txt id="yyg6mx"
blog::app
blog::presentation::middleware
blog::presentation::routes
```

This example keeps names simple so the structure is easy to understand.

## .env

Create:

```txt id="nwta3x"
.env
```

Add:

```dotenv id="oq8wkj"
APP_NAME=production-bootstrap-demo
APP_ENV=development

SERVER_HOST=0.0.0.0
SERVER_PORT=8080
SERVER_REQUEST_TIMEOUT=5000
SERVER_IO_THREADS=0
SERVER_SESSION_TIMEOUT_SEC=20
SERVER_BENCH_MODE=false

PUBLIC_PATH=public
PUBLIC_MOUNT=/
PUBLIC_INDEX=index.html
PUBLIC_CACHE_CONTROL=public, max-age=3600
PUBLIC_SPA_FALLBACK=true
PUBLIC_COMPRESSION=false
PUBLIC_COMPRESSION_MIN_SIZE=1024

VIX_LOG_LEVEL=info
VIX_LOG_FORMAT=kv
```

The important static file values are:

```txt id="gy14m9"
PUBLIC_PATH
PUBLIC_MOUNT
PUBLIC_INDEX
PUBLIC_CACHE_CONTROL
PUBLIC_SPA_FALLBACK
PUBLIC_COMPRESSION
PUBLIC_COMPRESSION_MIN_SIZE
```

## public/index.html

Create:

```txt id="ylu9bz"
public/index.html
```

Add:

```html id="w4d7k0"
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Vix Production Bootstrap</title>
    <link rel="stylesheet" href="/style.css" />
  </head>
  <body>
    <main class="page">
      <h1>Vix Production Bootstrap</h1>
      <p>This page is served from the public directory.</p>

      <button id="health-button">Check API health</button>
      <button id="products-button">Load products</button>

      <pre id="output">Click a button.</pre>
    </main>

    <script src="/app.js"></script>
  </body>
</html>
```

## public/style.css

Create:

```txt id="fjcbw2"
public/style.css
```

Add:

```css id="b8q9lr"
body {
  margin: 0;
  font-family: system-ui, sans-serif;
  background: #f6f7f9;
  color: #111827;
}

.page {
  max-width: 760px;
  margin: 80px auto;
  padding: 32px;
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 16px;
}

button {
  margin-right: 8px;
  padding: 10px 16px;
  border: 0;
  border-radius: 10px;
  cursor: pointer;
}

pre {
  margin-top: 24px;
  padding: 16px;
  background: #111827;
  color: #f9fafb;
  border-radius: 12px;
  overflow: auto;
}
```

## public/app.js

Create:

```txt id="ewz5ah"
public/app.js
```

Add:

```js id="w26fo9"
const output = document.querySelector("#output");

async function showJson(url) {
  const response = await fetch(url);
  const data = await response.json();

  output.textContent = JSON.stringify(data, null, 2);
}

document
  .querySelector("#health-button")
  .addEventListener("click", () => showJson("/api/health"));

document
  .querySelector("#products-button")
  .addEventListener("click", () => showJson("/api/products"));
```

## main.cpp

Create:

```txt id="sszy79"
main.cpp
```

Add:

```cpp id="hqaelq"
#include "src/AppBootstrap.hpp"

int main()
{
  return AppBootstrap::run();
}
```

`main()` stays small.

It does not know about middleware, routes, static files, or configuration details.

## src/AppBootstrap.hpp

Create:

```txt id="rx0s3z"
src/AppBootstrap.hpp
```

Add:

```cpp id="hhkzci"
#ifndef APP_BOOTSTRAP_HPP
#define APP_BOOTSTRAP_HPP

class AppBootstrap
{
public:
  static int run();
};

#endif
```

## src/AppBootstrap.cpp

Create:

```txt id="h17h7m"
src/AppBootstrap.cpp
```

Add:

```cpp id="n0ojaz"
#include "AppBootstrap.hpp"

#include "MiddlewareRegistry.hpp"
#include "RouteRegistry.hpp"

#include <cstddef>
#include <string>

#include <vix.hpp>
#include <vix/middleware.hpp>

static std::string config_string(
  vix::config::Config &cfg,
  const std::string &key,
  const std::string &fallback)
{
  return cfg.getString(key, fallback);
}

static bool config_bool(
  vix::config::Config &cfg,
  const std::string &key,
  bool fallback)
{
  return cfg.getBool(key, fallback);
}

static int config_int(
  vix::config::Config &cfg,
  const std::string &key,
  int fallback)
{
  return cfg.getInt(key, fallback);
}

static void configure_static_compression(vix::config::Config &cfg)
{
  const bool public_compression =
    config_bool(cfg, "public.compression", false);

  if (!public_compression)
    return;

  const int min_size =
    config_int(cfg, "public.compression_min_size", 1024);

  const auto options =
    vix::middleware::performance::CompressionOptions{
      .min_size = static_cast<std::size_t>(min_size),
      .add_vary = true,
      .enabled = true
    };

  vix::App::set_static_response_hook(
    vix::middleware::performance::compressed_static_response_hook(options)
  );
}

static void configure_static_files(
  vix::App &app,
  vix::config::Config &cfg)
{
  const std::string public_path =
    config_string(cfg, "public.path", "public");

  const std::string public_mount =
    config_string(cfg, "public.mount", "/");

  const std::string public_index =
    config_string(cfg, "public.index", "index.html");

  const std::string public_cache_control =
    config_string(cfg, "public.cache_control", "public, max-age=3600");

  const bool public_spa_fallback =
    config_bool(cfg, "public.spa_fallback", true);

  app.static_dir(
    public_path,
    public_mount,
    public_index,
    true,
    public_cache_control,
    true,
    public_spa_fallback
  );
}

int AppBootstrap::run()
{
  vix::config::Config cfg{".env"};
  vix::App app;

  configure_static_compression(cfg);
  configure_static_files(app, cfg);

  MiddlewareRegistry::register_all(app);
  RouteRegistry::register_all(app);

  app.run(cfg);
  return 0;
}
```

This file owns startup wiring.

It does four important things:

```txt id="cn8dk6"
loads .env
configures optional static compression hook
configures static files through app.static_dir(...)
registers middleware and routes
runs the app from config
```

## Static files are still Core App

This line serves static files:

```cpp id="xovxgx"
app.static_dir(...);
```

This line does not serve files:

```cpp id="gdrqmm"
vix::App::set_static_response_hook(...);
```

The hook only enhances static responses after Core App has produced them.

Keep the model clear:

```txt id="ja9xb8"
app.static_dir(...)
  serves public files

set_static_response_hook(...)
  optionally compresses eligible static responses

app.use(...)
  installs middleware for route handling
```

## src/MiddlewareRegistry.hpp

Create:

```txt id="j85wfc"
src/MiddlewareRegistry.hpp
```

Add:

```cpp id="d3mkgx"
#ifndef MIDDLEWARE_REGISTRY_HPP
#define MIDDLEWARE_REGISTRY_HPP

#include <vix.hpp>

class MiddlewareRegistry
{
public:
  static void register_all(vix::App &app);
};

#endif
```

## src/MiddlewareRegistry.cpp

Create:

```txt id="n2ostz"
src/MiddlewareRegistry.cpp
```

Add:

```cpp id="m6gv3i"
#include "MiddlewareRegistry.hpp"

#include <vix/middleware.hpp>

void MiddlewareRegistry::register_all(vix::App &app)
{
  app.use("/api", vix::middleware::app::recovery_dev());
  app.use("/api", vix::middleware::app::request_id_dev());
  app.use("/api", vix::middleware::app::timing_dev());

  app.use("/api", vix::middleware::app::security_headers_dev());

  app.use("/api", vix::middleware::app::cors_dev({
    "http://localhost:5173",
    "http://127.0.0.1:5173"
  }));

  app.use("/api", vix::middleware::app::rate_limit_custom_dev(
    60.0,
    1.0,
    "x-forwarded-for"
  ));

  app.use("/api", vix::middleware::app::body_limit_write_dev(
    1024 * 1024
  ));

  app.use("/api/products", vix::middleware::app::json_strict_dev(
    4096,
    false,
    true
  ));
}
```

This registry owns the API middleware stack.

The order is intentional:

```txt id="f7mpb4"
recovery
request id
timing
security headers
CORS
rate limit
body limit
JSON parser for product writes
```

The JSON parser is installed only on `/api/products`, not globally on `/api`.

That avoids rejecting API routes that do not need a request body.

## src/RouteRegistry.hpp

Create:

```txt id="m05pdz"
src/RouteRegistry.hpp
```

Add:

```cpp id="va4x8f"
#ifndef ROUTE_REGISTRY_HPP
#define ROUTE_REGISTRY_HPP

#include <vix.hpp>

class RouteRegistry
{
public:
  static void register_all(vix::App &app);
};

#endif
```

## src/RouteRegistry.cpp

Create:

```txt id="bn9q08"
src/RouteRegistry.cpp
```

Add:

```cpp id="yvc0l2"
#include "RouteRegistry.hpp"

#include <string>
#include <vector>

#include <vix/json.hpp>
#include <vix/middleware.hpp>

struct Product
{
  int id;
  std::string name;
  double price;
};

static std::vector<Product> products{
  {1, "Laptop", 999.99},
  {2, "Phone", 499.50}
};

static vix::json::Json product_to_json(const Product &product)
{
  using namespace vix::json;

  return o(
    "id", product.id,
    "name", product.name,
    "price", product.price
  );
}

static vix::json::Json products_to_json()
{
  using namespace vix::json;

  Json items = arr();

  for (const auto &product : products)
  {
    items.push_back(product_to_json(product));
  }

  return items;
}

void RouteRegistry::register_all(vix::App &app)
{
  app.get("/api/health", [](vix::Request &req, vix::Response &res)
  {
    auto *request_id =
      req.try_state<vix::middleware::basics::RequestId>();

    res.json({
      "ok", true,
      "service", "production-bootstrap",
      "request_id", request_id ? request_id->value : ""
    });
  });

  app.get("/api/products", [](vix::Request &, vix::Response &res)
  {
    using namespace vix::json;

    res.json(o(
      "ok", true,
      "products", products_to_json()
    ));
  });

  app.post("/api/products", [](vix::Request &req, vix::Response &res)
  {
    using namespace vix::json;

    auto &body =
      req.state<vix::middleware::parsers::JsonBody>();

    auto name = get_opt<std::string>(body.value, "name");
    const double price = get_or<double>(body.value, "price", 0.0);

    if (!name || name->empty())
    {
      res.status(422).json({
        "ok", false,
        "error", "Missing required field",
        "field", "name"
      });
      return;
    }

    if (price <= 0.0)
    {
      res.status(422).json({
        "ok", false,
        "error", "Price must be greater than zero",
        "field", "price"
      });
      return;
    }

    const int next_id = products.empty() ? 1 : products.back().id + 1;

    products.push_back(Product{
      next_id,
      *name,
      price
    });

    res.status(201).json(o(
      "ok", true,
      "product", product_to_json(products.back())
    ));
  });
}
```

Routes are now separated from startup code.

This makes the application easier to grow.

## CMakeLists.txt

Create:

```txt id="mzjils"
CMakeLists.txt
```

Add:

```cmake id="h0y8as"
cmake_minimum_required(VERSION 3.20)

project(production_bootstrap_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vix CONFIG REQUIRED)

add_executable(production_bootstrap_demo
  main.cpp
  src/AppBootstrap.cpp
  src/MiddlewareRegistry.cpp
  src/RouteRegistry.cpp
)

target_include_directories(production_bootstrap_demo
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(production_bootstrap_demo
  PRIVATE
    vix::vix
)
```

If your local Vix package exposes different CMake targets, use the target name installed by your Vix setup.

The example itself focuses on application structure.

## Run it with vix

From the project directory:

```bash id="b2xc31"
vix run main.cpp
```

For multi-file projects, use your normal Vix or CMake workflow.

A common CMake flow is:

```bash id="h0iv83"
cmake -S . -B build
cmake --build build
./build/production_bootstrap_demo
```

## Test the static site

Open:

```txt id="cgbczm"
http://127.0.0.1:8080/
```

Or use:

```bash id="k8az87"
curl -i http://127.0.0.1:8080/
```

Expected result:

```txt id="hgom61"
public/index.html
```

Test CSS:

```bash id="l6jncy"
curl -i http://127.0.0.1:8080/style.css
```

Test JavaScript:

```bash id="ry5qq9"
curl -i http://127.0.0.1:8080/app.js
```

## Test SPA fallback

Because `.env` has:

```dotenv id="kg7ola"
PUBLIC_SPA_FALLBACK=true
```

This should return `public/index.html`:

```bash id="p41j5y"
curl -i http://127.0.0.1:8080/dashboard
```

Use SPA fallback when your frontend has client-side routes.

Keep it false for a classic static site.

## Test API health

```bash id="mwhn1i"
curl -i http://127.0.0.1:8080/api/health
```

Expected body shape:

```json id="jg8d6d"
{
  "ok": true,
  "service": "production-bootstrap",
  "request_id": "..."
}
```

Expected headers may include:

```txt id="x2r67l"
x-request-id: ...
x-response-time: ...
server-timing: total;dur=...
X-Content-Type-Options: nosniff
X-Frame-Options: DENY
Referrer-Policy: no-referrer
Permissions-Policy: ...
```

## Test products list

```bash id="pqklwt"
curl -i http://127.0.0.1:8080/api/products
```

Expected body shape:

```json id="h49bby"
{
  "ok": true,
  "products": [
    {
      "id": 1,
      "name": "Laptop",
      "price": 999.99
    },
    {
      "id": 2,
      "name": "Phone",
      "price": 499.5
    }
  ]
}
```

## Create a product

```bash id="eyxocn"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":"Tablet","price":299.99}'
```

Expected status:

```txt id="mi7l96"
201 Created
```

Expected body shape:

```json id="n9l03p"
{
  "ok": true,
  "product": {
    "id": 3,
    "name": "Tablet",
    "price": 299.99
  }
}
```

## Test invalid JSON

```bash id="g0jgcx"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":}'
```

Expected status:

```txt id="lh4zz1"
400 Bad Request
```

The JSON parser rejects invalid JSON before the handler runs.

## Test validation

Missing name:

```bash id="nlww5z"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"price":299.99}'
```

Expected status:

```txt id="vqwo8b"
422 Unprocessable Entity
```

Invalid price:

```bash id="ahp93s"
curl -i \
  -X POST http://127.0.0.1:8080/api/products \
  -H "Content-Type: application/json" \
  -d '{"name":"Broken","price":0}'
```

Expected status:

```txt id="jhd1qg"
422 Unprocessable Entity
```

The parser validates JSON syntax.

The route validates business rules.

## Test CORS preflight

Allowed local frontend origin:

```bash id="noqciu"
curl -i \
  -X OPTIONS http://127.0.0.1:8080/api/products \
  -H "Origin: http://localhost:5173" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type"
```

Expected status:

```txt id="w4b8aa"
204 No Content
```

Blocked origin:

```bash id="lqmfrf"
curl -i \
  -X OPTIONS http://127.0.0.1:8080/api/products \
  -H "Origin: https://evil.com" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type"
```

Expected status:

```txt id="kde0vx"
403 Forbidden
```

## Why this structure matters

A small demo can put everything inside `main()`.

A real backend should not.

This is not ideal for growing projects:

```txt id="f6t9if"
main()
  load config
  configure static files
  install middleware
  register all routes
  start server
```

This is better:

```txt id="buvfot"
main()
  AppBootstrap::run()

AppBootstrap
  startup wiring

MiddlewareRegistry
  middleware stack

RouteRegistry
  routes
```

The result is easier to maintain.

## Production notes

For a real deployment:

```txt id="gp1h8e"
read secrets from environment or secure config
keep TLS termination clear
use Nginx or another reverse proxy when needed
control PUBLIC_CACHE_CONTROL carefully
enable PUBLIC_SPA_FALLBACK only for SPA frontends
enable PUBLIC_COMPRESSION only if Vix should compress static responses
avoid double compression if Nginx or CDN already compresses
keep API middleware separate from static file serving
```

Static files and API routes are different concerns.

```txt id="d0rbdz"
static files
  app.static_dir(...)

API middleware
  app.use("/api", ...)

routes
  app.get(...)
  app.post(...)
```

## Summary

A production-style Vix app should keep startup clean.

Recommended shape:

```txt id="oexb4f"
main.cpp
  calls AppBootstrap::run()

AppBootstrap
  configures App

MiddlewareRegistry
  installs middleware

RouteRegistry
  registers routes
```

Static files are configured with:

```cpp id="qz7e5a"
app.static_dir(...);
```

Static compression is optional and uses:

```cpp id="m2wv5w"
vix::App::set_static_response_hook(...);
```

API middleware is installed with:

```cpp id="sbinzi"
app.use("/api", ...);
```

This structure gives you a clean base for a real Vix backend.
