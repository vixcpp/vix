# Composition Conformance Probes

These probes evaluate whether a higher-level Operation, Activity, or Provider
preserves the Standard semantics of the work it combines. They do not require
universal `when_all`, `when_any`, `race`, `retry`, `timeout`, scope, nursery,
or operation-group C++ APIs.

## Parent and child Operations

| Probe | Required observation |
|---|---|
| Dependent child | A child participates in its parent's logical lifetime even when it progresses on another thread, coroutine, event loop, Provider, or process. |
| Parent completion | A parent does not expose terminal completion while a dependent child still uses state or Resources whose validity ends with the parent. |
| Exactly once | Many child completions, retries, cancellation requests, and cleanup stages produce one terminal `Completion` for the logical parent. |
| Explicit independence | A detached child has its own valid owner, Resource lifetime, cancellation policy, Deadline policy, and Failure-observation path. A parent does not accidentally detach work by failing to join, cancel, or retain it. |
| Resource transfer | A child-created Resource has documented authority before the parent exposes Success: it is transferred, retained by an owner, or returned as a documented borrow. |

## Sequential composition

| Probe | Required observation |
|---|---|
| DNS → TCP → TLS → HTTP | Each later stage begins only under the higher-level contract established by successful earlier work; a lower Failure prevents later dependent work unless an explicit alternate policy applies. |
| Error propagation | A same-abstraction boundary preserves the child `ErrorIdentity`. |
| Error translation | A meaningful abstraction boundary creates a higher-domain identity, retains the child error as cause, and adds only useful diagnostic context. |
| Cancellation | Dependent stages observe parent cancellation and do not continue using parent-bounded state after parent completion. |
| Deadline | Every stage receives the one parent temporal boundary or an earlier local boundary; no stage receives a fresh full duration. |
| Partial progress | Temporary owned Resources are cleaned up in dependency-safe order; borrowed Resources remain valid and are not closed merely because the composed work failed. |

## Concurrent composition

| Policy probe | Required observation |
|---|---|
| Wait for all | The parent defines whether any child Failure or Cancelled completion prevents Success and retains/aggregates every outcome required by its result contract. |
| First success | Once a winner is accepted, no-longer-needed children receive cancellation requests when appropriate, are still allowed to reach their own terminal states, and cannot retain parent-bounded state past parent completion. |
| First terminal completion | The chosen child outcome is mapped explicitly to the parent outcome; timer racing is a policy decision and does not redefine Deadline or cancellation semantics. |
| Quorum or aggregate | The contract specifies how individual Success, Failure, and Cancelled outcomes determine the parent result; no child Error is silently discarded when it remains relevant to diagnostics or aggregation. |
| Completion race | Success, Failure, cancellation request, Deadline condition, and provider shutdown race to establish one parent completion with no double resume, cleanup, or callback. |

## Cancellation and Deadline propagation

| Probe | Required observation |
|---|---|
| Parent cancellation | It normally requests cancellation of dependent children. A request is not a claim that a child has already terminated. |
| Child cancellation | It does not cancel the parent automatically. The parent contract explicitly decides whether that child outcome is optional, fatal, retried, aggregated, or causes parent cancellation. |
| Local child source | A composition may use a child-local source to stop no-longer-needed work without granting that child cancellation authority over siblings or its parent. |
| Parent Deadline | A child effective deadline is no later than the parent boundary. A stricter child deadline is the earlier of the two. |
| Retry/fallback deadline | Delays, alternate Providers, and every attempt consume one logical parent budget; none resets it. |
| Cleanup under cancellation | Cleanup has a documented cancellation policy, but cancellation never permits destruction of Resources still needed to make owned cleanup/lifetime safe. |

## Retry, fallback, and side effects

| Probe | Required observation |
|---|---|
| Retry | It is explicit composition policy, observes parent Cancellation and Deadline, and does not infer retry eligibility from a universal Error flag. |
| Side effects | A Failure or Cancelled completion does not imply that an attempt had no external effect. Retrying relies only on an operation's documented idempotence/side-effect contract. |
| Attempt diagnostics | The parent exposes its final outcome according to contract; retained previous-attempt errors, aggregate diagnostics, and cause history are policy-specific but must not make relevant causal information unavailable. |
| Provider fallback | An alternate Provider is used only when it satisfies the same required capability/security/durability/ordering contract, unless the changed semantics are explicitly exposed. |
| Partial construction | Failure after acquiring A and B but before C cleans up owned A/B in valid dependency order. Borrowed inputs are not closed. |

## Activities and shutdown

| Probe | Required observation |
|---|---|
| Server Activity | Stopping intake, draining or cancelling request children, releasing owned listener/state, and reporting final termination occur in an order that keeps dependencies valid. |
| Connection-pool shutdown | New acquisition has documented admission behavior; checked-out/active work follows a documented drain, cancellation, or failure path before pool-owned resources disappear. |
| Child Activity | A parent does not report final termination while a dependent child Activity uses parent-owned Resources. Independent child Activity ownership is explicitly transferred or established. |
| Shutdown Failure | Failure during shutdown is observed through the documented finite shutdown Operation/Completion path; it is not hidden by destruction. |
| Execution migration | Children may use different Providers or contexts, but affinity constraints and all logical lifetime, Cancellation, Deadline, ownership, and completion rules survive the transition. |

## Policy boundaries

The following are domain or library composition policies, not universal Standard
algorithms: result aggregation shape, retry schedule, fallback selection,
timeout policy, concurrency limit, queueing, fairness, flow control,
backpressure, ordering, and transactional rollback.

Domains may provide helpers for these policies only when their contracts state
the required lifetime, Cancellation, Deadline, Error/cause, ownership, and
Completion behavior. The foundational Standard supplies the invariants those
helpers MUST preserve; it does not impose one generic implementation.

Transaction-like all-or-nothing effects, rollback, distributed atomicity, and
compensation are also domain contracts. `Result`, `Completion`, cancellation,
or an Error alone never imply that partial effects were undone.
