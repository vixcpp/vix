# UDP

This guide shows how to use async UDP with Vix Async.

Use this page when you want to bind a UDP socket, send datagrams, or receive datagrams from an async task.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What UDP provides

The UDP API is built around `udp_socket`.

A UDP socket can bind to a local endpoint, send datagrams to a remote endpoint, receive datagrams from any remote endpoint, close the socket, and report whether it is open.

UDP is message-based. Each send sends one datagram. Each receive reads one datagram into a buffer.

## UDP endpoint

A UDP endpoint has a host and a port.

```cpp
vix::async::net::udp_endpoint endpoint{
    "127.0.0.1",
    9000
};
```

Use it for local bind addresses and remote destinations.

```cpp
vix::async::net::udp_endpoint local{
    "127.0.0.1",
    9000
};

vix::async::net::udp_endpoint remote{
    "127.0.0.1",
    9001
};
```

## Create a UDP socket

Use `make_udp_socket` with an `io_context`.

```cpp
auto socket = vix::async::net::make_udp_socket(ctx);
```

The socket is attached to the async runtime.

## Bind a socket

Use `async_bind` before receiving datagrams.

```cpp
co_await socket->async_bind({
    "127.0.0.1",
    9000
});
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <exception>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  try
  {
    auto socket = vix::async::net::make_udp_socket(ctx);

    co_await socket->async_bind({
        "127.0.0.1",
        9000
    });

    vix::print("udp open =", socket->is_open() ? "yes" : "no");

    socket->close();
  }
  catch (const std::exception &ex)
  {
    vix::eprint(ex.what());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Run:

```
vix run main.cpp
```

Expected output:

```
udp open = yes
```

## Send a datagram

Use `async_send_to` with a byte buffer and a remote endpoint.

```cpp
std::string message = "hello";

const std::size_t bytes = co_await socket->async_send_to(
    std::as_bytes(std::span{message}),
    {
        "127.0.0.1",
        9000
    });
```

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <exception>
#include <span>
#include <string>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  try
  {
    auto socket = vix::async::net::make_udp_socket(ctx);

    std::string message = "hello from Vix UDP";

    const std::size_t bytes = co_await socket->async_send_to(
        std::as_bytes(std::span{message}),
        {
            "127.0.0.1",
            9000
        });

    vix::print("sent bytes =", bytes);

    socket->close();
  }
  catch (const std::exception &ex)
  {
    vix::eprint(ex.what());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

## Receive a datagram

Use `async_recv_from` with a mutable byte buffer.

```cpp
std::array<std::byte, 1024> buffer{};

const auto datagram = co_await socket->async_recv_from(
    std::span<std::byte>{buffer.data(), buffer.size()});
```

The result contains:

| Field                | Purpose                   |
| -------------------- | ------------------------- |
| `datagram.from.host` | Sender IP address.        |
| `datagram.from.port` | Sender port.              |
| `datagram.bytes`     | Number of bytes received. |

Complete example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <array>
#include <exception>
#include <span>
#include <string>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  try
  {
    auto socket = vix::async::net::make_udp_socket(ctx);

    co_await socket->async_bind({
        "127.0.0.1",
        9000
    });

    vix::print("waiting for UDP datagram");

    std::array<std::byte, 1024> buffer{};

    const auto datagram = co_await socket->async_recv_from(
        std::span<std::byte>{buffer.data(), buffer.size()});

    std::string text;
    text.resize(datagram.bytes);

    for (std::size_t i = 0; i < datagram.bytes; ++i)
    {
      text[i] = static_cast<char>(buffer[i]);
    }

    vix::print("from =", datagram.from.host, datagram.from.port);
    vix::print("message =", text);

    socket->close();
  }
  catch (const std::exception &ex)
  {
    vix::eprint(ex.what());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Test from another terminal:

```
printf "hello\n" | nc -u 127.0.0.1 9000
```

Example output:

```
waiting for UDP datagram
from = 127.0.0.1 54321
message = hello
```

## UDP echo server

This example receives one datagram and sends it back to the sender.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <array>
#include <exception>
#include <span>

vix::async::core::task<void> server(vix::async::core::io_context &ctx)
{
  try
  {
    auto socket = vix::async::net::make_udp_socket(ctx);

    co_await socket->async_bind({
        "127.0.0.1",
        9000
    });

    vix::print("udp echo server ready");

    std::array<std::byte, 1024> buffer{};

    const auto datagram = co_await socket->async_recv_from(
        std::span<std::byte>{buffer.data(), buffer.size()});

    vix::print("received bytes =", datagram.bytes);

    (void)co_await socket->async_send_to(
        std::span<const std::byte>{buffer.data(), datagram.bytes},
        datagram.from);

    socket->close();
  }
  catch (const std::exception &ex)
  {
    vix::eprint(ex.what());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = server(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Test:

```
printf "hello\n" | nc -u 127.0.0.1 9000
```

Expected output from the server:

```
udp echo server ready
received bytes = 6
```

## Send and receive in one program

This example starts a receiver and a sender in the same `io_context`.

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <array>
#include <chrono>
#include <span>
#include <string>

using namespace std::chrono_literals;

vix::async::core::task<void> receiver(vix::async::core::io_context &ctx)
{
  auto socket = vix::async::net::make_udp_socket(ctx);

  co_await socket->async_bind({
      "127.0.0.1",
      9000
  });

  std::array<std::byte, 1024> buffer{};

  const auto datagram = co_await socket->async_recv_from(
      std::span<std::byte>{buffer.data(), buffer.size()});

  std::string text;
  text.resize(datagram.bytes);

  for (std::size_t i = 0; i < datagram.bytes; ++i)
  {
    text[i] = static_cast<char>(buffer[i]);
  }

  vix::print("received =", text);

  socket->close();
  ctx.stop();
  co_return;
}

vix::async::core::task<void> sender(vix::async::core::io_context &ctx)
{
  co_await ctx.timers().sleep_for(100ms);

  auto socket = vix::async::net::make_udp_socket(ctx);

  std::string message = "hello";

  (void)co_await socket->async_send_to(
      std::as_bytes(std::span{message}),
      {
          "127.0.0.1",
          9000
      });

  socket->close();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto recv_task = receiver(ctx);
  auto send_task = sender(ctx);

  std::move(recv_task).start(ctx.get_scheduler());
  std::move(send_task).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

Expected output:

```
received = hello
```

## Cancellation

UDP send and receive operations accept a `cancel_token`.

```cpp
vix::async::core::cancel_source source;

auto datagram = co_await socket->async_recv_from(
    std::span<std::byte>{buffer.data(), buffer.size()},
    source.token());
```

You can use cancellation with:

```cpp
co_await socket->async_send_to(buffer, endpoint, token);
co_await socket->async_recv_from(buffer, token);
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <array>
#include <chrono>
#include <exception>
#include <span>
#include <system_error>

using namespace std::chrono_literals;

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::async::core::cancel_source source;

  try
  {
    auto socket = vix::async::net::make_udp_socket(ctx);

    co_await socket->async_bind({
        "127.0.0.1",
        9000
    });

    ctx.timers().after(100ms, [&source]()
    {
      source.request_cancel();
    });

    std::array<std::byte, 1024> buffer{};

    const auto datagram = co_await socket->async_recv_from(
        std::span<std::byte>{buffer.data(), buffer.size()},
        source.token());

    vix::print("received bytes =", datagram.bytes);

    socket->close();
  }
  catch (const std::system_error &ex)
  {
    vix::eprint("udp cancelled or failed:", ex.code().message());
  }
  catch (const std::exception &ex)
  {
    vix::eprint(ex.what());
  }

  ctx.stop();
  co_return;
}

int main()
{
  vix::async::core::io_context ctx;

  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();

  return 0;
}
```

## Close a socket

Use `close()` when the socket is no longer needed.

```cpp
socket->close();
```

Check whether it is open:

```cpp
vix::print("open =", socket->is_open() ? "yes" : "no");
```

## UDP lifecycle

A receiver flow:

```
create io_context
create udp_socket
bind local endpoint
receive datagram
close socket
stop context
```

A sender flow:

```
create io_context
create udp_socket
send datagram to endpoint
close socket
stop context
```

## UDP API summary

| API                                      | Purpose                               |
| ---------------------------------------- | ------------------------------------- |
| `make_udp_socket(ctx)`                   | Creates a UDP socket.                 |
| `udp_endpoint`                           | Describes a UDP host and port.        |
| `udp_datagram`                           | Describes a received UDP datagram.    |
| `async_bind(endpoint)`                   | Binds the socket to a local endpoint. |
| `async_send_to(buffer, endpoint, token)` | Sends one datagram.                   |
| `async_recv_from(buffer, token)`         | Receives one datagram.                |
| `close()`                                | Closes the socket.                    |
| `is_open()`                              | Checks open state.                    |

## Common workflows

### Bind a UDP socket

```cpp
auto socket = vix::async::net::make_udp_socket(ctx);

co_await socket->async_bind({
    "127.0.0.1",
    9000
});
```

### Send a datagram

```cpp
std::string message = "hello";

const std::size_t bytes = co_await socket->async_send_to(
    std::as_bytes(std::span{message}),
    {
        "127.0.0.1",
        9000
    });
```

### Receive a datagram

```cpp
std::array<std::byte, 1024> buffer{};

const auto datagram = co_await socket->async_recv_from(
    std::span<std::byte>{buffer.data(), buffer.size()});
```

### Reply to sender

```cpp
(void)co_await socket->async_send_to(
    std::span<const std::byte>{buffer.data(), datagram.bytes},
    datagram.from);
```

### Cancel a receive

```cpp
vix::async::core::cancel_source source;

const auto datagram = co_await socket->async_recv_from(
    std::span<std::byte>{buffer.data(), buffer.size()},
    source.token());
```

## Common mistakes

### Forgetting to bind before receiving

Wrong:

```cpp
auto socket = vix::async::net::make_udp_socket(ctx);

auto datagram = co_await socket->async_recv_from(buffer);
```

Correct:

```cpp
auto socket = vix::async::net::make_udp_socket(ctx);

co_await socket->async_bind({
    "127.0.0.1",
    9000
});

auto datagram = co_await socket->async_recv_from(buffer);
```

### Passing text directly to async_send_to

`async_send_to` expects bytes.

Wrong:

```cpp
co_await socket->async_send_to(message, endpoint);
```

Correct:

```cpp
co_await socket->async_send_to(
    std::as_bytes(std::span{message}),
    endpoint);
```

### Reading into an immutable buffer

`async_recv_from` needs a mutable byte span.

```cpp
std::array<std::byte, 1024> buffer{};

auto datagram = co_await socket->async_recv_from(
    std::span<std::byte>{buffer.data(), buffer.size()});
```

### Expecting UDP to behave like TCP

UDP is datagram-based. There is no connection stream. Each receive reads one datagram. Each send sends one datagram.

### Ignoring the received byte count

Only `datagram.bytes` bytes are valid in the receive buffer.

```cpp
std::string text;
text.resize(datagram.bytes);
```

### Forgetting to close the socket

```cpp
socket->close();
```

### Forgetting to stop the context

```cpp
ctx.stop();
co_return;
```

## Best practices

Bind before receiving. Keep buffers alive until the awaited operation completes. Use `std::as_bytes` for send buffers. Use mutable `std::span<std::byte>` for receive buffers. Use `datagram.bytes` to read only valid bytes. Use `datagram.from` when replying to the sender. Use cancellation tokens for long-running receives. Close the socket explicitly when finished. Stop the `io_context` when the UDP flow is complete.

## Related pages

| Page                                          | Purpose                          |
| --------------------------------------------- | -------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context.       |
| [Tasks](/modules/async/tasks)                 | Learn coroutine tasks.           |
| [Timers](/modules/async/timers)               | Learn timeout-style flows.       |
| [Cancellation](/modules/async/cancellation)   | Learn cancellation tokens.       |
| [TCP](/modules/async/tcp)                     | Learn TCP streams and listeners. |
| [DNS](/modules/async/dns)                     | Learn hostname resolution.       |
| [API Reference](/modules/async/api-reference) | See the public API surface.      |

## Next step

Continue with [DNS](/modules/async/dns).
