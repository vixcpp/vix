# thread_pool (async/core)

`vix::async::core::thread_pool` is a small CPU thread pool designed to work with `io_context`.
It runs submitted jobs on worker threads and resumes awaiting coroutines back onto the `io_context` scheduler thread.

This gives you a clean split:

- Worker threads: do compute work (hashing, parsing, compression, DB client CPU work, etc.)
- Scheduler thread: resumes coroutines and continues async flows

## Header

```cpp
#include <vix/async/core/thread_pool.hpp>
```

## Concepts

### Two submission styles

1) Fire-and-forget

- `submit(std::function<void()>)`
- No result
- Useful for background work where you do not need to await completion

2) Coroutine-friendly submission

- `submit(Fn, cancel_token) -> task<R>`
- Lets you `co_await` the result
- Captures exceptions from the worker and rethrows them when resuming

### Cancellation model

The coroutine-friendly overload accepts a `cancel_token`.

- If cancellation is already requested when the worker starts executing the job, the job fails with a cancellation error.
- That cancellation error is `std::system_error(cancelled_ec())` where `cancelled_ec()` is the async cancellation error code.

Important: cancellation here is cooperative and checked only at job start.
If your job is long-running, you should also check `ct.is_cancelled()` inside the job body and return early.

### Exception model

- Any exception thrown by the job on a worker thread is captured.
- When the awaiting coroutine resumes, `await_resume()` rethrows that exception.

That means your coroutine can use normal `try/catch` around `co_await pool.submit(...)`.

## API

### Construction

```cpp
explicit thread_pool(io_context& ctx,
                     std::size_t threads = std::thread::hardware_concurrency());
```

- `ctx` is used to post coroutine continuations back to the scheduler.
- `threads` controls the number of worker threads.

### Destruction

```cpp
~thread_pool();
```

- Requests stop and joins all worker threads.

### Fire-and-forget

```cpp
void submit(std::function<void()> fn);
```

### Awaitable submission

```cpp
template <typename Fn>
auto submit(Fn&& fn, cancel_token ct = {})
  -> task<std::invoke_result_t<Fn>>;
```

- `Fn` runs on a worker thread.
- The awaiting coroutine resumes on the `io_context` scheduler thread.

### Stop

```cpp
void stop() noexcept;
```

Requests workers to exit.

### Size

```cpp
std::size_t size() const noexcept;
```

Returns number of worker threads.

## Usage

### Example 1: Await a compute result

```cpp
#include <iostream>
#include <string>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/thread_pool.hpp>
#include <vix/async/core/task.hpp>

using namespace vix::async::core;

task<void> app(io_context& ctx)
{
  // Lazily creates the pool in io_context in your codebase,
  // but you can also create it directly.
  auto& pool = ctx.cpu_pool();

  std::string input = "hello";

  // Run on worker threads
  auto out = co_await pool.submit([s = input]() {
    // Heavy compute goes here
    std::string r = s;
    for (auto& c : r) c = static_cast<char>(c - 32);
    return r;
  });

  // Resumed on scheduler thread here
  std::cout << out << "\n";
  co_return;
}

int main()
{
  io_context ctx;

  // Run the coroutine on the scheduler thread
  auto t = app(ctx);
  std::move(t).start(ctx.get_scheduler());

  ctx.run();
}
```

### Example 2: Cancellation

```cpp
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/thread_pool.hpp>
#include <vix/async/core/cancel.hpp>

using namespace vix::async::core;

task<void> work(io_context& ctx)
{
  auto& pool = ctx.cpu_pool();

  cancel_source cs;
  auto ct = cs.token();

  // Request cancellation before scheduling
  cs.request_cancel();

  try
  {
    auto v = co_await pool.submit([=]() -> int {
      // This will not run if cancellation is observed at start
      return 42;
    }, ct);

    (void)v;
  }
  catch (const std::system_error& e)
  {
    // e.code() should equal cancelled_ec()
  }

  co_return;
}
```

If you want deeper cooperative cancellation for long jobs, check the token inside the job:

```cpp
auto v = co_await pool.submit([ct]() -> int {
  for (int i = 0; i < 10'000'000; ++i)
  {
    if (ct.is_cancelled())
      throw std::system_error(cancelled_ec());
    // do work...
  }
  return 1;
}, ct);
```

### Example 3: Fire-and-forget

```cpp
auto& pool = ctx.cpu_pool();

pool.submit([] {
  // runs on a worker thread
  // no awaiting, no result
});
```

## Notes

- `thread_pool` is intended for CPU-bound work.
  For I/O-bound work, prefer your async networking and timers services.
- Awaited jobs always resume on the `io_context` scheduler thread.
  This keeps coroutine code deterministic and reduces locking in user code.
- The pool uses a single mutex and a FIFO job queue.
  This keeps behavior explicit and simple.

## Related

- `cancel_source`, `cancel_token` in `vix::async::core::cancel`
- `scheduler` in `vix::async::core::scheduler`
- `task<T>` in `vix::async::core::task`
- `io_context` in `vix::async::core::io_context`

