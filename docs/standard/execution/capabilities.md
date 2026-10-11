# Capabilities

A Capability describes an ability that software may require or an environment may provide.

Capabilities allow Vix to describe requirements independently from the concrete mechanism that satisfies them.

Examples include filesystem access, process execution, TCP communication, DNS resolution, TLS, monotonic time, asynchronous I/O, CPU execution, persistent storage, and other abilities that may exist differently across platforms.

The purpose of the Capability model is to let software express what it needs without unnecessarily depending on how that need is implemented.

A Capability is therefore a semantic concept first.

It is not automatically a class, interface, module, runtime object, service, Provider, or platform.

## Capability and implementation

A Capability describes what can be done.

A Provider describes how that Capability is supplied.

For example:

```text
Capability:
    monotonic time

Possible Provider:
    operating-system steady clock
```

or:

```text
Capability:
    TLS

Possible Providers:
    OpenSSL-backed implementation
    operating-system TLS implementation
    another compatible implementation
```

The meaning of the Capability should remain stable when the Provider changes.

Software that requires TLS should not need to redefine its semantic contract merely because the underlying implementation changes from one TLS library to another.

This separation allows implementations to evolve without forcing higher-level domains to depend on vendor-specific details.

## Capability is not Module

A Vix module may implement one Capability, several Capabilities, part of a Capability, or supporting infrastructure used by multiple Capabilities.

The repository layout does not define the Capability model.

For example, a module named `async` does not mean that “Async” must become one indivisible Capability.

The implementation may provide several distinct abilities such as:

```text
asynchronous I/O progression
timers
TCP
UDP
DNS
coroutine support
```

These may have different consumers and different portability constraints.

The Standard should reason about the abilities software requires rather than the directory in which their current implementation happens to live.

## Capability is not Provider

A Capability and its Provider must remain distinct.

The statement:

```text
HTTP requires DNS
```

describes a possible capability dependency.

The statement:

```text
HTTP requires Asio
```

describes a concrete implementation dependency.

The second may be true for the current implementation without becoming part of the Standard.

A conforming implementation should remain free to satisfy the same DNS semantics through another Provider.

This distinction is central to portability.

## Capability is not Resource

A Capability describes an ability.

A Resource is an instance with a meaningful lifetime.

For example:

```text
TCP Capability
    enables connect Operation
        produces Socket Resource
```

The TCP Capability is not the Socket.

Likewise:

```text
Process Capability
    enables spawn Operation
        produces Process Resource
```

A Capability may be provided by a stateful system whose implementation is itself a Resource, but the two concepts remain different.

## Capability is not Authorization

The existence of a Capability does not mean that every possible use of that Capability is permitted.

An environment may provide filesystem capability while denying access to a particular path.

A device may provide camera capability while the user has denied camera permission.

A platform may provide networking while policy prevents connection to some destination.

The distinction is:

```text
Capability
    can this environment support this class of behavior?

Authorization
    is this particular use permitted?
```

An Operation may therefore fail because authorization is denied even though the required Capability is fully available.

That is different from the Capability itself being absent.

## Required Capability

A Required Capability is an ability that software depends on in order to satisfy a contract.

The requirement may belong to an Operation, Activity, Provider, subsystem, application, or profile.

For example:

```text
DNS lookup Operation
    requires DNS Capability

TCP client
    requires TCP Capability

deadline enforcement
    requires monotonic time and timer capability
```

A requirement describes need.

It does not imply that the current environment satisfies that need.

The distinction between required and available Capabilities is fundamental.

## Available Capability

An Available Capability is one that the applicable environment can provide with the semantics required by the relevant contract.

Availability may be known at build time, configuration time, startup, runtime, or through a combination of these stages.

For example, native process execution may be known to be unavailable for a particular WebAssembly target before the program runs.

Hardware acceleration may only be discoverable at runtime.

A network-related capability may exist generally while some optional transport feature is only available on certain systems.

The Standard does not require one universal discovery mechanism.

## Missing required Capability

When software requires a Capability that the environment cannot provide, the situation is different from an Operation failing while using an available Capability.

For example:

```text
Application requires filesystem Capability

Target profile has no filesystem Capability
```

is an environment, profile, configuration, or conformance mismatch.

By contrast:

```text
Filesystem Capability exists

open("/missing")
    → file_not_found
```

is an operational Failure.

These situations occur at different semantic levels and should not be collapsed into one generic Error model.

## Architectural Capabilities

Some Capabilities are useful primarily for architectural reasoning.

For example:

```text
Web requires asynchronous I/O.
Process execution requires process capability.
```

These requirements may guide dependency direction and implementation ownership without appearing as explicit runtime objects.

Architectural Capabilities help Vix describe the shape of a system without defining that shape in terms of current modules.

This level is particularly useful when reasoning about Platform and domain boundaries.

## Compile-time Capabilities

Some Capability requirements may be expressed through the C++ type system.

For example, an API may require a type that supports a particular semantic contract.

Possible implementation mechanisms include:

```text
C++ concepts
traits
template constraints
overload selection
specific types
```

The Standard does not require one of these mechanisms.

What matters is that the compile-time contract corresponds to a meaningful semantic Capability rather than merely encoding accidental implementation details.

Compile-time checking can prevent invalid combinations before the program runs, but it is not suitable for every form of Capability availability.

## Runtime Capabilities

Some Capabilities can only be determined or selected at runtime.

Examples may include:

```text
GPU available
IPv6 available
camera available
particular hardware feature present
optional persistent storage available
host service available
```

A runtime Capability model may allow software to adapt to the current environment.

The Standard does not require all Capabilities to be dynamically inspectable.

Runtime inspection should exist where the nature of the Capability makes it useful.

## One Capability, several discovery stages

A Capability may participate in more than one stage of discovery.

For example:

```text
compile time
    target supports networking family

startup
    network Provider initialized

runtime
    IPv6 path currently available
```

These observations may relate to the same broader Capability without having identical granularity.

The Standard allows such layering.

What must remain stable is the semantic meaning of each declared Capability and guarantee.

## Capability discovery

The Standard does not define one universal Capability registry.

An implementation may discover Capabilities through:

```text
build configuration
C++ types
platform profile
feature query
Provider initialization
runtime probing
dependency construction
```

depending on the nature of the system.

This flexibility is intentional.

A universal registry would risk turning the Capability model into a global service locator.

The Standard does not require such an architecture.

## No universal Capability object

A Capability does not need a runtime object.

For example, monotonic time may be provided through a function.

Compile-time serialization support may be represented through a C++ concept.

CPU execution may be provided through a stateful Resource.

Filesystem semantics may be represented through a set of functions and Resource types.

These are all valid.

The Standard defines the contract, not the shape of the object graph.

## No universal Context requirement

The Capability model must not be interpreted as requiring one global object containing every available service.

For example, the Standard does not require an API such as:

```text
VixContext
    filesystem()
    network()
    clock()
    executor()
    logger()
    database()
    ...
```

Such a design may be appropriate in a specific architecture, but it is not implied by the Capability model.

A universal Context can hide dependencies, blur ownership, and create accidental coupling between otherwise independent domains.

Capabilities should make dependencies clearer, not less visible.

## Capability granularity

Capabilities should be large enough to describe coherent semantic abilities and small enough to avoid unnecessary coupling.

The Standard does not define one mechanical rule for granularity.

However, extremely fine-grained Capabilities such as:

```text
CanReadOneByte
CanCloseSocket
CanCreateDirectoryEntry
```

would usually provide little architectural value and would make composition difficult to understand.

Likewise, a Capability such as:

```text
EverythingRuntimeCanDo
```

would be too broad to express meaningful requirements.

A useful Capability normally describes a coherent family of semantics.

Examples may include:

```text
filesystem
TCP
DNS
TLS
monotonic time
CPU execution
asynchronous I/O
process execution
```

The exact Capability catalog should evolve from real architectural needs rather than being invented exhaustively in advance.

## Capability composition

Higher-level behavior may require several lower-level Capabilities.

For example:

```text
HTTPS request
    requires
        DNS
        TCP
        TLS
        time
        asynchronous I/O
```

This does not necessarily mean that `HTTPS` itself belongs in the foundational Capability set.

It may instead be a domain-level feature built from lower-level Capabilities.

The Standard should avoid creating a new foundational Capability for every useful composed feature.

Capabilities should describe reusable semantic abilities.

Domain specifications may define higher-level requirements using them.

## Provider dependencies

A Provider may itself require other Capabilities.

For example:

```text
HTTP Provider
    requires DNS
    requires TCP
    may require TLS
```

or:

```text
TLS Provider
    requires transport
    may require cryptographic entropy
    may require time for certificate validation
```

This creates a dependency graph between semantic requirements and implementations.

The graph should remain expressed in terms of the smallest meaningful Capabilities rather than unnecessarily coupling Providers to specific implementation packages.

## Capability resolution

Capability resolution is the process by which required Capabilities are matched with available Providers or platform facilities.

The Standard defines the relationship but does not require one universal resolution system.

Resolution may occur:

```text
statically through the build
through dependency construction
through configuration
through platform selection
through runtime discovery
```

depending on the system.

This means Vix can support both lightweight compile-time systems and dynamic environments without forcing all software through the same dependency-injection mechanism.

## Required and optional Capabilities

Software may require some Capabilities while treating others as optional.

A required Capability is necessary to satisfy the relevant contract.

An optional Capability allows enhanced or alternative behavior when available.

For example:

```text
Application
    requires local storage
    optionally uses GPU acceleration
```

If the required Capability is unavailable, the contract cannot be satisfied.

If the optional Capability is unavailable, the software may fall back to another path.

The fallback itself must still have a valid contract and required Capability set.

## Fallback behavior

A fallback should not silently change fundamental semantics promised to the caller.

For example, falling back from a hardware Provider to a software Provider may change performance while preserving the same result semantics.

That is usually safe.

Falling back from durable storage to volatile memory may fundamentally change persistence guarantees.

That difference must be visible in the applicable contract.

Capability fallback is therefore not merely an implementation choice when the fallback changes observable guarantees.

## Capability strength

Two Providers may satisfy the same general Capability while offering different additional guarantees.

For example:

```text
Timer Provider A
    monotonic timing
    millisecond resolution

Timer Provider B
    monotonic timing
    microsecond resolution
```

Both may satisfy a baseline monotonic-time Capability.

The stronger Provider may expose additional profile or provider-specific guarantees.

The foundational Capability should define the minimum semantic contract necessary for interoperability.

Provider-specific strength should not silently redefine the baseline Capability.

## Capability subsets

A broader Capability may contain optional or separately identifiable sub-capabilities.

For example, a filesystem environment may support reading and writing but not file watching.

A networking environment may support outbound connections but not listening sockets.

The Standard does not require broad Capability names to hide meaningful platform differences.

Where such differences affect whether a contract can be satisfied, they should be modeled explicitly enough for software to reason about them.

The appropriate granularity should be driven by actual portability and composition requirements.

## Capability versioning

The semantic contract of a Capability may evolve.

A Provider that claims to satisfy a Capability must satisfy the version or Standard requirements it claims.

Adding optional stronger guarantees may remain compatible.

Changing the meaning of an existing Capability in a way that breaks existing contracts requires deliberate Standard evolution.

Providers must not silently use an old or incompatible interpretation while claiming conformance to a newer Capability contract.

## Capability and profiles

Profiles define sets of Capability guarantees and constraints.

For example, a future native profile may guarantee capabilities such as filesystem, process execution, native networking, and threading.

A WebAssembly profile may guarantee a different set.

The exact profile catalog is defined separately.

The Capability model allows these profiles to differ without giving shared concepts different meanings.

A Capability guaranteed by two profiles should preserve the same foundational semantics unless the profile explicitly defines a permitted stronger specialization.

## Capability and platform

A Platform is the environment in which Providers operate.

The Platform may directly supply some Capabilities.

Other Capabilities may be implemented by Vix Providers using lower-level platform facilities.

For example:

```text
Platform
    provides operating-system sockets

Vix networking Provider
    uses those sockets

Provider
    satisfies TCP Capability
```

The Standard should not require application code to depend directly on the platform mechanism when a provider-independent Capability contract is sufficient.

## External libraries

An external library is not automatically a Capability or Provider in the Vix semantic model.

For example:

```text
OpenSSL
libcurl
Asio
SQLite
```

are external technologies.

A Vix Provider may use them to satisfy a Capability.

The Vix contract should be defined independently enough that replacing the external library does not unnecessarily redefine the Capability.

If software intentionally depends on vendor-specific behavior, that may be represented as a provider-specific or domain-specific contract rather than pretending the behavior is universally portable.

## Capability and authorization

A Capability may exist while authorization changes per Operation.

For example:

```text
Filesystem Capability
    available

Operation A
    read permitted file
    → Success

Operation B
    read protected file
    → Failure(permission denied)
```

The second Failure does not mean that filesystem Capability disappeared.

Likewise, gaining authorization does not create the Capability if the platform cannot support the operation in the first place.

Keeping these concepts separate improves diagnostics and deployment reasoning.

## Capability and configuration

Configuration may influence which Provider satisfies a Capability.

For example, TLS may use one Provider in one deployment and another elsewhere.

Configuration may also disable optional Capabilities.

This does not mean configuration itself is a Capability.

Configuration selects or constrains behavior.

Capability describes the ability being supplied.

The Standard should avoid mixing the two concepts into one generic configuration service.

## Capability and lifecycle

A Capability itself does not necessarily have a lifetime.

A Provider or Resource supplying the Capability may.

For example:

```text
monotonic-time Capability
    may be stateless

CPU execution Capability
    may be supplied by WorkerPool Resource

database query Capability
    may be supplied through ConnectionPool Resource
```

When the Provider is a Resource, normal ownership and lifecycle rules apply.

A consumer that uses the Capability does not automatically own the Provider.

## Capability and shutdown

If a Capability depends on a Provider with a lifecycle, shutdown of that Provider may make the Capability unavailable for new work.

Existing Operations may also be affected.

The Provider contract must define what happens to already accepted work.

The semantic Capability itself does not define one universal shutdown policy.

Shutdown belongs to the Provider and Resource lifecycle.

## Capability and execution

Execution is one area where Capability boundaries are particularly important.

Vix should not assume that:

```text
CPU execution
asynchronous I/O
timer progression
blocking-work execution
```

must all be supplied by one Provider.

A domain should require the form of execution it actually needs.

Forcing all execution through one broad dependency can import unrelated lifecycle, scheduling, metrics, priority, or threading semantics.

The Capability model should instead allow these needs to remain separate where they are genuinely separate.

## Capability and testing

Capability boundaries can make implementations easier to test because software may be exercised against alternate Providers.

However, testability alone is not sufficient reason to create a new Capability abstraction.

A Capability should correspond to a real semantic requirement.

Test Providers, simulated Providers, deterministic clocks, in-memory storage, or controlled networking can then satisfy those same contracts when appropriate.

The Standard does not require every Capability to have a mock interface.

## Capability and observability

Observability may describe which Capabilities are available, which Provider is active, or why a Capability could not be resolved.

This information can be useful for diagnostics and deployment.

Observability must not become the semantic definition of availability.

A Capability exists because the applicable environment and Provider contract can satisfy it, not because a registry happens to contain a string naming it.

## Capability names

Capability names should describe semantic abilities rather than implementation brands.

Names such as:

```text
TCP
DNS
monotonic time
process execution
```

describe what software can rely on.

Names such as:

```text
Asio
OpenSSL
epoll
IOCP
```

describe particular implementations or mechanisms.

Provider-specific capabilities may exist when software intentionally requires those semantics, but generic domain code should prefer provider-independent Capability contracts where practical.

## Capability failure

Operations performed through an available Capability may still fail.

For example:

```text
DNS Capability available
    resolve unknown name
    → Failure

TCP Capability available
    connect unavailable host
    → Failure
```

Capability availability promises that the kind of Operation can be attempted according to the Capability contract.

It does not promise that every attempt succeeds.

This distinction is essential for a meaningful Failure model.

## Dynamic loss of Capability

Some environments may lose a Capability or the Resource supplying it while software is running.

For example, a removable device may disappear, network connectivity may change, or a Provider may terminate.

The Standard does not require every Capability to remain permanently available once observed.

Where availability may change dynamically and that matters to callers, the applicable contract should define the stability or observation semantics.

Operations already in progress still require coherent terminal outcomes.

## Normative requirements

A Capability MUST describe a semantic ability rather than a specific implementation unless it is explicitly provider-specific.

A Capability MUST remain conceptually distinct from the Provider that satisfies it.

A Capability MUST remain conceptually distinct from Resource instances created or used through that Capability.

Capability availability MUST NOT be treated as equivalent to authorization for every possible Operation.

Required Capability and available Capability MUST remain distinguishable.

Missing required Capability MUST remain conceptually distinct from operational Failure occurring while using an available Capability.

A Provider claiming a Capability MUST preserve the normative semantics of that Capability.

Changing Provider MUST NOT require changing the semantic meaning of the Capability unless the contract explicitly depends on provider-specific behavior.

The Capability model MUST NOT require every Capability to be represented as a runtime object or interface.

The Capability model MUST NOT require one universal service locator or global Context object.

A Capability MAY be known architecturally, at compile time, at runtime, or through a combination of stages.

A Provider MAY satisfy multiple Capabilities.

A Capability MAY have multiple conforming Providers.

Fallback between Providers MUST NOT silently weaken observable guarantees promised by the applicable contract.

When a Provider supplying a Capability is also a Resource, normal ownership and lifecycle semantics MUST apply.

Capability requirements SHOULD use the smallest coherent semantic ability needed by the consumer rather than unnecessarily broad runtime dependencies.

Foundational Capability names SHOULD describe abilities rather than implementation brands.

Capability granularity SHOULD be driven by real composition, portability, or contract differences rather than by a desire to model every individual function separately.

## Open questions

The Standard does not yet define a complete canonical catalog of Vix Capabilities.

The initial Capability set should emerge from real Platform and domain requirements rather than from an attempt to classify every existing module.

The Standard also does not yet define one universal runtime Capability-discovery API.

Possible environments may use static configuration, profile declarations, dependency construction, feature queries, or runtime negotiation.

A future Vix Evolution Proposal may standardize parts of capability discovery or resolution if repeated cross-domain requirements justify it.

Likewise, the relationship between broad Capabilities and optional sub-capabilities should be refined using concrete portability cases rather than decided purely in the abstract.

## Design consequence

A Capability should answer:

> What ability does this software require or this environment provide?

A Provider should answer:

> What implementation satisfies that ability here?

A Resource should answer:

> What lifetime-bearing instance is being used?

Authorization should answer:

> Is this particular use permitted?

Keeping those questions separate gives Vix room to support different platforms, providers, deployment environments, and future implementations without making application code depend directly on infrastructure choices.

The Capability model exists to make requirements clearer.

It should not become another layer of infrastructure that software must understand before it can perform simple work.
