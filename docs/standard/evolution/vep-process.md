# Vix Evolution Proposal Process

A Vix Evolution Proposal, or VEP, is the mechanism used to propose meaningful changes to the Vix programming model.

A VEP exists when a decision deserves more than an implementation patch.

It is used when Vix needs to decide what programmers, Providers, Profiles, or implementations should be allowed to rely on over time.

The purpose of the process is not to slow implementation work.

Its purpose is to prevent local implementation decisions from becoming permanent semantic commitments without deliberate review.

A VEP records the problem, the proposed semantic change, the alternatives considered, the compatibility consequences, and the reasoning behind the final decision.

If accepted, the VEP becomes part of the history of why the Standard has its current form.

The Standard itself remains the authoritative description of what Vix means now.

## When a VEP is needed

A VEP should be considered when a proposal changes, introduces, or removes behavior that should remain stable across implementations, domains, or future releases.

Examples include changes to:

```text
Operation semantics
Resource ownership
borrowing rules
Activity lifecycle
Cancellation
Completion
Failure and Fault classification
Deadline behavior
execution guarantees
Capability definitions
Provider requirements
Profiles
conformance rules
Standard versioning
```

A proposal may also deserve a VEP when it introduces a new cross-domain abstraction whose meaning should remain stable throughout Vix.

The important question is not how many files will change.

A one-line semantic change may deserve a VEP.

A refactoring touching thousands of lines may not.

The deciding question is whether the change affects what software or conforming implementations may rely on.

## When a VEP is not needed

Ordinary implementation work normally does not require a VEP when it preserves the existing semantic contract.

Examples include:

```text
internal refactoring
performance optimization
moving code between modules
renaming private types
replacing one conforming Provider with another
changing an internal algorithm
improving diagnostics
fixing a bug so behavior matches the current Standard
```

These changes may still require design review, tests, or architecture documentation.

They simply do not change the Standard.

A VEP should not become a mandatory ceremony for every engineering decision.

Its value comes from being reserved for decisions with semantic significance.

## Research before proposal

A difficult question does not need to become a VEP immediately.

Research should come first when the problem or solution space is not yet understood.

Research may explore:

```text
several competing models
existing behavior across modules
implementation experiments
portability constraints
performance consequences
failure cases
external ecosystem practices
prototype APIs
```

A research document may remain intentionally inconclusive.

It may recommend that no Standard change be made.

It may reveal that the original problem belongs to architecture rather than semantics.

A VEP should normally begin once the problem is sufficiently understood that a concrete semantic proposal can be evaluated.

## Creating a VEP

A new VEP begins as a Draft.

The author should assign a descriptive title and, once the repository process supports it, a stable VEP identifier.

For example:

```text
VEP-0007: Structured Operation Lifetimes
```

The identifier is a reference mechanism.

It does not indicate priority, importance, or acceptance.

A Draft VEP may change substantially during discussion.

Its existence means only that the proposal is being considered.

## Recommended document structure

A VEP should be understandable without requiring the reader to inspect the implementation that motivated it.

The following structure is recommended:

```text
Title
Status
Authors
Created
Target Standard
Summary
Problem
Motivation
Current Behavior
Proposed Semantics
Detailed Design
Interaction with Existing Standard
Alternatives Considered
Compatibility
Migration
Implementation Experience
Conformance Impact
Open Questions
Decision
```

Not every section requires the same level of detail for every proposal.

The document should contain enough information to evaluate the semantic decision.

## Title

The title should identify the concept or change being proposed.

Good titles describe the semantic topic rather than an implementation task.

For example:

```text
Define Parent-to-Child Cancellation Propagation
```

is more useful than:

```text
Refactor async module
```

The first describes a decision that could belong to the Standard.

The second describes implementation work.

## Status

Every VEP should have an explicit status.

The initial recommended states are:

```text
Draft
Under Review
Accepted
Rejected
Withdrawn
Superseded
```

Additional states may be introduced later if the process requires them.

The status must make clear whether the proposal currently affects the Standard.

Only an Accepted proposal that has been incorporated into the relevant Standard revision becomes part of the normative contract.

## Draft

A Draft is actively being developed.

The proposal may contain unresolved questions and may change substantially.

Implementation experiments may occur during this stage.

A Draft is not normative.

Software must not rely on Draft semantics as part of the current Standard.

A Draft may return to research if its underlying problem turns out to be insufficiently understood.

## Under Review

A proposal moves to Under Review when it is considered sufficiently complete for a semantic decision.

At this stage, reviewers should be able to understand:

```text
the problem
the proposed contract
the affected existing semantics
the alternatives
the compatibility consequences
the expected implementation impact
```

Under Review does not mean the proposal is likely to be accepted.

It means the proposal is mature enough to evaluate seriously.

Substantial new discoveries may return it to Draft.

## Accepted

An Accepted VEP represents an approved semantic decision.

Acceptance does not mean that every implementation change has already been completed.

It means Vix has decided what the resulting semantic contract should be.

After acceptance, the relevant Standard text should be updated.

Implementation migration and conformance work then bring the codebase into agreement with the accepted semantics.

The VEP records the reasoning.

The Standard records the resulting meaning.

## Rejected

A Rejected VEP was considered and intentionally not adopted.

The VEP should retain enough reasoning to explain why the proposal was rejected.

This is useful historical information.

A future proposal may revisit the subject if new evidence or requirements appear.

A rejected proposal has no normative effect.

Experimental code based on it must not be treated as Standard behavior merely because the experiment still exists.

## Withdrawn

A proposal is Withdrawn when its author or responsible maintainers decide not to continue pursuing it.

This may happen because the problem disappeared, another approach became preferable, or the proposal was premature.

A Withdrawn VEP has no normative effect.

It may remain in the repository as historical context.

## Superseded

A VEP becomes Superseded when another proposal replaces its decision or design.

The newer VEP should identify the proposal it supersedes.

The older VEP should point to the replacement.

Superseded proposals remain useful because they preserve the history of the design.

The applicable Standard version determines which semantics are current.

## Summary

The Summary should describe the proposal in a small number of clear paragraphs.

It should explain what semantic change is being proposed without requiring the reader to understand all details immediately.

The Summary should not substitute for the Problem or Detailed Design sections.

It exists to help a reader determine quickly what the proposal concerns.

## Problem

The Problem section should explain what is wrong, missing, ambiguous, fragmented, or unnecessarily difficult in the current model.

The problem should be described before the solution.

For example, a strong problem statement might explain that several subsystems interpret cancellation differently, making composed lifetime behavior unpredictable.

A weak problem statement would simply say:

```text
We need CancellationToken.
```

The first identifies a semantic problem.

The second begins with a representation before proving the need for it.

The VEP process should prefer problem-first reasoning.

## Motivation

The Motivation section explains why the problem is important enough to change or extend the Standard.

Useful evidence may include:

```text
multiple domains needing the same concept
real implementation inconsistencies
unsafe lifetime behavior
portability problems
ambiguous user-facing semantics
repeated compatibility issues
difficulty writing conformance tests
```

The motivation should distinguish foundational need from local convenience.

A proposal should not become foundational simply because one module would become easier to implement.

## Current Behavior

When changing existing semantics, the VEP should explain the current situation.

This may include differences between modules or Providers.

For example:

```text
Subsystem A treats timeout as Failure.
Subsystem B treats timeout as cancellation.
Subsystem C resets timeout for every retry attempt.
```

The goal is not to shame historical implementation.

The goal is to establish what is actually being changed.

If no standardized behavior currently exists, the VEP should say so.

## Proposed Semantics

The Proposed Semantics section is the heart of the VEP.

It should describe what programmers and conforming implementations would be allowed or required to rely on if the proposal is accepted.

The proposal should use the terminology of the Vix Standard wherever possible.

For example, it should distinguish Resource ownership from shared memory representation, or Cancellation Request from Cancelled Completion, rather than inventing local language unnecessarily.

The proposal should focus first on meaning.

Concrete APIs may be discussed separately.

## Detailed Design

The Detailed Design explains the proposal deeply enough to expose difficult cases.

Depending on the subject, this may cover:

```text
lifetime relationships
state transitions
race behavior
Failure behavior
Cancellation
Deadlines
Resource ownership
Capability requirements
Provider interaction
platform differences
composition behavior
```

The purpose is to test whether the proposed semantics remain coherent outside the simplest example.

A proposal that only works in the happy path is not yet ready for Standard acceptance.

## Semantic model before API shape

When a proposal introduces a new concept, it should normally define the semantic model before fixing one concrete C++ representation.

For example, a VEP about cancellation should first define:

```text
who can request cancellation
what the request means
when cancellation becomes terminal
how it propagates
how it races with Success
```

before deciding whether the API should use:

```text
CancellationToken
std::stop_token
custom state object
coroutine context
another representation
```

Representation matters.

It should follow the semantic decision rather than replace it.

## Interaction with the existing Standard

A VEP must consider the concepts it interacts with.

For example, a proposal about asynchronous scopes may affect:

```text
Operations
Activities
Resources
ownership
Cancellation
Deadlines
execution
composition
```

The VEP should explain whether existing semantics are preserved, extended, specialized, or changed.

This prevents new abstractions from accidentally creating duplicate meanings.

A proposal should reuse established terminology where it genuinely applies.

## Alternatives considered

A meaningful proposal should discuss serious alternatives.

The purpose is not to create an artificially long list.

The section should record approaches that could reasonably solve the same problem and explain why they were not selected.

Alternatives may include:

```text
keep current behavior
solve only in architecture
use a narrower domain-specific rule
use an existing C++ facility
introduce a different abstraction
defer standardization
```

The rejected alternatives often become important historical context later.

Without them, future maintainers may repeat the same design debate without knowing why the earlier choice was made.

## Do nothing

For foundational proposals, “do nothing” is often a valid alternative and should be considered honestly.

Standardization itself has cost.

It creates compatibility obligations.

A proposal should therefore explain why leaving the issue unspecified or implementation-defined is not sufficient when that is a realistic option.

Sometimes doing nothing is the correct decision.

The Standard should not grow merely because a concept can be standardized.

## Compatibility

The Compatibility section explains what existing assumptions are affected.

Compatibility may include several dimensions:

```text
semantic compatibility
source compatibility
binary compatibility
Provider compatibility
Profile compatibility
application migration
```

Not every VEP affects all of them.

Semantic compatibility is especially important.

A change may require no source edits while still changing the behavior existing software was permitted to rely on.

That is still a compatibility issue.

## Standard version impact

The VEP should identify whether the proposal appears to be:

```text
editorial clarification
compatible semantic clarification
compatible extension
new optional capability
new Profile
strengthening of an existing requirement
weakening of an existing guarantee
deprecation
breaking semantic change
```

This classification may evolve during review.

The purpose is to make versioning consequences visible before acceptance.

## Profile impact

A proposal affecting Capabilities should consider whether existing Profiles are affected.

For example, adding a new Capability to the Standard does not necessarily mean every existing Profile must guarantee it.

Adding a mandatory Capability to an existing Profile may cause currently conforming implementations to stop conforming.

The VEP should state this explicitly.

Profile contracts must not change accidentally as a side effect of another proposal.

## Provider impact

If a proposal changes a Capability contract, existing Providers may need modification.

The VEP should consider whether:

```text
all existing Providers already satisfy the new semantics
some Providers need migration
some Providers can no longer conform
a stronger Provider-specific extension is being standardized
```

This helps separate conceptual correctness from implementation cost without ignoring either.

## Migration

If existing Vix software or implementations need to change, the VEP should describe the migration path.

Migration should explain semantic differences.

For example:

```text
Before:
    component receiving shared execution object stops it during shutdown.

After:
    externally supplied execution object is borrowed and remains owned by caller.
```

This is more useful than simply saying:

```text
Remove executor_->stop().
```

The implementation edit is temporary knowledge.

The ownership change is the durable reason.

## Compatibility layers

A VEP may propose a temporary compatibility layer.

The proposal should explain:

```text
what historical behavior is preserved
how users opt into or out of it
which behavior is canonical
whether the compatibility layer is expected to disappear
```

Compatibility should support migration.

It should not permanently make conflicting semantics equally canonical.

## Implementation experience

Implementation experience is valuable evidence for many proposals.

A VEP may reference:

```text
prototype implementations
existing experimental code
migration of one subsystem
performance measurements
stress tests
sanitizer results
portability experiments
```

Implementation experience can demonstrate that the semantics are practical.

However, the proposal must still distinguish:

```text
the semantic contract
```

from:

```text
the implementation used to test it
```

The experiment is evidence.

It is not automatically the required future architecture.

## Implementation before acceptance

It is valid to implement a proposal experimentally before acceptance.

This may be necessary to understand the design.

Such code should be treated as experimental where practical.

Reviewers should remain free to reject the semantics even if the implementation works.

The sunk cost of a prototype must not become a reason to standardize a weak design.

## Implementation after acceptance

Once a VEP is accepted, implementation work should move the codebase toward conformance with the accepted semantics.

This may include:

```text
removing historical abstractions
changing ownership relationships
introducing new Providers
updating public APIs
adding compatibility layers
migrating modules
```

Implementation work should reference the accepted semantic decision where useful.

This creates a traceable relationship between code changes and Standard evolution.

## Conformance impact

A VEP should identify how the new semantics can be validated.

The proposal does not need to contain every final test.

It should explain what observable behavior distinguishes a conforming implementation from a non-conforming one.

For example:

```text
Given a borrowed execution Resource,
stopping the borrower must not stop the borrowed Resource.
```

is testable semantic behavior.

By contrast:

```text
Class X must contain field Y.
```

is normally an architectural assertion rather than conformance behavior.

## Tests should follow semantics

When implementation details change, conformance tests should continue validating the contract.

A VEP should avoid defining tests in a way that permanently freezes an experimental implementation structure.

Useful tests often focus on boundaries:

```text
races
shutdown
ownership transfer
missing Capabilities
Deadline exhaustion
child lifetime
Error propagation
```

These are where semantic bugs frequently appear.

## Open questions

A VEP may retain open questions while it is Draft.

Before acceptance, questions that affect the core normative contract should normally be resolved.

Questions about implementation details may remain open when the semantics do not depend on them.

For example, a VEP may establish the semantics of Cancelled Completion while intentionally leaving the exact C++ representation for a later proposal.

That is valid if the accepted contract remains implementable and coherent.

## Splitting proposals

A VEP should be split when it tries to decide several largely independent semantic questions at once.

Large proposals are sometimes necessary because concepts genuinely depend on one another.

However, a proposal that simultaneously redesigns:

```text
Cancellation
Error representation
scheduler API
Profile system
networking API
```

may become impossible to evaluate clearly.

The author should look for the smallest boundary that still produces a coherent decision.

This helps the Standard evolve incrementally.

## Combining proposals

The opposite problem also exists.

Splitting one semantic decision into many tiny proposals can hide important interactions.

For example, Cancellation and child lifetime may need to be considered together if each rule depends on the other.

Proposal boundaries should follow semantic decisions rather than file boundaries or implementation tasks.

## Review

Review should focus primarily on the contract being created.

Reviewers should ask questions such as:

```text
Is the problem real and shared?
Does this belong in the Standard?
Is the terminology consistent?
Does the proposal preserve existing invariants?
What happens under cancellation races?
What happens during shutdown?
Who owns each Resource?
How do Deadlines compose?
What are the Failure semantics?
Can different Providers implement this?
What happens on constrained Profiles?
Is the compatibility cost justified?
```

The review should not become merely a code review of one prototype.

## Review against concrete cases

Abstract semantic models should be tested mentally and, where useful, experimentally against real cases.

A proposal may be examined through scenarios involving:

```text
HTTP requests
WebSocket Activities
process execution
filesystem Operations
database work
timers
background jobs
shared execution infrastructure
```

A foundational abstraction that only makes sense in the subsystem that introduced it may not yet be foundational.

Concrete examples help reveal this.

## Cross-domain review

Changes to foundational concepts should receive review from more than one affected domain when practical.

For example, a new cancellation model should not be evaluated only against networking if it is also expected to govern processes, timers, filesystem work, and CPU Operations.

Cross-domain review reduces the risk of standardizing a local assumption as a universal rule.

## Decision criteria

A proposal should not be accepted merely because it is implementable.

Acceptance should consider whether the proposal:

```text
solves a real semantic problem
improves coherence
can be explained clearly
composes with existing concepts
supports intended platforms
preserves reasonable implementation freedom
has acceptable compatibility cost
can be validated
```

No single criterion automatically decides the outcome.

The decision should reflect the long-term programming model of Vix.

## Decision record

When a VEP reaches a final decision, the decision should be recorded in the document.

For an Accepted proposal, the record should identify the accepted semantic direction and any important conditions or deferred questions.

For a Rejected proposal, it should summarize the principal reason for rejection.

The decision record should be concise enough that future readers can understand what was decided without reconstructing the entire discussion history.

## Acceptance authority

The exact governance authority responsible for accepting VEPs is not fixed by this document.

While Vix remains under its current project governance, acceptance may be made by the person or group responsible for the Standard.

As governance grows, that authority may move to a broader maintainer group, technical committee, foundation, or another structure.

What matters for the process is that acceptance authority is explicit.

Repository merge permission alone should not implicitly determine Standard semantics.

## Consensus

Consensus is valuable for foundational decisions, but the Standard does not currently define a formal consensus algorithm.

Not every reviewer must necessarily prefer the accepted design.

The goal is to ensure that substantial objections are understood and addressed rather than ignored.

A future governance specification may define stronger procedural requirements.

The technical VEP format should remain usable under different governance models.

## VEP updates during review

A Draft or Under Review VEP may change in response to feedback.

Significant changes should be visible.

If the proposal changes so substantially that earlier review no longer applies, it may return to Draft before another review period.

The process should favor clarity over pretending that a proposal remained unchanged simply to preserve momentum.

## Accepted VEP modification

An Accepted VEP is primarily a historical record.

Substantial new semantic changes should normally be introduced through a new VEP rather than rewriting the old proposal as though the original decision had always been different.

Minor editorial fixes may still be made.

If the decision itself changes, the historical relationship should remain visible through supersession or another explicit mechanism.

## Standard integration

Acceptance alone does not replace the Standard text.

The semantic decision must be integrated into the relevant normative documents.

For example:

```text
VEP about ownership
    ↓
resources-and-ownership.md updated

VEP about cancellation
    ↓
cancellation.md updated

VEP defining Profile
    ↓
profiles.md updated
```

The integration should preserve consistency across related sections.

A VEP should not leave contradictory rules scattered throughout the Standard.

## Standard review after integration

After integrating an accepted VEP, the affected Standard sections should be reviewed as a whole.

This helps catch contradictions introduced by editing one document in isolation.

For example, changing Cancellation semantics may require reviewing:

```text
operations.md
completion-and-failure.md
cancellation.md
composition.md
conformance.md
```

Semantic concepts often cross document boundaries.

The published Standard should read as one coherent model.

## Implementation tracking

Implementation work may be tracked independently from the VEP itself.

An accepted proposal may require multiple issues or commits across several modules.

The VEP should not become a detailed task tracker.

It may reference implementation work for traceability.

The important distinction remains:

```text
VEP
    semantic decision

implementation tracking
    engineering work required to realize it
```

## Partial implementation

A VEP may be accepted before every Vix implementation or Provider supports it.

During migration, some implementation branches may conform to the new contract while others still follow historical behavior.

This transitional state must not be confused with full conformance.

A release claiming the new Standard must satisfy the requirements associated with that claim.

Implementation progress and Standard acceptance are distinct milestones.

## Rejection after experimentation

A proposal may be rejected even after substantial experimental implementation work.

This is acceptable.

Experimentation exists to learn.

If the experiment reveals that the semantics are too complex, too restrictive, or based on a wrong abstraction, rejecting the proposal is a successful outcome of the process.

The Standard should not preserve a weak idea merely because implementation effort has already been spent.

## Deferred proposals

A proposal may be deferred without being rejected when the idea appears valuable but Vix lacks enough evidence to make a stable decision.

In the current minimal status model, such a proposal may return to Draft or be Withdrawn with an explanation.

A dedicated `Deferred` status may be introduced later if repeated usage justifies it.

Deferral should make clear what evidence or prerequisite is missing.

## Relationship with implementation bugs

Not every disagreement between implementation and desired behavior requires a VEP.

If the current Standard already defines the intended behavior, the implementation should normally be fixed to conform.

A VEP is needed when the project is reconsidering the contract itself.

This distinction prevents the evolution process from becoming a way to legitimize every implementation defect by changing the Standard afterward.

## Relationship with architecture decisions

Architecture decisions may be documented separately.

For example:

```text
Standard:
    borrowed execution Resources must not be stopped by borrowers.

Architecture:
    Web uses Async io_context as its execution facility.

Implementation:
    remove RuntimeExecutor member from WebSocket Session.
```

Only the first statement necessarily belongs in the Standard.

The VEP process should maintain this separation.

A good architecture may change later while continuing to satisfy the same semantic decision.

## Relationship with API design

A VEP may include API sketches when they help demonstrate feasibility.

An API sketch should not automatically become normative syntax unless the proposal explicitly standardizes that API.

The project may choose to separate:

```text
semantic VEP
```

from:

```text
API proposal or Reference update
```

when doing so improves clarity.

This is especially useful for concepts where several C++ representations can preserve the same semantics.

## Security considerations

A proposal should discuss security consequences when the semantic change affects trust, authorization, Resource isolation, memory safety, process boundaries, networking, cryptography, credentials, or other security-relevant behavior.

The VEP process does not require a security section for changes with no meaningful security impact.

When security assumptions change, they should not remain implicit.

A semantic guarantee that affects security becomes part of the contract programmers may rely on.

## Performance considerations

Performance may influence Standard design, but performance alone does not determine semantic correctness.

A VEP should discuss meaningful performance consequences when the proposed contract would require substantial overhead or prevent important implementation strategies.

The Standard should preserve implementation freedom where possible.

At the same time, semantic guarantees should not be weakened merely to preserve one historical optimization without examining alternatives.

## Portability considerations

A foundational VEP should consider whether the proposed semantics can be implemented across the environments Vix intends to support.

This does not mean every Profile must provide every Capability.

It means shared semantics should not accidentally assume one operating system, one threading model, one event mechanism, or one vendor library.

If a proposal genuinely requires a Capability unavailable in some Profiles, that requirement should be explicit.

## Documentation quality

A VEP should be written so that someone who did not participate in the original discussion can understand the decision later.

The document should prefer clear paragraphs over unexplained slogans or lists of implementation names.

Examples are useful when they expose semantics.

Terminology should match the Standard.

A VEP is part of the long-term technical memory of the project.

It should remain useful after the original implementation has changed.

## Recommended lifecycle

The complete lifecycle can be summarized as:

```text
Observation
    ↓
Research
    ↓
Draft VEP
    ↓
Under Review
    ↓
Decision
    ├── Accepted
    ├── Rejected
    └── Withdrawn
            ↓
Accepted VEP
    ↓
Standard integration
    ↓
Architecture and implementation migration
    ↓
Conformance tests
    ↓
Release claiming the new Standard
```

A later accepted proposal may supersede an earlier one.

At every stage, the status of the semantic decision should remain distinguishable from the state of its implementation.

## Minimal VEP template

A future VEP repository may begin with a template similar to:

```markdown
# VEP-XXXX: Title

Status: Draft
Authors:
Created:
Target Standard:

## Summary

## Problem

## Motivation

## Current Behavior

## Proposed Semantics

## Detailed Design

## Interaction with Existing Standard

## Alternatives Considered

## Compatibility

## Migration

## Implementation Experience

## Conformance Impact

## Open Questions

## Decision
```

The template exists to ensure important questions are considered.

It should not encourage authors to fill sections with meaningless text merely to satisfy a format.

Sections that do not apply may say so briefly.

## Normative requirements

A VEP MUST be used when a proposed change materially alters established Vix Standard semantics unless the change is handled through another explicitly defined Standard evolution mechanism.

A VEP MUST remain non-normative until it has been accepted and incorporated into the applicable Standard revision.

A VEP proposing a semantic change MUST describe the problem the change is intended to solve.

A VEP MUST distinguish the proposed semantic contract from implementation details where that distinction is relevant.

A VEP changing existing behavior MUST consider semantic compatibility.

A VEP affecting foundational concepts MUST examine its interaction with related Standard concepts.

A VEP affecting Capability or Profile guarantees MUST describe the relevant conformance impact.

A VEP MUST NOT gain normative authority merely because an experimental implementation has been merged.

An Accepted VEP MUST be integrated into the relevant Standard documents before the new semantics are treated as part of a published Standard revision.

A Rejected, Withdrawn, Draft, or Under Review VEP MUST NOT be treated as current Standard semantics.

Substantial changes to an already Accepted semantic decision SHOULD be introduced through a new VEP or explicit superseding proposal rather than rewriting history.

VEP review SHOULD consider portability, lifetime, Failure, Cancellation, Deadline, ownership, Capability, Provider, and composition consequences when those concepts are affected.

VEPs SHOULD discuss meaningful alternatives rather than presenting the selected design as though no other approach existed.

VEPs SHOULD describe migration when existing software or implementations are affected.

VEPs SHOULD identify observable behavior that can be used for conformance validation.

VEPs SHOULD standardize the smallest semantic concept necessary to solve the demonstrated problem.

## Open governance questions

This process intentionally leaves several governance details unresolved.

The Vix project still needs to define:

```text
the permanent location of VEP documents
the assignment of VEP numbers
who may move a proposal to Under Review
who has final acceptance authority
whether a formal review period exists
whether consensus is required
how accepted VEPs are collected into Standard releases
```

These are governance questions rather than unresolved semantic concepts.

They can evolve as the Vix community and institutional structure evolve.

The technical purpose of the VEP process remains stable even if the people responsible for decisions change.

## Design consequence

A VEP should answer:

> Why should Vix change what programmers are allowed to rely on?

The Standard should then answer:

> What may programmers rely on now?

Architecture should answer:

> How does Vix implement that contract?

Conformance should answer:

> Does the implementation actually keep the promise?

Keeping these questions in separate artifacts gives Vix a history without forcing history into every programmer's daily experience.

It allows experimentation without immediately creating compatibility obligations.

It allows implementation to change without silently changing semantics.

Most importantly, it gives Vix a way to evolve deliberately while keeping the programming model coherent over many years.
