# Operations

An Operation is a finite unit of work that represents something being attempted.

Operations are used throughout Vix to model actions such as opening a file, resolving a hostname, connecting a socket, reading data, writing data, executing a query, starting a process, waiting for a timer, performing an HTTP request, or carrying out another bounded piece of work.

The defining property of an Operation is not how it is implemented. An Operation may run synchronously or asynchronously. It may execute entirely on the calling thread, suspend through a coroutine, use an event loop, delegate work to a thread pool, call into an operating-system service, or combine several lower-level operations.

What makes it an Operation is that it represents finite work governed by a contract and eventually reaches one terminal completion.

## Finite work

An Operation has a bounded logical lifetime.

This does not mean that the duration is known in advance or that the implementation is guaranteed to complete within a particular amount of time. It means that the Operation is conceptually expected to reach a terminal state rather than continue producing independent events indefinitely.

Long-lived behavior such as a server, subscription, file watcher, or persistent message stream is better modeled as an Activity.

An Activity may create or supervise many Operations during its lifetime, while each Operation still has its own finite completion.

For example:

```text id="9djqq8"
Server Activity
├── Request Operation
├── Request Operation
├── Request Operation
└── ...
```

The server may continue running while individual request operations begin and complete.

## One logical completion

Each logical Operation has one terminal completion.

The implementation may contain many internal steps, retries, child operations, scheduler transitions, suspensions, callbacks, or lower-level completions. These internal events do not create multiple terminal completions for the logical Operation seen by its caller.

Once the terminal completion of an Operation has been observed, that logical Operation is complete.

An implementation MUST NOT report more than one terminal completion for the same Operation.

The completion may represent success, operational failure, or cancellation according to the applicable contract.

Progress, retry attempts, cancellation requests, deadline expiration, intermediate values, and internal state transitions are not terminal completion by themselves.

## Operation identity

The Standard defines Operation as a semantic concept and does not require every Operation to have an explicit runtime identifier.

An implementation may assign IDs for tracing, diagnostics, replay, observability, distributed correlation, or other purposes.

Such identifiers may be useful, but they are not what makes something an Operation.

The logical identity of an Operation is determined by its contract and lifetime rather than by the existence of a numeric or textual ID.

## Operation contract

Every Operation is governed by a contract.

The contract describes the observable semantics of the work being attempted.

Depending on the Operation, the contract may define:

- valid inputs;
- required capabilities;
- required resource validity;
- ownership and borrowing relationships;
- successful completion;
- expected operational failures;
- cancellation behavior;
- deadline behavior;
- partial progress;
- side effects;
- child-operation relationships;
- cleanup expectations;
- and any other observable guarantees relevant to the caller.

The contract determines whether a condition is considered Success, Failure, Cancelled completion, or a programming Fault.

For example, malformed external data may be an expected Failure for a parser whose purpose is to validate untrusted input. The same malformed state may represent a Fault inside an internal component whose contract guarantees that such data has already been validated.

Classification therefore follows the contract rather than the physical symptom alone.

## Starting an Operation

An Operation begins when responsibility for performing the requested work has been established according to the API contract.

The exact point at which this occurs may differ between synchronous and asynchronous APIs.

A synchronous function may begin and complete the Operation entirely within one function call.

An asynchronous API may return a handle, coroutine object, future, task, or another representation whose execution begins immediately or later according to its contract.

The Standard does not require one universal start mechanism.

What matters is that the semantics of when the Operation becomes active are understandable.

APIs SHOULD avoid ambiguous behavior in which it is unclear whether construction, submission, awaiting, polling, or another action actually begins the Operation.

## Synchronous Operations

A synchronous Operation completes before control returns from the call that performs it.

Its execution may still use resources, capabilities, system calls, internal helper operations, or implementation-specific machinery.

The synchronous nature of the API does not make it semantically different from an asynchronous Operation in areas such as failure, resource validity, ownership, or contract definition.

For example:

```text id="y3n8t6"
read_config()
    begins Operation
    performs work
    returns Success or Failure
```

The Operation lifetime is contained within the call.

## Asynchronous Operations

An asynchronous Operation may remain active after the initiating call returns or suspends.

Its lifetime therefore becomes especially important.

The Operation may retain resources, borrowed state, buffers, callbacks, execution facilities, cancellation state, deadlines, or other dependencies while it remains incomplete.

An asynchronous Operation MUST NOT continue accessing borrowed state after the borrowing contract has expired.

If the Operation needs state to survive for its entire lifetime, the implementation must establish an appropriate lifetime relationship through ownership, retained borrowing, copying, transfer, or another valid mechanism.

The fact that an API is asynchronous must not make resource lifetime implicit.

## Operation representation

The Standard does not require an Operation to be represented by a class named `Operation`.

An Operation may be represented through:

- a normal function call;
- a coroutine;
- a future;
- a task type;
- a callback-based API;
- a request object;
- an internal state machine;
- a system operation;
- or another mechanism.

A type such as `task<T>` may represent the asynchronous execution of an Operation, but `task<T>` and Operation are not synonymous.

The semantic model exists above the representation.

This distinction allows Vix to change implementation techniques without redefining the work being performed.

## Operation and execution

An Operation describes what work is being attempted.

Execution describes how that work is progressed.

The two concepts are related but distinct.

For example:

```text id="8cj5w9"
Operation:
    resolve "example.com"

Possible execution:
    operating-system resolver
    asynchronous I/O provider
    worker thread
```

The Operation remains name resolution even if the execution mechanism changes.

An Operation MUST NOT require a concrete execution implementation merely because the current implementation happens to use that mechanism.

If an Operation genuinely requires an execution capability, its contract may depend on that capability without depending unnecessarily on a specific provider.

## Operations and scheduling

Scheduling determines when and where executable work progresses.

An Operation may be affected by scheduling policy without becoming identical to the scheduler.

Priority, fairness, worker selection, queueing, affinity, deadlines, or other scheduling concerns may influence execution.

These concerns should not redefine the semantic identity of the Operation.

A scheduler may also outlive many individual Operations.

An Operation using a scheduler normally borrows the scheduling or execution facility unless its contract explicitly gives it ownership.

Completing or cancelling the Operation does not automatically authorize it to stop the scheduler.

## Required capabilities

An Operation may require one or more capabilities.

For example:

```text id="7pcyw0"
HTTP request
may require:
    DNS
    TCP
    TLS
    monotonic time
    asynchronous I/O
```

A process-spawn Operation may require process execution capability.

A filesystem Operation may require filesystem capability.

The Standard does not require the Operation to carry a runtime list of these capabilities.

The relationship is semantic.

Required capabilities describe what must be available for the Operation contract to be implementable.

If a required capability is absent entirely from the applicable environment or profile, that is different from the capability being present but an individual Operation failing while using it.

## Operations and Resources

Operations frequently use Resources.

An Operation may:

- borrow an existing Resource;
- create a new Resource;
- transfer a Resource;
- consume a Resource;
- temporarily retain access to a Resource;
- or produce a Resource as part of successful completion.

For example:

```text id="oe96ok"
TCP connect Operation
    uses TCP capability
    may use DNS capability
    on Success
        produces Socket Resource
```

An open-file Operation may similarly produce a File Resource.

The ownership of a Resource produced by an Operation must be defined by the Operation contract.

Successful completion does not by itself say whether the caller owns, borrows, or shares the returned Resource.

## Borrowed Resources

If an Operation borrows a Resource, the Resource must remain valid for the period required by the Operation.

For synchronous Operations this period may end when the call returns.

For asynchronous Operations it may extend until terminal completion or another explicitly defined release point.

The Operation MUST NOT assume lifecycle authority over a borrowed Resource.

Completion of the Operation does not authorize it to close or stop a broader-scope Resource unless that authority is part of the contract.

This remains true even if the Operation retains a shared pointer or another reference-counted representation.

## Resources created by Operations

An Operation may create a Resource as part of its work.

The contract must define the lifecycle relationship of the newly created Resource.

A common model is:

```text id="rjqsrm"
Operation
    creates Resource
    completes successfully
    transfers Resource ownership to caller
```

For example:

```text id="22kq31"
open(path)
    → Success(File)
```

may transfer ownership of the opened file to the caller.

However, other models are possible.

An Operation may create a Resource that remains owned by a parent Activity, an orchestration layer, a connection pool, or another component.

Ownership must be defined rather than inferred from the fact that the Operation created the Resource internally.

## Successful completion

Success means that the Operation satisfied its contract.

The precise meaning is operation-specific.

Success may produce a value:

```text id="qmef10"
resolve()
    → address
```

or a Resource:

```text id="h8rjgf"
connect()
    → socket
```

or no meaningful returned value:

```text id="e0f304"
flush()
    → success
```

or an observable state transition:

```text id="0k3y6r"
commit()
    → transaction committed
```

The Standard does not require successful completion to carry a value.

## Failure

An Operation may complete with an operational Failure when it was valid to attempt the work but the contract could not be satisfied because of an operational condition.

Examples may include:

```text id="qnwah4"
file not found
permission denied
connection refused
invalid external input
remote service unavailable
disk full
```

The applicable contract determines which failures are expected and how they are represented.

Operational Failure is distinct from a programming Fault.

A conforming API SHOULD make expected operational failures available through a structured mechanism rather than requiring callers to inspect diagnostic text.

## Faults during Operations

An Operation may encounter a programming Fault.

A Fault means that some programming contract or invariant has been violated.

This is different from an ordinary Operation Failure.

The Standard does not require programming Faults to be converted into operational Error values.

Doing so automatically could hide defects by turning broken invariants into routine control flow.

The implementation may diagnose, throw, terminate, assert, or otherwise react according to the applicable Fault policy.

The semantic distinction between Fault and Failure must remain preserved even if a particular build configuration handles them using similar low-level mechanisms.

## Cancellation

An active Operation may support Cancellation.

Cancellation begins with a cancellation request.

The request expresses intent to discontinue the Operation.

It does not itself complete the Operation.

The Operation may need to observe the request, cancel lower-level work, release resources, perform cleanup, or wait for an underlying system operation to reach a terminal state.

A cancellation request may race with successful completion.

If Success becomes terminal according to the Operation contract before cancellation takes effect, the terminal completion remains Success.

An Operation MUST NOT produce both Success and Cancelled completion for the same logical lifetime.

## Cancellation support

Not every Operation is necessarily cancellable.

Some Operations may complete too quickly for meaningful cancellation. Some underlying platforms may not support interruption of a particular system action. Some contracts may define cancellation only at specific boundaries.

If an Operation advertises cancellation support, the contract should define what that support means.

Cancellation support does not necessarily mean immediate termination.

The implementation SHOULD make clear whether cancellation is cooperative, best-effort, deferred to safe points, or governed by another defined model when that distinction is observable.

## Cancellation and side effects

Cancelled completion does not imply that nothing happened.

An Operation may perform partial work before cancellation takes effect.

Examples include:

```text id="l7k1sp"
partial network transmission
partial file write
partial external request
partial computation
resource allocation followed by cleanup
```

If partial progress or externally visible effects matter to the caller, the Operation contract should define how they are observed or reasoned about.

The Standard does not assume transactional rollback merely because an Operation is cancelled.

## Deadlines

An Operation may have a Deadline.

The Deadline expresses a temporal constraint on the Operation or on the lifetime in which the Operation participates.

A Deadline is not itself a terminal completion.

When the Deadline is reached, the applicable contract defines the reaction.

Possible reactions may include requesting Cancellation, producing a Failure, returning a normal timeout result, or applying another defined policy.

The Standard does not require all deadline expiration to map to one universal terminal category.

## Parent deadlines

When an Operation is part of a parent Operation with a bounded lifetime, the child should normally respect the parent's effective Deadline.

A child Operation MUST NOT silently extend the parent's effective time budget unless the contract explicitly allows it.

For example:

```text id="34i0br"
HTTP Request
overall deadline: 5 seconds

    DNS
    TCP connect
    TLS handshake
    response read
```

The child Operations should participate in the remaining overall deadline rather than each automatically receiving a fresh five-second budget.

This rule prevents composition from changing the meaning of higher-level time constraints.

## Child Operations

An Operation may create or supervise child Operations.

A child Operation is part of the logical work of its parent when their lifetimes are connected by the composition contract.

For example:

```text id="6bkxqu"
HTTP Request Operation
├── DNS Operation
├── Connect Operation
├── TLS Handshake Operation
├── Write Operation
└── Read Operation
```

The parent still exposes one terminal completion even though several child Operations may complete internally.

Child Operations may have their own operational failures, cancellation handling, resources, and internal implementation mechanisms.

The parent contract determines how those outcomes contribute to the parent completion.

## Cancellation propagation

When a child Operation participates in the lifetime of its parent, cancellation of the parent should normally propagate to that child.

Propagation does not mean that the child becomes instantly complete.

It means that the child receives the appropriate cancellation request according to the composition contract.

A child with intentionally independent lifetime may be exempt from parent cancellation.

Such independence must be explicit.

The Standard SHOULD prefer cancellation propagation within shared operation lifetimes over silently leaving dependent child work running after its parent has been abandoned.

## Detached Operations

An Operation may intentionally become independent from the lifetime that initiated it.

This is detached work.

Detachment changes the lifetime relationship and therefore must be explicit in the semantic contract.

A detached Operation must not continue using resources whose lifetime belonged only to the original parent unless an appropriate independent lifetime has also been established for those resources.

Detachment is not merely an execution detail.

It is a lifetime decision.

The Standard does not require one API named `detach`, but accidental detachment is not considered a valid default model.

## Internal attempts and retries

An Operation may perform more than one internal attempt.

For example:

```text id="ualptf"
Logical Request Operation
├── Attempt 1 → Failure
├── Attempt 2 → Failure
└── Attempt 3 → Success
```

The caller may still observe only one logical terminal completion:

```text id="qcy0s2"
Success
```

Internal attempts do not automatically become separate logical Operations at the public contract boundary.

Retry policies must continue to respect the lifetime, cancellation, and deadline constraints of the logical Operation.

A retry MUST NOT silently reset an inherited Deadline unless the applicable contract explicitly defines that behavior.

Retries also must not erase important error context needed to explain why the logical Operation ultimately failed.

## Partial progress

Some Operations can make observable progress before terminal completion.

Examples include reading some bytes, writing part of a buffer, processing several records, or receiving part of a response.

Partial progress does not necessarily imply terminal Success.

The Operation contract determines how progress relates to completion.

For example:

```text id="p2vexz"
read_some()
```

may treat receiving any non-zero amount of data as successful completion.

By contrast:

```text id="7kfyqm"
read_exactly(1024)
```

may require all requested data before reporting Success.

The physical event is similar, but the Operation contracts differ.

The Standard therefore does not classify partial progress independently from the contract.

## Side effects

Operations may have side effects.

Failure or Cancellation does not automatically imply that all side effects were undone.

An Operation that requires atomicity, rollback, transactional behavior, or all-or-nothing semantics must define that guarantee explicitly.

Callers must not infer atomicity merely from the existence of an Error or Cancelled completion.

This matters especially for networking, filesystem operations, distributed systems, process control, and other domains where external effects may occur incrementally.

## Operation composition

Higher-level Operations are often composed from lower-level Operations.

Composition should preserve the semantics defined by the participating contracts.

In particular, composition should preserve understandable behavior for:

```text id="sq3pms"
ownership
resource lifetime
cancellation
deadlines
failure identity
error context
partial effects
child lifetime
```

A higher-level Operation may translate or enrich lower-level outcomes when necessary for its domain, but it should not erase semantic distinctions without a clear contract.

For example, a low-level connection failure may become part of an HTTP request Failure while retaining enough causal information to diagnose the connection problem.

## Error context

When a child Operation fails, a parent Operation may add context.

For example:

```text id="so2qow"
connection_refused
```

may become:

```text id="vyo80w"
while connecting to api.example.com
    caused by connection_refused
```

which may become:

```text id="q4eb37"
while loading account profile
    caused by HTTP connection failure
        caused by connection_refused
```

Adding context should improve diagnostics without changing the underlying identity of the original cause unnecessarily.

The parent may also expose its own higher-level Error identity when appropriate, but causal information should remain available when the distinction is important.

## Operation completion and cleanup

Terminal completion does not necessarily mean that every implementation object has already been destroyed.

An Operation may finish logically while local cleanup, tracing, deallocation, or bookkeeping continues internally.

However, once terminal completion is observable to the caller, the implementation must respect the guarantees associated with that completion.

For example, if successful completion transfers ownership of a Resource to the caller, that Resource must already be valid according to the contract.

If cancelled completion guarantees that a particular borrow is no longer used, the implementation must satisfy that guarantee before exposing the completion.

Logical completion therefore defines a semantic boundary even when internal cleanup details continue.

## Completion observation

An API may expose completion through different mechanisms.

Examples include:

```text id="1qwc6z"
return value
co_await
future readiness
callback
poll result
event notification
```

The representation does not alter the single-completion rule.

An implementation must ensure that multiple observation mechanisms do not accidentally turn one Operation into multiple terminal completions.

If multiple observers are allowed, the contract must define whether they observe the same terminal state, consume it, share it, or follow another explicit model.

## Operation state

Implementations may expose Operation state such as:

```text id="rqm60g"
created
pending
running
cancellation_requested
completing
completed
```

The Standard does not require a universal Operation state machine.

Such states are implementation or API concerns unless their behavior becomes part of the public contract.

The Standard only requires that intermediate states not be confused with terminal completion.

## Operations that produce no terminal value

Some Operations perform actions whose Success carries no meaningful result value.

Examples include:

```text id="cf69ok"
flush
close
send acknowledgement
apply configuration
sleep
```

These are still Operations.

The absence of a return value does not make the Operation an Activity.

The distinction remains whether the work is finite and terminates once.

## Operations that produce Resources

When successful completion creates or yields a Resource, the Operation contract must define the resulting ownership relationship.

Examples include:

```text id="th9dc2"
open file
connect socket
create process
begin transaction
acquire connection
```

The caller may receive exclusive ownership, shared lifecycle participation, a borrow, a lease, or another defined relationship.

The Standard does not assume one ownership model for all resource-producing Operations.

## Operations and Activities

Operations and Activities frequently interact.

An Operation may start an Activity.

For example:

```text id="0y8zky"
start_server()
    → Success(Server)
```

where the resulting server participates in a long-running Activity.

An Activity may produce Operations:

```text id="ht8p0b"
Server Activity
    → Request Operation
```

An Operation may stop or modify an Activity:

```text id="4oxjqe"
shutdown_server()
    → Operation
```

These relationships do not collapse the concepts.

The distinction helps Vix reason separately about finite completion and continuing behavior.

## Operations and authorization

Having the capability required for an Operation does not mean the Operation is authorized.

For example, filesystem capability may exist while a particular path is inaccessible.

The Operation may therefore fail because authorization is denied even though the required capability is present.

This is an operational Failure rather than a missing-capability condition.

The contract should preserve the distinction where it matters.

## Normative requirements

An Operation MUST represent finite logical work.

An Operation MUST have no more than one terminal completion for a single logical lifetime.

Terminal completion MUST be distinguishable from progress, cancellation requests, deadline expiration, retries, and other intermediate events.

The contract of an Operation MUST define the observable conditions for successful completion.

Expected operational inability to satisfy the Operation contract MUST remain semantically distinct from programming Faults.

An asynchronous Operation MUST preserve the lifetime requirements of every Resource or borrowed object it continues to use.

An Operation MUST NOT assume lifecycle authority over a borrowed Resource unless that authority has been explicitly granted or transferred.

Cancellation requests MUST NOT be treated as proof that an Operation has already completed.

A cancelled Operation MUST NOT also expose a second terminal Success or Failure for the same logical lifetime.

A child Operation participating in a parent's bounded lifetime MUST NOT silently extend the parent's effective Deadline unless explicitly permitted by contract.

Detached Operation lifetime MUST be explicit.

Internal retries MUST NOT produce multiple terminal completions for the same logical Operation.

Internal retries MUST respect applicable cancellation and deadline constraints unless the public contract explicitly defines otherwise.

Failure or Cancelled completion MUST NOT be assumed to imply absence of partial progress or side effects unless the contract provides such a guarantee.

An Operation that transfers ownership of a Resource on completion MUST define that ownership relationship clearly.

Implementations SHOULD avoid exposing concrete execution providers in Operation APIs when only a semantic execution capability is required.

Operation contracts SHOULD make significant cancellation, deadline, ownership, and side-effect behavior understandable to callers.

## Design consequence

An Operation should answer a simple question:

> What finite piece of work is being attempted, and what does it mean for that work to be finished?

The answer should remain understandable even when the implementation changes from synchronous to asynchronous execution, replaces its scheduler, moves to another operating system, changes providers, introduces retries, or decomposes the work into additional child Operations.

The implementation is allowed to become more sophisticated.

The logical contract should remain coherent.

That separation between finite intent and execution mechanism is what allows Operations to serve as one of the central concepts of the Vix Standard.
