#pragma once
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
