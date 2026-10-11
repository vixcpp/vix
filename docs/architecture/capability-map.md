# Capability Map

The Capability Map connects the semantic capabilities described by the Vix Standard to their architectural ownership inside the Vix Platform.

Its purpose is not to create a registry of every function or module in the repository.

It exists to answer a more useful question:

> When a part of Vix requires an ability, where should that requirement be satisfied?

The map distinguishes the capability itself from the component that currently implements it.

This distinction is important because implementation ownership may move while the semantic capability remains stable.

For example, CPU execution may currently be provided by Threadpool. That does not make `Threadpool` the semantic definition of CPU execution.

Likewise, asynchronous networking may currently use a particular Async implementation. Higher-level domains should depend on the asynchronous I/O capability they need rather than on accidental details of the current Provider.

## How to read this map

Each capability can be understood through four architectural questions.

The first is semantic:

> What ability is required?

The second is ownership:

> Which Vix area owns the canonical contract and implementation direction for that ability?

The third is provision:

> Which current or intended Provider supplies the ability?

The fourth is consumption:

> Which parts of Vix depend on it?

The relationship is approximately:

```text id="capmap01"
Consumer
    ↓ requires

Capability
    ↓ supplied by

Provider
    ↓ implemented using

OS / external library / hardware / lower capability
```

Not every capability requires a runtime Provider object.

Some capabilities are implemented through ordinary types or functions.

The term Provider is used when there is a meaningful implementation mechanism that satisfies the capability.

## Status terminology

This document describes both current implementation reality and intended architecture.

A capability may therefore be described as:

```text id="capmap02"
canonical
    responsibility and implementation direction are already clear

current
    implementation exists and is usable,
    but the long-term boundary may still evolve

migration
    historical ownership is being replaced by a clearer canonical boundary

open
    semantic need is understood,
    but the final public representation or Provider boundary is not yet fixed
```

The status is architectural, not a Standard conformance status.

## Structured Error capability

Vix requires one common ability to represent expected operational Failure structurally.

Architectural ownership belongs to the Error foundation.

The canonical semantic model includes Error identity, Result, context, and causal information where supported.

Conceptually:

```text id="capmap03"
Structured Failure Capability
    ↓
Error Foundation
```

Consumers include almost every Platform and Ecosystem domain.

Filesystem Operations may return filesystem Errors.

Networking may return connection or resolution Errors.

Process execution may report spawn or wait Errors.

Web and database layers may define higher-level Error identities.

These domain Errors should compose with the common Error foundation rather than replacing it.

The current architectural direction is canonical.

The exact future relationship between `Result<T>`, asynchronous completion, and Cancelled completion remains partially open.

## Path capability

Path handling provides canonical filesystem-oriented path representation and manipulation.

Architectural ownership belongs to the Path foundation.

This capability should remain usable independently from asynchronous execution or higher-level filesystem behavior.

Conceptually:

```text id="capmap04"
Path Capability
    ↓
Path Foundation
```

Likely consumers include Filesystem, Toolchain, Process, database storage, cache, game assets, configuration, and applications.

Path should not depend on Web, networking, Async, or other higher-level domains merely to provide path manipulation.

Its architectural status is canonical.

## Filesystem capability

Filesystem capability provides general interaction with filesystem Resources and filesystem Operations.

Architectural ownership belongs to the Filesystem foundation.

Conceptually:

```text id="capmap05"
Filesystem Capability
    ↓
Filesystem Foundation
    ↓
OS / host filesystem mechanisms
```

Consumers may include Toolchain, cache, database storage, game assets, application code, configuration, packaging, and future Providers.

Filesystem Operations should use the common Error, Resource, ownership, and Time semantics.

Some environments may not provide full filesystem capability.

That difference should be expressed through Profiles rather than by giving filesystem concepts different meanings on each platform.

## Environment capability

Environment capability provides access to process or host environment values.

Architectural ownership belongs to the Environment foundation.

Conceptually:

```text id="capmap06"
Environment Capability
    ↓
Environment Foundation
    ↓
process / host environment
```

Consumers include Toolchain, Process, application configuration layers, Providers, and ordinary application code.

The capability should remain narrower than a complete configuration framework.

Its purpose is to provide environment access, not to own application configuration policy.

Its architectural status is canonical.

## Time capability

Time capability provides the foundational temporal model required throughout Vix.

Architectural ownership belongs to the Time foundation.

The semantic surface includes concepts such as Duration, Clock, Instant, and Deadline.

Conceptually:

```text id="capmap07"
Time Capability
    ↓
Time Foundation
    ↓
platform clocks
```

Consumers include Async, networking, Process, retries, shutdown, cache, Web, database, game systems, observability, and many future domains.

Time is foundational and should remain independent from Async even though Async is a major consumer.

The Time capability is canonical.

The final public representation of some time concepts may still evolve.

## Timer capability

Timer capability is related to Time but includes execution.

It provides the ability to wait until a temporal condition becomes satisfied and then progress work.

Architecturally:

```text id="capmap08"
Time Foundation
    defines Duration / Instant / Deadline

Timer Capability
    ↓
Async / Timer Provider
    ↓
OS or host timer mechanism
```

Consumers include asynchronous Operations, retries, networking, Activity shutdown, polling, heartbeat systems, and realtime behavior.

The semantic ownership of time remains with Time.

The execution mechanism for asynchronous timers belongs with the appropriate Async or timer Provider.

This capability is conceptually clear, while its final public API remains open.

## Logging capability

Logging capability provides common runtime diagnostic emission.

Architectural ownership belongs to the Logging foundation.

Conceptually:

```text id="capmap09"
Logging Capability
    ↓
Logging Foundation
    ↓
configured sinks / console / file / other outputs
```

Consumers exist throughout Platform and Ecosystem code.

Logging is not the Error model.

Errors may be logged, but semantic correctness must not depend on logging availability.

The current architectural direction is canonical.

## Asynchronous I/O capability

Asynchronous I/O capability provides the ability to progress I/O Operations without requiring one blocked thread per Operation.

Architectural ownership belongs to Async.

Conceptually:

```text id="capmap10"
Asynchronous I/O Capability
    ↓
Async
    ↓
Async Provider / I/O Context
    ↓
OS or host event mechanism
```

Consumers include TCP, UDP, DNS implementations where appropriate, HTTP client, HTTP server, WebSocket, realtime systems, and potentially database or process facilities.

This capability is one of the most important execution boundaries in the current migration.

Higher-level I/O domains should increasingly depend on Async where that is their actual execution requirement rather than retaining historical `RuntimeExecutor` dependencies.

The architectural direction is canonical.

## Coroutine support

Coroutine support is an implementation and API capability closely associated with Async.

It may provide mechanisms for representing asynchronous Operations naturally in C++.

Conceptually:

```text id="capmap11"
Coroutine Support
    ↓
Async
```

Consumers may include networking, Web, WebSocket, Process, database, and application code.

Coroutine support should not redefine Operation itself.

The Standard Operation concept remains independent from representation.

The current architectural direction is clear, while the final canonical asynchronous public representation may still evolve.

## Cancellation capability

Cancellation provides a common mechanism for expressing and propagating cancellation requests across Operations.

The Standard has already defined the semantic model.

Architecturally, the implementation is expected to live close to Async while remaining usable across domains.

Conceptually:

```text id="capmap12"
Cancellation Capability
    ↓
Runtime Foundation
    closely integrated with Async
```

Consumers may include Async I/O, timers, Process, networking, HTTP, WebSocket, database operations, shutdown orchestration, and CPU work where cancellation is supported.

Cancellation authority remains with Operation or Activity lifetime.

The Provider only helps deliver or observe the request.

The semantic ownership is clear.

The final C++ representation remains open.

## CPU execution capability

CPU execution provides the ability to run processor-bound work outside the immediate caller when required.

The current Provider is Threadpool.

Conceptually:

```text id="capmap13"
CPU Execution Capability
    ↓
Threadpool Provider
```

Known consumers include Game JobSystem and any Platform or Ecosystem Operation that needs explicit CPU offload.

Future consumers may include compression, crypto, parsing, transformation, or application background computation.

The capability is architectural.

`Threadpool` is the current Provider rather than the permanent semantic concept.

The long-term public execution interface remains open.

## Blocking-work execution capability

Blocking-work execution provides isolation for work that may block a thread and would otherwise prevent an execution context such as Async from making progress.

The current likely Provider is also Threadpool.

Conceptually:

```text id="capmap14"
Blocking-work Execution Capability
    ↓
current Provider:
        Threadpool
```

Potential consumers include blocking filesystem operations, foreign-library calls, process waits, legacy APIs, or other work that cannot integrate directly with Async.

CPU execution and blocking-work execution may share a Provider while remaining distinct capabilities.

The final Provider separation and public representation remain open.

## Process capability

Process capability provides the ability to create, observe, communicate with, and wait for operating-system processes according to the applicable contract.

Architectural ownership belongs to the Process area of the Platform.

Conceptually:

```text id="capmap15"
Process Capability
    ↓
Process Provider
    ↓
OS / host process mechanisms
```

Process may consume Path, Environment, Error, Time, Cancellation, Async, or blocking execution depending on the Operation being performed.

Consumers may include Toolchain, applications, Agent-related behavior, testing, orchestration, or other domains.

Some Profiles may not provide process execution.

Process capability should not be defined in terms of a particular Unix or Windows API.

## TCP capability

TCP capability provides stream-oriented network transport semantics.

Architectural ownership belongs to the canonical networking foundation rather than historical broad Net aggregation.

Conceptually:

```text id="capmap16"
TCP Capability
    ↓
TCP Provider
    ↓
Async + OS / host socket mechanism
```

Consumers include HTTP client, HTTP server, WebSocket, database drivers, realtime systems, P2P components, and applications.

TCP Operations should use shared Error, Time, Cancellation, and ownership semantics.

The capability is canonical in architectural intent.

The precise future networking module structure remains subject to later redesign.

## UDP capability

UDP capability provides datagram-oriented network transport.

Its architectural role parallels TCP while retaining different transport semantics.

Conceptually:

```text id="capmap17"
UDP Capability
    ↓
UDP Provider
    ↓
Async + OS / host datagram mechanism
```

Consumers may include networking protocols, P2P, discovery, realtime or application-specific systems.

UDP should not be forced through TCP abstractions merely because both belong to networking.

The canonical capability is clear even if the future networking implementation layout is not yet finalized.

## DNS capability

DNS capability provides hostname or service resolution.

Architectural ownership belongs to the networking Platform.

Conceptually:

```text id="capmap18"
DNS Capability
    ↓
DNS Provider
    ↓
OS / host resolver
    or another conforming resolver implementation
```

Consumers include HTTP client, WebSocket client, database clients, P2P, service communication, and ordinary applications.

DNS may use Async depending on Provider implementation.

Higher-level clients should require name resolution rather than depend directly on a particular resolver library.

## Network reachability capability

Some consumers need to know whether network access appears available without requiring the entire networking stack.

This is distinct from TCP, HTTP, or DNS.

The current migration has already shown this distinction in areas such as cache and sync behavior.

Conceptually:

```text id="capmap19"
Network Reachability Capability
    ↓
Network Probe Provider
```

Consumers may include Cache, Sync, offline-aware behavior, and application connectivity logic.

This capability should remain narrow.

A consumer that only needs reachability information should not depend on the entire historical Net subsystem.

Its current architectural direction is migration toward a dedicated NetworkProbe-style responsibility.

## TLS capability

TLS provides secure transport semantics above an underlying transport.

Architectural ownership belongs to the Platform networking/security capability layer.

Conceptually:

```text id="capmap20"
TLS Capability
    ↓
Vix TLS Provider
    ↓
external TLS library / platform facility
```

Potential lower requirements include transport, Crypto, Time, Error, and Async.

Consumers include HTTPS, secure WebSocket, database drivers, realtime communication, and applications.

External libraries such as OpenSSL are implementation mechanisms, not the Capability itself.

The Provider boundary should preserve this distinction.

## Cryptographic capability

Crypto provides broadly reusable cryptographic facilities required by several domains and Providers.

Architectural ownership belongs to the Platform.

Conceptually:

```text id="capmap21"
Crypto Capability
    ↓
Crypto Provider / implementation
```

Consumers may include TLS, authentication, storage, hashing, signatures, protocols, and application code.

The exact Capability decomposition may eventually distinguish secure randomness, hashing, encryption, signatures, or other needs.

That decomposition should be driven by real dependency requirements rather than created prematurely.

## JSON capability

JSON provides common parsing, serialization, and data-model functionality.

Architectural ownership belongs to the Platform as a broadly reusable data capability.

Conceptually:

```text id="capmap22"
JSON Capability
    ↓
Vix JSON implementation
```

Consumers include HTTP, configuration, WebRPC, storage, tooling, Agent, applications, and other domains.

JSON parsing should use the common Error model.

JSON is broadly useful but should not become a dependency of unrelated foundational primitives such as Time or Path.

## HTTP client capability

HTTP client functionality provides outbound HTTP communication as a general Platform capability.

It is intentionally separated architecturally from the Web server ecosystem.

Conceptually:

```text id="capmap23"
HTTP Client Capability
    ↓
HTTP Client implementation
    ↓
DNS
TCP
TLS where required
Async
Time
Error
```

Consumers may include applications, Agent, Cloud integration, WebRPC, package tooling, service communication, and many future domains.

The HTTP client should compose lower-level Platform capabilities rather than creating separate networking, timeout, or cancellation semantics.

## HTTP server domain

HTTP server behavior is important, but it is not treated as a foundational Platform capability in this architecture.

It belongs to the Web Ecosystem.

Conceptually:

```text id="capmap24"
Web Ecosystem
    HTTP Server
    Routing
    Middleware
    Sessions
    OpenAPI

uses Platform:
    Async
    TCP
    TLS
    Time
    Error
    CPU execution where needed
```

This distinction prevents server-framework semantics from becoming Runtime foundation dependencies.

## WebSocket domain

WebSocket belongs to the Ecosystem as a protocol-level capability built from Platform networking and execution.

Conceptually:

```text id="capmap25"
WebSocket
    ↓ uses
Async I/O
TCP / TLS
Time
Error
```

WebSocket owns protocol semantics, connection Activities, message behavior, and WebSocket-specific Resources.

It should not own a separate Async runtime or foundational lifecycle model.

The current migration away from historical RuntimeExecutor dependencies is aligned with this boundary.

## Database capability

Database support spans multiple layers.

Low-level drivers may behave as Platform-adjacent capabilities, while higher-level database abstractions, ORM, migrations, models, and domain behavior belong to the Ecosystem.

The architecture should avoid deciding that every database concern has the same level.

Conceptually:

```text id="capmap26"
Database transport / driver capability
    may use
        TCP
        TLS
        Async
        Error

Database Ecosystem
    query abstraction
    ORM
    models
    migrations
    higher-level data semantics
```

The exact boundary should be refined through the database architecture rather than assumed from current module names.

## Persistent storage capability

Persistent storage is broader than one database API.

Future Vix environments may need a semantic ability to retain state durably across application lifetimes.

This may be supplied through filesystem, database, KV, host storage, or another Provider depending on the environment.

The capability is useful conceptually, but its canonical Platform contract is not yet fixed.

For now it should be treated as an architectural area requiring further evidence rather than a finalized foundational Capability.

## Key-value storage

KV behavior currently exists as an Ecosystem or specialized storage facility.

It may eventually participate in a broader persistent-storage Capability model.

Until that model is standardized, KV should not be assumed to define persistence for all of Vix.

Its own ownership, Error, and durability guarantees should remain explicit.

## Cache capability

Cache is a higher-level storage and performance behavior.

It may depend on Filesystem, Time, network reachability, persistent storage, or application memory depending on Provider.

Cache should not become the owner of any of those foundational capabilities.

Conceptually:

```text id="capmap27"
Cache
    may use
        Time
        Filesystem
        Network Reachability
        storage Provider
```

The current migration away from broad Net dependency is consistent with this model.

## Sync capability

Sync and durable-outbox behavior belong above lower Platform foundations.

Conceptually:

```text id="capmap28"
Sync
    may use
        Network Reachability
        HTTP Client
        persistent storage
        Time
        Error
        Async
```

Sync should own synchronization semantics.

It should not own network detection, HTTP, time, or storage foundations merely because it composes them.

## Realtime domain

Realtime belongs to the Ecosystem.

It may compose WebSocket or another transport with Activity lifetime, reconnection, timers, and message semantics.

Conceptually:

```text id="capmap29"
Realtime
    ↓
WebSocket / transport
Async
Time
Error
```

Realtime may define reconnect policy, channel semantics, subscriptions, or presence.

It should reuse foundational cancellation, Activity, and Deadline semantics.

## P2P domain

P2P belongs to the Ecosystem.

It may consume several Platform capabilities such as TCP, UDP, DNS, HTTP client, Crypto, Async, and Time.

Its protocol and peer-management semantics remain domain-specific.

Historical dependency on broad Net should continue to be evaluated in terms of the actual capabilities P2P requires.

## Game domain

Game belongs to the Ecosystem.

It may consume Time, Filesystem, input facilities, rendering or UI facilities, and CPU execution.

The existing JobSystem wrapping Threadpool is an example of domain-level composition over a Platform execution Provider.

Conceptually:

```text id="capmap30"
Game JobSystem
    ↓
CPU Execution Capability
    ↓
Threadpool Provider
```

Game may define its own jobs, scenes, events, frames, or scheduling policies without redefining the foundational meaning of execution.

## Agent domain

Agent belongs to the Ecosystem.

It may consume HTTP client, Process, Filesystem, JSON, Async, Time, networking, or other Platform capabilities depending on the Agent contract.

Agent should not depend on historical broad networking or execution modules when narrower canonical capabilities are sufficient.

Its trust, tool, and model semantics remain domain-specific.

## Validation capability

Validation is broadly reusable and currently has canonical ownership through the Validation area.

Validation may be used by application input, configuration, HTTP, data, or domain models.

It is not itself a replacement for operational Error semantics.

A validation result may produce structured Error information.

The architecture should keep Validation focused on checking values and contracts rather than allowing it to become a generic utility aggregation point.

## Input and UI capabilities

Input and UI are specialized Platform/Ecosystem boundaries that may require future refinement.

Basic input-device access could become a Platform capability where multiple domains require it.

High-level UI composition belongs to a domain layer.

The current Standard and architecture do not yet define these boundaries fully.

They should remain open until concrete cross-domain requirements justify a canonical Platform abstraction.

## Current high-level capability graph

The current intended graph can be summarized approximately as:

```text id="capmap31"
Foundation
├── Error / Result
├── Path
├── Filesystem
├── Environment
├── Time
├── Logging
├── Async
└── Cancellation

General Platform Capabilities
├── CPU Execution
│   └── Threadpool Provider
├── Blocking Execution
│   └── current Threadpool Provider
├── Process
├── TCP
├── UDP
├── DNS
├── Network Reachability
├── TLS
├── Crypto
├── JSON
└── HTTP Client

Ecosystem
├── Web
├── WebSocket
├── Database / ORM
├── Cache
├── Sync
├── Realtime
├── P2P
├── Game
├── UI
└── Agent
```

This graph is an architectural direction.

It is not a final module tree.

## Dependency examples

A secure HTTP request should conceptually resemble:

```text id="capmap32"
HTTP Client
    ↓
DNS
TCP
TLS
Time
Async
Error
```

A WebSocket server should resemble:

```text id="capmap33"
WebSocket
    ↓
TCP / TLS
Async
Time
Error
```

A process Operation may resemble:

```text id="capmap34"
Process
    ↓
Path
Environment
Time
Error
Cancellation
Async or blocking execution where needed
```

A CPU-oriented game job may resemble:

```text id="capmap35"
Game
    ↓
CPU Execution
    ↓
Threadpool
```

A cache with offline awareness may resemble:

```text id="capmap36"
Cache
    ↓
Time
Storage
Network Reachability
```

These relationships tell more architectural truth than dependencies such as:

```text id="capmap37"
WebSocket → Core
Cache → Net
Game → Utils
```

because they express the actual reason for dependency.

## Capabilities should not mirror modules

A capability is not created merely because a module exists.

For example, Vix should not automatically define:

```text id="capmap38"
Core Capability
Utils Capability
CLI Capability
```

because those names exist in the repository.

The capability map is derived from semantic abilities.

Modules are then evaluated against that map.

This direction matters during migration.

Otherwise the architecture merely documents historical structure instead of improving it.

## Modules may provide several capabilities

The reverse is also true.

One module or implementation area may provide more than one capability.

For example, Async may currently provide:

```text id="capmap39"
asynchronous I/O progression
timers
coroutine machinery
parts of networking execution
```

Threadpool may provide:

```text id="capmap40"
CPU execution
blocking-work execution
```

This is acceptable.

The capability map does not require every capability to have one physical module.

It requires that consumers and maintainers understand which semantic responsibility is being used.

## One capability may have several Providers

A capability may have multiple Providers.

For example:

```text id="capmap41"
TLS Capability
├── Provider A
└── Provider B
```

or:

```text id="capmap42"
CPU Execution
├── Threadpool Provider
└── future specialized Provider
```

Provider multiplicity should not force domain code to redefine the capability.

The same semantic contract should remain usable where the Providers satisfy it.

## Provider selection is separate

This document maps capabilities to architectural ownership.

It does not define the complete mechanism by which a Provider is selected.

Selection may eventually happen through:

```text id="capmap43"
build configuration
target Profile
dependency construction
runtime discovery
explicit application configuration
```

depending on the capability.

The architecture should not prematurely introduce a universal Provider registry solely because the map contains several Providers.

## Capability and ownership

Capability dependency does not imply Resource ownership.

For example:

```text id="capmap44"
WebSocket
    requires Async I/O Capability

Application
    owns Async Context

WebSocket
    borrows Async Context
```

The capability relationship says what WebSocket needs.

The ownership relationship says who controls the Resource satisfying that need.

These maps complement each other.

`ownership-map.md` records the important lifecycle relationships in more detail.

## Capability and conformance

A Platform implementation may claim a Profile guaranteeing some of these capabilities.

The Profile then creates a conformance obligation.

For example:

```text id="capmap45"
Profile guarantees TCP
```

means the implementation must have a conforming TCP Provider.

It does not mean every TCP connection succeeds.

The Capability Map helps locate which architectural component is responsible for satisfying that guarantee.

## Capability and migration

The Capability Map should be used during historical dependency removal.

For each dependency such as:

```text id="capmap46"
Consumer → Core
Consumer → Utils
Consumer → Net
Consumer → RuntimeExecutor
```

the migration should ask:

> Which capability does the consumer actually need?

If the answer is none, remove the dependency.

If the answer is Time, use the Time foundation.

If the answer is Async I/O, use Async.

If the answer is CPU execution, use the current CPU execution Provider.

If the answer is network reachability, use the narrow reachability capability.

This prevents broad historical dependencies from simply being renamed.

## Capability audit rule

A new foundational or general Platform Capability should not be introduced only because two pieces of code look similar.

Before creating one, architecture should be able to answer:

> What stable semantic ability does this represent?

> Which independent consumers need it?

> Which Providers could plausibly satisfy it?

> What becomes simpler or more portable when this capability is explicit?

> Would inconsistent implementations of this ability create a meaningful problem?

If those questions do not have useful answers, the abstraction may belong to implementation architecture rather than the Capability model.

## Capabilities currently considered canonical

The current architecture has strong evidence for canonical ownership of:

```text id="capmap47"
structured Error / Result
Path
Filesystem
Environment
Time
Logging
Async I/O
Cancellation semantics
CPU execution
Process
TCP
UDP
DNS
network reachability
TLS
JSON
Crypto
HTTP client
```

Some have mature implementations.

Others still need clearer Provider boundaries or API refinement.

Canonical here means that their architectural role is understood, not that their final public C++ form is frozen.

## Capabilities still requiring research

Several possible capabilities remain insufficiently defined to standardize architecturally.

These include areas such as:

```text id="capmap48"
general persistent storage
structured concurrency
general scheduling
execution scope
input devices
hardware acceleration
generic background work
```

They may eventually become first-class capabilities.

For now, the architecture should gather requirements from real consumers rather than filling the map speculatively.

## Map maintenance

This document should change when architectural responsibility changes.

It should not change merely because private implementation files move.

If Threadpool is replaced by another Provider while CPU execution remains the same capability, the Provider mapping changes.

If a historical Net module disappears while TCP and DNS remain available through canonical owners, the capability model does not change.

If Vix discovers that two apparently separate abilities are actually one stable semantic capability, the map may evolve through architecture work and, where semantics change, the VEP process.

## Architectural requirements

Consumers should depend on the narrowest coherent capability that satisfies their contract.

Historical module names must not define capabilities automatically.

A capability may be implemented by more than one Provider.

A Provider may satisfy more than one capability.

Capability dependency must remain distinct from Provider ownership.

Higher-level domains should not redefine foundational capabilities locally.

Provider-specific implementation details should remain below capability boundaries unless intentionally exposed.

Capability absence under a claimed Profile must be distinguishable from ordinary Operation Failure.

Migration away from broad historical dependencies should identify the real capability before introducing any replacement abstraction.

New capabilities should emerge from demonstrated semantic need rather than repository symmetry.

The Capability Map should remain an architectural responsibility map, not a service-locator specification.

## Current migration interpretation

Several completed migration steps can now be described directly through this map.

Historical Utils dependencies were removed because consumers actually needed narrower canonical responsibilities such as Logging, Environment, Result, Validation, and Time.

Historical Net dependencies were reduced because some consumers needed only Network Reachability rather than an entire networking aggregation.

WebSocket Session and LowLevelServer lost RuntimeExecutor because their real execution requirement was asynchronous I/O already provided by Async.

These examples demonstrate the purpose of the map.

The architecture becomes clearer when dependencies name the capability that actually matters.

## Design consequence

The Capability Map should make a dependency understandable before anyone opens the implementation.

Instead of seeing:

```text id="capmap49"
A → B
```

and asking why A needs B, the architecture should increasingly be expressible as:

```text id="capmap50"
WebSocket
    requires Async I/O

Game
    requires CPU Execution

HTTP Client
    requires DNS + TCP + TLS + Time + Async

Cache
    requires Time + Storage + Network Reachability
```

The concrete Provider may change.

The repository layout may change.

The capability requirement should continue to explain the architecture.

That is the purpose of this map.
