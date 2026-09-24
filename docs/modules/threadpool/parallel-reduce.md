# Parallel Reduce

`parallel_reduce` reduces an input range in parallel and returns one final value.

It is useful when you have code like this:

```cpp
int sum = 0;

for (int value : values)
{
  sum += value;
}
```

and you want Vix to split the work across worker threads safely.

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Basic usage

```cpp
#include <iostream>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4};

  const int sum =
      vix::threadpool::parallel_reduce(
          pool,
          values,
          0,
          [](int current, int value)
          {
            return current + value;
          });

  std::cout << "sum: " << sum << '\n'; // 10

  pool.shutdown();

  return 0;
}
```

## What `parallel_reduce` does

1. Receives an input range
2. Splits it into chunks
3. Submits chunk tasks to the pool
4. Reduces each chunk locally
5. Waits for all chunk tasks
6. Combines partial results on the caller thread
7. Returns the final value

## Function shapes

Container overload (simplest):

```cpp
vix::threadpool::parallel_reduce(pool, container, initial, reduce);
```

Iterator overload:

```cpp
vix::threadpool::parallel_reduce(pool, first, last, initial, reduce);
```

## Reducer signature

The reducer receives the current accumulator and the current value, and returns the next accumulator:

```cpp
[](int current, int value) { return current + value; }

// For strings:
[](std::string current, const std::string &value) { return current + value; }

// For custom objects:
[](Stats current, const Record &record)
{
  current.count += 1;
  current.total += record.value;
  return current;
}
```

## Initial value

The initial value is used as the starting value for each chunk and when combining partial results:

```cpp
// Sum:
vix::threadpool::parallel_reduce(pool, values, 0, reduce);

// Product:
const int product =
    vix::threadpool::parallel_reduce(
        pool, values, 1,
        [](int current, int value) { return current * value; });
```

## Empty input

If the input range is empty, `parallel_reduce` returns the initial value:

```cpp
std::vector<int> values;

const int result = vix::threadpool::parallel_reduce(pool, values, 42, reduce);
result; // 42
```

## Iterator range overload

Useful when you only want to reduce part of a container:

```cpp
const int partial =
    vix::threadpool::parallel_reduce(
        pool,
        values.begin() + 2,
        values.end(),
        0,
        [](int current, int value) { return current + value; });
```

## How work is split

`parallel_reduce` splits the input into chunks. Each chunk becomes one submitted task, computes a partial result, and the caller combines partial results at the end:

```txt
input: 1000 elements, chunk size: 250

chunks:    0..250  →  partial 1
          250..500  →  partial 2
          500..750  →  partial 3
          750..1000 →  partial 4
                     ↓
                  final result
```

## Chunk size

### Default

If no chunk size is provided, Vix computes one automatically based on input size, worker count, and target chunk count.

### Custom

```cpp
vix::threadpool::ParallelReduceOptions options;
options.chunk_size = 128;

// Or:
auto options = vix::threadpool::ParallelReduceOptions::with_chunk_size(128);
```

### Choosing chunk size

| Work per element | Recommended chunk size |
|---|---|
| Cheap | 1024 – 4096 |
| Medium | 128 – 256 |
| Expensive | 1 – 64 |

Small chunks → better load balancing, more overhead. Large chunks → less overhead, less load balancing.

## Task options

`ParallelReduceOptions` contains `task_options` applied to every chunk task:

```cpp
vix::threadpool::ParallelReduceOptions options;

options.chunk_size = 256;
options.task_options.set_priority(vix::threadpool::TaskPriority::high);
options.task_options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

const int result =
    vix::threadpool::parallel_reduce(pool, values, 0, reduce, options);
```

Priority affects chunk ordering while queued. Timeouts are observational.

## Exception behavior

If one chunk throws, `parallel_reduce` still waits for all submitted chunks to finish, then rethrows the first captured exception:

```cpp
try
{
  const int result =
      vix::threadpool::parallel_reduce(
          pool, values, 0,
          [](int current, int value)
          {
            if (value == 4) throw std::runtime_error{"reduce failed"};
            return current + value;
          });
}
catch (const std::exception &e)
{
  std::cout << "parallel_reduce failed: " << e.what() << '\n';
}
```

## Associativity matters

For best results, the reducer should be associative. Parallel reduction may group values differently from a sequential loop.

Good (associative): `+`, `*`, `min`, `max`.

Less ideal (non-associative): `-`, `/`.

### Floating-point note

Floating-point addition is not perfectly associative due to rounding. `parallel_reduce` with `double` may not produce the exact same bit-level result as a sequential loop. This is normal.

## Custom accumulator

```cpp
struct Stats { int count{0}; int total{0}; };

Stats stats =
    vix::threadpool::parallel_reduce(
        pool, values, Stats{},
        [](Stats current, int value)
        {
          current.count += 1;
          current.total += value;
          return current;
        });
```

## Thread safety

The reducer runs concurrently in chunk tasks. The safest reducer does not mutate shared state:

```cpp
// Good:
[](int current, int value) { return current + value; }

// Bad (data race):
int shared = 0;
[&shared](int current, int value) { shared += value; return current + value; }
```

## Temporary pool overload

Convenient for examples, but prefer reusing a pool in production:

```cpp
// Convenient but creates a new pool each time:
const int sum = vix::threadpool::parallel_reduce(values, 0, reduce);

// Better — reuse the pool:
vix::threadpool::ThreadPool pool(4);

auto a = vix::threadpool::parallel_reduce(pool, valuesA, 0, reduce);
auto b = vix::threadpool::parallel_reduce(pool, valuesB, 0, reduce);

pool.shutdown();
```

## Using the parallel namespace

```cpp
const int sum =
    vix::threadpool::parallel::reduce(pool, values, 0, reduce);
```

## Complete example with options

```cpp
#include <iostream>
#include <numeric>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values(100);
  std::iota(values.begin(), values.end(), 1);

  vix::threadpool::ParallelReduceOptions options;
  options.chunk_size = 10;
  options.task_options.set_priority(vix::threadpool::TaskPriority::high);

  const int sum =
      vix::threadpool::parallel_reduce(
          pool, values, 0,
          [](int current, int value) { return current + value; },
          options);

  std::cout << "sum: " << sum << '\n'; // 5050

  pool.shutdown();

  return 0;
}
```

## Metrics after `parallel_reduce`

Each chunk is submitted as one task. Metrics count chunks, not input elements:

```cpp
// 1000 elements, chunk size 100 → ~10 chunk tasks
const auto metrics = pool.metrics();
std::cout << metrics.completed_tasks << '\n';
```

## When to use `parallel_reduce`

Use when: you need one final result, each element can be processed independently, the reducer is associative, you want to avoid shared mutable state, and the input is large enough to benefit from parallelism.

Good examples: sum values, count records, compute min/max, aggregate statistics, combine validation results, compute checksums.

## When not to use `parallel_reduce`

Avoid when: the operation depends on strict left-to-right order, the reducer mutates shared state, the operation is not associative, the input is too small, or work per element is too cheap.

For transformations use `parallel_map`. For index-based writes use `parallel_for`. For container mutation by reference use `parallel_for_each`.

## Common mistakes

### Shared accumulator (data race)

```cpp
// Bad:
int sum = 0;
vix::threadpool::parallel_for_each(pool, values, [&sum](int value) { sum += value; });

// Good:
int sum = vix::threadpool::parallel_reduce(pool, values, 0,
    [](int current, int value) { return current + value; });
```

### Non-associative reducer

```cpp
// Unpredictable parallel result:
vix::threadpool::parallel_reduce(pool, values, 0,
    [](int current, int value) { return current - value; });

// Predictable:
vix::threadpool::parallel_reduce(pool, values, 0,
    [](int current, int value) { return current + value; });
```

## Simple mental model

```txt
parallel_reduce(pool, input, initial, reduce)

  splits input into chunks
  submits chunk tasks
  each chunk reduces locally
  waits for futures
  combines partial results
  rethrows first exception if any
  returns final value
```

The user writes:

```cpp
auto result = parallel_reduce(pool, values, initial, reduce);
```

Vix handles chunking, task submission, worker execution, waiting, exception propagation, partial result collection, and final combination.
