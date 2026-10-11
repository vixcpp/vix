# Standard Foundation Integration Review

This review connects the closed Standard axes into one programming model. It
does not introduce another foundational abstraction.

## Dependency model

```text
ErrorIdentity
  └── Error
        └── Result<T>                 Success(T) | Failure(Error)

Duration + MonotonicInstant + MonotonicClock
  └── Deadline                       temporal constraint, never completion

CancellationSource
  └── CancellationToken
        └── cancellation request      control input, never completion

Operation
  └── Completion<T>                  Success(T) | Failure(Error) | Cancelled

Resource / ownership / borrowing
  └── Operation and Activity validity/lifecycle relationships

Capability
  └── Provider                        implementation satisfying an ability
        └── execution / scheduling     progresses Operations and Activities

Composition
  └── preserves every applicable relationship above
```

The arrows describe semantic dependency, not mandatory C++ object containment.
In particular, an Operation is not a thread, coroutine, task, Provider, or
Resource; an Activity is not automatically an owner; and a `shared_ptr` is not
lifecycle authority.

There is no semantic cycle in this model. `Error` describes operational
Failure; `Result` and `Completion` carry that description but do not define
it. Cancellation and Deadline constrain an Operation but do not establish its
terminal state. Resource validity constrains work without becoming execution.
Providers supply capabilities without redefining an Operation's meaning.

## Concrete C++ vocabulary

| Type | Required role | Non-role |
|---|---|---|
| `ErrorIdentity` | Stable machine-matchable domain-local failure identity | Success state, provider registry key, diagnostic text |
| `Error` | Expected operational Failure with identity, diagnostics, contexts, and optional cause | Fault, cancellation, retry policy, severity |
| `Result<T>` / `Result<void>` | Non-cancellable finite outcome: Success or Failure | Pending work, timeout, cancellation, Activity state |
| `Duration` | Signed elapsed amount | Clock-domain point or timeout outcome |
| `WallInstant` / `WallClock` | Civil/system time, persistence, logging | Deadline or elapsed-time guarantee |
| `MonotonicInstant` / `MonotonicClock` | Process-local elapsed-time reasoning | Persistent/cross-process timestamp |
| `Deadline` | Optional-operation temporal boundary via `std::optional<Deadline>` | Error, cancellation request, terminal completion, lifecycle authority |
| `CancellationSource` | Shared authority to request cancellation | Resource close/stop authority or completion state |
| `CancellationToken` | Shared observation of a request | Authority to request cancellation or lifecycle authority |
| `CancellationRegistration` | RAII lifetime for synchronous cancellation callback observation | General event subscription or provider lifecycle handle |
| `Completion<T>` / `Completion<void>` | Cancellable Operation terminal outcome | Operation implementation, scheduler, or Activity termination protocol |

No universal `Resource`, `Owner`, `Borrower`, `Activity`, `Capability`,
`Provider`, `Executor`, `Runtime`, service locator, or composition-helper type
is required. Those are semantic contracts or domain/provider responsibilities.

## Cross-axis production probes

### Network request

```text
HTTP parent Operation
  ├── DNS child
  ├── TCP child
  ├── TLS child
  ├── request write
  └── response read
```

One parent `Deadline` is propagated as the same absolute boundary, optionally
tightened for a child. One parent `CancellationToken` is observed by dependent
children; cancellation requests do not close borrowed I/O or execution
resources. DNS/TCP/TLS provider failures either preserve their identity within
the same abstraction or become higher-domain Errors retaining lower causes and
context. Temporary owned socket/TLS state is cleaned up before a terminal
outcome makes parent-bounded state unavailable. Provider shutdown, successful
I/O, Failure, Deadline policy, and cancellation race to one `Completion`.

### Server Activity

```text
Server Activity
  ├── Listener Resource
  ├── Accept Operations
  ├── Connection Resources
  └── Request Operations
```

The server may close an internally-created listener. A caller-supplied listener
is borrowed: shutdown stops use/intake but does not close it. Shutdown first
applies its documented admission policy, then drains or requests cancellation
of dependent work while connections and execution resources remain valid.
Its shutdown `Deadline` can tighten child work but does not itself stop a
borrowed provider. Final Activity termination is observed only after dependent
children no longer access server-owned state; shutdown Failure remains
diagnosable through its documented finite lifecycle operation.

### Database transaction

A pool owns pool resources; a checked-out or leased connection has a
domain-defined ownership/borrow relationship. A query can complete Cancelled
without closing that connection. Transaction commit, rollback, connection
return, and cleanup order remain database-domain contracts, but they must keep
borrowed resources valid, preserve Error causes across provider/domain
translation, obey the effective Deadline, and establish one completion per
query/transaction operation. Pool shutdown defines acquisition admission and
active-work treatment before pool-owned state is released.

### Process and filesystem

Cancelling a process `wait` cancels the wait Operation; it does not terminate
the Process Resource unless a distinct lifecycle-authorized terminate Operation
is requested. Likewise, cancelling a file read/write does not close a borrowed
File. An owned file supports explicit close when cleanup failure must be
observable; destruction is only documented fallback cleanup and cannot be the
sole error-reporting path. Spawn/open, wait/read/write/flush, and explicit
close are each finite operations with their own `Result` or `Completion`
contract.

## Fault boundary

Invalid `Result` or `Completion` observation, invalid checked time arithmetic,
double terminal completion, illegal lifecycle authority, and an asynchronous
borrow outliving its valid owner are Faults where their documented precondition
is violated. They are never manufactured as ordinary `Error` values.

The concrete Standard currently uses deterministic termination for invalid
`Result`/`Completion` observation, invalid checked time arithmetic, and
escaping cancellation callback exceptions. That is implementation policy, not
a public exception hierarchy. It remains valid in exception-disabled builds.

## Threading and cost boundaries

| Primitive/family | Shared state and synchronization | Allocation contract |
|---|---|---|
| `ErrorIdentity`, `Duration`, instants, `Deadline` | Immutable value types; no shared state or synchronization | Never allocate |
| `WallClock` / `MonotonicClock` | Thread-safe platform clock observation; no Vix shared state | Never allocate |
| `Error` | Immutable observation is safe; same-object mutation/move is not concurrent-safe | Identity-only construction does not allocate. Vix-controlled diagnostic allocation is process-fatal on exhaustion. |
| `Result` / `Completion` | No shared mutable state; concurrent const observation is safe; mutation/move/destruction of one object is not | The wrapper does not allocate. Its `T` retains its own allocation/exception contract; contained Vix `Error` allocation is process-fatal on exhaustion. |
| Cancellation source/token/registration | Shared cancellation state is thread-safe; request/registration/reset may synchronize and invoke arbitrary callbacks | Source state, callback registration, and callback storage may allocate; Vix-controlled allocation is process-fatal on exhaustion. Token copies share state without a separate state allocation. |
| Resources, Activities, Providers, scheduling | Domain/provider contract | Domain/provider contract |

No foundational contract promises that arbitrary Resources, Activities,
Providers, or composed Operations are thread-safe. Their concurrency,
close-versus-operation, affinity, ordering, and shutdown guarantees are
declared by their domain/provider contract.

## Programming grammar

Across domains, ordinary code retains the same shape:

```cpp
vix::Completion<Response> request(
    Request request,
    vix::CancellationToken cancel,
    std::optional<vix::Deadline> deadline);

auto outcome = co_await request(make_request(), cancel, deadline);
if (outcome.status() == vix::CompletionStatus::Cancelled) {
  // The operation was deliberately discontinued; no borrowed resource closed.
}
if (outcome.status() == vix::CompletionStatus::Failure) {
  const vix::Error& error = outcome.error();
  // Match error.identity(); inspect message/context/cause for diagnostics.
}
if (outcome.status() == vix::CompletionStatus::Success) {
  Response response = outcome.take();
}
```

For value-bearing successful outcomes, `value()` borrows for inspection and
`take()` explicitly transfers the payload. A successful `take()` does not
rewrite the terminal state: a `Completion<T>` remains `Success`, and a
`Result<T>` remains successful, but their transferred payload can no longer be
observed or transferred again. Invalid access is a Fault. `Result<void>` and
`Completion<void>` have no payload and therefore no `take()`.

For a non-cancellable finite operation, the same domain vocabulary returns
`Result<T>` and uses `if (!result) return result.error();`. Domain nouns change
(`File`, `Process`, `Socket`, `Transaction`, `Request`), while failure, time,
cancellation, completion, ownership, and lifecycle language does not.

## Closure result

The production scenarios above require no additional cross-domain primitive.
Transactions/rollback, backpressure, retry policy, result aggregation, task
groups, provider selection, and shutdown sequencing remain explicit
domain/library policy constrained by the Standard invariants. They are not
missing foundation concepts.
