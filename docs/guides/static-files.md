# Static files

This guide shows how to serve static files with Vix.

## What you will build

```txt
GET /           → public/index.html
GET /health     → JSON
GET /api/ping   → JSON
GET /style.css  → CSS
GET /app.js     → JavaScript
```

## Setup

```bash
vix new static-files-app
cd static-files-app
mkdir -p public
```

## Serve a single file

```cpp
app.get("/", [](Request &, Response &res) {
  res.file("public/index.html");
});
```

## Serve a full directory

```cpp
app.static_dir("public");
```

This serves `/index.html`, `/style.css`, `/app.js` etc.

## Static files plus API routes

```cpp
int main()
{
  App app;

  app.static_dir("public");

  app.get("/", [](Request &, Response &res) {
    res.file("public/index.html");
  });

  app.get("/health", [](Request &, Response &res) {
    res.json({"ok", true});
  });

  app.get("/api/ping", [](Request &, Response &res) {
    res.json({
      "ok", true,
      "message", "pong"
    });
  });

  app.run(8080);

  return 0;
}
```

## Manual wildcard static route

```cpp
app.get("/*", [](Request &req, Response &res){
  std::string path = "public" + req.path();
  res.header("Cache-Control", "public, max-age=86400");
  res.file(path);
});
```

## Cache headers

| Header value                            | Meaning                                      |
|-----------------------------------------|----------------------------------------------|
| `no-store`                              | Does not store the response in any cache.    |
| `no-cache`                              | Revalidates the response before reuse.       |
| `public, max-age=3600`                  | Caches the response for one hour.            |
| `public, max-age=86400`                 | Caches the response for one day.             |
| `public, max-age=31536000, immutable`   | Caches versioned assets for one year.        |

## Advanced static files middleware

```cpp
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/performance/static_files.hpp>

app.use(vix::middleware::app::adapt_ctx(
  vix::middleware::performance::static_files(
    std::filesystem::path{"public"},
    {
      .mount = "/",
      .index_file = "index.html",
      .add_cache_control = true,
      .cache_control = "public, max-age=3600",
      .fallthrough = true,
    }
  )
));
```

## SPA fallback

```cpp
app.get("/*", [](Request &req, Response &res){
  const std::string path = req.path();
  if (path.rfind("/api/", 0) == 0)
  {
    res.status(404).json({
      "ok", false,
      "error", "api route not found"
    });
    return;
  }

  res.file("public/index.html");
});
```

## Production with Nginx

```nginx
location /api/ {
    proxy_pass http://127.0.0.1:8080;
}

location / {
    try_files $uri $uri/ /index.html;
}
```

## Common mistakes

### Wildcard route catches API routes

```cpp
// Wrong order
app.get("/*", serve_static);
app.get("/api/ping", api_ping);

// Correct order
app.get("/api/ping", api_ping);
app.get("/*", spa_fallback);
```

### Caching `index.html` too aggressively

Use `no-cache` or a short `max-age` for `index.html`. Reserve long immutable caching for versioned assets.

### Serving user uploads without validation

Validate file paths to prevent directory traversal attacks.

## Recommended structure

```txt
project/
├── src/main.cpp
└── public/
    ├── index.html
    ├── style.css
    └── app.js
```

## What to use next

- [Templates guide](/guides/templates)
- [WebSocket chat guide](/guides/websocket-chat)
- [Production Nginx + systemd guide](/guides/production-nginx-systemd)
