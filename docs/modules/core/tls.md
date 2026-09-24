# TLS

This page explains TLS support in Vix Core.

Use it when you want to understand how Vix serves HTTPS, how TLS is configured, how TLS sessions work, and how encrypted connections are connected to the normal HTTP session layer.

## Public header

```cpp
#include <vix.hpp>
```

You can also include the TLS headers directly:

```cpp
#include <vix/server/TlsConfig.hpp>
#include <vix/session/TlsSession.hpp>
#include <vix/session/TlsTransport.hpp>
```

## What TLS provides

TLS allows the Vix HTTP server to serve HTTPS connections.

It provides:

- optional HTTPS support
- certificate and private key configuration
- TLS server handshake
- encrypted reads
- encrypted writes
- clean separation between TLS and HTTP parsing
- fallback to plain HTTP when TLS is disabled
- OpenSSL-based implementation when enabled at build time

TLS is optional.

When TLS is disabled, the server accepts plain HTTP connections.

When TLS is enabled and valid, the server wraps accepted TCP streams in a TLS transport before the HTTP session starts.

## Basic model

Plain HTTP uses this flow:

```text
TCP stream
  -> PlainTransport
  -> Session
```

HTTPS uses this flow:

```text
TCP stream
  -> TlsSession
  -> TlsTransport
  -> TLS handshake
  -> Session
```

The normal HTTP session is reused after the TLS handshake.

This means request parsing, routing, handlers, and response writing stay the same.

## TLS configuration

TLS is configured with `vix::server::TlsConfig`.

```cpp
vix::server::TlsConfig tls;

tls.enabled = true;
tls.cert_file = "/etc/letsencrypt/live/example.com/fullchain.pem";
tls.key_file = "/etc/letsencrypt/live/example.com/privkey.pem";
```

The configuration contains:

| Field       | Purpose                       |
| ----------- | ----------------------------- |
| `enabled`   | Enable or disable TLS.        |
| `cert_file` | Path to the certificate file. |
| `key_file`  | Path to the private key file. |

## Configuration checks

`TlsConfig` provides helper methods.

```cpp
tls.is_enabled();
tls.is_configured();
tls.is_valid();
```

Meaning:

| Method            | Meaning                                                    |
| ----------------- | ---------------------------------------------------------- |
| `is_enabled()`    | Returns `true` if TLS is enabled.                          |
| `is_configured()` | Returns `true` if certificate and key paths are not empty. |
| `is_valid()`      | Returns `true` if TLS is enabled and configured.           |

Example:

```cpp
if (tls.is_valid())
{
  vix::print("TLS is ready");
}
```

## Enable TLS from Config

A Vix app normally receives TLS settings through `vix::config::Config`.

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

    res.text("Hello over HTTPS");
  });

  app.run(cfg);

  return 0;
}
```

Then open:

```text
https://localhost:8443
```

## TLS and App

Most applications do not create TLS objects directly.

They configure the app and let the HTTP server choose the right session type.

```text
App
  -> Config
  -> HTTPServer
  -> TlsSession when TLS is valid
```

Application handlers stay the same.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("secure response");
});
```

The handler does not need to know whether the request came through HTTP or HTTPS.

## TLS and HTTPServer

The HTTP server checks TLS configuration when a client connects.

If TLS is valid:

```text
accepted tcp_stream
  -> TlsSession
```

If TLS is disabled or invalid:

```text
accepted tcp_stream
  -> Session
```

The server uses the same router and executor in both cases.

```text
HTTPServer
  -> Router
  -> RuntimeExecutor
```

## TlsSession

`TlsSession` owns an accepted TCP stream during the TLS setup phase.

Its role is:

```text
accepted TCP stream
  -> read TLS config
  -> create TlsTransport
  -> perform TLS handshake
  -> create normal Session
  -> run Session
```

After the handshake, `TlsSession` delegates to the normal HTTP session.

```text
TlsSession
  -> TlsTransport
  -> Session
```

## TlsTransport

`TlsTransport` adapts an accepted TCP stream into an encrypted transport.

It implements the same transport interface used by normal sessions.

```text
Transport
  -> async_read
  -> async_write
  -> is_open
  -> close
```

The difference is that `TlsTransport` decrypts incoming bytes and encrypts outgoing bytes.

```text
encrypted TCP bytes
  -> TlsTransport
  -> decrypted HTTP bytes
  -> Session
```

and:

```text
Session response bytes
  -> TlsTransport
  -> encrypted TCP bytes
```

## TLS handshake

Before the HTTP session can read a request, the TLS handshake must complete.

```cpp
co_await transport->async_handshake();
```

The handshake prepares the encrypted connection.

Conceptually:

```text
create TLS context
  -> load certificate
  -> load private key
  -> attach socket
  -> SSL_accept
  -> handshake complete
```

If the handshake fails, the session stops and the stream is closed.

## Certificate file

The certificate file should usually be the full chain certificate.

Example:

```text
/etc/letsencrypt/live/example.com/fullchain.pem
```

Set it with:

```cpp
cfg.set("server.tls.cert_file", "/etc/letsencrypt/live/example.com/fullchain.pem");
```

## Private key file

The private key file must match the certificate.

Example:

```text
/etc/letsencrypt/live/example.com/privkey.pem
```

Set it with:

```cpp
cfg.set("server.tls.key_file", "/etc/letsencrypt/live/example.com/privkey.pem");
```

If the private key does not match the certificate, TLS initialization fails.

## Plain HTTP fallback

When TLS is disabled:

```cpp
cfg.set("server.tls.enabled", false);
```

The server accepts plain HTTP connections.

```text
tcp_stream
  -> Session
  -> PlainTransport
```

Application code stays the same.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("plain HTTP");
});
```

## Invalid TLS configuration

If TLS is enabled but the certificate or key path is missing, TLS is not usable.

```cpp
vix::server::TlsConfig tls;

tls.enabled = true;
tls.cert_file = "";
tls.key_file = "";
```

Result:

```cpp
tls.is_enabled();     // true
tls.is_configured();  // false
tls.is_valid();       // false
```

The server should not start a valid HTTPS session from this configuration.

## OpenSSL support

TLS support depends on OpenSSL support in the core build.

The core module can be built with OpenSSL enabled.

```text
VIX_CORE_WITH_OPENSSL=ON
```

When OpenSSL is available, Vix can create TLS contexts and perform TLS handshakes.

When OpenSSL is not available, TLS operations are not available and will fail with an error.

## CMake option

The core module exposes an option for OpenSSL support.

```cmake
-DVIX_CORE_WITH_OPENSSL=ON
```

If OpenSSL is found, the core target is compiled with TLS support.

If OpenSSL is requested but not found, the core can build without TLS support.

## TLS version

The TLS implementation requires a modern TLS protocol.

The TLS context sets a minimum protocol version.

```text
TLS 1.2 or newer
```

This avoids older insecure protocol versions.

## TLS read

After the handshake, `async_read(...)` returns decrypted bytes.

```text
encrypted socket bytes
  -> SSL_read
  -> decrypted HTTP bytes
  -> Session parser
```

The session reads from the transport normally.

```cpp
const std::size_t n = co_await transport.async_read(buffer, token);
```

## TLS write

`async_write(...)` receives plaintext HTTP bytes and writes them through TLS.

```text
HTTP response bytes
  -> SSL_write
  -> encrypted socket bytes
```

The session does not encrypt data manually.

It only writes to the transport.

## TLS close

Closing a TLS transport performs a best-effort shutdown.

```text
TlsTransport::close
  -> SSL_shutdown when possible
  -> close TCP stream
  -> release TLS resources
```

Close operations should be safe to call during error handling or shutdown.

## TLS and cancellation

TLS operations receive a cancellation token.

```cpp
task<std::size_t> async_read(
    std::span<std::byte> buffer,
    cancel_token token);
```

```cpp
task<std::size_t> async_write(
    std::span<const std::byte> buffer,
    cancel_token token);
```

If the token is cancelled, the operation can fail with a cancellation error.

This connects TLS operations to session timeout and shutdown behavior.

## TLS and timeouts

Sessions can use timers and cancellation tokens around request work.

The TLS transport participates in that model through the transport API.

```text
start timeout
  -> TLS read or write
  -> timeout fires
  -> cancel token
  -> close transport
```

This prevents encrypted connections from blocking session progress forever.

## TLS and Session

The normal `Session` does not need TLS-specific parsing.

It only sees:

```text
Transport
```

For plain HTTP:

```text
Transport = PlainTransport
```

For HTTPS:

```text
Transport = TlsTransport
```

The HTTP code stays shared.

```text
Session::read_request
Session::dispatch_request
Session::send_response
```

## TLS and Router

TLS does not affect routing.

The router sees the same request shape.

```cpp
app.get("/status", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.json({"status", "ok"});
});
```

The route matches the same way over HTTP or HTTPS.

```text
GET /status
```

## TLS and handlers

Handlers do not change when TLS is enabled.

```cpp
app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("Hello");
});
```

The handler writes a response.

The transport decides whether bytes are written as plain TCP or encrypted TLS.

## TLS architecture

Full HTTPS flow:

```text
client
  -> TCP connection
  -> HTTPServer accept loop
  -> TlsSession
  -> TlsTransport
  -> TLS handshake
  -> Session
  -> read decrypted HTTP request
  -> Router
  -> RequestHandler
  -> user handler
  -> Response
  -> write encrypted HTTP response
  -> client
```

Plain HTTP flow:

```text
client
  -> TCP connection
  -> HTTPServer accept loop
  -> Session
  -> PlainTransport
  -> read HTTP request
  -> Router
  -> RequestHandler
  -> user handler
  -> Response
  -> write HTTP response
  -> client
```

## Environment configuration

Depending on your configuration system, TLS values can come from environment variables.

Common keys:

```text
SERVER_TLS_ENABLED=true
SERVER_TLS_CERT_FILE=/etc/letsencrypt/live/example.com/fullchain.pem
SERVER_TLS_KEY_FILE=/etc/letsencrypt/live/example.com/privkey.pem
SERVER_PORT=8443
```

Then run the app normally:

```cpp
vix::App app;

app.get("/", [](vix::Request &req, vix::Response &res)
{
  (void)req;

  res.text("secure app");
});

app.run();
```

## Local development

For local development, plain HTTP is usually enough.

```cpp
app.run(8080);
```

Use TLS locally only when you need to test HTTPS-specific behavior.

Examples:

- secure cookies
- reverse proxy behavior
- HTTPS-only clients
- production-like testing

## Production usage

In production, there are two common models.

One model is TLS directly in Vix:

```text
client
  -> HTTPS
  -> Vix server
```

Another model is TLS termination at a reverse proxy:

```text
client
  -> HTTPS
  -> Nginx/Caddy
  -> HTTP
  -> Vix server
```

When using a reverse proxy, Vix can run plain HTTP behind the proxy.

## Reverse proxy model

With a reverse proxy:

```text
Internet
  -> Nginx or Caddy handles TLS
  -> Vix listens on localhost:8080
```

Vix configuration:

```cpp
app.run(8080);
```

The proxy handles certificates and HTTPS.

This is often simpler for deployment.

## Direct TLS model

With direct TLS:

```text
Internet
  -> Vix handles TLS
```

Vix configuration:

```cpp
vix::config::Config cfg;

cfg.setServerPort(8443);
cfg.set("server.tls.enabled", true);
cfg.set("server.tls.cert_file", "/etc/letsencrypt/live/example.com/fullchain.pem");
cfg.set("server.tls.key_file", "/etc/letsencrypt/live/example.com/privkey.pem");

app.run(cfg);
```

Use this when you want the Vix process itself to terminate TLS.

## Complete example

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

    res.text("Hello over HTTPS");
  });

  app.get("/api/status", [](vix::Request &req, vix::Response &res)
  {
    (void)req;

    res.json({
      "status", "ok",
      "tls", true
    });
  });

  app.run(cfg);

  return 0;
}
```

Open:

```text
https://localhost:8443
```

## API summary

| API                               | Purpose                                             |
| --------------------------------- | --------------------------------------------------- |
| `TlsConfig::enabled`              | Enable or disable TLS.                              |
| `TlsConfig::cert_file`            | Certificate file path.                              |
| `TlsConfig::key_file`             | Private key file path.                              |
| `TlsConfig::is_enabled()`         | Return whether TLS is enabled.                      |
| `TlsConfig::is_configured()`      | Return whether certificate and key paths are set.   |
| `TlsConfig::is_valid()`           | Return whether TLS is enabled and configured.       |
| `Config::isTlsEnabled()`          | Return whether TLS is enabled in app configuration. |
| `Config::getTlsCertFile()`        | Return TLS certificate path.                        |
| `Config::getTlsKeyFile()`         | Return TLS private key path.                        |
| `Config::getTlsConfig()`          | Return a complete `TlsConfig`.                      |
| `TlsSession::run()`               | Perform TLS session lifecycle.                      |
| `TlsTransport::async_handshake()` | Perform TLS server handshake.                       |
| `TlsTransport::async_read()`      | Read decrypted bytes.                               |
| `TlsTransport::async_write()`     | Write encrypted bytes.                              |
| `TlsTransport::is_open()`         | Check whether the TLS transport is open.            |
| `TlsTransport::close()`           | Close TLS and TCP resources.                        |

## Best practices

Use plain HTTP for local development unless HTTPS behavior matters.

```cpp
app.run(8080);
```

Use a reverse proxy for production when you want simple certificate management.

```text
Nginx/Caddy -> Vix
```

Use direct TLS when the Vix process should terminate HTTPS itself.

```cpp
cfg.set("server.tls.enabled", true);
```

Always provide a matching certificate and private key.

```cpp
cfg.set("server.tls.cert_file", "/path/fullchain.pem");
cfg.set("server.tls.key_file", "/path/privkey.pem");
```

Keep handlers independent from TLS.

```cpp
app.get("/", handler);
```

The handler should not care whether the transport is plain or encrypted.

## Next steps

Read the next pages:

- [Transports](./transports.md)
- [Sessions](./sessions.md)
- [HTTP server](./http-server.md)
- [Configuration](./configuration.md)
- [Async and runtime](./async-and-runtime.md)
