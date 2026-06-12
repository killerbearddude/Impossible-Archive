#pragma once
#include "archive_engine_state.h"
#include "candidate_artifact_plan_model.h"

namespace archive {

[[nodiscard]] CandidateArtifactPlan make_plan_from_evidence_potential(
    const ArchiveEngineState& state,
    const EvidencePotential& potential,
    AccessLevel access
);

[[nodiscard]] CandidateArtifactPlanStatus classify_candidate_artifact_plan_status(
    const ArchiveEngineState& state,
    const CandidateArtifactPlan& plan,
    AccessLevel access
);

[[nodiscard]] CandidateArtifactPlanReport derive_candidate_artifact_plans(
    const ArchiveEngineState& state,
    AccessLevel access
);

void derive_candidate_artifact_plans_into_state(
    ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_plans(
    const ArchiveEngineState& state
);

[[nodiscard]] std::string format_candidate_artifact_plan_summary(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_candidate_artifact_plan_validation(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_candidate_artifact_plan_list(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_candidate_artifact_plan_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& plan_id
);

} // namespace archive
