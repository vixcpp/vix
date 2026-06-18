# Best Practices

This page gives practical rules for using `vix::threadpool` safely and effectively.

The goal is simple:

```txt
make multithreaded C++ easier
without hiding correctness problems
```

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Use one reusable pool

Create a pool once and reuse it. Creating and destroying threads repeatedly adds overhead.

```cpp
// Good:
vix::threadpool::ThreadPool pool(4);

pool.post(work_a);
pool.post(work_b);
pool.post(work_c);

pool.wait_idle();
pool.shutdown();

// Bad:
for (int i = 0; i < 100; ++i)
{
  vix::threadpool::ThreadPool pool(4);
  pool.post([]() { do_work(); });
  pool.wait_idle();
  pool.shutdown();
}
```

## Prefer explicit shutdown

`ThreadPool` shuts down in its destructor, but explicit shutdown is clearer in examples, tests, services, and runtime modules:

```cpp
pool.wait_idle();
pool.shutdown();
```

## Use `post` for fire-and-forget work

```cpp
const bool accepted = pool.post([]() { flush_logs(); });

if (!accepted)
{
  // Task was rejected.
}
```

Always check the return value when rejection matters.

## Use `submit` when result or errors matter

```cpp
auto future = pool.submit([]() { return compute_value(); });

try { auto value = future.get(); }
catch (const std::exception &e) { /* Handle task failure. */ }
```

Do not ignore important futures:

```cpp
// Bad:
pool.submit([]() { throw std::runtime_error{"failed"}; });

// Good:
auto future = pool.submit([]() { throw std::runtime_error{"failed"}; });
try { future.get(); }
catch (const std::exception &e) { std::cout << e.what() << '\n'; }
```

## Use `handle` for cancellation control

```cpp
auto handle = pool.handle([]() { return run_job(); });

handle.cancel();

try { auto result = handle.get(); }
catch (const std::exception &e) {}
```

## Remember cancellation is cooperative

Cancellation does not kill running C++ code. For long-running work, pass a token and check it:

```cpp
vix::threadpool::CancellationSource source;
auto token = source.token();

vix::threadpool::TaskOptions options;
options.set_cancellation(token);

auto future =
    pool.submit(
        [token]()
        {
          while (token.can_continue())
          {
            do_one_unit_of_work();
          }

          return -1;
        },
        options);

source.request_cancel();
```

Good long-running tasks should check `if (token.cancelled()) { return; }`.

## Remember timeouts are observational

Timeouts do not interrupt running C++ code. For real early exit, check time inside the task:

```cpp
auto started = std::chrono::steady_clock::now();
auto limit = std::chrono::milliseconds{100};

auto future =
    pool.submit(
        [started, limit]()
        {
          while (true)
          {
            if (std::chrono::steady_clock::now() - started > limit)
            {
              return false;
            }

            do_one_unit_of_work();
          }
        });
```

## Use deadlines for stale work

```cpp
vix::threadpool::TaskOptions options;
options.set_deadline(
    vix::threadpool::Deadline::after(std::chrono::milliseconds{100}));

auto future = pool.submit([]() { return expensive_work(); }, options);
```

If the task waits too long in the queue and the deadline expires, it can be skipped.

## Use priority carefully

Priority affects queued work only — it does not interrupt running tasks.

Good use cases for **high** priority: health checks, control-plane work, short user-facing tasks, shutdown coordination, latency-sensitive callbacks.

Good use cases for **low** priority: metrics flushing, cache cleanup, log compaction, background indexing, telemetry.

Do not make every task high priority. If every task uses `highest`, priority becomes meaningless.

Keep high-priority tasks short. Long CPU-heavy work should use normal priority unless truly urgent.

## Protect shared data

```cpp
// Bad:
std::vector<int> values;
pool.post([&values]() { values.push_back(1); }); // data race

// Good:
std::mutex mutex;
std::vector<int> values;
pool.post([&mutex, &values]()
{
  std::lock_guard<std::mutex> lock(mutex);
  values.push_back(1);
});
```

## Prefer indexed writes over locks

```cpp
// Good — no locking needed:
std::vector<int> output(input.size());

vix::threadpool::parallel_for(pool, std::size_t{0}, input.size(),
    [&input, &output](std::size_t index)
    {
      output[index] = input[index] * input[index];
    });
```

## Use atomics for simple counters

```cpp
std::atomic<int> counter{0};

pool.post([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
```

Use a mutex for complex shared state. Use atomics for simple numeric counters and flags.

## Use the right parallel helper

### `parallel_for` — index-based work

```cpp
vix::threadpool::parallel_for(pool, std::size_t{0}, values.size(),
    [&values](std::size_t index) { values[index] = static_cast<int>(index * 2); });
```

Good for: filling vectors, processing array ranges, computing image pixels, transforming buffers by index.

### `parallel_for_each` — container mutation

```cpp
vix::threadpool::parallel_for_each(pool, values, [](int &value) { value *= 2; });
```

Do not use it for reductions into shared variables (data race).

### `parallel_map` — transformations

```cpp
std::vector<int> squares =
    vix::threadpool::parallel_map(pool, values, [](int value) { return value * value; });
```

Prefer pure mapping functions. Avoid side effects inside the mapper.

### `parallel_reduce` — aggregation

```cpp
// Good:
int sum = vix::threadpool::parallel_reduce(pool, values, 0,
    [](int current, int value) { return current + value; });

// Bad (data race):
int sum = 0;
vix::threadpool::parallel_for_each(pool, values, [&sum](int v) { sum += v; });
```

Reducers should ideally be associative: addition, multiplication, min, max, logical and/or. Be careful with subtraction, division, or order-sensitive operations.

## Choose chunk size based on work cost

| Work per item | Recommended chunk size |
|---|---|
| Cheap | 1024 – 4096 |
| Medium | 128 – 256 |
| Expensive | 1 – 64 |

```cpp
vix::threadpool::ParallelForOptions options;
options.chunk_size = 256;
```

Use benchmarks with realistic data to tune this.

## Avoid too many tiny tasks

```cpp
// Bad — one task per tiny item:
for (std::size_t i = 0; i < values.size(); ++i)
{
  pool.post([i]() { very_tiny_work(i); });
}

// Good — use chunking:
vix::threadpool::parallel_for(pool, std::size_t{0}, values.size(),
    [](std::size_t index) { very_tiny_work(index); },
    vix::threadpool::ParallelForOptions::with_chunk_size(1024));
```

## Use `Scope` for structured concurrency

```cpp
void load_all(vix::threadpool::ThreadPool &pool)
{
  vix::threadpool::Scope scope(pool);

  scope.spawn([]() { load_users(); });
  scope.spawn([]() { load_products(); });

  scope.wait_and_rethrow();
}
```

Scope gives a clear lifecycle: spawn tasks → wait for all → rethrow first exception if needed → leave function safely.

## Use `TaskGroup` for manual coordination

For most application code, prefer `Scope`. For runtime internals and lower-level coordination, `TaskGroup` gives explicit counters and shared cancellation state.

## Stop periodic tasks before shutting down the pool

```cpp
// Good:
periodic.stop();
periodic.join();
pool.wait_idle();
pool.shutdown();

// Bad:
pool.shutdown();
periodic.stop();
periodic.join();
```

## Avoid overlapping periodic callbacks unless intended

```cpp
std::atomic<bool> running{false};

vix::threadpool::PeriodicTask task(pool,
    [&running]()
    {
      bool expected = false;
      if (!running.compare_exchange_strong(expected, true)) { return; }

      // Reset flag when done, even on exception:
      vix::threadpool::detail::ScopeExit reset(
          [&running]() { running.store(false); });

      do_work();
    },
    config);
```

## Use bounded queues for backpressure

```cpp
vix::threadpool::ThreadPoolConfig config;
config.thread_count = 4;
config.max_queue_size = 1024;

vix::threadpool::ThreadPool pool(config);

if (!pool.post(best_effort_work))
{
  // Drop or retry later.
}
```

## Use metrics for observability

```cpp
const auto metrics = pool.metrics();

std::cout << "workers=" << metrics.worker_count
          << " pending=" << metrics.pending_tasks
          << " active=" << metrics.active_tasks
          << " completed=" << metrics.completed_tasks
          << " failed=" << metrics.failed_tasks
          << " rejected=" << metrics.rejected_tasks
          << '\n';
```

Use metrics for debugging, tests, benchmarks, health checks, service logs, and backpressure decisions.

## Use `pending` for backpressure

```cpp
const auto metrics = pool.metrics();

if (metrics.pending_tasks > 1000)
{
  // Skip non-urgent work.
}
else
{
  pool.post(non_urgent_work);
}
```

## Do not treat metrics as synchronization

```cpp
// Bad:
while (pool.metrics().active_tasks > 0) {}

// Good:
pool.wait_idle();
```

Use `wait_idle()` for synchronization. Use `metrics()` for observation.

## Avoid blocking worker threads unnecessarily

A worker thread should spend most of its time doing useful work. Avoid long blocking waits inside tasks. If the work is a timer, prefer a scheduler or `PeriodicTask`. If the work waits on I/O, consider async APIs.

## Avoid deadlocks with nested waits

Be careful when a task submits work to the same pool and waits for it. With one worker, this can deadlock. Avoid waiting inside pool tasks, increase worker count, or use structured task design.

## Do not hold locks while waiting on futures

```cpp
// Bad:
std::lock_guard<std::mutex> lock(mutex);
future.get(); // task may need the same mutex → deadlock

// Good:
auto value = future.get(); // wait outside the lock
std::lock_guard<std::mutex> lock(mutex);
use(value);
```

## Keep task bodies exception-safe

Use RAII for cleanup. If `do_work()` throws, resources are still destroyed correctly:

```cpp
pool.post([]() { Resource resource; do_work(resource); });
```

## Use `InlineExecutor` for deterministic tests

```cpp
vix::threadpool::InlineExecutor executor;

executor.post([]() { /* Runs immediately on caller thread. */ });
```

## Production shutdown pattern

```cpp
stop_accepting_new_work();

periodic.stop();
periodic.join();

pool.wait_idle();

const auto metrics = pool.metrics();
const auto stats = pool.stats();

log(metrics, stats);

pool.shutdown();
```

## Safety checklist

Before submitting concurrent work, ask:

- Does this task access shared mutable data? Is it protected?
- Can this task throw? Should I use `submit()` instead of `post()`?
- Can this task run too long? Should it have a timeout?
- Should it support cancellation?
- Can this work be chunked? Should this be `parallel_for`/`map`/`reduce`?
- Will shutdown wait for this task?

## Common mistakes

### Shared `push_back` without lock

```cpp
// Bad:
std::vector<int> output;
vix::threadpool::parallel_for(pool, 0, 100,
    [&output](int index) { output.push_back(index); }); // data race

// Good:
std::vector<int> output(100);
vix::threadpool::parallel_for(pool, 0, 100,
    [&output](int index) { output[static_cast<std::size_t>(index)] = index; });
```

### Using parallel work for tiny data

For very small input, normal sequential code may be faster. Use parallel algorithms when the input or work cost is large enough.

### Expecting real-time scheduling

`vix::threadpool` is not a real-time scheduler. Priority does not guarantee real-time execution. Timeout does not forcibly stop running code. `PeriodicTask` does not guarantee exact timing. The module is designed for safe, practical concurrency — not hard real-time systems.

### Relying on output order for side effects

Task execution order is not guaranteed. Even with priority, side effects like printing can interleave. If order matters, store results and process them afterward.

## Simple default pattern

For most app code:

```cpp
vix::threadpool::ThreadPool pool(4);

auto future = pool.submit([]() { return do_work(); });
auto value = future.get();

pool.shutdown();
```

For many items:

```cpp
vix::threadpool::parallel_map(pool, values,
    [](const auto &value) { return transform(value); });
```

For service lifecycle:

```cpp
pool.wait_idle();
pool.shutdown();
```

## Final mental model

```txt
Use ThreadPool to own workers.
Use post for background work.
Use submit for results and exceptions.
Use handle for cancellation.
Use Scope for structured concurrency.
Use parallel_for for index ranges.
Use parallel_map for transformations.
Use parallel_reduce for aggregation.
Use metrics and stats for observability.
Use wait_idle and shutdown for lifecycle.
Protect shared data yourself.
```
