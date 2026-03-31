# UDP (async/net)

`udp.hpp` defines Vix.cpp's coroutine-first UDP abstraction: endpoints, datagram metadata, and an `udp_socket` interface that can be implemented by a runtime backend (typically Asio) and integrated with `vix::async::core::io_context`.

UDP is message oriented: you send and receive datagrams. There is no connection and no delivery guarantee. This API models that directly.

## Header

```cpp
#include <vix/async/net/udp.hpp>
```

Namespace: `vix::async::net`

Related core types:
- `vix::async::core::task<T>`
- `vix::async::core::cancel_token`
- `vix::async::core::io_context`

---

## Types

### udp_endpoint

Represents a UDP endpoint by host and port.

```cpp
struct udp_endpoint {
  std::string host;
  std::uint16_t port{0};
};
```

Notes:
- `host` can be a hostname or an IP string (IPv4 or IPv6).
- `port` is in host byte order.

### udp_datagram

Metadata returned by `async_recv_from()`.

```cpp
struct udp_datagram {
  udp_endpoint from;
  std::size_t bytes{0};
};
```

Meaning:
- `from`: the sender endpoint.
- `bytes`: number of bytes written into the receive buffer.

---

## udp_socket

`udp_socket` is the abstract interface for coroutine-based UDP operations.

```cpp
class udp_socket {
public:
  virtual ~udp_socket() = default;

  virtual core::task<void> async_bind(const udp_endpoint& bind_ep) = 0;

  virtual core::task<std::size_t> async_send_to(
    std::span<const std::byte> buf,
    const udp_endpoint& to,
    core::cancel_token ct = {}
  ) = 0;

  virtual core::task<udp_datagram> async_recv_from(
    std::span<std::byte> buf,
    core::cancel_token ct = {}
  ) = 0;

  virtual void close() noexcept = 0;
  virtual bool is_open() const noexcept = 0;
};
```

### async_bind(bind_ep)

Binds the socket to a local endpoint.

- Completes when binding succeeds.
- Throws `std::system_error` on failure.

### async_send_to(buf, to, ct)

Sends one datagram.

- Returns the number of bytes sent.
- Throws `std::system_error` on send failure or cancellation.

### async_recv_from(buf, ct)

Receives one datagram into `buf`.

- Returns `udp_datagram` with sender + byte count.
- Throws `std::system_error` on receive failure or cancellation.

### close() and is_open()

- `close()` is idempotent.
- `is_open()` reports whether the underlying socket is open.

---

## Factory

### make_udp_socket(ctx)

Creates a UDP socket associated with a core `io_context`.

```cpp
std::unique_ptr<udp_socket> make_udp_socket(core::io_context& ctx);
```

The concrete type is runtime dependent and typically backed by Asio.

---

## Cancellation

Most operations accept `core::cancel_token`:
- If cancellation is observed, the implementation should fail the operation by throwing `std::system_error(core::cancelled_ec())`.
- If you pass a default token `{}`, the operation is not cancel aware.

Example token setup:

```cpp
vix::async::core::cancel_source src;
auto ct = src.token();

// later, from another context or thread:
src.request_cancel();
```

---

## Example: UDP echo (single socket)

This example shows:
- bind
- recv_from
- send_to back to sender

```cpp
#include <array>
#include <cstddef>
#include <string_view>

#include <vix/async/core/io_context.hpp>
#include <vix/async/net/udp.hpp>

using namespace vix::async;

static core::task<void> udp_echo_server(core::io_context& ctx)
{
  auto sock = net::make_udp_socket(ctx);

  co_await sock->async_bind(net::udp_endpoint{"0.0.0.0", 9000});

  std::array<std::byte, 2048> buf{};

  while (sock->is_open())
  {
    auto dg = co_await sock->async_recv_from(std::span<std::byte>(buf.data(), buf.size()));

    // echo back exactly what we received
    std::span<const std::byte> out(buf.data(), dg.bytes);
    co_await sock->async_send_to(out, dg.from);
  }

  co_return;
}

int main()
{
  core::io_context ctx;

  // Spawn the server and run the scheduler
  core::spawn_detached(ctx, udp_echo_server(ctx));
  ctx.run();

  return 0;
}
```

---

## Example: UDP discovery ping

A typical use case in Vix style is discovery and heartbeats. This sketch sends a ping datagram to a known endpoint.

```cpp
#include <array>
#include <cstddef>
#include <cstring>

#include <vix/async/core/io_context.hpp>
#include <vix/async/net/udp.hpp>

using namespace vix::async;

static core::task<void> send_ping(core::io_context& ctx)
{
  auto sock = net::make_udp_socket(ctx);

  // Optional: bind to an ephemeral port
  co_await sock->async_bind(net::udp_endpoint{"0.0.0.0", 0});

  std::array<std::byte, 4> msg{};
  std::memcpy(msg.data(), "PING", 4);

  std::span<const std::byte> out(msg.data(), msg.size());
  co_await sock->async_send_to(out, net::udp_endpoint{"127.0.0.1", 37020});

  sock->close();
  co_return;
}

int main()
{
  core::io_context ctx;
  core::spawn_detached(ctx, send_ping(ctx));
  ctx.run();
  return 0;
}
```

---

## Behavior and design notes

- UDP is datagram based: each `async_send_to()` corresponds to one datagram, and `async_recv_from()` returns one datagram.
- Your receive buffer size matters: if the datagram is larger than the buffer, the backend may truncate it (backend specific).
- `udp_endpoint.host` is a string in the public API. Backend implementations usually resolve it to an address internally.
- `close()` should wake any pending operations according to backend rules. If a pending coroutine resumes, it should see an error (for example `errc::closed`) or a cancellation style error depending on implementation.

---

## Common patterns

- Bind once, then loop on `async_recv_from()` for servers.
- For clients: optionally bind to port 0, then send and receive.
- Combine UDP with `timer` for retry windows, and with `cancel_token` for shutdown.

