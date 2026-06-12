#pragma once
#include "archive_engine_state.h"
#include "candidate_model.h"

namespace archive {

[[nodiscard]] std::vector<HiddenProposal> generate_hidden_proposals(const ArchiveEngineState& state,
                                                                    const CandidateGenerationRequest& request);
[[nodiscard]] HiddenProposalEvaluation evaluate_hidden_proposal(const ArchiveEngineState& state,
                                                               const HiddenProposal& proposal);
[[nodiscard]] std::string hidden_proposal_proposed_year_text(const HiddenProposal& proposal);
[[nodiscard]] HiddenProposalMigrationPlan plan_hidden_proposal_migration(const ArchiveEngineState& state,
                                                                        const HiddenProposal& proposal);
[[nodiscard]] std::string format_hidden_proposal_migration_plan(const ArchiveEngineState& state,
                                                               AccessLevel access,
                                                               const CandidateGenerationRequest& request,
                                                               std::size_t proposal_index);
[[nodiscard]] std::string format_hidden_proposals(const ArchiveEngineState& state,
                                                  AccessLevel access,
                                                  const CandidateGenerationRequest& request);
[[nodiscard]] std::string format_hidden_proposal_evaluation(const ArchiveEngineState& state,
                                                           AccessLevel access,
                                                           const CandidateGenerationRequest& request,
                                                           std::size_t proposal_index);
[[nodiscard]] GeneratedHiddenTimelineCluster generate_hidden_timeline_cluster(
    const ArchiveEngineState& state,
    const HiddenTimelineClusterRequest& request
);
[[nodiscard]] HiddenTimelineClusterEvaluation evaluate_hidden_timeline_cluster(
    const ArchiveEngineState& state,
    const GeneratedHiddenTimelineCluster& cluster
);
[[nodiscard]] std::string format_hidden_timeline_cluster(
    const ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTimelineClusterRequest& request
);

} // namespace archive
