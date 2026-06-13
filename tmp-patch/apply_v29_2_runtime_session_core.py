#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise SystemExit(f"expected text not found in {path}: {old[:160]!r}")
    p.write_text(text.replace(old, new, 1))


Path("src/runtime_session_model.h").write_text(r'''#pragma once
#include "archive_engine_state.h"

namespace archive {

enum class RuntimeSessionState {
    Inactive,
    Active,
    Ended,
};

enum class RuntimeSessionQueryPolicy {
    AllowedReadOnly,
    DeniedMutating,
    DeniedUnknown,
    DeniedSessionControl,
};

struct RuntimeSession {
    RuntimeSessionState lifecycle = RuntimeSessionState::Inactive;
    ArchiveEngineState state;
    std::string source_label;
    std::vector<std::string> command_history_summary;
    std::size_t accepted_query_count = 0;
    std::size_t rejected_query_count = 0;

    [[nodiscard]] bool active() const {
        return lifecycle == RuntimeSessionState::Active;
    }
};

struct RuntimeSessionCommandResult {
    bool accepted = false;
    RuntimeSessionQueryPolicy policy = RuntimeSessionQueryPolicy::DeniedUnknown;
    std::string query_name;
    std::string message;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(RuntimeSessionState state);
[[nodiscard]] std::string to_string(RuntimeSessionQueryPolicy policy);

} // namespace archive
''')

Path("src/runtime_session_api.h").write_text(r'''#pragma once
#include "runtime_session_model.h"

namespace archive {

[[nodiscard]] RuntimeSession initialize_runtime_session(ArchiveEngineState state, std::string source_label);
void end_runtime_session(RuntimeSession& session, std::string summary_note = {});

[[nodiscard]] RuntimeSessionQueryPolicy classify_runtime_session_query(std::string_view query_name);
[[nodiscard]] bool runtime_session_query_allowed(std::string_view query_name);
[[nodiscard]] bool runtime_session_query_denied_as_mutating(std::string_view query_name);

[[nodiscard]] RuntimeSessionCommandResult record_runtime_session_query(RuntimeSession& session, std::string_view query_name);
[[nodiscard]] std::vector<std::string> validate_runtime_session(const RuntimeSession& session);
[[nodiscard]] std::string format_runtime_session_summary(const RuntimeSession& session);

} // namespace archive
''')

Path("src/runtime_session.cpp").write_text(r'''#include "runtime_session_api.h"
#include "validation_api.h"

#include <algorithm>
#include <array>
#include <sstream>

namespace archive {
namespace {

constexpr std::array<std::string_view, 66> kReadOnlyQueries = {
    "report",
    "validate-civilization-specs",
    "list-civilization-tags",
    "list-civilizations-by-tag",
    "show-civilization-spec",
    "bootstrap-civilization",
    "list-generation-targets",
    "generate-candidates",
    "hidden-cluster",
    "validate-civilization-fragments",
    "list-civilization-fragments",
    "show-civilization-fragment",
    "list-golden-fixtures",
    "show-golden-fixture",
    "archive-snapshot",
    "compare-archive-snapshots",
    "evidence-potential-summary",
    "list-evidence-potentials",
    "validate-evidence-potentials",
    "knowledge-horizon-summary",
    "validate-knowledge-horizon",
    "list-knowledge-horizon-findings",
    "show-knowledge-horizon-finding",
    "contradiction-budget-summary",
    "validate-contradiction-budget",
    "list-contradiction-budget-buckets",
    "show-contradiction-budget-bucket",
    "candidate-artifact-plan-summary",
    "validate-candidate-artifact-plans",
    "list-candidate-artifact-plans",
    "show-candidate-artifact-plan",
    "candidate-artifact-plan-evaluation-summary",
    "validate-candidate-artifact-plan-evaluations",
    "list-candidate-artifact-plan-evaluations",
    "show-candidate-artifact-plan-evaluation",
    "candidate-artifact-proposal-summary",
    "validate-candidate-artifact-proposals",
    "list-candidate-artifact-proposals",
    "show-candidate-artifact-proposal",
    "candidate-artifact-proposal-audit-summary",
    "validate-candidate-artifact-proposal-audits",
    "list-candidate-artifact-proposal-audits",
    "show-candidate-artifact-proposal-audit",
    "candidate-artifact-draft-summary",
    "validate-candidate-artifact-drafts",
    "list-candidate-artifact-drafts",
    "show-candidate-artifact-draft",
    "candidate-artifact-draft-review-summary",
    "validate-candidate-artifact-draft-reviews",
    "list-candidate-artifact-draft-reviews",
    "show-candidate-artifact-draft-review",
    "control-layer-audit-summary",
    "validate-control-layer-audit",
    "list-control-layer-audit-entries",
    "show-control-layer-audit-entry",
    "list-targets",
    "list-tags",
    "show-spec",
    "validate-specs",
    "list-fragments",
    "show-fragment",
    "validate-fragments",
    "list-fixtures",
    "show-fixture",
    "self-test",
    "help",
};

constexpr std::array<std::string_view, 2> kMutatingQueries = {
    "materialize-hidden-cluster",
    "materialize-hidden-mutation-artifact-candidate",
};

constexpr std::array<std::string_view, 4> kSessionControlQueries = {
    "start-session",
    "initialize-session",
    "end-session",
    "quit",
};

[[nodiscard]] bool contains_query(const auto& queries, std::string_view query_name) {
    return std::find(queries.begin(), queries.end(), query_name) != queries.end();
}

void append_history(RuntimeSession& session, std::string_view entry) {
    session.command_history_summary.push_back(std::string(entry));
    constexpr std::size_t max_history = 32U;
    if (session.command_history_summary.size() > max_history) {
        session.command_history_summary.erase(session.command_history_summary.begin());
    }
}

} // namespace

[[nodiscard]] std::string to_string(RuntimeSessionState state) {
    switch (state) {
        case RuntimeSessionState::Inactive: return "inactive";
        case RuntimeSessionState::Active: return "active";
        case RuntimeSessionState::Ended: return "ended";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(RuntimeSessionQueryPolicy policy) {
    switch (policy) {
        case RuntimeSessionQueryPolicy::AllowedReadOnly: return "allowed_read_only";
        case RuntimeSessionQueryPolicy::DeniedMutating: return "denied_mutating";
        case RuntimeSessionQueryPolicy::DeniedUnknown: return "denied_unknown";
        case RuntimeSessionQueryPolicy::DeniedSessionControl: return "denied_session_control";
    }
    return "unknown";
}

[[nodiscard]] RuntimeSession initialize_runtime_session(ArchiveEngineState state, std::string source_label) {
    RuntimeSession session;
    session.lifecycle = RuntimeSessionState::Active;
    session.state = std::move(state);
    session.source_label = source_label.empty() ? std::string{"unspecified"} : std::move(source_label);
    append_history(session, "initialize-session");
    return session;
}

void end_runtime_session(RuntimeSession& session, std::string summary_note) {
    if (!summary_note.empty()) {
        append_history(session, summary_note);
    }
    append_history(session, "end-session");
    session.lifecycle = RuntimeSessionState::Ended;
}

[[nodiscard]] RuntimeSessionQueryPolicy classify_runtime_session_query(std::string_view query_name) {
    if (query_name.empty()) {
        return RuntimeSessionQueryPolicy::DeniedUnknown;
    }
    if (contains_query(kSessionControlQueries, query_name)) {
        return RuntimeSessionQueryPolicy::DeniedSessionControl;
    }
    if (contains_query(kMutatingQueries, query_name)) {
        return RuntimeSessionQueryPolicy::DeniedMutating;
    }
    if (contains_query(kReadOnlyQueries, query_name)) {
        return RuntimeSessionQueryPolicy::AllowedReadOnly;
    }
    return RuntimeSessionQueryPolicy::DeniedUnknown;
}

[[nodiscard]] bool runtime_session_query_allowed(std::string_view query_name) {
    return classify_runtime_session_query(query_name) == RuntimeSessionQueryPolicy::AllowedReadOnly;
}

[[nodiscard]] bool runtime_session_query_denied_as_mutating(std::string_view query_name) {
    return classify_runtime_session_query(query_name) == RuntimeSessionQueryPolicy::DeniedMutating;
}

[[nodiscard]] RuntimeSessionCommandResult record_runtime_session_query(RuntimeSession& session, std::string_view query_name) {
    RuntimeSessionCommandResult result;
    result.query_name = std::string(query_name);
    result.policy = classify_runtime_session_query(query_name);

    if (!session.active()) {
        result.message = "runtime session is not active";
        result.errors.push_back(result.message);
        ++session.rejected_query_count;
        return result;
    }
    if (result.policy != RuntimeSessionQueryPolicy::AllowedReadOnly) {
        result.message = "query is not allowed in v29.2 runtime session core: " + std::string(query_name);
        result.errors.push_back(result.message);
        ++session.rejected_query_count;
        append_history(session, "rejected:" + std::string(query_name));
        return result;
    }

    result.accepted = true;
    result.message = "query accepted for read-only session execution seam";
    ++session.accepted_query_count;
    append_history(session, "accepted:" + std::string(query_name));
    return result;
}

[[nodiscard]] std::vector<std::string> validate_runtime_session(const RuntimeSession& session) {
    std::vector<std::string> errors;
    if (session.lifecycle == RuntimeSessionState::Active && session.source_label.empty()) {
        errors.push_back("active RuntimeSession has empty source label");
    }
    if (session.lifecycle == RuntimeSessionState::Inactive && session.accepted_query_count != 0U) {
        errors.push_back("inactive RuntimeSession has accepted query count");
    }
    if (session.command_history_summary.size() > 32U) {
        errors.push_back("RuntimeSession command history exceeds compact summary limit");
    }
    std::vector<std::string> state_errors = validate_full_state(session.state);
    errors.insert(errors.end(), state_errors.begin(), state_errors.end());
    return errors;
}

[[nodiscard]] std::string format_runtime_session_summary(const RuntimeSession& session) {
    const std::vector<std::string> errors = validate_runtime_session(session);
    std::ostringstream out;
    out << "RuntimeSession summary:\n";
    out << "- lifecycle: " << to_string(session.lifecycle) << "\n";
    out << "- source_label: " << session.source_label << "\n";
    out << "- accepted_query_count: " << session.accepted_query_count << "\n";
    out << "- rejected_query_count: " << session.rejected_query_count << "\n";
    out << "- command_history_summary_count: " << session.command_history_summary.size() << "\n";
    out << "- validation: " << (errors.empty() ? "passed" : "failed") << "\n";
    if (!errors.empty()) {
        out << "Validation errors:\n";
        for (const std::string& error : errors) {
            out << "- " << error << "\n";
        }
    }
    out << "- persistence: memory_only\n";
    out << "- query_execution: read_only_policy_seam_only\n";
    return out.str();
}

} // namespace archive
''')

replace_once(
    "src/impossible_archive.h",
    '#include "golden_fixture_model.h"\n#include "golden_fixtures_api.h"\n',
    '#include "golden_fixture_model.h"\n#include "golden_fixtures_api.h"\n#include "runtime_session_model.h"\n#include "runtime_session_api.h"\n',
)

print("Applied v29.2 runtime session core seam.")
