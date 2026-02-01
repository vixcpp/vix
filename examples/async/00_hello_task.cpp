#include <cassert>
#include <iostream>
#include <thread>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/timer.hpp>
#include <vix/async/core/thread_pool.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

static task<void> app(io_context &ctx)
{
  std::cout << "[async] hello from task\n";

  // Timer: sleep for 50ms (does not block the event loop thread)
  co_await ctx.timers().sleep_for(std::chrono::milliseconds(50));
  std::cout << "[async] after timer\n";

  // Thread pool: run CPU work off the event loop, then resume here
  int v = co_await ctx.cpu_pool().submit([]() -> int
                                         {
    // pretend CPU work
    int sum = 0;
    for (int i = 0; i < 100000; ++i)
      sum += (i % 7);
    return sum; });

  std::cout << "[async] cpu_pool result = " << v << "\n";
  assert(v >= 0);

  // Stop the runtime once done
  ctx.stop();
  co_return;
}

int main()
{
  io_context ctx;

  // Kick off the app task by posting its coroutine handle to the scheduler.
  // Our task starts suspended (initial_suspend = suspend_always), so we must resume it.
  auto t = app(ctx);
  ctx.post(t.handle());

  // Run the event loop. It will stop when app() calls ctx.stop().
  ctx.run();

  std::cout << "[async] done\n";
  return 0;
}
