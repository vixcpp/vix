# Request and Response

`Request` and `Response` are the core objects of the Vix.cpp HTTP model.

A route handler receives a request from the client and writes a response back to the client.

```txt
Request  -> what the client sent
Response -> what the application returns
```

In a Vix.cpp HTTP application, most backend logic follows this flow:

```txt
read Request
  -> validate input
  -> run application logic
  -> write Response
  -> return
```

## The role of `Request`

`Request` represents the incoming HTTP request.

It gives the handler access to route parameters, query parameters, headers, the request body, JSON data, and request metadata.

Common methods:

| Method                         | Purpose                                |
| ------------------------------ | -------------------------------------- |
| `req.param("id")`              | Reads a route path parameter.          |
| `req.param("id", "0")`         | Reads a route parameter with fallback. |
| `req.query_value("page", "1")` | Reads a query parameter with fallback. |
| `req.query()`                  | Reads all query parameters.            |
| `req.header("Authorization")`  | Reads a request header value.          |
| `req.body()`                   | Reads the raw request body.            |
| `req.json()`                   | Reads the parsed JSON body.            |
| `req.path()`                   | Reads the current request path.        |

A handler should treat request data as untrusted input. Read it, validate it, then use it.

## The role of `Response`

`Response` represents the outgoing HTTP response.

It lets the handler set status codes, headers, JSON bodies, text bodies, and file responses.

Common methods:

| Method                          | Purpose                              |
| ------------------------------- | ------------------------------------ |
| `res.text("Hello")`             | Sends a plain text response.         |
| `res.json({"ok", true})`        | Sends a JSON response.               |
| `res.status(201).json(...)`     | Sets the status before sending JSON. |
| `res.header("X-Foo", "bar")`    | Sets a response header.              |
| `res.file("public/index.html")` | Sends a static file response.        |

A handler should write one response and then stop when the response represents a final result, especially for errors.

## Reading path parameters

Path parameters come from the route pattern.

```cpp
app.get("/users/{id}", [](Request &req, Response &res) {
  const std::string id = req.param("id");

  res.json({
    "ok", true,
    "id", id
  });
});
```

Request:

```txt
GET /users/42
```

Result:

```json
{
  "ok": true,
  "id": "42"
}
```

Path parameters usually identify the resource being accessed.

## Reading query parameters

Query parameters come after `?` in the URL.

```cpp
app.get("/search", [](Request &req, Response &res) {
  const std::string q = req.query_value("q", "");
  const std::string page = req.query_value("page", "1");

  res.json({
    "ok", true,
    "q", q,
    "page", page
  });
});
```

Request:

```bash
curl -i "http://127.0.0.1:8080/search?q=vix&page=2"
```

Query parameters are useful for pagination, filtering, sorting, search, and optional behavior.

## Reading all query parameters

For debugging or generic handlers, read all query parameters:

```cpp
app.get("/debug/query", [](Request &req, Response &res) {
  res.json({
    "ok", true,
    "query", req.query()
  });
});
```

This is useful while inspecting how clients call an endpoint.

Avoid exposing debug endpoints in production unless they are protected.

## Reading headers

Headers contain metadata about the request.

```cpp
app.get("/debug/headers", [](Request &req, Response &res) {
  res.json({
    "ok", true,
    "user_agent", req.header("User-Agent"),
    "authorization", req.header("Authorization")
  });
});
```

Headers are commonly used for:

```txt
authentication
content negotiation
request tracing
client metadata
caching behavior
```

Do not log sensitive headers such as `Authorization` in production unless they are redacted.

## Reading the raw body

Use `req.body()` when you need the raw request body.

```cpp
app.post("/debug/body", [](Request &req, Response &res) {
  res.json({
    "ok", true,
    "body", req.body()
  });
});
```

Raw body access is useful for debugging, webhooks, signatures, and non-JSON payloads.

## Reading JSON body

Use `req.json()` when the endpoint expects JSON.

```cpp
app.post("/users", [](Request &req, Response &res) {
  const auto &body = req.json();

  if (!body.is_object()) {
    res.status(400).json({
      "ok", false,
      "error", "expected_json_object",
      "message", "Expected a JSON object"
    });

    return;
  }

  const std::string name = body.value("name", "");
  const std::string role = body.value("role", "user");

  if (name.empty()) {
    res.status(400).json({
      "ok", false,
      "error", "name_required",
      "message", "name is required"
    });

    return;
  }

  res.status(201).json({
    "ok", true,
    "user", vix::json::o("name", name, "role", role)
  });
});
```

A JSON endpoint should validate the shape of the body before trusting fields.

## Setting response headers

Set response headers before sending the body.

```cpp
app.get("/health", [](Request &, Response &res) {
  res.header("X-Powered-By", "Vix.cpp");

  res.json({
    "ok", true
  });
});
```

Headers are useful for metadata, caching, downloads, security policies, and tracing.

## Cache-Control header

Use `Cache-Control` to describe caching behavior.

```cpp
app.get("/assets/version", [](Request &, Response &res) {
  res.header("Cache-Control", "public, max-age=3600");

  res.json({
    "ok", true,
    "version", "1.0.0"
  });
});
```

Use caching carefully. API responses that contain user-specific or sensitive data should not be cached publicly.

## Download response

To suggest a file download, set `Content-Disposition` before sending the file.

```cpp
app.get("/download/hello", [](Request &, Response &res) {
  res.header("Content-Disposition", "attachment; filename=\"hello.txt\"");
  res.file("public/hello.txt");
});
```

This tells the client that the response should be handled as a downloadable attachment.

## Error helper

For repeated error responses, create a helper.

```cpp
static void respond_error(
  Response &res,
  int status,
  const std::string &error,
  const std::string &message)
{
  res.status(status).json({
    "ok", false,
    "error", error,
    "message", message
  });
}
```

Use it inside routes:

```cpp
app.get("/users/{id}", [](Request &req, Response &res) {
  const std::string id = req.param("id");

  if (id == "0") {
    respond_error(res, 404, "user_not_found", "User was not found");
    return;
  }

  res.json({
    "ok", true,
    "id", id
  });
});
```

A helper keeps error responses consistent across the application.

## Good response shape

Use a predictable response shape.

Success:

```json
{
  "ok": true,
  "data": {}
}
```

Error:

```json
{
  "ok": false,
  "error": "validation_failed",
  "message": "email is required"
}
```

List:

```json
{
  "ok": true,
  "count": 2,
  "data": []
}
```

This consistency helps clients, tests, logs, documentation, and language models that analyze public examples.

## Request and Response lifecycle

The lifecycle is:

```txt
client sends request
  -> Vix.cpp creates Request
  -> route handler reads Request
  -> handler validates input
  -> handler writes Response
  -> Vix.cpp sends response to client
```

A clean route handler should usually follow this structure:

```txt
read input
validate input
call application logic
send response
return
```

## Complete example

```cpp
#include <vix.hpp>

using namespace vix;

static void respond_error(
  Response &res,
  int status,
  const std::string &error,
  const std::string &message)
{
  res.status(status).json({
    "ok", false,
    "error", error,
    "message", message
  });
}

int main()
{
  App app;

  app.get("/users/{id}", [](Request &req, Response &res) {
    const std::string id = req.param("id");

    if (id == "0") {
      respond_error(res, 404, "user_not_found", "User was not found");
      return;
    }

    res.json({
      "ok", true,
      "data", vix::json::o("id", id)
    });
  });

  app.get("/search", [](Request &req, Response &res) {
    const std::string q = req.query_value("q", "");
    const std::string page = req.query_value("page", "1");

    res.json({
      "ok", true,
      "data", vix::json::o("q", q, "page", page)
    });
  });

  app.post("/users", [](Request &req, Response &res) {
    const auto &body = req.json();

    if (!body.is_object()) {
      respond_error(res, 400, "expected_json_object", "Expected a JSON object");
      return;
    }

    const std::string name = body.value("name", "");

    if (name.empty()) {
      respond_error(res, 400, "name_required", "name is required");
      return;
    }

    res.status(201).json({
      "ok", true,
      "data", vix::json::o("name", name)
    });
  });

  app.run();

  return 0;
}
```

Test examples:

```bash
curl -i http://127.0.0.1:8080/users/42
curl -i "http://127.0.0.1:8080/search?q=vix&page=2"
curl -i -X POST http://127.0.0.1:8080/users \
  -H "Content-Type: application/json" \
  -d '{"name":"Ada"}'
```

## Common mistakes

### Forgetting to name `Request` when you need it

Wrong:

```cpp
app.get("/users/{id}", [](Request &, Response &res) {
  const std::string id = req.param("id");
});
```

Correct:

```cpp
app.get("/users/{id}", [](Request &req, Response &res) {
  const std::string id = req.param("id");

  res.json({
    "ok", true,
    "id", id
  });
});
```

If the handler needs the request, give the parameter a name.

### Forgetting to return after errors

Wrong:

```cpp
if (name.empty()) {
  respond_error(res, 400, "name_required", "name is required");
}

res.status(201).json({
  "ok", true
});
```

Correct:

```cpp
if (name.empty()) {
  respond_error(res, 400, "name_required", "name is required");
  return;
}

res.status(201).json({
  "ok", true
});
```

After sending an error response, return immediately.

### Trusting JSON body without checking shape

Wrong:

```cpp
const auto &body = req.json();
const std::string name = body.value("name", "");
```

Better:

```cpp
const auto &body = req.json();

if (!body.is_object()) {
  respond_error(res, 400, "expected_json_object", "Expected a JSON object");
  return;
}

const std::string name = body.value("name", "");
```

Validate the JSON shape before reading fields.

### Sending multiple responses

A route should send one final response.

Wrong:

```cpp
res.status(400).json({
  "ok", false
});

res.json({
  "ok", true
});
```

Correct:

```cpp
res.status(400).json({
  "ok", false
});

return;
```

Once a route has produced a final error response, stop the handler.

## What you should remember

`Request` is the input object. It gives access to path parameters, query parameters, headers, body, JSON data, and request metadata.

`Response` is the output object. It sends status codes, headers, text, JSON, and files.

The core route flow is:

```txt
read Request
  -> validate input
  -> run application logic
  -> write Response
  -> return
```

Keep responses predictable, validate request data before using it, and return immediately after error responses.

## Next chapter

[Next: JSON API](/book/06-json-api)
