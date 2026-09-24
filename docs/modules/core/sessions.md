# Sessions

This page explains HTTP sessions in Vix Core.

Use it when you want to understand how one client connection is processed, how requests are read, how responses are written, how keep-alive works, how timeouts are handled, and how sessions use transports.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the session header directly:

```cpp
#include <vix/session/Session.hpp>
```

## What sessions provide

A session represents one connected client.

It is responsible for:

- reading bytes from a transport
- parsing HTTP request headers
- reading request bodies
- creating `vix::Request`
- applying basic request validation
- dispatching requests to the router
- writing `vix::Response`
- handling keep-alive
- handling request timeouts
- closing the connection safely

Most applications do not create sessions directly.

They use:

```cpp
vix::App app;

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello");
});

app.run(8080);
```

## Session role in Core

A session sits between the HTTP server and the router.

```text
HTTPServer
  -> Session
  -> Transport
  -> Request parser
  -> Router
  -> RequestHandler
  -> Response writer
```

The server accepts a TCP connection.

The session owns the lifecycle of that connection.

```text
accepted TCP stream
  -> Session
  -> read request
  -> dispatch request
  -> send response
  -> repeat or close
```

## Basic request flow

For one request, the session flow is:

```text
read raw bytes
  -> parse request head
  -> read body
  -> build Request
  -> dispatch to Router
  -> receive Response
  -> serialize response
  -> write bytes
```

Application code only sees:

```cpp
app.get("/hello", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello from Vix");
});
```

## Session lifecycle

A session starts with `run()`.

```text
Session::run
  -> read_request
  -> dispatch_request
  -> send_response
  -> repeat while transport is open
  -> close_stream_gracefully
```

A session can process more than one request if the connection stays open.

```text
request 1
  -> response 1
request 2
  -> response 2
close
```

## Session and Transport

The session does not read directly from a concrete TCP or TLS type.

It uses the `Transport` interface.

```text
Transport
  -> async_read
  -> async_write
  -> is_open
  -> close
```

This keeps the HTTP session independent from whether the connection is plain HTTP or HTTPS.

```text
PlainTransport
  -> normal TCP

TlsTransport
  -> TLS-encrypted TCP
```

## Plain session

For normal HTTP:

```text
tcp_stream
  -> PlainTransport
  -> Session
```

The plain transport adapts `vix::async::net::tcp_stream` to the generic session transport interface.

## TLS session

For HTTPS:

```text
tcp_stream
  -> TlsSession
  -> TlsTransport
  -> TLS handshake
  -> Session
```

After the TLS handshake, the same `Session` logic is used.

This avoids duplicating HTTP parsing and response writing.

## Reading a request

The session reads a request in stages.

```text
read_header_block
  -> parse_request_head
  -> read_request_body
  -> make_request
```

This produces a native Vix request object:

```cpp
vix::http::Request
```

Application handlers receive it as:

```cpp
vix::Request &req
```

## Header block

The session reads raw bytes until it finds the HTTP header terminator.

```text
\r\n\r\n
```

The header block contains:

```text
GET /path HTTP/1.1
Host: localhost:8080
Connection: keep-alive
```

Once the header terminator is found, the session can parse the request line and headers.

## Request head

The parsed request head contains:

```text
method
target
version
headers
content_length
keep_alive
```

Example:

```text
GET /users/42?page=1 HTTP/1.1
Host: localhost:8080
Connection: keep-alive
```

becomes:

```text
method = GET
target = /users/42?page=1
version = HTTP/1.1
keep_alive = true
```

## Request body

If the request has a body, the session reads it according to `Content-Length`.

Example:

```text
POST /api/users HTTP/1.1
Content-Type: application/json
Content-Length: 16

{"name":"Ada"}
```

The session reads the body and stores it in the `Request`.

```cpp
req.body();
```

## Request size limits

The session protects the server from oversized requests.

The default maximum request body size is:

```text
10 MB
```

If the request body is too large, the session can return:

```text
413 Payload Too Large
```

## Building Request

After parsing, the session builds a native request.

```text
ParsedRequestHead + body
  -> vix::http::Request
```

The request contains:

- method
- target
- path
- query string
- headers
- body
- route parameters later added by the handler adapter
- request state

Example handler:

```cpp
app.post("/echo", [](vix::Request &req, vix::Response &res)
{
  res.json({
    "method", req.method(),
    "path", req.path(),
    "body", req.body()
  });
});
```

## Dispatching a request

Once a request is built, the session dispatches it to the router.

```text
Session::dispatch_request
  -> Router::handle_request
  -> RequestHandler
  -> user handler
```

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

The router matches:

```text
GET /status
```

then runs the registered handler.

## Sending a response

After the handler writes a response, the session serializes it.

The session writes:

```text
status line
headers
blank line
body
```

Example response:

```text
HTTP/1.1 200 OK
Content-Type: text/plain; charset=utf-8
Content-Length: 2
Server: Vix.cpp

OK
```

Application code writes the response through `vix::Response`.

```cpp
res.text("OK");
```

The session handles serialization and transport writes.

## Small responses

For small responses, the session can combine headers and body into one write buffer.

```text
headers + body
  -> write_all
```

This reduces overhead for small HTTP responses.

## Larger responses

For larger responses, the session can write headers first, then the body.

```text
write headers
write body
```

This avoids unnecessary extra copying for bigger response bodies.

## Writing all bytes

The session writes until the full response is sent.

```text
while written < size
  -> async_write remaining bytes
```

If the transport writes `0` bytes or fails, the session closes the connection.

## Keep-alive

The session can keep a connection open after sending a response.

Simplified flow:

```text
read request
  -> send response
  -> if keep-alive
       read next request
     else
       close connection
```

This allows multiple HTTP requests on the same connection.

## Connection close

A request or response can ask to close the connection.

Example request header:

```text
Connection: close
```

The response can also mark itself as close.

```cpp
res.res.set_should_close(true);
res.header("Connection", "close");
```

Most application code does not need to control this manually.

## Default response headers

Before sending, the session ensures important headers exist.

Common headers include:

```text
Server: Vix.cpp
Date: <http date>
Content-Length: <body size>
Connection: keep-alive
```

If the handler already set a header, the session keeps the handler value.

## Timeouts

The session can start a per-request timeout.

The timeout protects the server from slow or stuck connections.

Conceptually:

```text
start timer
  -> wait for configured timeout
  -> close connection if request is still active
```

When the request finishes, the timer is cancelled.

```text
read request complete
  -> cancel timer

write response complete
  -> cancel timer
```

## Cancellation

Session timeouts use cancellation tokens.

```text
cancel_source
  -> cancel_token
  -> async read/write/timer operations
```

When cancellation is requested, pending operations can stop.

This keeps shutdown and timeout behavior predictable.

## Normal disconnects

Some disconnects are normal and should not be treated as fatal server errors.

Examples:

```text
client closed connection
EOF
connection reset
broken pipe
operation canceled
timeout
```

The session treats these as expected connection lifecycle events.

## Malformed requests

If a request is malformed, the session can send:

```text
400 Bad Request
```

Example response:

```json
{
  "message": "Malformed HTTP request"
}
```

After that, the session can close the connection.

## WAF checks

The session applies basic request checks before dispatching to the router.

These checks can block suspicious requests before they reach user handlers.

The checks can include:

- target length
- invalid target characters
- suspicious URL patterns
- suspicious body patterns
- maximum body size

When a request is blocked, the session can return:

```text
400 Bad Request
```

## Benchmark mode

In benchmark mode, the session can use a very fast path for benchmark routes.

This is useful for measuring the raw HTTP stack with minimal application overhead.

Normal application code does not need to interact with benchmark mode directly.

## Closing a session

A session closes the stream gracefully when it exits.

```text
close_stream_gracefully
  -> cancel active timer
  -> close transport
```

The close path is best-effort.

Close errors are ignored because the connection is already ending.

## Session with plain HTTP

Example flow:

```text
HTTPServer accepts tcp_stream
  -> creates Session
  -> Session creates PlainTransport
  -> Session::run
  -> read HTTP request
  -> router dispatch
  -> write HTTP response
```

Application code:

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("plain HTTP");
  });

  app.run(8080);

  return 0;
}
```

## Session with HTTPS

Example flow:

```text
HTTPServer accepts tcp_stream
  -> creates TlsSession
  -> TlsSession creates TlsTransport
  -> TLS handshake
  -> creates Session with TLS transport
  -> Session::run
```

Application code remains the same.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("secure HTTP");
});
```

TLS is configured through server configuration.

## Session and Router

The session sends parsed requests to the router.

```text
Session
  -> Router::handle_request
```

The router finds the correct handler.

```text
method + path
  -> RouteNode
  -> RequestHandler
```

Then the handler writes a response.

```cpp
res.json({
  "status", "ok"
});
```

## Session and Response

The session is responsible for turning a response object into HTTP bytes.

```text
vix::http::Response
  -> status line
  -> headers
  -> body
  -> transport.async_write
```

This means handlers do not write raw HTTP manually.

They use:

```cpp
res.text("OK");
res.json({"ok", true});
res.file("public/index.html");
```

## Session and async

Sessions are asynchronous.

They use `vix::async::core::task<void>`.

Important operations are awaited:

```cpp
co_await read_request()
co_await dispatch_request()
co_await send_response()
co_await close_stream_gracefully()
```

The session runs on the HTTP server I/O context.

## Session and runtime executor

The session receives the runtime executor from the HTTP server.

```text
HTTPServer
  -> Session
  -> RuntimeExecutor
```

The executor exists so application work can be connected to `vix::runtime`.

This keeps the architecture ready for separating I/O work from runtime execution.

## Session architecture

The simplified architecture is:

```text
Session
  -> Transport
  -> Config
  -> Router
  -> RuntimeExecutor
  -> read buffer
  -> cancel source
```

The session depends on these components but keeps its main job focused:

```text
read request
dispatch request
send response
close connection
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
      "status", "ok"
    });
  });

  app.post("/api/echo", [](vix::Request &req, vix::Response &res)
  {
    res.json({
      "body", req.body()
    });
  });

  app.run(8080);

  return 0;
}
```

Request flow for `/api/status`:

```text
client
  -> TCP connection
  -> HTTPServer
  -> Session
  -> read request
  -> Router
  -> handler
  -> response JSON
  -> Session writes response
  -> client
```

## API summary

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

Most of these methods are internal to Core.

Application code usually works with:

```cpp
vix::App
vix::Request
vix::Response
```

## Best practices

Use `vix::App` instead of creating sessions manually.

```cpp
vix::App app;
app.run(8080);
```

Keep handlers small and clear.

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "ok"});
});
```

Return immediately after sending errors.

```cpp
if (!allowed)
{
  res.status(403).json({"error", "forbidden"});
  return;
}
```

Do not write raw HTTP from handlers.

```cpp
res.text("OK");
```

Use configuration for timeouts, WAF, TLS, and server behavior.

```cpp
vix::config::Config cfg;
app.run(cfg);
```

## Next steps

Read the next pages:

- [Transports](./transports.md)
- [TLS](./tls.md)
- [HTTP server](./http-server.md)
- [Request](./request.md)
- [Response](./response.md)
- [Runtime executor](./runtime-executor.md)
