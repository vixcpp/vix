#include <csignal>

#include <vix/console.hpp>

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

  vix::console.info("[async] waiting for SIGINT/SIGTERM (Ctrl+C)");

  sig.on_signal([&](int s)
                {
    vix::console.warn("[async] signal received:", s, "-> stopping");
    ctx.stop(); });

  int s = co_await sig.async_wait();
  vix::console.warn("[async] async_wait got signal:", s, "-> stopping");
  ctx.stop();

  co_return;
}

int main()
{
  io_context ctx;

  auto t = app(ctx);
  ctx.post(t.handle());

  ctx.run();

  vix::console.info("[async] stopped");
  return 0;
}
