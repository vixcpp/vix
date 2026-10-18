# Vix Concurrency I1–I3 Contracts

Status: I1 execution, I2 one-shot notification, and I3 deadline contracts.
These describe the implemented source surface; deferred capabilities below
are not implied.

## Normative I1 decisions

- `vix::concurrency::ExecutionResource` is explicitly created and locally
  owned. It owns the native execution workers and every work item it accepts.
  There is no global runtime and no public worker-pool type.
- Admission is bounded by `max_concurrency + max_pending`. An idle execution
  slot accepts work even when `max_pending == 0`. A submission rejected for
  capacity or shutdown does not invoke or move from the submitted callable.
  `max_concurrency` must be nonzero; arithmetic or container-capacity overflow
  is rejected as `invalid_configuration`. `create()` defaults to one worker
  and no queued allowance, avoiding hardware-dependent worker creation or an
  implicit memory queue. Callers needing concurrency must configure it.
  `is_shutdown()` reports admission closure, not worker quiescence; successful
  `shutdown()` is the quiescence boundary.
- Accepted work is owned by the resource until terminal publication and
  removal from its active set. Copyable `WorkHandle<T>` values share control
  and observation of that one work record; destroying a handle neither
  cancels nor detaches work. The callable object is destroyed before terminal
  publication, so a handle retained after completion does not retain its
  captures; borrowed referents remain the caller's responsibility while work
  is pending or running.
- Submission failures use `vix::Result<WorkHandle<T>>`. Terminal outcomes use
  `vix::Completion<T>`: successful `T`/`void`, an operational `vix::Error`, or
  cancellation before invocation. Callable `Result<T>` and `Completion<T>`
  returns are normalized to the same terminal `Completion<T>`; nested
  `Completion<Result<T>>` is rejected.
- `request_cancel()` is a cooperative request. It does not interrupt a running
  callable or rewrite a result the callable actually returns. A pending item
  transitions to Cancelled immediately after its synchronous cancellation
  callbacks return; a running callable decides how to respond by inspecting
  `WorkContext::cancellation_token()`. It may synchronously invoke Standard
  cancellation callbacks and destroy a pending callable, so it can block. A
  cancelled queued entry remains capacity-accounted until a worker retires
  that queue entry or shutdown drains it.
- `WorkHandle<T>::on_ready()` atomically returns either an already-ready
  observation or the unique pending notification slot for that work item.
  An already-ready result never invokes its callback inline. Notifications are
  dispatched by resource-owned workers after terminal publication, including
  publication caused by pending cancellation or shutdown. A work item has one
  registration slot for its lifetime; copied handles compete for it, and a
  consumed or deactivated slot cannot be replaced. Outstanding registrations
  are bounded by the resource's total accepted-work capacity and excess
  registrations fail with `notification_capacity_exhausted`. A registration
  that is not extracted from `ReadySubscription` is deactivated by its RAII
  cleanup.
- `NotificationRegistration::deactivate()` is nonblocking. It prevents a
  queued callback from starting when possible; an invocation already running
  continues. `reset()` deactivates and waits for quiescence, and `wait()` waits
  without deactivating. Both return a callback's failure, if any. Quiescence
  includes destruction of the callback closure, which occurs outside internal
  locks before waiters are released. External destruction deactivates and
  waits. Self-deactivation from the callback is nonblocking; self-reset returns
  `would_deadlock` and preserves the registration so an external observer can
  collect its eventual outcome. The worker retains the callback state through
  return. Destruction on a resource worker
  while a callback is running elsewhere is a contract fault rather than an
  unsafe non-quiescent return. Borrowed callback captures must remain alive
  until reset/destruction completes or callback completion is observed.
  Callbacks run on a resource worker without internal locks held; they may
  return `void` or `Result<void>`. Returned errors are preserved and thrown
  exceptions become `notification_exception`, separate from the work's
  immutable `Completion<T>`. A ready outcome does not retain or invoke the
  supplied callback. `deactivate`, `wait`, and `reset` may race on the same
  live registration object; moving or destroying that C++ object must not race
  with a member call.
- `shutdown()` closes admission, requests cancellation of accepted work,
  prevents still-pending work from beginning, and waits until running work
  exits, cancellation callbacks started by shutdown have returned, and workers
  are joined. It may wait indefinitely for non-cooperative work or a blocking
  cancellation callback. A token deliberately retained by caller code may be
  used to register a later callback after shutdown; that caller-owned callback
  is governed by Standard's CancellationToken contract, not resource ownership.
  Concurrent external shutdowns converge on the same quiescent result.
  It also drains all accepted notifications and does not return while a
  resource-owned notification callback is queued or running. A registration
  for an existing pending/running item remains admissible after closure begins;
  its outstanding obligation keeps workers alive through callback dispatch.
  Resource-owned references to completed records are released outside internal
  locks before the first successful shutdown returns, so user value destructors
  are not invoked under the resource mutex.
- Calling `shutdown()` on one of the resource's workers, or from a Standard
  cancellation callback, returns a specific error instead of self-joining or
  re-entering shutdown. Calling it from notification invocation or callback
  closure cleanup for that same resource returns
  `shutdown_from_notification_callback`. Shutdown reentered on its own
  shutdown thread also returns `shutdown_reentrant`. Destroying the live resource on its own worker,
  inside its cancellation callback, or reentrantly on the shutdown owner thread
  is a contract fault and terminates; callers must retain an external owner
  until shutdown completes.
- Standard cancellation callbacks run synchronously on the thread that wins
  cancellation delivery. An exception escaping such a callback follows
  Standard's callback Fault policy (termination), not `worker_exception`.
- Catchable exceptions escaping a callable become a terminal
  `worker_exception` failure. A `std::exception::what()` diagnostic is copied
  when available. Failure to allocate Vix-controlled error diagnostics follows
  Standard's fatal allocation policy. Exception-disabled builds retain the
  same Result/Completion API; the callable must not throw in such a build.
  The I1 exception-disabled target is compile-only. On native standard
  libraries configured without exceptions, failure to construct `std::thread`
  cannot be translated portably into `resource_creation_failed` and follows
  that library's no-exception failure behavior; hosted runtime qualification
  for that configuration is not claimed.
- Work callables must be nothrow-move-constructible and nothrow-destructible,
  and their result must be nothrow-move-constructible. This makes ownership
  transfer and one-shot terminal observation deterministic.
- `WorkOptions::deadline` is an optional absolute `vix::Deadline` using
  Standard's process-local monotonic clock. `submit(callable)` remains
  equivalent to submission with empty `WorkOptions`; the deadline overload is
  `submit(options, callable)`. A deadline already reached at admission rejects
  synchronously with `errors::deadline_expired` and does not invoke or move
  from the callable. Admission checks the deadline again at its resource-lock
  linearization point.
- Each native execution resource owns one private deadline-observer thread in
  addition to its configured work workers. It scans the bounded active-work
  set and sleeps until the next deadline or a schedule/shutdown change; it is
  not a public scheduler, global runtime, detached thread, or Network reactor.
  Observation latency is best-effort and depends on host scheduling and
  synchronous cancellation callbacks; no hard real-time bound is promised.
- A worker checks the deadline while claiming pending work. If the deadline
  has been reached before that claim, the work becomes terminal with a
  `deadline_expired` failure and its callable is destroyed without invocation.
  The observer uses the same work-state lock, so a claim and expiry cannot both
  win. A pending timeout is published once and follows the ordinary I2
  notification path. Its queued work entry still consumes capacity until a
  worker safely retires that entry.
- If expiry wins while work is already running, the resource requests the
  work's existing Standard cancellation source. This request is cooperative
  and invokes cancellation callbacks synchronously on the deadline-observer
  thread; callbacks execute without resource/work locks and can delay timer
  processing. The callable is never forcibly interrupted. Its actual terminal
  result is preserved, including success after the deadline. Explicit
  cancellation and expiry arbitrate under the work-state mutex; whichever
  transition wins is not overwritten, and only one cancellation delivery and
  one terminal publication occur.
- Shutdown closes admission, signals and joins the deadline observer, requests
  cancellation of accepted work, and waits for work and notifications to
  quiesce. Expiry racing shutdown is resolved by the same per-work state lock:
  pending work terminalizes exactly once as either cancelled or timed out.
  WorkHandles remain usable for observing terminal results after resource
  shutdown. WASI Preview 2 continues to reject execution with
  `execution_unavailable`; it does not instantiate a timer or worker thread.
- If a callable accepts both `WorkContext&` and no arguments, the context-taking
  form is selected. Lvalue callables are copied; rvalue callables are moved.
  The empty work record and any required lvalue copy are prepared before
  admission; construction exceptions return `resource_creation_failed` without
  consuming the caller's callable. After admission, only a nothrow callable
  move and bounded queue commit remain; Standard-controlled allocation
  exhaustion remains fatal.
- Blocking `wait()` on unfinished work from a worker of the same resource or
  from a Standard cancellation callback is rejected as `would_deadlock`.
  Callers must not block event loops/reactors or required continuation threads
  on unfinished work.

## Deferred capabilities

Coroutine awaiting, work scopes, priorities, fairness guarantees, forced
interruption, and autonomous progress without native workers are not
implemented. In particular, WASI
Preview 2 native execution is explicitly unsupported and resource creation
returns `execution_unavailable`; it does not fabricate an execution resource.
These omissions are not placeholders and must not be inferred from the API.
No relative timeout policy is introduced; callers construct absolute deadlines
through Standard's `vix::Deadline` API.

## I1–I2 error identities

`errors::invalid_configuration`, `execution_unavailable`,
`resource_creation_failed`, `capacity_exhausted`, `resource_shutting_down`,
`invalid_work_handle`, `completion_already_taken`, `would_deadlock`,
`shutdown_from_worker`, `shutdown_from_cancellation_callback`,
`shutdown_reentrant`, `shutdown_from_notification_callback`, `worker_exception`, `notification_exception`,
`notification_already_registered`, `invalid_notification_registration`,
`notification_registration_failed`, and `notification_capacity_exhausted`
and `deadline_expired` are distinct machine-readable identities in the
`vix.concurrency` domain.
Worker failures appear in work `Completion<T>`; notification failures are
observed through `NotificationRegistration::wait/reset`; other identities
report resource creation, admission, control, or observation errors through
`Result`.

## Progress and visibility

The successful capacity reservation under the resource mutex is the admission
linearization point. The submitted callable is then moved into its work record
outside the mutex; shutdown closes new reservations and waits for these
in-progress accepted submissions to commit before taking its quiescence
snapshot. A rejected reservation does not move from the callable. Completion
is published once under the work-state mutex.
`ready()` uses synchronized observation; successful `wait()` and completion
observation acquire the same synchronization and therefore observe the
callable's effects sequenced before terminal publication.

Admission and shutdown linearize under the resource mutex. A submission that
observes the closed state is rejected without consuming its callable. A
reservation committed before closure is accepted and accounted for by
shutdown, even if construction of the internal work record is still in
progress.

Capacity is reserved for accepted, not-yet-retired records. A record can remain
counted briefly after `ready()` becomes true while its worker retires it; a
concurrent submission may therefore receive `capacity_exhausted` during that
cleanup interval. This is bounded and does not affect the completed result.

## Lifetime boundary

The resource owns the callable object, not objects borrowed by that callable.
References, raw pointers, spans, string views, and references captured by a
closure must remain valid until the work is terminal. Prefer owned values or
explicit shared ownership. A `WorkHandle` does not extend the lifetime of
borrowed callable captures beyond the resource's normal work ownership.

## Platform boundary

The native provider is selected for Linux, macOS, Windows, and supported
Android NDK ABIs. The WASI provider only reports unsupported native execution.
No source-level implementation is a platform verification claim; native
build, runtime, sanitizer, and installed-consumer qualification remain
required.
