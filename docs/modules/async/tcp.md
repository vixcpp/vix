# TCP (async/net)

This page documents the public TCP interfaces in **`vix::async::net`**.

These types are intentionally small and abstract:
- `tcp_stream` represents a connected TCP socket.
- `tcp_listener` represents a listening socket that accepts connections.
- `make_tcp_stream()` and `make_tcp_listener()` create runtime-backed implementations (typically Asio-backed) that integrate with `vix::async::core::io_context`.

The design goal is to keep the public API stable while allowing the internal networking backend to evolve without leaking implementation details into user code.

---

## Files

Typical include:

```cpp
#include <vix/async/net/tcp.hpp>
```

Related core pieces used by the TCP layer:

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/cancel.hpp>
#include <vix/async/core/error.hpp>
```

---

## Types

### tcp_endpoint

A simple endpoint description used by both connect and listen operations.

```cpp
struct tcp_endpoint {
  std::string host;
  std::uint16_t port{0};
};
```

Notes:
- `host` can be a DNS name (`"example.com"`) or an IP string (`"127.0.0.1"`, `"::1"`).
- `port` is in host byte order.

---

## tcp_stream

`tcp_stream` is an abstract interface for a connected TCP socket.

```cpp
class tcp_stream {
public:
  virtual ~tcp_stream() = default;

  virtual core::task<void> async_connect(
      const tcp_endpoint& ep,
      core::cancel_token ct = {}) = 0;

  virtual core::task<std::size_t> async_read(
      std::span<std::byte> buf,
      core::cancel_token ct = {}) = 0;

  virtual core::task<std::size_t> async_write(
      std::span<const std::byte> buf,
      core::cancel_token ct = {}) = 0;

  virtual void close() noexcept = 0;
  virtual bool is_open() const noexcept = 0;
};
```

### Behavior rules

- **Coroutine-based:** all async operations return `core::task<...>`.
- **Cancellation:** operations accept a `core::cancel_token`.
  - If cancellation is observed, implementations should fail with `std::system_error(cancelled_ec())`.
- **Errors:** networking failures should be reported via `std::system_error` (e.g. connect refused, host not found, broken pipe).
- **Partial IO:** `async_read` and `async_write` return the number of bytes actually transferred.
  - A return value of `0` from `async_read()` typically indicates "peer closed" (EOF), depending on backend semantics.
- **Idempotent close:** `close()` may be called multiple times safely.
- **State:** `is_open()` reports whether the underlying socket is usable.

### Buffer types

The API uses `std::span`:
- Read: `std::span<std::byte>` (mutable destination buffer)
- Write: `std::span<const std::byte>` (immutable source buffer)

This allows callers to manage buffers explicitly and avoids hidden allocations.

---

## tcp_listener

`tcp_listener` is an abstract interface for a listening socket that accepts connections.

```cpp
class tcp_listener {
public:
  virtual ~tcp_listener() = default;

  virtual core::task<void> async_listen(
      const tcp_endpoint& bind_ep,
      int backlog = 128) = 0;

  virtual core::task<std::unique_ptr<tcp_stream>> async_accept(
      core::cancel_token ct = {}) = 0;

  virtual void close() noexcept = 0;
  virtual bool is_open() const noexcept = 0;
};
```

### Behavior rules

- **async_listen():**
  - Binds to `bind_ep.host:bind_ep.port`
  - Starts listening with the provided `backlog`
  - Fails with `std::system_error` on bind/listen errors

- **async_accept():**
  - Waits for one incoming connection and returns a `std::unique_ptr<tcp_stream>`
  - Accept can be canceled via `cancel_token`
  - Accept can fail with `std::system_error` (closed listener, system error, etc.)

- **close():**
  - Stops accepting new connections
  - Wakes an ongoing `async_accept()` depending on backend behavior

---

## Factories

These functions create concrete runtime-backed implementations that are associated with an `io_context`.

```cpp
std::unique_ptr<tcp_stream> make_tcp_stream(core::io_context& ctx);
std::unique_ptr<tcp_listener> make_tcp_listener(core::io_context& ctx);
```

### Why factories

- Keeps the public API minimal and stable
- Allows different backends (Asio today, potentially others later)
- Centralizes integration with the scheduler in `io_context`

---

## Usage examples

### 1) Connect + write + read (client)

```cpp
#include <array>
#include <cstddef>
#include <string>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/spawn.hpp>
#include <vix/async/net/tcp.hpp>

using namespace vix::async;

static core::task<void> client(core::io_context& ctx)
{
  auto s = net::make_tcp_stream(ctx);

  net::tcp_endpoint ep;
  ep.host = "127.0.0.1";
  ep.port = 8080;

  co_await s->async_connect(ep);

  const std::string msg = "ping\n";
  std::span<const std::byte> out{
      reinterpret_cast<const std::byte*>(msg.data()),
      msg.size()
  };

  (void)co_await s->async_write(out);

  std::array<std::byte, 1024> buf{};
  const std::size_t n = co_await s->async_read(std::span<std::byte>{buf.data(), buf.size()});

  // You decide how to interpret bytes (text, binary, framing, etc).
  (void)n;

  s->close();
  co_return;
}

static void run_client()
{
  core::io_context ctx;
  core::spawn_detached(ctx, client(ctx));
  ctx.run();
}

int main()
{
  run_client();
}
```

Notes:
- `spawn_detached()` schedules a `task<void>` without returning a join handle.
- IO is buffer-explicit; conversions from `std::string` to bytes are done explicitly.

---

### 2) Listen + accept loop (server)

```cpp
#include <array>
#include <cstddef>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/spawn.hpp>
#include <vix/async/net/tcp.hpp>

using namespace vix::async;

static core::task<void> handle_client(std::unique_ptr<net::tcp_stream> s)
{
  std::array<std::byte, 1024> buf{};

  while (s->is_open())
  {
    const std::size_t n = co_await s->async_read(std::span<std::byte>{buf.data(), buf.size()});
    if (n == 0)
      break;

    // echo back
    (void)co_await s->async_write(std::span<const std::byte>{buf.data(), n});
  }

  s->close();
  co_return;
}

static core::task<void> server(core::io_context& ctx)
{
  auto l = net::make_tcp_listener(ctx);

  net::tcp_endpoint bind;
  bind.host = "0.0.0.0";
  bind.port = 8080;

  co_await l->async_listen(bind, 128);

  while (l->is_open())
  {
    auto s = co_await l->async_accept();
    core::spawn_detached(ctx, handle_client(std::move(s)));
  }

  co_return;
}

static void run_server()
{
  core::io_context ctx;
  core::spawn_detached(ctx, server(ctx));
  ctx.run();
}

int main()
{
  run_server();
}
```

Notes:
- This is a minimal echo server. Real servers add framing, protocol parsing, backpressure, and timeouts.
- Spawning one detached coroutine per accepted connection is a simple model. Higher-level servers may use connection limits or pools.

---

## Cancellation pattern

Cancellation is cooperative: you pass a `cancel_token` into async calls.

```cpp
#include <vix/async/core/cancel.hpp>

vix::async::core::cancel_source cs;
auto ct = cs.token();

// Later: request cancellation from any thread
cs.request_cancel();

// Then pass ct into async_connect/read/write/accept
```

The exact point where cancellation is observed depends on the backend implementation.

---

## Design notes

- The TCP API is intentionally low-level: it does not impose framing, buffering strategy, or protocol rules.
- `std::span<std::byte>` makes IO explicit and avoids implicit conversions.
- Abstract interfaces + factories keep user code stable while allowing backend improvements.

