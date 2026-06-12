#pragma once
#include "public_archive_model.h"

namespace archive {

enum class TropeFlag {
    GenericMoonCult,
    DivineKingAnalogue,
    LostEmpireHubrisCollapse,
    RomanStyleRestorationInscription,
    GenericChosenProphecy,
    GenericAncientBureaucracy,
    GenericSacredKingship,
};

enum class OriginalityFeatureKind {
    Unknown,
    Office,
    Claim,
    Artifact,
    Script,
    Institution,
    Ritual,
    LegalFormula,
};

struct OriginalityFeature {
    std::string id;
    OriginalityFeatureKind kind = OriginalityFeatureKind::Unknown;
    std::string description;
};

struct OriginalityRationale {
    TropeFlag flag = TropeFlag::GenericMoonCult;
    std::string matched_pattern;
    std::string explanation;
};

// Originality audit signal. This is a curator/canon/debug read-only critique layer:
// it must never rewrite artifacts or hidden truth automatically.
struct OriginalitySignal {
    std::string feature_id;
    OriginalityFeatureKind feature_kind = OriginalityFeatureKind::Unknown;
    std::vector<TropeFlag> trope_flags;
    std::vector<OriginalityRationale> rationales;
    double trope_similarity_score = 0.0;
    double transformed_trope_score = 0.0;
    double direct_copy_risk_score = 0.0;
    double civilization_specificity_score = 0.0;
    std::vector<std::string> required_local_dependencies;
    std::string assessment;
};

enum class CandidateFeatureType {
    Artifact,
    Claim,
    Event,
    Entity,
    Mystery,
};

// Structured metadata for candidate artifacts. When present, candidate
// evaluation trusts this typed data over prose heuristics and requires typed
// declared_mediations for anachronism mediation.
struct CandidateArtifactMetadata {
    std::optional<int> true_creation_year;
    std::optional<int> claimed_creation_year;
    std::optional<int> discovery_year;
    std::optional<std::string> location_created;
    std::optional<std::string> location_found;
    std::optional<std::string> language_id;
    std::optional<std::string> dialect_id;
    std::optional<std::string> script_id;
    std::vector<std::string> referenced_entity_ids;
    std::vector<EvidenceModifier> declared_mediations;
};

// Structured claim payload for v15 ingestion. When present, candidate
// evaluation validates these typed fields directly; prose descriptions are
// not allowed to provide hidden semantic defaults or bypass invalid metadata.
struct CandidateClaimMetadata {
    ClaimType claim_type = ClaimType::FactualClaim;
    PredicateType predicate_type = PredicateType::ExistedInYear;
    std::string subject;
    std::string predicate;
    std::string object;
    std::string literal_content;
    std::optional<std::string> subject_entity_id;
    std::optional<std::string> object_entity_id;
    std::optional<int> claimed_year;
    double confidence = 0.5;
    AccessLevel min_access = AccessLevel::Public;
};

// Provenance for v25 candidates generated from a successful hidden-truth
// mutation record. This is candidate-side source metadata only: it does not
// materialize public artifacts and must be redacted below curator access.
struct HiddenMutationArtifactSource {
    std::string mutation_record_id;
    std::string source_cluster_id;
    std::vector<std::string> source_entity_ids;
    std::vector<std::string> source_event_ids;
};

// Compact public-safe summary for mutation-derived candidates. It is derived
// from HiddenMutationArtifactSource and exposes counts/labels without leaking
// hidden entity, event, cluster, or mutation IDs below curator/canon/debug access.
struct HiddenMutationCandidateSourceSummary {
    std::string public_origin_label;
    std::string public_effect_label;
    std::size_t source_entity_count = 0;
    std::size_t source_event_count = 0;
    bool has_curator_trace = false;
};

// Proposed archive addition. Candidate evaluation is non-mutating;
// materialization is a separate curator/canon/debug-gated path with rollback.
struct CandidateFeature {
    std::string id;
    CandidateFeatureType type = CandidateFeatureType::Artifact;
    std::string description;
    std::vector<std::string> proposed_links;
    std::optional<CandidateArtifactMetadata> structured_artifact_metadata;
    std::vector<CandidateClaimMetadata> structured_claims;
    std::optional<HiddenMutationArtifactSource> hidden_mutation_source;
};

enum class CandidateGenerationStrategy {
    AddCorroboratingFragment,
    AddMisleadingForgery,
    AddRitualVariant,
    BuildTargetDossier,
};

// Semantic role for generated dossier candidates. Role selection is less
// brittle than raw index selection when deterministic batch ordering changes
// by seed.
enum class GeneratedCandidateRole {
    CorroboratingFragment,
    RitualVariant,
    MisleadingForgery,
    Unknown,
};

// Candidate generation request. Generation is deterministic and non-mutating.
// It proposes candidate features; it does not insert them.
struct CandidateGenerationRequest {
    CandidateGenerationStrategy strategy = CandidateGenerationStrategy::AddCorroboratingFragment;
    int target_year = 620;
    std::string target_topic = "lock_authority";
    std::uint64_t seed = 0;
};

struct GeneratedCandidateBatch {
    CandidateGenerationRequest request;
    std::optional<GenerationTarget> resolved_target;
    std::vector<CandidateFeature> candidates;
};

enum class CandidateDecision {
    Accept,
    Reject,
    AcceptAsForgery,
    AcceptAsLaterCopy,
    AcceptAsMythicCompression,
    NeedsCuratorReview,
};

// Result of candidate evaluation. This may contain privileged internals; use the
// access-aware formatter for public/scholar presentation.
struct CandidateEvaluation {
    CandidateDecision decision = CandidateDecision::Reject;
    std::string evaluated_candidate_id;
    std::vector<std::string> validation_errors;
    std::vector<AnachronismReport> anachronisms;
    std::vector<Contradiction> predicted_contradictions;
    OriginalitySignal originality;
    std::string explanation;
};

// Batch-level audit for generated candidate dossiers. Unlike individual
// candidate evaluation, this asks whether the generated set is too clean, too
// forged, too ambiguous, or too likely to over-resolve a protected mystery.
struct DossierEvaluation {
    CandidateGenerationRequest request;
    std::optional<GenerationTarget> resolved_target;
    std::vector<CandidateEvaluation> candidate_evaluations;
    double corroboration_pressure = 0.0;
    double ambiguity_pressure = 0.0;
    double forgery_pressure = 0.0;
    double mystery_resolution_pressure = 0.0;
    double originality_balance = 0.0;
    std::string assessment;
};

// Curator workflow plan for materializing one candidate out of a generated
// dossier. The plan is advisory and non-mutating; actual mutation still goes
// through materialize_candidate_feature and its validation/rollback boundary.
struct DossierMaterializationPlan {
    DossierEvaluation dossier_evaluation;
    std::vector<std::size_t> selected_candidate_indices;
    std::string selected_candidate_id;
    std::string selected_candidate_role;
    double projected_corroboration_pressure = 0.0;
    double projected_ambiguity_pressure = 0.0;
    double projected_forgery_pressure = 0.0;
    std::string recommendation;
};

// v20 planning object for selecting multiple generated dossier candidates. The
// plan is advisory and non-mutating; materialization is a separate explicit
// curator/canon/debug action and rolls back the whole selected batch on failure.
struct DossierSelectionPlan {
    DossierEvaluation dossier_evaluation;
    std::vector<std::size_t> requested_candidate_indices;
    std::vector<std::size_t> selected_candidate_indices;
    std::vector<std::string> selected_candidate_ids;
    std::vector<std::string> selected_candidate_roles;
    bool duplicate_selections_ignored = false;
    double projected_corroboration_pressure = 0.0;
    double projected_ambiguity_pressure = 0.0;
    double projected_forgery_pressure = 0.0;
    std::string recommendation;
};

// v21 hidden-truth proposal gate. Proposals are advisory objects: they may be
// generated and validated against hidden chronology, but this MVP intentionally


[[nodiscard]] std::string to_string(TropeFlag flag);
[[nodiscard]] std::string to_string(OriginalityFeatureKind kind);
[[nodiscard]] std::string to_string(CandidateFeatureType type);
[[nodiscard]] std::string to_string(CandidateGenerationStrategy strategy);
[[nodiscard]] CandidateGenerationStrategy parse_candidate_generation_strategy(std::string_view text);
[[nodiscard]] std::string to_string(GeneratedCandidateRole role);
[[nodiscard]] GeneratedCandidateRole parse_generated_candidate_role(std::string_view text);
[[nodiscard]] std::string to_string(CandidateDecision decision);

} // namespace archive
