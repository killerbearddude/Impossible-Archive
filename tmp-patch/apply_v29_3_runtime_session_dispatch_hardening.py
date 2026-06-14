#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise SystemExit(f"expected text not found in {path}: {old[:200]!r}")
    p.write_text(text.replace(old, new, 1))


old_session_dispatch = r'''[[nodiscard]] bool is_runtime_session_cli_supported_query(std::string_view query) {
    return query == "report" ||
           query == "candidate-artifact-draft-summary" ||
           query == "validate-candidate-artifact-drafts" ||
           query == "candidate-artifact-draft-review-summary" ||
           query == "validate-candidate-artifact-draft-reviews" ||
           query == "list-candidate-artifact-draft-reviews" ||
           query == "show-candidate-artifact-draft-review" ||
           query == "control-layer-audit-summary" ||
           query == "validate-control-layer-audit" ||
           query == "list-control-layer-audit-entries" ||
           query == "show-control-layer-audit-entry" ||
           query == "evidence-potential-summary" ||
           query == "validate-evidence-potentials" ||
           query == "contradiction-budget-summary" ||
           query == "validate-contradiction-budget";
}

[[nodiscard]] FormattedCommandResult format_runtime_session_state_query(const RuntimeSession& session, const CliOptions& options) {
    const ArchiveEngineState& state = session.state;
    if (options.query == "report") {
        return {format_report(state, options.access, options.archive_year), EXIT_SUCCESS};
    }
    if (options.query == "candidate-artifact-draft-summary") {
        return {format_candidate_artifact_draft_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-candidate-artifact-drafts") {
        return {format_candidate_artifact_draft_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "candidate-artifact-draft-review-summary") {
        return {format_candidate_artifact_draft_review_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-candidate-artifact-draft-reviews") {
        return {format_candidate_artifact_draft_review_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-candidate-artifact-draft-reviews") {
        return {format_candidate_artifact_draft_review_list(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-candidate-artifact-draft-review") {
        return {format_candidate_artifact_draft_review_detail(state, options.access, options.candidate_artifact_draft_review_id), EXIT_SUCCESS};
    }
    if (options.query == "control-layer-audit-summary") {
        return {format_control_layer_audit_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-control-layer-audit") {
        return {format_control_layer_audit_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-control-layer-audit-entries") {
        return {format_control_layer_audit_entries(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-control-layer-audit-entry") {
        return {format_control_layer_audit_entry_detail(state, options.access, options.control_layer_audit_entry_id), EXIT_SUCCESS};
    }
    if (options.query == "evidence-potential-summary") {
        return {format_evidence_potential_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-evidence-potentials") {
        return {format_evidence_potential_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "contradiction-budget-summary") {
        return {format_contradiction_budget_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-contradiction-budget") {
        return {format_contradiction_budget_validation(state, options.access), EXIT_SUCCESS};
    }
    return {"RuntimeSession unsupported query:\n- query: " + options.query + "\n", EXIT_FAILURE};
}
'''

new_session_dispatch = r'''[[nodiscard]] bool is_shared_read_only_state_query(std::string_view query) {
    return query == "report" ||
           query == "candidate-artifact-draft-summary" ||
           query == "validate-candidate-artifact-drafts" ||
           query == "candidate-artifact-draft-review-summary" ||
           query == "validate-candidate-artifact-draft-reviews" ||
           query == "list-candidate-artifact-draft-reviews" ||
           query == "show-candidate-artifact-draft-review" ||
           query == "control-layer-audit-summary" ||
           query == "validate-control-layer-audit" ||
           query == "list-control-layer-audit-entries" ||
           query == "show-control-layer-audit-entry" ||
           query == "evidence-potential-summary" ||
           query == "validate-evidence-potentials" ||
           query == "contradiction-budget-summary" ||
           query == "validate-contradiction-budget";
}

[[nodiscard]] std::optional<FormattedCommandResult> format_shared_read_only_state_query_result(const ArchiveEngineState& state,
                                                                                               const CliOptions& options) {
    if (options.query == "report") {
        return FormattedCommandResult{format_report(state, options.access, options.archive_year), EXIT_SUCCESS};
    }
    if (options.query == "candidate-artifact-draft-summary") {
        return FormattedCommandResult{format_candidate_artifact_draft_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-candidate-artifact-drafts") {
        return FormattedCommandResult{format_candidate_artifact_draft_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "candidate-artifact-draft-review-summary") {
        return FormattedCommandResult{format_candidate_artifact_draft_review_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-candidate-artifact-draft-reviews") {
        return FormattedCommandResult{format_candidate_artifact_draft_review_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-candidate-artifact-draft-reviews") {
        return FormattedCommandResult{format_candidate_artifact_draft_review_list(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-candidate-artifact-draft-review") {
        return FormattedCommandResult{format_candidate_artifact_draft_review_detail(state, options.access, options.candidate_artifact_draft_review_id), EXIT_SUCCESS};
    }
    if (options.query == "control-layer-audit-summary") {
        return FormattedCommandResult{format_control_layer_audit_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-control-layer-audit") {
        return FormattedCommandResult{format_control_layer_audit_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-control-layer-audit-entries") {
        return FormattedCommandResult{format_control_layer_audit_entries(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-control-layer-audit-entry") {
        return FormattedCommandResult{format_control_layer_audit_entry_detail(state, options.access, options.control_layer_audit_entry_id), EXIT_SUCCESS};
    }
    if (options.query == "evidence-potential-summary") {
        return FormattedCommandResult{format_evidence_potential_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-evidence-potentials") {
        return FormattedCommandResult{format_evidence_potential_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "contradiction-budget-summary") {
        return FormattedCommandResult{format_contradiction_budget_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-contradiction-budget") {
        return FormattedCommandResult{format_contradiction_budget_validation(state, options.access), EXIT_SUCCESS};
    }
    return std::nullopt;
}

[[nodiscard]] bool is_runtime_session_cli_supported_query(std::string_view query) {
    return is_shared_read_only_state_query(query);
}

[[nodiscard]] FormattedCommandResult format_runtime_session_state_query(const RuntimeSession& session, const CliOptions& options) {
    const std::optional<FormattedCommandResult> shared = format_shared_read_only_state_query_result(session.state, options);
    if (shared.has_value()) {
        return *shared;
    }
    return {"RuntimeSession unsupported query:\n- query: " + options.query + "\n", EXIT_FAILURE};
}
'''

replace_once("src/cli.cpp", old_session_dispatch, new_session_dispatch)

replace_once(
    "src/cli.cpp",
    "    ArchiveEngineState state = runtime_state.state;\n\n    if (options.query == \"report\") {\n",
    "    ArchiveEngineState state = runtime_state.state;\n\n    if (const std::optional<FormattedCommandResult> shared = format_shared_read_only_state_query_result(state, options)) {\n        std::cout << shared->text;\n        return shared->exit_code;\n    }\n\n    if (options.query == \"report\") {\n",
)

print("Applied v29.3 runtime session dispatch hardening patch.")
