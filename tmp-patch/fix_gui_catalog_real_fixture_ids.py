#!/usr/bin/env python3
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)

cli_path = Path("src/cli.cpp")
cli = cli_path.read_text()

replacements = [
    (
        '{"show-candidate-artifact-plan", "Show CandidateArtifactPlan", "Candidate artifact plan", "curator", "--candidate-artifact-plan-id", "candidate_artifact_plan.0000"}',
        '{"show-candidate-artifact-plan", "Show CandidateArtifactPlan", "Candidate artifact plan", "curator", "--candidate-artifact-plan-id", "candidate_artifact_plan.evidence_potential.0000.administrative_docket"}',
        'candidate artifact plan example id',
    ),
    (
        '{"show-candidate-artifact-plan-evaluation", "Show CandidateArtifactPlanEvaluation", "Candidate artifact plan evaluation", "curator", "--candidate-artifact-plan-evaluation-id", "candidate_artifact_plan_evaluation.0000"}',
        '{"show-candidate-artifact-plan-evaluation", "Show CandidateArtifactPlanEvaluation", "Candidate artifact plan evaluation", "curator", "--candidate-artifact-plan-evaluation-id", "candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket"}',
        'candidate artifact plan evaluation example id',
    ),
    (
        '{"show-candidate-artifact-proposal", "Show CandidateArtifactProposal", "Candidate artifact proposal", "curator", "--candidate-artifact-proposal-id", "candidate_artifact_proposal.0000"}',
        '{"show-candidate-artifact-proposal", "Show CandidateArtifactProposal", "Candidate artifact proposal", "curator", "--candidate-artifact-proposal-id", "candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket"}',
        'candidate artifact proposal example id',
    ),
    (
        '{"show-candidate-artifact-proposal-audit", "Show CandidateArtifactProposalAudit", "Candidate artifact proposal audit", "curator", "--candidate-artifact-proposal-audit-id", "candidate_artifact_proposal_audit.0000"}',
        '{"show-candidate-artifact-proposal-audit", "Show CandidateArtifactProposalAudit", "Candidate artifact proposal audit", "curator", "--candidate-artifact-proposal-audit-id", "candidate_artifact_proposal_audit.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket"}',
        'candidate artifact proposal audit example id',
    ),
    (
        '{"show-candidate-artifact-draft", "Show CandidateArtifactDraft", "Candidate artifact draft", "curator", "--candidate-artifact-draft-id", "candidate_artifact_draft.0000"}',
        '{"show-candidate-artifact-draft", "Show CandidateArtifactDraft", "Candidate artifact draft", "curator", "--candidate-artifact-draft-id", "candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket"}',
        'candidate artifact draft example id',
    ),
    (
        '{"show-candidate-artifact-draft-review", "Show CandidateArtifactDraftReview", "Candidate artifact draft review", "curator", "--candidate-artifact-draft-review-id", "candidate_artifact_draft_review.0000"}',
        '{"show-candidate-artifact-draft-review", "Show CandidateArtifactDraftReview", "Candidate artifact draft review", "curator", "--candidate-artifact-draft-review-id", "candidate_artifact_draft_review.candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket"}',
        'candidate artifact draft review example id',
    ),
]

for old, new, label in replacements:
    cli = replace_once(cli, old, new, label)

cli_path.write_text(cli)

smoke_path = Path("scripts/smoke_test_cli_workflows.sh")
smoke = smoke_path.read_text()
smoke = replace_once(
    smoke,
    'run_and_grep gui_query_catalog "GUI query catalog:|query: knowledge-horizon-summary|argv: --runtime fixed-fixture --access curator --query show-evidence-potential --evidence-potential-id evidence_potential.0000" "$BIN" --query gui-query-catalog\n',
    'run_and_grep gui_query_catalog "GUI query catalog:|query: knowledge-horizon-summary|argv: --runtime fixed-fixture --access curator --query show-evidence-potential --evidence-potential-id evidence_potential.0000" "$BIN" --query gui-query-catalog\nrun_and_grep gui_query_catalog_candidate_detail_examples "candidate_artifact_plan.evidence_potential.0000.administrative_docket|candidate_artifact_draft_review.candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket" "$BIN" --query gui-query-catalog\n',
    'gui catalog real fixture smoke coverage',
)
smoke_path.write_text(smoke)
