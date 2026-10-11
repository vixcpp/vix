# Vix Architecture

The Vix Architecture describes how the Vix implementation is organized to realize the contracts defined by the Vix Standard.

The Standard defines what Vix concepts mean.

Architecture defines where responsibilities live, how major parts depend on one another, which components provide foundational capabilities, and which implementation boundaries should remain stable enough for the codebase to stay understandable as it evolves.

Architecture is therefore concerned with responsibility and dependency.

It is not the source of Vix semantics.

If the architecture and the Standard appear to disagree about the meaning of a foundational concept, the Standard is authoritative and the implementation should be reviewed.

## Relationship with the Standard

The Vix Standard defines concepts such as Resources, Operations, Activities, ownership, cancellation, deadlines, capabilities, Providers, and conformance.

Those concepts do not necessarily correspond one-to-one with modules or C++ classes.

For example, the Standard defines an Operation as finite logical work with one terminal completion.

That does not require a module named `operation`, a universal `Operation` base class, or one specific asynchronous representation.

Architecture decides how Vix realizes that semantic model using actual code.

The relationship is:

```text
Standard
    defines meaning

Architecture
    assigns responsibility

Public API / Reference
    defines concrete programming interfaces

Implementation
    realizes those interfaces and responsibilities
```

These layers should remain distinguishable.

A module layout is not a Standard.

A C++ type is not automatically a foundational concept.

A historical implementation dependency is not automatically an architectural requirement.

## What Architecture answers

Architecture should answer questions such as:

> Which part of Vix owns this responsibility?

> Which Capability does this component require?

> Which Provider satisfies that Capability?

> Which Resources are owned here and which are borrowed?

> Which dependencies are foundational and which are domain-specific?

> Where does Platform end and Ecosystem begin?

> Which historical abstractions can be removed because their responsibilities now belong elsewhere?

Architecture should make these answers understandable without requiring a programmer to reconstruct them from include graphs or build files.

## What Architecture does not define

Architecture does not redefine the semantic contracts established by the Standard.

For example, this documentation may state that a particular module provides CPU execution capability.

It must not invent a different meaning of execution.

It may state that a server borrows an execution Resource.

It must not redefine borrowing so that the server gains lifecycle authority merely because the implementation stores a `shared_ptr`.

Architecture also does not need to freeze every internal class or source file.

Private implementation details should remain free to change when their responsibility and observable behavior remain stable.

## Platform and Ecosystem

Vix is organized conceptually into Platform and Ecosystem.

The Platform provides the foundational environment on which higher-level Vix software is built.

The Ecosystem contains domain-oriented functionality that uses the Platform.

Conceptually:

```text
Vix
│
├── Platform
│   ├── Toolchain
│   └── Runtime
│
└── Ecosystem
    ├── Web
    ├── Database
    ├── WebSocket
    ├── Realtime
    ├── Game
    ├── Agent
    └── other domains
```

The boundary is semantic rather than merely organizational.

A component belongs in the Platform when it provides broadly reusable infrastructure required across domains or when it participates directly in implementing foundational Vix capabilities.

A component belongs in the Ecosystem when it builds domain behavior on top of those foundations.

This distinction prevents domain-specific abstractions from gradually becoming universal runtime dependencies.

## Runtime Foundation

The Runtime Foundation contains responsibilities that are broadly shared across Vix.

These include areas such as:

```text
Error and Result
Path and Filesystem
Environment
Time
Logging
Asynchronous execution
Cancellation
general execution capabilities
```

The exact module structure may evolve.

The important architectural property is that foundational responsibilities have clear ownership and do not need to be reimplemented independently by every domain.

For example, WebSocket should not invent a second cancellation model.

Database code should not define an unrelated deadline model.

Game systems should not redefine ownership merely because their execution pattern differs.

Domains may specialize behavior, but the foundational semantics remain shared.

## Capabilities before concrete dependencies

Architecture should describe dependencies in terms of the actual capabilities a component requires.

For example, a WebSocket implementation may require asynchronous I/O progression.

That does not automatically mean that WebSocket should depend on a particular historical runtime executor.

A subsystem that requires CPU execution does not necessarily require the entire implementation object that currently owns a worker pool.

A component should depend on the smallest coherent capability necessary to satisfy its contract.

This allows Providers to change without propagating implementation dependencies throughout the system.

## Providers

Providers connect Vix capabilities to concrete mechanisms.

A Provider may use operating-system APIs, external libraries, hardware, or other Vix facilities.

For example:

```text
TLS Capability
    ↓
TLS Provider
    ↓
external TLS library or platform facility
```

Architecture documents where these responsibilities live and how Providers depend on lower-level capabilities.

Higher-level domains should generally depend on the capability contract rather than unnecessarily binding themselves to one Provider implementation.

Provider details are described further in `providers.md`.

## Ownership as architecture

Ownership is not only a C++ memory-management concern.

It is an architectural relationship.

When a component receives a Resource from another owner, architecture should make clear whether the Resource is:

```text
owned
borrowed
adopted
transferred
shared under explicit lifecycle authority
```

This is especially important for execution infrastructure, servers, connections, Providers, and long-lived Activities.

For example:

```text
Application
    owns Execution Context

Server
    borrows Execution Context
```

The Server may retain and use the execution context.

It must not become responsible for shutting down the broader execution infrastructure unless lifecycle authority has explicitly been transferred.

These relationships are described further in `ownership-map.md`.

## Execution is not one universal subsystem

Vix does not assume that all execution should be routed through one universal Executor.

Different work has different execution requirements.

Examples include:

```text
asynchronous I/O progression
CPU-bound work
blocking work
timers
event-loop progression
```

A single implementation may provide several of these capabilities, but architecture should not collapse them into one dependency merely because historical code happened to do so.

The execution architecture is described in `execution.md`.

## Modules are implementation boundaries

Modules are useful implementation boundaries.

They provide code ownership, build isolation, dependency direction, testing boundaries, and packaging structure.

They are not automatically semantic boundaries.

For example, the fact that a concept currently lives in a module called `core` does not make `core` a foundational Standard concept.

Likewise, moving functionality from one module to another does not necessarily change Vix semantics.

Architecture should be willing to dissolve historical modules when their responsibilities are clearer elsewhere.

The module graph should follow the architecture rather than define it accidentally.

## Historical abstractions

Vix contains abstractions created during earlier stages of its development.

Some remain useful.

Others may combine responsibilities that the current architecture now separates.

Examples may include historical aggregation modules, broad runtime objects, compatibility networking layers, or utility collections.

Architecture documentation should describe the intended destination rather than treating every historical abstraction as permanent.

Migration may therefore follow a pattern such as:

```text
historical abstraction
    ↓ audit responsibility
    ↓ identify actual required capabilities
    ↓ move responsibility to canonical owner
    ↓ migrate consumers
    ↓ retain compatibility only when necessary
    ↓ eventually remove historical dependency
```

The goal is not removal for its own sake.

The goal is a system whose dependencies correspond to real responsibilities.

## Architecture and migration

Architecture should guide migration in small verifiable steps.

A large subsystem does not need to be rewritten simply because its current dependency graph is imperfect.

Instead, individual responsibilities can be separated progressively.

For example, if a component receives an execution object but never uses execution-specific behavior, the dependency may be removable without inventing a replacement abstraction.

If another component uses only asynchronous I/O, it may depend directly on the corresponding capability rather than on a broad historical runtime.

Migration should reduce conceptual dependencies rather than merely rename them.

## Architecture and public API

Architecture may influence public API design, but the two are not identical.

A runtime may internally use several Providers while presenting a simple public interface.

A public API may represent a Standard concept without exposing the internal module that implements it.

For example, a future canonical Deadline type might be implemented by the Time foundation while being used naturally throughout networking, Web, process execution, and storage.

Programmers should not need to understand internal ownership boundaries merely to perform ordinary operations.

Architecture should carry complexity where possible while preserving explicit semantics where the caller must make a decision.

## Architecture and external libraries

External libraries are implementation tools.

They should not become architectural concepts merely because Vix currently depends on them.

For example:

```text
Asio
OpenSSL
SQLite
operating-system APIs
```

may participate in Provider implementations.

Architecture should describe the Vix responsibility first.

The external mechanism should appear underneath that responsibility.

This makes replacement possible and keeps the Vix programming model independent from vendor-specific structures where appropriate.

## Dependency direction

Dependencies should generally flow from domain-specific behavior toward foundational capabilities.

Conceptually:

```text
Ecosystem
    ↓
Runtime capabilities
    ↓
Providers
    ↓
platform mechanisms / external libraries
```

A foundational layer should not depend on a higher-level domain merely because one implementation currently lives there.

Circular responsibility between foundational and domain layers should be treated as a sign that ownership may be unclear.

Not every source-level dependency is automatically invalid, but architectural dependency direction should remain understandable.

## One responsibility, one canonical owner

Where a responsibility is foundational, Vix should aim for one canonical owner.

This does not mean one implementation object.

Several Providers may satisfy one Capability.

Several modules may consume one semantic primitive.

The rule concerns authority over the definition and primary implementation responsibility.

For example, if Time owns canonical Duration and Deadline primitives, Web should use them rather than maintaining a competing time model.

If Error owns the canonical structured Result model, domains may define domain-specific Errors without inventing unrelated foundational Result semantics.

Canonical ownership reduces fragmentation.

## Avoiding architecture by accumulation

Vix should not create a new architectural layer merely because several components can be grouped under a broad name.

Names such as:

```text
Core
Runtime
Manager
Context
Engine
Services
```

can become containers for unrelated responsibilities if their boundaries are not defined precisely.

Architecture should prefer explicit responsibility over broad aggregation.

A component should exist because it owns a coherent role, not because the system needed somewhere to put code that did not fit elsewhere.

## Architecture should remain revisable

Unlike the foundational semantics of the Standard, internal architecture is expected to evolve more frequently.

A Capability may move to a different module.

A Provider may be replaced.

A historical compatibility layer may disappear.

Several modules may merge or one may split.

These changes are valid when the same Standard contracts remain preserved.

Architecture documentation should therefore describe current intended responsibility, not pretend that every directory boundary is permanent.

The stable part is the reasoning.

The physical implementation remains replaceable.

## Documents in this section

The architecture documentation begins with a small set of files.

`platform.md` defines the boundary between the Vix Platform and the Ecosystem and describes the major responsibilities of the Platform.

`runtime-foundation.md` describes the foundational runtime responsibilities and their intended ownership.

`execution.md` maps the Standard execution model onto the implementation architecture.

`capability-map.md` maps important Standard capabilities to their architectural consumers and Providers.

`ownership-map.md` records important lifecycle ownership and borrowing relationships.

`providers.md` describes the concrete Provider layer and its dependency rules.

`ecosystem-boundaries.md` defines what higher-level domains may rely on and which foundational responsibilities they should not reimplement.

These documents may expand as the architecture becomes more concrete.

They should not expand merely to mirror every module in the repository.

## Relationship with VEPs

Architecture may reveal a semantic question that the current Standard does not answer.

When this happens, architecture should not silently invent the missing rule.

The issue should return to research or the Vix Evolution Proposal process.

Conceptually:

```text
Architecture work
    discovers unresolved semantic question
        ↓
Research / VEP
        ↓
Standard decision
        ↓
Architecture resumes under accepted contract
```

This prevents implementation pressure from accidentally redefining the Standard.

By contrast, when the Standard already defines the semantic rule and only implementation responsibility is unclear, the issue remains architectural.

## Relationship with conformance

Architecture is not itself proof of conformance.

A document may correctly state that a component borrows a Resource while the implementation still stops that Resource incorrectly.

Conformance is established through actual behavior.

Architecture provides the structure that should make conforming behavior natural.

Conformance tests verify that the implementation keeps the Standard contract.

A healthy Vix system should therefore align three things:

```text
Standard
    says what must be true

Architecture
    assigns responsibility so it can be true

Implementation and tests
    demonstrate that it is true
```

## Current architectural direction

The current Vix architectural direction is to reduce broad historical dependencies and make responsibilities explicit.

In particular, foundational domains should not depend on historical aggregation modules merely for convenience.

Execution dependencies should reflect actual execution needs.

Borrowed infrastructure should not gain accidental lifecycle ownership.

Platform capabilities should remain independent from the Providers currently used to implement them.

Higher-level Ecosystem components should compose the Runtime foundation instead of recreating their own foundational semantics.

This direction should guide migration without requiring unnecessary rewrites.

## Design principle

The Architecture should make one question easy to answer:

> Where does this responsibility belong in Vix, and what may depend on it?

If the answer requires knowing accidental repository history, ownership is probably still unclear.

If changing one Provider forces unrelated domains to understand that Provider, the boundary is probably too weak.

If several domains independently implement the same foundational semantic concept, responsibility is probably fragmented.

Good architecture should make the Standard easier to implement, not duplicate it.

The Standard provides the meaning.

Architecture gives that meaning a home.
