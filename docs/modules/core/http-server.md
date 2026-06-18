# HTTP server

This page explains the native HTTP server used by Vix Core.

Use it when you want to understand how Vix starts the server, accepts TCP connections, runs I/O threads, creates sessions, handles TLS, and shuts down cleanly.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the HTTP server header directly:

```cpp
#include <vix/server/HTTPServer.hpp>
```

## What the HTTP server provides

`vix::server::HTTPServer` is the native asynchronous HTTP server used by `vix::App`.

It is responsible for:

- owning the async I/O context
- binding the TCP listener
- accepting client connections
- launching I/O threads
- creating plain HTTP sessions
- creating TLS sessions when TLS is enabled
- dispatching sessions to the router
- using the runtime executor for application work
- monitoring server state
- stopping and joining internal threads safely

Most applications do not create `HTTPServer` directly.

They use:

```cpp
vix::App app;

app.run(8080);
```

## Basic server through App

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

Run:

```bash
vix run main.cpp
```

Then open:

```text
http://localhost:8080
```

Expected response:

```text
Hello from Vix
```

## Server role in Core

The HTTP server sits below `App` and above `Session`.

```text
App
  -> HTTPServer
    -> io_context
    -> TCP listener
    -> accept loop
    -> Session or TlsSession
```

The simplified flow is:

```text
App::run
  -> App::listen
  -> HTTPServer::run
  -> start I/O threads
  -> start server coroutine
  -> bind listener
  -> start accept loop
```

## Main responsibilities

The HTTP server owns:

```text
io_context
listener
router
executor
I/O threads
metrics thread
shutdown state
```

The server coordinates these pieces so application code can stay simple.

```cpp
vix::App app;
app.run(8080);
```

## I/O context

The HTTP server owns a shared async I/O context.

```cpp
std::shared_ptr<vix::async::core::io_context>
```

The I/O context is used to run:

- listener setup
- TCP accept operations
- session coroutines
- TCP read operations
- TCP write operations
- timers
- cancellation-aware async tasks

The server starts multiple I/O threads and each thread runs the same context.

```text
io_thread_0 -> io_context.run()
io_thread_1 -> io_context.run()
io_thread_N -> io_context.run()
```

## I/O thread count

The server computes the number of I/O threads from hardware concurrency.

If hardware concurrency cannot be detected, a fallback value is used.

Conceptually:

```text
hardware_concurrency()
  -> I/O thread count
```

The goal is to keep the async I/O loop responsive while multiple connections are active.

## Start server

`HTTPServer::run()` starts the server.

It performs the main startup work:

```text
check shutdown state
check executor
start I/O threads
spawn start_server coroutine
start metrics monitor
```

When using `App`, this is called internally.

```cpp
app.run(8080);
```

or:

```cpp
app.listen(8080);
```

## Server startup flow

The startup flow is:

```text
HTTPServer::run
  -> start_io_threads
  -> spawn_detached(start_server)
  -> monitor_metrics
```

Then the async startup coroutine runs:

```text
start_server
  -> validate port
  -> init_listener
  -> start_accept
```

Then the accept loop begins:

```text
accept_loop
  -> async_accept
  -> handle_client
```

## Listener initialization

The server creates a native Vix TCP listener.

```text
make_tcp_listener(io_context)
```

Then it binds and listens on an endpoint.

```text
0.0.0.0:<port>
```

The listener is initialized asynchronously.

```text
init_listener
  -> make_tcp_listener
  -> async_listen
  -> store bound port
```

## Bind endpoint

The default bind endpoint uses:

```text
host = 0.0.0.0
port = config.getServerPort()
```

This allows the server to accept connections from available network interfaces.

For local development, users usually connect through:

```text
http://localhost:8080
```

## Bound port

The server stores the actual bound port.

This is useful when the configured port is `0`.

```cpp
const int port = app.server().bound_port();
```

When port `0` is used, the operating system chooses an available port.

```cpp
app.listen_port(0, [](int port)
{
  vix::print("listening on", port);
});
```

## Accept loop

The accept loop waits for incoming TCP connections.

```text
while server is not stopping
  -> async_accept
  -> if stream accepted
       spawn handle_client(stream)
```

Each accepted stream is handled by its own session coroutine.

```text
accepted TCP stream
  -> handle_client
  -> Session or TlsSession
```

## Client handling

When a client connects, the server chooses the session type.

If TLS is enabled:

```text
tcp_stream
  -> TlsSession
  -> TlsTransport
  -> TLS handshake
  -> Session
```

If TLS is disabled:

```text
tcp_stream
  -> Session
  -> PlainTransport
```

The high-level flow is:

```text
handle_client
  -> check TLS config
  -> create session
  -> co_await session.run()
```

## Plain HTTP session

When TLS is disabled, the server creates a normal session.

```text
tcp_stream
  -> Session
  -> PlainTransport
```

The session reads raw HTTP bytes, parses requests, dispatches to the router, and writes responses.

```text
read request
  -> router
  -> handler
  -> response
  -> write response
```

## TLS session

When TLS is enabled and configured, the server creates a TLS session.

```text
tcp_stream
  -> TlsSession
  -> TlsTransport
  -> async_handshake
  -> Session
```

The TLS layer is separate from the HTTP parser.

After the handshake, the normal HTTP session runs on top of the encrypted transport.

## Transport separation

The HTTP server does not duplicate HTTP logic for plain HTTP and HTTPS.

Instead, the session uses a transport abstraction.

```text
Transport
  -> PlainTransport
  -> TlsTransport
```

The session only needs:

```text
async_read
async_write
is_open
close
```

This keeps the HTTP parser independent from TLS.

## Router ownership

The HTTP server owns the router used to dispatch requests.

`App` retrieves the router from the server.

```cpp
auto router = app.router();
```

Internally:

```text
App
  -> HTTPServer
  -> Router
```

Route registration happens through `App`.

Request dispatch happens through `Session`.

```text
Session
  -> Router::handle_request
  -> RequestHandler
  -> user handler
```

## Executor ownership

The HTTP server receives a shared `RuntimeExecutor`.

```text
HTTPServer
  -> RuntimeExecutor
```

The executor is shared with the application.

```cpp
auto &executor = app.executor();
```

The executor bridges Core to `vix::runtime`.

```text
RuntimeExecutor
  -> vix::runtime::Runtime
  -> Scheduler
  -> Workers
```

## HTTP server and vix::async

The HTTP server uses `vix::async` for network I/O.

This includes:

- TCP listener creation
- listening on an endpoint
- accepting connections
- running session coroutines
- reading request bytes
- writing response bytes
- timers and cancellation in sessions

The server does not use Boost.Asio or Boost.Beast.

Networking is provided by Vix async networking.

## HTTP server and vix::runtime

The HTTP server receives an executor built on `vix::runtime`.

This separates the two models:

```text
vix::async
  -> I/O, sessions, reads, writes, timers

vix::runtime
  -> runtime workers, task scheduling, executor metrics
```

The HTTP server connects both through Core.

## Session lifecycle

For each accepted connection:

```text
accept TCP stream
  -> create Session
  -> run session
  -> read request
  -> dispatch request
  -> write response
  -> repeat while keep-alive
  -> close transport
```

If the connection is kept alive, the same session can process multiple requests.

If the client closes the connection or the response requests close, the session exits.

## Keep-alive

The session can keep a connection open when allowed by the request and response.

Simplified flow:

```text
read request
  -> send response
  -> if keep-alive
       wait for next request
     else
       close connection
```

The HTTP server does not handle this directly.

The session handles it after the connection has been accepted.

## Error handling

The HTTP server treats some accept errors as normal during shutdown.

Examples:

```text
operation canceled
bad file descriptor
listener closed
native cancellation after shutdown
```

These errors can happen when the server is stopping.

They should not always be logged as application errors.

## Closing a client stream

The server has a safe close path for streams.

```text
close_stream
  -> if stream exists
  -> try stream.close()
  -> ignore close exceptions
```

This is used when a stream was accepted but cannot be processed.

## Metrics monitor

The HTTP server starts a background monitoring thread.

In the current model, it periodically waits while the server is running.

This can be extended for:

- metrics
- heartbeats
- health checks
- diagnostics
- runtime status logs

The monitor is stopped during server shutdown.

## Stop async

`stop_async()` requests server shutdown without joining threads by itself.

It performs:

```text
mark stop requested
notify monitor thread
close listener
stop io_context
```

This is useful when shutdown is requested from destructors or app lifecycle code.

## Stop blocking

`stop_blocking()` is the stronger shutdown path.

It performs:

```text
stop async
join internal threads
```

Use it when the caller needs to wait until the server is fully stopped.

`App::close()` uses the blocking shutdown path.

## Join threads

`join_threads()` joins the internal threads owned by the server.

This includes:

- I/O threads
- metrics thread

The operation is idempotent.

Repeated calls should be safe.

## Server destructor

The HTTP server destructor performs best-effort shutdown.

Conceptually:

```text
~HTTPServer
  -> stop_async
  -> join_threads if needed
```

This prevents leaked threads when the server object is destroyed.

## App and server lifecycle

Most applications use the `App` lifecycle.

```cpp
app.run(8080);
```

This expands to:

```text
listen
wait
close
```

With `listen(...)`:

```cpp
app.listen(8080);
app.wait();
app.close();
```

The `App` owns the server and calls the right server lifecycle methods.

## Manual server access

Advanced code can access the server.

```cpp
vix::App app;

auto &server = app.server();
```

This can be useful for:

- reading the bound port
- advanced lifecycle management
- integration tests
- diagnostics
- tooling

Most applications should not need direct server access.

## Example with bound port

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("hello");
  });

  app.listen_port(0, [](int port)
  {
    vix::print("server is listening on", port);
  });

  app.wait();

  return 0;
}
```

Port `0` means the operating system chooses an available port.

## Example with explicit listen and close

```cpp
#include <vix.hpp>

int main()
{
  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("hello");
  });

  app.listen(8080);

  app.wait();

  app.close();

  return 0;
}
```

## Example with configuration

```cpp
#include <vix.hpp>

int main()
{
  vix::config::Config cfg;

  cfg.setServerPort(8080);

  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("configured server");
  });

  app.run(cfg);

  return 0;
}
```

## Example with TLS config

```cpp
#include <vix.hpp>

int main()
{
  vix::config::Config cfg;

  cfg.setServerPort(8443);
  cfg.set("server.tls.enabled", true);
  cfg.set("server.tls.cert_file", "/etc/letsencrypt/live/example.com/fullchain.pem");
  cfg.set("server.tls.key_file", "/etc/letsencrypt/live/example.com/privkey.pem");

  vix::App app;

  app.get("/", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.text("secure hello");
  });

  app.run(cfg);

  return 0;
}
```

When TLS is enabled and valid, the server uses `TlsSession`.

## Server architecture

The server architecture is:

```text
HTTPServer
  -> io_context
  -> tcp_listener
  -> router
  -> runtime executor
  -> I/O threads
  -> metrics thread
```

Connection architecture:

```text
TCP connection
  -> tcp_stream
  -> Session or TlsSession
  -> Transport
  -> Request parser
  -> Router
  -> Handler
  -> Response writer
```

## Startup sequence

Detailed startup:

```text
App::listen
  -> create server thread
  -> HTTPServer::run
  -> start_io_threads
  -> spawn start_server
  -> monitor_metrics

start_server
  -> validate port
  -> init_listener
  -> start_accept

start_accept
  -> spawn accept_loop
```

## Request sequence

Detailed request flow:

```text
accept_loop
  -> listener.async_accept
  -> handle_client
  -> Session::run
  -> read_request
  -> dispatch_request
  -> router.handle_request
  -> request handler
  -> send_response
```

## Shutdown sequence

Detailed shutdown:

```text
App::close
  -> server.stop_async
  -> server.stop_blocking
  -> join server thread

HTTPServer::stop_async
  -> set stop_requested
  -> notify metrics thread
  -> close listener
  -> stop io_context

HTTPServer::join_threads
  -> join I/O threads
  -> join metrics thread
```

## API summary

| API                            | Purpose                                               |
| ------------------------------ | ----------------------------------------------------- |
| `HTTPServer(config, executor)` | Construct a server from configuration and executor.   |
| `run()`                        | Start I/O threads, startup coroutine, and monitoring. |
| `start_accept()`               | Start the async accept loop.                          |
| `calculate_io_thread_count()`  | Compute the number of I/O threads.                    |
| `getRouter()`                  | Return the shared router.                             |
| `monitor_metrics()`            | Start the background monitor thread.                  |
| `stop_async()`                 | Request asynchronous shutdown.                        |
| `stop_blocking()`              | Stop and wait for internal threads.                   |
| `join_threads()`               | Join server-owned threads.                            |
| `is_stop_requested()`          | Return whether shutdown was requested.                |
| `bound_port()`                 | Return the actual bound TCP port.                     |
| `executor()`                   | Return the shared runtime executor.                   |

## Best practices

Use `vix::App` for normal applications.

```cpp
vix::App app;
app.run(8080);
```

Use `listen_port(0, ...)` in tests when you want a free port.

```cpp
app.listen_port(0, callback);
```

Prefer configuration for server options.

```cpp
vix::config::Config cfg;
app.run(cfg);
```

Let `App` manage shutdown.

```cpp
app.run(8080);
```

Use direct `HTTPServer` access only for advanced tooling, diagnostics, or tests.

```cpp
auto &server = app.server();
```

## Next steps

Read the next pages:

- [Sessions](./sessions.md)
- [Transports](./transports.md)
- [TLS](./tls.md)
- [Runtime executor](./runtime-executor.md)
- [Async and runtime](./async-and-runtime.md)
