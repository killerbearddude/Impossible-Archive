#pragma once
#include "archive_engine_state.h"
#include "candidate_artifact_plan_evaluation_model.h"

namespace archive {

[[nodiscard]] CandidateArtifactPlanEvaluation evaluate_candidate_artifact_plan(
    const ArchiveEngineState& state,
    const CandidateArtifactPlan& plan,
    AccessLevel access
);

[[nodiscard]] CandidateArtifactPlanEvaluationDecision classify_candidate_artifact_plan_evaluation(
    const CandidateArtifactPlanEvaluation& evaluation
);

[[nodiscard]] CandidateArtifactPlanEvaluationReport evaluate_candidate_artifact_plans(
    const ArchiveEngineState& state,
    AccessLevel access
);

void evaluate_candidate_artifact_plans_into_state(ArchiveEngineState& state, AccessLevel access);

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_plan_evaluations(const ArchiveEngineState& state);

[[nodiscard]] std::string format_candidate_artifact_plan_evaluation_summary(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_plan_evaluation_validation(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_plan_evaluation_list(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_plan_evaluation_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& evaluation_id
);

} // namespace archive
