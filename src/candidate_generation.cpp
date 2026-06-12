/*
 * Deterministic candidate proposal generation. This module proposes candidates from resolved targets; it does not evaluate or materialize them.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

namespace {


[[nodiscard]] std::string metadata_or_unspecified_local(std::string_view value) {
    return value.empty() ? std::string{"unspecified"} : std::string{value};
}

void append_runtime_source_summary(std::ostringstream& out, const CivilizationRuntimeSource& source) {
    out << "- runtime: spec-selected\n";
    out << "- civilization_id: " << source.civilization_id << "\n";
    out << "- civilization: " << source.display_name << "\n";
    out << "- catalog_id: " << metadata_or_unspecified_local(source.catalog_id) << "\n";
    out << "- schema_version: " << metadata_or_unspecified_local(source.schema_version) << "\n";
}
[[nodiscard]] std::uint64_t stable_hash(std::string_view text) {
    std::uint64_t value = 1469598103934665603ULL;
    for (const char ch : text) {
        value ^= static_cast<unsigned char>(ch);
        value *= 1099511628211ULL;
    }
    return value;
}

[[nodiscard]] std::string normalize_generation_topic(std::string_view text) {
    std::string normalized;
    for (const char ch : text) {
        if (ch == '-' || ch == ' ') {
            normalized.push_back('_');
        } else {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
    }
    return normalized.empty() ? "lock_authority" : normalized;
}

[[nodiscard]] std::uint64_t mix_generation_seed(const CandidateGenerationRequest& request) {
    const std::string normalized_topic = normalize_generation_topic(request.target_topic);
    std::uint64_t value = request.seed ^ stable_hash(normalized_topic);
    const std::uint64_t golden_ratio = static_cast<std::uint64_t>(0x9e3779b97f4a7c15ULL);
    const std::uint64_t strategy_multiplier = static_cast<std::uint64_t>(0xbf58476d1ce4e5b9ULL);
    value ^= static_cast<std::uint64_t>(request.target_year) + golden_ratio + (value << 6U) + (value >> 2U);
    value ^= static_cast<std::uint64_t>(static_cast<int>(request.strategy)) * strategy_multiplier;
    return value;
}

[[nodiscard]] std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}

[[nodiscard]] std::string generated_candidate_id(const CandidateGenerationRequest& request,
                                                 std::string_view role,
                                                 std::size_t index) {
    const std::string topic = normalize_generation_topic(request.target_topic);
    std::ostringstream material;
    material << topic << "|" << request.target_year << "|" << request.seed << "|"
             << static_cast<int>(request.strategy) << "|" << role << "|" << index;
    std::uint64_t value = stable_hash(material.str());
    constexpr std::uint64_t golden_ratio = static_cast<std::uint64_t>(0x9e3779b97f4a7c15ULL);
    constexpr std::uint64_t role_multiplier = static_cast<std::uint64_t>(0xbf58476d1ce4e5b9ULL);
    value ^= static_cast<std::uint64_t>(request.target_year) + golden_ratio + (value << 6U) + (value >> 2U);
    value ^= (static_cast<std::uint64_t>(index) + static_cast<std::uint64_t>(1U)) * role_multiplier;
    return "candidate.generated." + std::string(role) + "." + topic + "_" +
           std::to_string(request.target_year) + "_" + hex64(value);
}

[[nodiscard]] std::string topic_or_default(const CandidateGenerationRequest& request) {
    return normalize_generation_topic(request.target_topic);
}

[[nodiscard]] int bounded_year(int requested, int low, int high, std::uint64_t salt) {
    if (requested >= low && requested <= high) {
        return requested;
    }
    const int span = high - low + 1;
    return low + static_cast<int>(salt % static_cast<std::uint64_t>(span));
}

[[nodiscard]] bool target_has_entity(const GenerationTarget& target, std::string_view entity_id) {
    return std::find(target.entity_ids.begin(), target.entity_ids.end(), std::string(entity_id)) != target.entity_ids.end();
}

void add_target_links(CandidateFeature& candidate, const GenerationTarget& target) {
    if (target.mystery_id.has_value()) {
        candidate.proposed_links.push_back("mystery:" + *target.mystery_id);
    }
    for (const std::string& entity_id : target.entity_ids) {
        candidate.proposed_links.push_back("entity:" + entity_id);
    }
    for (const std::string& claim_id : target.claim_ids) {
        candidate.proposed_links.push_back("claim:" + claim_id);
    }
}

[[nodiscard]] std::string public_label_from_key(std::string_view key) {
    std::string text;
    text.reserve(key.size());
    bool capitalize_next = true;
    for (char ch : key) {
        if (ch == '_' || ch == '-') {
            text.push_back(' ');
            capitalize_next = true;
            continue;
        }
        if (capitalize_next) {
            text.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            capitalize_next = false;
        } else {
            text.push_back(ch);
        }
    }
    return text;
}

[[nodiscard]] std::string key_from_scoped_id(std::string_view id) {
    const std::size_t last_dot = id.rfind('.');
    if (last_dot == std::string_view::npos || last_dot + 1U >= id.size()) {
        return std::string(id);
    }
    return std::string(id.substr(last_dot + 1U));
}

[[nodiscard]] std::string public_label_from_entity_id(std::string_view entity_id) {
    return public_label_from_key(key_from_scoped_id(entity_id));
}

[[nodiscard]] std::optional<std::string> first_id_with_prefix(const std::vector<std::string>& ids,
                                                             std::string_view prefix) {
    for (const std::string& id : ids) {
        if (id.rfind(std::string(prefix), 0) == 0) {
            return id;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::string spec_bootstrap_entity_id(std::string_view kind,
                                                   const SpecGenerationTargetSource& source,
                                                   std::string_view key) {
    return std::string(kind) + "." + source.civilization_id + "." + std::string(key);
}

[[nodiscard]] std::string spec_target_subject_id(const GenerationTarget& target) {
    if (target.spec_source.has_value() && !target.spec_source->source_entity_ids.empty()) {
        return target.spec_source->source_entity_ids.front();
    }
    return target.entity_ids.empty() ? std::string{} : target.entity_ids.front();
}

[[nodiscard]] std::string spec_target_site_id(const GenerationTarget& target) {
    if (const std::optional<std::string> site = first_id_with_prefix(target.entity_ids, "site.")) {
        return *site;
    }
    if (target.spec_source.has_value()) {
        if (const std::optional<std::string> site = first_id_with_prefix(target.spec_source->source_entity_ids, "site.")) {
            return *site;
        }
    }
    return {};
}

[[nodiscard]] std::string spec_target_script_id(const GenerationTarget& target) {
    if (const std::optional<std::string> script = first_id_with_prefix(target.entity_ids, "script.")) {
        return *script;
    }
    if (target.spec_source.has_value()) {
        return spec_bootstrap_entity_id("script", *target.spec_source, "bootstrap_script");
    }
    return {};
}

[[nodiscard]] std::string spec_generated_candidate_id(const CandidateGenerationRequest& request,
                                                       const SpecGenerationTargetSource& source,
                                                       std::string_view role,
                                                       std::size_t index) {
    const std::string topic = normalize_generation_topic(request.target_topic);
    std::ostringstream material;
    material << source.civilization_id << "|" << source.target_kind << "|" << source.target_key << "|"
             << topic << "|" << request.target_year << "|" << request.seed << "|"
             << static_cast<int>(request.strategy) << "|" << role << "|" << index;
    std::uint64_t value = stable_hash(material.str());
    constexpr std::uint64_t golden_ratio = static_cast<std::uint64_t>(0x9e3779b97f4a7c15ULL);
    value ^= static_cast<std::uint64_t>(request.target_year) + golden_ratio + (value << 6U) + (value >> 2U);
    return "candidate.generated." + std::string(role) + "." + source.civilization_id + "." +
           topic + "_" + std::to_string(request.target_year) + "_" + hex64(value);
}

[[nodiscard]] CandidateFeature make_spec_bootstrap_candidate(const CandidateGenerationRequest& request,
                                                             const GenerationTarget& target,
                                                             std::string_view role,
                                                             std::size_t index,
                                                             ClaimType claim_type,
                                                             EvidenceModifier mediation,
                                                             bool add_mediation) {
    const SpecGenerationTargetSource& source = *target.spec_source;
    CandidateFeature candidate;
    candidate.id = spec_generated_candidate_id(request, source, role, index);
    candidate.type = CandidateFeatureType::Artifact;

    const int low = std::max(target.earliest_valid_year, 1);
    const int high = std::max(low, target.latest_valid_year);
    const int year = bounded_year(request.target_year, low, high, mix_generation_seed(request) + index * 37ULL);
    const std::string subject_id = spec_target_subject_id(target);
    const std::string site_id = spec_target_site_id(target);
    const std::string script_id = spec_target_script_id(target);
    const std::string language_id = spec_bootstrap_entity_id("language", source, "bootstrap_language");
    const std::string dialect_id = spec_bootstrap_entity_id("dialect", source, "bootstrap_dialect");

    std::string target_phrase = source.target_kind;
    if (source.target_kind == "authority_conflict" && source.source_entity_ids.size() >= 2U) {
        target_phrase = public_label_from_entity_id(source.source_entity_ids[0]) + " contests " +
                        public_label_from_entity_id(source.source_entity_ids[1]);
    } else if (!subject_id.empty()) {
        target_phrase = public_label_from_entity_id(subject_id);
    }

    candidate.description = "Generated " + std::string(role) + " for " + source.civilization_id +
        ": a public archive fragment preserves a spec-derived trace of " + target_phrase +
        " around year " + std::to_string(year) +
        "; it uses only entities/events from the selected spec-bootstrapped runtime.";
    add_target_links(candidate, target);

    CandidateArtifactMetadata metadata;
    metadata.true_creation_year = year;
    metadata.claimed_creation_year = year;
    metadata.discovery_year = std::min(target.latest_valid_year, std::max(year + 1, year + 20 + static_cast<int>(index)));
    metadata.location_created = site_id;
    metadata.location_found = site_id;
    metadata.language_id = language_id;
    metadata.dialect_id = dialect_id;
    metadata.script_id = script_id;
    metadata.referenced_entity_ids = target.entity_ids;
    add_unique_string(metadata.referenced_entity_ids, language_id);
    add_unique_string(metadata.referenced_entity_ids, dialect_id);
    add_unique_string(metadata.referenced_entity_ids, script_id);
    if (add_mediation) {
        metadata.declared_mediations = {mediation};
    }
    candidate.structured_artifact_metadata = metadata;

    CandidateClaimMetadata claim;
    claim.claim_type = claim_type;
    claim.predicate_type = PredicateType::LocatedAt;
    claim.subject = target_phrase;
    claim.predicate = "located_at";
    claim.object = public_label_from_entity_id(site_id);
    claim.literal_content = "Spec-derived public fragment links " + target_phrase + " to " + claim.object + ".";
    if (!subject_id.empty()) {
        claim.subject_entity_id = subject_id;
    }
    if (!site_id.empty()) {
        claim.object_entity_id = site_id;
    }
    claim.claimed_year = year;
    claim.confidence = add_mediation ? 0.42 : 0.57;
    claim.min_access = AccessLevel::Public;
    candidate.structured_claims.push_back(std::move(claim));
    return candidate;
}

[[nodiscard]] std::string target_dependency_phrase(const GenerationTarget& target) {
    if (target.topic == "silt_levy") {
        return "silt levy, lower-lock accounting, and Reservoir Gate tax records";
    }
    if (target.topic == "three_keepers") {
        return "Three Keepers song variants, Moon-office language, and the Third Lock Authority mystery";
    }
    if (target.topic == "drowned_chancellor") {
        return "Drowned Chancellor office language and post-schism lock authority";
    }
    return "Drowned Chancellor, Reservoir Gate, lower lock authority, and the Third Lock Authority mystery";
}

[[nodiscard]] std::string script_for_year(int year) {
    return year < 612 ? "script.pre_green_seal" : "script.green_seal";
}

[[nodiscard]] std::string dialect_for_year(int year) {
    if (year < 620) {
        return "dialect.lower_lattice";
    }
    if (year < 640) {
        return "dialect.upper_lattice";
    }
    return "dialect.late_lock_hand";
}

[[nodiscard]] std::vector<std::string> metadata_references_for(const GenerationTarget& target, int year) {
    std::vector<std::string> references;
    for (const std::string& entity_id : target.entity_ids) {
        references.push_back(entity_id);
    }
    const std::string script_id = script_for_year(year);
    if (std::find(references.begin(), references.end(), script_id) == references.end()) {
        references.push_back(script_id);
    }
    return references;
}

[[nodiscard]] CandidateClaimMetadata corroborating_claim_for_target(const GenerationTarget& target, int year) {
    if (target.topic == "silt_levy") {
        return CandidateClaimMetadata{
            ClaimType::FactualClaim,
            PredicateType::ExistedInYear,
            "silt levy",
            "existed_in_year",
            std::to_string(year),
            "Generated structured levy fragment records lower-lock silt levy receipts at Reservoir Gate.",
            std::nullopt,
            std::optional<std::string>{"site.reservoir_gate"},
            std::optional<int>{year},
            0.58,
            AccessLevel::Public,
        };
    }

    return CandidateClaimMetadata{
        ClaimType::FactualClaim,
        PredicateType::Received,
        "Drowned Chancellor",
        "received",
        "lower lock authority",
        "Generated structured lock fragment states that the Drowned Chancellor held lower lock authority after the silt levy.",
        std::optional<std::string>{"office.drowned_chancellor"},
        std::optional<std::string>{"site.reservoir_gate"},
        std::optional<int>{year},
        0.56,
        AccessLevel::Public,
    };
}

[[nodiscard]] CandidateClaimMetadata misleading_forgery_claim() {
    return CandidateClaimMetadata{
        ClaimType::LegalFiction,
        PredicateType::CreatedOffice,
        "King Aru",
        "appointed",
        "Drowned Chancellor",
        "Generated structured decree claims Aru appointed the Drowned Chancellor under Green-Seal authority.",
        std::optional<std::string>{"person.aru"},
        std::optional<std::string>{"office.drowned_chancellor"},
        std::optional<int>{553},
        0.18,
        AccessLevel::Public,
    };
}

[[nodiscard]] CandidateClaimMetadata ritual_variant_claim(int year) {
    return CandidateClaimMetadata{
        ClaimType::MythicCompression,
        PredicateType::Became,
        "three keepers",
        "became",
        "one moon judge",
        "Generated structured ritual variant compresses three lock keepers into one moon judge.",
        std::nullopt,
        std::optional<std::string>{"office.drowned_chancellor"},
        std::optional<int>{year},
        0.39,
        AccessLevel::Public,
    };
}

[[nodiscard]] CandidateClaimMetadata dossier_context_claim_for_target(const GenerationTarget& target, int year) {
    if (target.topic == "silt_levy") {
        return CandidateClaimMetadata{
            ClaimType::FactualClaim,
            PredicateType::Preceded,
            "silt levy",
            "preceded",
            "Reservoir Gate unrest",
            "Generated structured dossier note says lower-lock levy accounting preceded Reservoir Gate unrest.",
            std::nullopt,
            std::optional<std::string>{"site.reservoir_gate"},
            std::optional<int>{year},
            0.46,
            AccessLevel::Public,
        };
    }

    return CandidateClaimMetadata{
        ClaimType::FactualClaim,
        PredicateType::LocatedAt,
        "lock authority docket",
        "located_at",
        "Reservoir Gate archive",
        "Generated structured dossier note places lower lock authority records at Reservoir Gate.",
        std::nullopt,
        std::optional<std::string>{"site.reservoir_gate"},
        std::optional<int>{year},
        0.44,
        AccessLevel::Public,
    };
}

[[nodiscard]] CandidateFeature make_corroborating_fragment(const CandidateGenerationRequest& request,
                                                           const GenerationTarget& target,
                                                           std::uint64_t seed_value,
                                                           std::size_t index) {
    if (target.spec_source.has_value()) {
        return make_spec_bootstrap_candidate(request, target, "corroborating_fragment", index,
                                            ClaimType::FactualClaim, EvidenceModifier::NarrowScope, false);
    }
    const int low = std::max(target.earliest_valid_year, target.topic == "silt_levy" ? 604 : 617);
    const int high = std::min(target.latest_valid_year, 690);
    const int year = bounded_year(request.target_year, low, high, seed_value + index * 17ULL);
    const std::vector<std::string> materials = target.topic == "silt_levy"
        ? std::vector<std::string>{"waxed levy tally", "reed tax docket", "lower-lock receipt shard"}
        : std::vector<std::string>{"green clay docket", "canal-seal tablet", "lockhouse tally shard"};
    const std::string material = materials[static_cast<std::size_t>((seed_value + index) % materials.size())];

    CandidateFeature candidate;
    candidate.id = generated_candidate_id(request, "corroborating_fragment", index);
    candidate.type = CandidateFeatureType::Artifact;
    candidate.description = "Generated corroborating fragment for " + target.topic +
        ": a " + material + " from year " + std::to_string(year) +
        " tied to " + target_dependency_phrase(target) +
        "; it uses resolved target bindings and creates no new hidden events.";
    add_target_links(candidate, target);

    CandidateArtifactMetadata metadata;
    metadata.true_creation_year = year;
    metadata.claimed_creation_year = year;
    metadata.discovery_year = 813 + static_cast<int>(index);
    metadata.location_created = "site.reservoir_gate";
    metadata.location_found = target.topic == "silt_levy" ? "site.reservoir_gate" : "site.salt_cellar_archive";
    metadata.language_id = "language.lattice_dialect";
    metadata.dialect_id = dialect_for_year(year);
    metadata.script_id = script_for_year(year);
    metadata.referenced_entity_ids = metadata_references_for(target, year);
    candidate.structured_artifact_metadata = metadata;
    candidate.structured_claims.push_back(corroborating_claim_for_target(target, year));
    return candidate;
}

[[nodiscard]] CandidateFeature make_misleading_forgery(const CandidateGenerationRequest& request,
                                                       const GenerationTarget& target,
                                                       std::uint64_t seed_value,
                                                       std::size_t index) {
    if (target.spec_source.has_value()) {
        return make_spec_bootstrap_candidate(request, target, "misleading_forgery", index,
                                            ClaimType::LegalFiction, EvidenceModifier::Forgery, true);
    }
    const std::vector<std::string> mediation_terms = {"forged", "forgery", "declared forgery"};
    const std::string mediation = mediation_terms[static_cast<std::size_t>((seed_value + index) % mediation_terms.size())];

    CandidateFeature candidate;
    candidate.id = generated_candidate_id(request, "misleading_forgery", index);
    candidate.type = CandidateFeatureType::Artifact;
    candidate.description = "Generated misleading forgery for " + target.topic +
        ": claimed year 553 " + mediation +
        " document uses later target language around " + target_dependency_phrase(target) +
        "; the candidate explicitly declares forgery mediation and should never be auto-inserted.";
    add_target_links(candidate, target);

    CandidateArtifactMetadata metadata;
    metadata.true_creation_year = 660 + static_cast<int>((seed_value + index) % 9ULL);
    metadata.claimed_creation_year = 553;
    metadata.discovery_year = 813 + static_cast<int>(index);
    metadata.location_created = "site.salt_cellar_archive";
    metadata.location_found = "site.salt_cellar_archive";
    metadata.language_id = "language.lattice_dialect";
    metadata.dialect_id = "dialect.late_lock_hand";
    metadata.script_id = "script.green_seal";
    metadata.referenced_entity_ids = metadata_references_for(target, 660);
    if (target_has_entity(target, "person.aru") || target_has_entity(target, "office.drowned_chancellor")) {
        if (std::find(metadata.referenced_entity_ids.begin(), metadata.referenced_entity_ids.end(), "person.aru") == metadata.referenced_entity_ids.end()) {
            metadata.referenced_entity_ids.push_back("person.aru");
        }
    }
    metadata.declared_mediations = {EvidenceModifier::Forgery};
    candidate.structured_artifact_metadata = metadata;
    candidate.structured_claims.push_back(misleading_forgery_claim());
    return candidate;
}

[[nodiscard]] CandidateFeature make_ritual_variant(const CandidateGenerationRequest& request,
                                                   const GenerationTarget& target,
                                                   std::uint64_t seed_value,
                                                   std::size_t index) {
    if (target.spec_source.has_value()) {
        return make_spec_bootstrap_candidate(request, target, "ritual_variant", index,
                                            ClaimType::MythicCompression, EvidenceModifier::MythicCompression, true);
    }
    const int low = std::max(target.earliest_valid_year, 650);
    const int high = std::min(target.latest_valid_year, 760);
    const int year = bounded_year(request.target_year, low, high, seed_value + index * 29ULL);
    const std::vector<std::string> refrains = {"three voices enter the lock", "three keepers answer as one", "one wet judge speaks for three"};
    const std::string refrain = refrains[static_cast<std::size_t>((seed_value + index) % refrains.size())];

    CandidateFeature candidate;
    candidate.id = generated_candidate_id(request, "ritual_variant", index);
    candidate.type = CandidateFeatureType::Artifact;
    candidate.description = "Generated ritual variant for " + target.topic +
        ": an oral fragment from year " + std::to_string(year) +
        " where " + refrain +
        "; it deepens only the mysteries explicitly resolved by the target binding.";
    add_target_links(candidate, target);

    CandidateArtifactMetadata metadata;
    metadata.true_creation_year = year;
    metadata.claimed_creation_year = year;
    metadata.discovery_year = 813 + static_cast<int>(index);
    metadata.location_created = "site.reservoir_gate";
    metadata.location_found = "site.salt_cellar_archive";
    metadata.language_id = "language.lattice_dialect";
    metadata.dialect_id = year < 650 ? "dialect.late_lock_hand" : "dialect.lower_lattice_late";
    metadata.script_id = "script.green_seal";
    metadata.referenced_entity_ids = metadata_references_for(target, year);
    metadata.declared_mediations = {EvidenceModifier::MythicCompression};
    candidate.structured_artifact_metadata = metadata;
    candidate.structured_claims.push_back(ritual_variant_claim(year));
    return candidate;
}

[[nodiscard]] bool target_entity_exists(const ArchiveEngineState& state, const std::string& entity_id) {
    return state.hidden_truth.find_entity(entity_id) != nullptr;
}

[[nodiscard]] bool target_claim_exists(const ArchiveEngineState& state, const std::string& claim_id) {
    return state.public_archive.find_claim(claim_id) != nullptr;
}

[[nodiscard]] bool target_mystery_exists(const ArchiveEngineState& state, const std::string& mystery_id) {
    return std::any_of(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
        return mystery.id == mystery_id;
    });
}

[[nodiscard]] std::optional<GenerationTarget> make_target_if_valid(const ArchiveEngineState& state, GenerationTarget target) {
    if (target.mystery_id.has_value() && !target_mystery_exists(state, *target.mystery_id)) {
        return std::nullopt;
    }
    for (const std::string& entity_id : target.entity_ids) {
        if (!target_entity_exists(state, entity_id)) {
            return std::nullopt;
        }
    }
    for (const std::string& claim_id : target.claim_ids) {
        if (!target_claim_exists(state, claim_id)) {
            return std::nullopt;
        }
    }
    return target;
}

[[nodiscard]] std::optional<std::string> first_existing_entity_id_with_prefix(const ArchiveEngineState& state,
                                                                              std::string_view prefix) {
    for (const auto& [id, entity] : state.hidden_truth.entities()) {
        (void)entity;
        if (id.rfind(std::string(prefix), 0) == 0) {
            return id;
        }
    }
    return std::nullopt;
}

void add_spec_context_entities(const ArchiveEngineState& state,
                               const CivilizationRuntimeSource& source,
                               std::vector<std::string>& entity_ids) {
    if (const std::optional<std::string> site = first_existing_entity_id_with_prefix(state, "site." + source.civilization_id + ".")) {
        add_unique_string(entity_ids, *site);
    }
    if (const std::optional<std::string> script = first_existing_entity_id_with_prefix(state, "script." + source.civilization_id + ".")) {
        add_unique_string(entity_ids, *script);
    }
    add_unique_string(entity_ids, "language." + source.civilization_id + ".bootstrap_language");
    add_unique_string(entity_ids, "dialect." + source.civilization_id + ".bootstrap_dialect");
}

[[nodiscard]] std::optional<std::size_t> parse_nonnegative_index(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }
    std::size_t value = 0U;
    for (char ch : text) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return std::nullopt;
        }
        value = value * 10U + static_cast<std::size_t>(ch - '0');
    }
    return value;
}

[[nodiscard]] std::string spec_event_id(const CivilizationRuntimeSource& source, std::string_view event_key) {
    return "event." + source.civilization_id + "." + std::string(event_key);
}

[[nodiscard]] std::string spec_entity_id(const CivilizationRuntimeSource& source,
                                         std::string_view kind,
                                         std::string_view key) {
    return std::string(kind) + "." + source.civilization_id + "." + std::string(key);
}

[[nodiscard]] GenerationTarget make_spec_event_target(const ArchiveEngineState& state,
                                                      const CivilizationRuntimeSource& source,
                                                      std::string topic,
                                                      std::string target_kind,
                                                      std::string target_key,
                                                      const Event& event) {
    std::vector<std::string> entity_ids = event.participant_entity_ids;
    add_spec_context_entities(state, source, entity_ids);
    GenerationTarget target;
    target.topic = std::move(topic);
    target.entity_ids = entity_ids;
    target.earliest_valid_year = event.start_year;
    target.latest_valid_year = source.latest_year;
    target.spec_source = SpecGenerationTargetSource{
        source.civilization_id,
        std::move(target_kind),
        std::move(target_key),
        event.participant_entity_ids,
        {event.id},
    };
    return target;
}

} // namespace

[[nodiscard]] std::optional<GenerationTarget> resolve_spec_generation_target(const ArchiveEngineState& state,
                                                                             std::string_view target_topic) {
    if (!state.civilization_source.has_value()) {
        return std::nullopt;
    }
    const CivilizationRuntimeSource& source = *state.civilization_source;
    const std::string topic = normalize_generation_topic(target_topic);
    constexpr std::string_view authority_prefix = "authority_conflict_";
    constexpr std::string_view institution_prefix = "institution_";
    constexpr std::string_view site_prefix = "site_";

    if (topic.rfind(std::string(authority_prefix), 0) == 0) {
        const std::string index_text = topic.substr(authority_prefix.size());
        const std::optional<std::size_t> index = parse_nonnegative_index(index_text);
        if (!index.has_value()) {
            return std::nullopt;
        }
        const std::string event_id = spec_event_id(source, "conflict_" + std::to_string(*index));
        const Event* event = state.hidden_truth.find_event(event_id);
        if (event == nullptr) {
            return std::nullopt;
        }
        return make_target_if_valid(state, make_spec_event_target(state, source, topic, "authority_conflict", index_text, *event));
    }

    if (topic.rfind(std::string(institution_prefix), 0) == 0) {
        const std::string key = topic.substr(institution_prefix.size());
        const std::string entity_id = spec_entity_id(source, "institution", key);
        if (state.hidden_truth.find_entity(entity_id) == nullptr) {
            return std::nullopt;
        }
        std::vector<std::string> entity_ids{entity_id};
        add_spec_context_entities(state, source, entity_ids);
        GenerationTarget target;
        target.topic = topic;
        target.entity_ids = entity_ids;
        target.earliest_valid_year = source.earliest_year;
        target.latest_valid_year = source.latest_year;
        const std::string institution_event_id = spec_event_id(source, "institution." + key);
        std::vector<std::string> event_ids;
        if (state.hidden_truth.find_event(institution_event_id) != nullptr) {
            event_ids.push_back(institution_event_id);
        }
        target.spec_source = SpecGenerationTargetSource{source.civilization_id, "institution", key, {entity_id}, event_ids};
        return make_target_if_valid(state, target);
    }

    if (topic.rfind(std::string(site_prefix), 0) == 0) {
        const std::string key = topic.substr(site_prefix.size());
        const std::string entity_id = spec_entity_id(source, "site", key);
        if (state.hidden_truth.find_entity(entity_id) == nullptr) {
            return std::nullopt;
        }
        std::vector<std::string> entity_ids{entity_id};
        add_spec_context_entities(state, source, entity_ids);
        GenerationTarget target;
        target.topic = topic;
        target.entity_ids = entity_ids;
        target.earliest_valid_year = source.earliest_year;
        target.latest_valid_year = source.latest_year;
        target.spec_source = SpecGenerationTargetSource{source.civilization_id, "site", key, {entity_id}, {spec_event_id(source, "foundation")}};
        return make_target_if_valid(state, target);
    }

    if (topic == "recordkeeping" || topic == "mystery_seed") {
        const std::string event_id = spec_event_id(source, topic);
        const Event* event = state.hidden_truth.find_event(event_id);
        if (event == nullptr) {
            return std::nullopt;
        }
        return make_target_if_valid(state, make_spec_event_target(state, source, topic, topic, topic, *event));
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<GenerationTarget> resolve_generation_target(const ArchiveEngineState& state,
                                                                        std::string_view target_topic) {
    if (state.civilization_source.has_value()) {
        if (const std::optional<GenerationTarget> spec_target = resolve_spec_generation_target(state, target_topic)) {
            return spec_target;
        }
    }
    const std::string topic = normalize_generation_topic(target_topic);
    if (topic == "lock_authority") {
        return make_target_if_valid(state, GenerationTarget{
            "lock_authority",
            std::optional<std::string>{"mystery.third_lock_authority"},
            {"office.drowned_chancellor", "site.reservoir_gate", "script.green_seal"},
            {"claim.moon_office_locks", "claim.three_as_one", "claim.levy_exists_607"},
            617,
            760,
            std::nullopt,
        });
    }
    if (topic == "drowned_chancellor") {
        return make_target_if_valid(state, GenerationTarget{
            "drowned_chancellor",
            std::nullopt,
            {"office.drowned_chancellor", "site.reservoir_gate", "script.green_seal", "person.aru"},
            {"claim.aru_created_office"},
            617,
            900,
            std::nullopt,
        });
    }
    if (topic == "three_keepers") {
        return make_target_if_valid(state, GenerationTarget{
            "three_keepers",
            std::optional<std::string>{"mystery.third_lock_authority"},
            {"office.drowned_chancellor", "site.reservoir_gate"},
            {"claim.three_as_one"},
            650,
            760,
            std::nullopt,
        });
    }
    if (topic == "silt_levy") {
        return make_target_if_valid(state, GenerationTarget{
            "silt_levy",
            std::nullopt,
            {"site.reservoir_gate"},
            {"claim.levy_exists_607", "claim.levy_before_revolt"},
            604,
            640,
            std::nullopt,
        });
    }
    return std::nullopt;
}

[[nodiscard]] std::vector<std::string> list_generation_targets_for_state(const ArchiveEngineState& state) {
    std::vector<std::string> targets;
    if (!state.civilization_source.has_value()) {
        targets.push_back("lock_authority: Third Lock Authority / Reservoir Gate");
        targets.push_back("drowned_chancellor: Drowned Chancellor office context");
        targets.push_back("three_keepers: Three Keepers ritual variants");
        targets.push_back("silt_levy: Reservoir Gate levy evidence");
        return targets;
    }

    const CivilizationRuntimeSource& source = *state.civilization_source;
    for (const auto& [id, event] : state.hidden_truth.events()) {
        if (event.event_type == "spec_bootstrap_authority_conflict") {
            const std::string prefix = "event." + source.civilization_id + ".conflict_";
            if (id.rfind(prefix, 0) == 0) {
                const std::string index = id.substr(prefix.size());
                std::string line = "authority_conflict_" + index + ": ";
                if (event.participant_entity_ids.size() >= 2U) {
                    line += public_label_from_entity_id(event.participant_entity_ids[0]) + " contests " +
                            public_label_from_entity_id(event.participant_entity_ids[1]);
                } else {
                    line += event.title;
                }
                targets.push_back(std::move(line));
            }
        }
    }
    for (const auto& [id, entity] : state.hidden_truth.entities()) {
        if (id.rfind("institution." + source.civilization_id + ".", 0) == 0) {
            targets.push_back("institution_" + key_from_scoped_id(id));
        }
    }
    for (const auto& [id, entity] : state.hidden_truth.entities()) {
        if (id.rfind("site." + source.civilization_id + ".", 0) == 0) {
            targets.push_back("site_" + key_from_scoped_id(id));
        }
    }
    targets.push_back("recordkeeping");
    targets.push_back("mystery_seed");
    return targets;
}

[[nodiscard]] std::string format_generation_targets_for_state(const ArchiveEngineState& state,
                                                              AccessLevel access) {
    std::ostringstream out;
    out << "Generation targets:\n";
    const std::vector<std::string> targets = list_generation_targets_for_state(state);
    for (const std::string& target : targets) {
        out << "- " << target << "\n";
    }
    if (state.civilization_source.has_value()) {
        append_runtime_source_summary(out, *state.civilization_source);
        if (can_view(access, AccessLevel::Curator)) {
            out << "Spec target trace:\n";
            for (const auto& [id, event] : state.hidden_truth.events()) {
                if (event.event_type == "spec_bootstrap_authority_conflict" ||
                    event.event_type == "spec_bootstrap_recordkeeping" ||
                    event.event_type == "spec_bootstrap_mystery_seed") {
                    out << "- " << id << " participants:";
                    for (const std::string& participant_id : event.participant_entity_ids) {
                        out << " " << participant_id;
                    }
                    out << "\n";
                }
            }
        }
    } else {
        out << "- runtime: fixed fixture regression mode\n";
    }
    return out.str();
}

[[nodiscard]] GeneratedCandidateBatch generate_candidate_batch(const ArchiveEngineState& state,
                                                              const CandidateGenerationRequest& request) {
    GeneratedCandidateBatch batch;
    batch.request = request;
    batch.resolved_target = resolve_generation_target(state, request.target_topic);
    if (!batch.resolved_target.has_value()) {
        return batch;
    }

    const GenerationTarget& target = *batch.resolved_target;
    const std::uint64_t seed_value = mix_generation_seed(request);

    if (request.strategy == CandidateGenerationStrategy::BuildTargetDossier) {
        CandidateFeature primary = make_corroborating_fragment(request, target, seed_value, 0U);
        if (primary.structured_artifact_metadata.has_value() && !primary.structured_claims.empty()) {
            const int claim_year = primary.structured_claims.front().claimed_year.value_or(
                primary.structured_artifact_metadata->claimed_creation_year.value_or(request.target_year));
            primary.structured_claims.push_back(dossier_context_claim_for_target(target, claim_year));
            primary.description += " The dossier version carries a second typed contextual claim so multi-claim materialization is explicit.";
        }
        batch.candidates.push_back(std::move(primary));

        if (target.mystery_id.has_value()) {
            batch.candidates.push_back(make_ritual_variant(request, target, seed_value, 1U));
            batch.candidates.push_back(make_misleading_forgery(request, target, seed_value, 2U));
        } else {
            batch.candidates.push_back(make_corroborating_fragment(request, target, seed_value, 1U));
            batch.candidates.push_back(make_corroborating_fragment(request, target, seed_value, 2U));
        }
    } else {
        for (std::size_t i = 0; i < 3; ++i) {
            switch (request.strategy) {
                case CandidateGenerationStrategy::AddCorroboratingFragment:
                    batch.candidates.push_back(make_corroborating_fragment(request, target, seed_value, i));
                    break;
                case CandidateGenerationStrategy::AddMisleadingForgery:
                    batch.candidates.push_back(make_misleading_forgery(request, target, seed_value, i));
                    break;
                case CandidateGenerationStrategy::AddRitualVariant:
                    batch.candidates.push_back(make_ritual_variant(request, target, seed_value, i));
                    break;
                case CandidateGenerationStrategy::BuildTargetDossier:
                    break;
            }
        }
    }

    if ((seed_value % 2ULL) == 1ULL) {
        std::reverse(batch.candidates.begin(), batch.candidates.end());
    }

    return batch;
}

[[nodiscard]] std::vector<CandidateFeature> generate_candidate_features(const ArchiveEngineState& state,
                                                                        const CandidateGenerationRequest& request) {
    return generate_candidate_batch(state, request).candidates;
}

[[nodiscard]] std::optional<CandidateFeature> generated_candidate_at(const ArchiveEngineState& state,
                                                                     const CandidateGenerationRequest& request,
                                                                     std::size_t candidate_index) {
    const GeneratedCandidateBatch batch = generate_candidate_batch(state, request);
    if (candidate_index >= batch.candidates.size()) {
        return std::nullopt;
    }
    return batch.candidates[candidate_index];
}

[[nodiscard]] std::string format_generated_candidates(const ArchiveEngineState& state,
                                                      AccessLevel access,
                                                      const CandidateGenerationRequest& request) {
    std::ostringstream out;
    out << "Generated candidate proposals visible to " << to_string(access) << ":\n";
    out << "- strategy: " << to_string(request.strategy) << "\n";
    out << "- target_topic: " << topic_or_default(request) << "\n";
    out << "- target_year: " << request.target_year << "\n";

    const std::optional<GenerationTarget> target = resolve_generation_target(state, request.target_topic);
    if (!target.has_value()) {
        out << "- target_resolution: unresolved\n";
        out << "- generated_count: 0\n";
        out << "- explanation: unknown generation target topic; no candidates were generated and archive state was not mutated\n";
        return out.str();
    }
    out << "- target_resolution: " << target->topic;
    if (target->mystery_id.has_value() && can_view(access, AccessLevel::Curator)) {
        out << " -> " << *target->mystery_id;
    }
    out << "\n";
    if (target->spec_source.has_value()) {
        if (state.civilization_source.has_value()) {
            append_runtime_source_summary(out, *state.civilization_source);
        } else {
            out << "- runtime: spec-selected\n";
        }
        if (can_view(access, AccessLevel::Curator)) {
            out << "Spec target source:\n";
            out << "- civilization_id: " << target->spec_source->civilization_id << "\n";
            out << "- target_kind: " << target->spec_source->target_kind << "\n";
            out << "- target_key: " << target->spec_source->target_key << "\n";
            out << "- source_entities:";
            for (const std::string& id : target->spec_source->source_entity_ids) {
                out << " " << id;
            }
            out << "\n";
            out << "- source_events:";
            for (const std::string& id : target->spec_source->source_event_ids) {
                out << " " << id;
            }
            out << "\n";
        }
    }

    if (!can_view(access, AccessLevel::Scholar)) {
        out << "- candidate generation output is restricted to scholar/curator/canon/debug access\n";
        return out.str();
    }

    const std::string before = serialize_for_replay_test(state);
    const GeneratedCandidateBatch batch = generate_candidate_batch(state, request);
    const std::string after = serialize_for_replay_test(state);

    out << "- seed: " << request.seed << "\n";
    out << "- generated_count: " << batch.candidates.size() << "\n";
    out << "- archive_mutated: " << (before == after ? "false" : "true") << "\n";
    if (can_view(access, AccessLevel::Debug)) {
        out << "- generation_trace: deterministic target-aware generator; no archive insertion; seed_path=" << mix_generation_seed(request) << "\n";
        out << "- target_entities:";
        for (const std::string& entity_id : target->entity_ids) {
            out << " " << entity_id;
        }
        out << "\n";
    }

    for (const CandidateFeature& candidate : batch.candidates) {
        out << "\n";
        out << format_candidate_evaluation(evaluate_candidate_feature(state, candidate, access), candidate, access);
    }

    return out.str();
}

[[nodiscard]] std::string shape_slug(HiddenMutationArtifactCandidateShape shape) {
    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket: return "admin_docket";
        case HiddenMutationArtifactCandidateShape::RitualNotice: return "ritual_notice";
        case HiddenMutationArtifactCandidateShape::ScholarFragment: return "scholar_fragment";
    }
    return "unknown_shape";
}

[[nodiscard]] std::string shape_public_title(HiddenMutationArtifactCandidateShape shape) {
    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket: return "administrative docket";
        case HiddenMutationArtifactCandidateShape::RitualNotice: return "ritual notice";
        case HiddenMutationArtifactCandidateShape::ScholarFragment: return "later scholar fragment";
    }
    return "artifact candidate";
}

[[nodiscard]] std::string hidden_mutation_candidate_id(const HiddenTruthMutationRecord& record,
                                                       const CandidateGenerationRequest& request,
                                                       HiddenMutationArtifactCandidateShape shape,
                                                       std::size_t index) {
    const std::string topic = normalize_generation_topic(record.target_topic.empty() ? request.target_topic : record.target_topic);
    const std::string shape_name = shape_slug(shape);
    std::ostringstream material;
    material << record.id << "|" << record.source_cluster_id << "|" << request.seed << "|"
             << topic << "|" << request.target_year << "|" << shape_name << "|" << index;
    return "candidate.hidden_mutation_artifact." + shape_name + "." + topic + "_" +
           std::to_string(request.target_year) + "_" + hex64(stable_hash(material.str()));
}

[[nodiscard]] int source_artifact_year(const HiddenTruthMutationRecord& record, const CandidateGenerationRequest& request) {
    if (request.target_year >= record.start_year && request.target_year <= record.end_year + 80) {
        return std::max(request.target_year, std::max(record.end_year, 620));
    }
    return std::max(record.end_year + 1, 620);
}

[[nodiscard]] int candidate_year_for_shape(const HiddenTruthMutationRecord& record,
                                           const CandidateGenerationRequest& request,
                                           HiddenMutationArtifactCandidateShape shape) {
    const int base_year = source_artifact_year(record, request);
    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket:
            return base_year;
        case HiddenMutationArtifactCandidateShape::RitualNotice:
            return std::max(base_year + 2, record.end_year + 2);
        case HiddenMutationArtifactCandidateShape::ScholarFragment:
            return std::min(std::max(base_year + 100, 640), 755);
    }
    return base_year;
}

void add_unique_entity_references(std::vector<std::string>& references, const std::vector<std::string>& ids) {
    for (const std::string& id : ids) {
        if (std::find(references.begin(), references.end(), id) == references.end()) {
            references.push_back(id);
        }
    }
}

[[nodiscard]] std::string first_existing_spec_entity(const ArchiveEngineState& state,
                                                     const CivilizationRuntimeSource& source,
                                                     std::string_view kind) {
    return first_existing_entity_id_with_prefix(state, std::string(kind) + "." + source.civilization_id + ".").value_or(std::string{});
}

[[nodiscard]] std::string first_existing_spec_target_entity(const ArchiveEngineState& state,
                                                            const GenerationTarget& target,
                                                            std::string_view kind) {
    const std::string prefix = std::string(kind) + ".";
    if (const std::optional<std::string> from_target = first_id_with_prefix(target.entity_ids, prefix)) {
        if (state.hidden_truth.find_entity(*from_target) != nullptr) {
            return *from_target;
        }
    }
    if (target.spec_source.has_value()) {
        if (const std::optional<std::string> from_source = first_id_with_prefix(target.spec_source->source_entity_ids, prefix)) {
            if (state.hidden_truth.find_entity(*from_source) != nullptr) {
                return *from_source;
            }
        }
    }
    return {};
}

[[nodiscard]] std::string spec_target_phrase_for_mutation(const GenerationTarget& target) {
    if (target.spec_source.has_value()) {
        const SpecGenerationTargetSource& source = *target.spec_source;
        if (source.target_kind == "authority_conflict" && source.source_entity_ids.size() >= 2U) {
            return public_label_from_entity_id(source.source_entity_ids[0]) + " contests " +
                   public_label_from_entity_id(source.source_entity_ids[1]);
        }
        if (!source.source_entity_ids.empty()) {
            return public_label_from_entity_id(source.source_entity_ids.front());
        }
        return public_label_from_key(source.target_key.empty() ? source.target_kind : source.target_key);
    }
    return public_label_from_key(target.topic);
}

void rewrite_hidden_mutation_candidate_for_spec_state(const ArchiveEngineState& state,
                                                      const GenerationTarget& target,
                                                      CandidateFeature& candidate,
                                                      HiddenMutationArtifactCandidateShape shape,
                                                      int year) {
    if (!state.civilization_source.has_value() || !target.spec_source.has_value() ||
        !candidate.structured_artifact_metadata.has_value()) {
        return;
    }

    const CivilizationRuntimeSource& runtime = *state.civilization_source;
    const std::string site_id = !first_existing_spec_target_entity(state, target, "site").empty()
        ? first_existing_spec_target_entity(state, target, "site")
        : first_existing_spec_entity(state, runtime, "site");
    const std::string script_id = !first_existing_spec_target_entity(state, target, "script").empty()
        ? first_existing_spec_target_entity(state, target, "script")
        : first_existing_spec_entity(state, runtime, "script");
    const std::string language_id = first_existing_spec_entity(state, runtime, "language");
    const std::string dialect_id = first_existing_spec_entity(state, runtime, "dialect");
    const std::string target_phrase = spec_target_phrase_for_mutation(target);
    const int effective_year = std::min(std::max(year, runtime.earliest_year), std::max(runtime.earliest_year, runtime.latest_year - 1));

    CandidateArtifactMetadata& metadata = *candidate.structured_artifact_metadata;
    metadata.true_creation_year = effective_year;
    metadata.claimed_creation_year = effective_year;
    metadata.discovery_year = std::min(runtime.latest_year, std::max(effective_year + 1, effective_year + 20));
    metadata.location_created = site_id;
    metadata.location_found = site_id;
    metadata.language_id = language_id;
    metadata.dialect_id = dialect_id;
    metadata.script_id = script_id;
    metadata.referenced_entity_ids = target.entity_ids;
    add_unique_string(metadata.referenced_entity_ids, site_id);
    add_unique_string(metadata.referenced_entity_ids, language_id);
    add_unique_string(metadata.referenced_entity_ids, dialect_id);
    add_unique_string(metadata.referenced_entity_ids, script_id);
    if (candidate.hidden_mutation_source.has_value()) {
        add_unique_entity_references(metadata.referenced_entity_ids, candidate.hidden_mutation_source->source_entity_ids);
    }

    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket:
            candidate.description = "Generated public artifact candidate: an administrative docket from " +
                runtime.display_name + " around year " + std::to_string(effective_year) +
                " preserves a spec-derived trace of " + target_phrase +
                " without inserting an artifact or exposing hidden source IDs.";
            break;
        case HiddenMutationArtifactCandidateShape::RitualNotice:
            candidate.description = "Generated public artifact candidate: a ritual notice from " +
                runtime.display_name + " around year " + std::to_string(effective_year) +
                " translates " + target_phrase +
                " into public ceremonial language while preserving uncertainty.";
            break;
        case HiddenMutationArtifactCandidateShape::ScholarFragment:
            candidate.description = "Generated public artifact candidate: a later scholar fragment from " +
                runtime.display_name + " around year " + std::to_string(effective_year) +
                " catalogues copied evidence for " + target_phrase +
                " without disclosing hidden mutation provenance.";
            break;
    }

    for (CandidateClaimMetadata& claim : candidate.structured_claims) {
        claim.subject = target_phrase;
        claim.claimed_year = effective_year;
        if (!target.spec_source->source_entity_ids.empty()) {
            claim.subject_entity_id = target.spec_source->source_entity_ids.front();
        }
        if (shape == HiddenMutationArtifactCandidateShape::RitualNotice) {
            claim.predicate_type = PredicateType::Received;
            claim.predicate = "received";
            claim.object = "public ritual acknowledgement";
            claim.object_entity_id.reset();
            claim.literal_content = "A public ritual notice acknowledges " + target_phrase +
                " within " + runtime.display_name + " without exposing hidden mutation identifiers.";
        } else {
            claim.predicate_type = PredicateType::LocatedAt;
            claim.predicate = "located_at";
            claim.object = site_id.empty() ? runtime.display_name + " public archive" : public_label_from_entity_id(site_id);
            if (!site_id.empty()) {
                claim.object_entity_id = site_id;
            } else {
                claim.object_entity_id.reset();
            }
            claim.literal_content = "A public archive fragment links " + target_phrase +
                " to " + claim.object + " without exposing hidden mutation identifiers.";
        }
    }
}

[[nodiscard]] std::string candidate_description_for_shape(const HiddenTruthMutationRecord& record,
                                                          const CandidateGenerationRequest& request,
                                                          HiddenMutationArtifactCandidateShape shape,
                                                          int year) {
    const std::string topic = normalize_generation_topic(record.target_topic.empty() ? request.target_topic : record.target_topic);
    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket:
            return "Generated public artifact candidate: a reviewable lower-lock administrative docket from year " +
                   std::to_string(year) + " that projects accepted lock-authority context into archive-visible evidence without inserting an artifact.";
        case HiddenMutationArtifactCandidateShape::RitualNotice:
            return "Generated public artifact candidate: a ceremonial lockhouse notice from year " +
                   std::to_string(year) + " that records a procedural observance around " + topic +
                   " while preserving public uncertainty.";
        case HiddenMutationArtifactCandidateShape::ScholarFragment:
            return "Generated public artifact candidate: a later scholar catalog fragment from year " +
                   std::to_string(year) + " that describes copied lower-lock evidence and its disputed public consequences.";
    }
    return "Generated public artifact candidate from accepted lock-authority context.";
}

void populate_common_hidden_mutation_metadata(CandidateArtifactMetadata& metadata,
                                              const HiddenTruthMutationRecord& record,
                                              int year,
                                              std::size_t index,
                                              HiddenMutationArtifactCandidateShape shape) {
    metadata.true_creation_year = year;
    metadata.claimed_creation_year = year;
    metadata.discovery_year = std::max(814 + static_cast<int>(index) + static_cast<int>(year % 7), year + 1);
    metadata.location_created = "site.reservoir_gate";
    metadata.location_found = "site.salt_cellar_archive";
    metadata.language_id = "language.lattice_dialect";
    metadata.dialect_id = dialect_for_year(year);
    metadata.script_id = script_for_year(year);
    metadata.referenced_entity_ids = {"site.reservoir_gate", "script.green_seal"};

    if (shape == HiddenMutationArtifactCandidateShape::ScholarFragment) {
        metadata.location_created = "site.salt_cellar_archive";
        metadata.declared_mediations.push_back(EvidenceModifier::LaterCopy);
    }

    add_unique_entity_references(metadata.referenced_entity_ids, record.inserted_entity_ids);
}

[[nodiscard]] CandidateClaimMetadata claim_for_hidden_mutation_shape(const HiddenTruthMutationRecord& record,
                                                                     HiddenMutationArtifactCandidateShape shape,
                                                                     int year) {
    CandidateClaimMetadata claim;
    claim.claimed_year = year;
    claim.min_access = AccessLevel::Public;
    claim.object_entity_id = "site.reservoir_gate";

    switch (shape) {
        case HiddenMutationArtifactCandidateShape::AdministrativeDocket:
            claim.claim_type = ClaimType::FactualClaim;
            claim.predicate_type = PredicateType::LocatedAt;
            claim.subject = "lower-lock administrative docket";
            claim.predicate = "located_at";
            claim.object = "Reservoir Gate archive";
            claim.literal_content = "A public docket places lower-lock authority traces in the Reservoir Gate archive without exposing restricted source identifiers.";
            claim.confidence = 0.54;
            break;
        case HiddenMutationArtifactCandidateShape::RitualNotice:
            claim.claim_type = ClaimType::LegalFiction;
            claim.predicate_type = PredicateType::Received;
            claim.subject = "lockhouse ritual notice";
            claim.predicate = "received";
            claim.object = "lower-lock observance";
            claim.literal_content = "A ceremonial notice records that lockhouse procedure acknowledged a lower-lock authority trace in public ritual language.";
            claim.confidence = 0.48;
            break;
        case HiddenMutationArtifactCandidateShape::ScholarFragment:
            claim.claim_type = ClaimType::TranslationGuess;
            claim.predicate_type = PredicateType::LocatedAt;
            claim.subject = "later catalog fragment";
            claim.predicate = "references";
            claim.object = "copied lower-lock evidence";
            claim.literal_content = "A later scholar fragment describes copied lower-lock evidence as a disputed archive-visible consequence, not as direct hidden-truth disclosure.";
            claim.confidence = 0.43;
            break;
    }

    if (!record.inserted_entity_ids.empty()) {
        claim.subject_entity_id = record.inserted_entity_ids.front();
    }
    return claim;
}

[[nodiscard]] CandidateFeature make_hidden_mutation_candidate(const ArchiveEngineState& state,
                                                              const HiddenTruthMutationRecord& record,
                                                              const CandidateGenerationRequest& request,
                                                              HiddenMutationArtifactCandidateShape shape,
                                                              std::size_t index) {
    const int year = candidate_year_for_shape(record, request, shape);
    const std::string topic = record.target_topic.empty() ? topic_or_default(request) : record.target_topic;

    CandidateFeature candidate;
    candidate.id = hidden_mutation_candidate_id(record, request, shape, index);
    candidate.type = CandidateFeatureType::Artifact;
    candidate.description = candidate_description_for_shape(record, request, shape, year);

    const std::optional<GenerationTarget> maybe_target = resolve_generation_target(state, topic);
    if (maybe_target.has_value()) {
        add_target_links(candidate, *maybe_target);
    }

    CandidateArtifactMetadata metadata;
    populate_common_hidden_mutation_metadata(metadata, record, year, index, shape);
    candidate.structured_artifact_metadata = metadata;
    candidate.structured_claims.push_back(claim_for_hidden_mutation_shape(record, shape, year));

    candidate.hidden_mutation_source = HiddenMutationArtifactSource{
        record.id,
        record.source_cluster_id,
        record.inserted_entity_ids,
        record.inserted_event_ids,
    };
    if (maybe_target.has_value()) {
        rewrite_hidden_mutation_candidate_for_spec_state(state, *maybe_target, candidate, shape, year);
    }
    return candidate;
}

[[nodiscard]] GeneratedCandidateBatch generate_candidates_from_hidden_mutation(
    const ArchiveEngineState& state,
    const HiddenTruthMutationRecord& record,
    const CandidateGenerationRequest& request
) {
    GeneratedCandidateBatch batch;
    batch.request = request;
    batch.resolved_target = resolve_generation_target(state, record.target_topic.empty() ? request.target_topic : record.target_topic);

    if (!validate_hidden_mutation_artifact_source(state, record).empty()) {
        return batch;
    }

    const std::array<HiddenMutationArtifactCandidateShape, 3> shapes{
        HiddenMutationArtifactCandidateShape::AdministrativeDocket,
        HiddenMutationArtifactCandidateShape::RitualNotice,
        HiddenMutationArtifactCandidateShape::ScholarFragment,
    };

    for (std::size_t i = 0; i < shapes.size(); ++i) {
        batch.candidates.push_back(make_hidden_mutation_candidate(state, record, request, shapes[i], i));
    }
    return batch;
}

[[nodiscard]] HiddenMutationCandidateSourceSummary summarize_hidden_mutation_candidate_source(
    const CandidateFeature& candidate,
    AccessLevel access
) {
    HiddenMutationCandidateSourceSummary summary;
    summary.public_origin_label = "lower-lock administrative pressure";
    summary.public_effect_label = "archive-visible lock authority trace";

    bool spec_derived = false;
    if (candidate.hidden_mutation_source.has_value()) {
        const HiddenMutationArtifactSource& source = *candidate.hidden_mutation_source;
        summary.source_entity_count = source.source_entity_ids.size();
        summary.source_event_count = source.source_event_ids.size();
        if (!source.source_entity_ids.empty() && source.source_entity_ids.front().rfind("entity.generated.", 0) == 0) {
            spec_derived = true;
            summary.public_origin_label = "spec-derived hidden mutation pressure";
            summary.public_effect_label = "archive-visible spec target trace";
        }
    }

    if (!spec_derived && contains_ci(candidate.id, "ritual_notice")) {
        summary.public_origin_label = "lockhouse ceremonial procedure";
        summary.public_effect_label = "public ritual memory of lock authority";
    } else if (!spec_derived && contains_ci(candidate.id, "scholar_fragment")) {
        summary.public_origin_label = "later cataloging of copied fragments";
        summary.public_effect_label = "archive-visible reinterpretation pressure";
    }

    summary.has_curator_trace = can_view(access, AccessLevel::Curator) && candidate.hidden_mutation_source.has_value();
    return summary;
}

void append_hidden_mutation_source_summary(std::ostringstream& out,
                                           const CandidateFeature& candidate,
                                           AccessLevel access) {
    const HiddenMutationCandidateSourceSummary summary = summarize_hidden_mutation_candidate_source(candidate, access);
    out << "Public source summary:\n";
    out << "- source kind: mutation-derived public evidence candidate\n";
    out << "- public origin: " << summary.public_origin_label << "\n";
    out << "- public effect: " << summary.public_effect_label << "\n";
    out << "- source entity count: " << summary.source_entity_count << "\n";
    out << "- source event count: " << summary.source_event_count << "\n";
    if (can_view(access, AccessLevel::Curator)) {
        out << "- curator trace available: " << (summary.has_curator_trace ? "true" : "false") << "\n";
    }
}

[[nodiscard]] std::string format_hidden_mutation_source_trace(const CandidateFeature& candidate) {
    std::ostringstream out;
    if (!candidate.hidden_mutation_source.has_value()) {
        out << "- no hidden mutation source attached\n";
        return out.str();
    }
    const HiddenMutationArtifactSource& source = *candidate.hidden_mutation_source;
    out << "Hidden mutation source trace:\n";
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
    return out.str();
}

[[nodiscard]] std::string format_candidates_from_hidden_mutation(
    const ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTruthMutationRecord& record,
    const CandidateGenerationRequest& request
) {
    std::ostringstream out;
    out << "Hidden-mutation artifact candidates visible to " << to_string(access) << ":\n";
    out << "- target_topic: " << (record.target_topic.empty() ? topic_or_default(request) : record.target_topic) << "\n";
    out << "- target_year: " << request.target_year << "\n";

    const std::vector<std::string> source_errors = validate_hidden_mutation_artifact_source(state, record);
    if (!source_errors.empty()) {
        out << "- generated_count: 0\n";
        out << "- archive_mutated: false\n";
        out << "- explanation: hidden mutation source failed validation; no public artifact candidates were generated.\n";
        if (can_view(access, AccessLevel::Curator)) {
            out << "Source validation errors:\n";
            for (const std::string& error : source_errors) {
                out << "- " << error << "\n";
            }
        }
        return out.str();
    }

    const std::string before = serialize_for_replay_test(state);
    const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, record, request);
    const std::string after = serialize_for_replay_test(state);
    out << "- generated_count: " << batch.candidates.size() << "\n";
    out << "- archive_mutated: " << (before == after ? "false" : "true") << "\n";
    if (can_view(access, AccessLevel::Curator)) {
        out << "- mutation_record_id: " << record.id << "\n";
        out << "- source_cluster_id: " << record.source_cluster_id << "\n";
    } else {
        out << "- hidden mutation provenance is restricted to curator/canon/debug access\n";
    }

    for (const CandidateFeature& candidate : batch.candidates) {
        const CandidateEvaluation evaluation = evaluate_candidate_feature(state, candidate, access);
        out << "\n";
        out << "Candidate shape: " << shape_public_title(contains_ci(candidate.id, "ritual_notice") ?
            HiddenMutationArtifactCandidateShape::RitualNotice :
            (contains_ci(candidate.id, "scholar_fragment") ? HiddenMutationArtifactCandidateShape::ScholarFragment : HiddenMutationArtifactCandidateShape::AdministrativeDocket)) << "\n";
        append_hidden_mutation_source_summary(out, candidate, access);
        out << format_candidate_evaluation(evaluation, candidate, access);
        if (can_view(access, AccessLevel::Curator)) {
            out << format_hidden_mutation_source_trace(candidate);
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_hidden_mutation_artifact_generation_query(
    ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTimelineClusterRequest& cluster_request,
    const CandidateGenerationRequest& candidate_request
) {
    std::ostringstream out;
    out << "Hidden-mutation artifact generation visible to " << to_string(access) << ":\n";
    out << "- cluster_scope: " << to_string(cluster_request.scope) << "\n";
    out << "- target_topic: " << cluster_request.target_topic << "\n";
    out << "- year_window: " << cluster_request.start_year << "-" << cluster_request.end_year << "\n";

    const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
    const HiddenClusterMaterializationResult materialization = materialize_hidden_timeline_cluster(state, cluster, access);
    out << "- hidden_materialization_mutated: " << (materialization.mutated ? "true" : "false") << "\n";
    out << "- source_decision: " << to_string(materialization.source_decision) << "\n";

    if (!materialization.mutated || materialization.mutation_record_id.empty()) {
        out << "- generated_count: 0\n";
        out << "- archive_artifacts_inserted: false\n";
        out << "- explanation: hidden cluster was not materialized, so no mutation-derived public artifact candidates were generated.\n";
        if (can_view(access, AccessLevel::Curator) && !materialization.validation_errors.empty()) {
            out << "Materialization validation errors:\n";
            for (const std::string& error : materialization.validation_errors) {
                out << "- " << error << "\n";
            }
        }
        if (!can_view(access, AccessLevel::Curator)) {
            out << "- hidden mutation internals are restricted to curator/canon/debug access\n";
        }
        return out.str();
    }

    const auto record_it = std::find_if(state.hidden_truth_mutations.begin(), state.hidden_truth_mutations.end(), [&](const HiddenTruthMutationRecord& record) {
        return record.id == materialization.mutation_record_id;
    });
    if (record_it == state.hidden_truth_mutations.end()) {
        out << "- generated_count: 0\n";
        out << "- archive_artifacts_inserted: false\n";
        out << "- explanation: materialization did not leave an inspectable mutation record; no candidates were generated.\n";
        return out.str();
    }

    out << "- archive_artifacts_inserted: false\n";
    out << "- explanation: hidden mutation was used only as source context for evaluated public artifact candidates; public archive was not mutated.\n\n";
    out << format_candidates_from_hidden_mutation(state, access, *record_it, candidate_request);
    return out.str();
}

namespace {

[[nodiscard]] bool candidate_has_claim_type(const CandidateFeature& candidate, ClaimType type) {
    return std::any_of(candidate.structured_claims.begin(), candidate.structured_claims.end(), [&](const CandidateClaimMetadata& claim) {
        return claim.claim_type == type;
    });
}

[[nodiscard]] bool candidate_has_declared_mediation(const CandidateFeature& candidate, EvidenceModifier modifier) {
    if (!candidate.structured_artifact_metadata.has_value()) {
        return false;
    }
    const std::vector<EvidenceModifier>& mediations = candidate.structured_artifact_metadata->declared_mediations;
    return std::find(mediations.begin(), mediations.end(), modifier) != mediations.end();
}

[[nodiscard]] bool candidate_has_mystery_link(const CandidateFeature& candidate) {
    return std::any_of(candidate.proposed_links.begin(), candidate.proposed_links.end(), [](const std::string& link) {
        return has_prefix(link, "mystery:");
    });
}

[[nodiscard]] bool candidate_is_corrob(const CandidateFeature& candidate, const CandidateEvaluation& evaluation) {
    if (evaluation.decision != CandidateDecision::Accept) {
        return false;
    }
    if (contains_substr(candidate.id, "corroborating_fragment")) {
        return true;
    }
    return candidate_has_claim_type(candidate, ClaimType::FactualClaim) &&
           !candidate_has_claim_type(candidate, ClaimType::MythicCompression) &&
           !candidate_has_declared_mediation(candidate, EvidenceModifier::Forgery);
}

[[nodiscard]] bool candidate_is_ritual_or_ambiguous(const CandidateFeature& candidate, const CandidateEvaluation& evaluation) {
    if (evaluation.decision == CandidateDecision::AcceptAsMythicCompression) {
        return true;
    }
    return contains_substr(candidate.id, "ritual_variant") ||
           candidate_has_claim_type(candidate, ClaimType::MythicCompression) ||
           candidate_has_declared_mediation(candidate, EvidenceModifier::MythicCompression);
}

[[nodiscard]] bool candidate_is_forgery_like(const CandidateFeature& candidate, const CandidateEvaluation& evaluation) {
    return evaluation.decision == CandidateDecision::AcceptAsForgery ||
           contains_substr(candidate.id, "misleading_forgery") ||
           candidate_has_declared_mediation(candidate, EvidenceModifier::Forgery);
}

[[nodiscard]] double ratio(std::size_t count, std::size_t total) {
    if (total == 0U) {
        return 0.0;
    }
    return static_cast<double>(count) / static_cast<double>(total);
}

[[nodiscard]] std::string dossier_assessment_text(std::size_t total,
                                                  std::size_t corroborating,
                                                  std::size_t ritual,
                                                  std::size_t forgery,
                                                  bool touches_mystery,
                                                  double mystery_pressure,
                                                  double originality_balance) {
    if (total == 0U) {
        return "No generated candidates were available for this request.";
    }
    if (corroborating == total) {
        return "Over-confirmation pressure: the dossier contains only corroborating fragments and may be too clean for a damaged archive.";
    }
    if (forgery == total) {
        return "Authenticity instability: the dossier contains only forgery-mediated candidates.";
    }
    if (ritual == total) {
        return "High ambiguity with low factual support: the dossier contains only ritual or mythic variants.";
    }
    if (corroborating > 0U && ritual > 0U && forgery > 0U) {
        std::string text = "Balanced dossier: corroborating, ritual, and forgery-mediated evidence are all present.";
        if (touches_mystery && mystery_pressure >= 0.30) {
            text += " It touches a protected mystery and carries explicit mystery-resolution pressure.";
        }
        if (originality_balance < 0.45) {
            text += " Originality balance is weak and should be reviewed before curator acceptance.";
        }
        return text;
    }
    if (touches_mystery && mystery_pressure >= 0.30) {
        return "Mystery-resolution pressure: the dossier touches a protected mystery but lacks enough ambiguity or forgery pressure to keep the answer unstable.";
    }
    return "Mixed dossier: evidence types vary, but the batch should still be reviewed for corroboration and ambiguity balance.";
}

} // namespace

[[nodiscard]] DossierEvaluation evaluate_generated_dossier(const ArchiveEngineState& state,
                                                          const CandidateGenerationRequest& request,
                                                          AccessLevel access) {
    const GeneratedCandidateBatch batch = generate_candidate_batch(state, request);
    DossierEvaluation evaluation;
    evaluation.request = request;
    evaluation.resolved_target = batch.resolved_target;

    std::size_t corroborating = 0U;
    std::size_t ritual = 0U;
    std::size_t forgery = 0U;
    std::size_t mystery_linked = 0U;
    double specificity_total = 0.0;
    double transformed_total = 0.0;

    for (const CandidateFeature& candidate : batch.candidates) {
        CandidateEvaluation candidate_evaluation = evaluate_candidate_feature(state, candidate, access);
        if (candidate_is_corrob(candidate, candidate_evaluation)) {
            ++corroborating;
        }
        if (candidate_is_ritual_or_ambiguous(candidate, candidate_evaluation)) {
            ++ritual;
        }
        if (candidate_is_forgery_like(candidate, candidate_evaluation)) {
            ++forgery;
        }
        if (candidate_has_mystery_link(candidate)) {
            ++mystery_linked;
        }
        specificity_total += candidate_evaluation.originality.civilization_specificity_score;
        transformed_total += candidate_evaluation.originality.transformed_trope_score;
        evaluation.candidate_evaluations.push_back(std::move(candidate_evaluation));
    }

    const std::size_t total = batch.candidates.size();
    evaluation.corroboration_pressure = ratio(corroborating, total);
    evaluation.ambiguity_pressure = ratio(ritual, total);
    evaluation.forgery_pressure = ratio(forgery, total);

    const bool touches_mystery = evaluation.resolved_target.has_value() && evaluation.resolved_target->mystery_id.has_value();
    if (touches_mystery) {
        const double mystery_link_ratio = ratio(mystery_linked, total);
        evaluation.mystery_resolution_pressure = clamp01(
            0.20 +
            0.45 * evaluation.corroboration_pressure +
            0.25 * mystery_link_ratio -
            0.18 * evaluation.ambiguity_pressure -
            0.12 * evaluation.forgery_pressure
        );
    }

    const double average_specificity = total == 0U ? 0.0 : specificity_total / static_cast<double>(total);
    const double average_transformation = total == 0U ? 0.0 : transformed_total / static_cast<double>(total);
    const std::size_t role_count = (corroborating > 0U ? 1U : 0U) + (ritual > 0U ? 1U : 0U) + (forgery > 0U ? 1U : 0U);
    const double diversity_ratio = static_cast<double>(role_count) / 3.0;
    evaluation.originality_balance = clamp01((average_specificity * 0.45) + (average_transformation * 0.25) + (diversity_ratio * 0.30));

    evaluation.assessment = dossier_assessment_text(
        total,
        corroborating,
        ritual,
        forgery,
        touches_mystery,
        evaluation.mystery_resolution_pressure,
        evaluation.originality_balance
    );
    return evaluation;
}

[[nodiscard]] std::string format_dossier_evaluation(const ArchiveEngineState& state,
                                                    AccessLevel access,
                                                    const CandidateGenerationRequest& request) {
    std::ostringstream out;
    out << "Dossier evaluation visible to " << to_string(access) << ":\n";
    out << "- strategy: " << to_string(request.strategy) << "\n";
    out << "- target_topic: " << topic_or_default(request) << "\n";
    out << "- target_year: " << request.target_year << "\n";

    const std::string before = serialize_for_replay_test(state);
    const DossierEvaluation evaluation = evaluate_generated_dossier(state, request, access);
    const std::string after = serialize_for_replay_test(state);

    if (!evaluation.resolved_target.has_value()) {
        out << "- target_resolution: unresolved\n";
        out << "- generated_count: 0\n";
        out << "- archive_mutated: " << (before == after ? "false" : "true") << "\n";
        out << "- assessment: No dossier could be evaluated because the target topic did not resolve.\n";
        return out.str();
    }

    out << "- target_resolution: " << evaluation.resolved_target->topic;
    if (evaluation.resolved_target->mystery_id.has_value()) {
        out << " -> " << *evaluation.resolved_target->mystery_id;
    }
    out << "\n";
    out << "- generated_count: " << evaluation.candidate_evaluations.size() << "\n";
    out << "- archive_mutated: " << (before == after ? "false" : "true") << "\n";

    if (!can_view(access, AccessLevel::Curator)) {
        out << "- dossier meta-audit internals are restricted to curator/canon/debug access\n";
        return out.str();
    }

    out << "- assessment: " << evaluation.assessment << "\n";
    out << std::fixed << std::setprecision(2);
    out << "Pressure scores:\n";
    out << "- corroboration_pressure=" << evaluation.corroboration_pressure << "\n";
    out << "- ambiguity_pressure=" << evaluation.ambiguity_pressure << "\n";
    out << "- forgery_pressure=" << evaluation.forgery_pressure << "\n";
    out << "- mystery_resolution_pressure=" << evaluation.mystery_resolution_pressure << "\n";
    out << "- originality_balance=" << evaluation.originality_balance << "\n";

    out << "Candidate decisions:\n";
    for (const CandidateEvaluation& candidate_evaluation : evaluation.candidate_evaluations) {
        out << "- " << candidate_evaluation.evaluated_candidate_id
            << ": " << to_string(candidate_evaluation.decision)
            << ", specificity=" << candidate_evaluation.originality.civilization_specificity_score
            << ", transformed=" << candidate_evaluation.originality.transformed_trope_score
            << "\n";
    }
    if (access == AccessLevel::Debug) {
        out << "Debug trace: dossier evaluation is non-mutating and derived from deterministic generated candidate evaluation.\n";
    }
    return out.str();
}

} // namespace archive
