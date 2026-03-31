# Timers: interval

`vix::timers::interval()` is a tiny utility that runs a repeating timer loop on a dedicated thread, and posts the actual work to an executor.

This design keeps timing separate from execution:
- the **timer thread** only wakes up and schedules
- the **executor** runs the callback (potentially on a pool)

---

## Files

- `vix/timers/interval.hpp`

---

## Key types

### `vix::timers::IntervalHandle`

RAII handle that owns:
- a shared stop flag (`IntervalHandle::State`)
- a worker thread that wakes up every period and posts the task

When the handle is destroyed, it stops the loop and joins the thread.

Important members:
- `std::shared_ptr<State> state`
- `std::thread t`

Important methods:
- `stopNow()` stops and joins (safe to call multiple times)
- destructor calls `stopNow()` (RAII)

---

## API

### `IntervalHandle interval(IExecutor& exec, milliseconds period, function<void()> fn, TaskOptions opt = {})`

Behavior:
1. Creates `IntervalHandle` with a shared `State{stop=false}`
2. Starts a thread that:
   - computes `next = now + period`
   - loops:
     - exits if handle is gone or `stop=true`
     - `exec.post(fn, opt)`
     - `sleep_until(next)`
     - `next += period`
3. Returns the handle

Notes:
- The return handle controls lifetime. If you drop it, the interval stops.
- The callback is posted to `exec`. It does **not** run on the timer thread.
- The scheduling is “fixed cadence” using `sleep_until(next)` then `next += period` to reduce drift.

---

## Typical usage

### 1) Store the handle in a long-lived owner (server, app)

```cpp
#include <vix/timers/interval.hpp>

class MyServer {
public:
  explicit MyServer(vix::executor::IExecutor& exec) : exec_(exec) {}

  void start() {
    metrics_ = vix::timers::interval(
      exec_,
      std::chrono::seconds(5),
      [this] { collectMetrics(); },
      {} // TaskOptions
    );
  }

  void stop() {
    metrics_.stopNow(); // optional; destructor also stops
  }

private:
  void collectMetrics() {
    // read counters, log, etc
  }

  vix::executor::IExecutor& exec_;
  vix::timers::IntervalHandle metrics_{};
};
```

### 2) Stop explicitly

```cpp
auto h = vix::timers::interval(exec, 250ms, []{ /* ... */ });
/* ... */
h.stopNow(); // stops loop + joins thread
```

---

## Concurrency and safety

- `IntervalHandle` is movable, not copyable.
- `stopNow()` is idempotent:
  - sets `stop=true` if state exists
  - joins the thread if joinable
- The worker thread captures `exec` by reference.
  - Make sure `exec` outlives the interval handle.
- The loop uses a `weak_ptr` to detect handle destruction safely.

---

## Practical guidance

- Use it for “background periodic work”:
  - executor metrics logging
  - housekeeping tasks
  - cache refresh
- Keep the posted callback short. If it is heavy, your executor should route it appropriately (priority, heavy routes, separate pool).
- If you need stronger timer semantics (jitter control, cancel callbacks already queued), that logic should live in the executor or in a higher-level scheduler.

