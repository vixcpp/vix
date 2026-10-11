# Composition

Composition is the process of building higher-level behavior from lower-level Resources, Operations, Activities, Capabilities, and Providers.

Most useful software is composed.

An HTTP request may depend on DNS resolution, TCP connection establishment, TLS negotiation, serialization, timers, and asynchronous I/O. A database query may depend on a connection pool, transport, authentication, encoding, retries, and storage. A server may combine listening Resources, routing, request Operations, execution Providers, and shutdown policy.

The purpose of the Vix composition model is to ensure that higher-level abstractions can combine lower-level behavior without destroying the semantics that make those lower layers understandable.

Composition should simplify the programmer's view while preserving the important contracts underneath.

## Composition preserves meaning

A higher-level abstraction may hide implementation details.

It must not silently redefine foundational concepts.

If a child Operation has a Deadline, the parent may propagate or tighten that Deadline, but should not silently replace it with unrelated timeout semantics.

If a Resource is borrowed, composing it into a larger subsystem does not automatically transfer ownership.

If a child Operation fails, the parent may translate or enrich that Failure, but should not turn a programming Fault into ordinary Failure merely because the higher-level API prefers one Error path.

Composition exists above the foundational semantics.

It does not override them.

## Higher-level Operations

A higher-level Operation may be composed from several child Operations.

For example:

```text id="gabfd2"
HTTP Request Operation
├── DNS Resolution Operation
├── TCP Connect Operation
├── TLS Handshake Operation
├── Write Operation
└── Read Operation
```

The caller sees one logical HTTP Request Operation.

Internally, several child Operations may begin and complete.

The logical parent still has one terminal completion.

The completion of a child is not automatically the completion of the parent.

The parent contract determines how child outcomes contribute to the higher-level result.

## Parent and child relationship

A Parent Operation is responsible for work that includes or supervises one or more child Operations.

The relationship is semantic rather than structural.

A child does not need to be implemented as a direct function call.

It may execute through another coroutine, worker thread, Provider, process, callback, remote request, or another mechanism.

What matters is that its work participates in the logical lifetime and contract of the parent.

For example:

```text id="d8o70k"
Parent Operation
    owns logical request lifetime

Child Operation
    performs network connect

different execution mechanism
same composed lifetime
```

The child remains part of the parent composition even when execution is physically separated.

## Parent completion

A parent Operation may only expose terminal completion when the conditions required by its contract have been established.

This often means that dependent child Operations have also reached appropriate terminal states.

The parent must not expose completion while child work continues using parent-owned Resources unless that work has explicitly moved to an independent lifetime.

For example:

```text id="onhvra"
Parent Operation
    uses temporary buffer

Child Operation
    borrows temporary buffer
```

The parent must not complete and destroy the buffer while the child still uses it.

Composition must preserve lifetime correctness.

## Sequential composition

Operations may be composed sequentially.

For example:

```text id="gzfw3t"
resolve
    ↓
connect
    ↓
handshake
    ↓
send
```

Each step may depend on the successful result or Resource produced by the previous step.

A Failure in an earlier step may prevent later Operations from starting.

The higher-level contract determines how that lower-level Failure is represented to the caller.

Sequential composition must preserve the applicable ownership, Deadline, cancellation, and Error semantics across each step.

## Concurrent composition

A parent Operation may execute several child Operations concurrently.

For example:

```text id="ve4aj3"
Parent Operation
├── Child A
├── Child B
└── Child C
```

The parent contract must define what combination of child completions determines the parent completion.

Possible policies include:

```text id="uhpmf8"
all children must succeed
first successful child wins
first terminal child wins
collect all results
quorum
best available result
```

The foundational Standard does not impose one universal concurrency composition policy.

It requires the policy to remain coherent with single completion, cancellation, lifetime, and Failure semantics.

## All-success composition

Some parent Operations require all children to succeed.

Conceptually:

```text id="7861cu"
Parent
├── A → Success
├── B → Success
└── C → Success

Parent → Success
```

If one child fails, the parent may fail immediately or wait for the remaining children depending on the contract.

If remaining children are no longer useful, the parent may request their cancellation.

The cancelled children are not necessarily Failures.

They may simply have become unnecessary because the parent could no longer satisfy its contract.

## First-success composition

Some Operations may race alternatives and use the first successful result.

For example:

```text id="qiwje9"
connect address A
connect address B
connect address C
```

One child may succeed while the others remain active.

Once the parent has enough information to complete successfully, it may request cancellation of the remaining children.

Conceptually:

```text id="002lg7"
A → Failure
B → Success
C → cancellation requested

Parent → Success
```

The Failure of A does not require the parent to fail because the parent contract allowed another alternative.

The cancellation of C is also not a Failure of C's mechanism.

It became unnecessary.

## First-completion composition

Another contract may choose whichever child completes first regardless of outcome.

For example:

```text id="qgxez1"
Operation
races against
Timer Operation
```

If the main Operation finishes first, its completion determines the parent.

If the timer finishes first, the parent applies its timeout policy and may request cancellation of the main Operation.

The timer and Operation remain separate child lifetimes inside one composed contract.

The implementation must still preserve one parent terminal completion.

## Collecting results

Some compositions need to observe all child outcomes.

For example:

```text id="p3uth9"
perform work on several independent targets
collect successes and failures
```

The parent may complete successfully with a collection containing per-child outcomes, or may expose an aggregate Failure according to its contract.

The Standard does not require one universal aggregation representation.

The parent contract must define how child Success, Failure, and Cancelled outcomes contribute to the aggregate result.

## Cancellation propagation

When child Operations belong to the lifetime of their parent, cancellation should normally propagate downward.

Conceptually:

```text id="s916iu"
Parent cancellation requested
    ↓
Child A cancellation requested
Child B cancellation requested
Child C cancellation requested
```

The child Operations still complete independently according to their own contracts.

Propagation does not allow the parent to pretend that the children have already terminated.

If the parent completion contract requires the children to stop using parent-owned state before completion becomes observable, the parent must wait until that condition is true.

## Upward cancellation

Child cancellation does not automatically cancel the parent.

A child may be cancelled because:

```text id="8l8ccj"
another branch won
the child was optional
local policy stopped it
its Resource became unnecessary
```

The parent determines whether that child outcome prevents the parent from satisfying its contract.

For some compositions, one cancelled child is enough to cancel the parent.

For others, it is not.

Cancellation direction is therefore defined by composition policy rather than by a universal bidirectional rule.

## Failure propagation

A child Failure does not automatically become the parent Failure unchanged.

The parent may:

```text id="mpgrc6"
propagate it
translate it
add context
try an alternative
retry
aggregate it
ignore it when allowed by contract
```

For example:

```text id="hhgjw7"
DNS Failure
    ↓
HTTP Request Failure
```

The HTTP layer may expose an Error meaningful to HTTP callers while preserving the DNS Error as a cause.

Composition should add abstraction without erasing diagnostic depth.

## Fault propagation

Programming Faults should not be normalized into ordinary parent Failure merely because they occurred in a child layer.

If a child violates an invariant, the parent should not automatically convert that defect into something like:

```text id="mnbmbv"
Error("request failed")
```

unless a specific boundary intentionally handles that Fault according to a defined contract.

Composition must preserve the semantic distinction between operational Failure and programming Fault.

## Error context

Composition is one of the main places where Error context is added.

A low-level Error may describe the immediate cause:

```text id="hi7whx"
connection_refused
```

The parent may add:

```text id="69bi6i"
while connecting to api.example.com
```

Another layer may add:

```text id="x6tk07"
while sending account request
```

The result may become:

```text id="z06wh6"
while loading account
    while sending account request
        while connecting to api.example.com
            connection_refused
```

Each layer should add information meaningful at its abstraction level without destroying the lower-level cause.

## Deadline propagation

A parent Deadline normally constrains children that participate in the same bounded lifetime.

Consider:

```text id="sdfb0d"
Parent deadline = D
```

A child that begins later should use the remaining time before `D`, not receive the parent's original full Duration again.

This prevents nested composition from silently extending the caller's time budget.

A child may impose a stricter local Deadline.

Conceptually:

```text id="7kuo5j"
effective child deadline
=
earliest(parent deadline, child-local deadline)
```

unless the contract explicitly establishes an independent time lifetime.

## Deadline ownership

A child Operation does not own the parent Deadline.

It participates in the constraint.

The child may use the Deadline to configure timers, provider requests, or cancellation behavior.

It must not extend the parent's Deadline for its own convenience.

A lower-level implementation detail must not weaken a higher-level temporal contract.

## Retry composition

Retry is a form of composition.

A logical Operation may contain several attempts:

```text id="u74dzz"
Logical Operation
├── Attempt 1 → Failure
├── delay
├── Attempt 2 → Failure
├── delay
└── Attempt 3 → Success
```

The attempts are internal components of the logical Operation.

The caller still observes one terminal completion.

Retries must preserve the parent Operation's cancellation and Deadline semantics.

A retry policy must not automatically reset the entire Operation lifetime.

## Retry eligibility

The Standard does not define Error as universally retryable or non-retryable.

Retry decisions depend on:

```text id="moevfm"
Failure identity
Operation semantics
side effects
remaining Deadline
application policy
Provider behavior
```

For example, retrying a connection refusal may be useful in one context and harmful in another.

Composition policy decides whether retry is appropriate.

The Error itself describes what happened.

## Retry and side effects

Retry can be dangerous when an earlier attempt may have produced side effects.

For example:

```text id="6vfys8"
send payment request
connection lost before response
```

The caller may not know whether the remote system processed the request.

Blind retry could perform the action twice.

A retry policy must therefore respect the Operation's idempotence and side-effect contract.

The Standard does not assume that a Failure means the previous attempt had no effect.

## Idempotence

An Operation is idempotent with respect to some effect when repeating it according to the contract does not produce an unintended additional effect.

Idempotence is domain-specific.

The foundational Standard does not classify all Operations as idempotent or non-idempotent.

Composition mechanisms such as retry should only rely on idempotence when the relevant Operation contract provides that guarantee.

## Fallback composition

A parent Operation may try an alternative after a Failure.

For example:

```text id="ooaqgb"
Provider A
    ↓ Failure
Provider B
    ↓ Success
```

Fallback differs from retry when the alternative uses a different strategy or Provider.

The parent may still expose one terminal completion.

Fallback must not silently weaken required semantics.

If Provider B offers weaker durability, security, ordering, or other guarantees, using it may violate the original Operation contract.

Fallback is valid only when the alternative still satisfies the required contract or when the changed semantics are explicitly exposed.

## Provider composition

Providers may compose lower-level Capabilities.

For example:

```text id="7anwdw"
HTTP Provider
├── DNS Provider
├── TCP Provider
├── TLS Provider
└── Timer Provider
```

The higher-level Provider is responsible for satisfying the HTTP-level contract it exposes.

The lower-level Providers remain responsible for their own Capability contracts.

The composition boundary must preserve ownership and lifetime relationships between those Providers.

The HTTP Provider does not automatically own every lower-level Provider it uses.

## Resource composition

A higher-level Resource may depend on several lower-level Resources.

For example:

```text id="96cu55"
Client Resource
├── borrows Execution Context
├── owns Connection Pool
├── uses TLS Provider
└── stores Configuration
```

Each relationship may have different ownership semantics.

The higher-level Resource must not flatten them into one undifferentiated notion of “dependencies”.

Composition should make it possible to know:

```text id="vwr2fe"
what is owned
what is borrowed
what may outlive the parent
what must terminate with the parent
```

## Resource creation inside composition

A child Operation may produce a Resource that becomes part of the parent result.

For example:

```text id="bwhpb7"
Connect Operation
    → Socket Resource

TLS Handshake
    uses Socket
    → Secure Connection Resource

Parent
    returns Secure Connection
```

The composition contract must define how ownership changes during this process.

Intermediate Resources that are no longer needed must be cleaned up appropriately on Failure or Cancellation.

Successful composition must transfer or establish ownership of the final Resource before completion is exposed.

## Cleanup composition

When a composed Operation fails or is cancelled, several intermediate Resources may require cleanup.

For example:

```text id="79hz0m"
DNS succeeded
Socket created
TLS handshake fails
```

The parent may need to close the Socket before exposing terminal Failure.

Cleanup obligations follow Resource ownership.

A borrowed Resource must not be invalidated merely because the composed Operation failed.

An owned temporary Resource must not be leaked merely because the successful path was not reached.

## Cleanup ordering

Cleanup order may matter when Resources depend on one another.

For example:

```text id="gqk88t"
child Operation
    uses Resource B

Resource B
    depends on Resource A
```

The parent must not destroy Resource A before dependent child work and Resource B have reached a state where A is no longer required.

The Standard does not impose one universal cleanup algorithm.

It requires composition to preserve valid lifetime ordering.

## Activities and composition

Activities may also be composed.

For example:

```text id="bn7c0l"
Application Activity
├── HTTP Server Activity
├── Background Worker Activity
└── Maintenance Activity
```

or:

```text id="bcvcsa"
Realtime Activity
├── WebSocket Activity
├── Heartbeat Activity
└── Reconnect Activity
```

The parent Activity contract determines how child Activity lifetimes relate to its own.

Stopping the parent may stop child Activities that belong to the parent lifetime.

An independent child Activity must have its own owner and Resource lifetime.

## Activity shutdown composition

A parent Activity may coordinate shutdown in stages.

For example:

```text id="u5ltqe"
Application shutdown
    ↓
stop accepting new HTTP work
    ↓
allow active requests to drain
    ↓
stop background consumer
    ↓
flush owned state
    ↓
close Resources
    ↓
terminate
```

Different child Activities may have different shutdown semantics.

Composition should coordinate them rather than forcing one universal `stop()` behavior.

The parent is responsible for ensuring that owned Resources remain valid until dependent child work no longer needs them.

## Activity Failure composition

One child Activity failing does not necessarily terminate the parent Activity.

For example, an application may continue running if an optional telemetry Activity fails.

Another child may be essential.

For example:

```text id="g38wol"
Application Activity
    requires database service Activity
```

If the required child Activity terminates permanently, the parent may no longer be able to satisfy its contract.

The parent composition policy determines the consequence.

The Standard does not assign universal criticality to Activities.

## Capability composition

A higher-level feature may require several Capabilities.

For example:

```text id="fnttht"
Secure network client
    requires
        DNS
        TCP
        TLS
        time
```

The higher-level feature does not need to expose every Provider directly.

It may depend only on the semantic Capabilities required by its contract.

Capability composition should preserve the distinction between required and optional Capabilities.

An optional enhancement must not silently become a required dependency without changing the contract.

## Capability failure in composition

If a required Capability cannot be satisfied, the composed system cannot provide the corresponding contract.

This is different from a child Operation failing while using an available Capability.

For example:

```text id="kwlls1"
TLS Capability unavailable
```

is a capability-resolution problem.

```text id="lm6zjh"
TLS Capability available
handshake fails because certificate is invalid
```

is an operational Failure.

Composition must preserve that distinction.

## Optional capabilities

A composition may use an optional Capability when available.

For example:

```text id="3dgvc5"
compression Capability available
    use compression

otherwise
    send uncompressed
```

This fallback is valid if both paths satisfy the higher-level contract.

If the higher-level contract promises compression, then compression is not optional.

Optionality belongs to the contract rather than the implementation's preference.

## Execution composition

One logical Operation may use several execution Capabilities.

For example:

```text id="st58qn"
Request Operation
    network I/O on event loop
    decompression on CPU worker pool
    resume response handling on event loop
```

The movement of execution between Providers does not split the logical Operation.

Cancellation, Deadline, ownership, and single-completion semantics continue across those execution transitions.

A Provider transition must not accidentally create detached lifetime.

## Execution affinity in composition

Some child work may require execution on a particular context or thread.

The parent must preserve that requirement when composing the work.

For example:

```text id="92alqf"
Resource
    confined to event-loop thread
```

A child Operation using that Resource must not be resumed on an arbitrary worker if doing so violates the Resource contract.

Execution flexibility remains subordinate to Resource and Operation correctness.

## Composition and backpressure

Higher-level Activities may compose producers and consumers with different processing rates.

For example:

```text id="887nxg"
Network Activity
    produces messages

Processing Activity
    consumes messages
```

If production exceeds consumption, the composition must have an overload policy.

Possible strategies include:

```text id="fcpjwq"
backpressure
bounded buffering
dropping
coalescing
rejection
disconnect
load shedding
```

The foundational Standard does not choose one universal strategy.

The composition contract should define behavior where overload affects correctness, Resource use, or externally visible semantics.

## Composition and ordering

Child Operations or Events may have ordering requirements.

Some compositions require strict ordering.

Others permit concurrent or out-of-order completion.

The Standard does not assume ordering merely because work was initiated in a particular sequence.

If order matters, it must be part of the applicable contract.

For example:

```text id="sc02fc"
send A
send B
```

does not automatically guarantee that A is externally observed before B unless the transport or higher-level contract guarantees that ordering.

## Composition and concurrency limits

A parent may limit how many child Operations are active at once.

This may protect Resources, control memory, respect remote limits, or provide backpressure.

Concurrency limits are composition policy.

They do not change the semantic identity of child Operations.

If waiting for capacity consumes the parent Deadline or responds to cancellation, that relationship should remain consistent with the parent contract.

## Composition and observability

Higher-level Operations and Activities may expose tracing, metrics, or diagnostics that summarize their children.

For example, an HTTP trace may contain phases for:

```text id="23npfo"
DNS
connect
TLS
write
read
```

Observability should help explain composition.

It must not redefine completion or Error semantics.

A child attempt recorded in a trace is not necessarily a separate public logical Operation.

Likewise, a retry Failure recorded for diagnostics does not mean the parent Operation failed if a later attempt succeeded.

## Composition boundaries

A composition boundary is the point where lower-level behavior is presented through a higher-level contract.

At that boundary, the higher-level abstraction decides:

```text id="5hg1t6"
which child details remain visible
which Errors are translated
which Resources are exposed
which lifetimes are owned
which Capabilities become implementation details
```

A good boundary hides unnecessary implementation detail while preserving semantics needed by the caller.

A bad boundary either leaks every internal mechanism or hides information necessary for correct reasoning.

## Abstraction without semantic loss

Higher-level APIs should simplify.

For example, an HTTP caller should not need to manually perform:

```text id="pk9ug3"
DNS
TCP
TLS
serialization
response parsing
```

for an ordinary request.

The HTTP abstraction is valuable because it composes those concerns.

However, simplification must not mean losing all information about:

```text id="4do1fr"
Failure cause
Deadline
Cancellation
ownership
side effects
```

when those concepts matter to the caller.

The goal is semantic compression, not semantic erasure.

## Independent lifetime

Composition sometimes intentionally creates independent work.

For example:

```text id="8c8g7w"
Request Operation
    submits durable Job

Job
    owned by Application Activity
```

After successful transfer, the Job may continue after the Request completes.

This is valid because the Job now belongs to another lifetime.

The transition must be explicit.

A child must not simply become detached because an implementation forgot to join, cancel, or retain it correctly.

## Ownership transfer in composition

Higher-level composition may transfer ownership between components.

For example:

```text id="6w4k8i"
Operation A
    creates Resource

Operation B
    adopts Resource

Parent
    transfers final ownership to caller
```

Each ownership transition must be semantically clear.

Temporary use is not transfer.

Passing a `shared_ptr` is not automatically transfer.

Moving an implementation object may represent transfer but does not define the semantic relationship by itself.

## Composition and contracts

Every composition creates a higher-level contract.

That contract should answer questions such as:

```text id="2e6fzt"
What constitutes Success?
Which child Failures become parent Failure?
What gets retried?
What gets cancelled?
Which Deadline applies?
Which Resources are owned?
What survives parent completion?
What side effects are possible?
Which Capabilities are required?
```

The implementation can then be changed while preserving those answers.

This is the core purpose of the composition model.

## Composition should not expose the module graph

Higher-level semantic composition should not require callers to understand how Vix modules are physically organized.

For example, a Web Operation should not require the caller to understand that current implementations happen to use separate modules for:

```text id="iyiv9e"
async
networking
TLS
time
thread pool
```

unless those distinctions are semantically relevant to the caller.

Module boundaries belong to architecture.

Composition boundaries belong to the programming model.

The two may align, but they are not required to.

## Normative requirements

Composition MUST preserve the single terminal completion of each logical Operation.

A parent Operation MUST NOT expose terminal completion while dependent child work continues using Resources whose valid lifetime ends with the parent, unless that child lifetime has been explicitly transferred or detached.

Composition MUST preserve ownership and borrowing relationships across abstraction boundaries.

A higher-level component MUST NOT acquire lifecycle authority over borrowed Resources merely because it coordinates their use.

Child Operations participating in a parent's bounded lifetime MUST NOT silently extend the parent's effective Deadline.

Internal retry attempts MUST remain subject to the logical Operation's applicable cancellation and Deadline constraints unless explicitly independent.

Cancellation of a parent SHOULD propagate to dependent child Operations that are no longer needed.

Child cancellation MUST NOT automatically be treated as parent Failure unless the parent contract defines that relationship.

Child Failure MUST NOT automatically erase causal information when that information remains relevant at the parent boundary.

Programming Faults MUST NOT be silently normalized into ordinary operational Failure during composition.

Fallback MUST NOT silently weaken semantic guarantees required by the parent contract.

Retry MUST NOT assume that previous Failure implies absence of side effects.

Ownership transfer MUST be explicit when composition moves Resource lifecycle authority between components.

Independent child lifetime MUST be explicit.

Execution migration between Providers MUST preserve the logical Operation's lifetime, cancellation, Deadline, Resource, and completion semantics.

Required Capability absence MUST remain distinct from Failure encountered while using an available Capability.

Higher-level abstractions SHOULD hide unnecessary implementation detail while preserving semantic information necessary for correct caller behavior.

Composed Activities SHOULD define the relationship between parent shutdown and child Activity or Operation lifetimes where that relationship affects Resource validity.

Composition policies SHOULD define relevant ordering, concurrency, retry, fallback, and overload behavior when those properties affect observable correctness.

## Standard API boundary

The Standard does not define canonical generic C++ abstractions for composition
patterns such as:

```text id="57zqfj"
when_all
when_any
race
retry
timeout
scope
nursery
task group
```

This is a deliberate boundary, not unfinished foundational work. Each helper
necessarily selects policies for aggregation, sibling cancellation, admission,
cleanup, detachment, Deadline handling, side effects, and Error retention.
Those policies cannot be made universal without hiding domain-significant
behavior behind a generic name.

A domain or library MAY provide such a helper when it states those policies
explicitly and preserves the requirements in this composition model. It MUST
NOT give `Result`, `Completion`, Cancellation, Deadline, Resource ownership,
or Activity lifecycle alternate meanings.

Likewise, the Standard does not require one universal structured-concurrency
mechanism. Any implementation that offers one is constrained by the same
requirements: identifiable lifetime, explicit independence, cancellation
propagation, Resource safety, and coherent terminal completion.

## Design consequence

Composition should answer:

> How do smaller pieces of valid Vix behavior become one larger piece of valid Vix behavior?

The answer should not be:

> By forgetting the contracts at the boundary.

A higher-level abstraction may hide complexity, but it must preserve the semantic guarantees that allow the system to remain understandable.

Ownership still matters.

Lifetime still matters.

Cancellation still matters.

Deadlines still matter.

Failure still has meaning.

Capabilities still describe real requirements.

The value of composition is that programmers should not need to manage every lower-level detail themselves.

The responsibility of composition is to ensure that hiding those details does not make the resulting behavior ambiguous.
