#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

BIN="./impossible_archive_mvp_v28_11"
SPEC_FILE="examples/40_civilization_specs_v1_1.json"
FRAGMENT_FILE="examples/v12_fragment_examples.json"

if [[ ! -x "$BIN" ]]; then
  make build
fi

run_and_grep() {
  local name="$1"
  local pattern="$2"
  shift 2
  local output="/tmp/impossible_archive_smoke_${name//[^A-Za-z0-9_]/_}.txt"
  echo "[smoke] $name"
  "$@" >"$output"
  if ! grep -qE "$pattern" "$output"; then
    echo "[smoke] expected pattern not found for $name: $pattern" >&2
    echo "[smoke] command output:" >&2
    cat "$output" >&2
    exit 1
  fi
}

run_expect_failure() {
  local name="$1"
  shift
  echo "[smoke] expected failure: $name"
  if "$@" >/tmp/impossible_archive_smoke_expected_failure.txt 2>&1; then
    echo "[smoke] command unexpectedly succeeded: $*" >&2
    cat /tmp/impossible_archive_smoke_expected_failure.txt >&2
    exit 1
  fi
}


run_and_reject_grep() {
  local name="$1"
  local required_pattern="$2"
  local forbidden_pattern="$3"
  shift 3
  local output="/tmp/impossible_archive_smoke_${name//[^A-Za-z0-9_]/_}.txt"
  echo "[smoke] $name"
  "$@" >"$output"
  if ! grep -qE "$required_pattern" "$output"; then
    echo "[smoke] expected pattern not found for $name: $required_pattern" >&2
    cat "$output" >&2
    exit 1
  fi
  if grep -qE "$forbidden_pattern" "$output"; then
    echo "[smoke] forbidden pattern found for $name: $forbidden_pattern" >&2
    cat "$output" >&2
    exit 1
  fi
}

run_failure_and_grep() {
  local name="$1"
  local pattern="$2"
  shift 2
  local output="/tmp/impossible_archive_smoke_${name//[^A-Za-z0-9_]/_}.txt"
  echo "[smoke] expected failure: $name"
  if "$@" >"$output" 2>&1; then
    echo "[smoke] command unexpectedly succeeded: $*" >&2
    cat "$output" >&2
    exit 1
  fi
  if ! grep -qE "$pattern" "$output"; then
    echo "[smoke] expected failure pattern not found for $name: $pattern" >&2
    cat "$output" >&2
    exit 1
  fi
}

run_session_and_grep() {
  local name="$1"
  local pattern="$2"
  local input="$3"
  shift 3
  local output="/tmp/impossible_archive_smoke_${name//[^A-Za-z0-9_]/_}.txt"
  echo "[smoke] $name"
  printf "%b" "$input" | "$@" >"$output"
  if ! grep -qE "$pattern" "$output"; then
    echo "[smoke] expected pattern not found for $name: $pattern" >&2
    cat "$output" >&2
    exit 1
  fi
}

run_and_grep self_test "All self-tests passed" "$BIN" --self-test
run_and_grep validate_specs "CivilizationSpec validation|valid:" "$BIN" --query validate-civilization-specs
run_and_grep list_tags "CivilizationSpec tags" "$BIN" --query list-civilization-tags
run_and_grep list_by_tag "marsh_citadel|glass_delta|river_delta" "$BIN" --query list-civilizations-by-tag --tag river_delta
run_and_grep show_spec "Marsh Citadel Polity|tags:|profile:" "$BIN" --query show-civilization-spec --civilization-id marsh_citadel
run_and_grep bootstrap "Civilization bootstrap|runtime: spec-selected|marsh_citadel" "$BIN" --query bootstrap-civilization
run_and_grep list_targets "Generation targets|authority_conflict_0" "$BIN" --query list-generation-targets
run_and_grep generate_candidates "Generated candidate|decision|authority_conflict_0" "$BIN" --query generate-candidates --target-topic authority_conflict_0 --target-year 625 --seed 42
run_and_grep hidden_cluster "Hidden timeline cluster|authority_conflict_0|decision" "$BIN" --access curator --query hidden-cluster --target-topic authority_conflict_0 --cluster-scope institution_origin --start-year 250 --end-year 380 --seed 42
run_and_grep fixed_fixture_targets "runtime: fixed fixture regression mode|lock_authority" "$BIN" --runtime fixed-fixture --query list-generation-targets
run_and_grep validate_fragments "CivilizationSpec fragment validation|valid fragments" "$BIN" --query validate-civilization-fragments --spec-file "$FRAGMENT_FILE"
run_and_grep list_fragments "CivilizationSpec fragments|fragment.high_artifact_density" "$BIN" --query list-civilization-fragments --spec-file "$FRAGMENT_FILE"
run_and_grep show_fragment "High Artifact Density|artifact_bias|target_public_artifact_count" "$BIN" --query show-civilization-fragment --spec-file "$FRAGMENT_FILE" --fragment-id fragment.high_artifact_density
run_and_grep list_golden_fixtures "Golden fixture worlds|fixture.default_archive" "$BIN" --query list-golden-fixtures
run_and_grep show_golden_fixture "Golden fixture world|fixture.default_archive" "$BIN" --query show-golden-fixture --fixture-id fixture.default_archive
run_and_grep archive_snapshot "ArchiveSnapshot|summary_digest|validation: passed" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive
run_and_grep compare_snapshots "ArchiveSnapshot comparison|result: same" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive
run_and_grep fragment_catalog_snapshot "civilization_fragment_count: 6|validation: passed" "$BIN" --query archive-snapshot --fixture-id fixture.fragment_catalog_only
run_and_grep evidence_potential_summary "EvidencePotential summary|total: [1-9]" "$BIN" --query evidence-potential-summary
run_and_grep evidence_potential_list "EvidencePotentials visible to curator|source_id:|rationale:" "$BIN" --access curator --query list-evidence-potentials
run_and_grep evidence_potential_validation "EvidencePotential validation|result: passed" "$BIN" --query validate-evidence-potentials
run_and_grep evidence_potential_snapshot "evidence_potential_count: [1-9]" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive
run_failure_and_grep fixture_rejects_spec_file "remove --spec-file, --civilization-id, --seed, and --archive-year" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive --spec-file definitely_missing.json
run_failure_and_grep fixture_rejects_seed "remove --spec-file, --civilization-id, --seed, and --archive-year" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive --seed 123

run_expect_failure partial_spec_flags "$BIN" --spec-file "$SPEC_FILE" --query list-generation-targets

# Stable public-access gate smoke: public users must not materialize hidden clusters.
run_and_grep public_gate "mutated: false|requires curator|not mutated" "$BIN" --query materialize-hidden-cluster --target-topic authority_conflict_0 --cluster-scope institution_origin --start-year 250 --end-year 380 --seed 42

run_and_grep knowledge_horizon_summary "KnowledgeHorizon summary|total_findings: [1-9]" "$BIN" --query knowledge-horizon-summary
run_and_grep knowledge_horizon_validation "KnowledgeHorizon validation|findings: [1-9]" "$BIN" --query validate-knowledge-horizon
run_and_grep knowledge_horizon_list "KnowledgeHorizon findings visible to curator|knowledge_horizon.0000" "$BIN" --access curator --query list-knowledge-horizon-findings
run_and_reject_grep knowledge_horizon_public_detail_blocked "KnowledgeHorizon finding:|- found: false" "status:|context_type:|subject_type:|context_year:|earliest_available_year:|context_id:|subject_id:|explanation:" "$BIN" --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0039
run_and_grep knowledge_horizon_curator_detail "KnowledgeHorizon finding:|- found: true|status:|context_type:|subject_type:|context_id:|subject_id:|explanation:" "$BIN" --access curator --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0039

run_and_grep contradiction_budget_summary "ContradictionBudget summary|contradiction_density|status:" "$BIN" --query contradiction-budget-summary
run_and_grep contradiction_budget_validation "ContradictionBudget validation|result: passed" "$BIN" --query validate-contradiction-budget
run_and_grep contradiction_budget_buckets "ContradictionBudget buckets visible to curator|contradiction_budget.archive" "$BIN" --runtime fixed-fixture --access curator --query list-contradiction-budget-buckets
run_and_grep contradiction_budget_bucket_detail "ContradictionBudget bucket:|- found: true|representative_contradiction_ids" "$BIN" --runtime fixed-fixture --access curator --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive
run_and_reject_grep contradiction_budget_public_detail "ContradictionBudget bucket:|- found: true|details: restricted" "representative_contradiction_ids|contradiction\." "$BIN" --runtime fixed-fixture --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive
run_and_grep contradiction_budget_snapshot "contradiction_budget_bucket_count: [1-9]" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive
run_and_grep contradiction_budget_compare_snapshots "ArchiveSnapshot comparison|result: same|contradiction_budget_bucket_count" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive

run_and_grep candidate_plan_summary "CandidateArtifactPlan summary|total_plans: [1-9]" "$BIN" --query candidate-artifact-plan-summary
run_and_grep candidate_plan_validation "CandidateArtifactPlan validation|result: passed" "$BIN" --query validate-candidate-artifact-plans
run_and_grep candidate_plan_list "CandidateArtifactPlans visible to curator|candidate_artifact_plan\." "$BIN" --runtime fixed-fixture --access curator --query list-candidate-artifact-plans
run_and_grep candidate_plan_detail "CandidateArtifactPlan:|- found: true|source_id:|knowledge_horizon_finding_ids|contradiction_budget_bucket_ids" "$BIN" --runtime fixed-fixture --access curator --query show-candidate-artifact-plan --candidate-artifact-plan-id candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_reject_grep candidate_plan_public_detail_blocked "CandidateArtifactPlan:|- found: false" "source_id:|rationale:|knowledge_horizon_finding_ids|contradiction_budget_bucket_ids" "$BIN" --runtime fixed-fixture --query show-candidate-artifact-plan --candidate-artifact-plan-id candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_grep candidate_plan_snapshot "candidate_artifact_plan_count: [1-9]" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive

run_and_grep candidate_plan_evaluation_summary "CandidateArtifactPlanEvaluation summary|total_evaluations: [1-9]" "$BIN" --query candidate-artifact-plan-evaluation-summary
run_and_grep candidate_plan_evaluation_validation "CandidateArtifactPlanEvaluation validation|result: passed" "$BIN" --query validate-candidate-artifact-plan-evaluations
run_and_grep candidate_plan_evaluation_list "CandidateArtifactPlanEvaluations visible to curator|candidate_artifact_plan_evaluation\." "$BIN" --runtime fixed-fixture --access curator --query list-candidate-artifact-plan-evaluations
run_and_grep candidate_plan_evaluation_detail "CandidateArtifactPlanEvaluation:|- found: true|plan_id:|Findings:|related_id=" "$BIN" --runtime fixed-fixture --access curator --query show-candidate-artifact-plan-evaluation --candidate-artifact-plan-evaluation-id candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_reject_grep candidate_plan_evaluation_public_detail_blocked "CandidateArtifactPlanEvaluation:|- found: false" "plan_id:|Findings:|related_id=|knowledge_horizon\.|contradiction_budget\." "$BIN" --runtime fixed-fixture --query show-candidate-artifact-plan-evaluation --candidate-artifact-plan-evaluation-id candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_grep candidate_plan_evaluation_snapshot "candidate_artifact_plan_evaluation_count: [1-9]" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive

run_and_grep candidate_artifact_proposal_summary "CandidateArtifactProposal summary|total_proposals: [1-9]" "$BIN" --query candidate-artifact-proposal-summary
run_and_grep candidate_artifact_proposal_validation "CandidateArtifactProposal validation|result: passed" "$BIN" --query validate-candidate-artifact-proposals
run_and_grep candidate_artifact_proposal_list "CandidateArtifactProposals visible to curator|candidate_artifact_proposal\." "$BIN" --runtime fixed-fixture --access curator --query list-candidate-artifact-proposals
run_and_grep candidate_artifact_proposal_detail "CandidateArtifactProposal:|- found: true|evaluation_id:|plan_id:|source_evidence_potential_id:|Proposed claim skeletons:|Proposed validation gates:" "$BIN" --runtime fixed-fixture --access curator --query show-candidate-artifact-proposal --candidate-artifact-proposal-id candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_reject_grep candidate_artifact_proposal_public_detail_blocked "CandidateArtifactProposal:|- found: false" "source_evidence_potential_id:|evaluation_id:|plan_id:|Proposed validation gates:|blocking_evaluation_finding|knowledge_horizon|contradiction_budget" "$BIN" --runtime fixed-fixture --query show-candidate-artifact-proposal --candidate-artifact-proposal-id candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_grep candidate_artifact_proposal_snapshot "candidate_artifact_proposal_count: [1-9]" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive

run_and_grep candidate_artifact_proposal_audit_summary "CandidateArtifactProposalAudit summary|total_audits: [1-9]" "$BIN" --query candidate-artifact-proposal-audit-summary
run_and_grep candidate_artifact_proposal_audit_validation "CandidateArtifactProposalAudit validation|result: passed" "$BIN" --query validate-candidate-artifact-proposal-audits
run_and_grep candidate_artifact_proposal_audit_list "CandidateArtifactProposalAudits visible to curator|candidate_artifact_proposal_audit\." "$BIN" --runtime fixed-fixture --access curator --query list-candidate-artifact-proposal-audits
run_and_grep candidate_artifact_proposal_audit_detail "CandidateArtifactProposalAudit:|- found: true|proposal_id:|Policy thresholds:|Policy comparison:|Findings:|reason_code=|Required revisions:" "$BIN" --runtime fixed-fixture --access curator --query show-candidate-artifact-proposal-audit --candidate-artifact-proposal-audit-id candidate_artifact_proposal_audit.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_reject_grep candidate_artifact_proposal_audit_public_detail_blocked "CandidateArtifactProposalAudit:|- found: false" "proposal_id:|Findings:|reason_code=|related_id=|Required revisions:|Policy thresholds:|knowledge_horizon|contradiction_budget|evidence_potential" "$BIN" --runtime fixed-fixture --query show-candidate-artifact-proposal-audit --candidate-artifact-proposal-audit-id candidate_artifact_proposal_audit.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_grep candidate_artifact_proposal_audit_snapshot "candidate_artifact_proposal_audit_count: [1-9]" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive
run_and_grep candidate_artifact_draft_summary "CandidateArtifactDraft summary|total_drafts: [1-9]|current_artifact_insertion_enabled: 0" "$BIN" --query candidate-artifact-draft-summary
run_and_grep candidate_artifact_draft_validation "CandidateArtifactDraft validation|result: passed" "$BIN" --query validate-candidate-artifact-drafts
run_and_grep candidate_artifact_draft_list "CandidateArtifactDrafts visible to curator|candidate_artifact_draft\." "$BIN" --runtime fixed-fixture --access curator --query list-candidate-artifact-drafts
run_and_grep candidate_artifact_draft_detail "CandidateArtifactDraft detail:|- found: true|proposal_id:|audit_id:|Claim outline lines:|Required validation gates:" "$BIN" --runtime fixed-fixture --access curator --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_reject_grep candidate_artifact_draft_public_detail_blocked "CandidateArtifactDraft detail:|- found: false" "proposal_id:|audit_id:|source_evidence_potential_id:|Source chain IDs|Claim outline lines:|Required validation gates:" "$BIN" --runtime fixed-fixture --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_grep candidate_artifact_draft_snapshot "candidate_artifact_draft_count: [1-9]|candidate_artifact_draft_mutation_enabled_count: 0" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive
run_and_grep candidate_artifact_draft_compare_snapshots "ArchiveSnapshot comparison|result: same|candidate_artifact_draft_count" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive

run_and_grep candidate_artifact_draft_review_summary "CandidateArtifactDraftReview summary|total_reviews: [1-9]|mutation_or_generation_enabled: 0" "$BIN" --query candidate-artifact-draft-review-summary
run_and_grep candidate_artifact_draft_review_validation "CandidateArtifactDraftReview validation|result: passed" "$BIN" --query validate-candidate-artifact-draft-reviews
run_and_grep candidate_artifact_draft_review_list "CandidateArtifactDraftReviews visible to curator|candidate_artifact_draft_review\." "$BIN" --runtime fixed-fixture --access curator --query list-candidate-artifact-draft-reviews
run_and_grep candidate_artifact_draft_review_detail "CandidateArtifactDraftReview detail:|- found: true|draft_id:|proposal_id:|audit_id:|decision:|revision_pressure_score:" "$BIN" --runtime fixed-fixture --access curator --query show-candidate-artifact-draft-review --candidate-artifact-draft-review-id candidate_artifact_draft_review.candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_reject_grep candidate_artifact_draft_review_public_detail_blocked "CandidateArtifactDraftReview detail:|- found: false" "draft_id:|proposal_id:|audit_id:|outline_completeness_score:|reason_code:|required_revision:" "$BIN" --runtime fixed-fixture --query show-candidate-artifact-draft-review --candidate-artifact-draft-review-id candidate_artifact_draft_review.candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_grep candidate_artifact_draft_review_snapshot "candidate_artifact_draft_review_record_count: [1-9]|candidate_artifact_draft_review_mutation_enabled_count: 0" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive
run_and_grep candidate_artifact_draft_review_compare_snapshots "ArchiveSnapshot comparison|result: same|candidate_artifact_draft_review_record_count" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive

run_and_grep control_layer_audit_summary "ControlLayerAudit summary|total_entries: [1-9]|mutation_capable_entries" "$BIN" --query control-layer-audit-summary
run_and_grep control_layer_audit_validation "ControlLayerAudit validation|result: passed" "$BIN" --query validate-control-layer-audit
run_and_grep control_layer_audit_entries "ControlLayerAudit entries visible to curator|control_layer.candidate_artifact_proposal_audit" "$BIN" --runtime fixed-fixture --access curator --query list-control-layer-audit-entries
run_and_grep control_layer_audit_entry_detail "ControlLayerAudit entry:|- found: true|Primary files|behavior: audit_only" "$BIN" --runtime fixed-fixture --access curator --query show-control-layer-audit-entry --control-layer-audit-entry-id control_layer.candidate_artifact_proposal_audit
run_and_reject_grep control_layer_audit_public_detail_blocked "ControlLayerAudit entry:|- found: false" "primary_files|validation_functions|snapshot_fields|Known gaps|Notes" "$BIN" --runtime fixed-fixture --query show-control-layer-audit-entry --control-layer-audit-entry-id control_layer.hidden_truth
run_and_grep control_layer_audit_snapshot "control_layer_audit_entry_count: [1-9]|control_layer_audit_mutation_capable_count" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive
run_and_grep control_layer_audit_compare_snapshots "ArchiveSnapshot comparison|result: same|control_layer_audit_entry_count" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive

run_session_and_grep runtime_session_two_read_queries "RuntimeSession initialized|CandidateArtifactDraftReview summary|ControlLayerAudit summary|RuntimeSession ended" "--query candidate-artifact-draft-review-summary
--query control-layer-audit-summary
end-session
" "$BIN" --session
run_session_and_grep runtime_session_knowledge_horizon_summary_and_validation "RuntimeSession initialized|KnowledgeHorizon summary|KnowledgeHorizon validation|RuntimeSession ended" "--query knowledge-horizon-summary
--query validate-knowledge-horizon
end-session
" "$BIN" --session
run_session_and_grep runtime_session_knowledge_horizon_list_and_detail "RuntimeSession initialized|KnowledgeHorizon findings visible to curator|KnowledgeHorizon finding:|- found: true|RuntimeSession ended" "--access curator --query list-knowledge-horizon-findings
--access curator --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0039
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_evidence_potential_list_and_detail "RuntimeSession initialized|EvidencePotentials visible to curator|EvidencePotential:|- found: true|RuntimeSession ended" "--access curator --query list-evidence-potentials
--access curator --query show-evidence-potential --evidence-potential-id evidence_potential.0000
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_contradiction_budget_list_and_detail "RuntimeSession initialized|ContradictionBudget buckets visible to curator|ContradictionBudget bucket:|- found: true|RuntimeSession ended" "--access curator --query list-contradiction-budget-buckets
--access curator --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_candidate_plan_evaluation_summary_and_validation "RuntimeSession initialized|CandidateArtifactPlanEvaluation summary|CandidateArtifactPlanEvaluation validation|RuntimeSession ended" "--query candidate-artifact-plan-evaluation-summary
--query validate-candidate-artifact-plan-evaluations
end-session
" "$BIN" --session
run_session_and_grep runtime_session_candidate_plan_evaluation_list_and_detail "RuntimeSession initialized|CandidateArtifactPlanEvaluations visible to curator|CandidateArtifactPlanEvaluation:|- found: true|RuntimeSession ended" "--access curator --query list-candidate-artifact-plan-evaluations
--access curator --query show-candidate-artifact-plan-evaluation --candidate-artifact-plan-evaluation-id candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_candidate_proposal_summary_and_validation "RuntimeSession initialized|CandidateArtifactProposal summary|CandidateArtifactProposal validation|RuntimeSession ended" "--query candidate-artifact-proposal-summary
--query validate-candidate-artifact-proposals
end-session
" "$BIN" --session
run_session_and_grep runtime_session_candidate_proposal_audit_summary_and_validation "RuntimeSession initialized|CandidateArtifactProposalAudit summary|CandidateArtifactProposalAudit validation|RuntimeSession ended" "--query candidate-artifact-proposal-audit-summary
--query validate-candidate-artifact-proposal-audits
end-session
" "$BIN" --session
run_session_and_grep runtime_session_candidate_plan_summary_and_validation "RuntimeSession initialized|CandidateArtifactPlan summary|CandidateArtifactPlan validation|RuntimeSession ended" "--query candidate-artifact-plan-summary
--query validate-candidate-artifact-plans
end-session
" "$BIN" --session
run_session_and_grep runtime_session_candidate_plan_list_and_detail "RuntimeSession initialized|CandidateArtifactPlans visible to curator|CandidateArtifactPlan:|- found: true|RuntimeSession ended" "--access curator --query list-candidate-artifact-plans
--access curator --query show-candidate-artifact-plan --candidate-artifact-plan-id candidate_artifact_plan.evidence_potential.0000.administrative_docket
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_candidate_draft_list_and_detail "RuntimeSession initialized|CandidateArtifactDrafts visible to curator|CandidateArtifactDraft detail:|- found: true|RuntimeSession ended" "--access curator --query list-candidate-artifact-drafts
--access curator --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_candidate_proposal_list_and_detail "RuntimeSession initialized|CandidateArtifactProposals visible to curator|CandidateArtifactProposal:|- found: true|RuntimeSession ended" "--access curator --query list-candidate-artifact-proposals
--access curator --query show-candidate-artifact-proposal --candidate-artifact-proposal-id candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_candidate_proposal_audit_list_and_detail "RuntimeSession initialized|CandidateArtifactProposalAudits visible to curator|CandidateArtifactProposalAudit:|- found: true|RuntimeSession ended" "--access curator --query list-candidate-artifact-proposal-audits
--access curator --query show-candidate-artifact-proposal-audit --candidate-artifact-proposal-audit-id candidate_artifact_proposal_audit.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_invalid_query_recovers "RuntimeSession query rejected|denied_unknown|session_active: true|CandidateArtifactDraftReview summary|RuntimeSession ended" "--query definitely-not-a-query
--query candidate-artifact-draft-review-summary
end-session
" "$BIN" --session
run_session_and_grep runtime_session_mutating_query_denied "RuntimeSession query rejected|denied_mutating|materialize-hidden-cluster|session_active: true|CandidateArtifactDraftReview summary|RuntimeSession ended" "--query materialize-hidden-cluster
--query candidate-artifact-draft-review-summary
end-session
" "$BIN" --session

echo "CLI workflow smoke tests passed."
