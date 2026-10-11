# Profiles

A Profile defines a coherent set of Capability guarantees, constraints, and semantic expectations for a class of Vix environments.

Profiles exist because not every environment can or should provide the same facilities.

A native desktop system, a WebAssembly host, an embedded target, and a restricted sandbox may all support Vix while exposing very different sets of Capabilities.

The Vix Standard should represent those differences explicitly without redefining the meaning of shared concepts.

A Profile therefore describes what an environment guarantees.

It does not define a new version of Vix semantics.

## Why Profiles exist

Without Profiles, software is often forced to infer platform behavior from operating-system names, architecture macros, build flags, or undocumented assumptions.

For example:

```text
#ifdef __linux__
    // assume processes exist
#endif
```

may appear reasonable but mixes two separate questions:

```text
What platform is this?
```

and:

```text
Does this environment provide process execution?
```

The second question is what the software actually depends on.

A Linux environment running inside a restrictive sandbox may not provide every facility normally associated with Linux.

A WebAssembly runtime may expose Capabilities that another WebAssembly environment does not.

A future Vix runtime may provide a Capability through a portable Provider even when the underlying operating system does not expose it directly.

Profiles allow Vix to reason about guarantees instead of relying on platform stereotypes.

## Profile as a contract

A Profile is a contract between an environment and software targeting that environment.

The Profile states which Capabilities are guaranteed and may also define constraints relevant to those Capabilities.

Conceptually:

```text
Profile
    guarantees Capability A
    guarantees Capability B
    guarantees Capability C
    defines constraints
```

Software targeting that Profile may rely on those guarantees.

An implementation claiming the Profile is responsible for satisfying them.

If a guaranteed Capability cannot be provided, the implementation does not satisfy the Profile under that configuration.

This is a conformance issue rather than an ordinary Operation Failure.

## Profiles are not operating systems

A Profile should not normally be named or defined solely by an operating system.

For example:

```text
Linux Profile
Windows Profile
macOS Profile
```

would risk encoding assumptions about infrastructure rather than describing the actual programming contract.

The same operating system may support several Vix Profiles depending on build configuration, runtime restrictions, deployment environment, or available Providers.

Likewise, several operating systems may conform to the same Profile.

A Profile should therefore be based on semantic guarantees.

Platform-specific implementation details belong to Providers, architecture, and deployment configuration.

## Profiles are not implementations

A Profile does not prescribe how its guarantees are implemented.

Suppose a Profile guarantees:

```text
TCP
DNS
monotonic time
asynchronous I/O
```

One implementation may satisfy those guarantees using one operating-system networking stack and event mechanism.

Another may use entirely different mechanisms.

If both preserve the required semantics, both may conform to the same Profile.

Profiles describe what software may rely on.

They do not prescribe the internal architecture that supplies it.

## Profiles and Capabilities

Capabilities are the primary building blocks of Profiles.

For example, a Profile may guarantee:

```text
filesystem
process execution
TCP
DNS
TLS
monotonic time
timers
CPU execution
asynchronous I/O
```

The existence of these guarantees does not mean that every Operation performed through them succeeds.

A Profile guaranteeing filesystem Capability does not guarantee that every file exists.

A Profile guaranteeing TCP does not guarantee that every remote host accepts connections.

The guarantee means that the environment can provide the Capability according to its contract.

Individual Operations remain subject to normal operational Failure.

## Required and optional Capabilities

A Profile may distinguish between guaranteed and optional Capabilities.

Guaranteed Capabilities are part of the Profile contract.

An implementation claiming that Profile must provide them.

Optional Capabilities may be available in some conforming environments without being required in all of them.

For example:

```text
Profile P

Guaranteed:
    monotonic time
    timers
    filesystem

Optional:
    GPU acceleration
```

Software that requires only the guaranteed set may target Profile P directly.

Software that requires GPU acceleration must additionally require that Capability or target a stronger Profile that guarantees it.

Optionality should remain explicit.

## Profiles and stronger environments

An environment may provide more Capabilities than its Profile guarantees.

For example, a Profile might guarantee:

```text
filesystem
TCP
DNS
time
```

while one implementation additionally provides:

```text
process execution
GPU acceleration
hardware cryptography
```

The additional Capabilities do not invalidate Profile conformance.

The Profile defines the minimum guarantees software may assume.

Software may choose to discover and use additional Capabilities when available.

Doing so means that the software's actual requirements extend beyond the baseline Profile.

## Stronger Profiles

One Profile may be stronger than another.

For example:

```text
Profile A
    guarantees:
        time
        filesystem

Profile B
    guarantees:
        time
        filesystem
        process execution
        native networking
```

An implementation satisfying Profile B may also satisfy Profile A if all requirements and constraints of A are preserved.

This relationship should be explicit rather than inferred merely from profile names.

A stronger Profile adds guarantees.

It must not silently change the meaning of Capabilities inherited from a weaker Profile.

## Profile constraints

Profiles may define more than Capability presence.

They may also define constraints that affect whether software can rely on a Capability in a particular way.

Examples may include:

```text
native threads unavailable
filesystem is sandboxed
process creation unavailable
network listeners prohibited
persistent storage limited
callbacks execute on host event loop
```

Such constraints are useful when they materially affect portable program behavior.

A Profile should not become a dump of every platform detail.

Only constraints that matter to the programming contract belong in the Profile.

## Capability presence versus Capability strength

Two environments may provide the same general Capability while offering different stronger guarantees.

For example:

```text
Profile A
    monotonic timers available

Profile B
    monotonic timers available
    higher minimum timer precision
```

The baseline Capability semantics remain the same.

The stronger Profile may add an additional guarantee.

Software targeting only the baseline should not need to understand the stronger requirement.

Software that depends on the stronger timing guarantee must target the stronger contract.

## Profiles and execution

Execution Capabilities are especially important in Profiles because different environments may provide fundamentally different execution mechanisms.

One Profile may guarantee:

```text
native CPU worker execution
asynchronous I/O
native threads
```

Another may guarantee:

```text
host event-loop execution
asynchronous host I/O
no unrestricted native threads
```

These differences are valid.

The Profile describes the available execution Capabilities and constraints.

It does not redefine Operation, Cancellation, Resource ownership, or Completion semantics.

The same foundational model remains applicable.

## Profiles and WebAssembly

WebAssembly illustrates why Profiles should not be reduced to platform labels.

A WebAssembly environment may run:

```text
in a browser
in a server runtime
in an embedded host
inside an application
```

These environments may expose different networking, filesystem, threading, process, and storage Capabilities.

A single assumption that “WASM means X” would therefore be too weak for a durable Standard.

A future Vix WebAssembly Profile may define a particular set of guarantees.

Additional WebAssembly Profiles may exist if materially different environments require different contracts.

The Profile should describe the guarantees rather than simply repeating the target format name.

## Profiles and native environments

A future native Profile may guarantee Capabilities commonly expected from general-purpose operating systems.

That may include concepts such as:

```text
filesystem
process execution
TCP
UDP
DNS
monotonic time
timers
CPU execution
asynchronous I/O
```

The exact list should not be standardized until Vix has audited the actual requirements of its native Platform.

The important principle is that a native Profile should mean more than:

> This was compiled for a native operating system.

It should state what Vix software can rely on.

## Profiles and restricted environments

An environment may intentionally offer fewer Capabilities.

For example, a sandbox may provide:

```text
monotonic time
limited storage
outbound HTTP
```

while providing no:

```text
process execution
raw TCP listener
arbitrary filesystem access
```

Such an environment can still be a valid Vix target.

A smaller Profile is not inherently inferior or non-conforming.

It represents a different contract.

Software requiring unavailable Capabilities simply cannot target that Profile without adaptation.

## Profile selection

A Profile may be selected at build time, deployment time, startup, or another appropriate stage.

The Standard does not require one universal selection mechanism.

For example:

```text
build target selects Profile
```

may be appropriate for a statically compiled application.

Another system may select or validate its Profile during startup after discovering the host environment.

What matters is that software should know which guarantees it is permitted to rely on.

## Declared Profile

An implementation may declare that it conforms to a particular Profile.

Such a declaration is a conformance claim.

The declaration must only be made when the implementation can satisfy all normative requirements of that Profile.

A build option, configuration value, or string saying:

```text
profile = "native"
```

does not itself establish conformance.

The actual environment must satisfy the corresponding contract.

## Profile verification

An implementation may verify Profile requirements at different stages.

Some requirements may be known statically.

Others may require runtime checks.

For example:

```text
compile time:
    target supports required C++ features

startup:
    required Provider initialized

runtime discovery:
    required host Capability available
```

The Standard does not require every Profile guarantee to be checked dynamically.

It does require that an implementation not claim guarantees it cannot actually provide.

## Failure to satisfy a Profile

If an implementation claims a Profile but cannot provide one of its required Capabilities, it does not currently satisfy that Profile.

For example:

```text
Profile requires process execution.

No conforming process Provider is available.
```

This should not be hidden until some arbitrary process Operation later returns a generic Error.

The system should detect and report the profile mismatch at the earliest practical stage.

That may be during build, configuration, startup, deployment, or runtime resolution depending on when the relevant information becomes available.

## Profile mismatch

Software may require a stronger environment than the selected Profile provides.

For example:

```text
Application requires:
    filesystem
    process execution

Selected Profile guarantees:
    filesystem
```

The application requirements and Profile guarantees do not match.

This is different from a process Operation failing after process execution has already been provided.

The mismatch exists before the Operation can legitimately be attempted under the declared environment contract.

Tooling should eventually be able to diagnose such mismatches directly.

## Profile and application requirements

An application may declare Capability requirements independently of a Profile.

For example:

```text
Application requires:
    TCP
    DNS
    TLS
    monotonic time
```

A Profile is suitable when its guaranteed Capabilities, together with explicitly available extensions, satisfy those requirements.

This relationship makes Profiles useful for portability reasoning.

Instead of asking:

> Does this application support Linux?

Vix should increasingly be able to ask:

> Does this environment satisfy the capabilities this application requires?

That is a more durable question.

## Profile portability

Software that depends only on guarantees shared by several Profiles may be portable across those Profiles.

For example:

```text
Profile A
    filesystem
    time
    TCP

Profile B
    filesystem
    time
    TCP
    process execution
```

Software requiring only filesystem, time, and TCP may potentially run in both.

Software requiring process execution cannot rely on Profile A.

Portability therefore emerges from compatible Capability requirements rather than from assuming all targets behave identically.

## Profiles and optional adaptation

Applications may adapt to optional Capabilities.

For example:

```text
if hardware acceleration is available:
    use accelerated Provider
otherwise:
    use software Provider
```

This is valid when both paths satisfy the application's required semantics.

The application should not silently fall back to behavior that violates its own contract.

Adaptation should preserve required guarantees while taking advantage of stronger environments where possible.

## Profiles and Provider selection

A Profile guarantees semantic Capabilities, not particular Providers.

For example:

```text
Profile guarantees TLS
```

does not necessarily mean:

```text
Profile guarantees OpenSSL
```

One implementation may satisfy TLS through one Provider.

Another may use a different Provider.

If software requires OpenSSL-specific behavior, that is an additional Provider-specific requirement rather than part of the generic Profile unless the Profile intentionally includes it.

## Profiles and external libraries

Profiles should not generally encode dependency on external libraries unless those libraries themselves are intentionally part of the environment contract.

For example, the Standard should prefer:

```text
TLS Capability
```

over:

```text
OpenSSL installed
```

when the software only requires TLS semantics.

This allows implementations to replace external libraries while preserving the same Profile.

## Profiles and Resource limits

Profiles may define important Resource constraints when software must be able to rely on them.

For example, a specialized Profile might define limits around:

```text
memory
threads
concurrent Operations
persistent storage
open Resources
```

The foundational Profile model does not require every Resource limit to be standardized.

Most limits remain environment-specific operational conditions.

A limit belongs in a Profile when it is necessary to define meaningful portability or execution guarantees.

## Profiles and authorization

A Profile guaranteeing a Capability does not mean that every use of it is authorized.

For example:

```text
Profile guarantees filesystem Capability.
```

This does not imply:

```text
every path is readable and writable.
```

Authorization remains separate.

Likewise, a Profile guaranteeing networking does not promise unrestricted access to every address.

Profile guarantees describe available kinds of behavior, not universal permission.

## Profiles and dynamic conditions

Temporary operational conditions do not normally change Profile identity.

For example, a Profile may guarantee networking while the machine is temporarily offline.

The networking Capability still exists.

Individual Operations may fail because connectivity is unavailable.

By contrast, if the environment fundamentally cannot provide the networking Capability required by the Profile, that is a Profile conformance problem.

The distinction is between:

```text
Capability exists but Operation cannot currently succeed
```

and:

```text
Capability required by Profile does not exist
```

## Dynamic Capability loss

Some Providers or host facilities may disappear while the program is running.

A Profile should distinguish between a Capability that is structurally guaranteed and a Resource or operational condition that may become unavailable.

For example, removable storage may disappear even though the environment still supports storage Capability in general.

Dynamic conditions should be modeled through Resource and Operation semantics where possible rather than constantly changing the declared Profile.

If the environment permanently loses a Capability that the Profile itself guarantees, the implementation may no longer satisfy the Profile.

## Profile inheritance

Profiles may be defined in terms of other Profiles when doing so improves clarity.

For example:

```text
Profile B
    includes all guarantees of Profile A
    plus Capability X
    plus constraint Y
```

Inheritance should preserve all inherited semantics.

A derived Profile must not use inheritance merely for naming convenience while weakening guarantees inherited from its parent.

Where Profiles differ significantly, explicit composition may be clearer than deep inheritance hierarchies.

The Standard should avoid unnecessarily complex Profile taxonomies.

## Profile composition

Some environments may be better described through combinations of independent Profile dimensions rather than a single enormous Profile hierarchy.

For example, future work might distinguish:

```text
execution guarantees
storage guarantees
networking guarantees
deployment constraints
```

The Standard does not yet choose between flat Profiles, inheritance, composable Profile features, or another representation.

The semantic requirement is simpler:

software must be able to determine which guarantees it may rely on.

The representation can evolve later.

## Profile names

Profile names should communicate a stable semantic contract.

A name should not be selected solely because it corresponds to a current implementation target.

Names such as:

```text
Core
Native
WASM
```

may eventually be useful, but their meaning must come from documented guarantees rather than from intuition about those words.

A Profile name without a normative Capability definition is only a label.

The Standard should avoid granting strong meaning to a Profile name before its contract is written.

## Baseline Profile

The Vix project may eventually define a minimal baseline Profile.

Such a Profile would represent the smallest environment in which a useful subset of the Vix programming model can operate.

The current Standard does not yet define that set.

Choosing it prematurely could accidentally exclude environments that Vix may want to support later or force foundational concepts to depend on facilities they do not actually require.

A baseline Profile should emerge from implementation experience and concrete portability requirements.

## Native Profile

A future Native Profile may describe guarantees expected from general-purpose native environments.

The current Standard does not yet define its complete Capability set.

Before standardization, Vix should audit which Capabilities are genuinely required by its Platform rather than simply including everything available on current Linux development systems.

The Profile should describe durable guarantees, not reproduce the current machine used to develop Vix.

## WebAssembly Profile

A future WebAssembly Profile may describe a coherent Vix environment built around host-provided capabilities and the constraints of a WebAssembly runtime.

The Standard should not assume in advance that all WebAssembly hosts provide identical filesystem, network, threading, timer, or persistence semantics.

The actual Profile contract should be based on the target environment Vix intends to support.

Additional Profiles may be necessary if materially different WebAssembly hosts cannot satisfy one useful common contract.

## Specialized Profiles

Future domains may define specialized Profiles where stronger guarantees are required.

Examples could include environments optimized for:

```text
embedded execution
deterministic systems
restricted sandboxes
server deployment
edge execution
```

These are possibilities, not currently standardized Profiles.

A specialized Profile should exist because software needs a meaningful set of guarantees, not because every platform deserves its own named category.

## Profile evolution

Profiles may evolve as the Standard evolves.

Adding a new optional Capability may be compatible.

Adding a new required Capability changes what implementations must provide and may therefore require a new Profile version or a new Profile.

Weakening an existing guarantee can break software that legitimately relied on it.

Such changes must go through the Vix evolution process.

Profile evolution should preserve the principle that a declared Profile gives software a stable contract.

## Profile versioning

Profile versions should be related to, but not necessarily identical to, implementation release versions.

For example, a Vix implementation release may continue conforming to the same Profile contract across several releases.

Likewise, a Profile may evolve independently of repository restructuring.

The exact versioning relationship is defined in `versioning.md`.

A Profile conformance claim should identify the applicable version when ambiguity is possible.

## Profile conformance tests

Profiles should eventually have conformance tests for guarantees that can be tested.

For example, a Profile guaranteeing a monotonic Clock should test the required semantics of that Clock.

A Profile guaranteeing process execution should verify that a conforming Provider is actually available.

Tests should focus on guarantees rather than internal mechanisms.

A Profile test should not require a particular operating-system API when several mechanisms can satisfy the same contract.

## Profile diagnostics

When software cannot run under a selected Profile, diagnostics should describe the missing contract clearly.

For example:

```text
Application requires process execution.

Selected profile does not guarantee process execution.
```

is more useful than:

```text
unsupported
```

Likewise:

```text
Selected profile guarantees TLS,
but no conforming TLS Provider could be initialized.
```

identifies a conformance or configuration problem rather than presenting it as an unrelated network Failure.

Clear diagnostics are one of the practical benefits of explicit Profiles.

## Normative requirements

A Profile MUST define semantic guarantees rather than merely identify an operating system or implementation.

An implementation claiming a Profile MUST satisfy every required Capability and normative constraint defined by that Profile.

A Profile MUST NOT redefine the foundational meaning of a Capability in order to accommodate one platform.

A Capability shared by several Profiles MUST preserve its foundational semantics across those Profiles.

A Profile MAY guarantee stronger behavior than the foundational Capability contract when that stronger guarantee is documented.

An implementation MAY provide Capabilities beyond those guaranteed by the Profile it claims.

Optional Capability absence MUST NOT be treated as Profile non-conformance when the Profile explicitly permits that absence.

Required Capability absence MUST be treated as a Profile conformance failure rather than ordinary Operation Failure.

Profile guarantees MUST NOT be inferred solely from operating-system, architecture, runtime, or target names.

A Profile MUST NOT require a specific Provider when the guarantee can be expressed through a provider-independent Capability, unless Provider identity is intentionally part of the contract.

Authorization MUST remain distinct from Profile Capability guarantees.

Temporary operational Failure MUST NOT automatically be interpreted as loss of Profile conformance.

A stronger or derived Profile MUST preserve all guarantees it claims to inherit.

Profile changes that weaken existing normative guarantees MUST go through the Standard evolution process.

Software and tooling SHOULD detect Profile requirement mismatches at the earliest practical stage.

Profile diagnostics SHOULD identify missing Capabilities or unsatisfied guarantees directly.

Profile definitions SHOULD remain small enough to communicate useful environment contracts rather than accumulating unrelated platform details.

## Open questions

The Vix Standard does not yet define the canonical Profile catalog.

Names such as:

```text
Core
Native
WASM
```

are useful working candidates, but their Capability guarantees should not be standardized until Vix has examined the requirements of its existing Platform and intended targets.

The Standard also does not yet decide whether future Profiles should use:

```text
flat named Profiles
Profile inheritance
composable Profile features
Capability requirement expressions
or a combination of these
```

That decision should follow practical use cases.

A future Vix Evolution Proposal should define the first canonical Profiles after the Capability audit is sufficiently mature.

The first Profile specification should include both its guarantees and the reasoning for the boundary it chooses.

## Design consequence

A Profile should answer:

> What can software safely assume about this Vix environment before it begins relying on environment-specific facilities?

It should not answer:

> Which operating system are we compiling for?

The operating system may help determine how guarantees are implemented.

The Profile states what those guarantees actually are.

This allows Vix software to reason in terms of Capabilities instead of accumulating platform checks throughout the codebase.

A program should ultimately be portable because its requirements match the guarantees of another environment, not because someone happened to add another `#ifdef`.

That is the role Profiles play in the Vix Standard.
