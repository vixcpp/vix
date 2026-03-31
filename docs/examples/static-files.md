# Static Files

This guide explains how to serve static assets (HTML, CSS, JS, images) in Vix.cpp.

You have two valid approaches:

1) `app.static_dir("./public")` (high-level App API)
2) `middleware::performance::static_files(...)` (explicit middleware, more control)

Both can be used in production. Prefer `app.static_dir()` for the simplest setup, and the middleware when you want explicit options or to mount multiple static roots.

---

## When to serve static files from Vix

Static files are useful for:
- Landing pages
- Simple dashboards
- OAuth connect pages
- Offline docs UI assets (Swagger UI offline)
- SPA bundles (React/Vue build output)
- Serving uploads in a controlled way (careful)

If you run a separate CDN or object storage (S3, R2, etc.), you often serve static assets there. But Vix can still serve them directly for small or offline deployments.

---

## Option A: The simplest way (recommended first)

### Serve a directory

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
  App app;

  // Serve ./public under /
  app.static_dir("./public");

  // API example
  app.get("/api/ping", [](Request&, Response& res){
    res.json({"ok", true});
  });

  app.run(8080);
}
```

### Serve individual files

```cpp
app.get("/", [](Request&, Response& res)
{
  res.file("./public/index.html");
});

app.get("/connect", [](Request&, Response& res)
{
  res.file("./public/connect.html");
});
```

What happens:
- `static_dir("./public")` mounts the directory on `/` by default.
- `res.file(path)` reads and returns a file, sets a best-effort Content-Type, and adds safe defaults like `X-Content-Type-Options: nosniff`.

---

## Option B: Explicit middleware (more control)

Your example uses the performance middleware:

```cpp
#include <vix.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/performance/static_files.hpp>

using namespace vix;

int main()
{
  App app;

  app.use(vix::middleware::app::adapt_ctx(
      vix::middleware::performance::static_files(
          "./public",
          {
              .mount = "/",
              .index_file = "index.html",
              .add_cache_control = true,
              .cache_control = "public, max-age=3600",
              .fallthrough = true,
          })));

  app.get("/api/ping", [](Request&, Response& res){ res.json({"ok", true}); });

  app.run(8080);
}
```

Meaning of the options:
- `mount`: URL prefix where static assets are exposed.
- `index_file`: returned when the path points to a directory.
- `add_cache_control`: whether to add Cache-Control when missing.
- `cache_control`: Cache-Control value to set.
- `fallthrough`: if true, missing static file continues to the next route (useful for SPA routing).

---

## SPA routing pattern (React/Vue)

For a SPA, routes like `/app/settings` should usually return `index.html` so the frontend router can handle it.

Use this clean pattern:

- Serve static assets under `/` (or `/app`)
- Keep API under `/api`
- Enable fallthrough so unknown paths can be handled by a final route that serves `index.html`

Example shape:

```cpp
App app;

// or mount "/app" depending on your app layout
app.static_dir("./public", "/");

app.get("/api/ping", [](Request&, Response& res){
  res.json({"ok", true});
});

// Final SPA fallback (only if your static serving does not already do it)
app.get("/app/*", [](Request&, Response& res){
  res.file("./public/index.html");
});

app.run(8080);
```

If your static middleware already supports index fallback with `fallthrough=true`,
you can keep the fallback route minimal or skip it depending on behavior.

---

## Caching and production defaults

### Good defaults for static assets
- HTML: low cache or no-cache (because it changes and references hashed assets)
- Hashed assets (app.9f3a2.js): long cache (`max-age=31536000, immutable`)
- Images/fonts: moderate to long cache depending on naming strategy

If you do not use hashed filenames, keep cache short to avoid clients getting stale files.

### Recommended approach
- Use `Cache-Control` for static assets
- Use `ETag` middleware if you want conditional GET and 304 responses

ETag is especially nice when you keep cache short but still want bandwidth savings.

---

## Security notes

Static serving is a common source of bugs. Keep these rules:

- Do not allow path traversal (`..`) to escape your root directory.
- If you serve uploads, put them in a dedicated directory with strict rules.
- Consider disabling directory listing (should be disabled by default).
- Set `X-Content-Type-Options: nosniff`.
- In production, add security headers (CSP, etc.) for HTML pages if you serve app UI.

If you serve API and static from the same origin,
you can still apply security headers only on `/api` or only on UI routes depending on your needs.

---

## Quick curl tests

```bash
# Fetch index
curl -i http://localhost:8080/

# Fetch a text file
curl -i http://localhost:8080/hello.txt

# API is still available
curl -i http://localhost:8080/api/ping
```

---

## What to choose

Use `app.static_dir()` when:
- You want the most beginner-friendly setup
- One static root is enough
- You want to keep code short

Use `static_files(...)` middleware when:
- You want explicit caching options per mount
- You want multiple mounts (ex: /docs, /assets, /app)
- You want clear fallthrough behavior for SPA routing

---

## Next production step

If your app serves both UI and API, the production structure usually looks like:

- `/api/*` protected by middleware (auth, CORS, CSRF, security headers)
- `/docs/*` (optional) offline Swagger UI
- `/` (or `/app`) served as static SPA assets

This keeps your server architecture predictable and clean.

