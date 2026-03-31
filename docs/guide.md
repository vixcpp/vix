# Routes and Middleware

This guide explains how to write routes and attach middleware in Vix.cpp, using the style shown in:

- `vix/examples/vix_routes_showcase.cpp` (routing patterns)
- `vix/examples/http_middleware/mega_middleware_routes.cpp` (middleware patterns)

Audience: beginners coming from Node.js (Express, Fastify) or Python (Flask, FastAPI).

---

## 1) Quick start

Run an example file directly:

```bash
vix run examples/vix_routes_showcase.cpp
```

Then test:

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
curl -i http://127.0.0.1:8080/users/42
curl -i "http://127.0.0.1:8080/search?q=vix&page=2&limit=5"
```

Middleware mega example:

```bash
vix run examples/http_middleware/mega_middleware_routes.cpp
```

---

## 2) Core mental model

### Routes
A route is `(method, path) -> handler`.

You write routes on `App`:

```cpp
App app;

app.get("/ping", [](Request&, Response& res){
  res.json({"ok", true});
});

app.run(8080);
```

### Middleware
A middleware runs before (and sometimes after) a route handler. It can:

- read request headers/body
- add headers to the response
- store data in `RequestState` (type-based request storage)
- block the request early (return 401/403/400)
- call `next()` to continue to the next middleware/handler

You can install middleware globally or for a path prefix.

---

## 3) Routes

### 3.1 Minimal route

```cpp
app.get("/", [](Request&, Response& res){
  res.json({"message", "Hello, Vix!"});
});
```

### 3.2 Return values vs using `res`

Vix supports two styles:

#### Style A: explicit `res.*`
You fully control the response:

```cpp
app.get("/status/created", [](Request&, Response& res){
  res.status(201).json({"status", 201, "message", "Created"});
});
```

#### Style B: return payload (auto-send)
Returning a value sends it automatically:

```cpp
app.get("/txt", [](const Request&, Response&){
  return "Hello world";
});

app.get("/hello", [](const Request&, Response&){
  return vix::json::o("message", "Hello", "id", 20);
});
```

Important: if you already sent a response using `res`, the returned value is ignored:

```cpp
app.get("/mix", [](Request&, Response& res){
  res.status(201).send("Created");
  return vix::json::o("ignored", true); // ignored
});
```

### 3.3 Path parameters

Use `{name}` in the path and read it with `req.param()`.

```cpp
app.get("/users/{id}", [](Request& req, Response& res){
  const std::string id = req.param("id", "0");

  if (id == "0") {
    res.status(404).json({"error", "User not found", "id", id});
    return;
  }

  res.json({"id", id, "name", "User#" + id, "vip", (id == "42")});
});
```

Multiple parameters:

```cpp
app.get("/posts/{year}/{slug}", [](Request& req, Response& res){
  res.json({
    "year", req.param("year", "2026"),
    "slug", req.param("slug", "hello")
  });
});
```

### 3.4 Query parameters

Read query values with `req.query_value(key, fallback)`:

```cpp
app.get("/search", [](Request& req, Response& res){
  const std::string q = req.query_value("q", "");
  const int page = to_int_or(req.query_value("page", "1"), 1);
  const int limit = to_int_or(req.query_value("limit", "10"), 10);

  res.json({"q", q, "page", page, "limit", limit});
});
```

### 3.5 Headers

Read headers:

```cpp
app.get("/headers", [](Request& req, Response& res){
  res.json({
    "host", req.header("Host"),
    "user_agent", req.header("User-Agent"),
    "accept", req.header("Accept")
  });
});
```

Check presence:

```cpp
app.get("/auth/check", [](Request& req, Response& res){
  res.json({"has_authorization", req.has_header("Authorization")});
});
```

### 3.6 Request body and JSON

Raw body:

```cpp
app.post("/echo/body", [](Request& req, Response& res){
  const std::string body = req.body();
  res.json({"bytes", (long long)body.size(), "body", body});
});
```

Parsed JSON (when JSON parsing middleware is installed, or when `req.json()` is available):

```cpp
app.post("/echo/json/fields", [](Request& req, Response& res){
  const auto& j = req.json();

  std::string name = "unknown";
  bool vip = false;

  if (j.is_object()) {
    if (j.contains("name") && j["name"].is_string()) name = j["name"].get<std::string>();
    if (j.contains("vip") && j["vip"].is_boolean()) vip = j["vip"].get<bool>();
  }

  res.json({"name", name, "vip", vip, "raw", j});
});
```

### 3.7 Status codes

```cpp
app.get("/status/{code}", [](Request& req, Response& res){
  const int code = to_int_or(req.param("code", "200"), 200);
  res.status(code).json({
    "status", code,
    "ok", (code >= 200 && code < 300)
  });
});
```

---

## 4) Organizing routes (recommended pattern)

Keep `main()` clean. Put registration in functions:

```cpp
static void register_api_routes(App& app) {
  app.get("/api/ping", [](Request&, Response& res){
    res.json({"ok", true});
  });
}

static int run_server() {
  App app;
  register_api_routes(app);
  app.run(8080);
  return 0;
}

int main() { return run_server(); }
```

This scales well when your examples become a real service.

---

## 5) Middleware

Vix supports two middleware styles:

1) Context-based middleware (recommended): works with `Context` and `Next`.
2) Legacy HTTP middleware: works with `Request`, `Response`, `Next`.

Adapters exist so you can install both types into the App pipeline.

### 5.1 Context-based middleware (recommended)

Signature:

```cpp
[](vix::middleware::Context& ctx, vix::middleware::Next next) {
  // read ctx.req()
  // write ctx.res()
  next();
}
```

Example: request id middleware storing a value in request state and setting a header:

```cpp
static vix::middleware::MiddlewareFn mw_request_id() {
  return [](vix::middleware::Context& ctx, vix::middleware::Next next) {
    RequestId rid;
    rid.value = "..." ; // generate id

    ctx.req().emplace_state<RequestId>(std::move(rid));
    ctx.res().header("x-request-id", ctx.req().state<RequestId>().value);

    next();
  };
}
```

Install globally with the adapter:

```cpp
using namespace vix::middleware::app;

app.use(adapt_ctx(mw_request_id()));
```

### 5.2 Legacy HttpMiddleware

Signature:

```cpp
[](vix::Request& req, vix::Response& res, vix::middleware::Next next) {
  next();
}
```

Example: require a header:

```cpp
static vix::middleware::HttpMiddleware mw_require_header(std::string header, std::string expected) {
  return [header = std::move(header), expected = std::move(expected)](
    vix::Request& req, vix::Response& res, vix::middleware::Next next
  ) mutable {
    const std::string got = req.header(header);
    if (got != expected) {
      res.status(401).json({"ok", false, "error", "unauthorized"});
      return;
    }
    next();
  };
}
```

Install it on a path using `adapt()`:

```cpp
using namespace vix::middleware::app;

install_exact(app, "/api/ping", adapt(mw_require_header("x-demo", "1")));
```

---

## 6) Where middleware runs

You typically install middleware in three ways:

### 6.1 Global middleware (runs for all routes)

```cpp
app.use(cors_dev());
app.use(security_headers_dev(false));
```

Good for: CORS, security headers, logging, request id, metrics.

### 6.2 Prefix middleware (runs only for routes under a prefix)

```cpp
using namespace vix::middleware::app;

install(app, "/api/", rate_limit_dev(120, std::chrono::minutes(1)));
install(app, "/api/secure/", api_key_dev("dev_key_123"));
```

Good for: auth on a whole API group, rate limiting, caching.

### 6.3 Exact middleware (runs only for a single route)

```cpp
install_exact(app, "/api/echo/json", json_dev(1024, true, true));
install_exact(app, "/api/echo/form", form_dev(1024, true));
```

Good for: parsers and special checks that should not affect other endpoints.

---

## 7) Chaining middleware

Most middleware helpers return something you can combine.

Example: install JSON parsing plus a custom marker middleware:

```cpp
using namespace vix::middleware::app;

install_exact(
  app,
  "/api/echo/json",
  chain(
    json_dev(1024, true, true),
    adapt_ctx(mw_mark_parsed_json())
  )
);
```

---

## 8) Using RequestState (share data across middleware and handlers)

RequestState is type-based storage attached to the request. It is perfect for:

- auth results (user id, role)
- request id
- parsed body metadata
- timings and diagnostics

Store a value:

```cpp
ctx.req().emplace_state<AuthInfo>(AuthInfo{true, "gaspard", "admin"});
```

Read it later in a handler:

```cpp
app.get("/api/who", [](Request& req, Response& res){
  if (req.has_state_type<AuthInfo>()) {
    const auto& a = req.state<AuthInfo>();
    res.json({"authed", a.authed, "subject", a.subject, "role", a.role});
    return;
  }
  res.json({"authed", false});
});
```

---

## 9) Common presets (what you typically want)

These are used in the middleware mega example.

### Security headers
```cpp
app.use(vix::middleware::app::security_headers_dev(false));
```

### CORS (dev)
```cpp
app.use(vix::middleware::app::cors_dev());
```

### Rate limit
```cpp
vix::middleware::app::install(app, "/api/", vix::middleware::app::rate_limit_dev(120, std::chrono::minutes(1)));
```

### JSON/Form/Multipart parsing
```cpp
vix::middleware::app::install_exact(app, "/api/echo/json", vix::middleware::app::json_dev(1024, true, true));
vix::middleware::app::install_exact(app, "/api/echo/form", vix::middleware::app::form_dev(1024, true));
vix::middleware::app::install_exact(app, "/api/echo/multipart", vix::middleware::app::multipart_save_dev("uploads", 10 * 1024 * 1024));
```

### API key auth (dev)
```cpp
vix::middleware::app::install(app, "/api/secure/", vix::middleware::app::api_key_dev("dev_key_123"));
```

### HTTP GET cache middleware
A typical config:

```cpp
vix::middleware::app::HttpCacheAppConfig cfg;
cfg.prefix = "/api/cache/";
cfg.only_get = true;
cfg.ttl_ms = 25'000;
cfg.allow_bypass = true;
cfg.bypass_header = "x-vix-cache";
cfg.bypass_value = "bypass";
cfg.add_debug_header = true;
cfg.debug_header = "x-vix-cache-status";
cfg.vary_headers = {"accept-encoding", "accept"};

vix::middleware::app::install_http_cache(app, std::move(cfg));
```

Then test:

```bash
curl -i http://127.0.0.1:8080/api/cache/demo
curl -i http://127.0.0.1:8080/api/cache/demo
curl -i -H "x-vix-cache: bypass" http://127.0.0.1:8080/api/cache/demo
```

Look for `x-vix-cache-status`.

---

## 10) A practical layout for real projects

A simple structure that beginners understand (Node/Python style):

- `register_public_routes(app)`
- `register_api_routes(app)`
- `register_admin_routes(app)`
- `install_global_middlewares(app)`
- `install_api_middlewares(app)`
- `install_admin_middlewares(app)`

Then in `main()`:

```cpp
int main() {
  vix::App app;

  install_global_middlewares(app);
  install_api_middlewares(app);

  register_public_routes(app);
  register_api_routes(app);

  app.run(8080);
  return 0;
}
```

This mirrors Express/Fastify and stays readable.

---

## 11) Troubleshooting tips

- Use `curl -i` to see response headers (request id, cache status).
- If a route returns 401/403, check which middleware is installed for its prefix.
- If JSON parsing fails, confirm the JSON middleware is installed on that route and that `Content-Type: application/json` is sent.

Example JSON request:

```bash
curl -i -X POST http://127.0.0.1:8080/api/echo/json   -H "Content-Type: application/json"   -d '{"name":"Ada","vip":true}'
```

