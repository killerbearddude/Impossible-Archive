#!/usr/bin/env python3
from pathlib import Path


def update(path: str, transform) -> None:
    p = Path(path)
    text = p.read_text()
    new_text = transform(text)
    if new_text == text:
        raise SystemExit(f"no changes made to {path}")
    p.write_text(new_text)


def insert_before_once(text: str, marker: str, insertion: str) -> str:
    if marker not in text:
        raise SystemExit(f"expected marker not found: {marker[:160]!r}")
    return text.replace(marker, insertion + marker, 1)


def expand_cli(text: str) -> str:
    if "query == \"list-candidate-artifact-proposals\"" not in text:
        text = insert_before_once(
            text,
            "           query == \"candidate-artifact-draft-summary\" ||\n",
            "           query == \"list-candidate-artifact-proposals\" ||\n"
            "           query == \"show-candidate-artifact-proposal\" ||\n",
        )

    if "format_candidate_artifact_proposal_list(state, options.access)" not in text:
        text = insert_before_once(
            text,
            "    if (options.query == \"candidate-artifact-draft-summary\") {\n",
            "    if (options.query == \"list-candidate-artifact-proposals\") {\n"
            "        return FormattedCommandResult{format_candidate_artifact_proposal_list(state, options.access), EXIT_SUCCESS};\n"
            "    }\n"
            "    if (options.query == \"show-candidate-artifact-proposal\") {\n"
            "        return FormattedCommandResult{format_candidate_artifact_proposal_detail(state, options.access, options.candidate_artifact_proposal_id), EXIT_SUCCESS};\n"
            "    }\n",
        )

    return text


def expand_smoke(text: str) -> str:
    if "runtime_session_candidate_proposal_list_and_detail" in text:
        return text

    insertion = (
        "run_session_and_grep runtime_session_candidate_proposal_list_and_detail "
        "\"RuntimeSession initialized|CandidateArtifactProposals visible to curator|CandidateArtifactProposal:|- found: true|RuntimeSession ended\" "
        "\"--access curator --query list-candidate-artifact-proposals\n"
        "--access curator --query show-candidate-artifact-proposal --candidate-artifact-proposal-id candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket\n"
        "end-session\n"
        "\" \"$BIN\" --session --runtime fixed-fixture\n"
    )
    return insert_before_once(
        text,
        "run_session_and_grep runtime_session_invalid_query_recovers ",
        insertion,
    )


update("src/cli.cpp", expand_cli)
update("scripts/smoke_test_cli_workflows.sh", expand_smoke)

print("Applied v29.3 RuntimeSession query coverage 2 patch.")
