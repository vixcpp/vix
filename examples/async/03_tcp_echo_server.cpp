#include <csignal>
#include <system_error>
#include <vector>

#include <vix/console.hpp>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/signal.hpp>
#include <vix/async/core/spawn.hpp>

#include <vix/async/net/tcp.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

static task<void> handle_client(std::unique_ptr<vix::async::net::tcp_stream> client)
{
  vix::console.info("[async] client connected");

  std::vector<std::byte> buf(4096);

  while (client && client->is_open())
  {
    std::size_t n = 0;

    try
    {
      n = co_await client->async_read(std::span<std::byte>(buf.data(), buf.size()));
    }
    catch (const std::system_error &e)
    {
      vix::console.error("[async] read error:", e.code().message());
      break;
    }

    if (n == 0)
      break;

    try
    {
      co_await client->async_write(std::span<const std::byte>(buf.data(), n));
    }
    catch (const std::system_error &e)
    {
      vix::console.error("[async] write error:", e.code().message());
      break;
    }
  }

  client->close();
  vix::console.info("[async] client disconnected");
  co_return;
}

static task<void> server(io_context &ctx)
{
  auto &sig = ctx.signals();
  sig.add(SIGINT);
  sig.add(SIGTERM);

  sig.on_signal([&](int s)
                {
    vix::console.warn("[async] signal", s, "received -> stopping");
    ctx.stop(); });

  auto listener = vix::async::net::make_tcp_listener(ctx);

  co_await listener->async_listen({"0.0.0.0", 9090}, 128);
  vix::console.info("[async] echo server listening on 0.0.0.0:9090");

  while (ctx.is_running())
  {
    try
    {
      auto client = co_await listener->async_accept();
      vix::async::core::spawn_detached(ctx, handle_client(std::move(client)));
    }
    catch (const std::system_error &e)
    {
      vix::console.error("[async] accept error:", e.code().message());
      break;
    }
  }

  listener->close();
  ctx.stop();
  co_return;
}

int main()
{
  io_context ctx;

  auto t = server(ctx);
  ctx.post(t.handle());

  ctx.run();
  vix::console.info("[async] server stopped");
  return 0;
}
