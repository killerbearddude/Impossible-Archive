#include "golden_fixtures_api.h"
#include "archive_views_api.h"
#include "civilization_bootstrap_api.h"
#include "civilization_specs_api.h"
#include "civilization_fragments_api.h"
#include "validation_api.h"
#include "evidence_potential_api.h"
#include "candidate_artifact_plan_api.h"
#include "candidate_artifact_plan_evaluation_api.h"
#include "candidate_artifact_proposal_api.h"
#include "candidate_artifact_proposal_audit_api.h"
#include "control_layer_audit_api.h"

#include <sstream>

namespace archive {

namespace {

[[nodiscard]] const std::vector<GoldenFixtureWorldDefinition>& fixture_registry() {
    static const std::vector<GoldenFixtureWorldDefinition> fixtures{
        GoldenFixtureWorldDefinition{
            "fixture.default_archive",
            "Default spec-selected marsh_citadel archive using the bundled v1.1 catalog.",
            "spec-selected",
            "examples/40_civilization_specs_v1_1.json",
            "marsh_citadel",
            42U,
            kOpenEndedYear,
        },
        GoldenFixtureWorldDefinition{
            "fixture.marsh_citadel_seeded",
            "Spec-selected marsh_citadel archive with a fixed regression seed and bounded archive year.",
            "spec-selected",
            "examples/40_civilization_specs_v1_1.json",
            "marsh_citadel",
            2713U,
            625,
        },
        GoldenFixtureWorldDefinition{
            "fixture.fragment_catalog_only",
            "Fragment-intake fixture that loads v1.2 fragment records as inert catalog data only.",
            "spec-selected",
            "examples/v12_fragment_examples.json",
            "marsh_citadel",
            42U,
            kOpenEndedYear,
        },
    };
    return fixtures;
}

[[nodiscard]] bool is_fixed_fixture(const GoldenFixtureWorldDefinition& definition) {
    return definition.runtime_mode == "fixed-fixture";
}

} // namespace

[[nodiscard]] std::vector<GoldenFixtureWorldDefinition> list_golden_fixture_worlds() {
    return fixture_registry();
}

[[nodiscard]] const GoldenFixtureWorldDefinition* find_golden_fixture_world(std::string_view fixture_id) {
    const std::vector<GoldenFixtureWorldDefinition>& fixtures = fixture_registry();
    const auto it = std::find_if(fixtures.begin(), fixtures.end(), [&](const GoldenFixtureWorldDefinition& fixture) {
        return fixture.id == fixture_id;
    });
    if (it == fixtures.end()) {
        return nullptr;
    }
    return &*it;
}

[[nodiscard]] GoldenFixtureBuildResult build_golden_fixture_world(const GoldenFixtureWorldDefinition& definition) {
    GoldenFixtureBuildResult result;
    result.definition = definition;

    if (is_fixed_fixture(definition)) {
        result.state = initialize_archive_engine(definition.seed);
        derive_evidence_potentials_into_state(result.state);
        derive_candidate_artifact_plans_into_state(result.state, AccessLevel::Curator);
        evaluate_candidate_artifact_plans_into_state(result.state, AccessLevel::Curator);
        draft_candidate_artifact_proposals_into_state(result.state, AccessLevel::Curator);
        audit_candidate_artifact_proposals_into_state(result.state, AccessLevel::Curator);
        build_control_layer_audit_into_state(result.state);
        result.ok = validate_full_state(result.state).empty();
        if (!result.ok) {
            result.errors = validate_full_state(result.state);
        }
        return result;
    }

    if (definition.runtime_mode != "spec-selected") {
        result.errors.push_back("unsupported golden fixture runtime_mode: " + definition.runtime_mode);
        return result;
    }

    const CivilizationSpecLoadResult load = load_civilization_specs_from_json_file(definition.spec_file);
    if (!load.ok()) {
        result.errors = load.errors;
        result.warnings = load.warnings;
        return result;
    }

    const CivilizationSpecValidationResult catalog_validation = validate_civilization_catalog(load.catalog);
    result.warnings.insert(result.warnings.end(), catalog_validation.warnings.begin(), catalog_validation.warnings.end());
    if (!catalog_validation.valid) {
        result.errors = catalog_validation.errors;
        return result;
    }

    const CivilizationSpecValidationResult fragment_validation = validate_civilization_fragments(load.catalog);
    result.warnings.insert(result.warnings.end(), fragment_validation.warnings.begin(), fragment_validation.warnings.end());
    for (const std::string& error : fragment_validation.errors) {
        result.warnings.push_back("inert fragment validation issue ignored during fixture runtime bootstrap: " + error);
    }

    const CivilizationSpec* spec = find_civilization_spec(load.catalog, definition.civilization_id);
    if (spec == nullptr) {
        result.errors.push_back("CivilizationSpec not found for golden fixture: " + definition.civilization_id);
        return result;
    }

    const CivilizationBootstrapResult bootstrap = bootstrap_archive_state_from_civilization_spec(
        *spec,
        load.catalog.catalog_id,
        load.catalog.schema_version
    );
    result.warnings.insert(result.warnings.end(), bootstrap.warnings.begin(), bootstrap.warnings.end());
    if (!bootstrap.ok) {
        result.errors = bootstrap.errors;
        return result;
    }

    result.state = bootstrap.state;
    derive_evidence_potentials_into_state(result.state);
    derive_candidate_artifact_plans_into_state(result.state, AccessLevel::Curator);
    evaluate_candidate_artifact_plans_into_state(result.state, AccessLevel::Curator);
    draft_candidate_artifact_proposals_into_state(result.state, AccessLevel::Curator);
    audit_candidate_artifact_proposals_into_state(result.state, AccessLevel::Curator);
        build_control_layer_audit_into_state(result.state);
    result.state.civilization_spec_count = load.catalog.civilizations.size();
    result.state.civilization_fragment_count = load.catalog.fragments.size();
    result.ok = true;
    return result;
}

[[nodiscard]] GoldenFixtureBuildResult build_golden_fixture_world(std::string_view fixture_id) {
    const GoldenFixtureWorldDefinition* definition = find_golden_fixture_world(fixture_id);
    if (definition == nullptr) {
        GoldenFixtureBuildResult result;
        result.errors.push_back("Golden fixture not found: " + std::string(fixture_id));
        return result;
    }
    return build_golden_fixture_world(*definition);
}

[[nodiscard]] std::string format_golden_fixture_worlds() {
    std::ostringstream out;
    out << "Golden fixture worlds:\n";
    for (const GoldenFixtureWorldDefinition& fixture : fixture_registry()) {
        out << "- " << fixture.id << ": " << fixture.description << "\n";
        out << "  runtime_mode: " << fixture.runtime_mode << "\n";
        if (!fixture.civilization_id.empty()) {
            out << "  civilization_id: " << fixture.civilization_id << "\n";
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_golden_fixture_world(const GoldenFixtureWorldDefinition& definition) {
    std::ostringstream out;
    out << "Golden fixture world:\n";
    out << "- id: " << definition.id << "\n";
    out << "- description: " << definition.description << "\n";
    out << "- runtime_mode: " << definition.runtime_mode << "\n";
    out << "- spec_file: " << (definition.spec_file.empty() ? std::string{"none"} : definition.spec_file) << "\n";
    out << "- civilization_id: " << (definition.civilization_id.empty() ? std::string{"none"} : definition.civilization_id) << "\n";
    out << "- fixture_seed: " << definition.seed << "\n";
    out << "- fixture_archive_year: " << archive_year_text(definition.archive_year) << "\n";
    return out.str();
}

} // namespace archive
