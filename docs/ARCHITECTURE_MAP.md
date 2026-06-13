# Architecture Map

This is a practical map of the current v29.1 runtime and control-layer code paths. It describes the implemented repository state after the CandidateArtifactDraftReview policy layer and its snapshot/smoke coverage. It does not describe a composition resolver, persistent runtime, GUI/API layer, artifact text generation, Artifact insertion, PublicClaim insertion, or discovery expansion because those do not exist yet.

## 1. Runtime selection

Primary files:

- `src/cli.cpp`
- `src/cli_model.h`
- `src/civilization_bootstrap.cpp`
- `src/civilization_bootstrap_api.h`
- `src/engine_fixture.cpp`

Flow:

```text
CLI options
-> build_runtime_state_for_query(...)
-> ArchiveRuntimeMode
   -> SpecSelected
      -> load CivilizationSpecCatalog
      -> validate catalog
      -> select civilization_id
      -> bootstrap ArchiveEngineState
      -> derive current control-layer, draft-outline, and draft-review records
   -> FixedFixture
      -> build regression fixture state
      -> derive current control-layer, draft-outline, and draft-review records
```

`SpecSelected` is the default runtime. The bundled default spec file is `examples/40_civilization_specs_v1_1.json`, and the default civilization is `marsh_citadel`. `FixedFixture` is explicit regression mode via `--runtime fixed-fixture`.

## 2. Spec loading and validation

Primary files:

- `src/civilization_specs.cpp`
- `src/civilization_specs_api.h`
- `src/civilization_spec_model.h`
- `examples/40_civilization_specs_v1_1.json`

Responsibilities:

- Parse the narrow `CivilizationSpec v1.1` catalog schema.
- Validate required fields, catalog cardinality, duplicate IDs, authority conflicts, tags, and informational profile metadata.
- Provide catalog lookup and inspection formatting.

## 3. Fragment record intake

Primary files:

- `src/civilization_fragment_model.h`
- `src/civilization_fragments_api.h`
- `src/civilization_fragments.cpp`
- `src/civilization_specs.cpp`
- `examples/v12_fragment_examples.json`

Boundary:

```text
CivilizationSpecFragment records
-> load / inspect / validate only
-> no patch application
-> no composition resolver
-> no generation behavior changes
-> no ArchiveEngineState mutation
```

## 4. ArchiveEngineState bootstrap

Primary files:

- `src/civilization_bootstrap.cpp`
- `src/civilization_bootstrap_api.h`
- `src/archive_engine_state.h`
- `src/validation.cpp`

Flow:

```text
CivilizationSpec
-> deterministic spec-scoped hidden entities/events
-> CivilizationRuntimeSource
-> validate_full_state(...)
-> one ArchiveEngineState
```

The bootstrap produces a minimal valid state. It does not generate a complete artifact pool.

## 5. Current state spine

Primary file:

- `src/archive_engine_state.h`

Current `ArchiveEngineState` owns:

```text
seed
include_seeded_calendar_dispute
hidden_truth
public_archive
discovery_log
mysteries
hidden_truth_mutations
evidence_potentials
candidate_artifact_plans
candidate_artifact_plan_evaluations
candidate_artifact_proposals
candidate_artifact_proposal_audits
candidate_artifact_drafts
candidate_artifact_draft_reviews
control_layer_audit_entries
civilization_source
civilization_spec_count
civilization_fragment_count
```

This is an in-memory state object for one CLI invocation. There is no file-backed or database-backed session persistence.

## 6. Generation target resolution

Primary files:

- `src/candidate_generation.cpp`
- `src/candidate_generation_api.h`

Spec-selected targets include:

```text
authority_conflict_N
institution_<key>
site_<key>
recordkeeping
mystery_seed
```

Fixed fixture targets remain available only in explicit regression mode.

## 7. Candidate generation

Primary files:

- `src/candidate_generation.cpp`
- `src/candidate_generation_api.h`
- `src/candidate_model.h`
- `src/originality.cpp`

Flow:

```text
ArchiveEngineState
-> resolve generation target
-> generate CandidateFeature batch
-> evaluate_candidate_feature(...)
-> format through access filter
```

Generation proposes; it does not mutate public archive state.

## 8. Hidden cluster generation

Primary files:

- `src/hidden_clusters.cpp`
- `src/hidden_workflows_api.h`
- `src/hidden_truth_model.h`

Flow:

```text
GenerationTarget
-> GeneratedHiddenTimelineCluster
-> evaluate on copied HiddenTruthGraph
-> format through access filter
```

Hidden-cluster generation and evaluation are non-mutating.

## 9. Hidden mutation materialization

Primary files:

- `src/hidden_clusters.cpp`
- `src/hidden_workflows_api.h`
- `src/validation.cpp`

Flow:

```text
GeneratedHiddenTimelineCluster
-> access gate: curator/canon/debug
-> fresh evaluation
-> snapshot ArchiveEngineState
-> insert hidden entities/events
-> create HiddenTruthMutationRecord
-> validate hidden graph and full state
-> commit or rollback
```

Failed, rejected, unauthorized, or duplicate materialization creates no false mutation record.

## 10. Hidden-mutation artifact candidate generation

Primary files:

- `src/candidate_generation.cpp`
- `src/candidate_generation_api.h`
- `src/candidate_model.h`

Flow:

```text
HiddenTruthMutationRecord
-> validate hidden mutation source
-> generate three public-facing CandidateFeature shapes
-> evaluate through existing candidate gate
-> redact hidden source below curator access
```

The generated artifacts remain candidates until explicitly materialized.

## 11. Public artifact materialization

Primary files:

- `src/candidates_materialization.cpp`
- `src/materialization_api.h`
- `src/public_archive_model.h`
- `src/originality.cpp`

Flow:

```text
CandidateFeature
-> access gate: curator/canon/debug
-> fresh candidate evaluation
-> originality/specificity materialization gate
-> snapshot ArchiveEngineState
-> insert artifact/claims/discovery/provenance
-> validate_full_state(...)
-> commit or rollback
```

Low civilization specificity, high direct-copy risk, or originality trope flags block materialization.

## 12. PublicArchive invariant boundary

Primary file:

- `src/public_archive_model.h`

`PublicArchive::add_claim_to_artifact(...)` is the preferred archive-owned API for inserting a `Claim` that belongs to an existing `Artifact`. It validates the target artifact, `source_artifact_id`, duplicate claim ID, and existing artifact claim links before mutation. It then inserts the claim, links the claim ID to the artifact, and adds a voice claim link only when `voice_weight > 0`.

The older detached-artifact helper remains as a quarantined compatibility path for fixture/materialization builders that assemble an `Artifact` before archive insertion.

## 13. Validation and rollback

Primary files:

- `src/validation.cpp`
- `src/validation_api.h`
- `src/serialization.cpp`

Validation checks cross-references, hidden chronology, discovery records, candidate metadata, mutation provenance, control-layer records, and full archive consistency. Mutating paths snapshot state before insertion and roll back on validation failure.

## 14. Access filtering and redaction

Primary files:

- `src/artifact_voice_and_views.cpp`
- `src/mysteries_answers.cpp`
- `src/interpreters_discovery.cpp`
- `src/cli.cpp`

Public/scholar output is filtered through access-aware formatters. Curator/canon/debug can inspect internal traces where the workflow allows it.

Diagnostic detail access for the tracked v28/v29 outline surfaces is routed through centralized helper surfaces where migrated. Public/scholar output for hidden or diagnostic details remains restricted or `found: false`; curator/canon/debug can inspect internal traces where the workflow allows it.

## 15. Golden fixtures and ArchiveSnapshot

Primary files:

- `src/golden_fixture_model.h`
- `src/golden_fixtures_api.h`
- `src/golden_fixtures.cpp`
- `src/archive_snapshot_model.h`
- `src/archive_snapshot_api.h`
- `src/archive_snapshot.cpp`

Golden fixture inspection is intentionally separate from normal runtime query execution.

```text
CLI fixture query
-> find_golden_fixture_world(...)
-> reject incompatible fixture override flags before fixture/spec/state construction
-> build_golden_fixture_world(...)
   -> FixedFixture: initialize_archive_engine(seed)
   -> SpecSelected: load catalog -> validate catalog only -> observe inert fragments -> select spec -> bootstrap ArchiveEngineState
-> derive current control-layer, draft-outline, and draft-review records
-> build_archive_snapshot(...)
-> format_archive_snapshot(...) or format_archive_snapshot_comparison(...)
```

`ArchiveSnapshot` is a summary-level regression tripwire. It records counts, validation errors, fixture/runtime seed and archive-year fields, and a `summary_digest`. It is not a persistence format, does not serialize full hidden truth for users, and does not make fragments active in runtime generation.

`compare-archive-snapshots` rebuilds the selected golden fixture twice and compares the resulting summary snapshots. It is a same-fixture determinism check, not an arbitrary two-snapshot diff command.

## 16. EvidencePotential projection seam

Primary files:

- `src/evidence_potential_model.h`
- `src/evidence_potential_api.h`
- `src/evidence_potential.cpp`
- `src/archive_engine_state.h`
- `src/archive_snapshot.cpp`
- `src/golden_fixtures.cpp`
- `src/cli.cpp`

Runtime shape:

```text
ArchiveEngineState
  -> hidden_truth
  -> evidence_potentials
  -> public_archive
```

`EvidencePotential` records plausible traces from existing hidden events, site/office entities, and public mysteries. It is deterministic, value-based, and inspectable, but inert. It must not drive artifact generation, candidate generation, discovery scheduling, public archive mutation, hidden truth mutation, spec selection, fragment activation, composition resolution, or patch application.

## 17. KnowledgeHorizon validation seam

Primary files:

- `src/knowledge_horizon_model.h`
- `src/knowledge_horizon_api.h`
- `src/knowledge_horizon.cpp`
- `src/cli.cpp`

`KnowledgeHorizon` validates whether an artifact creator, interpreter, public archive view, curator audit, EvidencePotential derivation, or snapshot validation context could plausibly know a referenced subject.

The seam is validation-only. It must not generate artifacts, mutate public archive state, mutate hidden truth, alter discovery, activate fragments, resolve compositions, apply patches, or persist runtime state.

Public formatting reports aggregate counts and hides hidden IDs/explanations. KnowledgeHorizon finding IDs and detailed finding records are curator/debug diagnostics. Public and scholar access cannot retrieve or enumerate hidden finding details by ID; `show-knowledge-horizon-finding` returns `found: false` for hidden or inaccessible findings.

## 18. ContradictionBudget telemetry and policy seam

Primary files:

- `src/contradiction_budget_model.h`
- `src/contradiction_budget_api.h`
- `src/contradiction_budget.cpp`
- `src/archive_snapshot.cpp`
- `src/cli.cpp`

`ContradictionBudget` computes deterministic, read-only contradiction-pressure telemetry over the current single-shot `ArchiveEngineState`. It creates archive-level, type-level, cause-level, mystery-linked, EvidencePotential-linked, and KnowledgeHorizon-linked buckets.

v28.11 adds explicit policy thresholds, deterministic reason codes, too-clean archive detection, generation-bug pressure classification, productive ambiguity classification, and stricter status/reason validation.

This seam observes and classifies only. It must not enforce budgets, repair contradictions, reject materialization, generate artifacts, schedule discovery, convert EvidencePotential records, mutate hidden truth, mutate public archive state, activate fragments, resolve compositions, apply patches, persist state, or introduce an interactive runtime session.

## 19. CandidateArtifactPlan planning seam

Primary files:

- `src/candidate_artifact_plan_model.h`
- `src/candidate_artifact_plan_api.h`
- `src/candidate_artifact_plan.cpp`

Flow:

```text
EvidencePotential -> CandidateArtifactPlan
```

CandidateArtifactPlan computes deterministic, inert plans that describe plausible future artifact-candidate shapes, evidence roles, validation steps, public-safety constraints, KnowledgeHorizon diagnostics, and ContradictionBudget pressure. The layer never creates candidates, inserts artifacts, discovers evidence, mutates hidden truth, mutates the public archive, activates fragments, resolves specs, or persists state.

## 20. CandidateArtifactPlanEvaluation seam

Primary files:

- `src/candidate_artifact_plan_evaluation_model.h`
- `src/candidate_artifact_plan_evaluation_api.h`
- `src/candidate_artifact_plan_evaluation.cpp`

CandidateArtifactPlanEvaluation is a read-only advisory layer over CandidateArtifactPlan records. It computes deterministic decisions, readiness/risk scores, gate findings, and required next checks. It never generates candidate artifacts, renders artifact text, discovers evidence, mutates hidden truth, mutates the public archive, activates fragments, introduces resolver/composition behavior, or enables an interactive session.

## 21. CandidateArtifactProposal drafting seam

Primary files:

- `src/candidate_artifact_proposal_model.h`
- `src/candidate_artifact_proposal_api.h`
- `src/candidate_artifact_proposal.cpp`

CandidateArtifactProposal is a read-only drafting layer over CandidateArtifactPlanEvaluation records. It records a proposed artifact shape, type, voice register, title, claim skeleton strings, validation gates, damage/distortion modes, safety, and access notes. It never creates Artifact records, generates final artifact prose, inserts PublicClaim records, schedules discovery, mutates hidden truth, mutates the public archive, activates fragments, introduces resolver/composition behavior, or enables an interactive session.

CandidateArtifactProposal records are access-neutral state. Public/scholar/curator/debug safety is projected at formatting time from visibility class and diagnostic-content flags.

## 22. CandidateArtifactProposalAudit seam

Primary files:

- `src/candidate_artifact_proposal_audit_model.h`
- `src/candidate_artifact_proposal_audit_api.h`
- `src/candidate_artifact_proposal_audit.cpp`

CandidateArtifactProposalAudit classification is policy-backed. The audit module exposes deterministic thresholds, reason-coded findings, required revision enforcement for non-pass audits, and stronger validation for policy threshold violations.

This is not a generation or mutation layer. Audit `Pass` means audit-clean only; it does not enable artifact generation, candidate materialization, discovery scheduling, public archive mutation, hidden truth mutation, resolver/composition behavior, persistence, or interactive runtime.

## 23. CandidateArtifactDraft outline seam

Primary files:

- `src/candidate_artifact_draft_model.h`
- `src/candidate_artifact_draft_api.h`
- `src/candidate_artifact_draft.cpp`
- `src/archive_snapshot.cpp`
- `src/cli.cpp`

Flow:

```text
CandidateArtifactProposal
+ CandidateArtifactProposalAudit
-> CandidateArtifactDraft
-> validate / inspect / snapshot only
```

CandidateArtifactDraft is a non-mutating outline layer. It stores outline title, intended artifact type/register, claim-outline lines, required validation gates, public-safe summaries, source-chain diagnostics, and curator notes. All mutation flags remain false: no Artifact insertion, no PublicClaim insertion, no discovery scheduling, no hidden truth mutation, no PublicArchive mutation, no persistence, no resolver/composition behavior, and no final artifact prose generation.

Public/scholar access receives aggregate or public-safe summaries only. Curator/debug access can inspect proposal/audit IDs, source chains, validation gates, and diagnostic notes.

## 24. CandidateArtifactDraftReview policy seam

Primary files:

- `src/candidate_artifact_draft_review_model.h`
- `src/candidate_artifact_draft_review_api.h`
- `src/candidate_artifact_draft_review.cpp`
- `src/archive_snapshot.cpp`
- `src/cli.cpp`

Flow:

```text
CandidateArtifactDraft
-> CandidateArtifactDraftReview
-> validate / inspect / snapshot only
```

CandidateArtifactDraftReview is a non-mutating policy/review layer. It scores outline completeness, traceability, safety, specificity, and revision pressure; emits deterministic pass/revision/review/block/invalid decisions; and requires actionable revisions for non-pass decisions. Review pass means review-clean only. It does not enable artifact prose, Artifact insertion, PublicClaim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, resolver/composition behavior, or an interactive runtime session.

Public/scholar access receives aggregate review summaries only. Curator/debug access can inspect review IDs, draft/proposal/audit IDs, scores, reason codes, and required revisions.

## 25. ControlLayerAudit

Primary files:

- `src/control_layer_audit_model.h`
- `src/control_layer_audit_api.h`
- `src/control_layer_audit.cpp`

`ControlLayerAudit` is a deterministic, read-only consolidation layer that inventories the v28 control stack. It classifies core state, validation, telemetry, planning, evaluation, proposal, audit, formatting, CLI, and mutation-capable workflows by persistence, behavior, access gates, snapshot coverage, smoke/self-test coverage, full-state-validation coverage, mutation capability, and known gaps.

It is not a generation, discovery, resolver, persistence, or mutation layer.

## 26. Test and release surfaces

Primary files:

- `src/self_tests.cpp`
- `scripts/smoke_test_readme_workflows.sh`
- `Makefile`
- `.github/workflows/ci.yml`
- `RELEASE_CHECKLIST.md`

Local validation surfaces include:

```bash
make test
make CXXSTD=c++17 test
make strict
make sanitize
make smoke
make release-check
```

GitHub Actions currently covers C++20 self-test, C++17 self-test, and strict warnings self-test. CI parity for sanitizer and smoke remains a recommended follow-up.

## 27. Explicit non-goals in the current architecture

```text
No artifact text generation from proposal/audit/draft/review records.
No Artifact insertion or PublicClaim insertion from CandidateArtifactDraft or CandidateArtifactDraftReview records.
No discovery scheduling from CandidateArtifactDraft or CandidateArtifactDraftReview records.
No EvidencePotential-to-artifact conversion.
No discovery expansion from the v28 control chain.
No proposal materialization.
No resolver/composition behavior.
No fragment activation.
No file/database persistence.
No interactive runtime session.
No GUI/API layer.
No multi-spec runtime state.
No cross-civilization merge.
```
