#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise SystemExit(f"expected text not found in {path}: {old[:200]!r}")
    p.write_text(text.replace(old, new, 1))


# Add opt-in session flag to CLI options.
replace_once(
    "src/cli_model.h",
    "    bool self_test = false;\n",
    "    bool self_test = false;\n    bool runtime_session = false;\n",
)

# Parse --session / --runtime-session.
replace_once(
    "src/cli.cpp",
    "        } else if (arg == \"--self-test\") {\n            options.self_test = true;\n        } else if (arg == \"--seed\") {\n",
    "        } else if (arg == \"--self-test\") {\n            options.self_test = true;\n        } else if (arg == \"--session\" || arg == \"--runtime-session\") {\n            options.runtime_session = true;\n        } else if (arg == \"--seed\") {\n",
)

# Update usage header and examples without broad text churn.
replace_once(
    "src/cli.cpp",
    "    out << \"Usage: \" << exe << \" [--runtime fixed-fixture|spec-selected]",
    "    out << \"Usage: \" << exe << \" [--session] [--runtime fixed-fixture|spec-selected]",
)
replace_once(
    "src/cli.cpp",
    "    out << \"  \" << exe << \" --self-test\\n\";\n",
    "    out << \"  \" << exe << \" --self-test\\n\";\n    out << \"  printf '%s\\\\n' '--query candidate-artifact-draft-review-summary' '--query control-layer-audit-summary' 'end-session' | \" << exe << \" --session\\n\";\n",
)

session_helpers = r'''
[[nodiscard]] std::vector<std::string> split_session_command_line(std::string_view line) {
    std::istringstream input{std::string(line)};
    std::vector<std::string> tokens;
    std::string token;
    while (input >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

[[nodiscard]] CliArgs make_session_cli_args(const std::vector<std::string>& tokens) {
    CliArgs args;
    args.reserve(tokens.size() + 1U);
    args.push_back("session-query");
    for (const std::string& token : tokens) {
        args.push_back(token);
    }
    return args;
}

[[nodiscard]] CliOptions runtime_session_initialization_options(CliOptions options) {
    options.runtime_session = false;
    options.query = "report";
    return options;
}

[[nodiscard]] std::string runtime_session_source_label(const RuntimeStateSelectionResult& selection) {
    std::ostringstream out;
    out << (selection.selection.mode == ArchiveRuntimeMode::SpecSelected ? "spec-selected" : "fixed-fixture");
    if (!selection.selection.spec_file.empty()) {
        out << ":" << selection.selection.spec_file;
    }
    if (!selection.selection.civilization_id.empty()) {
        out << ":" << selection.selection.civilization_id;
    }
    return out.str();
}

[[nodiscard]] bool is_runtime_session_end_command(std::string_view query) {
    return query == "end-session" || query == "quit" || query == "exit";
}

[[nodiscard]] bool is_runtime_session_cli_supported_query(std::string_view query) {
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

void record_runtime_session_unsupported(RuntimeSession& session, std::string_view query) {
    ++session.rejected_query_count;
    session.command_history_summary.push_back("unsupported:" + std::string(query));
    constexpr std::size_t max_history = 32U;
    if (session.command_history_summary.size() > max_history) {
        session.command_history_summary.erase(session.command_history_summary.begin());
    }
}

int run_runtime_session_cli(const CliOptions& options) {
    const CliOptions init_options = runtime_session_initialization_options(options);
    const RuntimeStateSelectionResult runtime_state = build_runtime_state_for_query(init_options);
    if (!runtime_state.ok) {
        std::cout << format_runtime_selection_errors(init_options, runtime_state);
        return EXIT_FAILURE;
    }

    RuntimeSession session = initialize_runtime_session(runtime_state.state, runtime_session_source_label(runtime_state));
    std::cout << "RuntimeSession initialized:\n";
    std::cout << format_runtime_session_summary(session);

    std::string line;
    while (std::getline(std::cin, line)) {
        const std::vector<std::string> tokens = split_session_command_line(line);
        if (tokens.empty()) {
            continue;
        }
        if (tokens.size() == 1U && is_runtime_session_end_command(tokens.front())) {
            end_runtime_session(session, "explicit_end_command");
            std::cout << "RuntimeSession ended:\n";
            std::cout << format_runtime_session_summary(session);
            return EXIT_SUCCESS;
        }

        CliOptions query_options;
        try {
            query_options = parse_cli(make_session_cli_args(tokens));
        } catch (const std::exception& ex) {
            ++session.rejected_query_count;
            std::cout << "RuntimeSession query rejected:\n";
            std::cout << "- query: <parse-error>\n";
            std::cout << "- policy: denied_unknown\n";
            std::cout << "- error: " << ex.what() << "\n";
            std::cout << "- session_active: " << (session.active() ? "true" : "false") << "\n";
            continue;
        }

        if (is_runtime_session_end_command(query_options.query)) {
            end_runtime_session(session, "explicit_end_query");
            std::cout << "RuntimeSession ended:\n";
            std::cout << format_runtime_session_summary(session);
            return EXIT_SUCCESS;
        }

        const RuntimeSessionQueryPolicy policy = classify_runtime_session_query(query_options.query);
        if (policy != RuntimeSessionQueryPolicy::AllowedReadOnly) {
            const RuntimeSessionCommandResult rejected = record_runtime_session_query(session, query_options.query);
            std::cout << "RuntimeSession query rejected:\n";
            std::cout << "- query: " << rejected.query_name << "\n";
            std::cout << "- policy: " << to_string(rejected.policy) << "\n";
            std::cout << "- message: " << rejected.message << "\n";
            std::cout << "- session_active: " << (session.active() ? "true" : "false") << "\n";
            continue;
        }

        if (!is_runtime_session_cli_supported_query(query_options.query)) {
            record_runtime_session_unsupported(session, query_options.query);
            std::cout << "RuntimeSession query rejected:\n";
            std::cout << "- query: " << query_options.query << "\n";
            std::cout << "- policy: allowed_read_only_but_not_in_cli_loop_subset\n";
            std::cout << "- message: read-only query is not implemented in the v29.2 session CLI loop subset yet\n";
            std::cout << "- session_active: " << (session.active() ? "true" : "false") << "\n";
            continue;
        }

        const RuntimeSessionCommandResult accepted = record_runtime_session_query(session, query_options.query);
        std::cout << "RuntimeSession query accepted:\n";
        std::cout << "- query: " << accepted.query_name << "\n";
        std::cout << "- policy: " << to_string(accepted.policy) << "\n";
        const FormattedCommandResult result = format_runtime_session_state_query(session, query_options);
        std::cout << result.text;
    }

    end_runtime_session(session, "input_eof");
    std::cout << "RuntimeSession ended:\n";
    std::cout << format_runtime_session_summary(session);
    return EXIT_SUCCESS;
}

'''

replace_once(
    "src/cli.cpp",
    "int run_cli(const CliArgs& args) {\n",
    session_helpers + "int run_cli(const CliArgs& args) {\n",
)
replace_once(
    "src/cli.cpp",
    "    if (options.query == \"help\") {\n        const std::string_view exe = args.empty() ? std::string_view{\"impossible_archive_mvp\"} : args.front();\n        std::cout << usage(exe);\n        return EXIT_SUCCESS;\n    }\n\n    if (is_civilization_spec_query(options.query)) {\n",
    "    if (options.query == \"help\") {\n        const std::string_view exe = args.empty() ? std::string_view{\"impossible_archive_mvp\"} : args.front();\n        std::cout << usage(exe);\n        return EXIT_SUCCESS;\n    }\n\n    if (options.runtime_session || options.query == \"runtime-session\") {\n        return run_runtime_session_cli(options);\n    }\n\n    if (is_civilization_spec_query(options.query)) {\n",
)

# Add smoke helper and tests.
replace_once(
    "scripts/smoke_test_cli_workflows.sh",
    "run_failure_and_grep() {\n  local name=\"$1\"\n  local pattern=\"$2\"\n  shift 2\n  local output=\"/tmp/impossible_archive_smoke_${name//[^A-Za-z0-9_]/_}.txt\"\n  echo \"[smoke] expected failure: $name\"\n  if \"$@\" >\"$output\" 2>&1; then\n    echo \"[smoke] command unexpectedly succeeded: $*\" >&2\n    cat \"$output\" >&2\n    exit 1\n  fi\n  if ! grep -qE \"$pattern\" \"$output\"; then\n    echo \"[smoke] expected failure pattern not found for $name: $pattern\" >&2\n    cat \"$output\" >&2\n    exit 1\n  fi\n}\n",
    "run_failure_and_grep() {\n  local name=\"$1\"\n  local pattern=\"$2\"\n  shift 2\n  local output=\"/tmp/impossible_archive_smoke_${name//[^A-Za-z0-9_]/_}.txt\"\n  echo \"[smoke] expected failure: $name\"\n  if \"$@\" >\"$output\" 2>&1; then\n    echo \"[smoke] command unexpectedly succeeded: $*\" >&2\n    cat \"$output\" >&2\n    exit 1\n  fi\n  if ! grep -qE \"$pattern\" \"$output\"; then\n    echo \"[smoke] expected failure pattern not found for $name: $pattern\" >&2\n    cat \"$output\" >&2\n    exit 1\n  fi\n}\n\nrun_session_and_grep() {\n  local name=\"$1\"\n  local pattern=\"$2\"\n  local input=\"$3\"\n  shift 3\n  local output=\"/tmp/impossible_archive_smoke_${name//[^A-Za-z0-9_]/_}.txt\"\n  echo \"[smoke] $name\"\n  printf \"%b\" \"$input\" | \"$@\" >\"$output\"\n  if ! grep -qE \"$pattern\" \"$output\"; then\n    echo \"[smoke] expected pattern not found for $name: $pattern\" >&2\n    cat \"$output\" >&2\n    exit 1\n  fi\n}\n",
)

append_after = """run_and_grep control_layer_audit_compare_snapshots "ArchiveSnapshot comparison|result: same|control_layer_audit_entry_count" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive
"""
session_smokes = """run_and_grep control_layer_audit_compare_snapshots "ArchiveSnapshot comparison|result: same|control_layer_audit_entry_count" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive

run_session_and_grep runtime_session_two_read_queries "RuntimeSession initialized|CandidateArtifactDraftReview summary|ControlLayerAudit summary|RuntimeSession ended" "--query candidate-artifact-draft-review-summary\n--query control-layer-audit-summary\nend-session\n" "$BIN" --session
run_session_and_grep runtime_session_invalid_query_recovers "RuntimeSession query rejected|denied_unknown|session_active: true|CandidateArtifactDraftReview summary|RuntimeSession ended" "--query definitely-not-a-query\n--query candidate-artifact-draft-review-summary\nend-session\n" "$BIN" --session
run_session_and_grep runtime_session_mutating_query_denied "RuntimeSession query rejected|denied_mutating|materialize-hidden-cluster|session_active: true|CandidateArtifactDraftReview summary|RuntimeSession ended" "--query materialize-hidden-cluster\n--query candidate-artifact-draft-review-summary\nend-session\n" "$BIN" --session
"""
replace_once("scripts/smoke_test_cli_workflows.sh", append_after, session_smokes)

print("Applied v29.2 runtime session CLI loop patch.")
