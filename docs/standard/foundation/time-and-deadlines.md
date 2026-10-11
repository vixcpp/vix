# Time and Deadlines

Time appears throughout Vix in networking, asynchronous execution, process control, storage, retries, shutdown, scheduling, and many other domains.

Because these domains often evolve independently, time-related concepts can easily become inconsistent. A timeout may be interpreted as a duration in one API, an absolute deadline in another, a cancellation request somewhere else, and an Error category in another subsystem.

The Vix Standard separates these ideas so that time has one coherent meaning across the platform.

The fundamental concepts are Duration, Clock, Instant, Deadline, and Timeout. They are related, but they are not interchangeable.

## Duration

A Duration represents an amount of elapsed time.

Examples include:

```text
10 milliseconds
2 seconds
5 minutes
```

A Duration does not identify a particular point in time.

It may be used to describe how long something should wait, how much time remains, how frequently something occurs, or how much time has elapsed between two compatible instants.

A Duration should remain independent of civil concepts such as timezone, calendar date, or daylight-saving changes unless a domain explicitly requires those concepts.

For example, a network timeout of five seconds should mean an elapsed interval of approximately five seconds, not “until the wall clock shows a particular civil time”.

## Clock

A Clock defines a time domain from which Instants can be observed.

Different clocks exist because different kinds of time serve different purposes.

A wall clock is intended to represent civil or calendar time. It may be adjusted by the operating system, network time synchronization, administrator action, timezone configuration, or other mechanisms.

A monotonic clock is intended to move consistently forward and is suitable for measuring elapsed time and enforcing operational deadlines.

The Standard does not require one universal Clock for every use.

Instead, the contract must use a Clock appropriate to the semantic purpose.

## Instant

An Instant represents a point within a particular Clock domain.

An Instant only has meaning relative to its Clock.

Instants from unrelated Clock domains must not be compared as though they belonged to one common timeline unless an explicit conversion is defined.

For example, an Instant from a monotonic Clock should not be treated as a Unix timestamp.

Likewise, a wall-clock timestamp should not automatically be used to measure an operation timeout.

This distinction allows Vix to use calendar time where calendar meaning matters and monotonic time where elapsed-time correctness matters.

## Wall-clock time

Wall-clock time is appropriate when software needs to reason about civil time.

Examples include:

```text
displaying the current date
recording a calendar timestamp
scheduling an event for a civil time
formatting logs for people
interpreting a user-provided date
```

Wall-clock time may move forward or backward because of synchronization, configuration changes, timezone rules, or administrative action.

For this reason, wall-clock time should not normally be used to measure the elapsed lifetime of an Operation.

An Operation whose timeout depends directly on wall-clock adjustment may unexpectedly run longer or shorter than intended.

## Monotonic time

Monotonic time is intended for measuring elapsed intervals and enforcing operational time constraints.

A monotonic Clock should not move backward during a process lifetime under ordinary platform behavior.

Operations such as:

```text
network timeout
shutdown grace period
retry budget
timer wait
lock wait
operation deadline
```

should normally use monotonic time where the platform provides an appropriate monotonic Clock.

This keeps their behavior independent from civil-time adjustments.

The exact resolution and precision of a monotonic Clock may vary between platforms. The Standard does not require identical hardware precision everywhere.

It requires the semantic purpose of the Clock to remain appropriate.

## Deadline

A Deadline is a temporal boundary applied to an Operation or another bounded lifetime.

A Deadline answers a question such as:

> Until what point may this work continue under the current contract?

A Deadline is commonly represented as an Instant in a monotonic Clock domain.

For example:

```text
now = monotonic_now()
deadline = now + 5 seconds
```

The resulting Deadline describes a boundary five seconds into the applicable monotonic timeline.

A Deadline is different from a Duration.

`5 seconds` is a Duration.

`monotonic instant D` is a Deadline when the contract says work must respect that boundary.

## Relative timeout and absolute deadline

Public APIs may allow programmers to express time constraints using a relative Duration:

```text
timeout = 5 seconds
```

or an absolute Deadline:

```text
deadline = D
```

These may be different API representations of related intent.

When a relative timeout governs an Operation, an implementation may convert it into an effective Deadline when the Operation begins.

For example:

```text
requested timeout = 5 seconds

Operation starts at T

effective deadline = T + 5 seconds
```

Once an Operation has an effective Deadline, child work should normally reason from that Deadline rather than repeatedly creating fresh timeout Durations.

This is important for composition.

## Timeout

A Timeout is the condition in which an applicable temporal constraint expires before the event required by the contract occurs.

Timeout is not itself one universal terminal outcome.

Depending on the contract, timeout may result in:

```text
Failure(timeout)
Cancelled completion
normal "not ready" result
retry policy activation
Activity lifecycle transition
another domain-defined outcome
```

The Standard therefore does not define Timeout as synonymous with Error.

It also does not define Timeout as synonymous with Cancellation.

Timeout describes what happened with respect to time.

The contract determines what that condition means for the surrounding Operation or Activity.

## Deadline expiration

A Deadline is considered reached according to its Clock and the comparison semantics defined by the applicable contract.

Reaching the Deadline does not mean the Operation has already terminated.

For example:

```text
Operation active
    ↓
Deadline reached
    ↓
timeout policy activates
    ↓
cancellation requested
    ↓
Operation eventually completes
```

The Operation may require additional time to observe cancellation, clean up, wait for underlying system completion, or establish its terminal state.

The moment the Deadline is reached and the moment terminal completion is observed are therefore distinct.

## Deadline and cancellation

Deadline expiration may be implemented by requesting cancellation.

This is a useful and common relationship, but the concepts remain distinct.

A Deadline is a time constraint.

Cancellation is a control request.

Cancelled completion is one possible terminal outcome.

For example:

```text
Deadline
    ↓ expires
Timeout policy
    ↓
Cancellation request
    ↓
Cancelled completion
```

is one valid contract.

Another contract may use:

```text
Deadline
    ↓ expires
underlying work stopped
    ↓
Failure(timeout)
```

The internal cancellation mechanism does not require the public outcome to be classified as Cancelled.

Mechanism and semantic contract must remain separate.

## Deadline races

Deadline expiration may race with normal completion in the same way cancellation can.

Consider:

```text
t0    Operation active
t1    successful completion becomes terminal
t2    deadline observer runs
```

The Operation has already succeeded.

The later observation of the Deadline must not rewrite the terminal outcome.

Another execution may be:

```text
t0    Operation active
t1    Deadline reached
t2    timeout policy takes effect
t3    Operation terminates according to timeout contract
```

The terminal outcome then reflects the timeout policy.

One Operation still has one terminal completion.

## Parent and child deadlines

Composition is one of the most important reasons to distinguish Duration from Deadline.

Consider an HTTP request with a five-second overall limit:

```text
HTTP Request
    DNS
    TCP connection
    TLS handshake
    request write
    response read
```

A naive implementation might give each phase a fresh five-second timeout:

```text
DNS             5 seconds
TCP             5 seconds
TLS             5 seconds
write           5 seconds
read            5 seconds
```

The logical request could then take far longer than the five seconds requested by the caller.

That violates the meaning of the parent time constraint.

Instead, the logical parent may establish:

```text
overall deadline = D
```

and each child Operation uses the remaining time before `D`.

The child must not silently extend the parent's effective Deadline.

## Remaining time

The remaining Duration before a Deadline may be derived from:

```text
deadline - current monotonic instant
```

subject to the applicable Clock semantics.

If the Deadline has already been reached, the remaining time is effectively exhausted.

An implementation should avoid converting a parent Deadline back into a fresh full timeout for each child Operation.

For example:

```text
parent deadline: D

DNS consumes 300 ms
TCP consumes 500 ms
TLS consumes 700 ms
```

The later read phase receives only the budget that remains before `D`.

This allows several layers to compose while preserving the caller's original time constraint.

## Child deadlines

A child Operation may have a stricter local Deadline than its parent.

For example:

```text
parent remaining budget = 5 seconds
DNS local maximum = 1 second
```

The effective child Deadline can be the earlier of the two applicable boundaries.

Conceptually:

```text
effective child deadline
=
min(parent deadline, child-local deadline)
```

The exact implementation may differ, but a child participating in the parent's bounded lifetime must not silently choose a later effective Deadline.

This gives lower-level Operations freedom to impose stricter constraints without violating higher-level limits.

## Independent child lifetime

A child with an explicitly independent lifetime may have a Deadline unrelated to the parent.

For example, an Operation may submit durable work to a broader application-owned service.

Once ownership and lifetime have been transferred, the background work may follow its own Deadline policy.

The important distinction is that the lifetime became independently owned.

A child must not simply ignore the parent Deadline while continuing to depend on parent-owned state.

## Deadline inheritance

Deadline inheritance is semantic rather than necessarily mechanical.

The Standard does not require every function signature to accept a `Deadline` parameter.

An implementation may propagate the effective constraint through an Operation context, coroutine state, provider request, structured object, or another mechanism.

What matters is that composition preserves the applicable time boundary.

An implementation detail that hides the Deadline internally is acceptable.

An implementation that accidentally resets the deadline at every layer is not.

## Multiple time constraints

An Operation may be subject to more than one time constraint.

For example:

```text
overall request deadline
provider-specific connection limit
shutdown grace deadline
user-defined local timeout
```

When several constraints apply to the same lifetime, the contract should define their interaction.

A common and predictable model is that the earliest applicable Deadline becomes effective.

The Standard does not require every domain to expose these constraints identically, but higher-level constraints must not be silently weakened by lower-level ones.

## Delays

A Delay represents waiting for a Duration or until an Instant before allowing some work to continue.

For example:

```text
sleep for 500 ms
retry after 1 second
wait until monotonic instant T
```

A Delay is not the same thing as a Deadline.

A Delay says:

> Do not proceed before this temporal condition is satisfied.

A Deadline says:

> This work must respect this temporal boundary.

The same timer provider may implement both, but the semantic contracts differ.

## Timers

A Timer is an implementation or Resource used to observe time-related conditions.

A Timer may support delays, periodic notifications, deadline observation, or another temporal behavior.

The Standard does not define one universal Timer API.

A Timer is not itself a Duration, Deadline, or Timeout.

It is a mechanism that may help implement those semantics.

This distinction allows different platforms to provide different timer facilities while preserving the same time model.

## Periodic behavior

Periodic behavior is commonly described using a Duration between occurrences.

Examples include:

```text
heartbeat every 10 seconds
maintenance every minute
poll every 500 milliseconds
```

A periodic Activity should not assume that repeatedly sleeping for a fixed Duration necessarily provides the same semantics as scheduling against a stable sequence of target Instants.

Execution delays can otherwise accumulate drift.

The foundational Standard does not prescribe one universal periodic scheduling policy.

Where drift, alignment, or missed intervals matter, the relevant Activity or scheduling contract should define the behavior.

## Time precision

Different platforms provide different clock resolutions and timer precision.

A requested Duration of one nanosecond does not imply that the platform can schedule or observe events with one-nanosecond accuracy.

The Standard distinguishes semantic time values from physical scheduling guarantees.

Providers should document meaningful precision or resolution limits when they affect observable behavior.

APIs should not promise timing precision that their supported providers cannot reliably deliver.

## Deadline precision

A Deadline defines a boundary, but the Operation may not stop at the exact physical instant represented by that boundary.

Scheduling latency, operating-system behavior, provider limitations, non-interruptible work, and cancellation processing may delay terminal completion.

Therefore:

```text
deadline reached
```

does not imply:

```text
Operation physically stopped at exactly that instant
```

The contract may define stronger guarantees for particular providers, but the general Standard does not assume real-time execution.

## Real-time guarantees

The Vix Standard does not, by default, define hard real-time guarantees.

Monotonic clocks, deadlines, and timers provide coherent temporal semantics, but they do not imply that an Operation will be scheduled or cancelled within a strict maximum latency.

A platform or specialized profile may define stronger real-time properties separately.

Such guarantees must be explicit.

They must not be inferred merely from the existence of a Deadline API.

## Wall-clock deadlines

Some domains genuinely require civil-time boundaries.

For example:

```text
run job at 09:00 UTC
certificate valid until a calendar timestamp
subscription expires at a contractual date
```

These are valid uses of wall-clock time.

They should not be confused with elapsed-time Operation deadlines.

A domain using wall-clock boundaries should define what happens when the wall clock changes.

The foundational Standard cannot assume monotonic behavior for civil time.

Where a wall-clock event begins an Operation, the Operation may subsequently use monotonic time for its own elapsed-time constraints.

## Clock conversion

Conversion between Clock domains must not be assumed to be exact or universally available.

For example, converting a wall-clock timestamp into an equivalent monotonic Instant may depend on an observation made at a particular moment and may become inaccurate after wall-clock adjustment.

The Standard therefore discourages unnecessary Clock-domain conversion.

Contracts should express time in the Clock domain that matches the actual semantic need.

## Time and retries

Retries must remain within the logical Operation's applicable time budget unless the contract explicitly defines otherwise.

For example:

```text
overall deadline = D

attempt 1
attempt 2
attempt 3
```

Each attempt must respect the remaining time before `D`.

A retry policy must not automatically reset the full original timeout for every attempt.

This prevents retry mechanisms from silently changing the caller's contract.

A policy that intentionally provides a separate timeout per attempt may do so, but it must remain subordinate to any applicable overall Deadline unless independence is explicit.

## Retry delay

A retry policy may introduce a Delay between attempts.

That Delay consumes time from the logical Operation lifetime when the retry remains part of that Operation.

For example:

```text
overall deadline: 5 seconds
attempt 1: 1 second
backoff: 1 second
attempt 2: ...
```

The backoff does not pause the parent Deadline.

If the overall Deadline is reached during the delay, the retry policy must respond according to the parent Operation contract.

## Time and shutdown

Activities may have shutdown time constraints.

For example:

```text
shutdown requested
    ↓
10-second grace period
    ↓
remaining child work cancelled or forced to stop
```

This grace period belongs to Activity lifecycle policy.

It may interact with child Operation Deadlines but does not automatically replace them.

A child Operation may already have an earlier Deadline and should still respect it.

Likewise, the Activity's shutdown policy may impose an earlier effective bound on remaining child work.

## Time and Resources

Some Resources have time-based validity.

Examples include:

```text
lease
session expiration
temporary credential
connection idle lifetime
cache entry validity
```

These are Resource or Activity lifetime constraints rather than necessarily Operation Deadlines.

The same Duration and Clock concepts may be used, but the semantic object being constrained is different.

The contract should distinguish:

```text
Operation deadline
Resource expiration
Activity lifetime
scheduled civil-time event
```

even when all of them use time internally.

## Expiration

Expiration means that a time-based validity condition has ended.

A Resource may expire.

A lease may expire.

A credential may expire.

An Activity may reach a configured lifetime limit.

Expiration is not automatically identical to Operation Timeout.

The contract determines the consequences of expiration.

For example, Resource expiration may cause later Operations to fail, trigger renewal, stop an Activity, or begin another lifecycle process.

## Time budgets

A time budget is a Duration available to a bounded piece of work.

A budget is often converted into a Deadline so that lower-level composition can preserve the original boundary.

Using an absolute effective Deadline is generally safer for nested work because elapsed time is automatically accounted for.

For example:

```text
budget = 5 seconds
start = now
deadline = start + budget
```

Children then reason against `deadline` rather than independently receiving the original five-second Duration.

The Standard does not require a public `TimeBudget` type, but the semantic distinction can be useful when designing APIs.

## Infinite or absent deadlines

Some Operations have no applicable Deadline.

This does not necessarily mean that they are allowed to run forever regardless of cancellation, Resource shutdown, or parent lifetime.

It only means that no temporal boundary is imposed by that part of the contract.

APIs should distinguish an absent Deadline from a Deadline represented by an arbitrary extremely large Duration.

Using a magic number such as “100 years means no timeout” weakens semantic clarity.

The exact representation of an absent Deadline is an API decision.

## Zero and expired deadlines

An Operation may receive a Deadline that has already been reached when the Operation begins.

The contract must define the resulting behavior.

In many cases, the implementation may avoid starting expensive work and immediately apply the timeout policy.

However, the Standard does not universally require a particular terminal category.

Likewise, a zero Duration used as a timeout should not rely on scheduler timing accidents.

Its meaning should follow the API contract.

## Time overflow and range

Duration arithmetic and Deadline calculation may encounter values outside the representable range of the implementation.

The Standard does not prescribe a single numeric representation for time values.

APIs should define behavior for values that cannot be represented safely.

Silent wraparound that converts a large positive Duration into an unrelated earlier Deadline should be avoided.

Where arithmetic safety affects correctness, overflow must be handled according to the applicable Fault or Failure contract.

## Negative Duration

Some time representations may technically support negative Duration values.

Whether a public API accepts such values depends on its contract.

A negative elapsed difference may be meaningful in some calculations.

A negative timeout supplied to an API expecting a non-negative wait period may instead violate a precondition or be normalized according to the API contract.

The Standard does not assign one universal interpretation to negative Duration values.

The contract must avoid ambiguous accidental behavior.

## Time and Error reporting

When time-related behavior produces an operational Failure, the Error should carry stable programmatic meaning.

Callers should not need to identify a timeout by comparing diagnostic text.

For example:

```text
"request timed out"
```

is useful diagnostic information but should not be the only machine-readable identity when the timeout is part of the public Failure contract.

The domain may expose a timeout-specific Error identity or a broader domain Error with structured cause information.

## Time and diagnostics

Diagnostics may include:

```text
requested timeout
effective deadline
elapsed time
remaining budget
provider timing information
```

when useful.

Such information should help explain behavior without redefining the time semantics.

For example, a diagnostic may say that an Operation exceeded its five-second deadline after spending time in several child Operations.

The Deadline remains the semantic boundary regardless of how the diagnostic is formatted.

## Time and observability

Metrics and traces may record Operation start time, completion time, elapsed Duration, deadline expiration, queueing delay, or cancellation latency.

These measurements are observability features.

They should use appropriate Clock domains and must not alter the Operation contract.

A wall-clock timestamp may be useful for correlating traces across systems, while a monotonic Clock may be used internally to calculate elapsed latency.

Using both for different purposes is valid.

The important requirement is not to confuse their semantics.

## Provider responsibilities

A time Provider may supply clocks, timers, delays, or related facilities.

Different Providers may use different operating-system mechanisms.

The Provider must preserve the semantic contract of the capability it claims to implement.

A provider implementing monotonic time must not expose a clock that can behave like an ordinary adjustable wall clock while claiming monotonic semantics.

A timer Provider must not promise timing precision or cancellation behavior that the underlying platform cannot support.

Provider limitations may be documented through profile or provider-specific guarantees.

## Platform differences

Clock resolution, timer precision, scheduling latency, maximum Duration range, and available civil-time information may vary between platforms.

These differences are expected.

They must not redefine the shared concepts.

A Duration remains a Duration.

A monotonic Clock remains intended for monotonic elapsed-time reasoning.

A Deadline remains a temporal boundary.

A Timeout remains a condition defined relative to a time constraint.

Profiles may describe which time capabilities are guaranteed on a particular class of environment.

## Normative requirements

A Duration MUST represent an amount of time rather than a particular Instant.

An Instant MUST be interpreted within a defined Clock domain.

Implementations MUST NOT compare unrelated Clock-domain Instants as though they were directly interchangeable unless an explicit conversion contract exists.

Operational elapsed-time measurement and Deadlines SHOULD use a monotonic Clock when an appropriate monotonic Clock is available.

Wall-clock adjustment MUST NOT silently redefine a monotonic Operation Deadline.

A Deadline MUST be treated as a temporal constraint and MUST NOT itself be treated as terminal completion.

Deadline expiration MUST NOT universally imply Failure, Cancelled completion, or any other single terminal category.

The applicable contract MUST define the effect of Deadline expiration on observable completion.

A child Operation participating in a parent's bounded lifetime MUST NOT silently extend the parent's effective Deadline unless explicitly permitted by contract.

Internal retries MUST respect the logical Operation's effective Deadline unless the contract explicitly establishes an independent time budget.

Retry delays that remain part of a logical Operation MUST consume that Operation's applicable time budget.

Deadline expiration MUST NOT retroactively replace a terminal completion that was already established.

APIs MUST NOT imply hard real-time guarantees merely by exposing Duration, Timer, or Deadline concepts.

A Provider claiming monotonic-clock capability MUST preserve monotonic semantics according to the guarantees of its profile.

Timeout Failure exposed as part of an API contract SHOULD have stable programmatic identity independent of diagnostic message text.

APIs SHOULD distinguish absence of a Deadline from arbitrary large timeout values.

Time-related contracts SHOULD make the relevant Clock domain understandable where confusion could affect correctness.

## Open representation questions

The Standard does not yet require one canonical C++ representation for:

```text
Duration
Instant
Deadline
optional Deadline
Timeout
Clock
```

Nor does it require every API to expose both relative timeout and absolute Deadline forms.

Those representations should be designed after examining the needs of Async, networking, processes, scheduling, filesystem operations, Activity shutdown, and other domains together.

The Standard also does not yet define one universal mechanism for propagating an effective Deadline through nested Operations.

Possible implementations may use explicit parameters, Operation context, coroutine state, request objects, or another mechanism.

Whatever representation is chosen must preserve the semantics defined here.

## Design consequence

Duration should answer:

> How much time?

Instant should answer:

> At what point in this Clock domain?

Deadline should answer:

> Until when may this bounded work continue?

Timeout should answer:

> Did the relevant time constraint expire before the required condition occurred?

These questions are related, but they are not interchangeable.

Keeping them separate allows Vix to compose time correctly across networking, execution, shutdown, retries, storage, processes, and future domains without turning every temporal condition into the same generic timeout mechanism.

The goal is not to make time APIs complicated.

The goal is to ensure that five seconds means five seconds even when the work passes through several layers of Vix.
