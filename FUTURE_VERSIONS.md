# Future Versions Roadmap

This file records version history and recommended future slices after the current v28.11 baseline. It is a roadmap, not active runtime behavior.

## Current baseline

```text
v28.11 — ContradictionBudget Validator-Backed Status Tightening
```

The current engine is in v28 control-layer consolidation. It has a deterministic, access-aware, read-only/advisory chain from hidden truth through EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, and ControlLayerAudit.

The current baseline still does not include artifact text generation, discovery expansion, proposal materialization, EvidencePotential-to-artifact conversion, persistence, resolver/composition behavior, or interactive runtime behavior.

## Implemented historical notes

### v24.1 — Hidden Mutation Audit / Provenance Records

Implemented first-class provenance for successful hidden-truth mutation. Failed, rejected, unauthorized, or duplicate materializations create no false record, and replay serialization includes `M|...` mutation records.

### v24.2 — Build System + Typed Causal Links Cleanup

Implemented the root Makefile and typed `ProposedCausalLink` records for hidden-cluster causal review metadata.

### v25 — Public Artifact Candidates from Accepted Hidden Clusters

Implemented non-mutating public artifact candidate generation from successful hidden mutation records.

### v25.1 — Hidden-Mutation Candidate Hardening

Implemented three deterministic mutation-derived candidate shapes, compact public-safe summaries, stronger source diagnostics, and curator/canon/debug trace formatting.

### v26 — Curator-Approved Materialization of Hidden-Mutation Artifact Candidates

Implemented explicit curator/canon/debug materialization of one selected hidden-mutation-derived candidate into `PublicArchive`, with fresh evaluation, provenance, validation, rollback, duplicate rejection, and public redaction.

### v26.0.1 — Header Decomposition / Interface Boundary Cleanup

Split the former monolithic `impossible_archive.h` into cohesive model/API headers while retaining the umbrella compatibility header.

### v26.1 — CivilizationSpec v1.1 Catalog Intake

Implemented inspection-only `CivilizationSpec`/catalog models, dependency-free schema-specific JSON loading, load diagnostics, validation, catalog lookup, and validate/list/show CLI commands. Catalogs of one or more specs are supported.

### v26.2 — Single-Civilization Bootstrap from Selected CivilizationSpec

Implemented explicit `bootstrap-civilization` support that builds one minimal deterministic `ArchiveEngineState` from one selected spec and preserves the fixed fixture as the default runtime at that stage.

### v26.3 — Spec-Bootstrapped Workflow Compatibility Pass

Implemented explicit opt-in spec runtime support for `list-generation-targets`, `generate-candidates`, and `hidden-cluster`.

### v26.4 — Spec-Driven Hidden Mutation / Materialization Compatibility

Implemented opt-in spec runtime support for mutation-bearing workflows while preserving fixed-fixture defaults, curator/canon/debug gates, provenance, validation, rollback, and public redaction.

### v26.5 — Spec-Derived Artifact Quality and Maintainability Hardening

Implemented materialization quality gates for low civilization specificity, high direct-copy risk, and originality trope flags. Added catalog metadata to the bundled fixture, formatted missing metadata as `unspecified`, clarified materialization access as curator/canon/debug, added Makefile dependency generation, and began replacing umbrella includes in CivilizationSpec/bootstrap implementation files.

### v26.6 — CivilizationSpec v1.1 Default-Readiness / Runtime Selection Hardening

Implemented explicit runtime mode modeling, centralized runtime-state selection, `--runtime fixed-fixture`, nonzero CLI usage/runtime error exits, and a representative five-spec workflow matrix while keeping spec runtime opt-in and fixed fixture behavior unchanged by default.

### v27 — CivilizationSpec-Selected Runtime Default

Implemented the runtime default cutover: runtime workflows now select the bundled CivilizationSpec catalog and `marsh_citadel` by default, while `--runtime fixed-fixture` preserves the legacy fixture as explicit regression mode.

### v27.1 — CivilizationSpec v1.1.x Metadata Seam + Default-Runtime Quality Pass

Implemented optional `tags` and informational `profile` metadata for `CivilizationSpec`, including loader support, light validation, spec inspection formatting, and representative bundled-catalog metadata.

### v27.2 — Catalog Metadata Vocabulary + Default-Runtime Quality Matrix

Implemented as a metadata-only catalog-quality pass: advisory tag vocabulary documentation, bundled-catalog tags/profile coverage, tag listing/filtering/validation CLI queries, and a 10-spec default-runtime workflow matrix. Tags/profile remain inspection metadata only and do not affect generation behavior.

### v27.3 — v1.2 Readiness Pack / Composition Design Fixtures

Implemented documentation-only readiness fixtures for future v1.2 design review: a derived tag registry, compatibility notes, non-runtime fragment candidate sketches, non-runtime composition examples, and a draft patch-path registry. No v1.2 runtime symbols, fragment loader, composition resolver, patch strategies, CLI resolve commands, tag-driven generation, cross-civilization merge, or multi-spec runtime state were added.

### v27.4 — Release Hardening / CI Readiness

Implemented clean-build release hardening: CI workflow configuration, sanitizer build/test target, README workflow smoke script, release checklist, architecture map, and explicit Makefile release-check target. No archive features or v1.2 runtime machinery were added.

### v28.0 — CivilizationSpec v1.2 Fragment Model Intake, No Resolver

Implemented the first bounded v1.2 slice: fragment records, categories, inert patch strategies, restricted patch values, fragment loading/validation, fragment examples, and list/show/validate CLI inspection. No composition records, resolver, patch application, resolved spec output, generation behavior based on fragments/tags, cross-civilization merge, or multi-spec runtime state were added.

### v28.1 — Golden Fixture Worlds + ArchiveSnapshot Foundation

Implemented a deterministic regression foundation: named golden fixture worlds, summary `ArchiveSnapshot` generation, snapshot comparison, and CLI inspection queries. Fragments remain inert catalog data and do not affect runtime selection, generation, target resolution, materialization, or archive mutation.

### v28.1.1 — Fixture/Snapshot Semantics Tightening

Clarified fixture/runtime metadata in `ArchiveSnapshot`, standardized `summary_digest`, rejected fixture override flags early, and documented inert-fragment runtime isolation.

### v28.2 — EvidencePotential / Evidence Projection Model Foundation

Implemented an inert EvidencePotential layer: deterministic derivation from current runtime state, validation, CLI inspection, fixture integration, snapshot counting, and summary digest participation. No artifacts are generated from potentials, no discovery behavior changes, and fragments remain inert catalog data.

### v28.3 — KnowledgeHorizon Validation Foundation

Implemented deterministic validation for what actors, artifacts, interpretations, EvidencePotential derivation, and snapshot contexts could plausibly know at a given time. No composition resolution, patch application, or EvidencePotential-driven artifact generation was added.

### v28.3.1 — KnowledgeHorizon Public Detail Access Hardening

Implemented a corrective access-control patch: KnowledgeHorizon finding IDs and detailed records are curator/debug diagnostics, and public/scholar detail lookup returns `found: false` for hidden or inaccessible findings. Public aggregate summaries remain available.

### v28.4 — ContradictionBudget Telemetry Foundation

Implemented deterministic, read-only ContradictionBudget telemetry for archive-level, contradiction-type, contradiction-cause, and mystery-linked pressure. The layer participates in CLI inspection, validation, snapshot counts, and `summary_digest` material without enforcing budgets, repairing contradictions, generating artifacts, changing discovery, activating fragments, or adding persistence.

### v28.5 — EvidencePotential to CandidateArtifactPlan Planning

Implemented gated planning only. CandidateArtifactPlan records describe plausible future artifact shapes from EvidencePotential records without materializing, discovering, or mutating archive state.

### v28.6 — CandidateArtifactPlan Evaluation, No Archive Mutation

Implemented CandidateArtifactPlanEvaluation records that evaluate plans without generating artifacts, materializing candidates, mutating archive state, activating fragments, or introducing resolver/composition behavior.

### v28.7 — CandidateArtifactProposal Drafting, No Archive Mutation

Implemented inert proposal records from evaluations. Proposals record proposed shape/type/register, claim skeletons, validation gates, safety, and access notes without creating artifacts, generating final artifact text, inserting claims, scheduling discoveries, mutating state, activating fragments, or introducing resolver/composition behavior.

### v28.7.1 — PublicArchive Invariant Hardening

Implemented a corrective Core Guidelines hardening slice before adding more proposal/reporting layers. `PublicArchive::add_claim_to_artifact(...)` owns the preferred claim insertion relationship boundary, including artifact claim links and positive-weight voice claim links. The detached artifact helper remains only as a quarantined compatibility path and prevalidates failure cases before mutation.

### v28.7.2 — CandidateArtifactProposal Access-Neutral Model Hardening

Made CandidateArtifactProposal stored state access-neutral. Access-specific proposal safety is computed at formatting/query time, and full-state validation includes persistent proposals.

### v28.7.3 — CLI Argument View Refactor

Moved internal CLI parsing and execution from raw `argc`/`argv` pointer-count interfaces to a typed `CliArgs` argument view. Raw argv adaptation remains at the executable boundary. This is interface cleanup only and does not change runtime behavior.

### v28.7.4 — ReliabilityComponents Primitive-Cluster Cleanup

Replaced the former nine-adjacent-double helper with explicit `ReliabilityComponents` member initialization while preserving reliability semantics.

### v28.8 — CandidateArtifactProposal Audit / Proposal Quality Gates, No Archive Mutation

Implemented read-only proposal audits with quality/specificity/safety/revision-pressure scoring over CandidateArtifactProposal records without generating artifacts or mutating state.

### v28.9 — Control-Layer Consolidation Audit

Implemented deterministic ControlLayerAudit records, CLI inspection, snapshot counts, summary-digest participation, smoke/self-test coverage, and `docs/CURRENT_STATE_AUDIT.md` to classify the v28 control stack before future generation/discovery work.

### v28.10 — Proposal Quality Gate Policy Tightening, No Mutation

Implemented policy hardening for CandidateArtifactProposalAudit. Audit decisions use explicit deterministic thresholds, findings carry reason codes, non-pass audits require actionable revisions, and validation rejects policy-inconsistent audit states. This remains no-mutation and no-generation.

### v28.11 — ContradictionBudget Validator-Backed Status Tightening, No Mutation

Implemented explicit ContradictionBudgetPolicy thresholds, deterministic reason codes, too-clean archive detection, productive ambiguity classification, generation-bug pressure classification, and stricter validation while keeping the layer advisory-only and non-mutating.

## Recommended next slices

### v28.12 — Documentation / Roadmap Reconciliation and Release-Gate Parity Prep

Purpose: reconcile stale roadmap/audit documentation with the v28.11 baseline and prepare the repository for the next feature-shaped slice.

Allowed work:

```text
Update docs that still describe v28.9/v28.10 as current or future.
Clarify that v28.11 is current.
Clarify that v29.0 is the next feature-shaped target.
Record CI parity and self-test structure as follow-up hardening work.
Do not change runtime behavior.
```

### v28.13 — CI Parity / Test-Structure Hardening, No Runtime Behavior Change

Purpose: bring GitHub Actions closer to the local release gate and make the monolithic self-test suite easier to maintain.

Allowed work:

```text
Add CI coverage for smoke and sanitizer paths where practical.
Split self-tests by subsystem without removing assertions.
Keep CLI/runtime behavior unchanged.
```

### v28.14 — Centralized Diagnostic Detail Access Gates, No Output Expansion

Purpose: reduce hidden/diagnostic leak risk before v29 adds richer draft/detail surfaces.

Allowed work:

```text
Introduce small access-policy helpers for diagnostic detail surfaces.
Migrate KnowledgeHorizon, ContradictionBudget, CandidateArtifactProposalAudit, and ControlLayerAudit detail gates incrementally.
Keep public/scholar output at least as restrictive as today.
```

### v29.0 — CandidateArtifactDraft / Text Outline, No Artifact Insertion

Purpose: introduce a draft-outline layer derived from audited CandidateArtifactProposal records.

Allowed work:

```text
Create CandidateArtifactDraft records from valid proposal/audit chains.
Store outline titles, intended artifact type/register, claim-outline lines, and required validation gates.
Expose public-safe summaries and curator/debug diagnostics.
Add validation, snapshot counts, smoke coverage, and self-tests.
```

Non-goals:

```text
No Artifact insertion.
No PublicClaim insertion.
No discovery scheduling.
No hidden truth mutation.
No public archive mutation.
No persistence.
No resolver/composition behavior.
No final artifact prose generation.
```

## Later hardening passes

```text
Header hygiene: continue migrating implementation files from the umbrella include to narrower headers when useful.
Indexing: add indexes for claims/entities/contradictions only after data volume warrants it.
Serialization: add a stable external state format only after materialization and generated candidates are stable.
External ingestion: accept typed candidate records from controlled JSON/YAML only after diagnostics and no-default semantics are explicit.
Access-policy centralization: replace distributed formatting/query gates with explicit policy helpers over time.
```

## Deferred until after v29 draft stabilization

```text
artifact generation
artifact text generation
discovery expansion
proposal materialization
EvidencePotential-to-artifact conversion
resolver/composition behavior
fragment activation
file/database persistence
interactive runtime
GUI/API layer
multi-spec runtime state
cross-civilization merge
```
