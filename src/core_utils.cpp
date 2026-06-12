/*
 * Shared conversion, parsing, and small helper functions used across the MVP.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] std::string year_text(int year) {
    if (year == kOpenEndedYear) {
        return "present";
    }
    return std::to_string(year);
}

[[nodiscard]] std::string interval_text(const Interval& interval) {
    return std::to_string(interval.start_year) + "-" + year_text(interval.end_year);
}

[[nodiscard]] int rank(AccessLevel access) {
    return static_cast<int>(access);
}

[[nodiscard]] bool can_view(AccessLevel current, AccessLevel required) {
    return rank(current) >= rank(required);
}

[[nodiscard]] std::string to_string(AccessLevel access) {
    switch (access) {
        case AccessLevel::Public: return "public";
        case AccessLevel::Scholar: return "scholar";
        case AccessLevel::Curator: return "curator";
        case AccessLevel::Canon: return "canon";
        case AccessLevel::Debug: return "debug";
    }
    return "unknown";
}

[[nodiscard]] AccessLevel parse_access(std::string_view text) {
    if (text == "public") return AccessLevel::Public;
    if (text == "scholar") return AccessLevel::Scholar;
    if (text == "curator") return AccessLevel::Curator;
    if (text == "canon") return AccessLevel::Canon;
    if (text == "debug") return AccessLevel::Debug;
    throw std::invalid_argument("unknown access level: " + std::string(text));
}

[[nodiscard]] std::string to_string(EntityType type) {
    switch (type) {
        case EntityType::Person: return "person";
        case EntityType::Office: return "office";
        case EntityType::Settlement: return "settlement";
        case EntityType::Site: return "site";
        case EntityType::Technology: return "technology";
        case EntityType::Language: return "language";
        case EntityType::Dialect: return "dialect";
        case EntityType::Script: return "script";
        case EntityType::Faction: return "faction";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(TruthLayer layer) {
    switch (layer) {
        case TruthLayer::CanonicalTruth: return "canonical_truth";
        case TruthLayer::PublicInference: return "public_inference";
        case TruthLayer::MythicTruth: return "mythic_truth";
        case TruthLayer::ForgedTruth: return "forged_truth";
        case TruthLayer::ProtectedMystery: return "protected_mystery";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ArtifactType type) {
    switch (type) {
        case ArtifactType::Inscription: return "inscription";
        case ArtifactType::DamagedManuscript: return "damaged manuscript";
        case ArtifactType::ForgedDecree: return "forged decree";
        case ArtifactType::OralHistory: return "oral history";
        case ArtifactType::TradeLedger: return "trade ledger";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ArtifactVoiceRegister voice) {
    switch (voice) {
        case ArtifactVoiceRegister::RoyalInscription: return "royal_inscription";
        case ArtifactVoiceRegister::DamagedChronicle: return "damaged_chronicle";
        case ArtifactVoiceRegister::TradeLedger: return "trade_ledger";
        case ArtifactVoiceRegister::OralSong: return "oral_song";
        case ArtifactVoiceRegister::DisputedDecree: return "disputed_decree";
    }
    return "unknown";
}

[[nodiscard]] ArtifactVoiceRegister default_voice_register_for(ArtifactType type) {
    switch (type) {
        case ArtifactType::Inscription: return ArtifactVoiceRegister::RoyalInscription;
        case ArtifactType::DamagedManuscript: return ArtifactVoiceRegister::DamagedChronicle;
        case ArtifactType::ForgedDecree: return ArtifactVoiceRegister::DisputedDecree;
        case ArtifactType::OralHistory: return ArtifactVoiceRegister::OralSong;
        case ArtifactType::TradeLedger: return ArtifactVoiceRegister::TradeLedger;
    }
    return ArtifactVoiceRegister::RoyalInscription;
}

[[nodiscard]] std::string to_string(ArtifactVoiceClaimRole role) {
    switch (role) {
        case ArtifactVoiceClaimRole::PrimaryLine: return "primary_line";
        case ArtifactVoiceClaimRole::SecondaryLine: return "secondary_line";
        case ArtifactVoiceClaimRole::CatalogNote: return "catalog_note";
        case ArtifactVoiceClaimRole::TranslationNote: return "translation_note";
        case ArtifactVoiceClaimRole::OmittedFromPublicRendering: return "omitted_from_public_rendering";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ClaimType type) {
    switch (type) {
        case ClaimType::FactualClaim: return "factual_claim";
        case ClaimType::LegalFiction: return "legal_fiction";
        case ClaimType::MythicCompression: return "mythic_compression";
        case ClaimType::PropagandaClaim: return "propaganda_claim";
        case ClaimType::TranslationGuess: return "translation_guess";
        case ClaimType::NegativeEvidence: return "negative_evidence";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(PredicateType type) {
    switch (type) {
        case PredicateType::Restored: return "restored";
        case PredicateType::Caused: return "caused";
        case PredicateType::Preceded: return "preceded";
        case PredicateType::Appointed: return "appointed";
        case PredicateType::Received: return "received";
        case PredicateType::Became: return "became";
        case PredicateType::ExistedInYear: return "existed_in_year";
        case PredicateType::CreatedOffice: return "created_office";
        case PredicateType::UsesScript: return "uses_script";
        case PredicateType::LocatedAt: return "located_at";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ContradictionType type) {
    switch (type) {
        case ContradictionType::DateContradiction: return "date contradiction";
        case ContradictionType::PersonContradiction: return "person contradiction";
        case ContradictionType::TitleContradiction: return "title contradiction";
        case ContradictionType::LanguageContradiction: return "language contradiction";
        case ContradictionType::RitualContradiction: return "ritual contradiction";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ContradictionCause cause) {
    switch (cause) {
        case ContradictionCause::None: return "none";
        case ContradictionCause::Forgery: return "Forgery";
        case ContradictionCause::Damage: return "Damage";
        case ContradictionCause::CalendarConversionError: return "CalendarConversionError";
        case ContradictionCause::Propaganda: return "Propaganda";
        case ContradictionCause::MythologizedMemory: return "MythologizedMemory";
        case ContradictionCause::RitualAnachronism: return "RitualAnachronism";
        case ContradictionCause::UnresolvedGenerationBug: return "UnresolvedGenerationBug";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(AnachronismStatus status) {
    switch (status) {
        case AnachronismStatus::Valid: return "valid";
        case AnachronismStatus::ValidBecauseForged: return "valid_because_forged";
        case AnachronismStatus::ValidBecauseLaterCopy: return "valid_because_later_copy";
        case AnachronismStatus::ValidBecauseRitual: return "valid_because_ritual";
        case AnachronismStatus::InvalidGenerationBug: return "invalid_generation_bug";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(EvidenceModifier cause) {
    switch (cause) {
        case EvidenceModifier::Forgery: return "Forgery";
        case EvidenceModifier::LaterCopy: return "LaterCopy";
        case EvidenceModifier::Interpolation: return "Interpolation";
        case EvidenceModifier::RitualAnachronism: return "RitualAnachronism";
        case EvidenceModifier::Mistranslation: return "Mistranslation";
        case EvidenceModifier::CalendarError: return "CalendarError";
        case EvidenceModifier::Propaganda: return "Propaganda";
        case EvidenceModifier::Damage: return "Damage";
        case EvidenceModifier::MisdatedStratum: return "MisdatedStratum";
        case EvidenceModifier::MythicCompression: return "MythicCompression";
        case EvidenceModifier::NarrowScope: return "NarrowScope";
    }
    return "unknown";
}

[[nodiscard]] double clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

[[nodiscard]] double compute_reliability(const ReliabilityComponents& c) {
    const double positive =
        c.provenance_confidence +
        c.preservation_integrity +
        c.temporal_proximity +
        c.creator_access_to_events +
        c.translation_confidence +
        c.external_corroboration;

    const double penalty =
        c.bias_penalty +
        c.forgery_penalty +
        c.contradiction_penalty;

    // Six positive components and three penalty components. Unknown evidence no
    // longer receives an artificial floor; penalties scale down only the positive
    // evidence actually present.
    const double positive_score = positive / 6.0;
    const double penalty_score = penalty / 3.0;
    return clamp01(positive_score * (1.0 - penalty_score));
}

[[nodiscard]] bool has_evidence_modifier(const Artifact& artifact, EvidenceModifier cause) {
    return std::find(artifact.evidence_modifiers.begin(), artifact.evidence_modifiers.end(), cause) != artifact.evidence_modifiers.end();
}

[[nodiscard]] bool has_any_evidence_modifier(const Artifact& artifact, std::initializer_list<EvidenceModifier> causes) {
    return std::any_of(causes.begin(), causes.end(), [&](EvidenceModifier cause) {
        return has_evidence_modifier(artifact, cause);
    });
}

[[nodiscard]] AccessLevel evidence_modifier_min_access(EvidenceModifier modifier) {
    switch (modifier) {
        case EvidenceModifier::Forgery:
            return AccessLevel::Curator;
        case EvidenceModifier::LaterCopy:
        case EvidenceModifier::Interpolation:
        case EvidenceModifier::Mistranslation:
        case EvidenceModifier::CalendarError:
            return AccessLevel::Scholar;
        case EvidenceModifier::RitualAnachronism:
        case EvidenceModifier::Propaganda:
        case EvidenceModifier::Damage:
        case EvidenceModifier::MisdatedStratum:
        case EvidenceModifier::MythicCompression:
        case EvidenceModifier::NarrowScope:
            return AccessLevel::Public;
    }
    return AccessLevel::Debug;
}

[[nodiscard]] bool has_visible_evidence_modifier(const Artifact& artifact, EvidenceModifier modifier, AccessLevel access) {
    return can_view(access, evidence_modifier_min_access(modifier)) && has_evidence_modifier(artifact, modifier);
}

[[nodiscard]] bool has_any_visible_evidence_modifier(const Artifact& artifact,
                                                    std::initializer_list<EvidenceModifier> modifiers,
                                                    AccessLevel access) {
    return std::any_of(modifiers.begin(), modifiers.end(), [&](EvidenceModifier modifier) {
        return has_visible_evidence_modifier(artifact, modifier, access);
    });
}

[[nodiscard]] std::string evidence_modifier_list_text(const Artifact& artifact) {
    std::ostringstream out;
    for (std::size_t i = 0; i < artifact.evidence_modifiers.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << to_string(artifact.evidence_modifiers[i]);
    }
    return out.str();
}

[[nodiscard]] std::string availability_range_text(const AnachronismReport& report) {
    if (report.availability_start_year == 0 && report.availability_end_year == 0) {
        return "unknown";
    }
    return year_text(report.availability_start_year) + "-" + year_text(report.availability_end_year);
}

[[nodiscard]] std::string to_string(RevealMode mode) {
    switch (mode) {
        case RevealMode::FullyResolvable: return "fully_resolvable";
        case RevealMode::PartiallyResolvable: return "partially_resolvable";
        case RevealMode::NeverFullyResolvable: return "never_fully_resolvable";
        case RevealMode::AccessLocked: return "access_locked";
        case RevealMode::ContradictoryByDesign: return "contradictory_by_design";
        case RevealMode::ResolvedOnlyInMythicTerms: return "resolved_only_in_mythic_terms";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(MysteryEvidenceRole role) {
    switch (role) {
        case MysteryEvidenceRole::CoreClue: return "core_clue";
        case MysteryEvidenceRole::ContextClue: return "context_clue";
        case MysteryEvidenceRole::MisleadingClue: return "misleading_clue";
        case MysteryEvidenceRole::FalseResolution: return "false_resolution";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(TropeFlag flag) {
    switch (flag) {
        case TropeFlag::GenericMoonCult: return "GenericMoonCult";
        case TropeFlag::DivineKingAnalogue: return "DivineKingAnalogue";
        case TropeFlag::LostEmpireHubrisCollapse: return "LostEmpireHubrisCollapse";
        case TropeFlag::RomanStyleRestorationInscription: return "RomanStyleRestorationInscription";
        case TropeFlag::GenericChosenProphecy: return "GenericChosenProphecy";
        case TropeFlag::GenericAncientBureaucracy: return "GenericAncientBureaucracy";
        case TropeFlag::GenericSacredKingship: return "GenericSacredKingship";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(OriginalityFeatureKind kind) {
    switch (kind) {
        case OriginalityFeatureKind::Unknown: return "unknown";
        case OriginalityFeatureKind::Office: return "office";
        case OriginalityFeatureKind::Claim: return "claim";
        case OriginalityFeatureKind::Artifact: return "artifact";
        case OriginalityFeatureKind::Script: return "script";
        case OriginalityFeatureKind::Institution: return "institution";
        case OriginalityFeatureKind::Ritual: return "ritual";
        case OriginalityFeatureKind::LegalFormula: return "legal_formula";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateFeatureType type) {
    switch (type) {
        case CandidateFeatureType::Artifact: return "artifact";
        case CandidateFeatureType::Claim: return "claim";
        case CandidateFeatureType::Event: return "event";
        case CandidateFeatureType::Entity: return "entity";
        case CandidateFeatureType::Mystery: return "mystery";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateGenerationStrategy strategy) {
    switch (strategy) {
        case CandidateGenerationStrategy::AddCorroboratingFragment: return "corroborating_fragment";
        case CandidateGenerationStrategy::AddMisleadingForgery: return "misleading_forgery";
        case CandidateGenerationStrategy::AddRitualVariant: return "ritual_variant";
        case CandidateGenerationStrategy::BuildTargetDossier: return "target_dossier";
    }
    return "unknown";
}

[[nodiscard]] CandidateGenerationStrategy parse_candidate_generation_strategy(std::string_view text) {
    if (text == "corroborating_fragment" || text == "add_corroborating_fragment") {
        return CandidateGenerationStrategy::AddCorroboratingFragment;
    }
    if (text == "misleading_forgery" || text == "add_misleading_forgery") {
        return CandidateGenerationStrategy::AddMisleadingForgery;
    }
    if (text == "ritual_variant" || text == "add_ritual_variant") {
        return CandidateGenerationStrategy::AddRitualVariant;
    }
    if (text == "target_dossier" || text == "build_target_dossier" || text == "dossier") {
        return CandidateGenerationStrategy::BuildTargetDossier;
    }
    throw std::invalid_argument("unknown generation strategy: " + std::string(text));
}

[[nodiscard]] std::string to_string(GeneratedCandidateRole role) {
    switch (role) {
        case GeneratedCandidateRole::CorroboratingFragment: return "corroborating_fragment";
        case GeneratedCandidateRole::RitualVariant: return "ritual_variant";
        case GeneratedCandidateRole::MisleadingForgery: return "misleading_forgery";
        case GeneratedCandidateRole::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] GeneratedCandidateRole parse_generated_candidate_role(std::string_view text) {
    if (text == "corroborating_fragment" || text == "corroborating" || text == "corrob") {
        return GeneratedCandidateRole::CorroboratingFragment;
    }
    if (text == "ritual_variant" || text == "ritual" || text == "mythic") {
        return GeneratedCandidateRole::RitualVariant;
    }
    if (text == "misleading_forgery" || text == "forgery" || text == "forged") {
        return GeneratedCandidateRole::MisleadingForgery;
    }
    if (text == "unknown") {
        return GeneratedCandidateRole::Unknown;
    }
    throw std::invalid_argument("unknown generated candidate role: " + std::string(text));
}


[[nodiscard]] std::string to_string(HiddenProposalType type) {
    switch (type) {
        case HiddenProposalType::Entity: return "entity";
        case HiddenProposalType::Event: return "event";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(HiddenProposalDecision decision) {
    switch (decision) {
        case HiddenProposalDecision::AcceptableProposal: return "AcceptableProposal";
        case HiddenProposalDecision::Reject: return "Reject";
        case HiddenProposalDecision::NeedsCuratorReview: return "NeedsCuratorReview";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(HiddenClusterScope scope) {
    switch (scope) {
        case HiddenClusterScope::InstitutionOrigin: return "institution_origin";
        case HiddenClusterScope::SchismPrecursor: return "schism_precursor";
        case HiddenClusterScope::EcologicalPressure: return "ecological_pressure";
        case HiddenClusterScope::PoliticalRealignment: return "political_realignment";
        case HiddenClusterScope::RitualCodification: return "ritual_codification";
    }
    return "unknown";
}

[[nodiscard]] HiddenClusterScope parse_hidden_cluster_scope(std::string_view text) {
    if (text == "institution_origin" || text == "institution") {
        return HiddenClusterScope::InstitutionOrigin;
    }
    if (text == "schism_precursor" || text == "schism") {
        return HiddenClusterScope::SchismPrecursor;
    }
    if (text == "ecological_pressure" || text == "ecology") {
        return HiddenClusterScope::EcologicalPressure;
    }
    if (text == "political_realignment" || text == "political") {
        return HiddenClusterScope::PoliticalRealignment;
    }
    if (text == "ritual_codification" || text == "ritual") {
        return HiddenClusterScope::RitualCodification;
    }
    throw std::invalid_argument("unknown hidden cluster scope: " + std::string(text));
}

[[nodiscard]] std::string to_string(HiddenClusterDecision decision) {
    switch (decision) {
        case HiddenClusterDecision::AcceptableCluster: return "AcceptableCluster";
        case HiddenClusterDecision::Reject: return "Reject";
        case HiddenClusterDecision::NeedsCuratorReview: return "NeedsCuratorReview";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateDecision decision) {
    switch (decision) {
        case CandidateDecision::Accept: return "Accept";
        case CandidateDecision::Reject: return "Reject";
        case CandidateDecision::AcceptAsForgery: return "AcceptAsForgery";
        case CandidateDecision::AcceptAsLaterCopy: return "AcceptAsLaterCopy";
        case CandidateDecision::AcceptAsMythicCompression: return "AcceptAsMythicCompression";
        case CandidateDecision::NeedsCuratorReview: return "NeedsCuratorReview";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(MaterializationDecision decision) {
    switch (decision) {
        case MaterializationDecision::InsertArtifact: return "InsertArtifact";
        case MaterializationDecision::InsertClaim: return "InsertClaim";
        case MaterializationDecision::Reject: return "Reject";
    }
    return "unknown";
}

void add_unique_string(std::vector<std::string>& values, const std::string& value) {
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

void add_mystery(ArchiveEngineState& state, Mystery mystery) {
    for (const MysteryEvidenceLink& link : mystery.evidence_links) {
        add_unique_string(mystery.clue_artifact_ids, link.artifact_id);
        if (link.role == MysteryEvidenceRole::MisleadingClue || link.role == MysteryEvidenceRole::FalseResolution) {
            add_unique_string(mystery.misleading_artifact_ids, link.artifact_id);
        }
    }

    for (const std::string& artifact_id : mystery.clue_artifact_ids) {
        Artifact* artifact = state.public_archive.find_artifact_mutable(artifact_id);
        if (artifact != nullptr) {
            add_unique_string(artifact->mystery_links, mystery.id);
        }
    }
    for (const std::string& artifact_id : mystery.misleading_artifact_ids) {
        Artifact* artifact = state.public_archive.find_artifact_mutable(artifact_id);
        if (artifact != nullptr) {
            add_unique_string(artifact->mystery_links, mystery.id);
        }
    }

    state.mysteries.push_back(std::move(mystery));
}

[[nodiscard]] std::string to_string(EpistemicStyle style) {
    switch (style) {
        case EpistemicStyle::BureaucraticMinimalist: return "bureaucratic_minimalist";
        case EpistemicStyle::RitualFormalist: return "ritual_formalist";
        case EpistemicStyle::AntiDynasticRevisionist: return "anti_dynastic_revisionist";
    }
    return "unknown";
}

[[nodiscard]] double random_between(std::mt19937_64& rng, double low, double high) {
    std::uniform_real_distribution<double> dist(low, high);
    return dist(rng);
}

void add_claim_to_archive(PublicArchive& archive,
                          Artifact& artifact,
                          Claim claim,
                          ArtifactVoiceClaimRole voice_role,
                          double voice_weight) {
    const std::string claim_id = claim.id;
    if (archive.find_artifact(artifact.id) != nullptr) {
        archive.add_claim_to_artifact(artifact.id, std::move(claim), voice_role, voice_weight);
        return;
    }

    // Legacy pre-insert construction path: fixture and candidate-materialization
    // builders still assemble an Artifact value before inserting it into the
    // archive. Keep this quarantined helper strongly validated so failures do
    // not leave detached artifact/claim links behind. New post-insert code
    // should call PublicArchive::add_claim_to_artifact() directly.
    if (claim_id.empty()) {
        throw std::runtime_error("cannot add claim with empty id to detached artifact: " + artifact.id);
    }
    if (claim.source_artifact_id != artifact.id) {
        throw std::runtime_error("claim " + claim_id + " source_artifact_id " + claim.source_artifact_id +
                                 " does not match detached artifact " + artifact.id);
    }
    if (archive.find_claim(claim_id) != nullptr) {
        throw std::runtime_error("duplicate claim id: " + claim_id);
    }
    if (std::find(artifact.claim_ids.begin(), artifact.claim_ids.end(), claim_id) != artifact.claim_ids.end()) {
        throw std::runtime_error("detached artifact " + artifact.id + " already links claim " + claim_id);
    }

    const bool add_voice_link = voice_weight > 0.0;
    const std::size_t old_claim_link_count = artifact.claim_ids.size();
    const std::size_t old_voice_link_count = artifact.voice_claim_links.size();

    artifact.claim_ids.reserve(old_claim_link_count + 1);
    if (add_voice_link) {
        artifact.voice_claim_links.reserve(old_voice_link_count + 1);
    }

    archive.add_claim(std::move(claim));
    try {
        artifact.claim_ids.push_back(claim_id);
        if (add_voice_link) {
            artifact.voice_claim_links.push_back(ArtifactVoiceClaimLink{claim_id, voice_role, voice_weight});
        }
    } catch (...) {
        artifact.claim_ids.resize(old_claim_link_count);
        artifact.voice_claim_links.resize(old_voice_link_count);
        archive.remove_claim(claim_id);
        throw;
    }
}

[[nodiscard]] Artifact finalize_artifact(Artifact artifact) {
    if (artifact.voice_register == ArtifactVoiceRegister::RoyalInscription && artifact.type != ArtifactType::Inscription) {
        artifact.voice_register = default_voice_register_for(artifact.type);
    }
    artifact.reliability_score = compute_reliability(artifact.reliability_components);
    return artifact;
}

[[nodiscard]] std::string claim_suffix_for_id(std::string text) {
    for (char& ch : text) {
        if (ch == '.' || ch == '-' || ch == ' ') {
            ch = '_';
        }
    }
    return text;
}

[[nodiscard]] std::string make_contradiction_id(std::string_view rule_name, std::vector<std::string> components) {
    std::ostringstream out;
    out << "contradiction.auto." << rule_name;
    for (std::string component : components) {
        out << "." << claim_suffix_for_id(std::move(component));
    }
    return out.str();
}

[[nodiscard]] std::optional<ContradictionCause> contradiction_cause_from_evidence(const Artifact& source) {
    if (has_evidence_modifier(source, EvidenceModifier::Forgery)) {
        return ContradictionCause::Forgery;
    }
    if (has_evidence_modifier(source, EvidenceModifier::MythicCompression)) {
        return ContradictionCause::MythologizedMemory;
    }
    if (has_evidence_modifier(source, EvidenceModifier::RitualAnachronism)) {
        return ContradictionCause::RitualAnachronism;
    }
    if (has_evidence_modifier(source, EvidenceModifier::CalendarError)) {
        return ContradictionCause::CalendarConversionError;
    }
    if (has_evidence_modifier(source, EvidenceModifier::Damage) ||
        has_evidence_modifier(source, EvidenceModifier::LaterCopy) ||
        has_evidence_modifier(source, EvidenceModifier::Interpolation)) {
        return ContradictionCause::Damage;
    }
    if (has_evidence_modifier(source, EvidenceModifier::Propaganda)) {
        return ContradictionCause::Propaganda;
    }
    return std::nullopt;
}

[[nodiscard]] ContradictionType contradiction_type_for_unavailable_entity(EntityType type) {
    switch (type) {
        case EntityType::Person: return ContradictionType::PersonContradiction;
        case EntityType::Office: return ContradictionType::TitleContradiction;
        case EntityType::Language:
        case EntityType::Dialect:
        case EntityType::Script:
            return ContradictionType::LanguageContradiction;
        case EntityType::Settlement:
        case EntityType::Site:
        case EntityType::Technology:
        case EntityType::Faction:
            return ContradictionType::DateContradiction;
    }
    return ContradictionType::DateContradiction;
}

void add_unique_contradiction(std::vector<Contradiction>& contradictions, Contradiction contradiction) {
    const auto exists = std::any_of(contradictions.begin(), contradictions.end(), [&](const Contradiction& existing) {
        return existing.id == contradiction.id;
    });
    if (!exists) {
        contradictions.push_back(std::move(contradiction));
    }
}

} // namespace archive
