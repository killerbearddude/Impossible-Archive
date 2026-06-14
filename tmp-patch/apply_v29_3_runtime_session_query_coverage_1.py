#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise SystemExit(f"expected text not found in {path}: {old[:200]!r}")
    p.write_text(text.replace(old, new, 1))


# Expand the existing shared read-only session subset by the draft list/detail pair.
replace_once(
    "src/cli.cpp",
    "           query == \"candidate-artifact-draft-summary\" ||\n           query == \"validate-candidate-artifact-drafts\" ||\n           query == \"candidate-artifact-draft-review-summary\" ||",
    "           query == \"candidate-artifact-draft-summary\" ||\n           query == \"validate-candidate-artifact-drafts\" ||\n           query == \"list-candidate-artifact-drafts\" ||\n           query == \"show-candidate-artifact-draft\" ||\n           query == \"candidate-artifact-draft-review-summary\" ||",
)

replace_once(
    "src/cli.cpp",
    "    if (options.query == \"validate-candidate-artifact-drafts\") {\n        return FormattedCommandResult{format_candidate_artifact_draft_validation(state, options.access), EXIT_SUCCESS};\n    }\n    if (options.query == \"candidate-artifact-draft-review-summary\") {",
    "    if (options.query == \"validate-candidate-artifact-drafts\") {\n        return FormattedCommandResult{format_candidate_artifact_draft_validation(state, options.access), EXIT_SUCCESS};\n    }\n    if (options.query == \"list-candidate-artifact-drafts\") {\n        return FormattedCommandResult{format_candidate_artifact_draft_list(state, options.access), EXIT_SUCCESS};\n    }\n    if (options.query == \"show-candidate-artifact-draft\") {\n        return FormattedCommandResult{format_candidate_artifact_draft_detail(state, options.access, options.candidate_artifact_draft_id), EXIT_SUCCESS};\n    }\n    if (options.query == \"candidate-artifact-draft-review-summary\") {",
)

# Add smoke coverage for the newly supported session queries.
replace_once(
    "scripts/smoke_test_cli_workflows.sh",
    "run_session_and_grep runtime_session_two_read_queries \"RuntimeSession initialized|CandidateArtifactDraftReview summary|ControlLayerAudit summary|RuntimeSession ended\" \"--query candidate-artifact-draft-review-summary\n--query control-layer-audit-summary\nend-session\n\" \"$BIN\" --session\n",
    "run_session_and_grep runtime_session_two_read_queries \"RuntimeSession initialized|CandidateArtifactDraftReview summary|ControlLayerAudit summary|RuntimeSession ended\" \"--query candidate-artifact-draft-review-summary\n--query control-layer-audit-summary\nend-session\n\" \"$BIN\" --session\nrun_session_and_grep runtime_session_candidate_draft_list_and_detail \"RuntimeSession initialized|CandidateArtifactDrafts visible to curator|CandidateArtifactDraft detail:|- found: true|RuntimeSession ended\" \"--access curator --query list-candidate-artifact-drafts\n--access curator --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket\nend-session\n\" \"$BIN\" --session --runtime fixed-fixture\n",
)

print("Applied v29.3 runtime session query coverage patch.")
