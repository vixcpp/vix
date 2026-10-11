# Execution Architecture

Execution in Vix is intentionally not organized around one universal Executor abstraction.

The Vix Standard separates the Operation being performed from the mechanism that allows that work to progress. The architecture follows the same rule.

Different forms of work have different execution requirements. Asynchronous network I/O, CPU-intensive computation, blocking foreign-library calls, timer progression, and synchronous code do not necessarily need the same Provider or the same lifecycle.

The execution architecture therefore begins by identifying the capability actually required by each consumer.

It does not begin by asking which global runtime object every subsystem should receive.

## Architectural objective

The execution architecture should make it possible to answer:

> What kind of execution does this component actually require?

> Which Provider supplies that capability?

> Who owns the Provider or execution Resource?

> Does the consumer merely use execution, or does it also control scheduling or lifecycle?

These questions should remain separate.

Historically, broad execution abstractions can make them difficult to distinguish.

A component may receive an Executor because another component already used that Executor, even though the new consumer only needs asynchronous I/O progression.

Another component may retain the same object only so that it can be passed farther down the call graph.

Eventually, implementation plumbing begins to look like an architectural dependency even when no real execution requirement exists.

The Vix3 direction is to reverse that process.

Execution dependencies should correspond to actual execution semantics.

## Execution capabilities

The Runtime currently needs to reason about several distinct forms of execution.

At a high level:

```text
Execution
│
├── synchronous / inline execution
│
├── asynchronous I/O progression
│
├── CPU execution
│
├── blocking-work execution
│
└── timer progression
```

These capabilities can interact.

They may even share Providers.

They should not automatically be treated as one capability.

The distinction exists because the constraints of each kind of work are different.

## Synchronous execution

Some Operations require no explicit execution infrastructure.

A pure parser, path transformation, small computation, or simple synchronous filesystem operation may execute directly on the calling thread.

Conceptually:

```text
Caller
    ↓
Operation executes inline
    ↓
Completion returned
```

No explicit Executor object is required.

This is important because the execution architecture should not force every Operation through runtime machinery simply for consistency of implementation shape.

The Standard provides semantic consistency.

The implementation does not need artificial mechanical uniformity.

## Asynchronous I/O progression

Asynchronous I/O progression is a foundational Runtime capability.

It allows Operations involving sockets, timers, and other asynchronous Resources to remain pending without dedicating one blocked thread to each Operation.

This capability is currently most naturally associated with Vix Async.

Conceptually:

```text
Async I/O Capability
    ↓
Async Provider / I/O Context
    ↓
OS or host event mechanism
```

Higher-level domains such as Web, WebSocket, realtime systems, and network clients may depend on this capability.

They should not need to understand the entire internal execution system simply because their I/O must progress asynchronously.

## Async Context as a Resource

The execution environment used for asynchronous I/O is a Resource when it has a meaningful lifecycle.

It may own or coordinate operating-system handles, event-loop state, queues, threads, timers, or completion delivery.

Normal ownership rules apply.

A common relationship is:

```text
Application / Orchestrator
    owns Async Context

HTTP Server
    borrows Async Context

WebSocket Server
    borrows Async Context

Realtime
    borrows Async Context
```

The borrowing components may use the Async Context to progress their Operations.

They do not gain authority to stop the shared context merely because they retain access to it.

This distinction should remain visible throughout the implementation.

## Async does not own application lifecycle

Async supplies execution capability.

It does not automatically own the lifetime of every Activity that uses it.

For example:

```text
Server Activity
    uses Async

WebSocket Activity
    uses Async
```

The Async Provider progresses work for these Activities.

The Activities still have their own owners, shutdown rules, Resources, and child Operations.

Likewise, terminating one Activity does not imply terminating the shared Async Provider.

This prevents execution infrastructure from becoming the hidden owner of the application.

## CPU execution

CPU execution represents the ability to progress computational work that primarily consumes processor time.

Examples include:

```text
compression
image transformation
cryptographic computation
large parsing work
game jobs
data processing
```

The current Vix Threadpool implementation is a natural Provider of CPU execution.

Conceptually:

```text
CPU Execution Capability
    ↓
Threadpool Provider
```

The architectural concept is CPU execution.

`Threadpool` is one implementation.

This distinction allows the Provider to evolve without turning the current pool implementation into permanent Platform semantics.

## Threadpool is not the universal Executor

The existence of a Threadpool Provider does not mean all Runtime work should pass through it.

Network I/O should not be moved onto a worker pool merely to satisfy a common Executor interface.

Likewise, simple synchronous work should not necessarily be queued onto workers.

Threadpool should provide the capability it is good at providing.

Higher-level components should consume that capability when they actually need CPU execution.

This keeps execution dependencies meaningful.

## Blocking-work execution

Blocking work is work that may occupy an executing thread while waiting for something that does not integrate with the asynchronous I/O Provider.

Examples may include:

```text
blocking system calls
legacy library calls
foreign library APIs
blocking filesystem operations on some platforms
blocking process waits
```

Such work may need to be offloaded so that an I/O event loop remains responsive.

The implementation may use the same Threadpool Provider for both CPU work and blocking work.

Architecturally, however, the needs are distinct.

A CPU workload is constrained mainly by processor availability.

A blocking workload may spend much of its lifetime waiting while still occupying a worker.

Those differences may eventually justify different scheduling or sizing policies.

For that reason, the architecture should not define:

```text
Threadpool
=
all non-async execution
```

as a permanent semantic rule.

## Shared Provider, separate capabilities

One Provider may satisfy several execution capabilities.

For example:

```text
Threadpool Provider
    provides CPU execution
    provides blocking-work execution
```

This is valid.

The capabilities remain separately understandable.

A future implementation may use:

```text
CPU execution
    → work-stealing pool

Blocking execution
    → dedicated blocking pool
```

without requiring higher-level APIs to change their meaning.

This is the architectural benefit of separating Capability from Provider.

## Timer progression

Timers combine Time semantics with execution.

The Time foundation defines:

```text
Duration
Clock
Instant
Deadline
```

A timer Provider is responsible for observing temporal conditions and arranging for waiting Operations to progress.

Conceptually:

```text
Time
    defines Deadline

Async / Timer Provider
    observes Deadline
    progresses waiting Operation
```

This means timers may be implemented close to Async while Time itself remains independent.

A Deadline must not become an Async-specific concept merely because asynchronous timers are a major consumer.

## Execution and Operation representation

The execution architecture does not yet require one universal C++ Operation type.

An asynchronous Operation may eventually be represented by something such as:

```cpp
task<T>
```

or another canonical form.

A synchronous Operation may simply be a normal function call.

An internal system request may be implemented as a callback-driven state machine.

All may still satisfy the Standard Operation semantics.

Architecture should therefore avoid requiring all execution Providers to consume one base `Operation` class.

The shared contract is semantic.

## Coroutine ownership

C++ coroutines may form a major part of Vix asynchronous execution.

A coroutine frame has a concrete lifetime that must remain consistent with the semantic lifetime of the work it represents.

The architecture must therefore ensure that coroutine-based work does not silently outlive Resources it borrows.

For example:

```text
Coroutine Operation
    borrows Session

Session destroyed
    while coroutine still active
```

is invalid regardless of whether the coroutine syntax appears safe.

Execution architecture must preserve the same Resource and ownership rules as every other implementation mechanism.

## Continuations

An asynchronous Operation may suspend and later resume through a continuation.

That continuation may run:

```text
on the same event loop
on another thread
on a CPU worker
through a completion callback
```

depending on the contract.

Moving execution between contexts does not create a new logical Operation automatically.

The implementation must preserve:

```text
Operation lifetime
Cancellation state
Deadline
borrowed Resources
Error context
single completion
```

across these transitions.

## Execution migration

A logical Operation may use several execution capabilities during its lifetime.

For example:

```text
HTTP Request Operation
    ↓
Async I/O: read compressed response
    ↓
CPU execution: decompress body
    ↓
Async I/O: continue protocol work
```

This remains one logical parent Operation.

The execution mechanism changed.

The Operation identity did not.

This pattern should be supported naturally by architecture without requiring one universal Executor to pretend that all phases are the same kind of work.

## Scheduling

Scheduling determines when and where work is allowed to progress.

Some Providers may expose scheduling behavior such as:

```text
priority
queue order
worker affinity
fairness
load balancing
work stealing
```

These are Provider concerns unless higher-level contracts genuinely require them.

The architecture should not make scheduling policy a dependency of every consumer of execution.

For example, WebSocket needs its I/O to progress.

It does not necessarily need to know the queue-depth metrics or work-stealing strategy of a CPU pool.

## Scheduling should remain local where possible

Provider-local scheduling policy should remain replaceable.

A Threadpool may move from one queue strategy to another.

An Async Provider may change internal completion dispatch.

Such changes should not force changes through unrelated domain APIs when the observable guarantees remain the same.

This is consistent with the Standard principle that implementation mechanisms should remain replaceable beneath stable semantics.

## Execution affinity

Some Resources may have execution-affinity constraints.

For example, a Resource may only be safely accessed from one event-loop thread or one Provider context.

If such a constraint affects correctness, it must be respected by Operations using that Resource.

Conceptually:

```text
Resource
    bound to Execution Context A

Operation
    borrows Resource
    therefore must preserve required affinity
```

This is not a general argument for making every Resource thread-confined.

Affinity should exist only where the Provider or Resource contract genuinely requires it.

## Reentrancy

Execution Providers may differ in whether submitted or completed work can run inline.

This can affect:

```text
locks
state transitions
callback ordering
destruction
user code
```

The architecture should avoid assumptions such as:

```text
submit()
always means
run later
```

unless the Provider contract guarantees that behavior.

Where inline versus deferred execution matters to a public API, the behavior should eventually be specified explicitly.

## Execution and Cancellation

Cancellation authority comes from Operation or Activity lifetime.

Execution Providers help implement cancellation but should not normally own cancellation policy.

For example:

```text
Request Operation owner
    decides Request is no longer needed
        ↓
Cancellation Request
        ↓
Async Provider
    attempts to cancel pending I/O
```

The Provider is responsible for carrying out the request according to its capabilities.

It did not decide that the Request should be cancelled.

Keeping control policy outside the Provider prevents execution infrastructure from taking over higher-level lifecycle decisions.

## Provider cancellation strength

Different execution Providers may offer different cancellation behavior.

One may support cancellation of queued CPU work before it begins.

Another may support operating-system cancellation of pending I/O.

Another may only allow an Operation to observe cancellation after a blocking call returns.

These are legitimate Provider differences.

The higher-level Operation contract must not promise stronger cancellation behavior than the selected Provider can satisfy.

Profiles may eventually guarantee minimum cancellation properties where needed.

## Execution and Deadlines

Execution Providers may participate in Deadline enforcement.

For example, Async may schedule a timer that triggers cancellation of an I/O Operation.

A CPU scheduler might eventually support deadline-aware scheduling.

These mechanisms do not own the semantic Deadline.

Time remains the canonical owner of Deadline representation and meaning.

The relationship is:

```text
Operation contract
    has Deadline
        ↓
Execution / Timer Provider
    helps enforce Deadline
```

Providers must not silently reset inherited Deadlines simply because their local API is expressed as a timeout Duration.

## Execution and Resource ownership

Execution Resources must have explicit owners.

For example:

```text
Application
    owns Async Context
    owns CPU Pool

Server
    borrows Async Context

Game subsystem
    borrows CPU Pool
```

This architecture avoids a common failure mode:

```text
component retains shared_ptr
    ↓
component assumes ownership
    ↓
component stops shared Provider
    ↓
unrelated components lose execution
```

A retained reference preserves memory lifetime.

It does not automatically grant operational shutdown authority.

## Component-owned execution

Some components may legitimately own private execution Resources.

For example:

```text
Component
    creates dedicated Worker Pool
    owns Worker Pool
```

In this case, the component is responsible for shutting down that Provider according to its contract.

The important distinction is not that shared Providers are always better.

It is that ownership must match the actual lifecycle.

Private execution is valid when isolation or domain requirements justify it.

## Orchestration-owned execution

Shared execution Resources are often best owned by an orchestration layer.

For example:

```text
Application Orchestrator
├── owns Async Context
├── owns CPU Provider
├── creates HTTP Server
├── creates WebSocket Server
└── coordinates shutdown
```

The individual servers borrow the execution facilities.

This gives the application one place to coordinate global shutdown without requiring every subsystem to understand every other subsystem using the same Provider.

## Execution startup

Stateful execution Providers may require startup.

For example:

```text
create Async Context
start event processing

create Worker Pool
start workers
```

The owner is responsible for this lifecycle unless authority is explicitly delegated.

A borrower should not silently start a Provider simply because it discovers that the Provider is idle, unless the Provider contract specifically defines that behavior.

Start authority and use authority are different.

## Execution shutdown

Shutdown must be coordinated with Operations and Activities that still depend on the Provider.

The owner should not simply destroy the execution Resource while dependent work remains active.

A shutdown process may look like:

```text
stop accepting new domain work
    ↓
allow or cancel active Operations
    ↓
terminate dependent Activities
    ↓
stop execution Provider
    ↓
release Provider Resource
```

The exact ordering depends on the application.

The architectural principle is that infrastructure should generally outlive the dependent work that needs it to terminate cleanly.

## Provider shutdown semantics

Execution Providers should define how they behave when shutting down.

Possible behavior includes:

```text
reject new work
drain accepted work
cancel queued work
allow running work to finish
request cancellation
stop immediately where safe
```

These choices may vary between Providers.

The Provider contract must make the behavior understandable.

Dependent components should not infer it from implementation details.

## Execution rejection

An execution Provider may reject new work.

For example, a worker pool may be shutting down or a bounded queue may be full.

If rejection is a valid operational possibility, the caller must receive an observable outcome.

Work should not disappear silently.

The exact representation may differ between execution APIs.

The semantic requirement is that accepted and rejected work remain distinguishable when correctness depends on the distinction.

## Backpressure

Execution Providers may have finite capacity.

Queueing work without limit can transform temporary overload into memory exhaustion.

The architecture should allow Providers to expose bounded capacity or backpressure behavior where appropriate.

This does not require one universal queue policy.

Possible Provider strategies include:

```text
reject submission
block or suspend producer
limit concurrency
shed optional work
```

Higher-level components must understand the behavior they depend on.

Backpressure policy should not be accidentally inherited from whichever queue implementation happens to be used internally.

## Detached execution

Submitting work without a direct awaiter does not eliminate lifetime responsibility.

Detached work must have an owner.

For example:

```text
Request Operation
    submits Job
        ↓ ownership transfer
Application Background Activity
    owns Job
```

This is legitimate.

By contrast:

```text
Request
    launches coroutine
    returns
    coroutine still borrows Request state
```

is not legitimate merely because the execution Provider continues running.

The execution architecture should make explicit lifetime transfer possible while making accidental detachment difficult.

## Fire-and-forget

Fire-and-forget should therefore be treated as an explicit lifecycle design rather than a convenience execution primitive.

A fire-and-forget facility needs a policy for:

```text
lifetime
Failure observation
Fault handling
shutdown
Resource ownership
```

If no one can answer who owns the work after submission, the architecture is incomplete.

Vix should avoid using fire-and-forget as a generic escape hatch from structured lifetime.

## Failure in execution infrastructure

Execution Providers can encounter operational Failure.

Examples may include:

```text
worker creation failure
submission rejection
platform I/O setup failure
timer facility unavailable
Provider shutdown
```

These Failures should be represented according to the relevant Provider or Operation contract.

They should not automatically be translated into generic domain Errors that hide the execution cause.

Likewise, an internal invariant violation in the scheduler or queue is a Fault, not merely an operational Failure because it happened inside runtime infrastructure.

## Exceptions crossing execution boundaries

Execution boundaries are common places for C++ exceptions to become difficult.

User callbacks, third-party code, allocation, or internal logic may throw while running through a Provider.

The architecture should eventually define clear policies for where exceptions may propagate and where they must be captured.

Until that policy is standardized, Providers should avoid silently swallowing unexpected exceptions or converting every exception into the same operational Error.

The Failure/Fault distinction must remain preserved.

## Async and CPU execution composition

Async code may need CPU offload.

For example:

```text
Async Operation
    receives data
    ↓
CPU-heavy transformation
    ↓
resume Async Operation
```

The architecture should support this through composition of capabilities.

Conceptually:

```text
Async
    requires CPU Execution for specific work

not

Async owns universal Threadpool
```

This distinction keeps the CPU Provider independently configurable and reusable.

## Async and blocking execution composition

The same principle applies to blocking calls.

An asynchronous Operation may need to invoke a blocking external library.

It may offload that step through blocking-work execution.

The entire Async system should not become coupled permanently to one worker pool because one Operation occasionally needs this capability.

The dependency should exist at the point where the capability is required.

## Current Threadpool role

The current `threadpool` area should be treated architecturally as a Provider of execution capabilities rather than as the universal definition of execution.

Its existing implementation can remain valuable.

The architectural questions are:

```text
Which capabilities does Threadpool currently satisfy?

Who owns Threadpool Resources?

Which consumers genuinely require it?

Which consumers merely inherited a dependency historically?
```

The answers should guide migration.

The module name itself should not decide the architecture.

## Current Async role

The current Async area is the canonical direction for asynchronous I/O and coroutine-oriented execution.

Networking and other I/O-heavy domains should increasingly rely on Async where Async already provides the required capability.

This avoids retaining historical RuntimeExecutor dependencies solely to preserve constructor shapes or object graphs.

Async itself should remain focused on asynchronous execution responsibilities and avoid absorbing unrelated CPU scheduling, domain behavior, or application lifecycle.

## Historical Runtime

The historical Core Runtime represents an earlier execution model centered around callable tasks and work-stealing runtime behavior.

That implementation may still serve useful historical or compatibility needs.

It should not automatically remain a foundational dependency simply because higher-level classes already receive it.

The architectural migration question is:

> What execution capability does each current consumer actually use from this Runtime?

Possible answers may include:

```text
none
CPU execution
task submission
lifecycle coordination
historical compatibility only
```

Each answer may require a different migration.

There should not be a presumption that every removed Runtime dependency needs a new replacement object.

## Historical RuntimeExecutor

`RuntimeExecutor` is an adapter around the historical Core Runtime.

Its architectural role must therefore be evaluated through actual behavior rather than its name.

A consumer that calls meaningful execution methods may still need an execution capability.

A consumer that merely stores and forwards `RuntimeExecutor` does not have an execution requirement simply because the constructor accepts it.

This distinction has already proven useful in the current Web and WebSocket migration.

## Current WebSocket evidence

The WebSocket execution path provides a concrete architectural example.

Historically, execution dependencies flowed roughly through:

```text
WebSocket App
    ↓ RuntimeExecutor
Server
    ↓ RuntimeExecutor
LowLevelServer
    ↓ RuntimeExecutor
Session
```

Audit showed that the lower levels did not actually use RuntimeExecutor to progress WebSocket work.

The real execution mechanism was Async.

As a result, RuntimeExecutor dependencies could be removed from Session and LowLevelServer without inventing a replacement execution abstraction.

The architecture became closer to the real behavior:

```text
WebSocket
    ↓
Async I/O
```

This is exactly the migration model this document recommends.

## Server and App remain orchestration questions

Higher-level WebSocket `Server` and `App` may still retain RuntimeExecutor-related responsibilities during migration.

These dependencies should be examined separately.

If a layer uses RuntimeExecutor only to construct lower-level objects that no longer require it, the dependency may become removable.

If the layer performs genuine orchestration or execution behavior, that responsibility should be identified explicitly before replacement.

The architecture should not decide the answer from type presence alone.

Behavioral audit comes first.

## HTTP execution

HTTP server execution should follow the same reasoning.

If actual request I/O progresses through Async, then HTTP should depend on Async for that capability.

A historical RuntimeExecutor may still exist at orchestration boundaries.

Each use should be audited for actual execution behavior.

The desired outcome is not necessarily:

```text
HTTP → NewExecutorInterface
```

The desired outcome is:

```text
HTTP → only the execution capabilities it actually needs
```

Sometimes that may be Async alone.

Sometimes another execution capability may also be justified.

## Middleware execution

Some middleware or background facilities may genuinely require generic task execution.

For example, a periodic task or independent CPU/background unit may submit callable work.

This is different from a Session that only performs asynchronous socket I/O.

Such consumers should not be forced into the same migration merely because both historically depended on `IExecutor`.

A real generic execution need should remain until a better canonical capability representation exists.

The architecture should preserve legitimate differences.

## Do not replace before understanding

A major principle of execution migration is:

```text
remove historical dependency
    does not imply
introduce new abstraction immediately
```

The correct sequence is:

```text
inspect usage
    ↓
classify actual capability
    ↓
remove unused dependency
    ↓
reuse existing canonical capability if sufficient
    ↓
only then consider a new abstraction
```

This prevents Vix3 from replacing old generalized execution infrastructure with new generalized infrastructure that solves no demonstrated problem.

## Future execution interfaces

Vix may eventually need canonical public interfaces for some execution capabilities.

Possible examples include:

```text
CPU execution
blocking execution
scheduling
structured concurrency
operation scopes
```

Those interfaces should emerge from repeated consumer requirements.

They should not be created merely because the Standard uses the word `Execution`.

A future VEP may be appropriate when the representation needs to become stable public API.

Until then, architecture should describe responsibility and allow implementation experimentation.

## Execution capability map

The current conceptual execution map is:

```text
Asynchronous I/O
    canonical direction:
        Async

CPU execution
    current Provider:
        Threadpool

Blocking-work execution
    current likely Provider:
        Threadpool
    long-term Provider boundary:
        still open

Timer progression
    semantic owner:
        Time for temporal primitives
    execution Provider:
        Async / timer mechanism

Historical callable-task runtime
    current implementation:
        Core Runtime
    architectural status:
        compatibility / migration subject,
        not assumed canonical
```

This map should evolve as audits provide more evidence.

## Dependency direction

A simplified intended dependency direction is:

```text
Ecosystem domain
    ↓
required execution capability
    ↓
canonical Provider
    ↓
OS / runtime mechanism
```

For example:

```text
WebSocket
    ↓
Async I/O
    ↓
Async Provider
    ↓
OS networking event mechanism
```

or:

```text
Game JobSystem
    ↓
CPU execution
    ↓
Threadpool Provider
```

The domain should not depend downward through unrelated historical aggregation layers when a direct canonical capability exists.

## Execution must not become a dependency inversion trap

Capability abstraction is useful only when it clarifies actual requirements.

The architecture should not create abstract interfaces solely so that every concrete implementation sits behind one virtual method table.

Sometimes the direct canonical implementation is already an appropriate dependency.

For example, if Async is itself the canonical Platform facility for asynchronous I/O, a domain may depend on Async directly without needing an additional `IAsyncExecutionProvider` abstraction.

Capability thinking does not imply maximal indirection.

It means the dependency should represent the real semantic need.

## Execution and Profiles

Profiles determine which execution capabilities are guaranteed.

A native environment may guarantee:

```text
native asynchronous I/O
native threads
CPU execution
```

A restricted environment may provide:

```text
host event-loop execution
timers
no native process execution
limited threading
```

The architecture should support these differences through Provider selection and capability guarantees.

Higher-level domains should not rely unnecessarily on one operating-system execution mechanism.

## Execution and portability

Provider-specific implementation is expected.

Async may use different mechanisms on Linux, Windows, macOS, WebAssembly, or another target.

Threadpool implementation may also differ.

The shared architectural role remains stable.

Portability should come from stable capabilities above changing mechanisms.

This does not mean every Provider must behave identically in performance or scheduling.

It means that differences must remain within the guarantees of the capability and profile being claimed.

## Execution observability

Providers may expose useful observability such as:

```text
active workers
queue depth
pending I/O
completion latency
rejected work
```

These facilities should remain optional Provider features unless higher-level contracts require them.

A domain should not need to depend on the entire Provider metrics interface merely to submit work.

Observability should explain execution without becoming the definition of execution.

## Execution testing

Execution tests should increasingly verify the semantic boundaries that matter.

Examples include:

```text
borrowed execution Resource is not stopped by borrower

cancellation race produces one completion

Provider shutdown gives accepted work defined outcomes

Operation moving between Async and CPU execution
preserves lifetime and Deadline

detached work does not retain dead parent state
```

Provider-specific tests may additionally verify scheduling algorithms, queues, thread management, or operating-system integration.

The two kinds of tests serve different purposes.

## Migration strategy

Execution migration should proceed component by component.

For each historical execution dependency:

```text
1. inspect actual method usage
2. determine whether execution is used at all
3. classify the required capability
4. identify current canonical Provider
5. preserve ownership semantics
6. remove forwarding-only dependencies
7. introduce a new abstraction only when existing capabilities are insufficient
8. validate behavior
```

This strategy is deliberately conservative.

The goal is not to replace all execution code at once.

The goal is to make every surviving dependency meaningful.

## Architectural requirements

Execution dependencies should reflect actual capabilities required by consumers.

Asynchronous I/O progression should have one canonical architectural direction through Async.

CPU execution should remain conceptually distinct from asynchronous I/O.

Blocking-work execution should remain distinguishable from CPU execution even when both use the same current Provider.

Time should own temporal semantics while timer Providers participate in execution.

Execution Resources must have explicit lifecycle owners.

Borrowers must not stop shared execution Resources.

Operation completion must remain independent from Provider lifetime.

Cancellation authority should come from Operation or Activity lifetime rather than from execution infrastructure.

Execution Providers must define observable shutdown and rejection behavior where relevant.

Moving an Operation between execution contexts must preserve its lifetime, Deadline, Cancellation, Resource, and Error semantics.

Detached execution must have explicit lifetime ownership.

Historical Runtime and RuntimeExecutor dependencies should be audited behaviorally rather than preserved because of existing constructor signatures.

Unused execution dependencies should be removed without mandatory replacement.

New universal Executor abstractions should not be introduced without demonstrated cross-domain need.

## Unresolved architecture questions

The final public representation of CPU execution remains open.

The long-term relationship between CPU execution and blocking-work execution also remains open.

Vix has not yet decided whether these capabilities eventually require common concepts, separate public interfaces, templates, concrete canonical Providers, or another representation.

Structured concurrency also remains an open implementation and API question.

The Standard already constrains its eventual semantics through explicit lifetime, cancellation propagation, and detachment rules.

The architecture should gather evidence from real Async, Web, process, filesystem, and background-work requirements before fixing the representation.

The eventual role of the historical callable-task Runtime must also be determined through remaining consumer audits.

It may retain a specialized purpose, become compatibility-only, be replaced by more precise capabilities, or disappear.

That decision should follow evidence rather than a desire for architectural symmetry.

## Design consequence

The execution architecture should make this dependency meaningful:

```text
Component
    requires
        a specific form of work progression
```

It should not turn into:

```text
Component
    requires
        whatever object Vix currently calls Executor
```

Async exists because asynchronous I/O has real execution requirements.

Threadpool exists because CPU and blocking work need execution resources.

Timers combine Time with progression.

Other execution mechanisms may appear in the future.

They do not need to look identical.

What must remain coherent is how they interact with Operation lifetime, Resource ownership, Cancellation, Deadlines, Completion, and Providers.

Vix does not need one Executor to rule every kind of work.

It needs execution dependencies that tell the truth about what the software actually requires.
