# Providers

A Provider is an implementation that satisfies one or more Capabilities.

Providers connect the semantic requirements described by the Vix Standard to concrete platform mechanisms, external libraries, runtime facilities, hardware, or other implementation layers.

A Capability describes what software needs.

A Provider describes how that ability is supplied in a particular environment.

This distinction is one of the main mechanisms that allows Vix to preserve stable semantics while implementations evolve.

A Provider may change without requiring the Capability itself to change.

## Provider and Capability

A Provider exists in relation to one or more Capability contracts.

For example:

```text id="e15m1k"
Capability
    monotonic time

Provider
    operating-system steady clock wrapper
```

or:

```text id="rn2g4t"
Capabilities
    TCP
    UDP
    DNS
    timers

Provider
    asynchronous networking implementation
```

A Provider may satisfy several Capabilities at once.

Likewise, one Capability may have several valid Providers.

The relationship is therefore not one-to-one.

What matters is that the Provider preserves the semantics promised by each Capability it claims to satisfy.

## Provider conformance

A Provider is conforming when its observable behavior satisfies the normative contract of the Capability or profile it claims to implement.

A Provider must not claim conformance merely because it exposes similarly named APIs.

For example, a Provider claiming monotonic time must actually preserve the relevant monotonic semantics.

A Provider claiming a cancellation-capable execution facility must not imply interruption guarantees that it cannot preserve.

A Provider claiming persistent storage must not silently substitute volatile memory while preserving the same Capability name if persistence is part of the contract.

Conformance is about behavior, not naming.

## Provider implementation freedom

The Standard does not prescribe how Providers are internally implemented.

A Provider may use:

```text id="t7n24q"
operating-system APIs
external libraries
hardware
runtime services
other Vix Providers
custom algorithms
```

provided that the required semantic contract is preserved.

This freedom is intentional.

The Provider boundary exists partly so that Vix can replace low-level implementation mechanisms without forcing higher-level software to change its mental model.

## External libraries

An external library may be used by a Provider without becoming part of the Vix semantic contract.

For example:

```text id="0uwr9r"
Vix TLS Provider
    uses OpenSSL
```

The Capability exposed to Vix software is TLS.

OpenSSL is an implementation dependency.

Likewise:

```text id="5pncjm"
Vix async networking Provider
    uses Asio
```

does not mean that domain APIs should automatically expose Asio as part of their semantic contract.

A Provider should isolate external-library details when those details are not intentionally part of the public contract.

## Provider-specific behavior

Some software may intentionally depend on Provider-specific behavior.

This is valid when the dependency is explicit.

For example, a specialized environment may require a particular hardware accelerator, database engine, runtime service, or vendor API.

In such a case, the contract is no longer fully provider-independent.

The dependency should be represented as such rather than pretending that vendor-specific behavior is universally available through a generic Capability.

Provider independence is a design goal where practical, not a requirement to hide every useful implementation detail.

## Provider dependencies

A Provider may depend on other Capabilities.

For example:

```text id="f7g14s"
HTTP Provider
    requires DNS
    requires TCP
    may require TLS
    requires time
```

or:

```text id="mcqs3u"
TLS Provider
    requires transport
    may require entropy
    may require wall-clock time for certificate validation
```

This means Providers may form dependency graphs.

The dependency should be expressed in terms of the semantic Capabilities actually required rather than unnecessarily naming specific lower-level Provider implementations.

This keeps the graph replaceable.

## Provider composition

A higher-level Provider may compose several lower-level Providers.

For example:

```text id="ofk699"
HTTPS Provider
    DNS Provider
    TCP Provider
    TLS Provider
    Timer Provider
```

The higher-level Provider is responsible for preserving the contract it exposes even though several lower-level implementations participate internally.

Lower-level Providers remain responsible for their own contracts.

Composition should not erase ownership, Failure, cancellation, Deadline, or Resource semantics across the boundary.

## Provider and Resource

A Provider may or may not itself be a Resource.

A stateless Provider may have no meaningful lifetime.

A stateful Provider may own threads, queues, connections, handles, caches, memory, or other Resources.

Examples include:

```text id="9p86mm"
WorkerPool Provider
EventLoop Provider
ConnectionPool Provider
Database Provider
GPU Provider
```

When a Provider has a meaningful lifetime, the normal Resource and ownership rules apply.

Consumers may use the Capability supplied by the Provider without owning the Provider itself.

## Provider ownership

Using a Provider does not imply lifecycle ownership.

A consumer may borrow a Provider from a broader application or runtime.

For example:

```text id="bo9b6h"
Application
    owns WorkerPool Provider

Subsystem
    borrows CPU execution Capability
```

The subsystem may submit work.

It must not stop the WorkerPool merely because it uses it.

The Provider Resource contract determines lifecycle authority.

This distinction becomes especially important when many independent Operations or Activities share the same Provider.

## Provider initialization

Some Providers require initialization before they can satisfy their Capabilities.

Initialization may involve:

```text id="l0wrsa"
allocating Resources
opening system handles
loading configuration
discovering platform features
starting worker threads
negotiating external services
```

Initialization may itself be an Operation.

For example:

```text id="24in52"
create Provider
    ↓
initialize
    ↓
Success
or
Failure
```

The Standard does not require one initialization model.

What matters is that Capability availability should not be assumed before the Provider can actually satisfy its contract.

## Provider readiness

A Provider object may exist before it is ready to provide its Capability.

If this distinction is observable, the contract should define it.

For example:

```text id="opbgwf"
constructed
initializing
ready
stopping
stopped
```

The Standard does not require a universal state machine.

It does require that callers not be forced to infer readiness from incidental implementation details.

If Operations are only valid after readiness, that must be part of the contract.

## Provider shutdown

A stateful Provider may support shutdown.

Provider shutdown is a Resource-lifecycle concern.

Shutdown may include:

```text id="q5v1tm"
stop accepting new work
drain accepted work
request cancellation
close owned Resources
join workers
release platform handles
```

The exact behavior depends on the Provider.

The Standard does not define one universal shutdown sequence.

The Provider contract must define the effect of shutdown on new requests and already accepted work.

## Provider shutdown and existing Operations

An Operation already using a Provider may be affected when that Provider begins shutting down.

Possible outcomes include:

```text id="jt1e29"
Operation allowed to complete
Operation receives cancellation
Operation fails
submission rejected
provider drains before stopping
```

The contract must define the behavior.

A Provider MUST NOT simply disappear while dependent Operations continue assuming its Capability remains valid.

Shutdown must preserve coherent terminal semantics.

## Submission after Provider shutdown

A Provider that no longer accepts new work may reject requests.

If callers are legitimately allowed to race with shutdown, rejection should be represented as an operational condition rather than undefined behavior.

If the contract prohibits use after shutdown, violating that precondition may instead be a programming Fault.

The boundary must be explicit.

Silent work loss is not an acceptable substitute for a defined contract.

## Provider replacement

A Provider may be replaced while preserving the same Capability contract.

For example:

```text id="lus6hc"
TLS Provider A
    replaced by
TLS Provider B
```

If both Providers satisfy the same baseline Capability, higher-level software should not need to change its semantic reasoning.

Performance, implementation technology, diagnostics, or optional stronger guarantees may differ.

The baseline contract must remain preserved.

This replaceability is one of the reasons Provider identity should not leak unnecessarily into domain APIs.

## Provider selection

Provider selection may occur at several stages.

Examples include:

```text id="g4lf8f"
build time
link time
application startup
configuration
dependency construction
runtime negotiation
```

The Standard does not require one universal selection mechanism.

Some Providers may be fixed for an entire build.

Others may be chosen per application instance, per environment, or even per Operation.

The appropriate mechanism depends on the Capability and domain.

## Static Provider selection

Static selection may be useful when a target platform has one known implementation.

For example:

```text id="kkebtz"
Linux build
    selects native filesystem Provider
```

This can reduce runtime complexity.

Static selection does not make the Provider part of the semantic Capability.

The higher-level contract remains provider-independent where intended.

## Runtime Provider selection

Runtime selection may be useful when more than one Provider is available or when the environment is only known after startup.

For example:

```text id="jzec5x"
GPU available
    use accelerated Provider

otherwise
    use CPU Provider
```

Runtime selection is valid when all selected Providers preserve the contract expected by the caller.

If Providers offer materially different guarantees, the selection logic must not pretend that the resulting behavior is identical.

## Provider fallback

A system may fall back from one Provider to another.

Fallback is safe when the replacement Provider preserves the guarantees required by the caller.

For example:

```text id="gtb6gz"
hardware compression Provider unavailable
    ↓
software compression Provider
```

may preserve semantics while changing performance.

By contrast:

```text id="0euwyl"
persistent storage unavailable
    ↓
use volatile memory
```

changes persistence semantics.

Such a fallback must not be invisible if the caller depends on persistence.

Fallback may change performance or implementation.

It must not silently weaken required semantics.

## Provider strength

A Provider may offer stronger guarantees than the minimum Capability contract.

Examples include:

```text id="k1qf7a"
higher timing precision
stronger cancellation support
better ordering guarantees
hardware acceleration
stronger durability
additional metrics
```

These stronger guarantees may be useful.

They should be described as Provider-specific or profile-specific extensions unless they become part of the shared Capability contract.

A stronger Provider may satisfy a weaker baseline Capability.

A weaker Provider may not claim a stronger contract.

## Provider compatibility

Two Providers are compatible for a given use when both satisfy the required Capability semantics and any additional guarantees required by the consumer.

The fact that two Providers implement similarly named APIs does not prove compatibility.

Likewise, binary or type compatibility does not automatically imply semantic compatibility.

Provider interchangeability is ultimately a contract question.

## Provider identity

Some applications may need to know which Provider is active for diagnostics, debugging, optimization, policy, or deployment reasons.

The Standard allows Provider identity to be observable.

Provider identity should not become necessary for ordinary semantic reasoning when the Capability contract is sufficient.

For example, logging:

```text id="bnh6np"
TLS Provider: OpenSSL-backed
```

may be useful.

Application logic that branches on Provider identity should do so only when Provider-specific behavior is intentionally part of the contract.

## Provider configuration

Providers may have configuration.

Examples include:

```text id="czgq42"
worker count
queue size
TLS certificate store
network interface choice
database endpoint
hardware preference
```

Configuration controls how the Provider operates.

Configuration itself is not the Capability.

Changing configuration may preserve the same Capability or may alter which guarantees the Provider can satisfy.

Where configuration can weaken a required guarantee, the system must not continue claiming a stronger Capability than it can actually provide.

## Provider-local policy

A Provider may implement local policy such as:

```text id="paq9fk"
scheduling strategy
cache policy
reconnection strategy
queue discipline
resource reuse
batching
```

These policies are implementation details unless their behavior is exposed through the public contract.

The Standard should not freeze local Provider algorithms unnecessarily.

This allows Providers to improve over time.

## Provider Failure

A Provider may fail to initialize, become unavailable, reject work, or encounter internal operational conditions.

Such Failures should be represented according to the applicable contract.

For example:

```text id="ev8k42"
Provider initialization
    → Failure

Operation through Provider
    → Failure

Provider Resource lost
    → Activity termination or capability loss
```

The exact outcome depends on where the Failure occurs.

Provider Failure should not automatically be classified as programming Fault.

At the same time, an internal invariant violation inside a Provider remains a Fault.

## Provider health

Some stateful Providers may expose health information.

For example:

```text id="jtnap4"
ready
degraded
unavailable
```

The Standard does not require a universal health interface.

Health should only be exposed when it has stable semantic meaning for callers.

A provider-specific metric or transient internal condition should not automatically become part of the Capability contract.

## Dynamic Provider loss

A Provider may become unavailable while the application is running.

Examples include:

```text id="tqwcaa"
external service disconnects
device removed
network subsystem lost
worker process exits
hardware accelerator resets
```

If the Capability can disappear dynamically, the Provider contract should define the consequences for:

```text id="uyvg76"
new Operations
active Operations
Activities
Resources previously created through the Provider
```

The Standard does not assume that Capability availability is permanently stable.

## Resources created by Providers

A Provider may create Resources.

For example:

```text id="yfc0jp"
TCP Provider
    creates Socket Resource

Process Provider
    creates Process Resource

Filesystem Provider
    creates File Resource
```

The ownership of those Resources must be defined by the relevant Operation or Provider contract.

The Resource does not automatically remain owned by the Provider simply because the Provider created it internally.

Likewise, it does not automatically become caller-owned.

Ownership transfer must be explicit.

## Provider-scoped Resources

Some Resources may only remain valid while the Provider remains alive.

For example, a Resource may internally depend on handles or state owned by the Provider.

In that case, the Provider lifetime becomes a parent Resource lifetime.

The contract must ensure that dependent Resources do not outlive the Provider unless they have been made independent.

A Provider owner must not shut down the Provider while dependent Resources are still expected to remain valid without defining what happens to them.

## Provider-independent Resources

Other Resources may become independent after creation.

For example, a Provider may create an operating-system handle and transfer complete ownership to the caller.

The caller may then continue using that Resource without retaining the Provider.

The contract must define this relationship.

The Standard does not assume all Provider-created Resources remain tied to Provider lifetime.

## Provider threading model

A Provider may have threading or affinity requirements.

Examples include:

```text id="qe5kjc"
thread-safe
single-thread confined
callbacks on event-loop thread
operations may complete on worker threads
```

If these properties affect safe use, they belong to the Provider contract.

The Standard does not impose one universal threading model.

Higher-level APIs should avoid depending on Provider-specific threading behavior when it is not semantically necessary.

## Provider callback model

Providers may invoke callbacks or resume continuations.

The contract should define relevant behavior such as:

```text id="5plq7d"
inline or deferred
thread or execution context
serialization guarantees
reentrancy
ordering
```

when callers need to rely on it.

The Standard does not require one callback model.

It does require that callback execution not create hidden ownership or lifetime violations.

## Provider and cancellation

Providers may offer different levels of cancellation support.

For example:

```text id="d7w3f2"
Provider A
    can cancel queued work

Provider B
    can cancel pending OS I/O

Provider C
    cannot interrupt underlying foreign call
```

All may participate in the same general cancellation semantics while providing different responsiveness guarantees.

A Provider must not claim immediate cancellation if the implementation can only observe cancellation after the underlying work returns.

Provider-specific cancellation strength may be documented as an extension.

## Provider and time

Providers that implement time-related Capabilities must preserve the semantics defined by the Standard.

A Provider claiming monotonic time must use an appropriate monotonic source.

A Timer Provider must describe relevant precision and scheduling limitations.

A Provider must not silently reset inherited Operation Deadlines merely because its local API is expressed in relative timeout Durations.

Higher-level temporal contracts remain binding across Provider boundaries.

## Provider and Errors

Providers may expose domain-specific operational Errors.

These Errors should be structured and should not require callers to inspect human-readable text.

A higher-level Provider may translate lower-level Errors into its own domain contract.

Where useful, it should preserve causal information.

Provider boundaries should improve abstraction without destroying the reason an Operation failed.

## Provider and Faults

Internal Provider invariant violations are programming Faults.

They should not automatically become ordinary operational Failures merely because the Provider boundary makes that translation convenient.

For example, a corrupt internal queue or impossible state in a Provider indicates an implementation defect.

A remote service being unavailable is an operational condition.

The Provider contract should preserve this distinction.

## Provider observability

Providers may expose diagnostics and observability information such as:

```text id="0hj9u2"
active Provider
configuration
resource usage
queue depth
latency
health
platform backend
```

These facilities are useful for operations and debugging.

They are not automatically part of the Capability semantics.

Observability must not become a hidden requirement for using the Provider unless the Capability contract explicitly includes it.

## Provider testability

Alternate Providers can be useful for testing.

Examples include:

```text id="mjkxsy"
deterministic Clock Provider
in-memory storage Provider
simulated networking Provider
single-threaded execution Provider
```

A test Provider should preserve the semantic contract of the Capability it claims to implement unless the test explicitly targets a weaker or specialized contract.

Mock convenience should not justify violating the semantics being tested.

## Provider security boundaries

A Provider may cross security or trust boundaries.

For example:

```text id="8dltx2"
remote storage Provider
process Provider
network Provider
credential Provider
external model Provider
```

The foundational Provider model does not define detailed security policy.

However, Provider substitution must not silently weaken security guarantees that are part of the applicable Capability or domain contract.

Security-relevant guarantees should be explicit when they affect whether Providers are interchangeable.

## Provider locality

A Provider may be:

```text id="1gcwp2"
in-process
operating-system backed
remote
distributed
hardware-backed
```

The Standard does not require Providers to be local.

A remote Provider may still satisfy a Capability if it preserves the required semantics.

However, remoteness may introduce Failure modes, latency, authorization requirements, and availability behavior that must be represented honestly in the Provider contract.

## Provider layering

Providers may be layered.

For example:

```text id="l60s9h"
Application-level Provider
    ↓
Vix Provider
    ↓
external library
    ↓
operating-system mechanism
```

Not every layer needs to be exposed to the application.

The Capability contract should be defined at the layer meaningful to its consumers.

Layering should simplify dependency reasoning rather than expose every implementation detail.

## Provider cycles

Provider dependency graphs should avoid semantic cycles that make initialization, ownership, or shutdown impossible to reason about.

For example, if Provider A requires Capability B and Provider B requires Capability A, the architecture must define how that cycle is resolved.

The Standard does not universally prohibit implementation cycles, because some systems may use staged initialization or mutually supporting Resources.

However, cycles that make Capability satisfaction or lifecycle authority ambiguous should be avoided.

Provider dependency direction should remain understandable.

## Provider scopes

A Provider may exist at different scopes.

Examples include:

```text id="7t8ekz"
process-wide
application-wide
session-scoped
request-scoped
Operation-scoped
Resource-scoped
```

Scope affects ownership and lifetime.

The Standard does not require one universal scope model.

A consumer should not assume that a Provider is globally unique unless the contract says so.

Likewise, a Provider being globally accessible does not automatically make global ownership semantics correct.

## Provider multiplicity

More than one Provider for the same Capability may exist simultaneously.

For example:

```text id="9u4hm2"
two database Providers
two network Providers
multiple execution pools
multiple storage backends
```

The Capability model must not assume global singleton Providers.

Provider selection may depend on application configuration, Resource ownership, Operation context, or another explicit relationship.

This is another reason the Standard does not require a global Capability registry.

## Provider binding

Software may be bound to a Provider for some lifetime.

For example:

```text id="14itn7"
Client Resource
    bound to TLS Provider A

Operations through Client
    use TLS Provider A
```

The binding may last for the Resource lifetime or may be replaceable according to the contract.

Provider binding should not be changed silently if doing so would alter observable guarantees.

## Provider migration

A long-lived Activity or Resource may migrate from one Provider to another if its contract permits it.

For example, a system may move future work from one execution pool to another.

This does not necessarily create a new logical Activity or Resource lifetime.

However, migration must preserve the guarantees promised to callers.

If Provider-specific state or semantics cannot be transferred safely, the migration may require a new Resource or explicit lifecycle transition.

## Provider choice and deterministic behavior

Different Providers may produce different valid execution orderings, timings, or performance characteristics while preserving the same semantic contract.

Applications should not rely on incidental Provider behavior unless that behavior is guaranteed.

For example, one Provider may execute two independent Operations in a different order from another.

If ordering matters, it must be part of the relevant contract rather than an accidental property of one Provider.

## Normative requirements

A Provider MUST satisfy the normative semantics of every Capability it claims to provide.

A Provider MUST NOT claim conformance based only on API shape or naming.

A Provider MUST NOT silently weaken guarantees required by the applicable Capability or profile.

A Provider MAY use operating-system mechanisms, external libraries, hardware, remote services, or other Providers internally.

External implementation details MUST NOT become part of a provider-independent Capability contract unless explicitly exposed.

Using a Provider MUST NOT automatically grant lifecycle ownership of a stateful Provider Resource.

A borrowed Provider Resource MUST NOT be stopped or destroyed by the borrower unless lifecycle authority has been explicitly granted or transferred.

A stateful Provider MUST define the observable consequences of shutdown for new and already accepted work when those consequences are relevant.

A Provider MUST NOT silently discard accepted work when its contract promises completion or observable termination.

Provider replacement MUST preserve all semantic guarantees required by the consumer.

Provider fallback MUST NOT silently weaken required observable guarantees.

A Provider claiming stronger guarantees than the baseline Capability MUST actually preserve those stronger guarantees.

Provider-created Resource ownership MUST be defined by the applicable contract and MUST NOT be inferred solely from creation.

Provider-dependent Resources MUST NOT silently outlive the Provider lifetime if their validity depends on it.

A Provider MUST preserve applicable Operation cancellation, Deadline, ownership, and completion semantics across its implementation boundary.

Provider-specific Failures SHOULD be represented structurally.

Higher-level Providers SHOULD preserve meaningful causal information when translating lower-level Failures.

Providers SHOULD expose implementation-specific behavior only where consumers genuinely need to depend on it.

Provider dependency graphs SHOULD remain understandable in terms of semantic Capability requirements rather than unnecessary concrete implementation coupling.

## Open questions

The Standard does not yet define a universal Provider registration, selection, or resolution API.

It also does not define whether some Provider categories should eventually share common C++ concepts or interfaces.

Those decisions should emerge from repeated requirements across real Capabilities.

The Standard intentionally does not require:

```text id="g6vxxf"
IProvider
ProviderRegistry
GlobalRuntime
ServiceLocator
DependencyContainer
```

as foundational abstractions.

A future Vix Evolution Proposal may standardize narrower Provider-discovery or selection mechanisms if actual cross-domain usage justifies them.

Provider health, dynamic replacement, migration, and stronger profile-specific guarantees may also require future specialized specifications.

## Design consequence

A Capability should remain stable enough that software can reason about what it needs.

A Provider should remain replaceable enough that Vix can improve how that need is satisfied.

The Provider boundary should therefore answer:

> Which implementation is responsible for satisfying this Capability here, and what guarantees does that implementation actually provide?

It should not answer:

> Which implementation detail must every caller now depend on?

The purpose of Providers is to connect stable semantics to replaceable mechanisms.

If replacing the mechanism requires rewriting the meaning of the software above it, the Provider boundary has failed to do its job.
