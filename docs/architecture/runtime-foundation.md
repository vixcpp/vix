# Runtime Foundation

The Runtime Foundation is the part of the Vix Platform that provides the common primitives and execution semantics used throughout the rest of the Runtime and the Ecosystem.

Its purpose is not to contain every generally useful library.

It exists to give Vix one canonical implementation home for the small set of responsibilities that many domains depend on and whose fragmentation would make the programming model inconsistent.

The Runtime Foundation is where the semantic concepts defined by the Vix Standard begin to acquire concrete implementation ownership.

The Standard defines what concepts such as Error, Duration, Deadline, Cancellation, borrowing, and asynchronous Operation mean.

The Runtime Foundation determines which Vix components are responsible for providing the canonical primitives and mechanisms that realize those concepts.

## Position in the Platform

The Runtime Foundation sits below broader Platform capabilities and below the Ecosystem.

Conceptually:

```text
Vix Platform
│
├── Toolchain
│
└── Runtime
    │
    ├── Foundation
    │   ├── Error / Result
    │   ├── Path / Filesystem foundation
    │   ├── Environment
    │   ├── Time
    │   ├── Logging
    │   └── Async / Cancellation
    │
    └── General Capabilities
        ├── CPU and blocking execution
        ├── Process
        ├── TCP / UDP / DNS
        ├── TLS
        ├── JSON
        ├── Crypto
        └── HTTP client
```

The physical repository does not need to reproduce this diagram exactly.

Some responsibilities may live in separate modules. Some closely related responsibilities may share implementation infrastructure.

The diagram expresses architectural ownership, not required directory structure.

## Why a Runtime Foundation exists

Without a clear foundation, cross-domain concerns tend to be implemented repeatedly.

One subsystem defines its own timeouts.

Another invents a different cancellation model.

Another treats a shared execution object as locally owned.

Another returns plain strings for Errors.

Another introduces a broad runtime abstraction simply because several unrelated facilities need somewhere to live.

Individually, each decision may appear reasonable.

Together, they create several incompatible versions of the same programming model.

The Runtime Foundation exists so that higher-level code does not need to solve those problems independently.

A domain should be able to build its own semantics while relying on one common foundation for the concepts that are genuinely shared.

## What belongs in the Foundation

A responsibility belongs in the Runtime Foundation when inconsistency in that responsibility would affect many otherwise independent Vix domains.

This includes responsibilities such as:

```text
structured operational failure
common Result semantics
path representation
basic filesystem access
environment access
time primitives
logging infrastructure
asynchronous Operation machinery
cancellation machinery
```

These responsibilities are not grouped together because they use identical implementation techniques.

They are grouped because they form common infrastructure on which broader Platform capabilities and Ecosystem domains can rely.

The Foundation should remain deliberately small.

A feature should not be placed here merely because it is convenient or broadly useful.

## What does not belong in the Foundation

The Runtime Foundation should not absorb every reusable feature.

For example:

```text
HTTP server routing
WebSocket protocol semantics
ORM
database abstractions
realtime messaging
game scene management
P2P protocols
agent behavior
```

belong to higher-level domains.

Even some general Platform capabilities do not need to be part of the Foundation itself.

Process execution, TLS, TCP, JSON, cryptography, and an HTTP client can be broadly reusable without being foundational to the meaning of every Vix program.

This distinction keeps the Foundation from becoming another historical `core`.

## Foundation does not mean one dependency

The Runtime Foundation is an architectural layer.

It is not one C++ library that every component must link against wholesale.

A component that only needs Time should be able to depend on Time.

A component that only needs structured Error should not automatically gain dependencies on Async, filesystem, or logging.

The architecture should prefer narrow dependency relationships even among foundational responsibilities.

Conceptually:

```text
Consumer
    ↓
specific foundation responsibility
```

rather than:

```text
Consumer
    ↓
entire Runtime Foundation object
```

The Foundation is unified semantically while remaining decomposable physically.

## Error and Result

Structured operational failure belongs in the Runtime Foundation because nearly every domain needs a way to express expected inability to satisfy an Operation contract.

The canonical ownership should reside with the Vix Error foundation.

Its responsibilities include the common representation needed for concepts such as:

```text
Error
Result<T>
error identity
diagnostic context
causal information
```

The exact public representation may evolve.

The architectural rule is that Vix should not maintain several competing foundational Result models.

Domains may define domain-specific Error identities.

For example, networking may define connection-related Errors and a database domain may define constraint-related Errors.

Those Errors should participate in the shared Error model rather than creating unrelated mechanisms.

Conceptually:

```text
Error Foundation
    defines canonical structured failure machinery
        ↑
        ├── Filesystem Errors
        ├── Network Errors
        ├── Process Errors
        ├── Database Errors
        └── Web Errors
```

Domain specificity belongs above the common structure.

## Result is not the entire completion model

The Runtime Foundation may provide a canonical `Result<T>` representation for value-or-operational-Failure behavior.

That does not mean `Result<T>` must become the representation of every Standard completion concept.

The Standard already distinguishes Success, Failure, and Cancelled completion.

The concrete relationship between asynchronous completion and `Result<T>` remains an API-design question.

Architecture should therefore avoid forcing Error ownership to decide unresolved cancellation representation prematurely.

The Error foundation owns structured operational Failure.

Async may own the machinery that represents asynchronous completion.

The eventual API must compose those responsibilities without collapsing their meanings.

## Fault handling

Programming Faults are semantically distinct from operational Failure.

The Runtime Foundation may eventually provide common diagnostic or assertion facilities for Faults, but it should not automatically encode every programming defect as `Result<T>`.

The precise Vix Fault policy remains a separate design question.

Architecture must preserve room for assertions, diagnostics, exceptions, termination, sanitizers, or other appropriate mechanisms depending on the contract.

The important boundary is that the Error foundation should not become a mechanism for hiding every broken invariant as ordinary application Failure.

## Path

Path representation belongs in the Runtime Foundation because paths are used by filesystem operations, tooling, configuration, package handling, application code, assets, databases, and many other areas.

Vix should have one canonical path model rather than domain-specific string conventions.

The Path responsibility should provide portable representation and manipulation of filesystem-oriented paths without importing unrelated filesystem I/O where separation is useful.

For example:

```text
Path
    representation
    normalization rules
    composition
    inspection
```

may be usable independently from:

```text
Filesystem
    open
    read
    write
    remove
    enumerate
```

The physical modules may remain separate if that produces cleaner dependencies.

## Filesystem

Filesystem behavior belongs to the Platform foundation because it is broadly reusable and closely tied to Resource, Operation, ownership, and Failure semantics.

The filesystem foundation should provide canonical Vix behavior for general filesystem operations.

Higher-level domains may build specialized abstractions on top of it.

For example:

```text
Toolchain
    uses Filesystem

Database
    may use Filesystem

Game assets
    may use Filesystem

Cache
    may use Filesystem
```

Those consumers should not need separate foundational filesystem models.

Filesystem support may vary by Profile.

A Profile without filesystem Capability does not invalidate the conceptual ownership of filesystem semantics within the Platform.

## Filesystem Resources

Files, directory iterators, watchers, mappings, and related facilities may be Resources.

Their implementation should follow the common ownership model.

For example, opening a file may be an Operation that produces an owned File Resource.

A watcher may behave as an Activity.

A filesystem timeout or cancellation path should use the common Time and Cancellation semantics when supported.

The filesystem foundation therefore consumes the Standard model rather than inventing a private one.

## Environment

Environment access belongs in the Runtime Foundation because process and host environment values are frequently needed by Toolchain code, applications, configuration systems, Providers, and domains.

The responsibility should remain narrow.

The Environment foundation provides access to environment values and related low-level environment behavior.

It should not become the universal application configuration system.

Conceptually:

```text
Environment
    get/set/query environment state
```

is foundational.

```text
Application Configuration
    schema
    defaults
    validation policy
    configuration hierarchy
```

is a higher-level concern.

Keeping these distinct prevents Environment from becoming another generic service container.

## Time

Time is one of the most important Runtime Foundation responsibilities because it participates in many otherwise unrelated areas.

Examples include:

```text
network deadlines
timers
retry budgets
process waits
shutdown grace periods
cache expiration
Activity timing
game timing
observability
```

The Time foundation should own the canonical Vix representation of foundational time concepts when those representations are standardized.

These include concepts such as:

```text
Duration
Instant
Clock
Deadline
```

The Standard defines their semantics.

The Runtime Foundation gives those semantics a canonical implementation home.

## Time should not be owned by Async

Async will use Time heavily.

That does not mean Time should conceptually belong to Async.

Synchronous code may also need Duration, Clock, or Deadline.

Processes, caching, application logic, scheduling, and Toolchain code may use time independently from asynchronous I/O.

The dependency direction should therefore normally be:

```text
Time
    independent foundation

Async
    uses Time
```

rather than making all time concepts available only through an Async subsystem.

This keeps foundational time reusable.

## Timers

Timer implementation sits near the boundary between Time and execution.

Time defines the semantic primitives.

An asynchronous timer also requires some execution mechanism to observe a Deadline and resume or complete work.

Architecture may therefore divide responsibility conceptually as:

```text
Time
    Duration
    Clock
    Instant
    Deadline

Async / Timer Provider
    waiting
    asynchronous timer Operations
    completion delivery
```

The exact API remains to be decided.

The important boundary is that the timer mechanism should consume the common Time model rather than defining another one.

## Logging

Logging belongs in the Runtime Foundation because diagnostics and observability are needed throughout both Platform and Ecosystem code.

The Logging foundation should provide a canonical mechanism for structured or formatted runtime log emission.

It should not redefine operational Error.

For example:

```text
Error
    describes why an Operation failed

Log record
    records information about execution
```

An Error may be logged.

A log message does not replace the Error returned to the caller.

Likewise, a domain may add context or choose severity policy without owning a separate logging framework.

## Logging dependency direction

Foundational logging should remain low enough in the architecture that most of the Runtime can use it without dependency cycles.

At the same time, extremely low-level primitives should not require complex logging infrastructure merely to function.

For example, Error creation should not depend on a global logger being initialized.

A Failure must remain representable even when logging is unavailable.

This suggests an important architectural principle:

```text
diagnostics may consume semantic state

semantic correctness must not depend on diagnostics
```

Logging supports the Runtime.

It should not become a prerequisite for basic correctness.

## Async

Async is a central Runtime Foundation responsibility because several domains need asynchronous Operations and asynchronous I/O progression.

The Async foundation should provide the common machinery required for asynchronous work without becoming a universal home for every form of execution.

Its responsibilities may include areas such as:

```text
coroutine integration
asynchronous Operation representation
I/O progression
completion dispatch
cancellation integration
timers
asynchronous resource coordination
```

The exact API surface remains subject to further design.

The architectural ownership is clearer: cross-domain asynchronous semantics should have one canonical foundation rather than being independently reinvented by Web, WebSocket, process, database, and other domains.

## Async is not all execution

Async must not become synonymous with all work performed by Vix.

CPU-heavy computation has different requirements from asynchronous socket I/O.

Blocking foreign-library work may need offloading.

A synchronous Operation may require no Async machinery.

The architecture should therefore distinguish:

```text
Async
    asynchronous I/O and Operation progression

CPU execution
    processor-bound work

Blocking execution
    isolation of blocking work
```

Providers may share implementation infrastructure where appropriate.

The conceptual capabilities remain separable.

## Coroutines

C++ coroutines are a useful representation for asynchronous Operations.

They may form an important part of the canonical Vix Async API.

They should not become the semantic definition of Operation itself.

The Runtime Foundation can use coroutines aggressively while preserving the Standard rule that an Operation is a semantic concept independent of representation.

This matters because some internal Operations may use callbacks, platform requests, state machines, synchronous functions, or other mechanisms.

Architecture should not require every internal action to become a coroutine simply for visual uniformity.

## Cancellation

Cancellation belongs closely with Async because asynchronous Operations are one of the primary places where cancellation state must propagate and be observed.

However, Cancellation is semantically broader than one networking or coroutine implementation.

The Runtime Foundation should provide one canonical cancellation mechanism once its representation has been standardized.

That mechanism must satisfy the Standard model:

```text
request cancellation
    ≠ terminal completion

cancellation may propagate
cancellation may race with success
one Operation still completes once
```

The current architecture should avoid creating additional local cancellation types unless a domain genuinely requires specialized behavior.

## Cancellation ownership

Cancellation authority should follow Operation and Activity lifetime rather than execution-provider ownership.

For example:

```text
Parent Operation
    may request cancellation of Child Operation

Execution Context
    progresses Child Operation
```

The execution context does not necessarily own the child lifetime.

Likewise, an Async provider may deliver cancellation but does not automatically decide which work should be cancelled.

This separation prevents cancellation policy from becoming hidden inside infrastructure.

## Async execution context

The Async implementation may use a stateful execution context.

That context is a Resource.

Normal ownership rules apply.

For example:

```text
Application
    owns Async Context

HTTP
    borrows Async Context

WebSocket
    borrows Async Context

Realtime
    borrows Async Context
```

This relationship should remain explicit in architecture.

A domain using Async does not gain authority to terminate shared Async infrastructure.

This rule directly affects shutdown design.

## Foundation dependency direction

The Runtime Foundation should aim for an understandable dependency graph.

A simplified direction may look like:

```text
Error
Path
Environment
Time
Logging
    ↑
    │
Async
```

This diagram is intentionally incomplete.

It does not mean Async must depend on every listed foundation.

It illustrates that foundational value and diagnostic concepts should not unnecessarily depend upward on complex asynchronous execution machinery.

For example:

```text
Time
    should not require Async

Error
    should not require HTTP

Path
    should not require Filesystem Activity machinery

Logging
    should not require Web
```

Dependency direction should follow responsibility.

## Foundation and CPU execution

CPU execution is a general Runtime Capability adjacent to the Foundation.

The current Threadpool implementation may provide it.

Threadpool should not automatically become part of the semantic Foundation simply because Async or Game uses it.

The architectural relationship is closer to:

```text
CPU Execution Capability
    ↓
Threadpool Provider
```

Higher-level code that requires CPU work should consume that capability.

If Async needs CPU offload for a particular operation, Async may compose with the CPU execution capability.

It should not make Threadpool a hidden requirement of all asynchronous Operations.

## Blocking-work execution

Blocking-work isolation is another execution capability.

It may be provided by the same Threadpool implementation as CPU execution, but the semantic need is different.

A long CPU computation and a blocking foreign-library call may both occupy worker threads, yet they may require different scheduling, cancellation, or sizing policies.

The architecture should avoid deciding that one historical Threadpool class permanently defines both capabilities.

Provider reuse is allowed.

Semantic conflation is not required.

## Foundation and Process

Process execution is a general Platform capability built on top of foundational concepts.

It should consume:

```text
Path
Environment
Error
Time
Cancellation
execution capabilities where required
```

without becoming part of their implementation.

For example, process waiting may use Time and Async.

That does not make Time depend on Process.

This direction keeps the Foundation below process-specific behavior.

## Foundation and networking

TCP, UDP, and DNS are general Platform capabilities.

They commonly depend on Async, Error, Time, and possibly other foundational responsibilities.

Conceptually:

```text
TCP / UDP / DNS
    ↓
Async
    ↓
platform I/O mechanisms

TCP / UDP / DNS
    uses Error
    uses Time
```

The networking capabilities should not need a historical broad `net` abstraction merely to access these foundations.

Consumers should also not need the entire networking stack when they require only one smaller capability.

## Foundation and TLS

TLS is a general Platform capability layered over transport and cryptographic facilities.

It may use:

```text
transport
Crypto
Time
Error
Async
```

depending on implementation.

TLS should not own a separate Deadline or cancellation model.

Its Operations participate in the shared Runtime Foundation semantics.

Likewise, a TLS Provider may use an external library without making that external library part of the Foundation.

## Foundation and JSON

JSON is broadly reusable but is not itself a foundational execution concept.

It belongs among general Platform capabilities or common data facilities.

It should use common Error semantics where parsing can fail.

JSON should not become part of the Error foundation merely because Errors may serialize to JSON.

Dependency should follow actual responsibility.

## Foundation and Crypto

Crypto is a general Platform capability rather than a universal Runtime semantic primitive.

It may provide hashes, secure randomness, encryption, signing, or other facilities depending on its eventual contract.

TLS and higher-level domains may consume it.

The Foundation should not depend on high-level cryptographic policy.

Where secure randomness is required by a foundational Provider, the dependency should be expressed as a specific capability rather than pulling all Crypto behavior downward.

## Foundation and HTTP client

An HTTP client composes several Platform capabilities and foundational responsibilities.

Conceptually:

```text
HTTP Client
    ↓
DNS
TCP
TLS
Time
Async
Error
```

It belongs above those foundations.

The HTTP client should not define new foundational completion, timeout, cancellation, or Error machinery simply because it exposes a convenient higher-level API.

This relationship is a practical example of why the Runtime Foundation exists.

## Foundation and Ecosystem

Ecosystem components should consume the Runtime Foundation through the responsibilities they actually require.

For example:

```text
WebSocket
    uses Async
    uses Time
    uses Error
    uses transport

Game
    uses Time
    may use CPU execution
    uses Filesystem for assets

Database
    uses Error
    may use Async
    may use TCP/TLS
    may use Filesystem

Realtime
    uses Async
    uses Time
    uses transport
```

They should not reimplement those foundational concerns simply because domain behavior differs.

Domain-specific semantics remain above the Foundation.

## One canonical owner

Each foundational responsibility should have one canonical architectural owner.

This does not mean one class.

It means that Vix should know where the canonical implementation and semantic representation live.

For example:

```text
structured operational Error
    → Error foundation

Path representation
    → Path foundation

filesystem Operations
    → Filesystem foundation

environment access
    → Environment foundation

Duration / Clock / Deadline
    → Time foundation

logging machinery
    → Logging foundation

asynchronous Operation machinery
    → Async foundation

canonical cancellation machinery
    → Async / cancellation foundation
```

The precise C++ APIs may still evolve.

The architectural responsibility should no longer be ambiguous.

## Historical Utils migration

Historically, utility aggregation can cause foundational concepts to become mixed together.

The current architectural direction is to dissolve that ambiguity.

Responsibilities such as logging, environment, Result, validation, and time should have canonical homes rather than depending on a broad Utilities boundary.

The migration pattern is:

```text
historical utility responsibility
    ↓
identify semantic owner
    ↓
move or expose canonical implementation
    ↓
migrate consumers
    ↓
remove dependency on broad Utils aggregation
```

Compatibility wrappers may remain temporarily where necessary.

They should not remain the canonical ownership model.

## Historical Core migration

Historical Core has also accumulated responsibilities that do not belong to one long-term architectural boundary.

Some code may belong to Web.

Some may belong to execution infrastructure.

Some may be configuration or compatibility code.

Runtime Foundation architecture should not preserve Core merely because many existing modules know how to include it.

Instead, each dependency should be audited.

The migration question should be:

> Which actual foundational or domain responsibility does this consumer require?

If the answer is Async, Time, Error, or another specific capability, the dependency should eventually express that directly.

If no real responsibility is required, the dependency may simply disappear.

## Historical Net migration

Historical networking aggregation should be treated similarly.

A consumer may need:

```text
TCP
DNS
network reachability
HTTP
```

without requiring one universal `Net` dependency.

The Runtime architecture should identify the actual capability and migrate consumers toward its canonical owner.

This is especially important because networking often touches Async, Time, TLS, Error, and process-wide Resources.

Broad aggregation can hide those relationships.

## Avoid replacement aggregators

Removing `core`, `utils`, or historical `net` only to introduce a new object containing the same unrelated responsibilities would not solve the architectural problem.

For example, replacing them with:

```text
RuntimeContext
    Error
    Time
    Logging
    Async
    Filesystem
    Network
    Process
    Everything else
```

would preserve most of the same coupling under a new name.

The Runtime Foundation should be coherent without becoming monolithic.

Consumers should depend on explicit responsibilities.

## Foundation startup

Most foundational facilities should not require one global startup sequence merely because they belong to the same architectural layer.

Value-oriented facilities such as Path, Duration, Error, and Result should generally be usable independently.

Stateful Providers may require initialization.

For example:

```text
Async Context
Worker Pool
Logging sink
```

may have lifetimes.

Their initialization should follow their own Resource contracts.

A simple Vix program should not need to initialize unrelated runtime infrastructure merely to use a foundational value type.

## Foundation shutdown

Stateful Foundation Resources must follow the ownership model.

An owner is responsible for their lifecycle.

Borrowers use them without gaining shutdown authority.

For example:

```text
Application
    owns Async Context

WebSocket Server
    borrows Async Context
```

Stopping the WebSocket Server ends the Activity and its own Resources.

It does not stop the borrowed Async Context.

This pattern should become normal throughout the Runtime rather than being handled as a special case in each subsystem.

## Foundation APIs should remain simple

The existence of a precise architectural model does not mean public APIs should expose every Foundation concept explicitly.

A simple filesystem call should remain simple.

A normal HTTP request should not require manual construction of Error, Cancellation, Provider, Clock, and execution objects unless the programmer actually needs control over them.

The Foundation exists partly so that higher-level APIs can provide strong defaults with coherent underlying semantics.

Precision belongs in contracts.

Complexity should only surface where the caller needs to make a meaningful decision.

## Foundation should support composition

The Runtime Foundation should make higher-level composition easier.

A domain combining Async, Time, Error, and Resources should not need adapter layers merely because each foundation uses incompatible concepts.

For example, a Deadline should flow naturally through Async networking Operations.

Cancellation should be usable by process waits and network Operations without redefining what a cancellation request means.

Errors from different domains should support coherent context and causal information.

The Foundation succeeds when these responsibilities compose naturally while remaining separately owned.

## Foundation portability

The Foundation must be implementable across different Vix Profiles.

Not every Profile provides every general capability.

The foundational concepts themselves should avoid unnecessary assumptions about a particular operating system or execution model.

For example:

```text
Duration
Error
Result
Path
Cancellation semantics
```

should not fundamentally depend on Linux.

Async Providers may differ substantially across platforms.

Filesystem support may vary.

Logging sinks may vary.

The architectural responsibility remains stable while Providers adapt to the environment.

## Foundation testing

Tests for the Runtime Foundation should focus on both local correctness and cross-component semantic guarantees.

Examples include:

```text
Error identity remains stable when context is added
borrowed Resource is not stopped by borrower
Deadline uses the correct clock semantics
Cancellation races still produce one completion
Async Operation does not outlive invalid borrowed state
```

Where a concept is standardized, tests should increasingly validate the Standard contract rather than accidental private implementation structure.

Architecture tests may additionally verify intended dependency direction.

## Foundation migration policy

Migration toward this architecture should remain incremental.

For each historical dependency, the process should be:

```text
1. identify what the consumer actually uses
2. identify the canonical architectural owner
3. determine whether an existing API already satisfies the need
4. remove unnecessary dependency
5. introduce a new abstraction only when a real semantic need remains
6. validate behavior
```

The fourth and fifth steps are especially important.

Removing an old dependency does not imply that Vix needs a new replacement interface.

Sometimes the correct architecture is simply less machinery.

## Current implementation direction

The current Vix migration already provides useful evidence for this Foundation model.

Logging, Environment, Result, Validation, and Time have been moved away from broad utility ownership toward canonical responsibilities.

Historical networking dependencies are being reduced to the actual capabilities consumers need.

WebSocket execution dependencies are being audited so that components that only need Async do not retain unrelated historical RuntimeExecutor dependencies.

These migrations are examples of the desired direction:

```text
broad historical dependency
    ↓
inspect actual behavior
    ↓
identify foundational need
    ↓
depend on canonical responsibility
```

The Standard now gives those migrations a common semantic basis.

The architecture gives them a destination.

## Current responsibility map

At the architectural level, the Runtime Foundation currently points toward the following ownership:

```text
Error / Result
    canonical Vix error foundation

Path
    canonical Vix path foundation

Filesystem
    canonical Vix filesystem foundation

Environment
    canonical Vix environment foundation

Time
    canonical Vix time foundation

Logging
    canonical Vix logging foundation

Async
    canonical asynchronous Operation and I/O foundation

Cancellation
    canonical cross-domain cancellation semantics,
    implementation expected to integrate closely with Async
```

This map describes architectural direction.

It does not freeze exact module names, public types, or repository paths forever.

Those details may evolve while responsibility remains clear.

## Unresolved implementation choices

Several important representation questions remain intentionally open.

The architecture does not yet decide the final canonical C++ forms of:

```text
asynchronous Operation representation
Cancelled completion representation
Cancellation source/token mechanism
Deadline propagation context
general execution capability interfaces
structured concurrency primitives
```

These questions should not be answered merely to complete the architecture diagram.

They require examination across actual consumers.

When a representation decision creates a durable public contract, it may require a VEP.

The architecture should constrain those decisions without prematurely making them.

## Architectural requirements

The Runtime Foundation should have one canonical architectural owner for each foundational responsibility.

Higher-level domains should consume foundational responsibilities rather than maintain competing foundational models.

Foundation responsibilities should remain independently usable where their semantics do not require a shared runtime object.

Time should remain conceptually independent from Async even when Async uses Time extensively.

Structured Error should remain independent from Logging.

Async should not become synonymous with all forms of execution.

CPU and blocking-work execution should remain expressible as capabilities distinct from asynchronous I/O progression.

Stateful Foundation Providers must follow the Standard Resource ownership model.

Borrowing a Foundation Resource must not imply lifecycle authority.

Foundation APIs should not depend upward on Ecosystem domains.

Historical aggregation modules should not be treated as permanent owners merely because they currently contain foundational code.

Migration should remove unnecessary dependencies before introducing replacement abstractions.

External libraries should remain beneath Vix capability and Provider boundaries unless their identity is intentionally part of a public contract.

## Design consequence

The Runtime Foundation should answer:

> Which responsibilities must Vix implement once so that every domain can rely on the same foundation?

Its answer should be small and precise.

It should not be:

> Everything commonly used belongs here.

It should be:

> These are the responsibilities whose shared implementation and shared semantics keep Vix coherent.

Error gives Vix one model of operational Failure.

Path and Filesystem give it one model of filesystem interaction.

Environment gives it one low-level environment boundary.

Time gives it one temporal model.

Logging gives it one common diagnostic facility.

Async and Cancellation give it one foundation for asynchronous work and lifetime control.

Everything above can then specialize, compose, and evolve without rebuilding those foundations independently.

That is the role of the Vix Runtime Foundation.
