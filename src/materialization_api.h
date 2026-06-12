#pragma once
#include "archive_engine_state.h"
#include "candidate_generation_api.h"
#include "hidden_workflows_api.h"

namespace archive {

enum class MaterializationDecision {
    InsertArtifact,
    InsertClaim,
    Reject,
};

// Result of explicit materialization. Successful mutation requires curator/canon/debug
// access, insertable evaluation, structured metadata, post-insertion validation,
// and rollback on failure.
struct MaterializationResult {
    MaterializationDecision decision = MaterializationDecision::Reject;
    bool mutated = false;
    std::vector<std::string> inserted_artifact_ids;
    std::vector<std::string> inserted_claim_ids;
    std::vector<std::string> inserted_contradiction_ids;
    std::string explanation;
};

// Complete in-memory MVP state. Query builders should use access/archive-year


[[nodiscard]] std::string to_string(MaterializationDecision decision);
void register_discovery_for_artifact(ArchiveEngineState& state, const std::string& artifact_id);
[[nodiscard]] bool materialization_decision_is_insertable(CandidateDecision decision);
[[nodiscard]] std::optional<Artifact> materialized_candidate_artifact(const CandidateFeature& candidate,
                                                                      const CandidateEvaluation& evaluation);
void add_materialized_candidate_claims(PublicArchive& archive, Artifact& artifact, const CandidateFeature& candidate);
[[nodiscard]] std::vector<std::string> contradiction_ids(const ArchiveEngineState& state);
[[nodiscard]] std::vector<std::string> newly_inserted_contradiction_ids(const std::vector<std::string>& before_ids,
                                                                        const ArchiveEngineState& state);
MaterializationResult materialize_candidate_feature(ArchiveEngineState& state,
                                                    const CandidateFeature& candidate,
                                                    const CandidateEvaluation& evaluation,
                                                    AccessLevel access);
[[nodiscard]] std::string format_materialization_result(const MaterializationResult& result,
                                                        const CandidateFeature& candidate,
                                                        AccessLevel access);
[[nodiscard]] std::string format_materialization_query(ArchiveEngineState& state,
                                                       AccessLevel access,
                                                       std::string_view candidate_id);
[[nodiscard]] std::string format_generated_materialization_query(ArchiveEngineState& state,
                                                                AccessLevel access,
                                                                const CandidateGenerationRequest& request,
                                                                std::size_t candidate_index);
[[nodiscard]] DossierMaterializationPlan build_dossier_materialization_plan(const ArchiveEngineState& state,
                                                                            const CandidateGenerationRequest& request,
                                                                            std::size_t candidate_index,
                                                                            AccessLevel access);
[[nodiscard]] std::string format_dossier_materialization_query(ArchiveEngineState& state,
                                                               AccessLevel access,
                                                               const CandidateGenerationRequest& request,
                                                               std::size_t candidate_index);
[[nodiscard]] std::string format_dossier_materialization_query_by_role(ArchiveEngineState& state,
                                                                       AccessLevel access,
                                                                       const CandidateGenerationRequest& request,
                                                                       GeneratedCandidateRole role);
[[nodiscard]] DossierSelectionPlan build_dossier_selection_plan(const ArchiveEngineState& state,
                                                                const CandidateGenerationRequest& request,
                                                                const std::vector<std::size_t>& candidate_indices,
                                                                AccessLevel access);
[[nodiscard]] std::string format_dossier_selection_plan(const ArchiveEngineState& state,
                                                        AccessLevel access,
                                                        const CandidateGenerationRequest& request,
                                                        const std::vector<std::size_t>& candidate_indices);
[[nodiscard]] std::string format_dossier_selection_materialization_query(ArchiveEngineState& state,
                                                                         AccessLevel access,
                                                                         const CandidateGenerationRequest& request,
                                                                         const std::vector<std::size_t>& candidate_indices);
[[nodiscard]] HiddenClusterMaterializationResult materialize_hidden_timeline_cluster(
    ArchiveEngineState& state,
    const GeneratedHiddenTimelineCluster& cluster,
    AccessLevel access
);
[[nodiscard]] std::string format_hidden_cluster_materialization_query(
    ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTimelineClusterRequest& request
);
MaterializationResult materialize_hidden_mutation_artifact_candidate(
    ArchiveEngineState& state,
    const CandidateFeature& candidate,
    AccessLevel access
);
[[nodiscard]] std::string format_hidden_mutation_artifact_materialization_result(
    const MaterializationResult& result,
    const CandidateFeature& candidate,
    AccessLevel access
);
[[nodiscard]] std::string format_hidden_mutation_artifact_candidate_materialization_query(
    ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTimelineClusterRequest& cluster_request,
    const CandidateGenerationRequest& candidate_request,
    std::optional<HiddenMutationArtifactCandidateShape> shape,
    std::optional<std::size_t> index
);

} // namespace archive
