# API Reference

This page summarizes the main public APIs exposed by Vix Core.

Use it when you want a quick reference for `vix::App`, routing, handlers, middleware, requests, responses, sessions, transports, TLS, configuration, and the runtime executor.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the core header directly:

```cpp
#include <vix/core.hpp>
```

## Main aliases

`vix/core.hpp` exposes the main developer-facing aliases.

```cpp
namespace vix
{
  using App = vix::App;
  using AppPtr = std::shared_ptr<App>;

  using SessionPtr = std::shared_ptr<vix::session::Session>;

  using Request = vix::http::Request;
  using Response = vix::http::ResponseWrapper;

  namespace tmpl = vix::template_;
}
```

Most examples use:

```cpp
vix::App app;
vix::Request &req;
vix::Response &res;
vix::tmpl::Context ctx;
```

## vix::App

Header:

```cpp
#include <vix/app/App.hpp>
```

`vix::App` is the main application object.

It owns or coordinates:

```text
Config
Router
RuntimeExecutor
HTTPServer
middleware
templates
static files
server lifecycle
```

### Construction

```cpp
vix::App app;
```

Create an app with a custom executor:

```cpp
auto executor = std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::App app{executor};
```

### Lifecycle

| API                                                              | Purpose                                              |
| ---------------------------------------------------------------- | ---------------------------------------------------- |
| `run(int port = 8080)`                                           | Start the server and block until shutdown.           |
| `run(const vix::config::Config &cfg)`                            | Start the server using an explicit config and block. |
| `listen(int port = 8080, ListenCallback cb = {})`                | Start the server asynchronously.                     |
| `listen(const vix::config::Config &cfg, ListenCallback cb = {})` | Start asynchronously using an explicit config.       |
| `listen_port(int port, ListenPortCallback cb = {})`              | Start and pass the bound port to a callback.         |
| `wait()`                                                         | Wait until shutdown is requested.                    |
| `close()`                                                        | Stop the server and release resources.               |
| `is_running()`                                                   | Return whether the app is running.                   |
| `set_shutdown_callback(cb)`                                      | Register shutdown logic.                             |
| `request_stop_from_signal()`                                     | Request stop from a signal handler.                  |

Example:

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Hello from Vix");
  });

  app.run(8080);

  return 0;
}
```

### Route registration

| API                         | Purpose                        |
| --------------------------- | ------------------------------ |
| `get(path, handler)`        | Register a `GET` route.        |
| `post(path, handler)`       | Register a `POST` route.       |
| `put(path, handler)`        | Register a `PUT` route.        |
| `patch(path, handler)`      | Register a `PATCH` route.      |
| `del(path, handler)`        | Register a `DELETE` route.     |
| `head(path, handler)`       | Register a `HEAD` route.       |
| `options(path, handler)`    | Register an `OPTIONS` route.   |
| `get_heavy(path, handler)`  | Register a heavy `GET` route.  |
| `post_heavy(path, handler)` | Register a heavy `POST` route. |

Example:

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({
    "status", "ok"
  });
});
```

### Middleware

| API                               | Purpose                     |
| --------------------------------- | --------------------------- |
| `use(middleware)`                 | Register global middleware. |
| `use(prefix, middleware)`         | Register prefix middleware. |
| `protect(prefix, middleware)`     | Protect a route prefix.     |
| `protect_exact(path, middleware)` | Protect one exact path.     |

Middleware signature:

```cpp
void(vix::Request &req, vix::Response &res, vix::App::Next next)
```

Example:

```cpp
app.use("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  if (!req.has_header("Authorization"))
  {
    res.status(401).json({
      "error", "unauthorized"
    });
    return;
  }

  next();
});
```

### Groups

| API                 | Purpose                                               |
| ------------------- | ----------------------------------------------------- |
| `group(prefix, fn)` | Create a temporary route group and invoke a callback. |
| `group(prefix)`     | Return a reusable group object.                       |

Example:

```cpp
app.group("/api", [](auto &api)
{
  api.get("/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({"status", "ok"});
  });
});
```

### Static files

| API                                                                                  | Purpose                                              |
| ------------------------------------------------------------------------------------ | ---------------------------------------------------- |
| `static_dir(root, mount, index_file, add_cache_control, cache_control, fallthrough)` | Mount a static directory.                            |
| `set_static_handler(fn)`                                                             | Install a custom static file handler implementation. |

Common usage:

```cpp
app.static_dir("public", "/");
```

With explicit options:

```cpp
app.static_dir(
    "public",
    "/",
    "index.html",
    true,
    "public, max-age=3600",
    true);
```

### Templates

| API                    | Purpose                                  |
| ---------------------- | ---------------------------------------- |
| `templates(directory)` | Configure the template root directory.   |
| `has_views()`          | Return whether templates are configured. |
| `views()`              | Access the template view facade.         |

Example:

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

### Accessors

| API                       | Purpose                                     |
| ------------------------- | ------------------------------------------- |
| `config()`                | Return the mutable app configuration.       |
| `router()`                | Return the shared router.                   |
| `server()`                | Return the underlying HTTP server.          |
| `executor()`              | Return the runtime executor.                |
| `server_ready_info()`     | Return the last server ready info.          |
| `has_server_ready_info()` | Return whether ready info is available.     |
| `setDevMode(value)`       | Enable or disable development mode.         |
| `isDevMode()`             | Return whether development mode is enabled. |

## Router

Header:

```cpp
#include <vix/router/Router.hpp>
```

`vix::router::Router` matches HTTP methods and paths to handlers.

### Main APIs

| API                                              | Purpose                                          |
| ------------------------------------------------ | ------------------------------------------------ |
| `setNotFoundHandler(handler)`                    | Set a custom not-found handler.                  |
| `add_route(method, path, handler)`               | Register a route.                                |
| `add_route(method, path, handler, options)`      | Register a route with options.                   |
| `add_route(method, path, handler, options, doc)` | Register a route with options and docs metadata. |
| `handle_request(req, res)`                       | Dispatch a request to the matching handler.      |
| `is_heavy(req)`                                  | Return whether the matched route is heavy.       |
| `has_route(method, path)`                        | Return whether a route exists.                   |
| `routes()`                                       | Return registered route records.                 |
| `strip_query(target)`                            | Remove query string from a target.               |

Example:

```cpp
if (app.router()->has_route("GET", "/status"))
{
  vix::print("route exists");
}
```

### Route records

```cpp
struct RouteRecord
{
  std::string method;
  std::string path;
  bool heavy;
  RouteDoc doc;
};
```

List routes:

```cpp
for (const auto &route : app.router()->routes())
{
  vix::print(route.method, route.path);
}
```

## Route options

Header:

```cpp
#include <vix/router/RouteOptions.hpp>
```

```cpp
struct RouteOptions
{
  bool heavy{false};
};
```

Use through app helpers:

```cpp
app.get_heavy("/reports", handler);
app.post_heavy("/reports", handler);
```

Or directly through the router:

```cpp
vix::router::RouteOptions opt;
opt.heavy = true;
```

## Route documentation

Header:

```cpp
#include <vix/router/RouteDoc.hpp>
```

```cpp
struct RouteDoc
{
  std::string summary;
  std::string description;
  std::vector<std::string> tags;

  nlohmann::json request_body;
  nlohmann::json responses;
  nlohmann::json x;

  bool empty() const noexcept;
};
```

`RouteDoc` is used by documentation and OpenAPI tooling.

## Route node

Header:

```cpp
#include <vix/router/RouteNode.hpp>
```

`RouteNode` is the internal routing tree node.

It stores:

```text
children
handler
isParam
paramName
heavy
```

Main APIs:

| API                            | Purpose                                          |
| ------------------------------ | ------------------------------------------------ |
| `has_handler()`                | Return whether this node has a terminal handler. |
| `has_child(segment)`           | Return whether a child exists.                   |
| `find_child(segment)`          | Return a child node or `nullptr`.                |
| `get_or_create_child(segment)` | Get or create a child node.                      |
| `mark_as_param(name)`          | Mark the node as a path parameter.               |
| `set_handler(handler)`         | Assign the terminal request handler.             |
| `set_heavy(value)`             | Mark the route as heavy or not heavy.            |

Most application code does not use `RouteNode` directly.

## Request handlers

Headers:

```cpp
#include <vix/http/IRequestHandler.hpp>
#include <vix/http/RequestHandler.hpp>
```

### IRequestHandler

`IRequestHandler` is the internal interface used by the router.

```cpp
class IRequestHandler
{
public:
  virtual vix::async::core::task<void> handle_request(
      const Request &req,
      Response &res) = 0;
};
```

The router stores handlers through this interface.

### RequestHandler

`RequestHandler<Handler>` adapts a user handler to `IRequestHandler`.

Common user handler shape:

```cpp
[](vix::Request &req, vix::Response &res)
{
  res.text("OK");
}
```

The adapter handles:

```text
route parameter extraction
ResponseWrapper creation
user handler invocation
auto-send return values
response finalization
error handling
```

## Request

Header:

```cpp
#include <vix/http/Request.hpp>
```

`vix::Request` is an alias for `vix::http::Request`.

### Types

```cpp
using ParamMap = std::unordered_map<std::string, std::string>;
using QueryMap = std::unordered_map<std::string, std::string>;
using HeaderMap = std::unordered_map<std::string, std::string>;
using StatePtr = std::shared_ptr<vix::http::RequestState>;
```

### Method and target

| API                  | Purpose                                   |
| -------------------- | ----------------------------------------- |
| `method()`           | Return the HTTP method.                   |
| `set_method(method)` | Set the HTTP method.                      |
| `target()`           | Return the full request target.           |
| `set_target(target)` | Set target and recompute path/query data. |
| `path()`             | Return the path without query string.     |
| `query_string()`     | Return the raw query string.              |

Example:

```cpp
res.json({
  "method", req.method(),
  "path", req.path(),
  "target", req.target()
});
```

### Route parameters

| API                     | Purpose                            |
| ----------------------- | ---------------------------------- |
| `params()`              | Return all route parameters.       |
| `set_params(params)`    | Replace route parameters.          |
| `has_param(name)`       | Check if a route parameter exists. |
| `param(name, fallback)` | Return a route parameter.          |
| `with_params(params)`   | Return a copy with new params.     |

Example:

```cpp
app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "id", req.param("id")
  });
});
```

### Query parameters

| API                           | Purpose                            |
| ----------------------------- | ---------------------------------- |
| `query()`                     | Return parsed query parameters.    |
| `has_query(name)`             | Check if a query parameter exists. |
| `query_value(name, fallback)` | Return a query value.              |

Example:

```cpp
const std::string page = req.query_value("page", "1");
```

### Headers

| API                       | Purpose                                     |
| ------------------------- | ------------------------------------------- |
| `headers()`               | Return all headers.                         |
| `set_headers(headers)`    | Replace all headers.                        |
| `header(name)`            | Return one header value or an empty string. |
| `has_header(name)`        | Check whether a header exists.              |
| `set_header(name, value)` | Set one header.                             |
| `remove_header(name)`     | Remove one header.                          |

Example:

```cpp
if (!req.has_header("Authorization"))
{
  res.status(401).json({"error", "unauthorized"});
  return;
}
```

### Body and JSON

| API              | Purpose                            |
| ---------------- | ---------------------------------- |
| `body()`         | Return the request body.           |
| `set_body(body)` | Replace the request body.          |
| `json()`         | Parse and return the body as JSON. |
| `json_as<T>()`   | Convert JSON body to type `T`.     |

Example:

```cpp
app.post("/echo", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "body", req.body()
  });
});
```

### State

| API                   | Purpose                                  |
| --------------------- | ---------------------------------------- |
| `has_state()`         | Return whether a state container exists. |
| `state()`             | Access request-scoped state.             |
| `has_state_type<T>()` | Return whether state contains `T`.       |

Example:

```cpp
struct CurrentUser
{
  std::string id;
};

req.state().set(CurrentUser{"42"});

const auto &user = req.state().get<CurrentUser>();
```

## RequestState

Header:

```cpp
#include <vix/http/RequestState.hpp>
```

`RequestState` is type-safe request-scoped storage.

### APIs

| API                   | Purpose                                          |
| --------------------- | ------------------------------------------------ |
| `emplace<T>(args...)` | Construct and store a value of type `T`.         |
| `set<T>(value)`       | Store or replace a value of type `T`.            |
| `has<T>()`            | Return whether a value of type `T` exists.       |
| `get<T>()`            | Return a stored value or throw if missing.       |
| `try_get<T>()`        | Return a pointer to a stored value or `nullptr`. |

Example:

```cpp
struct CurrentUser
{
  std::string id;
};

app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)res;

  req.state().set(CurrentUser{"42"});

  next();
});

app.get("/me", [](vix::Request &req, vix::Response &res)
{
  const auto &user = req.state().get<CurrentUser>();

  res.json({
    "id", user.id
  });
});
```

## Response

Headers:

```cpp
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
```

`vix::Response` is an alias for `vix::http::ResponseWrapper`.

The lower-level native response object is `vix::http::Response`.

## ResponseWrapper

`ResponseWrapper` is the public helper used in handlers.

### Status

| API                    | Purpose                         |
| ---------------------- | ------------------------------- |
| `status(code)`         | Set the HTTP status code.       |
| `set_status(code)`     | Alias for `status(code)`.       |
| `status_c<Code>()`     | Set a compile-time status code. |
| `set_status_c<Code>()` | Alias for `status_c<Code>()`.   |
| `sendStatus(code)`     | Send a status response.         |

Example:

```cpp
res.status(201).json({
  "created", true
});
```

### Headers

| API                   | Purpose                          |
| --------------------- | -------------------------------- |
| `header(name, value)` | Set a response header.           |
| `set(name, value)`    | Alias for `header(name, value)`. |
| `append(name, value)` | Append a header value.           |
| `type(mime)`          | Set `Content-Type`.              |
| `contentType(mime)`   | Alias for `type(mime)`.          |
| `has_header(name)`    | Return whether a header exists.  |

Example:

```cpp
res.header("X-App", "Vix.cpp");
res.type("text/plain; charset=utf-8");
```

### Body helpers

| API                     | Purpose                               |
| ----------------------- | ------------------------------------- |
| `send()`                | Send an empty or current response.    |
| `send(value)`           | Send a value.                         |
| `text(data)`            | Send plain text.                      |
| `json(value)`           | Send JSON.                            |
| `redirect(url)`         | Send a redirect.                      |
| `redirect(code, url)`   | Send a redirect with explicit status. |
| `file(path)`            | Send a file.                          |
| `render(name, context)` | Render a template.                    |
| `has_body()`            | Return whether the body is non-empty. |

Example:

```cpp
res.text("OK");
res.json({"ok", true});
res.redirect("/login");
res.file("public/index.html");
```

## Native Response

Header:

```cpp
#include <vix/http/Response.hpp>
```

`vix::http::Response` stores the raw HTTP response state.

### APIs

| API                       | Purpose                                     |
| ------------------------- | ------------------------------------------- |
| `status()`                | Return the status code.                     |
| `set_status(code)`        | Set the status code.                        |
| `body()`                  | Return the body.                            |
| `set_body(body)`          | Replace the body.                           |
| `headers()`               | Return response headers.                    |
| `set_headers(headers)`    | Replace all headers.                        |
| `has_header(name)`        | Check whether a header exists.              |
| `header(name)`            | Return one header value.                    |
| `set_header(name, value)` | Set one header.                             |
| `remove_header(name)`     | Remove one header.                          |
| `clear_headers()`         | Remove all headers.                         |
| `should_close()`          | Return whether the connection should close. |
| `set_should_close(value)` | Set close behavior.                         |
| `reason()`                | Return custom reason phrase.                |
| `set_reason(reason)`      | Set custom reason phrase.                   |
| `clear_reason()`          | Clear custom reason phrase.                 |
| `version()`               | Return HTTP version.                        |
| `set_version(version)`    | Set HTTP version.                           |
| `to_http_string()`        | Serialize response to raw HTTP text.        |

Advanced example:

```cpp
res.res.set_status(200);
res.res.set_header("X-App", "Vix");
res.res.set_body("hello");
```

Prefer wrapper methods in normal code:

```cpp
res.status(200).text("hello");
```

## Status codes

Header:

```cpp
#include <vix/http/Status.hpp>
```

### Common constants

```cpp
vix::http::OK
vix::http::CREATED
vix::http::ACCEPTED
vix::http::NO_CONTENT

vix::http::BAD_REQUEST
vix::http::UNAUTHORIZED
vix::http::FORBIDDEN
vix::http::NOT_FOUND
vix::http::PAYLOAD_TOO_LARGE

vix::http::INTERNAL_ERROR
vix::http::SERVICE_UNAVAILABLE
```

### Helpers

| API                      | Purpose                                    |
| ------------------------ | ------------------------------------------ |
| `is_valid_status(code)`  | Return whether code is in `100..599`.      |
| `normalize_status(code)` | Return the code if valid, otherwise `500`. |
| `reason_phrase(code)`    | Return the canonical reason phrase.        |
| `status_to_string(code)` | Return a status string.                    |

Example:

```cpp
res.status(vix::http::OK).json({
  "status", "ok"
});
```

## HTTPServer

Header:

```cpp
#include <vix/server/HTTPServer.hpp>
```

`HTTPServer` is the native async HTTP server used by `App`.

Most applications access it through:

```cpp
auto &server = app.server();
```

### APIs

| API                           | Purpose                                            |
| ----------------------------- | -------------------------------------------------- |
| `run()`                       | Start I/O threads, startup coroutine, and monitor. |
| `start_accept()`              | Start accepting connections.                       |
| `calculate_io_thread_count()` | Compute I/O thread count.                          |
| `getRouter()`                 | Return the shared router.                          |
| `monitor_metrics()`           | Start the metrics monitor.                         |
| `stop_async()`                | Request asynchronous shutdown.                     |
| `stop_blocking()`             | Stop and join internal threads.                    |
| `join_threads()`              | Join server-owned threads.                         |
| `bound_port()`                | Return the actual bound port.                      |
| `is_stop_requested()`         | Return whether shutdown was requested.             |
| `executor()`                  | Return the runtime executor.                       |

Example:

```cpp
const int port = app.server().bound_port();
```

## Session

Header:

```cpp
#include <vix/session/Session.hpp>
```

`Session` represents one client connection.

Most applications do not create sessions manually.

### APIs

| API                                            | Purpose                                    |
| ---------------------------------------------- | ------------------------------------------ |
| `Session(stream, router, config, executor)`    | Create a session from a TCP stream.        |
| `Session(transport, router, config, executor)` | Create a session from a generic transport. |
| `run()`                                        | Start the session lifecycle.               |
| `read_request()`                               | Read and parse the next request.           |
| `dispatch_request(req)`                        | Dispatch a request to the router.          |
| `send_response(res)`                           | Serialize and write a response.            |
| `send_error(status, msg)`                      | Send a standard error response.            |
| `close_stream_gracefully()`                    | Close the transport safely.                |

Session flow:

```text
read request
  -> dispatch request
  -> send response
  -> repeat or close
```

## Transport

Header:

```cpp
#include <vix/session/Transport.hpp>
```

`Transport` is the abstract byte I/O interface used by sessions.

```cpp
class Transport
{
public:
  virtual task<std::size_t> async_read(
      std::span<std::byte> buffer,
      cancel_token token) = 0;

  virtual task<std::size_t> async_write(
      std::span<const std::byte> buffer,
      cancel_token token) = 0;

  virtual bool is_open() const noexcept = 0;

  virtual void close() noexcept = 0;
};
```

### APIs

| API                          | Purpose                               |
| ---------------------------- | ------------------------------------- |
| `async_read(buffer, token)`  | Read bytes.                           |
| `async_write(buffer, token)` | Write bytes.                          |
| `is_open()`                  | Check whether the connection is open. |
| `close()`                    | Close the connection.                 |

## PlainTransport

Header:

```cpp
#include <vix/session/PlainTransport.hpp>
```

`PlainTransport` adapts a native TCP stream to `Transport`.

```text
tcp_stream
  -> PlainTransport
  -> Session
```

### APIs

| API                          | Purpose                                     |
| ---------------------------- | ------------------------------------------- |
| `PlainTransport(stream)`     | Create a plain transport from a TCP stream. |
| `async_read(buffer, token)`  | Read bytes from the TCP stream.             |
| `async_write(buffer, token)` | Write bytes to the TCP stream.              |
| `is_open()`                  | Return whether the TCP stream is open.      |
| `close()`                    | Close the TCP stream.                       |

## TLS

Headers:

```cpp
#include <vix/server/TlsConfig.hpp>
#include <vix/session/TlsSession.hpp>
#include <vix/session/TlsTransport.hpp>
```

## TlsConfig

`TlsConfig` describes TLS settings.

```cpp
struct TlsConfig
{
  bool enabled{false};
  std::string cert_file{};
  std::string key_file{};

  bool is_enabled() const noexcept;
  bool is_configured() const noexcept;
  bool is_valid() const noexcept;
};
```

### APIs

| API               | Purpose                                           |
| ----------------- | ------------------------------------------------- |
| `enabled`         | Enable or disable TLS.                            |
| `cert_file`       | Certificate file path.                            |
| `key_file`        | Private key file path.                            |
| `is_enabled()`    | Return whether TLS is enabled.                    |
| `is_configured()` | Return whether certificate and key paths are set. |
| `is_valid()`      | Return whether TLS is enabled and configured.     |

Example:

```cpp
vix::server::TlsConfig tls;

tls.enabled = true;
tls.cert_file = "/etc/letsencrypt/live/example.com/fullchain.pem";
tls.key_file = "/etc/letsencrypt/live/example.com/privkey.pem";
```

## TlsSession

`TlsSession` owns an accepted TCP stream during TLS setup.

### APIs

| API                                            | Purpose                                           |
| ---------------------------------------------- | ------------------------------------------------- |
| `TlsSession(stream, router, config, executor)` | Create a TLS session from an accepted TCP stream. |
| `run()`                                        | Perform TLS handshake and run the HTTP session.   |

Flow:

```text
tcp_stream
  -> TlsSession
  -> TlsTransport
  -> TLS handshake
  -> Session
```

## TlsTransport

`TlsTransport` adapts a TCP stream into encrypted transport.

### APIs

| API                            | Purpose                               |
| ------------------------------ | ------------------------------------- |
| `TlsTransport(stream, config)` | Create a TLS transport.               |
| `async_handshake(token)`       | Perform TLS server handshake.         |
| `async_read(buffer, token)`    | Read decrypted bytes.                 |
| `async_write(buffer, token)`   | Write encrypted bytes.                |
| `is_open()`                    | Return whether the transport is open. |
| `close()`                      | Close TLS and TCP resources.          |

## Config

Header:

```cpp
#include <vix/config/Config.hpp>
```

`vix::config::Config` stores server and application configuration.

### Construction

```cpp
vix::config::Config cfg;
vix::config::Config production{".env.production"};
```

### General APIs

| API                        | Purpose                             |
| -------------------------- | ----------------------------------- |
| `loadConfig()`             | Reload configuration.               |
| `set(key, value)`          | Set a dotted key.                   |
| `has(key)`                 | Return whether a dotted key exists. |
| `getInt(key, fallback)`    | Read an integer value.              |
| `getBool(key, fallback)`   | Read a boolean value.               |
| `getString(key, fallback)` | Read a string value.                |

### Server APIs

| API                      | Purpose                                   |
| ------------------------ | ----------------------------------------- |
| `setServerPort(port)`    | Set server port.                          |
| `getServerPort()`        | Return server port.                       |
| `getRequestTimeout()`    | Return request timeout in milliseconds.   |
| `getIOThreads()`         | Return I/O thread count.                  |
| `getSessionTimeoutSec()` | Return session timeout in seconds.        |
| `isBenchMode()`          | Return whether benchmark mode is enabled. |

### Logging APIs

| API                      | Purpose                                         |
| ------------------------ | ----------------------------------------------- |
| `getLogAsync()`          | Return whether async logging is enabled.        |
| `getLogQueueMax()`       | Return async log queue max size.                |
| `getLogDropOnOverflow()` | Return whether logs can be dropped on overflow. |

### WAF APIs

| API                    | Purpose                               |
| ---------------------- | ------------------------------------- |
| `getWafMode()`         | Return WAF mode.                      |
| `getWafMaxTargetLen()` | Return max target length.             |
| `getWafMaxBodyBytes()` | Return max body bytes for WAF checks. |

### TLS APIs

| API                | Purpose                            |
| ------------------ | ---------------------------------- |
| `isTlsEnabled()`   | Return whether TLS is enabled.     |
| `getTlsCertFile()` | Return TLS certificate path.       |
| `getTlsKeyFile()`  | Return TLS private key path.       |
| `getTlsConfig()`   | Return complete TLS configuration. |

### Database APIs

| API                      | Purpose                                    |
| ------------------------ | ------------------------------------------ |
| `getDbHost()`            | Return database host.                      |
| `getDbUser()`            | Return database user.                      |
| `getDbName()`            | Return database name.                      |
| `getDbPort()`            | Return database port.                      |
| `getDbPasswordFromEnv()` | Return database password from environment. |

Example:

```cpp
vix::config::Config cfg;

cfg.setServerPort(8080);
cfg.set("server.tls.enabled", false);

vix::App app;
app.run(cfg);
```

## RuntimeExecutor

Header:

```cpp
#include <vix/executor/RuntimeExecutor.hpp>
```

`RuntimeExecutor` bridges Core to `vix::runtime`.

### Construction

```cpp
vix::executor::RuntimeExecutor executor{4};
```

Use with app:

```cpp
auto executor = std::make_shared<vix::executor::RuntimeExecutor>(4);

vix::App app{executor};
```

### Lifecycle

| API               | Purpose                                  |
| ----------------- | ---------------------------------------- |
| `start()`         | Start the underlying runtime.            |
| `stop()`          | Stop the runtime.                        |
| `stop_and_wait()` | Drain current work, then stop.           |
| `started()`       | Return whether the executor has started. |
| `running()`       | Return whether the runtime is running.   |
| `accepting()`     | Return whether new work is accepted.     |

### Submission

| API                            | Purpose                           |
| ------------------------------ | --------------------------------- |
| `submit(task)`                 | Submit a runtime task.            |
| `submit(task_fn, affinity)`    | Submit a low-level task function. |
| `post(fn, options)`            | Submit a normal callable.         |
| `post_http_fast(fn, affinity)` | Submit short HTTP fast-path work. |

Example:

```cpp
executor.post([]()
{
  vix::print("work");
});
```

Direct runtime task:

```cpp
executor.submit([]() -> vix::runtime::TaskResult
{
  return vix::runtime::TaskResult::complete;
});
```

### Metrics

| API                           | Purpose                                        |
| ----------------------------- | ---------------------------------------------- |
| `metrics()`                   | Return executor metrics.                       |
| `wait_idle()`                 | Wait until pending and active work reach zero. |
| `submitted_tasks()`           | Return accepted submission count.              |
| `rejected_tasks()`            | Return rejected submission count.              |
| `fast_http_submitted_tasks()` | Return fast HTTP submission count.             |

Example:

```cpp
const auto m = app.executor().metrics();

res.json({
  "pending", m.pending,
  "active", m.active,
  "timed_out", m.timed_out
});
```

## TaskOptions

Header:

```cpp
#include <vix/executor/TaskOptions.hpp>
```

```cpp
struct TaskOptions
{
  int priority = 0;
  std::chrono::milliseconds timeout{0};
  std::chrono::milliseconds deadline{0};
  bool may_block = false;
};
```

| Field       | Purpose                                        |
| ----------- | ---------------------------------------------- |
| `priority`  | Priority value reserved for scheduling policy. |
| `timeout`   | Timeout threshold for metrics.                 |
| `deadline`  | Deadline value reserved for scheduling policy. |
| `may_block` | Marks whether the task may block.              |

Example:

```cpp
vix::executor::TaskOptions options;

options.timeout = std::chrono::milliseconds(100);

app.executor().post([]()
{
  vix::print("work");
}, options);
```

## Metrics

Header:

```cpp
#include <vix/executor/Metrics.hpp>
```

```cpp
struct Metrics
{
  std::uint64_t pending{0};
  std::uint64_t active{0};
  std::uint64_t timed_out{0};
};
```

| Field       | Purpose                                          |
| ----------- | ------------------------------------------------ |
| `pending`   | Number of pending tasks.                         |
| `active`    | Number of active tracked tasks.                  |
| `timed_out` | Number of tasks that exceeded timeout threshold. |

## Complete example

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
  {
    (void)res;

    vix::print(req.method(), req.path());

    next();
  });

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Home");
  });

  app.get("/users/{id}", [](vix::Request &req, vix::Response &res)
  {
    res.json({
      "id", req.param("id"),
      "page", req.query_value("page", "1")
    });
  });

  app.get("/metrics", [&app](vix::Request &req, vix::Response &res)
  {
    (void)req;

    const auto m = app.executor().metrics();

    res.json({
      "pending", m.pending,
      "active", m.active,
      "timed_out", m.timed_out
    });
  });

  app.static_dir("public/assets", "/assets");

  app.run(8080);

  return 0;
}
```

## Header map

| Area                      | Header                               |
| ------------------------- | ------------------------------------ |
| Umbrella                  | `<vix.hpp>`                          |
| Core umbrella             | `<vix/core.hpp>`                     |
| App                       | `<vix/app/App.hpp>`                  |
| Router                    | `<vix/router/Router.hpp>`            |
| Route docs                | `<vix/router/RouteDoc.hpp>`          |
| Route node                | `<vix/router/RouteNode.hpp>`         |
| Route options             | `<vix/router/RouteOptions.hpp>`      |
| Request                   | `<vix/http/Request.hpp>`             |
| Request state             | `<vix/http/RequestState.hpp>`        |
| Response                  | `<vix/http/Response.hpp>`            |
| Response wrapper          | `<vix/http/ResponseWrapper.hpp>`     |
| Status                    | `<vix/http/Status.hpp>`              |
| Request handler interface | `<vix/http/IRequestHandler.hpp>`     |
| Request handler adapter   | `<vix/http/RequestHandler.hpp>`      |
| HTTP server               | `<vix/server/HTTPServer.hpp>`        |
| TLS config                | `<vix/server/TlsConfig.hpp>`         |
| Session                   | `<vix/session/Session.hpp>`          |
| Transport                 | `<vix/session/Transport.hpp>`        |
| Plain transport           | `<vix/session/PlainTransport.hpp>`   |
| TLS session               | `<vix/session/TlsSession.hpp>`       |
| TLS transport             | `<vix/session/TlsTransport.hpp>`     |
| Config                    | `<vix/config/Config.hpp>`            |
| Runtime executor          | `<vix/executor/RuntimeExecutor.hpp>` |
| Task options              | `<vix/executor/TaskOptions.hpp>`     |
| Metrics                   | `<vix/executor/Metrics.hpp>`         |

## Recommended public usage

For normal applications, use:

```cpp
#include <vix.hpp>
```

Then write:

```cpp
vix::App app;

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello from Vix");
});

app.run(8080);
```

Use direct headers only when you are writing advanced integrations, tests, module-level code, or documentation.

## Next steps

Read the related pages:

- [App](./app.md)
- [Routing](./routing.md)
- [Handlers](./handlers.md)
- [Request](./request.md)
- [Response](./response.md)
- [HTTP server](./http-server.md)
- [Runtime executor](./runtime-executor.md)
