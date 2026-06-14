#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise SystemExit(f"expected text not found in {path}: {old[:200]!r}")
    p.write_text(text.replace(old, new, 1))


# Expand the shared read-only session subset by the CandidateArtifactProposal list/detail pair.
replace_once(
    "src/cli.cpp",
    "           query == \"report\" ||\n           query == \"candidate-artifact-draft-summary\" ||",
    "           query == \"report\" ||\n           query == \"list-candidate-artifact-proposals\" ||\n           query == \"show-candidate-artifact-proposal\" ||\n           query == \"candidate-artifact-draft-summary\" ||",
)

replace_once(
    "src/cli.cpp",
    "    if (options.query == \"report\") {\n        return FormattedCommandResult{format_report(state, options.access, options.archive_year), EXIT_SUCCESS};\n    }\n    if (options.query == \"candidate-artifact-draft-summary\") {",
    "    if (options.query == \"report\") {\n        return FormattedCommandResult{format_report(state, options.access, options.archive_year), EXIT_SUCCESS};\n    }\n    if (options.query == \"list-candidate-artifact-proposals\") {\n        return FormattedCommandResult{format_candidate_artifact_proposal_list(state, options.access), EXIT_SUCCESS};\n    }\n    if (options.query == \"show-candidate-artifact-proposal\") {\n        return FormattedCommandResult{format_candidate_artifact_proposal_detail(state, options.access, options.candidate_artifact_proposal_id), EXIT_SUCCESS};\n    }\n    if (options.query == \"candidate-artifact-draft-summary\") {",
)

# Add fixed-fixture session smoke coverage for the newly supported proposal list/detail pair.
replace_once(
    "scripts/smoke_test_cli_workflows.sh",
    "run_session_and_grep runtime_session_candidate_draft_list_and_detail \"RuntimeSession initialized|CandidateArtifactDrafts visible to curator|CandidateArtifactDraft detail:|- found: true|RuntimeSession ended\" \"--access curator --query list-candidate-artifact-drafts\n--access curator --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket\nend-session\n\" \"$BIN\" --session --runtime fixed-fixture\n",
    "run_session_and_grep runtime_session_candidate_draft_list_and_detail \"RuntimeSession initialized|CandidateArtifactDrafts visible to curator|CandidateArtifactDraft detail:|- found: true|RuntimeSession ended\" \"--access curator --query list-candidate-artifact-drafts\n--access curator --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket\nend-session\n\" \"$BIN\" --session --runtime fixed-fixture\nrun_session_and_grep runtime_session_candidate_proposal_list_and_detail \"RuntimeSession initialized|CandidateArtifactProposals visible to curator|CandidateArtifactProposal:|- found: true|RuntimeSession ended\" \"--access curator --query list-candidate-artifact-proposals\n--access curator --query show-candidate-artifact-proposal --candidate-artifact-proposal-id candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket\nend-session\n\" \"$BIN\" --session --runtime fixed-fixture\n",
)

print("Applied v29.3 RuntimeSession query coverage 2 patch.")
