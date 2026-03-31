# Async Worker (Beginner Guide)

Welcome 👋

This page explains how to use Vix async in a simple and practical way.
If you are new to coroutines or async C++, this guide is for you.

------------------------------------------------------------------------

## What is an Async Worker?

An async worker allows your program to:

-   Run tasks without blocking the main thread
-   Wait for timers without freezing
-   Run heavy CPU work in background threads
-   Stop cleanly when receiving Ctrl+C

In simple words:

Async = Do not block.\
Worker = Do work safely in background.

------------------------------------------------------------------------

# 1️⃣ Minimal Example -- Hello Async

This is the smallest possible example.

``` cpp
#include <iostream>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

task<void> app(io_context& ctx)
{
    std::cout << "Hello async world!\n";
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
```

What happens here?

-   io_context = async runtime
-   task\<\> = coroutine function
-   ctx.post() = schedule the task
-   ctx.run() = start the event loop
-   ctx.stop() = stop the runtime

------------------------------------------------------------------------

# 2️⃣ Waiting Without Blocking (Timer)

This example waits 1 second without freezing the program.

``` cpp
#include <iostream>
#include <chrono>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/timer.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

task<void> app(io_context& ctx)
{
    std::cout << "Waiting 1 second...\n";

    co_await ctx.timers().sleep_for(std::chrono::seconds(1));

    std::cout << "Done!\n";
    ctx.stop();
    co_return;
}

int main()
{
    io_context ctx;
    auto t = app(ctx);
    ctx.post(t.handle());
    ctx.run();
}
```

Important:

sleep_for() does NOT block the event loop thread.

------------------------------------------------------------------------

# 3️⃣ Running Heavy Work in Background (CPU Pool)

Never block your event loop with heavy computation.

Instead:

``` cpp
#include <iostream>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include <vix/async/core/thread_pool.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

task<void> app(io_context& ctx)
{
    int result = co_await ctx.cpu_pool().submit([] {
        int sum = 0;
        for (int i = 0; i < 100000; ++i)
            sum += i;
        return sum;
    });

    std::cout << "Result: " << result << "\n";

    ctx.stop();
    co_return;
}

int main()
{
    io_context ctx;
    auto t = app(ctx);
    ctx.post(t.handle());
    ctx.run();
}
```

Here:

-   The heavy loop runs on a worker thread
-   The event loop stays responsive

------------------------------------------------------------------------

# 4️⃣ Clean Shutdown (Ctrl+C)

Production programs must stop safely.

``` cpp
#include <iostream>
#include <signal.h>

#include <vix/async/core/io_context.hpp>
#include <vix/async/core/signal.hpp>
#include <vix/async/core/task.hpp>

using vix::async::core::io_context;
using vix::async::core::task;

task<void> app(io_context& ctx)
{
    auto& sig = ctx.signals();

    sig.add(SIGINT);
    sig.add(SIGTERM);

    std::cout << "Press Ctrl+C to stop\n";

    sig.on_signal([&](int){
        std::cout << "Stopping...\n";
        ctx.stop();
    });

    co_await sig.async_wait();
    co_return;
}

int main()
{
    io_context ctx;
    auto t = app(ctx);
    ctx.post(t.handle());
    ctx.run();
}
```

------------------------------------------------------------------------

# Final Notes for Beginners

✔ Always use cpu_pool() for heavy work\
✔ Use timers instead of std::this_thread::sleep_for()\
✔ Handle SIGINT for production apps\
✔ Keep event loop clean and responsive

------------------------------------------------------------------------

Generated on 2026-02-17\
Vix Async Beginner Guide

