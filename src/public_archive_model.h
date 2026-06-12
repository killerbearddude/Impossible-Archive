#pragma once
#include "archive_common.h"
#include "hidden_truth_model.h"

namespace archive {

enum class ArtifactType {
    Inscription,
    DamagedManuscript,
    ForgedDecree,
    OralHistory,
    TradeLedger,
};

enum class ArtifactVoiceRegister {
    RoyalInscription,
    DamagedChronicle,
    TradeLedger,
    OralSong,
    DisputedDecree,
};

struct ArtifactVoiceProfile {
    ArtifactVoiceRegister register_type = ArtifactVoiceRegister::RoyalInscription;
    std::vector<std::string> formulae;
    std::vector<std::string> damage_markers;
    std::vector<std::string> translation_notes;
    bool uses_catalog_note = false;
};

enum class ArtifactVoiceClaimRole {
    PrimaryLine,
    SecondaryLine,
    CatalogNote,
    TranslationNote,
    OmittedFromPublicRendering,
};

struct ArtifactVoiceClaimLink {
    std::string claim_id;
    ArtifactVoiceClaimRole role = ArtifactVoiceClaimRole::PrimaryLine;
    double weight = 1.0;
};

enum class ClaimType {
    FactualClaim,
    LegalFiction,
    MythicCompression,
    PropagandaClaim,
    TranslationGuess,
    NegativeEvidence,
};

enum class PredicateType {
    Restored,
    Caused,
    Preceded,
    Appointed,
    Received,
    Became,
    ExistedInYear,
    CreatedOffice,
    UsesScript,
    LocatedAt,
};

struct ClaimSemantics {
    PredicateType predicate_type = PredicateType::ExistedInYear;
    std::optional<std::string> subject_entity_id;
    std::optional<std::string> object_entity_id;
    std::optional<int> claimed_year;
};

enum class ContradictionType {
    DateContradiction,
    PersonContradiction,
    TitleContradiction,
    LanguageContradiction,
    RitualContradiction,
};

enum class ContradictionCause {
    None,
    Forgery,
    Damage,
    CalendarConversionError,
    Propaganda,
    MythologizedMemory,
    RitualAnachronism,
    UnresolvedGenerationBug,
};

enum class AnachronismStatus {
    Valid,
    ValidBecauseForged,
    ValidBecauseLaterCopy,
    ValidBecauseRitual,
    InvalidGenerationBug,
};

enum class EvidenceModifier {
    Forgery,
    LaterCopy,
    Interpolation,
    RitualAnachronism,
    Mistranslation,
    CalendarError,
    Propaganda,
    Damage,
    MisdatedStratum,
    MythicCompression,
    NarrowScope,
};


struct Claim {
    std::string id;
    std::string source_artifact_id;
    ClaimType type = ClaimType::FactualClaim;
    std::string subject;
    std::string predicate;
    std::string object;
    std::string literal_content;
    double confidence = 0.0;
    AccessLevel min_access = AccessLevel::Public;
    std::optional<ClaimSemantics> semantics;
};

struct ReliabilityComponents {
    double provenance_confidence = 0.0;
    double preservation_integrity = 0.0;
    double temporal_proximity = 0.0;
    double creator_access_to_events = 0.0;
    double bias_penalty = 0.0;
    double forgery_penalty = 0.0;
    double translation_confidence = 0.0;
    double external_corroboration = 0.0;
    double contradiction_penalty = 0.0;
};

// Provenance for public artifacts explicitly materialized from v25.1
// hidden-mutation-derived candidates. This is internal audit metadata and must
// be redacted from public/scholar formatting.
struct MaterializedHiddenMutationArtifactProvenance {
    std::string candidate_id;
    std::string mutation_record_id;
    std::string source_cluster_id;
    std::vector<std::string> source_entity_ids;
    std::vector<std::string> source_event_ids;
    HiddenMutationArtifactCandidateShape source_shape = HiddenMutationArtifactCandidateShape::AdministrativeDocket;
};

// Public evidence object. Artifacts are historically situated projections of
// hidden truth or mediated distortions of it. Required metadata is validated
// before artifacts participate in answers, theories, mysteries, or generation.
struct Artifact {
    std::string id;
    ArtifactType type = ArtifactType::Inscription;
    ArtifactVoiceRegister voice_register = ArtifactVoiceRegister::RoyalInscription;
    std::string title;
    std::string creator_id;
    std::string attributed_creator_id;
    int true_creation_year = 0;
    int claimed_creation_year = 0;
    int discovery_year = 0;
    std::string location_created;
    std::string location_found;
    std::string language_id;
    std::string dialect_id;
    std::string script_id;
    std::string material;
    double preservation_quality = 0.0;
    std::string damage_profile;
    std::string transmission_history;
    std::string creator_knowledge_scope;
    std::string creator_bias_profile;
    std::string creator_motive;
    std::string intended_audience;
    std::string public_text;
    std::string literal_translation;
    std::string scholarly_translation;
    std::vector<std::string> claim_ids;
    std::vector<ArtifactVoiceClaimLink> voice_claim_links;
    std::vector<std::string> hidden_event_links;
    std::vector<std::string> referenced_entity_ids;
    std::string distortion_profile;
    std::vector<EvidenceModifier> evidence_modifiers;
    ReliabilityComponents reliability_components;
    double reliability_score = 0.0;
    std::vector<std::string> contradiction_ids;
    std::vector<std::string> mystery_links;
    AccessLevel min_access = AccessLevel::Public;
    std::optional<MaterializedHiddenMutationArtifactProvenance> hidden_mutation_artifact_provenance;
    std::string generation_trace;
};

// Public contradiction record. Contradictions are allowed only when the engine
// can assign a historical/evidentiary cause or restrict the problem as a debug
// generation issue. Artifact links are kept bidirectional by PublicArchive.
struct Contradiction {
    std::string id;
    std::string detector_rule;
    std::vector<std::string> involved_claim_ids;
    std::vector<std::string> involved_artifact_ids;
    ContradictionType type = ContradictionType::DateContradiction;
    ContradictionCause assigned_cause = ContradictionCause::None;
    AccessLevel cause_min_access = AccessLevel::Scholar;
    std::string hidden_truth_resolution;
    AccessLevel hidden_resolution_min_access = AccessLevel::Canon;
    std::string public_resolution_status;
    int detected_year = 0;
};

struct AnachronismReport {
    std::string artifact_id;
    std::string referenced_item;
    std::string checked_year_kind;
    int checked_year = 0;
    int availability_start_year = 0;
    int availability_end_year = 0;
    std::string severity;
    std::string allowed_explanation;
    AnachronismStatus status = AnachronismStatus::Valid;
};

// Canonical hidden graph. This stores the causal model used for validation and


[[nodiscard]] bool has_any_evidence_modifier(const Artifact& artifact, std::initializer_list<EvidenceModifier> causes);


class PublicArchive {
public:
    void add_artifact(Artifact artifact) {
        const std::string id = artifact.id;
        auto [it, inserted] = artifacts_.emplace(id, std::move(artifact));
        (void)it;
        if (!inserted) {
            throw std::runtime_error("duplicate artifact id: " + id);
        }
    }

    // Low-level import/bootstrap insertion only. Prefer add_claim_to_artifact()
    // for ordinary archive mutation so PublicArchive owns the
    // claim/artifact/voice-link relationship invariant.
    void add_claim(Claim claim) {
        const std::string id = claim.id;
        auto [it, inserted] = claims_.emplace(id, std::move(claim));
        (void)it;
        if (!inserted) {
            throw std::runtime_error("duplicate claim id: " + id);
        }
    }

    void add_claim_to_artifact(std::string_view artifact_id,
                               Claim claim,
                               ArtifactVoiceClaimRole voice_role = ArtifactVoiceClaimRole::PrimaryLine,
                               double voice_weight = 1.0) {
        const std::string target_artifact_id{artifact_id};
        Artifact* artifact = find_artifact_mutable(target_artifact_id);
        if (artifact == nullptr) {
            throw std::runtime_error("cannot add claim to missing artifact: " + target_artifact_id);
        }

        const std::string claim_id = claim.id;
        if (claim_id.empty()) {
            throw std::runtime_error("cannot add claim with empty id to artifact: " + target_artifact_id);
        }
        if (claim.source_artifact_id != target_artifact_id) {
            throw std::runtime_error("claim " + claim_id + " source_artifact_id " + claim.source_artifact_id +
                                     " does not match target artifact " + target_artifact_id);
        }
        if (find_claim(claim_id) != nullptr) {
            throw std::runtime_error("duplicate claim id: " + claim_id);
        }
        if (std::find(artifact->claim_ids.begin(), artifact->claim_ids.end(), claim_id) != artifact->claim_ids.end()) {
            throw std::runtime_error("artifact " + target_artifact_id + " already links claim " + claim_id);
        }

        const bool add_voice_link = voice_weight > 0.0;
        const std::size_t old_claim_link_count = artifact->claim_ids.size();
        const std::size_t old_voice_link_count = artifact->voice_claim_links.size();

        artifact->claim_ids.reserve(old_claim_link_count + 1);
        if (add_voice_link) {
            artifact->voice_claim_links.reserve(old_voice_link_count + 1);
        }

        auto [it, inserted] = claims_.emplace(claim_id, std::move(claim));
        if (!inserted) {
            throw std::runtime_error("duplicate claim id: " + claim_id);
        }

        try {
            artifact->claim_ids.push_back(claim_id);
            if (add_voice_link) {
                artifact->voice_claim_links.push_back(ArtifactVoiceClaimLink{claim_id, voice_role, voice_weight});
            }
        } catch (...) {
            artifact->claim_ids.resize(old_claim_link_count);
            artifact->voice_claim_links.resize(old_voice_link_count);
            claims_.erase(it);
            throw;
        }
    }

    void add_contradiction_link_to_artifact(const std::string& artifact_id, const std::string& contradiction_id) {
        Artifact* artifact = find_artifact_mutable(artifact_id);
        if (artifact == nullptr) {
            return;
        }
        if (std::find(artifact->contradiction_ids.begin(), artifact->contradiction_ids.end(), contradiction_id) == artifact->contradiction_ids.end()) {
            artifact->contradiction_ids.push_back(contradiction_id);
        }
    }

    void add_contradiction(Contradiction contradiction) {
        const std::string id = contradiction.id;
        auto [it, inserted] = contradictions_.emplace(id, std::move(contradiction));
        if (!inserted) {
            throw std::runtime_error("duplicate contradiction id: " + id);
        }

        for (const std::string& artifact_id : it->second.involved_artifact_ids) {
            add_contradiction_link_to_artifact(artifact_id, id);
        }
    }

    [[nodiscard]] Artifact* find_artifact_mutable(const std::string& id) {
        const auto it = artifacts_.find(id);
        if (it == artifacts_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] const Artifact* find_artifact(const std::string& id) const {
        const auto it = artifacts_.find(id);
        if (it == artifacts_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] const Claim* find_claim(const std::string& id) const {
        const auto it = claims_.find(id);
        if (it == claims_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] Claim* find_claim_mutable(const std::string& id) {
        const auto it = claims_.find(id);
        if (it == claims_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    bool remove_claim(const std::string& id) {
        const auto it = claims_.find(id);
        if (it == claims_.end()) {
            return false;
        }

        Artifact* source = find_artifact_mutable(it->second.source_artifact_id);
        if (source != nullptr) {
            source->claim_ids.erase(
                std::remove(source->claim_ids.begin(), source->claim_ids.end(), id),
                source->claim_ids.end()
            );
            source->voice_claim_links.erase(
                std::remove_if(source->voice_claim_links.begin(), source->voice_claim_links.end(), [&](const ArtifactVoiceClaimLink& link) {
                    return link.claim_id == id;
                }),
                source->voice_claim_links.end()
            );
        }

        std::vector<std::string> contradictions_to_remove;
        for (const auto& [contradiction_id, contradiction] : contradictions_) {
            if (std::find(contradiction.involved_claim_ids.begin(), contradiction.involved_claim_ids.end(), id) != contradiction.involved_claim_ids.end()) {
                contradictions_to_remove.push_back(contradiction_id);
            }
        }

        for (const std::string& contradiction_id : contradictions_to_remove) {
            const auto contradiction_it = contradictions_.find(contradiction_id);
            if (contradiction_it != contradictions_.end()) {
                for (const std::string& artifact_id : contradiction_it->second.involved_artifact_ids) {
                    Artifact* artifact = find_artifact_mutable(artifact_id);
                    if (artifact != nullptr) {
                        artifact->contradiction_ids.erase(
                            std::remove(artifact->contradiction_ids.begin(), artifact->contradiction_ids.end(), contradiction_id),
                            artifact->contradiction_ids.end()
                        );
                    }
                }
                contradictions_.erase(contradiction_it);
            }
        }

        claims_.erase(it);
        return true;
    }

    [[nodiscard]] const Contradiction* find_contradiction(const std::string& id) const {
        const auto it = contradictions_.find(id);
        if (it == contradictions_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] const std::map<std::string, Artifact>& artifacts() const {
        return artifacts_;
    }

    [[nodiscard]] const std::map<std::string, Claim>& claims() const {
        return claims_;
    }

    [[nodiscard]] const std::map<std::string, Contradiction>& contradictions() const {
        return contradictions_;
    }

    [[nodiscard]] std::vector<std::string> validate_metadata() const {
        std::vector<std::string> errors;

        for (const auto& artifact_entry : artifacts_) {
            const std::string& artifact_id = artifact_entry.first;
            const Artifact& artifact = artifact_entry.second;
            auto require = [&](const std::string& value, std::string_view field) {
                if (value.empty()) {
                    errors.push_back("artifact " + artifact_id + " missing " + std::string(field));
                }
            };

            require(artifact.id, "id");
            require(artifact.title, "title");
            require(artifact.creator_id, "creator_id");
            require(artifact.attributed_creator_id, "attributed_creator_id");
            require(artifact.location_created, "location_created");
            require(artifact.location_found, "location_found");
            require(artifact.language_id, "language_id");
            require(artifact.dialect_id, "dialect_id");
            require(artifact.script_id, "script_id");
            require(artifact.creator_bias_profile, "creator_bias_profile");
            require(artifact.distortion_profile, "distortion_profile");
            require(artifact.generation_trace, "generation_trace");
            if (artifact.evidence_modifiers.empty()) {
                errors.push_back("artifact " + artifact_id + " has no structured evidence modifiers");
            }

            if (artifact.true_creation_year == 0 || artifact.claimed_creation_year == 0 || artifact.discovery_year == 0) {
                errors.push_back("artifact " + artifact_id + " has an unset required date");
            }

            if (artifact.true_creation_year > artifact.discovery_year) {
                errors.push_back("artifact " + artifact_id + " has true creation year after discovery year");
            }

            if (artifact.claimed_creation_year != artifact.true_creation_year &&
                !has_any_evidence_modifier(artifact, {EvidenceModifier::Forgery, EvidenceModifier::LaterCopy, EvidenceModifier::Interpolation, EvidenceModifier::MisdatedStratum})) {
                errors.push_back("artifact " + artifact_id + " has mismatched claimed/true dates without structured date-mediation cause");
            }

            if (artifact.reliability_score < 0.0 || artifact.reliability_score > 1.0) {
                errors.push_back("artifact " + artifact_id + " has invalid reliability score");
            }

            if (artifact.hidden_event_links.empty()) {
                errors.push_back("artifact " + artifact_id + " has no hidden event link");
            }

            for (const std::string& claim_id : artifact.claim_ids) {
                if (find_claim(claim_id) == nullptr) {
                    errors.push_back("artifact " + artifact_id + " references missing claim " + claim_id);
                }
            }

            if (!artifact.claim_ids.empty() && artifact.voice_claim_links.empty()) {
                errors.push_back("artifact " + artifact_id + " has claims but no explicit voice claim links");
            }

            std::set<std::string> voice_link_claim_ids;
            for (const ArtifactVoiceClaimLink& link : artifact.voice_claim_links) {
                if (link.claim_id.empty()) {
                    errors.push_back("artifact " + artifact_id + " has empty voice claim link");
                    continue;
                }
                if (link.weight <= 0.0) {
                    errors.push_back("artifact " + artifact_id + " voice claim link " + link.claim_id + " has non-positive weight");
                }
                if (!voice_link_claim_ids.insert(link.claim_id).second) {
                    errors.push_back("artifact " + artifact_id + " has duplicate voice claim link " + link.claim_id);
                }
                const Claim* linked_claim = find_claim(link.claim_id);
                if (linked_claim == nullptr) {
                    errors.push_back("artifact " + artifact_id + " voice claim link references missing claim " + link.claim_id);
                    continue;
                }
                if (linked_claim->source_artifact_id != artifact_id) {
                    errors.push_back("artifact " + artifact_id + " voice claim link " + link.claim_id + " belongs to different source artifact " + linked_claim->source_artifact_id);
                }
            }
        }

        for (const auto& [contradiction_id, contradiction] : contradictions_) {
            if (contradiction.detector_rule.empty()) {
                errors.push_back("contradiction " + contradiction_id + " has no detector rule");
            }
            if (contradiction.assigned_cause == ContradictionCause::None) {
                errors.push_back("contradiction " + contradiction_id + " has no assigned cause");
            }
            if (contradiction.involved_claim_ids.empty()) {
                errors.push_back("contradiction " + contradiction_id + " has no claims");
            }
            for (const std::string& claim_id : contradiction.involved_claim_ids) {
                if (find_claim(claim_id) == nullptr) {
                    errors.push_back("contradiction " + contradiction_id + " references missing claim " + claim_id);
                }
            }
        }

        return errors;
    }

private:
    std::map<std::string, Artifact> artifacts_;
    std::map<std::string, Claim> claims_;
    std::map<std::string, Contradiction> contradictions_;
};

struct Discovery {
    std::string id;
    std::string artifact_id;
    int discovery_year = 0;
    std::string site_id;
    AccessLevel min_access = AccessLevel::Public;
};

enum class RevealMode {
    FullyResolvable,
    PartiallyResolvable,
    NeverFullyResolvable,
    AccessLocked,
    ContradictoryByDesign,
    ResolvedOnlyInMythicTerms,
};

enum class MysteryEvidenceRole {
    CoreClue,
    ContextClue,
    MisleadingClue,
    FalseResolution,
};

// Mystery evidence links are deliberately explicit. A clue artifact may be
// relevant only through specific claims or only as context, so mystery
// assessment must not treat every visible claim on a clue artifact as core
// evidence.
struct MysteryEvidenceLink {
    std::string artifact_id;
    std::optional<std::string> claim_id;
    MysteryEvidenceRole role = MysteryEvidenceRole::CoreClue;
    double weight_multiplier = 1.0;
};

struct Mystery {
    std::string id;
    std::string title;
    std::string protected_question;
    RevealMode reveal_mode = RevealMode::PartiallyResolvable;
    double max_public_confidence = 1.0;
    double max_scholar_confidence = 1.0;
    std::vector<std::string> clue_artifact_ids;
    std::vector<std::string> misleading_artifact_ids;
    std::vector<MysteryEvidenceLink> evidence_links;
    AccessLevel min_access = AccessLevel::Public;
};


[[nodiscard]] std::string to_string(ArtifactType type);
[[nodiscard]] std::string to_string(ArtifactVoiceRegister voice);
[[nodiscard]] ArtifactVoiceRegister default_voice_register_for(ArtifactType type);
[[nodiscard]] std::string to_string(ArtifactVoiceClaimRole role);
[[nodiscard]] std::string to_string(ClaimType type);
[[nodiscard]] std::string to_string(PredicateType type);
[[nodiscard]] std::string to_string(ContradictionType type);
[[nodiscard]] std::string to_string(ContradictionCause cause);
[[nodiscard]] std::string to_string(AnachronismStatus status);
[[nodiscard]] std::string to_string(EvidenceModifier cause);
[[nodiscard]] double compute_reliability(const ReliabilityComponents& c);
[[nodiscard]] bool has_evidence_modifier(const Artifact& artifact, EvidenceModifier cause);
[[nodiscard]] AccessLevel evidence_modifier_min_access(EvidenceModifier modifier);
[[nodiscard]] bool has_visible_evidence_modifier(const Artifact& artifact, EvidenceModifier modifier, AccessLevel access);
[[nodiscard]] bool has_any_visible_evidence_modifier(const Artifact& artifact,
                                                    std::initializer_list<EvidenceModifier> modifiers,
                                                    AccessLevel access);
[[nodiscard]] std::string evidence_modifier_list_text(const Artifact& artifact);
[[nodiscard]] std::string availability_range_text(const AnachronismReport& report);
[[nodiscard]] std::string to_string(RevealMode mode);
[[nodiscard]] std::string to_string(MysteryEvidenceRole role);
void add_claim_to_archive(PublicArchive& archive,
                          Artifact& artifact,
                          Claim claim,
                          ArtifactVoiceClaimRole voice_role = ArtifactVoiceClaimRole::PrimaryLine,
                          double voice_weight = 1.0);
[[nodiscard]] Artifact finalize_artifact(Artifact artifact);
[[nodiscard]] std::string claim_suffix_for_id(std::string text);
[[nodiscard]] std::string make_contradiction_id(std::string_view rule_name, std::vector<std::string> components);
[[nodiscard]] std::optional<ContradictionCause> contradiction_cause_from_evidence(const Artifact& source);
[[nodiscard]] ContradictionType contradiction_type_for_unavailable_entity(EntityType type);
void add_unique_contradiction(std::vector<Contradiction>& contradictions, Contradiction contradiction);

} // namespace archive
