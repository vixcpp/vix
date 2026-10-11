# Standard Evolution

The Vix Standard is expected to evolve.

New domains will appear, existing abstractions will be tested under more demanding workloads, portability requirements will expand, and some early assumptions will eventually prove incomplete.

Evolution is therefore necessary.

What must remain controlled is how semantic change enters the Standard.

The Standard should not change because one implementation was refactored, because a module was renamed, because one Provider changed, or because a convenient local behavior appeared in a subsystem.

A change becomes part of the Standard only when Vix deliberately decides that the programming model itself should change.

## Why evolution needs a process

The implementation of Vix can change quickly.

The Standard should change more carefully.

This difference is intentional.

An implementation can experiment with new execution mechanisms, new Resource types, alternative error representations, different scheduling policies, or new ways of exposing asynchronous work.

Those experiments are valuable because they reveal what the Standard may eventually need.

They do not automatically redefine Vix semantics.

Without a deliberate evolution process, implementation decisions can slowly become accidental standards.

A type created for one subsystem may begin to appear everywhere. A lifecycle assumption may spread across modules. A Provider-specific behavior may become something callers unknowingly rely on.

By the time the inconsistency is noticed, software may already depend on it.

The evolution process exists to interrupt that path.

Before a semantic decision becomes part of Vix, it should be possible to explain what problem it solves, what alternatives were considered, what existing contracts it affects, and why the new model belongs in the shared Standard rather than only in one implementation.

## Research comes before standardization

Not every unresolved problem needs an immediate normative answer.

Some problems require experimentation.

For example, Vix may need to explore several representations for cancellation before deciding whether a canonical C++ cancellation type belongs in the Standard.

It may need to compare different models for structured concurrency before defining one.

It may need experience across networking, process execution, filesystem work, and storage before deciding whether a particular Capability should be foundational.

This work belongs first in research.

Research may include:

```text id="evo1"
implementation experiments
design notes
benchmarks
domain audits
failure analysis
prototypes
comparisons
questions that remain unresolved
```

Research is allowed to be incomplete.

It is allowed to investigate competing models.

It is allowed to conclude that the Standard should not change.

The purpose of research is understanding, not immediate standardization.

## Research is not normative

Research documents do not define what conforming Vix implementations must do.

A prototype may explore behavior that contradicts the current Standard.

A research note may propose several mutually incompatible alternatives.

An experimental branch may intentionally test semantics that are not yet accepted.

This is valid as long as the experimental status is clear.

Software must not be expected to rely on research material as though it were a published Standard contract.

The boundary between current semantics and investigation should remain visible.

## From research to proposal

When research indicates that a shared semantic decision is needed, the next step is a Vix Evolution Proposal.

A Vix Evolution Proposal, or VEP, is the document in which the change is argued.

The VEP should explain the problem before prescribing the answer.

It should identify the current behavior or semantic gap, show why the problem matters beyond one isolated implementation, and describe the consequences of leaving the issue unresolved.

A VEP may propose:

```text id="evo2"
a new foundational concept
a change to an existing semantic contract
a new Capability
a new Profile
a new domain-level rule
a deprecation
a compatibility boundary
a canonical representation of existing semantics
```

Not every implementation improvement requires a VEP.

The process is for decisions whose meaning needs to remain stable beyond the implementation that introduced them.

## What belongs in a VEP

A VEP should make the decision understandable independently from the final code.

The exact structure may vary, but a serious proposal should normally explain:

```text id="evo3"
the problem
the motivation
current behavior
proposed semantics
alternatives considered
compatibility impact
interaction with existing Standard concepts
migration consequences
open questions
implementation experience, when available
```

A VEP is where disagreement and alternatives belong.

The Standard should normally record the accepted semantics, not reproduce the full design debate.

This separation allows the Standard to remain readable while preserving the reasoning that led to it.

## VEP and Standard have different purposes

A VEP answers questions such as:

> Why should Vix change?

> What alternatives were considered?

> Why was this model selected?

> What compatibility cost does it introduce?

The Standard answers a different question:

> What does Vix mean now?

Once a VEP is accepted and its semantics are incorporated into the Standard, programmers should not need to read the entire historical debate merely to understand the current contract.

The VEP remains valuable for maintainers, future evolution, and anyone who needs to understand why the decision exists.

## Architecture is separate again

The Standard does not determine every implementation detail required to realize an accepted VEP.

After a semantic decision is accepted, architecture work may determine:

```text id="evo4"
which module owns the implementation
which Providers are affected
which dependencies must move
which compatibility layer is required
which internal types should be replaced
```

These decisions belong to implementation architecture unless they expose semantics that software must rely on.

The distinction should remain:

```text id="evo5"
VEP
    why and what should change

Standard
    accepted semantic meaning

Architecture
    how Vix implements that meaning

Reference
    exact public APIs
```

Mixing these layers makes future evolution harder.

## Implementation does not precede acceptance automatically

A working implementation is useful evidence.

It is not automatic approval.

Vix may experiment with a new abstraction in the codebase before standardizing it.

That implementation may reveal problems that cause the proposal to change or be abandoned.

Likewise, an implementation may temporarily use a representation that satisfies accepted semantics without that representation becoming permanently standardized.

The existence of code answers:

> Can this design be implemented?

It does not necessarily answer:

> Should every future Vix implementation preserve this exact representation?

## Accepted semantics

When a VEP is accepted, the relevant semantic decision should be incorporated into the appropriate Standard documents.

The Standard should contain the final contract in the terminology and structure of the rest of the specification.

It should not merely link to the VEP and require the reader to reconstruct the normative rule from historical discussion.

For example, if a VEP changes Resource ownership semantics, the accepted rule belongs in the Resource and ownership specification.

If it adds a new Capability, the Capability and conformance documents should reflect that addition.

The VEP remains the rationale.

The Standard becomes the authority.

## Implementation migration

After semantics are accepted, the existing implementation may need migration.

That migration should be evaluated against the new Standard rather than against whichever subsystem first implemented the proposal.

Several modules may need to change.

Old compatibility code may need to remain temporarily.

Tests may need to be rewritten around observable semantics rather than internal structure.

This stage is where the Standard becomes concrete engineering work.

Conceptually:

```text id="evo6"
accepted semantics
    ↓
architecture plan
    ↓
implementation migration
    ↓
conformance validation
```

Implementation completion is important, but it is downstream from the semantic decision.

## Conformance closes the loop

A semantic change is not useful if the implementation claims the new behavior without validating it.

Conformance tests should therefore be updated when normative requirements change.

If a VEP establishes that borrowed execution Resources must not be stopped by borrowers, tests should exercise that behavior.

If a new deadline rule says child Operations must respect the parent's effective Deadline, tests should verify the composed behavior.

The test should validate the contract.

It should not merely verify that a particular class or file was changed.

This makes conformance testing the final link between the Standard and real behavior.

## The evolution path

The general path for semantic evolution is:

```text id="evo7"
Problem or observation
    ↓
Research
    ↓
Vix Evolution Proposal
    ↓
Review and decision
    ↓
Accepted semantic model
    ↓
Standard update
    ↓
Architecture and implementation migration
    ↓
Conformance validation
```

Not every idea reaches the end.

Research may conclude that no change is necessary.

A VEP may be rejected.

A proposal may return to research because its consequences are not understood well enough.

An accepted proposal may require implementation experience before a representation is standardized.

The process should allow uncertainty rather than forcing premature decisions.

## Small changes

Not every Standard edit needs a full VEP.

Editorial corrections, spelling fixes, formatting improvements, and clarifications that do not change normative meaning may be handled directly.

Small additions that merely restate an already accepted semantic decision may also not require a new proposal.

The important question is whether the change alters what implementations or software may rely on.

If it does, the change should receive deliberate semantic review.

The project should prefer a short justified proposal over silently introducing a meaningful semantic change because the diff happens to be small.

## Clarification versus semantic change

A clarification makes existing meaning easier to understand.

A semantic change alters that meaning.

The distinction must be handled carefully.

Suppose the existing Standard allows two reasonable interpretations and a new edit chooses one while making the other non-conforming.

That is not merely editorial clarification for implementations that reasonably followed the discarded interpretation.

The evolution process should acknowledge the compatibility impact.

The word “clarification” must not be used to hide a breaking decision.

## When a VEP is appropriate

A VEP is generally appropriate when a proposal changes or establishes behavior that should remain stable across multiple Vix domains or implementations.

Examples include:

```text id="evo8"
changing Operation completion semantics
standardizing a cancellation representation
introducing structured concurrency
defining a canonical Profile
changing ownership rules
adding a foundational Capability
altering Error propagation requirements
standardizing execution guarantees
```

A VEP may also be useful when a local implementation problem reveals a broader semantic inconsistency.

The fact that the problem was discovered in one module does not mean the solution belongs only to that module.

## When a VEP is probably unnecessary

Changes that normally do not require Standard evolution include:

```text id="evo9"
renaming an internal class
moving files between modules
replacing one conforming Provider with another
changing a private scheduling algorithm
improving performance without changing guarantees
adding internal diagnostics
fixing an implementation bug so behavior matches the existing Standard
```

These changes may still require architecture review or normal engineering discussion.

They do not require Standard evolution when the semantic contract stays unchanged.

## Domain-specific evolution

Not every semantic decision belongs in the foundational Standard.

A Web-specific rule may belong in a future Web specification.

A database transaction rule may belong in a Data specification.

A protocol-specific behavior may belong in the corresponding domain specification.

The same evolution discipline should still apply.

A domain proposal should identify which foundational Standard concepts it builds upon and must not redefine them silently.

If a domain repeatedly needs a concept that is missing from the foundation, that may be evidence that the foundational Standard should evolve.

## Foundational promotion

Sometimes a concept begins in one domain and later proves useful across several domains.

For example, a lifetime mechanism first introduced for networking may later prove necessary for processes, storage, and background work.

At that point, Vix may consider promoting the concept into the foundational Standard.

Promotion should not happen simply because the first implementation exists.

The evolution process should ask whether the concept has stable meaning outside its original domain.

A foundational concept creates a long-term commitment.

That commitment should be justified by repeated semantic need.

## Avoiding premature universality

The Standard should resist turning every recurring implementation pattern into a universal concept.

Two modules using similar classes do not necessarily prove the existence of a shared semantic abstraction.

A useful question is:

> Would inconsistent meanings of this concept across Vix make the programming model harder to understand or compose?

If the answer is yes, standardization may be justified.

If the answer is merely that code could be deduplicated, the issue may belong to architecture instead.

The Standard exists to unify meaning, not all implementation.

## Evidence for evolution

Strong proposals should be supported by evidence appropriate to the problem.

Evidence may include:

```text id="evo10"
multiple affected domains
existing inconsistencies
implementation experiments
failure cases
portability constraints
conformance difficulty
performance consequences
user-facing complexity
real API usage
```

Not every proposal requires benchmarks or several prototypes.

The evidence should be sufficient to understand the tradeoff being standardized.

Foundational changes deserve stronger evidence than local optional extensions because their long-term compatibility cost is greater.

## Compatibility analysis

A VEP that changes existing semantics should explain what currently conforming behavior becomes invalid or different.

Compatibility analysis should consider more than source code.

It may include:

```text id="evo11"
semantic compatibility
source compatibility
binary compatibility
Profile compatibility
Provider compatibility
migration of existing applications
migration of existing Vix modules
```

Not every dimension applies to every proposal.

The proposal should identify the ones that matter.

A change that requires no source edits can still be semantically breaking.

## Migration strategy

When a semantic change affects existing software, the proposal should describe a reasonable migration strategy.

Possible approaches include:

```text id="evo12"
direct migration
temporary compatibility layer
deprecation period
new explicit mode
Profile revision
major Standard revision
```

Compatibility should serve migration rather than create permanent ambiguity.

If old and new semantics conflict, Vix should eventually make clear which one is canonical.

## Rejected proposals

A rejected VEP can still be valuable.

It records a question that was considered and explains why a particular direction was not accepted.

Future contributors can then avoid repeating the same discussion without new evidence.

A rejected proposal does not become part of the Standard.

Its implementation, if experimental code exists, should not be treated as normative behavior.

## Superseded proposals

A later VEP may replace an earlier accepted proposal.

When that happens, the historical relationship should be recorded.

The new proposal should explain what it supersedes and why the previous model is no longer sufficient.

The Standard should describe only the currently applicable semantic contract for its version.

The proposal history preserves the evolution path.

## Reopening accepted decisions

Accepted decisions may be revisited.

The purpose of a Standard is stability, not refusal to learn.

However, changing an accepted foundational semantic rule has greater cost than changing an experiment.

Reopening such a decision should require evidence that the existing rule causes a meaningful problem or prevents Vix from meeting its goals.

The stronger the existing compatibility commitment, the stronger the justification for change should be.

## Standard drafts

Work toward a future Standard revision may exist in draft form.

Draft material must be identifiable as draft.

A draft may contain incomplete language, unresolved questions, or proposals not yet accepted.

Implementations may experiment against draft semantics, but they must not claim current Standard conformance based solely on draft material.

Publication status matters.

## Current and next Standard

The project may maintain a stable current Standard while developing a future revision.

Conceptually:

```text id="evo13"
Current Standard
    normative for current conformance

Next Standard draft
    evolving future contract
```

This allows Vix to continue engineering against a stable contract while future semantics are explored.

The project should avoid mixing accepted and speculative rules in a way that makes it impossible to know which contract is current.

## VEP identifiers

Vix Evolution Proposals should eventually have stable identifiers.

For example:

```text id="evo14"
VEP-0001
VEP-0002
VEP-0003
```

The exact numbering format is not yet important.

Stable identifiers make it possible for Standard changes, implementation commits, discussions, and conformance tests to reference the same semantic decision.

An identifier does not imply acceptance.

Proposal status must remain separate.

## VEP status

A VEP may move through states such as:

```text id="evo15"
Draft
Under Review
Accepted
Rejected
Withdrawn
Superseded
```

The exact workflow is defined in `vep-process.md`.

Status should make it clear whether the proposal currently affects the Standard.

Only accepted proposals incorporated into the applicable Standard revision become normative.

## Decision ownership

The evolution process should make clear who is authorized to accept changes to the Standard.

That governance model may evolve as the Vix project and community grow.

The Standard itself does not need to encode every organizational detail.

It does need a reliable process that prevents semantic changes from becoming normative merely because they were merged into one implementation branch.

Technical acceptance and repository write access are not necessarily the same authority.

## Evolution and implementation history

Existing implementation behavior is important evidence, but it does not automatically have permanent normative status.

Historical Vix may contain:

```text id="evo16"
compatibility abstractions
temporary ownership assumptions
old executor dependencies
duplicated semantics
module-local conventions
```

The Standard may intentionally define a cleaner model.

When this occurs, the migration should be explicit.

The project should not standardize an accidental historical behavior solely because it already exists.

## Evolution and simplicity

A mature Standard can become more complex over time if every edge case results in another universal abstraction.

The evolution process should therefore ask not only whether a proposal is correct, but also whether it belongs at the level being changed.

Sometimes the right outcome is a domain-specific rule.

Sometimes the right outcome is an implementation detail.

Sometimes the right outcome is to leave several representations possible under one semantic contract.

The Standard should grow only where shared meaning genuinely requires growth.

## Evolution and implementation freedom

Standardization should constrain what needs stable meaning and leave the rest free.

For example, Vix may standardize that an Operation has one terminal completion without standardizing the internal state machine used to implement that rule.

It may standardize borrowed lifecycle authority without standardizing whether the API uses references, handles, or smart pointers.

It may standardize Capability semantics without requiring a global runtime registry.

This balance allows the implementation to continue improving after the Standard becomes stable.

## Normative requirements

Semantic changes to established Standard contracts MUST be introduced deliberately rather than emerging accidentally from implementation changes.

Research and experimental implementation MUST NOT be treated as normative Standard text merely because they exist in the repository.

A VEP MUST remain conceptually distinct from the Standard text it proposes to change.

Accepted semantics MUST be incorporated into the relevant Standard documents rather than requiring readers to derive the current contract solely from proposal history.

Implementation architecture MUST NOT redefine accepted Standard semantics without going through the evolution process.

Changes that affect what conforming implementations or software may rely on MUST receive semantic compatibility review.

Editorial corrections MUST NOT be used to hide substantive semantic changes.

A rejected, withdrawn, or draft VEP MUST NOT be treated as part of the current Standard.

Domain-specific evolution MUST preserve applicable foundational semantics.

A proposal introducing independent lifetime, new ownership rules, new completion semantics, new Capability guarantees, or other foundational behavior MUST consider its interaction with existing Standard concepts.

Accepted semantic changes SHOULD be followed by implementation migration and conformance validation.

VEPs SHOULD preserve enough rationale that future evolution can understand why the accepted decision was made.

Compatibility mechanisms SHOULD provide a migration path without leaving conflicting semantics permanently ambiguous.

Foundational concepts SHOULD be standardized only when shared semantic need justifies the long-term compatibility commitment.

## Open questions

The precise governance model for accepting VEPs is not yet defined here.

The project still needs to decide questions such as:

```text id="evo17"
who may author a VEP
who reviews it
who has final acceptance authority
whether decisions require consensus
how long review remains open
how Standard releases collect accepted VEPs
```

Those decisions may evolve with Vix governance.

The repository location and exact file format for VEPs also remain to be standardized.

They may eventually live outside `docs/standard/`, for example in a dedicated `docs/veps/` hierarchy, because proposals and normative Standard text have different roles.

What is already fixed is the conceptual separation.

Research explores.

A VEP argues.

A decision accepts or rejects.

The Standard records accepted meaning.

Architecture implements it.

Conformance validates it.

## Design consequence

The evolution process should make it possible for Vix to change without making its meaning accidental.

A useful question for every proposed Standard change is:

> Are we changing how Vix is implemented, or are we changing what programmers are allowed to rely on?

If only the implementation changes, the Standard may not need to move.

If the answer changes what programmers can rely on, then the decision belongs in semantic evolution.

That distinction is what allows Vix to remain both stable and capable of learning.
