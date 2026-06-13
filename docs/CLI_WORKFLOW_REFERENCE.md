# Impossible Archive MVP v28.11 — ContradictionBudget Validator-Backed Status Tightening

This package builds on v28.1 and keeps `CivilizationSpec v1.1`-selected worlds as the default runtime path while adding a release-stability foundation: named golden fixture worlds, compact `ArchiveSnapshot` summaries, and same-fixture snapshot comparison with clarified fixture/runtime metadata. Fragment records from v28.0 remain inert catalog data only. With no runtime flags, the CLI loads the bundled catalog at `examples/40_civilization_specs_v1_1.json` and selects `marsh_citadel`. The old hand-authored fixed fixture remains available as explicit regression mode through `--runtime fixed-fixture`.

Implemented milestones now included:

1. **v21.2 Candidate Runtime Hardening** — generated candidate IDs use wider deterministic digests; structured candidates do not receive validation/mediation effects from prose triggers; malformed hidden proposal types and years reject or render defensively.
2. **v22 Hidden Proposal Migration Planning** — hidden entity/event proposals can be converted into non-mutating migration plans showing projected hidden changes and affected public archive pressure.
3. **v23 Procedural Hidden Timeline Cluster Generation** — connected hidden-world slices can be generated and evaluated on a copied `HiddenTruthGraph` without live mutation.
4. **v24 Controlled Hidden Cluster Materialization** — curator/canon/debug users can explicitly insert an evaluated hidden cluster into live `HiddenTruthGraph` with fresh evaluation, snapshot/rollback, hidden-graph validation, and full archive validation.
5. **v24.1 Hidden Mutation Audit / Provenance Records** — each successful hidden-cluster materialization creates one `HiddenTruthMutationRecord`; failed, rejected, unauthorized, or duplicate materializations create no false record.
6. **v24.2 Build System + Typed Causal Links Cleanup** — the root `Makefile` is the official object-file build path, and hidden-cluster causal review metadata uses typed `ProposedCausalLink` records.
7. **v25 Public Artifact Candidates from Accepted Hidden Clusters** — successful in-memory hidden mutation records can seed public-facing `CandidateFeature` artifact candidates. The generated candidates carry optional `HiddenMutationArtifactSource` provenance, pass through the existing candidate evaluation gate, redact hidden provenance below curator access, and are **not** inserted into the public archive automatically.
8. **v26 Curator-Approved Materialization of Hidden-Mutation Artifact Candidates** — curator/canon/debug users can explicitly select one hidden-mutation-derived candidate by shape or index, freshly evaluate it, materialize it into `PublicArchive`, attach internal `MaterializedHiddenMutationArtifactProvenance`, validate full state, and roll back on failure.
9. **v26.0.1 Header Decomposition / Interface Boundary Cleanup** — split the former monolithic `impossible_archive.h` into cohesive model and API headers while keeping `impossible_archive.h` as a compatibility umbrella.
10. **v26.1 CivilizationSpec v1.1 Catalog Intake** — added `CivilizationSpec`, `CivilizationSpecCatalog`, load/validation diagnostics, schema-specific JSON parsing, catalog lookup, and inspection-only CLI queries for validating, listing, and showing specs.
11. **v26.2 Single-Civilization Bootstrap from CivilizationSpec** — added explicit `bootstrap-civilization` support for creating one valid, deterministic, minimal `ArchiveEngineState` from one selected spec without replacing the fixed fixture default.
12. **v26.3 Spec-Bootstrapped Workflow Compatibility Pass** — added opt-in spec runtime support for listing generation targets, generating candidates, and hidden-cluster review against one selected spec-bootstrapped state.
13. **v26.4 Spec-Driven Hidden Mutation / Materialization Compatibility** — extended opt-in spec runtime support to curator/canon/debug hidden-cluster materialization, mutation-derived artifact candidate generation, and selected hidden-mutation artifact candidate materialization.
14. **v26.5 Spec-Derived Artifact Quality and Maintainability Hardening** — materialization now rejects low civilization specificity, high direct-copy risk, or originality trope flags; bundled spec catalog metadata is populated; materialization access policy is documented as curator/canon/debug; and the Makefile uses compiler-generated dependency files.
15. **v26.6 CivilizationSpec v1.1 Default-Readiness / Runtime Selection Hardening** — added explicit runtime mode modeling, centralized runtime-state selection, `--runtime fixed-fixture`, nonzero CLI usage/runtime error exits, and a 5-spec workflow readiness matrix while keeping fixed fixture behavior unchanged by default.
16. **v27 CivilizationSpec-Selected Runtime Default** — makes spec-selected runtime the normal/default path using the bundled catalog and `marsh_citadel`, while demoting the fixed fixture to explicit regression mode.
17. **v27.1 CivilizationSpec v1.1.x Metadata Seam + Default-Runtime Quality Pass** — added optional spec-level `tags` and informational `profile` metadata, displays them in spec inspection, lightly validates them with warnings for duplicates/unknown informational values, and keeps generation behavior unchanged.
18. **v27.2 Catalog Metadata Vocabulary + Default-Runtime Quality Matrix** — adds an advisory tag vocabulary document, expands bundled catalog tags/profile metadata across all bundled specs, adds tag listing/filtering/validation CLI inspection queries, and runs a 10-spec default-runtime workflow matrix while keeping tags/profile metadata-only.
19. **v27.3 v1.2 Readiness Pack / Composition Design Fixtures** — adds non-runtime design fixtures for tag registry review, compatibility notes, future fragment sketches, composition sketches, and patch-path planning. No v1.2 runtime machinery, resolver, fragment loader, patch strategy, or generation behavior change is introduced.
20. **v27.4 Release Hardening / CI Readiness** — adds CI workflow configuration, sanitizer build/test target, README workflow smoke script, release checklist, architecture map, and clean-build/release-check Makefile targets. No archive features or v1.2 runtime machinery are introduced.
21. **v28.0 CivilizationSpec v1.2 Fragment Model Intake, No Resolver** — adds typed fragment, patch-strategy, and inert patch-value records; fragment JSON loading; validation; examples; and list/show/validate CLI inspection. No resolver, patch application, composition runtime, or generation behavior changes are introduced.
22. **v28.1 Golden Fixture Worlds + ArchiveSnapshot Foundation** — adds named deterministic fixture worlds, summary-level archive snapshots, snapshot comparison, and CLI inspection queries. No resolver, patch application, fragment-driven generation, persistence, or archive feature behavior changes are introduced.
23. **v28.1.1 Fixture/Snapshot Semantics Tightening** — distinguishes fixture metadata from runtime state values in `ArchiveSnapshot`, uses `summary_digest` as the snapshot digest field, rejects fixture query override flags early, and documents inert-fragment runtime isolation.
24. **v28.2 EvidencePotential / Evidence Projection Model Foundation** — adds an inert, deterministic, validatable `EvidencePotential` layer that records possible traces implied by hidden events, site/office entities, and mysteries without generating artifacts, discoveries, candidates, mutations, or fragment-driven behavior.
25. **v28.3 KnowledgeHorizon Validation Foundation** — adds deterministic validation of what artifact creators, interpreters, EvidencePotentials, and snapshot contexts can plausibly know, while keeping the layer validation-only and non-mutating.
26. **v28.3.1 KnowledgeHorizon Public Detail Access Hardening** — gates finding-detail lookup so public/scholar users cannot confirm or enumerate hidden KnowledgeHorizon finding metadata by sequential ID.
27. **v28.4 ContradictionBudget Telemetry Foundation** — adds deterministic, read-only contradiction pressure telemetry with archive/type/cause/mystery buckets, severity/status assignment, CLI inspection, and snapshot digest participation.
28. **v28.5 CandidateArtifactPlan Planning Foundation** — adds deterministic, read-only planning from EvidencePotential records to future candidate artifact shapes without generating candidates, materializing artifacts, discovering evidence, mutating archive state, activating fragments, or introducing resolver/composition behavior.
29. **v28.6 CandidateArtifactPlan Evaluation Foundation** — adds deterministic, read-only evaluation records with readiness/risk scoring and advisory gate findings for CandidateArtifactPlans, without candidate generation, artifact text generation, materialization, discovery, archive mutation, fragment activation, or resolver/composition behavior.
30. **v28.7 CandidateArtifactProposal Drafting Foundation** — adds deterministic, read-only proposal records drafted from CandidateArtifactPlanEvaluation results. Proposals include shape/type/register, claim skeleton strings, validation gates, safety, and access notes without creating artifacts, generating final artifact text, inserting claims, scheduling discovery, mutating archive state, activating fragments, or introducing resolver/composition behavior.
31. **v28.7.1 PublicArchive Invariant Hardening** — adds `PublicArchive::add_claim_to_artifact(...)` so claim insertion, artifact claim links, and voice claim links can be mutated through an archive-owned invariant boundary. The legacy detached-artifact helper is now quarantined and prevalidates failures before changing detached artifact links. No proposal audit layer, generation, materialization, discovery, persistence, resolver/composition behavior, or broad type rewrite is introduced.
32. **v28.7.2 CandidateArtifactProposal Access-Neutral Model Hardening** — removes access-specific safety from stored proposal records, stores access-neutral visibility/diagnostic facts, computes proposal safety at formatting/query time, and includes persistent proposal validation in full-state validation.
33. **v28.7.3 CLI Argument View Refactor** — moves internal CLI parsing/execution to a C++17-compatible `CliArgs` (`std::vector<std::string_view>`) argument view, keeps raw `argc`/`argv` adaptation at the executable edge, and removes `const_cast<char*>` CLI test setup without changing runtime behavior.
34. **v28.7.4 ReliabilityComponents Primitive-Cluster Cleanup** — replaces the nine-adjacent-double `components(...)` helper call sites with explicit `ReliabilityComponents` member initialization while preserving reliability values and runtime behavior.
35. **v28.8 CandidateArtifactProposal Audit / Quality Gates** — adds read-only proposal audits with quality/specificity/safety/revision-pressure scoring over CandidateArtifactProposal records without generating artifacts or mutating state.
36. **v28.9 Control-Layer Consolidation Audit** — adds deterministic ControlLayerAudit records, CLI inspection, snapshot counts, summary-digest participation, smoke/self-test coverage, and `docs/CURRENT_STATE_AUDIT.md` to classify the v28 control stack before future generation/discovery work.
37. **v28.10 Proposal Quality Gate Policy Tightening** — adds explicit CandidateArtifactProposalAuditPolicy thresholds, deterministic reason codes, required revision enforcement, and stronger validation for proposal audit decisions while keeping audits read-only and access-gated.
38. **v28.11 ContradictionBudget Validator-Backed Status Tightening** — adds explicit ContradictionBudgetPolicy thresholds, deterministic budget reason codes, too-clean archive detection, productive ambiguity classification, and stricter validation while keeping the layer advisory-only and non-mutating.

v27 changes the default runtime selection policy only. It does **not** merge civilizations, introduce multi-spec runtime state, add CivilizationSpec v1.2 fragments/compositions, generate a full artifact pool from specs, add persistence, add database storage, add external ingestion beyond the schema-specific spec file reader, add a UI/API layer, or add broad procedural world generation. The CLI remains in-memory and stateless across invocations.


## v28.11 ContradictionBudget validator-backed status tightening

ContradictionBudget now uses an explicit deterministic policy object and per-bucket reason codes. It distinguishes productive ambiguity, ritual/legal contradiction, expected damaged-evidence disagreement, protected-mystery pressure, too-clean archive slices, density/unresolved-ratio budget pressure, generation-bug pressure, KnowledgeHorizon pressure, missing causes, and invalid metrics. Over-budget remains advisory only: it does not reject mutation, repair contradictions, generate artifacts, materialize candidates, schedule discoveries, mutate hidden truth, mutate public archive state, activate fragments, add resolver/composition behavior, add persistence, or add interactive runtime behavior.

Curator/debug bucket detail exposes policy thresholds and reason codes. Public output remains aggregate/safe and does not expose diagnostic reason-code material. `ArchiveSnapshot` includes watch, too-clean, productive-ambiguity, over-budget, generation-bug, and reason-code digest material.

```bash
./impossible_archive_mvp_v28_11 --query contradiction-budget-summary
./impossible_archive_mvp_v28_11 --query validate-contradiction-budget
./impossible_archive_mvp_v28_11 --access curator --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive
./impossible_archive_mvp_v28_11 --query control-layer-audit-summary
./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive
./impossible_archive_mvp_v28_11 --query compare-archive-snapshots --fixture-id fixture.default_archive
```


## v28.10 Proposal quality gate policy tightening

CandidateArtifactProposalAudit now uses an explicit deterministic policy object. Curator/debug audit detail shows policy thresholds, score-threshold comparisons, reason codes, and required revisions. Public detail remains gated. This slice is policy-only: `Pass` means audit-clean only and does not enable generation, materialization, discovery scheduling, public archive mutation, hidden truth mutation, fragment activation, resolver/composition behavior, persistence, or interactive runtime.

```bash
./impossible_archive_mvp_v28_11 --query candidate-artifact-proposal-audit-summary
./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-proposal-audits
./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-proposal-audit --candidate-artifact-proposal-audit-id candidate_artifact_proposal_audit.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0019.administrative_docket
./impossible_archive_mvp_v28_11 --query control-layer-audit-summary
```


## v28.9 Control-layer consolidation audit

ControlLayerAudit records classify the v28 control stack by persistence, behavior, access gating, snapshot coverage, smoke/self-test coverage, full-state validation coverage, mutation capability, inert/advisory status, and known gaps. This is audit/consolidation only. The engine is not release-ready; artifact generation and discovery expansion remain deferred. See `docs/CURRENT_STATE_AUDIT.md`.

```bash
./impossible_archive_mvp_v28_11 --query control-layer-audit-summary
./impossible_archive_mvp_v28_11 --query validate-control-layer-audit
./impossible_archive_mvp_v28_11 --access curator --query list-control-layer-audit-entries
./impossible_archive_mvp_v28_11 --access curator --query show-control-layer-audit-entry --control-layer-audit-entry-id control_layer.candidate_artifact_proposal_audit
```

This slice deliberately does not add artifact generation, artifact text generation, proposal materialization, discovery scheduling, persistence, resolver/composition behavior, fragment activation, interactive runtime behavior, public archive mutation, or hidden truth mutation.


## v28.7.3 CLI argument view refactor

Internal CLI parsing and execution use `CliArgs`, a C++17-compatible `std::vector<std::string_view>` argument view. Raw `argc`/`argv` handling is isolated to `main()` and compatibility wrappers, while self-tests exercise the typed argument-view path directly. This removes `const_cast<char*>` test setup and keeps query behavior, access gates, expected failure paths, and smoke workflows unchanged.


## v28.7.2 CandidateArtifactProposal access-neutral model hardening

CandidateArtifactProposal now stores access-neutral domain facts only. Stored proposal records carry a visibility class and diagnostic-content flags, while public/scholar/curator/debug safety is computed during formatting/query projection. Proposal drafting into state under public, scholar, curator, or debug access produces equivalent stored proposal records and stable snapshot digests. Full-state validation now includes persistent CandidateArtifactProposal validation.

Public proposal detail remains access-gated and does not expose source EvidencePotential IDs, plan/evaluation IDs, KnowledgeHorizon or ContradictionBudget diagnostics, protected mystery details, hidden rationale, or curator-only warnings. Curator/debug detail continues to expose full diagnostics.


`PublicArchive` continues to own the preferred claim/artifact relationship mutation path through `PublicArchive::add_claim_to_artifact(...)`. The method validates the target artifact, claim source artifact, duplicate claim ID, and existing artifact links before inserting the claim and linking it to the artifact. Voice claim links are added only when the requested voice weight is positive.

The older detached-artifact construction helper remains only as a quarantined compatibility path for existing fixture and materialization builders that assemble an `Artifact` value before insertion. It now prevalidates source mismatch, duplicate IDs, and duplicate detached links before mutating the detached artifact or archive.

Regression coverage verifies missing-artifact rejection, duplicate-claim rejection, source-artifact mismatch rejection, positive-weight voice linking, zero-weight no-voice insertion, existing workflow compatibility, and deterministic snapshots. This slice deliberately does not add CandidateArtifactProposalAudit, artifact generation, materialization, discovery scheduling, persistence, resolver/composition behavior, fragment activation, a primitive-type rewrite, or a test-suite split.

## v28.7 CandidateArtifactProposal drafting remains proposal-only

CandidateArtifactProposal records are inert drafts produced from CandidateArtifactPlanEvaluation records. They may describe a proposed artifact shape, type, voice register, claim skeletons, damage/distortion modes, and validation gates, but they do not create `Artifact` records, generate final artifact prose, insert `PublicClaim` records, schedule discoveries, mutate hidden truth, mutate the public archive, activate fragments, or resolve/spec-compose anything.

```bash
./impossible_archive_mvp_v28_11 --query candidate-artifact-proposal-summary
./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-proposals
./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-proposals
./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-proposal --candidate-artifact-proposal-id candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0019.administrative_docket
```

Public and scholar proposal formatting remains access-filtered: hidden source IDs, KnowledgeHorizon/ContradictionBudget diagnostic IDs, protected mystery details, blocking internals, and curator-only rationale remain restricted.

## v28.0 fragment intake remains inert

Fragment records are data-only. They can be loaded, inspected, and validated from `examples/v12_fragment_examples.json`, but they cannot alter runtime selection, bootstrap, generation, materialization, or archive state. Strict `validate-civilization-fragments` reports invalid fragment records; normal spec-selected runtime bootstrap does not fail solely because inert fragments are invalid unless the user explicitly requests fragment validation.

```bash
./impossible_archive_mvp_v28_11 --query validate-civilization-fragments --spec-file examples/v12_fragment_examples.json
./impossible_archive_mvp_v28_11 --query list-civilization-fragments --spec-file examples/v12_fragment_examples.json
./impossible_archive_mvp_v28_11 --query show-civilization-fragment --spec-file examples/v12_fragment_examples.json --fragment-id fragment.high_artifact_density
```

## v28.1.1 golden fixtures and snapshots

Golden fixtures are named deterministic test worlds. `ArchiveSnapshot` is a compact summary-level digest and count report over an `ArchiveEngineState`; it is not persistence and it does not expose hidden IDs in public formatting. Snapshots distinguish `fixture_seed` from `state_seed` and `fixture_archive_year` from `effective_archive_year`. `summary_digest` is a compact regression tripwire, not a full semantic golden-state digest.

```bash
./impossible_archive_mvp_v28_11 --query list-golden-fixtures
./impossible_archive_mvp_v28_11 --query show-golden-fixture --fixture-id fixture.default_archive
./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive
./impossible_archive_mvp_v28_11 --query compare-archive-snapshots --fixture-id fixture.default_archive
./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.fragment_catalog_only
```

`compare-archive-snapshots` rebuilds the selected golden fixture twice and compares the resulting summary snapshots. It is a same-fixture determinism check, not an arbitrary two-snapshot comparison command. Fixture-backed queries reject `--spec-file`, `--civilization-id`, `--seed`, and `--archive-year` before fixture construction or spec-file loading because the fixture definition already fixes those values.

## v28.2 EvidencePotential projection seam

`EvidencePotential` is an inert model layer between hidden truth and public/discovered evidence. It describes plausible future traces such as ledger records, ritual traces, material deposits, copy traditions, and absences. It does not create artifacts, schedule discoveries, affect candidate generation, mutate public archive state, mutate hidden truth, activate fragments, or resolve v1.2 compositions.

```text
hidden truth
-> evidence potential
-> discovered evidence
-> public claims
-> interpretation
```

Inspection commands:

```bash
./impossible_archive_mvp_v28_11 --query evidence-potential-summary
./impossible_archive_mvp_v28_11 --access curator --query list-evidence-potentials
./impossible_archive_mvp_v28_11 --access curator --query show-evidence-potential --evidence-potential-id evidence_potential.0000
./impossible_archive_mvp_v28_11 --query validate-evidence-potentials
```

Public formatting reports aggregate counts and avoids hidden-source IDs or hidden rationales. Curator/debug formatting exposes source type, source ID, rationale, likely sites/creators, and likely distortions for inspection. `ArchiveSnapshot` includes `evidence_potential_count`, and `summary_digest` includes stable EvidencePotential summary material.

## v28.3 KnowledgeHorizon validation seam

`KnowledgeHorizon` is a validation-only layer that checks whether a subject could plausibly be known from a given actor/date/access/transmission context. It validates artifact creator references, interpreter/theory citations, and EvidencePotential public-safety boundaries. It does not generate artifacts, schedule discoveries, mutate public or hidden state, activate fragments, resolve specs, apply patches, or introduce persistence.

```bash
./impossible_archive_mvp_v28_11 --query validate-knowledge-horizon
./impossible_archive_mvp_v28_11 --query knowledge-horizon-summary
./impossible_archive_mvp_v28_11 --access curator --query list-knowledge-horizon-findings
./impossible_archive_mvp_v28_11 --access curator --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0000
```

Public formatting is aggregate-only and hides hidden IDs/explanations. KnowledgeHorizon finding IDs and detailed records are curator/debug diagnostics; public and scholar users receive `found: false` from `show-knowledge-horizon-finding` for hidden or otherwise inaccessible IDs, so detail lookup cannot be used to enumerate validation metadata. Curator/debug formatting exposes deterministic finding IDs, context IDs, subject IDs, statuses, mediation, and explanations. `ArchiveSnapshot` includes `knowledge_horizon_finding_count` and `knowledge_horizon_error_count`, and `summary_digest` includes stable KnowledgeHorizon status/type material.



## v28.4 ContradictionBudget telemetry seam

`ContradictionBudget` is a read-only telemetry layer that observes contradiction pressure in the current single-shot runtime invocation. It summarizes archive-level, type-level, cause-level, and mystery-linked contradiction pressure, including contradiction density, unresolved ratio, generation-bug ratio, protected-mystery pressure, and KnowledgeHorizon error pressure.

It is telemetry only. It does not reject, repair, generate, mutate, materialize, discover, resolve, persist, or introduce interactive session behavior. It measures only the `ArchiveEngineState` visible to the current command invocation.

```bash
./impossible_archive_mvp_v28_11 --query contradiction-budget-summary
./impossible_archive_mvp_v28_11 --query validate-contradiction-budget
./impossible_archive_mvp_v28_11 --runtime fixed-fixture --access curator --query list-contradiction-budget-buckets
./impossible_archive_mvp_v28_11 --runtime fixed-fixture --access curator --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive
```

Public formatting exposes aggregate counts, severity, status, and validation result only. Curator/debug formatting exposes representative contradiction IDs, scope IDs, reason codes, policy thresholds, and diagnostic notes. `ArchiveSnapshot` includes `contradiction_budget_bucket_count`, `contradiction_budget_over_budget_count`, `contradiction_budget_watch_count`, `contradiction_budget_too_clean_count`, `contradiction_budget_productive_ambiguity_count`, and `contradiction_budget_generation_bug_count`; `summary_digest` includes stable ContradictionBudget bucket/status/severity/reason-code material.


## v28.6 CandidateArtifactPlan planning seam

CandidateArtifactPlan is an inert planning bridge from EvidencePotential to future artifact-candidate evaluation. It describes a plausible future artifact shape, role, validation checklist, risk level, and diagnostic references. It does not generate candidate artifacts, materialize artifacts, discover evidence, mutate hidden truth, mutate the public archive, activate fragments, or introduce resolver/composition behavior.

```bash
./impossible_archive_mvp_v28_11 --query candidate-artifact-plan-summary
./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-plans
./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-plans
./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-plan --candidate-artifact-plan-id candidate_artifact_plan.evidence_potential.0019.administrative_docket
```

Public access receives aggregate counts and public-safe summaries only. Curator/debug access can inspect source EvidencePotential IDs, KnowledgeHorizon finding IDs, ContradictionBudget bucket IDs, validation steps, warnings, and rationale. `current_materialization_enabled` is always false.

## v28.6 CandidateArtifactPlan evaluation seam

v28.6 evaluates existing CandidateArtifactPlans only. A `Pass` decision means evaluation-clean; it does **not** enable generation or materialization. `current_generation_enabled` and `current_materialization_enabled` remain false and are validation errors if enabled.

```bash
./impossible_archive_mvp_v28_11 --query candidate-artifact-plan-evaluation-summary
./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-plan-evaluations
./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-plan-evaluations
./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-plan-evaluation --candidate-artifact-plan-evaluation-id candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0019.administrative_docket
```

Public access receives aggregate evaluation counts only unless an evaluation is explicitly public-safe. Curator/debug access can inspect plan IDs, advisory gate findings, related diagnostic IDs, risk/readiness scores, and required next checks.

## Build

Preferred object build:

```bash
make build
```

Run the regression suite:

```bash
make test
```

Strict warning build and self-test:

```bash
make strict
```

Clean generated build artifacts:

```bash
make clean
```

C++17 compatibility path:

```bash
make clean
make CXXSTD=c++17 test
```

Manual fallback:

```bash
mkdir -p build
g++ -std=c++20 -Wall -Wextra -pedantic -O0 -c src/*.cpp
mv *.o build/
g++ build/*.o -o impossible_archive_mvp_v28_11
./impossible_archive_mvp_v28_11 --self-test
```


## v27.4 release hardening

Release-oriented commands:

```bash
make clean test
make clean CXXSTD=c++17 test
make clean strict
make clean sanitize
make smoke
make release-check
```

The smoke script runs the README workflow surface and expected failure checks:

```bash
scripts/smoke_test_readme_workflows.sh
```

Release support files:

- `.github/workflows/ci.yml` — C++20, C++17, and strict warning CI jobs.
- `scripts/smoke_test_readme_workflows.sh` — fail-fast CLI smoke tests for documented workflows.
- `RELEASE_CHECKLIST.md` — repeatable release gate checklist.
- `docs/ARCHITECTURE_MAP.md` — concise map of implemented runtime paths and mutation gates.

v27.4 is a stability slice. It does not add new archive features, v1.2 runtime symbols, persistence, UI/API layers, cross-civilization merge, or multi-spec runtime state.

## Example commands

```bash
./impossible_archive_mvp_v28_11 --query list-generation-targets
./impossible_archive_mvp_v28_11 --access scholar --query generate-candidates --target-topic authority_conflict_0 --target-year 625 --seed 42
./impossible_archive_mvp_v28_11 --access curator --query hidden-cluster --cluster-scope institution_origin --target-topic authority_conflict_0 --start-year 590 --end-year 625 --seed 42
./impossible_archive_mvp_v28_11 --access curator --query materialize-hidden-cluster --cluster-scope institution_origin --target-topic authority_conflict_0 --start-year 590 --end-year 625 --seed 42
./impossible_archive_mvp_v28_11 --access curator --query generate-artifacts-from-hidden-mutation --cluster-scope institution_origin --target-topic authority_conflict_0 --start-year 590 --end-year 625 --target-year 625 --seed 42
./impossible_archive_mvp_v28_11 --access curator --query materialize-hidden-mutation-artifact-candidate --candidate-shape ritual_notice --cluster-scope institution_origin --target-topic authority_conflict_0 --start-year 590 --end-year 625 --target-year 625 --seed 42
./impossible_archive_mvp_v28_11 --runtime fixed-fixture --access curator --query hidden-cluster --cluster-scope institution_origin --target-topic lock_authority --start-year 590 --end-year 625 --seed 42
```


## CivilizationSpec v1.1 inspection, bootstrap, and default runtime

The `CivilizationSpec` validate/list/show queries remain inspection-only. Runtime workflows now default to one selected spec: bundled catalog plus `marsh_citadel`. `--runtime spec-selected` may be supplied explicitly, `--civilization-id ash_steppe` selects another civilization from the bundled catalog, and `--spec-file custom.json --civilization-id custom_id` selects a custom catalog/spec. `--runtime fixed-fixture` is the explicit regression path. Catalog loading still accepts one or more specs through the narrow internal JSON reader. Empty catalogs are invalid; duplicate civilization IDs are invalid; no code assumes a fixed 40-spec catalog size.

```bash
./impossible_archive_mvp_v28_11 --query validate-civilization-specs --spec-file examples/40_civilization_specs_v1_1.json
./impossible_archive_mvp_v28_11 --query list-civilization-specs --spec-file examples/40_civilization_specs_v1_1.json
./impossible_archive_mvp_v28_11 --query show-civilization-spec --spec-file examples/40_civilization_specs_v1_1.json --civilization-id marsh_citadel
./impossible_archive_mvp_v28_11 --query bootstrap-civilization --spec-file examples/40_civilization_specs_v1_1.json --civilization-id marsh_citadel
./impossible_archive_mvp_v28_11 --access curator --query bootstrap-civilization --spec-file examples/40_civilization_specs_v1_1.json --civilization-id ash_steppe
```

The bundled `examples/40_civilization_specs_v1_1.json` is an external stress fixture for validation, not a required catalog size. One-spec and two-spec catalogs are valid when their contents satisfy the same schema and semantic rules.

## v27.2 CivilizationSpec tag vocabulary and catalog inspection

`CivilizationSpec` now accepts optional root-level `tags` and an optional informational `profile` block:

```json
"tags": ["river_delta", "urban", "water_access"],
"profile": {
  "target_depth": "prototype",
  "future_modules": ["fragment_composition", "language_drift", "artifact_voice"]
}
```

These fields are inspection metadata only in v27.x. Existing v1.1 catalogs without them still load and validate. Duplicate tags/future modules and unknown informational profile values produce warnings rather than hard errors. Generation, bootstrap, hidden-cluster, mutation, and materialization workflows do not branch on tags/profile.

The bundled example catalog now includes tags/profile metadata for all bundled specs. `docs/CIVILIZATION_TAG_VOCABULARY.md` documents an advisory vocabulary for geography/ecology, settlement/polity, artifact ecology, truth/mystery, strangeness/constraints, and technology/era tags. The vocabulary is guidance, not a hard schema lock.

Catalog tag inspection commands:

```bash
./impossible_archive_mvp_v28_11 --query list-civilization-tags
./impossible_archive_mvp_v28_11 --query list-civilizations-by-tag --tag river_delta
./impossible_archive_mvp_v28_11 --query validate-civilization-tags
```

## v27.3 v1.2 readiness design fixtures

v27.3 adds documentation-only readiness material for eventual v1.2 design review. These documents do not define runtime schemas, parser inputs, resolver behavior, or generation switches.

- `docs/CIVILIZATION_TAG_REGISTRY.md` — controlled registry derived from the tags present in the bundled 40-spec catalog.
- `docs/CIVILIZATION_COMPATIBILITY_NOTES.md` — compatible tag groups, advisory conflict pairs, and future fragment-family notes.
- `docs/V12_FRAGMENT_CANDIDATE_SKETCHES.md` — 15 non-runtime fragment candidate sketches.
- `docs/V12_COMPOSITION_EXAMPLES.md` — 5 non-runtime composition example sketches.
- `docs/PATCH_PATH_REGISTRY_DRAFT.md` — patch-path planning checklist and candidate field-family notes.

These are content/design fixtures only. v27.3 does not add `CivilizationSpecFragment`, `CivilizationSpecComposition`, `SpecPatch`, `PatchStrategy`, `CivilizationSpecResolver`, `ResolutionTrace`, fragment loading, composition resolving, tag-driven generation, cross-civilization merge, or multi-spec runtime state.

## v27 runtime selection behavior

Spec-selected runtime is now the default and remains all-or-nothing: runtime workflows use either one selected spec-bootstrapped state or, when explicitly requested, the fixed fixture regression state. Supplying `--spec-file` without `--civilization-id` is a usage error. Supplying `--civilization-id` without `--spec-file` uses the bundled catalog under spec-selected runtime. Unsupported spec-runtime queries return exit status `1`.

```bash
./impossible_archive_mvp_v28_11 --query list-generation-targets --spec-file examples/40_civilization_specs_v1_1.json --civilization-id marsh_citadel
./impossible_archive_mvp_v28_11 --access scholar --query generate-candidates --spec-file examples/40_civilization_specs_v1_1.json --civilization-id marsh_citadel --target-topic authority_conflict_0 --target-year 625 --seed 42
./impossible_archive_mvp_v28_11 --access curator --query hidden-cluster --spec-file examples/40_civilization_specs_v1_1.json --civilization-id ash_steppe --target-topic authority_conflict_1 --start-year 250 --end-year 380 --seed 42
./impossible_archive_mvp_v28_11 --access curator --query hidden-cluster --spec-file examples/40_civilization_specs_v1_1.json --civilization-id glass_delta --target-topic institution_pilot_guild --start-year 300 --end-year 430 --seed 42
```

Spec-derived target names include `authority_conflict_N`, `institution_<key>`, `site_<key>`, `recordkeeping`, and `mystery_seed`. Public/scholar output shows public-safe labels and decisions; curator/canon/debug output includes source entity/event trace IDs.

## v26 hidden-mutation artifact materialization

The v25.1 generator still produces three evaluated `CandidateFeature` shapes from a successful in-memory hidden mutation record:

- Administrative docket — a lockhouse/archive document derived from accepted lower-lock context.
- Ritual notice — a ceremonial/procedural public notice from the same source context.
- Later scholar fragment — a copied or cataloged fragment that references public consequences without exposing hidden source IDs.

v26 adds an explicit `materialize-hidden-mutation-artifact-candidate` path. The command internally creates the hidden mutation, generates the candidate set, selects one candidate by `--candidate-shape` or `--candidate-index`, freshly evaluates it, and inserts exactly that public artifact/claims only if curator/canon/debug access and validation succeed. Public/scholar attempts remain non-mutating and redacted.

In v26.5+, insertable evaluation is necessary but no longer sufficient: materialization also requires originality quality. Candidates with `civilization_specificity_score < 0.30`, `direct_copy_risk_score >= 0.35`, or any originality trope flags remain visible for review but are rejected before public artifact, claim, or discovery insertion.

## Invariants to preserve

- Generation proposes.
- Evaluation judges.
- Candidate generation from hidden mutations is non-mutating.
- Generated public artifacts from hidden mutations remain candidates first.
- Candidate evaluation gates are reused, not bypassed.
- Archive candidate materialization mutates public archive only with curator/canon/debug authority.
- Hidden proposal planning does not mutate `HiddenTruthGraph`.
- Hidden cluster generation/evaluation does not mutate `HiddenTruthGraph`.
- Hidden cluster materialization requires curator/canon/debug access, fresh evaluation, validation, rollback, and audit recording.
- `Event::cause_event_ids` remains the hidden-graph causal source of truth.
- `GeneratedHiddenTimelineCluster::causal_links` remains typed review/display metadata.
- Successful hidden-cluster materialization creates exactly one audit record.
- Failed, rejected, unauthorized, or duplicate hidden-cluster materialization creates no false audit record.
- Public/scholar output must not expose hidden proposal, hidden cluster, hidden mutation, hidden entity, or hidden event internals.
- Structured candidate fields override prose; prose heuristics are legacy-only.
- The CLI is stateless between invocations; v27 intentionally does not add persistence.

## v26.0.1 header layout

`src/impossible_archive.h` is now an umbrella compatibility header over:

- `archive_common.h`
- `hidden_truth_model.h`
- `public_archive_model.h`
- `candidate_model.h`
- `archive_engine_state.h`
- `validation_api.h`
- `candidate_generation_api.h`
- `hidden_workflows_api.h`
- `materialization_api.h`
- `archive_views_api.h`
- `cli_model.h`
- `civilization_spec_model.h`
- `civilization_specs_api.h`

Existing translation units may continue to include `impossible_archive.h`; new code can include narrower headers when useful. `src/civilization_specs.cpp` implements the v26.1 schema-specific loader, validator, lookup, and formatting API.


## v26.4/v26.5 spec-driven hidden mutation/materialization compatibility

These workflows remain explicit and access-gated. Public/scholar users cannot materialize hidden clusters or hidden-mutation artifact candidates.

```bash
./impossible_archive_mvp_v28_11 --access curator --query materialize-hidden-cluster --spec-file examples/40_civilization_specs_v1_1.json --civilization-id marsh_citadel --target-topic authority_conflict_0 --cluster-scope institution_origin --start-year 250 --end-year 380 --seed 42
./impossible_archive_mvp_v28_11 --access curator --query generate-artifacts-from-hidden-mutation --spec-file examples/40_civilization_specs_v1_1.json --civilization-id ash_steppe --target-topic authority_conflict_1 --cluster-scope institution_origin --start-year 250 --end-year 380 --target-year 390 --seed 42
./impossible_archive_mvp_v28_11 --access curator --query materialize-hidden-mutation-artifact-candidate --spec-file examples/40_civilization_specs_v1_1.json --civilization-id glass_delta --target-topic institution_pilot_guild --cluster-scope institution_origin --start-year 250 --end-year 380 --target-year 390 --candidate-shape ritual_notice --seed 42
```
