# Principles

The Vix Standard is built around a small number of principles intended to keep the platform coherent as it grows.

These principles are not tied to a particular implementation of Vix. They are meant to survive changes in modules, runtimes, providers, APIs, operating systems, and internal architecture. Their role is to provide a stable way to reason about software built with Vix.

The Standard should make it possible to change how Vix works internally without constantly changing what Vix means to the programmer.

## One concept should have one meaning

A foundational concept should preserve the same meaning wherever it appears in Vix.

An operation should not mean one thing in networking and another thing in database code. Cancellation should not behave according to unrelated models in Web, process execution, filesystem operations, and asynchronous I/O. Ownership should not depend on which module happens to hold a pointer.

Different domains may expose different APIs and may impose additional rules, but they should build on the same foundational semantics.

This does not mean that every domain must behave identically. A network request and a database transaction are different operations with different contracts. The principle is that when they use the same foundational concept, such as a deadline, failure, borrowed resource, or cancellation request, the concept should not silently change its meaning.

A programmer should be able to learn a foundational idea once and reuse that understanding throughout Vix.

## Intent should be separated from mechanism

Vix should describe what software is trying to accomplish independently from the mechanism used to accomplish it.

Resolving a hostname is an operation. Whether that operation is implemented with an operating-system resolver, an asynchronous library, a dedicated thread, or another mechanism is an implementation decision.

Reading from a socket is an operation. It should not become conceptually defined by `epoll`, IOCP, a coroutine implementation, a thread pool, or a particular event loop.

This separation allows Vix to evolve internally without forcing higher-level APIs to depend on infrastructure details they do not need.

It also allows the same semantic model to survive across platforms that may use very different execution mechanisms.

Implementation mechanisms are important, but they should not become the definition of the operation itself.

## Access does not imply ownership

Being able to access a resource does not automatically grant authority over its lifecycle.

A component may hold a pointer, reference, handle, or shared ownership object and still only be borrowing the underlying resource.

The right to use a resource and the right to stop, close, release, or destroy it are separate concerns.

This distinction should remain explicit in Vix contracts.

A borrowed execution context must not be stopped simply because a component retains a reference to it. A borrowed connection must not be closed unless the contract grants that authority. A server using an externally supplied resource must not silently assume that it owns the resource's lifecycle.

Ownership follows the semantic contract, not the memory-management representation.

## Resource, Operation, and Activity are distinct concepts

Vix should distinguish between something that exists, something being attempted, and something that continues over time.

A Resource represents something whose validity and lifetime matter.

An Operation represents finite work that eventually reaches one terminal completion.

An Activity represents continuing behavior that may produce multiple events before terminating.

These concepts often appear together, but they should not be collapsed into one abstraction.

A WebSocket connection may be a Resource. Establishing the connection may be an Operation. The ongoing exchange of messages may form an Activity. Sending an individual message may again be an Operation.

A server may be a Resource supporting a long-running Activity, while each request handled by that server is a separate Operation.

Keeping these concepts distinct allows Vix to model both finite and long-lived behavior without forcing everything into the same abstraction.

## Operations complete once

A logical Operation represents one finite attempt and eventually reaches one terminal completion.

Internal implementations may retry, schedule multiple steps, create child operations, suspend, resume, or move execution between different mechanisms. None of those internal transitions should create multiple terminal completions for the same logical operation.

A caller should be able to reason about an operation as something that begins, progresses, and eventually finishes once.

Progress notifications, cancellation requests, deadline expiration, internal retries, and intermediate events are not terminal completions by themselves.

The completion contract should remain stable even when the implementation becomes more complex.

## Failure is not the same as a programming fault

Vix should distinguish operational failure from defects in the program itself.

An operational Failure occurs when a valid operation cannot satisfy its contract because of a condition the program may reasonably encounter. A missing file, a refused network connection, unavailable storage, malformed external data, or denied permission may all be operational failures depending on the applicable contract.

A Fault indicates a broken programming contract or invariant.

Reaching an impossible internal state, violating a required precondition, corrupting internal state, or otherwise breaking an invariant should not automatically be treated as ordinary application control flow.

This distinction keeps error handling meaningful.

Programs should be able to handle expected operational failures without being required to treat programming defects as if they were routine environmental conditions.

## Errors should be structured, not identified by text

An Error should carry stable programmatic meaning independently from its human-readable message.

Diagnostic text exists for people. Error identity exists for program logic.

Changing the wording of an error message should not change the meaning of the error or break code that handles it.

Higher layers should also be able to add context without destroying the identity of the underlying failure.

For example, a low-level connection refusal may gain context explaining that it occurred while contacting an HTTP endpoint, which may in turn gain context explaining that the application was loading a user profile.

Each layer may improve the diagnostic explanation while preserving the underlying cause.

This allows Vix to provide rich diagnostics without making diagnostic strings part of the programmatic contract.

## Cancellation is a request, not a completion

Requesting cancellation does not mean that an operation has already stopped.

An operation may need time to observe the request. It may already be completing. It may perform cleanup before termination. It may even complete successfully before cancellation becomes effective.

The control request and the terminal outcome must therefore remain separate concepts.

This distinction is especially important when cancellation races with successful completion.

The existence of a cancellation request must not allow an implementation to report multiple terminal outcomes or retroactively redefine an already completed operation.

Cancellation should also not imply that no partial work or external side effects occurred before termination.

The contract of the operation determines what remains observable after cancellation.

## Lifetime should be identifiable

Asynchronous and long-running work should have an identifiable lifetime relationship.

Work should not silently outlive the resources or state that it continues to use.

When a child operation belongs to a parent operation, their lifetime relationship should be understandable. When work intentionally becomes independent, that independence should be explicit in the contract.

Vix should avoid designs where detached work appears accidentally as a side effect of implementation details.

This does not require one particular structured-concurrency API. It requires that lifetime relationships remain visible enough to reason about safely.

## Independent lifetime should be explicit

Some work legitimately needs to survive the operation or activity that created it.

Background processing, durable delivery, external services, long-running subscriptions, or system-level orchestration may intentionally have independent lifetimes.

The Standard should allow such designs.

What it should avoid is accidental independence.

If work becomes detached from its initiating lifetime, that transition should be explicit in the API or contract. The programmer should be able to recognize that the work is now governed by a different owner or lifetime.

Explicit detachment makes resource use, shutdown, cancellation, and failure behavior easier to reason about.

## Time should have coherent semantics

Vix should not treat time as a collection of unrelated timeout integers scattered across modules.

Duration, clock, deadline, delay, and timeout represent different ideas and should remain distinct.

Operational time measurement should use a monotonic clock when appropriate and available. A change to civil time, timezone, or wall-clock synchronization should not unexpectedly extend or shorten an internal operation deadline.

A Deadline represents a temporal constraint. Reaching the deadline does not automatically determine whether an operation must report Failure, Cancelled completion, a normal result, or another outcome.

That behavior belongs to the operation contract.

The important principle is that the underlying time concepts remain consistent across Vix.

## Parent constraints should not silently expand in child work

When a higher-level operation has a bounded lifetime, composing lower-level operations should not silently extend that bound.

For example, an HTTP request with a five-second overall deadline should not automatically allow five seconds for DNS resolution, another five seconds for connection establishment, another five seconds for TLS negotiation, and another five seconds for reading the response.

Child operations that participate in the same bounded lifetime should respect the effective constraints inherited from their parent unless the contract explicitly defines otherwise.

This principle applies to time, but it also reflects a broader rule about composition: higher-level constraints should not disappear simply because work is delegated to lower-level components.

## Execution is not the same as the operation

Vix should distinguish the work being performed from the mechanism that progresses the work.

An operation may execute synchronously on the calling thread, through an event loop, through a thread pool, through an operating-system asynchronous I/O facility, through a coroutine runtime, or through another provider.

Those mechanisms are forms of execution.

They are not the semantic identity of the operation.

This distinction allows an operation to keep the same meaning even when its execution mechanism changes across platforms or implementations.

It also prevents domain APIs from depending on concrete executor implementations when they do not require those implementations directly.

## Execution, scheduling, and lifecycle are separate concerns

Execution describes the progression of work.

Scheduling determines when and where work is allowed to progress.

Lifecycle authority determines who may start, stop, close, or otherwise control the lifetime of the facility providing that execution.

These responsibilities may appear together in one implementation object, but the Standard should not treat them as inherently inseparable.

Using a scheduler does not imply ownership of that scheduler.

Submitting work to an execution facility does not automatically grant authority to stop it.

A component that only needs work to progress should not be forced to depend on unrelated metrics, lifecycle control, scheduling policy, or implementation-specific behavior.

This separation is intended to prevent large execution abstractions from becoming accidental dependencies throughout Vix.

## Capabilities describe needs, providers satisfy them

Software should express what ability it requires without unnecessarily depending on the specific mechanism that supplies that ability.

Filesystem access, process execution, DNS resolution, TCP communication, TLS, monotonic time, asynchronous I/O, and CPU execution are examples of capabilities.

A Provider satisfies one or more capabilities.

The provider may internally rely on an operating system, external library, runtime service, hardware facility, or another provider.

The meaning of the capability should remain stable even if the provider changes.

A dependency on a capability should therefore not become a hidden dependency on a particular vendor library, scheduler, runtime, or operating-system API unless the contract is intentionally provider-specific.

## A capability is not necessarily an interface

The capability model exists to describe semantic requirements, not to force Vix into a dependency-injection framework.

A capability may be represented through a type, free function, C++ concept, runtime object, service, platform facility, or another mechanism.

Some capabilities may have no meaningful runtime object at all.

The Standard should not require an `ICapability` interface, a universal service locator, or a global context object simply because capabilities exist conceptually.

The abstraction should only appear in code when the implementation actually benefits from it.

The semantic model should guide implementation rather than manufacture unnecessary abstraction.

## Availability does not imply authorization

A platform may provide a capability while a particular operation remains unauthorized to use it.

Filesystem capability does not imply permission to access every path. Camera capability does not imply that the user granted camera access. Networking capability does not imply that every remote destination is permitted.

Capability availability describes what the environment is able to support.

Authorization describes whether a particular use is permitted.

The two concepts should not be confused.

## Platform differences should be expressed through capabilities and profiles

Vix is expected to operate in environments with different facilities.

A native system may provide processes, threads, raw sockets, and a traditional filesystem. A WebAssembly environment may provide browser-managed networking, an event loop, and constrained storage. An embedded system may expose a smaller or more specialized set of facilities.

The Standard should not pretend these platforms are identical.

Instead, platforms should differ in the capabilities and profiles they provide while preserving the meaning of concepts they share.

A Resource should not mean something fundamentally different merely because the implementation runs in a browser. Cancellation should not become a different concept on Windows. Deadline semantics should not depend on whether the underlying event mechanism is `epoll`, IOCP, or a browser event loop.

Portability means stable semantics across varying capability sets, not pretending every platform has the same infrastructure.

## Composition should preserve semantics

Higher-level functionality in Vix will often be built from lower-level operations and resources.

An HTTP request may involve DNS resolution, connection establishment, TLS negotiation, writing, reading, timers, and cancellation. A database operation may use a connection pool, network transport, serialization, and retry policy.

Composition should not erase the semantic contracts of those lower layers.

Ownership should remain understandable. Cancellation should propagate according to lifetime relationships. Deadlines should respect parent constraints. Errors should gain context without losing their underlying identity. Resources should be released according to their ownership rules.

The purpose of composition is to make higher-level behavior simpler to use, not to make lower-level semantics unpredictable.

## Internal retries should not change the logical contract

An implementation may perform several internal attempts while exposing one logical Operation.

A connection strategy may try more than one address. A request policy may retry a temporary failure. A storage provider may attempt recovery before returning to the caller.

These internal attempts do not automatically become separate logical operations from the caller's perspective.

The outer operation still has one terminal completion.

Retry behavior should also respect the operation's applicable lifetime, cancellation, and deadline constraints rather than silently resetting them for every attempt.

## Domain specialization should extend, not redefine

Web, networking, data, realtime systems, game systems, process execution, and other domains may introduce concepts that do not belong in the foundational Standard.

That is expected.

A domain specification may define requests, routes, transactions, channels, entities, sessions, or other specialized behavior.

What it should not do is redefine a foundational concept for local convenience.

A Web specialization may define how a request uses cancellation, but it should not create a different meaning of cancellation.

A database specialization may define transaction-specific failure categories, but it should not redefine the distinction between operational Failure and programming Fault.

Specialization should add meaning at the domain level while preserving the common semantic foundation.

## Stable semantics should allow replaceable implementations

The Standard exists partly so that implementation work can remain aggressive without destabilizing the programming model.

Schedulers may be replaced. Runtime internals may be redesigned. Providers may change. Modules may be split or merged. Different platforms may use entirely different system mechanisms.

Those changes should remain possible when the observable semantic contract does not change.

Vix should therefore prefer stable concepts and replaceable mechanisms over APIs whose meaning depends too heavily on the current internal architecture.

This principle allows the implementation to improve without forcing programmers to relearn foundational concepts every time the internals evolve.

## The simplest correct API is preferred

A precise semantic model should make Vix easier to use, not expose every internal concept directly to the programmer.

The Standard may discuss ownership, lifetime, capabilities, execution, cancellation, deadlines, composition, and providers in detail because these concepts need precise contracts.

That does not mean ordinary APIs should require programmers to configure all of them manually.

A straightforward operation should remain straightforward to express.

The implementation should carry as much of the complexity as it reasonably can while preserving visible behavior that is predictable and explicit where it matters.

The goal is not to create abstraction for its own sake. The goal is to make simple code mean something precise.

## Semantic rules should precede representation

When Vix standardizes a concept, it should first decide what the concept means before deciding how that concept is represented in C++.

The Standard should not conclude that a concept has a particular meaning merely because the current implementation already has a type with a convenient name.

`Result<T>`, task types, execution classes, resource handles, schedulers, and cancellation objects are implementation or API representations. They may eventually become canonical representations of Standard semantics, but the semantic decision should come first.

This keeps the Standard from becoming a frozen snapshot of historical implementation choices.

## Local convenience should not create semantic divergence

A subsystem should not redefine a foundational concept merely because doing so is convenient locally.

Small local inconsistencies eventually become global complexity.

If a domain needs behavior that appears incompatible with an existing Standard concept, the difference should be examined explicitly. It may represent a legitimate specialization, a missing concept in the Standard, or a reason to evolve the Standard itself.

Silent divergence should not be the default solution.

Vix should prefer one coherent programming model over several locally convenient but incompatible ones.

## The Standard should remain smaller than the platform

Not every useful idea belongs in the Standard.

The Standard should contain the concepts whose shared meaning is important for understanding and composing Vix software.

Repository layout, implementation algorithms, vendor choices, domain protocols, specialized APIs, and short-lived architectural decisions should remain outside it unless they expose semantics that truly need cross-domain stability.

This restraint is part of the design.

A Standard that attempts to describe everything eventually becomes another implementation manual. A Standard that defines the small set of meanings that must remain stable can continue to guide the platform even as most of the implementation changes.

## Long-term principle

The central objective of these principles is that Vix should become easier to understand as it grows, not harder.

New domains should reuse existing concepts where those concepts already apply. New providers should be able to replace old ones without redefining the software built on top of them. New platforms should expose their real capabilities without fragmenting shared semantics.

A programmer who understands the foundational model should be able to carry that understanding from one part of Vix to another.

That continuity is what the Standard is intended to preserve.
