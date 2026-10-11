# Capability, Provider, and Execution Conformance Probes

These probes evaluate the semantic boundaries between Capabilities, Providers,
execution, scheduling, readiness, and lifecycle. They do not require a global
registry, service locator, universal Runtime, `ICapability`, `IProvider`, or
`Executor` C++ base type.

## Capability claims and availability

| Probe | Required observation |
|---|---|
| Capability versus API | A coherent ability such as TCP, DNS, TLS, monotonic time, or asynchronous I/O progression is named independently of any particular module, API spelling, or implementation library. Tiny individual operations are not independently promoted to Capabilities. |
| Capability versus Provider | Replacing a POSIX, Asio, OpenSSL, native-platform, or other implementation does not change the claimed baseline semantics. |
| Capability versus Resource | TCP capability enables a connect Operation that may produce a Socket Resource; neither is the other. |
| Capability versus authorization | An available filesystem or networking capability may still yield an authorization Failure for a particular path or endpoint. |
| Profile-guaranteed capability absent | A selected profile claiming a Capability that its environment cannot supply fails conformance/configuration; each subsequent Operation does not rediscover that absence as an ordinary operational Error. |
| Explicitly optional capability absent | The API or configuration surface exposes the absence at the appropriate selection/startup/runtime boundary. It does not silently substitute weaker semantics. |

Capability requirements may be expressed by profile, construction/configuration,
explicit dependency, or a C++ type-level contract. No universal runtime
`has_capability()` or `get_provider()` mechanism is implied.

## Provider conformance, composition, and replacement

| Probe | Required observation |
|---|---|
| One Provider, several Capabilities | A stateful evented networking provider may satisfy DNS, TCP, timer, and asynchronous-I/O progression contracts without collapsing those Capabilities into one universal capability. |
| One Capability, several Providers | Two TLS providers both preserve the baseline TLS semantics claimed by the selected contract. |
| External dependency isolation | A Provider using OpenSSL, Asio, an OS API, or a driver does not expose that dependency as public Vix semantics unless an explicitly provider-specific API chooses to do so. |
| Provider composition | An HTTPS implementation composes DNS, TCP, TLS, and time/deadline facilities while preserving Error translation, Cancellation, Deadline, Resource, and Completion rules at its own boundary. |
| Provider replacement | A replacement preserves every required baseline guarantee. Provider-specific extensions remain explicit and are not silently assumed by portable callers. |
| Fallback | A fallback is selected explicitly or by a documented policy and is allowed only when it satisfies the same requested semantics; it must not weaken security, persistence, cancellation, or deadline guarantees invisibly. |

## Provider readiness and lifecycle

| Probe | Required observation |
|---|---|
| Stateless syscall adapter | No artificial initialization or shutdown lifecycle is imposed. |
| Stateful provider startup | A pool, engine, device, or remote-backed provider distinguishes construction from readiness whenever initialization can fail or take finite time. Its initialization Operation reports `Result` or `Completion` according to whether cancellation is possible. |
| Borrowed provider | A library using an application-owned provider may use only the documented capability and MUST NOT start, stop, drain, reset, or destroy it merely because it retains a pointer or shared ownership handle. |
| Owned provider | The creator or explicit adopter controls initialization and shutdown of the provider and its owned resources. |
| Shutdown admission | The provider contract states whether new work is rejected, accepted during a drain phase, or prohibited as a caller Fault once shutdown begins. Work is never silently lost. |
| Active Operations at shutdown | For every accepted Operation, the provider contract deterministically permits normal completion, draining, a cancellation request, or Failure. Each Operation still establishes exactly one terminal `Completion`. |
| Shutdown reporting | When shutdown has observable finite failure or cancellation semantics, it is itself an Operation returning `Result<void>` or `Completion<void>` as applicable. Destruction remains fallback cleanup, not the only error-reporting path. |
| Borrowed external resources | Provider shutdown affects only resources for which the provider has lifecycle authority. It does not close caller-owned resources merely because they were used by provider work. |

## Execution and scheduling

Execution is the progression of work; scheduling decides when and where it
progresses. The following Capability needs remain distinct unless a specific
provider deliberately satisfies more than one:

| Required work | Capability being probed | Must not be inferred |
|---|---|---|
| Parallel CPU transformation | CPU execution | asynchronous I/O progression or timer support |
| Socket/DNS/TLS progress without occupying a caller thread | asynchronous I/O progression | arbitrary CPU job submission |
| Delay/deadline wait | timer progression | a worker pool or general task priority |
| Foreign or blocking call off the responsiveness-critical path | blocking-work offload | event-loop-safe CPU execution |

| Probe | Required observation |
|---|---|
| Thread migration | An Operation may progress on another thread or mechanism without changing its logical identity, CancellationToken, Deadline, Error/Completion semantics, or exactly-once terminal outcome. |
| Inline versus deferred progression | If either choice affects reentrancy, lock ordering, callback timing, or affinity, the relevant provider/domain contract declares it. Otherwise scheduling choice remains implementation policy. |
| Ordering/serialization | A provider declares ordering, serialization, and affinity only where correctness depends on them. Queue topology, work stealing, default parallelism, fairness, metrics, and priority remain provider policy unless explicitly promised. |
| Coroutine/callback lifetime | Provider machinery retains the necessary continuation and cancellation-registration lifetime until exactly one resume/terminal completion path wins. |
| Cancellation/deadline/shutdown race | A request, Deadline condition, provider shutdown, success, or Failure may race, but produces neither double resume nor double cleanup and exactly one `Completion<T>` outcome. |
| Fire-and-forget work | Detachment establishes an explicit independent lifetime and an error-observation policy. Submission without an awaited value does not authorize silently losing Failure or Fault. |

An Operation is not a thread, coroutine, task object, queue entry, or Provider.
Those are possible mechanisms for progression.

## Error classification

| Situation | Required classification |
|---|---|
| Profile promises unavailable required capability | profile/configuration/conformance failure, not an ordinary per-operation provider Error |
| Optional capability selected but unavailable | explicit availability/selection result at the documented boundary |
| Ready provider cannot perform accepted work | domain-owned `Error` in `Result` or `Completion` after provider-specific details are translated as required |
| Provider not ready | documented readiness precondition, initialization result, or defined rejection; never an inferred successful capability claim |
| Provider shutting down | documented admission/active-operation behavior, distinct from arbitrary provider failure |
| Contract misuse, double completion, or illegal lifecycle authority | Fault, not an ordinary `Error` |

## Minimum Standard representation

The following concepts are Standard semantic contracts, not required universal
C++ runtime types:

| Concept | Representation decision |
|---|---|
| Capability | Semantic contract; optionally expressed by profiles, construction/configuration, explicit dependency, or type-level constraints. |
| Provider | Provider/domain implementation type satisfying one or more declared contracts. |
| Execution | Semantic progression contract. |
| Scheduler | Provider/domain contract for correctness-relevant placement and ordering. |
| Runtime/context | No universal Standard type; a stateful implementation may be a Resource governed by the lifecycle contract. |

This keeps dependency construction explicit and avoids making ordinary Vix code
depend on a global service registry or a historical implementation topology.
