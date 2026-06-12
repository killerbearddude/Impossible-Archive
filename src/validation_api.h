#pragma once
#include "archive_engine_state.h"

namespace archive {

[[nodiscard]] std::vector<std::string> validate_cross_references(const ArchiveEngineState& state);

[[nodiscard]] std::vector<std::string> validate_mysteries(const ArchiveEngineState& state);

[[nodiscard]] std::vector<std::string> validate_discovery_log(const ArchiveEngineState& state);

[[nodiscard]] std::vector<std::string> validate_hidden_truth_mutations(const ArchiveEngineState& state);

[[nodiscard]] std::vector<std::string> validate_hidden_mutation_artifact_source(const ArchiveEngineState& state,
                                                                                const HiddenTruthMutationRecord& record);

[[nodiscard]] std::vector<std::string> validate_hidden_mutation_artifact_source(const ArchiveEngineState& state,
                                                                                const HiddenMutationArtifactSource& source);

[[nodiscard]] std::vector<std::string> validate_materialized_hidden_mutation_artifact_provenance(const ArchiveEngineState& state,
                                                                                                  const Artifact& artifact);

[[nodiscard]] HiddenMutationCandidateSourceSummary summarize_hidden_mutation_candidate_source(
    const CandidateFeature& candidate,
    AccessLevel access
);

[[nodiscard]] std::vector<std::string> validate_full_state(const ArchiveEngineState& state);


[[nodiscard]] std::string format_validation(const ArchiveEngineState& state, AccessLevel access);

} // namespace archive
