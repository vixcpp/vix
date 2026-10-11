# Terminology

The Vix Standard uses a small set of terms with precise meanings.

These terms are intentionally defined independently from current modules, namespaces, classes, or implementation details. A term in the Standard describes a semantic concept. An implementation may represent that concept with a C++ type, a function, a coroutine, a handle, a runtime object, or no dedicated object at all.

The purpose of this vocabulary is to make discussions across Vix consistent. A term should not quietly acquire a different meaning because it appears in Web, networking, processes, storage, asynchronous execution, or another domain.

When a domain needs a more specific concept, it may specialize the vocabulary defined here, but it should not redefine the underlying meaning.

## Resource

A **Resource** is something whose validity and lifetime matter.

A resource may represent something physical, logical, local, remote, or entirely software-defined. Examples include an open file, a socket, a process handle, a database connection, a listener, an execution context, a timer source, or another object whose continued validity affects the operations that use it.

A resource does not need to correspond directly to an operating-system resource. The important property is that there is a meaningful distinction between the resource being valid and no longer being valid.

Resources may be created, acquired, borrowed, shared, transferred, closed, released, or destroyed according to their contract.

The fact that code can access a resource does not by itself determine who controls its lifetime.

## Lifetime

A **Lifetime** is the period during which a resource, operation, activity, or other semantic entity remains valid for the uses permitted by its contract.

Lifetime is broader than C++ object lifetime.

A C++ object may still exist while the logical resource it represents has already been closed. Conversely, multiple C++ objects may participate in the lifetime of one underlying resource.

The Standard therefore uses lifetime to describe semantic validity rather than merely memory allocation and destruction.

## Owner

An **Owner** is an entity that has lifecycle authority over a resource or other lifetime-bearing entity.

Lifecycle authority may include responsibility for starting, stopping, closing, releasing, or otherwise ending the entity's usable lifetime.

Ownership is defined by contract, not by the incidental C++ representation used to hold the resource.

For example, possession of a `std::shared_ptr` does not automatically mean that the holder has authority to stop the service represented by that object.

## Borrower

A **Borrower** is an entity that is permitted to use a resource without receiving ownership of its lifecycle.

A borrower may retain access for as long as the contract allows, but borrowing does not automatically grant permission to stop, close, destroy, or otherwise invalidate the borrowed resource.

Borrowing is therefore a semantic relationship, not merely a pointer category.

## Ownership Transfer

An **Ownership Transfer** is an explicit change in lifecycle authority.

After ownership has been transferred, the receiving entity becomes responsible for the lifecycle obligations defined by the contract, while the previous owner no longer retains those responsibilities unless shared ownership is explicitly part of that contract.

Ownership transfer must not be inferred simply from passing an object to another function or component.

## Operation

An **Operation** is a finite unit of work that eventually reaches one terminal completion.

An operation represents something being attempted. Examples include opening a file, resolving a name, connecting a socket, reading data, writing data, executing a query, launching a process, waiting for a timer, or performing an HTTP request.

An operation may be synchronous or asynchronous. It may complete immediately or remain active for some time. It may use resources, require capabilities, depend on an execution environment, carry a deadline, or receive a cancellation request.

These details do not change the defining property of an operation: it represents finite work and has one terminal completion.

An operation is not the same thing as a coroutine, thread, future, task object, scheduler entry, or executor. Those may be implementation mechanisms used to represent or progress an operation.

## Activity

An **Activity** is continuing behavior that may remain active over time and may produce multiple events before terminating.

Examples include a server accepting connections, a WebSocket connection receiving messages, a filesystem watcher, a realtime subscription, or another long-lived source of ongoing behavior.

An activity differs from an operation because an operation represents one finite attempt, while an activity may produce an open-ended sequence of events.

An activity may still terminate normally, fail, or be stopped by lifecycle control.

Activities often use resources and may create or supervise operations during their lifetime.

For example, an HTTP server may be represented conceptually as a resource supporting a server activity, while each individual request is handled through one or more operations.

## Event

An **Event** is a discrete observation produced during the lifetime of an activity or another event-producing facility.

An event may represent something such as an incoming connection, a received message, a filesystem change, or another occurrence that can be observed without terminating the activity that produced it.

The Standard does not require all events to share one universal event representation.

## Completion

A **Completion** is the terminal outcome of an operation.

Once terminal completion has occurred, that logical operation is finished.

Signals such as progress notifications, cancellation requests, retries, timer expiration, or intermediate results are not terminal completions by themselves.

The Standard distinguishes conceptually between successful completion, operational failure, and cancelled completion, while leaving their concrete C++ representation to the API and implementation.

## Success

**Success** means that an operation completed according to its contract.

Success does not necessarily imply that a value is returned. An operation may successfully produce a value, create a resource, modify state, transmit data, or simply complete an intended action.

The operation's contract determines what successful completion means for that operation.

## Failure

A **Failure** is the terminal inability of an otherwise valid operation to satisfy its contract because of an operational condition.

Examples may include a missing file, permission denial, connection refusal, malformed external input, unavailable remote service, exhausted storage, or another condition that the program may reasonably encounter while performing valid work.

Failure is distinct from a programming fault.

A failure is normally represented by structured error information so that the caller can inspect, propagate, enrich, or handle it.

## Error

An **Error** is a structured description of an operational failure.

An error exists to provide stable programmatic identity and useful diagnostic information about why an operation failed.

The human-readable message associated with an error is not its semantic identity. Programs should be able to reason about the nature of an error without depending on exact diagnostic wording.

An error may carry context or causal information added by higher-level operations. Adding context should enrich the explanation of a failure without changing the underlying identity of the original cause.

The exact C++ representation of an Error is outside the responsibility of this terminology document.

## Fault

A **Fault** is a violation of a programming contract, internal invariant, or assumption that should hold during correct execution.

Examples include reaching an impossible internal state, violating a required precondition, corrupting internal state, or performing another action that indicates a programming defect rather than an expected operational condition.

A fault is not an ordinary operational failure.

The Standard does not require faults to participate in normal `Result`-style control flow.

The exact reaction to a fault may depend on the implementation, build configuration, diagnostics system, platform, and applicable contract.

## Result

A **Result** is the conceptual representation of either a successful value or an operational failure.

In current and future Vix APIs this concept may be represented by a type such as `Result<T>`, but the Standard distinguishes the semantics of the concept from any particular implementation.

A Result is not identical to an Operation. An Operation describes work and its lifetime. A Result describes a value-or-failure outcome that may be produced by that work.

The relationship between Result and cancelled completion is intentionally not fixed solely by this terminology document.

## Cancellation

**Cancellation** is the control process by which discontinuation of an operation is requested.

Cancellation is cooperative at the semantic level: requesting cancellation does not itself prove that the operation has stopped.

The operation may observe the request later, may already have completed, or may complete successfully before cancellation takes effect.

Cancellation therefore describes control intent rather than terminal state.

## Cancellation Request

A **Cancellation Request** is a request that an operation discontinue its work when permitted by its contract.

A cancellation request may originate from the caller, a parent operation, a deadline policy, a shutdown process, a resource transition, or another controlling entity.

Repeated requests do not imply repeated operation completion.

## Cancelled Completion

A **Cancelled Completion** is a terminal operation outcome in which cancellation determined that the operation did not complete successfully.

A cancelled completion is distinct from merely having received a cancellation request.

Cancellation also does not imply that no work occurred. An operation may have produced partial effects before cancellation became effective.

## Duration

A **Duration** is a quantity of elapsed time.

A duration does not identify a particular instant by itself.

Durations may be used for delays, intervals, time budgets, or to derive deadlines.

## Clock

A **Clock** provides a time domain in which instants can be observed or compared.

Different clocks serve different purposes.

A wall clock represents civil or calendar time and may be adjusted. A monotonic clock is intended to progress consistently and is normally suitable for measuring elapsed time and enforcing operational deadlines.

The Standard does not assume that all time-related behavior should use the same clock.

## Deadline

A **Deadline** is a temporal boundary associated with an operation or another bounded lifetime.

A deadline expresses that some behavior is subject to a time constraint.

Reaching a deadline does not by itself determine the operation's terminal completion category. The applicable operation contract or timeout policy defines what happens when the deadline is exceeded.

Operational deadlines should normally be based on monotonic time when the platform provides an appropriate monotonic clock.

## Timeout

A **Timeout** is the condition in which an applicable time constraint expires before the required event or completion occurs.

Timeout is not automatically synonymous with failure or cancellation.

Depending on the operation contract, timeout may trigger cancellation, produce an operational failure, produce a normal non-success result, or cause another defined reaction.

The Standard therefore treats timeout as a time-related condition whose terminal meaning is defined by the contract in which it occurs.

## Execution

**Execution** is the progression of work toward the completion of an operation or the continued behavior of an activity.

Execution describes the fact that work is being advanced, not a specific implementation technology.

Execution may be provided by the calling thread, a thread pool, an event loop, an operating-system I/O mechanism, a coroutine runtime, a hardware queue, or another provider.

Execution is distinct from the operation being executed.

## Scheduling

**Scheduling** is the policy or mechanism that determines when and where executable work is allowed to progress.

Scheduling may include decisions related to ordering, priority, worker selection, fairness, deadlines, or other execution policies.

Scheduling is distinct from ownership and lifecycle.

An entity may use a scheduler without owning the scheduler's lifetime.

## Execution Context

An **Execution Context** is an environment in which work can progress.

An execution context may be stateful and may itself be a resource with a meaningful lifetime.

The Standard does not require one universal execution-context interface.

The term should be used only when an actual execution environment is relevant and should not become a generic container for unrelated services.

## Capability

A **Capability** describes an ability that software may require or an environment may provide.

Examples include filesystem access, process execution, TCP communication, DNS resolution, TLS, monotonic time, asynchronous I/O, or CPU execution.

A capability describes semantics, not implementation.

A capability may be known architecturally, at compile time, at runtime, or through a combination of these mechanisms.

The presence of a capability does not imply that every operation using that capability is authorized or guaranteed to succeed.

## Required Capability

A **Required Capability** is a capability that software needs in order to satisfy a particular contract.

A requirement describes what is needed, not whether the current environment provides it.

## Available Capability

An **Available Capability** is a capability that the current environment can provide according to the relevant profile and runtime conditions.

Required capabilities and available capabilities are distinct concepts.

A missing required capability is different from an operational failure that occurs while using an available capability.

## Provider

A **Provider** is an implementation that satisfies one or more capabilities.

A provider may use operating-system services, external libraries, runtime facilities, hardware, or other providers internally.

A capability may have multiple providers, and a provider may satisfy multiple capabilities.

A provider is not necessarily a resource. Some providers are stateless, while others have their own lifetime and therefore also participate in the resource model.

An external library is not automatically a Vix provider simply because a Vix implementation uses it.

## Platform

A **Platform** is the environment in which a Vix implementation operates.

A platform may include operating-system facilities, runtime constraints, hardware characteristics, sandbox restrictions, host APIs, or another execution environment.

Linux, Windows, a WebAssembly host, an embedded environment, or another system may act as platforms for Vix implementations.

Platform and profile are not synonyms.

## Profile

A **Profile** is a named set of semantic guarantees, required capabilities, and constraints that a conforming implementation promises to provide.

Profiles allow different environments to support different capability sets without redefining the meaning of concepts shared between those environments.

A profile is not simply an operating-system name.

Multiple platforms may conform to the same profile, and a platform may potentially support more than one profile.

## Conformance

**Conformance** means satisfying the normative requirements of the applicable part of the Vix Standard.

Conformance includes preserving the semantics of Standard concepts and, where a profile is claimed, providing the guarantees required by that profile.

An implementation does not become conforming merely by exposing APIs with familiar names. Observable behavior must satisfy the applicable semantic contract.

## Composition

**Composition** is the construction of higher-level behavior from lower-level resources, operations, activities, or capabilities.

Composition includes questions such as how ownership is preserved, how child operation lifetime relates to parent lifetime, how cancellation and deadlines propagate, and how lower-level failures gain higher-level context.

Composition does not require that all composed work share the same implementation mechanism.

## Parent Operation

A **Parent Operation** is an operation whose logical work includes or supervises one or more child operations.

Parenthood is a lifetime and composition relationship, not merely a call-stack relationship.

A child may be implemented by a coroutine, callback, task, separate thread, remote request, or another mechanism while still belonging to the logical lifetime of its parent.

## Child Operation

A **Child Operation** is an operation whose lifetime participates in a parent operation's composition contract.

A child operation normally inherits relevant lifetime constraints such as cancellation or an effective deadline unless its contract explicitly establishes an independent lifetime.

## Detached Work

**Detached Work** is work whose lifetime has intentionally been separated from the lifetime that initiated it.

Detachment must be explicit in the semantic contract.

Work should not accidentally outlive the resources, operation, or activity whose state it still uses.

The Standard does not require one particular API named `detach`.

## Shutdown

**Shutdown** is a lifecycle process used to bring an activity, service, subsystem, or other continuing facility toward termination.

Shutdown is broader than cancellation.

A shutdown process may stop accepting new work, allow existing work to drain, request cancellation of operations, close owned resources, and eventually terminate the activity.

The exact sequence is defined by the relevant contract.

## Close

**Close** is a lifecycle action that ends or begins ending the usable state of a resource.

Closing a resource is conceptually different from cancelling an operation that currently uses it, although closing may cause such operations to complete.

Only an entity with appropriate lifecycle authority may close a resource.

## Authorization

**Authorization** determines whether a particular actor or operation is permitted to perform an action.

Authorization is distinct from capability availability.

A platform may provide filesystem capability while denying permission to access a particular file. Likewise, a device may provide camera capability while the user denies access to it.

Capability describes what the environment can support. Authorization describes what a particular operation is allowed to do.

## Contract

A **Contract** defines the expected semantics of an API, resource, operation, activity, capability, or other Standard concept.

A contract may define valid inputs, ownership relationships, successful outcomes, expected failures, cancellation behavior, deadline behavior, side effects, lifetime requirements, and other observable guarantees.

The distinction between an operational failure and a programming fault often depends on the applicable contract.

A condition explicitly accepted as part of normal operation may produce a Failure. Violating a required invariant or precondition may instead constitute a Fault.

## Invariant

An **Invariant** is a condition that must remain true while the relevant contract applies.

Violation of an invariant indicates a programming fault unless the contract explicitly defines another behavior.

## Normative

**Normative** text defines requirements that participate in Standard conformance.

Normative requirements may use terms such as `MUST`, `MUST NOT`, `SHOULD`, `SHOULD NOT`, and `MAY`, as defined by the conformance section of the Standard.

## Informative

**Informative** text explains, illustrates, motivates, or provides historical and implementation context without independently defining conformance requirements.

Examples, rationale, implementation notes, and mappings to the current Vix codebase are generally informative unless explicitly stated otherwise.

## Vix Evolution Proposal

A **Vix Evolution Proposal**, or **VEP**, is a document used to propose and reason about a change to the Vix semantic model or another governed part of the platform.

A VEP may describe the problem, alternatives, trade-offs, compatibility consequences, migration strategy, and reasoning behind a proposed decision.

A VEP is not itself part of the Standard merely because it exists.

Once a proposal is accepted, the relevant normative result may be incorporated into the Standard.

## Use of these terms

Documents in the Vix Standard should use these terms consistently.

Domain specifications may introduce more specific vocabulary, but when they reuse a term defined here, they should preserve its Standard meaning.

Implementation documentation may refer to concrete classes, modules, providers, schedulers, tasks, handles, or other mechanisms. Those implementation terms should not be treated as replacements for the semantic vocabulary defined by the Standard.

The goal of this terminology is not to force one vocabulary onto every line of Vix code. It is to ensure that when foundational ideas are discussed, designed, documented, or standardized, the same words continue to describe the same concepts.
