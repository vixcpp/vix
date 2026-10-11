# Cancellation

Cancellation is the mechanism by which an Operation is asked to stop before it would otherwise reach its natural completion.

It is a control mechanism, not a terminal outcome by itself.

This distinction is important because an Operation does not necessarily stop at the exact moment cancellation is requested. The request may need to travel through several layers, reach an underlying system operation, be observed by a coroutine or worker, or wait for a safe interruption point. In some cases the Operation may even complete successfully before the cancellation request takes effect.

The Vix Standard therefore treats cancellation as a process rather than as an instantaneous state change.

## Cancellation request

A cancellation request expresses that the controlling lifetime no longer wants an Operation to continue.

The request may come from the caller, a parent Operation, an Activity that is shutting down, a deadline policy, a user action, a Resource transition, or another authority defined by the contract.

Requesting cancellation does not mean that the Operation is already finished.

For example:

```text id="lnv3ba"
Operation running
    ↓
cancellation requested
    ↓
Operation observes request
    ↓
cleanup or lower-level cancellation
    ↓
terminal completion
```

The time between the request and terminal completion may be extremely short or may be observable.

An implementation must not confuse those two moments.

## Cancellation is not completion

A cancellation request is an intermediate control event.

Terminal completion remains governed by the Operation contract.

An Operation that has received a cancellation request may eventually complete as Cancelled, but another terminal outcome may still win if it becomes final first.

Consider a network read:

```text id="p2rt8y"
read in progress
    ↓
cancellation requested
    ↓
data arrives before cancellation becomes effective
    ↓
read completes successfully
```

If successful completion became terminal according to the contract before cancellation took effect, the final outcome remains Success.

The existence of an earlier cancellation request must not retroactively change that completion.

Likewise, once Cancelled completion has become terminal, a late success callback or lower-level completion must not produce a second terminal result.

One logical Operation still has one terminal completion.

## Cancellation races

Cancellation commonly races with normal completion.

This is not an exceptional condition. It is a normal part of concurrent and asynchronous systems.

The Standard does not require implementations to make cancellation win every race.

Instead, the Operation contract must preserve one coherent terminal outcome.

For example:

```text id="g44pvk"
t0    Operation active
t1    cancellation requested
t2    successful result becomes terminal
t3    lower-level cancellation notification arrives
```

The result is Success.

Another execution may be:

```text id="4s4eif"
t0    Operation active
t1    cancellation requested
t2    cancellation reaches underlying work
t3    Operation completes as Cancelled
t4    late lower-level success is discarded
```

The result is Cancelled.

What must never occur is:

```text id="uqt09n"
Success
then
Cancelled
```

or:

```text id="upk4fz"
Cancelled
then
Failure
```

for the same logical Operation lifetime.

## Cancelled completion

Cancelled completion means that cancellation became the determining reason why the Operation did not complete successfully.

This is different from merely having received a cancellation request.

The Standard treats Cancelled as semantically distinct from ordinary operational Failure.

If the user navigates away from a screen and the application cancels a network request that is no longer needed, the network stack did not necessarily fail. The work was deliberately discontinued.

Likewise, if application shutdown cancels outstanding Operations, those Operations may have been perfectly healthy before the shutdown policy decided that they should stop.

Cancellation describes a control decision.

Failure describes inability to satisfy an Operation contract because of an operational condition.

The concrete API may choose to encode both through related types, but the semantic distinction must remain available.

## Cancellation support

Not every Operation must support meaningful cancellation.

Some Operations complete synchronously before a cancellation request could be useful. Some system calls or external libraries may not allow interruption once work has begun. Some Operations may only support cancellation at specific boundaries.

An API that does not support cancellation must not pretend that it does.

An API that does support cancellation should define what that support means in practice.

Cancellation may be immediate, cooperative, deferred, best-effort, or limited by the underlying provider.

The Standard does not require all cancellable Operations to stop within a fixed amount of time after a request.

It does require that the contract avoid implying stronger guarantees than the implementation can provide.

## Cooperative cancellation

Many Operations observe cancellation cooperatively.

This means the Operation or one of its lower-level components periodically reaches points where it can notice that continued work is no longer desired.

For example:

```text id="tmf2it"
process data chunk
check cancellation
process next chunk
check cancellation
...
```

A coroutine may observe cancellation before beginning another asynchronous step.

A networking provider may request that the operating system cancel pending I/O.

A worker may stop before starting another expensive phase.

Cooperative cancellation does not mean that the Operation is badly designed. It simply reflects that safe interruption often requires coordination.

The contract should be clear when cancellation responsiveness matters.

## Non-interruptible work

Some work cannot safely or practically be interrupted once it has begun.

An Operation may still accept a cancellation request while a non-interruptible section is active, but the request may only take effect after that section finishes.

For example, an external library call may provide no interruption mechanism.

In such a case:

```text id="bq94sg"
cancellation requested
    ↓
non-interruptible work continues
    ↓
control returns
    ↓
Operation observes cancellation
```

The Operation may then stop before beginning further work.

The Standard does not require unsafe interruption merely to provide the appearance of immediate cancellation.

## Cancellation authority

Not every component that can observe an Operation automatically has the authority to cancel it.

Cancellation authority is part of the Operation contract.

The caller that owns the Operation lifetime commonly has this authority.

A parent Operation may have authority to request cancellation of child Operations that belong to its lifetime.

An Activity may have authority to cancel Operations it supervises during shutdown.

An observer that merely watches progress may have no cancellation authority at all.

The API should make important cancellation authority relationships understandable rather than deriving them from incidental access to an internal cancellation object.

## Parent and child cancellation

When a child Operation belongs to the lifetime of a parent Operation, cancellation of the parent should normally propagate to the child.

For example:

```text id="09zkot"
HTTP Request Operation
├── DNS Operation
├── Connect Operation
├── TLS Operation
└── Read Operation
```

If the HTTP Request is cancelled, there is usually no reason for an in-progress DNS, connect, TLS, or read Operation that exists only for that request to continue indefinitely.

The parent may therefore request cancellation of the active child work.

Propagation does not mean that the child is instantly complete.

Each child still follows its own cancellation contract.

The parent may also need to wait until dependent children have reached terminal completion before the parent can safely expose its own final completion.

## Independent child lifetime

A child Operation may intentionally outlive the parent that created it.

This is an independent lifetime and must be explicit.

For example, a request may submit durable work to an application-owned background system. Once ownership of that work has been transferred to the background system, cancellation of the original request may no longer apply.

The important point is that the work did not simply escape by accident.

Its lifetime and cancellation authority moved to another owner.

A child Operation must not ignore parent cancellation while continuing to use Resources whose validity still depends on the parent.

## Cancellation and borrowed Resources

Cancellation does not change Resource ownership.

If an Operation borrows an execution context, connection, buffer, service, or another Resource, cancelling the Operation does not authorize it to invalidate that Resource.

The Operation may stop using the Resource.

It may release its borrow.

It may cancel child work that belongs to its own lifetime.

It must not stop or close broader-scope borrowed infrastructure unless the Resource contract explicitly grants that authority.

For example:

```text id="wch29c"
Application
    owns ExecutionContext

Operation
    borrows ExecutionContext
```

Cancelling the Operation ends the Operation's need for the execution context.

It does not give the Operation permission to stop the execution context.

## Cancellation and owned Resources

An Operation may own Resources created specifically for its own lifetime.

If cancellation prevents the Operation from transferring those Resources elsewhere, the Operation remains responsible for cleaning them up according to their contracts.

For example:

```text id="4vt9s0"
Operation
    creates temporary Resource
    cancellation requested
    Operation terminates
    temporary Resource cleaned up
```

The exact cleanup sequence depends on the Resource and Operation contracts.

Cancellation must not silently leak Resources merely because normal completion was interrupted.

## Cancellation and partial progress

Cancelled completion does not imply that no useful or externally visible work occurred.

A network write may have transmitted some bytes.

A file operation may have changed part of a file.

A remote request may already have reached another system.

A process may already have been started.

A database interaction may have performed work outside a transaction.

The Standard therefore does not interpret cancellation as rollback.

If an Operation provides atomicity, rollback, or another stronger guarantee, that guarantee must come from the Operation contract.

Where partial progress matters, the API should make it possible for the caller to understand the resulting state.

## Cancellation and side effects

Some side effects cannot be undone after cancellation.

For example, cancelling an HTTP client Operation after the request was transmitted does not prove that the remote server ignored the request.

Cancelling a process-launch Operation after the operating system created the process does not necessarily mean that no process exists.

Cancelling a message-send Operation may occur after another system has already received the message.

This is particularly important in distributed systems.

The local terminal outcome of an Operation must not be interpreted as proof of the state of external systems unless the contract provides such a guarantee.

## Cancellation and deadlines

A deadline may cause a cancellation request, but deadline expiration and cancellation are not the same concept.

The relationship may be:

```text id="3mw53u"
Deadline reached
    ↓
timeout policy
    ↓
request cancellation
    ↓
Operation eventually completes
```

The final completion may be represented as Cancelled, Failure, or another contract-defined outcome.

The Standard does not require all deadline-driven termination to appear as cancellation to callers.

What remains stable is that reaching a deadline is a time condition, while cancellation is a control mechanism.

The Operation contract determines how the two interact.

## Explicit cancellation and deadline cancellation

An implementation may wish to preserve the reason why cancellation was requested.

For example:

```text id="b5uwgt"
explicit caller cancellation
parent cancellation
deadline expiration
Activity shutdown
Resource shutdown
```

These reasons may matter for diagnostics or domain policy.

The Standard allows an implementation to preserve this information.

It does not require one universal cancellation-reason enumeration.

The semantic distinction between reasons should only become normative where callers need to rely on it.

## Cancellation and timeout reporting

A deadline-driven cancellation may still be reported to the caller as a timeout Failure if that is the contract of the Operation.

For example:

```text id="ky0ztv"
connect(deadline)
    deadline expires
    underlying connect work is cancelled
    public completion = Failure(timeout)
```

Internally, cancellation may be the mechanism.

Externally, timeout may be the semantic result promised by the API.

This is valid because mechanism and contract are distinct.

Another API may expose the same internal event as Cancelled with a deadline reason.

Both are possible as long as the public contract is coherent.

## Cancellation and Activities

Cancellation primarily applies to finite Operations.

Long-lived Activities are normally governed by lifecycle concepts such as stop, shutdown, close, or unsubscribe.

An Activity may use cancellation internally to terminate child Operations.

For example:

```text id="8mvej4"
Server shutdown requested
    ↓
stop accepting new requests
    ↓
request cancellation of selected active Operations
    ↓
wait for completion
    ↓
Server Activity terminates
```

The Activity itself is shutting down.

The child Operations are being cancelled.

These actions should not be collapsed into one concept.

## Shutdown propagation

When an Activity is shutting down, it may request cancellation of Operations that belong to its lifetime.

The exact policy depends on the Activity contract.

A graceful shutdown may allow current Operations to complete until a grace deadline.

A more immediate shutdown may request cancellation immediately.

Some Operations may be allowed to continue because their lifetime was transferred elsewhere.

The Standard does not impose one universal shutdown policy.

It does require that the relationship between Activity shutdown and child cancellation be understandable.

## Cancellation observation

An Operation may expose cancellation state internally or publicly.

Possible observations include:

```text id="e021jg"
cancellation not requested
cancellation requested
cancellation being processed
```

The Standard does not define a universal cancellation state machine.

Such states should not be mistaken for terminal completion.

A query such as “is cancellation requested?” answers a control question, not “has the Operation finished?”

## Repeated cancellation requests

An Operation may receive the same cancellation intent more than once.

Repeated requests must not cause multiple terminal completions.

Implementations should normally make repeated cancellation requests safe to issue.

Whether repeated requests are strict no-ops, refresh metadata, record additional reasons, or behave in another defined way is implementation-specific unless exposed by the contract.

The central guarantee is that cancellation remains associated with one logical Operation lifetime and one terminal completion.

## Cancellation after completion

A cancellation request arriving after terminal completion has no power to change the completed outcome.

If the implementation still holds a cancellation object or control handle after completion, requesting cancellation through it must not reopen or rewrite the Operation lifetime.

The Operation has already finished.

The API may treat such a request as a no-op, reject it, or follow another harmless policy.

What it must not do is create a second terminal outcome.

## Completion after cancellation

Once cancellation has become the terminal outcome, later implementation events must not produce another public terminal completion.

A lower-level callback, system completion packet, worker result, or scheduled continuation may still arrive because cancellation and execution mechanisms often operate concurrently.

The implementation must reconcile those events internally.

The caller should continue to observe the one terminal outcome already established.

## Cancellation registration

Some implementations allow callbacks or handlers to be registered so that code is notified when cancellation is requested.

The Standard does not require one registration API.

If cancellation callbacks exist, their lifetime and invocation rules must be safe.

A callback must not be invoked after the state it references has become invalid.

Registration and unregistration must follow the same ownership and borrowing principles as other lifetime-bearing relationships.

Cancellation infrastructure must not introduce hidden use-after-lifetime behavior.

## Cancellation callback execution

A cancellation callback may execute synchronously with the request, asynchronously through an execution facility, or through another provider-defined mechanism.

If that distinction matters to callers, the contract must define it.

An API should not unexpectedly run arbitrary user code while holding internal locks or while a lifecycle transition is only partially complete unless that behavior is part of the contract.

The Standard does not mandate one callback execution policy, but cancellation must not become an excuse for undefined execution ownership.

## Cancellation tokens and sources

An implementation may represent cancellation using concepts similar to sources and tokens.

One entity may hold authority to request cancellation while another only observes whether cancellation has been requested.

This maps naturally to the semantic distinction between cancellation authority and cancellation observation.

The Standard does not require particular type names or one concrete token/source API.

A future canonical Vix API may define such types.

The semantics described here must remain valid regardless of representation.

## Cancellation and capability boundaries

Some Providers may have stronger cancellation capabilities than others.

One provider may support true operating-system cancellation of pending I/O.

Another may only stop scheduling future work.

Another may be unable to interrupt an external library call at all.

These provider differences do not redefine the meaning of cancellation.

They affect how quickly or completely the request can be honored.

An Operation contract should not promise provider-independent responsiveness when the underlying capability cannot support it.

Profiles or provider documentation may describe stronger cancellation guarantees where they exist.

## Cancellation and blocking work

Blocking work deserves particular care.

If blocking work runs directly on an execution context that must remain responsive, cancellation may not be observed until the blocking call returns.

Moving blocking work to another execution capability may improve responsiveness, but that is an execution-design decision rather than a change to the meaning of cancellation.

The Standard does not require every blocking system call to become interruptible.

It does require APIs to avoid misleading callers about cancellation guarantees.

## Cancellation and retries

A cancellation request applies to the logical Operation, not merely to the current retry attempt, unless the contract explicitly states otherwise.

For example:

```text id="4ovfew"
Request Operation
    attempt 1 fails
    attempt 2 begins
    cancellation requested
```

The retry policy should not silently start attempt 3 after cancellation merely because the current attempt ended.

Likewise, retrying must not create a fresh independent cancellation lifetime by default.

Retries remain subordinate to the logical Operation lifetime.

## Cancellation and composition

Higher-level Operations may combine multiple child Operations concurrently.

For example:

```text id="z1rcvg"
Parent Operation
├── Child A
├── Child B
└── Child C
```

If the parent is cancelled, the composition policy may request cancellation of all dependent children.

If Child A fails and the parent contract says the remaining children are no longer useful, the parent may request cancellation of B and C.

This is a composition policy.

The cancelled children remain responsible for reaching their own terminal completions according to their contracts.

The parent should not simply abandon dependent child Operations while those children still reference parent-owned state.

## When one child wins

Some composed Operations race several alternatives.

For example:

```text id="rk1jjl"
connect to address A
connect to address B
use first successful result
```

When one child wins, the parent may request cancellation of the remaining children.

The losers are not necessarily Failures.

They may simply become unnecessary because another branch satisfied the parent contract.

This is another reason cancellation must remain distinct from operational Failure.

## Cleanup after cancellation

Cancellation often requires cleanup before terminal completion becomes observable.

Cleanup may include:

```text id="s4k67j"
releasing owned Resources
unregistering callbacks
joining dependent work
closing temporary handles
restoring invariants
removing pending timers
```

An Operation must not expose Cancelled completion before the guarantees associated with that completion are actually true.

For example, if the contract promises that the Operation no longer accesses a borrowed buffer after completion, cancellation cleanup must establish that condition before completion is reported.

## Cleanup failure during cancellation

Cleanup performed during cancellation may itself encounter Failure.

This creates a difficult boundary between the reason the Operation was asked to stop and the fact that cleanup could not be completed as expected.

The Standard does not impose one universal representation for this case.

The Operation contract may prioritize the cleanup Failure, preserve cancellation as context, aggregate information, or use another domain-appropriate model.

What matters is that significant cleanup Failure must not be silently discarded when doing so would leave the caller with an incorrect understanding of system state.

This area may require more specific rules in domain specifications or future Vix Evolution Proposals.

## Cancellation and diagnostics

Cancellation is often expected behavior and should not automatically be diagnosed as an Error.

For example:

```text id="h6h1we"
user navigated away
request cancelled
```

may require no alarming diagnostic at all.

By contrast, an Operation that repeatedly fails to honor cancellation within its promised responsiveness may warrant diagnostic information.

Diagnostics should therefore distinguish expected cancellation from actual implementation Failure.

The existence of a Cancelled completion does not by itself determine log severity.

## Cancellation and observability

Tracing and metrics may record cancellation requests, cancellation latency, final outcomes, or reasons.

These observability features should not change the Operation semantics.

A trace event saying “cancellation requested” must not be treated as evidence that the Operation had already completed at that moment.

Likewise, a metric counting cancelled Operations should be based on terminal outcomes rather than merely counting every request to cancel.

## Normative requirements

A cancellation request MUST be treated as a control request, not as terminal completion.

A cancellation request MUST NOT be considered proof that the Operation has stopped.

An Operation MUST expose no more than one terminal completion even when cancellation races with Success or Failure.

A terminal completion that became final before cancellation took effect MUST NOT be retroactively changed by a later cancellation event.

Cancelled completion MUST remain semantically distinct from ordinary operational Failure.

Cancellation MUST NOT be assumed to imply rollback, absence of side effects, or absence of partial progress.

Cancelling an Operation MUST NOT grant lifecycle authority over Resources that the Operation only borrows.

An Operation that owns temporary Resources MUST preserve its cleanup obligations when cancellation interrupts normal work.

A child Operation that participates in a parent's lifetime SHOULD receive cancellation when the parent is cancelled and the child is no longer needed.

A child Operation intended to survive parent cancellation MUST have an explicitly independent lifetime.

Cancellation of a logical Operation MUST apply across internal retry attempts unless the contract explicitly defines otherwise.

Repeated cancellation requests MUST NOT produce repeated terminal completions.

A cancellation request issued after terminal completion MUST NOT change the completed outcome.

An implementation MUST reconcile late lower-level completions internally after a terminal Cancelled outcome has already been established.

Cancellation infrastructure MUST preserve the lifetime validity of registered callbacks, observers, and Resources.

APIs that advertise cancellation SHOULD describe any observable limitations on responsiveness or interruptibility.

Shutdown of an Activity SHOULD use cancellation for child Operations where cancellation is semantically appropriate rather than treating Activity shutdown and Operation cancellation as identical concepts.

## Open representation questions

The Standard intentionally does not yet define one canonical C++ cancellation representation.

It does not require specific types such as:

```text id="k68c6h"
CancellationToken
CancellationSource
StopToken
StopSource
Cancelled
```

nor does it define whether cancelled completion is encoded directly inside `Result<T>`, in an asynchronous completion wrapper, through a dedicated state, or through another representation.

Those choices affect Async, networking, process execution, thread pools, filesystem operations, and other domains.

They should therefore be standardized only after those requirements have been examined together.

The semantic requirements in this document are intended to remain valid regardless of the eventual representation.

## Design consequence

Cancellation should answer one question:

> Is this Operation still wanted?

Completion answers another:

> How did this Operation actually finish?

Keeping those questions separate prevents races, shutdown behavior, timeouts, retries, and asynchronous composition from collapsing into one ambiguous state.

A programmer should be able to request that work stop without having to pretend that the work has already stopped.

A caller should also be able to trust that once an Operation reports its terminal outcome, later cancellation or execution events will not rewrite that history.

That separation is the foundation of predictable cancellation in Vix.
