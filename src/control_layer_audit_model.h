#pragma once
#include "archive_common.h"

namespace archive {

enum class ControlLayerKind {
    CoreState,
    Fixture,
    Snapshot,
    Validation,
    Telemetry,
    Planning,
    Evaluation,
    Proposal,
    Audit,
    MutationWorkflow,
    Formatting,
    CLI,
};

enum class ControlLayerPersistence {
    PersistentState,
    DerivedCachedState,
    DerivedViewOnly,
    ReportOnly,
    Unknown,
};

enum class ControlLayerBehavior {
    RuntimeEnforced,
    ValidationOnly,
    TelemetryOnly,
    PlanningOnly,
    EvaluationOnly,
    ProposalOnly,
    AuditOnly,
    FormattingOnly,
    MutationCapable,
    Unknown,
};

enum class ControlLayerRisk {
    Low,
    Moderate,
    High,
    Unknown,
};

struct ControlLayerAuditEntry {
    std::string id;
    std::string name;

    ControlLayerKind kind = ControlLayerKind::CoreState;
    ControlLayerPersistence persistence = ControlLayerPersistence::Unknown;
    ControlLayerBehavior behavior = ControlLayerBehavior::Unknown;
    ControlLayerRisk risk = ControlLayerRisk::Unknown;

    bool access_gated = false;
    bool public_detail_gated = false;
    bool snapshot_covered = false;
    bool summary_digest_covered = false;
    bool smoke_covered = false;
    bool self_test_covered = false;
    bool full_state_validation_covered = false;
    bool can_mutate_state = false;
    bool should_remain_inert = false;

    std::vector<std::string> primary_files;
    std::vector<std::string> cli_queries;
    std::vector<std::string> validation_functions;
    std::vector<std::string> snapshot_fields;
    std::vector<std::string> known_gaps;
    std::vector<std::string> notes;
};

struct ControlLayerAuditReport {
    std::vector<ControlLayerAuditEntry> entries;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(ControlLayerKind kind);
[[nodiscard]] std::string to_string(ControlLayerPersistence persistence);
[[nodiscard]] std::string to_string(ControlLayerBehavior behavior);
[[nodiscard]] std::string to_string(ControlLayerRisk risk);

} // namespace archive
