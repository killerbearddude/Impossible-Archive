#pragma once
#include "archive_engine_state.h"
#include "knowledge_horizon_model.h"

namespace archive {

[[nodiscard]] KnowledgeHorizonFinding evaluate_knowledge_reference(
    const ArchiveEngineState& state,
    KnowledgeContextType context_type,
    const std::string& context_id,
    KnowledgeSubjectType subject_type,
    const std::string& subject_id,
    int context_year,
    AccessLevel access
);

[[nodiscard]] KnowledgeHorizonReport validate_knowledge_horizon(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_knowledge_horizon_summary(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_knowledge_horizon_validation(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_knowledge_horizon_findings(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_knowledge_horizon_finding_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& finding_id
);

} // namespace archive
