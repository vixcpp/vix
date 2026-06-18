# Static files

This page shows how to serve static files with Vix Core.

Use it when you want to expose assets such as HTML, CSS, JavaScript, images, icons, fonts, or public files from a directory.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the app header directly:

```cpp
#include <vix/app/App.hpp>
```

## What static files provide

Static files let a Vix application serve files from the filesystem.

Use static files for:

- HTML files
- CSS files
- JavaScript files
- images
- icons
- fonts
- downloads
- frontend assets
- public folders

Static files are mounted with `app.static_dir(...)`.

```cpp
app.static_dir("public", "/");
```

## Basic static directory

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.static_dir("public", "/");

  app.run(8080);

  return 0;
}
```

With this structure:

```text
public/
  index.html
  style.css
  app.js
```

These requests can be served:

```text
GET /index.html
GET /style.css
GET /app.js
```

## Mount under a prefix

Mount a directory under a URL prefix.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.static_dir("public/assets", "/assets");

  app.run(8080);

  return 0;
}
```

With this structure:

```text
public/
  assets/
    style.css
    logo.svg
```

These requests can be served:

```text
GET /assets/style.css
GET /assets/logo.svg
```

## Static directory signature

`static_dir` accepts:

```cpp
app.static_dir(
    root,
    mount,
    index_file,
    add_cache_control,
    cache_control,
    fallthrough);
```

Arguments:

| Argument            | Purpose                                                       |
| ------------------- | ------------------------------------------------------------- |
| `root`              | Filesystem directory to serve.                                |
| `mount`             | URL prefix where files are exposed.                           |
| `index_file`        | File used when a directory is requested.                      |
| `add_cache_control` | Whether to add a `Cache-Control` header.                      |
| `cache_control`     | Cache-Control header value.                                   |
| `fallthrough`       | Whether missing files should continue to normal 404 handling. |

## Default options

The default call:

```cpp
app.static_dir("public", "/");
```

is equivalent to:

```cpp
app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true);
```

## Root directory

The first argument is the filesystem directory.

```cpp
app.static_dir("public", "/");
```

Example:

```text
public/style.css
```

can be served as:

```text
GET /style.css
```

## Mount path

The second argument is the URL mount point.

```cpp
app.static_dir("public/assets", "/assets");
```

Example:

```text
public/assets/app.css
```

can be served as:

```text
GET /assets/app.css
```

## Index file

When the requested path maps to a directory, Vix can serve an index file.

```cpp
app.static_dir("public", "/", "index.html");
```

Request:

```text
GET /
```

can serve:

```text
public/index.html
```

Request:

```text
GET /docs
```

can serve:

```text
public/docs/index.html
```

when `public/docs` is a directory.

## Cache-Control

By default, static files can receive a cache header.

```cpp
app.static_dir(
    "public",
    "/assets",
    "index.html",
    true,
    "public, max-age=3600",
    true);
```

This can add:

```text
Cache-Control: public, max-age=3600
```

Disable automatic cache control:

```cpp
app.static_dir(
    "public",
    "/assets",
    "index.html",
    false,
    "",
    true);
```

## Fallthrough

The `fallthrough` option controls what happens when a file is not found.

When `fallthrough` is `true`, missing files continue to the normal not-found handling.

```cpp
app.static_dir(
    "public",
    "/assets",
    "index.html",
    true,
    "public, max-age=3600",
    true);
```

When `fallthrough` is `false`, Vix returns a static-file 404 immediately.

```cpp
app.static_dir(
    "public",
    "/assets",
    "index.html",
    true,
    "public, max-age=3600",
    false);
```

## Static files and routing

Static files are integrated through the not-found fallback.

The request flow is:

```text
request
  -> router
  -> route found?
      yes -> run route handler
      no  -> try static files
              found -> send file
              missing -> default 404
```

This means explicit routes have priority over static files.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("home route");
});

app.static_dir("public", "/");
```

For:

```text
GET /
```

the route handler runs first.

## Serve one file from a handler

Use `res.file(...)` when you want to send a single file from a route.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/download", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.file("public/files/report.pdf");
  });

  app.run(8080);

  return 0;
}
```

Use `static_dir(...)` when you want to expose a whole directory.

```cpp
app.static_dir("public", "/assets");
```

## Serve a frontend app

A common setup is to mount a built frontend directory.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.static_dir("dist", "/");

  app.run(8080);

  return 0;
}
```

With:

```text
dist/
  index.html
  assets/
    app.js
    style.css
```

Requests can resolve to:

```text
GET /
GET /assets/app.js
GET /assets/style.css
```

## Serve assets under /assets

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.static_dir("public/assets", "/assets");

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.file("public/index.html");
  });

  app.run(8080);

  return 0;
}
```

Example mapping:

```text
GET /assets/logo.svg
```

serves:

```text
public/assets/logo.svg
```

## HEAD requests

Static file serving supports `GET` and `HEAD`.

For `HEAD`, Vix resolves the file but sends the response without the file body.

```text
HEAD /assets/style.css
```

This is useful for clients that only need headers.

## Supported MIME types

Vix detects common MIME types from file extensions.

| Extension | Content-Type                            |
| --------- | --------------------------------------- |
| `.html`   | `text/html; charset=utf-8`              |
| `.css`    | `text/css; charset=utf-8`               |
| `.js`     | `application/javascript; charset=utf-8` |
| `.json`   | `application/json; charset=utf-8`       |
| `.png`    | `image/png`                             |
| `.jpg`    | `image/jpeg`                            |
| `.jpeg`   | `image/jpeg`                            |
| `.gif`    | `image/gif`                             |
| `.svg`    | `image/svg+xml`                         |
| `.ico`    | `image/x-icon`                          |
| `.txt`    | `text/plain; charset=utf-8`             |
| `.woff`   | `font/woff`                             |
| `.woff2`  | `font/woff2`                            |

Unknown extensions use:

```text
application/octet-stream
```

## Path safety

Static file serving performs basic path traversal checks.

A request path that resolves to a path containing `..` is rejected.

```text
GET /assets/../../secret.txt
```

This returns a bad request response instead of serving the file.

## Directory requests

When a request maps to a directory, Vix appends the configured index file.

```cpp
app.static_dir("public", "/", "index.html");
```

Request:

```text
GET /docs
```

can map to:

```text
public/docs/index.html
```

## Missing file

When a file does not exist and `fallthrough` is enabled:

```cpp
app.static_dir("public", "/assets", "index.html", true, "public, max-age=3600", true);
```

Request:

```text
GET /assets/missing.css
```

continues to the default not-found handler.

When `fallthrough` is disabled:

```cpp
app.static_dir("public", "/assets", "index.html", true, "public, max-age=3600", false);
```

Vix returns a static-file `404`.

## Multiple static mounts

You can register multiple static directories.

```cpp
app.static_dir("public/assets", "/assets");
app.static_dir("public/uploads", "/uploads");
app.static_dir("public/docs", "/docs");
```

Example mappings:

```text
GET /assets/app.css   -> public/assets/app.css
GET /uploads/a.png    -> public/uploads/a.png
GET /docs/index.html  -> public/docs/index.html
```

## Static files with middleware

Middleware can be used with mounted static paths.

```cpp
app.use("/assets", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  res.header("X-Static", "true");

  next();
});

app.static_dir("public/assets", "/assets");
```

This can add headers before the static fallback writes the file response.

## Static files with cache headers

```cpp
app.static_dir(
    "public/assets",
    "/assets",
    "index.html",
    true,
    "public, max-age=86400",
    true);
```

This is useful for versioned assets.

```text
/assets/app.3f2a9.js
/assets/style.a91c2.css
```

## Static files without cache

```cpp
app.static_dir(
    "public",
    "/",
    "index.html",
    false,
    "",
    true);
```

This is useful during development.

## Static files and templates

Static files and templates solve different problems.

Use static files for files that already exist on disk:

```cpp
app.static_dir("public", "/assets");
```

Use templates when a response should be rendered dynamically:

```cpp
app.templates("views");

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Home");

  res.render("index.html", ctx);
});
```

## Static files and API routes

You can mix API routes and static files.

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok"
    });
  });

  app.static_dir("public", "/");

  app.run(8080);

  return 0;
}
```

Example:

```text
GET /api/status
```

runs the route.

```text
GET /style.css
```

can serve a file from `public/style.css`.

## Complete example

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)req;

    res.header("X-Powered-By", "Vix.cpp");

    next();
  });

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok"
    });
  });

  app.get("/download", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.file("public/files/readme.txt");
  });

  app.static_dir(
      "public/assets",
      "/assets",
      "index.html",
      true,
      "public, max-age=3600",
      true);

  app.static_dir(
      "public",
      "/",
      "index.html",
      false,
      "",
      true);

  app.run(8080);

  return 0;
}
```

Example requests:

```text
GET /api/status
GET /download
GET /assets/style.css
GET /
```

## API summary

| API                                                                                      | Purpose                                          |
| ---------------------------------------------------------------------------------------- | ------------------------------------------------ |
| `app.static_dir(root)`                                                                   | Mount a static directory at `/`.                 |
| `app.static_dir(root, mount)`                                                            | Mount a static directory at a URL prefix.        |
| `app.static_dir(root, mount, index_file, add_cache_control, cache_control, fallthrough)` | Mount static files with explicit options.        |
| `res.file(path)`                                                                         | Send one file from a route handler.              |
| `res.header(name, value)`                                                                | Set response headers before sending a file.      |
| `res.type(mime)`                                                                         | Set a custom content type when sending manually. |

## Best practices

Use `/assets` for frontend assets.

```cpp
app.static_dir("public/assets", "/assets");
```

Use `/` for a simple public directory.

```cpp
app.static_dir("public", "/");
```

Use cache headers for versioned assets.

```cpp
app.static_dir("dist/assets", "/assets", "index.html", true, "public, max-age=31536000", true);
```

Disable cache headers during development.

```cpp
app.static_dir("public", "/", "index.html", false, "", true);
```

Use explicit routes for APIs.

```cpp
app.get("/api/status", handler);
```

Use `res.file(...)` for one-off downloads.

```cpp
res.file("public/files/report.pdf");
```

Avoid exposing sensitive directories.

```cpp
app.static_dir("public", "/");
```

Do not mount project roots, home directories, secrets, or build folders that contain private files.

## Next steps

Read the next pages:

- [Templates](./templates.md)
- [Response](./response.md)
- [Routing](./routing.md)
- [Middleware](./middleware.md)
- [HTTP server](./http-server.md)
