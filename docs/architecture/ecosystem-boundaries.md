# Ecosystem Boundaries

The Vix Ecosystem contains domain-oriented functionality built on top of the Vix Platform.

Its purpose is to provide useful higher-level systems such as Web, WebSocket, database access, realtime communication, caching, synchronization, peer-to-peer communication, game systems, UI, and agent-oriented functionality without duplicating the foundational responsibilities already owned by the Platform.

The Ecosystem is where Vix becomes specialized.

The Platform provides the common ground.

The Ecosystem gives that ground domain meaning.

The architectural boundary between them exists so that domain behavior can evolve rapidly without turning every useful feature into permanent Runtime infrastructure.

## The role of the Ecosystem

An Ecosystem component owns semantics that belong primarily to a particular domain.

For example, Web may own routing and middleware semantics.

WebSocket owns framing, connection protocol behavior, and message-level interaction.

Database components own query, transaction, driver, or ORM semantics.

Game owns concepts such as scenes, game events, input-oriented domain behavior, and jobs at the game level.

These concepts may be important and reusable.

They are still different from foundational responsibilities such as Time, Async, Error, Process, Filesystem, or CPU execution.

A useful architectural relationship is:

```text
Ecosystem
    builds domain behavior

Platform
    provides reusable capabilities

Standard
    provides shared semantic foundations
```

The Ecosystem should specialize the Platform rather than redefine it.

## Platform versus Ecosystem

The Platform answers questions such as:

> How does Vix represent time?

> How does asynchronous I/O progress?

> How is operational Failure represented?

> How is CPU work executed?

> How is a TCP connection established?

The Ecosystem answers questions such as:

> How is an HTTP route matched?

> How does a WebSocket connection behave?

> What does a database transaction mean?

> How does realtime reconnection work?

> How are game jobs grouped?

> How does synchronization policy operate?

The distinction is not based on whether functionality is frequently used.

HTTP servers may be extremely common and still remain Ecosystem functionality.

Time may be invisible to most simple applications and still remain foundational Platform functionality.

Architectural level is determined by responsibility.

## Dependency direction

The intended dependency direction is:

```text
Ecosystem
    ↓
Platform capabilities
    ↓
Providers
    ↓
OS / external libraries / hardware
```

An Ecosystem component may depend on another Ecosystem component when the relationship is meaningful.

For example:

```text
Realtime
    may use WebSocket
```

or:

```text
ORM
    may use Database
```

Such dependencies should still preserve clear domain layering.

A lower Platform layer should not need to depend on a higher Ecosystem domain merely because the higher domain currently contains a useful implementation detail.

## Ecosystem domains should depend on capabilities

An Ecosystem component should depend on the actual Platform capabilities it requires.

For example, WebSocket may need:

```text
Async I/O
TCP
TLS
Time
Error
```

This expresses the real architectural relationship.

A dependency such as:

```text
WebSocket → Core
```

does not.

Likewise, Cache may require:

```text
Time
Storage
Network Reachability
```

without depending on an entire networking subsystem.

The Platform boundary becomes stronger when Ecosystem dependencies reveal their actual reason for existing.

## Ecosystem domains own domain semantics

Each Ecosystem domain should have authority over its own concepts.

For example, Web may define:

```text
Request
Response
Route
Middleware
Session
Server
```

WebSocket may define:

```text
Connection
Frame
Message
Handshake
Close behavior
```

Database may define:

```text
Query
Transaction
Connection Pool
Driver
Migration
Model
```

These concepts should not be moved into the Platform merely because several applications use them.

The Platform should remain focused on cross-domain infrastructure.

## Ecosystem domains do not own foundational semantics

A domain must not redefine a foundational concept simply because it needs a specialized policy.

For example, Web may define HTTP request timeout policy.

It should still use the canonical Vix Time and Deadline semantics.

Database may define transaction cancellation behavior.

It should still use the common cancellation model.

Game may define job scheduling policy.

It should still consume CPU execution rather than redefining what execution ownership means.

Conceptually:

```text
domain policy
    specializes

foundational semantics
    remain shared
```

This relationship is one of the central architectural boundaries in Vix.

## Error boundaries

Ecosystem domains may define domain-specific Errors.

For example:

```text
Web:
    route not found
    malformed request

Database:
    constraint violation
    transaction conflict

WebSocket:
    protocol error
    invalid frame
```

These Errors should participate in the canonical Vix Error model.

A domain should not introduce a completely separate Result system merely because its Error identities differ.

The shared Error model provides structure.

The domain provides meaning.

## Failure translation

An Ecosystem component may translate lower-level Errors into domain Errors.

For example:

```text
TCP connection refused
    ↓
HTTP client
    ↓
HTTP transport Failure
```

or:

```text
TLS certificate validation Failure
    ↓
WebSocket client
    ↓
secure connection Failure
```

Translation should preserve useful cause and context where possible.

A domain abstraction may hide irrelevant implementation detail without destroying diagnostic history.

The domain should not flatten all lower-level Failures into generic strings.

## Time boundaries

Domains may define policies involving time.

Examples include:

```text
HTTP request timeout
WebSocket heartbeat interval
database transaction deadline
cache expiration
realtime reconnect delay
game frame duration
```

These are domain policies.

They should use the canonical Time foundation.

A domain should not introduce an incompatible Duration or Deadline model simply because its policy is domain-specific.

This allows different domains to compose without requiring time conversion between unrelated abstractions.

## Cancellation boundaries

Domains may decide when cancellation should be requested.

For example:

```text
HTTP client
    cancels operation because caller no longer needs response

WebSocket
    cancels pending receive during connection shutdown

Database
    cancels query because transaction is terminating
```

The cancellation policy belongs to the domain or Operation owner.

The meaning of cancellation remains foundational.

The domain should not redefine cancellation as a different terminal outcome unless its contract explicitly maps the foundational result into a domain-specific abstraction.

## Execution boundaries

An Ecosystem component should not own a private execution model unless the domain genuinely requires one.

For example, WebSocket uses Async because its transport work requires asynchronous I/O.

That does not mean WebSocket should define another event loop.

Game may define a JobSystem because jobs are useful domain-level coordination.

The JobSystem may still consume the Platform CPU execution capability.

Conceptually:

```text
Game JobSystem
    domain abstraction
        ↓
CPU Execution
    Platform capability
```

This is a valid specialization.

## Resource ownership boundaries

An Ecosystem domain may own its domain Resources.

For example:

```text
Web
    owns Server Resources

WebSocket
    owns Connection Resources

Database
    owns Pool / Connection Resources

Realtime
    owns subscription or client Activity
```

The same domain may borrow shared Platform Resources:

```text
Async Context
CPU Provider
Logging
Time source
network Providers
```

The domain's shutdown should terminate what it owns.

It should preserve externally owned Platform Resources.

This distinction must remain explicit.

## Activities in the Ecosystem

Many Ecosystem concepts are naturally Activities.

Examples include:

```text
HTTP Server
WebSocket Connection
Realtime Subscription
Sync Loop
Background Agent
Game Loop
```

Each Activity may create many child Operations.

An Activity should own or coordinate its domain-specific lifetime while borrowing whatever Platform infrastructure is required to progress it.

The Activity should not become the owner of shared infrastructure simply because it lives longer than individual Operations.

## Web boundary

Web belongs to the Ecosystem.

Its responsibilities may include:

```text
HTTP server behavior
routing
middleware
request/response abstractions
sessions
OpenAPI integration
server configuration
application composition
```

Web consumes Platform capabilities such as:

```text
Async
TCP
TLS
Time
Error
Logging
CPU execution where needed
```

Web should not own the canonical implementations of those Platform capabilities.

Its architecture should remain understandable as composition over them.

## HTTP client boundary

HTTP client functionality is architecturally different from Web server functionality.

The HTTP client is treated as a general Platform capability because outbound HTTP communication is useful across many otherwise unrelated domains.

Web server framework semantics remain Ecosystem-level.

This gives:

```text
Platform
    HTTP Client

Ecosystem / Web
    HTTP Server
    Router
    Middleware
    Sessions
```

The exact physical implementation may share code.

The architectural responsibility remains distinct.

## WebSocket boundary

WebSocket belongs to the Ecosystem.

It owns:

```text
handshake semantics
frame processing
message semantics
connection Activity
close protocol
WebSocket routing where applicable
```

It consumes:

```text
Async
TCP
TLS
Time
Error
```

The current migration away from RuntimeExecutor at lower WebSocket layers reflects this boundary.

A Session should use the execution capability it requires rather than inherit unrelated runtime infrastructure.

## WebSocket and Web

WebSocket may integrate with Web without becoming identical to Web.

For example, an HTTP server may perform an upgrade into a WebSocket connection.

This creates a domain relationship.

The architecture should still preserve separate ownership of:

```text
HTTP request semantics

WebSocket connection semantics
```

Shared transport or routing infrastructure does not require the two domains to collapse into one module.

## Database boundary

Database functionality belongs primarily to the Ecosystem.

Its responsibilities may include:

```text
database client behavior
queries
transactions
connection pools
drivers
migrations
ORM
models
```

Database implementations may consume Platform capabilities such as:

```text
TCP
TLS
Filesystem
Async
Time
Error
CPU or blocking execution
```

depending on the backend.

The database layer should not redefine those capabilities.

## Driver versus ORM

Database architecture may contain several levels.

For example:

```text
ORM
    ↓
Database abstraction
    ↓
Driver
    ↓
Platform networking / storage
```

The ORM owns object-relational semantics.

The driver owns communication with a specific database system.

The Platform owns general transport, execution, filesystem, and time capabilities.

Keeping these boundaries explicit prevents ORM concerns from leaking into the Runtime.

## KV boundary

Key-value storage belongs to the storage Ecosystem unless a future Standard capability defines a more general persistent-storage contract.

KV may use:

```text
Filesystem
database backend
memory
remote service
```

depending on Provider.

KV semantics should remain its own domain contract.

It should not become the definition of persistence for all Vix software merely because several domains need simple storage.

## Cache boundary

Cache belongs to the Ecosystem.

It owns caching semantics such as:

```text
cache entry lifetime
eviction policy
cache key behavior
staleness
reuse policy
```

It may consume:

```text
Time
Storage
Filesystem
Network Reachability
```

depending on configuration.

Cache should not own the Time model or networking model.

A cache implementation may use those capabilities without becoming their canonical home.

## Sync boundary

Sync belongs to the Ecosystem.

It owns synchronization behavior such as:

```text
outbox management
retry policy
conflict policy
delivery tracking
synchronization Activity
```

It may consume:

```text
HTTP Client
Network Reachability
persistent storage
Async
Time
Error
```

Sync should not invent a second HTTP client, network probe, or Deadline model solely for internal convenience.

## Realtime boundary

Realtime belongs to the Ecosystem.

It owns higher-level semantics such as:

```text
channels
subscriptions
presence
reconnection
message delivery policy
```

It may use WebSocket or another transport.

Conceptually:

```text
Realtime
    ↓
WebSocket / transport
    ↓
Platform networking and Async
```

Realtime should not collapse transport semantics and realtime application semantics into one indistinguishable layer.

This separation makes alternate transports possible.

## P2P boundary

P2P belongs to the Ecosystem.

It may use:

```text
TCP
UDP
DNS
HTTP Client
Crypto
Async
Time
```

depending on the protocol.

It owns concepts such as:

```text
peer discovery
peer state
peer protocol
replication policy
peer Activities
```

Historical dependency on broad networking aggregation should be replaced progressively by explicit capability dependencies.

## Game boundary

Game belongs to the Ecosystem.

It may own:

```text
Scene
EventBus
game Time policy
Input abstraction
JobSystem
asset coordination
game loop
```

It may consume Platform capabilities such as:

```text
Time
Filesystem
CPU Execution
Logging
```

A game-specific JobSystem may add scheduling and coordination useful to game workloads.

It should not redefine the foundational meaning of CPU execution or Resource ownership.

## UI boundary

UI belongs primarily to the Ecosystem.

It may depend on future Platform capabilities such as:

```text
input
windowing
graphics
filesystem
time
```

depending on Vix's eventual scope.

The current architecture should not pull UI concerns into the Runtime Foundation merely because application code uses them frequently.

Any lower-level capability promoted to the Platform should be justified independently.

## Agent boundary

Agent belongs to the Ecosystem.

It may compose many Platform capabilities:

```text
HTTP Client
Process
Filesystem
JSON
Crypto
Async
Time
Logging
```

and may also use other Ecosystem domains.

Agent owns semantics such as:

```text
tool execution
model interaction
agent workflow
provider-specific model behavior
trust policy
```

Those semantics should not redefine Process, Filesystem, HTTP, or execution foundations.

## Middleware boundary

Middleware is domain composition behavior.

For Web, middleware may process Requests and Responses.

Other domains may eventually have analogous pipeline concepts.

Middleware should not be promoted into the Runtime Foundation merely because several systems use chains of handlers.

If a truly cross-domain semantic abstraction emerges later, it can be researched independently.

Until then, middleware remains owned by the domain that defines its contract.

## Validation boundary

Validation is broadly reusable and may sit near the Platform/Ecosystem boundary depending on its exact role.

Canonical validation primitives may be shared.

Domain validation policy remains local.

For example:

```text
Validation foundation
    checks constraints

Web
    defines request validation policy

Database
    defines model constraints

Application
    defines business validation
```

A common validation mechanism should not become the owner of domain business rules.

## Configuration boundary

Low-level Environment capability belongs to the Platform.

Higher-level configuration belongs closer to applications and domains.

For example:

```text
Environment
    Platform

Web server configuration
    Web

database configuration
    Database

game configuration
    Game

application configuration
    Application
```

This prevents a generic configuration subsystem from gradually absorbing every domain's policy.

Shared parsing or schema facilities may still be reused.

## Logging boundary

Domains may choose what to log and which context matters.

They should use the shared Logging foundation.

For example:

```text
WebSocket
    emits connection diagnostics

Database
    emits query diagnostics

Game
    emits frame or asset diagnostics
```

They should not create separate logging infrastructures merely because their log data differs.

Domain logging policy belongs to the domain.

Logging transport and basic representation belong to the Platform.

## Metrics and observability

Metrics may eventually span both Platform and Ecosystem.

A Provider may expose queue depth.

Web may expose request latency.

Database may expose pool utilization.

Realtime may expose reconnect counts.

A future observability model may unify some of this.

The architecture should not prematurely make one global metrics abstraction responsible for every domain measurement.

Observability should follow ownership of the thing being observed while using shared infrastructure where justified.

## Domain composition

Ecosystem domains may compose one another.

For example:

```text
Realtime
    uses WebSocket

WebRPC
    may use HTTP or WebSocket

Agent
    may use HTTP Client and Database

Sync
    may use HTTP Client and Storage
```

Composition is valid when the dependency expresses real domain behavior.

The architecture should avoid circular domain ownership.

If two domains depend on one another deeply, that may indicate either:

```text
a missing lower-level shared capability

or

a boundary that should be reconsidered
```

Circular module dependencies should therefore trigger architectural review.

## Shared domain utilities

Two Ecosystem domains may share implementation code.

Shared implementation does not automatically require promotion into the Platform.

For example, Web and WebSocket may share:

```text
header parsing
connection metadata
routing helpers
```

without turning those concepts into foundational Runtime capabilities.

The question should remain semantic:

> Is this behavior truly part of the common Vix Platform contract, or simply shared implementation between related domains?

Code reuse alone is not enough to decide architectural level.

## Promotion to the Platform

An Ecosystem concept may later become a Platform capability.

This should happen only when repeated use across independent domains demonstrates that the concept is broadly foundational.

A typical evolution may look like:

```text
Domain A
    introduces concept

Domain B
    needs equivalent concept

Domain C
    needs same semantic contract

Architecture
    identifies cross-domain capability

Research / VEP if semantic standardization is needed

Platform
    becomes canonical owner
```

Promotion should be based on semantic need, not popularity.

Once promoted, the original domain should consume the new canonical Platform capability rather than retaining a competing version.

## Avoid premature promotion

Vix should not promote a domain abstraction simply because it feels generic.

For example, concepts such as:

```text
middleware
session
channel
scene
model
route
peer
```

may sound reusable.

Their meaning may still depend heavily on their domain.

Premature promotion creates vague abstractions that later become difficult to remove.

The Platform should grow conservatively.

## Domain-local policy

Domains are expected to define policy.

Examples include:

```text
Web retry policy
Realtime reconnect policy
Cache eviction policy
Database transaction policy
Game scheduling policy
Agent tool policy
```

These policies may use foundational capabilities.

They do not belong in the Platform simply because they control behavior over time or execution.

Platform provides mechanism and foundational semantics.

Domains provide domain policy.

## Domain-local Resources

An Ecosystem domain may create specialized Resources.

Examples include:

```text
HTTP Server
WebSocket Connection
Database Transaction
Realtime Subscription
Peer Connection
Game Scene
Agent Session
```

These Resources should follow the shared Resource and ownership model.

Their specialized lifecycle belongs to the domain.

The foundational meaning of ownership does not change.

## Domain-local Operations

Domains may define specialized Operations.

Examples include:

```text
HTTP Request
WebSocket Send
Database Query
Sync Flush
Peer Connect
Asset Load
Agent Tool Invocation
```

These Operations should preserve the common Operation invariants:

```text
finite logical work
one terminal completion
explicit lifetime
Failure semantics
Cancellation semantics where supported
Deadline semantics where applicable
```

A domain may add stronger guarantees.

It should not weaken the foundational ones silently.

## Domain-local Activities

Domains may define continuing Activities.

Examples include:

```text
Server
WebSocket Connection
Realtime Subscription
Sync Loop
Game Loop
Agent Runtime
```

The Activity model allows these domains to represent ongoing behavior without pretending that one endless Operation is equivalent to a finite request.

This distinction should remain visible in domain APIs where it improves lifecycle clarity.

## Ecosystem APIs should remain simple

Clear architecture should not require users to manually assemble every Platform dependency for ordinary use.

For example, a normal WebSocket client should not necessarily require explicit construction of:

```text
DNS Provider
TCP Provider
TLS Provider
Async Context
Clock
Cancellation source
```

if Vix can provide sensible composition.

A high-level domain API may use default Providers.

Advanced users may receive explicit control where necessary.

The architecture should preserve the underlying boundaries without forcing them into every simple call.

## Default composition

An Ecosystem domain may provide convenient default composition.

For example:

```text
WebSocket Client
    by default uses
        canonical Async
        canonical DNS
        canonical TCP
        canonical TLS
```

This convenience does not mean WebSocket owns those capabilities.

The domain is composing defaults.

The Platform remains the canonical owner.

This distinction is important for customization and testing.

## Custom composition

Advanced users or internal Vix components may need to supply specific Providers or Resources.

For example:

```text
WebSocket Client
    uses externally supplied Async Context

Database
    uses custom TLS Provider

Game
    uses dedicated CPU Provider
```

Such composition should preserve ownership rules.

External injection normally creates borrowing unless ownership transfer is explicit.

Customization should not introduce ambiguity about shutdown authority.

## Ecosystem and Profiles

Ecosystem domains must operate within the capabilities guaranteed by the selected Profile.

For example, a domain requiring TCP cannot operate under a Profile that does not provide TCP unless the domain supplies an alternative capability that satisfies its contract.

A Game domain may work in a Profile without Process.

An Agent feature requiring Process may not.

Profiles therefore determine which Ecosystem combinations are available without changing the meaning of the domains themselves.

## Capability requirements should be visible

An Ecosystem component should make its required capabilities understandable.

For example:

```text
WebSocket
    requires TCP
    requires Async
    may require TLS

P2P
    may require TCP + UDP + Crypto

Agent tool execution
    may require Process

Database driver
    may require TCP + TLS
```

These requirements may eventually become machine-readable.

Even before that, architecture should document them.

This helps portability and Profile design.

## Optional capabilities

A domain may adapt when an optional Platform capability is absent.

For example:

```text
Cache
    persistent storage available
        → durable cache

    persistent storage unavailable
        → memory-only mode
```

This adaptation is valid only if the resulting behavior remains within the advertised domain contract.

A domain must not silently claim durable semantics while falling back to memory-only storage.

Optional adaptation must preserve truthful semantics.

## Degraded modes

A domain may expose an explicit degraded mode.

For example:

```text
Realtime
    WebSocket unavailable
        → polling mode
```

if polling still satisfies an explicitly defined weaker or alternative contract.

The fallback should be visible when it changes guarantees.

Architecture should not treat silent semantic weakening as ordinary Provider selection.

## Security boundaries

Security-sensitive responsibilities should remain clearly owned.

For example:

```text
TLS
    Platform secure transport capability

Web
    HTTP authentication / authorization policy

Database
    database credential handling

Agent
    tool authorization and trust policy
```

A domain may use Crypto or TLS.

It should not reimplement cryptographic primitives casually.

Likewise, a Platform TLS Provider should not own Web authorization policy.

Security improves when responsibilities are separated clearly.

## Data boundaries

Data formats may cross domains.

JSON is a Platform-level common facility.

Database models, Web request bodies, Agent payloads, or realtime messages may use JSON.

Their domain meaning remains local.

A JSON object is not a Web Request merely because Web uses JSON.

Shared representation should not collapse domain semantics.

## Historical ecosystem coupling

Historical Vix modules may contain dependencies that do not match these boundaries.

Examples include:

```text
WebSocket depending on RuntimeExecutor
Cache depending on broad Net
Agent depending on broad Net
P2P depending on historical Net
domain modules depending on Utils
```

These dependencies should be audited according to actual capability usage.

The current migration has already shown that many can be removed or narrowed without creating replacement abstractions.

The Ecosystem boundary should guide the remaining work.

## Migration pattern

For an Ecosystem domain with a historical broad dependency, migration should proceed as:

```text
1. inspect what the domain actually uses
2. identify the domain responsibility
3. identify required Platform capabilities
4. identify ownership of shared Resources
5. remove forwarding-only dependencies
6. replace broad dependency with canonical capability where necessary
7. keep domain-specific semantics local
8. validate behavior
```

This process should be repeated incrementally rather than through a full rewrite.

## Do not rebuild historical aggregation

Migration should not replace:

```text
Core
Utils
Net
RuntimeExecutor
```

with one new universal Ecosystem dependency.

For example, a new object such as:

```text
VixServices
    Async
    Time
    Network
    Storage
    Process
    Logging
```

passed through every domain would recreate much of the same coupling.

Domains should depend on coherent responsibilities.

Convenience composition may exist at application boundaries without becoming the architectural dependency of every subsystem.

## Ecosystem packages and modules

The physical repository may organize Ecosystem functionality into modules or packages.

These boundaries are useful for:

```text
build isolation
dependency management
testing
distribution
ownership
```

They should follow domain responsibility.

They do not need to map one-to-one to every Standard concept or Platform capability.

A module may contain several closely related domain concepts.

A domain may eventually span several modules if its scale justifies it.

Architecture should remain understandable above the file layout.

## Domain dependencies

Ecosystem-to-Ecosystem dependencies should be explicit.

A higher-level domain may depend on a lower-level specialized domain when the relationship is natural.

For example:

```text
Realtime
    → WebSocket

ORM
    → Database

WebRPC
    → Web / HTTP
```

These dependencies should not create foundational cycles.

If lower-level WebSocket begins depending on Realtime, the layering should be questioned.

Domain direction should reflect semantic construction.

## Compatibility layers

Historical domain APIs may remain available through compatibility layers.

Compatibility code should not become the new architectural owner of a responsibility.

For example:

```text
historical Web API
    compatibility wrapper
        ↓
canonical Web implementation
```

This allows migration without preserving outdated internal boundaries forever.

The canonical direction should remain clear.

## Domain versioning

Ecosystem domains may evolve faster than the foundational Standard.

A Web API may add features.

Database Providers may change.

Game systems may gain new capabilities.

These changes do not necessarily require Standard evolution.

A Standard change is required only when foundational semantic contracts change.

Domain specifications may eventually have their own versioning where stable cross-implementation contracts become important.

## Domain specifications

Some Ecosystem areas may eventually require formal specifications.

Possible future examples include:

```text
Vix Web Specification
Vix Networking Specification
Vix Data Specification
Vix Realtime Specification
```

These specifications would build on the foundational Vix Standard.

They should not redefine concepts such as ownership, Operation, Failure, Cancellation, or Capability with conflicting meanings.

A specialization may add domain rules.

The foundation remains shared.

## Ecosystem conformance

Most Ecosystem components are not automatically part of core Standard conformance.

A Vix implementation might conform to a foundational Profile without shipping every Ecosystem domain.

Domain-level conformance may eventually be defined separately.

For example:

```text
Vix Standard conformance
    foundational semantics

Native Profile conformance
    capability guarantees

Web specification conformance
    Web behavior
```

This keeps the Standard small enough to remain durable.

## Testing boundaries

Ecosystem tests should validate domain behavior while also protecting foundational contracts at domain boundaries.

For example, WebSocket tests may verify:

```text
protocol behavior
message ordering
close semantics
borrowed Async Context remains alive
Deadline behavior
Cancellation behavior
```

Database tests may verify:

```text
transaction behavior
query Failure
pool lifecycle
borrowed execution lifecycle
```

A domain test does not need to retest every Platform implementation detail.

It should verify that the domain uses the Platform correctly.

## Ecosystem architecture audits

Each major domain should eventually be auditable through a small set of questions:

> What semantics does this domain own?

> Which Platform capabilities does it require?

> Which other Ecosystem domains does it depend on?

> Which Resources does it own?

> Which Resources does it borrow?

> Which Providers are configurable?

> Which historical dependencies can disappear?

> Which behaviors are public contract versus implementation detail?

These questions should be enough to understand the domain without reconstructing the entire repository history.

## Current ecosystem direction

The current Vix3 migration already points toward clearer Ecosystem boundaries.

WebSocket lower layers are moving away from historical RuntimeExecutor dependencies toward their actual Async execution requirement.

Cache and Sync have moved away from broad networking dependencies toward the narrower Network Reachability capability where appropriate.

Agent and P2P historical networking dependencies are being reduced according to their actual needs.

Utilities such as Logging, Environment, Result, Validation, and Time have moved toward canonical ownership instead of domain-local or broad utility access.

These changes are not isolated cleanups.

Together they move the codebase toward:

```text
domain semantics
    owned by domain

foundational semantics
    owned by Platform

implementation mechanism
    owned by Provider
```

## Boundary test

When deciding where new functionality belongs, Vix should ask:

> Is this concept meaningful across independent domains?

> Would inconsistent versions of it create a foundational programming-model problem?

> Is this an ability that domains consume, or behavior specific to one domain?

> Does it describe mechanism or policy?

> Would placing it in the Platform reduce real duplication of semantics, or merely move shared code?

If the concept describes broad ability or foundational semantics, it may belong to the Platform.

If it describes domain policy or application behavior, it probably belongs to the Ecosystem.

## Architectural requirements

Ecosystem domains should own domain-specific semantics and Resources.

Ecosystem domains should consume canonical Platform capabilities rather than redefine foundational mechanisms.

Domain-specific Errors should participate in the shared Error model.

Domain time policy should use canonical Time semantics.

Domain cancellation policy should use canonical Cancellation semantics.

Domain execution abstractions should compose Platform execution capabilities rather than silently creating competing foundational execution models.

Externally owned Platform Resources must remain borrowed unless ownership is explicitly transferred.

Domain shutdown must not terminate unrelated shared Platform infrastructure.

Platform layers must not depend upward on Ecosystem domains merely to obtain implementation convenience.

Shared code between domains must not be promoted to the Platform solely because it is shared.

Promotion from Ecosystem to Platform should require demonstrated cross-domain semantic need.

Fallback or degraded modes must not silently weaken advertised guarantees.

Ecosystem-to-Ecosystem dependencies should preserve understandable semantic layering.

Historical broad dependencies should be replaced by the actual capabilities domains require.

Compatibility layers should preserve migration paths without becoming canonical architectural owners.

## Open boundary questions

Several Ecosystem boundaries still need deeper audits.

The exact boundary between low-level database drivers and general Platform storage capabilities remains open.

Persistent storage may eventually become a formal Platform Capability, but the current evidence is not yet sufficient to standardize one universal model.

Input and low-level UI support may eventually expose Platform capabilities while high-level UI remains in the Ecosystem.

Networking itself may later receive a more detailed architecture separating transports, secure transport, addressing, resolution, and protocol layers.

The future relationship between HTTP server infrastructure and WebSocket upgrade handling should be refined without collapsing the two domains.

Agent may eventually need stronger security and trust boundaries around Process, Filesystem, network, and external model Providers.

These questions should be handled through focused architecture work and VEPs where foundational semantics are affected.

## Design consequence

The Ecosystem boundary should make it possible to look at a domain and understand:

```text
what this domain means
what it owns
what it borrows
what capabilities it requires
what lower domains it composes
```

without making the domain responsible for the entire Vix Runtime.

Web should be able to be Web.

Database should be able to be Database.

Game should be able to be Game.

Realtime should be able to be Realtime.

They should share the same foundations without becoming the same system.

That separation is what allows the Vix Ecosystem to grow while the Platform remains coherent.

The Platform provides capabilities.

The Ecosystem turns those capabilities into domains programmers can build with.
