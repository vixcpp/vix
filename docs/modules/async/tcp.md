# TCP

This guide shows how to use async TCP with Vix Async.

Use this page when you want to connect to a TCP server, read and write bytes, or accept incoming TCP connections from an async server.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What TCP provides

The TCP API is built around two interfaces.

| Type           | Purpose                            |
| -------------- | ---------------------------------- |
| `tcp_stream`   | Represents a connected TCP stream. |
| `tcp_listener` | Represents a listening TCP socket. |

A TCP stream can connect to a remote endpoint, read bytes, write bytes, close the connection, and expose the native socket handle when supported.

A TCP listener can listen on a local endpoint, accept incoming connections, and close the listener.

## TCP endpoint

A TCP endpoint has a host and a port.

```cpp
vix::async::net::tcp_endpoint endpoint{
    "127.0.0.1",
    8080
};
```

You can use an IP address or a hostname:

```cpp
vix::async::net::tcp_endpoint endpoint{
    "example.com",
    80
};
```

## Create a TCP stream

Use `make_tcp_stream` with an `io_context`.

```cpp
auto stream = vix::async::net::make_tcp_stream(ctx);
```

The stream is attached to the async runtime.

## Connect to a server

Use `async_connect`.

```cpp
co_await stream->async_connect({
    "example.com",
    80
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
    auto stream = vix::async::net::make_tcp_stream(ctx);

    co_await stream->async_connect({
        "example.com",
        80
    });

    vix::print("connected =", stream->is_open() ? "yes" : "no");

    stream->close();
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

## Write to a TCP stream

Use `async_write` with a byte buffer.

```cpp
std::string request = "hello";

auto bytes = co_await stream->async_write(
    std::as_bytes(std::span{request}));
```

Example:

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
    auto stream = vix::async::net::make_tcp_stream(ctx);

    co_await stream->async_connect({
        "127.0.0.1",
        8080
    });

    std::string message = "hello from Vix";

    const std::size_t bytes = co_await stream->async_write(
        std::as_bytes(std::span{message}));

    vix::print("written =", bytes);

    stream->close();
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

## Read from a TCP stream

Use `async_read` with a mutable byte buffer.

```cpp
std::array<std::byte, 1024> buffer{};

const std::size_t bytes = co_await stream->async_read(
    std::span<std::byte>{buffer.data(), buffer.size()});
```

Example:

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
    auto stream = vix::async::net::make_tcp_stream(ctx);

    co_await stream->async_connect({
        "127.0.0.1",
        8080
    });

    std::array<std::byte, 1024> buffer{};

    const std::size_t bytes = co_await stream->async_read(
        std::span<std::byte>{buffer.data(), buffer.size()});

    std::string text;
    text.resize(bytes);

    for (std::size_t i = 0; i < bytes; ++i)
    {
      text[i] = static_cast<char>(buffer[i]);
    }

    vix::print("received =", text);

    stream->close();
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

## HTTP request example

TCP streams can be used to write simple protocols.

This example sends a minimal HTTP request.

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
    auto stream = vix::async::net::make_tcp_stream(ctx);

    co_await stream->async_connect({
        "example.com",
        80
    });

    std::string request =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    const std::size_t written = co_await stream->async_write(
        std::as_bytes(std::span{request}));

    vix::print("written =", written);

    std::array<std::byte, 4096> buffer{};

    const std::size_t read = co_await stream->async_read(
        std::span<std::byte>{buffer.data(), buffer.size()});

    std::string response;
    response.resize(read);

    for (std::size_t i = 0; i < read; ++i)
    {
      response[i] = static_cast<char>(buffer[i]);
    }

    vix::print(response);

    stream->close();
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

## Create a TCP listener

Use `make_tcp_listener`.

```cpp
auto listener = vix::async::net::make_tcp_listener(ctx);
```

Then listen on an endpoint.

```cpp
co_await listener->async_listen({
    "127.0.0.1",
    8080
});
```

## Accept a connection

Use `async_accept`.

```cpp
auto client = co_await listener->async_accept();
```

The accepted connection is returned as a `std::unique_ptr<tcp_stream>`.

## Minimal TCP server

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <array>
#include <exception>
#include <span>
#include <string>

vix::async::core::task<void> server(vix::async::core::io_context &ctx)
{
  try
  {
    auto listener = vix::async::net::make_tcp_listener(ctx);

    co_await listener->async_listen({
        "127.0.0.1",
        8080
    });

    vix::print("listening on 127.0.0.1:8080");

    auto client = co_await listener->async_accept();

    vix::print("client accepted");

    std::string message = "hello from Vix TCP\n";

    (void)co_await client->async_write(
        std::as_bytes(std::span{message}));

    client->close();
    listener->close();
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

Test from another terminal:

```
nc 127.0.0.1 8080
```

Expected output:

```
hello from Vix TCP
```

## Echo server

This example accepts one client, reads data, and writes it back.

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
    auto listener = vix::async::net::make_tcp_listener(ctx);

    co_await listener->async_listen({
        "127.0.0.1",
        8080
    });

    vix::print("echo server ready");

    auto client = co_await listener->async_accept();

    std::array<std::byte, 1024> buffer{};

    const std::size_t bytes = co_await client->async_read(
        std::span<std::byte>{buffer.data(), buffer.size()});

    (void)co_await client->async_write(
        std::span<const std::byte>{buffer.data(), bytes});

    client->close();
    listener->close();
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
printf "hello\n" | nc 127.0.0.1 8080
```

Expected output:

```
hello
```

## Cancellation

TCP operations accept a `cancel_token`.

```cpp
vix::async::core::cancel_source source;

co_await stream->async_connect(endpoint, source.token());
```

You can use cancellation with:

```cpp
co_await stream->async_connect(endpoint, token);
co_await stream->async_read(buffer, token);
co_await stream->async_write(buffer, token);
co_await listener->async_accept(token);
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <chrono>
#include <exception>
#include <system_error>

using namespace std::chrono_literals;

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  vix::async::core::cancel_source source;

  ctx.timers().after(100ms, [&source]()
  {
    source.request_cancel();
  });

  try
  {
    auto stream = vix::async::net::make_tcp_stream(ctx);

    co_await stream->async_connect({
        "10.255.255.1",
        80
    }, source.token());

    stream->close();
  }
  catch (const std::system_error &ex)
  {
    vix::eprint("tcp cancelled or failed:", ex.code().message());
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

## Close a stream

Use `close()` when the connection is no longer needed.

```cpp
stream->close();
```

Check whether it is open:

```cpp
vix::print("open =", stream->is_open() ? "yes" : "no");
```

## Close a listener

Use `close()` when the listener should stop accepting new connections.

```cpp
listener->close();
```

Check whether it is open:

```cpp
vix::print("listening =", listener->is_open() ? "yes" : "no");
```

## Native handle

`tcp_stream::native_handle()` returns the native socket handle when supported.

```cpp
const int fd = stream->native_handle();
```

This is mainly useful for lower-level integrations such as TLS adapters. Most application code should not need it.

## TCP lifecycle

Client flow:

```
create io_context
create tcp_stream
connect
write or read
close stream
stop context
```

Server flow:

```
create io_context
create tcp_listener
listen
accept client
read or write
close client
close listener
stop context
```

## TCP API summary

| API                               | Purpose                                      |
| --------------------------------- | -------------------------------------------- |
| `make_tcp_stream(ctx)`            | Creates a TCP stream.                        |
| `make_tcp_listener(ctx)`          | Creates a TCP listener.                      |
| `async_connect(endpoint, token)`  | Connects to a remote endpoint.               |
| `async_read(buffer, token)`       | Reads bytes from a stream.                   |
| `async_write(buffer, token)`      | Writes bytes to a stream.                    |
| `async_listen(endpoint, backlog)` | Starts listening on a local endpoint.        |
| `async_accept(token)`             | Accepts one incoming connection.             |
| `close()`                         | Closes a stream or listener.                 |
| `is_open()`                       | Checks open state.                           |
| `native_handle()`                 | Returns native socket handle when supported. |

## Common workflows

### Connect to a server

```cpp
auto stream = vix::async::net::make_tcp_stream(ctx);

co_await stream->async_connect({
    "example.com",
    80
});
```

### Write bytes

```cpp
std::string message = "hello";

const std::size_t bytes = co_await stream->async_write(
    std::as_bytes(std::span{message}));
```

### Read bytes

```cpp
std::array<std::byte, 1024> buffer{};

const std::size_t bytes = co_await stream->async_read(
    std::span<std::byte>{buffer.data(), buffer.size()});
```

### Listen for one connection

```cpp
auto listener = vix::async::net::make_tcp_listener(ctx);

co_await listener->async_listen({
    "127.0.0.1",
    8080
});

auto client = co_await listener->async_accept();
```

### Close resources

```cpp
client->close();
listener->close();
```

## Common mistakes

### Forgetting to run the context

Wrong:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());
```

Correct:

```cpp
auto t = app(ctx);
std::move(t).start(ctx.get_scheduler());

ctx.run();
```

### Forgetting to stop the context

After the TCP flow is complete, stop the runtime.

```cpp
ctx.stop();
co_return;
```

### Using TCP APIs outside a coroutine

Wrong:

```cpp
stream->async_connect({"127.0.0.1", 8080});
```

Correct:

```cpp
co_await stream->async_connect({"127.0.0.1", 8080});
```

### Passing a text buffer directly to async_write

`async_write` expects bytes.

Wrong:

```cpp
co_await stream->async_write(message);
```

Correct:

```cpp
co_await stream->async_write(
    std::as_bytes(std::span{message}));
```

### Reading into an immutable buffer

`async_read` needs a mutable byte span.

```cpp
std::array<std::byte, 1024> buffer{};

const std::size_t bytes = co_await stream->async_read(
    std::span<std::byte>{buffer.data(), buffer.size()});
```

### Forgetting to close sockets

Close streams and listeners when you are done.

```cpp
stream->close();
listener->close();
```

### Expecting async_read to fill the whole buffer

`async_read` returns the number of bytes actually read. Use the returned byte count.

## Best practices

Use `tcp_stream` for outgoing client connections. Use `tcp_listener` for incoming server connections. Keep buffers alive until the awaited operation completes. Use `std::as_bytes` for write buffers. Use mutable `std::span<std::byte>` for read buffers. Handle `std::system_error` around network operations. Use cancellation tokens for long-running connect, read, write, and accept operations. Close streams and listeners explicitly when finished. Stop the `io_context` when the top-level TCP flow is complete.

## Related pages

| Page                                          | Purpose                     |
| --------------------------------------------- | --------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context.  |
| [Tasks](/modules/async/tasks)                 | Learn coroutine tasks.      |
| [Timers](/modules/async/timers)               | Learn timeout-style flows.  |
| [Cancellation](/modules/async/cancellation)   | Learn cancellation tokens.  |
| [UDP](/modules/async/udp)                     | Learn UDP sockets.          |
| [DNS](/modules/async/dns)                     | Learn hostname resolution.  |
| [API Reference](/modules/async/api-reference) | See the public API surface. |

## Next step

Continue with [UDP](/modules/async/udp).
