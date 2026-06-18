# Transports

This page explains the transport layer used by Vix Core sessions.

Use it when you want to understand how the HTTP session reads and writes bytes without depending directly on plain TCP or TLS.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the transport headers directly:

```cpp
#include <vix/session/Transport.hpp>
#include <vix/session/PlainTransport.hpp>
#include <vix/session/TlsTransport.hpp>
```

## What transports provide

A transport is the byte I/O abstraction used by the HTTP session.

It provides a common interface for:

- reading bytes
- writing bytes
- checking whether the connection is open
- closing the connection

This lets the HTTP session work with both plain HTTP and HTTPS without duplicating HTTP parsing logic.

```text
Session
  -> Transport
      -> PlainTransport
      -> TlsTransport
```

## Why transports exist

The HTTP session should not care whether bytes come from a plain TCP stream or an encrypted TLS stream.

The session only needs to know:

```text
read bytes
write bytes
check open
close
```

So Vix uses a shared transport interface.

```text
Plain HTTP
  -> PlainTransport
  -> Session

HTTPS
  -> TlsTransport
  -> Session
```

After the transport is ready, the same `Session` logic handles HTTP parsing, routing, and response writing.

## Transport interface

The core interface is:

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

The interface is asynchronous and uses `vix::async::core::task`.

## Transport methods

| Method                       | Purpose                                           |
| ---------------------------- | ------------------------------------------------- |
| `async_read(buffer, token)`  | Read bytes into a buffer.                         |
| `async_write(buffer, token)` | Write bytes from a buffer.                        |
| `is_open()`                  | Return whether the underlying connection is open. |
| `close()`                    | Close the transport safely.                       |

## Transport and async

Transports use `vix::async`.

That means read and write operations return async tasks.

```cpp
task<std::size_t> async_read(...);
task<std::size_t> async_write(...);
```

The session can then do:

```cpp
const std::size_t n = co_await transport.async_read(buffer, token);
```

and:

```cpp
co_await transport.async_write(buffer, token);
```

This keeps I/O non-blocking from the session point of view.

## PlainTransport

`PlainTransport` adapts a native Vix TCP stream to the generic `Transport` interface.

```text
tcp_stream
  -> PlainTransport
  -> Transport
  -> Session
```

It is used for normal HTTP connections without TLS.

## Plain transport flow

For a plain HTTP connection:

```text
HTTPServer accepts tcp_stream
  -> creates Session
  -> Session creates PlainTransport
  -> Session reads HTTP bytes
  -> Session writes HTTP response
```

Simplified flow:

```text
client
  -> TCP
  -> PlainTransport
  -> Session
  -> Router
  -> Handler
  -> Response
  -> PlainTransport
  -> TCP
  -> client
```

## PlainTransport read

`PlainTransport::async_read(...)` delegates to the underlying TCP stream.

```text
PlainTransport::async_read
  -> tcp_stream::async_read
```

The session does not need to know that the transport is backed by TCP.

## PlainTransport write

`PlainTransport::async_write(...)` delegates to the underlying TCP stream.

```text
PlainTransport::async_write
  -> tcp_stream::async_write
```

The session passes serialized HTTP bytes to the transport.

## PlainTransport close

`PlainTransport::close()` closes the underlying TCP stream.

The close operation is best-effort.

If closing throws internally, the transport ignores the exception.

This makes shutdown paths safer.

## TlsTransport

`TlsTransport` adapts a native Vix TCP stream into an encrypted transport.

```text
tcp_stream
  -> TlsTransport
  -> TLS handshake
  -> Transport
  -> Session
```

It is used for HTTPS connections.

## TLS transport flow

For HTTPS:

```text
HTTPServer accepts tcp_stream
  -> creates TlsSession
  -> creates TlsTransport
  -> performs TLS handshake
  -> creates Session with TLS transport
  -> Session reads decrypted HTTP bytes
  -> Session writes encrypted response bytes
```

After the handshake, the HTTP session works the same as it does for plain HTTP.

## TLS handshake

`TlsTransport` must complete the TLS server handshake before the session reads HTTP requests.

```cpp
co_await transport->async_handshake();
```

Conceptually:

```text
accepted TCP stream
  -> TLS context
  -> certificate
  -> private key
  -> SSL_accept
  -> encrypted connection ready
```

If the handshake fails, the connection is closed and the session does not continue.

## TLS read

After the handshake, reads return decrypted bytes.

```text
TLS encrypted bytes
  -> TlsTransport::async_read
  -> decrypted HTTP bytes
  -> Session parser
```

From the session point of view, this looks like a normal read.

```cpp
const std::size_t n = co_await transport.async_read(buffer, token);
```

## TLS write

Writes accept normal plaintext HTTP response bytes.

```text
Session response bytes
  -> TlsTransport::async_write
  -> encrypted TLS bytes
  -> TCP stream
```

The session does not need to perform encryption itself.

## TLS close

`TlsTransport::close()` closes both the TLS connection state and the underlying TCP stream.

The close path is best-effort.

If TLS shutdown or socket close fails, the transport still releases resources safely.

## Transport and Session

The session owns a transport.

```text
Session
  -> std::unique_ptr<Transport>
```

This allows the session to use dynamic dispatch without knowing the concrete transport type.

```text
Session::read_header_block
  -> transport.async_read

Session::send_response
  -> transport.async_write

Session::close_stream_gracefully
  -> transport.close
```

## Session constructors

A session can be constructed from a TCP stream.

```text
Session(tcp_stream)
  -> creates PlainTransport internally
```

Or it can be constructed from a generic transport.

```text
Session(Transport)
  -> uses provided transport
```

This second form is used by TLS.

```text
TlsSession
  -> creates TlsTransport
  -> creates Session(TlsTransport)
```

## Plain vs TLS

The important difference is where encryption happens.

```text
PlainTransport
  -> reads and writes raw TCP bytes

TlsTransport
  -> reads decrypted bytes
  -> writes encrypted bytes
```

The session sees both as:

```text
Transport
```

So HTTP parsing stays shared.

## Transport and cancellation

Transport reads and writes accept a cancellation token.

```cpp
cancel_token token
```

This is used for:

- request timeouts
- session shutdown
- server shutdown
- cancelled operations

Example shape:

```cpp
co_await transport.async_read(buffer, timer_cancel_.token());
```

If cancellation is requested, the operation can stop.

## Transport and timeouts

The session starts a timer for request or response work.

If the timeout fires, the session can cancel the token and close the connection.

```text
start timer
  -> transport read/write
  -> timeout fires
  -> cancel token
  -> close transport
```

This keeps slow connections from holding the session forever.

## Transport and keep-alive

The transport remains open while the session handles keep-alive requests.

```text
request 1
  -> response 1
request 2
  -> response 2
request 3
  -> response 3
```

When the connection closes, the transport closes.

```text
Connection: close
  -> Session closes Transport
```

## Transport and errors

Transport errors are handled by the session.

Common normal disconnects include:

```text
EOF
connection reset
broken pipe
operation canceled
timeout
```

The session can treat these as normal connection lifecycle events.

Unexpected read or write failures can be logged and cause the connection to close.

## Transport architecture

The simplified architecture is:

```text
HTTPServer
  -> accepts tcp_stream

Plain HTTP:
  -> Session
  -> PlainTransport
  -> tcp_stream

HTTPS:
  -> TlsSession
  -> TlsTransport
  -> TLS handshake
  -> Session
  -> Transport
```

The shared part is:

```text
Session
  -> read request
  -> router
  -> handler
  -> write response
```

## Plain HTTP example

Application code does not create the transport directly.

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

Flow:

```text
client
  -> HTTPServer
  -> Session
  -> PlainTransport
  -> handler
```

## HTTPS example

Application code stays almost the same.

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

    res.text("secure HTTP");
  });

  app.run(cfg);

  return 0;
}
```

Flow:

```text
client
  -> HTTPServer
  -> TlsSession
  -> TlsTransport
  -> Session
  -> handler
```

## Transport abstraction benefit

The transport abstraction keeps the HTTP session simple.

Without transport abstraction:

```text
Session must know plain TCP
Session must know TLS
Session must duplicate read/write logic
```

With transport abstraction:

```text
Session only knows Transport
PlainTransport handles TCP
TlsTransport handles TLS
```

This keeps Core easier to maintain and extend.

## Future transport extensions

The same interface can support more transports later.

Possible examples:

```text
Unix domain socket transport
in-memory test transport
custom encrypted transport
proxy transport
instrumented transport
```

The session would not need to change as long as the transport implements:

```text
async_read
async_write
is_open
close
```

## Testing with transports

Because the session depends on `Transport`, advanced tests can provide a custom transport.

A test transport could simulate:

- request bytes
- partial reads
- write failures
- disconnects
- timeouts
- malformed requests

This makes the session easier to test independently from real sockets.

## Transport lifecycle

The lifecycle is:

```text
construct transport
  -> read/write while open
  -> close on error, timeout, shutdown, or connection close
  -> destroy transport
```

For TLS:

```text
construct TlsTransport
  -> TLS handshake
  -> read/write while open
  -> TLS shutdown and socket close
  -> destroy transport
```

## Complete architecture flow

```text
App
  -> HTTPServer
    -> accepts tcp_stream

Plain HTTP:
  -> Session
    -> PlainTransport
      -> tcp_stream

HTTPS:
  -> TlsSession
    -> TlsTransport
      -> TLS handshake
    -> Session
      -> Transport

Both:
  -> read request
  -> Router
  -> RequestHandler
  -> Response
  -> write response
```

## API summary

| API                                     | Purpose                                      |
| --------------------------------------- | -------------------------------------------- |
| `Transport::async_read(buffer, token)`  | Read bytes from the connection.              |
| `Transport::async_write(buffer, token)` | Write bytes to the connection.               |
| `Transport::is_open()`                  | Check whether the connection is open.        |
| `Transport::close()`                    | Close the connection.                        |
| `PlainTransport(tcp_stream)`            | Adapt a TCP stream to `Transport`.           |
| `TlsTransport(tcp_stream, config)`      | Adapt a TCP stream to encrypted `Transport`. |
| `TlsTransport::async_handshake(token)`  | Perform the TLS server handshake.            |

## Best practices

Use `vix::App` for normal application code.

```cpp
vix::App app;
app.run(8080);
```

Do not create transports manually unless you are extending Core, writing tests, or building advanced integrations.

Use `static_dir`, routes, handlers, and templates for application behavior.

```cpp
app.get("/", handler);
app.static_dir("public", "/assets");
```

Use TLS configuration to switch from plain transport to TLS transport.

```cpp
cfg.set("server.tls.enabled", true);
```

Keep HTTP logic independent from the transport type.

```text
Handler code should not care whether the request came through HTTP or HTTPS.
```

## Next steps

Read the next pages:

- [TLS](./tls.md)
- [Sessions](./sessions.md)
- [HTTP server](./http-server.md)
- [Async and runtime](./async-and-runtime.md)
- [Configuration](./configuration.md)
