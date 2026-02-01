#include <iostream>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/signal.hpp>
#include <vix/async/core/task.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

static task<void> app(io_context &ctx)
{
  auto &sig = ctx.signals();

  // Register signals to handle
  sig.add(SIGINT);
  sig.add(SIGTERM);

  std::cout << "[async] waiting for SIGINT/SIGTERM (Ctrl+C)\n";

  sig.on_signal([&](int s)
                {
    std::cout << "[async] signal received: " << s << " -> stopping\n";
    ctx.stop(); });

  int s = co_await sig.async_wait();
  std::cout << "[async] async_wait got signal: " << s << " -> stopping\n";
  ctx.stop();

  co_return;
}

int main()
{
  io_context ctx;

  auto t = app(ctx);
  ctx.post(t.handle());

  ctx.run();

  std::cout << "[async] stopped\n";
  return 0;
}
