# Vix Platform

The Vix Platform is the part of Vix responsible for providing the foundational environment in which Vix software can be built and executed.

It sits below domain-specific functionality such as Web, database access, WebSocket, realtime systems, game systems, or agent-oriented features.

The Platform is not defined by the current repository layout.

It is defined by responsibility.

A component belongs to the Platform when it provides infrastructure that is broadly reusable across domains, participates directly in implementing foundational Vix capabilities, or is necessary for the common development and execution model of Vix applications.

The Platform should remain smaller than the complete Vix ecosystem.

Its purpose is to provide a coherent foundation, not to absorb every useful feature.

## Platform and Ecosystem

Vix is divided conceptually into two large areas:

```text
Vix
│
├── Platform
│
└── Ecosystem
```

The Platform provides the common foundation.

The Ecosystem builds domain-specific behavior on top of that foundation.

For example:

```text
Platform
    time
    filesystem
    environment
    asynchronous I/O
    CPU execution
    networking capabilities
    process execution
    logging
    structured failure

Ecosystem
    HTTP
    WebSocket
    database
    ORM
    realtime
    P2P
    game
    agent
```

The exact implementation modules may change.

The architectural distinction should remain understandable.

A domain does not become part of the Platform merely because several applications use it.

Likewise, a foundational capability does not belong in the Ecosystem merely because its first implementation happened to be created for one domain.

## Platform responsibilities

The Platform has two major responsibilities:

```text
Toolchain
Runtime
```

The Toolchain is concerned with transforming, building, running, diagnosing, packaging, and otherwise supporting Vix software as a development artifact.

The Runtime provides the foundational capabilities used by software while it executes.

Conceptually:

```text
Vix Platform
│
├── Toolchain
│
└── Runtime
```

These parts serve different purposes, but together they form the common environment that makes Vix a platform rather than only a collection of libraries.

## Toolchain

The Toolchain is responsible for the developer-facing machinery required to build and operate Vix software.

This may include areas such as:

```text
project discovery
build orchestration
compiler integration
diagnostics
dependency resolution
code generation
run and development workflows
testing integration
packaging
target selection
```

The Toolchain may use the same foundational concepts as the Runtime, but it should not be confused with the Runtime itself.

For example, both may use structured Errors and filesystem operations.

That does not make compilation an application Runtime responsibility.

Likewise, the Runtime should not need to depend on build orchestration merely because both are part of the Vix Platform.

## Runtime

The Runtime provides the foundational execution environment for Vix applications and higher-level ecosystem components.

Its responsibilities include semantic foundations and general capabilities needed across several domains.

Examples include:

```text
Error and Result
Path
Filesystem
Environment
Time
Logging
Async
Cancellation
Execution
Process
TCP
UDP
DNS
TLS
JSON
Crypto
HTTP client
```

This is an architectural responsibility map, not a declaration that each item must correspond to one module.

Some areas may consist primarily of value types and functions.

Others may contain stateful Providers or Resources.

Some may later be reorganized.

The Platform boundary should remain based on the role these capabilities play.

## Runtime Foundation and general capabilities

Within the Runtime it is useful to distinguish between foundational concepts used nearly everywhere and broader capabilities that applications may or may not require.

Conceptually:

```text
Runtime
│
├── Foundation
│   ├── Error / Result
│   ├── Path
│   ├── Filesystem foundations
│   ├── Environment
│   ├── Time
│   ├── Logging
│   ├── Async
│   └── Cancellation
│
└── General Capabilities
    ├── CPU / blocking execution
    ├── Process
    ├── TCP
    ├── UDP
    ├── DNS
    ├── TLS
    ├── JSON
    ├── Crypto
    └── HTTP client
```

This distinction does not necessarily need to appear as two physical directories.

Its purpose is architectural.

The Foundation contains concepts and mechanisms with broad cross-domain importance.

General Capabilities provide reusable behavior that may be required by some domains and not others.

## Platform capabilities are reusable

A Platform capability should normally be reusable outside the subsystem that first needed it.

For example, DNS resolution may be used by HTTP, WebSocket, database drivers, service discovery, or other future domains.

TCP may support Web, database protocols, realtime systems, or custom applications.

Monotonic time may support networking deadlines, process waits, retries, game timing, and shutdown.

This cross-domain usefulness is one signal that the responsibility belongs in the Platform.

The goal is not to maximize reuse artificially.

The goal is to avoid making foundational infrastructure the private property of one domain.

## Platform does not mean globally mandatory

A capability can belong to the Platform without being required by every Vix program.

For example, process execution may be part of the Platform while being unavailable in some Profiles.

TLS may be available without being needed by a particular application.

CPU worker execution may exist even when a simple synchronous program never uses it.

Platform means that Vix defines and owns the foundational responsibility.

It does not mean every application must initialize, link, or use every Platform capability.

This distinction is important for portability and constrained environments.

## Capability availability and Profiles

Profiles determine which Platform capabilities are guaranteed in a particular environment.

For example, one environment may provide:

```text
filesystem
TCP
DNS
monotonic time
async I/O
```

while another may additionally provide:

```text
process execution
native threads
UDP listeners
```

The Platform defines the capabilities.

Profiles describe which of them are guaranteed.

This allows Vix to support multiple execution environments without fragmenting the semantic meaning of those capabilities.

## Platform is not one runtime object

The existence of the Platform does not imply one universal object named something like:

```text
VixPlatform
VixRuntime
VixContext
Services
```

through which every capability must be accessed.

Such an object may eventually be useful for a specific orchestration problem, but it is not implied by the architecture.

The Platform is a responsibility boundary.

It may be realized through several independent APIs, types, Resources, Providers, and compile-time facilities.

Vix should avoid turning the Platform concept into a global service locator.

## Platform Resources

Some Platform facilities are Resources with meaningful lifetimes.

Examples may include:

```text
execution contexts
worker pools
process handles
sockets
files
timers
connection pools
```

Other Platform facilities may have no meaningful runtime lifetime.

Examples may include pure utility value types, stateless algorithms, or compile-time facilities.

The Platform architecture should preserve this distinction.

Not every capability needs to become a stateful service.

## Platform ownership

The Platform may create or own Resources that are shared by several higher-level components.

For example:

```text
Application orchestration
    owns Async execution context

HTTP Server
    borrows Async execution context

WebSocket
    borrows Async execution context

Realtime
    borrows Async execution context
```

This is a valid Platform relationship.

The higher-level domains depend on the capability without gaining lifecycle authority over the shared Resource.

Platform architecture should make such ownership relationships explicit where they affect shutdown or Resource validity.

## Platform Providers

Capabilities may be implemented by Providers.

For example:

```text
TCP Capability
    ↓
native TCP Provider

TLS Capability
    ↓
OpenSSL-backed Provider

CPU execution Capability
    ↓
Threadpool Provider
```

Providers may differ between platforms.

Higher-level Ecosystem components should generally not depend on the concrete Provider unless they intentionally require Provider-specific behavior.

This keeps domain code portable and allows Vix to replace mechanisms without rewriting the Ecosystem.

## Platform and external libraries

The Platform may use external libraries as implementation mechanisms.

For example:

```text
Asio
OpenSSL
system networking APIs
system process APIs
```

These technologies may be extremely important to the implementation.

They are still beneath the Vix capability boundary.

The architecture should normally be described as:

```text
Vix capability
    ↓
Vix Provider
    ↓
external mechanism
```

rather than allowing the external library to become the architectural contract consumed directly throughout the Ecosystem.

## Platform and historical modules

The current repository contains historical modules whose names or boundaries do not necessarily correspond to the intended Platform architecture.

Examples include broad aggregation or compatibility areas such as historical `core`, `utils`, or `net`.

These names should not be treated as permanent Platform concepts merely because existing code depends on them.

Migration should ask:

> What responsibility is this code actually providing?

A historical module may contain several responsibilities that eventually belong in different canonical areas.

For example:

```text
historical Core
    may contain:
        Web behavior
        execution behavior
        configuration
        compatibility code
```

The architectural goal is to assign each responsibility to its real owner rather than preserve the aggregation indefinitely.

## Core is not the Platform

The Platform should not be represented by a generic `core` module that accumulates unrelated infrastructure.

A broad Core may appear convenient early in a project because foundational code needs somewhere to live.

Over time, this can hide responsibility.

A dependency such as:

```text
WebSocket → Core
```

does not explain whether WebSocket needs:

```text
time
execution
configuration
logging
filesystem
networking
```

or something else.

The current architecture should move toward dependencies that reflect the actual required capability.

`Core` may remain temporarily for compatibility or migration, but it should not define the future Platform boundary.

## Utils is not the Platform

Generic utility aggregation has a similar problem.

If logging, time, environment access, validation, Result, and unrelated helpers all live under one utility boundary, consumers gradually depend on the whole collection.

The architectural direction is to give foundational responsibilities canonical homes.

For example:

```text
logging
    owned by logging foundation

time
    owned by time foundation

environment
    owned by environment foundation

Result
    owned by Error foundation
```

This makes dependencies meaningful.

A domain should depend on what it uses rather than on a generic collection of helpers.

## Historical networking aggregation

Networking should also be modeled in terms of actual capabilities.

A historical networking module may have accumulated multiple concerns such as transport, DNS, HTTP, probing, or compatibility wrappers.

The Platform architecture should instead identify the required capability.

For example:

```text
Cache
    requires network reachability information
```

does not necessarily mean:

```text
Cache
    requires entire Net subsystem
```

Likewise:

```text
WebSocket
    requires asynchronous transport
```

does not imply dependence on every networking abstraction Vix has ever provided.

Capability-based boundaries reduce accidental coupling.

## Ecosystem components

The Ecosystem contains features whose primary meaning belongs to a specific application domain.

Examples include:

```text
HTTP server and routing
WebSocket protocol
Realtime messaging
Database abstractions
ORM
KV
P2P
Game
UI
Agent
```

These components may be important and widely used.

That does not automatically make them foundational Platform responsibilities.

They build richer domain semantics using the Platform.

For example:

```text
WebSocket
    uses TCP / TLS / Async / Time

Database
    may use TCP / TLS / filesystem / execution

Game
    may use Time / CPU execution / filesystem
```

The dependency should generally flow from Ecosystem toward Platform.

## Domain-specific protocols remain outside the Platform

A protocol may depend heavily on Platform facilities while still belonging to the Ecosystem.

HTTP server routing is a good example.

The Platform may provide:

```text
TCP
TLS
Async
Time
structured Error
```

while the Web domain provides:

```text
HTTP requests
HTTP responses
routing
middleware
sessions
server behavior
OpenAPI
```

This keeps the Runtime focused on reusable infrastructure.

The Platform should not become a collection of every protocol Vix supports.

## HTTP client as a Platform capability

HTTP client functionality may reasonably belong to the general Platform capabilities because many domains and ordinary applications need the ability to communicate with remote HTTP services without adopting the full Web server ecosystem.

This is architecturally different from:

```text
HTTP server
routing
middleware
session management
```

which are domain-level Web concerns.

The exact implementation boundary may evolve, but the distinction is useful:

```text
general outbound communication capability
    Platform

Web application/server framework semantics
    Ecosystem
```

The architecture should preserve that difference.

## JSON and serialization

JSON is broadly reusable across the Platform and Ecosystem.

It may participate in configuration, HTTP, persistence, protocols, tooling, and application code.

For this reason, canonical JSON support can reasonably be treated as a general Platform capability.

That does not mean every future serialization format belongs in the Runtime automatically.

A format should be placed according to its breadth of use and architectural role.

The Platform should not become a catalog of data formats.

## Crypto

Cryptographic primitives may also serve several domains and Providers.

TLS may require them.

Authentication may use them.

Storage or protocol implementations may use hashes, signatures, or secure randomness.

Canonical cryptographic capability therefore belongs naturally in the Platform rather than being privately owned by one higher-level domain.

Security-sensitive abstractions still require careful contracts.

The Platform boundary does not by itself standardize every cryptographic algorithm or policy.

## Process execution

Process execution belongs naturally in the Platform because it is a general operating-environment capability rather than a domain-specific application abstraction.

Toolchain code may use it.

Applications may use it.

Higher-level Ecosystem components may use it.

Some Profiles may not provide it.

The capability should therefore remain separate from assumptions about a particular operating system.

## Filesystem and Path

Path and filesystem capabilities belong to the Platform because they are broadly reusable and closely related to Resource and environment semantics.

Path should describe filesystem locations independently from one domain.

Filesystem Operations should follow the common Operation, Failure, cancellation, and ownership model.

Higher-level systems such as Web, database, cache, tooling, and game assets may all depend on these foundations.

They should not each invent private filesystem semantics.

## Environment

Environment access belongs in the Platform because configuration and process-environment interaction are cross-domain concerns.

This does not mean every configuration system must be part of the Platform.

Reading environment values is foundational.

An application-specific configuration schema belongs at a higher level.

The Platform should provide general mechanisms without absorbing domain policy.

## Logging

Logging belongs to the Runtime Foundation because many parts of Vix need a common diagnostic mechanism.

This does not mean every domain must use the same log messages or severity policy.

The Platform provides the shared facility.

Domains contribute context and policy appropriate to their behavior.

Logging should also remain distinct from structured operational Error semantics.

A log record does not replace an Error value.

## Time

Time belongs to the Platform because Deadline, Duration, timers, retries, scheduling, shutdown, and many domain lifetimes depend on it.

The Platform should provide the canonical time model.

Domains may define their own time-related policies, such as HTTP request deadlines or session expiry.

They should build those policies using the shared time foundation rather than inventing incompatible clock or timeout semantics.

## Async

Async belongs to the Platform because asynchronous work and I/O progression are general execution concerns used by several domains.

Async should provide foundational mechanisms without becoming a container for all domain behavior.

For example, WebSocket may execute over Async.

That does not make WebSocket part of Async.

Likewise, Async may support TCP Operations without needing to own HTTP routing semantics.

The boundary should remain capability-oriented.

## CPU execution

CPU and blocking-work execution belong to the Platform as general execution capabilities.

The current Threadpool implementation may serve as a Provider.

That does not mean `Threadpool` itself is the Standard concept.

Higher-level components should depend on CPU execution needs rather than unnecessarily depending on one concrete pool implementation.

This makes future replacement possible.

## Platform dependency direction

The intended dependency direction is approximately:

```text
Ecosystem
    ↓
Platform public capabilities
    ↓
Platform Providers
    ↓
OS / external libraries / hardware
```

Dependencies within the Platform may also form layers.

For example:

```text
HTTP Client
    ↓
TLS / TCP / DNS / Time
    ↓
Async / OS mechanisms
```

The exact graph will be documented in `capability-map.md`.

The important property is that a lower-level capability should not depend on a higher-level domain merely to obtain functionality that belongs below it.

## Platform layering should remain shallow

A Platform can become difficult to understand if every capability is wrapped through many abstract layers.

The architecture should avoid abstraction solely for symmetry.

A capability may be implemented directly over an operating-system facility when that is sufficient.

Another may require a stateful Provider.

Another may be a header-only value model.

The Platform should not force every responsibility into the same architectural shape.

Coherence of semantics is more important than visual symmetry of implementation.

## Platform portability

The Platform should isolate platform-specific mechanisms where practical.

For example:

```text
Process Capability
    Linux Provider
    Windows Provider
    other Provider
```

may eventually exist.

The public semantic contract should remain stable.

Platform-specific code is expected.

Platform-specific meaning should be minimized.

This is how Vix can support different environments without making every application branch on operating-system details.

## Platform failure boundaries

Platform Operations may fail operationally.

A DNS lookup may fail.

A socket connection may be refused.

A process may not start.

A file may be missing.

These are ordinary Operation Failures when the corresponding Capability exists.

By contrast, if the selected Profile guarantees process execution but no process Provider exists, that is a Platform conformance or configuration problem.

The Platform architecture should preserve this distinction so that diagnostics can explain the right problem.

## Platform startup

Some Platform facilities may require startup.

For example, stateful execution Providers or shared runtime Resources may need initialization.

The architecture should avoid one universal startup requirement when independent capabilities can be used independently.

A simple program that only uses Path and Result should not necessarily need to initialize a large global Runtime.

Where orchestration is necessary, ownership should remain explicit.

For example:

```text
Application
    creates Async Context
    creates CPU Execution Provider
    starts required Activities
```

The exact API is not yet standardized.

## Platform shutdown

Shutdown follows the same ownership model.

A Platform Resource should be terminated by its owner or by an explicitly authorized orchestrator.

Higher-level Ecosystem components must not shut down shared Platform Resources simply because they borrowed them.

For example:

```text
Application
    owns Async Context

HTTP Server
    borrows Async Context

WebSocket Server
    borrows Async Context
```

Stopping either server should not stop the Async Context unless orchestration explicitly assigns that authority.

This principle should guide both current migration and future API design.

## Platform evolution

The Platform architecture may evolve while the Standard remains stable.

A capability may move between modules.

A Provider may be replaced.

Several historical abstractions may disappear.

A new Provider may support a new target.

These are normal architectural changes.

If the semantic contract exposed to software remains the same, the Standard need not change.

If architectural work reveals that the Standard lacks a concept required across several capabilities, that question should return to research or VEP.

## Architectural test

When deciding whether something belongs in the Platform, several questions are useful.

Does the responsibility represent a broadly reusable capability rather than one domain's application semantics?

Would several independent domains otherwise need to implement the same foundation?

Does the responsibility participate directly in implementing Standard concepts or Profile guarantees?

Can the responsibility be described independently from one current consumer?

Would placing it in the Ecosystem create an inverted dependency from foundational code toward a higher-level domain?

No single question decides every case.

Together they provide a useful architectural test.

## Current Platform direction

The current Platform direction is to make foundational ownership increasingly explicit.

This means reducing dependencies on historical aggregation modules, identifying the actual Capability each consumer needs, and allowing domain components to depend directly on canonical foundations.

The direction can be summarized as:

```text
historical aggregation
    ↓
identify responsibility
    ↓
assign canonical Platform owner
    ↓
expose required Capability
    ↓
migrate consumers
    ↓
retain compatibility only where necessary
```

This work should be incremental.

The objective is not to rewrite working subsystems merely to produce a cleaner diagram.

The objective is to remove ambiguous responsibility where that ambiguity causes coupling, lifecycle errors, duplicated semantics, or difficulty evolving the Platform.

## Design consequence

The Platform should answer:

> What common foundation does Vix provide before domain-specific behavior begins?

The answer should be expressed in terms of responsibilities and capabilities rather than historical modules.

A strong Platform gives Web, database, realtime, game, agent, and future domains a common foundation without forcing them into one implementation mechanism.

It should be powerful enough that domains do not need to reinvent foundational semantics.

It should remain small enough that domain-specific behavior does not become permanent runtime infrastructure.

The Platform is where Vix provides the common ground.

The Ecosystem is where that ground becomes specialized software.
