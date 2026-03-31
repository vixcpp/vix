---
title: Minimal HTTP Server (Vix async)
---

# Minimal HTTP Server (Vix async)

This example shows how to build a tiny HTTP server using **Vix async**:

- single `io_context` event loop
- `tcp_listener` accept loop
- per-connection coroutine handler
- graceful stop on `SIGINT` / `SIGTERM`
- log output via `vix::console`

It is intentionally minimal so you can copy/paste it into a demo or evolve it into a real HTTP module.

---

## What you get

- Listens on `0.0.0.0:8080`
- Accepts multiple clients
- Reads the HTTP headers until `\r\n\r\n`
- Responds with a fixed `200 OK` and the body:

```
Hello from Vix async
```

- Closes the connection after sending the response

---

## Full code

```cpp
#include <cstddef>
#include <csignal>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/signal.hpp>
#include <vix/async/core/spawn.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/net/tcp.hpp>
#include <vix/console.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

namespace
{
  static std::string ok(std::string_view body)
  {
    std::string r;
    r += "HTTP/1.1 200 OK\r\n";
    r += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    r += "Connection: close\r\n\r\n";
    r.append(body.data(), body.size());
    return r;
  }

  static task<void> client(std::unique_ptr<vix::async::net::tcp_stream> c)
  {
    if (!c || !c->is_open())
      co_return;

    std::vector<std::byte> buf(2048);
    std::string req;

    while (c->is_open() && req.find("\r\n\r\n") == std::string::npos)
    {
      std::size_t n = co_await c->async_read(std::span<std::byte>(buf.data(), buf.size()));
      if (n == 0)
        break;

      req.append(reinterpret_cast<const char *>(buf.data()), n);

      // Safety cap to avoid unbounded memory growth for this demo
      if (req.size() > 64 * 1024)
        break;
    }

    const std::string resp = ok("Hello from Vix async\n");
    co_await c->async_write(std::span<const std::byte>(
        reinterpret_cast<const std::byte *>(resp.data()), resp.size()));

    c->close();
    co_return;
  }

  static task<void> server(io_context &ctx)
  {
    // Stop cleanly on Ctrl+C or a SIGTERM from a process manager
    auto &sig = ctx.signals();
    sig.add(SIGINT);
    sig.add(SIGTERM);
    sig.on_signal([&](int)
                  { vix::console.warn("[http] stop"); ctx.stop(); });

    auto l = vix::async::net::make_tcp_listener(ctx);

    // Note: the endpoint expects a uint16_t port, hence the cast
    co_await l->async_listen({"0.0.0.0", static_cast<uint16_t>(8080)}, 128);

    vix::console.log("[http] ready http://0.0.0.0:8080");

    while (ctx.is_running())
    {
      auto c = co_await l->async_accept();
      vix::async::core::spawn_detached(ctx, client(std::move(c)));
    }

    l->close();
    co_return;
  }

} // namespace

int main()
{
  io_context ctx;

  // Start the server coroutine (task starts suspended in Vix async)
  auto t = server(ctx);
  ctx.post(t.handle());

  // Run the event loop until ctx.stop()
  ctx.run();
  return 0;
}
```

---

## Run it

### 1) Compile and run with Vix

```bash
vix run server.cpp
```

If you want colored console output (when supported):

```bash
vix run server.cpp --log-color=always
```

You should see something like:

```
[http] ready http://0.0.0.0:8080
```

### 2) Test with curl

```bash
curl -i http://127.0.0.1:8080
```

### 3) Test with HTTPie

```bash
http :8080
```

---

## How it works

### `io_context`

`io_context` is the runtime event loop. We post the coroutine handle and then run the loop:

- `ctx.post(t.handle())` schedules the coroutine to start
- `ctx.run()` drives I/O, timers, and coroutine resumes
- `ctx.stop()` stops the loop (triggered by signals in this example)

### `server()` coroutine

The server coroutine is responsible for:

1) Installing signal handlers (`SIGINT`, `SIGTERM`)
2) Listening on an address and port (`async_listen`)
3) Accepting connections (`async_accept`)
4) Spawning one detached coroutine per connection (`spawn_detached`)

### `client()` coroutine

The client handler does:

1) Read data until it sees `\r\n\r\n` (end of headers)
2) Write a minimal HTTP response
3) Close the socket

---

## Notes and limitations

This is a minimal demo, so it does **not**:

- parse request method/path
- handle request bodies (`Content-Length`)
- support keep-alive
- implement timeouts
- implement proper HTTP header handling

It is meant to be a clear starting point.

---

## Next steps

If you want to evolve this into a real HTTP server, the next upgrades are:

1) Parse the request line (`GET /path HTTP/1.1`)
2) Add routing (`/`, `/health`, `/metrics`)
3) Support `Content-Length` for POST bodies
4) Add read/write timeouts
5) Keep-alive and request pipelining (optional)

If you plan to publish this as a reusable example, you can also package it for the Vix Registry and ship it with `vix publish`.

