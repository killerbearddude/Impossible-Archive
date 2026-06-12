#pragma once
#include "archive_engine_state.h"
#include "validation_api.h"

namespace archive {

[[nodiscard]] std::string lower_ascii(std::string text);
[[nodiscard]] bool contains_ci(const std::string& haystack, std::string_view needle);
void add_unique_trope(std::vector<TropeFlag>& flags, TropeFlag flag);
void add_originality_rationale(OriginalitySignal& signal,
                               TropeFlag flag,
                               std::string matched_pattern,
                               std::string explanation);
[[nodiscard]] std::string trope_flags_text(const std::vector<TropeFlag>& flags);
[[nodiscard]] std::string dependency_list_text(const std::vector<std::string>& dependencies);
[[nodiscard]] std::vector<std::string> detect_local_dependencies(const std::string& text);
[[nodiscard]] bool has_substantial_local_dependency(const std::vector<std::string>& dependencies);
[[nodiscard]] bool feature_kind_is(OriginalityFeatureKind actual, std::initializer_list<OriginalityFeatureKind> allowed);
[[nodiscard]] OriginalitySignal score_originality_feature(const OriginalityFeature& feature);
[[nodiscard]] OriginalitySignal score_originality_feature(const std::string& feature_id, const std::string& description);
[[nodiscard]] std::vector<OriginalitySignal> build_originality_signals(const ArchiveEngineState& state);
[[nodiscard]] std::string format_originality(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] OriginalityFeatureKind originality_kind_for_candidate(CandidateFeatureType type);
[[nodiscard]] OriginalityFeature originality_feature_for_candidate(const CandidateFeature& candidate);
[[nodiscard]] CandidateFeature sample_candidate_feature(std::string_view candidate_id);
[[nodiscard]] bool candidate_is_structured(const CandidateFeature& candidate);
[[nodiscard]] bool candidate_declares_mediation(const CandidateFeature& candidate, std::initializer_list<std::string_view> terms);
void validate_candidate_links(const ArchiveEngineState& state,
                              const CandidateFeature& candidate,
                              std::vector<std::string>& errors);
[[nodiscard]] AnachronismReport candidate_anachronism_report(const std::string& candidate_id,
                                                             const std::string& referenced_item,
                                                             int checked_year,
                                                             int start_year,
                                                             int end_year,
                                                             AnachronismStatus status,
                                                             std::string explanation);
[[nodiscard]] Contradiction candidate_predicted_contradiction(const CandidateFeature& candidate,
                                                              std::string entity_id,
                                                              ContradictionCause cause);
[[nodiscard]] CandidateEvaluation evaluate_candidate_feature(const ArchiveEngineState& state,
                                                             const CandidateFeature& candidate,
                                                             AccessLevel access);
[[nodiscard]] bool candidate_can_view_anachronism_detail(AccessLevel access);
[[nodiscard]] bool candidate_can_view_exact_valid_range(AccessLevel access);
[[nodiscard]] bool candidate_can_view_internal_status(AccessLevel access);
[[nodiscard]] bool candidate_can_view_predicted_contradiction_detail(AccessLevel access);
[[nodiscard]] std::string display_candidate_decision(CandidateDecision decision, AccessLevel access);
[[nodiscard]] std::string display_candidate_explanation(const CandidateEvaluation& evaluation, AccessLevel access);
[[nodiscard]] bool candidate_report_has_invalid_anachronism(const CandidateEvaluation& evaluation);
[[nodiscard]] bool candidate_report_has_mediated_anachronism(const CandidateEvaluation& evaluation);
void format_candidate_anachronisms_for_access(std::ostringstream& out,
                                              const CandidateEvaluation& evaluation,
                                              AccessLevel access);
void format_candidate_predicted_contradictions_for_access(std::ostringstream& out,
                                                          const CandidateEvaluation& evaluation,
                                                          AccessLevel access);
[[nodiscard]] std::string format_candidate_evaluation(const CandidateEvaluation& evaluation,
                                                      const CandidateFeature& candidate,
                                                      AccessLevel access);
[[nodiscard]] std::string format_candidate_query(const ArchiveEngineState& state,
                                                 AccessLevel access,
                                                 std::string_view candidate_id);
[[nodiscard]] std::optional<CandidateFeature> generated_candidate_at(const ArchiveEngineState& state,
                                                                     const CandidateGenerationRequest& request,
                                                                     std::size_t candidate_index);
[[nodiscard]] GeneratedCandidateRole generated_candidate_role(const CandidateFeature& candidate);
[[nodiscard]] std::string dossier_candidate_role(const CandidateFeature& candidate);
[[nodiscard]] std::optional<std::size_t> generated_candidate_index_for_role(const ArchiveEngineState& state,
                                                                            const CandidateGenerationRequest& request,
                                                                            GeneratedCandidateRole role);
[[nodiscard]] std::optional<GenerationTarget> resolve_spec_generation_target(const ArchiveEngineState& state,
                                                                             std::string_view target_topic);
[[nodiscard]] std::optional<GenerationTarget> resolve_generation_target(const ArchiveEngineState& state,
                                                                        std::string_view target_topic);
[[nodiscard]] std::vector<std::string> list_generation_targets_for_state(const ArchiveEngineState& state);
[[nodiscard]] std::string format_generation_targets_for_state(const ArchiveEngineState& state,
                                                              AccessLevel access);
[[nodiscard]] GeneratedCandidateBatch generate_candidate_batch(const ArchiveEngineState& state,
                                                              const CandidateGenerationRequest& request);
[[nodiscard]] std::vector<CandidateFeature> generate_candidate_features(const ArchiveEngineState& state,
                                                                        const CandidateGenerationRequest& request);
[[nodiscard]] std::string format_generated_candidates(const ArchiveEngineState& state,
                                                      AccessLevel access,
                                                      const CandidateGenerationRequest& request);
[[nodiscard]] DossierEvaluation evaluate_generated_dossier(const ArchiveEngineState& state,
                                                          const CandidateGenerationRequest& request,
                                                          AccessLevel access);
[[nodiscard]] std::string format_dossier_evaluation(const ArchiveEngineState& state,
                                                    AccessLevel access,
                                                    const CandidateGenerationRequest& request);
[[nodiscard]] HiddenMutationCandidateSourceSummary summarize_hidden_mutation_candidate_source(
    const CandidateFeature& candidate,
    AccessLevel access
);
[[nodiscard]] GeneratedCandidateBatch generate_candidates_from_hidden_mutation(
    const ArchiveEngineState& state,
    const HiddenTruthMutationRecord& record,
    const CandidateGenerationRequest& request
);
[[nodiscard]] std::string format_candidates_from_hidden_mutation(
    const ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTruthMutationRecord& record,
    const CandidateGenerationRequest& request
);
[[nodiscard]] std::string format_hidden_mutation_artifact_generation_query(
    ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTimelineClusterRequest& cluster_request,
    const CandidateGenerationRequest& candidate_request
);

} // namespace archive
