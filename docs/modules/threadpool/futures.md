# Futures

`Future<T>` represents the result of a task submitted to `vix::threadpool`.

Use futures when you need to: wait for a task, retrieve a return value, observe task status, receive exceptions from worker threads, or inspect rejection, cancellation, timeout, or failure state.

The recommended include is:

```cpp
#include <vix/threadpool.hpp>
```

## Basic idea

When you call `submit()`, Vix returns a future:

```cpp
auto future = pool.submit([]() { return 42; });

int value = future.get();
```

## Simple example

```cpp
#include <iostream>
#include <vix/threadpool.hpp>

int main()
{
  vix::threadpool::ThreadPool pool(4);

  auto future = pool.submit([]() { return 42; });

  std::cout << future.get() << '\n'; // 42

  pool.shutdown();

  return 0;
}
```

## Future type

The type depends on the callable return type:

```cpp
auto future = pool.submit([]() { return 42; });
// → Future<int>

auto future = pool.submit([]() { return std::string{"vix"}; });
// → Future<std::string>

auto future = pool.submit([]() { save_file(); });
// → Future<void>
```

## Key methods

### `get()`

Waits until the result is ready, then returns it. Consumes the result — calling `get()` more than once is an error.

```cpp
int value = future.get();
```

### `wait()`

Waits for completion without retrieving the value:

```cpp
future.wait();
int value = future.get(); // retrieve after waiting
```

### `ready()`

Non-blocking check whether the future already has a result:

```cpp
if (future.ready()) { int value = future.get(); }
```

### `valid()`

Checks whether the future is connected to a shared state. A default-constructed future is not valid:

```cpp
vix::threadpool::Future<int> future;
if (!future.valid()) { /* No result state attached. */ }
```

## `Future<void>`

For tasks that do not return a value. `get()` waits and rethrows exceptions, but returns nothing:

```cpp
auto future = pool.submit([]() { write_logs(); });
future.get();
```

## Exceptions

If a submitted task throws, the exception is captured and rethrown by `get()`. This prevents exceptions from escaping worker threads:

```cpp
auto future = pool.submit([]() -> int { throw std::runtime_error{"task failed"}; });

try { int value = future.get(); }
catch (const std::exception &e)
{
  std::cout << "error: " << e.what() << '\n';
}
```

## Status, result, and error

```cpp
auto future = pool.submit([]() { return 42; });
int value = future.get();

std::cout << vix::threadpool::to_string(future.status()) << '\n'; // completed
std::cout << vix::threadpool::to_string(future.result()) << '\n'; // success
std::cout << vix::threadpool::to_string(future.error()) << '\n';
```

Common statuses: `created`, `queued`, `running`, `completed`, `failed`, `cancelled`, `timed_out`, `rejected`.

Common results: `none`, `success`, `failure`, `cancelled`, `timeout`, `rejected`.

## Rejected future

If `submit()` is called after shutdown, the future is completed immediately as rejected:

```cpp
pool.shutdown();

auto future = pool.submit([]() { return 42; });

future.ready();  // true
future.status(); // rejected
future.result(); // rejected
future.error();  // rejected
```

Calling `get()` on a rejected future throws `std::system_error`:

```cpp
try { int value = future.get(); }
catch (const std::system_error &e) { /* Rejected task. */ }
```

## Cancellation and futures

A future can observe cancellation through `status()` and `result()`. For direct cancellation control, use `handle()` instead:

```cpp
auto handle = pool.handle([]() { return 42; });

handle.cancel();
handle.wait();

auto status = handle.status(); // cancelled
auto result = handle.result(); // cancelled
```

Cancellation is cooperative. If cancellation is requested before the task starts, the task can be skipped. Running C++ code is not forcibly interrupted.

## Timeout and futures

Timeouts are observational. After the task completes, inspect the state:

```cpp
vix::threadpool::TaskOptions options;
options.set_timeout(vix::threadpool::Timeout::milliseconds(50));

auto future = pool.submit(
    []() { std::this_thread::sleep_for(std::chrono::milliseconds{100}); return 42; },
    options);

try { int value = future.get(); } catch (...) {}

std::cout << vix::threadpool::to_string(future.status()) << '\n'; // timed_out
std::cout << vix::threadpool::to_string(future.result()) << '\n'; // timeout
```

Vix does not forcibly kill a running C++ function.

## `Promise<T>`

The producer side of a future. Most users do not need to create promises manually — use `pool.submit(fn)` instead.

```cpp
vix::threadpool::Promise<int> promise;
vix::threadpool::Future<int> future = promise.get_future();

promise.set_value(42);

int value = future.get();
```

For void:

```cpp
vix::threadpool::Promise<void> promise;
vix::threadpool::Future<void> future = promise.get_future();

promise.set_value();
future.get();
```

Setting an exception:

```cpp
try { throw std::runtime_error{"failed"}; }
catch (...) { promise.set_current_exception(); }

try { future.get(); }
catch (const std::exception &e) { std::cout << e.what() << '\n'; }
```

## Shared state

A future and promise communicate through shared state that stores: ready flag, status, result, error, value or exception, condition variable, and mutex.

```txt
Promise<T>  →  SharedState<T>  →  Future<T>
```

## Move-only behavior

`Future<T>` is move-only:

```cpp
auto future = pool.submit(fn);
auto moved = std::move(future); // valid

auto copy = future; // invalid — copying is not allowed
```

This prevents multiple owners from consuming the same result.

## Store many futures

```cpp
std::vector<vix::threadpool::Future<int>> futures;

for (int i = 0; i < 10; ++i)
{
  futures.push_back(pool.submit([i]() { return i * i; }));
}

for (auto &future : futures)
{
  std::cout << future.get() << '\n';
}
```

## Wait for many futures

Prefer `get()` over `wait()` when exceptions should be observed:

```cpp
for (auto &future : futures) { future.get(); }
```

### First exception pattern

```cpp
std::exception_ptr firstException = nullptr;

for (auto &future : futures)
{
  try { future.get(); }
  catch (...)
  {
    if (!firstException) { firstException = std::current_exception(); }
  }
}

if (firstException) { std::rethrow_exception(firstException); }
```

This is the same idea used by the parallel algorithms.

## Futures vs parallel helpers

For many independent computations, prefer higher-level helpers:

```cpp
// Manual:
std::vector<vix::threadpool::Future<int>> futures;
for (int i = 0; i < 100; ++i)
{
  futures.push_back(pool.submit([i]() { return i * i; }));
}
int sum = 0;
for (auto &f : futures) { sum += f.get(); }

// Better:
auto squares = vix::threadpool::parallel_map(pool, values,
    [](int value) { return value * value; });
```

## `post` vs `submit` vs `handle`

```cpp
pool.post(fn);              // fire-and-forget, no future
auto future = pool.submit(fn);  // future only
auto handle = pool.handle(fn);  // future + cancellation
```

## Best practices

Always call `get()` when you care about exceptions:

```cpp
auto future = pool.submit(fn);
try { future.get(); }
catch (const std::exception &e) { /* Handle failure. */ }
```

Do not ignore futures returned by important work. Use `post()` if the task is intentionally fire-and-forget. Store futures in a vector when submitting many tasks. Prefer `get()` over `wait()` for correctness.

## Simple mental model

```txt
submit(fn)
  creates Task
  creates Promise<T>
  returns Future<T>

worker thread
  runs Task
  stores result in Promise<T>

caller
  waits through Future<T>
  reads value or exception
```

In normal code:

```cpp
auto future = pool.submit(fn);
auto value = future.get();
```
