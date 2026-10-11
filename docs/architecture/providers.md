# Providers

Providers are the implementation mechanisms that satisfy Vix Capabilities.

A Capability describes an ability that software may require.

A Provider supplies that ability using concrete code, operating-system facilities, external libraries, hardware, remote services, or other lower-level Vix capabilities.

The distinction matters because Vix should be able to preserve the meaning of a Capability while replacing the mechanism that implements it.

For example, TLS is a Capability.

A TLS implementation backed by OpenSSL is a Provider.

OpenSSL itself is not the architectural definition of TLS in Vix.

Likewise, CPU execution is a Capability.

The current Threadpool implementation may provide it, but the Threadpool implementation is not the semantic definition of CPU execution.

The Provider layer exists to preserve this separation.

## Capability and Provider

The relationship is conceptually:

```text
Consumer
    ↓ requires

Capability
    ↓ satisfied by

Provider
    ↓ implemented using

OS / external library / hardware / lower Capability
```

A consumer should ideally depend on the Capability it needs rather than on accidental properties of the current Provider.

This does not mean every Capability needs a virtual interface or runtime object.

Capability and Provider are architectural concepts first.

Their concrete C++ representation may differ depending on the problem.

## Providers are implementation, not semantics

The Vix Standard defines the meaning of foundational concepts and Capability contracts.

Providers implement those contracts.

A Provider may choose different algorithms, internal data structures, threading models, libraries, system calls, or hardware facilities.

Those choices are valid as long as the Provider preserves the guarantees required by the applicable Capability and Profile.

This gives implementations freedom while keeping higher-level software predictable.

For example, two TCP Providers may use different operating-system networking mechanisms.

They may differ in performance, internal queues, or event dispatch.

They still need to preserve the same TCP capability semantics required by Vix.

## A Provider is not necessarily a class

The architecture does not require:

```cpp
class IProvider;
```

or:

```cpp
class CapabilityProvider;
```

for every capability.

A Provider may be represented by:

```text
a stateful object
a set of functions
a compile-time implementation
a platform backend
a policy object
a context
a library wrapper
```

depending on the capability.

Forcing every Provider into the same C++ shape would add abstraction without necessarily improving architecture.

The important property is that Vix knows which implementation responsibility satisfies which Capability.

## A Provider is not a service locator entry

Provider architecture must not imply the existence of one global registry such as:

```text
GlobalRuntime
    get<FileSystem>()
    get<Tcp>()
    get<Crypto>()
    get<Executor>()
    get<Everything>()
```

Such a mechanism may appear convenient because it centralizes access.

It also risks hiding dependencies, ownership, and initialization requirements.

A component should normally receive, construct, or directly depend on the facilities it actually requires.

The Provider model describes implementation substitution.

It does not require a dependency-injection framework.

## Provider ownership

Some Providers are stateful Resources.

Examples include:

```text
Async Context
Threadpool
Database connection pool
stateful TLS context
Network monitor
```

Such Providers require an owner.

Consumers may borrow the Provider or the capability it exposes.

For example:

```text
Application
    owns Async Provider

WebSocket
    borrows Async capability

HTTP Server
    borrows Async capability
```

Using a Provider does not automatically grant lifecycle authority over it.

The ownership rules described in `ownership-map.md` apply equally to Providers.

## Stateless Providers

Other Providers may have no meaningful runtime lifetime.

A pure parsing backend, stateless JSON implementation, or compile-time facility may require no startup or shutdown.

In these cases, explicit Provider ownership may be unnecessary.

The architecture should not create state merely to make all Providers look alike.

Stateful and stateless Providers may both satisfy capabilities.

## Provider creation

Provider creation should remain distinct from Provider use.

A Provider may be created by:

```text
application orchestration
platform initialization
a component that privately owns it
a factory
build-time configuration
target-specific initialization
```

The creator does not necessarily remain the owner after creation.

Ownership may be transferred.

For example:

```text
Factory
    creates Provider
        ↓ transfer

Application
    owns Provider
```

Creation and lifecycle authority must not be conflated.

## Provider initialization

Some Providers may require initialization before they can satisfy their Capability.

Examples include:

```text
starting worker threads
creating an event-loop backend
loading cryptographic state
opening platform handles
initializing an external library
```

Initialization may itself fail.

Such failure should be reported according to the applicable Operation and Error contracts.

An initialized C++ object and a ready Provider are not always the same state.

Where this distinction matters, the Provider contract should make it explicit.

## Provider readiness

A Provider may exist before it is ready to accept work.

Likewise, it may remain allocated while shutting down and no longer accept new work.

Architecturally, useful states may include:

```text
created
initializing
ready
shutting down
stopped
failed
```

The Standard does not require one universal Provider state machine.

The Provider contract should nevertheless make observable readiness behavior understandable.

A caller should not have to guess whether work submitted during shutdown will be accepted, rejected, cancelled, or silently ignored.

## Provider shutdown

A stateful Provider should define what shutdown means.

For an execution Provider, shutdown might determine what happens to:

```text
queued work
running work
new submissions
pending completions
```

For a connection pool, shutdown might determine what happens to:

```text
leased connections
idle connections
new acquisition requests
pending queries
```

The exact policy may differ.

The architecture requires that the behavior be deliberate rather than accidental.

## Provider shutdown and borrowers

Borrowers must not terminate a shared Provider unless lifecycle authority was explicitly granted.

For example:

```text
Application
    owns Threadpool

Game
    borrows CPU execution

Web
    borrows CPU execution
```

Stopping the Game subsystem must not stop the Threadpool while Web still depends on it.

The same rule applies to Async, network monitors, shared connection pools, and other stateful Providers.

## Provider dependencies

A Provider may depend on lower-level Capabilities.

For example:

```text
HTTP Client Provider
    requires DNS
    requires TCP
    may require TLS
    requires Time
    requires Async
```

or:

```text
TLS Provider
    requires transport
    may require Crypto
    requires Time
```

The Provider should depend on the lower-level capability rather than unnecessarily binding itself to one unrelated high-level implementation.

This creates a layered architecture.

## Providers form graphs

Providers do not necessarily form a simple stack.

They may form dependency graphs.

For example:

```text
HTTP Client
├── DNS Provider
├── TCP Provider
│   └── Async Provider
├── TLS Provider
│   ├── TCP
│   └── Crypto
└── Time
```

The graph should remain acyclic in lifecycle responsibility where practical.

A lower-level Provider should not depend upward on the higher-level domain that consumes it.

If such a cycle appears, architectural ownership should be reviewed.

## Provider replacement

Provider replacement should be possible when the Capability contract remains satisfied.

For example:

```text
CPU Execution Capability
    ↓

current:
    Threadpool Provider

future:
    WorkStealing Provider
```

Higher-level software should not need a semantic rewrite merely because the implementation changed.

The replacement may still require build, configuration, or performance tuning changes.

The important requirement is that the meaning of CPU execution does not depend on the Provider's class name.

## Provider-specific extensions

A Provider may offer behavior beyond the baseline Capability.

For example, a networking Provider might expose advanced kernel features.

A CPU Provider might expose affinity or queue statistics.

A TLS Provider might expose implementation-specific configuration.

Such extensions are valid.

They must remain distinguishable from baseline Vix guarantees.

Conceptually:

```text
Capability contract
    portable baseline

Provider extension
    additional behavior
```

Software relying on the extension becomes dependent on that Provider or on a stronger extension contract.

It should not accidentally appear portable under the baseline Capability.

## Stronger Providers

A Provider may provide stronger guarantees than the minimum required capability.

For example, a cancellation implementation may support immediate operating-system cancellation even if the baseline Capability only requires cooperative cancellation under certain conditions.

This is allowed.

The stronger behavior must not redefine the baseline semantics.

Portable code may rely only on the guarantees of the claimed Capability and Profile unless it explicitly opts into the stronger Provider contract.

## Weaker Providers

A Provider must not silently provide weaker behavior than the Capability contract it claims to satisfy.

For example, if the Capability guarantees Deadline handling, a Provider cannot simply ignore Deadlines and still claim conformance.

If the environment cannot satisfy the requirement, the correct response is to:

```text
use a different Profile
report missing Capability
use a Provider with weaker explicit contract
or reject configuration
```

rather than quietly weakening Vix semantics.

## Fallback Providers

Some capabilities may support fallback Providers.

For example:

```text
preferred Provider
    unavailable
        ↓
fallback Provider
```

Fallback is valid only when the fallback preserves the guarantees required by the caller and Profile.

A fallback must not silently weaken security, persistence, cancellation, durability, or other required semantics.

For example:

```text
secure TLS Provider unavailable
    → plaintext fallback
```

would not be an acceptable transparent fallback for a Capability requiring secure transport.

Fallback policy must preserve meaning.

## Provider selection

Provider selection may happen in several ways.

Possible mechanisms include:

```text
build-time target selection
compile-time configuration
application construction
Profile selection
runtime discovery
explicit configuration
```

Different capabilities may use different mechanisms.

The architecture does not require one universal Provider-selection framework.

A native TCP backend may be selected at compile time.

A database Provider may be selected by application configuration.

A CPU execution Provider may be explicitly created by the application.

The selection mechanism should fit the capability.

## Compile-time selection

Compile-time Provider selection is appropriate when the target environment already determines the implementation.

For example, platform-specific filesystem or networking backends may be selected during compilation.

Conceptually:

```text
TCP Capability

Linux target
    → Linux Provider

Windows target
    → Windows Provider
```

The higher-level code should continue to use the same capability semantics.

Conditional compilation should remain primarily inside the Provider boundary rather than spreading platform checks across consumers.

## Runtime selection

Runtime selection may be appropriate when several Providers can coexist or when the environment cannot be known fully at build time.

For example:

```text
Storage Capability
    Provider A
    Provider B
```

may be selected by application configuration.

Runtime selection does not require a global registry.

The selected Provider may simply be constructed and passed to the component that needs it.

## Multiple Providers at once

Vix should not assume one singleton Provider per Capability.

An application may legitimately use multiple Providers simultaneously.

For example:

```text
Database A
    uses Provider A

Database B
    uses Provider B
```

or:

```text
CPU Pool A
    optimized for latency

CPU Pool B
    optimized for blocking work
```

The architecture should allow this without forcing global state.

Capability identity does not imply Provider singleton identity.

## Provider scope

Provider lifetime may be scoped at different levels.

Examples include:

```text
process-wide
application-wide
subsystem-wide
connection-wide
Operation-local
```

A Provider should use the narrowest scope appropriate to its function.

Not every Provider needs to be global.

A TLS connection implementation may be connection-scoped.

An Async Context may be application-scoped.

A parser backend may effectively be stateless and have no scope.

The architecture should not impose one Provider lifetime pattern universally.

## Provider-created Resources

Providers often create Resources.

For example:

```text
TCP Provider
    creates Socket

Process Provider
    creates Process Resource

Filesystem Provider
    creates File Resource
```

The Provider does not necessarily remain the lifecycle owner of those Resources.

The creation Operation may transfer ownership to the caller.

Conceptually:

```text
Provider
    performs create Operation
        ↓ Success
Resource
        ↓ ownership transfer
Caller
```

Provider responsibility and Resource ownership must remain distinct.

## Provider-scoped Resources

Some Resources remain valid only while their Provider remains alive.

For example, a Resource may retain internal handles or state owned by the Provider.

In such cases the lifetime relationship must be explicit.

Conceptually:

```text
Provider
    owns internal infrastructure

Resource
    depends on Provider lifetime
```

The Resource owner must then ensure the Provider outlives the Resource, or the implementation must retain the necessary Provider lifetime safely.

This dependency should not be hidden.

## Provider-independent Resources

Other Resources may become independent after creation.

For example, a Provider may transfer a native handle or self-contained Resource whose remaining lifetime does not depend on the Provider object.

This is also valid.

Architecture should not require all Provider-created Resources to retain their creator unnecessarily.

The actual dependency should determine the ownership graph.

## Execution Providers

Execution is one of the most important Provider areas.

Current architecture recognizes at least:

```text
Async I/O Provider
CPU Execution Provider
Blocking-work Execution Provider
Timer Provider
```

These may share mechanisms but should not be conflated automatically.

The current mapping includes:

```text
Async I/O
    → Async

CPU Execution
    → Threadpool

Blocking-work Execution
    → currently Threadpool

Timer progression
    → Async / timer mechanism
```

The Provider layer gives Vix room to replace these implementations later.

## Async Provider

The Async Provider supplies asynchronous I/O progression.

It may use operating-system or host mechanisms such as event loops, completion queues, polling facilities, or host callbacks.

Higher-level domains should depend on Async semantics rather than directly depending on the platform event API.

Conceptually:

```text
WebSocket
    ↓
Async Capability
    ↓
Async Provider
    ↓
platform event mechanism
```

The Async Provider may be stateful and therefore requires explicit lifecycle ownership.

## CPU Provider

The current Threadpool implementation provides CPU execution.

Higher-level code should conceptually depend on CPU execution rather than on Threadpool-specific internals.

This allows future evolution of:

```text
scheduling algorithm
worker management
work stealing
priority behavior
affinity
queue implementation
```

without redefining higher-level domains.

Game JobSystem is a clear current consumer.

Other consumers should be added only when they actually require CPU execution.

## Blocking Provider

Blocking-work execution may currently use the same Threadpool implementation as CPU execution.

That is an implementation choice.

If future evidence shows that blocking work requires different sizing or scheduling behavior, Vix may introduce a separate Provider.

The Capability Map already preserves that possibility by distinguishing the capabilities.

The Provider architecture should not force premature separation either.

## Networking Providers

TCP, UDP, DNS, and network reachability each represent distinct capabilities.

Their Providers may use common underlying networking infrastructure.

That does not mean every consumer should depend on one historical Net Provider.

Conceptually:

```text
TCP Provider
UDP Provider
DNS Provider
Network Reachability Provider
```

may coexist while sharing lower-level platform facilities.

This makes dependency requirements explicit.

## TCP Provider

A TCP Provider supplies stream transport Operations and produces Socket-like Resources.

It commonly depends on Async and operating-system networking facilities.

Higher-level consumers may include:

```text
HTTP
WebSocket
database drivers
P2P
Realtime
applications
```

The TCP Provider should not own protocol semantics such as HTTP routing or WebSocket framing.

Its responsibility ends at the transport contract.

## DNS Provider

A DNS Provider supplies resolution.

It may use the system resolver, asynchronous host APIs, an external resolver library, or another implementation.

Consumers should depend on DNS resolution semantics.

They should not assume the underlying resolver implementation unless they intentionally depend on an extension.

## Network Reachability Provider

Network reachability is intentionally narrower than general networking.

A Network Reachability Provider may observe whether external connectivity appears usable.

Consumers such as Cache or Sync may use this signal to adapt behavior.

Reachability information is not proof that a particular future network Operation will succeed.

A Provider should not turn heuristic connectivity state into an impossible guarantee.

The contract should remain appropriately narrow.

## TLS Provider

The TLS Provider supplies secure transport according to the Vix TLS Capability.

A Provider may be backed by:

```text
OpenSSL
platform TLS facilities
another cryptographic implementation
```

depending on the target.

Higher-level consumers should not need to depend directly on OpenSSL types unless intentionally using Provider-specific extensions.

The Provider boundary should isolate vendor-specific implementation details where practical.

## Crypto Providers

Crypto may eventually consist of several narrower capabilities such as:

```text
secure randomness
hashing
signatures
encryption
key derivation
```

The current architecture should avoid creating unnecessary Provider interfaces before actual consumer requirements justify them.

Where a concrete Crypto module already provides these capabilities, higher-level code should use the canonical implementation rather than creating domain-local cryptographic mechanisms.

Security-sensitive Provider replacement must preserve the required security semantics.

## Filesystem Providers

Filesystem Providers bridge Vix filesystem semantics to host storage.

Different Profiles may provide different capabilities.

For example, a native host may expose broad filesystem access while a sandboxed environment exposes only restricted storage.

The Provider should preserve the same foundational semantics where the capability exists.

Restrictions should be expressed through Capability and Profile guarantees rather than hidden platform divergence.

## Process Providers

Process Providers bridge Vix process semantics to host process facilities.

They may differ substantially by operating system.

A process Provider may implement:

```text
spawn
wait
signal or termination request
stdio interaction
environment configuration
```

depending on the final capability contract.

Higher-level consumers should not need to depend directly on `fork`, `CreateProcess`, or another host-specific mechanism.

Those belong beneath the Provider boundary.

## Time Providers

The Time foundation may depend on host clock Providers.

For example:

```text
Monotonic Clock
    ↓
platform monotonic clock

Wall Clock
    ↓
platform civil/system clock
```

These Providers should preserve the distinction between clock domains.

A wall-clock implementation must not be substituted transparently where monotonic time is required for Deadline correctness.

Provider substitution must preserve semantic guarantees, not merely produce a number that looks like time.

## Logging Providers

Logging may use several output Providers or sinks.

For example:

```text
console
file
system log
structured collector
```

These Providers handle delivery.

The Logging foundation retains the common log semantics.

A subsystem emitting a log message should not need to understand whether output goes to console, file, or another sink.

Likewise, losing one optional sink should not necessarily destroy the operational Error model.

## JSON Providers

JSON may be implemented internally or through an external library.

The Provider boundary should preserve canonical Vix JSON semantics and Error behavior.

Higher-level domains should avoid coupling themselves to the external parser's private types if Vix already exposes a canonical JSON representation.

This is especially important when JSON is used by many otherwise independent domains.

## HTTP client Provider

HTTP client functionality is itself a higher-level Platform Provider composed from lower-level capabilities.

Conceptually:

```text
HTTP Client Provider
    uses
        DNS
        TCP
        TLS
        Async
        Time
        Error
```

The Provider may additionally own Resources such as:

```text
connection pools
keep-alive state
proxy configuration
```

depending on implementation.

Consumers should not need to rebuild this composition independently.

## Database Providers

Database Providers are more domain-specific and may belong partly in the Ecosystem.

A driver Provider may satisfy a database transport or query capability while using:

```text
TCP
TLS
Async
Time
Error
```

A higher-level database client or ORM should remain distinct from the low-level Provider mechanism.

Vix should not assume that all database engines require the same Provider lifecycle or transport model.

Database architecture will need its own more detailed map later.

## External libraries

External libraries are valuable implementation mechanisms.

They should be placed below the Provider boundary where possible.

Conceptually:

```text
Vix Capability
    ↓
Vix Provider
    ↓
External Library
```

This gives Vix control over its own semantic contract.

It also prevents vendor-specific types and behavior from spreading throughout unrelated domains.

There may be cases where exposing an external library directly is intentional.

Such cases should be explicit rather than accidental.

## Provider wrappers should have purpose

Vix should not wrap an external library merely to hide its name.

A Provider boundary is valuable when it provides one or more of:

```text
stable Vix semantics
ownership control
portable behavior
error normalization
Profile integration
lifecycle management
dependency isolation
provider replacement
```

A wrapper adding no semantic or architectural value may simply increase complexity.

The Provider model should not become wrapper-for-wrapper's-sake architecture.

## Error translation

Providers frequently need to translate lower-level Errors into Vix Errors.

For example:

```text
OS error
    ↓
TCP Provider
    ↓
Vix network Error
```

or:

```text
external TLS library error
    ↓
TLS Provider
    ↓
Vix TLS Error
```

Translation should preserve useful cause and diagnostic context where possible.

It should not flatten every failure into generic strings.

Provider boundaries are natural places to normalize implementation-specific failure while retaining underlying cause information.

## Provider Faults

An invalid internal Provider state is not automatically an operational Failure.

For example:

```text
corrupted internal queue invariant
impossible state transition
use-after-destruction caused by implementation bug
```

is a Fault.

The Provider should not hide programming defects behind ordinary capability Errors merely because they occurred below a Platform boundary.

The Standard Failure/Fault distinction applies inside Providers as well.

## Cancellation in Providers

Providers should implement cancellation according to the strength their mechanism can support.

For example, an Async networking Provider may be able to cancel a pending OS operation.

A blocking Provider may only observe cancellation once a blocking call returns.

Both may participate in the same Vix cancellation semantics while offering different responsiveness.

The Provider contract should make relevant limitations clear.

It must not claim stronger guarantees than it can provide.

## Deadlines in Providers

Providers commonly receive or derive time constraints from higher-level Operations.

They should preserve the effective Deadline.

For example:

```text
Parent HTTP Operation
    Deadline = D

DNS Provider
    remaining time ≤ D

TCP Provider
    remaining time ≤ D

TLS Provider
    remaining time ≤ D
```

Each Provider must not reset its local timeout to a fresh full duration unless the higher-level contract explicitly allows that behavior.

Provider composition must preserve temporal budgets.

## Provider threading model

Some Providers have meaningful threading constraints.

A Provider may be:

```text
thread-safe
thread-confined
single-loop
multi-worker
externally synchronized
```

Where these properties affect correctness, the Provider contract should document them.

Higher-level code should not infer thread safety merely because a Provider is shared through `shared_ptr`.

Memory sharing and concurrency guarantees are different concerns.

## Callback execution

Providers that call user or higher-level code should define relevant callback execution semantics.

For example:

```text
callback may run inline
callback always deferred
callback runs on Provider thread
callback resumes on caller-selected execution context
```

This matters for reentrancy, locks, lifetime, and latency.

The architecture should avoid leaving such behavior accidental where callers must depend on it.

## Provider observability

Providers may expose diagnostics or metrics such as:

```text
active connections
queue depth
worker count
pending Operations
rejected submissions
connection pool utilization
```

These are useful operational features.

They should remain separate from the baseline Capability contract unless the Capability explicitly requires them.

A component should not need metrics access merely to use the underlying Capability.

## Provider configuration

Provider configuration belongs near the Provider rather than in every consumer.

For example, Threadpool worker count should normally configure the execution Provider.

TLS certificate configuration belongs to TLS-related setup.

Database pool size belongs to the database Provider or pool.

Higher-level domains may expose convenient configuration surfaces, but the underlying responsibility should remain clear.

Configuration should not become one giant untyped global map.

## Provider-specific configuration

Some configuration is inherently Provider-specific.

For example, one TLS backend may expose a tuning option another backend does not support.

Such configuration should be marked as Provider-specific.

Portable application configuration should use Capability-level concepts where possible.

This distinction prevents implementation-specific details from silently becoming universal Vix API.

## Default Providers

Vix may provide default Providers for common environments.

Defaults improve usability.

They do not change the Capability/Provider distinction.

For example:

```text
Native Profile
    default Async Provider
    default TCP Provider
    default TLS Provider
```

may allow ordinary applications to work without manual Provider construction.

Advanced applications may still replace or customize Providers where supported.

Simple defaults and explicit architecture are compatible.

## Provider discovery

Runtime Provider discovery should only be introduced where needed.

The architecture does not require every capability to answer questions such as:

```text
find all installed Providers
select highest priority
load Provider plugin dynamically
```

Some capabilities may eventually benefit from plugin-style discovery.

Others are better resolved statically.

Provider discovery should arise from real distribution or extensibility requirements rather than from the Provider abstraction itself.

## Provider plugins

A future Vix ecosystem may support dynamically loaded Providers.

For example, external packages could potentially supply:

```text
database drivers
storage backends
hardware accelerators
specialized protocol implementations
```

Such a system would require clear ABI, compatibility, security, lifecycle, and conformance rules.

It should not be assumed by the current architecture.

The Provider model is intentionally usable without dynamic plugins.

## Profiles and Providers

Profiles define guarantees.

Providers satisfy those guarantees.

Conceptually:

```text
Profile
    guarantees Capability X
        ↓
Platform must supply conforming Provider X
```

A Profile does not normally require one vendor implementation unless the Profile explicitly says so.

This keeps Profile contracts semantic rather than implementation-specific.

## Missing Providers

If an application requires a Capability but no conforming Provider is available, the situation is not an ordinary domain Operation Failure.

For example:

```text
application requires Process Capability

selected Profile has no Process Provider
```

is a configuration or conformance problem.

By contrast:

```text
Process Provider exists

spawn("missing-program")
    fails
```

is an operational Failure.

The Provider architecture should preserve this distinction.

## Dynamic Provider loss

Some environments may allow a previously available Provider or underlying resource to become unavailable.

This is more complicated than simple initial absence.

For example, hardware may disconnect or a remote backing service may disappear.

Whether this represents:

```text
Provider Failure
Capability loss
Activity termination
ordinary Operation Failure
```

depends on the capability contract.

The architecture should not force one universal answer prematurely.

Providers should expose the strongest meaningful state they can support.

## Provider conformance

Provider conformance is behavioral.

A Provider satisfies a Capability because it preserves the required contract.

It does not satisfy the Capability merely because:

```text
it implements a class with the expected name
it inherits an interface
it uses the expected external library
it lives in the expected module
```

Interfaces can help enforce structure.

They do not replace behavioral conformance.

Tests should validate the relevant observable guarantees.

## Provider tests

Useful Provider tests include:

```text
accepted work receives defined completion

shutdown follows documented policy

borrower cannot terminate externally owned Provider

Deadline propagation preserves remaining budget

cancellation produces valid completion behavior

provider-specific errors retain useful cause

missing capability is distinguishable from Operation Failure
```

Provider-specific tests may additionally validate implementation details such as:

```text
thread creation
event-loop integration
queue behavior
system call handling
external library setup
```

Both levels are useful, but they test different things.

## Current Provider map

The current architecture points toward the following broad mapping:

```text
Async I/O Capability
    → Async Provider

CPU Execution Capability
    → Threadpool Provider

Blocking-work Execution Capability
    → currently Threadpool Provider

Timer progression
    → Async / timer Provider

Filesystem Capability
    → filesystem implementation / platform backend

Environment Capability
    → environment implementation / host environment

Time Capability
    → platform clock backends

Process Capability
    → process implementation / OS backend

TCP Capability
    → networking transport Provider

UDP Capability
    → networking datagram Provider

DNS Capability
    → resolver Provider

Network Reachability
    → NetworkProbe Provider

TLS Capability
    → Vix TLS Provider
       backed by external TLS mechanism

Crypto Capability
    → Vix Crypto implementation / Provider

JSON Capability
    → Vix JSON implementation

HTTP Client Capability
    → Vix HTTP client implementation
       composed from lower Platform Providers
```

This map expresses architectural direction.

It does not freeze the final public type names or repository organization.

## Historical Provider ambiguity

Historical aggregation can make Provider identity unclear.

For example, a broad runtime object may simultaneously:

```text
own threads
schedule tasks
coordinate shutdown
store configuration
act as execution dependency
```

This makes it difficult to know which Capability consumers actually require.

Likewise, a broad Net module may appear to provide every networking function even when one consumer only needs connectivity probing.

Migration should separate these responsibilities before introducing replacement Providers.

## RuntimeExecutor migration

The RuntimeExecutor migration demonstrates this principle.

A component receiving RuntimeExecutor was previously assumed to have an execution dependency.

Audit showed that some components merely stored and forwarded it.

Those components did not need a replacement Provider.

Their dependency was removed.

Other consumers that genuinely submit callable work may still require an execution capability.

The Provider should be chosen based on that actual requirement.

This is the intended migration model.

## Avoid Provider inflation

Once Capability language exists, it can become tempting to create Providers for everything.

Vix should resist this.

A Provider abstraction should exist when there is a meaningful implementation boundary.

Useful reasons include:

```text
multiple implementations
platform-specific backend
stateful infrastructure
external library isolation
test substitution
Profile variation
lifecycle boundary
```

A pure function does not need a Provider simply because it belongs to the Platform.

Architecture should remain proportional to the problem.

## Avoid interface inflation

Similarly, Provider does not imply interface.

For example, if Vix has one canonical JSON implementation with no practical substitution requirement, creating:

```cpp
IJsonProvider
JsonProviderBase
DefaultJsonProvider
JsonProviderFactory
```

may add little value.

The Capability can still be understood architecturally without such machinery.

Interfaces should exist where they improve substitution, testing, ownership, or separation.

Not for symmetry.

## Provider and public API

Provider details should remain below ordinary public APIs where possible.

A user making a normal HTTP request should not need to choose DNS, TCP, TLS, and Async Providers manually.

The HTTP client can compose reasonable defaults.

Advanced APIs may expose customization when users need it.

This produces a layered experience:

```text
simple API
    uses default Provider composition

advanced API
    allows Provider customization

architecture
    preserves explicit responsibility underneath
```

Strong architecture should make simple APIs possible rather than force every implementation detail onto the caller.

## Provider and Toolchain

The Toolchain may also consume or provide capabilities.

For example, build execution may require Process and Filesystem.

Package operations may require HTTP Client and Crypto.

The Toolchain should consume canonical Platform capabilities where appropriate rather than maintain unrelated copies of the same mechanisms.

At the same time, Toolchain-specific Providers do not automatically belong to the application Runtime.

The same semantic foundation can serve both without collapsing their lifecycles.

## Provider evolution

Providers may evolve more rapidly than the Capability contracts they implement.

A Provider may receive:

```text
performance improvements
new algorithms
new platform support
new diagnostics
new internal concurrency
external library upgrades
```

without requiring Standard changes.

A Capability contract changes only when the semantic guarantees change.

This separation is one of the main reasons Providers exist architecturally.

## Provider versioning

Provider implementation versions are independent from Standard versions.

A Provider version answers:

> Which implementation is this?

The Standard and Capability contract answer:

> Which behavior does it promise?

A new Provider version may continue conforming to the same Standard.

Likewise, several Provider implementations may conform to the same Capability contract.

Version numbers must not substitute for behavioral conformance.

## Migration strategy

When replacing a historical implementation dependency with a Provider-oriented architecture, the sequence should be:

```text
1. identify the actual Capability required
2. inspect existing ownership
3. determine whether a current canonical Provider already exists
4. remove forwarding-only or unused dependencies
5. preserve existing semantic behavior
6. isolate platform or vendor implementation details
7. introduce new Provider abstraction only when justified
8. validate lifecycle and behavior
```

The architecture should not begin by creating interfaces and factories.

It should begin by understanding the responsibility.

## Architectural requirements

A Provider must satisfy a defined Capability rather than redefine it locally.

Provider implementation details should remain below Capability boundaries where practical.

Provider usage must remain distinct from Provider lifecycle ownership.

A Provider may satisfy multiple Capabilities.

A Capability may have multiple Providers.

Vix must not assume one global Provider instance per Capability.

Provider selection does not require a universal registry.

Provider fallback must not silently weaken required semantics.

Provider-specific extensions must remain distinguishable from baseline Capability guarantees.

Provider-created Resources must have explicit ownership semantics.

Stateful Providers must have identifiable lifecycle owners.

Consumers borrowing shared Providers must not stop them without lifecycle authority.

Provider dependencies should be expressed through lower-level Capabilities where appropriate.

Provider shutdown and work-rejection behavior should be explicit where observable.

Providers must preserve Deadline, Cancellation, Failure, and ownership semantics required by the Operations they implement.

External libraries should remain implementation mechanisms unless their identity is intentionally part of the contract.

New Provider abstractions should be introduced only where they create a meaningful implementation boundary.

## Open Provider questions

Several Provider boundaries still require further work.

The long-term Provider representation for CPU execution remains open.

Blocking execution may remain on Threadpool or eventually receive a dedicated Provider.

The networking redesign still needs to define the canonical relationship among TCP, UDP, DNS, TLS, transport Resources, and Async.

Persistent storage does not yet have one accepted Capability and Provider model.

Database Providers need a dedicated architectural audit covering drivers, pools, Async, transactions, and storage semantics.

The final provider-selection mechanism for configurable application capabilities remains intentionally unspecified.

Vix also has not decided whether third-party Provider plugins should become a supported ecosystem mechanism.

These questions should be answered from real implementation and ecosystem requirements rather than from a desire to complete a generic Provider framework.

## Design consequence

Provider architecture should allow Vix to say:

```text
WebSocket needs Async I/O.

Game needs CPU execution.

HTTPS needs TLS.

Cache may need network reachability.

Process needs host process execution.
```

and then independently answer:

```text
Which implementation satisfies each need on this target?
```

Those are different questions.

When they remain separate, Vix can replace operating-system backends, external libraries, schedulers, pools, and other mechanisms without forcing the entire Ecosystem to learn the new implementation.

The Capability remains the promise.

The Provider is how Vix keeps that promise on a particular system.
