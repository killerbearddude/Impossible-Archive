#!/usr/bin/env python3
from pathlib import Path


def replace_segment(text: str, start_marker: str, end_marker: str, transform) -> str:
    start = text.find(start_marker)
    if start == -1:
        raise SystemExit(f"start marker not found: {start_marker!r}")
    end = text.find(end_marker, start)
    if end == -1:
        raise SystemExit(f"end marker not found after {start_marker!r}: {end_marker!r}")
    segment = text[start:end]
    new_segment = transform(segment)
    return text[:start] + new_segment + text[end:]


def insert_before_once(text: str, marker: str, insertion: str) -> str:
    if marker not in text:
        raise SystemExit(f"expected marker not found: {marker[:160]!r}")
    return text.replace(marker, insertion + marker, 1)


def expand_shared_query_predicate(segment: str) -> str:
    if "query == \"list-candidate-artifact-proposals\"" in segment:
        return segment
    return insert_before_once(
        segment,
        "           query == \"candidate-artifact-draft-summary\" ||\n",
        "           query == \"list-candidate-artifact-proposals\" ||\n"
        "           query == \"show-candidate-artifact-proposal\" ||\n",
    )


def expand_shared_formatter(segment: str) -> str:
    if "format_candidate_artifact_proposal_list(state, options.access)" in segment:
        return segment
    return insert_before_once(
        segment,
        "    if (options.query == \"candidate-artifact-draft-summary\") {\n",
        "    if (options.query == \"list-candidate-artifact-proposals\") {\n"
        "        return FormattedCommandResult{format_candidate_artifact_proposal_list(state, options.access), EXIT_SUCCESS};\n"
        "    }\n"
        "    if (options.query == \"show-candidate-artifact-proposal\") {\n"
        "        return FormattedCommandResult{format_candidate_artifact_proposal_detail(state, options.access, options.candidate_artifact_proposal_id), EXIT_SUCCESS};\n"
        "    }\n",
    )


def expand_cli(text: str) -> str:
    text = replace_segment(
        text,
        "[[nodiscard]] bool is_shared_read_only_state_query",
        "[[nodiscard]] std::optional<FormattedCommandResult> format_shared_read_only_state_query_result",
        expand_shared_query_predicate,
    )
    text = replace_segment(
        text,
        "[[nodiscard]] std::optional<FormattedCommandResult> format_shared_read_only_state_query_result",
        "[[nodiscard]] bool is_runtime_session_cli_supported_query",
        expand_shared_formatter,
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


paths = [
    ("src/cli.cpp", expand_cli),
    ("scripts/smoke_test_cli_workflows.sh", expand_smoke),
]
changed = []
for path, transform in paths:
    p = Path(path)
    before = p.read_text()
    after = transform(before)
    if after != before:
        p.write_text(after)
        changed.append(path)

if not changed:
    raise SystemExit("no changes made; expected src/cli.cpp and smoke script to change")

print("Applied v29.3 RuntimeSession query coverage 2 patch.")
print("Changed files: " + ", ".join(changed))
