#include <chrono>

#include <vix/console.hpp>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/timer.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

static task<void> app(io_context &ctx)
{
  vix::console.info("[async] timer demo start");

  co_await ctx.timers().sleep_for(std::chrono::milliseconds(100));
  vix::console.info("[async] +100ms");

  co_await ctx.timers().sleep_for(std::chrono::milliseconds(200));
  vix::console.info("[async] +200ms");

  ctx.timers().after(std::chrono::milliseconds(150), []()
                     { vix::console.info("[async] after(150ms) callback"); });

  co_await ctx.timers().sleep_for(std::chrono::milliseconds(250));
  vix::console.info("[async] timer demo done");

  ctx.stop();
  co_return;
}

int main()
{
  io_context ctx;

  auto t = app(ctx);
  ctx.post(t.handle());

  ctx.run();
  return 0;
}
