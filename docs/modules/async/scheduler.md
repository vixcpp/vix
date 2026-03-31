# scheduler

Minimal single-thread scheduler for async tasks and coroutine resumption.

---

## Overview

`scheduler` is the core execution engine of the Vix async runtime.

It provides:

- A thread-safe FIFO job queue
- An event loop (`run()`)
- Posting of:
  - Generic callables
  - Coroutine continuations
- An awaitable (`schedule()`) to hop onto the scheduler thread

It is intentionally:

- Small
- Deterministic
- Single-threaded (run() executes on the calling thread)
- Designed to be embedded inside `io_context`

---

## Basic Usage

```cpp
#include <vix/async/core/scheduler.hpp>

using namespace vix::async::core;

int main()
{
    scheduler sched;

    sched.post([] {
        std::cout << "Hello from scheduler\n";
    });

    sched.run(); // blocks
}
```

---

## Posting Work

### 1. Posting a Callable

```cpp
sched.post([] {
    do_something();
});
```

The callable is:

- Stored in a type-erased job
- Enqueued
- Executed when `run()` processes it

---

### 2. Posting a Coroutine Handle

```cpp
std::coroutine_handle<> h = ...;
sched.post(h);
```

The scheduler wraps the handle into a job that resumes the coroutine.

---

## Awaitable Scheduling

The scheduler provides:

```cpp
co_await sched.schedule();
```

### What it does

- Always suspends
- Posts the continuation to the scheduler queue
- Resumes execution on the scheduler thread

Example:

```cpp
task<void> my_task(scheduler& sched)
{
    co_await sched.schedule();
    std::cout << "Now running inside scheduler thread\n";
}
```

---

## Event Loop

### run()

```cpp
sched.run();
```

Behavior:

- Blocks the calling thread
- Waits on condition_variable
- Processes jobs in FIFO order
- Exits when:
  - `stop()` is requested
  - Queue is empty and stop flag observed

---

### stop()

```cpp
sched.stop();
```

- Sets stop flag
- Wakes all waiters
- Allows `run()` to exit cleanly

---

## Introspection

```cpp
bool running = sched.is_running();
std::size_t pending = sched.pending();
```

- `is_running()` → whether run() loop is active
- `pending()` → number of queued jobs

---

## Internal Design

### Job Queue

Jobs are stored as:

```cpp
std::deque<job>
```

Each job contains:

- A small type-erased callable holder
- Move-only semantics
- Virtual dispatch through a lightweight polymorphic base

This avoids std::function overhead while remaining generic.

---

## Thread Safety

Protected by:

- `std::mutex`
- `std::condition_variable`

Safe for:

- Multiple producers calling `post()`
- Single consumer running `run()`

---

## Design Principles

scheduler is intentionally:

- Minimal
- Predictable
- Not a thread pool
- Not a work-stealing system
- Not multi-threaded

Higher-level concurrency (CPU pools, networking, timers) is built on top of this primitive.

---

## Typical Role in Vix

`scheduler` is embedded inside:

```cpp
io_context
```

The io_context:

- Owns a scheduler
- Exposes higher-level async services
- Coordinates timers, signals, networking

---

## Summary

scheduler is the lowest-level execution primitive of the Vix async core:

- FIFO task execution
- Coroutine resumption
- Deterministic single-thread loop
- Foundation for the entire runtime

It is small by design — everything else builds on top of it.

