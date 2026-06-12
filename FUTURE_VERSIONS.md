# Future Versions Roadmap

This file records recommended future versions after v27.4. It is intentionally a roadmap, not active behavior.

## Implemented Historical Notes

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

Implemented explicit `bootstrap-civilization` support that builds one minimal deterministic `ArchiveEngineState` from one selected spec and preserves the fixed fixture as the default runtime.

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


### v28.1 — Golden Fixture Worlds + ArchiveSnapshot Foundation

Implemented a deterministic regression foundation: named golden fixture worlds, summary `ArchiveSnapshot` generation, snapshot comparison, and CLI inspection queries. Fragments remain inert catalog data and do not affect runtime selection, generation, target resolution, materialization, or archive mutation.

## Planned Next Versions

### v27.5 — Optional Default-Runtime Redaction and Specificity Snapshot Pass, if needed

Potential follow-up: add broader public/scholar redaction snapshots and artifact specificity review fixtures across a wider catalog sample. Keep tags/profile metadata-only unless a later design gate explicitly changes that boundary.

### v28.0 — CivilizationSpec v1.2 Fragment Model Intake, No Resolver

Implemented the first bounded v1.2 slice: fragment records, categories, inert patch strategies, restricted patch values, fragment loading/validation, fragment examples, and list/show/validate CLI inspection. No composition records, resolver, patch application, resolved spec output, generation behavior based on fragments/tags, cross-civilization merge, or multi-spec runtime state were added.

### v28.2 — EvidencePotential / Evidence Projection Model Foundation

Implemented an inert EvidencePotential layer: deterministic derivation from current runtime state, validation, CLI inspection, fixture integration, snapshot counting, and summary digest participation. No artifacts are generated from potentials, no discovery behavior changes, and fragments remain inert catalog data.

### v28.3 — KnowledgeHorizon validation

Implemented deterministic validation for what actors, artifacts, interpretations, EvidencePotential derivation, and snapshot contexts could plausibly know at a given time. No composition resolution, patch application, or EvidencePotential-driven artifact generation was added.

### v28.3.1 — KnowledgeHorizon public detail access hardening

Implemented a corrective access-control patch: KnowledgeHorizon finding IDs and detailed records are curator/debug diagnostics, and public/scholar detail lookup returns `found: false` for hidden or inaccessible findings. Public aggregate summaries remain available.

### v28.4 — ContradictionBudget Telemetry Foundation

Implemented deterministic, read-only ContradictionBudget telemetry for archive-level, contradiction-type, contradiction-cause, and mystery-linked pressure. The layer participates in CLI inspection, validation, snapshot counts, and `summary_digest` material without enforcing budgets, repairing contradictions, generating artifacts, changing discovery, activating fragments, or adding persistence.

## Later Hardening Passes

- Header hygiene: continue migrating implementation files from the umbrella include to narrower headers when useful.
- Indexing: add indexes for claims/entities/contradictions only after data volume warrants it.
- Sanitizer CI: CI currently covers C++20, C++17, and strict warnings; sanitizer runs are available locally through `make sanitize` and can be promoted to CI if runtime budget allows.
- Serialization: add a stable external state format only after materialization and generated candidates are stable.
- External ingestion: accept typed candidate records from controlled JSON/YAML only after diagnostics and no-default semantics are explicit.


### v28.5 — EvidencePotential -> CandidateArtifact Planning

Recommended next slice: add gated planning only. Do not materialize, discover, or mutate archive state.


## v28.6 — CandidateArtifactPlan Evaluation, No Archive Mutation

CandidateArtifactPlanEvaluation evaluates plans without generating artifacts, materializing candidates, mutating archive state, activating fragments, or introducing resolver/composition behavior.

## v28.7 — CandidateArtifactProposal Drafting, No Archive Mutation

CandidateArtifactProposal now drafts inert proposal records from evaluations. It records proposed shape/type/register, claim skeletons, validation gates, safety, and access notes without creating artifacts, generating final artifact text, inserting claims, scheduling discoveries, mutating state, activating fragments, or introducing resolver/composition behavior.

## v28.7.1 — PublicArchive Invariant Hardening

Implemented a corrective Core Guidelines hardening slice before adding more proposal/reporting layers. `PublicArchive::add_claim_to_artifact(...)` now owns the preferred claim insertion relationship boundary, including artifact claim links and positive-weight voice claim links. The detached artifact helper remains only as a quarantined compatibility path and prevalidates failure cases before mutation. No CandidateArtifactProposalAudit, generation, materialization, discovery scheduling, persistence, resolver/composition behavior, broad primitive-type rewrite, or test split was introduced.

## v28.7.3 — CLI Args Span Refactor / Test const_cast removal

Recommended next slice: reduce argument-lifetime and test-mutation risk around CLI argument construction. Keep behavior unchanged.

## v28.7.3 — CLI Argument View Refactor

Moves internal CLI parsing and execution from raw `argc`/`argv` pointer-count interfaces to a typed `CliArgs` argument view. Raw argv adaptation remains at the executable boundary. This is interface cleanup only and does not change runtime behavior.


## v28.7.4 — ReliabilityComponents primitive-cluster cleanup

Implemented hardening slice: production code no longer relies on the nine-adjacent-double `components(...)` helper. Reliability values are expressed through explicit `ReliabilityComponents` member initialization while preserving scoring behavior. Do not combine this with a broad `Year`/typed-ID rewrite.

## v28.8 — CandidateArtifactProposal Audit / Proposal Quality Gates, No Archive Mutation

## v28.10 — Proposal Quality Gate Policy Tightening

Landed as an audit/consolidation slice. It classifies the current v28 control stack and defers artifact generation/discovery expansion until follow-up policy tightening.

## v28.10 — Proposal Quality Gate Policy Tightening, No Mutation

Deferred until core hardening slices land. Audit proposal quality before any proposal can become an artifact candidate. Keep the layer read-only and do not introduce materialization.

## v28.7.2 — CandidateArtifactProposal Access-Neutral Model Hardening

Makes CandidateArtifactProposal stored state access-neutral. Access-specific proposal safety is computed at formatting/query time, and full-state validation includes persistent proposals. No proposal audit, generation, materialization, discovery, persistence, resolver/composition, or interactive runtime behavior is added.

## v28.10 — Proposal Quality Gate Policy Tightening

Implemented policy hardening for CandidateArtifactProposalAudit. Audit decisions now use explicit deterministic thresholds, findings carry reason codes, non-pass audits require actionable revisions, and validation rejects policy-inconsistent audit states. This remains no-mutation/no-generation.

Possible next slices after reassessment:

```text
v28.11  ContradictionBudget validator-backed status tightening, No Mutation
v29.0   CandidateArtifactDraft / Text Outline, No Artifact Insertion
```
