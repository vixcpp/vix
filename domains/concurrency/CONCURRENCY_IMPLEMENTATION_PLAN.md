# Vix Concurrency Implementation Plan

## I1 — bounded native execution slice

- Add the `vix::concurrency` target, installable public headers, package export,
  and canonical SDK consumer integration.
- Provide explicitly configured native workers, bounded admission, callable
  normalization, shared WorkHandle observation, cooperative cancellation,
  worker exception-to-Completion mapping, and safe external shutdown.
- Make WASI native execution explicitly unavailable without importing thread
  machinery or returning a fake resource.
- Add source-level unit, API-shape, and WASI unsupported-capability tests.

## I2 — one-shot completion notifications (this change)

- Add atomic subscribe-or-observe through `WorkHandle<T>::on_ready()` with a
  single lifetime slot per work item and resource-worker callback dispatch.
- Add move-only `ReadySubscription` and `NotificationRegistration` ownership,
  nonblocking deactivation, quiescent reset/wait, callback error observation,
  exception conversion, and shutdown draining.
- Bound outstanding notifications by the resource's accepted-work capacity;
  do not create a dispatcher, detached worker, coroutine Task, or global runtime.
- Extend native execution tests, the exception-disabled compile probe, and the
  installed SDK consumer to cover the new public contract.

## I3 — deadline-aware work (implemented in this change)

- Add top-level `WorkOptions` with an optional Standard absolute monotonic
  `vix::Deadline`; preserve `submit(callable)` and add
  `submit(options, callable)`.
- Reject expired deadlines before acceptance, recheck at the admission
  linearization point, and arbitrate worker claim against pending expiry under
  the work-state lock.
- Use one resource-owned native deadline observer, independent of work-worker
  availability, to terminalize pending work or request cooperative
  cancellation for running work. Keep timeout outcomes and I2 notifications
  exactly-once; preserve actual running callable results.
- Stop and join the observer during shutdown. Keep WASI Preview 2 execution
  explicitly unsupported without thread or timer creation.
- Add focused expiry, running cancellation, result-preservation, race,
  notification, post-shutdown observation, capacity-retirement, and
  exception-disabled/SDK compile coverage.

## Before stable API freeze

- Run strict-warning builds and runtime tests on Linux glibc/musl, macOS,
  Windows, and Android arm64-v8a/x86_64.
- Exercise races under sanitizers and verify callback reentrancy, concurrent
  shutdown, capacity boundaries, handle copies, and resource destruction.
- Verify installed package relocation and independent public consumers for
  each qualified host tuple.
- Review the provisional error identities, overload constraints, ABI, and
  exception-disabled contract before declaring the public API stable.

## Later capabilities (not promised by I1–I3)

Cooperative coroutine composition, structured scopes, and additional execution
strategies require separate contracts and conformance tests. They must not
take over Network's reactor or imply that cooperative progress is CPU
parallelism.
