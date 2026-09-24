# Parallel Map

`parallel_map` transforms an input range in parallel and returns a new `std::vector` containing the results.

It is useful when you have code like this:

```cpp
std::vector<int> output;

for (int value : input)
{
  output.push_back(transform(value));
}
```

and you want Vix to split the transformation across worker threads while preserving output order.

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

  std::vector<int> squares =
      vix::threadpool::parallel_map(
          pool,
          values,
          [](int value) { return value * value; });

  for (const int value : squares) { std::cout << value << ' '; }
  std::cout << '\n'; // 1 4 9 16

  pool.shutdown();

  return 0;
}
```

## What `parallel_map` does

1. Receives an input range
2. Splits it into chunks
3. Submits chunk tasks to the pool
4. Applies the mapping function to every element
5. Writes results into an output vector
6. Preserves input order
7. Waits for all chunks
8. Returns the output vector

## Output order

`parallel_map` always preserves the order of the input, even if chunks run on different workers:

```cpp
std::vector<int> values{10, 20, 30};

std::vector<int> result =
    vix::threadpool::parallel_map(
        pool, values, [](int value) { return value + 1; });

// result: 11 21 31  (not 31 11 21)
```

## Mapping to another type

The output type can differ from the input type:

```cpp
std::vector<int> values{1, 2, 3};

std::vector<std::string> result =
    vix::threadpool::parallel_map(
        pool, values,
        [](int value) { return std::string{"value-"} + std::to_string(value); });

// result: "value-1", "value-2", "value-3"
```

## Function shapes

Container overload (simplest):

```cpp
auto result = vix::threadpool::parallel_map(pool, container, fn);
```

Iterator overload:

```cpp
auto result = vix::threadpool::parallel_map(pool, first, last, fn);
```

## Empty input

If the input range is empty, `parallel_map` returns an empty vector.

## How work is split

`parallel_map` splits the input into chunks. Each chunk maps its own range and writes to the correct output positions:

```txt
input: 1000 elements, chunk size: 250

chunks:    0..250   → output[0..250]
          250..500  → output[250..500]
          500..750  → output[500..750]
          750..1000 → output[750..1000]
```

## Chunk size

### Default

If no chunk size is provided, Vix computes one automatically based on input size, worker count, and target chunk count.

### Custom

```cpp
vix::threadpool::ParallelMapOptions options;
options.chunk_size = 128;

// Or:
auto options = vix::threadpool::ParallelMapOptions::with_chunk_size(128);
```

### Choosing chunk size

| Transform cost | Recommended chunk size |
|---|---|
| Cheap | 1024 – 4096 |
| Medium | 128 – 256 |
| Expensive | 1 – 64 |

## Task options

`ParallelMapOptions` contains `task_options` applied to every chunk task:

```cpp
vix::threadpool::ParallelMapOptions options;

options.chunk_size = 256;
options.task_options.set_priority(vix::threadpool::TaskPriority::high);
options.task_options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto result = vix::threadpool::parallel_map(pool, values, fn, options);
```

Priority affects chunk ordering while queued. Timeouts are observational — Vix does not forcibly kill running C++ code.

## Exception behavior

If one chunk throws, `parallel_map` still waits for all submitted chunks to finish, then rethrows the first captured exception:

```cpp
try
{
  auto result =
      vix::threadpool::parallel_map(
          pool, values,
          [](int value)
          {
            if (value == 3) throw std::runtime_error{"map failed"};
            return value * 2;
          });
}
catch (const std::exception &e)
{
  std::cout << "parallel_map failed: " << e.what() << '\n';
}
```

## Thread safety

The mapping function runs concurrently. This is safe when the function only reads its input and returns a new value:

```cpp
// Good:
auto output = vix::threadpool::parallel_map(pool, values,
    [](int value) { return value * value; });

// Bad (data race):
std::vector<int> shared;
auto output = vix::threadpool::parallel_map(pool, values,
    [&shared](int value) { shared.push_back(value); return value; });

// If shared state is needed, protect it:
std::mutex mutex;
auto output = vix::threadpool::parallel_map(pool, values,
    [&mutex, &shared](int value)
    {
      { std::lock_guard<std::mutex> lock(mutex); shared.push_back(value); }
      return value * 2;
    });
```

Pure mapping functions are easiest to reason about.

## Mapping objects and strings

```cpp
struct Product { int id; double price; };
std::vector<Product> products{{1, 10.0}, {2, 20.0}, {3, 30.0}};

std::vector<double> prices =
    vix::threadpool::parallel_map(pool, products,
        [](const Product &p) { return p.price; });
```

```cpp
std::vector<std::string> names{"vix", "threadpool", "runtime"};

std::vector<std::size_t> lengths =
    vix::threadpool::parallel_map(pool, names,
        [](const std::string &name) { return name.size(); });
```

## Temporary pool overload

Convenient for examples, but prefer reusing a pool in production:

```cpp
// Convenient but creates a new pool each time:
auto result = vix::threadpool::parallel_map(values, fn);

// Better:
vix::threadpool::ThreadPool pool(4);

auto a = vix::threadpool::parallel_map(pool, valuesA, fnA);
auto b = vix::threadpool::parallel_map(pool, valuesB, fnB);

pool.shutdown();
```

## Using the parallel namespace

```cpp
auto result = vix::threadpool::parallel::map(pool, values, fn);
```

## Complete example with options

```cpp
#include <iostream>
#include <string>
#include <vector>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  std::vector<int> values{1, 2, 3, 4, 5};

  vix::threadpool::ParallelMapOptions options;
  options.chunk_size = 2;
  options.task_options.set_priority(vix::threadpool::TaskPriority::high);

  std::vector<std::string> output =
      vix::threadpool::parallel_map(
          pool, values,
          [](int value) { return std::string{"value-"} + std::to_string(value); },
          options);

  for (const auto &item : output) { std::cout << item << '\n'; }

  pool.shutdown();

  return 0;
}
```

Output:

```txt
value-1
value-2
value-3
value-4
value-5
```

## Metrics after `parallel_map`

Each chunk is submitted as one task. Metrics count chunks, not input elements:

```cpp
// 1000 elements, chunk size 100 → ~10 chunk tasks
const auto metrics = pool.metrics();
std::cout << metrics.completed_tasks << '\n';
```

## When to use `parallel_map`

Use when: you have input values, each value can be transformed independently, you want a new output vector, you want output order to match input order, and the transform is heavy enough to benefit from parallelism.

Good examples: compute image filters, parse many records, extract fields from objects, transform numeric buffers, convert many strings, validate inputs into result objects.

## When not to use `parallel_map`

Avoid when: the mapping function mutates shared state without synchronization, the transform is too cheap and overhead dominates, the output type is not default-constructible, you need streaming output, or you need ordered side effects.

**Note:** `parallel_map` creates `std::vector<Result>(total)`, so `Result` must be default-constructible.

For index-based output writing, use `parallel_for`. For aggregation, use `parallel_reduce`.

## Common mistakes

### Modifying input unsafely

```cpp
// Bad:
auto output = vix::threadpool::parallel_map(pool, values,
    [&values](int value) { values.push_back(value); return value; });

// Good — keep the mapper pure:
auto output = vix::threadpool::parallel_map(pool, values,
    [](int value) { return value; });
```

### Relying on side-effect order

The **output vector** order is guaranteed. **Side-effect order** is not:

```cpp
// Bad — console output order is not guaranteed:
vix::threadpool::parallel_map(pool, values,
    [](int value) { std::cout << value << '\n'; return value; });

// Good — print after the map:
auto output = vix::threadpool::parallel_map(pool, values,
    [](int value) { return value; });

for (const int value : output) { std::cout << value << '\n'; }
```

## Simple mental model

```txt
parallel_map(pool, input, fn)

  splits input into chunks
  allocates output vector
  submits chunk tasks
  each chunk maps input[i] to output[i]
  waits for futures
  rethrows first exception if any
  returns output (in input order)
```

The user writes:

```cpp
auto result = parallel_map(pool, values, fn);
```

Vix handles chunking, task submission, worker execution, waiting, exception propagation, and ordered output storage.
