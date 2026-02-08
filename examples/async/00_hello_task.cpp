#include <cassert>
#include <chrono>
#include <vix/console.hpp>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/timer.hpp>
#include <vix/async/core/thread_pool.hpp>
using namespace vix;
using vix::async::core::io_context;
using vix::async::core::task;

static task<void> app(io_context &ctx)
{
  console.info("[async] hello from task");
  // Timer: sleep for 50ms (does not block the event loop thread)
  co_await ctx.timers().sleep_for(std::chrono::milliseconds(50));
  console.info("[async] after timer");

  // Thread pool: run CPU work off the event loop, then resume here
  int v = co_await ctx.cpu_pool().submit([]() -> int
                                         {
    // pretend CPU work
    int sum = 0;
    for (int i = 0; i < 100000; ++i)
      sum += (i % 7);
    return sum; });

  console.info("[async] cpu_pool result =", v);
  assert(v >= 0);
  // Stop the runtime once done
  ctx.stop();
  co_return;
}

int main()
{
  io_context ctx;
  auto t = app(ctx);
  ctx.post(t.handle());
  ctx.run();

  vix::console.info("[async] done");
  return 0;
}
