# Completion and Failure

Every Operation eventually reaches a terminal completion.

Completion is the point at which the logical work represented by the Operation is finished according to its contract. After that point, the Operation may no longer transition to another terminal outcome.

The implementation may continue performing bookkeeping, tracing, memory release, or other internal cleanup after completion becomes observable, but the semantic result of the Operation is already fixed.

The Vix Standard distinguishes successful completion, operational failure, and cancelled completion because these outcomes describe different reasons why an Operation ended.

This distinction is semantic. It does not require every API to expose a type named `Completion`, nor does it require all Operations to use the same concrete C++ representation.

## Terminal completion

An Operation has one terminal completion for one logical lifetime.

The implementation may perform many internal actions before reaching that point. It may call lower-level Operations, suspend, resume, retry, receive cancellation requests, observe deadlines, allocate Resources, or move between execution mechanisms.

None of those events are themselves terminal completion unless the Operation contract defines that the work has finished.

Once terminal completion has been established, the Operation MUST NOT later expose another terminal outcome for the same logical lifetime.

For example, an Operation must not first report Success and later report Failure because a late cancellation request or internal callback arrived afterward.

The completion boundary must remain stable.

## Completion is semantic

Completion should not be confused with destruction of an implementation object.

A coroutine object may still exist after its logical Operation has completed.

A future may retain the completed state so that another observer can inspect it.

A callback wrapper may remain allocated temporarily.

A tracing system may continue recording information about the Operation.

These implementation details do not extend the logical lifetime of the Operation itself.

The important question is whether the caller can now observe that the requested work has reached its defined terminal outcome.

## Success

Success means that an Operation satisfied its contract.

The exact meaning of Success depends on the Operation being performed.

For some Operations, Success produces a value:

```text
resolve hostname
    → Address
```

For others, it produces a Resource:

```text
open file
    → File
```

Some Operations succeed by changing state:

```text
commit transaction
    → committed
```

Others may have no meaningful returned value:

```text
flush
    → Success
```

The Standard therefore does not define Success as “returning a value”.

Success means that the condition promised by the Operation contract was achieved.

## Success and partial work

The fact that an Operation performed some work does not necessarily mean that it succeeded.

Consider an Operation whose contract is to write an entire buffer.

If only part of the buffer is written before the Operation can no longer continue, the terminal outcome depends on the contract.

An Operation such as:

```text
write_some
```

may legitimately treat partial transfer as Success.

An Operation such as:

```text
write_exactly
```

may require all requested data before reporting Success.

The physical events may be similar while the semantic contracts differ.

Completion classification must therefore follow the Operation contract rather than a generic rule about how much work occurred.

## Failure

Failure is the terminal outcome of a valid Operation that could not satisfy its contract because of an operational condition.

The Operation itself was meaningful to attempt.

The program is not necessarily incorrect.

The environment, input, remote system, Resource state, authorization, or another operational condition prevented the requested work from being completed successfully.

Examples may include:

```text
file not found
permission denied
connection refused
remote service unavailable
disk full
invalid external input
name resolution failure
resource temporarily unavailable
```

Whether a condition is considered a Failure depends on the Operation contract.

A parser whose purpose is to accept arbitrary external input may treat invalid syntax as an expected operational Failure.

An internal parser that is only ever called after validated data has already been guaranteed may treat the same condition as evidence of a programming Fault.

The same physical condition can therefore be classified differently at different contract boundaries.

## Error

An Error is the structured description of an operational Failure.

The Error gives the program and the developer information about why the Operation failed.

The Standard does not define an Error as merely a string.

A useful Error representation should allow the program to distinguish one kind of Failure from another without depending on human-readable wording.

For example, code should not need to reason like this:

```text
if message == "connection refused"
```

The semantic identity of the Error must remain separate from its diagnostic message.

Human-readable text may change for clarity, localization, formatting, or improved diagnostics without changing the programmatic meaning of the Failure.

## Error identity

An Error should have stable semantic identity.

The exact representation of that identity is an API decision. It may use error codes, typed categories, structured values, enumerations, domain-specific identities, or another appropriate mechanism.

The important rule is that Error identity must not depend on exact message text.

Two Errors may have different context and messages while still representing the same underlying Failure identity.

Likewise, two Errors with similar messages may represent different Failure identities.

The programmatic contract must not rely on textual similarity.

## Error categories and domains

Different areas of Vix may define more specific Error identities.

Networking may have Errors related to resolution, connection, protocol behavior, or transport state.

Filesystem operations may have Errors related to paths, permissions, existence, capacity, or device state.

A database domain may define Errors related to query execution, constraints, connectivity, or transactions.

These domain-specific Errors do not require one enormous global enumeration containing every possible Failure in Vix.

The Standard requires semantic consistency, not one universal list.

Domain Error systems should still preserve the foundational distinction between Error, Failure, and Fault.

## Error context

Errors often become more useful as they move through higher-level Operations.

A low-level Operation may report:

```text
connection_refused
```

A higher-level networking Operation may add:

```text
while connecting to api.example.com:443
```

An HTTP Operation may then add:

```text
while sending GET /users/42
```

The application may add:

```text
while loading account profile
```

The resulting diagnostic can preserve the chain:

```text
while loading account profile
    while sending GET /users/42
        while connecting to api.example.com:443
            connection_refused
```

Adding context improves the explanation of the Failure.

It should not silently replace the underlying cause.

## Error cause

An Error may have another Error or system condition as its cause.

Causal information is especially useful when one domain builds on another.

For example, an HTTP Failure may be caused by a TLS Failure, which may itself originate from an operating-system or cryptographic error.

The Standard does not require every Error to contain a cause.

Some Errors are already the lowest meaningful semantic description.

When causal relationships exist and are important for diagnosis or program behavior, the implementation should preserve them rather than flattening everything into one generic message.

## Error context does not change identity automatically

Adding context to an Error does not automatically change the identity of the original Failure.

For example:

```text
connection_refused
```

does not stop being a connection refusal because the application adds:

```text
while loading profile
```

A higher-level Operation may choose to expose a higher-level Error identity as part of its own contract, but it should preserve the lower-level cause when that information is relevant.

The distinction is:

```text
what failed at this abstraction level
```

and:

```text
what caused that failure underneath
```

Both may be useful.

## Failure and recovery

The existence of a Failure does not determine whether retry or recovery is appropriate.

For example, a connection refusal may be worth retrying in one context and should fail immediately in another.

A missing file may be normal when looking for an optional configuration file and fatal when loading a required database.

The Standard therefore does not define universal properties such as:

```text
retryable = true
severity = fatal
```

as intrinsic properties of every Error.

Retryability, severity, escalation, fallback, and recovery normally depend on the Error together with the Operation and application policy.

An Error describes what happened.

Policy decides what should happen next.

## Fault

A Fault represents a broken programming contract or invariant.

A Fault means that correct execution was expected to make the observed condition impossible.

Examples may include:

```text
invalid internal state
violated precondition
broken invariant
use of an object in a state forbidden by its contract
impossible branch reached
internal corruption detected
```

Faults are fundamentally different from operational Failures.

A valid application may encounter a missing file.

A correct networking client may encounter connection refusal.

A parser may receive malformed user input.

Those conditions can be ordinary Failures.

An internal invariant being false means the implementation or caller violated a programming contract.

That is a Fault.

## Faults are not ordinary Result flow

The Standard does not require programming Faults to be represented as normal `Result` values.

Automatically converting every Fault into an Error may hide defects and make broken invariants look like ordinary environmental conditions.

Different Vix implementations or build configurations may react to Faults differently.

Possible mechanisms may include:

```text
diagnostic reporting
assertion
exception
termination
debug trap
sanitizer integration
```

The Standard does not choose one universal Fault mechanism here.

What must remain stable is the semantic distinction.

A programming defect should not silently become an ordinary operational Failure merely because an implementation can encode both in the same low-level structure.

## Contract boundary and classification

Whether a condition is Failure or Fault depends on the applicable contract.

Consider an invalid port number.

An API intended to consume untrusted configuration may define invalid ports as ordinary input validation Failure.

An internal function whose precondition guarantees a valid normalized port may treat the same invalid value as a Fault if that precondition is violated.

The difference is not the integer itself.

The difference is what the caller and callee agreed was valid at that boundary.

Contracts therefore play a central role in Failure classification.

## Result

A Result is the conceptual representation of a successful value or an operational Failure.

Conceptually:

```text
Result<T>
├── value
└── Error
```

This model is useful for Operations whose expected operational Failures should be handled explicitly by the caller.

The Standard does not require one exact implementation of `Result<T>` in this document.

Vix may provide a canonical C++ representation elsewhere, but the semantic model comes first.

A Result is not an Operation.

The Operation describes the work, lifetime, Resources, cancellation, deadlines, and execution relationships involved in attempting something.

The Result represents a value-or-Failure outcome associated with that work.

## Result and Success without a value

Some successful Operations do not need to produce a meaningful value.

Their concrete APIs may use a result representation for:

```text
success without value
or
Failure(Error)
```

The exact type used is an API decision.

The semantic model remains the same: the Operation either satisfied its contract or failed operationally.

## Result and Cancellation

The Standard deliberately distinguishes cancelled completion from operational Failure at the semantic level.

A cancellation request represents control intent.

Cancelled completion represents a terminal outcome in which cancellation determined that the Operation did not complete successfully.

An operational Failure represents inability to satisfy the Operation contract because of an operational condition.

These are different concepts.

This document does not require one particular C++ encoding for that distinction.

A future canonical API may represent cancelled completion:

```text
as a distinct completion state
through a structured Error identity
through an asynchronous wrapper around Result
or through another representation
```

provided that the observable semantics defined by the Standard are preserved.

The representation should be decided separately from the conceptual distinction.

## Cancellation is not Failure by definition

Cancellation should not automatically be described as a Failure of the underlying Operation mechanism.

If a user closes a screen and the associated request is intentionally cancelled, the network implementation may have behaved correctly.

If application shutdown cancels outstanding Operations, those Operations did not necessarily encounter environmental Failure.

The termination occurred because control policy decided the work was no longer required.

This is why the Standard keeps Cancelled completion conceptually distinct from operational Failure.

Domain APIs may still choose representations that share infrastructure between Errors and cancellation outcomes, but that implementation choice must not erase the semantic distinction.

## Deadline expiration and Failure

Reaching a Deadline is not automatically a Failure.

A Deadline defines a time constraint.

What happens when that constraint expires depends on the Operation contract.

For one Operation, reaching the Deadline may request cancellation.

For another, it may produce a timeout Error.

For another, it may produce a normal result indicating that no event occurred before the deadline.

For example:

```text
wait_for_message(deadline)
```

might define expiration as a normal “no message” outcome.

A network connection attempt may instead define expiration as an operational timeout Failure.

The time condition is similar.

The contract determines the completion semantics.

## Timeout Errors

A domain may define a timeout Error when exceeding a time constraint constitutes operational Failure for that contract.

In that case, the Error should preserve meaningful programmatic identity rather than requiring callers to inspect a message such as:

```text
"operation timed out"
```

A timeout Error also should not be assumed to mean that no partial work occurred before the timeout condition was reached.

The Operation contract remains responsible for side-effect and partial-progress guarantees.

## Failure and side effects

Failure does not imply rollback.

An Operation may perform externally visible work before failing.

Examples may include:

```text
part of a buffer written
remote request transmitted before response failure
some files created before a later step fails
process started before monitoring setup fails
partial records processed
```

If an Operation promises atomicity, rollback, transactional behavior, or all-or-nothing semantics, that guarantee must come from the Operation contract.

The existence of a Failure by itself does not provide such a guarantee.

## Cancelled completion and side effects

Cancelled completion likewise does not imply that nothing happened.

Cancellation may arrive after some work has already been completed.

A write may have transmitted bytes.

A process may already have started.

A remote request may already have reached another system.

An operation may have allocated and later cleaned up Resources.

The contract should define any observable partial effects that matter.

Callers must not assume that cancellation rewinds the world to the state that existed before the Operation began.

## Failure after partial success

Some Operations may produce useful partial information even though they ultimately fail.

The Standard does not require one universal way to represent such cases.

An API may return partial state together with Error information, expose progress separately, or define another domain-appropriate representation.

What matters is that the contract makes the meaning clear.

A Failure must not be presented as full Success merely because partial work occurred.

Likewise, useful partial state should not be silently discarded when callers need it to reason correctly about the outcome.

## Nested Operations and Failure

A higher-level Operation may be composed from several lower-level Operations.

For example:

```text
HTTP Request
├── DNS lookup
├── TCP connect
├── TLS handshake
├── write request
└── read response
```

A lower-level Failure may cause the parent Operation to fail.

The parent may expose a higher-level Error that describes the failure at its own abstraction level.

However, when useful, the causal chain should preserve the lower-level reason.

For example:

```text
HTTP request failed
    caused by TLS handshake failure
        caused by certificate validation failure
```

This allows higher-level APIs to remain meaningful without destroying diagnostic depth.

## Internal retries and Failure

A lower-level attempt may fail without the logical Operation itself completing with Failure.

For example:

```text
Request Operation
├── attempt 1 → connection Failure
├── attempt 2 → connection Failure
└── attempt 3 → Success
```

The logical Operation completes successfully.

The earlier attempt Failures remain internal unless the contract exposes them for diagnostics or observability.

If every retry attempt eventually fails, the logical Operation may complete with Failure.

The final Error should provide enough information to explain the relevant reason for the logical Failure without requiring the caller to understand implementation-specific retry machinery.

## Failure aggregation

Some composed Operations may encounter several independent Failures.

For example, an operation that queries several providers may receive multiple Failures before deciding that the logical Operation cannot succeed.

The Standard does not define one universal Error aggregation format.

The higher-level contract may choose one primary Failure, preserve several causes, expose an aggregate Error, or use another representation appropriate to the domain.

The representation should still preserve enough semantic information for the caller to understand the logical Failure.

## Activity Failure

Activities also encounter operational Failures.

An individual child Operation may fail without terminating the Activity.

For example, one HTTP request can fail while the HTTP server continues running.

An Activity Failure refers to an operational condition that prevents the continuing behavior of the Activity from satisfying its own contract.

Examples may include permanent loss of a required listener, an unrecoverable subscription failure, or another condition that makes continuation impossible.

The distinction between local Operation Failure and terminal Activity Failure belongs to the Activity contract.

The Error model may be shared, but the lifetime being affected is different.

## Error propagation

Errors may move across abstraction boundaries.

Propagation should preserve semantic meaning.

A lower-level Error should not be converted into an unrelated higher-level Error merely to fit a local implementation style.

Higher layers may translate Errors when their contract genuinely exposes a different abstraction.

Such translation should preserve causal information where appropriate.

The goal is not to expose every implementation detail to every caller.

The goal is to avoid losing the real reason an Operation failed.

## Error handling

The Standard does not require every Error to be handled locally.

An API may:

```text
handle the Error
translate it
enrich it
propagate it
aggregate it
convert it into another domain outcome
```

depending on its contract.

What it should not do is silently erase important Failure information without a semantic reason.

Higher-level APIs should make deliberate decisions about which Failures remain visible at their boundary.

## Error messages

Human-readable Error messages are diagnostic information.

They should be useful to developers and, where appropriate, users.

They should not serve as stable machine-readable identity.

This allows diagnostics to improve over time.

An implementation may add source locations, Resource names, endpoint details, system information, or contextual descriptions without breaking code that handles Errors programmatically.

Sensitive information must still be handled according to the applicable security and privacy requirements, which are outside the scope of this foundational document.

## Unexpected exceptions

Vix operates within the C++ ecosystem, where exceptions may originate from standard libraries, external libraries, user callbacks, allocation failures, or Vix itself.

The Standard does not require all unexpected exceptions to be converted into operational Errors.

An exception may represent:

```text
an expected operational condition
a programming Fault
resource exhaustion
a foreign-library behavior
an implementation failure
```

The applicable boundary must determine how it should be treated.

Blindly translating every exception into a generic Error may erase distinctions that matter for correctness and diagnosis.

Conversely, allowing arbitrary implementation exceptions to leak through an API whose contract promises structured operational Error behavior may violate that contract.

Exception policy therefore belongs to the relevant API and provider contracts while preserving the semantic distinction between Failure and Fault.

## Resource exhaustion

The Standard distinguishes process heap exhaustion from resources supplied by a
Capability or Provider.

Process heap exhaustion is not an operational `Error`. A Vix-controlled
allocation that cannot be satisfied enters a fatal allocation condition and
terminates deterministically. In an exception-enabled build, an implementation
MUST NOT let a recoverable `std::bad_alloc` change an API's declared Vix
outcome; it terminates at its controlled allocation boundary. In an
exception-disabled build, the same condition terminates through the configured
allocation mechanism. This policy means that enabling or disabling C++
exceptions does not change Vix semantics.

Callers remain responsible for allocations they perform before entering a Vix
API, such as materializing an argument expression. Those are ordinary C++
language/library behavior, not a Vix operational result.

By contrast, a Provider's exhaustion of a resource it owns or mediates can be
an expected operational Failure when its domain contract defines a stable
identity. Examples include file-descriptor/handle exhaustion, a provider
resource limit, storage quota exhaustion, or a filesystem/provider inability
to allocate its own resource. Such an identity MUST NOT mean process heap
exhaustion merely because a particular API returns `Result`.

An implementation must not claim recovery guarantees that it cannot reliably
uphold under the relevant exhaustion condition.

## Authorization Failure

An Operation may fail because the actor lacks permission to perform it.

This is distinct from the required capability being unavailable.

For example:

```text
filesystem capability exists

open protected path
    → permission denied
```

The filesystem capability is present.

The individual Operation failed because authorization was insufficient.

This is an operational Failure.

By contrast:

```text
environment provides no filesystem capability
```

is a capability or profile issue rather than a file-open Failure.

## Missing capability

When an Operation fundamentally requires a capability that the applicable environment does not provide, the problem exists before ordinary execution semantics can be satisfied.

Depending on where the requirement is known, this may be detected during:

```text
build
configuration
deployment
startup
runtime capability resolution
```

The Standard treats missing required capability separately from operational Failure inside an available capability.

This distinction allows Vix to reason clearly about environment compatibility.

## Invalid state

Calling an Operation while a Resource is in an unsuitable state may be a Failure or a Fault depending on the contract.

For a public network connection, attempting to send after the remote peer has disconnected may be an ordinary operational Failure.

For an internal object whose documented precondition says that a method may only be called after initialization, violating that precondition may be a Fault.

Again, classification follows the contract boundary.

## Diagnostics and Error

Error and diagnostic presentation are related but distinct.

Error represents the structured semantic description of Failure.

Diagnostics determine how that information is explained to humans.

Vix may provide rich diagnostics containing:

```text
source locations
causal chains
suggestions
Resource names
operation context
system information
formatted messages
```

without changing the underlying Error identity.

This separation allows diagnostics to become significantly better without destabilizing program logic.

## Normative requirements

An Operation MUST expose no more than one terminal completion for a single logical lifetime.

Success MUST mean that the Operation satisfied its contract.

Operational Failure MUST remain semantically distinct from programming Fault.

An Error used to represent operational Failure MUST have programmatic meaning that does not depend solely on human-readable message text.

Adding diagnostic context MUST NOT silently redefine the identity of an underlying Error.

A programming Fault MUST NOT automatically be treated as ordinary operational Failure merely for implementation convenience.

Failure MUST NOT be assumed to imply rollback, absence of side effects, or absence of partial progress unless the Operation contract explicitly provides such a guarantee.

Cancelled completion MUST remain conceptually distinct from operational Failure even when an API chooses a shared implementation representation.

A cancellation request MUST NOT itself be treated as terminal Failure or terminal completion.

Deadline expiration MUST NOT universally imply one specific completion category. The applicable Operation contract MUST define its effect.

A missing required capability MUST remain conceptually distinct from Failure occurring while using an available capability.

Higher-level Error translation SHOULD preserve meaningful causal information when doing so is relevant to diagnosis or program behavior.

Expected operational Failures SHOULD be represented structurally rather than requiring callers to inspect diagnostic strings.

APIs SHOULD make the distinction between expected Failure and programming Fault understandable at important contract boundaries.

Retryability, severity, and recovery policy SHOULD NOT be treated as universal intrinsic properties of Error when they depend on application context.

## Open representation question

The Standard defines cancelled completion as semantically distinct from operational Failure.

It does not yet require one canonical C++ representation for the relationship between:

```text
Result<T>
Error
Cancelled completion
```

Possible representations may include a dedicated completion type, a structured cancellation Error identity, an asynchronous wrapper that distinguishes cancellation from `Result<T>`, or another design.

That choice should be made through the Vix evolution process after the requirements of asynchronous I/O, process execution, networking, storage, and other affected domains have been examined together.

Until that representation is standardized, implementations must preserve the semantic distinction even if their current API encoding is less expressive.

## Design consequence

Completion should answer:

> How did this logical Operation finish?

Failure should answer:

> What operational condition prevented the contract from being satisfied?

Error should answer:

> How can that Failure be identified, explained, and propagated?

Fault should answer:

> What programming contract was broken?

Keeping these questions separate prevents Vix from turning every undesirable condition into the same generic Error path.

It also gives higher-level domains a consistent foundation on which to build precise failure handling, diagnostics, recovery policy, and composition.

The objective is not to make failure complicated.

It is to make the meaning of failure stable enough that the same reasoning continues to work throughout Vix.
