#include <iostream>
#include <vector>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/signal.hpp>
#include <vix/async/core/spawn.hpp>

#include <vix/async/net/tcp.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

static task<void> handle_client(std::unique_ptr<vix::async::net::tcp_stream> client)
{
  std::cout << "[async] client connected\n";

  std::vector<std::byte> buf(4096);

  while (client && client->is_open())
  {
    std::size_t n = 0;

    try
    {
      n = co_await client->async_read(
          std::span<std::byte>(buf.data(), buf.size()));
    }
    catch (const std::system_error &e)
    {
      std::cout << "[async] read error: " << e.code().message() << "\n";
      break;
    }

    if (n == 0)
      break;

    try
    {
      co_await client->async_write(
          std::span<const std::byte>(buf.data(), n));
    }
    catch (const std::system_error &e)
    {
      std::cout << "[async] write error: " << e.code().message() << "\n";
      break;
    }
  }

  client->close();
  std::cout << "[async] client disconnected\n";
  co_return;
}

static task<void> server(io_context &ctx)
{
  auto &sig = ctx.signals();
  sig.add(SIGINT);
  sig.add(SIGTERM);
  sig.on_signal([&](int s)
                {
    std::cout << "[async] signal " << s << " received -> stopping\n";
    ctx.stop(); });

  auto listener = vix::async::net::make_tcp_listener(ctx);

  co_await listener->async_listen({"0.0.0.0", 9090}, 128);
  std::cout << "[async] echo server listening on 0.0.0.0:9090\n";

  while (ctx.is_running())
  {
    try
    {
      auto client = co_await listener->async_accept();
      vix::async::core::spawn_detached(
          ctx,
          handle_client(std::move(client)));
    }
    catch (const std::system_error &e)
    {
      std::cout << "[async] accept error: " << e.code().message() << "\n";
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
  std::cout << "[async] server stopped\n";
  return 0;
}
