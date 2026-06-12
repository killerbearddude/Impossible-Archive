#pragma once
#include "archive_engine_state.h"
#include "candidate_artifact_proposal_model.h"

namespace archive {

[[nodiscard]] CandidateArtifactProposal draft_candidate_artifact_proposal(
    const ArchiveEngineState& state,
    const CandidateArtifactPlanEvaluation& evaluation,
    AccessLevel access
);

[[nodiscard]] CandidateArtifactProposalDecision classify_candidate_artifact_proposal(
    const CandidateArtifactPlanEvaluation& evaluation
);

[[nodiscard]] CandidateArtifactProposalSafety proposal_safety_for_access(
    const CandidateArtifactProposal& proposal,
    AccessLevel access
);

[[nodiscard]] bool candidate_artifact_proposal_visible_to(
    const CandidateArtifactProposal& proposal,
    AccessLevel access
);

[[nodiscard]] CandidateArtifactProposalReport draft_candidate_artifact_proposals(
    const ArchiveEngineState& state,
    AccessLevel access
);

void draft_candidate_artifact_proposals_into_state(ArchiveEngineState& state, AccessLevel access);

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_proposals(const ArchiveEngineState& state);

[[nodiscard]] std::string format_candidate_artifact_proposal_summary(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_proposal_validation(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_proposal_list(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_proposal_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& proposal_id
);

} // namespace archive
