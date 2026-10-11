# Versioning

The Vix Standard has its own evolution and versioning needs.

The Standard describes semantic contracts intended to remain stable across changes in implementation, repository structure, providers, modules, build systems, and release cadence.

For that reason, the version of the Vix Standard must not be treated as identical to the version of a particular Vix implementation.

A Vix implementation may change frequently while continuing to conform to the same Standard version.

Likewise, a future Standard revision may clarify or extend semantics without requiring every internal component of Vix to be rewritten or renumbered at the same time.

Versioning exists to make these relationships explicit.

## Standard version and implementation version

The Vix Standard version identifies a version of the semantic contract.

The Vix implementation version identifies a release of the software that implements some version of that contract.

These are different things.

For example, a relationship may conceptually look like:

```text id="stdimpl1"
Vix Standard 1.0

implemented by:

Vix 3.0
Vix 3.1
Vix 3.2
```

This would mean that several implementation releases continue to satisfy the same Standard contract.

A later implementation might support a newer Standard:

```text id="stdimpl2"
Vix 3.4
    conforms to Vix Standard 1.1
```

The exact numbers above are illustrative.

The important rule is that implementation release cadence must not force unnecessary Standard revisions.

## Why the versions are separate

Implementation releases may include changes such as:

```text id="stdvsep1"
performance improvements
internal refactoring
module restructuring
new Providers
bug fixes
diagnostic improvements
build-system changes
new optional APIs
```

None of these necessarily change the semantic contract.

If every such release created a new Standard version, the Standard would become a mirror of the repository rather than a durable specification.

Conversely, a semantic change may affect several implementation components at once.

Giving the Standard its own version allows that change to be described as one contract change even when implementation work is spread across several releases.

## What a Standard version identifies

A Standard version identifies a coherent set of normative semantic requirements.

This includes the applicable contracts for concepts such as:

```text id="stdver1"
Resources
ownership
Operations
Activities
Completion
Failure
Cancellation
time
execution
Capabilities
Providers
composition
conformance
Profiles
```

A conformance claim against a Standard version means that the implementation satisfies the normative requirements applicable to that claim.

The version should therefore represent semantic meaning rather than documentation publication time alone.

## Editorial changes

Not every change to the Standard requires a new semantic version.

Purely editorial changes may include:

```text id="edit1"
spelling corrections
grammar improvements
formatting
clarification of examples
link repairs
non-normative wording improvements
```

provided that they do not change the meaning of the normative contract.

Such changes may be published without being treated as a new semantic revision of the Standard.

The project may still track document revisions internally through source-control history or publication metadata.

Semantic versioning and document-edit history are different concerns.

## Clarifications

A clarification explains an existing requirement more precisely without intentionally changing what conforming implementations were already required to do.

For example, the Standard might clarify an ambiguous example while leaving the underlying normative rule unchanged.

A clarification should not be used to introduce a new requirement retroactively.

If previously conforming behavior becomes non-conforming because of a change, that change is more than editorial clarification.

It represents semantic evolution and must be versioned accordingly.

## Compatible semantic additions

The Standard may grow by defining new concepts, optional behavior, or additional guarantees without invalidating existing conforming implementations.

Examples may include:

```text id="compatadd1"
a new optional Capability
a new optional Profile
a new informative model
an additional Provider extension mechanism
a new domain specification
```

Such additions may be compatible when existing implementations are not required to change their established behavior.

Compatibility must be judged semantically rather than merely from source-code impact.

An addition that appears syntactically optional may still be incompatible if it changes the meaning of an existing contract.

## Strengthening a requirement

Strengthening an existing normative requirement may make previously conforming implementations non-conforming.

For example, changing a contract from:

```text id="strength1"
an implementation MAY preserve Error causes
```

to:

```text id="strength2"
an implementation MUST preserve Error causes
```

changes the conformance boundary.

That is a semantic change even if no public C++ signature changes.

The Standard must treat such changes deliberately.

Implementation compatibility alone is not sufficient to determine Standard compatibility.

## Weakening a requirement

Weakening a normative guarantee can also be breaking.

Suppose software legitimately relies on a Profile guaranteeing persistent storage.

Changing that guarantee to optional storage would make existing software unable to rely on the same Profile contract.

Even though more implementations might now qualify for the Profile, the contract became weaker for its consumers.

A Standard version must therefore consider both stronger and weaker semantic changes.

## Semantic compatibility

A new Standard revision is semantically compatible with an older one when software relying only on the older normative guarantees can continue to reason correctly under the newer contract.

This does not require every API or implementation detail to remain identical.

The relevant question is whether the meaning that software was allowed to rely on remains valid.

For example, replacing one internal networking Provider with another may be compatible if both preserve the same Capability contract.

Changing cancellation from single terminal completion to allowing two competing terminal notifications would not be compatible, even if the public type names remained unchanged.

## Source compatibility

Source compatibility concerns whether existing C++ source code continues to compile against a newer implementation or API.

This is important, but it is not the same as Standard semantic compatibility.

A release may preserve semantic compatibility while changing a concrete API in a source-incompatible way.

Likewise, source code may continue compiling while a semantic guarantee changes underneath it.

Vix should track these concerns separately.

The Standard defines semantic compatibility.

Concrete source-compatibility policy belongs to the relevant API and implementation release policy.

## Binary compatibility

Binary compatibility concerns whether previously compiled artifacts continue to work with newer binaries or libraries.

This is again separate from Standard conformance.

ABI policy may depend on compiler, platform, library packaging, deployment model, and implementation release strategy.

The Standard does not define a universal Vix ABI policy in this document.

An implementation may provide ABI guarantees independently from its Standard version.

## Behavioral compatibility

Behavioral compatibility concerns observable behavior promised by a contract.

This is the compatibility dimension most directly relevant to the Standard.

For example:

```text id="behcompat1"
borrowed Resource is not stopped by borrower
Operation produces one terminal completion
parent Deadline is not silently reset by child
```

Changing any of these behaviors changes foundational semantics.

A release may therefore be source-compatible and binary-compatible while still being semantically incompatible with the Standard.

## Standard releases

A published Standard revision should identify the semantic contract it represents.

The project may eventually use a numbering scheme such as:

```text id="stdnumber1"
major.minor
```

or another clearly documented system.

The exact numbering scheme is not fixed by this document.

Whatever scheme is chosen should distinguish at least between changes that preserve the previous semantic contract and changes that alter assumptions on which existing conforming software may rely.

The numbering scheme should serve semantic communication rather than imitate implementation release numbering.

## Major semantic changes

A major Standard change is one that intentionally changes an established semantic contract in a way that may invalidate assumptions permitted by an earlier Standard version.

Examples may include:

```text id="major1"
changing ownership authority rules
changing terminal completion semantics
changing Cancellation meaning
weakening an established Profile guarantee
redefining an existing Capability
changing Failure versus Fault classification globally
```

Such changes require deliberate evolution.

They should not enter the Standard accidentally through implementation refactoring.

A major semantic change should normally require a Vix Evolution Proposal describing the motivation, compatibility impact, alternatives, migration path, and affected contracts.

## Compatible Standard evolution

Not all semantic evolution requires a major break.

Compatible evolution may include:

```text id="minorch1"
adding a new optional Capability
adding a new independent Profile
specifying previously unstandardized behavior
adding stronger optional guarantees
introducing a new domain specification
```

provided that existing Standard guarantees remain valid.

Even compatible additions should be reviewed for interactions with existing concepts.

The goal is not merely to avoid changing old text.

It is to preserve the coherence of the programming model.

## Implementation releases

Implementation releases may declare which Standard version they conform to.

For example:

```text id="implrel1"
Vix 3.x
    conforms to Vix Standard 1.0
```

A later implementation release may add support for a newer Standard while retaining compatibility with earlier semantics where possible.

The exact format of this declaration is not yet standardized.

The important requirement is that users should eventually be able to distinguish:

```text id="implrel2"
implementation version
Standard version
Profile conformance
optional extensions
```

without inferring one from another.

## Supporting multiple Standard versions

An implementation may be capable of satisfying more than one Standard version simultaneously.

This is especially possible when a newer Standard is compatible with an older one.

For example:

```text id="multiver1"
Implementation X

conforms to:
    Standard 1.0
    Standard 1.1
```

The implementation need not expose separate runtime modes if one behavior satisfies both contracts.

If two Standard versions contain incompatible requirements, simultaneous conformance may require explicit compatibility modes or may be impossible.

The implementation should not claim multiple versions unless it actually satisfies each contract.

## Default semantic mode

A Vix implementation should have a clearly defined default semantic contract.

If several historical compatibility modes exist, users should not need to guess which semantics are active.

For example, if an implementation supports both a historical behavior and a newer standardized behavior, the selected mode should be determined explicitly through version, profile, configuration, language mode, or another defined mechanism.

The Standard should avoid creating a permanent situation where two conflicting meanings are both presented as the same canonical Vix behavior.

Compatibility mechanisms may preserve older software.

They should not make the current semantic model ambiguous.

## Capability versioning

Capabilities may evolve as the Standard evolves.

A Capability's versioning concern is semantic.

For example, adding a new optional diagnostic field to a Provider may not alter the Capability contract.

Changing the minimum cancellation guarantee may alter it.

Changing the meaning of persistent storage certainly may alter it.

A Provider must satisfy the Capability contract associated with the Standard or extension version it claims.

The Standard does not currently require every Capability to carry an independent numeric version.

Independent Capability versioning should only be introduced if real evolution needs justify the additional complexity.

## Provider versioning

Providers may have their own implementation versions.

For example:

```text id="providerv1"
TLS Provider implementation 4.2
```

may satisfy:

```text id="providerv2"
TLS Capability contract from Vix Standard 1.0
```

Provider implementation version and Capability semantic version are different dimensions.

Updating the Provider does not automatically change the Capability contract.

Likewise, a Provider version number alone does not establish conformance.

Conformance depends on behavior.

## Profile versioning

Profiles are contracts and therefore require versioning when their guarantees change.

Adding an optional Capability may be compatible.

Adding a new mandatory Capability may make the Profile stronger but may also invalidate existing implementations that previously conformed.

Removing a guaranteed Capability weakens the contract relied on by software.

Changing a constraint may also have compatibility consequences.

Profile changes must therefore be assessed semantically.

The project may eventually version Profiles separately or associate Profile revisions with Standard versions.

This document does not yet require one specific representation.

## Profile identity and stability

A named Profile should have stable meaning within the version context in which it is defined.

Software should not encounter a situation where:

```text id="profileunstable1"
Profile "Native"
```

means one incompatible set of guarantees today and a substantially different set tomorrow without any version distinction.

Profile names are useful only when they identify a meaningful contract.

If the contract changes incompatibly, that change needs an explicit version boundary.

## Domain specification versioning

Future Vix domain specifications may include areas such as:

```text id="domainspec1"
Web
Networking
Data
Realtime
Process
Storage
```

These specifications may evolve at different rates from the foundational Standard.

A domain specification must identify the foundational Standard assumptions on which it depends.

A domain specification must not silently redefine the foundational semantics merely to avoid a version change.

The project may choose whether domain specifications share the main Standard version or maintain related independent versions.

That policy is not fixed here.

## Extensions and versioning

Provider-specific, implementation-specific, or experimental extensions may evolve independently.

Software depending on an extension must not assume that the extension is part of the baseline Standard unless it has been standardized.

Extensions should be identifiable so that portable software can distinguish:

```text id="extver1"
Standard guarantee
```

from:

```text id="extver2"
implementation extension
```

If an extension later becomes standardized, its final Standard contract may differ from the experimental version.

The evolution process should define any necessary migration path.

## Experimental semantics

Experimental features should not acquire permanent Standard meaning merely because they ship in one implementation.

An implementation may expose experimental behavior in order to gather experience.

Such behavior should be identified as experimental when callers are not yet allowed to rely on long-term semantic stability.

This gives Vix space to test ideas without forcing the Standard to preserve premature decisions indefinitely.

Once semantics are accepted into the Standard, the stability expectations become stronger.

## Deprecation

The Standard may deprecate a semantic feature or contract before removing or replacing it.

Deprecation communicates that software should stop depending on the affected behavior even though it may remain supported for a transition period.

Deprecation should state:

```text id="depr1"
what is deprecated
why it is deprecated
what replaces it
when relevant, what migration is expected
```

Deprecation must not silently change existing semantics.

While the deprecated contract remains part of a supported Standard version, implementations claiming that version must continue respecting its required behavior unless the version explicitly defines otherwise.

## Removal

Removing an established normative contract can be a breaking semantic change.

Removal should therefore occur through a deliberate version boundary.

The Vix evolution process should document why the old contract can no longer remain part of the Standard and what software depending on it should do.

Historical implementation code may remain available through compatibility layers without remaining part of the canonical Standard.

The existence of compatibility code does not mean the old semantics remain canonical forever.

## Corrections to defective specification

A Standard may contain a genuine defect: contradictory requirements, impossible semantics, or wording that clearly fails to express the accepted design.

Correcting such a defect requires judgment.

The project should not automatically preserve an unusable contract merely because text was published.

At the same time, calling a breaking semantic change a “correction” must not be used to bypass versioning discipline.

If existing implementations or software could reasonably rely on the previous normative interpretation, the compatibility impact must be acknowledged explicitly.

## Evolution through VEPs

Meaningful semantic changes should flow through the Vix Evolution Proposal process.

Conceptually:

```text id="vepversion1"
Research
    ↓
VEP
    ↓
Accepted semantic decision
    ↓
Standard revision
    ↓
Implementation migration
    ↓
Conformance validation
```

This keeps implementation changes from silently redefining the Standard.

A VEP should identify whether its proposal is:

```text id="vepversion2"
compatible clarification
compatible extension
strengthening
weakening
deprecation
breaking semantic change
```

when that classification is relevant.

## Migration

A new Standard version may require implementation or application migration.

Migration guidance should focus on semantic differences rather than merely listing renamed types.

For example, if lifecycle authority changes, migration guidance should explain:

```text id="migrate1"
old ownership assumption
new ownership contract
affected behavior
required application change
```

This gives programmers a way to understand why migration is necessary.

Automated source changes may help with API changes, but they cannot replace explanation of changed semantics.

## Compatibility layers

Vix may provide compatibility layers for historical behavior.

A compatibility layer can be useful for gradual migration.

It must not blur which behavior is canonical under the current Standard.

For example:

```text id="compatlayer1"
current Standard semantics
    canonical

historical compatibility behavior
    explicitly selected
```

is preferable to allowing two incompatible behaviors to appear under one undifferentiated contract.

Compatibility exists to preserve software while transition occurs.

It should not permanently prevent semantic convergence.

## Conformance across versions

Conformance is always evaluated against a specific contract.

An implementation may conform to Standard 1.0 and fail to conform to Standard 2.0.

That does not imply that the implementation is defective relative to 1.0.

It means the conformance target changed.

Likewise, software written against 1.0 should not assume that an environment claiming only 2.0 preserves every removed 1.0 behavior unless backward compatibility is part of the 2.0 contract.

Version identifiers make these boundaries explicit.

## Conformance testing and versions

Conformance tests should identify the Standard and Profile version whose requirements they validate.

A test should not fail an implementation for lacking behavior introduced only in a newer Standard version when the implementation claims only the older version.

Likewise, a newer implementation should not pass conformance merely by running an outdated test suite that omits newer normative requirements.

The conformance suite and Standard version must remain aligned.

## Documentation versions

Documentation describing normative semantics should clearly correspond to a Standard version when multiple versions exist.

Users should be able to determine whether they are reading:

```text id="docversion1"
current Standard
historical Standard
draft next Standard
implementation-specific documentation
```

without ambiguity.

The project may maintain a single current documentation site while archiving previous versions, or use another publishing strategy.

The exact documentation infrastructure is outside the scope of this file.

The requirement is semantic clarity.

## Draft Standard changes

Work under discussion is not automatically part of the current Standard.

A proposal may exist in:

```text id="draftstd1"
research notes
VEP draft
experimental implementation
next-Standard branch
```

without changing the current normative contract.

Only accepted and published Standard changes define the new conformance target.

This distinction allows Vix to experiment openly without making every experiment an immediate compatibility commitment.

## Version negotiation

Some systems may eventually need to negotiate semantic or protocol versions between components.

The foundational Standard does not currently require a general runtime Standard-version negotiation mechanism.

Most conformance relationships are expected to be established through build, deployment, package, Provider, or profile metadata.

Domain protocols may define their own negotiation mechanisms where required.

A universal Vix negotiation protocol should only be introduced if concrete cross-component requirements justify it.

## Standard version exposure

An implementation may expose its supported Standard version for diagnostics or tooling.

For example, tooling may eventually display:

```text id="stdexpose1"
Vix implementation: 3.x
Vix Standard: 1.x
Profile: Native ...
```

The exact API and metadata format are not yet standardized.

Such information should represent actual conformance claims rather than merely copying build-time constants with no behavioral validation.

## Version comparison

A higher numeric version should not be blindly interpreted as universally “better” for every use.

A newer Standard may remove old guarantees, introduce new requirements, or target a changed semantic model.

Software compatibility must be determined from the relevant contract and migration policy.

Tooling may use version ordering as part of compatibility checks, but semantic compatibility rules remain authoritative.

## Normative requirements

The Vix Standard version MUST remain conceptually distinct from Vix implementation release versions.

A Standard version MUST identify a coherent normative semantic contract.

Implementation changes that do not alter Standard semantics MUST NOT require a Standard semantic version change solely because the implementation version changed.

An implementation claiming conformance MUST identify the applicable Standard version when multiple relevant versions exist.

A Provider implementation version MUST NOT be treated as proof of Capability conformance.

A named Profile MUST have stable meaning within its applicable version context.

An incompatible change to an established Profile guarantee MUST NOT be introduced without an explicit version boundary.

Editorial changes MUST NOT be presented as semantic changes when they preserve the existing normative contract.

Conversely, semantic changes MUST NOT be hidden as editorial clarification when they alter what conforming implementations or software may rely on.

A normative change that invalidates previously permitted assumptions MUST be treated as a compatibility-affecting Standard change.

Removing or weakening an established guarantee MUST be evaluated for compatibility even when the resulting implementation becomes simpler.

Strengthening a requirement MUST be evaluated for compatibility when previously conforming implementations may no longer satisfy it.

Experimental behavior MUST NOT automatically be treated as a stable Standard guarantee.

Compatibility layers MUST NOT make the canonical Standard semantics ambiguous.

Domain specifications MUST identify and preserve the foundational Standard semantics on which they depend.

Conformance tests MUST be evaluated against the Standard and Profile version they are intended to validate.

Meaningful semantic evolution SHOULD proceed through the Vix Evolution Proposal process.

Migration guidance SHOULD explain changed semantics rather than only changed API syntax.

Standard versioning SHOULD evolve more slowly than implementation release numbering when the semantic contract remains stable.

## Open questions

The Standard does not yet choose the final public numbering scheme.

A conventional form such as:

```text id="openver1"
Vix Standard 1.0
Vix Standard 1.1
Vix Standard 2.0
```

is a reasonable model, but the exact rules governing major and minor increments should be finalized through the evolution process before the first formal Standard release.

The project also has not yet decided whether Profiles and future domain specifications receive independent version numbers or are always identified through their associated Standard version.

Likewise, no machine-readable format currently exists for declaring:

```text id="openver2"
implementation version
supported Standard versions
Profile versions
Capability extensions
experimental features
```

These mechanisms should be introduced when tooling and distribution requirements make them useful.

What is already fixed at the semantic level is the separation of these version dimensions.

## Design consequence

Versioning should answer several different questions without confusing them.

The implementation version should answer:

> Which Vix software release is this?

The Standard version should answer:

> Which semantic contract does this implementation claim to preserve?

A Profile version should answer:

> Which environment guarantees may software rely on?

A Provider version should answer:

> Which implementation of a Capability is being used?

These numbers may sometimes move together.

They do not mean the same thing.

Keeping them separate allows Vix to refactor aggressively, replace Providers, reorganize modules, and release implementation improvements without constantly destabilizing its programming model.

The Standard should change when Vix changes what it means.

The implementation version should change when Vix changes what it ships.
