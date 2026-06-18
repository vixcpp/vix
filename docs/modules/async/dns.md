# DNS

This guide shows how to use async DNS resolution with Vix Async.

Use this page when you want to resolve a hostname into one or more IP addresses from an async task.

## Public header

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>
```

## What DNS provides

The DNS API is built around `dns_resolver`.

A DNS resolver can resolve hostnames asynchronously, return one or more resolved addresses, attach a port to each resolved address, and support cooperative cancellation with `cancel_token`.

## Resolved address

A resolved address contains an IP address and a port.

```cpp
struct resolved_address
{
  std::string ip;
  std::uint16_t port;
};
```

## Create a DNS resolver

Use `make_dns_resolver` with an `io_context`.

```cpp
auto resolver = vix::async::net::make_dns_resolver(ctx);
```

The resolver is attached to the async runtime.

## Resolve a hostname

Use `async_resolve`.

```cpp
auto addresses = co_await resolver->async_resolve(
    "example.com",
    80);
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
    auto resolver = vix::async::net::make_dns_resolver(ctx);

    auto addresses = co_await resolver->async_resolve(
        "example.com",
        80);

    for (const auto &address : addresses)
    {
      vix::print(address.ip, address.port);
    }
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

Example output:

```
93.184.216.34 80
```

## Resolve localhost

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <exception>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  try
  {
    auto resolver = vix::async::net::make_dns_resolver(ctx);

    auto addresses = co_await resolver->async_resolve(
        "localhost",
        8080);

    for (const auto &address : addresses)
    {
      vix::print("ip =", address.ip);
      vix::print("port =", address.port);
    }
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

Example output:

```
ip = 127.0.0.1
port = 8080
```

## Multiple results

A hostname can resolve to multiple addresses.

```cpp
auto addresses = co_await resolver->async_resolve(
    "example.com",
    443);

vix::print("count =", addresses.size());
```

Example:

```cpp
#include <vix/async.hpp>
#include <vix/print.hpp>

#include <exception>

vix::async::core::task<void> app(vix::async::core::io_context &ctx)
{
  try
  {
    auto resolver = vix::async::net::make_dns_resolver(ctx);

    auto addresses = co_await resolver->async_resolve(
        "example.com",
        443);

    vix::print("resolved addresses =", addresses.size());

    for (const auto &address : addresses)
    {
      vix::print(address.ip, address.port);
    }
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

## Use DNS before TCP

DNS is often used before opening a TCP connection.

```cpp
auto resolver = vix::async::net::make_dns_resolver(ctx);

auto addresses = co_await resolver->async_resolve(
    "example.com",
    80);
```

Then connect with TCP:

```cpp
auto stream = vix::async::net::make_tcp_stream(ctx);

co_await stream->async_connect({
    addresses.front().ip,
    addresses.front().port
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
    auto resolver = vix::async::net::make_dns_resolver(ctx);

    auto addresses = co_await resolver->async_resolve(
        "example.com",
        80);

    if (addresses.empty())
    {
      vix::eprint("no address resolved");
      ctx.stop();
      co_return;
    }

    auto stream = vix::async::net::make_tcp_stream(ctx);

    co_await stream->async_connect({
        addresses.front().ip,
        addresses.front().port
    });

    vix::print("connected to =", addresses.front().ip);

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

## Cancellation

DNS resolution accepts a `cancel_token`.

```cpp
vix::async::core::cancel_source source;

auto addresses = co_await resolver->async_resolve(
    "example.com",
    80,
    source.token());
```

Use cancellation when a DNS lookup is no longer needed.

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
    auto resolver = vix::async::net::make_dns_resolver(ctx);

    auto addresses = co_await resolver->async_resolve(
        "example.com",
        80,
        source.token());

    vix::print("resolved addresses =", addresses.size());
  }
  catch (const std::system_error &ex)
  {
    vix::eprint("dns cancelled or failed:", ex.code().message());
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

## Error handling

DNS failures can throw exceptions. Use `try` and `catch` around DNS operations.

```cpp
try
{
  auto addresses = co_await resolver->async_resolve(
      "invalid.host.example",
      80);

  vix::print("resolved =", addresses.size());
}
catch (const std::exception &ex)
{
  vix::eprint(ex.what());
}
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
    auto resolver = vix::async::net::make_dns_resolver(ctx);

    auto addresses = co_await resolver->async_resolve(
        "invalid.host.example",
        80);

    vix::print("resolved =", addresses.size());
  }
  catch (const std::exception &ex)
  {
    vix::eprint("resolve failed:", ex.what());
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

## DNS lifecycle

A normal DNS flow looks like this:

```
create io_context
create dns_resolver
resolve hostname and port
use resolved addresses
stop context
```

In code:

```cpp
auto resolver = vix::async::net::make_dns_resolver(ctx);

auto addresses = co_await resolver->async_resolve(
    "example.com",
    80);
```

## DNS API summary

| API                                | Purpose                              |
| ---------------------------------- | ------------------------------------ |
| `make_dns_resolver(ctx)`           | Creates a DNS resolver.              |
| `dns_resolver`                     | Abstract async resolver interface.   |
| `resolved_address`                 | Represents one resolved IP and port. |
| `async_resolve(host, port, token)` | Resolves a hostname asynchronously.  |

## Common workflows

### Resolve a hostname

```cpp
auto resolver = vix::async::net::make_dns_resolver(ctx);

auto addresses = co_await resolver->async_resolve(
    "example.com",
    80);
```

### Print resolved addresses

```cpp
for (const auto &address : addresses)
{
  vix::print(address.ip, address.port);
}
```

### Check for empty results

```cpp
if (addresses.empty())
{
  vix::eprint("no address resolved");
}
```

### Resolve before TCP connect

```cpp
auto addresses = co_await resolver->async_resolve(
    "example.com",
    80);

auto stream = vix::async::net::make_tcp_stream(ctx);

co_await stream->async_connect({
    addresses.front().ip,
    addresses.front().port
});
```

### Resolve with cancellation

```cpp
vix::async::core::cancel_source source;

auto addresses = co_await resolver->async_resolve(
    "example.com",
    80,
    source.token());
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

### Using DNS outside a coroutine

Wrong:

```cpp
resolver->async_resolve("example.com", 80);
```

Correct:

```cpp
auto addresses = co_await resolver->async_resolve(
    "example.com",
    80);
```

### Ignoring empty results

Always check that the result list contains at least one address before using `front()`.

Wrong:

```cpp
auto endpoint = addresses.front();
```

Correct:

```cpp
if (addresses.empty())
{
  vix::eprint("no address resolved");
  ctx.stop();
  co_return;
}

auto endpoint = addresses.front();
```

### Forgetting to handle DNS errors

```cpp
try
{
  auto addresses = co_await resolver->async_resolve(
      "example.com",
      80);
}
catch (const std::exception &ex)
{
  vix::eprint(ex.what());
}
```

### Confusing DNS with TCP connect

DNS resolves names to addresses. TCP connects to an endpoint.

```cpp
auto addresses = co_await resolver->async_resolve(
    "example.com",
    80);

auto stream = vix::async::net::make_tcp_stream(ctx);

co_await stream->async_connect({
    addresses.front().ip,
    addresses.front().port
});
```

## Best practices

Use DNS before TCP when you start with a hostname. Check whether the returned address list is empty. Handle exceptions around DNS resolution. Use cancellation tokens for lookups that may no longer be needed. Keep the resolver attached to the same `io_context` as the rest of the async flow. Stop the `io_context` when the top-level DNS flow is complete.

## Related pages

| Page                                          | Purpose                          |
| --------------------------------------------- | -------------------------------- |
| [io_context](/modules/async/io-context)       | Learn the runtime context.       |
| [Tasks](/modules/async/tasks)                 | Learn coroutine tasks.           |
| [Timers](/modules/async/timers)               | Learn timeout-style flows.       |
| [Cancellation](/modules/async/cancellation)   | Learn cancellation tokens.       |
| [TCP](/modules/async/tcp)                     | Learn TCP streams and listeners. |
| [UDP](/modules/async/udp)                     | Learn UDP sockets.               |
| [API Reference](/modules/async/api-reference) | See the public API surface.      |

## Next step

Continue with the [API reference](/modules/async/api-reference).
