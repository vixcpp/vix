# Execution Model

Execution describes how work progresses toward the completion of an Operation or the continued behavior of an Activity.

The Vix Standard deliberately separates execution from the work being executed.

An Operation expresses intent. It describes what should be accomplished and what completion means. Execution describes the mechanism that allows that work to progress.

This distinction matters because the same Operation may be executed in very different ways on different platforms or through different providers without changing its semantic meaning.

A hostname lookup may be performed through an operating-system resolver, a worker thread, an asynchronous runtime, or another provider. A file read may block on the calling thread on one platform and use asynchronous I/O on another. A CPU-intensive transformation may execute inline for small inputs and use a worker pool for larger ones.

These implementation choices affect execution, but they should not redefine the Operation itself.

## Execution is not Operation

An Operation is a semantic unit of finite work.

Execution is the progression of that work.

The two concepts should not be collapsed.

For example:

```text id="5u7ps1"
Operation
    compress buffer

Possible execution
    caller thread
    worker thread
    thread pool
    hardware accelerator
```

or:

```text id="rxwudn"
Operation
    read from socket

Possible execution
    blocking system call
    event loop
    operating-system asynchronous I/O
    coroutine runtime
```

The implementation may change while the Operation contract remains stable.

An API should therefore avoid exposing a concrete execution mechanism unless that mechanism is genuinely part of its contract.

## Execution capability

Some work requires an execution capability.

An execution capability describes the ability to make a particular class of work progress.

Different forms of work may require different execution capabilities.

For example:

```text id="hv76em"
CPU execution
asynchronous I/O progression
timer progression
event-loop progression
blocking-work execution
```

The Standard does not assume that all execution can or should be represented through one universal executor abstraction.

A platform may provide several execution capabilities through different mechanisms.

An Operation may require one or more of these capabilities without depending on the concrete Provider that satisfies them.

## Execution provider

A Provider may satisfy an execution capability.

Examples may include:

```text id="0cv20g"
caller thread
worker pool
event loop
coroutine scheduler
operating-system I/O service
GPU command queue
```

These are implementation mechanisms.

The Standard does not require one Provider to satisfy every execution need.

A CPU worker pool does not automatically satisfy asynchronous network I/O semantics.

An event loop does not automatically make blocking CPU work appropriate to execute directly within that loop.

The execution model should reflect the actual kind of work rather than forcing all work through one abstraction.

## Execution Context

An Execution Context is an environment in which work can progress.

It may provide queues, event processing, worker threads, completion dispatch, timer integration, or another execution facility.

An Execution Context may itself be a Resource with a meaningful lifetime.

Examples may include an event loop, an I/O context, a worker pool, or another stateful execution environment.

The Standard does not require a universal `ExecutionContext` base class.

The term describes a semantic role.

An implementation should not turn the Execution Context concept into a general-purpose container for unrelated services.

## Execution Context lifetime

When an Execution Context is a Resource, normal ownership rules apply.

A component may own an Execution Context.

Another component may borrow it.

Borrowing an Execution Context allows the borrower to use the execution capability permitted by the contract.

It does not automatically grant authority to start, stop, drain, reset, or destroy the Execution Context.

For example:

```text id="2kn6oa"
Application
    owns Execution Context

Server
    borrows Execution Context

Request Operations
    use Execution Context
```

Stopping the Server does not imply that the Server may stop the broader Execution Context.

Execution use and lifecycle authority remain separate.

## Execution ownership

Execution ownership describes lifecycle authority over an execution Resource.

It should not be confused with ownership of an Operation.

A caller may own the lifetime of an Operation while that Operation executes on a borrowed execution facility.

For example:

```text id="c4dj2k"
Caller
    owns Operation lifetime

Application Runtime
    owns Worker Pool

Operation
    borrows CPU execution capability
```

Cancelling or completing the Operation does not terminate the Worker Pool.

This separation allows many Operations to share common execution infrastructure safely.

## Execution and scheduling

Execution and Scheduling are related but distinct.

Execution describes work progressing.

Scheduling determines when and where executable work is allowed to progress.

Scheduling may involve:

```text id="70gyvo"
queueing
priority
worker selection
fairness
affinity
deadline awareness
resource limits
ordering
```

An execution Provider may also perform scheduling, but the concepts should remain separate.

An Operation should not automatically depend on scheduling features it does not need.

For example, an Operation that only needs asynchronous I/O should not be forced to depend on CPU job priority, worker metrics, or unrelated scheduling policy.

## Scheduling policy

Scheduling policy may vary between Providers.

A thread pool may use work stealing.

An event loop may process events in queue order.

A realtime environment may use fixed-priority scheduling.

A browser may rely on host event-loop behavior.

These differences do not necessarily change Operation semantics.

Where scheduling behavior affects the public contract, the Provider or profile must define the relevant guarantees.

Where it does not, the implementation should remain free to change scheduling strategy.

## Submission

Some execution Providers accept work through submission.

Conceptually:

```text id="edkl94"
submit work
    ↓
Provider accepts or rejects
    ↓
work eventually progresses
```

Submission is not itself equivalent to Operation completion.

Accepted work may still fail later.

Rejected submission may indicate that the execution Resource is shutting down, capacity is exhausted, the request is invalid, or another provider-specific condition exists.

If submission failure is observable and relevant to callers, the contract must define how that failure is represented.

The Standard does not require every execution mechanism to expose explicit submission.

An event-driven I/O Provider may instead associate Operations directly with the underlying system.

## Fire-and-forget execution

Some work may be submitted without a caller waiting directly for a returned value.

This is often called fire-and-forget execution.

The Standard treats this carefully because absence of a returned value does not remove lifetime, Error, or ownership responsibilities.

Fire-and-forget work still needs an identifiable lifetime.

If it is detached from the initiating lifetime, that detachment must be explicit.

The implementation must define how uncaught Failure or Fault is observed when no direct caller awaits completion.

Fire-and-forget must not become a mechanism for silently losing errors or creating uncontrolled background lifetime.

## Inline execution

An execution Provider may execute submitted work immediately on the calling thread.

This is valid when permitted by the contract.

However, inline execution can have observable consequences such as reentrancy, lock ordering, stack growth, or unexpected execution of user callbacks before a function returns.

If an API promises deferred execution, a Provider must not violate that promise by executing inline.

If inline execution is allowed, callers should not assume that submission always creates an asynchronous boundary.

The distinction belongs to the applicable execution contract.

## Deferred execution

Deferred execution means work will not begin inline within the initiating call according to the relevant contract.

The work may instead be queued, posted to an event loop, scheduled on another thread, or progressed through another mechanism.

The Standard does not require deferred execution by default.

APIs should define the behavior when the distinction between inline and deferred execution affects correctness.

## Blocking work

Blocking work occupies the thread or execution resource that performs it until the blocking action completes.

Examples may include:

```text id="f2xyx0"
blocking filesystem call
foreign-library call
CPU-heavy computation
blocking process wait
```

Blocking work should not be executed on a context whose responsiveness depends on prompt event processing unless the Provider explicitly supports that usage.

For example, long blocking work on a single-threaded I/O event loop can prevent unrelated network, timer, or cancellation progress.

Vix should distinguish blocking-work execution from asynchronous I/O progression rather than assuming one execution mechanism is appropriate for both.

## CPU-bound work

CPU-bound Operations primarily consume processor time.

They may be executed:

```text id="j0znsd"
inline
on a dedicated worker
on a thread pool
on specialized hardware
```

The appropriate execution strategy depends on workload, platform, and contract.

The Standard does not require all CPU work to use a thread pool.

It does require that an API avoid presenting a specific CPU executor as a fundamental dependency when the Operation only requires the semantic ability to perform CPU work.

## I/O-bound work

I/O-bound Operations often spend time waiting for external systems, devices, files, sockets, or other Resources.

An asynchronous I/O Provider may allow many such Operations to progress without dedicating one blocked thread to each Operation.

The Standard does not prescribe a specific asynchronous I/O implementation.

Providers may use:

```text id="uy1a6p"
epoll
io_uring
IOCP
kqueue
event loops
host runtime APIs
other platform mechanisms
```

The shared Operation semantics should remain stable across those implementations.

## Coroutine execution

Coroutines are one possible representation for asynchronous Operations.

They may make sequential control flow easier to express while relying on one or more execution Providers underneath.

The Standard does not define Operation as synonymous with coroutine.

Not every Operation needs to be a coroutine.

Not every coroutine necessarily corresponds to one public logical Operation.

A coroutine implementation detail should not become a domain-level semantic dependency unless the API explicitly exposes coroutine behavior as part of its contract.

## Threads

Threads are an execution mechanism.

They are not the foundational unit of work in the Vix Standard.

An Operation may use no additional threads, one dedicated thread, or many shared workers.

A Provider may also use threads internally without exposing them to callers.

The meaning of an Operation should not depend unnecessarily on the number of threads used to execute it.

Platform profiles may differ in whether native threading is available.

## Worker pools

A worker pool is a common Provider of CPU or blocking-work execution.

A worker pool is often a Resource with its own lifetime.

Operations may borrow the pool's execution capability.

The pool may expose additional scheduling policies such as priority, deadlines, grouping, or metrics.

Those features belong to the Provider contract.

They should not automatically become dependencies of every Operation that merely needs CPU execution.

## Event loops

An event loop is a common Provider for event-driven and asynchronous I/O progression.

An event loop may also coordinate timers, callbacks, and completion dispatch.

The event loop normally has a lifetime broader than any one Operation.

Operations using it usually borrow the execution Resource.

Completing one Operation does not terminate the event loop.

Shutting down one Activity does not automatically authorize that Activity to terminate a shared event loop.

## Execution progress

An active asynchronous Operation may require an execution Provider to continue being progressed.

For example, an event loop may need to run for socket completion callbacks or timers to execute.

If the required Provider stops progressing work, Operations may remain incomplete even though their own contracts remain otherwise valid.

The relationship between Provider lifetime and active Operation lifetime must therefore be understood.

An owner must not stop an execution Provider while dependent Operations are still expected to progress unless the shutdown contract defines how those Operations terminate.

## Execution shutdown

Execution Resources may support shutdown.

Shutdown may:

```text id="4hy60z"
stop accepting new work
allow queued work to drain
reject pending work
request cancellation
finish current callbacks
terminate workers
```

The exact semantics depend on the Provider.

The Standard does not define one universal executor shutdown model.

It does require that Provider shutdown have defined consequences for work that depends on that Provider.

Stopping an execution Resource must not leave dependent Operations in an undefined semantic state.

## Graceful execution shutdown

Some Providers may support graceful shutdown.

A graceful shutdown may stop accepting new work while allowing already accepted work to complete.

The meaning of “accepted work” and “complete” depends on the Provider contract.

A thread pool may drain its queue.

An event loop may continue until registered Operations have completed.

A Provider may also impose a shutdown Deadline.

The Standard does not require graceful shutdown support from every Provider.

Where supported, the guarantees must be clear.

## Immediate execution shutdown

A Provider may support a more immediate shutdown policy.

This may reject queued work, request cancellation of active Operations, or stop processing as soon as safely possible.

Immediate shutdown does not automatically mean that every Operation has already reached terminal completion.

Dependent Operations still require coherent outcomes.

The Provider and Operation contracts must define how shutdown is translated into cancellation, Failure, rejection, or another outcome.

## Submission after shutdown

An execution Resource that is no longer accepting work may reject new submissions.

If this condition is observable, the API should represent it structurally rather than relying on undefined behavior or silent work loss.

The rejection may be an operational Failure at the submission boundary.

It should not automatically be classified as a Fault if callers are legitimately allowed to race with shutdown according to the contract.

If submission after shutdown violates an explicit precondition, it may instead be a Fault.

The contract boundary determines the classification.

## Waiting for idle

Some execution Providers may expose a concept such as waiting until currently known work is idle.

Idle does not necessarily mean that the Provider will never receive new work.

It may only mean that no current work remains at the moment the condition is observed.

The Standard does not define a universal `wait_idle()` semantic.

Providers that expose such functionality must define precisely what it guarantees.

Domain APIs should not depend on Provider-specific idle semantics unless they genuinely require them.

## Metrics

Execution Providers may expose metrics such as:

```text id="x8tqzn"
queue depth
worker utilization
completed jobs
rejected work
latency
```

These are observability features.

They are not part of the foundational meaning of execution.

An Operation that only needs execution should not be forced to depend on metrics APIs.

The presence or absence of Provider metrics should not change Operation semantics.

## Priority

A Provider may support execution priority.

Priority is a scheduling policy.

It does not change the semantic identity of the Operation.

Providers may differ in what priority guarantees actually mean.

For example, a priority may influence queue ordering without guaranteeing a strict maximum latency.

If callers depend on priority behavior, the Provider contract must define the observable guarantee.

The Standard does not require universal priority semantics across all execution Providers.

## Affinity

Some Providers may support execution affinity to a thread, worker, processor, event loop, or another execution location.

Affinity is an execution or scheduling constraint.

It is not a foundational property of every Operation.

An API should expose affinity only when required by the domain or underlying Resource contract.

For example, some Resources may only be safely accessed from one event-loop thread.

Such constraints should be explicit where they affect correctness.

## Reentrancy

Execution policy may affect whether callbacks or continuations can run reentrantly.

For example, inline execution may invoke work before the initiating API returns.

Deferred execution may avoid that particular form of reentrancy.

Reentrancy can affect locking, state machines, ownership transitions, and user callbacks.

Where an execution API permits or forbids reentrant execution, the contract should make that behavior clear.

The Standard does not impose one universal reentrancy policy.

## Continuations

An asynchronous Operation may schedule a continuation after an internal step completes.

Continuations are implementation mechanisms for progressing the logical Operation.

They do not create a new logical Operation automatically.

A continuation may itself initiate child Operations when the composition contract requires them.

The distinction depends on semantic lifetime rather than the fact that another function or coroutine frame is executed.

## Operation migration

An Operation may begin execution in one place and continue elsewhere.

For example:

```text id="6ur7yb"
start on caller thread
    ↓
submit CPU work to worker pool
    ↓
resume on event loop
```

This does not create several logical Operations merely because execution moved between Providers.

The Operation remains one logical unit of work until its terminal completion.

The implementation must preserve Resource lifetime, cancellation, deadline, and completion semantics across these transitions.

## Cross-provider composition

A higher-level Operation may use more than one execution Provider.

For example:

```text id="c9t0gu"
HTTP Operation
    asynchronous network I/O provider
    CPU execution provider for compression
    timer provider for deadline observation
```

The Standard allows this.

No single Provider needs to own the entire logical Operation.

The Operation contract remains the common semantic boundary.

This is another reason the Standard should not equate Operation with Executor.

## Execution and cancellation

Cancellation may require cooperation from the execution Provider.

A Provider may support:

```text id="hzyjdv"
removing queued work
interrupting pending I/O
marking future work unnecessary
stopping before next phase
```

Other Providers may have weaker cancellation support.

These differences affect cancellation responsiveness.

They do not change the semantic rule that cancellation is a request rather than terminal completion.

An Operation must still eventually expose one coherent terminal outcome.

## Execution and deadlines

Execution Providers may help enforce Deadlines through timers, deadline-aware queues, or operating-system facilities.

A Provider may also be unaware of the Deadline while higher-level code observes time and requests cancellation.

Both designs are possible.

The Standard does not require the execution Provider itself to understand every Operation Deadline.

It requires the overall Operation contract to preserve the Deadline semantics defined by the Standard.

## Execution and Resources

Execution often uses stateful Resources such as:

```text id="nsdxif"
worker pool
event loop
I/O context
scheduler
GPU queue
```

These Resources have their own ownership and lifecycle.

Operations that use them normally borrow them.

The execution relationship must not erase Resource ownership boundaries.

For example:

```text id="nm2plw"
Application
    owns EventLoop

Server Activity
    borrows EventLoop

Request Operation
    executes through EventLoop
```

Neither the Request Operation nor the Server Activity automatically owns the EventLoop lifecycle.

## Provider-local state

An execution Provider may maintain state that is invisible to the Operation.

Examples include:

```text id="0s2c74"
worker queues
completion queues
thread-local state
scheduler metadata
timer wheels
platform handles
```

These are implementation details unless exposed through the Provider contract.

The Standard should not make Operation semantics depend on such details without a clear reason.

This allows Providers to evolve internally.

## Execution rejection

A Provider may be unable or unwilling to accept new work.

Reasons may include:

```text id="03g8ck"
shutdown in progress
capacity limits
invalid request
unsupported scheduling requirement
missing Resource
```

If the rejection is a valid operational possibility, it should be observable as structured Failure or another defined outcome at the appropriate boundary.

If the caller violates a strict precondition, the condition may instead be a Fault.

Rejected execution must not silently discard work when the API contract promises acceptance or observable rejection.

## Capacity

Execution Providers may have finite capacity.

A queue may fill.

A worker pool may impose a work limit.

A system may limit pending I/O.

The Standard does not require one universal overload strategy.

A Provider may:

```text id="q6fvo0"
block submission
reject work
apply backpressure
shed work
use bounded queues
use unbounded queues
```

according to its contract.

The important requirement is that capacity behavior must not be accidentally hidden when it affects correctness or Resource safety.

## Execution fairness

Some Providers may offer fairness guarantees.

Others may prioritize throughput or latency.

The Standard does not define universal fairness.

A conforming execution capability does not automatically imply that every submitted Operation will receive equal scheduling opportunity.

If starvation freedom, fairness, or latency bounds are important, they belong to the Provider or specialized profile contract.

## Execution and platform profiles

Different profiles may guarantee different execution capabilities.

A native profile may provide native threads and asynchronous I/O.

A WebAssembly profile may rely on host event-loop execution and may not expose unrestricted native threading.

An embedded profile may use cooperative polling or static execution infrastructure.

These differences are valid.

The shared concepts remain:

```text id="fd2t7b"
Operation
Execution
Scheduling
Provider
Resource
Ownership
Completion
```

Profiles vary in available capabilities and guarantees, not in the meaning of those concepts.

## No universal executor requirement

The Vix Standard does not require every subsystem to depend on a universal Executor object.

This is intentional.

A networking subsystem may need asynchronous I/O progression.

A CPU-intensive subsystem may need worker execution.

A timer facility may need clock and timer progression.

A synchronous parser may need no explicit execution Provider at all.

Forcing all of these through one interface can create dependencies on lifecycle, metrics, scheduling, priority, threading, or other semantics that the consumer does not need.

Vix should prefer the smallest meaningful execution capability required by the contract.

## No universal runtime context requirement

The execution model also does not imply that Vix applications should receive one large runtime object containing every facility.

A universal context object can easily become a service locator that hides dependencies and makes lifetime authority unclear.

Dependencies should remain understandable.

Where several capabilities genuinely share one runtime Resource, an implementation may expose an appropriate object.

The Standard does not prohibit such designs.

It simply does not make them a fundamental requirement of the programming model.

## Normative requirements

Execution MUST remain semantically distinct from the Operation being executed.

Scheduling MUST remain conceptually distinct from Operation semantics and lifecycle authority.

Using an execution Resource MUST NOT automatically grant authority over that Resource's lifecycle.

A component that borrows an execution Resource MUST NOT stop, close, or destroy it unless lifecycle authority has explicitly been granted or transferred.

An Operation MUST NOT require a specific execution Provider when only a provider-independent execution capability is semantically required.

Execution Provider shutdown MUST have defined consequences for dependent accepted work.

Provider shutdown MUST NOT leave Operations in an undefined terminal state.

Execution rejection MUST NOT silently discard work when the applicable contract promises either acceptance or observable rejection.

An Operation moving between execution Providers MUST preserve its logical lifetime, Resource obligations, cancellation state, Deadline constraints, and single-completion semantics.

Internal execution steps and continuations MUST NOT create multiple terminal completions for one logical Operation.

Detached execution MUST NOT be used to bypass explicit lifetime ownership requirements.

Execution infrastructure MUST NOT be assumed to be owned merely because it is retained through shared memory ownership.

Providers MUST NOT claim scheduling, cancellation, timing, or execution guarantees stronger than they can preserve on the applicable platform.

APIs SHOULD depend on the smallest execution capability required by their contract.

Blocking work SHOULD NOT run on an execution context whose responsiveness depends on non-blocking progress unless the Provider explicitly supports that behavior.

Execution APIs SHOULD make relevant inline-versus-deferred behavior understandable where it affects correctness.

Execution Resources SHOULD define shutdown, rejection, and accepted-work behavior where those states are observable.

Provider-specific scheduling features SHOULD remain outside generic Operation contracts unless the Operation genuinely depends on them.

## Open representation questions

The Standard does not yet define one canonical C++ representation for:

```text id="ulz4pc"
Execution Context
CPU executor
I/O executor
scheduler
submission result
execution capability
```

It also does not require a common base interface between CPU execution, asynchronous I/O, timers, or other execution capabilities.

A future Vix Evolution Proposal may determine that some common representation is useful.

Such a proposal should begin from demonstrated semantic requirements rather than from a desire to unify existing implementation classes.

The current Standard intentionally leaves room for different Providers to expose appropriately different interfaces.

## Design consequence

Execution should answer:

> How does this work make progress?

Scheduling should answer:

> When and where is that work allowed to progress?

Ownership should answer:

> Who controls the lifetime of the execution Resource?

The Operation should continue to answer:

> What work is being attempted, and what does completion mean?

Keeping these questions separate allows Vix to support synchronous code, asynchronous I/O, CPU worker pools, event loops, native platforms, WebAssembly, embedded environments, and future execution models without forcing them into one historical abstraction.

The execution model is therefore intentionally plural.

Vix does not need one executor everywhere.

It needs one coherent meaning for execution everywhere.
