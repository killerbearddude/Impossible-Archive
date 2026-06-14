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
    if "query == \"candidate-artifact-plan-evaluation-summary\"" in segment:
        return segment
    return insert_before_once(
        segment,
        "           query == \"list-candidate-artifact-proposals\" ||\n",
        "           query == \"candidate-artifact-plan-evaluation-summary\" ||\n"
        "           query == \"validate-candidate-artifact-plan-evaluations\" ||\n"
        "           query == \"list-candidate-artifact-plan-evaluations\" ||\n"
        "           query == \"show-candidate-artifact-plan-evaluation\" ||\n",
    )


def expand_shared_formatter(segment: str) -> str:
    if "format_candidate_artifact_plan_evaluation_summary(state, options.access)" in segment:
        return segment
    return insert_before_once(
        segment,
        "    if (options.query == \"list-candidate-artifact-proposals\") {\n",
        "    if (options.query == \"candidate-artifact-plan-evaluation-summary\") {\n"
        "        return FormattedCommandResult{format_candidate_artifact_plan_evaluation_summary(state, options.access), EXIT_SUCCESS};\n"
        "    }\n"
        "    if (options.query == \"validate-candidate-artifact-plan-evaluations\") {\n"
        "        return FormattedCommandResult{format_candidate_artifact_plan_evaluation_validation(state, options.access), EXIT_SUCCESS};\n"
        "    }\n"
        "    if (options.query == \"list-candidate-artifact-plan-evaluations\") {\n"
        "        return FormattedCommandResult{format_candidate_artifact_plan_evaluation_list(state, options.access), EXIT_SUCCESS};\n"
        "    }\n"
        "    if (options.query == \"show-candidate-artifact-plan-evaluation\") {\n"
        "        return FormattedCommandResult{format_candidate_artifact_plan_evaluation_detail(state, options.access, options.candidate_artifact_plan_evaluation_id), EXIT_SUCCESS};\n"
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
    if "runtime_session_candidate_plan_evaluation_summary_and_validation" in text:
        return text

    insertion = (
        "run_session_and_grep runtime_session_candidate_plan_evaluation_summary_and_validation "
        "\"RuntimeSession initialized|CandidateArtifactPlanEvaluation summary|CandidateArtifactPlanEvaluation validation|RuntimeSession ended\" "
        "\"--query candidate-artifact-plan-evaluation-summary\n"
        "--query validate-candidate-artifact-plan-evaluations\n"
        "end-session\n"
        "\" \"$BIN\" --session\n"
        "run_session_and_grep runtime_session_candidate_plan_evaluation_list_and_detail "
        "\"RuntimeSession initialized|CandidateArtifactPlanEvaluations visible to curator|CandidateArtifactPlanEvaluation:|- found: true|RuntimeSession ended\" "
        "\"--access curator --query list-candidate-artifact-plan-evaluations\n"
        "--access curator --query show-candidate-artifact-plan-evaluation --candidate-artifact-plan-evaluation-id candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket\n"
        "end-session\n"
        "\" \"$BIN\" --session --runtime fixed-fixture\n"
    )

    return insert_before_once(
        text,
        "run_session_and_grep runtime_session_candidate_plan_summary_and_validation ",
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

if sorted(changed) != ["scripts/smoke_test_cli_workflows.sh", "src/cli.cpp"]:
    raise SystemExit("expected src/cli.cpp and scripts/smoke_test_cli_workflows.sh to change; changed=" + ", ".join(changed))

print("Applied v29.3 RuntimeSession query coverage 5 patch.")
print("Changed files: " + ", ".join(changed))
