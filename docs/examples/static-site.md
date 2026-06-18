# Static Site

This example shows how to serve public files with `vix::App`.

Static files are handled by Core App, not by middleware.

Use this example when you want to serve:

```txt id="ec22i7"
HTML
CSS
JavaScript
images
SPA frontend files
public assets
```

The important API is:

```cpp id="v2p1e9"
app.static_dir(...);
```

Middleware can enhance static responses, for example by adding optional compression, but static file serving itself belongs to `vix::App`.

## What this example builds

The app serves files from:

```txt id="ls0tcc"
public/
```

Mounted at:

```txt id="fmsp70"
/
```

With:

```txt id="x7sim9"
index.html as the default file
Cache-Control for browser caching
optional SPA fallback
optional static compression hook
```

The app also exposes:

```txt id="dw8py8"
GET /api/health
```

This lets you serve a frontend and an API from the same Vix app.

## Project structure

Create this structure:

```txt id="oym7kv"
static_site_demo/
├── static_site.cpp
└── public/
    ├── index.html
    ├── app.js
    └── style.css
```

## public/index.html

Create:

```txt id="l6fuc4"
public/index.html
```

Add:

```html id="jm0awy"
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Vix Static Site</title>
    <link rel="stylesheet" href="/style.css" />
  </head>
  <body>
    <main class="page">
      <h1>Hello from Vix</h1>
      <p>This page is served from the public directory.</p>

      <button id="health-button">Check API health</button>

      <pre id="output">Click the button.</pre>
    </main>

    <script src="/app.js"></script>
  </body>
</html>
```

## public/style.css

Create:

```txt id="xge3sn"
public/style.css
```

Add:

```css id="v7s0rg"
body {
  margin: 0;
  font-family: system-ui, sans-serif;
  background: #f6f7f9;
  color: #111827;
}

.page {
  max-width: 720px;
  margin: 80px auto;
  padding: 32px;
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 16px;
}

button {
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

```txt id="te1qzn"
public/app.js
```

Add:

```js id="yo230t"
const button = document.querySelector("#health-button");
const output = document.querySelector("#output");

button.addEventListener("click", async () => {
  const response = await fetch("/api/health");
  const data = await response.json();

  output.textContent = JSON.stringify(data, null, 2);
});
```

## static_site.cpp

Create:

```txt id="rfyown"
static_site.cpp
```

Add:

```cpp id="oplizi"
#include <vix.hpp>

using namespace vix;

static void register_static_files(App &app)
{
  app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true,
    false
  );
}

static void register_routes(App &app)
{
  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "static-site"
    });
  });
}

int main()
{
  App app;

  register_static_files(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

## Run it

From the project directory:

```bash id="nl6xio"
vix run static_site.cpp
```

Open:

```txt id="zz5xi1"
http://127.0.0.1:8080/
```

Or test with `curl`:

```bash id="r0a6h4"
curl -i http://127.0.0.1:8080/
```

Expected response:

```txt id="x05ajb"
200 OK
```

The body should be the contents of `public/index.html`.

## Test static assets

CSS:

```bash id="wltd7g"
curl -i http://127.0.0.1:8080/style.css
```

JavaScript:

```bash id="ywydtc"
curl -i http://127.0.0.1:8080/app.js
```

Both files are served from the `public/` directory.

## Test API route

```bash id="ey0u3e"
curl -i http://127.0.0.1:8080/api/health
```

Expected body shape:

```json id="h5k6ch"
{
  "ok": true,
  "service": "static-site"
}
```

This shows that the same app can serve static files and dynamic API routes.

## Understanding `app.static_dir(...)`

The example uses:

```cpp id="g76f8s"
app.static_dir(
  "public",
  "/",
  "index.html",
  true,
  "public, max-age=3600",
  true,
  false
);
```

The arguments are:

```txt id="o6wjzf"
public directory
mount path
index file
enable index file
Cache-Control value
allow fallthrough
SPA fallback
```

In this example:

```txt id="h4zsmv"
public
  files are read from ./public

/
  files are mounted at the root URL

index.html
  / returns public/index.html

public, max-age=3600
  browser cache header for public assets

SPA fallback false
  unknown paths do not automatically return index.html
```

## Static files are not middleware

This is Core App behavior:

```cpp id="xnq5cy"
app.static_dir(...);
```

This is middleware behavior:

```cpp id="c8f96c"
app.use(...);
```

Keep the distinction clear:

```txt id="b02tzh"
app.static_dir(...)
  serves files from disk

app.use(...)
  installs route middleware

App::set_static_response_hook(...)
  optionally modifies static responses after Core writes them
```

Static files are served before your route handler is needed.

Middleware can still be used for API routes, but static file serving itself is not a middleware feature.

## Serve a SPA

For a Single Page Application, enable SPA fallback.

```cpp id="hdfn9l"
app.static_dir(
  "public",
  "/",
  "index.html",
  true,
  "public, max-age=3600",
  true,
  true
);
```

The last argument is:

```txt id="kp031a"
SPA fallback = true
```

With SPA fallback enabled, paths such as:

```txt id="rbq3vm"
/dashboard
/settings
/products/123
```

can return:

```txt id="a2znbm"
public/index.html
```

This is useful for frontend routers.

Example:

```cpp id="h25w5h"
static void register_static_files(App &app)
{
  app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true,
    true
  );
}
```

Use SPA fallback only when your frontend needs client-side routing.

For normal static sites, keep it false.

## Static site with middleware for API routes

Static files do not require middleware, but API routes often do.

Example:

```cpp id="a8u1xd"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static void register_static_files(App &app)
{
  app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true,
    true
  );
}

static void register_api_middleware(App &app)
{
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());
  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::rate_limit_dev());
}

static void register_routes(App &app)
{
  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "static-site"
    });
  });
}

int main()
{
  App app;

  register_static_files(app);
  register_api_middleware(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

This is a common shape:

```txt id="ac09et"
static files
  served by Core App

/api routes
  protected by middleware
```

## Enable static compression

Static compression is optional.

It uses a static response hook from the middleware performance module.

```cpp id="xrrubx"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

int main()
{
  App app;

  vix::middleware::performance::CompressionOptions options{
    .min_size = 1024,
    .add_vary = true,
    .enabled = true
  };

  vix::App::set_static_response_hook(
    vix::middleware::performance::compressed_static_response_hook(options)
  );

  app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true,
    true
  );

  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true
    });
  });

  app.run(8080);
}
```

This line does not serve files:

```cpp id="v2pgsl"
vix::App::set_static_response_hook(...);
```

It only enhances eligible static responses after Core has produced them.

The file serving still comes from:

```cpp id="tuafzf"
app.static_dir(...);
```

## Test static compression

Request with gzip support:

```bash id="g2484s"
curl -i \
  http://127.0.0.1:8080/app.js \
  -H "Accept-Encoding: gzip"
```

Depending on the build configuration and compression support, eligible responses can include:

```txt id="ogn63c"
Content-Encoding: gzip
Vary: Accept-Encoding
```

Small files may not be compressed because of `min_size`.

Already compressed assets should generally not be compressed again.

## Dynamic compression is different

This compresses dynamic route responses:

```cpp id="cy2j17"
app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::performance::compression(options)
));
```

This compresses eligible static file responses:

```cpp id="su9bu9"
vix::App::set_static_response_hook(
  vix::middleware::performance::compressed_static_response_hook(options)
);
```

They are different paths.

Use both only when you want both dynamic route compression and static response compression.

## Configuration-driven static site

In a generated or production-style app, static behavior can come from configuration.

Example `.env` values:

```dotenv id="f13qyp"
PUBLIC_PATH=public
PUBLIC_MOUNT=/
PUBLIC_INDEX=index.html
PUBLIC_CACHE_CONTROL=public, max-age=3600
PUBLIC_SPA_FALLBACK=true
PUBLIC_COMPRESSION=false
PUBLIC_COMPRESSION_MIN_SIZE=1024
```

Bootstrap code can wire those values:

```cpp id="wka3mx"
const std::string publicPath =
  cfg.getString("public.path", "public");

const std::string publicMount =
  cfg.getString("public.mount", "/");

const std::string publicIndex =
  cfg.getString("public.index", "index.html");

const std::string publicCacheControl =
  cfg.getString("public.cache_control", "public, max-age=3600");

const bool publicSpaFallback =
  cfg.getBool("public.spa_fallback", false);

app.static_dir(
  publicPath,
  publicMount,
  publicIndex,
  true,
  publicCacheControl,
  true,
  publicSpaFallback
);
```

If static compression is enabled:

```cpp id="rxm394"
const bool publicCompression =
  cfg.getBool("public.compression", false);

const int publicCompressionMinSize =
  cfg.getInt("public.compression_min_size", 1024);

if (publicCompression)
{
  vix::middleware::performance::CompressionOptions options{
    .min_size = static_cast<std::size_t>(publicCompressionMinSize),
    .add_vary = true,
    .enabled = true
  };

  vix::App::set_static_response_hook(
    vix::middleware::performance::compressed_static_response_hook(options)
  );
}
```

This keeps static behavior controlled by configuration instead of hardcoding every value.

## Cache-Control

The static directory call can set a `Cache-Control` value:

```cpp id="e8vj6r"
"public, max-age=3600"
```

That means browsers and caches may reuse files for a period of time.

For development, you may prefer a short cache:

```txt id="ytpml1"
no-cache
```

For production assets with hashed filenames, you may prefer a longer cache:

```txt id="qdopw5"
public, max-age=31536000, immutable
```

Use a policy that matches how your frontend assets are built.

If filenames change when content changes, longer caching is safer.

If filenames do not change, keep caching shorter.

## Static site plus API example

Here is a complete version with static files, API middleware, SPA fallback, and optional static compression.

```cpp id="t6pi70"
#include <vix.hpp>
#include <vix/middleware.hpp>

using namespace vix;

static void configure_static_compression(bool enabled)
{
  if (!enabled)
    return;

  vix::middleware::performance::CompressionOptions options{
    .min_size = 1024,
    .add_vary = true,
    .enabled = true
  };

  vix::App::set_static_response_hook(
    vix::middleware::performance::compressed_static_response_hook(options)
  );
}

static void register_static_files(App &app)
{
  app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true,
    true
  );
}

static void register_api_middleware(App &app)
{
  app.use("/api", middleware::app::request_id_dev());
  app.use("/api", middleware::app::timing_dev());
  app.use("/api", middleware::app::security_headers_dev());
  app.use("/api", middleware::app::rate_limit_dev());
}

static void register_routes(App &app)
{
  app.get("/api/health", [](Request &, Response &res)
  {
    res.json({
      "ok", true,
      "service", "static-site"
    });
  });
}

int main()
{
  App app;

  configure_static_compression(false);
  register_static_files(app);
  register_api_middleware(app);
  register_routes(app);

  app.run(8080);
  return 0;
}
```

## Complete test flow

Run:

```bash id="ne2rgm"
vix run static_site.cpp
```

Home page:

```bash id="rpi24t"
curl -i http://127.0.0.1:8080/
```

Static CSS:

```bash id="yez2ww"
curl -i http://127.0.0.1:8080/style.css
```

Static JS:

```bash id="tnxqsm"
curl -i http://127.0.0.1:8080/app.js
```

API health:

```bash id="qfjgmx"
curl -i http://127.0.0.1:8080/api/health
```

SPA fallback test, only when enabled:

```bash id="ut21b8"
curl -i http://127.0.0.1:8080/dashboard
```

Expected result with SPA fallback enabled:

```txt id="ckd0vd"
public/index.html
```

## Summary

Use `app.static_dir(...)` to serve public files.

```cpp id="s4t178"
app.static_dir(
  "public",
  "/",
  "index.html",
  true,
  "public, max-age=3600",
  true,
  true
);
```

Remember the separation:

```txt id="y9yb8r"
Core App
  serves static files

Middleware
  protects API routes
  can optionally enhance static responses

Static response hook
  can compress eligible static files
```

A good structure is:

```txt id="yq9vvp"
register static files
register API middleware
register API routes
run app
```

Static files are not middleware.

They are a core `vix::App` feature.
