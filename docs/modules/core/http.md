# HTTP (core)

This part of `core` provides the HTTP facade used by Vix apps and the internal adapter layer used by the router/server.

It includes:

- `IRequestHandler`: a uniform interface used by router/server
- `Request`: a high-level request facade (method, path, params, query, headers, JSON, state)
- `RequestHandler`: an adapter that wraps user handlers into `IRequestHandler`
- `RequestState`: type-safe per-request storage for middleware data sharing
- `Response`: static helpers to build Boost.Beast responses
- `ResponseWrapper`: a friendly fluent API to set status/headers and send text/JSON/files
- `Status`: numeric constants and helpers for HTTP status codes

---

## IRequestHandler

`vix::vhttp::IRequestHandler` is the low-level interface used by the router/server.

```cpp
class IRequestHandler
{
public:
  virtual void handle_request(
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res) = 0;

  virtual ~IRequestHandler() noexcept = default;
};
```

You typically do not implement this directly as an app developer. Vix generates adapters for you via `RequestHandler`.

---

## Request

`vix::vhttp::Request` is the developer-facing request facade.

It wraps a raw Boost.Beast request and exposes:

- `method()`
- `path()` (without query string)
- `target()` (original path + query)
- `params()` (route params extracted from patterns like `/users/{id}`)
- `query()` (lazy query string parsing)
- `header(name)` / `has_header(name)`
- `body()`
- `json()` / `json_as<T>()` (lazy JSON parsing)
- `RequestState` accessors (`state<T>()`, `try_state<T>()`, `set_state<T>()`, ...)

### Path and query parsing

The constructor splits the HTTP target once:

- `path_` gets the part before `?`
- `query_raw_` stores the raw query string part after `?`
- `query()` is parsed lazily on first access

### Route params

Route params are provided by the `RequestHandler` adapter, which matches a route pattern like:

- `/posts/{id}`
- `/users/{userId}/comments/{commentId}`

and builds a `ParamMap`.

### JSON caching

`json()` parses the request body once and caches the result. If the body is empty, it returns `{}`.

---

## RequestState

`RequestState` is a per-request storage container implemented with:

- `std::any`
- keyed by `std::type_index`

It is designed for middleware to pass strongly-typed data down the chain.

Example pattern:

```cpp
struct AuthUser { int id; std::string name; };

// middleware:
req.emplace_state<AuthUser>(AuthUser{42, "Ada"});

// handler:
auto& u = req.state<AuthUser>();
```

Core API:

- `emplace<T>(...)`
- `set<T>(value)`
- `has<T>()`
- `get<T>()`
- `try_get<T>()`

---

## Response

`vix::vhttp::Response` provides static helpers to build common responses on top of Boost.Beast.

It applies stable defaults like:

- `Server: Vix/master`
- `Date: <RFC7231 GMT date>`

It supports:

- JSON message responses (`{"message": "..."}`)
- JSON data responses
- plain text responses
- error helpers
- redirects

Example:

```cpp
vix::vhttp::Response::json_response(res, nlohmann::json{{"ok", true}});
```

### HTTP date formatting

`http_date_now()` formats a GMT date according to RFC 7231 and uses platform-safe `gmtime_*` variants.

---

## ResponseWrapper

`vix::vhttp::ResponseWrapper` is the fluent developer API used in handlers.

It wraps:

```cpp
http::response<http::string_body>&
```

and provides:

- status setters (`status(int)`, `status(http::status)`, `status_c<Code>()`)
- header utilities (`header`, `set`, `append`, `type`)
- send helpers (`text`, `json`, `send`, `sendStatus`, `redirect`)
- static file serving (`file(path)`)

### Common usage

```cpp
app.get("/ping", [](Request&, ResponseWrapper& res) {
  res.status(200).send("pong");
});

app.get("/json", [](Request&, ResponseWrapper& res) {
  res.json({ "message", "Hello" });
});
```

### Auto default behavior

- If status is not set, it defaults to `200 OK`.
- For `204` and `304`, bodies are cleared automatically.
- If you call `send()` with no body, it sends a default status text.

### Static files

`file(path)`:

- normalizes the path
- rejects `..` segments (basic traversal protection)
- maps file extensions to MIME types
- serves `index.html` when `path` is a directory
- sets `Cache-Control` if missing

---

## RequestHandler

`vix::vhttp::RequestHandler<Handler>` is the adapter that:

1. extracts `path_only` (target without query)
2. extracts route params based on the route pattern
3. creates a `RequestState`
4. constructs a `Request` facade
5. invokes the user handler
6. sets keep-alive headers and prepares payload

### Supported handler signatures

Vix supports both facade and raw handlers, and optionally passing route params.

Facade style (recommended):

- `void(Request&, ResponseWrapper&)`
- `void(Request&, ResponseWrapper&, const Request::ParamMap&)`

Raw style (advanced):

- `void(const RawRequest&, ResponseWrapper&)`
- `void(const RawRequest&, ResponseWrapper&, const Request::ParamMap&)`

Returnable handlers (auto-send)

Instead of returning `void`, a handler may return a value that can be sent.

Supported return forms:

- payload type accepted by `ResponseWrapper::send(payload)`
- `std::pair<status, payload>`
- `std::tuple<status, payload>`

Auto-send only happens when the response body is still empty.

### Error handling

The adapter catches:

- `std::range_error` (typically invalid status codes)
- `std::exception`

In debug builds (`!NDEBUG`), it returns a simple HTML error page with:

- route pattern
- method
- path

In release builds, it returns a safe JSON error response and logs details using `vix::utils::Logger`.

---

## Status

`vix::vhttp::Status` exposes numeric constants for common codes:

- `OK = 200`, `CREATED = 201`, `NO_CONTENT = 204`, ...
- `BAD_REQUEST = 400`, `NOT_FOUND = 404`, ...
- `INTERNAL_ERROR = 500`, ...

Helpers:

- `to_status(int)` converts `100..599` to `http::status` (asserts in debug)
- `status_to_string(int)` returns a human-readable string (e.g. `"404 Not Found"`)

---

## Practical guidance

- Prefer the facade handler style for most apps.
- Use `RequestState` for middleware to share typed data.
- Use `ResponseWrapper` for fluent status/headers/send patterns.
- Keep expensive work out of networking threads (use an executor via `App`).


