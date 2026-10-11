# Resources and Ownership

Resources are one of the foundations of the Vix programming model.

A resource is anything whose validity and lifetime matter to the behavior of software. It may represent something provided directly by the operating system, such as a file descriptor or socket, but it may also represent a higher-level object such as a database connection, a server, an execution context, a timer source, a process handle, a subscription, or another entity whose continued validity affects the operations that use it.

The Standard does not define resources by their C++ representation. A resource may be represented by a class, a handle, a pointer, a reference, a shared object, a value type, or another mechanism. What makes something a resource is the existence of meaningful lifecycle semantics around it.

The central rule of this section is that access to a resource and authority over its lifecycle are separate concepts.

A component may be allowed to use a resource without having the right to stop it, close it, destroy it, or otherwise invalidate it. This distinction must remain clear even when the underlying C++ representation allows several components to retain references to the same object.

## Resource lifetime

Every resource has a period during which it is valid for the operations allowed by its contract.

This semantic lifetime is not necessarily identical to the lifetime of the C++ object representing the resource.

A socket wrapper may remain allocated after the underlying socket has been closed. A handle object may still exist after the external service it represents is no longer available. Several wrapper objects may refer to the same underlying resource. A resource may also transition through states such as created, active, closing, and closed without the representing object itself being destroyed.

The Standard therefore treats resource lifetime as a semantic property.

Code must not assume that object existence alone proves that a resource remains usable.

The applicable resource contract must define the conditions under which the resource is considered valid for a particular operation.

## Ownership

Ownership describes lifecycle authority and responsibility.

An owner is an entity that is permitted and expected to control the lifetime of a resource according to its contract. Depending on the resource, that authority may include starting it, stopping it, closing it, releasing it, joining it, flushing it, or performing another action required to bring its lifetime to a valid end.

Ownership is not inferred from the syntax used to store the resource.

A `std::unique_ptr` often corresponds naturally to exclusive ownership of memory, and a `std::shared_ptr` often represents shared memory lifetime. Those properties do not by themselves define operational ownership of the resource represented by the object.

For example, a component may hold a `std::shared_ptr` to an execution context because the execution context must remain alive while the component uses it. That does not automatically give the component authority to stop the execution context.

The resource contract determines lifecycle authority.

## Borrowing

A borrower is permitted to use a resource without receiving ownership of its lifecycle.

Borrowing may be short-lived, such as passing a reference into a function, or long-lived, such as a component retaining access to an externally created execution service throughout its own lifetime.

The duration of the borrow does not change its ownership semantics.

A component that retains a borrowed resource for several hours does not become its owner merely because the borrow is long-lived.

A borrower MUST NOT close, stop, destroy, or otherwise invalidate the borrowed resource unless the contract explicitly grants that authority.

A borrower MAY perform operations on the resource that are permitted by the borrowing contract.

The contract must make it possible to determine whether the resource is borrowed when lifecycle authority matters.

The Standard does not require every borrowing relationship to be encoded in a dedicated C++ type, but the semantics must not be ambiguous.

## Shared lifetime is not shared lifecycle authority

Several entities may participate in keeping the same underlying object alive.

That does not necessarily mean that every participant has equal authority over the resource's operational lifecycle.

This distinction is especially important when resources are represented through reference-counted objects.

Consider an execution facility shared by several components. Each component may need the facility to remain allocated while it is in use, so shared memory lifetime may be appropriate. However, stopping the facility may affect every component using it.

The right to retain the object and the right to stop the service are therefore different rights.

A conforming design MUST NOT infer lifecycle authority solely from reference-counted ownership.

The same principle applies to connections, servers, thread pools, runtime environments, shared caches, database pools, and other resources whose shutdown has effects beyond the current component.

## Exclusive ownership

A resource may have one entity with exclusive lifecycle authority.

In that case, the owner is responsible for completing the lifecycle obligations defined by the resource contract.

Exclusive ownership does not require that no other component may access the resource. Other components may still borrow it.

For example:

```text
Application
    owns Server

Request handlers
    borrow Server state
```

The important property is that lifecycle authority remains with the application or other designated owner.

Borrowers must not independently terminate the owned resource.

## Shared lifecycle authority

Some resources may intentionally support shared lifecycle authority.

This is different from merely sharing references.

If several entities genuinely share lifecycle ownership, the resource contract must define what shared ownership means and how conflicting lifecycle actions are coordinated.

The Standard does not assume that shared lifecycle authority is appropriate merely because the underlying representation uses shared memory ownership.

Shared lifecycle authority should be explicit because shutdown actions often have global effects.

Where one clear owner is sufficient, Vix should prefer a clear owner plus borrowers over ambiguous shared lifecycle responsibility.

## Ownership transfer

Ownership may move from one entity to another.

An ownership transfer changes lifecycle authority.

After transfer, the receiving entity assumes the ownership responsibilities defined by the contract. The previous owner must not continue acting as if it retained exclusive lifecycle authority unless the contract explicitly establishes shared ownership.

Transfer should be explicit at the semantic boundary.

Passing an object as an argument does not automatically mean ownership transfer.

Moving a C++ object may often be used to represent transfer, but the Standard does not define ownership transfer purely in terms of move semantics. Some resources may use handles or other representations where semantic ownership transfer occurs without a conventional C++ move operation.

The API contract must make the transfer understandable.

## Creation and ownership

When a component creates a resource for its own use, it normally owns that resource unless the contract immediately transfers ownership elsewhere.

This leads to a useful default:

```text
creator
    creates resource
    owns resource
    ends resource lifetime
```

This default should not be applied blindly, but it provides a predictable starting point.

If a factory, platform service, or orchestration layer creates a resource and transfers it to another owner, that transfer must be part of the contract.

If the creator merely provides access to a resource that it continues to own, consumers are borrowers.

## Orchestration ownership

Some resources are created as part of a larger orchestration.

An orchestration layer may create several resources, connect them together, run a coordinated activity, and later shut them down in a defined order.

In that case, the orchestration layer may own resources that individual components merely borrow.

For example:

```text
Orchestrator
    owns ExecutionContext
    owns Server
    owns Transport

Server
    borrows ExecutionContext
```

When the server stops, it must not stop the execution context simply because it has access to it. The orchestrator remains responsible for ending the larger runtime lifetime.

This separation allows several components to share infrastructure without making each component responsible for global shutdown.

## Lifecycle actions

Resource contracts should distinguish the lifecycle actions that matter for that resource.

Depending on the resource, these may include operations such as:

```text
start
activate
stop
shutdown
close
release
join
flush
reset
destroy
```

The Standard does not give these words one universal API shape. Different resources have different lifecycle models.

However, lifecycle actions that invalidate or globally affect a resource must only be performed by an entity with the corresponding authority.

A method being publicly callable in C++ does not by itself mean that every holder of the object is semantically authorized to call it.

The API and architecture should make misuse difficult where practical, but semantic authority remains defined by the contract.

## Close and destruction

Closing a resource and destroying the C++ object representing it are not necessarily the same event.

A file object may be explicitly closed before its wrapper object is destroyed. A server object may transition to a stopped state while remaining inspectable. A resource handle may release external state while local metadata remains available.

The contract should define whether explicit close is required, optional, or performed automatically.

Automatic cleanup is encouraged where it preserves the required semantics, but RAII does not remove the need for a clear ownership model.

RAII answers when cleanup occurs for an owning object. It does not answer whether the current object was supposed to own the underlying resource in the first place.

## Start and stop symmetry

When a component owns a resource whose lifecycle requires an explicit start and stop, that component should normally be responsible for both sides of the lifecycle unless the contract defines another authority model.

A component that starts an externally owned resource introduces ambiguity unless the contract explicitly permits it.

Likewise, a component that stops a resource it did not create or adopt may invalidate unrelated users.

Vix should prefer lifecycle relationships in which the responsible owner is clear from creation through termination.

There are legitimate cases where ownership is adopted after creation or where startup is coordinated by another layer. Those cases should be explicit rather than accidental.

## Resource validity

An operation using a resource requires the resource to remain valid for the period defined by the operation contract.

For synchronous operations, this may simply mean that the resource remains valid until the call returns.

For asynchronous operations, the required lifetime may extend beyond the call that initiated the operation.

A conforming API must not silently rely on a borrowed resource after the permitted borrowing lifetime has ended.

When an asynchronous operation retains access to a resource, the relationship between the operation lifetime and resource lifetime must be clear enough to prevent use after invalidation.

The Standard does not require a single implementation mechanism for ensuring this. Implementations may use ownership, reference counting, scoped lifetime relationships, cancellation, joining, or another appropriate strategy.

## Borrowed data and asynchronous operations

Borrowing becomes especially important when an operation continues asynchronously.

An operation that borrows input memory, a resource, an execution context, or another object must respect the lifetime guaranteed by the caller.

An API must not retain a short-lived borrow beyond the contract that established it.

If an operation needs the resource to remain available for longer, the API must either establish an appropriate retained lifetime, copy the required data, transfer ownership, or otherwise make the extended lifetime explicit.

This applies not only to large resources but also to buffers, strings, callbacks, configuration objects, and other state accessed after the initiating call returns.

## Resource relationships in composition

Higher-level components often compose several resources.

For example, a client may use a resolver, transport, TLS provider, timer facility, and execution environment. A server may use a listener, router, logger, execution context, and collection of active sessions.

Composition must not erase ownership boundaries.

A component that borrows five resources does not become the owner of those resources simply because it coordinates their use.

Likewise, a resource owned by a parent component should not become independently owned by every child operation that temporarily uses it.

The composition contract should preserve which entity owns each resource and which entities merely borrow it.

## Parent and child lifetimes

Resources may be scoped to a larger lifetime.

A connection owned by a session may naturally become invalid when the session ends. A temporary file owned by an operation may be removed when the operation completes. A child activity may use resources owned by the parent activity.

These relationships should be explicit enough that cleanup order is predictable.

If a child resource cannot remain valid after its owner terminates, dependent operations must either complete, cancel, detach from that dependency, or otherwise stop using the resource before invalidation.

A parent lifetime ending must not leave children silently accessing invalid resources.

## Independent resources

Not every resource belongs to the lifetime of the component currently using it.

A global application runtime, shared database pool, externally managed event loop, or platform service may outlive many individual operations and activities.

Users of such resources are normally borrowers.

The fact that a component is shutting down does not automatically authorize it to terminate resources whose lifetime belongs to a broader scope.

This is one of the reasons ownership must be modeled separately from usage.

## Resource shutdown and active operations

Closing or shutting down a resource may affect operations currently using that resource.

The resource contract should define the observable relationship between lifecycle termination and active operations.

Depending on the resource, active operations may complete normally, fail, receive cancellation, drain before shutdown, or follow another defined behavior.

The Standard does not require one universal shutdown policy.

It does require that lifecycle termination not create ambiguous operation semantics.

A resource owner initiating shutdown must follow the resource contract for handling operations that still depend on the resource.

## Borrowers and shutdown

A borrower may observe that a borrowed resource is shutting down or has become unavailable.

That observation does not give the borrower ownership authority.

The borrower should respond according to the resource and operation contracts, which may include allowing current work to fail, cancelling dependent operations, releasing its borrow, or transitioning its own activity toward termination.

The borrower must not attempt to take over lifecycle control merely because the owner initiated shutdown.

## Failure during cleanup

Resource cleanup itself may fail.

For example, flushing a file may fail, shutting down a transport may encounter an operating-system error, or committing final state may be unsuccessful.

The Standard does not require all cleanup failures to be handled identically.

The resource contract should define which cleanup failures are observable, which can be reported through normal operation results, and which occur in contexts where ordinary error propagation is no longer possible.

Destructors deserve particular care because C++ destructors often cannot safely communicate ordinary operational failures.

Where cleanup failure matters to application correctness, APIs should provide an explicit lifecycle operation that allows the result to be observed rather than relying entirely on implicit destruction.

## Idempotent lifecycle operations

Some lifecycle actions may be safely repeated.

For example, a resource may define repeated `close()` or `stop()` calls as no-ops after the first successful transition.

Other resources may treat repeated lifecycle actions differently.

The Standard does not require every lifecycle operation to be idempotent.

When repeated lifecycle calls are allowed, the contract should define their behavior clearly and must not permit repeated calls to create multiple logical termination events for the same resource lifetime.

## Resource state

Some resources have meaningful runtime states.

For example:

```text
created
starting
active
stopping
closed
failed
```

The Standard does not require a universal resource-state machine.

However, when operations are only valid in particular states, the contract should define those conditions.

Calling an operation while the resource is in an invalid state may be an operational Failure or a programming Fault depending on the contract.

For example, attempting to use a remote connection after the peer has disconnected may be an operational condition. Calling an internal method before a required initialization step may indicate a programming fault.

The contract boundary determines the classification.

## Ownership and capabilities

Capabilities and resources are related but distinct.

A capability describes an ability that the environment can provide.

A resource is an instance whose lifetime matters.

For example:

```text
TCP capability
    enables connect operation
        produces Socket resource
```

The existence of the TCP capability does not itself create a socket resource.

Likewise, a provider that supplies a capability may or may not itself be a resource.

A stateless provider may have no meaningful lifecycle. A thread pool, connection pool, or event loop provider may have an important lifecycle and therefore participate in the resource model.

These roles must not be conflated.

## Ownership and authorization

Ownership does not automatically imply unrestricted permission.

An application may own a file handle while the operating system still limits which operations are allowed. A component may own a connection but lack authorization to perform a particular remote action.

Ownership governs lifecycle authority.

Authorization governs permission to perform an action.

The two may interact, but they are different concepts.

## Ownership should be visible at important boundaries

The Standard does not require ownership annotations on every parameter.

However, public boundaries where lifecycle mistakes would be significant should make ownership expectations understandable.

A programmer should be able to determine whether passing a resource to an API means:

- the API borrows the resource;
- the API retains a borrow;
- the API takes ownership;
- the API shares lifecycle authority;
- or the resource remains governed by another orchestrator.

This information may be communicated through type design, naming, documentation, constructor structure, factory behavior, or another appropriate mechanism.

What matters is that the contract is not dependent on guesswork.

## Normative requirements

A resource MUST have semantics that make its valid lifetime distinguishable from its invalid lifetime when that distinction affects observable behavior.

Access to a resource MUST NOT be treated as lifecycle ownership unless the contract grants that authority.

A borrower MUST NOT invalidate a borrowed resource unless lifecycle authority has explicitly been granted or transferred.

Shared memory ownership MUST NOT be assumed to imply shared operational lifecycle authority.

Ownership transfer MUST be explicit at the semantic boundary.

An implementation MUST preserve resource validity for the period required by operations that are permitted to use that resource.

Asynchronous work MUST NOT continue accessing a borrowed resource after the permitted borrowing lifetime has ended.

A component MUST NOT silently terminate a broader-scope resource merely because the component itself is terminating.

Lifecycle actions that affect active operations MUST have behavior defined by the applicable resource and operation contracts.

Resource composition MUST preserve ownership relationships rather than silently converting borrowed resources into owned resources.

When cleanup failure is important to application correctness, an API SHOULD provide a mechanism through which that failure can be observed.

Public APIs SHOULD make significant ownership and borrowing relationships understandable without requiring knowledge of internal implementation details.

Where one clear lifecycle owner is sufficient, implementations SHOULD prefer clear ownership with explicit borrowing over ambiguous shared lifecycle authority.

## Example: borrowed execution resource

Consider a server that receives an execution context created by the application:

```text
Application
    owns ExecutionContext

Application
    creates Server
        Server borrows ExecutionContext
```

The Server may retain access to the execution context for as long as the application contract permits.

The Server may submit or associate work with that context if the relevant execution contract allows it.

Stopping the Server does not authorize the Server to stop the ExecutionContext.

The ownership relationship remains:

```text
Application
    owns ExecutionContext

Server
    borrows ExecutionContext
```

even if both components hold reference-counted access to the same implementation object.

## Example: component-owned resource

Consider a client object that creates an internal transport solely for its own use:

```text
Client
    creates Transport
    owns Transport
```

Unless ownership is transferred elsewhere, the client is responsible for ending the transport lifetime according to its contract.

Other operations may borrow the transport while they are active.

When the client ends its owned transport, it must account for those dependent operations according to the applicable shutdown semantics.

## Example: orchestration-owned resources

Consider an orchestration helper that creates an execution facility, a server, and a transport:

```text
Orchestrator
├── owns Execution Facility
├── owns Server
└── owns Transport

Server
└── borrows Execution Facility
```

The orchestrator may start the components in the required order, run the coordinated activity, and later shut the owned resources down in the correct order.

The Server does not gain lifecycle authority over the execution facility merely because it needs that facility while running.

This allows the orchestration layer to manage the complete system lifetime without leaking global lifecycle responsibility into individual components.

## Design consequence

Ownership should answer a simple question:

> Who is responsible for deciding when this resource's usable lifetime ends?

Borrowing should answer a different question:

> Who is allowed to use this resource while someone else remains responsible for its lifetime?

Keeping those questions separate makes resource composition predictable.

It also allows Vix APIs to share infrastructure safely without turning every reference into implicit lifecycle authority.

That distinction is the foundation on which operation lifetime, activity shutdown, execution resources, provider lifetimes, and higher-level composition are built.
