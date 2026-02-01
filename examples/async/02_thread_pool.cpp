#include <cassert>
#include <iostream>
#include <vector>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/thread_pool.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

static int heavy_work(int n)
{
  // Fake CPU work
  int acc = 0;
  for (int i = 0; i < 200000; ++i)
    acc += (i * n) % 97;
  return acc;
}

static task<void> app(io_context &ctx)
{
  std::cout << "[async] thread_pool demo start\n";

  // Submit several CPU jobs in sequence (simple demo).
  // Later we can add when_all / gather for parallel awaits.
  int a = co_await ctx.cpu_pool().submit([]
                                         { return heavy_work(1); });
  int b = co_await ctx.cpu_pool().submit([]
                                         { return heavy_work(2); });
  int c = co_await ctx.cpu_pool().submit([]
                                         { return heavy_work(3); });

  std::cout << "[async] results: " << a << ", " << b << ", " << c << "\n";
  assert(a != 0 || b != 0 || c != 0);

  // Fire-and-forget job
  ctx.cpu_pool().submit([]
                        {
    // This runs on a worker thread
    volatile int x = 0;
    for (int i = 0; i < 100000; ++i)
      x += i; });

  std::cout << "[async] demo done\n";
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
