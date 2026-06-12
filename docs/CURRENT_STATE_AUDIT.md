# Current State Audit — v28.11 Control-Layer Consolidation

## 1. Executive summary

The engine is not release-ready. It is in v28 control-layer consolidation with a v28.11 baseline. Artifact generation, artifact text generation, discovery expansion, proposal materialization, EvidencePotential-to-artifact conversion, resolver/composition behavior, persistence, and interactive runtime behavior remain deferred.

v28.11 extends the v28 control stack with validator-backed `ContradictionBudget` policy/status tightening. The current control layers are intended to make the engine inspectable, deterministic, access-aware, and safe to extend without accidentally turning advisory records into archive mutation.

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
ControlLayerAudit
HiddenClusterPreview
HiddenMutationArtifactCandidate planning output
```

Inert means they may inspect, summarize, validate, classify, or prepare future work, but they must not insert artifacts, claims, discoveries, hidden events, hidden entities, resolver output, fragment-derived runtime state, or persistent session state.

## 7. Access-gated surfaces

Access gating is primarily enforced in formatting/query projection. Public and scholar detail surfaces for hidden or diagnostic records return restricted summaries or `found: false`; curator/debug may inspect full details.

Known risk: some access behavior remains distributed across formatters rather than centralized in a policy engine. This is acceptable for the current read-only/advisory layers, but should be tightened before v29 draft/detail surfaces become richer.

## 8. Snapshot/digest coverage

`ArchiveSnapshot` includes summary counts for v28 control layers through `CandidateArtifactProposalAudit`, `ControlLayerAudit`, and v28.11 `ContradictionBudget` reason/status counts. The `summary_digest` is a deterministic regression tripwire, not a persistence format and not a full semantic state digest.

## 9. Smoke/self-test coverage

The smoke workflow covers representative runtime selection, specs, fragments, fixtures, snapshots, EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, and ControlLayerAudit query surfaces, including public-detail blocking checks and expected failures.

Self-tests remain monolithic but broad. They cover runtime selection, catalog/tag metadata, access gates, generation/materialization constraints, archive invariants, deterministic snapshots, and control-layer validation. Splitting self-tests by subsystem is now a recommended maintainability improvement, not a behavior requirement.

## 10. Known gaps

High-level gaps:

```text
No database or file-backed persistence.
No resolver/composition behavior.
No EvidencePotential-to-artifact conversion.
No artifact text generation from proposal/audit records.
No discovery expansion from the v28 control chain.
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
```

The safest immediate direction is still non-mutating hardening: documentation reconciliation, CI parity, self-test structure cleanup, and centralized diagnostic access helper work.

## 12. Do-not-build-on-yet layers

Do not build artifact generation, discovery expansion, or proposal materialization directly on `CandidateArtifactProposal` or `CandidateArtifactProposalAudit` yet. The next artifact-facing slice should remain a draft/outline layer only, with explicit no-insertion and no-discovery invariants.

## 13. Recommended next slices

Recommended immediate maintenance slice:

```text
v28.12 — Documentation / roadmap reconciliation, CI parity planning, and test-structure hardening prep
```

Recommended next feature-shaped slice after maintenance:

```text
v29.0 — CandidateArtifactDraft / Text Outline, No Artifact Insertion
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
