# Release Checklist

## 1. Version and scope

- [ ] Version number and package name are correct.
- [ ] Scope matches the release note.
- [ ] Non-goals are explicitly preserved.

## 2. Build verification

- [ ] `make clean test` passes under C++20.
- [ ] `make clean CXXSTD=c++17 test` passes.
- [ ] `make clean strict` passes.
- [ ] `make clean sanitize` passes.

## 3. Test verification

- [ ] Built-in `--self-test` reports `All self-tests passed.`
- [ ] Runtime-selection self-tests pass.
- [ ] Catalog/tag metadata self-tests pass.
- [ ] Low-specificity materialization remains blocked.

## 4. Sanitizer verification

- [ ] AddressSanitizer reports no errors.
- [ ] UndefinedBehaviorSanitizer reports no errors.
- [ ] No sanitizer suppressions were added without written justification.

## 5. CLI smoke verification

- [ ] `scripts/smoke_test_readme_workflows.sh` passes.
- [ ] README commands match the smoke script.
- [ ] Expected failure paths return nonzero exit status.

## 6. Catalog validation

- [ ] Bundled catalog validates.
- [ ] Tag validation reports expected warnings/errors.
- [ ] Catalog cardinality remains dynamic: one or more specs are valid, empty catalogs are invalid.

## 7. Runtime-mode verification

- [ ] Default runtime is spec-selected.
- [ ] Bundled default civilization is `marsh_citadel`.
- [ ] Fixed fixture works only through `--runtime fixed-fixture`.
- [ ] Unsupported runtime/query combinations return exit status `1`.

## 8. Access/redaction verification

- [ ] Public/scholar redaction spot checks pass.
- [ ] Curator/canon/debug materialization policy remains intact.
- [ ] Hidden mutation IDs, hidden entity IDs, and hidden event IDs are not exposed below curator access.

## 9. Package/hash verification

- [ ] ZIP package created.
- [ ] TAR.GZ package created.
- [ ] ZIP SHA-256 recorded.
- [ ] TAR.GZ SHA-256 recorded.

## 10. Documentation verification

- [ ] `README.md` reflects current commands.
- [ ] `FUTURE_VERSIONS.md` roadmap is current.
- [ ] `docs/ARCHITECTURE_MAP.md` maps real code paths.
- [ ] `docs/CIVILIZATION_TAG_REGISTRY.md` and related design docs remain non-runtime guidance.

## 11. Non-goals / not included

- [ ] No `CivilizationSpecFragment` runtime model.
- [ ] No `CivilizationSpecComposition` runtime model.
- [ ] No `SpecPatch`, `PatchStrategy`, `CivilizationSpecResolver`, or `ResolutionTrace` runtime implementation.
- [ ] No persistence, database storage, UI/API layer, cross-civilization merge, or multi-spec runtime state.

## 12. Known issues

- [ ] Known issues are documented or explicitly empty.

## 13. Release sign-off

- [ ] Build verifier:
- [ ] Test verifier:
- [ ] Release approver:

## v28.1.1 Golden Fixture / Snapshot Checks

[ ] `./impossible_archive_mvp_v28_11 --query list-golden-fixtures` lists the known fixtures.
[ ] `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive` reports validation passed.
[ ] `./impossible_archive_mvp_v28_11 --query compare-archive-snapshots --fixture-id fixture.default_archive` reports `result: same`.
[ ] `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.fragment_catalog_only` reports a nonzero fragment count.
[ ] Snapshot formatting exposes counts, `fixture_seed`, `state_seed`, `fixture_archive_year`, `effective_archive_year`, and `summary_digest`, but not hidden entity/event details.
[ ] v28.0 fragments remain inert and do not affect generation behavior.

[ ] Fixture-backed queries reject `--spec-file`, `--civilization-id`, `--seed`, and `--archive-year` with a nonzero exit before fixture construction/spec loading.
[ ] Snapshot comparison is treated as a same-fixture rebuild determinism check.


## v28.2 EvidencePotential Checks

[ ] `./impossible_archive_mvp_v28_11 --query evidence-potential-summary` reports a nonzero total for the default runtime.
[ ] `./impossible_archive_mvp_v28_11 --access curator --query list-evidence-potentials` lists inspectable source/rationale details.
[ ] `./impossible_archive_mvp_v28_11 --query validate-evidence-potentials` reports `result: passed`.
[ ] `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive` reports a nonzero `evidence_potential_count`.
[ ] `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.fragment_catalog_only` counts inert fragments but has no fragment-derived evidence potentials.
[ ] Public EvidencePotential formatting avoids hidden source IDs and hidden rationales.
[ ] Candidate generation, discovery behavior, public archive materialization, hidden mutation, and fragment runtime behavior remain unchanged by EvidencePotential.

## v28.3 KnowledgeHorizon Checks

[ ] `./impossible_archive_mvp_v28_11 --query validate-knowledge-horizon` runs deterministically.
[ ] `./impossible_archive_mvp_v28_11 --query knowledge-horizon-summary` reports KnowledgeHorizon finding/error counts.
[ ] `./impossible_archive_mvp_v28_11 --access curator --query list-knowledge-horizon-findings` exposes finding IDs and diagnostics.
[ ] `./impossible_archive_mvp_v28_11 --access curator --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0000` exposes one detailed finding.
[ ] `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive` reports `knowledge_horizon_finding_count` and `knowledge_horizon_error_count`.
[ ] Public KnowledgeHorizon formatting avoids hidden IDs and hidden explanations.
[ ] `./impossible_archive_mvp_v28_11 --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0039` returns `found: false`.
[ ] Public blocked KnowledgeHorizon detail output does not contain `status:`, `context_type:`, `subject_type:`, `context_year:`, `earliest_available_year:`, `context_id:`, `subject_id:`, or `explanation:`.
[ ] `./impossible_archive_mvp_v28_11 --access curator --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0039` still exposes full diagnostic detail.
[ ] KnowledgeHorizon does not generate artifacts, discoveries, candidates, fragments, compositions, patches, or persistent state.


## v28.4 ContradictionBudget Checks

[ ] `./impossible_archive_mvp_v28_11 --query contradiction-budget-summary` reports archive-level telemetry.
[ ] `./impossible_archive_mvp_v28_11 --query validate-contradiction-budget` reports `result: passed`.
[ ] `./impossible_archive_mvp_v28_11 --runtime fixed-fixture --access curator --query list-contradiction-budget-buckets` lists `contradiction_budget.archive`.
[ ] `./impossible_archive_mvp_v28_11 --runtime fixed-fixture --access curator --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive` exposes representative contradiction IDs.
[ ] Public ContradictionBudget formatting avoids representative contradiction IDs, hidden causes, hidden source IDs, and diagnostic notes.
[ ] `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive` reports ContradictionBudget snapshot counts.
[ ] `summary_digest` includes stable ContradictionBudget bucket/status/count material.
[ ] Fragment catalog fixtures remain inert and do not create fragment-driven budget records.
[ ] ContradictionBudget does not reject, repair, generate, mutate, discover, resolve, persist, or introduce interactive session behavior.


## v28.6 CandidateArtifactPlan Checks

- `make test`
- `make CXXSTD=c++17 -j1 test`
- `make strict`
- `make smoke`
- `make sanitize` when the container sanitizer runtime permits it
- Manual CLI:
  - `./impossible_archive_mvp_v28_11 --query candidate-artifact-plan-summary`
  - `./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-plans`
  - `./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-plans`
  - `./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-plan --candidate-artifact-plan-id candidate_artifact_plan.evidence_potential.0019.administrative_docket`
  - `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive`
  - `./impossible_archive_mvp_v28_11 --query compare-archive-snapshots --fixture-id fixture.default_archive`

Confirm no CandidateArtifactPlan path generates candidates, materializes artifacts, discovers evidence, mutates hidden truth, mutates the public archive, activates fragments, or introduces resolver/composition behavior.


## v28.6 CandidateArtifactPlanEvaluation Checks

[ ] `./impossible_archive_mvp_v28_11 --query candidate-artifact-plan-evaluation-summary` reports nonzero evaluations.
[ ] `./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-plan-evaluations` reports `result: passed`.
[ ] `./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-plan-evaluations` lists evaluation IDs.
[ ] `./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-plan-evaluation --candidate-artifact-plan-evaluation-id candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0019.administrative_docket` exposes full diagnostics.
[ ] Public evaluation formatting avoids hidden plan IDs, KnowledgeHorizon IDs, ContradictionBudget IDs, protected mystery details, hidden rationale, and curator-only findings.
[ ] `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive` reports CandidateArtifactPlanEvaluation snapshot counts.

## v28.7 CandidateArtifactProposal Checks

[ ] `./impossible_archive_mvp_v28_11 --query candidate-artifact-proposal-summary` reports nonzero proposals.
[ ] `./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-proposals` reports `result: passed`.
[ ] `./impossible_archive_mvp_v28_11 --access curator --query list-candidate-artifact-proposals` lists proposal IDs.
[ ] `./impossible_archive_mvp_v28_11 --access curator --query show-candidate-artifact-proposal --candidate-artifact-proposal-id candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0019.administrative_docket` exposes full proposal diagnostics.
[ ] Public `show-candidate-artifact-proposal` does not expose source IDs, evaluation IDs, plan IDs, diagnostic gates, or hidden rationale.
[ ] `./impossible_archive_mvp_v28_11 --query archive-snapshot --fixture-id fixture.default_archive` reports CandidateArtifactProposal snapshot counts.
[ ] No proposal enables generation, materialization, or archive mutation.

## v28.7.2 PublicArchive invariant hardening checks

- [ ] `PublicArchive::add_claim_to_artifact(...)` rejects missing target artifacts without inserting claims.
- [ ] Duplicate claim IDs through `add_claim_to_artifact(...)` reject without changing artifact claim or voice links.
- [ ] Source-artifact mismatch rejects without inserting claims or artifact links.
- [ ] Valid claim insertion links the claim to the artifact and adds a voice claim link when `voice_weight > 0`.
- [ ] Zero or negative voice weight does not add a voice claim link.
- [ ] Quarantined detached-artifact helper prevalidates failures before mutating detached artifact links.
- [ ] Existing bootstrap/materialization workflows and deterministic snapshots still pass.
- [ ] No CandidateArtifactProposalAudit, generation, materialization, discovery scheduling, persistence, resolver/composition behavior, broad primitive-type rewrite, or full self-test split was introduced.

## v28.7.2 CandidateArtifactProposal access-neutral checks

[ ] Proposal derivation under public/curator access yields equivalent stored proposal records.
[ ] Proposal derivation under scholar/debug access yields equivalent stored proposal records.
[ ] `summary_digest` is stable across proposal derivation access levels.
[ ] Public proposal detail hides source/evaluation/plan IDs and diagnostic internals.
[ ] Curator/debug proposal detail exposes full diagnostics.
[ ] `validate_full_state()` rejects malformed persistent CandidateArtifactProposal records.
[ ] `PublicArchive::add_claim_to_artifact(...)` invariant tests from v28.7.1 still pass.


## v28.9 CLI argument view checks

[ ] `parse_cli(const CliArgs&)` parses normal query options.
[ ] `run_cli(const CliArgs&)` executes normal query paths.
[ ] CLI self-tests do not use `const_cast<char*>` for argument setup.
[ ] Fixture override expected failures still reject before spec-file access.
[ ] Public KnowledgeHorizon and CandidateArtifactProposal detail gates remain blocked.

## v28.9 Control-layer audit checks

- [ ] `control-layer-audit-summary` reports nonzero entries.
- [ ] `validate-control-layer-audit` reports passed.
- [ ] Public detail for `control_layer.hidden_truth` does not expose internals.
- [ ] Curator detail exposes primary files, validation functions, snapshot fields, known gaps, and notes.
- [ ] `docs/CURRENT_STATE_AUDIT.md` is present.

## v28.10 Proposal quality gate policy checks

[ ] `./impossible_archive_mvp_v28_11 --query candidate-artifact-proposal-audit-summary` reports nonzero audits.
[ ] `./impossible_archive_mvp_v28_11 --query validate-candidate-artifact-proposal-audits` reports `result: passed`.
[ ] Curator audit detail exposes policy thresholds, reason codes, and required revisions.
[ ] Public audit detail remains gated and does not expose reason codes or required revisions.
[ ] `./impossible_archive_mvp_v28_11 --query control-layer-audit-summary` still reports the control-layer audit.
[ ] Snapshot comparison remains deterministic.
[ ] No artifact generation, materialization, discovery scheduling, persistence, resolver/composition behavior, fragment activation, or interactive runtime behavior is introduced.

## v28.11 ContradictionBudget policy checks

[ ] `./impossible_archive_mvp_v28_11 --query contradiction-budget-summary` reports advisory-only behavior.
[ ] `./impossible_archive_mvp_v28_11 --query validate-contradiction-budget` reports `result: passed`.
[ ] Curator bucket detail exposes `ContradictionBudgetPolicy` thresholds and deterministic reason codes.
[ ] Public bucket detail remains aggregate/safe and does not expose reason codes, policy thresholds, representative contradiction IDs, or diagnostic notes.
[ ] Too-clean archive slices are classified as `watch` with `too_clean_archive` reason code.
[ ] Generation-bug pressure is classified as `over_budget` with `generation_bug_pressure` reason code.
[ ] Productive ambiguity, ritual/legal contradiction, damaged-evidence disagreement, and protected-mystery pressure are reason-coded where supported by existing data.
[ ] Validation rejects status/reason-code mismatches, missing generation-bug reasons, missing density/unresolved reasons, missing too-clean reasons, missing-cause reasons, and invalid metrics without `invalid_metric`.
[ ] `ArchiveSnapshot` reports watch, too-clean, productive-ambiguity, over-budget, and generation-bug budget counts.
[ ] `summary_digest` includes stable ContradictionBudget reason-code material.
[ ] Control-layer audit remains valid.
[ ] No hard contradiction-budget enforcement, repair, artifact generation, artifact text generation, candidate materialization, discovery scheduling, public archive mutation, hidden truth mutation, persistence, resolver/composition behavior, fragment activation, or interactive runtime behavior is introduced.
