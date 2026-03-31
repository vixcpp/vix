# async/core cancel

Cooperative cancellation primitives for Vix.cpp async code.

This header defines a tiny cancellation model:

- `cancel_source`: the owner that can request cancellation
- `cancel_token`: a cheap observer you pass into async work
- `cancel_state`: shared state storing the atomic cancellation flag
- `cancelled_ec()`: a standard `std::error_code` for “operation canceled”

The design is intentionally minimal and deterministic: no callbacks, no allocations on cancel, no surprises. It is safe to use across threads.

---

## Why this exists

In async systems, you often need a way to stop work early:

- user closes a request
- shutdown begins
- a timeout expires
- one branch in a fan-out fails and you want to cancel the rest

`cancel_token` gives every task a fast “should I stop?” check. `cancel_source` lets a controller signal that stop.

---

## Key types

### cancel_state

Shared internal object that holds the cancellation flag.

- `request_cancel()` sets an atomic boolean to true
- `is_cancelled()` reads it

Thread safety:
- writes use `memory_order_release`
- reads use `memory_order_acquire`

That is enough for a simple “flag” protocol across threads.

### cancel_token

Read-only view of a cancellation state.

- Default constructed tokens are “empty” (non-cancelable).
- Tokens are cheap to copy.
- Tokens can be passed across threads safely.

Methods:

- `can_cancel()` returns true if it is linked to a source
- `is_cancelled()` returns true if cancellation has been requested

### cancel_source

Owner of the shared state.

- Constructing a source creates a new shared state.
- `token()` returns a `cancel_token` observing the same state.
- `request_cancel()` flips the flag (idempotent).
- `is_cancelled()` reads the flag.

---

## Error model

`cancelled_ec()` returns a standard error code representing a cancelled operation:

```cpp
std::error_code ec = vix::async::core::cancelled_ec();
```

It uses `make_error_code(errc::canceled)` from `vix/async/core/error.hpp`.

Use this when you want to return an explicit “canceled” status from functions that use error codes.

---

## Usage

### 1) Basic pattern (cooperative loop)

```cpp
#include <vix/async/core/cancel.hpp>

using vix::async::core::cancel_source;
using vix::async::core::cancel_token;

void do_work(cancel_token ct)
{
  while (!ct.is_cancelled())
  {
    // ... do a chunk of work ...
    // keep chunks small enough that cancellation is responsive
  }
}
```

### 2) Cancel from another thread

```cpp
#include <thread>
#include <vix/async/core/cancel.hpp>

using vix::async::core::cancel_source;

void run()
{
  cancel_source src;
  auto tok = src.token();

  std::thread worker([tok] {
    while (!tok.is_cancelled())
    {
      // work
    }
  });

  // later
  src.request_cancel();

  worker.join();
}
```

### 3) Return cancellation as an error code

```cpp
#include <vix/async/core/cancel.hpp>

using vix::async::core::cancel_token;

std::error_code read_something(cancel_token ct)
{
  if (ct.is_cancelled())
    return vix::async::core::cancelled_ec();

  // do work...
  return {};
}
```

---

## Notes and best practices

- Cancellation is cooperative: nothing is forcibly stopped. Your code must check the token.
- Checkpoints matter: put checks at boundaries (loop iterations, before expensive steps, before blocking operations).
- Empty tokens are valid: treat them as “never canceled” and proceed normally.
- `request_cancel()` is idempotent: calling it multiple times is fine.

---

## Related

- `vix/async/core/error.hpp` provides the error category and `errc::canceled`.

