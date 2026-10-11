# Ownership Map

The Ownership Map describes where lifecycle authority lives across the Vix architecture.

Its purpose is not to document memory ownership for every C++ object.

It exists to record the relationships that matter when a component can start, stop, close, destroy, transfer, or otherwise control the usable lifetime of a Resource, Operation, Activity, or Provider.

The Vix Standard establishes the foundational rule:

> Access to a Resource does not imply authority over its lifecycle.

The architecture must make that rule concrete.

A component may retain a pointer, reference, handle, or `shared_ptr` to a Resource and still only borrow it.

Likewise, a component may own an Activity while borrowing the execution infrastructure used to progress that Activity.

These distinctions are particularly important in long-lived systems, where several domains may share the same Runtime Resources.

## Purpose of the map

The Capability Map answers:

> What ability does this component require?

The Ownership Map answers:

> Who controls the lifetime of the Resource that satisfies that requirement?

These questions are related but different.

For example:

```text
WebSocket Server
    requires Async I/O Capability
```

does not tell us who owns the Async Context.

The Ownership Map may establish:

```text
Application Orchestrator
    owns Async Context

WebSocket Server
    borrows Async Context
```

The Server can then use Async without gaining authority to terminate it.

This separation should remain visible throughout Vix.

## Ownership terminology

This architecture uses the terminology defined by the Vix Standard.

An **Owner** has lifecycle authority and responsibility.

A **Borrower** may use a Resource without controlling its lifetime.

An **Ownership Transfer** moves lifecycle authority from one owner to another.

A component may also participate in explicitly shared lifecycle authority, but this should be used only where the Resource contract genuinely requires it.

The architecture should prefer a clear owner with borrowers when one lifecycle authority is sufficient.

## Memory lifetime is not operational ownership

C++ memory-management types are implementation mechanisms.

They may help represent ownership, but they do not define Vix lifecycle authority by themselves.

For example:

```cpp
std::shared_ptr<ExecutionContext>
```

may mean that several components ensure the object remains allocated.

It does not automatically mean that every holder may call:

```cpp
stop();
shutdown();
reset();
```

on the underlying execution Resource.

Likewise, a raw reference may represent a valid borrow when the surrounding lifetime guarantees are clear.

The architecture should describe semantic ownership first.

The C++ representation should then support that contract.

## Application orchestration

At the top of many Vix applications is an orchestration lifetime.

This may be an application object, runtime launcher, server helper, command, test harness, or another component responsible for coordinating several Resources and Activities.

Conceptually:

```text
Application Orchestrator
├── owns Async Context
├── owns CPU Execution Provider
├── owns application-level Activities
└── coordinates shutdown
```

The exact set of Resources depends on the application.

The architectural point is that shared infrastructure often belongs to the orchestration lifetime rather than to one individual domain component.

This allows several domains to borrow the same infrastructure safely.

## Application-owned Async Context

The common intended ownership model for a shared Async Context is:

```text
Application / Orchestrator
    owns Async Context

HTTP Server
    borrows Async Context

WebSocket Server
    borrows Async Context

Realtime
    borrows Async Context

other asynchronous components
    borrow Async Context
```

The owner decides when the Async Context starts and when its lifetime ends.

Borrowers may schedule or associate work with it according to its execution contract.

Borrowers must not stop it simply because their own Activity is terminating.

This relationship is central to Vix server architecture.

## Component-owned Async Context

A component may instead create a private Async Context.

For example:

```text
Component
    creates Async Context
    owns Async Context
```

In that case, the component is responsible for ending the Context lifetime.

This may be appropriate for isolated components, tests, small helpers, or APIs that intentionally provide a self-contained runtime.

The distinction must remain explicit.

The architecture does not require every Async Context to be application-owned.

It requires the actual owner to be identifiable.

## Borrowed external execution

When an execution Resource is supplied from outside a component, the default architectural interpretation is borrowing unless the API explicitly transfers ownership.

For example:

```text
External Owner
    owns Execution Resource
        ↓
Component receives access
        ↓
Component borrows Execution Resource
```

The component may retain access for its own lifetime when the contract permits.

It must not stop the Resource during destruction or shutdown merely because it retained the object.

This rule directly prevents one subsystem from unexpectedly breaking unrelated users of shared execution infrastructure.

## Component-created execution

When a component creates execution infrastructure solely for itself, ownership normally stays with that component.

For example:

```text
Server Helper
    creates Async Context
    owns Async Context
    creates Server
    runs Server
    shuts down Server
    stops Async Context
```

This is a different contract from receiving an externally supplied Context.

The implementation may use similar C++ types in both cases.

The ownership semantics are not the same.

APIs that support both modes should preserve this distinction clearly.

## Orchestration adoption

An orchestration layer may intentionally adopt ownership of Resources created elsewhere.

For example:

```text
Factory
    creates Resource
        ↓ ownership transfer
Application Orchestrator
    owns Resource
```

After transfer, the factory is no longer the lifecycle authority unless the contract explicitly establishes shared ownership.

Adoption should be explicit.

The architecture should not rely on informal assumptions such as:

> this object is passed into the application, so the application probably owns it.

Transfer needs a real contract.

## Server ownership

A Server Resource and the execution context used by that Server are different ownership concerns.

A common model is:

```text
Application
├── owns Server
└── owns Async Context

Server
└── borrows Async Context
```

When the Application decides to stop the Server, the Server Activity terminates.

The Async Context may remain alive because other components continue to use it.

This separation is especially important when HTTP, WebSocket, realtime, timers, or other Activities share one execution environment.

## Server Activity ownership

The owner of a Server Resource normally controls the lifetime of the associated Server Activity.

Conceptually:

```text
Application
    owns Server Resource
        ↓
    starts Server Activity
        ↓
    later requests shutdown
```

The Server Activity may create many child Operations.

The server owner does not necessarily own each internal C++ object individually.

It owns the higher-level lifecycle.

The server implementation is responsible for managing its internal Resources consistently with that lifecycle.

## HTTP request Operations

An HTTP Server Activity may create Request Operations.

Conceptually:

```text
HTTP Server Activity
├── Request Operation A
├── Request Operation B
└── Request Operation C
```

These Operations belong to the server request-processing lifetime.

They may borrow:

```text
router
server configuration
Async Context
connection state
shared application services
```

according to their contracts.

They must not silently outlive Resources whose validity depends on the Request or Server lifetime.

If work needs to survive the Request, ownership must be transferred to a broader lifetime explicitly.

## WebSocket connection ownership

A WebSocket connection commonly involves several related concepts.

Conceptually:

```text
WebSocket Server Activity
    accepts connection
        ↓
Connection Resource
        ↓
Connection Activity
        ├── Receive Operations
        ├── Send Operations
        └── Close Operation
```

The Connection Resource may be owned by the connection/session layer after acceptance.

The Connection Activity uses that Resource.

Individual send and receive Operations borrow the connection Resource while active.

When the Connection Activity terminates, its owner is responsible for ending owned connection Resources according to the WebSocket contract.

The connection must not gain lifecycle authority over broader shared execution Resources merely because its Operations execute through them.

## WebSocket Session and Async

The intended architectural ownership relationship is:

```text
Application / Server orchestration
    owns Async Context

WebSocket Session
    borrows Async Context

WebSocket Session
    owns or participates in ownership of its connection state
```

The Session uses Async to progress network Operations.

Its destruction or close path must not stop the shared Async Context.

This is independent from how access to the Context is represented in C++.

## LowLevelServer ownership

A low-level WebSocket server should own only the Resources that belong to its own server implementation.

It may own:

```text
listener state
internal accept state
connection bookkeeping
server-specific Resources
```

It may borrow:

```text
Router
Async Context
shared configuration or services
```

depending on the API contract.

The removal of unnecessary historical execution dependencies helps make these relationships clearer.

A component that does not actually use a Resource should not retain it merely because previous constructors forwarded it.

## Router ownership

Routers are generally shared application or server configuration Resources rather than execution Resources.

Possible ownership may look like:

```text
Application / Server
    owns Router

Sessions / Request Operations
    borrow Router
```

or, where immutable shared lifetime is convenient:

```text
shared retained Router object
    with lifecycle authority still defined by server/application contract
```

Reference-counting may preserve object lifetime.

It should not create ambiguous authority to mutate or destroy broader application routing state.

The exact mutability contract belongs to the relevant domain specification.

## Connection transport ownership

A Session may receive a newly accepted transport Resource.

A common pattern is ownership transfer:

```text
Listener
    accepts transport
        ↓ ownership transfer
Session
    owns transport
```

The Session then becomes responsible for closing or releasing the transport according to its lifecycle contract.

This is different from the Session borrowing the shared Async Context used to progress that transport.

The distinction is:

```text
Session
    owns Connection Transport

Session
    borrows Async Context
```

These relationships should not be conflated simply because both objects are constructor dependencies.

## TCP socket ownership

A successful TCP connect or accept Operation may produce a Socket Resource.

The Operation contract should transfer or establish ownership explicitly.

Conceptually:

```text
connect Operation
    creates Socket
        ↓ Success
Caller
    owns Socket
```

or:

```text
accept Operation
    produces Socket
        ↓ ownership transfer
Session
    owns Socket
```

After transfer, the receiving owner is responsible for the Socket lifecycle.

The TCP Provider does not necessarily remain the lifecycle owner merely because it created the underlying operating-system handle.

## TLS ownership

TLS commonly wraps or composes a transport Resource.

Possible ownership models include:

```text
TLS Connection
    owns underlying transport
```

or:

```text
TLS Connection
    borrows transport
```

depending on the API.

The contract must make this explicit.

The architecture should not assume one answer before the TLS API is fully audited.

What is already fixed is that TLS access to a transport does not automatically imply ownership.

The same ownership rules apply at this boundary as elsewhere.

## Process ownership

A successful process-spawn Operation may produce a Process Resource.

Conceptually:

```text
spawn Operation
    ↓ Success
Process Resource
    ↓ ownership to caller
```

The caller or another explicitly designated owner becomes responsible for the Process Resource according to the process contract.

This may include waiting, closing handles, requesting termination, or another lifecycle responsibility.

Owning a Process Resource does not necessarily imply unrestricted authority over the external process itself beyond what the process API contract defines.

## Filesystem Resource ownership

Filesystem Operations may produce Resources such as files, mappings, iterators, or watchers.

For example:

```text
open Operation
    ↓
File Resource
        ↓
caller owns File
```

A read Operation may borrow the File while active.

A File watcher may instead support a continuing Activity.

Each Resource follows the same architectural principles even though the domain-specific lifecycle differs.

The filesystem foundation should not invent separate ownership terminology.

## Timer ownership

A Timer Resource may be owned by the Operation, Activity, or Provider that created it.

A timer used only to enforce one Operation Deadline may be an internal owned Resource of that Operation machinery.

A recurring timer may belong to an Activity.

A timer facility or timer wheel may belong to the Async Provider and be borrowed internally.

The architecture should distinguish the individual Timer from the Provider that makes timer progression possible.

Stopping one Timer does not imply stopping the Timer Provider.

## Threadpool ownership

The Threadpool Provider is a stateful execution Resource.

A common ownership model is:

```text
Application / Orchestrator
    owns Threadpool

Game JobSystem
    borrows CPU execution capability

other consumers
    borrow CPU or blocking execution
```

A component using Threadpool should not terminate it unless the component owns it or has been given explicit lifecycle authority.

This is especially important if multiple domains share one pool.

## Game JobSystem ownership

The Game JobSystem may be a domain abstraction over CPU execution.

Possible architecture:

```text
Game subsystem
    owns JobSystem

JobSystem
    borrows CPU Execution Provider
```

The JobSystem owns game-specific job coordination.

It does not necessarily own the underlying Threadpool.

Alternatively, a self-contained Game instance may create a private Threadpool and therefore own it.

The API should make the mode clear.

The architecture should not assume ownership from the fact that JobSystem holds the Provider.

## Logging ownership

Logging may use stateful Resources such as sinks, files, queues, or background workers.

Ownership depends on configuration.

For example:

```text
Application
    owns global Logging configuration / sinks

Subsystems
    borrow Logging capability
```

Subsystems should normally emit logs without owning global sink lifecycle.

A component shutting down should not close a shared log sink used by the rest of the application.

Private log sinks remain possible where explicitly owned.

## Error ownership

Error values are generally values rather than shared infrastructure Resources.

Their ownership follows ordinary value semantics or the representation chosen by the API.

Causal chains or diagnostic context may retain data, but the Error foundation should avoid requiring broad runtime ownership relationships merely to represent Failure.

An Error should remain usable independently from the lifetime of a Logger or domain component whenever possible.

## Path and value ownership

Path, Duration, Deadline, and similar foundational value types should normally have straightforward value ownership.

They should not require application-wide runtime Resources solely to exist.

For example:

```text
Deadline
    value

Path
    value
```

may be copied or moved according to their API contracts without introducing lifecycle authority over the filesystem or timer Provider.

This separation helps keep foundational types lightweight.

## Provider ownership

A Provider may itself be a Resource.

Examples include:

```text
Async Context
Threadpool
Connection Pool
stateful TLS context
database pool
```

The owner of the Provider controls its lifecycle.

Consumers use the Capability provided by that Provider.

They do not automatically own the Provider.

Conceptually:

```text
Application
    owns Provider

Subsystem
    borrows Capability through Provider
```

This should be the default mental model for shared stateful Providers.

## Stateless Providers

Some Providers may be stateless.

For example, a pure implementation facility may expose functions without meaningful startup or shutdown.

In such cases, lifecycle ownership may be irrelevant.

The architecture should not manufacture ownership state merely because the semantic model contains the word Provider.

Ownership only matters where lifetime actually matters.

## Connection pools

A connection pool is both a Provider-like facility and a Resource.

The pool owns or coordinates a set of connection Resources.

Consumers typically borrow or lease connections from it.

Conceptually:

```text
Application / Database subsystem
    owns Connection Pool

Query Operation
    acquires lease
        ↓
    temporarily uses connection
        ↓
    returns connection
```

The Query Operation does not normally own the pool.

Whether it owns or leases the individual connection depends on the pool contract.

This distinction should remain visible in future database architecture.

## Leases

Some Resource relationships are better described as leases than simple borrows.

A lease may grant temporary exclusive or limited authority while the underlying owner remains responsible for the broader Resource.

For example, a connection pool may retain lifecycle ownership of a connection while a Query Operation receives temporary exclusive use.

The foundational Standard does not yet require a universal Lease type.

Architecture may use the concept where it clarifies real lifetime relationships.

If it becomes common across several domains, it may later justify Standard evolution.

## Cache ownership

A Cache component may own its internal storage Resources.

It may borrow:

```text
Time capability
Network Reachability capability
Filesystem or storage Provider
```

depending on configuration.

For example:

```text
Application
    owns NetworkProbe Provider

Cache
    borrows Network Reachability
```

The Cache must not stop the shared NetworkProbe Provider merely because the Cache is destroyed.

Likewise, an externally supplied persistent store remains governed by its own ownership contract.

## Sync ownership

A Sync subsystem may own its synchronization Activity and internal queue or outbox.

It may borrow:

```text
HTTP Client
Network Reachability
Time
persistent storage
Async Context
```

depending on architecture.

The Sync Activity may continue across many individual Operations.

Its owner controls when synchronization starts and stops.

The providers it uses may belong to broader application lifetimes.

## HTTP client ownership

An HTTP client may be configured in several ways.

A lightweight client may borrow Platform networking Providers.

A long-lived client may own internal connection pools while borrowing Async and TLS Providers.

Conceptually:

```text
HTTP Client
    owns connection pool

HTTP Client
    borrows Async Context
    borrows DNS capability
    uses TCP / TLS Providers
```

The precise ownership depends on the implementation.

The important architectural rule is that HTTP client shutdown should end the Resources it owns without terminating unrelated shared Platform infrastructure.

## Database ownership

Database architecture should distinguish several lifetimes.

For example:

```text
Application
    owns Database Client / Pool

Database Client
    owns or coordinates Connections

Query Operation
    borrows Database Client
    leases Connection

Database Client
    borrows Async Context
    may borrow TLS Provider
```

An ORM belongs above this lifetime structure.

Destroying an ORM query object should not terminate the application database pool.

Similarly, shutting down a database subsystem should not stop an externally owned Async Context.

## Realtime ownership

A Realtime subsystem may own its logical Activity while using WebSocket or another transport underneath.

Conceptually:

```text
Application
    owns Realtime Client

Realtime Client
    owns Realtime Activity

Realtime Activity
    may own current WebSocket Connection
    borrows Async Context
    borrows Time capability
```

If reconnection replaces the internal connection Resource, the Realtime Activity may remain alive.

This demonstrates again why Activity and Resource ownership must remain distinct.

## P2P ownership

P2P systems may own peer-management Activities and connection Resources while borrowing Platform networking and execution infrastructure.

For example:

```text
P2P Node
    owns peer Activity state
    owns accepted/created peer connections
    borrows Async Context
    borrows networking Providers
```

A peer connection terminating should not terminate the P2P Node unless the higher-level contract requires it.

A P2P Node stopping should not terminate broader shared Async infrastructure.

## Agent ownership

Agent systems may create Operations involving HTTP, Processes, Filesystem, or other Resources.

An Agent Activity may own the coordination of those Operations.

It should not automatically own the Providers used to perform them.

For example:

```text
Agent Activity
    owns tool invocation lifetime

Process Provider
    borrowed

HTTP Client
    borrowed or application-owned
```

This is particularly important where several Agent Operations share the same runtime infrastructure.

## Parent and child Operation ownership

A Parent Operation owns the logical lifetime of dependent child Operations unless the contract establishes another owner.

Conceptually:

```text
Parent Operation
├── Child A
├── Child B
└── Child C
```

The parent may request cancellation of dependent children.

The parent must not complete while those children continue using parent-owned Resources unless the children's lifetime has been transferred explicitly.

This is semantic lifetime ownership rather than necessarily memory ownership of child objects.

## Child independence

A child Operation may become independent.

For example:

```text
Request Operation
    submits durable Job
        ↓ ownership transfer
Background Worker Activity
    owns Job lifetime
```

After transfer, the Request no longer owns the Job.

Cancellation of the Request does not necessarily cancel the Job.

The Job must also stop borrowing Request-local state.

This explicit change of owner is what makes independent lifetime safe.

## Detached work

Detached work must therefore have a destination owner.

The architecture should reject the idea that:

```text
no one awaits this work
```

means:

```text
no one owns this work
```

Every continuing Operation or Activity must have a lifetime under which its Resources remain valid and its Failure behavior can be handled.

A Provider keeping the work physically alive is not enough.

Execution survival is not lifecycle ownership.

## Parent and child Activity ownership

Activities may also form lifecycle hierarchies.

For example:

```text
Application Activity
├── HTTP Server Activity
├── Realtime Activity
└── Background Worker Activity
```

The Application may own these Activities.

During shutdown it coordinates their termination before releasing shared infrastructure.

A child Activity intentionally moved to another broader owner may survive.

That transfer must be explicit.

## Activity shutdown ordering

Ownership helps determine shutdown order.

A typical application shutdown may look like:

```text
request application shutdown
    ↓
stop accepting new domain work
    ↓
terminate or drain child Activities
    ↓
wait for dependent Operations
    ↓
release domain-owned Resources
    ↓
stop shared execution Providers
    ↓
release Platform infrastructure
```

The exact sequence depends on the application.

The important principle is that shared infrastructure should generally remain alive until dependent work no longer needs it.

## Resource dependency ordering

If Resource B depends on Resource A, A must normally remain valid while B can still use it.

For example:

```text
Async Context
    needed by
Server Activity
    needed by
Request Operation
```

Shutdown should therefore avoid:

```text
destroy Async Context
    ↓
Request still active
```

The ownership graph should make such ordering visible.

This is one reason architecture should document ownership rather than relying only on destructor ordering.

## Shutdown authority

Shutdown authority belongs to the owner or an explicitly authorized orchestrator.

A borrower may react to shutdown.

It may release its borrow.

It may cancel its own dependent Operations.

It may terminate its own Activity.

It does not automatically gain authority to shut down the Resource being borrowed.

For example:

```text
Server observes Async Context unavailable
    → Server may terminate

Server does not own Async Context
    → Server does not decide global Async shutdown
```

These are different directions of lifecycle influence.

## Close authority

Close follows the same rule.

A Session owning a Socket may close that Socket.

A Request borrowing the Socket for one write does not necessarily have authority to close the entire connection.

The exact contract may grant that authority in some cases, such as a fatal protocol error.

The important requirement is that the authority comes from the contract, not from possession of the handle.

## Cancellation authority

Cancellation authority is not identical to Resource ownership, but the concepts are related.

The owner of an Operation lifetime commonly has authority to request its cancellation.

For example:

```text
HTTP Request owner
    can cancel Request Operation
```

A child implementation observing the request does not thereby become the owner of the parent Operation.

Likewise, an execution Provider delivering cancellation does not own the Operation lifetime.

The architecture should avoid letting cancellation tokens accidentally become proxies for unrelated Resource ownership.

## Ownership and Deadline

A Deadline belongs semantically to the bounded lifetime whose contract establishes it.

A child Operation participates in the parent's Deadline when its work belongs to the parent lifetime.

It does not own the Deadline in the sense of being allowed to extend it.

Conceptually:

```text
Parent Operation
    establishes Deadline D

Child Operation
    inherits effective Deadline ≤ D
```

This is another form of lifecycle constraint flowing downward without transferring parent authority.

## Error ownership

Errors produced by child Operations may be propagated to parents.

The parent may add context or translate the Error.

This does not create lifecycle ownership concerns in the same way Resources do, but causal identity should remain preserved where relevant.

A parent should not destroy useful causal information simply because it now owns the higher-level Operation contract.

Ownership of abstraction does not imply ownership of history.

## Public API ownership signals

Public APIs should make important ownership relationships understandable.

Possible signals include:

```text
constructor structure
factory functions
move-only types
references
handles
explicit adopt functions
documentation
naming
```

No single C++ mechanism is mandatory.

The chosen representation should help programmers answer whether an API:

```text
borrows
retains a borrow
takes ownership
returns ownership
shares lifecycle authority
```

where the distinction matters.

## Retained borrowing

A borrow may survive beyond one function call.

For example:

```text
Server constructor receives Async Context
Server stores access for entire Server lifetime
```

This is still borrowing if lifecycle authority remains external.

The architecture should not use “borrowed” as shorthand for “temporary stack reference”.

Borrowing describes authority.

Its duration may be short or long.

## Shared memory lifetime

Reference counting can be useful when a borrower must ensure that the implementation object remains allocated while in use.

For example:

```text
shared_ptr<AsyncContext>
```

may prevent premature deallocation.

The semantic relationship may still be:

```text
Application
    lifecycle owner

Server
    retained borrower
```

The Architecture should document the semantic owner separately from the C++ reference count.

This distinction should be repeated wherever shutdown affects multiple consumers.

## Cyclic ownership

Strong reference cycles can prevent C++ objects from being destroyed.

Semantic ownership cycles can be even more dangerous because they make lifecycle authority unclear.

For example:

```text
Server waits for Worker
Worker waits for Server
```

may make shutdown impossible to define cleanly.

The architecture should prefer acyclic lifecycle authority where practical.

Mutual references may exist.

Mutual lifecycle ownership should require explicit justification.

## Resource registries

A registry may store references to Resources without owning their operational lifecycle.

For example, observability or lookup infrastructure may retain handles to active connections.

The architecture should not assume:

```text
registry contains Resource
    therefore registry owns Resource
```

The registry contract must distinguish observation, retention, and lifecycle authority.

This matters especially for global managers and service registries.

## Factories

A factory creates Resources.

Creation does not determine who remains owner after creation.

A factory may:

```text
create and retain ownership

create and transfer ownership

create Resource owned by another orchestrator

return a borrow to an internally owned Resource
```

The API must define which model applies.

The architecture should not infer ownership solely from creation.

## Providers creating Resources

The same rule applies to Providers.

A TCP Provider may create a Socket.

A Process Provider may create a Process handle.

A filesystem Provider may create a File Resource.

The Provider may transfer ownership to the caller even though it implemented the creation Operation.

Capability provision and Resource ownership remain separate dimensions.

## Historical ownership problems

Broad historical runtime abstractions often blur lifecycle authority.

A component receives one execution object.

The object is passed through several constructors.

Eventually a deep component calls `stop()` because it has access to the object.

The implementation may remain memory-safe while the system lifecycle becomes incorrect.

The Vix3 architecture should treat these cases as ownership defects rather than merely API inconvenience.

The migration should restore clear authority.

## Current RuntimeExecutor lesson

The historical RuntimeExecutor path is a concrete example.

A `shared_ptr<RuntimeExecutor>` could be stored by several components.

That representation did not answer whether each component was allowed to stop it.

Audit showed that several components did not even use RuntimeExecutor for actual execution.

Removing those dependencies reduced both coupling and ownership ambiguity.

Where RuntimeExecutor remains temporarily, its lifecycle authority should be explicit rather than inferred from shared storage.

## AttachedRuntime lesson

The AttachedRuntime lifecycle work established an important architectural pattern.

Externally supplied execution infrastructure is borrowed and must be preserved.

Helper-created infrastructure is owned by the helper and may be stopped by that helper.

Conceptually:

```text
external Resource
    → borrowed
    → preserve lifecycle

helper-created Resource
    → helper-owned
    → helper ends lifecycle
```

This distinction should become a general Vix ownership pattern rather than remain specific to one helper.

## Ownership modes

For important Resources, APIs may naturally support several modes.

For example:

```text
Borrowed
    externally owned Resource

Owned
    component-created Resource

Adopted
    Resource created elsewhere,
    ownership explicitly transferred
```

The Standard does not require a universal enum with these names.

The architecture can still use these categories to reason about APIs.

If a component behaves differently depending on ownership mode, that difference should not be hidden.

## Ownership should not depend on constructor accidents

A constructor receiving an argument does not automatically mean borrowing.

Moving an argument does not always prove semantic ownership transfer.

Storing an object does not automatically prove ownership.

The contract must decide.

The implementation representation should then make the contract difficult to violate.

This order matters.

Otherwise the architecture becomes an interpretation of C++ syntax rather than a design.

## Ownership and conformance

Ownership mistakes can become Standard conformance defects.

For example:

```text
borrowed execution Resource
    stopped by borrower
```

violates the Resource ownership semantics.

Likewise:

```text
parent completes
child continues using destroyed parent Resource
```

violates Operation and lifetime semantics.

Ownership tests should therefore validate observable behavior rather than only pointer types.

## Ownership tests

Useful tests include:

```text
borrower shutdown does not stop external Provider

component-owned Provider is stopped during component shutdown

ownership transfer leaves only new owner with lifecycle authority

child Operation cannot access parent-owned state after parent completion

detached work survives only after valid ownership transfer

server shutdown does not terminate shared Async Context

connection shutdown closes owned transport
without closing unrelated shared infrastructure
```

These tests make architectural ownership executable.

## Current high-level ownership map

The intended common structure is approximately:

```text
Application / Orchestrator
│
├── owns Async Context
├── owns CPU / Blocking Execution Providers
├── owns application-level shared Providers
├── owns major domain components
│
├── HTTP Server
│   ├── owns server-specific Resources
│   ├── owns Request Activity coordination
│   └── borrows Async / Platform Providers
│
├── WebSocket Server
│   ├── owns server-specific Resources
│   ├── owns Connection Activity coordination
│   └── borrows Async / Platform Providers
│
├── Database Client / Pool
│   ├── owns database-specific Resources
│   └── borrows execution / network Providers
│
├── Realtime
│   ├── owns Realtime Activity
│   ├── may own current transport
│   └── borrows shared Platform Providers
│
└── Game / Agent / other domains
    ├── own domain lifetime
    └── borrow general Platform capabilities
```

This diagram is illustrative.

Not every application needs all of these components.

The architectural pattern is what matters.

## Domain-owned versus Platform-owned Resources

A useful distinction is:

```text
Platform Resource
    execution context
    worker pool
    low-level Provider infrastructure

Domain Resource
    HTTP Server
    WebSocket connection
    database pool
    Realtime subscription
    game scene
```

A domain may own its domain Resources while borrowing Platform Resources.

This creates a natural dependency direction.

It also makes shutdown easier to reason about:

```text
stop domain work first
then stop shared Platform infrastructure
```

where the application owns both.

## Ownership and Ecosystem boundaries

An Ecosystem component should own the semantics and Resources specific to its domain.

It should not become the owner of foundational Platform infrastructure merely because it depends on that infrastructure.

For example:

```text
Web owns:
    routes
    server lifecycle
    request processing

Web does not automatically own:
    global Async Context
    global CPU Provider
    global Logging infrastructure
```

Similarly:

```text
Game owns:
    game Activities and Resources

Game does not automatically own:
    application-wide worker execution
```

unless the application explicitly gives it that ownership.

## Ownership and Provider replacement

A consumer should ideally remain unaffected when one Provider is replaced by another that preserves the same Capability and ownership contract.

For example:

```text
Application
    owns CPU Execution Provider

Game
    borrows CPU execution
```

should remain understandable whether the Provider is the current Threadpool or a future work-stealing implementation.

This is one reason ownership should be defined above concrete Provider types.

## Ownership and Profiles

Profiles may affect which Resources and Providers exist, but they do not change ownership semantics.

A WASM environment may provide an Async execution facility through a host event loop.

A native environment may use an operating-system event mechanism.

In both cases:

```text
borrowed execution Resource
    remains borrowed
```

The Provider changes.

The ownership rule does not.

This keeps lifecycle reasoning portable.

## Architectural requirements

Every stateful shared Runtime Resource should have an identifiable lifecycle owner.

Access to a Resource must not be treated as lifecycle authority.

Retained borrowing must remain borrowing unless ownership is explicitly transferred.

Shared memory ownership must not be used as the sole definition of operational lifecycle authority.

Externally supplied execution Resources should be treated as borrowed unless their contract explicitly transfers ownership.

Component-created private Resources should normally be owned by the component unless ownership is transferred.

Owners must keep Resources valid for dependent borrowers according to their contracts.

Borrowers must not stop or close broader-scope Resources merely because the borrower itself is terminating.

Parent Operations must not complete while dependent children still use parent-owned Resources unless ownership or lifetime has been transferred.

Detached work must acquire a valid independent owner.

Shared Platform infrastructure should generally outlive the domain Activities that depend on it during coordinated shutdown.

Provider ownership must remain distinct from Capability consumption.

Domain components should own domain-specific Resources while borrowing broader Platform Resources where appropriate.

Ownership transfer should be explicit at important architectural boundaries.

## Open ownership questions

Some ownership relationships still require further audit.

The long-term ownership model of higher-level Web and WebSocket `Server` and `App` execution infrastructure should be finalized as remaining RuntimeExecutor dependencies are removed.

The networking redesign may reveal whether some Provider Resources should be application-owned, client-owned, or transparently internal.

TLS transport ownership needs to be documented when the canonical TLS API is stabilized.

Database connection ownership, leasing, pool lifetime, and asynchronous execution relationships need a dedicated architecture audit.

The final canonical representation of retained borrowing and ownership transfer in public C++ APIs also remains open.

These questions should be resolved from real lifecycle requirements rather than by introducing a universal ownership wrapper prematurely.

## Design consequence

The Ownership Map should make one question answerable for every important stateful Resource:

> Who is allowed to decide when this Resource stops being usable?

A second question should then be separate:

> Who is allowed to use it while it remains valid?

Vix becomes difficult to reason about when these two questions receive the same answer merely because components share a pointer.

The architecture should instead allow relationships such as:

```text
Application
    owns Async Context

Server
    borrows Async Context

Session
    owns Connection

Operation
    borrows Connection

Background Service
    owns detached Job
```

to remain explicit.

When ownership is clear, shutdown becomes easier to design.

Cancellation becomes easier to propagate.

Resource lifetime becomes easier to test.

Provider replacement becomes safer.

And execution infrastructure can be shared without turning every consumer into a global lifecycle authority.

That is the purpose of the Vix Ownership Map.
