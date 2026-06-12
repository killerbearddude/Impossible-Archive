# Current State Audit — v28.9 Control-Layer Consolidation

## 1. Executive summary

The engine is not release-ready. It is in v28 control-layer consolidation. Artifact generation and discovery expansion remain deferred until the control stack is clearly classified and stable.

v28.9 adds a code-backed `ControlLayerAudit` inventory that classifies existing layers by persistence, behavior, access gating, snapshot coverage, test coverage, full-state validation coverage, mutation capability, and known gaps. This is documentation plus deterministic inspection only.

## 2. Current version baseline

Current baseline:

```text
v28.1     Golden fixtures + ArchiveSnapshot
v28.1.1   Fixture/snapshot semantics tightening
v28.2     EvidencePotential foundation
v28.3     KnowledgeHorizon validation
v28.3.1   KnowledgeHorizon public detail access hardening
v28.4     ContradictionBudget telemetry
v28.5     CandidateArtifactPlan planning foundation
v28.6     CandidateArtifactPlan evaluation foundation
v28.7     CandidateArtifactProposal drafting foundation
v28.7.1   PublicArchive invariant hardening
v28.7.2   CandidateArtifactProposal access-neutral model hardening
v28.7.3   CLI argument view refactor
v28.7.4   ReliabilityComponents primitive-cluster cleanup
v28.8     CandidateArtifactProposal audit / quality gates
v28.9     Control-layer consolidation audit
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

Runtime-enforced layers include core archive state, fixture construction, and snapshot determinism. Validation-only layers include fragments, EvidencePotential validation, KnowledgeHorizon, self-tests, and full-state validation. Telemetry-only remains ContradictionBudget. Planning/evaluation/proposal/audit-only layers remain inert and advisory.

## 5. Mutation-capable paths

Mutation-capable paths are explicitly identified in the audit. They include PublicArchive mutation-capable state and explicit materialization workflows such as candidate materialization and hidden-cluster materialization. These paths remain access-gated and validation-gated.

## 6. Inert/advisory-only paths

These layers must remain inert in v28.9:

```text
CivilizationSpecFragments
EvidencePotential
KnowledgeHorizon
ContradictionBudget
CandidateArtifactPlan
CandidateArtifactPlanEvaluation
CandidateArtifactProposal
CandidateArtifactProposalAudit
HiddenClusterPreview
HiddenMutationArtifactCandidate planning output
```

## 7. Access-gated surfaces

Access gating is primarily enforced in formatting/query projection. Public detail surfaces for hidden or diagnostic records return restricted summaries or `found: false`; curator/debug may inspect full details.

## 8. Snapshot/digest coverage

ArchiveSnapshot includes summary counts for v28 control layers through CandidateArtifactProposalAudit and now ControlLayerAudit. The `summary_digest` includes stable control-layer audit summary material.

## 9. Smoke/self-test coverage

The smoke workflow covers representative control-layer audit summary, validation, curator list/detail, public detail blocking, snapshot fields, and same-fixture snapshot comparison. Self-tests include deterministic audit construction, required entries, mutation-capable classification, inert classification, public/curator formatting, snapshot counts, digest stability, and validation.

## 10. Known gaps

The code-backed audit records current known gaps. High-level gaps include:

```text
No database or file-backed persistence.
No resolver/composition behavior.
No EvidencePotential-to-artifact conversion.
No artifact text generation from proposal/audit records.
No discovery expansion from the v28 control chain.
Some access control remains formatting/query-projection based rather than centralized policy-engine based.
Self-tests remain monolithic.
```

## 11. Safe-to-build-on layers

Safe next work should build on the control-layer audit, snapshot digest, public access gates, and full-state validation. The safest immediate direction is additional policy tightening that remains non-mutating.

## 12. Do-not-build-on-yet layers

Do not build artifact generation or discovery expansion directly on CandidateArtifactProposal or CandidateArtifactProposalAudit until proposal quality gate policy is tightened and the audit confirms exact future generation preconditions.

## 13. Recommended next slices

Recommended next slice:

```text
v28.10 — Proposal Quality Gate Policy Tightening, No Mutation
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
```
