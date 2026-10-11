# Vix Standard

The Vix Standard defines the common semantic model used across Vix.

Its purpose is not to describe the internal layout of the repository, document every module, or freeze the current implementation. The Standard exists to define the concepts that should keep the same meaning wherever they appear in Vix, regardless of the domain, platform, runtime implementation, or library used underneath.

Vix has grown across several areas such as asynchronous execution, networking, Web, processes, files, time, errors, data, realtime systems and developer tooling. As those areas evolved independently, some concepts began to overlap or acquire slightly different meanings depending on where they were used. Execution, cancellation, ownership, timeouts, errors and lifecycle are examples of concepts that can easily become fragmented when each subsystem defines them locally.

The Standard is intended to prevent that fragmentation.

Instead of defining Vix through its modules, the Standard defines a shared programming model. Concepts such as resources, operations, activities, capabilities, failures, deadlines and ownership are described independently from the code that currently implements them. A module may change, disappear, be split, or be replaced without changing the meaning of the concept it implements.

This distinction is important because Vix is expected to evolve over a long period of time. The implementation should remain free to improve while the semantic model remains understandable and predictable to users.

For example, an operation should not mean one thing in networking and another thing in database code. Borrowing a resource should not give lifecycle authority in one subsystem while silently implying ownership in another. A deadline should not acquire unrelated semantics simply because it is used by HTTP rather than a process or filesystem operation.

The goal is not to make every API identical. Different domains have different needs. The goal is to make the concepts beneath those APIs consistent.

The Standard therefore focuses on semantics rather than implementation mechanisms. It describes what concepts mean, how they relate to one another, and which guarantees conforming Vix implementations are expected to preserve. It does not require every concept to correspond to a class, interface, runtime object, module, or C++ type.

A capability, for example, describes an ability that software may require, such as filesystem access, monotonic time, asynchronous I/O, DNS resolution or process execution. The provider that satisfies that capability may differ across systems. Linux, Windows, WebAssembly, embedded environments and future platforms may use very different mechanisms while still preserving the same Vix semantics where the same capability exists.

The same principle applies to execution. The Standard does not define Vix in terms of a specific executor, thread pool, coroutine implementation or operating-system event loop. It distinguishes the work being attempted from the mechanism used to make that work progress. This allows execution implementations to change without forcing domain APIs to depend on infrastructure details they do not actually need.

The Standard also establishes a common model for lifetime and ownership. Holding a pointer, reference, shared handle or other form of access to a resource does not automatically grant authority over that resource's lifecycle. A component may borrow a resource and use it without having the right to stop, close or destroy it. Ownership and lifecycle authority must follow the contract of the resource rather than the incidental representation used by the implementation.

Operations are another central part of the model. An operation represents finite work that eventually reaches one terminal completion. Activities represent behavior that may continue over time and produce multiple events before terminating. Resources represent things whose validity and lifetime matter. These concepts are related, but they are deliberately kept distinct so that long-lived systems such as servers, streams, subscriptions and WebSocket connections do not have to be forced into the same model as a single read, write or request.

Failure, cancellation and time are treated in the same way. An operational failure is not the same thing as a programming fault. A cancellation request is not proof that an operation has already terminated. Reaching a deadline does not automatically determine whether an operation must report failure, cancellation or another outcome. These distinctions allow the Standard to define stable semantics while leaving domain-specific contracts enough room to express their actual behavior.

The Standard is also not the API reference.

The API reference describes the concrete C++ surface of Vix: namespaces, types, functions, constructors and signatures. The Standard describes the semantics those APIs are expected to preserve. A type such as `Result<T>` may be used by the implementation to represent part of the Standard's failure model, but the Standard does not begin by assuming that the current representation is permanently correct.

Likewise, the Standard is not the architecture document. Architecture defines where responsibilities live in the implementation, how modules depend on one another, and which components currently implement particular capabilities. Those decisions may evolve more frequently than the Standard itself.

The relationship is roughly:

```text
Standard
    defines meaning and semantic guarantees

Architecture
    assigns implementation responsibilities

Reference
    documents the concrete C++ API
```

Changes to the Standard should be deliberate because they change the meaning of Vix rather than merely its implementation. Larger semantic changes are expected to be developed through Vix Evolution Proposals before becoming normative parts of the Standard. The proposal records the problem, alternatives, reasoning and migration consequences. The Standard records the accepted result.

The documents in this directory build the Standard progressively. They begin by defining its scope, terminology and foundational principles, then describe resources and ownership, operations, activities, failure, cancellation, time, execution, capabilities, composition and conformance.

The Standard should remain small enough to understand as a coherent model. Domain-specific concepts such as HTTP routing, WebSocket protocol behavior, database schemas, ORM semantics or game systems belong in their own specifications and APIs. Those domains may build on the Standard, but they should not redefine its foundational concepts locally.

The long-term objective is simple: learning a concept once in Vix should make that concept understandable everywhere else in Vix.

The implementation may evolve. Providers may change. Modules may be reorganized. New domains may appear. The shared semantic model should remain coherent.
