# Parallel For

`parallel_for` runs a numeric index range in parallel using `vix::threadpool`.

It is the simplest helper when you have a loop like this:

```cpp
for (std::size_t i = 0; i < values.size(); ++i)
{
  values[i] = compute(i);
}
```

and you want Vix to split the work across worker threads.

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Basic usage

```cpp
#include <cstddef>
#include <iostream>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(10, 0);

  vix::threadpool::parallel_for(
      pool,
      std::size_t{0},
      values.size(),
      [&values](std::size_t index)
      {
        values[index] = static_cast<int>(index * index);
      });

  for (const int value : values) { std::cout << value << ' '; }
  std::cout << '\n';

  pool.shutdown();

  return 0;
}
```

## Function shape

The main overload is:

```cpp
vix::threadpool::parallel_for(pool, first, last, fn);
```

The range is half-open `[first, last)`:

```cpp
vix::threadpool::parallel_for(pool, 0, 10, fn);
// runs fn(0), fn(1), ..., fn(9) — not fn(10)
```

The callable receives one index. The index type must be an integral type:

```cpp
// int range:
vix::threadpool::parallel_for(pool, 0, 100, [](int index) { /* use index */ });

// std::size_t range:
vix::threadpool::parallel_for(pool, std::size_t{0}, values.size(),
    [](std::size_t index) { /* use index */ });
```

## Empty and reversed ranges

If `first == last` or `last <= first`, nothing runs. This makes it safe to call with computed bounds.

## How work is split

`parallel_for` splits the index range into chunks. Each chunk runs a normal internal loop:

```cpp
for (Index i = chunkFirst; i < chunkLast; ++i) { fn(i); }
```

This avoids submitting one task per element.

```txt
range: 0..100, chunk size: 25

chunks:
  0..25
 25..50
 50..75
 75..100
```

## Chunk size

### Default

If no chunk size is provided, Vix computes one automatically based on worker count, total item count, and target chunk count.

### Custom

```cpp
vix::threadpool::ParallelForOptions options;
options.chunk_size = 128;

// Or:
auto options = vix::threadpool::ParallelForOptions::with_chunk_size(128);
```

### Choosing chunk size

| Work per item | Recommended chunk size |
|---|---|
| Small (cheap) | 1024 – 4096 |
| Medium | 128 – 256 |
| Heavy | 1 – 64 |

Small chunks → better load balancing, more overhead. Large chunks → less overhead, less load balancing.

## Task options

`ParallelForOptions` contains `task_options` applied to every chunk task:

```cpp
vix::threadpool::ParallelForOptions options;

options.chunk_size = 256;
options.task_options.set_priority(vix::threadpool::TaskPriority::high);
options.task_options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

vix::threadpool::parallel_for(pool, std::size_t{0}, values.size(), fn, options);
```

Priority affects chunk ordering while queued. Timeouts are observational — Vix does not forcibly kill running C++ code.

## Exception behavior

If one chunk throws, `parallel_for` still waits for all submitted chunks to finish, then rethrows the first captured exception:

```cpp
try
{
  vix::threadpool::parallel_for(
      pool, 0, 100,
      [](int index)
      {
        if (index == 42) throw std::runtime_error{"failed"};
      });
}
catch (const std::exception &e)
{
  std::cout << "parallel_for failed: " << e.what() << '\n';
}
```

## Thread safety

`parallel_for` runs multiple indices concurrently. This is safe when each index writes to a separate location:

```cpp
// Safe — each task writes to a different element:
std::vector<int> values(100);
vix::threadpool::parallel_for(pool, std::size_t{0}, values.size(),
    [&values](std::size_t index) { values[index] = static_cast<int>(index); });

// Unsafe (data race):
int sum = 0;
vix::threadpool::parallel_for(pool, 0, 100, [&sum](int index) { sum += index; });
```

Use `parallel_reduce` for reductions. Use `std::atomic` for shared counters. Use a mutex only when necessary.

## Complete example with options

```cpp
#include <cstddef>
#include <iostream>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(1000, 0);

  vix::threadpool::ParallelForOptions options;
  options.chunk_size = 128;
  options.task_options.set_priority(vix::threadpool::TaskPriority::high);

  vix::threadpool::parallel_for(
      pool,
      std::size_t{0},
      values.size(),
      [&values](std::size_t index)
      {
        values[index] = static_cast<int>(index * 2);
      },
      options);

  std::cout << "first: " << values.front() << '\n'; // 0
  std::cout << "last: " << values.back() << '\n';   // 1998

  pool.shutdown();

  return 0;
}
```

## Temporary pool overload

Convenient for examples, but prefer reusing a pool in production:

```cpp
// Creates a temporary pool each time (avoid in hot paths):
vix::threadpool::parallel_for(0, 100, [](int index) { /* Work */ });

// Better:
vix::threadpool::ThreadPool pool(4);

vix::threadpool::parallel_for(pool, 0, 100, fn);
vix::threadpool::parallel_for(pool, 0, 200, fn);

pool.shutdown();
```

## Using the parallel namespace

```cpp
vix::threadpool::parallel::for_range(pool, 0, 100, fn);
```

## Metrics after `parallel_for`

Each chunk is submitted as a task. Metrics count chunks, not individual indices:

```cpp
// 1000 items, chunk size 100 → ~10 chunk tasks
const auto metrics = pool.metrics();
std::cout << metrics.completed_tasks << '\n';
```

## When to use `parallel_for`

Use when: you have numeric indices, each iteration is independent, each iteration can run safely in parallel, you want to fill or transform data by index.

Good examples: fill a vector, compute image pixels, process array ranges, run independent simulations, transform numeric buffers, validate many records by index.

## When not to use `parallel_for`

Avoid when: iterations depend on previous iterations, the loop updates shared state without synchronization, work per item is too tiny, the loop performs blocking I/O per item without limits, or you need ordered side effects.

For reductions use `parallel_reduce`. For a new output vector use `parallel_map`. For container iteration use `parallel_for_each`.

## Common mistakes

### Shared accumulator (data race)

```cpp
// Bad:
int sum = 0;
vix::threadpool::parallel_for(pool, 0, 1000, [&sum](int index) { sum += index; });

// Good:
std::vector<int> values(1000);
std::iota(values.begin(), values.end(), 0);

int sum = vix::threadpool::parallel_reduce(pool, values, 0,
    [](int current, int value) { return current + value; });
```

### Creating too many pools

```cpp
// Bad — creates a new pool every iteration:
for (int round = 0; round < 100; ++round)
{
  vix::threadpool::parallel_for(0, 1000, fn); // temporary pool each time
}

// Good:
vix::threadpool::ThreadPool pool(4);

for (int round = 0; round < 100; ++round)
{
  vix::threadpool::parallel_for(pool, 0, 1000, fn);
}

pool.shutdown();
```

## Best practices

- Use `parallel_for` for independent index-based work
- Prefer one reusable pool
- Choose chunk size based on work cost
- Use atomics or mutexes for shared state
- Prefer indexed writes over shared `push_back`
- Use `parallel_reduce` for sums and reductions
- Use `parallel_map` for transformations that return a new vector
- Inspect metrics when tuning performance

## Simple mental model

```txt
parallel_for(pool, first, last, fn)

  splits [first, last) into chunks
  submits chunk tasks
  each chunk runs a for loop over its range
  waits for all futures
  rethrows first exception if any
```

The user writes:

```cpp
parallel_for(pool, 0, n, fn);
```

Vix handles chunking, task submission, waiting, exception propagation, and worker execution.
