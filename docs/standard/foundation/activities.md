# Activities

An Activity represents continuing behavior that may remain active over time and may produce multiple events before terminating.

Activities are used for things such as servers, subscriptions, watchers, realtime channels, WebSocket connections, message consumers, long-running listeners, background services, and other behaviors that are not naturally described as one finite attempt with one result.

The defining property of an Activity is continuity.

An Operation represents finite work that reaches one terminal completion. An Activity may remain active, observe or produce many events, create many Operations, and continue after any one of those Operations has completed.

This distinction allows Vix to model long-lived systems without forcing them into an Operation model that does not fit their behavior.

## Continuing behavior

An Activity is expected to remain active until some termination condition occurs.

That condition may be explicit, such as shutdown, unsubscribe, disconnect, or stop. It may also be caused by Failure, loss of a required Resource, a platform event, or another condition defined by the Activity contract.

An Activity may exist for milliseconds or for the entire lifetime of an application.

Its duration does not determine whether it is an Activity.

What matters is that it represents ongoing behavior rather than one finite unit of work.

For example:

```text
HTTP Server
    accepts Request
    accepts Request
    accepts Request
    ...
```

The server does not complete after the first request.

The ongoing server behavior is therefore better modeled as an Activity.

Each request handled by that Activity may still be represented as an Operation.

## Activity and Operation

Activities and Operations are related but distinct.

An Activity may create Operations.

For example:

```text
Server Activity
    ├── Request Operation
    ├── Request Operation
    └── Request Operation
```

A WebSocket Activity may create send or receive Operations.

A filesystem watcher Activity may trigger processing Operations when changes are observed.

An Operation may also create or begin an Activity.

For example:

```text
connect()
    → Success(WebSocket Resource)

WebSocket Resource
    supports WebSocket Activity
```

Similarly:

```text
start_server()
    → Success(Server Resource)

Server Resource
    supports Server Activity
```

The Standard does not require these relationships to be represented by separate C++ objects.

One implementation object may participate in both Resource and Activity semantics.

The semantic distinction remains useful even when the representation is compact.

## Activity and Resource

Many Activities depend on one or more Resources.

A server Activity may depend on a listening socket, an execution environment, configuration, and routing state.

A WebSocket Activity may depend on a connected transport Resource.

A subscription Activity may depend on a remote connection and local state.

An Activity does not automatically own every Resource it uses.

Resource ownership remains governed by the applicable ownership contract.

For example:

```text
Application
    owns ExecutionContext

Server Activity
    borrows ExecutionContext
```

Stopping the Activity does not automatically authorize it to stop the borrowed ExecutionContext.

Activity termination and Resource lifetime remain separate concerns.

## Activity lifetime

An Activity has a semantic lifetime during which its continuing behavior is active.

The Activity may pass through implementation-specific states such as:

```text
created
starting
running
stopping
terminated
failed
```

The Standard does not require one universal Activity state machine.

What matters is that the Activity has a meaningful distinction between being active and no longer being active.

An Activity that has terminated must not continue producing normal activity events as though it were still active.

## Starting an Activity

An Activity may begin in different ways.

Some Activities may begin automatically when a Resource is created.

Others may require an explicit start Operation.

For example:

```text
create Server Resource
    ↓
start()
    ↓
Server Activity begins
```

Another API may combine these steps:

```text
serve()
    ↓
Activity begins
```

The Standard does not require one start pattern.

The contract should make it understandable when the Activity becomes active and which entity is responsible for beginning its lifetime.

Starting an Activity may itself be an Operation if startup can complete successfully or fail.

For example:

```text
start_server()
    → Success
    → Failure
```

The startup Operation and the resulting Activity are distinct concepts even if one API surface represents both.

## Events

An Activity may produce zero or more Events during its lifetime.

An Event is a discrete observation that does not by itself terminate the Activity.

Examples include:

```text
incoming connection
received message
filesystem change
subscription update
timer tick
state change
```

An Activity may continue after any one Event is processed.

The Standard does not require one universal Event type or dispatch mechanism.

Events may be represented through callbacks, streams, channels, queues, handlers, iterators, coroutine interfaces, or another mechanism.

The semantic role of the Event is independent of that representation.

## Event delivery

The contract of an Activity should define the observable behavior of its Events where ordering, reliability, buffering, duplication, or loss matter.

Some Activities may guarantee ordered delivery.

Others may permit reordering.

Some may buffer Events.

Others may apply backpressure, drop Events, coalesce them, or stop the Activity under overload.

These are domain-specific semantics and are not universally standardized by the foundational Activity model.

However, an Activity must not make such behavior unknowable when it is relevant to correctness.

Domain specifications should define these guarantees when they matter.

## Activity termination

An Activity eventually reaches one terminal termination for a particular lifetime.

Termination means that the continuing behavior represented by that Activity has ended.

Conceptually, termination may occur as:

```text
Normal termination
Failure
Controlled stop
```

The exact representation remains an API concern.

The important distinction is that termination is not the same thing as an intermediate Event or a request to stop.

Once terminal termination has been observed, that Activity lifetime is finished.

An implementation MUST NOT report multiple terminal terminations for the same Activity lifetime.

## Normal termination

Some Activities can end naturally.

Examples may include:

```text
remote peer performs an orderly WebSocket close
finite subscription reaches its defined end
event source reports permanent end-of-stream
worker activity completes its configured lifecycle
```

These conditions are not necessarily Failures.

The Activity contract determines what constitutes normal termination.

Normal termination means that the Activity ended in a manner allowed by its contract.

## Controlled stop

An Activity may be terminated intentionally through lifecycle control.

Examples include:

```text
server shutdown
unsubscribe
stop watcher
close channel
application shutdown
```

A stop request is not itself termination.

The Activity may need to stop accepting new work, allow existing Operations to drain, request cancellation, flush state, close owned Resources, or perform other lifecycle work before termination becomes complete.

The distinction mirrors the relationship between cancellation request and Operation completion.

```text
stop requested
    ≠
Activity terminated
```

## Failure

An Activity may terminate because an operational condition prevents it from continuing according to its contract.

For example:

```text
listener permanently fails
required Resource becomes unavailable
subscription transport is irrecoverably lost
watcher cannot continue
```

Such termination may be represented as an Activity Failure.

A transient failure inside one child Operation does not necessarily terminate the Activity.

The Activity contract determines which Failures are local to child Operations and which Failures are fatal to the continuing Activity itself.

For example, one HTTP request may fail while the server continues normally.

By contrast, permanent loss of the listening Resource may terminate the server Activity.

## Faults during Activities

A programming Fault remains distinct from Activity Failure.

An impossible internal state, broken invariant, invalid lifecycle transition, or other programming defect should not automatically become an ordinary Activity Failure.

The implementation may diagnose, throw, terminate, or otherwise react according to the applicable Fault policy.

The semantic distinction between operational Failure and programming Fault must remain preserved.

## Shutdown

Shutdown is a lifecycle process that moves an Activity toward termination.

A shutdown sequence may involve several phases.

For example:

```text
shutdown requested
    ↓
stop accepting new work
    ↓
drain or cancel active Operations
    ↓
release or close owned Resources
    ↓
Activity terminates
```

Not every Activity requires all of these phases.

The contract defines the appropriate shutdown behavior.

The Standard does not require one universal `shutdown()` method or state machine.

It requires that shutdown semantics remain understandable and consistent with ownership and Operation lifetime rules.

## Graceful shutdown

Some Activities support graceful shutdown.

Graceful shutdown generally means that the Activity stops beginning new work while allowing some existing work to reach a defined completion before final termination.

For example:

```text
HTTP Server Activity
    stop accepting new requests
    allow active requests to complete
    terminate
```

The exact meaning of graceful shutdown is domain-specific.

The Standard does not assume that every active Operation must always be allowed to complete.

The contract may define deadlines, cancellation policies, or forced termination after a grace period.

If graceful shutdown is supported, the Activity contract should define what work is allowed to continue and when the Activity is considered terminated.

## Immediate shutdown

Some Activities may support a more immediate shutdown mode.

This may involve requesting cancellation of active Operations, closing owned Resources, or otherwise prioritizing termination over graceful completion.

Immediate shutdown does not guarantee that external side effects have not already occurred.

Likewise, it does not imply that every child Operation terminates synchronously with the shutdown request.

The contract must define what guarantees remain observable.

## Activity ownership

The entity that owns an Activity is responsible for its lifecycle according to the Activity contract.

Ownership of an Activity may include responsibility for starting it, requesting shutdown, waiting for termination, or coordinating dependent Resources.

Activity ownership does not automatically imply ownership of every Resource used by that Activity.

For example:

```text
Application
    owns Server Activity

Server Activity
    borrows shared ExecutionContext
```

The application may own the server lifecycle while a broader runtime layer owns the execution environment.

The distinction between Activity ownership and Resource ownership must remain explicit.

## Borrowed Resources

An Activity may borrow Resources for part or all of its lifetime.

A borrowed Resource must remain valid for as long as the Activity is permitted to use it.

The Activity MUST NOT invalidate a borrowed Resource merely because the Activity itself is terminating.

If the owner of a borrowed Resource ends that Resource lifetime, the Activity must respond according to its contract.

This may require stopping, failing, cancelling child Operations, releasing the borrow, or transitioning toward termination.

It does not grant the Activity authority to take over the Resource lifecycle.

## Owned Resources

An Activity may own Resources created specifically for its lifetime.

For example:

```text
Activity
    owns internal Listener
    owns private Timer
    owns internal Queue
```

When the Activity terminates, it is responsible for ending the lifetimes of Resources it owns according to their contracts.

Shutdown ordering matters when owned Resources depend on one another.

For example, an Activity may need to stop accepting work before closing a Resource used by currently active child Operations.

The Standard does not prescribe one universal shutdown order, but ownership and dependency relationships must remain coherent.

## Child Operations

Activities often supervise many child Operations.

For example:

```text
WebSocket Activity
    ├── Receive Operation
    ├── Send Operation
    └── Close Operation
```

or:

```text
Server Activity
    ├── Request A
    ├── Request B
    └── Request C
```

The Activity contract determines whether child Operations must finish before Activity termination, are cancelled during shutdown, may continue independently, or follow another policy.

Child work MUST NOT silently survive the Activity lifetime while continuing to depend on Resources that become invalid when the Activity terminates.

If child work is intended to outlive the Activity, that independent lifetime must be explicit.

## Activity and cancellation

Cancellation primarily controls Operations.

An Activity may still participate in cancellation by requesting cancellation of child Operations during shutdown or in response to another lifecycle event.

The Activity itself is more naturally governed by lifecycle concepts such as start, stop, shutdown, close, or unsubscribe.

Vix should avoid treating Activity shutdown and Operation cancellation as identical concepts.

They may interact, but they serve different purposes.

For example:

```text
Server shutdown
    ↓
request cancellation of active Request Operations
```

The server Activity is shutting down.

The request Operations are being cancelled.

These are related but distinct semantic actions.

## Activity and deadlines

An Activity may have time-related lifecycle constraints.

Examples include:

```text
shutdown grace period
subscription expiry
lease duration
session lifetime
heartbeat deadline
```

These constraints may influence the Activity itself or child Operations.

The Standard does not require every time constraint associated with an Activity to be represented as an Operation Deadline.

Domain contracts should distinguish between a deadline on a finite Operation and a broader Activity lifetime policy where the distinction matters.

For example, a server may have a ten-second shutdown grace period while each active request still retains its own deadline.

## Activity restart

Some Activities may support restart.

A restarted Activity should be treated carefully because restart may create a new logical Activity lifetime even when the same C++ object is reused.

For example:

```text
Activity lifetime A
    start
    run
    terminate

same implementation object

Activity lifetime B
    start
    run
    terminate
```

The Standard does not require restartability.

If restart is supported, the contract must define whether state, Resources, observers, errors, and prior termination status carry into the new lifetime.

The single-termination rule applies separately to each logical Activity lifetime.

## Reconnection

Some long-lived behaviors may reconnect internally.

A realtime subscription, WebSocket client, or remote watcher may temporarily lose transport and establish another one.

This does not necessarily mean that the logical Activity has terminated.

The Activity contract may define reconnection as an internal recovery mechanism.

For example:

```text
Logical Subscription Activity
    transport connection 1 fails
    reconnect
    transport connection 2 established
    continue events
```

The caller may still observe one continuing Activity.

Alternatively, another contract may define transport loss as terminal.

Both models are possible.

What matters is that the Activity contract defines the boundary clearly.

## Internal recovery

Activities may perform internal recovery without exposing every recovery attempt as a new Activity lifetime.

An Activity may replace a Resource, retry a connection, rebuild internal state, or restart a child Operation while remaining logically active.

Recovery must still respect ownership, deadlines, cancellation, authorization, and other applicable contracts.

Internal recovery must not silently create an unbounded lifetime where the public contract promised bounded behavior.

## Events during shutdown

An Activity may still produce some Events after shutdown has been requested but before terminal termination.

For example, active child work may complete, final status may be emitted, or cleanup notifications may occur.

The Activity contract should define whether such Events are permitted.

After terminal termination, normal Activity Events MUST NOT continue to be emitted for that same logical lifetime.

Diagnostic or external observability records about the completed lifetime may still be produced by other systems, but those are not Events from the terminated Activity itself.

## Observing termination

Activity termination may be observed through different mechanisms.

Examples include:

```text
join
wait
callback
future
co_await
status query
termination Event
```

The Standard does not require one mechanism.

If multiple observers are supported, they must observe one coherent terminal state for the same Activity lifetime.

The representation must not accidentally create several contradictory termination outcomes.

## Activity state and public APIs

Implementations may expose Activity state.

For example:

```text
running
stopping
terminated
failed
```

The Standard does not require a universal state enumeration.

Public state should only be exposed when it provides meaningful behavior to callers.

APIs should avoid exposing implementation-state detail that callers cannot safely act upon.

For example, a transient internal state between two scheduler steps may not belong in the public contract.

## Backpressure

Activities that produce Events or create Operations may generate work faster than consumers can process it.

Backpressure is therefore an important domain concern.

The foundational Activity model does not impose one universal backpressure strategy.

Depending on the domain, an Activity may:

```text
block production
slow production
buffer
drop
coalesce
reject new work
disconnect
fail
```

The applicable domain specification should define the behavior when it matters.

The important foundational rule is that overload behavior should be part of the contract rather than an accidental property of internal queues.

## Activity composition

A higher-level Activity may be composed from several lower-level Activities and Resources.

For example:

```text
Realtime Activity
    ├── WebSocket Activity
    ├── Heartbeat Activity
    └── Reconnect Activity
```

or:

```text
Application Service Activity
    ├── HTTP Server Activity
    ├── Background Consumer Activity
    └── Maintenance Activity
```

The higher-level lifecycle contract must define how lower-level lifetimes are coordinated.

Stopping a parent Activity may stop child Activities that belong to its lifetime.

A child Activity with independent lifetime must be explicitly separated from the parent lifecycle.

Composition must preserve Resource ownership boundaries.

## Parent and child Activities

An Activity may own or supervise child Activities.

This is a lifecycle relationship, not merely an object-containment relationship.

If the child belongs to the parent lifetime, the parent should normally be responsible for coordinating its termination.

A parent Activity MUST NOT terminate while dependent child Activities continue using Resources whose lifetime ends with the parent.

The parent may drain, stop, join, transfer, or explicitly detach child Activities according to its contract.

## Detached Activities

An Activity may intentionally continue independently from the lifetime that created it.

This independence must be explicit.

A detached Activity must establish its own valid ownership and Resource lifetime relationships.

For example, a request handler may submit durable work to an application-owned background service. Once accepted by that service, the work no longer belongs to the request Activity or Operation lifetime.

That is valid because ownership has moved to a broader lifetime.

By contrast, simply starting background work that continues to reference request-local state after the request ends is not valid detachment.

## Activity Failure and recovery policy

An Activity may encounter Failures that do not immediately terminate it.

For example, a server may encounter one failed connection attempt and continue accepting others.

A watcher may fail to process one Event while continuing to watch.

A subscription may temporarily lose transport and reconnect.

The Activity contract determines which Failures are:

```text
local
recoverable within the Activity
terminal to the Activity
```

The Standard does not define universal retryability or severity on Error itself.

Recovery policy depends on the Activity and its context.

## Resource replacement

A long-running Activity may replace an internal Resource without changing its logical identity.

For example:

```text
Activity
    uses Connection A
    connection fails
    obtains Connection B
    continues
```

The internal Resource lifetime changes while the Activity remains active.

This is another reason Resource and Activity must remain separate concepts.

Replacing an internal Resource does not automatically create a new Activity lifetime unless the contract says so.

## Activities and providers

An Activity may depend on capabilities supplied by Providers.

For example, a server Activity may require networking and asynchronous I/O capabilities.

The Activity should depend on the required semantics rather than on an unnecessary concrete provider where practical.

If the Activity borrows a Provider that is also a Resource, the normal ownership rules apply.

Stopping the Activity does not automatically authorize it to terminate the Provider.

## Normative requirements

An Activity MUST represent continuing behavior rather than one finite logical attempt.

An Activity lifetime MUST have no more than one terminal termination.

Events and intermediate state transitions MUST NOT be treated as terminal termination.

A request to stop or shut down an Activity MUST NOT be treated as proof that termination has already completed.

An Activity MUST NOT invalidate borrowed Resources unless lifecycle authority has explicitly been granted or transferred.

An Activity MUST preserve the lifetime requirements of Resources it continues to use.

An Activity MUST NOT continue producing normal Activity Events after terminal termination of that logical lifetime.

Child Operations and child Activities MUST NOT silently outlive the parent Activity while still depending on Resources whose lifetime ends with the parent.

Independent child lifetime MUST be explicit.

Operational Activity Failure MUST remain semantically distinct from programming Fault.

Internal recovery or reconnection MUST NOT create multiple terminal terminations for one logical Activity lifetime.

If restart is supported, each restarted logical lifetime MUST have its own coherent termination semantics.

Shutdown behavior that affects active Operations SHOULD define whether those Operations drain, receive cancellation, fail, or follow another policy.

Long-running Activities SHOULD make significant ownership, shutdown, recovery, and Resource-lifetime behavior understandable to callers.

Activities SHOULD depend on semantic capabilities rather than unnecessary concrete Providers where practical.

## Design consequence

An Activity should answer a different question from an Operation.

An Operation asks:

> What finite piece of work is being attempted, and how does it finish?

An Activity asks:

> What continuing behavior is alive, what may happen while it is alive, and how does that lifetime end?

Keeping these questions separate gives Vix a clear model for servers, connections, subscriptions, watchers, background services, and other long-lived systems.

It also keeps lifecycle control distinct from Operation cancellation and Resource ownership.

That separation is what allows long-running behavior to compose with finite work without forcing both into the same abstraction.
