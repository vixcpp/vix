# dns

The `dns` module defines the asynchronous hostname resolution interface used by Vix async networking.

It provides an abstract, coroutine-friendly resolver that integrates with `io_context` and the async runtime.

---

## Overview

Header: `vix/async/net/dns.hpp`
Namespace: `vix::async::net`

Main components:

- `resolved_address`
- `dns_resolver` (abstract interface)
- `make_dns_resolver(io_context&)`

This layer is intentionally abstract so that different backends (Asio, system resolver, custom cache, etc.) can be plugged in without changing user code.

---

## resolved_address

```cpp
struct resolved_address {
  std::string ip;
  std::uint16_t port{0};
};
```

Represents a single resolved endpoint:

- `ip`: textual IPv4 or IPv6 address
- `port`: port in host byte order

Example:

```cpp
resolved_address a;
a.ip = "93.184.216.34";
a.port = 80;
```

---

## dns_resolver

```cpp
class dns_resolver {
public:
  virtual ~dns_resolver() = default;

  virtual core::task<std::vector<resolved_address>> async_resolve(
      std::string host,
      std::uint16_t port,
      core::cancel_token ct = {}) = 0;
};
```

### Contract

- Resolves a hostname (e.g. `"example.com"`)
- Returns one or more resolved IP addresses
- Supports cooperative cancellation
- Uses `task<>` for coroutine-based integration

### Error Model

`async_resolve` may:

- throw `std::system_error`
- throw runtime-specific resolution errors
- throw cancellation error if `cancel_token` is triggered

Exceptions are rethrown when the awaiting coroutine resumes.

---

## Factory

```cpp
std::unique_ptr<dns_resolver>
make_dns_resolver(core::io_context& ctx);
```

Creates the default resolver implementation associated with the given `io_context`.

The actual backend is runtime-defined (typically Asio-backed).

---

## Example

```cpp
#include <vix/async/net/dns.hpp>
#include <vix/async/core/io_context.hpp>

using namespace vix::async;

core::task<void> resolve_example(core::io_context& ctx)
{
  auto resolver = net::make_dns_resolver(ctx);

  auto results = co_await resolver->async_resolve("example.com", 80);

  for (auto& r : results)
  {
    // r.ip and r.port available
  }

  co_return;
}
```

---

## Design Notes

- DNS is modeled as a pure async operation returning `task<std::vector<resolved_address>>`.
- No blocking APIs are exposed.
- No dependency on concrete socket types.
- Fully compatible with cancellation and scheduler semantics.

---

## Related

- `tcp_stream`
- `udp_socket`
- `io_context`
- `task<T>`
- `cancel_token`

