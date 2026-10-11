# Conformance

Conformance describes whether an implementation, Provider, profile, or other declared Vix environment satisfies the semantic contracts it claims to support.

The Vix Standard defines behavior and meaning rather than repository structure.

For that reason, conformance is not determined by whether an implementation uses a particular module, library, class hierarchy, runtime, scheduler, operating-system API, or internal architecture.

Two implementations may be structurally very different and still conform to the same Vix semantics.

Likewise, two implementations may expose similar APIs while differing in behavior in ways that make only one of them conforming.

Conformance is therefore primarily about observable semantic guarantees.

## What conformance means

An implementation conforms to a part of the Vix Standard when the behavior it exposes satisfies the normative requirements of that part.

For example, an Operation implementation that claims conformance must preserve the rules associated with finite logical work, terminal completion, cancellation, deadlines, Resource lifetime, and Failure classification where those rules apply.

A Provider claiming a Capability must preserve the Capability semantics it claims to provide.

A profile claiming a set of guaranteed Capabilities must actually make those Capabilities available under the conditions defined by the profile.

Conformance should answer:

> Can software rely on the semantic contract being claimed?

The answer must not depend on undocumented implementation assumptions.

## Semantic conformance

Semantic conformance means that the foundational concepts defined by the Standard retain their required meaning.

Examples include:

```text
Resource ownership
borrowing
Operation completion
Activity termination
Cancellation
Deadline semantics
Failure and Fault distinction
Capability requirements
Provider guarantees
```

An implementation is not semantically conforming merely because it contains types with names such as `Operation`, `Result`, `Executor`, or `Provider`.

The behavior must satisfy the contract.

For example, an asynchronous abstraction that reports Success and later exposes another terminal Failure for the same logical Operation violates the single-completion rule regardless of its type names.

A component that stops an externally owned execution Resource merely because it holds a `shared_ptr` violates the ownership model regardless of memory safety.

Semantic conformance is concerned with these observable relationships.

## Focused conformance probes

- The [Resource and Activity Lifecycle Conformance Probes](resource-lifecycle.md)
  evaluate ownership, borrowing, asynchronous resource use, close/destruction,
  Activity shutdown, and lifecycle races.
- The [Capability, Provider, and Execution Conformance Probes](capability-provider-execution.md)
  evaluate capability claims, provider replacement/readiness/lifecycle,
  execution progression, scheduling boundaries, and shutdown races.
- The [Composition Conformance Probes](composition.md) evaluate parent/child
  lifetimes, sequential and concurrent policy, cancellation and Deadline
  propagation, retry/fallback, cleanup, and Activity shutdown composition.
- The [Standard Foundation Integration Review](foundation-integration.md)
  cross-checks the concrete Standard vocabulary and the full Error, time,
  cancellation, lifecycle, provider, execution, and composition model against
  representative production scenarios.
- The [Allocation and No-Exception Policy](allocation-and-no-exceptions.md)
  defines the cross-Standard distinction between fatal process heap exhaustion
  and recoverable Provider resource exhaustion.

## Conformance is not API identity

The Standard does not require every conforming implementation to expose identical C++ syntax unless a separate API specification requires it.

For example, one conforming implementation might represent an asynchronous Operation through a coroutine type while another uses a future-like representation.

If both preserve the same required semantics, both may conform at the semantic level.

Concrete API compatibility is a separate concern.

A future Vix Reference specification may standardize exact public types, functions, signatures, names, and source compatibility.

This document does not make semantic conformance depend on one physical representation unless the Standard explicitly says otherwise.

## Conformance is not architectural identity

An implementation does not need to reproduce the internal architecture of another Vix implementation in order to conform.

One implementation may use an event loop.

Another may use operating-system completion queues.

Another may use cooperative execution.

Another may use synchronous execution for some Capabilities.

If all required observable semantics are preserved, those architectural differences are permitted.

The Standard intentionally allows implementation freedom.

Architecture explains how Vix is built.

Conformance determines whether the result satisfies the required contract.

## Normative requirements

The words `MUST`, `MUST NOT`, `SHOULD`, `SHOULD NOT`, and `MAY` describe different strengths of requirement throughout the Standard.

`MUST` and `MUST NOT` express requirements necessary for conformance to the applicable contract.

An implementation that violates such a requirement does not conform to that requirement.

`SHOULD` and `SHOULD NOT` express strong recommendations that are expected to be followed unless there is a justified reason not to do so.

A conforming implementation may differ from a `SHOULD` requirement when the alternative remains consistent with the rest of the Standard and the deviation is understood.

`MAY` describes behavior that is permitted but not required.

Informative explanations, examples, diagrams, and design discussions help interpret the Standard but do not independently create conformance requirements unless they restate a normative rule.

## Claimed conformance

Conformance is always relative to what an implementation claims to support.

An implementation does not need to provide every possible Vix Capability in order to conform to the foundational Standard.

For example, an environment without process execution may still conform to Vix semantics for the Capabilities it does provide.

The important requirement is that the implementation must not claim process execution Capability if it cannot satisfy the applicable process-execution contract.

Similarly, an implementation may support only a defined profile rather than every Capability available on a full native system.

The conformance claim should make that boundary understandable.

## Partial implementations

A Vix implementation may be incomplete while still conforming to the portions it supports.

For example:

```text
Implementation
    supports
        monotonic time
        filesystem
        TCP

    does not support
        process execution
```

This does not automatically make the implementation non-conforming.

The result depends on the profile or Capability claims being made.

If the implementation claims only the supported Capabilities, it may conform to those contracts.

If it claims a profile that guarantees process execution, then the missing Capability violates that profile claim.

Conformance must therefore distinguish unsupported functionality from incorrectly implemented functionality.

## Capability conformance

A Provider conforms to a Capability when it preserves the normative semantics of that Capability.

The mechanism used is irrelevant unless the mechanism affects observable guarantees.

For example:

```text
monotonic time Capability
```

may be implemented differently on different operating systems.

Each Provider must still preserve the required monotonic behavior.

Likewise, two TLS Providers may use different libraries internally while satisfying the same baseline TLS Capability.

Provider conformance depends on semantics, not implementation lineage.

## Provider extensions

A Provider may offer behavior beyond the baseline Capability.

Examples include:

```text
higher timing precision
additional metrics
stronger cancellation guarantees
hardware acceleration
additional algorithms
provider-specific diagnostics
```

These extensions are permitted.

They do not automatically become part of the baseline Standard.

Software that depends on such an extension is depending on a stronger or Provider-specific contract.

The Provider must continue satisfying the baseline Capability while offering the extension unless the API explicitly selects a different contract.

## Profile conformance

A profile defines a set of guaranteed Capabilities and constraints.

An implementation conforms to a profile when it provides all Capabilities and guarantees required by that profile and respects its stated constraints.

Profile conformance is stronger than merely implementing several unrelated Capabilities.

For example:

```text
Profile P guarantees
    Capability A
    Capability B
    Capability C
```

An implementation claiming Profile P must provide A, B, and C according to their required semantics.

It must not claim Profile P while silently omitting C.

Profiles are defined separately in `profiles.md`.

## Missing profile guarantees

When a profile guarantees a Capability and an implementation claiming that profile cannot provide it, the problem is a conformance problem.

It should not normally be presented as though an ordinary user Operation happened to fail.

For example:

```text
Profile guarantees monotonic timers

implementation provides no monotonic timer facility
```

is not equivalent to:

```text
Timer Capability available

individual timer Operation fails
```

The first means the claimed environment does not satisfy its profile.

The second is an operational outcome inside a Capability that exists.

This distinction is important for diagnostics, deployment, and testing.

## Configuration and conformance

Configuration may determine whether some conformance claim can be satisfied.

For example, an implementation may contain support for a Capability but require a Provider to be configured before that Capability becomes available.

If a selected profile requires the Capability and configuration leaves no valid Provider available, the running system does not satisfy that profile under that configuration.

This should be detected at the earliest practical boundary.

Depending on the system, that may be:

```text
build time
configuration time
startup
deployment
runtime capability resolution
```

The Standard does not require one universal detection stage.

It does require that missing guaranteed functionality not be misrepresented as an ordinary unrelated Failure.

## Runtime capability discovery

Some Capability availability cannot be known until runtime.

For example, hardware or host features may need to be discovered dynamically.

A profile may define whether such a Capability is guaranteed or optional.

If a Capability is guaranteed by the active profile, runtime discovery must not simply make that guarantee disappear without affecting the conformance status of the environment.

If the Capability is optional, its absence may be a valid property of that environment.

Profile guarantees and runtime discovery must therefore remain consistent.

## Operation conformance

An Operation implementation conforms when it preserves the applicable Operation contract and the foundational Operation semantics.

Depending on the Operation, this includes requirements such as:

```text
finite logical lifetime
single terminal completion
defined Success
structured operational Failure
Resource lifetime safety
Cancellation semantics
Deadline semantics
side-effect contract
ownership transfer
```

Not every Operation uses every feature.

An Operation without cancellation support does not violate cancellation conformance merely because it cannot be cancelled, provided it does not claim that capability and its contract is clear.

Conformance is based on the behavior actually promised.

## Activity conformance

An Activity implementation conforms when its continuing behavior and lifecycle satisfy the applicable Activity contract.

This includes preserving distinctions such as:

```text
Activity lifetime
Events
shutdown request
terminal termination
child Operation lifetime
borrowed Resource authority
```

An Activity must not claim termination while continuing normal Activity behavior for the same lifetime.

Likewise, a borrowed Resource must not be terminated merely because the Activity is shutting down.

The same foundational ownership and lifecycle rules apply across domains.

## Resource conformance

A Resource contract conforms when it preserves valid lifetime, ownership, borrowing, and lifecycle authority according to the Standard.

An implementation violates Resource semantics when access is silently treated as ownership or when borrowed Resources are invalidated without authority.

The exact C++ representation is irrelevant.

For example, both raw references and reference-counted handles can participate in valid borrowing models.

Conversely, both can participate in invalid ones.

Conformance follows semantic authority, not pointer type.

## Composition conformance

A composed abstraction must preserve the semantics of the contracts it combines.

For example, a higher-level Operation must not silently reset a parent Deadline for each child Operation.

A retry mechanism must not produce multiple terminal completions for one logical Operation.

A parent must not complete while dependent child work continues using destroyed parent state.

Fallback must not silently weaken required guarantees.

Composition conformance therefore verifies not only individual components but also their relationships.

A set of individually conforming components can still be combined in a non-conforming way.

## Cross-domain conformance

Different Vix domains may define additional specifications.

For example, future specifications may exist for networking, Web, data, realtime systems, or other areas.

Those specifications may introduce domain-specific contracts.

They must still preserve the foundational Standard.

A domain specification may specialize a concept.

It must not silently redefine it.

For example, a Web specification may define exactly how HTTP timeout is exposed to callers.

It must still respect the foundational distinction between Deadline, timeout condition, cancellation request, and terminal completion.

## Stronger guarantees

An implementation may provide stronger guarantees than the Standard requires.

For example, a baseline Capability may allow best-effort cancellation while one Provider guarantees prompt operating-system cancellation under specific conditions.

Stronger guarantees are permitted.

They should be documented clearly.

A stronger guarantee must not contradict the baseline semantics.

Implementations should avoid presenting optional stronger behavior as though all conforming Providers necessarily offer it.

Otherwise software may accidentally become dependent on one implementation.

## Weaker behavior

An implementation may not claim conformance while providing behavior weaker than a normative guarantee.

For example, if a contract requires one terminal completion, an implementation cannot document that duplicate terminal callbacks are “possible under race conditions” and still claim conformance to that Operation contract.

Likewise, if a profile guarantees persistent storage, an implementation cannot replace it with volatile memory and continue claiming the same profile.

Documentation cannot redefine a normative requirement locally.

The implementation must either satisfy the contract, claim a weaker profile or Capability, or evolve the Standard through the proper process.

## Undefined behavior

The Standard should minimize cases where ordinary valid usage leads to undefined semantic behavior.

Some low-level C++ contracts may still contain preconditions whose violation results in programming Fault or language-level undefined behavior.

Such cases must not be used to avoid specifying behavior that legitimate callers need to rely on.

If callers are allowed to perform an action, the observable outcome should be governed by a contract.

If an action is forbidden, that precondition should be understandable.

Conformance should not depend on undocumented assumptions.

## Faults and conformance

A programming Fault occurring because the caller violated a documented precondition does not necessarily mean the implementation itself is non-conforming.

By contrast, an implementation reaching a broken invariant during valid use may indicate a conformance defect.

For example:

```text
caller uses Resource after contractually forbidden destruction
```

may be caller Fault.

But:

```text
valid concurrent cancellation produces two terminal completions
```

is an implementation violation of the Standard.

The contract boundary determines responsibility.

## Conformance tests

Conformance tests should verify observable semantic guarantees.

They should avoid testing internal structure unless that structure is itself standardized.

For example, a test for borrowed lifecycle semantics should verify that stopping a borrower does not stop the externally owned Resource.

It should not require the Resource to be stored in a particular class member or managed by a particular pointer type.

Likewise, an Operation completion test should verify single terminal completion under races rather than depending on the implementation's internal state machine.

This keeps tests aligned with the Standard rather than freezing architecture accidentally.

## Positive and negative tests

Conformance testing should include both valid behavior and prohibited behavior where practical.

A positive test may establish that cancellation eventually produces one coherent terminal outcome.

A negative or stress test may attempt to expose duplicate completion during a cancellation race.

A Resource test may verify correct borrowed use and also verify that a component does not terminate a broader-scope Resource during its own shutdown.

Concurrency and lifetime rules are particularly important to test under adversarial ordering.

## Deterministic testing

Some semantics involving time, cancellation, concurrency, or scheduling are difficult to test reliably using real elapsed time.

Conforming test infrastructure may use deterministic clocks, controlled Providers, simulated completion order, or other techniques.

The purpose is to test the semantic contract rather than the incidental timing behavior of one machine.

The Standard does not require one testing framework.

It encourages tests that can distinguish real contract violations from nondeterministic scheduling noise.

## Conformance suites

A future Vix conformance suite may organize tests by Standard section and profile.

For example:

```text
foundation
    resources
    operations
    activities
    completion
    cancellation
    time

execution
    capabilities
    providers
    composition

profiles
    profile-specific guarantees
```

Such organization is informative at this stage.

The Standard does not yet define an official suite format or certification process.

The important point is that conformance tests should be traceable to normative requirements.

## Conformance evidence

Test results are one form of conformance evidence.

Other evidence may include formal reasoning, platform documentation, Provider guarantees, static checks, or implementation review.

No single technique is required for every semantic rule.

The strength of evidence should match the importance and difficulty of the guarantee.

For example, a simple pure-function contract may be easy to test directly.

A concurrency guarantee may require repeated stress testing, sanitizer runs, deterministic simulation, or careful implementation review.

## Self-declared conformance

An implementation may declare which Standard version, profiles, Capabilities, and extensions it supports.

Such declarations should be precise enough to be meaningful.

For example, a declaration should distinguish between:

```text
supports Vix Standard foundational semantics
```

and:

```text
conforms to Native Profile X
```

and:

```text
provides optional Provider-specific extension Y
```

The exact metadata format is not yet standardized.

The semantic distinction should nevertheless remain clear.

## Conformance and versioning

Conformance claims should identify the applicable Standard version when version differences matter.

An implementation conforming to one version of the Standard may not automatically conform to a later version if normative requirements have changed.

Backward-compatible additions may allow one implementation to conform to several versions.

Breaking semantic changes require explicit version handling.

The versioning model is defined separately in `versioning.md`.

## Extensions

Implementations may provide extensions beyond the Standard.

Extensions are expected and useful.

An extension must not silently change the behavior of standardized concepts in a way that breaks the claimed baseline conformance.

For example, a Provider may offer additional scheduling controls.

It must still preserve the underlying execution semantics required by the Standard.

Extensions should be identifiable when portable software needs to distinguish them from baseline functionality.

## Implementation-defined behavior

Some areas may intentionally allow several valid behaviors.

The Standard may require the implementation to document which behavior it chooses.

This is implementation-defined behavior.

For example, a Provider may be allowed to choose between inline and deferred callback execution when both are permitted, while requiring that the chosen contract be documented.

Implementation-defined behavior is not the same as unspecified chaos.

The caller must still be able to reason from the declared contract.

## Unspecified behavior

In some cases, the Standard may permit several outcomes without requiring the implementation to document which occurs in each instance.

Such freedom should be used carefully, especially around lifetime, cancellation, Failure, and ownership.

The Standard should avoid unspecified behavior where the distinction would materially affect correctness.

Implementation freedom should not come at the cost of making safe composition impossible.

## Conformance diagnostics

When a system cannot satisfy a required profile or Capability, diagnostics should explain the semantic mismatch where practical.

For example:

```text
Native profile requires process execution,
but no conforming process Provider is available.
```

is more useful than allowing an unrelated user Operation to fail later with a generic message.

Conformance diagnostics should help distinguish:

```text
unsupported Capability
misconfiguration
Provider initialization Failure
ordinary Operation Failure
programming Fault
```

These conditions require different responses from the programmer.

## Conformance and portability

Conformance does not mean every Vix program runs unchanged in every environment.

Programs may require Capabilities that some profiles do not provide.

Portability means that requirements and guarantees are explicit enough to determine where the program can run and how shared concepts behave there.

For example, software requiring process execution may not be compatible with a profile that does not provide processes.

That does not mean the profile violates Vix semantics.

It means the software's requirements and the profile's guarantees do not match.

The Capability and profile models exist to make this distinction visible.

## Conformance and compatibility

Conformance and compatibility are related but different.

An implementation may conform semantically while using an API that is source-incompatible with another implementation.

Two versions may also expose compatible APIs while differing semantically in a way that affects conformance.

The Vix project may define separate compatibility policies for source compatibility, binary compatibility, package compatibility, or protocol compatibility.

Those policies should not be confused with semantic conformance to the Standard.

## Normative requirements

An implementation claiming conformance to a Standard requirement MUST satisfy the normative semantics of that requirement.

Conformance MUST be determined by observable contract behavior rather than repository structure or implementation technology unless the Standard explicitly requires a particular representation.

An implementation MUST NOT claim a Capability that it cannot satisfy according to the applicable Capability contract.

An implementation claiming a profile MUST provide all Capabilities and guarantees required by that profile.

A Capability guaranteed by a claimed profile MUST NOT silently disappear and be treated as an ordinary Operation Failure.

Provider-specific extensions MUST NOT weaken the baseline semantics of the Capability to which conformance is claimed.

A stronger implementation guarantee MAY extend baseline behavior provided that it does not contradict the Standard.

A composed abstraction MUST preserve the foundational semantics applicable to its child Operations, Activities, Resources, Capabilities, and Providers.

Domain-specific specifications MUST NOT silently redefine foundational Standard concepts.

Conformance tests SHOULD verify observable semantic requirements rather than internal implementation structure.

Conformance claims SHOULD identify the relevant Standard version and profile where applicable.

Unsupported functionality SHOULD be distinguished from incorrectly implemented functionality.

Implementations SHOULD detect unsatisfied profile requirements at the earliest practical stage.

Diagnostics SHOULD distinguish conformance or configuration problems from ordinary operational Failure when that distinction is known.

Extensions SHOULD be identifiable when portable software may need to determine whether behavior is baseline or implementation-specific.

## Open questions

The Standard does not yet define an official certification authority, compatibility badge, test-suite format, or machine-readable conformance manifest.

It also does not yet define exactly how an implementation should publish claims such as:

```text
Standard version
supported profiles
available Capabilities
Provider extensions
implementation-defined behavior
```

Those mechanisms should be introduced only when they become useful for real Vix implementations, packages, tooling, and deployment environments.

A future Vix Evolution Proposal may define an official conformance manifest and automated test-suite structure.

The semantic meaning of conformance should remain independent from whatever metadata format is eventually chosen.

## Design consequence

Conformance should answer:

> What may software safely rely on in this Vix environment?

That question is more important than whether two implementations look alike internally.

A conforming Vix implementation may use different modules, different libraries, different operating-system facilities, different schedulers, and different internal architecture.

What it may not do is change the meaning of the contracts it claims to implement.

This allows Vix to evolve aggressively underneath while remaining stable above.

The Standard defines the promise.

Conformance determines whether that promise is being kept.
