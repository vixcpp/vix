# Response

This page shows how to use `vix::Response` in Vix Core.

Use it when you want to send text, JSON, files, redirects, templates, headers, status codes, or empty responses from route handlers.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the response headers directly:

```cpp
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
#include <vix/http/Status.hpp>
```

## What Response provides

`vix::Response` is the public response helper used in route handlers.

It lets you build HTTP responses with:

- status codes
- plain text
- JSON
- redirects
- static files
- templates
- headers
- content types
- empty responses
- default HTTP status messages

Most handlers receive a response by reference.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello from Vix");
});
```

## Basic response

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Home");
  });

  app.run(8080);

  return 0;
}
```

Run:

```bash
vix run main.cpp
```

Expected response:

```text
Home
```

## Send text

Use `text(...)` to send plain text.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello from Vix");
});
```

The response content type is set to text when needed.

## Send JSON

Use `json(...)` to send JSON.

```cpp
app.get("/api/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({
    "status", "ok",
    "server", "Vix.cpp"
  });
});
```

Example response:

```json
{
  "status": "ok",
  "server": "Vix.cpp"
}
```

## Set status code

Use `status(...)` before sending the response body.

```cpp
app.post("/users", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(201).json({
    "created", true
  });
});
```

Example response:

```json
{
  "created": true
}
```

## Send an error response

```cpp
app.get("/admin", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  const bool allowed = false;

  if (!allowed)
  {
    res.status(403).json({
      "error", "forbidden"
    });
    return;
  }

  res.text("admin");
});
```

## Send an empty response

Use `send()` for an empty response.

```cpp
app.get("/empty", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(204).send();
});
```

A `204 No Content` response should not contain a body.

## Send a status response

Use `sendStatus(...)` when you want to send a response based on an HTTP status code.

```cpp
app.get("/missing", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.sendStatus(404);
});
```

For no-content statuses such as `204`, Vix sends an empty response.

For other statuses, Vix can send a default status message.

## Redirect

Use `redirect(...)` to send a redirect.

```cpp
app.get("/old", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.redirect("/new");
});
```

By default, this sends a redirect response.

You can also pass a status code.

```cpp
app.get("/old", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.redirect(301, "/new");
});
```

## Set headers

Use `header(...)` to set a response header.

```cpp
app.get("/custom", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.header("X-App", "Vix.cpp");
  res.text("custom header");
});
```

## Append header values

Use `append(...)` to append a value to an existing header.

```cpp
app.get("/cache", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.header("Cache-Control", "public");
  res.append("Cache-Control", "max-age=3600");

  res.text("cached");
});
```

Result shape:

```text
Cache-Control: public, max-age=3600
```

## Set content type

Use `type(...)` to set `Content-Type`.

```cpp
app.get("/plain", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.type("text/plain; charset=utf-8");
  res.send("plain text");
});
```

`contentType(...)` is an alias.

```cpp
res.contentType("application/json; charset=utf-8");
```

## Send a file

Use `file(...)` to send a file.

```cpp
app.get("/download", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.file("public/file.txt");
});
```

Vix detects common MIME types from file extensions.

Examples:

```text
.html -> text/html; charset=utf-8
.css  -> text/css; charset=utf-8
.js   -> application/javascript
.json -> application/json; charset=utf-8
.png  -> image/png
.jpg  -> image/jpeg
.svg  -> image/svg+xml
```

## File safety

`res.file(...)` performs basic path safety checks.

If the path contains `..`, Vix returns a bad request response.

```cpp
res.file("../secret.txt");
```

This protects against simple path traversal mistakes.

## Send static HTML

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.file("public/index.html");
});
```

## Render a template

First configure templates on the app.

```cpp
app.templates("views");
```

Then render from a handler.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  vix::tmpl::Context ctx;
  ctx.set("title", "Home");

  res.render("index.html", ctx);
});
```

If templates were not configured, `render(...)` throws.

## Send custom HTML

```cpp
app.get("/html", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.type("text/html; charset=utf-8");
  res.send("<h1>Hello from Vix</h1>");
});
```

## Send raw string

Use `send(...)` when you want the response helper to send a value.

```cpp
app.get("/hello", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.send("Hello");
});
```

For plain text, prefer:

```cpp
res.text("Hello");
```

## JSON with status

```cpp
app.post("/users", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(201).json({
    "id", 42,
    "created", true
  });
});
```

## Error JSON helper pattern

A common pattern is to return errors as JSON.

```cpp
app.get("/private", [](vix::Request &req, vix::Response &res)
{
  if (!req.has_header("Authorization"))
  {
    res.status(401).json({
      "error", "unauthorized"
    });
    return;
  }

  res.json({
    {"ok", true}
  });
});
```

## Response chaining

Most response helper methods return the response wrapper.

This allows chaining.

```cpp
res.status(201).json({
  "created", true
});
```

Another example:

```cpp
res.status(403)
   .header("X-Reason", "auth")
   .json({"error", "forbidden"});
```

## Access the native response

`vix::Response` is an alias for `vix::http::ResponseWrapper`.

The wrapper stores the native response as:

```cpp
res.res
```

Advanced code can access it directly.

```cpp
res.res.set_status(200);
res.res.set_header("X-App", "Vix");
res.res.set_body("hello");
```

Most application code should prefer the wrapper methods.

```cpp
res.status(200).text("hello");
```

## Native response object

The lower-level native response object is `vix::http::Response`.

It stores:

- status code
- body
- headers
- reason phrase
- HTTP version
- close flag

It can serialize itself as HTTP text.

```cpp
vix::http::Response raw;

raw.set_status(200);
raw.set_header("Content-Type", "text/plain; charset=utf-8");
raw.set_body("OK");

const std::string wire = raw.to_http_string();
```

Most applications use `vix::Response`, not `vix::http::Response` directly.

## Common headers

Core automatically applies common response headers when needed.

Examples:

```text
Server: Vix.cpp
Date: <http date>
Content-Length: <body size>
Connection: keep-alive
```

The session may also adjust connection behavior depending on the request and response.

## Connection close

Use the native response when you need to control connection closing.

```cpp
res.res.set_should_close(true);
res.header("Connection", "close");
```

Most application handlers do not need this.

## Status constants

Vix provides HTTP status constants.

```cpp
vix::http::OK
vix::http::CREATED
vix::http::NO_CONTENT
vix::http::BAD_REQUEST
vix::http::UNAUTHORIZED
vix::http::FORBIDDEN
vix::http::NOT_FOUND
vix::http::INTERNAL_ERROR
```

Example:

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.status(vix::http::OK).json({
    "status", "ok"
  });
});
```

## Validate status codes

The lower-level status API can validate status codes.

```cpp
vix::http::is_valid_status(200);
vix::http::is_valid_status(999);
```

A valid HTTP status code is in the range:

```text
100..599
```

Invalid status codes are normalized to `500` by the lower-level helpers.

## Reason phrases

Use `reason_phrase(...)` to read a standard reason phrase.

```cpp
auto phrase = vix::http::reason_phrase(404);
```

Result:

```text
Not Found
```

## Response lifecycle

A response is created for each request.

```text
Session
  -> create Response
  -> Router
  -> RequestHandler
  -> user handler writes response
  -> response is finalized
  -> Session serializes response
  -> Transport writes bytes
```

Application code usually writes the response here:

```cpp
app.get("/path", [](vix::Request &req, vix::Response &res)
{
  res.text("OK");
});
```

## Response and handlers

Handlers should write exactly one response.

Recommended:

```cpp
app.get("/ok", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"ok", true});
});
```

When sending an error early, return immediately.

```cpp
if (!allowed)
{
  res.status(403).json({"error", "forbidden"});
  return;
}
```

## Response and middleware

Middleware can set response headers before the handler runs.

```cpp
app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  (void)req;

  res.header("X-Powered-By", "Vix.cpp");

  next();
});
```

Middleware can also stop the request early.

```cpp
app.use("/admin", [](vix::Request &req, vix::Response &res, vix::App::Next next)
{
  if (!req.has_header("Authorization"))
  {
    res.status(401).json({"error", "unauthorized"});
    return;
  }

  next();
});
```

## Response and templates

`res.render(...)` uses the template view configured on the app.

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

## Response and static files

Use `res.file(...)` for one file.

```cpp
app.get("/logo", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.file("public/logo.svg");
});
```

Use `app.static_dir(...)` when you want to expose a whole directory.

```cpp
app.static_dir("public", "/assets");
```

## Complete example

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("Home");
  });

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok",
      "server", "Vix.cpp"
    });
  });

  app.post("/api/users", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.status(201).json({
      "created", true
    });
  });

  app.get("/old", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.redirect("/new");
  });

  app.get("/download", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.file("public/file.txt");
  });

  app.get("/empty", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.status(204).send();
  });

  app.run(8080);

  return 0;
}
```

## API summary

| API                         | Purpose                                       |
| --------------------------- | --------------------------------------------- |
| `res.status(code)`          | Set the HTTP status code.                     |
| `res.set_status(code)`      | Alias for `status(code)`.                     |
| `res.status_c<Code>()`      | Set a compile-time status code.               |
| `res.send()`                | Send an empty or current response.            |
| `res.send(value)`           | Send a value.                                 |
| `res.sendStatus(code)`      | Send a status response.                       |
| `res.text(data)`            | Send plain text.                              |
| `res.json(value)`           | Send JSON.                                    |
| `res.redirect(url)`         | Send a redirect.                              |
| `res.redirect(code, url)`   | Send a redirect with a status code.           |
| `res.file(path)`            | Send a file.                                  |
| `res.render(name, context)` | Render a template.                            |
| `res.header(name, value)`   | Set a header.                                 |
| `res.set(name, value)`      | Alias for `header(name, value)`.              |
| `res.append(name, value)`   | Append a header value.                        |
| `res.type(mime)`            | Set `Content-Type`.                           |
| `res.contentType(mime)`     | Alias for `type(mime)`.                       |
| `res.has_header(name)`      | Check whether a header is set.                |
| `res.has_body()`            | Check whether the response body is non-empty. |

## Native response API summary

| API                       | Purpose                                     |
| ------------------------- | ------------------------------------------- |
| `status()`                | Return the status code.                     |
| `set_status(code)`        | Set the status code.                        |
| `body()`                  | Return the body.                            |
| `set_body(body)`          | Replace the body.                           |
| `headers()`               | Return response headers.                    |
| `set_header(name, value)` | Set one header.                             |
| `remove_header(name)`     | Remove one header.                          |
| `clear_headers()`         | Remove all headers.                         |
| `should_close()`          | Return whether the connection should close. |
| `set_should_close(value)` | Set close behavior.                         |
| `reason()`                | Return custom reason phrase.                |
| `set_reason(reason)`      | Set custom reason phrase.                   |
| `version()`               | Return HTTP version.                        |
| `set_version(version)`    | Set HTTP version.                           |
| `to_http_string()`        | Serialize the response to HTTP text.        |

## Best practices

Send one clear response per handler.

```cpp
res.json({"ok", true});
```

Return immediately after sending an error.

```cpp
if (!allowed)
{
  res.status(403).json({"error", "forbidden"});
  return;
}
```

Use `json(...)` for API responses.

```cpp
res.json({"status", "ok"});
```

Use `text(...)` for simple plain text.

```cpp
res.text("OK");
```

Use `status(...)` before the body.

```cpp
res.status(201).json({"created", true});
```

Use `static_dir(...)` for directories and `file(...)` for single files.

```cpp
app.static_dir("public", "/assets");
res.file("public/file.txt");
```

Use `render(...)` only after configuring templates.

```cpp
app.templates("views");
```

## Next steps

Read the next pages:

- [Static files](./static-files.md)
- [Templates](./templates.md)
- [Handlers](./handlers.md)
- [Request](./request.md)
- [Sessions](./sessions.md)
