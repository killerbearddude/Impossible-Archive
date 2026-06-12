# Architecture Map

This is a practical map of the current runtime code paths. It describes the implemented v28.4 shape, including data-only v1.2 fragment intake. It does not describe a composition resolver because no resolver exists yet.

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
→ build_runtime_state_for_query(...)
→ ArchiveRuntimeMode
   → SpecSelected
      → load CivilizationSpecCatalog
      → validate catalog
      → select civilization_id
      → bootstrap ArchiveEngineState
   → FixedFixture
      → build regression fixture state
```

`SpecSelected` is the default runtime. `FixedFixture` is explicit regression mode via `--runtime fixed-fixture`.

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

## 2a. Fragment record intake

Primary files:

- `src/civilization_fragment_model.h`
- `src/civilization_fragments_api.h`
- `src/civilization_fragments.cpp`
- `src/civilization_specs.cpp`
- `examples/v12_fragment_examples.json`

Responsibilities:

- Parse optional top-level `fragments` arrays in CivilizationSpec catalogs.
- Validate `CivilizationSpecFragment` records, categories, inert patch strategies, patch values, and draft-known patch paths.
- Format/list/show fragment records through CLI inspection commands.

Boundary:

```text
CivilizationSpecFragment records
→ load / inspect / validate only
→ no patch application
→ no composition resolver
→ no generation behavior changes
→ no ArchiveEngineState mutation
```


## 3. ArchiveEngineState bootstrap

Primary files:

- `src/civilization_bootstrap.cpp`
- `src/civilization_bootstrap_api.h`
- `src/archive_engine_state.h`
- `src/validation.cpp`

Flow:

```text
CivilizationSpec
→ deterministic spec-scoped hidden entities/events
→ CivilizationRuntimeSource
→ validate_full_state(...)
→ one ArchiveEngineState
```

The bootstrap produces a minimal valid state. It does not generate a complete artifact pool.

## 4. Generation target resolution

Primary files:

- `src/candidate_generation.cpp`
- `src/candidate_generation_api.h`

Spec-selected targets include:

- `authority_conflict_N`
- `institution_<key>`
- `site_<key>`
- `recordkeeping`
- `mystery_seed`

Fixed fixture targets remain available only in explicit regression mode.

## 5. Candidate generation

Primary files:

- `src/candidate_generation.cpp`
- `src/candidate_generation_api.h`
- `src/candidate_model.h`
- `src/originality.cpp`

Flow:

```text
ArchiveEngineState
→ resolve generation target
→ generate CandidateFeature batch
→ evaluate_candidate_feature(...)
→ format through access filter
```

Generation proposes; it does not mutate public archive state.

## 6. Hidden cluster generation

Primary files:

- `src/hidden_clusters.cpp`
- `src/hidden_workflows_api.h`
- `src/hidden_truth_model.h`

Flow:

```text
GenerationTarget
→ GeneratedHiddenTimelineCluster
→ evaluate on copied HiddenTruthGraph
→ format through access filter
```

Hidden-cluster generation and evaluation are non-mutating.

## 7. Hidden mutation materialization

Primary files:

- `src/hidden_clusters.cpp`
- `src/hidden_workflows_api.h`
- `src/validation.cpp`

Flow:

```text
GeneratedHiddenTimelineCluster
→ access gate: curator/canon/debug
→ fresh evaluation
→ snapshot ArchiveEngineState
→ insert hidden entities/events
→ create HiddenTruthMutationRecord
→ validate hidden graph and full state
→ commit or rollback
```

Failed, rejected, unauthorized, or duplicate materialization creates no false mutation record.

## 8. Hidden-mutation artifact candidate generation

Primary files:

- `src/candidate_generation.cpp`
- `src/candidate_generation_api.h`
- `src/candidate_model.h`

Flow:

```text
HiddenTruthMutationRecord
→ validate hidden mutation source
→ generate three public-facing CandidateFeature shapes
→ evaluate through existing candidate gate
→ redact hidden source below curator access
```

The generated artifacts remain candidates until explicitly materialized.

## 9. Public artifact materialization

Primary files:

- `src/candidates_materialization.cpp`
- `src/materialization_api.h`
- `src/public_archive_model.h`
- `src/originality.cpp`

Flow:

```text
CandidateFeature
→ access gate: curator/canon/debug
→ fresh candidate evaluation
→ originality/specificity materialization gate
→ snapshot ArchiveEngineState
→ insert artifact/claims/discovery/provenance
→ validate_full_state(...)
→ commit or rollback
```

Low civilization specificity, high direct-copy risk, or originality trope flags block materialization.

## 10. Validation and rollback

Primary files:

- `src/validation.cpp`
- `src/validation_api.h`
- `src/serialization.cpp`

Validation checks cross-references, hidden chronology, discovery records, candidate metadata, mutation provenance, and full archive consistency. Mutating paths snapshot state before insertion and roll back on validation failure.

## 11. Access filtering and redaction

Primary files:

- `src/artifact_voice_and_views.cpp`
- `src/mysteries_answers.cpp`
- `src/interpreters_discovery.cpp`
- `src/cli.cpp`

Public/scholar output is filtered through access-aware formatters. Curator/canon/debug can inspect internal traces where the workflow allows it.

## 12. Fixed fixture regression mode

Primary files:

- `src/engine_fixture.cpp`
- `src/cli.cpp`

The fixed fixture remains a deterministic regression world, selected explicitly with:

```bash
./impossible_archive_mvp_v28_11 --runtime fixed-fixture --query report
```

It is not the default runtime path.

## v28.1.1 Golden Fixtures and ArchiveSnapshot

Golden fixture inspection is intentionally separate from normal runtime query execution.

```text
CLI fixture query
→ find_golden_fixture_world(...)
→ reject incompatible fixture override flags before fixture/spec/state construction
→ find_golden_fixture_world(...)
→ build_golden_fixture_world(...)
   → FixedFixture: initialize_archive_engine(seed)
   → SpecSelected: load catalog → validate catalog only → observe inert fragments → select spec → bootstrap ArchiveEngineState
→ build_archive_snapshot(...)
→ format_archive_snapshot(...) or format_archive_snapshot_comparison(...)
```

Main files:

- `src/golden_fixture_model.h`
- `src/golden_fixtures_api.h`
- `src/golden_fixtures.cpp`
- `src/archive_snapshot_model.h`
- `src/archive_snapshot_api.h`
- `src/archive_snapshot.cpp`

`ArchiveSnapshot` is a summary-level regression tripwire. It records counts, validation errors, fixture/runtime seed and archive-year fields, and a `summary_digest`. It is not a persistence format, does not serialize full hidden truth for users, and does not make fragments active in runtime generation.


### v28.1.1 fragment validation policy

`validate-civilization-fragments` is the strict catalog-validation surface for fragment records. Normal spec-selected runtime bootstrap and golden fixture bootstrap treat fragments as inert catalog data: invalid inert fragments are reportable by validation, but they do not activate resolver behavior, patch application, generation behavior, materialization, archive mutation, or hidden mutation.

### Snapshot comparison semantics

`compare-archive-snapshots` rebuilds the selected golden fixture twice and compares the resulting summary snapshots. It is a same-fixture determinism check, not an arbitrary two-snapshot diff command.


## v28.2 EvidencePotential Projection Seam

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

Golden fixture bootstrap derives potentials after constructing the selected runtime state. `ArchiveSnapshot` reports `evidence_potential_count`, and `summary_digest` includes stable potential IDs, source type, source ID, trace type, likely artifact type, year range, strength, `public_safe`, and `discoverable` values. Public snapshot formatting still exposes counts/digests only, not hidden source IDs.

CLI inspection:

```bash
./impossible_archive_mvp_v28_11 --query evidence-potential-summary
./impossible_archive_mvp_v28_11 --access curator --query list-evidence-potentials
./impossible_archive_mvp_v28_11 --access curator --query show-evidence-potential --evidence-potential-id evidence_potential.0000
./impossible_archive_mvp_v28_11 --query validate-evidence-potentials
```


## v28.3 KnowledgeHorizon Validation Seam

`KnowledgeHorizon` validates whether an artifact creator, interpreter, public archive view, curator audit, EvidencePotential derivation, or snapshot validation context could plausibly know a referenced subject. Findings are deterministic value records with context type/id, subject type/id, context year, earliest available year, status, mediation artifacts, and diagnostic text.

The seam is validation-only. It must not generate artifacts, mutate public archive state, mutate hidden truth, alter discovery, activate fragments, resolve compositions, apply patches, or persist runtime state. Public formatting reports aggregate counts and hides hidden IDs/explanations. KnowledgeHorizon finding IDs and detailed finding records are curator/debug diagnostics. Public and scholar access cannot retrieve or enumerate hidden finding details by ID; `show-knowledge-horizon-finding` returns `found: false` for hidden or inaccessible findings. Curator/debug formatting exposes diagnostic details.

```bash
./impossible_archive_mvp_v28_11 --query validate-knowledge-horizon
./impossible_archive_mvp_v28_11 --query knowledge-horizon-summary
./impossible_archive_mvp_v28_11 --access curator --query list-knowledge-horizon-findings
./impossible_archive_mvp_v28_11 --access curator --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0000
```


## v28.4 ContradictionBudget Telemetry Seam

`ContradictionBudget` computes deterministic, read-only contradiction-pressure telemetry over the current single-shot `ArchiveEngineState`. It creates archive-level, type-level, cause-level, and mystery-linked buckets with contradiction density, unresolved ratio, generation-bug ratio, severity, status, protected-mystery pressure, and KnowledgeHorizon error pressure.

This seam observes only. It must not enforce budgets, repair contradictions, reject materialization, generate artifacts, schedule discovery, convert EvidencePotential records, mutate hidden truth, mutate public archive state, activate fragments, resolve compositions, apply patches, persist state, or introduce an interactive runtime session.

Public formatting exposes aggregate count/severity/status data only. Curator/debug formatting exposes bucket IDs, representative contradiction IDs, scope IDs, and diagnostic notes. Archive snapshots include ContradictionBudget counts and the summary digest includes stable bucket/status/count material, but not hidden explanations.

```bash
./impossible_archive_mvp_v28_11 --query contradiction-budget-summary
./impossible_archive_mvp_v28_11 --query validate-contradiction-budget
./impossible_archive_mvp_v28_11 --runtime fixed-fixture --access curator --query list-contradiction-budget-buckets
./impossible_archive_mvp_v28_11 --runtime fixed-fixture --access curator --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive
```


## v28.5 CandidateArtifactPlan Planning Seam

CandidateArtifactPlan is a read-only planning layer:

```text
EvidencePotential -> CandidateArtifactPlan
```

It computes deterministic, inert plans that describe plausible future artifact-candidate shapes, evidence roles, validation steps, public-safety constraints, KnowledgeHorizon diagnostics, and ContradictionBudget pressure. The layer never creates candidates, inserts artifacts, discovers evidence, mutates hidden truth, mutates the public archive, activates fragments, resolves specs, or persists state. Public formatting is aggregate/public-safe only; curator/debug formatting exposes full diagnostics.

CLI checks:

```bash
./impossible_archive_mvp_v28_11 --query candidate-artifact-plan-summary
./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-plans
./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-plans
./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-plan --candidate-artifact-plan-id candidate_artifact_plan.evidence_potential.0019.administrative_docket
```


## v28.6 CandidateArtifactPlan Evaluation Seam

CandidateArtifactPlanEvaluation is a read-only advisory layer over CandidateArtifactPlan records. It computes deterministic decisions, readiness/risk scores, gate findings, and required next checks. It never generates candidate artifacts, renders artifact text, discovers evidence, mutates hidden truth, mutates the public archive, activates fragments, introduces resolver/composition behavior, or enables an interactive session.

```bash
./impossible_archive_mvp_v28_11 --query candidate-artifact-plan-evaluation-summary
./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-plan-evaluations
./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-plan-evaluations
./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-plan-evaluation --candidate-artifact-plan-evaluation-id candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0019.administrative_docket
```

## v28.7 CandidateArtifactProposal Drafting Seam

CandidateArtifactProposal is a read-only drafting layer over CandidateArtifactPlanEvaluation records. It records a proposed artifact shape, type, voice register, title, claim skeleton strings, validation gates, damage/distortion modes, safety, and access notes. It never creates Artifact records, generates final artifact prose, inserts PublicClaim records, schedules discovery, mutates hidden truth, mutates the public archive, activates fragments, introduces resolver/composition behavior, or enables an interactive session.

```bash
./impossible_archive_mvp_v28_11 --query candidate-artifact-proposal-summary
./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-proposals
./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-proposals
./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-proposal --candidate-artifact-proposal-id candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0019.administrative_docket
```

Public/scholar formatting is summary-only unless a proposal is explicitly safe. Hidden source IDs, diagnostic IDs, blocking internals, protected mystery details, and curator-only rationale remain restricted.

## v28.7.1 PublicArchive Invariant Hardening

`PublicArchive::add_claim_to_artifact(...)` is the preferred archive-owned API for inserting a `Claim` that belongs to an existing `Artifact`. It validates the target artifact, `source_artifact_id`, duplicate claim ID, and existing artifact claim links before mutation, then inserts the claim, links the claim ID to the artifact, and adds a voice claim link only when `voice_weight > 0`.

The older `add_claim_to_archive(...)` helper remains as a quarantined detached-construction compatibility path for fixture/materialization builders that assemble an `Artifact` before archive insertion. It now prevalidates source mismatch, duplicate IDs, and duplicate detached links before changing detached artifact links or archive claim storage.

This hardening slice does not add CandidateArtifactProposalAudit, artifact generation, materialization, discovery scheduling, persistence, resolver/composition behavior, fragment activation, broad primitive-type migration, or a test-suite split.

## v28.7.2 CandidateArtifactProposal Access-Neutral Model Hardening

CandidateArtifactProposal records are access-neutral state. Public/scholar/curator/debug safety is projected at formatting time from visibility class and diagnostic-content flags; public output remains aggregate or `found: false` for inaccessible detail. Full-state validation includes candidate proposal validation.


## v28.7.3 CLI Argument View Refactor

Internal CLI parsing uses typed `CliArgs` argument views while raw `argc`/`argv` adaptation remains at `main()`.

## v28.9 ReliabilityComponents Primitive-Cluster Cleanup

Reliability construction now uses explicit `ReliabilityComponents` member assignment rather than the former nine-adjacent-double helper, preserving reliability semantics while reducing ordering risk.

The CLI boundary now adapts raw process arguments into `CliArgs` at `main()`. Internal parsing and execution operate on `std::string_view` argument views, preserving all query behavior while avoiding C-style pointer/count parsing inside the engine and removing `const_cast`-based CLI test construction.

## v28.9 ControlLayerAudit

`ControlLayerAudit` is a deterministic, read-only consolidation layer that inventories the v28 control stack. It classifies core state, validation, telemetry, planning, evaluation, proposal, audit, formatting, CLI, and mutation-capable workflows by persistence, behavior, access gates, snapshot coverage, smoke/self-test coverage, full-state-validation coverage, mutation capability, and known gaps. It is not a generation, discovery, resolver, persistence, or mutation layer.

## v28.10 Proposal Quality Gate Policy Tightening

CandidateArtifactProposalAudit classification is now policy-backed. The audit module exposes a deterministic `CandidateArtifactProposalAuditPolicy`, reason-coded findings, required revision enforcement for non-pass audits, and stronger validation for policy threshold violations. Curator/debug detail shows policy thresholds, reason codes, score-threshold comparisons, and required revisions. Public detail remains gated.

This is not a generation or mutation layer. Audit `Pass` means audit-clean only; it does not enable artifact generation, candidate materialization, discovery scheduling, public archive mutation, hidden truth mutation, resolver/composition behavior, persistence, or interactive runtime.
