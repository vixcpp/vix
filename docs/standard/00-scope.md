# Scope

The Vix Standard defines the common semantics that should remain stable across the Vix ecosystem.

Its scope is intentionally narrower than the full Vix platform. The Standard does not attempt to describe every subsystem, library, protocol, implementation detail or public API. It focuses on the concepts that need a shared meaning because they are used across multiple domains and because inconsistencies in those concepts would make Vix harder to understand, compose and evolve.

The Standard therefore concentrates on foundational ideas such as resources, ownership, operations, activities, completion, failure, cancellation, time, execution, capabilities, providers and composition.

These concepts are broad enough to appear in many different areas of the system. A filesystem operation, a database query, a network request and a process launch may have different domain-specific behavior, but they still need compatible notions of lifetime, failure, cancellation, deadlines and completion.

The purpose of the Standard is to define those shared semantics without forcing unrelated domains to expose identical APIs.

A networking library does not need to look like a database library. A WebSocket connection does not need to expose the same interface as a file. A process does not need to be represented as an asynchronous task. What matters is that when these domains use concepts such as ownership, failure or cancellation, those concepts do not silently change meaning.

## What belongs in the Standard

A concept belongs in the Vix Standard when its meaning is expected to remain coherent across domains, implementations or platforms.

The Standard may therefore define the semantics of concepts such as:

- resources and their lifetime;
- ownership, borrowing and transfer;
- finite operations and long-lived activities;
- terminal completion;
- operational failure and programming faults;
- cancellation requests and cancelled completion;
- duration, clocks, deadlines and timeout behavior;
- execution and scheduling responsibilities;
- capabilities and providers;
- composition of operations and lifetimes;
- conformance and platform profiles.

The Standard may also define invariants between those concepts. For example, borrowing a resource does not automatically grant authority over its lifecycle, and requesting cancellation does not by itself mean that an operation has already completed.

These rules are part of the Standard because changing them in one subsystem would affect how programmers reason about Vix as a whole.

## What does not belong in the Standard

The Standard does not define the internal organization of the Vix repository.

Names such as `core`, `async`, `threadpool`, `websocket`, `net`, `error` or any future module name describe implementation organization. They may change as the architecture evolves. The Standard must remain meaningful even if those modules are renamed, split, merged or removed.

The Standard also does not define the concrete dependency graph between modules. Questions such as which package owns an implementation, which target links another target, or which directory contains a provider belong to the architecture of Vix rather than its semantic model.

The Standard does not prescribe implementation technologies either. It does not require Asio, OpenSSL, a particular scheduler, a particular thread pool, a particular event loop or a particular operating-system facility. Those mechanisms may be used by conforming implementations, but they do not define the meaning of Vix concepts.

For the same reason, an external library is not part of the Standard simply because Vix currently depends on it.

## The Standard is not the API reference

The Vix Standard and the Vix API reference serve different purposes.

The API reference describes the concrete C++ interface available to programmers. It documents namespaces, classes, functions, templates, constructors, overloads and exact signatures.

The Standard describes what those interfaces mean.

For example, Vix may expose a concrete `Result<T>` type as part of its public API. The Standard may define the semantics of operational success and failure that such a type represents, but it should not assume that every semantic concept must correspond directly to a particular C++ type.

Likewise, the Standard may define the concept of cancellation without requiring every cancellable API to expose the same function name or implementation mechanism.

This separation allows the API to evolve while preserving semantic compatibility.

## The Standard is not the architecture

The architecture describes how Vix implements the Standard.

It decides where responsibilities live, which modules implement which capabilities, how dependency direction is enforced, and how the platform is decomposed internally.

The Standard should not encode those decisions as permanent semantic rules.

For example, the Standard may define an asynchronous I/O capability. The architecture may decide that a particular runtime module currently provides that capability. A future implementation may provide it differently without requiring a change to the Standard as long as the same semantic contract is preserved.

This distinction is essential because architectural change should be possible without redefining the programming model.

## The Standard is not a domain specification

Domain-specific behavior does not automatically belong in the foundational Standard.

HTTP routing, WebSocket framing, SQL behavior, ORM relationships, realtime channels, game entities, user interfaces and agent protocols are examples of concepts that belong to their respective domains.

Those domains may depend on the Standard and reuse its concepts.

A Web request may be an operation. A WebSocket connection may be a resource supporting a long-lived activity. A database query may have a deadline and may fail operationally. These relationships allow domain specifications to build on the common model without moving every domain concept into the Standard itself.

When a domain requires additional rules, those rules should normally live in a domain specification rather than redefine a foundational concept.

## The Standard does not require one representation for every concept

The semantic model should not be confused with an object model.

A Resource does not require a base class named `Resource`. A Capability does not require an `ICapability` interface. An Operation does not require a class called `Operation<T>`. Ownership does not require every API to wrap references in a special ownership object.

A conforming implementation may use types, templates, free functions, coroutines, concepts, handles, runtime objects or other mechanisms as appropriate.

The important requirement is that the observable semantics remain consistent with the Standard.

This allows Vix to keep simple APIs even when the model behind them is precise.

## Platform differences

The Standard is intended to support different execution environments without pretending that every environment provides the same facilities.

A native operating system may provide processes, threads, raw sockets and a traditional filesystem. A WebAssembly environment may provide a different execution model and a different set of available capabilities. An embedded environment may expose an even smaller subset.

These differences should be expressed through capabilities, providers and profiles rather than by changing the meaning of shared concepts.

If two platforms both expose a Vix deadline, the meaning of that deadline should remain coherent. If both expose a resource ownership contract, borrowing should not mean one thing on one platform and something incompatible on another.

Platform diversity is therefore expected. Semantic contradiction is not.

## Conformance boundary

The Standard defines the semantics required for conformance, but conformance does not necessarily mean that every implementation supports every possible capability.

An implementation can conform to a defined profile and guarantee the semantics required by that profile.

The exact profile system is defined separately, but the principle is that missing capabilities and failed operations are different situations.

If a profile guarantees filesystem support and an implementation does not provide it, that is a conformance problem.

If filesystem support exists but opening a particular file fails because the file does not exist or permission is denied, that is an operational failure.

The Standard treats these as different categories because they occur at different layers of the system.

## Evolution of scope

The scope of the Standard may evolve, but new concepts should not be added merely because they exist in the implementation.

A concept should enter the Standard when Vix needs a stable cross-domain meaning for it.

Implementation convenience, module ownership or current repository structure are not sufficient reasons to make something normative.

Larger additions or changes to the semantic model should be developed through the Vix evolution process before becoming part of the Standard.

This allows the Standard to grow deliberately rather than becoming a mirror of every feature Vix accumulates.

## Guiding boundary

A useful question when deciding whether something belongs in the Standard is:

> Would inconsistent meanings for this concept across Vix make the programming model harder to understand or compose?

If the answer is yes, the concept may belong in the Standard.

If the question is instead about where code lives, which library implements it, which class exposes it, or which domain-specific behavior it provides, the subject probably belongs in architecture, reference documentation or a domain specification.

The Vix Standard exists to stabilize meaning, not implementation.
