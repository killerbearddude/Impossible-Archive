#pragma once
#include "archive_engine_state.h"

namespace archive {

[[nodiscard]] std::string archive_year_text(int archive_year);

[[nodiscard]] bool discovered_by_year(const Artifact& artifact, int archive_year);

[[nodiscard]] bool artifact_visible_to(const Artifact& artifact, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] bool claim_visible_to(const ArchiveEngineState& state, const Claim& claim, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::string display_artifact_type(const Artifact& artifact, AccessLevel access);

[[nodiscard]] std::string display_reliability(const Artifact& artifact, AccessLevel access);

[[nodiscard]] ArtifactVoiceProfile voice_profile_for(ArtifactVoiceRegister voice);

[[nodiscard]] std::vector<const Claim*> visible_claims_for_artifact_id(const ArchiveEngineState& state,
                                                                       const std::string& artifact_id,
                                                                       AccessLevel access,
                                                                       int archive_year);

[[nodiscard]] bool voice_role_publicly_renderable(ArtifactVoiceClaimRole role, AccessLevel access);

[[nodiscard]] std::vector<VoiceClaimView> visible_voice_claims_for_artifact(
    const Artifact& artifact,
    const ArchiveEngineState& state,
    AccessLevel access,
    int archive_year,
    std::optional<ArtifactVoiceClaimRole> role_filter = std::nullopt
);

[[nodiscard]] std::string trim_copy(std::string text);

[[nodiscard]] bool is_sentence_punctuation(char ch);

[[nodiscard]] std::string trim_trailing_sentence_punctuation(std::string text);

[[nodiscard]] std::string ensure_sentence_punctuation(std::string text);

[[nodiscard]] std::string compact_claim_phrase(const Claim& claim);

[[nodiscard]] std::string joined_voice_claim_phrases(const std::vector<VoiceClaimView>& claims, std::string_view separator);

[[nodiscard]] std::string joined_claim_phrases(const std::vector<const Claim*>& claims, std::string_view separator);

[[nodiscard]] std::string no_duplicate_punctuation(std::string text);

[[nodiscard]] std::string render_artifact_text(const Artifact& artifact,
                                               const ArchiveEngineState& state,
                                               AccessLevel access,
                                               int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_artifacts(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_claims(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] bool contradiction_visible_to(const ArchiveEngineState& state,
                                                                const Contradiction& contradiction,
                                                                AccessLevel access,
                                                                int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_contradictions(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_anachronisms(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::vector<const Artifact*> visible_artifacts(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::vector<const Claim*> visible_claims(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::vector<const Contradiction*> visible_contradictions(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] double citation_weight(const ArchiveEngineState& state, const Claim& claim);

[[nodiscard]] std::optional<std::string> format_citation_for_access(const EvidenceCitation& citation,
                                                                    const ArchiveEngineState& state,
                                                                    AccessLevel access,
                                                                    int archive_year);

[[nodiscard]] bool mystery_has_visible_evidence_link(const ArchiveEngineState& state,
                                                            const Mystery& mystery,
                                                            AccessLevel access,
                                                            int archive_year);

[[nodiscard]] std::vector<const Mystery*> visible_mysteries(const ArchiveEngineState& state,
                                                            AccessLevel access,
                                                            int archive_year = kOpenEndedYear);

[[nodiscard]] double confidence_cap_for_mystery(const Mystery& mystery, AccessLevel access);

[[nodiscard]] std::string mystery_status_for_confidence(double confidence, const Mystery& mystery);

[[nodiscard]] bool contradiction_relevant_to_citations(const Contradiction& contradiction,
                                                       const std::vector<EvidenceCitation>& citations);

[[nodiscard]] std::vector<const Claim*> visible_claims_for_artifact(const ArchiveEngineState& state,
                                                                    const Artifact& artifact,
                                                                    AccessLevel access,
                                                                    int archive_year);

void add_unique_citation(std::vector<EvidenceCitation>& citations, EvidenceCitation citation);

[[nodiscard]] bool link_visible_to(const ArchiveEngineState& state,
                                   const MysteryEvidenceLink& link,
                                   AccessLevel access,
                                   int archive_year);

[[nodiscard]] MysteryAssessment assess_mystery(const ArchiveEngineState& state,
                                               const Mystery& mystery,
                                               AccessLevel access,
                                               int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_mystery_assessment(const MysteryAssessment& assessment,
                                                    const ArchiveEngineState& state,
                                                    AccessLevel access,
                                                    int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_mysteries(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] const Claim* find_visible_claim_by_id(const std::vector<const Claim*>& claims, std::string_view id);

[[nodiscard]] std::vector<const Claim*> claims_by_predicate(const std::vector<const Claim*>& claims, PredicateType predicate);

[[nodiscard]] std::vector<const Claim*> high_confidence_claims(const std::vector<const Claim*>& claims, double threshold);

[[nodiscard]] double citation_weight(const ArchiveEngineState& state, const Claim& claim);

[[nodiscard]] std::string evidence_strength_text(double weight);

[[nodiscard]] std::string interpretive_confidence_label(double confidence);

[[nodiscard]] std::optional<std::string> format_citation_for_access(const EvidenceCitation& citation,
                                                                          const ArchiveEngineState& state,
                                                                          AccessLevel access,
                                                                          int archive_year = kOpenEndedYear);

[[nodiscard]] std::vector<const Event*> hidden_events_sorted_by_year(const HiddenTruthGraph& graph);

[[nodiscard]] bool contains_rule_contradiction(const std::vector<const Contradiction*>& contradictions, std::string_view rule_name);

void add_claim_citation_if_present(AnswerBlock& block, const ArchiveEngineState& state, const Claim* claim);

[[nodiscard]] bool contradiction_relevant_to_citations(const Contradiction& contradiction,
                                                       const std::vector<EvidenceCitation>& citations);

void add_relevant_contradictions_to_block(AnswerBlock& block, const std::vector<const Contradiction*>& contradictions);

[[nodiscard]] AnswerBlock build_public_answer_block(const ArchiveEngineState& state,
                                                    const std::vector<const Claim*>& claims,
                                                    const std::vector<const Contradiction*>& contradictions);

[[nodiscard]] std::optional<AnswerBlock> build_scholar_answer_block(const ArchiveEngineState& state,
                                                                     const std::vector<const Claim*>& claims,
                                                                     const std::vector<const Contradiction*>& contradictions);

[[nodiscard]] std::optional<AnswerBlock> build_curator_forgery_block(const ArchiveEngineState& state,
                                                                      const std::vector<const Claim*>& claims,
                                                                      const std::vector<const Contradiction*>& contradictions);

[[nodiscard]] std::optional<AnswerBlock> build_canon_hidden_sequence_block(const ArchiveEngineState& state);

[[nodiscard]] std::optional<AnswerBlock> build_debug_trace_block(const ArchiveEngineState& state,
                                                                  const std::vector<const Contradiction*>& contradictions);

[[nodiscard]] Answer build_answer_what_happened(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_answer(const Answer& answer, const ArchiveEngineState& state);

[[nodiscard]] std::string answer_what_happened(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::vector<Interpreter> default_interpreters();

[[nodiscard]] bool claim_is_in_contradiction(const Claim& claim, const std::vector<const Contradiction*>& contradictions);

[[nodiscard]] bool claim_has_visible_forgery_caveat(const Claim& claim,
                                                            const std::vector<const Contradiction*>& contradictions,
                                                            AccessLevel access);

[[nodiscard]] bool artifact_type_visible_as(const Artifact& artifact, ArtifactType expected, AccessLevel access);

[[nodiscard]] bool artifact_is_visible_disputed_decree(const Artifact& artifact, AccessLevel access);

[[nodiscard]] double epistemic_style_multiplier(EpistemicStyle style,
                                                const Claim& claim,
                                                const Artifact& artifact,
                                                const std::vector<const Contradiction*>& contradictions,
                                                AccessLevel access);

[[nodiscard]] std::vector<ScoredClaim> score_claims_for_interpreter(const ArchiveEngineState& state,
                                                                    const Interpreter& interpreter,
                                                                    const std::vector<const Claim*>& claims,
                                                                    const std::vector<const Contradiction*>& contradictions);

[[nodiscard]] std::string top_evidence_phrase(const ArchiveEngineState& state,
                                                const std::vector<ScoredClaim>& selected);

[[nodiscard]] bool selected_contains_claim(const std::vector<ScoredClaim>& selected, std::string_view claim_id);

[[nodiscard]] bool selected_has_visible_forgery_caveat(const std::vector<ScoredClaim>& selected,
                                                       const std::vector<const Contradiction*>& contradictions,
                                                       AccessLevel access);

[[nodiscard]] std::string theory_summary_for_style(EpistemicStyle style,
                                                   const std::vector<ScoredClaim>& selected,
                                                   const std::vector<const Contradiction*>& contradictions,
                                                   const ArchiveEngineState& state,
                                                   AccessLevel access);

[[nodiscard]] Theory build_theory_for_interpreter(const ArchiveEngineState& state, const Interpreter& interpreter, int archive_year = kOpenEndedYear);

[[nodiscard]] std::vector<Theory> build_theories(const ArchiveEngineState& state, int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_theory(const Theory& theory, const ArchiveEngineState& state, AccessLevel viewer_access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_theories(const ArchiveEngineState& state, AccessLevel viewer_access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_discoveries(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

[[nodiscard]] std::string format_hidden_timeline(const ArchiveEngineState& state, AccessLevel access);


[[nodiscard]] std::string format_hidden_truth_mutations(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string serialize_for_replay_test(const ArchiveEngineState& state);
[[nodiscard]] std::string serialize_world_content_for_seed_test(const ArchiveEngineState& state);
[[nodiscard]] std::string format_report(const ArchiveEngineState& state, AccessLevel access, int archive_year = kOpenEndedYear);

} // namespace archive
