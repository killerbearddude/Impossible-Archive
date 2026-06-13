#pragma once
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
