# Current State Audit — v29.1 CandidateArtifactDraftReview Baseline

## 1. Executive summary

The engine is not release-ready. It is at the v29.1 CandidateArtifactDraftReview baseline. Artifact generation, artifact text generation, Artifact insertion, PublicClaim insertion, discovery expansion/scheduling, proposal materialization, EvidencePotential-to-artifact conversion, resolver/composition behavior, persistence, and interactive runtime behavior remain deferred.

v29.1 extends the v29.0 draft-outline baseline with non-mutating `CandidateArtifactDraftReview` records derived from `CandidateArtifactDraft` records. The current control layers are intended to make the engine inspectable, deterministic, access-aware, and safe to extend without accidentally turning advisory records into archive mutation.

## 2. Current version baseline

Current baseline:

```text
v28.1     Golden fixtures + ArchiveSnapshot
v28.1.1   Fixture/snapshot semantics tightening
v28.2     EvidencePotential foundation
v28.3     KnowledgeHorizon validation
v28.3.1   KnowledgeHorizon public detail access hardening
v28.4     ContradictionBudget telemetry foundation
v28.5     CandidateArtifactPlan planning foundation
v28.6     CandidateArtifactPlan evaluation foundation
v28.7     CandidateArtifactProposal drafting foundation
v28.7.1   PublicArchive invariant hardening
v28.7.2   CandidateArtifactProposal access-neutral model hardening
v28.7.3   CLI argument view refactor
v28.7.4   ReliabilityComponents primitive-cluster cleanup
v28.8     CandidateArtifactProposal audit / quality gates
v28.9     Control-layer consolidation audit
v28.10    Proposal quality gate policy tightening
v28.11    ContradictionBudget validator-backed status tightening
v28.12    Documentation / roadmap reconciliation and release-gate parity prep
v28.13    CI parity and self-test structure hardening
v28.14    Centralized diagnostic detail access gates
v29.0     CandidateArtifactDraft outline layer, snapshot coverage, and smoke coverage
v29.1     CandidateArtifactDraftReview policy layer, snapshot coverage, and smoke coverage
```

## 3. Control-layer inventory

The code-backed audit includes entries for:

```text
HiddenTruthGraph
PublicArchive
GoldenFixtureWorlds
ArchiveSnapshot
CivilizationSpecFragments
EvidencePotential
KnowledgeHorizon
ContradictionBudget
CandidateArtifactPlan
CandidateArtifactPlanEvaluation
CandidateArtifactProposal
CandidateArtifactProposalAudit
CandidateArtifactDraft
CandidateArtifactDraftReview
CandidateGeneration
CandidateMaterialization
HiddenClusterPreview
HiddenClusterMaterialization
HiddenMutationArtifactCandidate
AccessControl
CLI
SmokeWorkflow
SelfTests
FullStateValidation
```

Use:

```bash
./impossible_archive_mvp_v28_11 --query control-layer-audit-summary
./impossible_archive_mvp_v28_11 --access curator --query list-control-layer-audit-entries
```

## 4. Runtime-enforced vs report-only matrix

| Layer group | Current behavior |
|---|---|
| Core archive state | Runtime-enforced. |
| Golden fixtures / snapshots | Runtime-enforced determinism and summary digest checks. |
| CivilizationSpec fragments | Data-only inspection and validation. No resolver or patch application. |
| EvidencePotential | Inert deterministic projection seam. No artifacts, discovery, candidates, or mutation. |
| KnowledgeHorizon | Validation-only. No generation, mutation, persistence, or resolver behavior. |
| ContradictionBudget | Read-only/advisory telemetry with v28.11 policy-backed status/reason validation. No hard enforcement or repair. |
| CandidateArtifactPlan | Planning-only bridge from EvidencePotential to possible future artifact shapes. |
| CandidateArtifactPlanEvaluation | Evaluation-only advisory records. |
| CandidateArtifactProposal | Proposal-only draft records. No final artifact text, claims, or discovery records. |
| CandidateArtifactProposalAudit | Audit-only quality gate with v28.10 policy thresholds and reason-coded findings. |
| CandidateArtifactDraft | Draft-outline-only records derived from proposal/audit chains. No artifact text generation, Artifact insertion, PublicClaim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, or resolver/composition behavior. |
| CandidateArtifactDraftReview | Review-policy-only records derived from CandidateArtifactDraft records. Scores outline completeness, traceability, safety, specificity, and revision pressure without enabling artifact prose, insertion, discovery, mutation, persistence, or resolver/composition behavior. |
| ControlLayerAudit | Audit-only inventory of the control stack. |
| Mutating workflows | Limited to explicit materialization paths, access-gated and validation-gated. |

## 5. Mutation-capable paths

Mutation-capable paths are explicitly identified in the audit. They include `PublicArchive` mutation-capable state and explicit materialization workflows such as candidate materialization and hidden-cluster materialization. These paths remain access-gated, validation-gated, and rollback-oriented.

## 6. Inert/advisory-only paths

These layers must remain inert in the current baseline:

```text
CivilizationSpecFragments
EvidencePotential
KnowledgeHorizon
ContradictionBudget
CandidateArtifactPlan
CandidateArtifactPlanEvaluation
CandidateArtifactProposal
CandidateArtifactProposalAudit
CandidateArtifactDraft
CandidateArtifactDraftReview
ControlLayerAudit
HiddenClusterPreview
HiddenMutationArtifactCandidate planning output
```

Inert means they may inspect, summarize, validate, classify, or prepare future work, but they must not insert artifacts, claims, discoveries, hidden events, hidden entities, resolver output, fragment-derived runtime state, or persistent session state.

## 7. Access-gated surfaces

Access gating is primarily enforced in formatting/query projection. Public and scholar detail surfaces for hidden or diagnostic records return restricted summaries or `found: false`; curator/debug may inspect full details.

Known risk: some access behavior remains distributed across formatters rather than centralized in a policy engine. This is acceptable for the current read-only/advisory layers, but should be tightened before v29 draft/detail surfaces become richer.

## 8. Snapshot/digest coverage

`ArchiveSnapshot` includes summary counts for control layers through `CandidateArtifactDraftReview`, `ControlLayerAudit`, and v28.11 `ContradictionBudget` reason/status counts. CandidateArtifactDraft and CandidateArtifactDraftReview contribute counters, mutation/generation-enabled count checks, deterministic digest material, replay lines, comparison fields, and count deltas. The `summary_digest` is a deterministic regression tripwire, not a persistence format and not a full semantic state digest.

## 9. Smoke/self-test coverage

The smoke workflow covers representative runtime selection, specs, fragments, fixtures, snapshots, EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, CandidateArtifactDraft, CandidateArtifactDraftReview, and ControlLayerAudit query surfaces, including public-detail blocking checks and expected failures.

Self-tests remain monolithic but broad. They cover runtime selection, catalog/tag metadata, access gates, generation/materialization constraints, archive invariants, deterministic snapshots, and control-layer validation. Splitting self-tests by subsystem is now a recommended maintainability improvement, not a behavior requirement.

## 10. Known gaps

High-level gaps:

```text
No database or file-backed persistence.
No resolver/composition behavior.
No EvidencePotential-to-artifact conversion.
No artifact text generation from proposal/audit/draft/review records.
No discovery expansion from the control chain.
Some access control remains formatting/query-projection based rather than centralized policy-engine based.
Self-tests remain monolithic.
CI does not yet run every local release-check surface by default.
```

## 11. Safe-to-build-on layers

Safe next work should build on:

```text
ControlLayerAudit
ArchiveSnapshot summary/digest coverage
Public access gates
Full-state validation
ContradictionBudget policy/reason-code validation
CandidateArtifactProposalAudit policy/revision outputs
CandidateArtifactDraft outline records and non-mutation invariants
CandidateArtifactDraftReview policy records and non-generation/non-mutation invariants
```

The safest immediate direction is still non-mutating hardening: documentation reconciliation and persistent runtime session planning without storage.

## 12. Do-not-build-on-yet layers

Do not build artifact generation, discovery expansion, or proposal materialization directly on `CandidateArtifactProposal`, `CandidateArtifactProposalAudit`, `CandidateArtifactDraft`, or `CandidateArtifactDraftReview` yet. The next slice should move laterally into persistent runtime session planning, in memory first, while preserving the existing single-shot CLI behavior.

## 13. Recommended next slices

Recommended immediate maintenance slice:

```text
v29.1 documentation reconciliation for the landed CandidateArtifactDraftReview baseline
```

Recommended next feature-shaped slice:

```text
v29.2 — Persistent Runtime Session Planning, In-Memory First
```

Still deferred:

```text
artifact generation
artifact text generation
discovery expansion
proposal materialization
EvidencePotential-to-artifact conversion
resolver/composition behavior
persistence
interactive runtime
GUI/API layer
```
