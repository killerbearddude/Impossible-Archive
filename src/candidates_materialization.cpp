/*
 * Candidate evaluation and explicit materialization. Evaluation is read-only; materialization is curator/canon/debug-gated and rollback-protected.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "materialization_api.h"
#include "archive_views_api.h"
#include "candidate_generation_api.h"
#include "validation_api.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <optional>
#include <sstream>
#include <utility>

namespace archive {
namespace {

constexpr double kMinimumMaterializableCivilizationSpecificity = 0.30;
constexpr double kMaximumMaterializableDirectCopyRisk = 0.35;

[[nodiscard]] std::optional<std::string> candidate_materialization_quality_error(const CandidateEvaluation& evaluation) {
    if (evaluation.originality.direct_copy_risk_score >= kMaximumMaterializableDirectCopyRisk) {
        std::ostringstream out;
        out << "candidate is not materializable: direct-copy risk above threshold ("
            << std::fixed << std::setprecision(2)
            << evaluation.originality.direct_copy_risk_score
            << " >= " << kMaximumMaterializableDirectCopyRisk << ")";
        return out.str();
    }
    if (evaluation.originality.civilization_specificity_score < kMinimumMaterializableCivilizationSpecificity) {
        std::ostringstream out;
        out << "candidate is not materializable: civilization specificity below threshold ("
            << std::fixed << std::setprecision(2)
            << evaluation.originality.civilization_specificity_score
            << " < " << kMinimumMaterializableCivilizationSpecificity << ")";
        return out.str();
    }
    if (!evaluation.originality.trope_flags.empty()) {
        return std::string{"candidate is not materializable: originality trope flags present"};
    }
    return std::nullopt;
}

} // namespace

[[nodiscard]] OriginalityFeatureKind originality_kind_for_candidate(CandidateFeatureType type) {
    switch (type) {
        case CandidateFeatureType::Artifact: return OriginalityFeatureKind::Artifact;
        case CandidateFeatureType::Claim: return OriginalityFeatureKind::Claim;
        case CandidateFeatureType::Event: return OriginalityFeatureKind::Institution;
        case CandidateFeatureType::Entity: return OriginalityFeatureKind::Institution;
        case CandidateFeatureType::Mystery: return OriginalityFeatureKind::Ritual;
    }
    return OriginalityFeatureKind::Unknown;
}

[[nodiscard]] std::string candidate_originality_humanized_token(std::string token) {
    for (char& ch : token) {
        if (ch == '.' || ch == '_' || ch == ':' || ch == '-') {
            ch = ' ';
        }
    }
    return token;
}

void append_candidate_originality_text(std::ostringstream& out, const std::string& value) {
    if (value.empty()) {
        return;
    }
    out << ' ' << value << ' ' << candidate_originality_humanized_token(value);
}

// v15.1: originality for structured candidates must inspect typed links and
// structured metadata, not only prose. This keeps the originality gate aligned
// with the structured validation/materialization pipeline.
[[nodiscard]] OriginalityFeature originality_feature_for_candidate(const CandidateFeature& candidate) {
    std::ostringstream description;
    description << candidate.description;

    for (const std::string& link : candidate.proposed_links) {
        append_candidate_originality_text(description, link);
    }

    if (candidate.structured_artifact_metadata.has_value()) {
        const CandidateArtifactMetadata& metadata = *candidate.structured_artifact_metadata;
        if (metadata.location_created.has_value()) append_candidate_originality_text(description, *metadata.location_created);
        if (metadata.location_found.has_value()) append_candidate_originality_text(description, *metadata.location_found);
        if (metadata.language_id.has_value()) append_candidate_originality_text(description, *metadata.language_id);
        if (metadata.dialect_id.has_value()) append_candidate_originality_text(description, *metadata.dialect_id);
        if (metadata.script_id.has_value()) append_candidate_originality_text(description, *metadata.script_id);
        for (const std::string& entity_id : metadata.referenced_entity_ids) {
            append_candidate_originality_text(description, entity_id);
        }
        for (EvidenceModifier mediation : metadata.declared_mediations) {
            append_candidate_originality_text(description, to_string(mediation));
        }
    }

    for (const CandidateClaimMetadata& claim : candidate.structured_claims) {
        append_candidate_originality_text(description, claim.subject);
        append_candidate_originality_text(description, claim.predicate);
        append_candidate_originality_text(description, claim.object);
        append_candidate_originality_text(description, claim.literal_content);
        if (claim.subject_entity_id.has_value()) append_candidate_originality_text(description, *claim.subject_entity_id);
        if (claim.object_entity_id.has_value()) append_candidate_originality_text(description, *claim.object_entity_id);
        append_candidate_originality_text(description, to_string(claim.claim_type));
        append_candidate_originality_text(description, to_string(claim.predicate_type));
    }

    if (candidate.hidden_mutation_source.has_value()) {
        const HiddenMutationArtifactSource& source = *candidate.hidden_mutation_source;
        append_candidate_originality_text(description, source.source_cluster_id);
        for (const std::string& entity_id : source.source_entity_ids) {
            append_candidate_originality_text(description, entity_id);
        }
        for (const std::string& event_id : source.source_event_ids) {
            append_candidate_originality_text(description, event_id);
        }
    }

    return OriginalityFeature{
        candidate.id,
        originality_kind_for_candidate(candidate.type),
        description.str(),
    };
}

[[nodiscard]] CandidateFeature sample_candidate_feature(std::string_view candidate_id) {
    if (candidate_id == "generic_moon_cult") {
        return CandidateFeature{
            "candidate.generic_moon_cult",
            CandidateFeatureType::Artifact,
            "A generic moon cult artifact where priests worship the moon goddess in a silver temple and proclaim a chosen prophecy.",
            {},
            std::nullopt,
            {},
            std::nullopt,
        };
    }
    if (candidate_id == "moon_lock_fragment") {
        return CandidateFeature{
            "candidate.moon_lock_fragment",
            CandidateFeatureType::Artifact,
            "A Moon-lock legal fragment from the Reservoir Gate: the Drowned Chancellor's mortuary seal regulates lower lock authority after the silt levy.",
            {"entity:office.drowned_chancellor", "entity:site.reservoir_gate", "entity:script.green_seal"},
            std::nullopt,
            {},
            std::nullopt,
        };
    }
    if (candidate_id == "early_green_seal_decree") {
        return CandidateFeature{
            "candidate.early_green_seal_decree",
            CandidateFeatureType::Artifact,
            "Claimed year 553 decree uses Green-Seal script and Drowned Chancellor language with no declared mediation.",
            {"entity:script.green_seal", "entity:office.drowned_chancellor"},
            std::nullopt,
            {},
            std::nullopt,
        };
    }
    if (candidate_id == "early_drowned_chancellor_forgery") {
        return CandidateFeature{
            "candidate.early_drowned_chancellor_forgery",
            CandidateFeatureType::Artifact,
            "Claimed year 553 forged decree names the Drowned Chancellor before the office exists; the candidate explicitly declares forgery mediation.",
            {"entity:office.drowned_chancellor", "entity:person.aru"},
            std::nullopt,
            {},
            std::nullopt,
        };
    }
    if (candidate_id == "early_drowned_chancellor_unmediated") {
        return CandidateFeature{
            "candidate.early_drowned_chancellor_unmediated",
            CandidateFeatureType::Artifact,
            "Claimed year 553 decree names the Drowned Chancellor before the office exists and gives no mediation.",
            {"entity:office.drowned_chancellor", "entity:person.aru"},
            std::nullopt,
            {},
            std::nullopt,
        };
    }
    if (candidate_id == "structured_lock_fragment") {
        CandidateFeature candidate;
        candidate.id = "candidate.structured_lock_fragment";
        candidate.type = CandidateFeatureType::Artifact;
        candidate.description = "Structured lock-authority fragment supplied through typed artifact and claim metadata.";
        candidate.proposed_links = {"entity:office.drowned_chancellor", "entity:site.reservoir_gate", "event:event.salt_moon_schism", "mystery:mystery.third_lock_authority"};
        CandidateArtifactMetadata artifact_metadata;
        artifact_metadata.true_creation_year = 620;
        artifact_metadata.claimed_creation_year = 620;
        artifact_metadata.discovery_year = 814;
        artifact_metadata.location_created = "site.reservoir_gate";
        artifact_metadata.location_found = "site.salt_cellar_archive";
        artifact_metadata.language_id = "language.lattice_dialect";
        artifact_metadata.dialect_id = "dialect.upper_lattice";
        artifact_metadata.script_id = "script.green_seal";
        artifact_metadata.referenced_entity_ids = {"office.drowned_chancellor", "site.reservoir_gate", "script.green_seal"};
        candidate.structured_artifact_metadata = artifact_metadata;
        CandidateClaimMetadata claim_metadata;
        claim_metadata.claim_type = ClaimType::FactualClaim;
        claim_metadata.predicate_type = PredicateType::Received;
        claim_metadata.subject = "Drowned Chancellor";
        claim_metadata.predicate = "received";
        claim_metadata.object = "lower lock authority";
        claim_metadata.literal_content = "Typed fragment states that the Drowned Chancellor received lower lock authority.";
        claim_metadata.subject_entity_id = "office.drowned_chancellor";
        claim_metadata.object_entity_id = "site.reservoir_gate";
        claim_metadata.claimed_year = 620;
        claim_metadata.confidence = 0.59;
        claim_metadata.min_access = AccessLevel::Public;
        candidate.structured_claims = {claim_metadata};
        return candidate;
    }

    if (candidate_id == "missing_metadata") {
        return CandidateFeature{
            "candidate.missing_metadata",
            CandidateFeatureType::Artifact,
            "A proposed artifact with missing creation date, missing location, missing script, and no source event links.",
            {},
            std::nullopt,
            {},
            std::nullopt,
        };
    }

    throw std::invalid_argument("unknown candidate id: " + std::string(candidate_id));
}

[[nodiscard]] bool candidate_metadata_declares(const CandidateFeature& candidate, EvidenceModifier modifier) {
    if (!candidate.structured_artifact_metadata.has_value()) {
        return false;
    }
    const std::vector<EvidenceModifier>& declared = candidate.structured_artifact_metadata->declared_mediations;
    return std::find(declared.begin(), declared.end(), modifier) != declared.end();
}

[[nodiscard]] bool candidate_is_structured(const CandidateFeature& candidate) {
    return candidate.structured_artifact_metadata.has_value() || !candidate.structured_claims.empty();
}

[[nodiscard]] bool candidate_declares_mediation(const CandidateFeature& candidate, std::initializer_list<std::string_view> terms) {
    return std::any_of(terms.begin(), terms.end(), [&](std::string_view term) {
        if (candidate_is_structured(candidate)) {
            if ((term == "forgery" || term == "forged") && candidate_metadata_declares(candidate, EvidenceModifier::Forgery)) {
                return true;
            }
            if ((term == "later copy" || term == "later-copy") && candidate_metadata_declares(candidate, EvidenceModifier::LaterCopy)) {
                return true;
            }
            if ((term == "mythic" || term == "mythic compression") && candidate_metadata_declares(candidate, EvidenceModifier::MythicCompression)) {
                return true;
            }
            return false;
        }
        return contains_ci(candidate.description, term);
    });
}

void validate_candidate_links(const ArchiveEngineState& state,
                              const CandidateFeature& candidate,
                              std::vector<std::string>& errors) {
    for (const std::string& link : candidate.proposed_links) {
        const std::size_t colon = link.find(':');
        if (colon == std::string::npos) {
            errors.push_back("candidate " + candidate.id + " has malformed proposed link " + link);
            continue;
        }
        const std::string kind = link.substr(0, colon);
        const std::string id = link.substr(colon + 1);
        if (kind == "entity") {
            if (state.hidden_truth.find_entity(id) == nullptr) {
                errors.push_back("candidate " + candidate.id + " references missing entity " + id);
            }
        } else if (kind == "event") {
            if (state.hidden_truth.find_event(id) == nullptr) {
                errors.push_back("candidate " + candidate.id + " references missing event " + id);
            }
        } else if (kind == "artifact") {
            if (state.public_archive.find_artifact(id) == nullptr) {
                errors.push_back("candidate " + candidate.id + " references missing artifact " + id);
            }
        } else if (kind == "claim") {
            if (state.public_archive.find_claim(id) == nullptr) {
                errors.push_back("candidate " + candidate.id + " references missing claim " + id);
            }
        } else if (kind == "mystery") {
            const auto it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
                return mystery.id == id;
            });
            if (it == state.mysteries.end()) {
                errors.push_back("candidate " + candidate.id + " references missing mystery " + id);
            }
        } else {
            errors.push_back("candidate " + candidate.id + " has unsupported link kind " + kind);
        }
    }
}

[[nodiscard]] AnachronismReport candidate_anachronism_report(const std::string& candidate_id,
                                                             const std::string& referenced_item,
                                                             int checked_year,
                                                             int start_year,
                                                             int end_year,
                                                             AnachronismStatus status,
                                                             std::string explanation) {
    return AnachronismReport{
        candidate_id,
        referenced_item,
        "claimed_creation_year",
        checked_year,
        start_year,
        end_year,
        status == AnachronismStatus::InvalidGenerationBug ? "high" : "medium",
        std::move(explanation),
        status,
    };
}

[[nodiscard]] Contradiction candidate_predicted_contradiction(const CandidateFeature& candidate,
                                                              std::string entity_id,
                                                              ContradictionCause cause) {
    return Contradiction{
        make_contradiction_id("candidate_unavailable_entity", {candidate.id, std::move(entity_id)}),
        "candidate_unavailable_entity",
        {candidate.id + ".claim"},
        {candidate.id},
        ContradictionType::TitleContradiction,
        cause,
        cause == ContradictionCause::Forgery ? AccessLevel::Curator : AccessLevel::Scholar,
        "Candidate references an entity before its hidden availability window; evaluation is predictive and does not mutate the archive.",
        AccessLevel::Canon,
        "Candidate would create a chronological conflict unless mediated by forgery, later copy, or mythic compression.",
        kOpenEndedYear,
    };
}

[[nodiscard]] bool candidate_metadata_has_any_mediation(const CandidateFeature& candidate,
                                                        std::initializer_list<EvidenceModifier> modifiers) {
    if (!candidate.structured_artifact_metadata.has_value()) {
        return false;
    }
    const std::vector<EvidenceModifier>& declared = candidate.structured_artifact_metadata->declared_mediations;
    return std::any_of(modifiers.begin(), modifiers.end(), [&](EvidenceModifier modifier) {
        return std::find(declared.begin(), declared.end(), modifier) != declared.end();
    });
}

[[nodiscard]] AnachronismStatus status_for_candidate_mediation(const CandidateFeature& candidate) {
    if (candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ||
        candidate_declares_mediation(candidate, {"forgery", "forged"})) {
        return AnachronismStatus::ValidBecauseForged;
    }
    if (candidate_metadata_has_any_mediation(candidate, {EvidenceModifier::LaterCopy, EvidenceModifier::Interpolation, EvidenceModifier::MisdatedStratum}) ||
        candidate_declares_mediation(candidate, {"later copy", "later-copy"})) {
        return AnachronismStatus::ValidBecauseLaterCopy;
    }
    if (candidate_metadata_has_any_mediation(candidate, {EvidenceModifier::RitualAnachronism, EvidenceModifier::MythicCompression}) ||
        candidate_declares_mediation(candidate, {"mythic", "mythic compression"})) {
        return AnachronismStatus::ValidBecauseRitual;
    }
    return AnachronismStatus::InvalidGenerationBug;
}

[[nodiscard]] std::string explanation_for_candidate_status(AnachronismStatus status, const CandidateFeature& candidate) {
    switch (status) {
        case AnachronismStatus::ValidBecauseForged:
            return "Forgery";
        case AnachronismStatus::ValidBecauseLaterCopy:
            if (candidate_metadata_declares(candidate, EvidenceModifier::Interpolation)) {
                return "Interpolation";
            }
            if (candidate_metadata_declares(candidate, EvidenceModifier::MisdatedStratum)) {
                return "MisdatedStratum";
            }
            return "LaterCopy";
        case AnachronismStatus::ValidBecauseRitual:
            if (candidate_metadata_declares(candidate, EvidenceModifier::RitualAnachronism)) {
                return "RitualAnachronism";
            }
            return "MythicCompression";
        case AnachronismStatus::Valid:
        case AnachronismStatus::InvalidGenerationBug:
            return "none";
    }
    return "none";
}

[[nodiscard]] ContradictionCause cause_for_candidate_status(AnachronismStatus status) {
    switch (status) {
        case AnachronismStatus::ValidBecauseForged:
            return ContradictionCause::Forgery;
        case AnachronismStatus::ValidBecauseLaterCopy:
            return ContradictionCause::UnresolvedGenerationBug;
        case AnachronismStatus::ValidBecauseRitual:
            return ContradictionCause::MythologizedMemory;
        case AnachronismStatus::Valid:
        case AnachronismStatus::InvalidGenerationBug:
            return ContradictionCause::UnresolvedGenerationBug;
    }
    return ContradictionCause::UnresolvedGenerationBug;
}

[[nodiscard]] bool candidate_type_is_allowed(EntityType actual, std::initializer_list<EntityType> allowed) {
    return std::find(allowed.begin(), allowed.end(), actual) != allowed.end();
}

[[nodiscard]] const Entity* validated_candidate_entity_field(const ArchiveEngineState& state,
                                                             const CandidateFeature& candidate,
                                                             const std::string& entity_id,
                                                             std::string_view field_name,
                                                             std::initializer_list<EntityType> allowed_types,
                                                             CandidateEvaluation& evaluation) {
    const Entity* entity = state.hidden_truth.find_entity(entity_id);
    if (entity == nullptr) {
        evaluation.validation_errors.push_back("candidate " + candidate.id + " structured metadata field " + std::string(field_name) + " references missing entity " + entity_id);
        return nullptr;
    }
    if (!candidate_type_is_allowed(entity->type, allowed_types)) {
        evaluation.validation_errors.push_back("candidate " + candidate.id + " structured metadata field " + std::string(field_name) + " expected compatible entity type but got " + to_string(entity->type) + " for " + entity_id);
        return nullptr;
    }
    return entity;
}

void add_candidate_metadata_temporal_error_if_needed(const CandidateFeature& candidate,
                                                     std::string_view field_name,
                                                     const Entity& entity,
                                                     std::string_view year_kind,
                                                     int year,
                                                     CandidateEvaluation& evaluation) {
    if (!entity.existence_interval.contains(year)) {
        evaluation.validation_errors.push_back(
            "candidate " + candidate.id + " structured metadata field " + std::string(field_name) +
            " is unavailable at " + std::string(year_kind) + " " + std::to_string(year) +
            "; valid range is " + std::to_string(entity.existence_interval.start_year) + "-" +
            std::to_string(entity.existence_interval.end_year));
    }
}

void add_candidate_claimed_surface_report_if_needed(const CandidateFeature& candidate,
                                                    std::string_view field_name,
                                                    const Entity& entity,
                                                    int claimed_year,
                                                    CandidateEvaluation& evaluation,
                                                    bool predict_contradiction) {
    if (entity.existence_interval.contains(claimed_year)) {
        return;
    }

    const AnachronismStatus status = status_for_candidate_mediation(candidate);
    evaluation.anachronisms.push_back(candidate_anachronism_report(
        candidate.id,
        std::string("claimed surface ") + std::string(field_name) + ": " + entity.canonical_name,
        claimed_year,
        entity.existence_interval.start_year,
        entity.existence_interval.end_year,
        status,
        explanation_for_candidate_status(status, candidate)
    ));

    if (predict_contradiction) {
        evaluation.predicted_contradictions.push_back(candidate_predicted_contradiction(
            candidate,
            entity.id,
            cause_for_candidate_status(status)
        ));
    }
}

void validate_candidate_artifact_metadata(const ArchiveEngineState& state,
                                          const CandidateFeature& candidate,
                                          CandidateEvaluation& evaluation) {
    if (!candidate.structured_artifact_metadata.has_value()) {
        return;
    }

    const CandidateArtifactMetadata& metadata = *candidate.structured_artifact_metadata;
    auto require_int = [&](const std::optional<int>& value, std::string_view field_name) -> std::optional<int> {
        if (!value.has_value()) {
            evaluation.validation_errors.push_back("candidate " + candidate.id + " structured metadata missing " + std::string(field_name));
            return std::nullopt;
        }
        return value;
    };
    auto require_string = [&](const std::optional<std::string>& value, std::string_view field_name) -> std::optional<std::string> {
        if (!value.has_value() || value->empty()) {
            evaluation.validation_errors.push_back("candidate " + candidate.id + " structured metadata missing " + std::string(field_name));
            return std::nullopt;
        }
        return value;
    };

    const std::optional<int> true_creation_year = require_int(metadata.true_creation_year, "true_creation_year");
    const std::optional<int> claimed_creation_year = require_int(metadata.claimed_creation_year, "claimed_creation_year");
    const std::optional<int> discovery_year = require_int(metadata.discovery_year, "discovery_year");
    const std::optional<std::string> location_created = require_string(metadata.location_created, "location_created");
    const std::optional<std::string> location_found = require_string(metadata.location_found, "location_found");
    const std::optional<std::string> language_id = require_string(metadata.language_id, "language_id");
    const std::optional<std::string> dialect_id = require_string(metadata.dialect_id, "dialect_id");
    const std::optional<std::string> script_id = require_string(metadata.script_id, "script_id");

    if (true_creation_year.has_value() && discovery_year.has_value() && *true_creation_year > *discovery_year) {
        evaluation.validation_errors.push_back("candidate " + candidate.id + " structured metadata true_creation_year is after discovery_year");
    }

    const Entity* created_site = nullptr;
    const Entity* found_site = nullptr;
    const Entity* language = nullptr;
    const Entity* dialect = nullptr;
    const Entity* script = nullptr;

    if (location_created.has_value()) {
        created_site = validated_candidate_entity_field(state, candidate, *location_created, "location_created", {EntityType::Site, EntityType::Settlement}, evaluation);
    }
    if (location_found.has_value()) {
        found_site = validated_candidate_entity_field(state, candidate, *location_found, "location_found", {EntityType::Site, EntityType::Settlement}, evaluation);
    }
    if (language_id.has_value()) {
        language = validated_candidate_entity_field(state, candidate, *language_id, "language_id", {EntityType::Language}, evaluation);
    }
    if (dialect_id.has_value()) {
        dialect = validated_candidate_entity_field(state, candidate, *dialect_id, "dialect_id", {EntityType::Dialect}, evaluation);
    }
    if (script_id.has_value()) {
        script = validated_candidate_entity_field(state, candidate, *script_id, "script_id", {EntityType::Script}, evaluation);
    }

    if (true_creation_year.has_value()) {
        if (created_site != nullptr) {
            add_candidate_metadata_temporal_error_if_needed(candidate, "location_created", *created_site, "true_creation_year", *true_creation_year, evaluation);
        }
        if (language != nullptr) {
            add_candidate_metadata_temporal_error_if_needed(candidate, "language_id", *language, "true_creation_year", *true_creation_year, evaluation);
        }
        if (dialect != nullptr) {
            add_candidate_metadata_temporal_error_if_needed(candidate, "dialect_id", *dialect, "true_creation_year", *true_creation_year, evaluation);
        }
        if (script != nullptr) {
            add_candidate_metadata_temporal_error_if_needed(candidate, "script_id", *script, "true_creation_year", *true_creation_year, evaluation);
        }
    }

    if (discovery_year.has_value() && found_site != nullptr) {
        add_candidate_metadata_temporal_error_if_needed(candidate, "location_found", *found_site, "discovery_year", *discovery_year, evaluation);
    }

    if (claimed_creation_year.has_value()) {
        if (language != nullptr) {
            add_candidate_claimed_surface_report_if_needed(candidate, "language", *language, *claimed_creation_year, evaluation, false);
        }
        if (dialect != nullptr) {
            add_candidate_claimed_surface_report_if_needed(candidate, "dialect", *dialect, *claimed_creation_year, evaluation, false);
        }
        if (script != nullptr) {
            add_candidate_claimed_surface_report_if_needed(candidate, "script", *script, *claimed_creation_year, evaluation, false);
        }

        for (const std::string& entity_id : metadata.referenced_entity_ids) {
            const Entity* referenced = state.hidden_truth.find_entity(entity_id);
            if (referenced == nullptr) {
                evaluation.validation_errors.push_back("candidate " + candidate.id + " structured metadata references missing entity " + entity_id);
                continue;
            }
            add_candidate_claimed_surface_report_if_needed(candidate, "referenced entity", *referenced, *claimed_creation_year, evaluation, referenced->type == EntityType::Office);
        }
    }
}


[[nodiscard]] bool candidate_claim_entity_is_mediated_at_year(const CandidateFeature& candidate) {
    return candidate_metadata_has_any_mediation(candidate, {
        EvidenceModifier::Forgery,
        EvidenceModifier::LaterCopy,
        EvidenceModifier::Interpolation,
        EvidenceModifier::RitualAnachronism,
        EvidenceModifier::MythicCompression,
        EvidenceModifier::MisdatedStratum,
    });
}

void validate_candidate_claim_entity(const ArchiveEngineState& state,
                                     const CandidateFeature& candidate,
                                     const CandidateClaimMetadata& claim,
                                     std::size_t index,
                                     std::string_view field_name,
                                     const std::optional<std::string>& maybe_entity_id,
                                     CandidateEvaluation& evaluation) {
    if (!maybe_entity_id.has_value()) {
        return;
    }

    const Entity* entity = state.hidden_truth.find_entity(*maybe_entity_id);
    if (entity == nullptr) {
        evaluation.validation_errors.push_back(
            "candidate " + candidate.id + " structured claim " + std::to_string(index) +
            " field " + std::string(field_name) + " references missing entity " + *maybe_entity_id);
        return;
    }

    if (claim.claimed_year.has_value() &&
        !entity->existence_interval.contains(*claim.claimed_year) &&
        !candidate_claim_entity_is_mediated_at_year(candidate)) {
        evaluation.validation_errors.push_back(
            "candidate " + candidate.id + " structured claim " + std::to_string(index) +
            " field " + std::string(field_name) + " references entity " + *maybe_entity_id +
            " outside valid range at claimed_year=" + std::to_string(*claim.claimed_year));
    }
}

void validate_candidate_claim_metadata(const ArchiveEngineState& state,
                                       const CandidateFeature& candidate,
                                       CandidateEvaluation& evaluation) {
    if (candidate.structured_claims.empty()) {
        return;
    }

    if (!candidate.structured_artifact_metadata.has_value()) {
        evaluation.validation_errors.push_back(
            "candidate " + candidate.id + " has structured claims but no structured artifact metadata");
        return;
    }

    for (std::size_t i = 0; i < candidate.structured_claims.size(); ++i) {
        const CandidateClaimMetadata& claim = candidate.structured_claims[i];
        auto require_text = [&](const std::string& value, std::string_view field_name) {
            if (value.empty()) {
                evaluation.validation_errors.push_back(
                    "candidate " + candidate.id + " structured claim " + std::to_string(i) +
                    " missing " + std::string(field_name));
            }
        };

        require_text(claim.subject, "subject");
        require_text(claim.predicate, "predicate");
        require_text(claim.object, "object");
        require_text(claim.literal_content, "literal_content");

        if (claim.confidence < 0.0 || claim.confidence > 1.0) {
            evaluation.validation_errors.push_back(
                "candidate " + candidate.id + " structured claim " + std::to_string(i) +
                " has confidence outside 0..1");
        }

        const CandidateArtifactMetadata& artifact_metadata = *candidate.structured_artifact_metadata;
        if (claim.claimed_year.has_value() && artifact_metadata.true_creation_year.has_value() &&
            *claim.claimed_year > *artifact_metadata.true_creation_year &&
            !candidate_metadata_declares(candidate, EvidenceModifier::RitualAnachronism)) {
            evaluation.validation_errors.push_back(
                "candidate " + candidate.id + " structured claim " + std::to_string(i) +
                " claimed_year is after source artifact true_creation_year");
        }

        validate_candidate_claim_entity(state, candidate, claim, i, "subject_entity_id", claim.subject_entity_id, evaluation);
        validate_candidate_claim_entity(state, candidate, claim, i, "object_entity_id", claim.object_entity_id, evaluation);

        if (claim.object_entity_id.has_value()) {
            const Entity* object = state.hidden_truth.find_entity(*claim.object_entity_id);
            if (object != nullptr) {
                if (claim.predicate_type == PredicateType::CreatedOffice && object->type != EntityType::Office) {
                    evaluation.validation_errors.push_back(
                        "candidate " + candidate.id + " structured claim " + std::to_string(i) +
                        " CreatedOffice expects object_entity_id to be an office");
                }
                if (claim.predicate_type == PredicateType::UsesScript && object->type != EntityType::Script) {
                    evaluation.validation_errors.push_back(
                        "candidate " + candidate.id + " structured claim " + std::to_string(i) +
                        " UsesScript expects object_entity_id to be a script");
                }
                if (claim.predicate_type == PredicateType::LocatedAt &&
                    object->type != EntityType::Site && object->type != EntityType::Settlement) {
                    evaluation.validation_errors.push_back(
                        "candidate " + candidate.id + " structured claim " + std::to_string(i) +
                        " LocatedAt expects object_entity_id to be a site or settlement");
                }
            }
        }
    }
}

[[nodiscard]] CandidateEvaluation evaluate_candidate_feature(const ArchiveEngineState& state,
                                                             const CandidateFeature& candidate,
                                                             AccessLevel access) {
    (void)access;
    CandidateEvaluation evaluation;
    evaluation.evaluated_candidate_id = candidate.id;
    evaluation.originality = score_originality_feature(originality_feature_for_candidate(candidate));

    validate_candidate_links(state, candidate, evaluation.validation_errors);
    validate_candidate_artifact_metadata(state, candidate, evaluation);
    validate_candidate_claim_metadata(state, candidate, evaluation);

    if (candidate.description.empty()) {
        evaluation.validation_errors.push_back("candidate " + candidate.id + " has empty description");
    }
    if (!candidate_is_structured(candidate) && contains_ci(candidate.description, "missing")) {
        evaluation.validation_errors.push_back("candidate " + candidate.id + " is missing required artifact metadata");
    }

    const bool claimed_early = contains_ci(candidate.description, "553") || contains_ci(candidate.description, "before 612") || contains_ci(candidate.description, "before the office exists");
    const bool uses_green_seal = contains_ci(candidate.description, "green-seal") || contains_ci(candidate.description, "green seal") || contains_ci(candidate.description, "script.green_seal");
    const bool references_drowned_chancellor = contains_ci(candidate.description, "drowned chancellor") || contains_ci(candidate.description, "office.drowned_chancellor");
    const bool declares_forgery = candidate_declares_mediation(candidate, {"forgery", "forged"});
    const bool declares_later_copy = candidate_declares_mediation(candidate, {"later copy", "later-copy"});
    const bool declares_mythic = candidate_declares_mediation(candidate, {"mythic", "mythic compression"});

    if (!candidate_is_structured(candidate) && claimed_early && uses_green_seal) {
        const AnachronismStatus status = declares_forgery ? AnachronismStatus::ValidBecauseForged :
                                          declares_later_copy ? AnachronismStatus::ValidBecauseLaterCopy :
                                          AnachronismStatus::InvalidGenerationBug;
        evaluation.anachronisms.push_back(candidate_anachronism_report(
            candidate.id,
            "claimed surface script: Green-Seal Script",
            553,
            612,
            900,
            status,
            status == AnachronismStatus::InvalidGenerationBug ? "none" : (declares_forgery ? "Forgery" : "LaterCopy")
        ));
    }

    if (!candidate_is_structured(candidate) && claimed_early && references_drowned_chancellor) {
        const AnachronismStatus status = declares_forgery ? AnachronismStatus::ValidBecauseForged :
                                          declares_later_copy ? AnachronismStatus::ValidBecauseLaterCopy :
                                          declares_mythic ? AnachronismStatus::ValidBecauseRitual :
                                          AnachronismStatus::InvalidGenerationBug;
        evaluation.anachronisms.push_back(candidate_anachronism_report(
            candidate.id,
            "referenced office: Drowned Chancellor",
            553,
            617,
            900,
            status,
            status == AnachronismStatus::InvalidGenerationBug ? "none" :
                (declares_forgery ? "Forgery" : (declares_later_copy ? "LaterCopy" : "MythicCompression"))
        ));
        evaluation.predicted_contradictions.push_back(candidate_predicted_contradiction(
            candidate,
            "office.drowned_chancellor",
            declares_forgery ? ContradictionCause::Forgery :
                (declares_mythic ? ContradictionCause::MythologizedMemory : ContradictionCause::UnresolvedGenerationBug)
        ));
    }

    const bool has_invalid_anachronism = std::any_of(evaluation.anachronisms.begin(), evaluation.anachronisms.end(), [](const AnachronismReport& report) {
        return report.status == AnachronismStatus::InvalidGenerationBug;
    });
    const bool high_originality_risk = evaluation.originality.direct_copy_risk_score >= 0.35 ||
                                       (evaluation.originality.civilization_specificity_score < 0.30 && !evaluation.originality.trope_flags.empty());

    if (!evaluation.validation_errors.empty()) {
        evaluation.decision = CandidateDecision::Reject;
        evaluation.explanation = "Candidate rejected because required metadata or references are invalid.";
    } else if (has_invalid_anachronism) {
        evaluation.decision = CandidateDecision::Reject;
        evaluation.explanation = "Candidate rejected because it contains an unmediated chronological conflict.";
    } else if (declares_forgery && !evaluation.anachronisms.empty()) {
        evaluation.decision = CandidateDecision::AcceptAsForgery;
        evaluation.explanation = "Candidate can fit only as an explicitly mediated forgery; archive state was not mutated.";
    } else if (declares_later_copy && !evaluation.anachronisms.empty()) {
        evaluation.decision = CandidateDecision::AcceptAsLaterCopy;
        evaluation.explanation = "Candidate can fit as a later copy; archive state was not mutated.";
    } else if (declares_mythic && !evaluation.anachronisms.empty()) {
        evaluation.decision = CandidateDecision::AcceptAsMythicCompression;
        evaluation.explanation = "Candidate can fit as mythic or ritual compression; archive state was not mutated.";
    } else if (high_originality_risk) {
        evaluation.decision = CandidateDecision::NeedsCuratorReview;
        evaluation.explanation = "Candidate is structurally possible but carries originality/trope risk that requires curator revision.";
    } else {
        evaluation.decision = CandidateDecision::Accept;
        evaluation.explanation = "Candidate is structurally compatible with the current archive slice; evaluation did not mutate the archive.";
    }

    return evaluation;
}

[[nodiscard]] bool candidate_can_view_anachronism_detail(AccessLevel access) {
    return can_view(access, AccessLevel::Curator);
}

[[nodiscard]] bool candidate_can_view_exact_valid_range(AccessLevel access) {
    return can_view(access, AccessLevel::Curator);
}

[[nodiscard]] bool candidate_can_view_internal_status(AccessLevel access) {
    return can_view(access, AccessLevel::Curator);
}

[[nodiscard]] bool candidate_can_view_predicted_contradiction_detail(AccessLevel access) {
    return can_view(access, AccessLevel::Curator);
}

[[nodiscard]] std::string display_candidate_decision(CandidateDecision decision, AccessLevel access) {
    if (can_view(access, AccessLevel::Curator)) {
        return to_string(decision);
    }

    switch (decision) {
        case CandidateDecision::Accept: return "Accept";
        case CandidateDecision::Reject: return "Reject";
        case CandidateDecision::AcceptAsForgery:
        case CandidateDecision::AcceptAsLaterCopy:
        case CandidateDecision::AcceptAsMythicCompression:
            return "AcceptWithDeclaredMediation";
        case CandidateDecision::NeedsCuratorReview:
            return "NeedsReview";
    }
    return "unknown";
}

[[nodiscard]] std::string display_candidate_explanation(const CandidateEvaluation& evaluation, AccessLevel access) {
    if (can_view(access, AccessLevel::Curator)) {
        return evaluation.explanation;
    }

    switch (evaluation.decision) {
        case CandidateDecision::Accept:
            return "Candidate is structurally compatible with the visible archive slice; evaluation did not mutate the archive.";
        case CandidateDecision::Reject:
            if (!evaluation.validation_errors.empty()) {
                return "Candidate rejected because required public-facing structure is incomplete or invalid.";
            }
            return "Candidate rejected because it appears chronologically incompatible without acceptable mediation.";
        case CandidateDecision::AcceptAsForgery:
        case CandidateDecision::AcceptAsLaterCopy:
        case CandidateDecision::AcceptAsMythicCompression:
            return "Candidate can fit only with declared mediation or non-original status; archive state was not mutated.";
        case CandidateDecision::NeedsCuratorReview:
            return "Candidate is structurally possible but requires restricted review before acceptance.";
    }
    return evaluation.explanation;
}

[[nodiscard]] bool candidate_report_has_invalid_anachronism(const CandidateEvaluation& evaluation) {
    return std::any_of(evaluation.anachronisms.begin(), evaluation.anachronisms.end(), [](const AnachronismReport& report) {
        return report.status == AnachronismStatus::InvalidGenerationBug;
    });
}

[[nodiscard]] bool candidate_report_has_mediated_anachronism(const CandidateEvaluation& evaluation) {
    return std::any_of(evaluation.anachronisms.begin(), evaluation.anachronisms.end(), [](const AnachronismReport& report) {
        return report.status != AnachronismStatus::InvalidGenerationBug;
    });
}

void format_candidate_anachronisms_for_access(std::ostringstream& out,
                                              const CandidateEvaluation& evaluation,
                                              AccessLevel access) {
    if (evaluation.anachronisms.empty()) {
        return;
    }

    if (!candidate_can_view_anachronism_detail(access)) {
        out << "Predicted issues:\n";
        if (candidate_report_has_invalid_anachronism(evaluation)) {
            out << "- Candidate appears chronologically incompatible with the visible archive.\n";
        }
        if (candidate_report_has_mediated_anachronism(evaluation)) {
            out << "- Candidate declares a mediated or non-original status; restricted catalog review would be required before publication.\n";
        }
        return;
    }

    out << "Predicted anachronisms:\n";
    for (const AnachronismReport& report : evaluation.anachronisms) {
        out << "- " << report.referenced_item
            << ", " << report.checked_year_kind << "=" << report.checked_year;
        if (candidate_can_view_exact_valid_range(access)) {
            out << ", valid_range=" << availability_range_text(report);
        }
        if (candidate_can_view_internal_status(access)) {
            out << ", status=" << to_string(report.status);
        }
        out << ", explanation=" << report.allowed_explanation << "\n";
    }
}

void format_candidate_predicted_contradictions_for_access(std::ostringstream& out,
                                                          const CandidateEvaluation& evaluation,
                                                          AccessLevel access) {
    if (evaluation.predicted_contradictions.empty()) {
        return;
    }

    out << "Predicted contradictions:\n";
    if (!candidate_can_view_predicted_contradiction_detail(access)) {
        out << "- Candidate may conflict with known chronology or cataloged authority terms.\n";
        return;
    }

    for (const Contradiction& contradiction : evaluation.predicted_contradictions) {
        out << "- " << contradiction.id << " [" << contradiction.detector_rule << "]";
        if (can_view(access, contradiction.cause_min_access)) {
            out << " assigned_cause=" << to_string(contradiction.assigned_cause);
        }
        out << "\n";
    }
}

[[nodiscard]] std::string format_candidate_evaluation(const CandidateEvaluation& evaluation,
                                                      const CandidateFeature& candidate,
                                                      AccessLevel access) {
    std::ostringstream out;
    out << "Candidate evaluation visible to " << to_string(access) << ":\n";
    out << "- candidate: " << candidate.id << " [" << to_string(candidate.type) << "]\n";
    out << "- decision: " << display_candidate_decision(evaluation.decision, access) << "\n";
    out << "- explanation: " << display_candidate_explanation(evaluation, access) << "\n";

    if (!evaluation.validation_errors.empty()) {
        out << "Validation errors:\n";
        if (can_view(access, AccessLevel::Curator)) {
            for (const std::string& error : evaluation.validation_errors) {
                out << "- " << error << "\n";
            }
        } else {
            out << "- Candidate has incomplete or invalid required structure.\n";
        }
    }

    format_candidate_anachronisms_for_access(out, evaluation, access);
    format_candidate_predicted_contradictions_for_access(out, evaluation, access);

    if (can_view(access, AccessLevel::Curator)) {
        out << "Originality audit:\n";
        out << "- trope_similarity=" << std::fixed << std::setprecision(2) << evaluation.originality.trope_similarity_score
            << ", transformed_trope=" << std::fixed << std::setprecision(2) << evaluation.originality.transformed_trope_score
            << ", direct_copy_risk=" << std::fixed << std::setprecision(2) << evaluation.originality.direct_copy_risk_score
            << ", civilization_specificity=" << std::fixed << std::setprecision(2) << evaluation.originality.civilization_specificity_score
            << ", flags=" << trope_flags_text(evaluation.originality.trope_flags)
            << ", local_dependencies=" << dependency_list_text(evaluation.originality.required_local_dependencies)
            << "\n";
        out << "  assessment: " << evaluation.originality.assessment << "\n";
        if (!evaluation.originality.rationales.empty()) {
            out << "  rationales:\n";
            for (const OriginalityRationale& rationale : evaluation.originality.rationales) {
                out << "    - " << to_string(rationale.flag) << ": " << rationale.matched_pattern
                    << " — " << rationale.explanation << "\n";
            }
        }
    } else {
        out << "- candidate originality internals are restricted to curator/canon/debug access\n";
    }

    return out.str();
}

[[nodiscard]] std::string format_candidate_query(const ArchiveEngineState& state,
                                                 AccessLevel access,
                                                 std::string_view candidate_id) {
    const CandidateFeature candidate = sample_candidate_feature(candidate_id);
    const CandidateEvaluation evaluation = evaluate_candidate_feature(state, candidate, access);
    return format_candidate_evaluation(evaluation, candidate, access);
}

void register_discovery_for_artifact(ArchiveEngineState& state, const std::string& artifact_id) {
    const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
    if (artifact == nullptr) {
        return;
    }

    const auto existing = std::find_if(state.discovery_log.begin(), state.discovery_log.end(), [&](const Discovery& discovery) {
        return discovery.artifact_id == artifact_id;
    });
    if (existing != state.discovery_log.end()) {
        return;
    }

    state.discovery_log.push_back(Discovery{
        "discovery." + claim_suffix_for_id(artifact_id),
        artifact_id,
        artifact->discovery_year,
        artifact->location_found,
        artifact->min_access,
    });

    std::sort(state.discovery_log.begin(), state.discovery_log.end(), [](const Discovery& lhs, const Discovery& rhs) {
        if (lhs.discovery_year != rhs.discovery_year) {
            return lhs.discovery_year < rhs.discovery_year;
        }
        return lhs.artifact_id < rhs.artifact_id;
    });
}

[[nodiscard]] bool materialization_decision_is_insertable(CandidateDecision decision) {
    switch (decision) {
        case CandidateDecision::Accept:
        case CandidateDecision::AcceptAsForgery:
        case CandidateDecision::AcceptAsLaterCopy:
        case CandidateDecision::AcceptAsMythicCompression:
            return true;
        case CandidateDecision::Reject:
        case CandidateDecision::NeedsCuratorReview:
            return false;
    }
    return false;
}


[[nodiscard]] bool is_generated_candidate(const CandidateFeature& candidate) {
    return candidate.id.rfind("candidate.generated.", 0) == 0;
}

[[nodiscard]] std::string generated_materialized_suffix(const CandidateFeature& candidate) {
    return claim_suffix_for_id(candidate.id);
}

[[nodiscard]] bool generated_candidate_is(CandidateFeature const& candidate, std::string_view strategy_name) {
    return candidate.id.find(std::string("candidate.generated.") + std::string(strategy_name) + ".") == 0;
}

[[nodiscard]] bool candidate_has_proposed_link(const CandidateFeature& candidate, std::string_view link) {
    return std::find(candidate.proposed_links.begin(), candidate.proposed_links.end(), std::string(link)) != candidate.proposed_links.end();
}

[[nodiscard]] std::vector<std::string> candidate_proposed_ids_with_prefix(const CandidateFeature& candidate, std::string_view prefix) {
    std::vector<std::string> ids;
    for (const std::string& link : candidate.proposed_links) {
        if (link.rfind(std::string(prefix), 0) == 0) {
            ids.push_back(link.substr(prefix.size()));
        }
    }
    return ids;
}

[[nodiscard]] bool generated_candidate_targets_silt_levy(const CandidateFeature& candidate) {
    return candidate_has_proposed_link(candidate, "claim:claim.levy_exists_607") &&
           !candidate_has_proposed_link(candidate, "entity:office.drowned_chancellor");
}

[[nodiscard]] ArtifactType generated_artifact_type_for(const CandidateFeature& candidate) {
    if (generated_candidate_is(candidate, "misleading_forgery")) {
        return ArtifactType::ForgedDecree;
    }
    if (generated_candidate_is(candidate, "ritual_variant")) {
        return ArtifactType::OralHistory;
    }
    if (generated_candidate_is(candidate, "corroborating_fragment")) {
        return ArtifactType::TradeLedger;
    }
    return ArtifactType::DamagedManuscript;
}

[[nodiscard]] std::vector<EvidenceModifier> generated_evidence_modifiers_for(const CandidateFeature& candidate) {
    std::vector<EvidenceModifier> modifiers;
    if (candidate.structured_artifact_metadata.has_value()) {
        modifiers = candidate.structured_artifact_metadata->declared_mediations;
    }
    if (generated_candidate_is(candidate, "corroborating_fragment")) {
        if (std::find(modifiers.begin(), modifiers.end(), EvidenceModifier::NarrowScope) == modifiers.end()) {
            modifiers.push_back(EvidenceModifier::NarrowScope);
        }
    }
    if (generated_candidate_is(candidate, "ritual_variant")) {
        if (std::find(modifiers.begin(), modifiers.end(), EvidenceModifier::MythicCompression) == modifiers.end()) {
            modifiers.push_back(EvidenceModifier::MythicCompression);
        }
        if (std::find(modifiers.begin(), modifiers.end(), EvidenceModifier::RitualAnachronism) == modifiers.end()) {
            modifiers.push_back(EvidenceModifier::RitualAnachronism);
        }
    }
    return modifiers;
}

[[nodiscard]] std::string generated_title_for(const CandidateFeature& candidate) {
    if (generated_candidate_is(candidate, "misleading_forgery")) {
        return "Generated Early Drowned Chancellor Forgery";
    }
    if (generated_candidate_is(candidate, "ritual_variant")) {
        return candidate_has_proposed_link(candidate, "mystery:mystery.third_lock_authority") ?
            "Generated Three Keepers Ritual Variant" : "Generated Ritual Variant";
    }
    if (generated_candidate_is(candidate, "corroborating_fragment")) {
        return generated_candidate_targets_silt_levy(candidate) ?
            "Generated Silt-Levy Corroborating Fragment" : "Generated Moon-Lock Corroborating Fragment";
    }
    return "Generated Candidate Artifact";
}

[[nodiscard]] ReliabilityComponents generated_reliability_for(const CandidateFeature& candidate) {
    if (generated_candidate_is(candidate, "misleading_forgery")) {
        return [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.22;
            value.preservation_integrity = 0.66;
            value.temporal_proximity = 0.04;
            value.creator_access_to_events = 0.18;
            value.bias_penalty = 0.45;
            value.forgery_penalty = 0.95;
            value.translation_confidence = 0.50;
            value.external_corroboration = 0.10;
            value.contradiction_penalty = 0.76;
            return value;
        }();
    }
    if (generated_candidate_is(candidate, "ritual_variant")) {
        return [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.40;
            value.preservation_integrity = 0.60;
            value.temporal_proximity = 0.30;
            value.creator_access_to_events = 0.36;
            value.bias_penalty = 0.22;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.50;
            value.external_corroboration = 0.34;
            value.contradiction_penalty = 0.40;
            return value;
        }();
    }
    return [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.58;
            value.preservation_integrity = 0.55;
            value.temporal_proximity = 0.62;
            value.creator_access_to_events = 0.58;
            value.bias_penalty = 0.12;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.62;
            value.external_corroboration = 0.44;
            value.contradiction_penalty = 0.12;
            return value;
        }();
}


[[nodiscard]] bool candidate_has_structured_materialization_payload(const CandidateFeature& candidate) {
    return candidate.structured_artifact_metadata.has_value() && !candidate.structured_claims.empty();
}

[[nodiscard]] std::vector<std::string> event_links_for_structured_candidate(const CandidateFeature& candidate) {
    std::vector<std::string> events = candidate_proposed_ids_with_prefix(candidate, "event:");
    if (!events.empty()) {
        return events;
    }
    if (candidate_has_proposed_link(candidate, "claim:claim.levy_exists_607")) {
        return {"event.canal_tax_reform"};
    }
    if (candidate_has_proposed_link(candidate, "entity:office.drowned_chancellor")) {
        return {"event.salt_moon_schism"};
    }
    return {"event.salt_moon_schism"};
}

[[nodiscard]] std::vector<EvidenceModifier> structured_candidate_evidence_modifiers_for(const CandidateFeature& candidate) {
    std::vector<EvidenceModifier> modifiers;
    if (candidate.structured_artifact_metadata.has_value()) {
        modifiers = candidate.structured_artifact_metadata->declared_mediations;
    }
    if (modifiers.empty()) {
        modifiers.push_back(EvidenceModifier::NarrowScope);
    }
    return modifiers;
}

[[nodiscard]] std::optional<Artifact> materialized_structured_candidate_artifact(const CandidateFeature& candidate,
                                                                                 const CandidateEvaluation& evaluation) {
    if (is_generated_candidate(candidate) ||
        !candidate_has_structured_materialization_payload(candidate) ||
        !materialization_decision_is_insertable(evaluation.decision)) {
        return std::nullopt;
    }

    const CandidateArtifactMetadata& metadata = *candidate.structured_artifact_metadata;
    if (!metadata.true_creation_year.has_value() || !metadata.claimed_creation_year.has_value() ||
        !metadata.discovery_year.has_value() || !metadata.location_created.has_value() ||
        !metadata.location_found.has_value() || !metadata.language_id.has_value() ||
        !metadata.dialect_id.has_value() || !metadata.script_id.has_value()) {
        return std::nullopt;
    }

    Artifact artifact;
    const std::string suffix = claim_suffix_for_id(candidate.id);
    artifact.id = "artifact.materialized_" + suffix;
    artifact.type = candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ? ArtifactType::ForgedDecree : ArtifactType::DamagedManuscript;
    artifact.voice_register = default_voice_register_for(artifact.type);
    artifact.title = "Materialized Structured Candidate";
    artifact.creator_id = candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ?
        "forger.structured_candidate" : "scribe.structured_candidate";
    artifact.attributed_creator_id = candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ?
        "person.aru" : artifact.creator_id;
    artifact.true_creation_year = *metadata.true_creation_year;
    artifact.claimed_creation_year = *metadata.claimed_creation_year;
    artifact.discovery_year = *metadata.discovery_year;
    artifact.location_created = *metadata.location_created;
    artifact.location_found = *metadata.location_found;
    artifact.language_id = *metadata.language_id;
    artifact.dialect_id = *metadata.dialect_id;
    artifact.script_id = *metadata.script_id;
    artifact.material = candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ?
        "structured disputed candidate tablet" : "structured candidate fragment";
    artifact.preservation_quality = 0.56;
    artifact.damage_profile = "structured candidate materialization with controlled catalog damage";
    artifact.transmission_history = "structured candidate supplied through typed ingestion metadata";
    artifact.creator_knowledge_scope = "candidate constrained by typed artifact and claim metadata";
    artifact.creator_bias_profile = candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ?
        "declared mediated forgery" : "typed candidate narrow context";
    artifact.creator_motive = candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ?
        "mediate anachronistic authority language" : "preserve typed candidate evidence";
    artifact.intended_audience = "curated structured candidate review";
    artifact.public_text = candidate.description;
    artifact.literal_translation = candidate.description;
    artifact.scholarly_translation = "Structured candidate materialized through typed artifact and claim metadata.";
    artifact.hidden_event_links = event_links_for_structured_candidate(candidate);
    artifact.referenced_entity_ids = metadata.referenced_entity_ids;
    artifact.distortion_profile = candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ?
        "structured candidate mediated as forgery" : "structured candidate narrow scope";
    artifact.evidence_modifiers = structured_candidate_evidence_modifiers_for(candidate);
    artifact.reliability_components = candidate_metadata_declares(candidate, EvidenceModifier::Forgery) ?
        [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.20;
            value.preservation_integrity = 0.56;
            value.temporal_proximity = 0.05;
            value.creator_access_to_events = 0.20;
            value.bias_penalty = 0.45;
            value.forgery_penalty = 0.90;
            value.translation_confidence = 0.50;
            value.external_corroboration = 0.10;
            value.contradiction_penalty = 0.70;
            return value;
        }() :
        [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.56;
            value.preservation_integrity = 0.56;
            value.temporal_proximity = 0.62;
            value.creator_access_to_events = 0.54;
            value.bias_penalty = 0.12;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.61;
            value.external_corroboration = 0.40;
            value.contradiction_penalty = 0.12;
            return value;
        }();
    artifact.generation_trace = "structured_candidate_materialization=" + candidate.id + "; source=typed_ingestion; mutation_requires_curator_or_higher";
    return finalize_artifact(std::move(artifact));
}

[[nodiscard]] std::optional<Artifact> materialized_generated_candidate_artifact(const CandidateFeature& candidate,
                                                                                const CandidateEvaluation& evaluation) {
    if (!is_generated_candidate(candidate) || !candidate.structured_artifact_metadata.has_value() ||
        !materialization_decision_is_insertable(evaluation.decision)) {
        return std::nullopt;
    }

    const CandidateArtifactMetadata& metadata = *candidate.structured_artifact_metadata;
    if (!metadata.true_creation_year.has_value() || !metadata.claimed_creation_year.has_value() ||
        !metadata.discovery_year.has_value() || !metadata.location_created.has_value() ||
        !metadata.location_found.has_value() || !metadata.language_id.has_value() ||
        !metadata.dialect_id.has_value() || !metadata.script_id.has_value()) {
        return std::nullopt;
    }

    Artifact artifact;
    const std::string suffix = generated_materialized_suffix(candidate);
    artifact.id = "artifact.materialized_" + suffix;
    artifact.type = generated_artifact_type_for(candidate);
    artifact.voice_register = default_voice_register_for(artifact.type);
    artifact.title = generated_title_for(candidate);
    artifact.creator_id = generated_candidate_is(candidate, "misleading_forgery") ?
        "forger.generated_silt_partisan" : "scribe.generated_lock_archive";
    artifact.attributed_creator_id = generated_candidate_is(candidate, "misleading_forgery") ?
        "person.aru" : artifact.creator_id;
    artifact.true_creation_year = *metadata.true_creation_year;
    artifact.claimed_creation_year = *metadata.claimed_creation_year;
    artifact.discovery_year = *metadata.discovery_year;
    artifact.location_created = *metadata.location_created;
    artifact.location_found = *metadata.location_found;
    artifact.language_id = *metadata.language_id;
    artifact.dialect_id = *metadata.dialect_id;
    artifact.script_id = *metadata.script_id;
    artifact.material = generated_candidate_is(candidate, "misleading_forgery") ? "generated disputed clay decree" :
        (generated_candidate_is(candidate, "ritual_variant") ? "generated oral transcript tablet" : "generated lockhouse tally shard");
    artifact.preservation_quality = generated_candidate_is(candidate, "misleading_forgery") ? 0.66 : 0.55;
    artifact.damage_profile = generated_candidate_is(candidate, "ritual_variant") ?
        "variant refrain preserved through late transcription" : "generated fragment with controlled catalog damage";
    artifact.transmission_history = "generated candidate materialization from deterministic candidate batch";
    artifact.creator_knowledge_scope = "generated from existing archive constraints; no new hidden events";
    artifact.creator_bias_profile = generated_candidate_is(candidate, "ritual_variant") ? "ritual compression" :
        (generated_candidate_is(candidate, "misleading_forgery") ? "dynastic legal fabrication" : "bureaucratic narrow record");
    artifact.creator_motive = generated_candidate_is(candidate, "misleading_forgery") ?
        "retroactively legitimate lock-office authority" : "preserve lock authority evidence";
    artifact.intended_audience = generated_candidate_is(candidate, "ritual_variant") ? "festival listeners and later catalogers" : "lock-office auditors";
    artifact.public_text = candidate.description;
    artifact.literal_translation = candidate.description;
    artifact.scholarly_translation = "Generated candidate materialized through curator-or-higher-approved structured metadata.";
    if (generated_candidate_is(candidate, "corroborating_fragment") && generated_candidate_targets_silt_levy(candidate)) {
        artifact.hidden_event_links = {"event.canal_tax_reform"};
    } else if (generated_candidate_is(candidate, "corroborating_fragment")) {
        artifact.hidden_event_links = {"event.salt_moon_schism", "event.canal_tax_reform"};
    } else {
        artifact.hidden_event_links = {"event.salt_moon_schism"};
    }
    artifact.referenced_entity_ids = metadata.referenced_entity_ids;
    artifact.distortion_profile = generated_candidate_is(candidate, "misleading_forgery") ? "generated mediated forgery" :
        (generated_candidate_is(candidate, "ritual_variant") ? "generated ritual variant; mythic compression" :
         (generated_candidate_targets_silt_levy(candidate) ? "generated silt levy corroborating fragment; narrow scope" : "generated corroborating legal fragment; narrow scope"));
    artifact.evidence_modifiers = generated_evidence_modifiers_for(candidate);
    artifact.reliability_components = generated_reliability_for(candidate);
    artifact.generation_trace = "generated_candidate_materialization=" + candidate.id + "; source=deterministic_batch; mutation_requires_curator_or_higher";
    return finalize_artifact(std::move(artifact));
}

[[nodiscard]] std::optional<Artifact> materialized_candidate_artifact(const CandidateFeature& candidate,
                                                                      const CandidateEvaluation& evaluation) {
    if (std::optional<Artifact> generated = materialized_generated_candidate_artifact(candidate, evaluation)) {
        return generated;
    }
    if (std::optional<Artifact> structured = materialized_structured_candidate_artifact(candidate, evaluation)) {
        return structured;
    }

    if (candidate.id == "candidate.moon_lock_fragment" && evaluation.decision == CandidateDecision::Accept) {
        Artifact artifact;
        artifact.id = "artifact.materialized_moon_lock_fragment";
        artifact.type = ArtifactType::DamagedManuscript;
        artifact.voice_register = ArtifactVoiceRegister::DamagedChronicle;
        artifact.title = "Materialized Moon-Lock Legal Fragment";
        artifact.creator_id = "scribe.materialized_late_lattice";
        artifact.attributed_creator_id = "scribe.materialized_late_lattice";
        artifact.true_creation_year = 620;
        artifact.claimed_creation_year = 620;
        artifact.discovery_year = 813;
        artifact.location_created = "site.reservoir_gate";
        artifact.location_found = "site.salt_cellar_archive";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.upper_lattice";
        artifact.script_id = "script.green_seal";
        artifact.material = "thin reed legal fragment";
        artifact.preservation_quality = 0.52;
        artifact.damage_profile = "edge loss; legal formula partly preserved";
        artifact.transmission_history = "candidate materialization from curator-or-higher-approved legal fragment";
        artifact.creator_knowledge_scope = "post-schism lock-office legal practice";
        artifact.creator_bias_profile = "bureaucratic legal formula";
        artifact.creator_motive = "record lock authority transfer";
        artifact.intended_audience = "lock-office auditors";
        artifact.public_text = "Moon-lock seal: the Drowned Chancellor holds lower lock authority after the silt levy.";
        artifact.literal_translation = "Moon-lock seal; Drowned Chancellor holds lower locks after silt levy.";
        artifact.scholarly_translation = "A legal fragment linking the Drowned Chancellor to lower lock authority after the silt levy.";
        artifact.hidden_event_links = {"event.salt_moon_schism", "event.canal_tax_reform"};
        artifact.referenced_entity_ids = {"office.drowned_chancellor", "site.reservoir_gate", "script.green_seal"};
        artifact.distortion_profile = "candidate materialized legal fragment; narrow context";
        artifact.evidence_modifiers = {EvidenceModifier::NarrowScope};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.58;
            value.preservation_integrity = 0.52;
            value.temporal_proximity = 0.64;
            value.creator_access_to_events = 0.58;
            value.bias_penalty = 0.12;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.63;
            value.external_corroboration = 0.42;
            value.contradiction_penalty = 0.12;
            return value;
        }();
        artifact.generation_trace = "candidate_materialization=moon_lock_fragment; source=evaluation_gate; mutation_requires_curator_or_higher";
        return finalize_artifact(std::move(artifact));
    }

    if (candidate.id == "candidate.early_drowned_chancellor_forgery" &&
        evaluation.decision == CandidateDecision::AcceptAsForgery) {
        Artifact artifact;
        artifact.id = "artifact.materialized_early_drowned_chancellor_forgery";
        artifact.type = ArtifactType::ForgedDecree;
        artifact.voice_register = ArtifactVoiceRegister::DisputedDecree;
        artifact.title = "Materialized Early Drowned Chancellor Forgery";
        artifact.creator_id = "forger.materialized_silt_partisan";
        artifact.attributed_creator_id = "person.aru";
        artifact.true_creation_year = 661;
        artifact.claimed_creation_year = 553;
        artifact.discovery_year = 813;
        artifact.location_created = "site.salt_cellar_archive";
        artifact.location_found = "site.salt_cellar_archive";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.late_lock_hand";
        artifact.script_id = "script.green_seal";
        artifact.material = "curator-sampled clay decree";
        artifact.preservation_quality = 0.69;
        artifact.damage_profile = "seal rim too clean; filing striations inconsistent";
        artifact.transmission_history = "candidate materialization from declared forgery evaluation";
        artifact.creator_knowledge_scope = "post-schism partisan archive knowledge";
        artifact.creator_bias_profile = "dynastic legal fabrication";
        artifact.creator_motive = "retroactively legitimate lock-office authority";
        artifact.intended_audience = "curators and court litigants";
        artifact.public_text = "Aru appoints the Drowned Chancellor under the Moon-lock seal.";
        artifact.literal_translation = "Aru appoints Drowned Chancellor to Moon-lock seal.";
        artifact.scholarly_translation = "A declared forgery candidate with late office and script forms.";
        artifact.hidden_event_links = {"event.salt_moon_schism"};
        artifact.referenced_entity_ids = {"person.aru", "office.drowned_chancellor", "script.green_seal"};
        artifact.distortion_profile = "candidate materialized forgery; anachronistic office and script";
        artifact.evidence_modifiers = {EvidenceModifier::Forgery};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.20;
            value.preservation_integrity = 0.69;
            value.temporal_proximity = 0.05;
            value.creator_access_to_events = 0.20;
            value.bias_penalty = 0.50;
            value.forgery_penalty = 0.95;
            value.translation_confidence = 0.54;
            value.external_corroboration = 0.12;
            value.contradiction_penalty = 0.78;
            return value;
        }();
        artifact.generation_trace = "candidate_materialization=early_drowned_chancellor_forgery; source=evaluation_gate; mutation_requires_curator_or_higher";
        return finalize_artifact(std::move(artifact));
    }

    return std::nullopt;
}

void add_materialized_candidate_claims(PublicArchive& archive, Artifact& artifact, const CandidateFeature& candidate) {
    if (!candidate.structured_claims.empty()) {
        const std::string suffix = claim_suffix_for_id(candidate.id);
        for (std::size_t i = 0; i < candidate.structured_claims.size(); ++i) {
            const CandidateClaimMetadata& metadata = candidate.structured_claims[i];
            const std::string claim_id = "claim.materialized_" + suffix +
                (candidate.structured_claims.size() == 1 ? std::string{} : ("_" + std::to_string(i)));
            add_claim_to_archive(archive, artifact, Claim{
                claim_id,
                artifact.id,
                metadata.claim_type,
                metadata.subject,
                metadata.predicate,
                metadata.object,
                metadata.literal_content,
                metadata.confidence,
                metadata.min_access,
                ClaimSemantics{metadata.predicate_type, metadata.subject_entity_id, metadata.object_entity_id, metadata.claimed_year},
            });
        }
        return;
    }

    if (is_generated_candidate(candidate)) {
        const int claimed_year = candidate.structured_artifact_metadata && candidate.structured_artifact_metadata->claimed_creation_year ?
            *candidate.structured_artifact_metadata->claimed_creation_year : artifact.claimed_creation_year;
        const std::string suffix = generated_materialized_suffix(candidate);
        const std::string claim_id = "claim.materialized_" + suffix;
        if (generated_candidate_is(candidate, "misleading_forgery")) {
            add_claim_to_archive(archive, artifact, Claim{
                claim_id,
                artifact.id,
                ClaimType::LegalFiction,
                "King Aru",
                "appointed",
                "Drowned Chancellor",
                "Generated decree claims Aru appointed the Drowned Chancellor under Green-Seal authority.",
                0.18,
                AccessLevel::Public,
                ClaimSemantics{PredicateType::CreatedOffice, std::optional<std::string>{"person.aru"}, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<int>{claimed_year}},
            });
            return;
        }
        if (generated_candidate_is(candidate, "ritual_variant")) {
            add_claim_to_archive(archive, artifact, Claim{
                claim_id,
                artifact.id,
                ClaimType::MythicCompression,
                "three keepers",
                "became",
                "one moon judge",
                "Generated ritual variant compresses three lock keepers into one moon judge.",
                0.39,
                AccessLevel::Public,
                ClaimSemantics{PredicateType::Became, std::nullopt, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<int>{claimed_year}},
            });
            return;
        }
        if (generated_candidate_targets_silt_levy(candidate)) {
            add_claim_to_archive(archive, artifact, Claim{
                claim_id,
                artifact.id,
                ClaimType::FactualClaim,
                "silt levy",
                "existed_in_year",
                std::to_string(claimed_year),
                "Generated silt-levy fragment records levy receipts at Reservoir Gate.",
                0.58,
                AccessLevel::Public,
                ClaimSemantics{PredicateType::ExistedInYear, std::nullopt, std::optional<std::string>{"site.reservoir_gate"}, std::optional<int>{claimed_year}},
            });
            return;
        }
        add_claim_to_archive(archive, artifact, Claim{
            claim_id,
            artifact.id,
            ClaimType::FactualClaim,
            "Drowned Chancellor",
            "received",
            "lower lock authority",
            "Generated lock fragment states that the Drowned Chancellor held lower lock authority after the silt levy.",
            0.56,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::Received, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<std::string>{"site.reservoir_gate"}, std::optional<int>{claimed_year}},
        });
        return;
    }

    if (candidate.id == "candidate.moon_lock_fragment") {
        add_claim_to_archive(archive, artifact, Claim{
            "claim.materialized_moon_lock_authority",
            artifact.id,
            ClaimType::FactualClaim,
            "Drowned Chancellor",
            "received",
            "lower lock authority",
            "Moon-lock seal: the Drowned Chancellor holds lower lock authority after the silt levy.",
            0.57,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::Received, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<std::string>{"site.reservoir_gate"}, std::optional<int>{620}},
        });
        return;
    }

    if (candidate.id == "candidate.early_drowned_chancellor_forgery") {
        add_claim_to_archive(archive, artifact, Claim{
            "claim.materialized_forged_aru_chancellor",
            artifact.id,
            ClaimType::LegalFiction,
            "King Aru",
            "appointed",
            "Drowned Chancellor",
            "Aru appoints the Drowned Chancellor under the Moon-lock seal.",
            0.18,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::CreatedOffice, std::optional<std::string>{"person.aru"}, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<int>{553}},
        });
    }
}


void link_materialized_generated_candidate_to_mystery(ArchiveEngineState& state,
                                                      const CandidateFeature& candidate,
                                                      const std::string& artifact_id,
                                                      const std::vector<std::string>& claim_ids) {
    if (claim_ids.empty()) {
        return;
    }

    const std::vector<std::string> target_mystery_ids = candidate_proposed_ids_with_prefix(candidate, "mystery:");
    if (target_mystery_ids.empty()) {
        return;
    }

    MysteryEvidenceRole role = MysteryEvidenceRole::CoreClue;
    if (generated_candidate_is(candidate, "misleading_forgery") ||
        candidate_metadata_declares(candidate, EvidenceModifier::Forgery)) {
        role = MysteryEvidenceRole::MisleadingClue;
    } else if (generated_candidate_is(candidate, "ritual_variant") ||
               candidate_metadata_declares(candidate, EvidenceModifier::MythicCompression)) {
        role = MysteryEvidenceRole::CoreClue;
    } else if (generated_candidate_is(candidate, "corroborating_fragment")) {
        role = MysteryEvidenceRole::CoreClue;
    }

    for (const std::string& mystery_id : target_mystery_ids) {
        const auto mystery_it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
            return mystery.id == mystery_id;
        });
        if (mystery_it == state.mysteries.end()) {
            continue;
        }

        add_unique_string(mystery_it->clue_artifact_ids, artifact_id);
        if (role == MysteryEvidenceRole::MisleadingClue || role == MysteryEvidenceRole::FalseResolution) {
            add_unique_string(mystery_it->misleading_artifact_ids, artifact_id);
        }
        const bool already_linked = std::any_of(mystery_it->evidence_links.begin(), mystery_it->evidence_links.end(), [&](const MysteryEvidenceLink& link) {
            return link.artifact_id == artifact_id && link.claim_id.has_value() && *link.claim_id == claim_ids.front();
        });
        if (!already_linked) {
            mystery_it->evidence_links.push_back(MysteryEvidenceLink{artifact_id, std::optional<std::string>{claim_ids.front()}, role, 0.85});
        }
        if (Artifact* artifact = state.public_archive.find_artifact_mutable(artifact_id)) {
            add_unique_string(artifact->mystery_links, mystery_it->id);
        }
    }
}

[[nodiscard]] std::vector<std::string> contradiction_ids(const ArchiveEngineState& state) {
    std::vector<std::string> ids;
    for (const auto& [id, contradiction] : state.public_archive.contradictions()) {
        (void)contradiction;
        ids.push_back(id);
    }
    return ids;
}

[[nodiscard]] std::vector<std::string> newly_inserted_contradiction_ids(const std::vector<std::string>& before_ids,
                                                                        const ArchiveEngineState& state) {
    std::vector<std::string> inserted;
    for (const auto& [id, contradiction] : state.public_archive.contradictions()) {
        (void)contradiction;
        if (std::find(before_ids.begin(), before_ids.end(), id) == before_ids.end()) {
            inserted.push_back(id);
        }
    }
    return inserted;
}

MaterializationResult materialize_candidate_feature(ArchiveEngineState& state,
                                                    const CandidateFeature& candidate,
                                                    const CandidateEvaluation& evaluation,
                                                    AccessLevel access) {
    MaterializationResult result;

    if (!can_view(access, AccessLevel::Curator)) {
        result.explanation = "Materialization requires curator/canon/debug access; archive state was not mutated.";
        return result;
    }

    if (!evaluation.evaluated_candidate_id.empty() && evaluation.evaluated_candidate_id != candidate.id) {
        result.explanation = "Candidate evaluation does not match the candidate being materialized; archive state was not mutated.";
        return result;
    }

    const CandidateEvaluation fresh_evaluation = evaluate_candidate_feature(state, candidate, AccessLevel::Curator);
    if (fresh_evaluation.decision != evaluation.decision) {
        result.explanation = "Candidate evaluation is stale or no longer matches the current archive state; archive state was not mutated.";
        return result;
    }

    if (const std::optional<std::string> quality_error = candidate_materialization_quality_error(fresh_evaluation)) {
        result.explanation = *quality_error + "; archive state was not mutated.";
        return result;
    }

    if (!materialization_decision_is_insertable(fresh_evaluation.decision)) {
        result.explanation = "Candidate evaluation decision is not insertable; archive state was not mutated.";
        return result;
    }

    if (candidate.type != CandidateFeatureType::Artifact) {
        result.explanation = "This MVP materializes artifact candidates only; archive state was not mutated.";
        return result;
    }

    std::optional<Artifact> maybe_artifact = materialized_candidate_artifact(candidate, fresh_evaluation);
    if (!maybe_artifact.has_value()) {
        result.explanation = "Candidate lacks structured materialization metadata; archive state was not mutated.";
        return result;
    }

    const ArchiveEngineState snapshot = state;
    const std::vector<std::string> before_contradictions = contradiction_ids(state);

    try {
        Artifact artifact = *maybe_artifact;
        const std::string artifact_id = artifact.id;
        add_materialized_candidate_claims(state.public_archive, artifact, candidate);
        std::vector<std::string> claim_ids = artifact.claim_ids;
        state.public_archive.add_artifact(std::move(artifact));
        register_discovery_for_artifact(state, artifact_id);
        link_materialized_generated_candidate_to_mystery(state, candidate, artifact_id, claim_ids);
        add_detected_contradictions_to_archive(state);

        const std::vector<std::string> errors = validate_full_state(state);
        if (!errors.empty()) {
            state = snapshot;
            result.explanation = "Materialization rolled back because full validation failed: " + errors.front();
            return result;
        }

        result.decision = MaterializationDecision::InsertArtifact;
        result.mutated = true;
        result.inserted_artifact_ids.push_back(artifact_id);
        result.inserted_claim_ids = std::move(claim_ids);
        result.inserted_contradiction_ids = newly_inserted_contradiction_ids(before_contradictions, state);
        result.explanation = "Candidate materialized through curator-or-higher-approved insertion; archive validation passed after mutation.";
        return result;
    } catch (const std::exception& ex) {
        state = snapshot;
        result.explanation = std::string("Materialization rolled back after exception: ") + ex.what();
        return result;
    }
}



[[nodiscard]] HiddenMutationArtifactCandidateShape hidden_mutation_shape_from_candidate(const CandidateFeature& candidate) {
    if (contains_substr(candidate.id, "ritual_notice")) {
        return HiddenMutationArtifactCandidateShape::RitualNotice;
    }
    if (contains_substr(candidate.id, "scholar_fragment")) {
        return HiddenMutationArtifactCandidateShape::ScholarFragment;
    }
    return HiddenMutationArtifactCandidateShape::AdministrativeDocket;
}

[[nodiscard]] std::string hidden_mutation_shape_slug_for_materialization(HiddenMutationArtifactCandidateShape shape) {
    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket: return "admin_docket";
        case HiddenMutationArtifactCandidateShape::RitualNotice: return "ritual_notice";
        case HiddenMutationArtifactCandidateShape::ScholarFragment: return "scholar_fragment";
    }
    return "admin_docket";
}

[[nodiscard]] std::string hidden_mutation_shape_public_title_for_materialization(HiddenMutationArtifactCandidateShape shape) {
    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket: return "administrative docket";
        case HiddenMutationArtifactCandidateShape::RitualNotice: return "ritual notice";
        case HiddenMutationArtifactCandidateShape::ScholarFragment: return "later scholar fragment";
    }
    return "artifact candidate";
}

void specialize_hidden_mutation_artifact_public_surface(Artifact& artifact,
                                                        HiddenMutationArtifactCandidateShape shape) {
    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket:
            artifact.type = ArtifactType::TradeLedger;
            artifact.voice_register = ArtifactVoiceRegister::TradeLedger;
            artifact.title = "Materialized Lower-Lock Administrative Docket";
            artifact.material = "curator-or-higher-approved lockhouse docket shard";
            artifact.damage_profile = "tabulated lower-lock entries with controlled edge loss";
            artifact.creator_bias_profile = "bureaucratic narrow record";
            artifact.creator_motive = "preserve archive-visible lower-lock authority trace";
            artifact.intended_audience = "lockhouse auditors and later catalogers";
            artifact.distortion_profile = "hidden-mutation-derived public docket; restricted provenance";
            break;
        case HiddenMutationArtifactCandidateShape::RitualNotice:
            artifact.type = ArtifactType::Inscription;
            artifact.voice_register = ArtifactVoiceRegister::RoyalInscription;
            artifact.title = "Materialized Lockhouse Ritual Notice";
            artifact.material = "ceremonial notice tablet";
            artifact.damage_profile = "formulaic notice text with ritual omissions";
            artifact.creator_bias_profile = "ceremonial procedural language";
            artifact.creator_motive = "publicly acknowledge lockhouse observance without exposing restricted sources";
            artifact.intended_audience = "lockhouse participants and public witnesses";
            artifact.distortion_profile = "hidden-mutation-derived ritual notice; restricted provenance";
            break;
        case HiddenMutationArtifactCandidateShape::ScholarFragment:
            artifact.type = ArtifactType::DamagedManuscript;
            artifact.voice_register = ArtifactVoiceRegister::DamagedChronicle;
            artifact.title = "Materialized Later Scholar Fragment";
            artifact.material = "late catalog copy fragment";
            artifact.damage_profile = "copy excerpt with lacunae and catalog gloss";
            artifact.creator_bias_profile = "later scholarly reconstruction";
            artifact.creator_motive = "classify copied lower-lock evidence under public uncertainty";
            artifact.intended_audience = "archive catalogers and scholars";
            artifact.distortion_profile = "hidden-mutation-derived later copy; restricted provenance";
            break;
    }
    artifact.creator_id = "scribe.hidden_mutation_public_projection";
    artifact.attributed_creator_id = artifact.creator_id;
    artifact.creator_knowledge_scope = "public-safe projection from curator-or-higher-approved hidden mutation provenance";
    artifact.transmission_history = "explicit curator-or-higher-approved materialization of a hidden-mutation-derived candidate";
    artifact.scholarly_translation = "Materialized from a curator-or-higher-approved hidden-mutation-derived candidate; hidden provenance remains restricted below curator access.";
    artifact.generation_trace = "hidden_mutation_artifact_candidate_materialization=" + artifact.id + "; source=v26_explicit_curator_gate";
}

MaterializationResult materialize_hidden_mutation_artifact_candidate(ArchiveEngineState& state,
                                                                     const CandidateFeature& candidate,
                                                                     AccessLevel access) {
    MaterializationResult result;

    if (!can_view(access, AccessLevel::Curator)) {
        result.explanation = "Hidden-mutation artifact materialization requires curator/canon/debug access; archive state was not mutated.";
        return result;
    }

    if (!candidate.hidden_mutation_source.has_value()) {
        result.explanation = "Candidate is not linked to hidden mutation provenance; archive state was not mutated.";
        return result;
    }
    if (!candidate.structured_artifact_metadata.has_value() || candidate.structured_claims.empty()) {
        result.explanation = "Candidate lacks hidden-mutation materialization payload; archive state was not mutated.";
        return result;
    }

    const std::vector<std::string> source_errors = validate_hidden_mutation_artifact_source(state, *candidate.hidden_mutation_source);
    if (!source_errors.empty()) {
        result.explanation = "Hidden-mutation candidate source validation failed: " + source_errors.front();
        return result;
    }

    const CandidateEvaluation fresh_evaluation = evaluate_candidate_feature(state, candidate, AccessLevel::Curator);
    if (const std::optional<std::string> quality_error = candidate_materialization_quality_error(fresh_evaluation)) {
        result.explanation = *quality_error + "; archive state was not mutated.";
        return result;
    }

    if (!materialization_decision_is_insertable(fresh_evaluation.decision)) {
        result.explanation = "Fresh candidate evaluation is not insertable; archive state was not mutated.";
        return result;
    }
    if (candidate.type != CandidateFeatureType::Artifact) {
        result.explanation = "Hidden-mutation candidate materialization supports artifact candidates only; archive state was not mutated.";
        return result;
    }

    std::optional<Artifact> maybe_artifact = materialized_candidate_artifact(candidate, fresh_evaluation);
    if (!maybe_artifact.has_value()) {
        result.explanation = "Candidate lacks materializable structured artifact metadata; archive state was not mutated.";
        return result;
    }

    Artifact artifact = *maybe_artifact;
    const HiddenMutationArtifactCandidateShape shape = hidden_mutation_shape_from_candidate(candidate);
    specialize_hidden_mutation_artifact_public_surface(artifact, shape);
    artifact.hidden_event_links = candidate.hidden_mutation_source->source_event_ids;
    artifact.hidden_mutation_artifact_provenance = MaterializedHiddenMutationArtifactProvenance{
        candidate.id,
        candidate.hidden_mutation_source->mutation_record_id,
        candidate.hidden_mutation_source->source_cluster_id,
        candidate.hidden_mutation_source->source_entity_ids,
        candidate.hidden_mutation_source->source_event_ids,
        shape,
    };

    if (state.public_archive.find_artifact(artifact.id) != nullptr) {
        result.explanation = "Hidden-mutation artifact candidate was already materialized; duplicate insertion was rejected.";
        return result;
    }

    const ArchiveEngineState snapshot = state;
    const std::vector<std::string> before_contradictions = contradiction_ids(state);

    try {
        const std::string artifact_id = artifact.id;
        add_materialized_candidate_claims(state.public_archive, artifact, candidate);
        std::vector<std::string> claim_ids = artifact.claim_ids;
        state.public_archive.add_artifact(std::move(artifact));
        register_discovery_for_artifact(state, artifact_id);
        link_materialized_generated_candidate_to_mystery(state, candidate, artifact_id, claim_ids);
        add_detected_contradictions_to_archive(state);

        std::vector<std::string> errors = validate_full_state(state);
        if (const Artifact* inserted = state.public_archive.find_artifact(artifact_id)) {
            const std::vector<std::string> provenance_errors = validate_materialized_hidden_mutation_artifact_provenance(state, *inserted);
            errors.insert(errors.end(), provenance_errors.begin(), provenance_errors.end());
        } else {
            errors.push_back("materialized artifact disappeared before provenance validation");
        }

        if (!errors.empty()) {
            state = snapshot;
            result.explanation = "Hidden-mutation artifact materialization rolled back because validation failed: " + errors.front();
            return result;
        }

        result.decision = MaterializationDecision::InsertArtifact;
        result.mutated = true;
        result.inserted_artifact_ids.push_back(artifact_id);
        result.inserted_claim_ids = std::move(claim_ids);
        result.inserted_contradiction_ids = newly_inserted_contradiction_ids(before_contradictions, state);
        result.explanation = "Hidden-mutation artifact candidate materialized through curator/canon/debug gate; full archive and provenance validation passed.";
        return result;
    } catch (const std::exception& ex) {
        state = snapshot;
        result.explanation = std::string("Hidden-mutation artifact materialization rolled back after exception: ") + ex.what();
        return result;
    }
}

[[nodiscard]] std::string format_hidden_mutation_artifact_materialization_result(const MaterializationResult& result,
                                                                                 const CandidateFeature& candidate,
                                                                                 AccessLevel access) {
    std::ostringstream out;
    out << "Hidden-mutation artifact candidate materialization visible to " << to_string(access) << ":\n";
    if (can_view(access, AccessLevel::Curator)) {
        out << "- candidate_id: " << candidate.id << "\n";
    } else {
        out << "- candidate: mutation-derived public evidence candidate\n";
    }
    out << "- decision: " << to_string(result.decision) << "\n";
    out << "- mutated: " << (result.mutated ? "true" : "false") << "\n";
    const HiddenMutationCandidateSourceSummary summary = summarize_hidden_mutation_candidate_source(candidate, access);
    out << "- public origin: " << summary.public_origin_label << "\n";
    out << "- public effect: " << summary.public_effect_label << "\n";
    out << "- hidden provenance: " << (can_view(access, AccessLevel::Curator) ? "available below" : "restricted") << "\n";
    out << "- explanation: " << result.explanation << "\n";

    if (!result.inserted_artifact_ids.empty()) {
        out << "Materialized public artifact:\n";
        if (can_view(access, AccessLevel::Curator)) {
            for (const std::string& id : result.inserted_artifact_ids) {
                out << "- artifact: " << id << "\n";
            }
        } else {
            out << "- artifact: restricted identifier\n";
        }
    }
    if (!result.inserted_claim_ids.empty()) {
        out << "Materialized public claims:\n";
        if (can_view(access, AccessLevel::Curator)) {
            for (const std::string& id : result.inserted_claim_ids) {
                out << "- claim: " << id << "\n";
            }
        } else {
            out << "- claim identifiers restricted\n";
        }
    }

    if (can_view(access, AccessLevel::Curator) && candidate.hidden_mutation_source.has_value()) {
        const HiddenMutationArtifactSource& source = *candidate.hidden_mutation_source;
        out << "Hidden mutation artifact provenance:\n";
        out << "- candidate_id: " << candidate.id << "\n";
        out << "- mutation_record_id: " << source.mutation_record_id << "\n";
        out << "- source_cluster_id: " << source.source_cluster_id << "\n";
        out << "- source_entities:";
        for (const std::string& id : source.source_entity_ids) {
            out << " " << id;
        }
        out << "\n";
        out << "- source_events:";
        for (const std::string& id : source.source_event_ids) {
            out << " " << id;
        }
        out << "\n";
        out << "- source_shape: " << hidden_mutation_shape_slug_for_materialization(hidden_mutation_shape_from_candidate(candidate)) << "\n";
    }

    return out.str();
}


[[nodiscard]] std::string format_hidden_mutation_artifact_candidate_materialization_query(
    ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTimelineClusterRequest& cluster_request,
    const CandidateGenerationRequest& candidate_request,
    std::optional<HiddenMutationArtifactCandidateShape> shape,
    std::optional<std::size_t> index
) {
    std::ostringstream out;
    out << "Hidden-mutation artifact candidate materialization workflow visible to " << to_string(access) << ":\n";
    out << "- cluster_scope: " << to_string(cluster_request.scope) << "\n";
    out << "- target_topic: " << cluster_request.target_topic << "\n";
    out << "- year_window: " << cluster_request.start_year << "-" << cluster_request.end_year << "\n";

    const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
    const HiddenClusterMaterializationResult hidden_result = materialize_hidden_timeline_cluster(state, cluster, access);
    out << "- hidden_materialization_mutated: " << (hidden_result.mutated ? "true" : "false") << "\n";
    out << "- source_decision: " << to_string(hidden_result.source_decision) << "\n";

    if (!hidden_result.mutated || hidden_result.mutation_record_id.empty()) {
        out << "- candidate_materialization_mutated: false\n";
        out << "- explanation: hidden cluster was not materialized, so no public artifact candidate was materialized.\n";
        if (!can_view(access, AccessLevel::Curator)) {
            out << "- hidden mutation internals are restricted to curator/canon/debug access\n";
        } else if (!hidden_result.validation_errors.empty()) {
            out << "Hidden materialization errors:\n";
            for (const std::string& error : hidden_result.validation_errors) {
                out << "- " << error << "\n";
            }
        }
        return out.str();
    }

    const auto record_it = std::find_if(state.hidden_truth_mutations.begin(), state.hidden_truth_mutations.end(), [&](const HiddenTruthMutationRecord& record) {
        return record.id == hidden_result.mutation_record_id;
    });
    if (record_it == state.hidden_truth_mutations.end()) {
        out << "- candidate_materialization_mutated: false\n";
        out << "- explanation: hidden materialization did not leave an inspectable mutation record.\n";
        return out.str();
    }

    const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, *record_it, candidate_request);
    out << "- generated_candidate_count: " << batch.candidates.size() << "\n";
    if (batch.candidates.empty()) {
        out << "- candidate_materialization_mutated: false\n";
        out << "- explanation: no hidden-mutation-derived candidates were generated from the mutation record.\n";
        return out.str();
    }

    std::optional<CandidateFeature> selected;
    if (shape.has_value()) {
        const std::string wanted = hidden_mutation_shape_slug_for_materialization(*shape);
        const auto it = std::find_if(batch.candidates.begin(), batch.candidates.end(), [&](const CandidateFeature& candidate) {
            return contains_substr(candidate.id, wanted);
        });
        if (it != batch.candidates.end()) {
            selected = *it;
        }
    } else if (index.has_value() && *index < batch.candidates.size()) {
        selected = batch.candidates[*index];
    } else if (!index.has_value()) {
        selected = batch.candidates.front();
    }

    if (!selected.has_value()) {
        out << "- candidate_materialization_mutated: false\n";
        out << "- explanation: requested hidden-mutation candidate selection did not resolve to a generated candidate.\n";
        return out.str();
    }

    const CandidateEvaluation evaluation = evaluate_candidate_feature(state, *selected, access);
    out << "- selected_shape: " << hidden_mutation_shape_public_title_for_materialization(hidden_mutation_shape_from_candidate(*selected)) << "\n";
    out << "- fresh_evaluation_decision: " << display_candidate_decision(evaluation.decision, access) << "\n";

    const MaterializationResult result = materialize_hidden_mutation_artifact_candidate(state, *selected, access);
    out << "- candidate_materialization_mutated: " << (result.mutated ? "true" : "false") << "\n\n";
    out << format_hidden_mutation_artifact_materialization_result(result, *selected, access);
    return out.str();
}

[[nodiscard]] std::string format_materialization_result(const MaterializationResult& result,
                                                        const CandidateFeature& candidate,
                                                        AccessLevel access) {
    std::ostringstream out;
    out << "Candidate materialization visible to " << to_string(access) << ":\n";
    out << "- candidate: " << candidate.id << " [" << to_string(candidate.type) << "]\n";
    out << "- decision: " << to_string(result.decision) << "\n";
    out << "- mutated: " << (result.mutated ? "true" : "false") << "\n";
    out << "- explanation: " << result.explanation << "\n";
    if (!result.inserted_artifact_ids.empty()) {
        out << "Inserted artifacts:\n";
        if (can_view(access, AccessLevel::Curator)) {
            for (const std::string& id : result.inserted_artifact_ids) {
                out << "- " << id << "\n";
            }
        } else {
            out << "- restricted\n";
        }
    }
    if (!result.inserted_claim_ids.empty()) {
        out << "Inserted claims:\n";
        if (can_view(access, AccessLevel::Curator)) {
            for (const std::string& id : result.inserted_claim_ids) {
                out << "- " << id << "\n";
            }
        } else {
            out << "- restricted\n";
        }
    }
    if (!result.inserted_contradiction_ids.empty()) {
        if (can_view(access, AccessLevel::Curator)) {
            out << "Inserted contradictions:\n";
            for (const std::string& id : result.inserted_contradiction_ids) {
                out << "- " << id << "\n";
            }
        } else {
            out << "Inserted contradictions:\n- restricted\n";
        }
    }
    return out.str();
}



[[nodiscard]] GeneratedCandidateRole generated_candidate_role(const CandidateFeature& candidate) {
    if (contains_substr(candidate.id, "misleading_forgery")) {
        return GeneratedCandidateRole::MisleadingForgery;
    }
    if (contains_substr(candidate.id, "ritual_variant")) {
        return GeneratedCandidateRole::RitualVariant;
    }
    if (contains_substr(candidate.id, "corroborating_fragment")) {
        return GeneratedCandidateRole::CorroboratingFragment;
    }

    const bool has_forgery_mediation = candidate.structured_artifact_metadata.has_value() &&
        std::find(candidate.structured_artifact_metadata->declared_mediations.begin(),
                  candidate.structured_artifact_metadata->declared_mediations.end(),
                  EvidenceModifier::Forgery) != candidate.structured_artifact_metadata->declared_mediations.end();
    if (has_forgery_mediation) {
        return GeneratedCandidateRole::MisleadingForgery;
    }

    const bool has_mythic_claim = std::any_of(candidate.structured_claims.begin(), candidate.structured_claims.end(), [](const CandidateClaimMetadata& claim) {
        return claim.claim_type == ClaimType::MythicCompression || claim.predicate_type == PredicateType::Became;
    });
    if (has_mythic_claim) {
        return GeneratedCandidateRole::RitualVariant;
    }

    const bool has_factual_claim = std::any_of(candidate.structured_claims.begin(), candidate.structured_claims.end(), [](const CandidateClaimMetadata& claim) {
        return claim.claim_type == ClaimType::FactualClaim;
    });
    if (has_factual_claim) {
        return GeneratedCandidateRole::CorroboratingFragment;
    }

    return GeneratedCandidateRole::Unknown;
}

[[nodiscard]] std::string dossier_candidate_role(const CandidateFeature& candidate) {
    return to_string(generated_candidate_role(candidate));
}

[[nodiscard]] std::optional<std::size_t> generated_candidate_index_for_role(const ArchiveEngineState& state,
                                                                            const CandidateGenerationRequest& request,
                                                                            GeneratedCandidateRole role) {
    if (role == GeneratedCandidateRole::Unknown) {
        return std::nullopt;
    }
    const GeneratedCandidateBatch batch = generate_candidate_batch(state, request);
    for (std::size_t i = 0; i < batch.candidates.size(); ++i) {
        if (generated_candidate_role(batch.candidates[i]) == role) {
            return i;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::string dossier_recommendation_for_selection(const DossierEvaluation& evaluation,
                                                               std::string_view selected_role) {
    if (evaluation.candidate_evaluations.empty()) {
        return "No dossier candidates are available for materialization.";
    }
    if (selected_role == "corroborating_fragment" && evaluation.corroboration_pressure >= 0.80) {
        return "Warning: selected corroborating candidate comes from an over-confirming dossier; curator should consider pairing it with ambiguity or forgery-mediated evidence.";
    }
    if (selected_role == "corroborating_fragment" && evaluation.mystery_resolution_pressure >= 0.30) {
        return "Selected corroborating candidate may increase protected-mystery resolution pressure; curator should review whether ambiguity remains preserved.";
    }
    if (selected_role == "ritual_variant") {
        return "Selected ritual variant increases ambiguity and can help preserve interpretive instability around the dossier target.";
    }
    if (selected_role == "misleading_forgery") {
        return "Selected forgery-mediated candidate increases authenticity instability and should be materialized only when the declared mediation is curator-visible.";
    }
    return "Selected candidate should be materialized only after reviewing the dossier pressure summary and normal candidate evaluation.";
}

[[nodiscard]] DossierMaterializationPlan build_dossier_materialization_plan(const ArchiveEngineState& state,
                                                                            const CandidateGenerationRequest& request,
                                                                            std::size_t candidate_index,
                                                                            AccessLevel access) {
    DossierMaterializationPlan plan;
    plan.dossier_evaluation = evaluate_generated_dossier(state, request, access);
    plan.selected_candidate_indices.push_back(candidate_index);

    const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, candidate_index);
    if (!maybe_candidate.has_value()) {
        plan.selected_candidate_role = "unavailable";
        plan.recommendation = "Generated candidate index is out of range; archive state should not be mutated.";
        return plan;
    }

    plan.selected_candidate_id = maybe_candidate->id;
    plan.selected_candidate_role = dossier_candidate_role(*maybe_candidate);
    if (plan.selected_candidate_role == "corroborating_fragment") {
        plan.projected_corroboration_pressure = 1.0;
    } else if (plan.selected_candidate_role == "ritual_variant") {
        plan.projected_ambiguity_pressure = 1.0;
    } else if (plan.selected_candidate_role == "misleading_forgery") {
        plan.projected_forgery_pressure = 1.0;
    }
    plan.recommendation = dossier_recommendation_for_selection(plan.dossier_evaluation, plan.selected_candidate_role);
    return plan;
}

[[nodiscard]] std::string format_dossier_materialization_query(ArchiveEngineState& state,
                                                               AccessLevel access,
                                                               const CandidateGenerationRequest& request,
                                                               std::size_t candidate_index) {
    std::ostringstream out;
    out << "Dossier candidate materialization visible to " << to_string(access) << ":\n";
    out << "- strategy: " << to_string(request.strategy) << "\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- target_year: " << request.target_year << "\n";
    out << "- candidate_index: " << candidate_index << "\n";

    const std::string before = serialize_for_replay_test(state);
    const DossierMaterializationPlan plan = build_dossier_materialization_plan(state, request, candidate_index, access);
    const std::string after_plan = serialize_for_replay_test(state);

    if (!plan.dossier_evaluation.resolved_target.has_value()) {
        out << "- target_resolution: unresolved\n";
        out << "- archive_mutated_by_plan: " << (before == after_plan ? "false" : "true") << "\n";
        out << "- decision: Reject\n";
        out << "- mutated: false\n";
        out << "- explanation: No dossier candidate could be materialized because the target topic did not resolve.\n";
        return out.str();
    }

    out << "- target_resolution: " << plan.dossier_evaluation.resolved_target->topic;
    if (plan.dossier_evaluation.resolved_target->mystery_id.has_value() && can_view(access, AccessLevel::Curator)) {
        out << " -> " << *plan.dossier_evaluation.resolved_target->mystery_id;
    }
    out << "\n";
    out << "- generated_count: " << plan.dossier_evaluation.candidate_evaluations.size() << "\n";
    out << "- archive_mutated_by_plan: " << (before == after_plan ? "false" : "true") << "\n";

    if (can_view(access, AccessLevel::Curator)) {
        out << "Dossier pressure summary before materialization:\n";
        out << "- assessment: " << plan.dossier_evaluation.assessment << "\n";
        out << std::fixed << std::setprecision(2);
        out << "- corroboration_pressure=" << plan.dossier_evaluation.corroboration_pressure << "\n";
        out << "- ambiguity_pressure=" << plan.dossier_evaluation.ambiguity_pressure << "\n";
        out << "- forgery_pressure=" << plan.dossier_evaluation.forgery_pressure << "\n";
        out << "- mystery_resolution_pressure=" << plan.dossier_evaluation.mystery_resolution_pressure << "\n";
        out << "Selected candidate plan:\n";
        out << "- selected_candidate_id: " << (plan.selected_candidate_id.empty() ? "unavailable" : plan.selected_candidate_id) << "\n";
        out << "- selected_candidate_role: " << plan.selected_candidate_role << "\n";
        out << "- projected_corroboration_pressure=" << plan.projected_corroboration_pressure << "\n";
        out << "- projected_ambiguity_pressure=" << plan.projected_ambiguity_pressure << "\n";
        out << "- projected_forgery_pressure=" << plan.projected_forgery_pressure << "\n";
        out << "- recommendation: " << plan.recommendation << "\n";
    } else {
        out << "- dossier materialization audit internals are restricted to curator/canon/debug access\n";
    }

    std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, candidate_index);
    if (!maybe_candidate.has_value()) {
        out << "- decision: Reject\n";
        out << "- mutated: false\n";
        out << "- explanation: Generated candidate index is out of range; archive state was not mutated.\n";
        return out.str();
    }

    const CandidateFeature candidate = *maybe_candidate;
    const CandidateEvaluation evaluation = evaluate_candidate_feature(state, candidate, access);
    const MaterializationResult result = materialize_candidate_feature(state, candidate, evaluation, access);
    out << format_materialization_result(result, candidate, access);
    return out.str();
}

[[nodiscard]] std::string format_dossier_materialization_query_by_role(ArchiveEngineState& state,
                                                                       AccessLevel access,
                                                                       const CandidateGenerationRequest& request,
                                                                       GeneratedCandidateRole role) {
    std::ostringstream out;
    out << "Dossier role-based candidate materialization visible to " << to_string(access) << ":\n";
    out << "- strategy: " << to_string(request.strategy) << "\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- target_year: " << request.target_year << "\n";
    out << "- candidate_role: " << to_string(role) << "\n";

    const std::optional<std::size_t> index = generated_candidate_index_for_role(state, request, role);
    if (!index.has_value()) {
        out << "- decision: Reject\n";
        out << "- mutated: false\n";
        out << "- explanation: No generated dossier candidate matched the requested role; archive state was not mutated.\n";
        return out.str();
    }

    out << "- resolved_candidate_index: " << *index << "\n\n";
    out << format_dossier_materialization_query(state, access, request, *index);
    return out.str();
}

[[nodiscard]] DossierSelectionPlan build_dossier_selection_plan(const ArchiveEngineState& state,
                                                                const CandidateGenerationRequest& request,
                                                                const std::vector<std::size_t>& candidate_indices,
                                                                AccessLevel access) {
    DossierSelectionPlan plan;
    plan.dossier_evaluation = evaluate_generated_dossier(state, request, access);
    plan.requested_candidate_indices = candidate_indices;

    std::set<std::size_t> seen;
    std::size_t selected_corrob = 0U;
    std::size_t selected_ritual = 0U;
    std::size_t selected_forgery = 0U;
    for (const std::size_t index : candidate_indices) {
        if (!seen.insert(index).second) {
            plan.duplicate_selections_ignored = true;
            continue;
        }
        plan.selected_candidate_indices.push_back(index);
        const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, index);
        if (!maybe_candidate.has_value()) {
            plan.selected_candidate_ids.push_back("unavailable");
            plan.selected_candidate_roles.push_back("unavailable");
            continue;
        }
        const std::string role = dossier_candidate_role(*maybe_candidate);
        plan.selected_candidate_ids.push_back(maybe_candidate->id);
        plan.selected_candidate_roles.push_back(role);
        if (role == "corroborating_fragment") {
            ++selected_corrob;
        } else if (role == "ritual_variant") {
            ++selected_ritual;
        } else if (role == "misleading_forgery") {
            ++selected_forgery;
        }
    }

    const std::size_t selected_total = plan.selected_candidate_ids.size();
    plan.projected_corroboration_pressure = selected_total == 0U ? 0.0 : static_cast<double>(selected_corrob) / static_cast<double>(selected_total);
    plan.projected_ambiguity_pressure = selected_total == 0U ? 0.0 : static_cast<double>(selected_ritual) / static_cast<double>(selected_total);
    plan.projected_forgery_pressure = selected_total == 0U ? 0.0 : static_cast<double>(selected_forgery) / static_cast<double>(selected_total);

    if (selected_total == 0U) {
        plan.recommendation = "No dossier candidates were selected; curator should choose one or more explicit indices or roles.";
    } else if (plan.projected_corroboration_pressure >= 0.80 && plan.dossier_evaluation.corroboration_pressure >= 0.80) {
        plan.recommendation = "Selection plan over-emphasizes corroboration in an already over-confirming dossier; add ambiguity or mediation before materializing as a batch.";
    } else if (plan.projected_ambiguity_pressure > 0.0 && plan.projected_corroboration_pressure > 0.0 && plan.projected_forgery_pressure == 0.0) {
        plan.recommendation = "Selection pairs corroboration with ritual ambiguity and avoids adding forgery pressure.";
    } else if (plan.projected_forgery_pressure > 0.0) {
        plan.recommendation = "Selection includes forgery-mediated material; curator should verify declared mediation and public catalog redaction before materialization.";
    } else {
        plan.recommendation = "Selection is explicit and can be materialized only through the curator/canon/debug validation boundary.";
    }
    return plan;
}

[[nodiscard]] std::string format_selected_indices(const std::vector<std::size_t>& indices) {
    if (indices.empty()) {
        return "none";
    }
    std::ostringstream out;
    for (std::size_t i = 0; i < indices.size(); ++i) {
        if (i != 0U) {
            out << ",";
        }
        out << indices[i];
    }
    return out.str();
}

[[nodiscard]] std::string format_dossier_selection_plan(const ArchiveEngineState& state,
                                                        AccessLevel access,
                                                        const CandidateGenerationRequest& request,
                                                        const std::vector<std::size_t>& candidate_indices) {
    std::ostringstream out;
    out << "Dossier selection plan visible to " << to_string(access) << ":\n";
    out << "- strategy: " << to_string(request.strategy) << "\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- target_year: " << request.target_year << "\n";
    out << "- requested_indices: " << format_selected_indices(candidate_indices) << "\n";

    const std::string before = serialize_for_replay_test(state);
    const DossierSelectionPlan plan = build_dossier_selection_plan(state, request, candidate_indices, access);
    const std::string after = serialize_for_replay_test(state);

    if (!plan.dossier_evaluation.resolved_target.has_value()) {
        out << "- target_resolution: unresolved\n";
        out << "- archive_mutated_by_plan: " << (before == after ? "false" : "true") << "\n";
        out << "- assessment: No dossier selection plan could be built because the target topic did not resolve.\n";
        return out.str();
    }

    out << "- target_resolution: " << plan.dossier_evaluation.resolved_target->topic;
    if (plan.dossier_evaluation.resolved_target->mystery_id.has_value() && can_view(access, AccessLevel::Curator)) {
        out << " -> " << *plan.dossier_evaluation.resolved_target->mystery_id;
    }
    out << "\n";
    out << "- generated_count: " << plan.dossier_evaluation.candidate_evaluations.size() << "\n";
    out << "- selected_indices: " << format_selected_indices(plan.selected_candidate_indices) << "\n";
    if (plan.duplicate_selections_ignored) {
        out << "- note: duplicate selected indices were ignored\n";
    }
    out << "- archive_mutated_by_plan: " << (before == after ? "false" : "true") << "\n";

    if (!can_view(access, AccessLevel::Curator)) {
        out << "- dossier selection internals are restricted to curator/canon/debug access\n";
        return out.str();
    }

    out << std::fixed << std::setprecision(2);
    out << "Dossier pressure summary:\n";
    out << "- assessment: " << plan.dossier_evaluation.assessment << "\n";
    out << "- corroboration_pressure=" << plan.dossier_evaluation.corroboration_pressure << "\n";
    out << "- ambiguity_pressure=" << plan.dossier_evaluation.ambiguity_pressure << "\n";
    out << "- forgery_pressure=" << plan.dossier_evaluation.forgery_pressure << "\n";
    out << "Selected candidates:\n";
    for (std::size_t i = 0; i < plan.selected_candidate_indices.size(); ++i) {
        const std::string id = i < plan.selected_candidate_ids.size() ? plan.selected_candidate_ids[i] : std::string{"unavailable"};
        const std::string role = i < plan.selected_candidate_roles.size() ? plan.selected_candidate_roles[i] : std::string{"unavailable"};
        out << "- index=" << plan.selected_candidate_indices[i] << ", id=" << id << ", role=" << role << "\n";
    }
    out << "Projected selected-candidate pressure:\n";
    out << "- projected_corroboration_pressure=" << plan.projected_corroboration_pressure << "\n";
    out << "- projected_ambiguity_pressure=" << plan.projected_ambiguity_pressure << "\n";
    out << "- projected_forgery_pressure=" << plan.projected_forgery_pressure << "\n";
    out << "- recommendation: " << plan.recommendation << "\n";
    return out.str();
}

[[nodiscard]] std::string format_dossier_selection_materialization_query(ArchiveEngineState& state,
                                                                         AccessLevel access,
                                                                         const CandidateGenerationRequest& request,
                                                                         const std::vector<std::size_t>& candidate_indices) {
    std::ostringstream out;
    out << "Dossier selection materialization visible to " << to_string(access) << ":\n";
    out << "- strategy: " << to_string(request.strategy) << "\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- target_year: " << request.target_year << "\n";
    out << "- requested_indices: " << format_selected_indices(candidate_indices) << "\n";

    const DossierSelectionPlan plan = build_dossier_selection_plan(state, request, candidate_indices, access);
    if (!plan.dossier_evaluation.resolved_target.has_value()) {
        out << "- decision: Reject\n";
        out << "- mutated: false\n";
        out << "- explanation: No dossier selection could be materialized because the target topic did not resolve.\n";
        return out.str();
    }

    if (!can_view(access, AccessLevel::Curator)) {
        out << "- dossier selection internals are restricted to curator/canon/debug access\n";
        out << "- decision: Reject\n";
        out << "- mutated: false\n";
        out << "- explanation: Materialization requires curator/canon/debug access; archive state was not mutated.\n";
        return out.str();
    }

    out << "- selected_indices: " << format_selected_indices(plan.selected_candidate_indices) << "\n";
    if (plan.duplicate_selections_ignored) {
        out << "- note: duplicate selected indices were ignored\n";
    }

    out << std::fixed << std::setprecision(2);
    out << "Dossier pressure summary before batch materialization:\n";
    out << "- assessment: " << plan.dossier_evaluation.assessment << "\n";
    out << "- corroboration_pressure=" << plan.dossier_evaluation.corroboration_pressure << "\n";
    out << "- ambiguity_pressure=" << plan.dossier_evaluation.ambiguity_pressure << "\n";
    out << "- forgery_pressure=" << plan.dossier_evaluation.forgery_pressure << "\n";
    out << "- recommendation: " << plan.recommendation << "\n";

    if (plan.selected_candidate_indices.empty()) {
        out << "- decision: Reject\n";
        out << "- mutated: false\n";
        out << "- explanation: No dossier candidates were selected; archive state was not mutated.\n";
        return out.str();
    }

    ArchiveEngineState snapshot = state;
    std::vector<std::string> aggregate_artifacts;
    std::vector<std::string> aggregate_claims;
    std::vector<std::string> aggregate_contradictions;

    for (std::size_t i = 0; i < plan.selected_candidate_indices.size(); ++i) {
        const std::size_t index = plan.selected_candidate_indices[i];
        const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, index);
        if (!maybe_candidate.has_value()) {
            state = snapshot;
            out << "- decision: Reject\n";
            out << "- mutated: false\n";
            out << "- rollback: true\n";
            out << "- explanation: Selected generated candidate index " << index << " is out of range; entire selected batch was rolled back.\n";
            return out.str();
        }
        const CandidateFeature candidate = *maybe_candidate;
        const CandidateEvaluation evaluation = evaluate_candidate_feature(state, candidate, access);
        const MaterializationResult result = materialize_candidate_feature(state, candidate, evaluation, access);
        if (!result.mutated) {
            state = snapshot;
            out << "- decision: Reject\n";
            out << "- mutated: false\n";
            out << "- rollback: true\n";
            out << "- failed_candidate_index: " << index << "\n";
            out << "- failed_candidate_id: " << candidate.id << "\n";
            out << "- explanation: " << result.explanation << " Entire selected batch was rolled back.\n";
            return out.str();
        }
        aggregate_artifacts.insert(aggregate_artifacts.end(), result.inserted_artifact_ids.begin(), result.inserted_artifact_ids.end());
        aggregate_claims.insert(aggregate_claims.end(), result.inserted_claim_ids.begin(), result.inserted_claim_ids.end());
        aggregate_contradictions.insert(aggregate_contradictions.end(), result.inserted_contradiction_ids.begin(), result.inserted_contradiction_ids.end());
    }

    const std::vector<std::string> final_errors = validate_full_state(state);
    if (!final_errors.empty()) {
        state = snapshot;
        out << "- decision: Reject\n";
        out << "- mutated: false\n";
        out << "- rollback: true\n";
        out << "- explanation: Final post-batch archive validation failed; entire selected batch was rolled back.\n";
        out << "Validation errors:\n";
        for (const std::string& error : final_errors) {
            out << "- " << error << "\n";
        }
        return out.str();
    }

    out << "- decision: InsertArtifact\n";
    out << "- mutated: true\n";
    out << "- explanation: Selected dossier candidates materialized as an explicit curator-or-higher-approved batch; archive validation passed after batch insertion.\n";
    out << "Inserted artifacts:\n";
    for (const std::string& id : aggregate_artifacts) {
        out << "- " << id << "\n";
    }
    out << "Inserted claims:\n";
    for (const std::string& id : aggregate_claims) {
        out << "- " << id << "\n";
    }
    if (!aggregate_contradictions.empty()) {
        out << "Inserted contradictions:\n";
        for (const std::string& id : aggregate_contradictions) {
            out << "- " << id << "\n";
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_materialization_query(ArchiveEngineState& state,
                                                       AccessLevel access,
                                                       std::string_view candidate_id) {
    const CandidateFeature candidate = sample_candidate_feature(candidate_id);
    const CandidateEvaluation evaluation = evaluate_candidate_feature(state, candidate, access);
    const MaterializationResult result = materialize_candidate_feature(state, candidate, evaluation, access);
    return format_materialization_result(result, candidate, access);
}

[[nodiscard]] std::string format_generated_materialization_query(ArchiveEngineState& state,
                                                                AccessLevel access,
                                                                const CandidateGenerationRequest& request,
                                                                std::size_t candidate_index) {
    std::ostringstream out;
    out << "Generated candidate materialization visible to " << to_string(access) << ":\n";
    out << "- strategy: " << to_string(request.strategy) << "\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- target_year: " << request.target_year << "\n";
    out << "- candidate_index: " << candidate_index << "\n";

    std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, candidate_index);
    if (!maybe_candidate.has_value()) {
        out << "- decision: Reject\n";
        out << "- mutated: false\n";
        out << "- explanation: Generated candidate index is out of range; archive state was not mutated.\n";
        return out.str();
    }

    const CandidateFeature candidate = *maybe_candidate;
    const CandidateEvaluation evaluation = evaluate_candidate_feature(state, candidate, access);
    MaterializationResult result = materialize_candidate_feature(state, candidate, evaluation, access);
    out << format_materialization_result(result, candidate, access);
    return out.str();
}

} // namespace archive
