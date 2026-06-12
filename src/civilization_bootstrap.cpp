#include "civilization_bootstrap_api.h"
#include "civilization_specs_api.h"
#include "validation_api.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <utility>

namespace archive {

namespace {

[[nodiscard]] std::string metadata_or_unspecified(std::string_view value) {
    return value.empty() ? std::string{"unspecified"} : std::string{value};
}


[[nodiscard]] std::string title_from_key(std::string_view key) {
    std::string text;
    text.reserve(key.size());
    bool capitalize_next = true;
    for (char ch : key) {
        if (ch == '_') {
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

[[nodiscard]] std::string scoped_id(std::string_view prefix,
                                    std::string_view civilization_id,
                                    std::string_view key) {
    std::string id;
    id.reserve(prefix.size() + civilization_id.size() + key.size() + 2U);
    id.append(prefix);
    id.push_back('.');
    id.append(civilization_id);
    id.push_back('.');
    id.append(key);
    return id;
}

[[nodiscard]] Entity make_bootstrap_entity(std::string id,
                                            EntityType type,
                                            std::string canonical_name,
                                            int earliest_year,
                                            int latest_year) {
    Entity entity;
    entity.id = std::move(id);
    entity.type = type;
    entity.canonical_name = std::move(canonical_name);
    entity.existence_interval = Interval{earliest_year, latest_year};
    entity.min_access = AccessLevel::Canon;
    return entity;
}

[[nodiscard]] bool contains_value(const std::vector<std::string>& values, std::string_view needle) {
    return std::any_of(values.begin(), values.end(), [&](const std::string& value) {
        return value == needle;
    });
}

[[nodiscard]] std::string entity_id_for_authority_actor(const CivilizationSpec& spec, std::string_view actor) {
    if (contains_value(spec.institution_archetypes, actor)) {
        return scoped_id("institution", spec.id, actor);
    }
    if (contains_value(spec.social_actor_archetypes, actor)) {
        return scoped_id("social_actor", spec.id, actor);
    }
    if (contains_value(spec.major_sites, actor)) {
        return scoped_id("site", spec.id, actor);
    }
    return {};
}

[[nodiscard]] std::optional<std::pair<std::string, std::string>> parse_authority_conflict(std::string_view conflict) {
    constexpr std::string_view marker = "_vs_";
    const std::size_t first = conflict.find(marker);
    if (first == std::string_view::npos) {
        return std::nullopt;
    }
    const std::size_t second = conflict.find(marker, first + marker.size());
    if (second != std::string_view::npos) {
        return std::nullopt;
    }
    const std::string left{conflict.substr(0U, first)};
    const std::string right{conflict.substr(first + marker.size())};
    if (left.empty() || right.empty()) {
        return std::nullopt;
    }
    return std::make_pair(left, right);
}

void add_bootstrap_event(HiddenTruthGraph& graph,
                         std::string id,
                         std::string event_type,
                         std::string title,
                         int start_year,
                         int end_year,
                         std::vector<std::string> participants,
                         std::vector<std::string> causes,
                         std::string description) {
    Event event;
    event.id = std::move(id);
    event.event_type = std::move(event_type);
    event.title = std::move(title);
    event.start_year = start_year;
    event.end_year = end_year;
    event.participant_entity_ids = std::move(participants);
    event.cause_event_ids = std::move(causes);
    event.canonical_description = std::move(description);
    event.truth_layer = TruthLayer::CanonicalTruth;
    event.min_access = AccessLevel::Canon;
    graph.add_event(std::move(event));
}

[[nodiscard]] int bounded_event_year(const CivilizationSpec& spec, int offset) {
    const int span = std::max(1, spec.latest_year - spec.earliest_year);
    const int safe_offset = std::min(offset, std::max(0, span - 1));
    return spec.earliest_year + safe_offset;
}

[[nodiscard]] std::string conflict_event_id(const CivilizationSpec& spec, std::size_t index) {
    return scoped_id("event", spec.id, "conflict_" + std::to_string(index));
}

} // namespace

[[nodiscard]] CivilizationBootstrapResult bootstrap_archive_state_from_civilization_spec(
    const CivilizationSpec& spec,
    std::string_view catalog_id,
    std::string_view schema_version
) {
    CivilizationBootstrapResult result;

    const CivilizationSpecValidationResult spec_validation = validate_civilization_spec(spec);
    result.warnings.insert(result.warnings.end(), spec_validation.warnings.begin(), spec_validation.warnings.end());
    if (!spec_validation.valid) {
        result.errors.insert(result.errors.end(), spec_validation.errors.begin(), spec_validation.errors.end());
        return result;
    }

    ArchiveEngineState state;
    state.seed = spec.seed;
    state.civilization_source = CivilizationRuntimeSource{
        spec.id,
        spec.display_name,
        spec.seed,
        spec.earliest_year,
        spec.latest_year,
        std::string(catalog_id),
        std::string(schema_version),
    };

    const std::string civilization_entity_id = "civilization." + spec.id;
    state.hidden_truth.add_entity(make_bootstrap_entity(
        civilization_entity_id,
        EntityType::Faction,
        spec.display_name,
        spec.earliest_year,
        spec.latest_year
    ));

    for (const std::string& feature : spec.geographic_features) {
        state.hidden_truth.add_entity(make_bootstrap_entity(
            scoped_id("geography", spec.id, feature),
            EntityType::Site,
            title_from_key(feature),
            spec.earliest_year,
            spec.latest_year
        ));
    }

    for (const std::string& site : spec.major_sites) {
        state.hidden_truth.add_entity(make_bootstrap_entity(
            scoped_id("site", spec.id, site),
            EntityType::Site,
            title_from_key(site),
            spec.earliest_year,
            spec.latest_year
        ));
    }

    for (const std::string& institution : spec.institution_archetypes) {
        state.hidden_truth.add_entity(make_bootstrap_entity(
            scoped_id("institution", spec.id, institution),
            EntityType::Office,
            title_from_key(institution),
            spec.earliest_year,
            spec.latest_year
        ));
    }

    for (const std::string& actor : spec.social_actor_archetypes) {
        state.hidden_truth.add_entity(make_bootstrap_entity(
            scoped_id("social_actor", spec.id, actor),
            EntityType::Faction,
            title_from_key(actor),
            spec.earliest_year,
            spec.latest_year
        ));
    }

    for (const std::string& trade_good : spec.trade_goods) {
        state.hidden_truth.add_entity(make_bootstrap_entity(
            scoped_id("trade_good", spec.id, trade_good),
            EntityType::Technology,
            title_from_key(trade_good),
            spec.earliest_year,
            spec.latest_year
        ));
    }

    for (const std::string& writing_system : spec.writing_system_archetypes) {
        state.hidden_truth.add_entity(make_bootstrap_entity(
            scoped_id("script", spec.id, writing_system),
            EntityType::Script,
            title_from_key(writing_system),
            spec.earliest_year,
            spec.latest_year
        ));
    }

    state.hidden_truth.add_entity(make_bootstrap_entity(
        scoped_id("language", spec.id, "bootstrap_language"),
        EntityType::Language,
        spec.display_name + " bootstrap language",
        spec.earliest_year,
        spec.latest_year
    ));
    state.hidden_truth.add_entity(make_bootstrap_entity(
        scoped_id("dialect", spec.id, "bootstrap_dialect"),
        EntityType::Dialect,
        spec.display_name + " bootstrap dialect",
        spec.earliest_year,
        spec.latest_year
    ));

    const std::string foundation_event_id = "event." + spec.id + ".foundation";
    add_bootstrap_event(
        state.hidden_truth,
        foundation_event_id,
        "spec_bootstrap_foundation",
        spec.display_name + " configured from CivilizationSpec",
        spec.earliest_year,
        spec.earliest_year,
        {civilization_entity_id},
        {},
        "Spec bootstrap establishes the minimal hidden civilization entity and chronology boundary."
    );

    std::vector<std::string> institution_event_ids;
    institution_event_ids.reserve(spec.institution_archetypes.size());
    for (std::size_t index = 0; index < spec.institution_archetypes.size(); ++index) {
        const std::string& institution = spec.institution_archetypes[index];
        const std::string event_id = scoped_id("event." + spec.id, "institution", institution);
        institution_event_ids.push_back(event_id);
        const int year = bounded_event_year(spec, 1 + static_cast<int>(index));
        add_bootstrap_event(
            state.hidden_truth,
            event_id,
            "spec_bootstrap_institution_formation",
            title_from_key(institution) + " enters the active authority map",
            year,
            year,
            {civilization_entity_id, scoped_id("institution", spec.id, institution)},
            {foundation_event_id},
            "Spec bootstrap converts an institution archetype into a minimal hidden authority actor."
        );
    }

    for (std::size_t index = 0; index < spec.authority_conflicts.size(); ++index) {
        const std::string& conflict = spec.authority_conflicts[index];
        const auto parsed = parse_authority_conflict(conflict);
        if (!parsed.has_value()) {
            result.errors.push_back("authority conflict became malformed during bootstrap: " + conflict);
            return result;
        }
        const std::string left_id = entity_id_for_authority_actor(spec, parsed->first);
        const std::string right_id = entity_id_for_authority_actor(spec, parsed->second);
        if (left_id.empty() || right_id.empty()) {
            result.errors.push_back("authority conflict participant could not be resolved during bootstrap: " + conflict);
            return result;
        }
        std::vector<std::string> causes{foundation_event_id};
        if (!institution_event_ids.empty()) {
            causes.push_back(institution_event_ids.front());
        }
        const int year = bounded_event_year(spec, 10 + static_cast<int>(index));
        add_bootstrap_event(
            state.hidden_truth,
            conflict_event_id(spec, index),
            "spec_bootstrap_authority_conflict",
            title_from_key(parsed->first) + " contests " + title_from_key(parsed->second),
            year,
            year,
            {left_id, right_id},
            causes,
            "Spec bootstrap records an authority-conflict pressure without resolving it into full public evidence."
        );
    }

    std::vector<std::string> recordkeeping_participants{civilization_entity_id};
    if (!spec.writing_system_archetypes.empty()) {
        recordkeeping_participants.push_back(scoped_id("script", spec.id, spec.writing_system_archetypes.front()));
    }
    const std::string recordkeeping_event_id = "event." + spec.id + ".recordkeeping";
    add_bootstrap_event(
        state.hidden_truth,
        recordkeeping_event_id,
        "spec_bootstrap_recordkeeping",
        "Recordkeeping pressure emerges from " + spec.display_name,
        bounded_event_year(spec, 20),
        bounded_event_year(spec, 20),
        recordkeeping_participants,
        {foundation_event_id},
        "Spec bootstrap seeds the future evidence layer using writing and recordkeeping archetypes."
    );

    const std::string mystery_seed_event_id = "event." + spec.id + ".mystery_seed";
    add_bootstrap_event(
        state.hidden_truth,
        mystery_seed_event_id,
        "spec_bootstrap_mystery_seed",
        "Protected mystery pressure is seeded for " + spec.display_name,
        bounded_event_year(spec, 30),
        bounded_event_year(spec, 30),
        {civilization_entity_id},
        {recordkeeping_event_id},
        "Spec bootstrap marks mystery archetypes as future generation pressure without creating public mysteries yet."
    );

    const std::vector<std::string> validation_errors = validate_full_state(state);
    if (!validation_errors.empty()) {
        result.errors.insert(result.errors.end(), validation_errors.begin(), validation_errors.end());
        return result;
    }

    result.state = state;
    result.ok = true;
    result.summary = format_civilization_bootstrap_summary(result.state, AccessLevel::Public);
    return result;
}

[[nodiscard]] std::string format_civilization_bootstrap_summary(const ArchiveEngineState& state, AccessLevel access) {
    std::ostringstream out;
    out << "Civilization bootstrap:\n";
    if (!state.civilization_source.has_value()) {
        out << "- runtime: fixed fixture regression mode\n";
        out << "- validation: " << (validate_full_state(state).empty() ? "passed" : "failed") << "\n";
        return out.str();
    }

    const CivilizationRuntimeSource& source = *state.civilization_source;
    const std::vector<std::string> errors = validate_full_state(state);
    out << "- runtime: spec-selected\n";
    out << "- civilization_id: " << source.civilization_id << "\n";
    out << "- civilization: " << source.display_name << "\n";
    out << "- display_name: " << source.display_name << "\n";
    out << "- catalog_id: " << metadata_or_unspecified(source.catalog_id) << "\n";
    out << "- schema_version: " << metadata_or_unspecified(source.schema_version) << "\n";
    out << "- year_range: " << source.earliest_year << "-" << source.latest_year << "\n";
    out << "- hidden entities: " << state.hidden_truth.entities().size() << "\n";
    out << "- hidden events: " << state.hidden_truth.events().size() << "\n";
    out << "- public artifacts: " << state.public_archive.artifacts().size() << "\n";
    out << "- mysteries: " << state.mysteries.size() << "\n";
    out << "- validation: " << (errors.empty() ? "passed" : "failed") << "\n";

    out << "- major sites:\n";
    for (const auto& [id, entity] : state.hidden_truth.entities()) {
        if (has_prefix(id, "site." + source.civilization_id + ".")) {
            out << "  - " << entity.canonical_name << "\n";
        }
    }

    if (!errors.empty()) {
        if (can_view(access, AccessLevel::Curator)) {
            out << "- validation errors:\n";
            for (const std::string& error : errors) {
                out << "  - " << error << "\n";
            }
        } else {
            out << "- validation details: restricted\n";
        }
    }

    if (can_view(access, AccessLevel::Curator)) {
        out << "Bootstrap trace:\n";
        out << "- catalog_id: " << metadata_or_unspecified(source.catalog_id) << "\n";
        out << "- schema_version: " << metadata_or_unspecified(source.schema_version) << "\n";
        out << "- seed: " << source.seed << "\n";
        out << "- generated hidden entity IDs:\n";
        for (const auto& [id, entity] : state.hidden_truth.entities()) {
            (void)entity;
            out << "  - " << id << "\n";
        }
        out << "- generated hidden event IDs:\n";
        for (const auto& [id, event] : state.hidden_truth.events()) {
            out << "  - " << id << ": " << event.title << "\n";
        }
        out << "- authority conflict event mapping:\n";
        for (const auto& [id, event] : state.hidden_truth.events()) {
            if (event.event_type == "spec_bootstrap_authority_conflict") {
                out << "  - " << id << " participants:";
                for (const std::string& participant_id : event.participant_entity_ids) {
                    out << " " << participant_id;
                }
                out << "\n";
            }
        }
    }

    return out.str();
}

} // namespace archive
