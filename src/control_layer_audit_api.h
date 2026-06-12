#pragma once
#include "archive_engine_state.h"
#include "control_layer_audit_model.h"

namespace archive {

[[nodiscard]] ControlLayerAuditReport build_control_layer_audit_report();
void build_control_layer_audit_into_state(ArchiveEngineState& state);
[[nodiscard]] std::vector<std::string> validate_control_layer_audit_report(const ControlLayerAuditReport& report);

[[nodiscard]] std::string format_control_layer_audit_summary(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_control_layer_audit_validation(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_control_layer_audit_entries(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_control_layer_audit_entry_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& entry_id
);

} // namespace archive
