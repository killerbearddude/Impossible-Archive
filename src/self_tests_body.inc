/*
 * Regression suite for invariants across access control, validation, generation, materialization, and formatting.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {
namespace {


void run_v27_runtime_default_selection_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const std::string catalog_path = "examples/40_civilization_specs_v1_1.json";

    require(parse_archive_runtime_mode("fixed-fixture") == ArchiveRuntimeMode::FixedFixture,
            "v27 should parse explicit fixed-fixture runtime mode");
    require(parse_archive_runtime_mode("spec-selected") == ArchiveRuntimeMode::SpecSelected,
            "v27 should parse explicit spec-selected runtime mode");
    require(default_runtime_config().default_spec_file == catalog_path &&
            default_runtime_config().default_civilization_id == "marsh_citadel",
            "v27 should expose bundled default spec runtime configuration");

    CliOptions default_spec;
    default_spec.query = "list-generation-targets";
    const RuntimeStateSelectionResult default_result = build_runtime_state_for_query(default_spec);
    require(default_result.ok && default_result.spec_selected() &&
            default_result.selection.spec_file == catalog_path &&
            default_result.selection.civilization_id == "marsh_citadel" &&
            default_result.state.civilization_source.has_value() &&
            default_result.state.civilization_source->civilization_id == "marsh_citadel",
            "v27 no runtime flags should select bundled marsh_citadel spec runtime by default");

    CliOptions explicit_fixed;
    explicit_fixed.query = "list-generation-targets";
    explicit_fixed.runtime_selection.mode = ArchiveRuntimeMode::FixedFixture;
    explicit_fixed.runtime_selection.explicit_runtime_mode = true;
    const RuntimeStateSelectionResult fixed_result = build_runtime_state_for_query(explicit_fixed);
    require(fixed_result.ok && !fixed_result.spec_selected() && !fixed_result.state.civilization_source.has_value(),
            "v27 explicit fixed-fixture mode should preserve fixed regression runtime");

    CliOptions spec_runtime_civ_only;
    spec_runtime_civ_only.query = "list-generation-targets";
    spec_runtime_civ_only.runtime_selection.mode = ArchiveRuntimeMode::SpecSelected;
    spec_runtime_civ_only.runtime_selection.explicit_runtime_mode = true;
    spec_runtime_civ_only.civilization_id = "ash_steppe";
    const RuntimeStateSelectionResult ash_result = build_runtime_state_for_query(spec_runtime_civ_only);
    require(ash_result.ok && ash_result.spec_selected() &&
            ash_result.selection.spec_file == catalog_path &&
            ash_result.selection.civilization_id == "ash_steppe" &&
            ash_result.state.civilization_source.has_value() &&
            ash_result.state.civilization_source->civilization_id == "ash_steppe",
            "v27 spec-selected runtime with only --civilization-id should use the bundled catalog");

    CliOptions partial;
    partial.query = "generate-candidates";
    partial.spec_file = catalog_path;
    const RuntimeStateSelectionResult partial_result = build_runtime_state_for_query(partial);
    require(!partial_result.ok && partial_result.usage_error &&
            !partial_result.errors.empty() && contains_substr(partial_result.errors.front(), "--spec-file requires --civilization-id"),
            "v27 should reject custom --spec-file without --civilization-id");

    CliOptions unsupported;
    unsupported.query = "candidate";
    const RuntimeStateSelectionResult unsupported_result = build_runtime_state_for_query(unsupported);
    require(!unsupported_result.ok && unsupported_result.usage_error &&
            !unsupported_result.errors.empty() && contains_substr(unsupported_result.errors.front(), "not supported"),
            "v27 default spec runtime should reject unsupported fixed-fixture-specific query combinations");

    const std::string default_targets = format_generation_targets_for_state(default_result.state, AccessLevel::Public);
    require(contains_substr(default_targets, "runtime: spec-selected") &&
            contains_substr(default_targets, "civilization_id: marsh_citadel") &&
            contains_substr(default_targets, "catalog_id: example.civilization_specs.v1_1.40") &&
            !contains_substr(default_targets, "site.reservoir_gate") &&
            !contains_substr(default_targets, "office.drowned_chancellor"),
            "v27 default target list should identify spec runtime and avoid fixed-fixture IDs");
}


void run_v27_1_civilization_spec_metadata_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    auto valid_spec_body = [](const std::string& metadata) {
        return std::string("{\"id\":\"metadata_test\",") +
            "\"display_name\":\"Metadata Test\"," +
            "\"description\":\"A compact valid civilization recipe for v27.1 metadata seam regression.\"," +
            metadata +
            "\"earliest_year\":100,\"latest_year\":700," +
            "\"geographic_features\":[\"river_delta\",\"salt_marsh\",\"raised_mounds\",\"canal_grid\"]," +
            "\"environmental_pressures\":[\"seasonal_flooding\",\"canal_silting\",\"drought_cycles\"]," +
            "\"major_sites\":[\"central_citadel\",\"lower_gate\",\"reed_shrine\",\"grain_quay\"]," +
            "\"economic_pressures\":[\"grain_tax\",\"canal_control\",\"dock_tolls\"]," +
            "\"trade_goods\":[\"grain\",\"salt\",\"reed_matting\",\"dried_fish\"]," +
            "\"institution_archetypes\":[\"water_office\",\"ritual_court\",\"merchant_house\",\"grain_tax_house\"]," +
            "\"social_actor_archetypes\":[\"marsh_clans\",\"dock_families\"]," +
            "\"authority_conflicts\":[\"water_office_vs_ritual_court\",\"merchant_house_vs_grain_tax_house\"]," +
            "\"religious_or_mythic_archetypes\":[\"flood_ancestor_cult\",\"reed_oracle\",\"drowned_founder_myth\"]," +
            "\"ritual_pressures\":[\"annual_gate_opening\",\"flood_appeasement\"]," +
            "\"writing_system_archetypes\":[\"reed_notches\",\"clay_seal_marks\"]," +
            "\"recordkeeping_styles\":[\"grain_ledger\",\"labor_roster\",\"boundary_stone\",\"ritual_song\"]," +
            "\"artifact_media\":[\"clay_tablet\",\"reed_ledger\",\"boundary_stone\",\"oral_song\"]," +
            "\"evidence_distortion_modes\":[\"flood_damage\",\"ritual_compression\",\"political_forgery\",\"calendar_drift\"]," +
            "\"mystery_archetypes\":[\"disputed_founder_identity\",\"missing_office_origin\",\"contradictory_flood_date\",\"forged_tax_decree\"]," +
            "\"target_hidden_entity_count\":18,\"target_hidden_event_count\":24," +
            "\"target_public_artifact_count\":16,\"target_mystery_count\":5,\"seed\":8801}";
    };
    auto catalog_with = [](const std::string& spec_body) {
        return std::string("{\"schema_version\":\"1.1\",\"catalog_id\":\"metadata_test_catalog\",\"civilizations\":[") + spec_body + "]}";
    };

    const CivilizationSpecLoadResult legacy_load = load_civilization_specs_from_json_text(catalog_with(valid_spec_body("")));
    require(legacy_load.ok() && legacy_load.catalog.civilizations.size() == 1U &&
            validate_civilization_catalog(legacy_load.catalog).valid,
            "v27.1 legacy v1.1 spec without tags/profile should remain valid");
    const std::string legacy_summary = format_civilization_spec_summary(legacy_load.catalog.civilizations.front());
    require(contains_substr(legacy_summary, "- tags: none") &&
            contains_substr(legacy_summary, "- profile: unspecified"),
            "v27.1 missing tags/profile should format as none/unspecified");

    const std::string metadata =
        "\"tags\":[\"river_delta\",\"urban\",\"water_access\"],"
        "\"profile\":{\"target_depth\":\"prototype\",\"future_modules\":[\"fragment_composition\",\"language_drift\",\"artifact_voice\"]},";
    const CivilizationSpecLoadResult metadata_load = load_civilization_specs_from_json_text(catalog_with(valid_spec_body(metadata)));
    require(metadata_load.ok() && metadata_load.catalog.civilizations.front().tags.size() == 3U &&
            metadata_load.catalog.civilizations.front().profile.has_value() &&
            validate_civilization_catalog(metadata_load.catalog).valid,
            "v27.1 tags/profile should load and validate as optional metadata");
    const std::string metadata_summary = format_civilization_spec_summary(metadata_load.catalog.civilizations.front());
    require(contains_substr(metadata_summary, "river_delta, urban, water_access") &&
            contains_substr(metadata_summary, "target_depth=prototype") &&
            contains_substr(metadata_summary, "fragment_composition, language_drift, artifact_voice"),
            "v27.1 show-civilization-spec summary should display tags and profile metadata");

    const CivilizationSpecLoadResult duplicate_tags_load = load_civilization_specs_from_json_text(
        catalog_with(valid_spec_body("\"tags\":[\"urban\",\"urban\",\"RiverDelta\"],")));
    const CivilizationSpecValidationResult duplicate_tags_validation = validate_civilization_catalog(duplicate_tags_load.catalog);
    require(duplicate_tags_load.ok() && duplicate_tags_validation.valid && duplicate_tags_validation.warnings.size() >= 2U,
            "v27.1 duplicate/non-standard tags should warn but remain valid");

    const CivilizationSpecLoadResult wrong_tags_type = load_civilization_specs_from_json_text(
        catalog_with(valid_spec_body("\"tags\":\"urban\",")));
    require(!wrong_tags_type.ok(),
            "v27.1 tags should reject wrong non-array type during load");

    const CivilizationSpecLoadResult wrong_profile_type = load_civilization_specs_from_json_text(
        catalog_with(valid_spec_body("\"profile\":\"prototype\",")));
    require(!wrong_profile_type.ok(),
            "v27.1 profile should reject wrong non-object type during load");

    const CivilizationSpecLoadResult unknown_depth_load = load_civilization_specs_from_json_text(
        catalog_with(valid_spec_body("\"profile\":{\"target_depth\":\"experimental\",\"future_modules\":[\"artifact_voice\"]},")));
    const CivilizationSpecValidationResult unknown_depth_validation = validate_civilization_catalog(unknown_depth_load.catalog);
    require(unknown_depth_load.ok() && unknown_depth_validation.valid && !unknown_depth_validation.warnings.empty(),
            "v27.1 unknown profile target_depth should warn but remain valid");

    const CivilizationSpecLoadResult duplicate_modules_load = load_civilization_specs_from_json_text(
        catalog_with(valid_spec_body("\"profile\":{\"target_depth\":\"prototype\",\"future_modules\":[\"artifact_voice\",\"artifact_voice\",\"future_unknown\"]},")));
    const CivilizationSpecValidationResult duplicate_modules_validation = validate_civilization_catalog(duplicate_modules_load.catalog);
    require(duplicate_modules_load.ok() && duplicate_modules_validation.valid && duplicate_modules_validation.warnings.size() >= 2U,
            "v27.1 duplicate/unknown future_modules should warn but remain valid");

    const CivilizationBootstrapResult legacy_bootstrap = bootstrap_archive_state_from_civilization_spec(
        legacy_load.catalog.civilizations.front(),
        legacy_load.catalog.catalog_id,
        legacy_load.catalog.schema_version
    );
    const CivilizationBootstrapResult metadata_bootstrap = bootstrap_archive_state_from_civilization_spec(
        metadata_load.catalog.civilizations.front(),
        metadata_load.catalog.catalog_id,
        metadata_load.catalog.schema_version
    );
    require(legacy_bootstrap.ok && metadata_bootstrap.ok &&
            serialize_for_replay_test(legacy_bootstrap.state) == serialize_for_replay_test(metadata_bootstrap.state),
            "v27.1 tags/profile should not affect bootstrap or generation state behavior");
}


void run_v27_2_catalog_tag_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const CivilizationSpecLoadResult bundled_load = load_civilization_specs_from_json_file(default_runtime_config().default_spec_file);
    require(bundled_load.ok(), "v27.2 bundled catalog should load for tag inspection tests");
    if (!bundled_load.ok()) {
        return;
    }
    const CivilizationSpecValidationResult validation = validate_civilization_catalog(bundled_load.catalog);
    require(validation.valid,
            "v27.2 bundled catalog should validate after metadata expansion");
    require(bundled_load.catalog.civilizations.size() >= 10U,
            "v27.2 bundled catalog should remain a dynamic 1+ spec catalog with enough specs for the quality matrix");

    const std::vector<std::string> tags = collect_civilization_catalog_tags(bundled_load.catalog);
    require(std::find(tags.begin(), tags.end(), "river_delta") != tags.end() &&
            std::find(tags.begin(), tags.end(), "artifact_rich") != tags.end() &&
            std::find(tags.begin(), tags.end(), "protected_ambiguity") != tags.end(),
            "v27.2 tag collection should return sorted unique catalog tags");

    const std::vector<const CivilizationSpec*> river_delta_specs = find_civilization_specs_by_tag(bundled_load.catalog, "river_delta");
    require(std::any_of(river_delta_specs.begin(), river_delta_specs.end(), [](const CivilizationSpec* spec) {
                return spec != nullptr && spec->id == "marsh_citadel";
            }),
            "v27.2 tag lookup should find marsh_citadel by river_delta");
    require(find_civilization_specs_by_tag(bundled_load.catalog, "tag_not_present_in_catalog").empty(),
            "v27.2 unknown tag lookup should return an empty result without errors");

    const std::string tag_summary = format_civilization_catalog_tags(bundled_load.catalog);
    require(contains_substr(tag_summary, "CivilizationSpec tags:") &&
            contains_substr(tag_summary, "river_delta:") &&
            contains_substr(tag_summary, "artifact_rich:"),
            "v27.2 tag summary should show tag counts");

    const std::string tagged_specs = format_civilization_specs_by_tag(bundled_load.catalog, "river_delta");
    require(contains_substr(tagged_specs, "Civilizations tagged river_delta:") &&
            contains_substr(tagged_specs, "marsh_citadel"),
            "v27.2 list-civilizations-by-tag formatter should show matching specs");
    const std::string missing_tag_specs = format_civilization_specs_by_tag(bundled_load.catalog, "tag_not_present_in_catalog");
    require(contains_substr(missing_tag_specs, "- none"),
            "v27.2 list-civilizations-by-tag formatter should show a clean empty result");

    std::size_t specs_with_metadata = 0U;
    for (const CivilizationSpec& spec : bundled_load.catalog.civilizations) {
        if (!spec.tags.empty() && spec.profile.has_value()) {
            ++specs_with_metadata;
        }
    }
    require(specs_with_metadata == bundled_load.catalog.civilizations.size(),
            "v27.2 bundled catalog should provide tags/profile for every bundled spec");

    const std::string tag_validation = format_civilization_catalog_tag_validation(bundled_load.catalog, validation);
    require(contains_substr(tag_validation, "CivilizationSpec tag validation:") &&
            contains_substr(tag_validation, "- unique tags:"),
            "v27.2 tag validation formatter should summarize loaded specs, unique tags, warnings, and errors");

    const std::string duplicate_tag_catalog =
        "{\"schema_version\":\"1.1\",\"catalog_id\":\"tag_warning_test\",\"civilizations\":[{"
        "\"id\":\"tag_warning_test\","
        "\"display_name\":\"Tag Warning Test\","
        "\"description\":\"A compact valid civilization recipe for v27.2 tag warning regression.\","
        "\"tags\":[\"urban\",\"urban\"],"
        "\"profile\":{\"target_depth\":\"prototype\",\"future_modules\":[\"artifact_voice\"]},"
        "\"earliest_year\":100,\"latest_year\":700,"
        "\"geographic_features\":[\"river_delta\",\"salt_marsh\",\"raised_mounds\",\"canal_grid\"],"
        "\"environmental_pressures\":[\"seasonal_flooding\",\"canal_silting\",\"drought_cycles\"],"
        "\"major_sites\":[\"central_citadel\",\"lower_gate\",\"reed_shrine\",\"grain_quay\"],"
        "\"economic_pressures\":[\"grain_tax\",\"canal_control\",\"dock_tolls\"],"
        "\"trade_goods\":[\"grain\",\"salt\",\"reed_matting\",\"dried_fish\"],"
        "\"institution_archetypes\":[\"water_office\",\"ritual_court\",\"merchant_house\",\"grain_tax_house\"],"
        "\"social_actor_archetypes\":[\"marsh_clans\",\"dock_families\"],"
        "\"authority_conflicts\":[\"water_office_vs_ritual_court\",\"merchant_house_vs_grain_tax_house\"],"
        "\"religious_or_mythic_archetypes\":[\"flood_ancestor_cult\",\"reed_oracle\",\"drowned_founder_myth\"],"
        "\"ritual_pressures\":[\"annual_gate_opening\",\"flood_appeasement\"],"
        "\"writing_system_archetypes\":[\"reed_notches\",\"clay_seal_marks\"],"
        "\"recordkeeping_styles\":[\"grain_ledger\",\"labor_roster\",\"boundary_stone\",\"ritual_song\"],"
        "\"artifact_media\":[\"clay_tablet\",\"reed_ledger\",\"boundary_stone\",\"oral_song\"],"
        "\"evidence_distortion_modes\":[\"flood_damage\",\"ritual_compression\",\"political_forgery\",\"calendar_drift\"],"
        "\"mystery_archetypes\":[\"disputed_founder_identity\",\"missing_office_origin\",\"contradictory_flood_date\",\"forged_tax_decree\"],"
        "\"target_hidden_entity_count\":18,\"target_hidden_event_count\":24,"
        "\"target_public_artifact_count\":16,\"target_mystery_count\":5,\"seed\":8810}]}";
    const CivilizationSpecLoadResult duplicate_tag_load = load_civilization_specs_from_json_text(duplicate_tag_catalog);
    const CivilizationSpecValidationResult duplicate_tag_validation = validate_civilization_catalog(duplicate_tag_load.catalog);
    require(duplicate_tag_load.ok() && duplicate_tag_validation.valid && !duplicate_tag_validation.warnings.empty(),
            "v27.2 duplicate catalog tags should remain warnings rather than hard errors");
}

void run_v26_5_originality_materialization_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const std::string valid_spec =
        "{\"schema_version\":\"1.1\",\"catalog_id\":\"quality_test_catalog\",\"civilizations\":[{"
        "\"id\":\"quality_marsh\","
        "\"display_name\":\"Quality Marsh\","
        "\"description\":\"A compact valid civilization recipe for v26.5 originality gate regression.\","
        "\"earliest_year\":100,\"latest_year\":700,"
        "\"geographic_features\":[\"river_delta\",\"salt_marsh\",\"raised_mounds\",\"canal_grid\"],"
        "\"environmental_pressures\":[\"seasonal_flooding\",\"canal_silting\",\"drought_cycles\"],"
        "\"major_sites\":[\"central_citadel\",\"lower_gate\",\"reed_shrine\",\"grain_quay\"],"
        "\"economic_pressures\":[\"grain_tax\",\"canal_control\",\"dock_tolls\"],"
        "\"trade_goods\":[\"grain\",\"salt\",\"reed_matting\",\"dried_fish\"],"
        "\"institution_archetypes\":[\"water_office\",\"ritual_court\",\"merchant_house\",\"grain_tax_house\"],"
        "\"social_actor_archetypes\":[\"marsh_clans\",\"dock_families\"],"
        "\"authority_conflicts\":[\"water_office_vs_ritual_court\",\"merchant_house_vs_grain_tax_house\"],"
        "\"religious_or_mythic_archetypes\":[\"flood_ancestor_cult\",\"reed_oracle\",\"drowned_founder_myth\"],"
        "\"ritual_pressures\":[\"annual_gate_opening\",\"flood_appeasement\"],"
        "\"writing_system_archetypes\":[\"reed_notches\",\"clay_seal_marks\"],"
        "\"recordkeeping_styles\":[\"grain_ledger\",\"labor_roster\",\"boundary_stone\",\"ritual_song\"],"
        "\"artifact_media\":[\"clay_tablet\",\"reed_ledger\",\"boundary_stone\",\"oral_song\"],"
        "\"evidence_distortion_modes\":[\"flood_damage\",\"ritual_compression\",\"political_forgery\",\"calendar_drift\"],"
        "\"mystery_archetypes\":[\"disputed_founder_identity\",\"missing_office_origin\",\"contradictory_flood_date\",\"forged_tax_decree\"],"
        "\"target_hidden_entity_count\":18,\"target_hidden_event_count\":24,"
        "\"target_public_artifact_count\":16,\"target_mystery_count\":5,\"seed\":7701}]}";

    const CivilizationSpecLoadResult load = load_civilization_specs_from_json_text(valid_spec);
    if (!load.ok() || load.catalog.civilizations.empty()) {
        require(false, "v26.5 originality/materialization test fixture should load");
        return;
    }
    const CivilizationBootstrapResult bootstrap = bootstrap_archive_state_from_civilization_spec(
        load.catalog.civilizations.front(),
        load.catalog.catalog_id,
        load.catalog.schema_version
    );
    if (!bootstrap.ok) {
        require(false, "v26.5 originality/materialization test fixture should bootstrap");
        return;
    }

    ArchiveEngineState state = bootstrap.state;
    const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "authority_conflict_0", 250, 380, 42};
    const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
    const HiddenClusterMaterializationResult hidden = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
    if (!hidden.mutated || state.hidden_truth_mutations.empty()) {
        require(false, "v26.5 originality/materialization test fixture should create one hidden mutation");
        return;
    }

    const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 390, "authority_conflict_0", 42};
    const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, state.hidden_truth_mutations.front(), candidate_request);
    const auto ritual_it = std::find_if(batch.candidates.begin(), batch.candidates.end(), [](const CandidateFeature& candidate) {
        return contains_substr(candidate.id, "ritual_notice");
    });
    if (ritual_it == batch.candidates.end()) {
        require(false, "v26.5 originality/materialization test fixture should generate ritual_notice candidate");
        return;
    }

    const CandidateEvaluation evaluation = evaluate_candidate_feature(state, *ritual_it, AccessLevel::Curator);
    const std::string before = serialize_for_replay_test(state);
    const MaterializationResult result = materialize_hidden_mutation_artifact_candidate(state, *ritual_it, AccessLevel::Curator);
    const std::string after = serialize_for_replay_test(state);
    require(evaluation.originality.civilization_specificity_score < 0.30 &&
            !result.mutated && before == after &&
            contains_substr(result.explanation, "civilization specificity below threshold"),
            "v26.5 named originality block should prove low specificity blocks materialization without mutation");
}


void run_v28_0_fragment_intake_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const CivilizationSpecLoadResult legacy = load_civilization_specs_from_json_file("examples/40_civilization_specs_v1_1.json");
    require(legacy.ok() && validate_civilization_catalog(legacy.catalog).valid && legacy.catalog.fragments.empty(),
            "v28.0 existing v1.1 catalog without fragments should remain valid");

    const CivilizationSpecLoadResult fixture = load_civilization_specs_from_json_file("examples/v12_fragment_examples.json");
    const CivilizationSpecValidationResult fragment_validation = validate_civilization_fragments(fixture.catalog);
    require(fixture.ok() && fixture.catalog.fragments.size() >= 6U && fragment_validation.valid,
            "v28.0 example v1.2 fragments should load and validate");
    const std::string fragment_list = format_civilization_fragment_list(fixture.catalog);
    require(contains_substr(fragment_list, "fragment.high_artifact_density") &&
            contains_substr(fragment_list, "[artifact_bias]"),
            "v28.0 fragment list formatter should expose ID, title, and category");
    const CivilizationSpecFragment* known_fragment = find_civilization_fragment(fixture.catalog, "fragment.high_artifact_density");
    require(known_fragment != nullptr &&
            contains_substr(format_civilization_spec_fragment(*known_fragment), "target_public_artifact_count max 18"),
            "v28.0 fragment inspect formatter should show inert patches");
    require(find_civilization_fragment(fixture.catalog, "fragment.missing") == nullptr,
            "v28.0 unknown fragment lookup should return null without mutation");
    require(contains_substr(format_civilization_fragment_validation(fixture.catalog, fragment_validation), "valid fragments"),
            "v28.0 fragment validation formatter should report loaded and valid counts");

    auto minimal_catalog = [](const std::string& fragment_json) {
        return std::string("{\"schema_version\":\"1.2\",\"catalog_id\":\"fragment_test\",\"civilizations\":[{") +
            "\"id\":\"fragment_base\",\"display_name\":\"Fragment Base\"," +
            "\"description\":\"A valid compact civilization for fragment intake tests.\"," +
            "\"earliest_year\":100,\"latest_year\":700," +
            "\"geographic_features\":[\"river_delta\",\"salt_marsh\",\"raised_mounds\",\"canal_grid\"]," +
            "\"environmental_pressures\":[\"seasonal_flooding\",\"canal_silting\",\"drought_cycles\"]," +
            "\"major_sites\":[\"central_citadel\",\"lower_gate\",\"reed_shrine\",\"grain_quay\"]," +
            "\"economic_pressures\":[\"grain_tax\",\"canal_control\",\"dock_tolls\"]," +
            "\"trade_goods\":[\"grain\",\"salt\",\"reed_matting\",\"dried_fish\"]," +
            "\"institution_archetypes\":[\"water_office\",\"ritual_court\",\"merchant_house\",\"grain_tax_house\"]," +
            "\"social_actor_archetypes\":[\"marsh_clans\",\"dock_families\"]," +
            "\"authority_conflicts\":[\"water_office_vs_ritual_court\",\"merchant_house_vs_grain_tax_house\"]," +
            "\"religious_or_mythic_archetypes\":[\"flood_ancestor_cult\",\"reed_oracle\",\"drowned_founder_myth\"]," +
            "\"ritual_pressures\":[\"annual_gate_opening\",\"flood_appeasement\"]," +
            "\"writing_system_archetypes\":[\"reed_notches\",\"clay_seal_marks\"]," +
            "\"recordkeeping_styles\":[\"grain_ledger\",\"labor_roster\",\"boundary_stone\",\"ritual_song\"]," +
            "\"artifact_media\":[\"clay_tablet\",\"reed_ledger\",\"boundary_stone\",\"oral_song\"]," +
            "\"evidence_distortion_modes\":[\"flood_damage\",\"ritual_compression\",\"political_forgery\",\"calendar_drift\"]," +
            "\"mystery_archetypes\":[\"disputed_founder_identity\",\"missing_office_origin\",\"contradictory_flood_date\",\"forged_tax_decree\"]," +
            "\"target_hidden_entity_count\":18,\"target_hidden_event_count\":24," +
            "\"target_public_artifact_count\":16,\"target_mystery_count\":5,\"seed\":8810}]," +
            "\"fragments\":[" + fragment_json + "]}";
    };
    auto fragment = [](const std::string& extra) {
        return std::string("{\"schema_version\":\"1.2\",\"kind\":\"civilization_fragment\",") +
            "\"id\":\"fragment.test\",\"title\":\"Test Fragment\",\"category\":\"culture\"," +
            extra + "}";
    };

    const CivilizationSpecLoadResult bad_category = load_civilization_specs_from_json_text(
        minimal_catalog(fragment("\"category\":\"bad_category\",\"patches\":[{\"path\":\"tags\",\"strategy\":\"append_unique\",\"value\":[\"urban\"]}]")));
    require(!bad_category.ok(), "v28.0 unknown fragment categories should produce load diagnostics");

    const CivilizationSpecLoadResult bad_strategy = load_civilization_specs_from_json_text(
        minimal_catalog(fragment("\"patches\":[{\"path\":\"tags\",\"strategy\":\"merge_magic\",\"value\":[\"urban\"]}]")));
    require(!bad_strategy.ok(), "v28.0 unknown patch strategies should produce load diagnostics");

    const CivilizationSpecLoadResult empty_path = load_civilization_specs_from_json_text(
        minimal_catalog(fragment("\"patches\":[{\"path\":\"\",\"strategy\":\"append_unique\",\"value\":[\"urban\"]}]")));
    require(empty_path.ok() && !validate_civilization_fragments(empty_path.catalog).valid,
            "v28.0 empty patch paths should fail fragment validation");
    if (empty_path.ok()) {
        const CivilizationSpec* invalid_fragment_spec = find_civilization_spec(empty_path.catalog, "fragment_base");
        require(invalid_fragment_spec != nullptr,
                "v28.1.1 invalid inert fragments should not hide the base civilization spec");
        if (invalid_fragment_spec != nullptr) {
            const CivilizationBootstrapResult bootstrap = bootstrap_archive_state_from_civilization_spec(
                *invalid_fragment_spec,
                empty_path.catalog.catalog_id,
                empty_path.catalog.schema_version
            );
            require(bootstrap.ok && bootstrap.state.civilization_fragment_count == 0U,
                    "v28.1.1 invalid inert fragments should not activate or block normal runtime bootstrap");
        }
    }

    const CivilizationSpecLoadResult unknown_path = load_civilization_specs_from_json_text(
        minimal_catalog(fragment("\"patches\":[{\"path\":\"not_a_known_path\",\"strategy\":\"append_unique\",\"value\":[\"urban\"]}]")));
    require(unknown_path.ok() && !validate_civilization_fragments(unknown_path.catalog).valid,
            "v28.0 unknown patch paths should fail fragment validation");

    const CivilizationSpecLoadResult overlap = load_civilization_specs_from_json_text(
        minimal_catalog(fragment("\"requires_tags\":[\"urban\"],\"excludes_tags\":[\"urban\"],\"patches\":[{\"path\":\"tags\",\"strategy\":\"append_unique\",\"value\":[\"urban\"]}]")));
    require(overlap.ok() && !validate_civilization_fragments(overlap.catalog).valid,
            "v28.0 requires/excludes tag overlap should fail fragment validation");

    const CivilizationSpecLoadResult duplicate_tags = load_civilization_specs_from_json_text(
        minimal_catalog(fragment("\"tags\":[\"urban\",\"urban\"],\"patches\":[{\"path\":\"tags\",\"strategy\":\"append_unique\",\"value\":[\"urban\"]}]")));
    const CivilizationSpecValidationResult duplicate_tags_validation = validate_civilization_fragments(duplicate_tags.catalog);
    require(duplicate_tags.ok() && duplicate_tags_validation.valid && !duplicate_tags_validation.warnings.empty(),
            "v28.0 duplicate fragment tags should warn but remain valid");

    const CivilizationSpecLoadResult wrong_patch_value = load_civilization_specs_from_json_text(
        minimal_catalog(fragment("\"patches\":[{\"path\":\"tags\",\"strategy\":\"append_unique\",\"value\":{\"bad\":\"object\"}}]")));
    require(!wrong_patch_value.ok(), "v28.0 unsupported object patch values should fail loading");

    CliOptions default_generation;
    default_generation.query = "generate-candidates";
    default_generation.target_topic = "authority_conflict_0";
    const RuntimeStateSelectionResult selected = build_runtime_state_for_query(default_generation);
    require(selected.ok && selected.spec_selected() && selected.state.civilization_source.has_value(),
            "v28.0 fragment intake must not alter default spec-selected generation runtime");
}


struct CapturedCliRun {
    int exit_code = EXIT_FAILURE;
    std::string stdout_text;
    std::string stderr_text;
};

[[nodiscard]] CapturedCliRun run_cli_captured(const CliArgs& args) {
    std::ostringstream stdout_capture;
    std::ostringstream stderr_capture;
    std::streambuf* old_stdout = std::cout.rdbuf(stdout_capture.rdbuf());
    std::streambuf* old_stderr = std::cerr.rdbuf(stderr_capture.rdbuf());
    int exit_code = EXIT_FAILURE;
    try {
        exit_code = run_cli(args);
    } catch (...) {
        std::cout.rdbuf(old_stdout);
        std::cerr.rdbuf(old_stderr);
        throw;
    }
    std::cout.rdbuf(old_stdout);
    std::cerr.rdbuf(old_stderr);
    return CapturedCliRun{exit_code, stdout_capture.str(), stderr_capture.str()};
}

void run_v28_1_fixture_snapshot_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const std::vector<GoldenFixtureWorldDefinition> fixtures = list_golden_fixture_worlds();
    require(fixtures.size() >= 3U && find_golden_fixture_world("fixture.default_archive") != nullptr &&
            find_golden_fixture_world("fixture.marsh_citadel_seeded") != nullptr &&
            find_golden_fixture_world("fixture.fragment_catalog_only") != nullptr,
            "v28.1 fixture registry should list the known golden fixtures");
    require(find_golden_fixture_world("fixture.missing") == nullptr,
            "v28.1 unknown fixture lookup should fail cleanly");

    const std::vector<CliArgs> incompatible_fixture_commands{
        {"impossible_archive_mvp_v28_5", "--query", "archive-snapshot", "--fixture-id", "fixture.default_archive", "--spec-file", "definitely_missing.json"},
        {"impossible_archive_mvp_v28_5", "--query", "archive-snapshot", "--fixture-id", "fixture.default_archive", "--civilization-id", "marsh_citadel"},
        {"impossible_archive_mvp_v28_5", "--query", "archive-snapshot", "--fixture-id", "fixture.default_archive", "--seed", "123"},
        {"impossible_archive_mvp_v28_5", "--query", "archive-snapshot", "--fixture-id", "fixture.default_archive", "--archive-year", "625"},
    };
    for (const CliArgs& command : incompatible_fixture_commands) {
        const CapturedCliRun captured = run_cli_captured(command);
        require(captured.exit_code != EXIT_SUCCESS && captured.stdout_text.empty() &&
                contains_substr(captured.stderr_text, "remove --spec-file, --civilization-id, --seed, and --archive-year") &&
                !contains_substr(captured.stderr_text, "definitely_missing"),
                "v28.1.1 fixture-backed queries should reject runtime override flags before fixture/spec/state construction");
    }

    const CapturedCliRun normal_runtime_with_seed = run_cli_captured({
        "impossible_archive_mvp_v28_5", "--query", "list-generation-targets", "--seed", "123", "--archive-year", "625"
    });
    require(normal_runtime_with_seed.exit_code == EXIT_SUCCESS && !normal_runtime_with_seed.stdout_text.empty(),
            "v28.1.1 normal non-fixture queries should still accept ordinary runtime flags");

    const GoldenFixtureBuildResult default_fixture = build_golden_fixture_world("fixture.default_archive");
    require(default_fixture.ok,
            "v28.1 default golden fixture should build");
    if (default_fixture.ok) {
        const std::string before = serialize_for_replay_test(default_fixture.state);
        const ArchiveSnapshot snapshot = build_archive_snapshot(
            default_fixture.state,
            default_fixture.definition.id,
            default_fixture.definition.seed,
            default_fixture.definition.archive_year,
            default_fixture.definition.archive_year
        );
        const std::string after = serialize_for_replay_test(default_fixture.state);
        require(snapshot.hidden_entity_count > 0U && snapshot.hidden_event_count > 0U &&
                snapshot.civilization_spec_count > 0U && snapshot.validation_errors.empty(),
                "v28.1 default fixture snapshot should have nonzero core counts and no validation errors");
        require(before == after,
                "v28.1 snapshot creation should not mutate ArchiveEngineState");
        const std::string public_snapshot_text = format_archive_snapshot(snapshot);
        require(!contains_substr(public_snapshot_text, "event.marsh_citadel.foundation") &&
                !contains_substr(public_snapshot_text, "institution.marsh_citadel.water_office"),
                "v28.1 public snapshot formatting should expose counts/digests, not hidden IDs");
        require(snapshot.fixture_seed == default_fixture.definition.seed &&
                snapshot.state_seed == default_fixture.state.seed &&
                snapshot.fixture_archive_year == default_fixture.definition.archive_year &&
                snapshot.effective_archive_year == default_fixture.definition.archive_year,
                "v28.1.1 snapshot should distinguish fixture metadata from effective runtime state fields");
        require(contains_substr(public_snapshot_text, "fixture_seed") &&
                contains_substr(public_snapshot_text, "state_seed") &&
                contains_substr(public_snapshot_text, "fixture_archive_year") &&
                contains_substr(public_snapshot_text, "effective_archive_year") &&
                contains_substr(public_snapshot_text, "summary_digest") &&
                !contains_substr(public_snapshot_text, "deterministic_digest"),
                "v28.1.1 snapshot formatting should expose clarified fields and hard-rename deterministic_digest");

        const GoldenFixtureBuildResult seeded_fixture = build_golden_fixture_world("fixture.marsh_citadel_seeded");
        require(seeded_fixture.ok,
                "v28.1.1 seeded marsh_citadel fixture should build");
        if (seeded_fixture.ok) {
            const ArchiveSnapshot seeded_snapshot = build_archive_snapshot(
                seeded_fixture.state,
                seeded_fixture.definition.id,
                seeded_fixture.definition.seed,
                seeded_fixture.definition.archive_year,
                seeded_fixture.definition.archive_year
            );
            require(seeded_snapshot.fixture_seed == seeded_fixture.definition.seed &&
                    seeded_snapshot.state_seed == seeded_fixture.state.seed &&
                    seeded_snapshot.fixture_seed != seeded_snapshot.state_seed,
                    "v28.1.1 marsh_citadel_seeded should visibly distinguish fixture_seed from state_seed");
            require(seeded_snapshot.fixture_archive_year == 625 && seeded_snapshot.effective_archive_year == 625,
                    "v28.1.1 marsh_citadel_seeded should report fixture and effective archive years");
        }

        const GoldenFixtureBuildResult default_fixture_again = build_golden_fixture_world("fixture.default_archive");
        require(default_fixture_again.ok,
                "v28.1 repeated default golden fixture should build");
        if (default_fixture_again.ok) {
            const ArchiveSnapshot repeated = build_archive_snapshot(
                default_fixture_again.state,
                default_fixture_again.definition.id,
                default_fixture_again.definition.seed,
                default_fixture_again.definition.archive_year,
                default_fixture_again.definition.archive_year
            );
            const ArchiveSnapshotComparison comparison = compare_archive_snapshots(snapshot, repeated);
            require(comparison.same && snapshot.summary_digest == repeated.summary_digest,
                    "v28.1 repeated snapshot of same fixture should have the same digest");
            const std::string same_comparison_text = format_archive_snapshot_comparison(snapshot, repeated);
            require(contains_substr(same_comparison_text, "result: same") &&
                    contains_substr(same_comparison_text, "before_summary_digest") &&
                    contains_substr(same_comparison_text, "after_summary_digest") &&
                    !contains_substr(same_comparison_text, "before_digest") &&
                    !contains_substr(same_comparison_text, "after_digest"),
                    "v28.1.1 snapshot comparison should report same using summary_digest fields");
        }
    }

    const GoldenFixtureBuildResult fragment_fixture = build_golden_fixture_world("fixture.fragment_catalog_only");
    require(fragment_fixture.ok,
            "v28.1 fragment catalog fixture should build");
    if (fragment_fixture.ok) {
        const ArchiveSnapshot fragment_snapshot = build_archive_snapshot(
            fragment_fixture.state,
            fragment_fixture.definition.id,
            fragment_fixture.definition.seed,
            fragment_fixture.definition.archive_year,
            fragment_fixture.definition.archive_year
        );
        require(fragment_snapshot.civilization_fragment_count > 0U && fragment_snapshot.validation_errors.empty(),
                "v28.1 fragment catalog fixture snapshot should count inert fragments and validate");

        const GeneratedCandidateBatch fragment_batch = generate_candidate_batch(
            fragment_fixture.state,
            CandidateGenerationRequest{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "authority_conflict_0", 42}
        );
        const GeneratedCandidateBatch default_batch = generate_candidate_batch(
            default_fixture.state,
            CandidateGenerationRequest{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "authority_conflict_0", 42}
        );
        require(!fragment_batch.candidates.empty() && !default_batch.candidates.empty() &&
                fragment_batch.candidates.front().id == default_batch.candidates.front().id,
                "v28.1 fragment catalog fixture should observe fragments without changing candidate generation IDs");

        if (default_fixture.ok) {
            const ArchiveSnapshot default_snapshot = build_archive_snapshot(
                default_fixture.state,
                default_fixture.definition.id,
                default_fixture.definition.seed,
                default_fixture.definition.archive_year,
                default_fixture.definition.archive_year
            );
            const std::string different_text = format_archive_snapshot_comparison(default_snapshot, fragment_snapshot);
            require(contains_substr(different_text, "result: different") &&
                    contains_substr(different_text, "civilization_fragment_count"),
                    "v28.1 snapshot comparison should report count deltas for intentionally different snapshots");
        }
    }

    ArchiveSnapshot invalid_snapshot;
    invalid_snapshot.source_fixture_id = "fixture.invalid.validation_surface";
    invalid_snapshot.validation_errors = {"synthetic validation error"};
    require(contains_substr(format_archive_snapshot(invalid_snapshot), "synthetic validation error"),
            "v28.1 snapshot formatting should include validation errors without crashing");
}


void run_v28_2_evidence_potential_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const GoldenFixtureBuildResult default_fixture = build_golden_fixture_world("fixture.default_archive");
    require(default_fixture.ok, "v28.2 default golden fixture should build");
    if (!default_fixture.ok) {
        return;
    }

    require(!default_fixture.state.evidence_potentials.empty(),
            "v28.2 default fixture should derive nonzero EvidencePotential records");
    require(validate_evidence_potentials(default_fixture.state).empty(),
            "v28.2 default fixture EvidencePotential records should validate");

    const ArchiveSnapshot default_snapshot = build_archive_snapshot(
        default_fixture.state,
        default_fixture.definition.id,
        default_fixture.definition.seed,
        default_fixture.definition.archive_year,
        default_fixture.definition.archive_year
    );
    const std::string default_snapshot_text = format_archive_snapshot(default_snapshot);
    require(default_snapshot.evidence_potential_count == default_fixture.state.evidence_potentials.size() &&
            default_snapshot.evidence_potential_count > 0U &&
            contains_substr(default_snapshot_text, "evidence_potential_count"),
            "v28.2 ArchiveSnapshot should include evidence_potential_count");

    std::set<std::string> potential_ids;
    for (const EvidencePotential& potential : default_fixture.state.evidence_potentials) {
        potential_ids.insert(potential.id);
    }
    require(potential_ids.size() == default_fixture.state.evidence_potentials.size(),
            "v28.2 EvidencePotential IDs should be unique");

    const GoldenFixtureBuildResult default_fixture_again = build_golden_fixture_world("fixture.default_archive");
    require(default_fixture_again.ok, "v28.2 repeated default fixture should build");
    if (default_fixture_again.ok) {
        const ArchiveSnapshot repeated_snapshot = build_archive_snapshot(
            default_fixture_again.state,
            default_fixture_again.definition.id,
            default_fixture_again.definition.seed,
            default_fixture_again.definition.archive_year,
            default_fixture_again.definition.archive_year
        );
        require(compare_archive_snapshots(default_snapshot, repeated_snapshot).same &&
                default_snapshot.summary_digest == repeated_snapshot.summary_digest,
                "v28.2 repeated fixture snapshot should preserve summary_digest with EvidencePotential material included");
    }

    const GoldenFixtureBuildResult marsh_fixture = build_golden_fixture_world("fixture.marsh_citadel_seeded");
    require(marsh_fixture.ok && !marsh_fixture.state.evidence_potentials.empty(),
            "v28.2 seeded marsh fixture should derive deterministic nonzero EvidencePotential records");
    if (marsh_fixture.ok) {
        const GoldenFixtureBuildResult marsh_again = build_golden_fixture_world("fixture.marsh_citadel_seeded");
        require(marsh_again.ok && marsh_again.state.evidence_potentials.size() == marsh_fixture.state.evidence_potentials.size(),
                "v28.2 seeded marsh fixture should preserve EvidencePotential count across rebuilds");
        if (marsh_again.ok) {
            const ArchiveSnapshot marsh_snapshot = build_archive_snapshot(
                marsh_fixture.state,
                marsh_fixture.definition.id,
                marsh_fixture.definition.seed,
                marsh_fixture.definition.archive_year,
                marsh_fixture.definition.archive_year
            );
            const ArchiveSnapshot marsh_repeated = build_archive_snapshot(
                marsh_again.state,
                marsh_again.definition.id,
                marsh_again.definition.seed,
                marsh_again.definition.archive_year,
                marsh_again.definition.archive_year
            );
            require(compare_archive_snapshots(marsh_snapshot, marsh_repeated).same,
                    "v28.2 seeded marsh fixture should compare equal across repeated EvidencePotential derivation");
        }
    }

    auto mutated_state_with_first_potential = [&]() {
        ArchiveEngineState state = default_fixture.state;
        if (state.evidence_potentials.empty()) {
            state.evidence_potentials.push_back(EvidencePotential{});
        }
        return state;
    };

    {
        ArchiveEngineState state = mutated_state_with_first_potential();
        state.evidence_potentials.front().id.clear();
        require(!validate_evidence_potentials(state).empty(),
                "v28.2 validation should reject empty EvidencePotential IDs");
    }
    {
        ArchiveEngineState state = mutated_state_with_first_potential();
        state.evidence_potentials.front().source_id.clear();
        require(!validate_evidence_potentials(state).empty(),
                "v28.2 validation should reject missing EvidencePotential source IDs");
    }
    {
        ArchiveEngineState state = mutated_state_with_first_potential();
        state.evidence_potentials.front().earliest_possible_year = 700;
        state.evidence_potentials.front().latest_possible_year = 600;
        require(!validate_evidence_potentials(state).empty(),
                "v28.2 validation should reject invalid EvidencePotential year ranges");
    }
    {
        ArchiveEngineState state = mutated_state_with_first_potential();
        state.evidence_potentials.front().source_id = "event.missing_source_for_v28_2";
        state.evidence_potentials.front().source_type = EvidencePotentialSourceType::HiddenEvent;
        require(!validate_evidence_potentials(state).empty(),
                "v28.2 validation should reject missing EvidencePotential source objects");
    }
    {
        ArchiveEngineState state = mutated_state_with_first_potential();
        state.evidence_potentials.front().rationale.clear();
        require(!validate_evidence_potentials(state).empty(),
                "v28.2 validation should reject empty EvidencePotential rationales");
    }
    {
        ArchiveEngineState state = mutated_state_with_first_potential();
        state.evidence_potentials.front().source_id = "fragment.synthetic";
        require(!validate_evidence_potentials(state).empty(),
                "v28.2 validation should reject fragment-derived EvidencePotential records");
    }
    {
        ArchiveEngineState state = mutated_state_with_first_potential();
        state.evidence_potentials.front().public_safe = true;
        require(!validate_evidence_potentials(state).empty(),
                "v28.2 validation should reject public_safe potentials backed by hidden-only sources");
    }

    const GoldenFixtureBuildResult fragment_fixture = build_golden_fixture_world("fixture.fragment_catalog_only");
    require(fragment_fixture.ok, "v28.2 fragment catalog fixture should still build");
    if (fragment_fixture.ok) {
        bool has_fragment_source = false;
        for (const EvidencePotential& potential : fragment_fixture.state.evidence_potentials) {
            if (has_prefix(potential.source_id, "fragment.") || has_prefix(potential.source_id, "civilization_fragment.")) {
                has_fragment_source = true;
            }
        }
        require(fragment_fixture.state.civilization_fragment_count > 0U && !has_fragment_source &&
                validate_evidence_potentials(fragment_fixture.state).empty(),
                "v28.2 fragment catalog fixture should count fragments but produce no fragment-derived EvidencePotential records");
    }

    const std::string public_list = format_evidence_potential_list(default_fixture.state, AccessLevel::Public);
    const std::string public_summary = format_evidence_potential_summary(default_fixture.state, AccessLevel::Public);
    require(!contains_substr(public_list, "source_id:") &&
            !contains_substr(public_list, "event.marsh_citadel") &&
            !contains_substr(public_list, "institution.marsh_citadel") &&
            !contains_substr(public_summary, "event.marsh_citadel") &&
            !contains_substr(public_summary, "institution.marsh_citadel"),
            "v28.2 public EvidencePotential formatting should avoid hidden-only details");

    const std::string curator_list = format_evidence_potential_list(default_fixture.state, AccessLevel::Curator);
    require(contains_substr(curator_list, "source_id:") && contains_substr(curator_list, "rationale:"),
            "v28.2 curator EvidencePotential formatting should expose inspectable source and rationale details");

    const CapturedCliRun summary_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--query", "evidence-potential-summary"});
    require(summary_cli.exit_code == EXIT_SUCCESS && contains_substr(summary_cli.stdout_text, "EvidencePotential summary"),
            "v28.2 CLI should expose EvidencePotential summary query");

    const CapturedCliRun validation_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--query", "validate-evidence-potentials"});
    require(validation_cli.exit_code == EXIT_SUCCESS && contains_substr(validation_cli.stdout_text, "result: passed"),
            "v28.2 CLI should expose EvidencePotential validation query");
}


void run_v28_3_knowledge_horizon_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const GoldenFixtureBuildResult default_fixture = build_golden_fixture_world("fixture.default_archive");
    require(default_fixture.ok, "v28.3 default fixture should build for KnowledgeHorizon validation");
    if (!default_fixture.ok) {
        return;
    }

    const KnowledgeHorizonReport default_report = validate_knowledge_horizon(default_fixture.state, AccessLevel::Curator);
    const KnowledgeHorizonReport repeated_report = validate_knowledge_horizon(default_fixture.state, AccessLevel::Curator);
    require(!default_report.findings.empty(), "v28.3 default fixture KnowledgeHorizon validation should produce findings");
    require(default_report.findings.size() == repeated_report.findings.size() &&
            default_report.errors == repeated_report.errors &&
            (default_report.findings.empty() || default_report.findings.front().id == repeated_report.findings.front().id),
            "v28.3 KnowledgeHorizon validation output should be deterministic");

    const ArchiveSnapshot snapshot = build_archive_snapshot(
        default_fixture.state,
        default_fixture.definition.id,
        default_fixture.definition.seed,
        default_fixture.definition.archive_year,
        default_fixture.definition.archive_year
    );
    require(snapshot.knowledge_horizon_finding_count == default_report.findings.size(),
            "v28.3 ArchiveSnapshot should include KnowledgeHorizon finding count");
    require(snapshot.knowledge_horizon_error_count == default_report.errors.size(),
            "v28.3 ArchiveSnapshot should include KnowledgeHorizon error count");
    const std::string formatted_snapshot = format_archive_snapshot(snapshot);
    require(contains_substr(formatted_snapshot, "knowledge_horizon_finding_count") &&
            contains_substr(formatted_snapshot, "knowledge_horizon_error_count"),
            "v28.3 formatted snapshot should expose KnowledgeHorizon summary counts only");

    const std::string public_summary = format_knowledge_horizon_summary(default_fixture.state, AccessLevel::Public);
    const std::string public_findings = format_knowledge_horizon_findings(default_fixture.state, AccessLevel::Public);
    require(!contains_substr(public_summary, "event.marsh_citadel") &&
            !contains_substr(public_summary, "institution.marsh_citadel") &&
            !contains_substr(public_findings, "subject=hidden_entity") &&
            !contains_substr(public_findings, "context_id:") &&
            contains_substr(public_findings, "aggregate-only"),
            "v28.3 public KnowledgeHorizon formatting should avoid hidden IDs and explanations");

    const std::string curator_findings = format_knowledge_horizon_findings(default_fixture.state, AccessLevel::Curator);
    require(contains_substr(curator_findings, "knowledge_horizon.0000") &&
            contains_substr(curator_findings, "context=") &&
            contains_substr(curator_findings, "subject="),
            "v28.3 curator KnowledgeHorizon list should expose diagnostic finding details");
    const std::string curator_detail = format_knowledge_horizon_finding_detail(default_fixture.state, AccessLevel::Curator, "knowledge_horizon.0000");
    require(contains_substr(curator_detail, "found: true") &&
            contains_substr(curator_detail, "context_id:") &&
            contains_substr(curator_detail, "subject_id:") &&
            contains_substr(curator_detail, "explanation:"),
            "v28.3 curator KnowledgeHorizon detail should expose IDs and explanations");

    const std::vector<std::string> forbidden_public_detail_fields = {
        "status:",
        "context_type:",
        "subject_type:",
        "context_year:",
        "earliest_available_year:",
        "context_id:",
        "subject_id:",
        "explanation:"
    };
    const std::string public_blocked_detail = format_knowledge_horizon_finding_detail(default_fixture.state, AccessLevel::Public, "knowledge_horizon.0039");
    bool public_detail_clean = contains_substr(public_blocked_detail, "KnowledgeHorizon finding") &&
                               contains_substr(public_blocked_detail, "found: false") &&
                               !contains_substr(public_blocked_detail, "found: true");
    for (const std::string& forbidden : forbidden_public_detail_fields) {
        public_detail_clean = public_detail_clean && !contains_substr(public_blocked_detail, forbidden);
    }
    require(public_detail_clean,
            "v28.4 public KnowledgeHorizon detail should not confirm hidden finding existence or expose structural metadata");

    bool public_enumeration_clean = true;
    for (int index = 0; index < 5; ++index) {
        std::ostringstream id;
        id << "knowledge_horizon.000" << index;
        const std::string enumerated_detail = format_knowledge_horizon_finding_detail(default_fixture.state, AccessLevel::Public, id.str());
        public_enumeration_clean = public_enumeration_clean &&
                                   contains_substr(enumerated_detail, "found: false") &&
                                   !contains_substr(enumerated_detail, "found: true");
        for (const std::string& forbidden : forbidden_public_detail_fields) {
            public_enumeration_clean = public_enumeration_clean && !contains_substr(enumerated_detail, forbidden);
        }
    }
    require(public_enumeration_clean,
            "v28.4 public KnowledgeHorizon detail should resist sequential finding enumeration");

    ArchiveEngineState future_state = initialize_archive_engine(42);
    future_state.hidden_truth.add_entity(Entity{"entity.future_public_term", EntityType::Technology, "Future Public Term", {}, Interval{900, 950}, "", "", AccessLevel::Public});
    const KnowledgeHorizonFinding future = evaluate_knowledge_reference(
        future_state,
        KnowledgeContextType::PublicArchive,
        "context.public_100",
        KnowledgeSubjectType::Technology,
        "entity.future_public_term",
        100,
        AccessLevel::Public
    );
    require(future.status == KnowledgeHorizonStatus::InvalidFutureKnowledge,
            "v28.3 KnowledgeHorizon should reject future-dated subject references");

    ArchiveEngineState access_state = initialize_archive_engine(42);
    access_state.hidden_truth.add_entity(Entity{"entity.curator_known_hidden_subject", EntityType::Office, "Curator Known Hidden Subject", {}, Interval{1, kOpenEndedYear}, "", "", AccessLevel::Curator});
    const KnowledgeHorizonFinding public_hidden = evaluate_knowledge_reference(
        access_state,
        KnowledgeContextType::PublicArchive,
        "context.public_hidden",
        KnowledgeSubjectType::Office,
        "entity.curator_known_hidden_subject",
        kOpenEndedYear,
        AccessLevel::Public
    );
    require(public_hidden.status == KnowledgeHorizonStatus::InvalidUnmediatedHiddenTruth,
            "v28.3 KnowledgeHorizon should reject hidden-only subject references at public access");
    const KnowledgeHorizonFinding curator_hidden = evaluate_knowledge_reference(
        access_state,
        KnowledgeContextType::CuratorAudit,
        "context.curator_hidden",
        KnowledgeSubjectType::Office,
        "entity.curator_known_hidden_subject",
        kOpenEndedYear,
        AccessLevel::Curator
    );
    require(curator_hidden.status == KnowledgeHorizonStatus::ValidByCuratorAccess,
            "v28.3 KnowledgeHorizon should allow hidden-only subjects at curator/debug access where appropriate");


    auto mediated_state = initialize_archive_engine(42);
    mediated_state.hidden_truth.add_entity(Entity{"entity.future_copy_subject", EntityType::Technology, "Future Copy Subject", {}, Interval{900, 999}, "", "", AccessLevel::Public});
    Artifact copy_artifact;
    copy_artifact.id = "artifact.later_copy_context";
    copy_artifact.title = "Later Copy Context";
    copy_artifact.creator_id = "person.ivara";
    copy_artifact.attributed_creator_id = "person.ivara";
    copy_artifact.true_creation_year = 100;
    copy_artifact.claimed_creation_year = 100;
    copy_artifact.discovery_year = 800;
    copy_artifact.location_created = "site.reservoir_gate";
    copy_artifact.location_found = "site.reservoir_gate";
    copy_artifact.script_id = "script.green_seal";
    copy_artifact.language_id = "language.lattice_dialect";
    copy_artifact.dialect_id = "dialect.upper_lattice";
    copy_artifact.material = "copy board";
    copy_artifact.transmission_history = "later copy transmission";
    copy_artifact.evidence_modifiers = {EvidenceModifier::LaterCopy};
    mediated_state.public_archive.add_artifact(copy_artifact);
    const KnowledgeHorizonFinding later_copy = evaluate_knowledge_reference(
        mediated_state,
        KnowledgeContextType::ArtifactCreator,
        "artifact.later_copy_context",
        KnowledgeSubjectType::Technology,
        "entity.future_copy_subject",
        100,
        AccessLevel::Curator
    );
    require(later_copy.status == KnowledgeHorizonStatus::ValidByLaterCopy,
            "v28.3 mediated later-copy reference should not be treated as accidental future knowledge");

    Artifact forgery_artifact = copy_artifact;
    forgery_artifact.id = "artifact.forgery_context";
    forgery_artifact.evidence_modifiers = {EvidenceModifier::Forgery};
    forgery_artifact.transmission_history = "forged context";
    mediated_state.public_archive.add_artifact(forgery_artifact);
    const KnowledgeHorizonFinding forgery = evaluate_knowledge_reference(
        mediated_state,
        KnowledgeContextType::ArtifactCreator,
        "artifact.forgery_context",
        KnowledgeSubjectType::Technology,
        "entity.future_copy_subject",
        100,
        AccessLevel::Curator
    );
    require(forgery.status == KnowledgeHorizonStatus::ValidByForgery,
            "v28.3 mediated forgery reference should not be treated as accidental future knowledge");

    ArchiveEngineState unsafe_potential_state = default_fixture.state;
    if (!unsafe_potential_state.evidence_potentials.empty()) {
        unsafe_potential_state.evidence_potentials.front().public_safe = true;
        const KnowledgeHorizonReport unsafe_report = validate_knowledge_horizon(unsafe_potential_state, AccessLevel::Curator);
        const bool saw_public_safety_error = std::any_of(unsafe_report.findings.begin(), unsafe_report.findings.end(), [](const KnowledgeHorizonFinding& finding) {
            return finding.status == KnowledgeHorizonStatus::InvalidAccessLeak;
        });
        require(saw_public_safety_error,
                "v28.3 EvidencePotential public safety should participate in KnowledgeHorizon validation");
    }

    const GoldenFixtureBuildResult fragment_fixture = build_golden_fixture_world("fixture.fragment_catalog_only");
    require(fragment_fixture.ok, "v28.3 fragment catalog fixture should still build");
    if (fragment_fixture.ok) {
        const KnowledgeHorizonReport fragment_report = validate_knowledge_horizon(fragment_fixture.state, AccessLevel::Curator);
        const bool has_fragment_finding = std::any_of(fragment_report.findings.begin(), fragment_report.findings.end(), [](const KnowledgeHorizonFinding& finding) {
            return finding.subject_type == KnowledgeSubjectType::CivilizationFragment ||
                   contains_substr(finding.subject_id, "fragment.") ||
                   contains_substr(finding.context_id, "fragment.");
        });
        require(!has_fragment_finding,
                "v28.3 fragment catalog fixture should not activate fragment-driven KnowledgeHorizon records");
    }

    const CapturedCliRun summary_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--query", "knowledge-horizon-summary"});
    require(summary_cli.exit_code == EXIT_SUCCESS && contains_substr(summary_cli.stdout_text, "KnowledgeHorizon summary"),
            "v28.3 CLI should expose KnowledgeHorizon summary query");
    const CapturedCliRun validation_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--query", "validate-knowledge-horizon"});
    require(validation_cli.exit_code == EXIT_SUCCESS && contains_substr(validation_cli.stdout_text, "KnowledgeHorizon validation"),
            "v28.3 CLI should expose KnowledgeHorizon validation query");
}


void run_v28_4_contradiction_budget_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    ArchiveEngineState state = initialize_archive_engine(42);
    derive_evidence_potentials_into_state(state);

    const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
    const ContradictionBudgetReport repeated_report = compute_contradiction_budget(state, AccessLevel::Curator);
    require(!report.buckets.empty() && report.errors.empty(),
            "v28.4 default fixed state should compute a valid ContradictionBudget report");
    require(report.buckets.size() == repeated_report.buckets.size() &&
            report.errors == repeated_report.errors &&
            !repeated_report.buckets.empty() &&
            report.buckets.front().id == repeated_report.buckets.front().id,
            "v28.4 ContradictionBudget output should be deterministic");

    const auto archive_it = std::find_if(report.buckets.begin(), report.buckets.end(), [](const ContradictionBudgetBucket& bucket) {
        return bucket.id == "contradiction_budget.archive";
    });
    require(archive_it != report.buckets.end(),
            "v28.4 ContradictionBudget should include archive-level bucket");
    if (archive_it != report.buckets.end()) {
        require(archive_it->contradiction_count == state.public_archive.contradictions().size(),
                "v28.4 archive bucket should count contradictions deterministically");
        require(archive_it->claim_count == state.public_archive.claims().size(),
                "v28.4 archive bucket should count claims deterministically");
        require(archive_it->contradiction_density == static_cast<double>(archive_it->contradiction_count) / static_cast<double>(std::max<std::size_t>(1U, archive_it->claim_count)),
                "v28.4 contradiction density should use the documented formula");
        require(archive_it->unresolved_ratio == static_cast<double>(archive_it->unresolved_contradiction_count) / static_cast<double>(std::max<std::size_t>(1U, archive_it->contradiction_count)),
                "v28.4 unresolved ratio should use the documented formula");
        require(archive_it->generation_bug_ratio == static_cast<double>(archive_it->generation_bug_count) / static_cast<double>(std::max<std::size_t>(1U, archive_it->contradiction_count)),
                "v28.4 generation bug ratio should use the documented formula");
        require(!to_string(archive_it->severity).empty() && !to_string(archive_it->status).empty(),
                "v28.4 severity/status assignment should be deterministic and formattable");
    }

    std::set<std::string> bucket_ids;
    bool unique_ids = true;
    for (const ContradictionBudgetBucket& bucket : report.buckets) {
        unique_ids = unique_ids && !bucket.id.empty() && bucket_ids.insert(bucket.id).second;
    }
    require(unique_ids, "v28.4 ContradictionBudget bucket IDs should be unique");
    require(validate_contradiction_budget_report(report).empty(),
            "v28.4 ContradictionBudget report validation should pass for computed telemetry");

    ContradictionBudgetReport invalid_report = report;
    if (!invalid_report.buckets.empty()) {
        invalid_report.buckets.front().id.clear();
        require(!validate_contradiction_budget_report(invalid_report).empty(),
                "v28.4 ContradictionBudget validation should reject empty bucket IDs");
    }

    const ArchiveSnapshot snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const ArchiveSnapshot repeated_snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const std::string formatted_snapshot = format_archive_snapshot(snapshot);
    require(snapshot.contradiction_budget_bucket_count == report.buckets.size() &&
            contains_substr(formatted_snapshot, "contradiction_budget_bucket_count") &&
            contains_substr(formatted_snapshot, "contradiction_budget_over_budget_count") &&
            contains_substr(formatted_snapshot, "contradiction_budget_generation_bug_count"),
            "v28.4 ArchiveSnapshot should include ContradictionBudget counts");
    require(snapshot.summary_digest == repeated_snapshot.summary_digest,
            "v28.4 summary_digest should remain deterministic with ContradictionBudget material");

    const std::string public_summary = format_contradiction_budget_summary(state, AccessLevel::Public);
    const std::string public_list = format_contradiction_budget_buckets(state, AccessLevel::Public);
    const std::string public_archive_detail = format_contradiction_budget_bucket_detail(state, AccessLevel::Public, "contradiction_budget.archive");
    require(contains_substr(public_summary, "ContradictionBudget summary") &&
            contains_substr(public_list, "aggregate-only") &&
            contains_substr(public_archive_detail, "details: restricted") &&
            !contains_substr(public_summary, "representative_contradiction_ids") &&
            !contains_substr(public_list, "contradiction.calendar") &&
            !contains_substr(public_archive_detail, "representative_contradiction_ids"),
            "v28.4 public ContradictionBudget formatting should avoid hidden diagnostic details");

    const std::string curator_detail = format_contradiction_budget_bucket_detail(state, AccessLevel::Curator, "contradiction_budget.archive");
    require(contains_substr(curator_detail, "found: true") &&
            contains_substr(curator_detail, "representative_contradiction_ids") &&
            contains_substr(curator_detail, "contradiction."),
            "v28.4 curator ContradictionBudget detail should expose representative contradiction IDs");

    const CapturedCliRun summary_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--runtime", "fixed-fixture", "--query", "contradiction-budget-summary"});
    require(summary_cli.exit_code == EXIT_SUCCESS && contains_substr(summary_cli.stdout_text, "ContradictionBudget summary"),
            "v28.4 CLI should expose ContradictionBudget summary query");
    const CapturedCliRun validation_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--runtime", "fixed-fixture", "--query", "validate-contradiction-budget"});
    require(validation_cli.exit_code == EXIT_SUCCESS && contains_substr(validation_cli.stdout_text, "ContradictionBudget validation") && contains_substr(validation_cli.stdout_text, "result: passed"),
            "v28.4 CLI should expose ContradictionBudget validation query");

    const GoldenFixtureBuildResult fragment_fixture = build_golden_fixture_world("fixture.fragment_catalog_only");
    require(fragment_fixture.ok, "v28.4 fragment catalog fixture should still build");
    if (fragment_fixture.ok) {
        const ContradictionBudgetReport fragment_report = compute_contradiction_budget(fragment_fixture.state, AccessLevel::Curator);
        bool fragment_inert = true;
        for (const ContradictionBudgetBucket& bucket : fragment_report.buckets) {
            fragment_inert = fragment_inert &&
                            !contains_substr(bucket.id, "fragment.") &&
                            !contains_substr(bucket.scope_id, "fragment.");
        }
        require(fragment_inert,
                "v28.4 fragment catalog fixture should not activate fragment-driven ContradictionBudget records");
    }
}


void run_v28_11_contradiction_budget_policy_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    auto has_reason = [](const ContradictionBudgetBucket& bucket, ContradictionBudgetReasonCode reason_code) {
        return std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), reason_code) != bucket.reason_codes.end();
    };

    auto archive_bucket = [](const ContradictionBudgetReport& report) -> const ContradictionBudgetBucket* {
        const auto it = std::find_if(report.buckets.begin(), report.buckets.end(), [](const ContradictionBudgetBucket& bucket) {
            return bucket.id == "contradiction_budget.archive";
        });
        return it == report.buckets.end() ? nullptr : &(*it);
    };

    auto make_budget_state = [](std::size_t claim_count,
                                std::size_t contradiction_count,
                                ContradictionCause cause,
                                ContradictionType type,
                                ClaimType claim_type) {
        ArchiveEngineState state;
        Artifact artifact;
        artifact.id = "artifact.v28_11_budget_fixture";
        artifact.title = "v28.11 Budget Fixture Artifact";
        artifact.discovery_year = 800;
        artifact.min_access = AccessLevel::Public;
        if (cause == ContradictionCause::Damage) {
            artifact.evidence_modifiers.push_back(EvidenceModifier::Damage);
        }
        if (cause == ContradictionCause::RitualAnachronism || type == ContradictionType::RitualContradiction) {
            artifact.evidence_modifiers.push_back(EvidenceModifier::RitualAnachronism);
        }
        state.public_archive.add_artifact(std::move(artifact));
        for (std::size_t index = 0; index < claim_count; ++index) {
            Claim claim;
            claim.id = "claim.v28_11_budget_fixture." + std::to_string(index);
            claim.source_artifact_id = "artifact.v28_11_budget_fixture";
            claim.type = claim_type;
            claim.subject = "budget subject";
            claim.predicate = "disagrees with";
            claim.object = "budget object";
            claim.literal_content = "budget claim";
            claim.confidence = 0.5;
            state.public_archive.add_claim_to_artifact("artifact.v28_11_budget_fixture", std::move(claim));
        }
        for (std::size_t index = 0; index < contradiction_count; ++index) {
            const std::size_t claim_index = claim_count == 0U ? 0U : index % claim_count;
            state.public_archive.add_contradiction(Contradiction{
                "contradiction.v28_11_budget_fixture." + std::to_string(index),
                "v28_11_budget_fixture",
                {"claim.v28_11_budget_fixture." + std::to_string(claim_index)},
                {"artifact.v28_11_budget_fixture"},
                type,
                cause,
                AccessLevel::Scholar,
                "test hidden resolution",
                AccessLevel::Canon,
                cause == ContradictionCause::UnresolvedGenerationBug ? "generation bug under review" : "classified test disagreement",
                800,
            });
        }
        return state;
    };

    const ContradictionBudgetPolicy policy = default_contradiction_budget_policy();
    const ContradictionBudgetPolicy repeated_policy = default_contradiction_budget_policy();
    require(policy.max_contradiction_density_watch == repeated_policy.max_contradiction_density_watch &&
            policy.max_contradiction_density_over_budget == repeated_policy.max_contradiction_density_over_budget &&
            policy.warn_on_too_clean_archive && policy.warn_on_generation_bugs,
            "v28.11 default contradiction budget policy should be deterministic");
    require(to_string(ContradictionBudgetReasonCode::GenerationBugPressure) == "generation_bug_pressure" &&
            to_string(ContradictionBudgetReasonCode::TooCleanArchive) == "too_clean_archive",
            "v28.11 contradiction budget reason codes should be formattable");

    {
        const ArchiveEngineState state = make_budget_state(10, 1, ContradictionCause::Damage, ContradictionType::DateContradiction, ClaimType::FactualClaim);
        const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
        const ContradictionBudgetBucket* bucket = archive_bucket(report);
        require(bucket != nullptr && bucket->status == ContradictionBudgetStatus::Watch && has_reason(*bucket, ContradictionBudgetReasonCode::WatchDensity),
                "v28.11 watch density should add watch_density reason code");
        require(bucket != nullptr && has_reason(*bucket, ContradictionBudgetReasonCode::ExpectedDamageDisagreement),
                "v28.11 damaged-evidence disagreement should add expected_damage_disagreement reason code");
    }

    {
        const ArchiveEngineState state = make_budget_state(10, 3, ContradictionCause::Damage, ContradictionType::DateContradiction, ClaimType::FactualClaim);
        const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
        const ContradictionBudgetBucket* bucket = archive_bucket(report);
        require(bucket != nullptr && bucket->status == ContradictionBudgetStatus::OverBudget && has_reason(*bucket, ContradictionBudgetReasonCode::OverBudgetDensity),
                "v28.11 over-budget density should add over_budget_density reason code");
    }

    {
        const ArchiveEngineState state = make_budget_state(4, 1, ContradictionCause::None, ContradictionType::DateContradiction, ClaimType::FactualClaim);
        const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
        const ContradictionBudgetBucket* bucket = archive_bucket(report);
        require(bucket != nullptr && bucket->status == ContradictionBudgetStatus::OverBudget &&
                has_reason(*bucket, ContradictionBudgetReasonCode::OverBudgetUnresolvedRatio) &&
                has_reason(*bucket, ContradictionBudgetReasonCode::MissingContradictionCause),
                "v28.11 over-budget unresolved/missing-cause pressure should be reason coded");
    }

    {
        const ArchiveEngineState state = make_budget_state(4, 1, ContradictionCause::UnresolvedGenerationBug, ContradictionType::LanguageContradiction, ClaimType::FactualClaim);
        const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
        const ContradictionBudgetBucket* bucket = archive_bucket(report);
        require(bucket != nullptr && bucket->status == ContradictionBudgetStatus::OverBudget && has_reason(*bucket, ContradictionBudgetReasonCode::GenerationBugPressure),
                "v28.11 generation bug pressure should add generation_bug_pressure reason code");
    }

    {
        const ArchiveEngineState state = make_budget_state(3, 0, ContradictionCause::Damage, ContradictionType::DateContradiction, ClaimType::FactualClaim);
        const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
        const ContradictionBudgetBucket* bucket = archive_bucket(report);
        require(bucket != nullptr && bucket->status == ContradictionBudgetStatus::Watch && has_reason(*bucket, ContradictionBudgetReasonCode::TooCleanArchive),
                "v28.11 too-clean archive slices should be watch/advisory and reason coded");
    }

    {
        const ArchiveEngineState state = make_budget_state(10, 1, ContradictionCause::MythologizedMemory, ContradictionType::RitualContradiction, ClaimType::FactualClaim);
        const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
        const ContradictionBudgetBucket* bucket = archive_bucket(report);
        require(bucket != nullptr && has_reason(*bucket, ContradictionBudgetReasonCode::ProductiveAmbiguity) &&
                has_reason(*bucket, ContradictionBudgetReasonCode::ValidRitualContradiction),
                "v28.11 productive ambiguity and ritual contradictions should be reason coded");
    }

    {
        const ArchiveEngineState state = make_budget_state(10, 1, ContradictionCause::CalendarConversionError, ContradictionType::DateContradiction, ClaimType::LegalFiction);
        const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
        const ContradictionBudgetBucket* bucket = archive_bucket(report);
        require(bucket != nullptr && has_reason(*bucket, ContradictionBudgetReasonCode::ValidLegalFiction),
                "v28.11 legal fiction contradictions should be reason coded where supported by claim type");
    }

    {
        ArchiveEngineState state = make_budget_state(10, 1, ContradictionCause::Damage, ContradictionType::DateContradiction, ClaimType::FactualClaim);
        Mystery mystery;
        mystery.id = "mystery.v28_11_protected";
        mystery.title = "v28.11 Protected Mystery";
        mystery.reveal_mode = RevealMode::NeverFullyResolvable;
        mystery.min_access = AccessLevel::Public;
        state.mysteries.push_back(mystery);
        Artifact* artifact = state.public_archive.find_artifact_mutable("artifact.v28_11_budget_fixture");
        if (artifact != nullptr) {
            artifact->mystery_links.push_back("mystery.v28_11_protected");
        }
        const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
        const ContradictionBudgetBucket* bucket = archive_bucket(report);
        require(bucket != nullptr && has_reason(*bucket, ContradictionBudgetReasonCode::ProtectedMysteryPressure),
                "v28.11 protected mystery pressure should be reason coded");
    }

    {
        ContradictionBudgetReport invalid;
        ContradictionBudgetBucket bucket;
        bucket.id = "contradiction_budget.archive";
        bucket.claim_count = 1;
        bucket.contradiction_count = 1;
        bucket.contradiction_density = 0.30;
        bucket.status = ContradictionBudgetStatus::OverBudget;
        bucket.severity = ContradictionBudgetSeverity::High;
        invalid.buckets.push_back(bucket);
        require(!validate_contradiction_budget_report(invalid).empty(),
                "v28.11 validation should reject over-budget bucket without reason code");
    }

    {
        ContradictionBudgetReport invalid;
        ContradictionBudgetBucket bucket;
        bucket.id = "contradiction_budget.archive";
        bucket.claim_count = 4;
        bucket.contradiction_count = 1;
        bucket.generation_bug_count = 1;
        bucket.contradiction_density = 0.25;
        bucket.generation_bug_ratio = 1.0;
        bucket.status = ContradictionBudgetStatus::OverBudget;
        bucket.severity = ContradictionBudgetSeverity::High;
        bucket.reason_codes = {ContradictionBudgetReasonCode::OverBudgetDensity};
        invalid.buckets.push_back(bucket);
        require(!validate_contradiction_budget_report(invalid).empty(),
                "v28.11 validation should reject generation-bug bucket without generation bug reason code");
    }

    {
        ContradictionBudgetReport invalid;
        ContradictionBudgetBucket bucket;
        bucket.id = "contradiction_budget.archive";
        bucket.claim_count = 2;
        bucket.contradiction_count = 0;
        bucket.status = ContradictionBudgetStatus::Watch;
        bucket.severity = ContradictionBudgetSeverity::Moderate;
        invalid.buckets.push_back(bucket);
        require(!validate_contradiction_budget_report(invalid).empty(),
                "v28.11 validation should reject/warn too-clean bucket without too_clean reason code");
    }

    {
        ContradictionBudgetReport invalid;
        ContradictionBudgetBucket bucket;
        bucket.id = "contradiction_budget.archive";
        bucket.contradiction_density = -0.1;
        bucket.status = ContradictionBudgetStatus::OverBudget;
        bucket.severity = ContradictionBudgetSeverity::High;
        invalid.buckets.push_back(bucket);
        require(!validate_contradiction_budget_report(invalid).empty(),
                "v28.11 validation should reject invalid metrics without invalid_metric reason code");
    }

    ArchiveEngineState default_state = initialize_archive_engine(42);
    derive_evidence_potentials_into_state(default_state);
    const ContradictionBudgetReport default_report = compute_contradiction_budget(default_state, AccessLevel::Curator);
    require(validate_contradiction_budget_report(default_report).empty(),
            "v28.11 computed default ContradictionBudget report should validate with policy reason codes");

    const std::string public_detail = format_contradiction_budget_bucket_detail(default_state, AccessLevel::Public, "contradiction_budget.archive");
    require(contains_substr(public_detail, "details: restricted") &&
            !contains_substr(public_detail, "reason_codes") &&
            !contains_substr(public_detail, "Policy thresholds"),
            "v28.11 public formatting should avoid hidden diagnostic reason-code/policy details");

    const std::string curator_detail = format_contradiction_budget_bucket_detail(default_state, AccessLevel::Curator, "contradiction_budget.archive");
    require(contains_substr(curator_detail, "reason_codes") && contains_substr(curator_detail, "Policy thresholds"),
            "v28.11 curator formatting should expose reason codes and policy thresholds");

    const ArchiveSnapshot snapshot = build_archive_snapshot(default_state, "fixture.synthetic_fixed", default_state.seed, kOpenEndedYear, kOpenEndedYear);
    const std::string formatted_snapshot = format_archive_snapshot(snapshot);
    require(contains_substr(formatted_snapshot, "contradiction_budget_watch_count") &&
            contains_substr(formatted_snapshot, "contradiction_budget_too_clean_count") &&
            contains_substr(formatted_snapshot, "contradiction_budget_productive_ambiguity_count"),
            "v28.11 snapshot should include contradiction budget reason-code aggregate counts");
    require(snapshot.summary_digest == build_archive_snapshot(default_state, "fixture.synthetic_fixed", default_state.seed, kOpenEndedYear, kOpenEndedYear).summary_digest,
            "v28.11 summary digest should remain deterministic with reason-code material");

    const std::string control_summary = format_control_layer_audit_summary(default_state, AccessLevel::Public);
    require(contains_substr(control_summary, "ControlLayerAudit summary"),
            "v28.11 control-layer audit should remain valid and queryable");
}


void run_v28_5_candidate_artifact_plan_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    ArchiveEngineState state = initialize_archive_engine(42);
    derive_evidence_potentials_into_state(state);
    derive_candidate_artifact_plans_into_state(state, AccessLevel::Curator);

    const CandidateArtifactPlanReport report = derive_candidate_artifact_plans(state, AccessLevel::Curator);
    const CandidateArtifactPlanReport repeated_report = derive_candidate_artifact_plans(state, AccessLevel::Curator);
    require(!report.plans.empty(),
            "v28.5 default fixture should derive nonzero CandidateArtifactPlans");
    require(report.plans.size() == repeated_report.plans.size() &&
            !repeated_report.plans.empty() &&
            report.plans.front().id == repeated_report.plans.front().id &&
            report.errors == repeated_report.errors,
            "v28.5 CandidateArtifactPlan derivation should be deterministic");

    std::set<std::string> plan_ids;
    bool unique_ids = true;
    bool references_existing_potentials = true;
    bool no_current_materialization = true;
    for (const CandidateArtifactPlan& plan : report.plans) {
        unique_ids = unique_ids && !plan.id.empty() && plan_ids.insert(plan.id).second;
        references_existing_potentials = references_existing_potentials &&
            std::any_of(state.evidence_potentials.begin(), state.evidence_potentials.end(), [&](const EvidencePotential& potential) {
                return potential.id == plan.source_id;
            });
        no_current_materialization = no_current_materialization && !plan.current_materialization_enabled;
    }
    require(unique_ids, "v28.5 CandidateArtifactPlan IDs should be unique");
    require(references_existing_potentials, "v28.5 every plan should reference an existing EvidencePotential");
    require(no_current_materialization, "v28.5 no plan should enable current materialization");
    require(validate_candidate_artifact_plans(state).empty(),
            "v28.5 computed CandidateArtifactPlans should validate");

    ArchiveEngineState missing_source_state = state;
    if (!missing_source_state.candidate_artifact_plans.empty()) {
        missing_source_state.candidate_artifact_plans.front().source_id = "evidence_potential.missing_for_v28_5";
        require(!validate_candidate_artifact_plans(missing_source_state).empty(),
                "v28.5 validation should reject missing source potential");
    }

    ArchiveEngineState duplicate_id_state = state;
    if (duplicate_id_state.candidate_artifact_plans.size() >= 2U) {
        duplicate_id_state.candidate_artifact_plans[1].id = duplicate_id_state.candidate_artifact_plans[0].id;
        require(!validate_candidate_artifact_plans(duplicate_id_state).empty(),
                "v28.5 validation should reject duplicate plan IDs");
    }

    ArchiveEngineState hidden_leak_state = state;
    if (!hidden_leak_state.candidate_artifact_plans.empty()) {
        hidden_leak_state.candidate_artifact_plans.front().public_safe = true;
        require(!validate_candidate_artifact_plans(hidden_leak_state).empty(),
                "v28.5 validation should reject public-safe hidden-source plan leaks");
    }

    ArchiveEngineState materialization_leak_state = state;
    if (!materialization_leak_state.candidate_artifact_plans.empty()) {
        materialization_leak_state.candidate_artifact_plans.front().current_materialization_enabled = true;
        require(!validate_candidate_artifact_plans(materialization_leak_state).empty(),
                "v28.5 validation should reject any current materialization enablement");
    }

    require(!report.plans.empty() && !to_string(report.plans.front().status).empty() && !to_string(report.plans.front().risk_level).empty(),
            "v28.5 status classification and risk assignment should be deterministic and formattable");

    const ArchiveSnapshot snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const ArchiveSnapshot repeated_snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const std::string formatted_snapshot = format_archive_snapshot(snapshot);
    require(snapshot.candidate_artifact_plan_count == report.plans.size() &&
            contains_substr(formatted_snapshot, "candidate_artifact_plan_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_plan_blocked_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_plan_curator_review_count"),
            "v28.5 ArchiveSnapshot should include CandidateArtifactPlan counts");
    require(snapshot.summary_digest == repeated_snapshot.summary_digest,
            "v28.5 summary_digest should include stable CandidateArtifactPlan material");

    const std::string public_summary = format_candidate_artifact_plan_summary(state, AccessLevel::Public);
    const std::string public_list = format_candidate_artifact_plan_list(state, AccessLevel::Public);
    const std::string public_detail = format_candidate_artifact_plan_detail(state, AccessLevel::Public, report.plans.front().id);
    require(contains_substr(public_summary, "CandidateArtifactPlan summary") &&
            contains_substr(public_list, "public_safe_visible_plans") &&
            contains_substr(public_detail, "found: false") &&
            !contains_substr(public_summary, "knowledge_horizon_finding_ids") &&
            !contains_substr(public_list, "evidence_potential.0000") &&
            !contains_substr(public_detail, "source_id:") &&
            !contains_substr(public_detail, "rationale:"),
            "v28.5 public CandidateArtifactPlan formatting should avoid hidden diagnostic details");

    const std::string curator_detail = format_candidate_artifact_plan_detail(state, AccessLevel::Curator, report.plans.front().id);
    require(contains_substr(curator_detail, "found: true") &&
            contains_substr(curator_detail, "source_id:") &&
            contains_substr(curator_detail, "rationale:") &&
            contains_substr(curator_detail, "knowledge_horizon_finding_ids") &&
            contains_substr(curator_detail, "contradiction_budget_bucket_ids"),
            "v28.5 curator CandidateArtifactPlan detail should expose full diagnostics");

    const CapturedCliRun summary_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--runtime", "fixed-fixture", "--query", "candidate-artifact-plan-summary"});
    require(summary_cli.exit_code == EXIT_SUCCESS && contains_substr(summary_cli.stdout_text, "CandidateArtifactPlan summary"),
            "v28.5 CLI should expose CandidateArtifactPlan summary query");
    const CapturedCliRun validation_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--runtime", "fixed-fixture", "--query", "validate-candidate-artifact-plans"});
    require(validation_cli.exit_code == EXIT_SUCCESS && contains_substr(validation_cli.stdout_text, "CandidateArtifactPlan validation") && contains_substr(validation_cli.stdout_text, "result: passed"),
            "v28.5 CLI should expose CandidateArtifactPlan validation query");
    const CapturedCliRun list_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--runtime", "fixed-fixture", "--access", "curator", "--query", "list-candidate-artifact-plans"});
    require(list_cli.exit_code == EXIT_SUCCESS && contains_substr(list_cli.stdout_text, "candidate_artifact_plan."),
            "v28.5 CLI should expose CandidateArtifactPlan list query");
    const CapturedCliRun detail_cli = run_cli_captured({"impossible_archive_mvp_v28_5", "--runtime", "fixed-fixture", "--access", "curator", "--query", "show-candidate-artifact-plan", "--candidate-artifact-plan-id", report.plans.front().id});
    require(detail_cli.exit_code == EXIT_SUCCESS && contains_substr(detail_cli.stdout_text, "CandidateArtifactPlan") && contains_substr(detail_cli.stdout_text, "found: true"),
            "v28.5 CLI should expose CandidateArtifactPlan detail query");

    const GoldenFixtureBuildResult fragment_fixture = build_golden_fixture_world("fixture.fragment_catalog_only");
    require(fragment_fixture.ok, "v28.5 fragment catalog fixture should still build");
    if (fragment_fixture.ok) {
        const CandidateArtifactPlanReport fragment_report = derive_candidate_artifact_plans(fragment_fixture.state, AccessLevel::Curator);
        bool fragment_inert = true;
        for (const CandidateArtifactPlan& plan : fragment_report.plans) {
            fragment_inert = fragment_inert &&
                            !contains_substr(plan.id, "fragment.") &&
                            !contains_substr(plan.source_id, "fragment.") &&
                            !contains_substr(plan.target_topic, "fragment.");
        }
        require(fragment_inert,
                "v28.5 fragment catalog fixture should not activate fragment-derived CandidateArtifactPlans");
    }
}



void run_v28_6_candidate_artifact_plan_evaluation_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    ArchiveEngineState state = initialize_archive_engine(42);
    derive_evidence_potentials_into_state(state);
    derive_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    evaluate_candidate_artifact_plans_into_state(state, AccessLevel::Curator);

    const CandidateArtifactPlanEvaluationReport report = evaluate_candidate_artifact_plans(state, AccessLevel::Curator);
    const CandidateArtifactPlanEvaluationReport repeated_report = evaluate_candidate_artifact_plans(state, AccessLevel::Curator);
    require(!report.evaluations.empty(),
            "v28.6 default fixture should evaluate CandidateArtifactPlans");
    require(report.evaluations.size() == state.candidate_artifact_plans.size(),
            "v28.6 evaluation count should equal plan count");
    require(report.evaluations.size() == repeated_report.evaluations.size() &&
            !repeated_report.evaluations.empty() &&
            report.evaluations.front().id == repeated_report.evaluations.front().id &&
            report.evaluations.front().decision == repeated_report.evaluations.front().decision &&
            report.evaluations.front().readiness_score == repeated_report.evaluations.front().readiness_score &&
            report.evaluations.front().risk_score == repeated_report.evaluations.front().risk_score,
            "v28.6 evaluation output should be deterministic");

    std::set<std::string> evaluation_ids;
    bool unique_ids = true;
    bool references_existing_plans = true;
    bool generation_disabled = true;
    bool materialization_disabled = true;
    for (const CandidateArtifactPlanEvaluation& evaluation : report.evaluations) {
        unique_ids = unique_ids && !evaluation.id.empty() && evaluation_ids.insert(evaluation.id).second;
        references_existing_plans = references_existing_plans &&
            std::any_of(state.candidate_artifact_plans.begin(), state.candidate_artifact_plans.end(), [&](const CandidateArtifactPlan& plan) {
                return plan.id == evaluation.plan_id;
            });
        generation_disabled = generation_disabled && !evaluation.current_generation_enabled;
        materialization_disabled = materialization_disabled && !evaluation.current_materialization_enabled;
    }
    require(unique_ids, "v28.6 evaluation IDs should be unique");
    require(references_existing_plans, "v28.6 every evaluation should reference an existing plan");
    require(generation_disabled, "v28.6 no evaluation should enable current generation");
    require(materialization_disabled, "v28.6 no evaluation should enable current materialization");
    require(validate_candidate_artifact_plan_evaluations(state).empty(),
            "v28.6 computed CandidateArtifactPlanEvaluations should validate");

    ArchiveEngineState missing_plan_state = state;
    if (!missing_plan_state.candidate_artifact_plan_evaluations.empty()) {
        missing_plan_state.candidate_artifact_plan_evaluations.front().plan_id = "candidate_artifact_plan.missing_for_v28_6";
        require(!validate_candidate_artifact_plan_evaluations(missing_plan_state).empty(),
                "v28.6 validation should reject missing plan references");
    }

    ArchiveEngineState duplicate_id_state = state;
    if (duplicate_id_state.candidate_artifact_plan_evaluations.size() >= 2U) {
        duplicate_id_state.candidate_artifact_plan_evaluations[1].id = duplicate_id_state.candidate_artifact_plan_evaluations[0].id;
        require(!validate_candidate_artifact_plan_evaluations(duplicate_id_state).empty(),
                "v28.6 validation should reject duplicate evaluation IDs");
    }

    ArchiveEngineState bad_score_state = state;
    if (!bad_score_state.candidate_artifact_plan_evaluations.empty()) {
        bad_score_state.candidate_artifact_plan_evaluations.front().readiness_score = 2.0;
        require(!validate_candidate_artifact_plan_evaluations(bad_score_state).empty(),
                "v28.6 validation should reject scores outside range");
    }

    ArchiveEngineState pass_error_state = state;
    if (!pass_error_state.candidate_artifact_plan_evaluations.empty()) {
        CandidateArtifactPlanEvaluation& evaluation = pass_error_state.candidate_artifact_plan_evaluations.front();
        evaluation.decision = CandidateArtifactPlanEvaluationDecision::Pass;
        CandidateArtifactPlanEvaluationFinding finding;
        finding.id = evaluation.id + ".forced_error";
        finding.gate = CandidateArtifactPlanEvaluationGate::KnowledgeHorizon;
        finding.severity = CandidateArtifactPlanEvaluationSeverity::Error;
        finding.message = "forced error";
        evaluation.findings.push_back(finding);
        require(!validate_candidate_artifact_plan_evaluations(pass_error_state).empty(),
                "v28.6 validation should reject Pass decisions with error findings");
    }

    require(!report.evaluations.empty() &&
            !to_string(report.evaluations.front().decision).empty() &&
            report.evaluations.front().readiness_score >= 0.0 &&
            report.evaluations.front().risk_score >= 0.0,
            "v28.6 decision classification and scoring should be deterministic");

    const ArchiveSnapshot snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const ArchiveSnapshot repeated_snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const std::string formatted_snapshot = format_archive_snapshot(snapshot);
    require(snapshot.candidate_artifact_plan_evaluation_count == report.evaluations.size() &&
            contains_substr(formatted_snapshot, "candidate_artifact_plan_evaluation_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_plan_evaluation_pass_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_plan_evaluation_blocked_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_plan_evaluation_review_count"),
            "v28.6 ArchiveSnapshot should include CandidateArtifactPlanEvaluation counts");
    require(snapshot.summary_digest == repeated_snapshot.summary_digest,
            "v28.6 summary_digest should include stable CandidateArtifactPlanEvaluation material");

    const std::string public_summary = format_candidate_artifact_plan_evaluation_summary(state, AccessLevel::Public);
    const std::string public_list = format_candidate_artifact_plan_evaluation_list(state, AccessLevel::Public);
    const std::string public_detail = format_candidate_artifact_plan_evaluation_detail(state, AccessLevel::Public, report.evaluations.front().id);
    require(contains_substr(public_summary, "CandidateArtifactPlanEvaluation summary") &&
            contains_substr(public_list, "public_safe_visible_evaluations") &&
            contains_substr(public_detail, "found: false") &&
            !contains_substr(public_summary, "knowledge_horizon.") &&
            !contains_substr(public_list, "candidate_artifact_plan.evidence_potential") &&
            !contains_substr(public_detail, "plan_id:") &&
            !contains_substr(public_detail, "related_id=") &&
            !contains_substr(public_detail, "Findings:"),
            "v28.6 public CandidateArtifactPlanEvaluation formatting should avoid hidden diagnostics");

    const std::string curator_detail = format_candidate_artifact_plan_evaluation_detail(state, AccessLevel::Curator, report.evaluations.front().id);
    require(contains_substr(curator_detail, "found: true") &&
            contains_substr(curator_detail, "plan_id:") &&
            contains_substr(curator_detail, "Findings:") &&
            contains_substr(curator_detail, "gate=") &&
            contains_substr(curator_detail, "related_id="),
            "v28.6 curator CandidateArtifactPlanEvaluation detail should expose full diagnostics");

    const CapturedCliRun summary_cli = run_cli_captured({"impossible_archive_mvp_v28_7_4", "--runtime", "fixed-fixture", "--query", "candidate-artifact-plan-evaluation-summary"});
    require(summary_cli.exit_code == EXIT_SUCCESS && contains_substr(summary_cli.stdout_text, "CandidateArtifactPlanEvaluation summary"),
            "v28.6 CLI should expose CandidateArtifactPlanEvaluation summary query");
    const CapturedCliRun validation_cli = run_cli_captured({"impossible_archive_mvp_v28_7_4", "--runtime", "fixed-fixture", "--query", "validate-candidate-artifact-plan-evaluations"});
    require(validation_cli.exit_code == EXIT_SUCCESS && contains_substr(validation_cli.stdout_text, "CandidateArtifactPlanEvaluation validation") && contains_substr(validation_cli.stdout_text, "result: passed"),
            "v28.6 CLI should expose CandidateArtifactPlanEvaluation validation query");
    const CapturedCliRun list_cli = run_cli_captured({"impossible_archive_mvp_v28_7_4", "--runtime", "fixed-fixture", "--access", "curator", "--query", "list-candidate-artifact-plan-evaluations"});
    require(list_cli.exit_code == EXIT_SUCCESS && contains_substr(list_cli.stdout_text, "candidate_artifact_plan_evaluation."),
            "v28.6 CLI should expose CandidateArtifactPlanEvaluation list query");
    const CapturedCliRun detail_cli = run_cli_captured({"impossible_archive_mvp_v28_7_4", "--runtime", "fixed-fixture", "--access", "curator", "--query", "show-candidate-artifact-plan-evaluation", "--candidate-artifact-plan-evaluation-id", report.evaluations.front().id});
    require(detail_cli.exit_code == EXIT_SUCCESS && contains_substr(detail_cli.stdout_text, "CandidateArtifactPlanEvaluation") && contains_substr(detail_cli.stdout_text, "found: true"),
            "v28.6 CLI should expose CandidateArtifactPlanEvaluation detail query");

    const GoldenFixtureBuildResult fragment_fixture = build_golden_fixture_world("fixture.fragment_catalog_only");
    require(fragment_fixture.ok, "v28.6 fragment catalog fixture should still build");
    if (fragment_fixture.ok) {
        const CandidateArtifactPlanEvaluationReport fragment_report = evaluate_candidate_artifact_plans(fragment_fixture.state, AccessLevel::Curator);
        bool fragment_inert = true;
        for (const CandidateArtifactPlanEvaluation& evaluation : fragment_report.evaluations) {
            fragment_inert = fragment_inert &&
                            !contains_substr(evaluation.id, "fragment.") &&
                            !contains_substr(evaluation.plan_id, "fragment.");
        }
        require(fragment_inert,
                "v28.6 fragment catalog fixture should not activate fragment-derived evaluations");
    }
}


void run_v28_7_candidate_artifact_proposal_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    ArchiveEngineState state = initialize_archive_engine(42);
    derive_evidence_potentials_into_state(state);
    derive_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    evaluate_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    draft_candidate_artifact_proposals_into_state(state, AccessLevel::Curator);

    const CandidateArtifactProposalReport report = draft_candidate_artifact_proposals(state, AccessLevel::Curator);
    const CandidateArtifactProposalReport repeated_report = draft_candidate_artifact_proposals(state, AccessLevel::Curator);
    require(!report.proposals.empty(),
            "v28.7 default fixture should draft CandidateArtifactProposals");
    require(report.proposals.size() == state.candidate_artifact_plan_evaluations.size(),
            "v28.7 proposal count should equal evaluation count");
    require(!repeated_report.proposals.empty() && report.proposals.front().id == repeated_report.proposals.front().id &&
            report.proposals.back().id == repeated_report.proposals.back().id,
            "v28.7 CandidateArtifactProposal output should be deterministic");

    std::set<std::string> proposal_ids;
    bool ids_unique = true;
    bool every_evaluation_exists = true;
    bool every_plan_exists = true;
    bool no_generation = true;
    bool no_materialization = true;
    bool no_mutation = true;
    for (const CandidateArtifactProposal& proposal : report.proposals) {
        ids_unique = ids_unique && proposal_ids.insert(proposal.id).second;
        every_evaluation_exists = every_evaluation_exists && std::any_of(state.candidate_artifact_plan_evaluations.begin(), state.candidate_artifact_plan_evaluations.end(), [&](const CandidateArtifactPlanEvaluation& evaluation) {
            return evaluation.id == proposal.evaluation_id;
        });
        every_plan_exists = every_plan_exists && std::any_of(state.candidate_artifact_plans.begin(), state.candidate_artifact_plans.end(), [&](const CandidateArtifactPlan& plan) {
            return plan.id == proposal.plan_id;
        });
        no_generation = no_generation && !proposal.current_generation_enabled;
        no_materialization = no_materialization && !proposal.current_materialization_enabled;
        no_mutation = no_mutation && !proposal.archive_mutation_enabled;
    }
    require(ids_unique, "v28.7 CandidateArtifactProposal IDs should be unique");
    require(every_evaluation_exists, "v28.7 every proposal should reference an existing evaluation");
    require(every_plan_exists, "v28.7 every proposal should reference an existing plan");
    require(no_generation, "v28.7 no proposal should enable generation");
    require(no_materialization, "v28.7 no proposal should enable materialization");
    require(no_mutation, "v28.7 no proposal should enable archive mutation");
    require(validate_candidate_artifact_proposals(state).empty(),
            "v28.7 computed CandidateArtifactProposals should validate");

    ArchiveEngineState missing_eval_state = state;
    if (!missing_eval_state.candidate_artifact_proposals.empty()) {
        missing_eval_state.candidate_artifact_proposals.front().evaluation_id = "candidate_artifact_plan_evaluation.missing_for_v28_7";
        require(!validate_candidate_artifact_proposals(missing_eval_state).empty(),
                "v28.7 validation should reject missing evaluation references");
    }

    ArchiveEngineState missing_plan_state = state;
    if (!missing_plan_state.candidate_artifact_proposals.empty()) {
        missing_plan_state.candidate_artifact_proposals.front().plan_id = "candidate_artifact_plan.missing_for_v28_7";
        require(!validate_candidate_artifact_proposals(missing_plan_state).empty(),
                "v28.7 validation should reject missing plan references");
    }

    ArchiveEngineState duplicate_state = state;
    if (duplicate_state.candidate_artifact_proposals.size() >= 2U) {
        duplicate_state.candidate_artifact_proposals[1].id = duplicate_state.candidate_artifact_proposals[0].id;
        require(!validate_candidate_artifact_proposals(duplicate_state).empty(),
                "v28.7 validation should reject duplicate proposal IDs");
    }

    ArchiveEngineState enabled_state = state;
    if (!enabled_state.candidate_artifact_proposals.empty()) {
        enabled_state.candidate_artifact_proposals.front().current_generation_enabled = true;
        require(!validate_candidate_artifact_proposals(enabled_state).empty(),
                "v28.7 validation should reject generation-enabled proposals");
        enabled_state = state;
        enabled_state.candidate_artifact_proposals.front().current_materialization_enabled = true;
        require(!validate_candidate_artifact_proposals(enabled_state).empty(),
                "v28.7 validation should reject materialization-enabled proposals");
        enabled_state = state;
        enabled_state.candidate_artifact_proposals.front().archive_mutation_enabled = true;
        require(!validate_candidate_artifact_proposals(enabled_state).empty(),
                "v28.7 validation should reject archive-mutation-enabled proposals");
    }

    ArchiveEngineState draftable_blocked_state = state;
    if (!draftable_blocked_state.candidate_artifact_proposals.empty()) {
        CandidateArtifactProposal& proposal = draftable_blocked_state.candidate_artifact_proposals.front();
        proposal.decision = CandidateArtifactProposalDecision::Draftable;
        proposal.blocking_evaluation_finding_ids.push_back("candidate_artifact_plan_evaluation.synthetic.finding.error");
        require(!validate_candidate_artifact_proposals(draftable_blocked_state).empty(),
                "v28.7 validation should reject Draftable proposals with blocking findings");
    }

    const std::string public_summary = format_candidate_artifact_proposal_summary(state, AccessLevel::Public);
    const std::string public_list = format_candidate_artifact_proposal_list(state, AccessLevel::Public);
    const std::string public_detail = format_candidate_artifact_proposal_detail(state, AccessLevel::Public, report.proposals.front().id);
    require(contains_substr(public_summary, "CandidateArtifactProposal summary") &&
            contains_substr(public_list, "public_safe_visible_proposals") &&
            contains_substr(public_detail, "found: false") &&
            !contains_substr(public_summary, "source_evidence_potential_id") &&
            !contains_substr(public_list, "candidate_artifact_plan_evaluation.") &&
            !contains_substr(public_detail, "evaluation_id:") &&
            !contains_substr(public_detail, "plan_id:") &&
            !contains_substr(public_detail, "Proposed validation gates:"),
            "v28.7 public CandidateArtifactProposal formatting should avoid hidden diagnostics");

    const std::string curator_detail = format_candidate_artifact_proposal_detail(state, AccessLevel::Curator, report.proposals.front().id);
    require(contains_substr(curator_detail, "found: true") &&
            contains_substr(curator_detail, "evaluation_id:") &&
            contains_substr(curator_detail, "plan_id:") &&
            contains_substr(curator_detail, "source_evidence_potential_id:") &&
            contains_substr(curator_detail, "Proposed claim skeletons:") &&
            contains_substr(curator_detail, "Proposed validation gates:"),
            "v28.7 curator CandidateArtifactProposal detail should expose full diagnostics");

    const ArchiveSnapshot snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const ArchiveSnapshot repeated_snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const std::string formatted_snapshot = format_archive_snapshot(snapshot);
    require(snapshot.candidate_artifact_proposal_count == report.proposals.size() &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_draftable_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_blocked_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_review_count"),
            "v28.7 ArchiveSnapshot should include CandidateArtifactProposal counts");
    require(snapshot.summary_digest == repeated_snapshot.summary_digest,
            "v28.7 summary_digest should include stable CandidateArtifactProposal material");

    const CapturedCliRun summary_cli = run_cli_captured({"impossible_archive_mvp_v28_7_4", "--runtime", "fixed-fixture", "--query", "candidate-artifact-proposal-summary"});
    require(summary_cli.exit_code == EXIT_SUCCESS && contains_substr(summary_cli.stdout_text, "CandidateArtifactProposal summary"),
            "v28.7 CLI should expose CandidateArtifactProposal summary query");
    const CapturedCliRun validation_cli = run_cli_captured({"impossible_archive_mvp_v28_7_4", "--runtime", "fixed-fixture", "--query", "validate-candidate-artifact-proposals"});
    require(validation_cli.exit_code == EXIT_SUCCESS && contains_substr(validation_cli.stdout_text, "CandidateArtifactProposal validation") && contains_substr(validation_cli.stdout_text, "result: passed"),
            "v28.7 CLI should expose CandidateArtifactProposal validation query");
    const CapturedCliRun list_cli = run_cli_captured({"impossible_archive_mvp_v28_7_4", "--runtime", "fixed-fixture", "--access", "curator", "--query", "list-candidate-artifact-proposals"});
    require(list_cli.exit_code == EXIT_SUCCESS && contains_substr(list_cli.stdout_text, "candidate_artifact_proposal."),
            "v28.7 CLI should expose CandidateArtifactProposal list query");
    const CapturedCliRun detail_cli = run_cli_captured({"impossible_archive_mvp_v28_7_4", "--runtime", "fixed-fixture", "--access", "curator", "--query", "show-candidate-artifact-proposal", "--candidate-artifact-proposal-id", report.proposals.front().id});
    require(detail_cli.exit_code == EXIT_SUCCESS && contains_substr(detail_cli.stdout_text, "CandidateArtifactProposal") && contains_substr(detail_cli.stdout_text, "found: true"),
            "v28.7 CLI should expose CandidateArtifactProposal detail query");

    const GoldenFixtureBuildResult fragment_fixture = build_golden_fixture_world("fixture.fragment_catalog_only");
    require(fragment_fixture.ok, "v28.7 fragment catalog fixture should still build");
    if (fragment_fixture.ok) {
        const CandidateArtifactProposalReport fragment_report = draft_candidate_artifact_proposals(fragment_fixture.state, AccessLevel::Curator);
        bool fragment_inert = true;
        for (const CandidateArtifactProposal& proposal : fragment_report.proposals) {
            fragment_inert = fragment_inert &&
                            !contains_substr(proposal.id, "fragment.") &&
                            !contains_substr(proposal.plan_id, "fragment.") &&
                            !contains_substr(proposal.evaluation_id, "fragment.") &&
                            !contains_substr(proposal.source_evidence_potential_id, "fragment.");
        }
        require(fragment_inert,
                "v28.7 fragment catalog fixture should not activate fragment-derived proposals");
    }
}

void run_v28_7_2_public_archive_invariant_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    auto require_throws = [&](auto&& fn, const std::string& message) {
        bool threw = false;
        try {
            fn();
        } catch (const std::exception&) {
            threw = true;
        }
        require(threw, message);
    };

    auto make_artifact = [](std::string id) {
        Artifact artifact;
        artifact.id = std::move(id);
        artifact.title = "PublicArchive Invariant Test Artifact";
        artifact.creator_id = "person.ivara";
        artifact.attributed_creator_id = "person.ivara";
        artifact.true_creation_year = 620;
        artifact.claimed_creation_year = 620;
        artifact.discovery_year = 810;
        artifact.location_created = "site.reservoir_gate";
        artifact.location_found = "site.reservoir_gate";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.upper_lattice";
        artifact.script_id = "script.green_seal";
        artifact.material = "test tablet";
        artifact.preservation_quality = 0.75;
        artifact.damage_profile = "minor edge loss";
        artifact.transmission_history = "primary test artifact";
        artifact.creator_knowledge_scope = "test context";
        artifact.creator_bias_profile = "test bias";
        artifact.creator_motive = "test motive";
        artifact.intended_audience = "test audience";
        artifact.public_text = "test artifact text";
        artifact.literal_translation = "test artifact text";
        artifact.scholarly_translation = "test artifact text";
        artifact.hidden_event_links = {"event.salt_moon_schism"};
        artifact.referenced_entity_ids = {"site.reservoir_gate"};
        artifact.distortion_profile = "narrow scope";
        artifact.evidence_modifiers = {EvidenceModifier::NarrowScope};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.75;
            value.preservation_integrity = 0.75;
            value.temporal_proximity = 0.75;
            value.creator_access_to_events = 0.75;
            value.bias_penalty = 0.0;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.75;
            value.external_corroboration = 0.75;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        artifact.generation_trace = "v28.7.2 invariant test artifact";
        return finalize_artifact(std::move(artifact));
    };

    auto make_claim = [](std::string id, std::string source_artifact_id) {
        return Claim{
            std::move(id),
            std::move(source_artifact_id),
            ClaimType::FactualClaim,
            "test subject",
            "mentions",
            "test object",
            "test subject mentions test object",
            0.70,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::ExistedInYear, std::nullopt, std::nullopt, std::optional<int>{620}},
        };
    };

    {
        PublicArchive archive;
        require_throws([&] {
            archive.add_claim_to_artifact("artifact.missing", make_claim("claim.missing_artifact", "artifact.missing"));
        }, "v28.7.2 adding claim to missing artifact should reject");
        require(archive.claims().empty() && archive.artifacts().empty(),
                "v28.7.2 missing-artifact claim insertion should leave archive unchanged");
    }

    {
        PublicArchive archive;
        archive.add_artifact(make_artifact("artifact.relationship_owner"));
        archive.add_claim_to_artifact(
            "artifact.relationship_owner",
            make_claim("claim.relationship_owner.primary", "artifact.relationship_owner"),
            ArtifactVoiceClaimRole::PrimaryLine,
            1.0
        );
        const Artifact* artifact = archive.find_artifact("artifact.relationship_owner");
        require(artifact != nullptr && artifact->claim_ids.size() == 1 && artifact->claim_ids.front() == "claim.relationship_owner.primary" &&
                artifact->voice_claim_links.size() == 1 && artifact->voice_claim_links.front().claim_id == "claim.relationship_owner.primary",
                "v28.7.2 valid add_claim_to_artifact should link claim and voice claim through PublicArchive");

        const std::vector<std::string> claim_links_before = artifact != nullptr ? artifact->claim_ids : std::vector<std::string>{};
        const std::vector<ArtifactVoiceClaimLink> voice_links_before = artifact != nullptr ? artifact->voice_claim_links : std::vector<ArtifactVoiceClaimLink>{};
        const std::size_t claim_count_before = archive.claims().size();
        require_throws([&] {
            archive.add_claim_to_artifact(
                "artifact.relationship_owner",
                make_claim("claim.relationship_owner.primary", "artifact.relationship_owner"),
                ArtifactVoiceClaimRole::SecondaryLine,
                1.0
            );
        }, "v28.7.2 duplicate claim ID should reject through add_claim_to_artifact");
        artifact = archive.find_artifact("artifact.relationship_owner");
        require(artifact != nullptr && artifact->claim_ids == claim_links_before &&
                artifact->voice_claim_links.size() == voice_links_before.size() &&
                archive.claims().size() == claim_count_before,
                "v28.7.2 duplicate claim rejection should preserve artifact claim and voice links");

        require_throws([&] {
            archive.add_claim_to_artifact(
                "artifact.relationship_owner",
                make_claim("claim.relationship_owner.mismatch", "artifact.other"),
                ArtifactVoiceClaimRole::PrimaryLine,
                1.0
            );
        }, "v28.7.2 source artifact mismatch should reject through add_claim_to_artifact");
        artifact = archive.find_artifact("artifact.relationship_owner");
        require(artifact != nullptr && artifact->claim_ids == claim_links_before &&
                artifact->voice_claim_links.size() == voice_links_before.size() &&
                archive.find_claim("claim.relationship_owner.mismatch") == nullptr,
                "v28.7.2 source-mismatch rejection should leave archive unchanged");

        archive.add_claim_to_artifact(
            "artifact.relationship_owner",
            make_claim("claim.relationship_owner.no_voice", "artifact.relationship_owner"),
            ArtifactVoiceClaimRole::CatalogNote,
            0.0
        );
        artifact = archive.find_artifact("artifact.relationship_owner");
        require(artifact != nullptr &&
                std::find(artifact->claim_ids.begin(), artifact->claim_ids.end(), "claim.relationship_owner.no_voice") != artifact->claim_ids.end() &&
                std::none_of(artifact->voice_claim_links.begin(), artifact->voice_claim_links.end(), [](const ArtifactVoiceClaimLink& link) {
                    return link.claim_id == "claim.relationship_owner.no_voice";
                }),
                "v28.7.2 voice claim link should be added only when voice_weight is positive");
    }

    {
        PublicArchive archive;
        Artifact detached = make_artifact("artifact.detached_builder");
        require_throws([&] {
            add_claim_to_archive(archive, detached, make_claim("claim.detached.mismatch", "artifact.other"));
        }, "v28.7.2 quarantined detached helper should reject source artifact mismatch before mutation");
        require(detached.claim_ids.empty() && detached.voice_claim_links.empty() && archive.claims().empty(),
                "v28.7.2 detached-helper rejection should preserve detached artifact and archive state");
    }

    {
        ArchiveEngineState first = initialize_archive_engine(42);
        ArchiveEngineState second = initialize_archive_engine(42);
        const ArchiveSnapshot first_snapshot = build_archive_snapshot(first, "fixture.synthetic_fixed", first.seed, kOpenEndedYear, kOpenEndedYear);
        const ArchiveSnapshot second_snapshot = build_archive_snapshot(second, "fixture.synthetic_fixed", second.seed, kOpenEndedYear, kOpenEndedYear);
        require(first_snapshot.summary_digest == second_snapshot.summary_digest,
                "v28.7.2 PublicArchive invariant hardening should preserve deterministic snapshots");
    }
}

} // namespace


void run_v28_7_2_candidate_artifact_proposal_access_neutral_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    auto prepare_state = [](AccessLevel proposal_access) {
        ArchiveEngineState state = initialize_archive_engine(42);
        derive_evidence_potentials_into_state(state);
        derive_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
        evaluate_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
        draft_candidate_artifact_proposals_into_state(state, proposal_access);
        return state;
    };

    auto serialize_proposals = [](const std::vector<CandidateArtifactProposal>& proposals) {
        std::ostringstream out;
        for (const CandidateArtifactProposal& proposal : proposals) {
            out << proposal.id << "|"
                << proposal.plan_id << "|"
                << proposal.evaluation_id << "|"
                << proposal.source_evidence_potential_id << "|"
                << to_string(proposal.decision) << "|"
                << to_string(proposal.completeness) << "|"
                << to_string(proposal.visibility_class) << "|"
                << (proposal.contains_hidden_source_reference ? "hidden_ref" : "no_hidden_ref") << "|"
                << (proposal.contains_curator_diagnostics ? "diagnostics" : "no_diagnostics") << "|"
                << (proposal.touches_protected_mystery ? "protected" : "not_protected") << "|"
                << (proposal.requires_curator_review ? "review" : "no_review") << "|"
                << to_string(proposal.text_status) << "|"
                << to_string(proposal.proposed_shape) << "|"
                << to_string(proposal.proposed_artifact_type) << "|"
                << to_string(proposal.proposed_voice_register) << "|"
                << proposal.proposed_title << "|"
                << proposal.target_topic << "|"
                << proposal.proposed_creation_year << "|"
                << proposal.proposed_discovery_year << "|"
                << proposal.evidence_role << "|"
                << proposal.proposal_rationale << "|"
                << (proposal.current_generation_enabled ? "gen" : "no_gen") << "|"
                << (proposal.current_materialization_enabled ? "mat" : "no_mat") << "|"
                << (proposal.archive_mutation_enabled ? "mut" : "no_mut") << "\n";
        }
        return out.str();
    };

    ArchiveEngineState public_state = prepare_state(AccessLevel::Public);
    ArchiveEngineState curator_state = prepare_state(AccessLevel::Curator);
    ArchiveEngineState scholar_state = prepare_state(AccessLevel::Scholar);
    ArchiveEngineState debug_state = prepare_state(AccessLevel::Debug);

    require(serialize_proposals(public_state.candidate_artifact_proposals) ==
                serialize_proposals(curator_state.candidate_artifact_proposals),
            "v28.7.2 proposal derivation under public access should produce the same stored proposals as curator access");
    require(serialize_proposals(scholar_state.candidate_artifact_proposals) ==
                serialize_proposals(debug_state.candidate_artifact_proposals),
            "v28.7.2 proposal derivation under scholar access should produce the same stored proposals as debug access");

    const ArchiveSnapshot public_snapshot = build_archive_snapshot(public_state, "fixture.synthetic_access_public", public_state.seed, kOpenEndedYear, kOpenEndedYear);
    const ArchiveSnapshot curator_snapshot = build_archive_snapshot(curator_state, "fixture.synthetic_access_public", curator_state.seed, kOpenEndedYear, kOpenEndedYear);
    require(public_snapshot.summary_digest == curator_snapshot.summary_digest,
            "v28.7.2 summary_digest should not change based only on proposal derivation access level");

    require(validate_full_state(curator_state).empty(),
            "v28.7.2 validate_full_state should include and accept valid persistent CandidateArtifactProposals");

    ArchiveEngineState malformed_state = curator_state;
    if (!malformed_state.candidate_artifact_proposals.empty()) {
        malformed_state.candidate_artifact_proposals.front().current_generation_enabled = true;
        require(!validate_full_state(malformed_state).empty(),
                "v28.7.2 malformed persistent CandidateArtifactProposal should fail full-state validation");
    }

    const CandidateArtifactProposalReport public_report = draft_candidate_artifact_proposals(curator_state, AccessLevel::Public);
    const CandidateArtifactProposalReport debug_report = draft_candidate_artifact_proposals(curator_state, AccessLevel::Debug);
    require(serialize_proposals(public_report.proposals) == serialize_proposals(debug_report.proposals),
            "v28.7.2 report drafting should keep stored proposal facts access-neutral");

    if (!curator_state.candidate_artifact_proposals.empty()) {
        const std::string proposal_id = curator_state.candidate_artifact_proposals.front().id;
        const std::string public_detail = format_candidate_artifact_proposal_detail(curator_state, AccessLevel::Public, proposal_id);
        const std::string curator_detail = format_candidate_artifact_proposal_detail(curator_state, AccessLevel::Curator, proposal_id);
        const std::vector<std::string> forbidden_public_fields = {
            "source_evidence_potential_id:",
            "evaluation_id:",
            "plan_id:",
            "blocking_evaluation_finding_ids:",
            "knowledge_horizon.",
            "contradiction_budget.",
            "protected mystery",
            "proposal_rationale:",
            "Warnings:"
        };
        bool public_clean = true;
        for (const std::string& forbidden : forbidden_public_fields) {
            public_clean = public_clean && !contains_substr(public_detail, forbidden);
        }
        require(public_clean,
                "v28.7.2 public proposal detail should hide hidden diagnostics and proposal internals");
        require(contains_substr(curator_detail, "evaluation_id:") &&
                contains_substr(curator_detail, "plan_id:") &&
                contains_substr(curator_detail, "source_evidence_potential_id:") &&
                contains_substr(curator_detail, "visibility_class:"),
                "v28.7.2 curator proposal detail should expose full access-neutral diagnostics");
    }
}



void run_v28_7_4_reliability_components_cleanup_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    ArchiveEngineState fixed_state = initialize_archive_engine(42);

    const Artifact* victory = fixed_state.public_archive.find_artifact("artifact.victory_inscription");
    require(victory != nullptr,
            "v28.7.4 representative fixture artifact should exist");
    if (victory != nullptr) {
        require(victory->reliability_components.provenance_confidence == 0.86 &&
                victory->reliability_components.preservation_integrity == 0.82 &&
                victory->reliability_components.temporal_proximity == 0.90 &&
                victory->reliability_components.creator_access_to_events == 0.68 &&
                victory->reliability_components.bias_penalty == 0.45 &&
                victory->reliability_components.forgery_penalty == 0.0 &&
                victory->reliability_components.translation_confidence == 0.72 &&
                victory->reliability_components.external_corroboration == 0.45 &&
                victory->reliability_components.contradiction_penalty == 0.15,
                "v28.7.4 explicit ReliabilityComponents initialization should preserve representative values");
    }

    const ArchiveSnapshot first = build_archive_snapshot(
        fixed_state,
        "fixture.synthetic_fixed",
        fixed_state.seed,
        kOpenEndedYear,
        kOpenEndedYear
    );
    const ArchiveSnapshot second = build_archive_snapshot(
        fixed_state,
        "fixture.synthetic_fixed",
        fixed_state.seed,
        kOpenEndedYear,
        kOpenEndedYear
    );
    require(first.summary_digest == second.summary_digest,
            "v28.7.4 snapshot summary_digest should remain deterministic after ReliabilityComponents cleanup");
}

void run_v28_7_4_cli_argument_view_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const CliArgs normal_query{
        "impossible_archive_mvp_v28_7_4",
        "--query",
        "archive-snapshot",
        "--fixture-id",
        "fixture.default_archive"
    };
    const CliOptions parsed = parse_cli(normal_query);
    require(parsed.query == "archive-snapshot" && parsed.fixture_id == "fixture.default_archive",
            "v28.7.4 parse_cli(CliArgs) should parse normal fixture snapshot query");

    const CapturedCliRun snapshot_cli = run_cli_captured(normal_query);
    require(snapshot_cli.exit_code == EXIT_SUCCESS && contains_substr(snapshot_cli.stdout_text, "snapshot_id:"),
            "v28.7.4 run_cli(CliArgs) should execute normal fixture snapshot query");

    const CapturedCliRun spec_reject = run_cli_captured({
        "impossible_archive_mvp_v28_7_4",
        "--query", "archive-snapshot",
        "--fixture-id", "fixture.default_archive",
        "--spec-file", "definitely_missing.json"
    });
    require(spec_reject.exit_code != EXIT_SUCCESS &&
            contains_substr(spec_reject.stderr_text, "fixed golden fixture definition") &&
            !contains_substr(spec_reject.stderr_text, "failed to open"),
            "v28.7.4 fixture query should still reject --spec-file before spec-file access");

    const CapturedCliRun seed_reject = run_cli_captured({
        "impossible_archive_mvp_v28_7_4",
        "--query", "archive-snapshot",
        "--fixture-id", "fixture.default_archive",
        "--seed", "123"
    });
    require(seed_reject.exit_code != EXIT_SUCCESS &&
            contains_substr(seed_reject.stderr_text, "fixed golden fixture definition"),
            "v28.7.4 fixture query should still reject --seed");

    const CapturedCliRun partial_spec = run_cli_captured({
        "impossible_archive_mvp_v28_7_4",
        "--query", "bootstrap-civilization",
        "--spec-file", "examples/40_civilization_specs_v1_1.json"
    });
    require(partial_spec.exit_code != EXIT_SUCCESS &&
            contains_substr(partial_spec.stdout_text, "--spec-file requires --civilization-id"),
            "v28.7.4 partial spec flag expected failure should still work");

    const GoldenFixtureBuildResult cli_fixture = build_golden_fixture_world("fixture.default_archive");
    require(cli_fixture.ok,
            "v28.7.4 default fixture should build for CLI access-gate checks");
    const ArchiveEngineState& state = cli_fixture.state;
    const std::string kh_public = format_knowledge_horizon_finding_detail(
        state,
        AccessLevel::Public,
        "knowledge_horizon.0039"
    );
    require(contains_substr(kh_public, "found: false") &&
            !contains_substr(kh_public, "context_type:") &&
            !contains_substr(kh_public, "subject_type:"),
            "v28.7.4 KnowledgeHorizon public detail block should remain active");

    const CandidateArtifactProposalReport report = draft_candidate_artifact_proposals(state, AccessLevel::Curator);
    require(!report.proposals.empty(),
            "v28.7.4 default fixture should still draft candidate artifact proposals");
    if (!report.proposals.empty()) {
        const std::string proposal_public = format_candidate_artifact_proposal_detail(
            state,
            AccessLevel::Public,
            report.proposals.front().id
        );
        require(contains_substr(proposal_public, "found: false") &&
                !contains_substr(proposal_public, "evaluation_id:") &&
                !contains_substr(proposal_public, "plan_id:") &&
                !contains_substr(proposal_public, "source_evidence_potential_id:"),
                "v28.7.4 CandidateArtifactProposal public detail block should remain active");
    }
}


void run_v28_8_candidate_artifact_proposal_audit_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    ArchiveEngineState state = initialize_archive_engine(42);
    derive_evidence_potentials_into_state(state);
    derive_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    evaluate_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    draft_candidate_artifact_proposals_into_state(state, AccessLevel::Curator);
    audit_candidate_artifact_proposals_into_state(state, AccessLevel::Curator);

    const CandidateArtifactProposalAuditReport report = audit_candidate_artifact_proposals(state, AccessLevel::Curator);
    const CandidateArtifactProposalAuditReport repeated_report = audit_candidate_artifact_proposals(state, AccessLevel::Curator);
    require(!report.audits.empty(),
            "v28.8 default fixture should audit CandidateArtifactProposals");
    require(report.audits.size() == state.candidate_artifact_proposals.size(),
            "v28.8 audit count should equal proposal count");
    require(!repeated_report.audits.empty() && report.audits.front().id == repeated_report.audits.front().id &&
            report.audits.back().id == repeated_report.audits.back().id,
            "v28.8 CandidateArtifactProposalAudit output should be deterministic");

    std::set<std::string> audit_ids;
    bool ids_unique = true;
    bool every_proposal_exists = true;
    bool no_generation = true;
    bool no_materialization = true;
    bool no_mutation = true;
    for (const CandidateArtifactProposalAudit& audit : report.audits) {
        ids_unique = ids_unique && audit_ids.insert(audit.id).second;
        every_proposal_exists = every_proposal_exists && std::any_of(state.candidate_artifact_proposals.begin(), state.candidate_artifact_proposals.end(), [&](const CandidateArtifactProposal& proposal) {
            return proposal.id == audit.proposal_id;
        });
        no_generation = no_generation && !audit.current_generation_enabled;
        no_materialization = no_materialization && !audit.current_materialization_enabled;
        no_mutation = no_mutation && !audit.archive_mutation_enabled;
    }
    require(ids_unique, "v28.8 CandidateArtifactProposalAudit IDs should be unique");
    require(every_proposal_exists, "v28.8 every audit should reference an existing proposal");
    require(no_generation, "v28.8 no audit should enable generation");
    require(no_materialization, "v28.8 no audit should enable materialization");
    require(no_mutation, "v28.8 no audit should enable archive mutation");
    require(validate_candidate_artifact_proposal_audits(state).empty(),
            "v28.8 computed CandidateArtifactProposalAudits should validate");

    ArchiveEngineState missing_proposal_state = state;
    if (!missing_proposal_state.candidate_artifact_proposal_audits.empty()) {
        missing_proposal_state.candidate_artifact_proposal_audits.front().proposal_id = "candidate_artifact_proposal.missing_for_v28_8";
        require(!validate_candidate_artifact_proposal_audits(missing_proposal_state).empty(),
                "v28.8 validation should reject missing proposal references");
    }

    ArchiveEngineState duplicate_state = state;
    if (duplicate_state.candidate_artifact_proposal_audits.size() >= 2U) {
        duplicate_state.candidate_artifact_proposal_audits[1].id = duplicate_state.candidate_artifact_proposal_audits[0].id;
        require(!validate_candidate_artifact_proposal_audits(duplicate_state).empty(),
                "v28.8 validation should reject duplicate audit IDs");
    }

    ArchiveEngineState score_state = state;
    if (!score_state.candidate_artifact_proposal_audits.empty()) {
        score_state.candidate_artifact_proposal_audits.front().proposal_quality_score = 1.5;
        require(!validate_candidate_artifact_proposal_audits(score_state).empty(),
                "v28.8 validation should reject score outside range");
    }

    ArchiveEngineState pass_error_state = state;
    if (!pass_error_state.candidate_artifact_proposal_audits.empty()) {
        CandidateArtifactProposalAudit& audit = pass_error_state.candidate_artifact_proposal_audits.front();
        audit.decision = CandidateArtifactProposalAuditDecision::Pass;
        if (audit.findings.empty()) {
            CandidateArtifactProposalAuditFinding finding;
            finding.id = audit.id + ".synthetic_error";
            finding.gate = CandidateArtifactProposalAuditGate::Structure;
            finding.severity = CandidateArtifactProposalAuditSeverity::Error;
            finding.message = "synthetic error";
            audit.findings.push_back(finding);
        } else {
            audit.findings.front().severity = CandidateArtifactProposalAuditSeverity::Error;
        }
        require(!validate_candidate_artifact_proposal_audits(pass_error_state).empty(),
                "v28.8 validation should reject Pass with error finding");
    }

    if (!report.audits.empty()) {
        const CandidateArtifactProposalAuditDecision first_decision = classify_candidate_artifact_proposal_audit(report.audits.front());
        const CandidateArtifactProposalAuditDecision repeated_decision = classify_candidate_artifact_proposal_audit(report.audits.front());
        require(first_decision == repeated_decision,
                "v28.8 audit decision classification should be deterministic");
    }

    const ArchiveSnapshot snapshot = build_archive_snapshot(
        state,
        "fixture.synthetic_fixed",
        state.seed,
        kOpenEndedYear,
        kOpenEndedYear
    );
    const ArchiveSnapshot repeated_snapshot = build_archive_snapshot(
        state,
        "fixture.synthetic_fixed",
        state.seed,
        kOpenEndedYear,
        kOpenEndedYear
    );
    const std::string formatted_snapshot = format_archive_snapshot(snapshot);
    require(snapshot.candidate_artifact_proposal_audit_count == report.audits.size() &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_audit_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_audit_pass_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_audit_blocked_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_audit_review_count") &&
            contains_substr(formatted_snapshot, "candidate_artifact_proposal_audit_revision_count"),
            "v28.8 ArchiveSnapshot should include CandidateArtifactProposalAudit counts");
    require(snapshot.summary_digest == repeated_snapshot.summary_digest,
            "v28.8 summary_digest should include stable CandidateArtifactProposalAudit material");

    if (!report.audits.empty()) {
        const std::string public_summary = format_candidate_artifact_proposal_audit_summary(state, AccessLevel::Public);
        const std::string public_detail = format_candidate_artifact_proposal_audit_detail(state, AccessLevel::Public, report.audits.front().id);
        require(contains_substr(public_summary, "CandidateArtifactProposalAudit summary") &&
                contains_substr(public_detail, "found: false") &&
                !contains_substr(public_detail, "proposal_id:") &&
                !contains_substr(public_detail, "related_id=") &&
                !contains_substr(public_detail, "Findings:") &&
                !contains_substr(public_detail, "Required revisions:"),
                "v28.8 public CandidateArtifactProposalAudit formatting should avoid hidden diagnostics");

        const std::string curator_detail = format_candidate_artifact_proposal_audit_detail(state, AccessLevel::Curator, report.audits.front().id);
        require(contains_substr(curator_detail, "found: true") &&
                contains_substr(curator_detail, "proposal_id:") &&
                contains_substr(curator_detail, "Findings:") &&
                contains_substr(curator_detail, "gate=") &&
                contains_substr(curator_detail, "related_id="),
                "v28.8 curator CandidateArtifactProposalAudit detail should expose full diagnostics");

        const CapturedCliRun summary_cli = run_cli_captured({"impossible_archive_mvp_v28_8", "--runtime", "fixed-fixture", "--query", "candidate-artifact-proposal-audit-summary"});
        require(summary_cli.exit_code == EXIT_SUCCESS && contains_substr(summary_cli.stdout_text, "CandidateArtifactProposalAudit summary"),
                "v28.8 CLI should expose CandidateArtifactProposalAudit summary query");
        const CapturedCliRun validation_cli = run_cli_captured({"impossible_archive_mvp_v28_8", "--runtime", "fixed-fixture", "--query", "validate-candidate-artifact-proposal-audits"});
        require(validation_cli.exit_code == EXIT_SUCCESS && contains_substr(validation_cli.stdout_text, "CandidateArtifactProposalAudit validation") && contains_substr(validation_cli.stdout_text, "result: passed"),
                "v28.8 CLI should expose CandidateArtifactProposalAudit validation query");
        const CapturedCliRun list_cli = run_cli_captured({"impossible_archive_mvp_v28_8", "--runtime", "fixed-fixture", "--access", "curator", "--query", "list-candidate-artifact-proposal-audits"});
        require(list_cli.exit_code == EXIT_SUCCESS && contains_substr(list_cli.stdout_text, "candidate_artifact_proposal_audit."),
                "v28.8 CLI should expose CandidateArtifactProposalAudit list query");
        const CapturedCliRun detail_cli = run_cli_captured({"impossible_archive_mvp_v28_8", "--runtime", "fixed-fixture", "--access", "curator", "--query", "show-candidate-artifact-proposal-audit", "--candidate-artifact-proposal-audit-id", report.audits.front().id});
        require(detail_cli.exit_code == EXIT_SUCCESS && contains_substr(detail_cli.stdout_text, "CandidateArtifactProposalAudit") && contains_substr(detail_cli.stdout_text, "found: true"),
                "v28.8 CLI should expose CandidateArtifactProposalAudit detail query");
    }

    const GoldenFixtureBuildResult fragment_fixture = build_golden_fixture_world("fixture.fragment_catalog_only");
    require(fragment_fixture.ok, "v28.8 fragment catalog fixture should still build");
    if (fragment_fixture.ok) {
        const CandidateArtifactProposalAuditReport fragment_report = audit_candidate_artifact_proposals(fragment_fixture.state, AccessLevel::Curator);
        bool fragment_inert = true;
        for (const CandidateArtifactProposalAudit& audit : fragment_report.audits) {
            fragment_inert = fragment_inert &&
                            !contains_substr(audit.id, "fragment.") &&
                            !contains_substr(audit.proposal_id, "fragment.");
        }
        require(fragment_inert,
                "v28.8 fragment catalog fixture should not activate fragment-derived audits");
    }
}


void run_v28_9_control_layer_audit_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const ControlLayerAuditReport report = build_control_layer_audit_report();
    const ControlLayerAuditReport repeated_report = build_control_layer_audit_report();
    require(!report.entries.empty(), "v28.9 control-layer audit report should build");
    require(report.entries.size() == repeated_report.entries.size() &&
            !report.entries.empty() &&
            report.entries.front().id == repeated_report.entries.front().id &&
            report.entries.back().id == repeated_report.entries.back().id,
            "v28.9 control-layer audit report should be deterministic");

    const std::vector<std::string> required_ids{
        "control_layer.hidden_truth",
        "control_layer.public_archive",
        "control_layer.golden_fixtures",
        "control_layer.archive_snapshot",
        "control_layer.fragments",
        "control_layer.evidence_potential",
        "control_layer.knowledge_horizon",
        "control_layer.contradiction_budget",
        "control_layer.candidate_artifact_plan",
        "control_layer.candidate_artifact_plan_evaluation",
        "control_layer.candidate_artifact_proposal",
        "control_layer.candidate_artifact_proposal_audit",
        "control_layer.candidate_generation",
        "control_layer.candidate_materialization",
        "control_layer.hidden_cluster_preview",
        "control_layer.hidden_cluster_materialization",
        "control_layer.hidden_mutation_artifact_candidate",
        "control_layer.access_control",
        "control_layer.cli",
        "control_layer.smoke_workflow",
        "control_layer.self_tests",
        "control_layer.full_state_validation",
    };
    for (const std::string& id : required_ids) {
        require(std::any_of(report.entries.begin(), report.entries.end(), [&](const ControlLayerAuditEntry& entry) {
                    return entry.id == id;
                }),
                "v28.9 required control-layer audit entry should exist: " + id);
    }

    std::set<std::string> ids;
    bool ids_unique = true;
    bool inert_not_mutation_capable = true;
    bool mutation_capable_marked = true;
    for (const ControlLayerAuditEntry& entry : report.entries) {
        ids_unique = ids_unique && ids.insert(entry.id).second;
        if (entry.should_remain_inert && entry.can_mutate_state) {
            inert_not_mutation_capable = false;
        }
        if (entry.can_mutate_state && entry.behavior != ControlLayerBehavior::MutationCapable) {
            mutation_capable_marked = false;
        }
    }
    require(ids_unique, "v28.9 control-layer audit entry IDs should be unique");
    require(inert_not_mutation_capable, "v28.9 inert/advisory layers should not be marked mutation-capable");
    require(mutation_capable_marked, "v28.9 known mutation-capable layers should be explicitly marked");

    auto find_entry = [&](const std::string& id) -> const ControlLayerAuditEntry* {
        const auto it = std::find_if(report.entries.begin(), report.entries.end(), [&](const ControlLayerAuditEntry& entry) {
            return entry.id == id;
        });
        return it == report.entries.end() ? nullptr : &*it;
    };
    const ControlLayerAuditEntry* budget = find_entry("control_layer.contradiction_budget");
    require(budget != nullptr && budget->behavior == ControlLayerBehavior::TelemetryOnly && !budget->can_mutate_state,
            "v28.9 ContradictionBudget should be classified telemetry-only");
    const ControlLayerAuditEntry* potential = find_entry("control_layer.evidence_potential");
    require(potential != nullptr && potential->behavior == ControlLayerBehavior::ValidationOnly && potential->should_remain_inert,
            "v28.9 EvidencePotential should be classified inert/validation-only");
    const ControlLayerAuditEntry* proposal_audit = find_entry("control_layer.candidate_artifact_proposal_audit");
    require(proposal_audit != nullptr && proposal_audit->behavior == ControlLayerBehavior::AuditOnly && proposal_audit->should_remain_inert,
            "v28.9 CandidateArtifactProposalAudit should be classified audit-only");
    const ControlLayerAuditEntry* proposal = find_entry("control_layer.candidate_artifact_proposal");
    require(proposal != nullptr && proposal->behavior == ControlLayerBehavior::ProposalOnly && proposal->access_gated,
            "v28.9 CandidateArtifactProposal should be proposal-only and access-gated");

    ArchiveEngineState state = initialize_archive_engine(42);
    derive_evidence_potentials_into_state(state);
    derive_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    evaluate_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    draft_candidate_artifact_proposals_into_state(state, AccessLevel::Curator);
    audit_candidate_artifact_proposals_into_state(state, AccessLevel::Curator);
    build_control_layer_audit_into_state(state);

    const std::string public_summary = format_control_layer_audit_summary(state, AccessLevel::Public);
    const std::string public_detail = format_control_layer_audit_entry_detail(state, AccessLevel::Public, "control_layer.hidden_truth");
    require(contains_substr(public_summary, "ControlLayerAudit summary") &&
            contains_substr(public_summary, "aggregate control-layer status") &&
            contains_substr(public_detail, "found: false") &&
            !contains_substr(public_detail, "Primary files") &&
            !contains_substr(public_detail, "Validation functions") &&
            !contains_substr(public_detail, "Known gaps"),
            "v28.9 public control-layer audit formatting should avoid internal diagnostics");

    const std::string curator_detail = format_control_layer_audit_entry_detail(state, AccessLevel::Curator, "control_layer.candidate_artifact_proposal_audit");
    require(contains_substr(curator_detail, "found: true") &&
            contains_substr(curator_detail, "behavior: audit_only") &&
            contains_substr(curator_detail, "Primary files") &&
            contains_substr(curator_detail, "Validation functions") &&
            contains_substr(curator_detail, "Snapshot fields"),
            "v28.9 curator control-layer audit detail should expose full audit details");

    const ArchiveSnapshot snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const ArchiveSnapshot repeated_snapshot = build_archive_snapshot(state, "fixture.synthetic_fixed", state.seed, kOpenEndedYear, kOpenEndedYear);
    const std::string formatted_snapshot = format_archive_snapshot(snapshot);
    require(snapshot.control_layer_audit_entry_count == report.entries.size() &&
            snapshot.control_layer_audit_mutation_capable_count > 0U &&
            contains_substr(formatted_snapshot, "control_layer_audit_entry_count") &&
            contains_substr(formatted_snapshot, "control_layer_audit_mutation_capable_count") &&
            contains_substr(formatted_snapshot, "control_layer_audit_known_gap_count"),
            "v28.9 ArchiveSnapshot should include control-layer audit counts");
    require(snapshot.summary_digest == repeated_snapshot.summary_digest,
            "v28.9 summary_digest should include stable control-layer audit material");
    require(validate_control_layer_audit_report(report).empty(),
            "v28.9 validate-control-layer-audit should report no errors");

    const CapturedCliRun summary_cli = run_cli_captured({"impossible_archive_mvp_v28_11", "--runtime", "fixed-fixture", "--query", "control-layer-audit-summary"});
    require(summary_cli.exit_code == EXIT_SUCCESS && contains_substr(summary_cli.stdout_text, "ControlLayerAudit summary"),
            "v28.9 CLI should expose control-layer audit summary query");
    const CapturedCliRun validation_cli = run_cli_captured({"impossible_archive_mvp_v28_11", "--runtime", "fixed-fixture", "--query", "validate-control-layer-audit"});
    require(validation_cli.exit_code == EXIT_SUCCESS && contains_substr(validation_cli.stdout_text, "ControlLayerAudit validation") && contains_substr(validation_cli.stdout_text, "result: passed"),
            "v28.9 CLI should expose control-layer audit validation query");
    const CapturedCliRun list_cli = run_cli_captured({"impossible_archive_mvp_v28_11", "--runtime", "fixed-fixture", "--access", "curator", "--query", "list-control-layer-audit-entries"});
    require(list_cli.exit_code == EXIT_SUCCESS && contains_substr(list_cli.stdout_text, "control_layer.candidate_artifact_proposal_audit"),
            "v28.9 CLI should expose control-layer audit list query");
    const CapturedCliRun detail_cli = run_cli_captured({"impossible_archive_mvp_v28_11", "--runtime", "fixed-fixture", "--access", "curator", "--query", "show-control-layer-audit-entry", "--control-layer-audit-entry-id", "control_layer.candidate_artifact_proposal_audit"});
    require(detail_cli.exit_code == EXIT_SUCCESS && contains_substr(detail_cli.stdout_text, "ControlLayerAudit entry") && contains_substr(detail_cli.stdout_text, "found: true"),
            "v28.9 CLI should expose control-layer audit detail query");
}


void run_v28_10_proposal_quality_gate_policy_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    ArchiveEngineState state = initialize_archive_engine(42);
    derive_evidence_potentials_into_state(state);
    derive_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    evaluate_candidate_artifact_plans_into_state(state, AccessLevel::Curator);
    draft_candidate_artifact_proposals_into_state(state, AccessLevel::Curator);
    audit_candidate_artifact_proposals_into_state(state, AccessLevel::Curator);

    const CandidateArtifactProposalAuditPolicy policy = default_candidate_artifact_proposal_audit_policy();
    const CandidateArtifactProposalAuditPolicy repeated_policy = default_candidate_artifact_proposal_audit_policy();
    require(policy.min_quality_score_for_pass == repeated_policy.min_quality_score_for_pass &&
            policy.min_specificity_score_for_pass == repeated_policy.min_specificity_score_for_pass &&
            policy.min_safety_score_for_pass == repeated_policy.min_safety_score_for_pass &&
            policy.max_revision_pressure_for_pass == repeated_policy.max_revision_pressure_for_pass,
            "v28.10 default audit policy should be deterministic");

    const CandidateArtifactProposalAuditReport report = audit_candidate_artifact_proposals(state, AccessLevel::Curator);
    require(!report.audits.empty() && !state.candidate_artifact_proposals.empty(),
            "v28.10 fixture should provide proposal audits for policy checks");

    if (!report.audits.empty()) {
        bool non_pass_has_revisions = true;
        bool error_findings_have_reason_codes = true;
        for (const CandidateArtifactProposalAudit& audit : report.audits) {
            if (audit.decision != CandidateArtifactProposalAuditDecision::Pass &&
                audit.decision != CandidateArtifactProposalAuditDecision::Invalid) {
                non_pass_has_revisions = non_pass_has_revisions && !audit.required_revisions.empty();
            }
            for (const CandidateArtifactProposalAuditFinding& finding : audit.findings) {
                if (finding.severity == CandidateArtifactProposalAuditSeverity::Error) {
                    error_findings_have_reason_codes = error_findings_have_reason_codes &&
                        finding.reason_code != CandidateArtifactProposalAuditReasonCode::None;
                }
            }
        }
        require(non_pass_has_revisions, "v28.10 non-pass proposal audits should include required revisions");
        require(error_findings_have_reason_codes, "v28.10 error audit findings should carry reason codes");

        CandidateArtifactProposalAudit clean_pass;
        clean_pass.id = "candidate_artifact_proposal_audit.synthetic_policy_pass";
        clean_pass.proposal_id = state.candidate_artifact_proposals.front().id;
        clean_pass.structure_valid = true;
        clean_pass.source_continuity_valid = true;
        clean_pass.proposal_specificity_clear = true;
        clean_pass.voice_readiness_clear = true;
        clean_pass.claim_skeleton_safe = true;
        clean_pass.distortion_plausible = true;
        clean_pass.damage_plausible = true;
        clean_pass.knowledge_horizon_clear = true;
        clean_pass.contradiction_budget_clear = true;
        clean_pass.protected_mystery_clear = true;
        clean_pass.access_safe = true;
        clean_pass.proposal_quality_score = policy.min_quality_score_for_pass;
        clean_pass.specificity_score = policy.min_specificity_score_for_pass;
        clean_pass.safety_score = policy.min_safety_score_for_pass;
        clean_pass.revision_pressure_score = policy.max_revision_pressure_for_pass;
        CandidateArtifactProposalAuditFinding pass_info;
        pass_info.id = clean_pass.id + ".finding.0";
        pass_info.gate = CandidateArtifactProposalAuditGate::Structure;
        pass_info.severity = CandidateArtifactProposalAuditSeverity::Info;
        pass_info.reason_code = CandidateArtifactProposalAuditReasonCode::None;
        pass_info.message = "synthetic policy pass";
        pass_info.related_id = clean_pass.proposal_id;
        clean_pass.findings.push_back(pass_info);
        clean_pass.decision = classify_candidate_artifact_proposal_audit(clean_pass);
        require(clean_pass.decision == CandidateArtifactProposalAuditDecision::Pass,
                "v28.10 Pass audit should meet policy thresholds exactly");

        ArchiveEngineState pass_quality_state = state;
        clean_pass.proposal_quality_score = policy.min_quality_score_for_pass - 0.01;
        clean_pass.decision = CandidateArtifactProposalAuditDecision::Pass;
        pass_quality_state.candidate_artifact_proposal_audits = {clean_pass};
        require(!validate_candidate_artifact_proposal_audits(pass_quality_state).empty(),
                "v28.10 validation should reject Pass below quality threshold");

        ArchiveEngineState pass_specificity_state = state;
        clean_pass.proposal_quality_score = policy.min_quality_score_for_pass;
        clean_pass.specificity_score = policy.min_specificity_score_for_pass - 0.01;
        pass_specificity_state.candidate_artifact_proposal_audits = {clean_pass};
        require(!validate_candidate_artifact_proposal_audits(pass_specificity_state).empty(),
                "v28.10 validation should reject Pass below specificity threshold");

        ArchiveEngineState pass_safety_state = state;
        clean_pass.specificity_score = policy.min_specificity_score_for_pass;
        clean_pass.safety_score = policy.min_safety_score_for_pass - 0.01;
        pass_safety_state.candidate_artifact_proposal_audits = {clean_pass};
        require(!validate_candidate_artifact_proposal_audits(pass_safety_state).empty(),
                "v28.10 validation should reject Pass below safety threshold");

        ArchiveEngineState pass_pressure_state = state;
        clean_pass.safety_score = policy.min_safety_score_for_pass;
        clean_pass.revision_pressure_score = policy.max_revision_pressure_for_pass + 0.01;
        pass_pressure_state.candidate_artifact_proposal_audits = {clean_pass};
        require(!validate_candidate_artifact_proposal_audits(pass_pressure_state).empty(),
                "v28.10 validation should reject Pass above revision-pressure threshold");

        ArchiveEngineState no_revision_state = state;
        CandidateArtifactProposalAudit no_revision_audit = report.audits.front();
        no_revision_audit.decision = CandidateArtifactProposalAuditDecision::NeedsRevision;
        no_revision_audit.required_revisions.clear();
        no_revision_state.candidate_artifact_proposal_audits = {no_revision_audit};
        require(!validate_candidate_artifact_proposal_audits(no_revision_state).empty(),
                "v28.10 validation should reject non-pass audits without required revisions");

        ArchiveEngineState no_reason_state = state;
        CandidateArtifactProposalAudit no_reason_audit = report.audits.front();
        no_reason_audit.decision = CandidateArtifactProposalAuditDecision::Blocked;
        no_reason_audit.required_revisions.push_back("synthetic required revision");
        CandidateArtifactProposalAuditFinding no_reason_finding;
        no_reason_finding.id = no_reason_audit.id + ".synthetic_error";
        no_reason_finding.gate = CandidateArtifactProposalAuditGate::KnowledgeHorizon;
        no_reason_finding.severity = CandidateArtifactProposalAuditSeverity::Error;
        no_reason_finding.reason_code = CandidateArtifactProposalAuditReasonCode::None;
        no_reason_finding.message = "synthetic missing reason code";
        no_reason_finding.related_id = no_reason_audit.proposal_id;
        no_reason_audit.findings.push_back(no_reason_finding);
        no_reason_state.candidate_artifact_proposal_audits = {no_reason_audit};
        require(!validate_candidate_artifact_proposal_audits(no_reason_state).empty(),
                "v28.10 validation should reject error findings without reason codes");
    }

    if (!state.candidate_artifact_proposals.empty()) {
        CandidateArtifactProposal proposal = state.candidate_artifact_proposals.front();

        CandidateArtifactProposal missing_source = proposal;
        missing_source.source_evidence_potential_id = "evidence_potential.missing_for_v28_10";
        const CandidateArtifactProposalAudit missing_source_audit = audit_candidate_artifact_proposal(state, missing_source, AccessLevel::Curator);
        require(missing_source_audit.decision == CandidateArtifactProposalAuditDecision::Blocked &&
                std::any_of(missing_source_audit.findings.begin(), missing_source_audit.findings.end(), [](const CandidateArtifactProposalAuditFinding& finding) {
                    return finding.reason_code == CandidateArtifactProposalAuditReasonCode::MissingEvidencePotential;
                }),
                "v28.10 missing source chain should block audit with reason code");

        CandidateArtifactProposal missing_claim = proposal;
        missing_claim.proposed_claim_skeletons.clear();
        const CandidateArtifactProposalAudit missing_claim_audit = audit_candidate_artifact_proposal(state, missing_claim, AccessLevel::Curator);
        require(missing_claim_audit.decision != CandidateArtifactProposalAuditDecision::Pass &&
                std::any_of(missing_claim_audit.findings.begin(), missing_claim_audit.findings.end(), [](const CandidateArtifactProposalAuditFinding& finding) {
                    return finding.reason_code == CandidateArtifactProposalAuditReasonCode::MissingClaimSkeleton;
                }),
                "v28.10 missing claim skeleton should trigger revision or block reason");

        CandidateArtifactProposal missing_distortion = proposal;
        missing_distortion.proposed_distortion_modes.clear();
        const CandidateArtifactProposalAudit missing_distortion_audit = audit_candidate_artifact_proposal(state, missing_distortion, AccessLevel::Curator);
        require(missing_distortion_audit.decision != CandidateArtifactProposalAuditDecision::Pass &&
                std::any_of(missing_distortion_audit.findings.begin(), missing_distortion_audit.findings.end(), [](const CandidateArtifactProposalAuditFinding& finding) {
                    return finding.reason_code == CandidateArtifactProposalAuditReasonCode::MissingDistortionMode;
                }),
                "v28.10 missing distortion mode should trigger revision reason");

        CandidateArtifactProposal missing_damage = proposal;
        missing_damage.proposed_damage_modes.clear();
        const CandidateArtifactProposalAudit missing_damage_audit = audit_candidate_artifact_proposal(state, missing_damage, AccessLevel::Curator);
        require(missing_damage_audit.decision != CandidateArtifactProposalAuditDecision::Pass &&
                std::any_of(missing_damage_audit.findings.begin(), missing_damage_audit.findings.end(), [](const CandidateArtifactProposalAuditFinding& finding) {
                    return finding.reason_code == CandidateArtifactProposalAuditReasonCode::MissingDamageMode;
                }),
                "v28.10 missing damage mode should trigger revision reason");

        CandidateArtifactProposal generation_enabled = proposal;
        generation_enabled.current_generation_enabled = true;
        require(audit_candidate_artifact_proposal(state, generation_enabled, AccessLevel::Curator).decision == CandidateArtifactProposalAuditDecision::Invalid,
                "v28.10 generation enabled should be invalid/blocking");

        CandidateArtifactProposal materialization_enabled = proposal;
        materialization_enabled.current_materialization_enabled = true;
        require(audit_candidate_artifact_proposal(state, materialization_enabled, AccessLevel::Curator).decision == CandidateArtifactProposalAuditDecision::Invalid,
                "v28.10 materialization enabled should be invalid/blocking");

        CandidateArtifactProposal mutation_enabled = proposal;
        mutation_enabled.archive_mutation_enabled = true;
        require(audit_candidate_artifact_proposal(state, mutation_enabled, AccessLevel::Curator).decision == CandidateArtifactProposalAuditDecision::Invalid,
                "v28.10 archive mutation enabled should be invalid/blocking");
    }

    if (!report.audits.empty()) {
        const std::string public_detail = format_candidate_artifact_proposal_audit_detail(state, AccessLevel::Public, report.audits.front().id);
        require(contains_substr(public_detail, "found: false") &&
                !contains_substr(public_detail, "reason_code=") &&
                !contains_substr(public_detail, "Required revisions:"),
                "v28.10 public audit detail should still hide internals");

        const std::string curator_detail = format_candidate_artifact_proposal_audit_detail(state, AccessLevel::Curator, report.audits.front().id);
        require(contains_substr(curator_detail, "Policy thresholds:") &&
                contains_substr(curator_detail, "Policy comparison:") &&
                contains_substr(curator_detail, "reason_code=") &&
                contains_substr(curator_detail, "Required revisions:"),
                "v28.10 curator audit detail should expose policy thresholds, reason codes, and revisions");
    }

    const ArchiveSnapshot snapshot = build_archive_snapshot(
        state,
        "fixture.synthetic_fixed",
        state.seed,
        kOpenEndedYear,
        kOpenEndedYear
    );
    const ArchiveSnapshot repeated_snapshot = build_archive_snapshot(
        state,
        "fixture.synthetic_fixed",
        state.seed,
        kOpenEndedYear,
        kOpenEndedYear
    );
    require(snapshot.summary_digest == repeated_snapshot.summary_digest,
            "v28.10 summary digest should remain deterministic with policy reason-code material");

    const CapturedCliRun control_cli = run_cli_captured({"impossible_archive_mvp_v28_11", "--runtime", "fixed-fixture", "--query", "control-layer-audit-summary"});
    require(control_cli.exit_code == EXIT_SUCCESS && contains_substr(control_cli.stdout_text, "ControlLayerAudit summary"),
            "v28.10 control-layer audit should remain valid and queryable");
}


int run_self_tests() {
    int failures = 0;
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    auto require_throws = [&](auto&& fn, const std::string& message) {
        bool threw = false;
        try {
            fn();
        } catch (const std::exception&) {
            threw = true;
        }
        require(threw, message);
    };

    run_v27_runtime_default_selection_tests(failures);
    run_v27_1_civilization_spec_metadata_tests(failures);
    run_v27_2_catalog_tag_tests(failures);
    run_v28_0_fragment_intake_tests(failures);
    run_v28_1_fixture_snapshot_tests(failures);
    run_v28_2_evidence_potential_tests(failures);
    run_v28_3_knowledge_horizon_tests(failures);
    run_v28_4_contradiction_budget_tests(failures);
    run_v28_11_contradiction_budget_policy_tests(failures);
    run_v28_5_candidate_artifact_plan_tests(failures);
    run_v28_6_candidate_artifact_plan_evaluation_tests(failures);
    run_v28_7_candidate_artifact_proposal_tests(failures);
    run_v28_7_2_public_archive_invariant_tests(failures);
    run_v28_7_2_candidate_artifact_proposal_access_neutral_tests(failures);
    run_v28_7_4_reliability_components_cleanup_tests(failures);
    run_v28_7_4_cli_argument_view_tests(failures);
    run_v28_8_candidate_artifact_proposal_audit_tests(failures);
    run_v28_9_control_layer_audit_tests(failures);
    run_v28_10_proposal_quality_gate_policy_tests(failures);
    run_v26_5_originality_materialization_tests(failures);

    auto make_valid_test_artifact = [](std::string id) {
        Artifact artifact;
        artifact.id = std::move(id);
        artifact.type = ArtifactType::TradeLedger;
        artifact.title = "Valid Test Artifact";
        artifact.creator_id = "person.ivara";
        artifact.attributed_creator_id = "person.ivara";
        artifact.true_creation_year = 620;
        artifact.claimed_creation_year = 620;
        artifact.discovery_year = 810;
        artifact.location_created = "site.reservoir_gate";
        artifact.location_found = "site.reservoir_gate";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.upper_lattice";
        artifact.script_id = "script.green_seal";
        artifact.material = "test board";
        artifact.preservation_quality = 0.5;
        artifact.damage_profile = "test damage";
        artifact.transmission_history = "primary test record";
        artifact.creator_knowledge_scope = "test";
        artifact.creator_bias_profile = "test bias";
        artifact.creator_motive = "test motive";
        artifact.intended_audience = "test audience";
        artifact.public_text = "valid test text";
        artifact.literal_translation = "valid test text";
        artifact.scholarly_translation = "valid test text";
        artifact.hidden_event_links = {"event.salt_moon_schism"};
        artifact.referenced_entity_ids = {"site.reservoir_gate"};
        artifact.distortion_profile = "narrow scope";
        artifact.evidence_modifiers = {EvidenceModifier::NarrowScope};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.5;
            value.preservation_integrity = 0.5;
            value.temporal_proximity = 0.5;
            value.creator_access_to_events = 0.5;
            value.bias_penalty = 0.0;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.5;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        artifact.generation_trace = "test helper artifact";
        return artifact;
    };

    const ArchiveEngineState a = initialize_archive_engine(42);
    const ArchiveEngineState b = initialize_archive_engine(42);
    const ArchiveEngineState c = initialize_archive_engine(43);

    require(validate_full_state(a).empty(), "state with seed 42 should validate");
    require(compute_reliability(ReliabilityComponents{}) == 0.0,
            "reliability scoring should not give an artificial floor to artifacts with no positive evidence");
    require_throws([&] {
        (void)parse_u64("-1");
    }, "negative seed input should be rejected instead of converted to a huge unsigned value");

    {
        HiddenTruthGraph provenance_graph;
        provenance_graph.add_entity(Entity{"person.provenance", EntityType::Person, "Provenance Person", {}, Interval{1, 10}, "event.missing_birth", "", AccessLevel::Canon});
        const std::vector<std::string> provenance_errors = provenance_graph.validate();
        const bool saw_missing_provenance_event = std::any_of(provenance_errors.begin(), provenance_errors.end(), [](const std::string& error) {
            return contains_substr(error, "references missing created_by_event_id event.missing_birth");
        });
        require(saw_missing_provenance_event,
                "entity created_by_event_id and ended_by_event_id values should be validated as event IDs when present");
    }

    {
        const std::vector<AnachronismReport> surface_reports = detect_anachronisms(a);
        const bool saw_claimed_surface_dialect = std::any_of(surface_reports.begin(), surface_reports.end(), [](const AnachronismReport& report) {
            return report.artifact_id == "artifact.aru_decree" &&
                   contains_substr(report.referenced_item, "dialect_id claimed-surface metadata") &&
                   report.checked_year_kind == "claimed_creation_year" &&
                   report.checked_year == 553 &&
                   report.status == AnachronismStatus::ValidBecauseForged;
        });
        const bool saw_claimed_surface_script = std::any_of(surface_reports.begin(), surface_reports.end(), [](const AnachronismReport& report) {
            return report.artifact_id == "artifact.aru_decree" &&
                   contains_substr(report.referenced_item, "script_id claimed-surface metadata") &&
                   report.checked_year_kind == "claimed_creation_year" &&
                   report.checked_year == 553 &&
                   report.status == AnachronismStatus::ValidBecauseForged;
        });
        require(saw_claimed_surface_dialect && saw_claimed_surface_script,
                "claimed-date dialect and script surface metadata should produce mediated anachronism reports for the forged Aru decree");
    }

    require_throws([&] {
        HiddenTruthGraph graph;
        graph.add_entity(Entity{"entity.duplicate", EntityType::Person, "Duplicate", {}, Interval{1, 2}, "", "", AccessLevel::Canon});
        graph.add_entity(Entity{"entity.duplicate", EntityType::Person, "Duplicate Again", {}, Interval{1, 2}, "", "", AccessLevel::Canon});
    }, "duplicate entity IDs should be rejected instead of silently ignored");
    require_throws([&] {
        PublicArchive archive;
        archive.add_claim(Claim{"claim.duplicate", "artifact.none", ClaimType::FactualClaim, "a", "b", "c", "a b c", 0.1, AccessLevel::Public, std::nullopt});
        archive.add_claim(Claim{"claim.duplicate", "artifact.none", ClaimType::FactualClaim, "a", "b", "c", "a b c", 0.1, AccessLevel::Public, std::nullopt});
    }, "duplicate claim IDs should be rejected instead of silently ignored");
    require(serialize_for_replay_test(a) == serialize_for_replay_test(b), "same seed should reproduce identical state");
    require(serialize_world_content_for_seed_test(a) != serialize_world_content_for_seed_test(c),
            "regression seeds 42 and 43 should exercise the MVP's seed-gated contradiction-detector input");

    const std::vector<Contradiction> detected_for_a = detect_contradictions(a);
    auto detected_has_id = [&](const std::string& id) {
        return std::any_of(detected_for_a.begin(), detected_for_a.end(), [&](const Contradiction& contradiction) {
            return contradiction.id == id;
        });
    };
    const std::string aru_title_id = make_contradiction_id("unavailable_entity", {"claim.aru_created_office", "office.drowned_chancellor"});
    const std::string three_keepers_id = make_contradiction_id("mythic_identity_compression", {"claim.three_as_one", "claim.moon_office_locks"});
    const std::string dry_count_id = make_contradiction_id("calendar_date_disagreement", {"claim.levy_before_revolt", "claim.levy_exists_607"});

    require(detected_has_id(aru_title_id),
            "automatic contradiction detection should find the Aru title/office anachronism from typed claim semantics");
    require(detected_has_id(three_keepers_id),
            "automatic contradiction detection should find the three-keepers identity conflict from typed claim semantics");
    require(a.public_archive.find_contradiction(aru_title_id) != nullptr &&
            a.public_archive.find_contradiction(three_keepers_id) != nullptr,
            "detected contradictions should be installed into the public archive");

    const Artifact* aru_artifact = a.public_archive.find_artifact("artifact.aru_decree");
    const Artifact* song_artifact = a.public_archive.find_artifact("artifact.three_keepers_song");
    const Artifact* chronicle_artifact = a.public_archive.find_artifact("artifact.broken_shelf_chronicle");
    const Artifact* ledger_artifact = a.public_archive.find_artifact("artifact.silt_ledger");
    require(aru_artifact != nullptr &&
            std::find(aru_artifact->contradiction_ids.begin(), aru_artifact->contradiction_ids.end(), aru_title_id) != aru_artifact->contradiction_ids.end(),
            "installing a detected contradiction should backfill the involved forged artifact contradiction link");
    require(song_artifact != nullptr && chronicle_artifact != nullptr &&
            std::find(song_artifact->contradiction_ids.begin(), song_artifact->contradiction_ids.end(), three_keepers_id) != song_artifact->contradiction_ids.end() &&
            std::find(chronicle_artifact->contradiction_ids.begin(), chronicle_artifact->contradiction_ids.end(), three_keepers_id) != chronicle_artifact->contradiction_ids.end(),
            "installing a detected contradiction should backfill every involved artifact contradiction link");

    if (a.include_seeded_calendar_dispute) {
        require(detected_has_id(dry_count_id) &&
                a.public_archive.find_contradiction(dry_count_id) != nullptr,
                "seed-gated calendar dispute should be detected and installed when enabled");
        require(chronicle_artifact != nullptr && ledger_artifact != nullptr &&
                std::find(chronicle_artifact->contradiction_ids.begin(), chronicle_artifact->contradiction_ids.end(), dry_count_id) != chronicle_artifact->contradiction_ids.end() &&
                std::find(ledger_artifact->contradiction_ids.begin(), ledger_artifact->contradiction_ids.end(), dry_count_id) != ledger_artifact->contradiction_ids.end(),
                "seed-gated detected contradictions should also backfill reverse artifact links");
    } else {
        require(!detected_has_id(dry_count_id) &&
                a.public_archive.find_contradiction(dry_count_id) == nullptr,
                "seed-gated calendar dispute should be absent when disabled");
    }
    require(!contains_substr(serialize_for_replay_test(a), "contradiction.aru_title_anachronism") &&
            !contains_substr(serialize_for_replay_test(a), "contradiction.three_keepers_identity"),
            "detected contradiction IDs should be generated from rules and involved claims rather than fixture aliases");

    const std::string public_answer = answer_what_happened(a, AccessLevel::Public);
    const std::string curator_answer = answer_what_happened(a, AccessLevel::Curator);
    const std::string canon_answer = answer_what_happened(a, AccessLevel::Canon);
    require(public_answer != canon_answer, "public and canon answers should differ");
    require(!contains_substr(public_answer, "reservoir mismanagement from 601-603"), "public answer should not leak canonical hidden cause");
    require(contains_substr(canon_answer, "The hidden timeline contains") && contains_substr(canon_answer, "event.reservoir_mismanagement"), "canon answer should derive and cite the hidden event sequence from state");
    require(contains_substr(public_answer, "claim.levy_exists_607") && contains_substr(public_answer, "claim.levy_before_revolt"),
            "public answer should cite the public claims it actually uses");
    require(!contains_substr(public_answer, "claim.moon_office_locks") && !contains_substr(public_answer, "assigned cause") && !contains_substr(public_answer, "hidden resolution"),
            "public answer should not cite scholar-only claims or restricted contradiction details");
    require(contains_substr(curator_answer, "assigned cause: Forgery"),
            "curator answer should include the forgery cause when citing the Aru contradiction");
    require(contains_substr(canon_answer, "event.reservoir_mismanagement") && !contains_substr(public_answer, "event.reservoir_mismanagement"),
            "canon answer should cite hidden events while public answer should not");
    require(!contains_substr(canon_answer, "caused the 604 silt levy"),
            "canon answer summary should no longer be a hand-authored causal paragraph");
    {
        const Contradiction* dry_count = a.public_archive.find_contradiction(dry_count_id);
        if (a.include_seeded_calendar_dispute) {
            require(dry_count != nullptr && dry_count->detector_rule == "calendar_date_disagreement",
                    "detected contradictions should expose detector_rule instead of requiring ID parsing");
        }
        const Contradiction* aru_contradiction = a.public_archive.find_contradiction(aru_title_id);
        require(aru_contradiction != nullptr && aru_contradiction->detector_rule == "unavailable_entity",
                "unavailable-entity contradictions should carry a structured detector rule");
    }
    {
        Answer unsafe;
        unsafe.access = AccessLevel::Public;
        AnswerBlock block;
        block.heading = "Unsafe citation test";
        block.summary = "This block intentionally contains a scholar-only citation.";
        block.supporting_evidence.push_back(EvidenceCitation{"artifact.broken_shelf_chronicle", std::optional<std::string>{"claim.moon_office_locks"}, 0.25});
        unsafe.blocks.push_back(block);
        const std::string unsafe_output = format_answer(unsafe, a);
        require(!contains_substr(unsafe_output, "claim.moon_office_locks"),
                "access-aware citation formatting should refuse citations hidden from the current access level");
    }

    {
        ArchiveEngineState without_ledger_claim = initialize_archive_engine(42);
        require(without_ledger_claim.public_archive.remove_claim("claim.levy_exists_607"),
                "test setup should remove the ledger claim");
        const std::string answer_without_ledger = answer_what_happened(without_ledger_claim, AccessLevel::Public);
        require(!contains_substr(answer_without_ledger, "claim.levy_exists_607") &&
                !contains_substr(answer_without_ledger, "silt ledger strongly supports a levy in 607"),
                "public answer should stop citing or asserting the ledger's 607 levy claim when that claim is removed");
        require(contains_substr(answer_without_ledger, "no visible ledger claim currently confirms the 607 levy"),
                "public answer should explicitly weaken the sequence when the ledger claim is absent");
    }

    {
        const Interpreter scholar_anti{"interpreter.test_anti_scholar", "Scholar Anti-Dynastic", EpistemicStyle::AntiDynasticRevisionist, AccessLevel::Scholar};
        const Interpreter curator_anti{"interpreter.test_anti_curator", "Curator Anti-Dynastic", EpistemicStyle::AntiDynasticRevisionist, AccessLevel::Curator};
        const Claim* aru_claim = a.public_archive.find_claim("claim.aru_created_office");
        const Artifact* aru_source = a.public_archive.find_artifact("artifact.aru_decree");
        require(aru_claim != nullptr && aru_source != nullptr, "access-safe anti-dynastic scoring test should find Aru evidence");
        if (aru_claim != nullptr && aru_source != nullptr) {
            const std::vector<const Contradiction*> scholar_caveats = visible_contradictions(a, scholar_anti.access);
            const std::vector<const Contradiction*> curator_caveats = visible_contradictions(a, curator_anti.access);
            const double scholar_multiplier = epistemic_style_multiplier(
                scholar_anti.style,
                *aru_claim,
                *aru_source,
                scholar_caveats,
                scholar_anti.access
            );
            const double curator_multiplier = epistemic_style_multiplier(
                curator_anti.style,
                *aru_claim,
                *aru_source,
                curator_caveats,
                curator_anti.access
            );
            require(scholar_multiplier < curator_multiplier,
                    "scholar-level theory scoring must not use curator-only forged-decree or forgery-cause facts");
            require(!claim_has_visible_forgery_caveat(*aru_claim, scholar_caveats, scholar_anti.access) &&
                    claim_has_visible_forgery_caveat(*aru_claim, curator_caveats, curator_anti.access),
                    "forgery caveat scoring should be visible to curator access but not scholar access");
        }

        const Theory scholar_theory = build_theory_for_interpreter(a, scholar_anti);
        require(!contains_substr(scholar_theory.summary, "restricted forgery cause"),
                "scholar anti-dynastic summary should not claim access to restricted forgery causes");
        const std::string formatted_scholar_theories = format_theories(a, AccessLevel::Scholar);
        require(contains_substr(formatted_scholar_theories, "Interpretive confidence:") &&
                (contains_substr(formatted_scholar_theories, "(moderate)") ||
                 contains_substr(formatted_scholar_theories, "(tentative)") ||
                 contains_substr(formatted_scholar_theories, "(strong)") ||
                 contains_substr(formatted_scholar_theories, "(weak)")),
                "formatted theories should label interpretive confidence, not only print a raw score");
    }

    {
        ArchiveEngineState weak_ledger_state = initialize_archive_engine(42);
        Claim* weak_ledger = weak_ledger_state.public_archive.find_claim_mutable("claim.levy_exists_607");
        require(weak_ledger != nullptr, "test setup should find mutable ledger claim");
        if (weak_ledger != nullptr) {
            weak_ledger->confidence = 0.10;
        }
        const std::string weak_answer = answer_what_happened(weak_ledger_state, AccessLevel::Public);
        require(contains_substr(weak_answer, "barely supports a levy in 607") &&
                !contains_substr(weak_answer, "strongly supports a levy in 607"),
                "public answer wording should change when claim confidence is lowered");
    }

    const std::string seed_without_dry_count_answer = answer_what_happened(c, AccessLevel::Public);
    require(!contains_substr(seed_without_dry_count_answer, dry_count_id),
            "public answer should omit the dry-count caveat when the seed-gated detector input is absent");

    {
        const std::string artifacts_805 = format_artifacts(a, AccessLevel::Public, 805);
        require(contains_substr(artifacts_805, "artifact.victory_inscription") &&
                !contains_substr(artifacts_805, "artifact.silt_ledger"),
                "archive-year visibility should hide artifacts discovered after the requested archive year");

        const std::string discoveries_805 = format_discoveries(a, AccessLevel::Public, 805);
        require(contains_substr(discoveries_805, "artifact.victory_inscription") &&
                !contains_substr(discoveries_805, "artifact.silt_ledger"),
                "discovery log should only show discoveries available by archive year");

        const std::string public_answer_805 = answer_what_happened(a, AccessLevel::Public, 805);
        require(!contains_substr(public_answer_805, "claim.levy_exists_607") &&
                contains_substr(public_answer_805, "does not currently contain enough visible claim evidence"),
                "answers must be built from archive-year-filtered evidence, not redacted after full construction");

        const std::string public_answer_806 = answer_what_happened(a, AccessLevel::Public, 806);
        require(contains_substr(public_answer_806, "claim.levy_exists_607") &&
                !contains_substr(public_answer_806, "claim.levy_before_revolt") &&
                contains_substr(public_answer_806, "not enough to confirm the revolt sequence"),
                "archive year 806 should include the ledger but not the later-discovered chronicle sequence claim");

        const std::string public_answer_807 = answer_what_happened(a, AccessLevel::Public, 807);
        require(contains_substr(public_answer_807, "claim.levy_exists_607") &&
                contains_substr(public_answer_807, "claim.levy_before_revolt"),
                "archive year 807 should allow the public answer to use both ledger and chronicle claims");

        const std::string public_contradictions_808 = format_contradictions(a, AccessLevel::Public, 808);
        const std::string public_contradictions_809 = format_contradictions(a, AccessLevel::Public, 809);
        require(!contains_substr(public_contradictions_808, aru_title_id) &&
                contains_substr(public_contradictions_809, aru_title_id),
                "contradiction visibility should be filtered by discovery year before contradiction formatting");

        const std::string curator_anachronisms_808 = format_anachronisms(a, AccessLevel::Curator, 808);
        const std::string curator_anachronisms_809 = format_anachronisms(a, AccessLevel::Curator, 809);
        require(!contains_substr(curator_anachronisms_808, "artifact.aru_decree") &&
                contains_substr(curator_anachronisms_809, "artifact.aru_decree"),
                "anachronism reports should not reveal future-discovered artifacts at earlier archive years");

        const std::string scholar_theories_811 = format_theories(a, AccessLevel::Scholar, 811);
        const std::string scholar_theories_812 = format_theories(a, AccessLevel::Scholar, 812);
        require(!contains_substr(scholar_theories_811, "claim.three_as_one") &&
                contains_substr(scholar_theories_812, "claim.three_as_one"),
                "interpreter theories should evolve when newly discovered artifacts enter the archive-year view");
    }

    {
        ArchiveEngineState boundary_state = initialize_archive_engine(42);
        Artifact boundary_start;
        boundary_start.id = "artifact.boundary_start";
        boundary_start.type = ArtifactType::Inscription;
        boundary_start.title = "Boundary Start Test";
        boundary_start.creator_id = "person.ivara";
        boundary_start.attributed_creator_id = "person.ivara";
        boundary_start.true_creation_year = 580;
        boundary_start.claimed_creation_year = 580;
        boundary_start.discovery_year = 810;
        boundary_start.location_created = "site.reservoir_gate";
        boundary_start.location_found = "site.reservoir_gate";
        boundary_start.language_id = "language.lattice_dialect";
        boundary_start.dialect_id = "dialect.lower_lattice";
        boundary_start.script_id = "script.pre_green_seal";
        boundary_start.material = "test stone";
        boundary_start.preservation_quality = 0.5;
        boundary_start.damage_profile = "boundary test";
        boundary_start.transmission_history = "boundary test";
        boundary_start.creator_knowledge_scope = "boundary test";
        boundary_start.creator_bias_profile = "boundary test";
        boundary_start.creator_motive = "boundary test";
        boundary_start.intended_audience = "boundary test";
        boundary_start.public_text = "boundary test";
        boundary_start.literal_translation = "boundary test";
        boundary_start.scholarly_translation = "boundary test";
        boundary_start.hidden_event_links = {"event.temple_revolt"};
        boundary_start.referenced_entity_ids = {"person.ivara"};
        boundary_start.distortion_profile = "boundary test";
        boundary_start.evidence_modifiers = {EvidenceModifier::NarrowScope};
        boundary_start.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.5;
            value.preservation_integrity = 0.5;
            value.temporal_proximity = 0.5;
            value.creator_access_to_events = 0.5;
            value.bias_penalty = 0.0;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.5;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        boundary_start.generation_trace = "test boundary start";
        boundary_state.public_archive.add_artifact(finalize_artifact(std::move(boundary_start)));
        register_discovery_for_artifact(boundary_state, "artifact.boundary_start");
        require(validate_full_state(boundary_state).empty(),
                "artifact exactly at an entity existence start boundary should validate");
    }

    {
        ArchiveEngineState before_state = initialize_archive_engine(42);
        Artifact before_existence;
        before_existence.id = "artifact.before_existence";
        before_existence.type = ArtifactType::Inscription;
        before_existence.title = "Before Existence Test";
        before_existence.creator_id = "person.ivara";
        before_existence.attributed_creator_id = "person.ivara";
        before_existence.true_creation_year = 579;
        before_existence.claimed_creation_year = 579;
        before_existence.discovery_year = 810;
        before_existence.location_created = "site.reservoir_gate";
        before_existence.location_found = "site.reservoir_gate";
        before_existence.language_id = "language.lattice_dialect";
        before_existence.dialect_id = "dialect.lower_lattice";
        before_existence.script_id = "script.pre_green_seal";
        before_existence.material = "test stone";
        before_existence.preservation_quality = 0.5;
        before_existence.damage_profile = "before existence test";
        before_existence.transmission_history = "before existence test";
        before_existence.creator_knowledge_scope = "before existence test";
        before_existence.creator_bias_profile = "before existence test";
        before_existence.creator_motive = "before existence test";
        before_existence.intended_audience = "before existence test";
        before_existence.public_text = "before existence test";
        before_existence.literal_translation = "before existence test";
        before_existence.scholarly_translation = "before existence test";
        before_existence.hidden_event_links = {"event.temple_revolt"};
        before_existence.referenced_entity_ids = {"person.ivara"};
        before_existence.distortion_profile = "anachronism mediated by typed forgery modifier";
        before_existence.evidence_modifiers = {EvidenceModifier::Forgery};
        before_existence.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.5;
            value.preservation_integrity = 0.5;
            value.temporal_proximity = 0.5;
            value.creator_access_to_events = 0.5;
            value.bias_penalty = 0.0;
            value.forgery_penalty = 0.6;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.5;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        before_existence.generation_trace = "test before existence";
        before_state.public_archive.add_artifact(finalize_artifact(std::move(before_existence)));
        register_discovery_for_artifact(before_state, "artifact.before_existence");
        const std::vector<AnachronismReport> before_reports = detect_anachronisms(before_state);
        const bool has_forged_boundary_anachronism = std::any_of(before_reports.begin(), before_reports.end(), [](const AnachronismReport& report) {
            return report.artifact_id == "artifact.before_existence" && report.status == AnachronismStatus::ValidBecauseForged;
        });
        require(has_forged_boundary_anachronism,
                "artifact one year before entity existence should be mediated by typed forgery evidence");
        require(validate_full_state(before_state).empty(),
                "forged boundary anachronism should not become a validation failure when explicitly mediated");
    }

    {
        HiddenTruthGraph cycle_graph;
        cycle_graph.add_entity(Entity{"person.test", EntityType::Person, "Test Person", {}, Interval{1, 100}, "", "", AccessLevel::Canon});
        cycle_graph.add_entity(Entity{"site.test", EntityType::Site, "Test Site", {}, Interval{1, 100}, "", "", AccessLevel::Public});
        cycle_graph.add_event(Event{"event.same_year_a", "test", "Same-Year Event A", 10, 10, {"person.test"}, {"event.same_year_b"}, {"site.test"}, "A", TruthLayer::CanonicalTruth, AccessLevel::Canon});
        cycle_graph.add_event(Event{"event.same_year_b", "test", "Same-Year Event B", 10, 10, {"person.test"}, {"event.same_year_a"}, {"site.test"}, "B", TruthLayer::CanonicalTruth, AccessLevel::Canon});
        const std::vector<std::string> cycle_errors = cycle_graph.validate();
        const bool saw_cycle = std::any_of(cycle_errors.begin(), cycle_errors.end(), [](const std::string& error) {
            return contains_substr(error, "causal cycle detected");
        });
        require(saw_cycle,
                "same-year circular event causality should be detected even when simple date ordering does not catch it");
    }

    {
        const std::string debug_contradictions = format_contradictions(a, AccessLevel::Debug);
        const std::string scholar_contradictions_at_complete_archive = format_contradictions(a, AccessLevel::Scholar);
        require(contains_substr(debug_contradictions, "assigned cause") && contains_substr(debug_contradictions, "hidden resolution"),
                "debug access should see contradiction causes and hidden resolutions");
        require(contains_substr(scholar_contradictions_at_complete_archive, "assigned cause: restricted") &&
                !contains_substr(scholar_contradictions_at_complete_archive, "hidden resolution"),
                "scholar access should see restricted-cause status but not hidden resolutions");
    }

    {
        const std::string public_answer_804 = answer_what_happened(a, AccessLevel::Public, 804);
        require(contains_substr(public_answer_804, "does not currently contain enough visible claim evidence") &&
                !contains_substr(public_answer_804, "claim.levy_exists_607") &&
                !contains_substr(public_answer_804, "claim.levy_before_revolt"),
                "archive year 804 should only have the earliest artifact and insufficient public sequence evidence");
    }

    {
        ArchiveEngineState empty_state;
        empty_state.seed = 0;
        const std::string empty_answer = answer_what_happened(empty_state, AccessLevel::Public);
        const std::string empty_artifacts = format_artifacts(empty_state, AccessLevel::Public);
        const std::vector<std::string> empty_errors = validate_full_state(empty_state);
        require(contains_substr(empty_answer, "does not currently contain enough visible claim evidence"),
                "empty archive answers should degrade to insufficient-evidence language");
        require(contains_substr(empty_artifacts, "Artifacts visible to public"),
                "empty artifact lists should still format safely");
        require(!empty_errors.empty() && contains_substr(empty_errors.front(), "not been initialized"),
                "empty engine state should fail full initialized-state validation");
    }

    {
        ArchiveEngineState mutation_state = initialize_archive_engine(42);
        const bool removed = mutation_state.public_archive.remove_claim("claim.levy_exists_607");
        require(removed, "claim removal mutation test should remove the ledger claim");
        const Claim* removed_claim = mutation_state.public_archive.find_claim("claim.levy_exists_607");
        const Artifact* updated_artifact = mutation_state.public_archive.find_artifact("artifact.silt_ledger");
        require(removed_claim == nullptr, "removed claim should no longer resolve from the archive");
        require(updated_artifact != nullptr &&
                std::find(updated_artifact->claim_ids.begin(), updated_artifact->claim_ids.end(), "claim.levy_exists_607") == updated_artifact->claim_ids.end(),
                "source artifact should no longer list a removed claim");
        require(validate_full_state(mutation_state).empty(),
                "state should remain cross-reference-valid after removing a claim and dependent contradictions");
    }

    {
        ArchiveEngineState stress_state = initialize_archive_engine(42);
        for (int i = 0; i < 50; ++i) {
            Artifact artifact = make_valid_test_artifact("artifact.stress_" + std::to_string(i));
            artifact.type = ArtifactType::Inscription;
            artifact.title = "Stress Test Artifact " + std::to_string(i);
            artifact.true_creation_year = 620;
            artifact.claimed_creation_year = 610;
            artifact.discovery_year = 830 + i;
            artifact.script_id = "script.green_seal";
            artifact.dialect_id = "dialect.upper_lattice";
            artifact.referenced_entity_ids = {"person.ivara", "office.drowned_chancellor"};
            artifact.distortion_profile = "stress-test forged early office reference";
            artifact.evidence_modifiers = {EvidenceModifier::Forgery};
            artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.4;
            value.preservation_integrity = 0.6;
            value.temporal_proximity = 0.2;
            value.creator_access_to_events = 0.4;
            value.bias_penalty = 0.1;
            value.forgery_penalty = 0.7;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.3;
            value.contradiction_penalty = 0.4;
            return value;
        }();
            artifact.generation_trace = "stress test forged contradiction " + std::to_string(i);
            add_claim_to_archive(stress_state.public_archive, artifact, Claim{
                "claim.stress_" + std::to_string(i),
                artifact.id,
                ClaimType::LegalFiction,
                "stress claimant " + std::to_string(i),
                "created office",
                "Drowned Chancellor",
                "stress claim creates the Drowned Chancellor too early",
                0.45,
                AccessLevel::Public,
                ClaimSemantics{PredicateType::CreatedOffice, std::nullopt, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<int>{610}},
            });
            stress_state.public_archive.add_artifact(finalize_artifact(std::move(artifact)));
            register_discovery_for_artifact(stress_state, "artifact.stress_" + std::to_string(i));
        }

        const std::vector<std::string> stress_errors = validate_full_state(stress_state);
        const std::vector<Contradiction> stress_detected = detect_contradictions(stress_state);
        const std::size_t stress_contradictions = static_cast<std::size_t>(std::count_if(stress_detected.begin(), stress_detected.end(), [](const Contradiction& contradiction) {
            return contains_substr(contradiction.id, "claim_stress_") && contradiction.detector_rule == "unavailable_entity";
        }));
        require(stress_errors.empty(), "stress-test archive with mediated early-office claims should still validate");
        require(stress_contradictions == 50,
                "stress-test archive should detect one unavailable-entity contradiction per generated stress claim");
    }

    {
        const Interpreter extreme_ritual{"interpreter.extreme_ritual", "Extreme Ritualist", EpistemicStyle::RitualFormalist, AccessLevel::Scholar};
        const Interpreter bureaucratic{"interpreter.bureaucratic_test", "Bureaucratic Tester", EpistemicStyle::BureaucraticMinimalist, AccessLevel::Scholar};
        const Theory extreme_theory = build_theory_for_interpreter(a, extreme_ritual, 812);
        const Theory bureaucratic_theory = build_theory_for_interpreter(a, bureaucratic, 812);
        const bool ritual_cites_mythic = std::any_of(extreme_theory.supporting_evidence.begin(), extreme_theory.supporting_evidence.end(), [](const EvidenceCitation& citation) {
            return citation.claim_id.has_value() && *citation.claim_id == "claim.three_as_one";
        });
        const bool bureaucratic_cites_ledger = std::any_of(bureaucratic_theory.supporting_evidence.begin(), bureaucratic_theory.supporting_evidence.end(), [](const EvidenceCitation& citation) {
            return citation.claim_id.has_value() && *citation.claim_id == "claim.levy_exists_607";
        });
        require(ritual_cites_mythic, "ritual formalist should prioritize mythic-compression evidence once the song is discovered");
        require(bureaucratic_cites_ledger, "bureaucratic minimalist should prioritize ledger evidence");
        require(!extreme_theory.supporting_evidence.empty() && !bureaucratic_theory.supporting_evidence.empty() &&
                extreme_theory.supporting_evidence.front().claim_id != bureaucratic_theory.supporting_evidence.front().claim_id,
                "different epistemic styles should select different primary evidence from the same archive-year view");
    }

    {
        ArchiveEngineState irrelevant_state = initialize_archive_engine(42);
        Artifact unrelated = make_valid_test_artifact("artifact.unrelated_public_note");
        unrelated.title = "Unrelated Public Note";
        unrelated.public_text = "A public note unrelated to the Reservoir Gate sequence.";
        unrelated.literal_translation = unrelated.public_text;
        unrelated.scholarly_translation = unrelated.public_text;
        add_claim_to_archive(irrelevant_state.public_archive, unrelated, Claim{
            "claim.unrelated_public_note",
            unrelated.id,
            ClaimType::FactualClaim,
            "unrelated note",
            "mentions",
            "festival baskets",
            "A public note unrelated to the Reservoir Gate sequence.",
            0.50,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::ExistedInYear, std::nullopt, std::nullopt, std::optional<int>{620}},
        });
        irrelevant_state.public_archive.add_artifact(finalize_artifact(std::move(unrelated)));
        irrelevant_state.public_archive.add_contradiction(Contradiction{
            "contradiction.manual.unrelated_public_note",
            "manual_irrelevant_test",
            {"claim.unrelated_public_note"},
            {"artifact.unrelated_public_note"},
            ContradictionType::DateContradiction,
            ContradictionCause::Damage,
            AccessLevel::Public,
            "irrelevant hidden note",
            AccessLevel::Canon,
            "irrelevant public caveat",
            820,
        });
        const std::string irrelevant_answer = answer_what_happened(irrelevant_state, AccessLevel::Public);
        require(!contains_substr(irrelevant_answer, "contradiction.manual.unrelated_public_note"),
                "public answer should not attach visible contradictions that are unrelated to the cited evidence");
    }

    const std::string hidden_timeline = format_hidden_timeline(a, AccessLevel::Canon);
    const std::size_t reservoir_pos = hidden_timeline.find("Reservoir mismanagement crisis");
    const std::size_t canal_pos = hidden_timeline.find("Canal tax reform");
    const std::size_t temple_pos = hidden_timeline.find("Temple revolt at Reservoir Gate");
    const std::size_t green_pos = hidden_timeline.find("Green-Seal standardization");
    const std::size_t schism_pos = hidden_timeline.find("Salt-Moon Schism");
    require(reservoir_pos < canal_pos && canal_pos < temple_pos && temple_pos < green_pos && green_pos < schism_pos,
            "hidden timeline should be printed in chronological order, not map-key order");

    const std::string public_claims = format_claims(a, AccessLevel::Public);
    const std::string scholar_claims = format_claims(a, AccessLevel::Scholar);
    require(!contains_substr(public_claims, "claim.moon_office_locks"), "public claims should not include scholar-only claim IDs");
    require(contains_substr(scholar_claims, "claim.moon_office_locks"), "scholar claims should include scholar-visible claim IDs");

    require(!contains_substr(public_claims, "semantic_predicate="),
            "public claim output should not expose typed semantic internals");
    require(contains_substr(scholar_claims, "semantic_predicate=received"),
            "scholar claim output should include typed claim semantics for visible claims");

    const std::string public_artifacts = format_artifacts(a, AccessLevel::Public);
    const std::string curator_artifacts = format_artifacts(a, AccessLevel::Curator);
    require(!contains_substr(public_artifacts, "forged decree") && contains_substr(public_artifacts, "royal decree, disputed"),
            "public artifact display should not reveal the internal forged-decree classification");
    require(contains_substr(curator_artifacts, "forged decree"),
            "curator artifact display should reveal the internal forged-decree classification");

    const std::string public_contradictions = format_contradictions(a, AccessLevel::Public);
    const std::string scholar_contradictions = format_contradictions(a, AccessLevel::Scholar);
    const std::string curator_contradictions = format_contradictions(a, AccessLevel::Curator);
    const std::string canon_contradictions = format_contradictions(a, AccessLevel::Canon);
    require(!contains_substr(public_contradictions, "assigned cause") && !contains_substr(public_contradictions, "hidden resolution"),
            "public contradiction view should not expose causes or hidden resolutions");
    require(contains_substr(scholar_contradictions, "assigned cause: restricted"),
            "scholar contradiction view should show restricted status for curator-only causes");
    require(contains_substr(curator_contradictions, "assigned cause: Forgery") && !contains_substr(curator_contradictions, "hidden resolution"),
            "curator contradiction view should expose curator causes but not canon resolutions");
    require(contains_substr(canon_contradictions, "hidden resolution"), "canon contradiction view should expose hidden resolutions");

    const std::vector<AnachronismReport> reports = detect_anachronisms(a);
    const bool saw_forged_anachronism = std::any_of(reports.begin(), reports.end(), [](const AnachronismReport& report) {
        return report.artifact_id == "artifact.aru_decree" && report.status == AnachronismStatus::ValidBecauseForged;
    });
    require(saw_forged_anachronism, "forged decree should have a justified anachronism");

    const bool saw_attribution_lifespan_report = std::any_of(reports.begin(), reports.end(), [](const AnachronismReport& report) {
        return report.artifact_id == "artifact.aru_decree" &&
               report.checked_year_kind == "true_creation_year" &&
               report.checked_year == 660 &&
               report.availability_start_year == 520 &&
               report.availability_end_year == 559 &&
               report.status == AnachronismStatus::ValidBecauseForged;
    });
    require(saw_attribution_lifespan_report,
            "forged Aru attribution should report checked true-creation year against Aru's lifespan range");

    const std::string public_anachronisms = format_anachronisms(a, AccessLevel::Public);
    const std::string public_anachronisms_805 = format_anachronisms(a, AccessLevel::Public, 805);
    const std::string scholar_anachronisms = format_anachronisms(a, AccessLevel::Scholar);
    const std::string curator_anachronisms = format_anachronisms(a, AccessLevel::Curator);
    require(contains_substr(public_anachronisms, "authenticity disputed") &&
            !contains_substr(public_anachronisms, "Forgery") &&
            !contains_substr(public_anachronisms, "valid_range"),
            "public anachronism view should avoid internal availability and forgery labels");
    require(contains_substr(public_anachronisms_805, "- none visible"),
            "filtered-empty anachronism sections should explicitly print none visible");
    require(contains_substr(scholar_anachronisms, "valid_range=617-900") &&
            !contains_substr(scholar_anachronisms, "explanation=Forgery"),
            "scholar anachronism view should expose availability ranges but not curator explanations");
    require(contains_substr(curator_anachronisms, "explanation=Forgery"),
            "curator anachronism view should expose the validator explanation");


    {
        ArchiveEngineState missing_link_state = initialize_archive_engine(42);
        Artifact bad;
        bad.id = "artifact.bad_missing_link";
        bad.type = ArtifactType::Inscription;
        bad.title = "Bad Missing Link Artifact";
        bad.creator_id = "person.ivara";
        bad.attributed_creator_id = "person.ivara";
        bad.true_creation_year = 620;
        bad.claimed_creation_year = 620;
        bad.discovery_year = 810;
        bad.location_created = "site.reservoir_gate";
        bad.location_found = "site.reservoir_gate";
        bad.language_id = "language.lattice_dialect";
        bad.dialect_id = "dialect.lower_lattice";
        bad.script_id = "script.pre_green_seal";
        bad.material = "test stone";
        bad.preservation_quality = 0.5;
        bad.damage_profile = "test damage";
        bad.transmission_history = "primary test record";
        bad.creator_knowledge_scope = "test";
        bad.creator_bias_profile = "test bias";
        bad.creator_motive = "test motive";
        bad.intended_audience = "test audience";
        bad.public_text = "bad link";
        bad.literal_translation = "bad link";
        bad.scholarly_translation = "bad link";
        bad.hidden_event_links = {"event.missing"};
        bad.referenced_entity_ids = {"person.missing"};
        bad.distortion_profile = "narrow scope";
        bad.evidence_modifiers = {EvidenceModifier::NarrowScope};
        bad.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.5;
            value.preservation_integrity = 0.5;
            value.temporal_proximity = 0.5;
            value.creator_access_to_events = 0.5;
            value.bias_penalty = 0.0;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.5;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        bad.generation_trace = "test missing links";
        missing_link_state.public_archive.add_artifact(finalize_artifact(std::move(bad)));
        const std::vector<std::string> errors = validate_full_state(missing_link_state);
        const bool saw_missing_hidden_event = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "references missing hidden event event.missing");
        });
        const bool saw_missing_entity = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "references missing entity person.missing");
        });
        require(saw_missing_hidden_event && saw_missing_entity,
                "validation should reject missing hidden event and entity cross-references");
    }

    {
        ArchiveEngineState metadata_state = initialize_archive_engine(42);
        Artifact bad;
        bad.id = "artifact.bad_metadata_id";
        bad.type = ArtifactType::Inscription;
        bad.title = "Bad Metadata ID Artifact";
        bad.creator_id = "person.ivara";
        bad.attributed_creator_id = "person.ivara";
        bad.true_creation_year = 620;
        bad.claimed_creation_year = 620;
        bad.discovery_year = 810;
        bad.location_created = "site.reservoir_gate";
        bad.location_found = "site.reservoir_gate";
        bad.language_id = "language.lattice_dialect";
        bad.dialect_id = "dialect.missing";
        bad.script_id = "script.pre_green_seal";
        bad.material = "test stone";
        bad.preservation_quality = 0.5;
        bad.damage_profile = "test damage";
        bad.transmission_history = "primary test record";
        bad.creator_knowledge_scope = "test";
        bad.creator_bias_profile = "test bias";
        bad.creator_motive = "test motive";
        bad.intended_audience = "test audience";
        bad.public_text = "bad metadata";
        bad.literal_translation = "bad metadata";
        bad.scholarly_translation = "bad metadata";
        bad.hidden_event_links = {"event.temple_revolt"};
        bad.referenced_entity_ids = {"person.ivara"};
        bad.distortion_profile = "narrow scope";
        bad.evidence_modifiers = {EvidenceModifier::NarrowScope};
        bad.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.5;
            value.preservation_integrity = 0.5;
            value.temporal_proximity = 0.5;
            value.creator_access_to_events = 0.5;
            value.bias_penalty = 0.0;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.5;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        bad.generation_trace = "test bad metadata id";
        metadata_state.public_archive.add_artifact(finalize_artifact(std::move(bad)));
        const std::vector<std::string> errors = validate_full_state(metadata_state);
        const bool saw_missing_dialect = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "field dialect_id references missing entity dialect.missing");
        });
        require(saw_missing_dialect, "validation should reject missing structured metadata IDs such as dialect_id");
    }

    {
        ArchiveEngineState creator_state = initialize_archive_engine(42);
        Artifact bad;
        bad.id = "artifact.bad_person_creator";
        bad.type = ArtifactType::TradeLedger;
        bad.title = "Bad Person Creator Artifact";
        bad.creator_id = "person.missing";
        bad.attributed_creator_id = "clerk.freeform_label";
        bad.true_creation_year = 620;
        bad.claimed_creation_year = 620;
        bad.discovery_year = 810;
        bad.location_created = "site.reservoir_gate";
        bad.location_found = "site.reservoir_gate";
        bad.language_id = "language.lattice_dialect";
        bad.dialect_id = "dialect.lower_lattice";
        bad.script_id = "script.pre_green_seal";
        bad.material = "test board";
        bad.preservation_quality = 0.5;
        bad.damage_profile = "test damage";
        bad.transmission_history = "primary test record";
        bad.creator_knowledge_scope = "test";
        bad.creator_bias_profile = "test bias";
        bad.creator_motive = "test motive";
        bad.intended_audience = "test audience";
        bad.public_text = "bad creator";
        bad.literal_translation = "bad creator";
        bad.scholarly_translation = "bad creator";
        bad.hidden_event_links = {"event.temple_revolt"};
        bad.referenced_entity_ids = {"site.reservoir_gate"};
        bad.distortion_profile = "narrow scope";
        bad.evidence_modifiers = {EvidenceModifier::NarrowScope};
        bad.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.5;
            value.preservation_integrity = 0.5;
            value.temporal_proximity = 0.5;
            value.creator_access_to_events = 0.5;
            value.bias_penalty = 0.0;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.5;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        bad.generation_trace = "test bad person creator id";
        creator_state.public_archive.add_artifact(finalize_artifact(std::move(bad)));
        const std::vector<std::string> errors = validate_full_state(creator_state);
        const bool saw_missing_person_creator = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "field creator_id references missing person entity person.missing");
        });
        require(saw_missing_person_creator, "validation should reject person.* creator IDs that do not resolve to person entities");
    }

    {
        ArchiveEngineState temporal_state = initialize_archive_engine(42);
        Artifact bad = make_valid_test_artifact("artifact.bad_temporal_metadata");
        bad.creator_id = "scribe.freeform_test";
        bad.attributed_creator_id = "scribe.freeform_test";
        bad.true_creation_year = 700;
        bad.claimed_creation_year = 700;
        bad.dialect_id = "dialect.lower_lattice";
        bad.generation_trace = "test temporal metadata availability";
        temporal_state.public_archive.add_artifact(finalize_artifact(std::move(bad)));
        const std::vector<std::string> errors = validate_full_state(temporal_state);
        const bool saw_temporal_metadata_error = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "dialect_id: Lower Lattice dialect unavailable at true_creation_year=700");
        });
        require(saw_temporal_metadata_error,
                "validation should reject structured metadata IDs that exist but are unavailable at the artifact's relevant year");
    }

    {
        ArchiveEngineState semantic_state = initialize_archive_engine(42);
        Artifact bad = make_valid_test_artifact("artifact.bad_semantic_missing_entity");
        add_claim_to_archive(semantic_state.public_archive, bad, Claim{
            "claim.bad_semantic_missing_entity",
            bad.id,
            ClaimType::FactualClaim,
            "missing person",
            "restored",
            "gate",
            "missing person restored gate",
            0.50,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::Restored, std::optional<std::string>{"person.missing"}, std::optional<std::string>{"site.reservoir_gate"}, std::optional<int>{620}},
        });
        semantic_state.public_archive.add_artifact(finalize_artifact(std::move(bad)));
        const std::vector<std::string> errors = validate_full_state(semantic_state);
        const bool saw_missing_semantic_entity = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "claim claim.bad_semantic_missing_entity semantics field subject_entity_id references missing entity person.missing");
        });
        require(saw_missing_semantic_entity,
                "validation should reject typed claim semantics that reference missing entities");
    }

    {
        ArchiveEngineState semantic_state = initialize_archive_engine(42);
        Artifact bad = make_valid_test_artifact("artifact.bad_semantic_type");
        add_claim_to_archive(semantic_state.public_archive, bad, Claim{
            "claim.bad_semantic_type",
            bad.id,
            ClaimType::FactualClaim,
            "Ivara",
            "created office",
            "Reservoir Gate",
            "Ivara created Reservoir Gate as an office",
            0.50,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::CreatedOffice, std::optional<std::string>{"person.ivara"}, std::optional<std::string>{"site.reservoir_gate"}, std::optional<int>{620}},
        });
        semantic_state.public_archive.add_artifact(finalize_artifact(std::move(bad)));
        const std::vector<std::string> errors = validate_full_state(semantic_state);
        const bool saw_semantic_type_error = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "CreatedOffice expects object_entity_id to be an office");
        });
        require(saw_semantic_type_error,
                "validation should enforce typed claim semantic object constraints");
    }

    {
        ArchiveEngineState semantic_state = initialize_archive_engine(42);
        Artifact bad = make_valid_test_artifact("artifact.bad_semantic_temporal_entity");
        add_claim_to_archive(semantic_state.public_archive, bad, Claim{
            "claim.bad_semantic_temporal_entity",
            bad.id,
            ClaimType::FactualClaim,
            "Ivara",
            "created office",
            "Drowned Chancellor",
            "Ivara created the Drowned Chancellor before the schism",
            0.50,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::CreatedOffice, std::optional<std::string>{"person.ivara"}, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<int>{553}},
        });
        semantic_state.public_archive.add_artifact(finalize_artifact(std::move(bad)));
        const std::vector<std::string> errors = validate_full_state(semantic_state);
        const bool saw_temporal_semantic_error = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "semantics field object_entity_id references entity office.drowned_chancellor outside valid range at claimed_year=553");
        });
        require(saw_temporal_semantic_error,
                "validation should reject unmediated typed claim semantics that use an entity before its valid range");
    }

    {
        ArchiveEngineState restricted_state = initialize_archive_engine(42);
        Artifact private_artifact;
        private_artifact.id = "artifact.restricted_catalog_note";
        private_artifact.type = ArtifactType::Inscription;
        private_artifact.title = "Restricted Catalog Note";
        private_artifact.creator_id = "person.ivara";
        private_artifact.attributed_creator_id = "person.ivara";
        private_artifact.true_creation_year = 620;
        private_artifact.claimed_creation_year = 620;
        private_artifact.discovery_year = 810;
        private_artifact.location_created = "site.reservoir_gate";
        private_artifact.location_found = "site.reservoir_gate";
        private_artifact.language_id = "language.lattice_dialect";
        private_artifact.dialect_id = "dialect.lower_lattice";
        private_artifact.script_id = "script.pre_green_seal";
        private_artifact.material = "test note";
        private_artifact.preservation_quality = 0.5;
        private_artifact.damage_profile = "test damage";
        private_artifact.transmission_history = "restricted catalog record";
        private_artifact.creator_knowledge_scope = "test";
        private_artifact.creator_bias_profile = "test bias";
        private_artifact.creator_motive = "test motive";
        private_artifact.intended_audience = "curators";
        private_artifact.public_text = "restricted contradiction context";
        private_artifact.literal_translation = "restricted";
        private_artifact.scholarly_translation = "restricted";
        private_artifact.hidden_event_links = {"event.temple_revolt"};
        private_artifact.referenced_entity_ids = {"site.reservoir_gate"};
        private_artifact.distortion_profile = "restricted audit note";
        private_artifact.evidence_modifiers = {EvidenceModifier::NarrowScope};
        private_artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.5;
            value.preservation_integrity = 0.5;
            value.temporal_proximity = 0.5;
            value.creator_access_to_events = 0.5;
            value.bias_penalty = 0.0;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.5;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        private_artifact.min_access = AccessLevel::Curator;
        private_artifact.generation_trace = "test restricted contradiction visibility";
        add_claim_to_archive(restricted_state.public_archive, private_artifact, Claim{
            "claim.restricted_note",
            private_artifact.id,
            ClaimType::FactualClaim,
            "restricted note",
            "disputes",
            "public catalog",
            "restricted contradiction context",
            0.50,
            AccessLevel::Curator,
            std::nullopt,
        });
        restricted_state.public_archive.add_artifact(finalize_artifact(std::move(private_artifact)));
        restricted_state.public_archive.add_contradiction(Contradiction{
            "contradiction.restricted_only",
            "manual_restricted_test",
            {"claim.restricted_note"},
            {"artifact.restricted_catalog_note"},
            ContradictionType::DateContradiction,
            ContradictionCause::Damage,
            AccessLevel::Curator,
            "restricted hidden resolution",
            AccessLevel::Canon,
            "restricted public status",
            810,
        });
        require(!contains_substr(format_contradictions(restricted_state, AccessLevel::Public), "contradiction.restricted_only"),
                "public contradiction view should skip contradictions whose involved artifacts are not visible");
        require(contains_substr(format_contradictions(restricted_state, AccessLevel::Curator), "contradiction.restricted_only"),
                "curator contradiction view should include contradictions with curator-visible involved artifacts");
    }

    {
        ArchiveEngineState string_only_state = initialize_archive_engine(42);
        Artifact bad;
        bad.id = "artifact.string_only_forgery";
        bad.type = ArtifactType::Inscription;
        bad.title = "String-Only Forgery Test";
        bad.creator_id = "person.aru";
        bad.attributed_creator_id = "person.aru";
        bad.true_creation_year = 553;
        bad.claimed_creation_year = 553;
        bad.discovery_year = 809;
        bad.location_created = "site.reservoir_gate";
        bad.location_found = "site.reservoir_gate";
        bad.language_id = "language.lattice_dialect";
        bad.dialect_id = "dialect.lower_lattice";
        bad.script_id = "script.pre_green_seal";
        bad.material = "test clay";
        bad.preservation_quality = 0.5;
        bad.damage_profile = "test damage";
        bad.transmission_history = "primary test record";
        bad.creator_knowledge_scope = "test";
        bad.creator_bias_profile = "test bias";
        bad.creator_motive = "test motive";
        bad.intended_audience = "test audience";
        bad.public_text = "A forged-looking text names the Drowned Chancellor.";
        bad.literal_translation = "test";
        bad.scholarly_translation = "test";
        bad.hidden_event_links = {"event.salt_moon_schism"};
        bad.referenced_entity_ids = {"office.drowned_chancellor"};
        bad.distortion_profile = "forgery word appears here, but structured cause is not forgery";
        bad.evidence_modifiers = {EvidenceModifier::Propaganda};
        bad.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.5;
            value.preservation_integrity = 0.5;
            value.temporal_proximity = 0.5;
            value.creator_access_to_events = 0.5;
            value.bias_penalty = 0.1;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.5;
            value.external_corroboration = 0.5;
            value.contradiction_penalty = 0.0;
            return value;
        }();
        bad.generation_trace = "test stringly typed mediation";
        string_only_state.public_archive.add_artifact(finalize_artifact(std::move(bad)));
        const std::vector<AnachronismReport> bad_reports = detect_anachronisms(string_only_state);
        const bool string_did_not_mediate = std::any_of(bad_reports.begin(), bad_reports.end(), [](const AnachronismReport& report) {
            return report.artifact_id == "artifact.string_only_forgery" && report.status == AnachronismStatus::InvalidGenerationBug;
        });
        require(string_did_not_mediate,
                "prose distortion_profile containing the word forgery should not mediate anachronisms without EvidenceModifier::Forgery");
    }

    {
        const std::vector<Theory> theories = build_theories(a);
        require(theories.size() == 3, "v6 should build exactly three MVP interpreter theories");

        auto find_theory = [&](EpistemicStyle style) -> const Theory* {
            const auto it = std::find_if(theories.begin(), theories.end(), [&](const Theory& theory) {
                return theory.style == style;
            });
            return it == theories.end() ? nullptr : &*it;
        };

        const Theory* bureaucratic = find_theory(EpistemicStyle::BureaucraticMinimalist);
        const Theory* ritual = find_theory(EpistemicStyle::RitualFormalist);
        const Theory* anti_dynastic = find_theory(EpistemicStyle::AntiDynasticRevisionist);
        require(bureaucratic != nullptr && ritual != nullptr && anti_dynastic != nullptr,
                "v6 should include bureaucratic, ritual, and anti-dynastic theories");

        auto theory_cites = [](const Theory* theory, const std::string& claim_id) {
            return theory != nullptr && std::any_of(theory->supporting_evidence.begin(), theory->supporting_evidence.end(), [&](const EvidenceCitation& citation) {
                return citation.claim_id.has_value() && *citation.claim_id == claim_id;
            });
        };

        require(theory_cites(bureaucratic, "claim.levy_exists_607"),
                "bureaucratic minimalist theory should cite the silt ledger claim");
        require(theory_cites(ritual, "claim.three_as_one") || theory_cites(ritual, "claim.aru_created_office"),
                "ritual formalist theory should cite ritual, legal, or mythic evidence");
        require(theory_cites(anti_dynastic, "claim.aru_created_office") || theory_cites(anti_dynastic, "claim.victory_restoration"),
                "anti-dynastic revisionist theory should cite Aru or Ivara political evidence");

        require(bureaucratic->summary != ritual->summary && ritual->summary != anti_dynastic->summary,
                "different epistemic styles should produce different theory summaries");
        require(!bureaucratic->supporting_evidence.empty() && !ritual->supporting_evidence.empty() && !anti_dynastic->supporting_evidence.empty() &&
                bureaucratic->supporting_evidence.front().claim_id != ritual->supporting_evidence.front().claim_id &&
                ritual->supporting_evidence.front().claim_id != anti_dynastic->supporting_evidence.front().claim_id,
                "same archive should produce different top-cited evidence for different epistemic styles");

        const std::string formatted_theories = format_theories(a, AccessLevel::Scholar);
        require(contains_substr(formatted_theories, "bureaucratic_minimalist") &&
                contains_substr(formatted_theories, "ritual_formalist") &&
                contains_substr(formatted_theories, "anti_dynastic_revisionist"),
                "formatted theories should include all three v6 epistemic styles");
        const std::string public_theory_output = format_theories(a, AccessLevel::Public);
        require(!contains_substr(public_theory_output, "claim.moon_office_locks") &&
                contains_substr(public_theory_output, "no interpreter theories visible"),
                "public theory view should not expose scholar-level interpreter theories or citations");
    }

    {
        Interpreter public_ritual{"interpreter.public_ritual", "Public Ritual Reader", EpistemicStyle::RitualFormalist, AccessLevel::Public};
        const Theory public_theory = build_theory_for_interpreter(a, public_ritual);
        const bool cites_scholar_only = std::any_of(public_theory.supporting_evidence.begin(), public_theory.supporting_evidence.end(), [](const EvidenceCitation& citation) {
            return citation.claim_id.has_value() && *citation.claim_id == "claim.moon_office_locks";
        });
        require(!cites_scholar_only,
                "public-level interpreter theory should not cite scholar-only Moon-office evidence");
    }

    {
        ArchiveEngineState weak_ledger_state = initialize_archive_engine(42);
        Claim* weak_ledger = weak_ledger_state.public_archive.find_claim_mutable("claim.levy_exists_607");
        require(weak_ledger != nullptr, "weak ledger theory test should find ledger claim");
        if (weak_ledger != nullptr) {
            weak_ledger->confidence = 0.05;
        }
        const Interpreter bureaucrat{"interpreter.test_bureaucrat", "Test Bureaucrat", EpistemicStyle::BureaucraticMinimalist, AccessLevel::Scholar};
        const Theory strong_theory = build_theory_for_interpreter(a, bureaucrat);
        const Theory weak_theory = build_theory_for_interpreter(weak_ledger_state, bureaucrat);
        require(weak_theory.confidence < strong_theory.confidence,
                "lowering ledger confidence should weaken the bureaucratic theory");
    }

    {
        const Interpreter anti{"interpreter.test_anti", "Test Anti-Dynastic", EpistemicStyle::AntiDynasticRevisionist, AccessLevel::Curator};
        const Claim* aru_claim = a.public_archive.find_claim("claim.aru_created_office");
        const Artifact* aru_source = a.public_archive.find_artifact("artifact.aru_decree");
        require(aru_claim != nullptr && aru_source != nullptr, "anti-dynastic contradiction scoring test should find Aru evidence");
        if (aru_claim != nullptr && aru_source != nullptr) {
            const std::vector<const Contradiction*> with_caveats = visible_contradictions(a, anti.access);
            const std::vector<const Contradiction*> no_caveats;
            const double with_score = citation_weight(a, *aru_claim) * epistemic_style_multiplier(anti.style, *aru_claim, *aru_source, with_caveats, anti.access);
            const double without_score = citation_weight(a, *aru_claim) * epistemic_style_multiplier(anti.style, *aru_claim, *aru_source, no_caveats, anti.access);
            require(with_score > without_score,
                    "anti-dynastic theory should boost Aru evidence when a forgery contradiction is visible");
        }
    }


    {
        const std::string mysteries_806 = format_mysteries(a, AccessLevel::Public, 806);
        const std::string mysteries_807 = format_mysteries(a, AccessLevel::Public, 807);
        const std::string mysteries_812 = format_mysteries(a, AccessLevel::Public, 812);
        require(contains_substr(mysteries_806, "no mysteries visible"),
                "mystery should be invisible before any clue artifact is discovered");
        require(contains_substr(mysteries_807, "mystery.third_lock_authority") &&
                contains_substr(mysteries_807, "open but constrained"),
                "mystery should appear after the first clue artifact is discovered");
        require(contains_substr(mysteries_812, "confidence=0.72") &&
                !contains_substr(mysteries_812, "confidence=1.00"),
                "partially resolvable mystery should not report full public certainty after all public clues are visible");
        require(contains_substr(mysteries_812, "Confidence is capped to preserve the mystery from over-resolution"),
                "public mystery assessment should explicitly report confidence capping when raw evidence exceeds the cap");
        require(contains_substr(mysteries_812, "Misleading evidence is visible"),
                "misleading clue artifacts should deepen ambiguity rather than cleanly resolve the mystery");
    }

    {
        const std::string public_mystery = format_mysteries(a, AccessLevel::Public, 812);
        const std::string scholar_mystery = format_mysteries(a, AccessLevel::Scholar, 812);
        require(contains_substr(public_mystery, "confidence=0.72") &&
                contains_substr(scholar_mystery, "confidence=0.84"),
                "scholar mystery confidence may exceed public confidence but should still be capped");
        require(!contains_substr(public_mystery, "claim.moon_office_locks") &&
                contains_substr(scholar_mystery, "claim.moon_office_locks"),
                "mystery assessment evidence should obey access-level claim visibility");
    }

    {
        const Mystery* third_lock = nullptr;
        for (const Mystery& mystery : a.mysteries) {
            if (mystery.id == "mystery.third_lock_authority") {
                third_lock = &mystery;
                break;
            }
        }
        require(third_lock != nullptr, "test setup should find third-lock mystery");
        if (third_lock != nullptr) {
            const MysteryAssessment public_807 = assess_mystery(a, *third_lock, AccessLevel::Public, 807);
            const bool public_core_has_levy_sequence = std::any_of(public_807.supporting_evidence.begin(), public_807.supporting_evidence.end(), [](const EvidenceCitation& citation) {
                return citation.claim_id.has_value() && *citation.claim_id == "claim.levy_before_revolt";
            });
            const bool public_context_has_levy_sequence = std::any_of(public_807.context_evidence.begin(), public_807.context_evidence.end(), [](const EvidenceCitation& citation) {
                return citation.claim_id.has_value() && *citation.claim_id == "claim.levy_before_revolt";
            });
            require(!public_core_has_levy_sequence && public_context_has_levy_sequence,
                    "public year-807 mystery assessment should treat levy_before_revolt as context, not direct lock-authority evidence");
            require(contains_substr(public_807.summary, "no directly relevant claim is visible"),
                    "public year-807 mystery assessment should say the direct claim is not visible");

            const MysteryAssessment scholar_807 = assess_mystery(a, *third_lock, AccessLevel::Scholar, 807);
            const bool scholar_core_has_moon_office = std::any_of(scholar_807.supporting_evidence.begin(), scholar_807.supporting_evidence.end(), [](const EvidenceCitation& citation) {
                return citation.claim_id.has_value() && *citation.claim_id == "claim.moon_office_locks";
            });
            require(scholar_core_has_moon_office && scholar_807.capped_confidence > public_807.capped_confidence,
                    "scholar year-807 assessment should cite moon-office as core evidence and be stronger than public assessment");
        }
    }

    {
        const Artifact* chronicle = a.public_archive.find_artifact("artifact.broken_shelf_chronicle");
        require(chronicle != nullptr &&
                std::find(chronicle->mystery_links.begin(), chronicle->mystery_links.end(), "mystery.third_lock_authority") != chronicle->mystery_links.end(),
                "mystery evidence installation should backfill artifact.mystery_links");
    }

    {
        ArchiveEngineState bad_link_state = initialize_archive_engine(42);
        add_mystery(bad_link_state, Mystery{
            "mystery.bad_claim_link",
            "Bad Claim Link Mystery",
            "This mystery points at a missing linked claim.",
            RevealMode::PartiallyResolvable,
            0.5,
            0.5,
            {"artifact.silt_ledger"},
            {},
            {MysteryEvidenceLink{"artifact.silt_ledger", std::optional<std::string>{"claim.missing"}, MysteryEvidenceRole::CoreClue, 1.0}},
            AccessLevel::Public,
        });
        const std::vector<std::string> errors = validate_full_state(bad_link_state);
        const bool saw_missing_linked_claim = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "mystery mystery.bad_claim_link evidence link references missing claim claim.missing");
        });
        require(saw_missing_linked_claim, "validation should reject missing mystery evidence-linked claims");
    }

    {
        ArchiveEngineState wrong_artifact_state = initialize_archive_engine(42);
        add_mystery(wrong_artifact_state, Mystery{
            "mystery.wrong_claim_artifact",
            "Wrong Claim Artifact Mystery",
            "This mystery links a claim to the wrong artifact.",
            RevealMode::PartiallyResolvable,
            0.5,
            0.5,
            {"artifact.silt_ledger"},
            {},
            {MysteryEvidenceLink{"artifact.silt_ledger", std::optional<std::string>{"claim.levy_before_revolt"}, MysteryEvidenceRole::CoreClue, 1.0}},
            AccessLevel::Public,
        });
        const std::vector<std::string> errors = validate_full_state(wrong_artifact_state);
        const bool saw_wrong_artifact = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "claim claim.levy_before_revolt does not belong to artifact artifact.silt_ledger");
        });
        require(saw_wrong_artifact, "validation should reject mystery evidence links whose claim does not belong to the linked artifact");
    }

    {
        ArchiveEngineState never_state = initialize_archive_engine(42);
        add_mystery(never_state, Mystery{
            "mystery.never_full_test",
            "Never Full Test Mystery",
            "Can a never-fully-resolvable mystery reach certainty?",
            RevealMode::NeverFullyResolvable,
            1.0,
            1.0,
            {"artifact.broken_shelf_chronicle", "artifact.three_keepers_song", "artifact.aru_decree", "artifact.silt_ledger"},
            {},
            {
                MysteryEvidenceLink{"artifact.broken_shelf_chronicle", std::optional<std::string>{"claim.moon_office_locks"}, MysteryEvidenceRole::CoreClue, 1.0},
                MysteryEvidenceLink{"artifact.three_keepers_song", std::optional<std::string>{"claim.three_as_one"}, MysteryEvidenceRole::CoreClue, 1.0},
                MysteryEvidenceLink{"artifact.aru_decree", std::optional<std::string>{"claim.aru_created_office"}, MysteryEvidenceRole::CoreClue, 1.0},
                MysteryEvidenceLink{"artifact.silt_ledger", std::optional<std::string>{"claim.levy_exists_607"}, MysteryEvidenceRole::ContextClue, 0.4},
            },
            AccessLevel::Public,
        });
        const Mystery* never_mystery = nullptr;
        for (const Mystery& mystery : never_state.mysteries) {
            if (mystery.id == "mystery.never_full_test") {
                never_mystery = &mystery;
                break;
            }
        }
        require(never_mystery != nullptr, "test setup should install never-fully-resolvable mystery");
        if (never_mystery != nullptr) {
            const MysteryAssessment assessment = assess_mystery(never_state, *never_mystery, AccessLevel::Public, 812);
            require(assessment.capped_confidence < 1.0 && assessment.status == "protected unresolved",
                    "never-fully-resolvable mystery should not reach full certainty");
        }
    }

    {
        ArchiveEngineState mystery_state = initialize_archive_engine(42);
        mystery_state.mysteries.push_back(Mystery{
            "mystery.bad_reference",
            "Bad Reference Mystery",
            "This mystery points at a missing artifact.",
            RevealMode::PartiallyResolvable,
            0.5,
            0.5,
            {"artifact.missing"},
            {},
            {MysteryEvidenceLink{"artifact.missing", std::nullopt, MysteryEvidenceRole::CoreClue, 1.0}},
            AccessLevel::Public,
        });
        const std::vector<std::string> errors = validate_full_state(mystery_state);
        const bool saw_missing_clue = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
            return contains_substr(error, "mystery mystery.bad_reference references missing clue artifact artifact.missing");
        });
        require(saw_missing_clue,
                "validation should reject mysteries that reference missing clue artifacts");
    }

    {
        const Artifact* ledger = a.public_archive.find_artifact("artifact.silt_ledger");
        const Artifact* song = a.public_archive.find_artifact("artifact.three_keepers_song");
        const Artifact* chronicle = a.public_archive.find_artifact("artifact.broken_shelf_chronicle");
        const Artifact* decree = a.public_archive.find_artifact("artifact.aru_decree");
        require(ledger != nullptr && song != nullptr && chronicle != nullptr && decree != nullptr,
                "v9 voice tests should find the fixture artifacts");
        if (ledger != nullptr && song != nullptr && chronicle != nullptr && decree != nullptr) {
            const std::vector<std::string> ledger_claims_before = ledger->claim_ids;
            const std::size_t hidden_event_count_before = a.hidden_truth.events().size();
            const std::string ledger_voice = render_artifact_text(*ledger, a, AccessLevel::Public, 812);
            const std::string song_voice = render_artifact_text(*song, a, AccessLevel::Public, 812);
            const std::string chronicle_voice = render_artifact_text(*chronicle, a, AccessLevel::Scholar, 807);
            const std::string public_decree_voice = render_artifact_text(*decree, a, AccessLevel::Public, 812);
            const std::string curator_decree_voice = render_artifact_text(*decree, a, AccessLevel::Curator, 812);

            require(contains_substr(ledger_voice, "Administrative ledger entries") &&
                    contains_substr(ledger_voice, "quantity/accounting record") &&
                    !contains_substr(ledger_voice, "refrain repeated"),
                    "trade ledger voice should render as terse administrative record, not oral-song prose");
            require(contains_substr(song_voice, "Oral song refrain") &&
                    contains_substr(song_voice, "refrain repeated") &&
                    !contains_substr(song_voice, "Administrative ledger entries"),
                    "oral history voice should render with repetitive mythic song form");
            require(contains_substr(chronicle_voice, "[lacuna]") &&
                    contains_substr(chronicle_voice, "[uncertain reading]") &&
                    contains_substr(chronicle_voice, "Translation note"),
                    "damaged manuscript voice should include lacunae and translation uncertainty");
            require(contains_substr(public_decree_voice, "disputed royal decree") &&
                    !contains_substr(public_decree_voice, "forged decree"),
                    "public disputed-decree voice should avoid internal forgery classification");
            require(contains_substr(curator_decree_voice, "forged/disputed decree") &&
                    contains_substr(curator_decree_voice, "forged decree"),
                    "curator disputed-decree voice should expose the stronger internal catalog classification");
            require(ledger->claim_ids == ledger_claims_before && a.hidden_truth.events().size() == hidden_event_count_before,
                    "voice rendering should not mutate artifact claims or hidden truth");
            require(contains_substr(render_artifact_text(*a.public_archive.find_artifact("artifact.victory_inscription"), a, AccessLevel::Public, 812), "secondary line"),
                    "secondary voice claim links should render as secondary lines rather than primary payload");

            auto has_bad_punctuation = [](const std::string& text) {
                return contains_substr(text, "..") || contains_substr(text, ".;") ||
                       contains_substr(text, ";.") || contains_substr(text, ";;");
            };
            require(!has_bad_punctuation(ledger_voice) && !has_bad_punctuation(song_voice) &&
                    !has_bad_punctuation(chronicle_voice) && !has_bad_punctuation(public_decree_voice) &&
                    !has_bad_punctuation(curator_decree_voice),
                    "rendered artifact text should not contain duplicate or conflicting sentence punctuation");

            Artifact ledger_as_song = *ledger;
            ledger_as_song.voice_register = ArtifactVoiceRegister::OralSong;
            const std::string ledger_as_song_voice = render_artifact_text(ledger_as_song, a, AccessLevel::Public, 812);
            require(ledger_voice != ledger_as_song_voice && ledger_as_song.claim_ids == ledger->claim_ids,
                    "changing voice register should change rendered text while preserving claim IDs");

            Artifact ledger_secondary = *ledger;
            if (!ledger_secondary.voice_claim_links.empty()) {
                ledger_secondary.voice_claim_links.front().role = ArtifactVoiceClaimRole::SecondaryLine;
            }
            const std::string ledger_secondary_voice = render_artifact_text(ledger_secondary, a, AccessLevel::Public, 812);
            require(ledger_secondary_voice != ledger_voice && contains_substr(ledger_secondary_voice, "no visible primary voice claim"),
                    "changing a voice claim role should change rendering while preserving the claim itself");
        }

        {
            ArchiveEngineState voice_link_state = initialize_archive_engine(42);
            Artifact* ledger_mutable = voice_link_state.public_archive.find_artifact_mutable("artifact.silt_ledger");
            require(ledger_mutable != nullptr, "voice-link test should find mutable ledger artifact");
            if (ledger_mutable != nullptr) {
                ledger_mutable->claim_ids.push_back("claim.unlinked_voice_note");
                voice_link_state.public_archive.add_claim(Claim{
                    "claim.unlinked_voice_note",
                    ledger_mutable->id,
                    ClaimType::FactualClaim,
                    "unlinked margin",
                    "mentions",
                    "non-rendered aside",
                    "UNLINKED VOICE SHOULD NOT RENDER.",
                    0.50,
                    AccessLevel::Public,
                    ClaimSemantics{PredicateType::ExistedInYear, std::nullopt, std::nullopt, std::optional<int>{607}},
                });
                const std::string rendered = render_artifact_text(*ledger_mutable, voice_link_state, AccessLevel::Public, 812);
                require(!contains_substr(rendered, "UNLINKED VOICE SHOULD NOT RENDER"),
                        "renderer should use only voice-linked claims, not every visible claim on the artifact");

                ledger_mutable->claim_ids.push_back("claim.omitted_public_voice_note");
                ledger_mutable->voice_claim_links.push_back(ArtifactVoiceClaimLink{
                    "claim.omitted_public_voice_note",
                    ArtifactVoiceClaimRole::OmittedFromPublicRendering,
                    1.0,
                });
                voice_link_state.public_archive.add_claim(Claim{
                    "claim.omitted_public_voice_note",
                    ledger_mutable->id,
                    ClaimType::FactualClaim,
                    "omitted margin",
                    "mentions",
                    "restricted rendering aside",
                    "OMITTED PUBLIC VOICE SHOULD NOT RENDER.",
                    0.50,
                    AccessLevel::Public,
                    ClaimSemantics{PredicateType::ExistedInYear, std::nullopt, std::nullopt, std::optional<int>{607}},
                });
                const std::string public_rendered = render_artifact_text(*ledger_mutable, voice_link_state, AccessLevel::Public, 812);
                require(!contains_substr(public_rendered, "OMITTED PUBLIC VOICE SHOULD NOT RENDER"),
                        "claims marked OmittedFromPublicRendering should not render publicly");
            }
        }

        {
            ArchiveEngineState missing_voice_link_state = initialize_archive_engine(42);
            Artifact* ledger_mutable = missing_voice_link_state.public_archive.find_artifact_mutable("artifact.silt_ledger");
            require(ledger_mutable != nullptr, "missing voice link validation test should find mutable ledger artifact");
            if (ledger_mutable != nullptr) {
                ledger_mutable->voice_claim_links.push_back(ArtifactVoiceClaimLink{"claim.missing_voice", ArtifactVoiceClaimRole::PrimaryLine, 1.0});
            }
            const std::vector<std::string> errors = validate_full_state(missing_voice_link_state);
            const bool saw_missing_voice_claim = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
                return contains_substr(error, "voice claim link references missing claim claim.missing_voice");
            });
            require(saw_missing_voice_claim, "validation should reject voice claim links to missing claims");
        }

        {
            ArchiveEngineState mismatch_voice_link_state = initialize_archive_engine(42);
            Artifact* ledger_mutable = mismatch_voice_link_state.public_archive.find_artifact_mutable("artifact.silt_ledger");
            require(ledger_mutable != nullptr, "mismatched voice link validation test should find mutable ledger artifact");
            if (ledger_mutable != nullptr) {
                ledger_mutable->voice_claim_links.push_back(ArtifactVoiceClaimLink{"claim.victory_restoration", ArtifactVoiceClaimRole::PrimaryLine, 1.0});
            }
            const std::vector<std::string> errors = validate_full_state(mismatch_voice_link_state);
            const bool saw_mismatch = std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
                return contains_substr(error, "voice claim link claim.victory_restoration belongs to different source artifact artifact.victory_inscription");
            });
            require(saw_mismatch, "validation should reject voice claim links whose claims belong to a different artifact");
        }

        const std::string artifact_output = format_artifacts(a, AccessLevel::Public, 812);
        require(contains_substr(artifact_output, "rendered text:") &&
                contains_substr(artifact_output, "Administrative ledger entries") &&
                contains_substr(artifact_output, "Oral song refrain"),
                "artifact display should use render_artifact_text() rather than raw public_text output");
    }


    {
        const OriginalitySignal generic_moon = score_originality_feature(
            "test.generic_moon_cult",
            "A generic moon cult worships the night goddess in a lost temple."
        );
        require(std::find(generic_moon.trope_flags.begin(), generic_moon.trope_flags.end(), TropeFlag::GenericMoonCult) != generic_moon.trope_flags.end(),
                "generic moon cult text should be flagged as GenericMoonCult");
        require(generic_moon.civilization_specificity_score < 0.30,
                "generic moon cult text should score low on civilization specificity");

        const std::vector<OriginalitySignal> signals = build_originality_signals(a);
        auto find_signal = [&](const std::string& feature_id) -> const OriginalitySignal* {
            const auto it = std::find_if(signals.begin(), signals.end(), [&](const OriginalitySignal& signal) {
                return signal.feature_id == feature_id;
            });
            return it == signals.end() ? nullptr : &*it;
        };

        const OriginalitySignal* drowned = find_signal("office.drowned_chancellor");
        require(drowned != nullptr && drowned->civilization_specificity_score >= 0.70,
                "Drowned Chancellor should score as civilization-specific");
        require(drowned != nullptr && std::find(drowned->trope_flags.begin(), drowned->trope_flags.end(), TropeFlag::GenericMoonCult) == drowned->trope_flags.end(),
                "Moon-lock / Drowned Chancellor material should not be treated as generic moon cult");

        const OriginalitySignal* three = find_signal("claim.three_as_one");
        require(three != nullptr && three->civilization_specificity_score >= 0.70,
                "Three Keepers song should score high on civilization-specific ritual authority compression");

        const OriginalitySignal* victory = find_signal("artifact.victory_inscription");
        require(victory != nullptr && std::find(victory->trope_flags.begin(), victory->trope_flags.end(), TropeFlag::RomanStyleRestorationInscription) != victory->trope_flags.end(),
                "victory inscription should receive restoration-inscription trope pressure");

        const OriginalitySignal divine = score_originality_feature(OriginalityFeature{
            "test.generic_divine_king",
            OriginalityFeatureKind::Institution,
            "The divine king ruled by sacred kingship and fulfilled a chosen prophecy."
        });
        require(std::find(divine.trope_flags.begin(), divine.trope_flags.end(), TropeFlag::DivineKingAnalogue) != divine.trope_flags.end() &&
                divine.civilization_specificity_score < 0.30 &&
                divine.direct_copy_risk_score > divine.transformed_trope_score,
                "generic divine king phrase should be flagged, score low specificity, and carry direct-copy risk");

        const OriginalitySignal moon_lock = score_originality_feature(OriginalityFeature{
            "test.moon_lock_authority",
            OriginalityFeatureKind::Office,
            "Moon-lock office regulates reservoir authority through lock tenders and local seal procedure."
        });
        require(std::find(moon_lock.trope_flags.begin(), moon_lock.trope_flags.end(), TropeFlag::GenericMoonCult) == moon_lock.trope_flags.end() &&
                moon_lock.civilization_specificity_score >= 0.60 &&
                moon_lock.transformed_trope_score > moon_lock.direct_copy_risk_score,
                "Moon-lock office language should not be treated as generic moon cult and should show transformed local specificity");

        const OriginalitySignal dead_king = score_originality_feature(OriginalityFeature{
            "test.dead_king_mortuary_office",
            OriginalityFeatureKind::LegalFormula,
            "The dead king's seal remains legally active through a mortuary office and archived lock procedure."
        });
        require(std::find(dead_king.trope_flags.begin(), dead_king.trope_flags.end(), TropeFlag::DivineKingAnalogue) == dead_king.trope_flags.end() &&
                dead_king.civilization_specificity_score >= 0.45,
                "dead-king legal seal through mortuary office should not collapse into generic divine kingship");

        const OriginalitySignal hubris = score_originality_feature(OriginalityFeature{
            "test.hubris_collapse",
            OriginalityFeatureKind::Institution,
            "The lost empire collapsed from hubris."
        });
        require(std::find(hubris.trope_flags.begin(), hubris.trope_flags.end(), TropeFlag::LostEmpireHubrisCollapse) != hubris.trope_flags.end() &&
                hubris.civilization_specificity_score < 0.30,
                "generic lost empire collapsed from hubris should be flagged and score low specificity");

        const OriginalitySignal reservoir_collapse = score_originality_feature(OriginalityFeature{
            "test.reservoir_collapse_specific",
            OriginalityFeatureKind::Claim,
            "Reservoir collapse follows tax revolt, Green-Seal script reform, and silt levy dispute."
        });
        require(std::find(reservoir_collapse.trope_flags.begin(), reservoir_collapse.trope_flags.end(), TropeFlag::LostEmpireHubrisCollapse) == reservoir_collapse.trope_flags.end() &&
                reservoir_collapse.civilization_specificity_score >= 0.55,
                "reservoir collapse tied to tax revolt and script reform should not be generic hubris collapse");

        require(victory != nullptr && !victory->rationales.empty(),
                "trope-flagged originality signals should carry detector rationales");
        require(victory != nullptr && victory->transformed_trope_score > 0.0 && victory->direct_copy_risk_score < victory->trope_similarity_score + 0.25,
                "local transformation and direct-copy risk should be tracked separately from raw trope similarity");

        const std::string public_originality = format_originality(a, AccessLevel::Public);
        const std::string scholar_originality = format_originality(a, AccessLevel::Scholar);
        const std::string curator_originality = format_originality(a, AccessLevel::Curator);
        const std::string debug_originality = format_originality(a, AccessLevel::Debug);
        require(!contains_substr(public_originality, "office.drowned_chancellor") &&
                contains_substr(public_originality, "restricted to curator/canon/debug access"),
                "public users should not see originality/meta-generation feature audit details");
        require(!contains_substr(scholar_originality, "office.drowned_chancellor"),
                "scholar users should not see originality/meta-generation feature audit details");
        require(contains_substr(curator_originality, "office.drowned_chancellor") &&
                contains_substr(curator_originality, "civilization_specificity="),
                "curator users should see feature-level originality audit details");
        require(contains_substr(debug_originality, "audit layer only"),
                "debug originality report should state that originality scoring is audit-only");

        const std::string before_originality_serialization = serialize_for_replay_test(a);
        (void)build_originality_signals(a);
        (void)format_originality(a, AccessLevel::Debug);
        const std::string after_originality_serialization = serialize_for_replay_test(a);
        require(before_originality_serialization == after_originality_serialization,
                "originality scoring should not mutate hidden truth, artifacts, claims, contradictions, or mysteries");
    }

    {
        const OriginalitySignal repeated_broad = score_originality_feature(OriginalityFeature{
            "test.repeated_broad_archive_tokens",
            OriginalityFeatureKind::Artifact,
            "seal lock moon reservoir seal lock moon reservoir seal lock moon reservoir"
        });
        require(repeated_broad.civilization_specificity_score < 0.45 &&
                repeated_broad.transformed_trope_score < 0.45,
                "v17.1 originality calibration should prevent repeated broad local tokens from maxing specificity");

        const OriginalitySignal strong_office = score_originality_feature(OriginalityFeature{
            "test.drowned_chancellor_specificity_calibrated",
            OriginalityFeatureKind::Office,
            "Drowned Chancellor receives lock authority at Reservoir Gate through Green-Seal procedure"
        });
        require(strong_office.civilization_specificity_score >= 0.70 &&
                strong_office.civilization_specificity_score < 1.0,
                "v17.1 calibration should keep Drowned Chancellor material high-specificity without reporting perfect saturation");

        const OriginalitySignal strong_ritual = score_originality_feature(OriginalityFeature{
            "test.three_keepers_specificity_calibrated",
            OriginalityFeatureKind::Ritual,
            "Three Keepers ritual compresses lower lock lineage into one moon judge under Reservoir Gate"
        });
        require(strong_ritual.civilization_specificity_score >= 0.70 &&
                strong_ritual.civilization_specificity_score < 1.0,
                "v17.1 calibration should keep Three Keepers ritual material high-specificity without perfect saturation");

        const OriginalitySignal broad_phrase = score_originality_feature(OriginalityFeature{
            "test.broad_moon_seal_lock_phrase",
            OriginalityFeatureKind::Artifact,
            "moon seal lock reservoir moon seal lock reservoir"
        });
        require(broad_phrase.civilization_specificity_score < 0.45,
                "generic moon/seal/lock/reservoir phrasing should remain low or moderate without real institutions");

        const OriginalitySignal generic_moon_after_calibration = score_originality_feature(
            "test.generic_moon_cult_after_calibration",
            "A generic moon cult worships the night goddess in a lost temple."
        );
        require(std::find(generic_moon_after_calibration.trope_flags.begin(),
                          generic_moon_after_calibration.trope_flags.end(),
                          TropeFlag::GenericMoonCult) != generic_moon_after_calibration.trope_flags.end(),
                "generic moon cult should remain flagged after originality calibration");
    }

    const bool all_contradictions_have_causes = std::all_of(
        a.public_archive.contradictions().begin(),
        a.public_archive.contradictions().end(),
        [](const auto& entry) {
            return entry.second.assigned_cause != ContradictionCause::None &&
                   entry.second.assigned_cause != ContradictionCause::UnresolvedGenerationBug;
        }
    );
    require(all_contradictions_have_causes, "every contradiction should have a non-bug cause");

    {
        const ArchiveEngineState candidate_state = initialize_archive_engine(42);
        const std::string before = serialize_for_replay_test(candidate_state);

        const CandidateFeature generic = sample_candidate_feature("generic_moon_cult");
        const CandidateEvaluation generic_eval = evaluate_candidate_feature(candidate_state, generic, AccessLevel::Curator);
        require(generic_eval.decision == CandidateDecision::NeedsCuratorReview || generic_eval.decision == CandidateDecision::Reject,
                "generic moon cult candidate should be rejected or require curator review due to originality risk");
        require(std::find(generic_eval.originality.trope_flags.begin(), generic_eval.originality.trope_flags.end(), TropeFlag::GenericMoonCult) != generic_eval.originality.trope_flags.end() &&
                !generic_eval.originality.rationales.empty(),
                "generic moon cult candidate should include originality rationales");

        const CandidateFeature moon_lock = sample_candidate_feature("moon_lock_fragment");
        const CandidateEvaluation moon_lock_eval = evaluate_candidate_feature(candidate_state, moon_lock, AccessLevel::Curator);
        require(moon_lock_eval.decision == CandidateDecision::Accept,
                "Moon-lock legal fragment candidate should be accepted when metadata links are valid and specificity is high");
        require(std::find(moon_lock_eval.originality.trope_flags.begin(), moon_lock_eval.originality.trope_flags.end(), TropeFlag::GenericMoonCult) == moon_lock_eval.originality.trope_flags.end() &&
                moon_lock_eval.originality.civilization_specificity_score >= 0.45,
                "Moon-lock candidate should not be treated as generic moon cult");

        const CandidateFeature early_green = sample_candidate_feature("early_green_seal_decree");
        const CandidateEvaluation early_green_eval = evaluate_candidate_feature(candidate_state, early_green, AccessLevel::Curator);
        require(early_green_eval.decision == CandidateDecision::Reject,
                "candidate using Green-Seal before 612 should be rejected unless mediated");
        require(std::any_of(early_green_eval.anachronisms.begin(), early_green_eval.anachronisms.end(), [](const AnachronismReport& report) {
            return report.status == AnachronismStatus::InvalidGenerationBug && contains_substr(report.referenced_item, "Green-Seal");
        }), "early Green-Seal candidate should include an invalid claimed-surface anachronism");

        const CandidateFeature unmediated_office = sample_candidate_feature("early_drowned_chancellor_unmediated");
        const CandidateEvaluation unmediated_eval = evaluate_candidate_feature(candidate_state, unmediated_office, AccessLevel::Curator);
        require(unmediated_eval.decision == CandidateDecision::Reject && !unmediated_eval.predicted_contradictions.empty(),
                "candidate referencing Drowned Chancellor before creation should be rejected without mediation and predict a contradiction");

        const CandidateFeature forged_office = sample_candidate_feature("early_drowned_chancellor_forgery");
        const CandidateEvaluation forged_eval = evaluate_candidate_feature(candidate_state, forged_office, AccessLevel::Curator);
        require(forged_eval.decision == CandidateDecision::AcceptAsForgery,
                "candidate referencing Drowned Chancellor before creation should be accepted as forgery when mediation is explicit");
        require(std::any_of(forged_eval.anachronisms.begin(), forged_eval.anachronisms.end(), [](const AnachronismReport& report) {
            return report.status == AnachronismStatus::ValidBecauseForged;
        }), "forged candidate should report mediated anachronism");

        const CandidateFeature missing = sample_candidate_feature("missing_metadata");
        const CandidateEvaluation missing_eval = evaluate_candidate_feature(candidate_state, missing, AccessLevel::Curator);
        require(missing_eval.decision == CandidateDecision::Reject && !missing_eval.validation_errors.empty(),
                "candidate with missing metadata should be rejected");

        const std::string public_candidate = format_candidate_query(candidate_state, AccessLevel::Public, "generic_moon_cult");
        const std::string curator_candidate = format_candidate_query(candidate_state, AccessLevel::Curator, "generic_moon_cult");
        require(!contains_substr(public_candidate, "Originality audit") &&
                !contains_substr(public_candidate, "trope_similarity") &&
                contains_substr(public_candidate, "candidate originality internals are restricted"),
                "public candidate output should not expose originality internals");
        require(contains_substr(curator_candidate, "Originality audit") && contains_substr(curator_candidate, "rationales"),
                "curator candidate output should include originality audit rationales");

        const std::string public_early_green = format_candidate_query(candidate_state, AccessLevel::Public, "early_green_seal_decree");
        require(!contains_substr(public_early_green, "valid_range=") &&
                !contains_substr(public_early_green, "invalid_generation_bug") &&
                !contains_substr(public_early_green, "office_drowned_chancellor") &&
                !contains_substr(public_early_green, "contradiction.auto"),
                "public candidate output should redact exact valid ranges, internal statuses, hidden entity IDs, and generated contradiction IDs");
        require(contains_substr(public_early_green, "Predicted issues:") &&
                contains_substr(public_early_green, "chronologically incompatible with the visible archive") &&
                contains_substr(public_early_green, "Candidate may conflict with known chronology or cataloged authority terms"),
                "public candidate output should use sanitized issue and contradiction summaries");

        const std::string curator_early_green = format_candidate_query(candidate_state, AccessLevel::Curator, "early_green_seal_decree");
        require(contains_substr(curator_early_green, "valid_range=612-900") &&
                contains_substr(curator_early_green, "valid_range=617-900") &&
                contains_substr(curator_early_green, "status=invalid_generation_bug") &&
                contains_substr(curator_early_green, "contradiction.auto.candidate_unavailable_entity"),
                "curator candidate output should retain exact predicted anachronism and contradiction details");

        const std::string public_forgery_candidate = format_candidate_query(candidate_state, AccessLevel::Public, "early_drowned_chancellor_forgery");
        require(contains_substr(public_forgery_candidate, "decision: AcceptWithDeclaredMediation") &&
                !contains_substr(public_forgery_candidate, "decision: AcceptAsForgery") &&
                !contains_substr(public_forgery_candidate, "valid_because_forged"),
                "public candidate output should redact internal AcceptAsForgery and valid_because_forged labels");

        const std::string debug_candidate = format_candidate_query(candidate_state, AccessLevel::Debug, "generic_moon_cult");
        require(contains_substr(debug_candidate, "Originality audit") && contains_substr(debug_candidate, "rationales"),
                "debug candidate output should retain originality rationales");

        const std::string after = serialize_for_replay_test(candidate_state);
        require(before == after,
                "candidate evaluation should not mutate archive state");
    }

    {
        ArchiveEngineState discovery_state = initialize_archive_engine(42);
        require(validate_discovery_log(discovery_state).empty(),
                "initialized archive should have one valid discovery record per artifact");
        discovery_state.discovery_log.pop_back();
        const std::vector<std::string> missing_discovery_errors = validate_discovery_log(discovery_state);
        const bool saw_missing_discovery = std::any_of(missing_discovery_errors.begin(), missing_discovery_errors.end(), [](const std::string& error) {
            return contains_substr(error, "has no matching discovery record");
        });
        require(saw_missing_discovery,
                "discovery-log validation should reject artifacts without a matching discovery record");

        ArchiveEngineState mismatch_discovery_state = initialize_archive_engine(42);
        require(!mismatch_discovery_state.discovery_log.empty(), "test setup should have discovery records");
        if (!mismatch_discovery_state.discovery_log.empty()) {
            mismatch_discovery_state.discovery_log.front().discovery_year += 1;
            mismatch_discovery_state.discovery_log.front().site_id = "site.wrong";
        }
        const std::vector<std::string> mismatch_errors = validate_discovery_log(mismatch_discovery_state);
        const bool saw_year_mismatch = std::any_of(mismatch_errors.begin(), mismatch_errors.end(), [](const std::string& error) {
            return contains_substr(error, "does not match artifact") && contains_substr(error, "discovery_year");
        });
        const bool saw_site_mismatch = std::any_of(mismatch_errors.begin(), mismatch_errors.end(), [](const std::string& error) {
            return contains_substr(error, "does not match artifact") && contains_substr(error, "location_found");
        });
        require(saw_year_mismatch && saw_site_mismatch,
                "discovery-log validation should reject discovery year and site mismatches");
    }

    {
        ArchiveEngineState materialization_state = initialize_archive_engine(42);
        const CandidateFeature moon_lock = sample_candidate_feature("moon_lock_fragment");
        const CandidateEvaluation moon_lock_eval = evaluate_candidate_feature(materialization_state, moon_lock, AccessLevel::Curator);
        const std::string before_public_attempt = serialize_for_replay_test(materialization_state);
        MaterializationResult public_result = materialize_candidate_feature(materialization_state, moon_lock, moon_lock_eval, AccessLevel::Public);
        require(!public_result.mutated && public_result.decision == MaterializationDecision::Reject &&
                serialize_for_replay_test(materialization_state) == before_public_attempt,
                "public users should not be able to materialize candidates or mutate archive state");

        MaterializationResult scholar_result = materialize_candidate_feature(materialization_state, moon_lock, moon_lock_eval, AccessLevel::Scholar);
        require(!scholar_result.mutated && serialize_for_replay_test(materialization_state) == before_public_attempt,
                "scholar users should not be able to materialize candidates or mutate archive state");

        MaterializationResult curator_result = materialize_candidate_feature(materialization_state, moon_lock, moon_lock_eval, AccessLevel::Curator);
        require(curator_result.mutated && curator_result.decision == MaterializationDecision::InsertArtifact &&
                materialization_state.public_archive.find_artifact("artifact.materialized_moon_lock_fragment") != nullptr &&
                materialization_state.public_archive.find_claim("claim.materialized_moon_lock_authority") != nullptr &&
                validate_full_state(materialization_state).empty(),
                "curator should be able to materialize an accepted Moon-lock artifact and leave the state valid");
        require(!contains_substr(format_artifacts(materialization_state, AccessLevel::Public, 812), "artifact.materialized_moon_lock_fragment") &&
                contains_substr(format_artifacts(materialization_state, AccessLevel::Public, 813), "artifact.materialized_moon_lock_fragment") &&
                !contains_substr(format_claims(materialization_state, AccessLevel::Public, 812), "claim.materialized_moon_lock_authority") &&
                contains_substr(format_claims(materialization_state, AccessLevel::Public, 813), "claim.materialized_moon_lock_authority"),
                "materialized artifacts and claims should obey archive-year visibility");

        const std::string before_duplicate_attempt = serialize_for_replay_test(materialization_state);
        MaterializationResult duplicate_result = materialize_candidate_feature(materialization_state, moon_lock, moon_lock_eval, AccessLevel::Curator);
        require(!duplicate_result.mutated && serialize_for_replay_test(materialization_state) == before_duplicate_attempt &&
                contains_substr(duplicate_result.explanation, "rolled back"),
                "failed duplicate materialization should roll back and preserve serialized state exactly");

        CandidateEvaluation mismatched_eval = moon_lock_eval;
        mismatched_eval.evaluated_candidate_id = "candidate.other";
        MaterializationResult mismatched_result = materialize_candidate_feature(materialization_state, moon_lock, mismatched_eval, AccessLevel::Curator);
        require(!mismatched_result.mutated && contains_substr(mismatched_result.explanation, "does not match"),
                "materialization should reject an evaluation bound to a different candidate");

        CandidateEvaluation stale_eval = moon_lock_eval;
        stale_eval.decision = CandidateDecision::AcceptAsForgery;
        MaterializationResult stale_result = materialize_candidate_feature(materialization_state, moon_lock, stale_eval, AccessLevel::Curator);
        require(!stale_result.mutated && contains_substr(stale_result.explanation, "stale"),
                "materialization should recompute evaluation and reject stale or tampered decisions");

        const std::string public_formatted_success = format_materialization_result(curator_result, moon_lock, AccessLevel::Public);
        require(!contains_substr(public_formatted_success, "artifact.materialized_moon_lock_fragment") &&
                !contains_substr(public_formatted_success, "claim.materialized_moon_lock_authority") &&
                contains_substr(public_formatted_success, "Inserted artifacts:\n- restricted") &&
                contains_substr(public_formatted_success, "Inserted claims:\n- restricted"),
                "materialization result formatting should hide inserted object IDs if a successful result is rendered for public access");
    }

    {
        ArchiveEngineState rejected_state = initialize_archive_engine(42);
        const std::string before = serialize_for_replay_test(rejected_state);
        const CandidateFeature early_green = sample_candidate_feature("early_green_seal_decree");
        const CandidateEvaluation early_green_eval = evaluate_candidate_feature(rejected_state, early_green, AccessLevel::Curator);
        const MaterializationResult rejected = materialize_candidate_feature(rejected_state, early_green, early_green_eval, AccessLevel::Curator);
        require(!rejected.mutated && serialize_for_replay_test(rejected_state) == before,
                "curator materialization of a rejected early Green-Seal candidate should not mutate state");

        const CandidateFeature missing = sample_candidate_feature("missing_metadata");
        const CandidateEvaluation missing_eval = evaluate_candidate_feature(rejected_state, missing, AccessLevel::Curator);
        const MaterializationResult missing_result = materialize_candidate_feature(rejected_state, missing, missing_eval, AccessLevel::Curator);
        require(!missing_result.mutated && serialize_for_replay_test(rejected_state) == before,
                "candidate lacking materialization-safe metadata should not mutate state");
    }

    {
        ArchiveEngineState forgery_state = initialize_archive_engine(42);
        const CandidateFeature forged = sample_candidate_feature("early_drowned_chancellor_forgery");
        const CandidateEvaluation forged_eval = evaluate_candidate_feature(forgery_state, forged, AccessLevel::Curator);
        const MaterializationResult forgery_result = materialize_candidate_feature(forgery_state, forged, forged_eval, AccessLevel::Curator);
        require(forgery_result.mutated &&
                forgery_state.public_archive.find_artifact("artifact.materialized_early_drowned_chancellor_forgery") != nullptr &&
                forgery_state.public_archive.find_claim("claim.materialized_forged_aru_chancellor") != nullptr &&
                !forgery_result.inserted_contradiction_ids.empty() &&
                validate_full_state(forgery_state).empty(),
                "curator should be able to materialize an accepted forgery with mediated anachronism and detected contradiction");
        const std::string public_materialized_forgery = format_artifacts(forgery_state, AccessLevel::Public, 813);
        const std::string curator_materialized_forgery = format_artifacts(forgery_state, AccessLevel::Curator, 813);
        require(contains_substr(public_materialized_forgery, "royal decree, disputed") &&
                !contains_substr(public_materialized_forgery, "forged decree") &&
                contains_substr(curator_materialized_forgery, "forged decree"),
                "materialized forgery should preserve public disputed-decree redaction and curator classification");
    }

    {
        ArchiveEngineState cli_materialization_state = initialize_archive_engine(42);
        const std::string materialization_output = format_materialization_query(cli_materialization_state, AccessLevel::Curator, "moon_lock_fragment");
        require(contains_substr(materialization_output, "mutated: true") &&
                contains_substr(materialization_output, "artifact.materialized_moon_lock_fragment") &&
                cli_materialization_state.public_archive.find_artifact("artifact.materialized_moon_lock_fragment") != nullptr,
                "materialization query should explicitly mutate the run state and report inserted IDs for curator access");
    }


    {
        const ArchiveEngineState generation_state = initialize_archive_engine(42);
        const std::string before_generation = serialize_for_replay_test(generation_state);

        const CandidateGenerationRequest corroborating_request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 42};
        const std::vector<CandidateFeature> generated_a = generate_candidate_features(generation_state, corroborating_request);
        const std::vector<CandidateFeature> generated_b = generate_candidate_features(generation_state, corroborating_request);
        require(generated_a.size() == 3 && generated_b.size() == 3,
                "candidate generator should produce a small deterministic batch");
        require(generated_a.front().id == generated_b.front().id && generated_a.front().description == generated_b.front().description,
                "same seed and request should generate identical candidate proposals");

        const CandidateGenerationRequest different_seed_request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 43};
        const std::vector<CandidateFeature> generated_different = generate_candidate_features(generation_state, different_seed_request);
        require(!generated_different.empty() &&
                (generated_different.front().id != generated_a.front().id || generated_different.front().description != generated_a.front().description),
                "different seed should change candidate ordering or surface details");

        require(serialize_for_replay_test(generation_state) == before_generation,
                "candidate generation should not mutate archive state");

        const CandidateEvaluation corroborating_eval = evaluate_candidate_feature(generation_state, generated_a.front(), AccessLevel::Curator);
        require(corroborating_eval.decision == CandidateDecision::Accept,
                "generated corroborating fragment should pass through candidate evaluator and be accepted");
        require(generated_a.front().structured_artifact_metadata.has_value() &&
                generated_a.front().structured_artifact_metadata->true_creation_year.has_value() &&
                *generated_a.front().structured_artifact_metadata->true_creation_year >= 617 &&
                *generated_a.front().structured_artifact_metadata->true_creation_year <= 690,
                "generated corroborating fragment should carry structured metadata");
        if (generated_a.front().structured_artifact_metadata.has_value() &&
            generated_a.front().structured_artifact_metadata->true_creation_year.has_value() &&
            *generated_a.front().structured_artifact_metadata->true_creation_year < 640) {
            require(generated_a.front().structured_artifact_metadata->dialect_id == std::optional<std::string>{"dialect.upper_lattice"},
                    "generated corroborating fragment before 640 should not use late_lock_hand dialect");
        }

        const CandidateGenerationRequest forgery_request{CandidateGenerationStrategy::AddMisleadingForgery, 553, "drowned_chancellor", 42};
        const std::vector<CandidateFeature> generated_forgery = generate_candidate_features(generation_state, forgery_request);
        require(!generated_forgery.empty(), "misleading-forgery strategy should generate candidates");
        const CandidateEvaluation forgery_eval = evaluate_candidate_feature(generation_state, generated_forgery.front(), AccessLevel::Curator);
        require(forgery_eval.decision == CandidateDecision::AcceptAsForgery && !forgery_eval.anachronisms.empty(),
                "generated early Green-Seal / Drowned Chancellor forgery should require explicit mediation");

        const CandidateEvaluation legacy_forgery_eval = evaluate_candidate_feature(
            generation_state,
            sample_candidate_feature("early_drowned_chancellor_forgery"),
            AccessLevel::Curator
        );
        require(legacy_forgery_eval.decision == CandidateDecision::AcceptAsForgery,
                "legacy unstructured sample candidate should still receive mediation from prose keyword fallback");

        const CandidateGenerationRequest ritual_request{CandidateGenerationStrategy::AddRitualVariant, 700, "three_keepers", 42};
        const std::vector<CandidateFeature> generated_ritual = generate_candidate_features(generation_state, ritual_request);
        require(!generated_ritual.empty(), "ritual-variant strategy should generate candidates");
        const CandidateEvaluation ritual_eval = evaluate_candidate_feature(generation_state, generated_ritual.front(), AccessLevel::Curator);
        require(ritual_eval.decision == CandidateDecision::Accept || ritual_eval.decision == CandidateDecision::NeedsCuratorReview,
                "generated ritual variant should evaluate without bypassing the candidate gate");
        require(serialize_for_replay_test(generation_state) == before_generation,
                "generated ritual candidates should not affect mysteries or archive state until materialized");

        const std::optional<GenerationTarget> lock_target = resolve_generation_target(generation_state, "lock_authority");
        require(lock_target.has_value() && lock_target->mystery_id == std::optional<std::string>{"mystery.third_lock_authority"} &&
                std::find(lock_target->entity_ids.begin(), lock_target->entity_ids.end(), "office.drowned_chancellor") != lock_target->entity_ids.end(),
                "target_topic=lock_authority should resolve to the Third Lock Authority mystery and Drowned Chancellor entity");

        const std::optional<GenerationTarget> silt_target = resolve_generation_target(generation_state, "silt_levy");
        require(silt_target.has_value() && !silt_target->mystery_id.has_value() &&
                std::find(silt_target->claim_ids.begin(), silt_target->claim_ids.end(), "claim.levy_exists_607") != silt_target->claim_ids.end(),
                "target_topic=silt_levy should resolve to levy evidence without defaulting to the lock-authority mystery");

        const CandidateGenerationRequest silt_request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "silt_levy", 42};
        const std::vector<CandidateFeature> generated_silt = generate_candidate_features(generation_state, silt_request);
        require(!generated_silt.empty() &&
                std::none_of(generated_silt.front().proposed_links.begin(), generated_silt.front().proposed_links.end(), [](const std::string& link) {
                    return link == "mystery:mystery.third_lock_authority";
                }) &&
                std::any_of(generated_silt.front().proposed_links.begin(), generated_silt.front().proposed_links.end(), [](const std::string& link) {
                    return link == "claim:claim.levy_exists_607";
                }),
                "generated silt_levy candidate should not automatically become Third Lock Authority mystery evidence");

        const std::optional<GenerationTarget> missing_target = resolve_generation_target(generation_state, "unknown_topic");
        const std::vector<CandidateFeature> generated_unknown = generate_candidate_features(generation_state,
            CandidateGenerationRequest{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "unknown_topic", 42});
        require(!missing_target.has_value() && generated_unknown.empty(),
                "unknown generation target topic should fail cleanly instead of silently linking to a fixture mystery");

        {
            CandidateFeature invalid_dialect;
            invalid_dialect.id = "candidate.test.invalid_structured_dialect";
            invalid_dialect.type = CandidateFeatureType::Artifact;
            invalid_dialect.description = "Structured test candidate with otherwise plausible lock authority metadata.";
            CandidateArtifactMetadata metadata;
            metadata.true_creation_year = 620;
            metadata.claimed_creation_year = 620;
            metadata.discovery_year = 813;
            metadata.location_created = "site.reservoir_gate";
            metadata.location_found = "site.salt_cellar_archive";
            metadata.language_id = "language.lattice_dialect";
            metadata.dialect_id = "dialect.late_lock_hand";
            metadata.script_id = "script.green_seal";
            metadata.referenced_entity_ids = {"office.drowned_chancellor"};
            invalid_dialect.structured_artifact_metadata = metadata;
            const CandidateEvaluation invalid_dialect_eval = evaluate_candidate_feature(generation_state, invalid_dialect, AccessLevel::Curator);
            require(invalid_dialect_eval.decision == CandidateDecision::Reject &&
                    std::any_of(invalid_dialect_eval.validation_errors.begin(), invalid_dialect_eval.validation_errors.end(), [](const std::string& error) {
                        return contains_substr(error, "dialect_id") && contains_substr(error, "unavailable");
                    }),
                    "candidate with invalid structured dialect year should be rejected through metadata validation");
        }

        {
            CandidateFeature missing_location;
            missing_location.id = "candidate.test.missing_structured_location";
            missing_location.type = CandidateFeatureType::Artifact;
            missing_location.description = "Structured test candidate with missing location metadata.";
            CandidateArtifactMetadata metadata;
            metadata.true_creation_year = 650;
            metadata.claimed_creation_year = 650;
            metadata.discovery_year = 813;
            metadata.location_created = "site.reservoir_gate";
            metadata.language_id = "language.lattice_dialect";
            metadata.dialect_id = "dialect.late_lock_hand";
            metadata.script_id = "script.green_seal";
            metadata.referenced_entity_ids = {"office.drowned_chancellor"};
            missing_location.structured_artifact_metadata = metadata;
            const CandidateEvaluation missing_location_eval = evaluate_candidate_feature(generation_state, missing_location, AccessLevel::Curator);
            require(missing_location_eval.decision == CandidateDecision::Reject &&
                    std::any_of(missing_location_eval.validation_errors.begin(), missing_location_eval.validation_errors.end(), [](const std::string& error) {
                        return contains_substr(error, "missing location_found");
                    }),
                    "candidate with missing structured location should be rejected");
        }

        {
            CandidateFeature missing_entity;
            missing_entity.id = "candidate.test.missing_structured_entity";
            missing_entity.type = CandidateFeatureType::Artifact;
            missing_entity.description = "Structured test candidate with missing referenced entity.";
            CandidateArtifactMetadata metadata;
            metadata.true_creation_year = 650;
            metadata.claimed_creation_year = 650;
            metadata.discovery_year = 813;
            metadata.location_created = "site.reservoir_gate";
            metadata.location_found = "site.salt_cellar_archive";
            metadata.language_id = "language.lattice_dialect";
            metadata.dialect_id = "dialect.late_lock_hand";
            metadata.script_id = "script.green_seal";
            metadata.referenced_entity_ids = {"office.no_such_office"};
            missing_entity.structured_artifact_metadata = metadata;
            const CandidateEvaluation missing_entity_eval = evaluate_candidate_feature(generation_state, missing_entity, AccessLevel::Curator);
            require(missing_entity_eval.decision == CandidateDecision::Reject &&
                    std::any_of(missing_entity_eval.validation_errors.begin(), missing_entity_eval.validation_errors.end(), [](const std::string& error) {
                        return contains_substr(error, "references missing entity office.no_such_office");
                    }),
                    "candidate with missing structured referenced entity should be rejected");
        }

        {
            CandidateFeature early_green_unmediated;
            early_green_unmediated.id = "candidate.test.structured_early_green_unmediated";
            early_green_unmediated.type = CandidateFeatureType::Artifact;
            early_green_unmediated.description = "Structured metadata says the candidate is early; no mediation is declared.";
            CandidateArtifactMetadata metadata;
            metadata.true_creation_year = 660;
            metadata.claimed_creation_year = 553;
            metadata.discovery_year = 813;
            metadata.location_created = "site.salt_cellar_archive";
            metadata.location_found = "site.salt_cellar_archive";
            metadata.language_id = "language.lattice_dialect";
            metadata.dialect_id = "dialect.late_lock_hand";
            metadata.script_id = "script.green_seal";
            metadata.referenced_entity_ids = {"office.drowned_chancellor"};
            early_green_unmediated.structured_artifact_metadata = metadata;
            const CandidateEvaluation early_green_unmediated_eval = evaluate_candidate_feature(generation_state, early_green_unmediated, AccessLevel::Curator);
            require(early_green_unmediated_eval.decision == CandidateDecision::Reject &&
                    std::any_of(early_green_unmediated_eval.anachronisms.begin(), early_green_unmediated_eval.anachronisms.end(), [](const AnachronismReport& report) {
                        return report.status == AnachronismStatus::InvalidGenerationBug && contains_substr(report.referenced_item, "Green-Seal");
                    }),
                    "candidate with claimed Green-Seal before 612 and no structured mediation should be rejected");
        }

        {
            CandidateFeature prose_only_forgery;
            prose_only_forgery.id = "candidate.test.structured_prose_only_forgery";
            prose_only_forgery.type = CandidateFeatureType::Artifact;
            prose_only_forgery.description = "Structured metadata is invalid but this prose says forgery; typed metadata does not declare mediation.";
            CandidateArtifactMetadata metadata;
            metadata.true_creation_year = 660;
            metadata.claimed_creation_year = 553;
            metadata.discovery_year = 813;
            metadata.location_created = "site.salt_cellar_archive";
            metadata.location_found = "site.salt_cellar_archive";
            metadata.language_id = "language.lattice_dialect";
            metadata.dialect_id = "dialect.late_lock_hand";
            metadata.script_id = "script.green_seal";
            metadata.referenced_entity_ids = {"office.drowned_chancellor"};
            prose_only_forgery.structured_artifact_metadata = metadata;
            const CandidateEvaluation prose_only_forgery_eval = evaluate_candidate_feature(generation_state, prose_only_forgery, AccessLevel::Curator);
            require(prose_only_forgery_eval.decision == CandidateDecision::Reject &&
                    std::any_of(prose_only_forgery_eval.anachronisms.begin(), prose_only_forgery_eval.anachronisms.end(), [](const AnachronismReport& report) {
                        return report.status == AnachronismStatus::InvalidGenerationBug;
                    }),
                    "structured candidate must not receive forgery mediation from prose keywords alone");
        }

        {
            CandidateFeature early_green_forged;
            early_green_forged.id = "candidate.test.structured_early_green_forged";
            early_green_forged.type = CandidateFeatureType::Artifact;
            early_green_forged.description = "Structured metadata declares mediation without relying on keyword heuristics.";
            CandidateArtifactMetadata metadata;
            metadata.true_creation_year = 660;
            metadata.claimed_creation_year = 553;
            metadata.discovery_year = 813;
            metadata.location_created = "site.salt_cellar_archive";
            metadata.location_found = "site.salt_cellar_archive";
            metadata.language_id = "language.lattice_dialect";
            metadata.dialect_id = "dialect.late_lock_hand";
            metadata.script_id = "script.green_seal";
            metadata.referenced_entity_ids = {"office.drowned_chancellor"};
            metadata.declared_mediations = {EvidenceModifier::Forgery};
            early_green_forged.structured_artifact_metadata = metadata;
            const CandidateEvaluation early_green_forged_eval = evaluate_candidate_feature(generation_state, early_green_forged, AccessLevel::Curator);
            require(early_green_forged_eval.decision == CandidateDecision::AcceptAsForgery &&
                    std::all_of(early_green_forged_eval.anachronisms.begin(), early_green_forged_eval.anachronisms.end(), [](const AnachronismReport& report) {
                        return report.status == AnachronismStatus::ValidBecauseForged;
                    }),
                    "candidate with claimed Green-Seal before 612 and structured Forgery mediation should be accepted as forgery");
        }

        {
            CandidateFeature structured_precedence;
            structured_precedence.id = "candidate.test.structured_precedence";
            structured_precedence.type = CandidateFeatureType::Artifact;
            structured_precedence.description = "Vague draft note mentions claimed year 553 and Green-Seal, but structured metadata gives the authoritative valid date.";
            CandidateArtifactMetadata metadata;
            metadata.true_creation_year = 650;
            metadata.claimed_creation_year = 650;
            metadata.discovery_year = 813;
            metadata.location_created = "site.reservoir_gate";
            metadata.location_found = "site.salt_cellar_archive";
            metadata.language_id = "language.lattice_dialect";
            metadata.dialect_id = "dialect.late_lock_hand";
            metadata.script_id = "script.green_seal";
            metadata.referenced_entity_ids = {"office.drowned_chancellor"};
            structured_precedence.structured_artifact_metadata = metadata;
            const CandidateEvaluation structured_precedence_eval = evaluate_candidate_feature(generation_state, structured_precedence, AccessLevel::Curator);
            require(structured_precedence_eval.decision == CandidateDecision::Accept && structured_precedence_eval.anachronisms.empty(),
                    "structured metadata should take precedence over vague description text when validating generated candidates");
        }


        {
            CandidateFeature structured_valid = sample_candidate_feature("structured_lock_fragment");
            const CandidateEvaluation structured_valid_eval = evaluate_candidate_feature(generation_state, structured_valid, AccessLevel::Curator);
            require(structured_valid_eval.decision == CandidateDecision::Accept && structured_valid_eval.validation_errors.empty(),
                    "structured candidate with valid artifact metadata and one valid claim should evaluate as acceptable");
        }

        {
            CandidateFeature missing_claim_field = sample_candidate_feature("structured_lock_fragment");
            missing_claim_field.id = "candidate.test.structured_missing_claim_field";
            missing_claim_field.description = "Structured candidate with an intentionally incomplete claim.";
            missing_claim_field.structured_claims.front().subject.clear();
            const CandidateEvaluation missing_claim_eval = evaluate_candidate_feature(generation_state, missing_claim_field, AccessLevel::Curator);
            require(missing_claim_eval.decision == CandidateDecision::Reject &&
                    std::any_of(missing_claim_eval.validation_errors.begin(), missing_claim_eval.validation_errors.end(), [](const std::string& error) {
                        return contains_substr(error, "missing subject");
                    }),
                    "structured candidate claims with missing required fields should be rejected");
        }

        {
            CandidateFeature bad_claim_entity = sample_candidate_feature("structured_lock_fragment");
            bad_claim_entity.id = "candidate.test.structured_bad_claim_entity";
            bad_claim_entity.structured_claims.front().subject_entity_id = "person.missing";
            const CandidateEvaluation bad_claim_entity_eval = evaluate_candidate_feature(generation_state, bad_claim_entity, AccessLevel::Curator);
            require(bad_claim_entity_eval.decision == CandidateDecision::Reject &&
                    std::any_of(bad_claim_entity_eval.validation_errors.begin(), bad_claim_entity_eval.validation_errors.end(), [](const std::string& error) {
                        return contains_substr(error, "references missing entity person.missing");
                    }),
                    "structured candidate claims with invalid entity references should be rejected");
        }

        {
            CandidateFeature bad_confidence = sample_candidate_feature("structured_lock_fragment");
            bad_confidence.id = "candidate.test.structured_bad_confidence";
            bad_confidence.structured_claims.front().confidence = 1.25;
            const CandidateEvaluation bad_confidence_eval = evaluate_candidate_feature(generation_state, bad_confidence, AccessLevel::Curator);
            require(bad_confidence_eval.decision == CandidateDecision::Reject &&
                    std::any_of(bad_confidence_eval.validation_errors.begin(), bad_confidence_eval.validation_errors.end(), [](const std::string& error) {
                        return contains_substr(error, "confidence outside 0..1");
                    }),
                    "structured candidate claims with confidence outside 0..1 should be rejected");
        }

        {
            CandidateFeature unmediated_claim = sample_candidate_feature("structured_lock_fragment");
            unmediated_claim.id = "candidate.test.structured_unmediated_claim_entity";
            unmediated_claim.structured_artifact_metadata->true_creation_year = 553;
            unmediated_claim.structured_artifact_metadata->claimed_creation_year = 553;
            unmediated_claim.structured_artifact_metadata->dialect_id = "dialect.lower_lattice";
            unmediated_claim.structured_artifact_metadata->script_id = "script.pre_green_seal";
            unmediated_claim.structured_artifact_metadata->referenced_entity_ids = {"site.reservoir_gate"};
            unmediated_claim.structured_claims.front().subject_entity_id = "office.drowned_chancellor";
            unmediated_claim.structured_claims.front().claimed_year = 553;
            const CandidateEvaluation unmediated_claim_eval = evaluate_candidate_feature(generation_state, unmediated_claim, AccessLevel::Curator);
            require(unmediated_claim_eval.decision == CandidateDecision::Reject &&
                    std::any_of(unmediated_claim_eval.validation_errors.begin(), unmediated_claim_eval.validation_errors.end(), [](const std::string& error) {
                        return contains_substr(error, "outside valid range at claimed_year=553");
                    }),
                    "structured claim metadata should reject entity use before availability without typed mediation");
        }

        {
            CandidateFeature structured_claim_precedence = sample_candidate_feature("structured_lock_fragment");
            structured_claim_precedence.id = "candidate.test.structured_claim_precedence";
            structured_claim_precedence.description = "Vague prose says claimed year 553 and Drowned Chancellor before the office exists, but typed claim fields use valid year 620.";
            const CandidateEvaluation precedence_eval = evaluate_candidate_feature(generation_state, structured_claim_precedence, AccessLevel::Curator);
            require(precedence_eval.decision == CandidateDecision::Accept && precedence_eval.validation_errors.empty(),
                    "structured claim metadata should take precedence over prose description text");
        }

        {
            ArchiveEngineState structured_materialization_state = initialize_archive_engine(42);
            const CandidateFeature structured = sample_candidate_feature("structured_lock_fragment");
            const CandidateEvaluation evaluation = evaluate_candidate_feature(structured_materialization_state, structured, AccessLevel::Curator);
            const std::string before = serialize_for_replay_test(structured_materialization_state);
            const MaterializationResult public_result = materialize_candidate_feature(structured_materialization_state, structured, evaluation, AccessLevel::Public);
            require(!public_result.mutated && serialize_for_replay_test(structured_materialization_state) == before,
                    "public users should not materialize structured candidates");
            const MaterializationResult curator_result = materialize_candidate_feature(structured_materialization_state, structured, evaluation, AccessLevel::Curator);
            require(curator_result.mutated && !curator_result.inserted_artifact_ids.empty() && !curator_result.inserted_claim_ids.empty() &&
                    validate_full_state(structured_materialization_state).empty(),
                    "curator should materialize a valid structured candidate and leave the archive valid");
            require(contains_substr(format_claims(structured_materialization_state, AccessLevel::Public, kOpenEndedYear), curator_result.inserted_claim_ids.front()),
                    "materialized structured candidate claim should enter the public claim graph after discovery");
            require(contains_substr(format_mysteries(structured_materialization_state, AccessLevel::Public, kOpenEndedYear), curator_result.inserted_claim_ids.front()),
                    "materialized structured candidate should participate in linked mysteries after discovery");
        }

        const std::string public_generated = format_generated_candidates(generation_state, AccessLevel::Public, corroborating_request);
        const std::string scholar_generated = format_generated_candidates(generation_state, AccessLevel::Scholar, corroborating_request);
        const std::string curator_generated = format_generated_candidates(generation_state, AccessLevel::Curator, forgery_request);
        require(contains_substr(public_generated, "restricted to scholar/curator/canon/debug access") &&
                !contains_substr(public_generated, "candidate.generated"),
                "public candidate-generation output should not expose generated proposal internals");
        require(contains_substr(scholar_generated, "candidate.generated") &&
                contains_substr(scholar_generated, "candidate originality internals are restricted"),
                "scholar candidate-generation output should show candidate summaries through access-safe evaluation formatting");
        require(contains_substr(curator_generated, "Originality audit") && contains_substr(curator_generated, "Predicted anachronisms"),
                "curator candidate-generation output should include full evaluation details");

        ArchiveEngineState materialization_guard_state = initialize_archive_engine(42);
        const CandidateEvaluation generated_eval = evaluate_candidate_feature(materialization_guard_state, generated_a.front(), AccessLevel::Curator);
        const std::string before_generated_materialization = serialize_for_replay_test(materialization_guard_state);
        const MaterializationResult public_generated_materialization = materialize_candidate_feature(materialization_guard_state, generated_a.front(), generated_eval, AccessLevel::Public);
        require(!public_generated_materialization.mutated &&
                serialize_for_replay_test(materialization_guard_state) == before_generated_materialization,
                "generated candidate materialization should still require curator/canon/debug access and never auto-insert");

        {
            ArchiveEngineState generated_materialization_state = initialize_archive_engine(42);
            const CandidateGenerationRequest request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 42};
            const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(generated_materialization_state, request, 0U);
            require(maybe_candidate.has_value(), "generated candidate index 0 should resolve deterministically");
            const CandidateFeature candidate = *maybe_candidate;
            const CandidateEvaluation evaluation = evaluate_candidate_feature(generated_materialization_state, candidate, AccessLevel::Curator);
            const std::string before = serialize_for_replay_test(generated_materialization_state);
            const MaterializationResult public_result = materialize_candidate_feature(generated_materialization_state, candidate, evaluation, AccessLevel::Public);
            require(!public_result.mutated && serialize_for_replay_test(generated_materialization_state) == before,
                    "public users must not materialize generated candidates");
            const MaterializationResult scholar_result = materialize_candidate_feature(generated_materialization_state, candidate, evaluation, AccessLevel::Scholar);
            require(!scholar_result.mutated && serialize_for_replay_test(generated_materialization_state) == before,
                    "scholar users must not materialize generated candidates");

            const MaterializationResult curator_result = materialize_candidate_feature(generated_materialization_state, candidate, evaluation, AccessLevel::Curator);
            require(curator_result.mutated && !curator_result.inserted_artifact_ids.empty() && !curator_result.inserted_claim_ids.empty() &&
                    validate_full_state(generated_materialization_state).empty(),
                    "curator should materialize a generated corroborating candidate and leave the archive valid");
            const Artifact* inserted = generated_materialization_state.public_archive.find_artifact(curator_result.inserted_artifact_ids.front());
            require(inserted != nullptr, "generated materialization should insert an artifact with a deterministic ID");
            if (inserted != nullptr) {
                require(!contains_substr(format_artifacts(generated_materialization_state, AccessLevel::Public, inserted->discovery_year - 1), inserted->id) &&
                        contains_substr(format_artifacts(generated_materialization_state, AccessLevel::Public, inserted->discovery_year), inserted->id),
                        "generated materialized artifact should obey discovery-year visibility");
                require(contains_substr(format_mysteries(generated_materialization_state, AccessLevel::Public, inserted->discovery_year), curator_result.inserted_claim_ids.front()),
                        "generated materialized clue should affect mystery assessment after discovery");
            }

            const std::string after_success = serialize_for_replay_test(generated_materialization_state);
            const MaterializationResult duplicate_result = materialize_candidate_feature(generated_materialization_state, candidate, evaluation, AccessLevel::Curator);
            require(!duplicate_result.mutated && serialize_for_replay_test(generated_materialization_state) == after_success,
                    "failed duplicate generated materialization should roll back exactly");
        }

        {
            ArchiveEngineState silt_materialization_state = initialize_archive_engine(42);
            const std::string before = serialize_for_replay_test(silt_materialization_state);
            const CandidateGenerationRequest request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "silt_levy", 42};
            const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(silt_materialization_state, request, 0U);
            require(maybe_candidate.has_value(), "generated silt levy candidate should resolve deterministically");
            if (maybe_candidate.has_value()) {
                const CandidateEvaluation evaluation = evaluate_candidate_feature(silt_materialization_state, *maybe_candidate, AccessLevel::Curator);
                const MaterializationResult result = materialize_candidate_feature(silt_materialization_state, *maybe_candidate, evaluation, AccessLevel::Curator);
                require(result.mutated && !result.inserted_artifact_ids.empty() && validate_full_state(silt_materialization_state).empty(),
                        "curator should materialize a generated silt levy candidate and leave the archive valid");
                require(serialize_for_replay_test(silt_materialization_state) != before,
                        "successful generated silt levy materialization should explicitly mutate state");
                if (!result.inserted_artifact_ids.empty()) {
                    const std::string mystery_output = format_mysteries(silt_materialization_state, AccessLevel::Public, kOpenEndedYear);
                    require(!contains_substr(mystery_output, result.inserted_artifact_ids.front()),
                            "materialized silt_levy candidate should not become Third Lock Authority mystery evidence by default");
                }
            }
        }

        {
            ArchiveEngineState forgery_materialization_state = initialize_archive_engine(42);
            const CandidateGenerationRequest request{CandidateGenerationStrategy::AddMisleadingForgery, 553, "drowned_chancellor", 42};
            const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(forgery_materialization_state, request, 0U);
            require(maybe_candidate.has_value(), "generated misleading forgery candidate should resolve deterministically");
            const CandidateFeature candidate = *maybe_candidate;
            const CandidateEvaluation evaluation = evaluate_candidate_feature(forgery_materialization_state, candidate, AccessLevel::Curator);
            const MaterializationResult result = materialize_candidate_feature(forgery_materialization_state, candidate, evaluation, AccessLevel::Curator);
            require(result.mutated && !result.inserted_artifact_ids.empty() &&
                    validate_full_state(forgery_materialization_state).empty(),
                    "curator should materialize a generated misleading forgery with validation");
            if (!result.inserted_artifact_ids.empty()) {
                const std::string public_view = format_artifacts(forgery_materialization_state, AccessLevel::Public, kOpenEndedYear);
                const std::string curator_view = format_artifacts(forgery_materialization_state, AccessLevel::Curator, kOpenEndedYear);
                require(contains_substr(public_view, "royal decree, disputed") &&
                        contains_substr(curator_view, "forged decree"),
                        "generated materialized forgery should preserve public/curator access split");
            }
        }

        {
            ArchiveEngineState no_metadata_state = initialize_archive_engine(42);
            CandidateFeature generated_without_metadata;
            generated_without_metadata.id = "candidate.generated.corroborating_fragment.no_metadata";
            generated_without_metadata.type = CandidateFeatureType::Artifact;
            generated_without_metadata.description = "Generated-looking candidate without structured metadata.";
            const CandidateEvaluation evaluation = evaluate_candidate_feature(no_metadata_state, generated_without_metadata, AccessLevel::Curator);
            const std::string before = serialize_for_replay_test(no_metadata_state);
            const MaterializationResult result = materialize_candidate_feature(no_metadata_state, generated_without_metadata, evaluation, AccessLevel::Curator);
            require(!result.mutated && serialize_for_replay_test(no_metadata_state) == before,
                    "generated candidates without structured metadata should not materialize");
        }

        {
            ArchiveEngineState deterministic_a = initialize_archive_engine(42);
            ArchiveEngineState deterministic_b = initialize_archive_engine(42);
            const CandidateGenerationRequest request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 99};
            const std::optional<CandidateFeature> candidate_a = generated_candidate_at(deterministic_a, request, 1U);
            const std::optional<CandidateFeature> candidate_b = generated_candidate_at(deterministic_b, request, 1U);
            require(candidate_a.has_value() && candidate_b.has_value() && candidate_a->id == candidate_b->id,
                    "regenerating the same seed/request/index should produce the same candidate identity");
            if (candidate_a.has_value() && candidate_b.has_value()) {
                const CandidateEvaluation evaluation_a = evaluate_candidate_feature(deterministic_a, *candidate_a, AccessLevel::Curator);
                const CandidateEvaluation evaluation_b = evaluate_candidate_feature(deterministic_b, *candidate_b, AccessLevel::Curator);
                const MaterializationResult result_a = materialize_candidate_feature(deterministic_a, *candidate_a, evaluation_a, AccessLevel::Curator);
                const MaterializationResult result_b = materialize_candidate_feature(deterministic_b, *candidate_b, evaluation_b, AccessLevel::Curator);
                require(result_a.mutated && result_b.mutated &&
                        result_a.inserted_artifact_ids == result_b.inserted_artifact_ids &&
                        result_a.inserted_claim_ids == result_b.inserted_claim_ids,
                        "same generated seed/request/index should materialize the same artifact and claim IDs");
            }
        }

        {
            ArchiveEngineState cli_generated_state = initialize_archive_engine(42);
            const std::string public_output = format_generated_materialization_query(cli_generated_state, AccessLevel::Public,
                CandidateGenerationRequest{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 42}, 0U);
            require(contains_substr(public_output, "mutated: false") &&
                    contains_substr(public_output, "requires curator/canon/debug access"),
                    "public generated-materialization CLI output should reject without mutation");
            const std::string before = serialize_for_replay_test(cli_generated_state);
            const std::string curator_output = format_generated_materialization_query(cli_generated_state, AccessLevel::Curator,
                CandidateGenerationRequest{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 42}, 0U);
            require(contains_substr(curator_output, "mutated: true") &&
                    serialize_for_replay_test(cli_generated_state) != before,
                    "curator generated-materialization CLI output should explicitly mutate the local run state");
        }
    }

    {
        const CandidateFeature structured = sample_candidate_feature("structured_lock_fragment");
        const OriginalitySignal structured_signal = score_originality_feature(originality_feature_for_candidate(structured));
        const OriginalitySignal prose_only_signal = score_originality_feature(OriginalityFeature{
            "candidate.prose_only_structured_lock_fragment",
            originality_kind_for_candidate(structured.type),
            structured.description,
        });
        require(structured_signal.civilization_specificity_score > prose_only_signal.civilization_specificity_score &&
                structured_signal.civilization_specificity_score >= 0.30,
                "v15.1 originality input should use structured candidate links and claims, not only prose description");
        require(!structured_signal.required_local_dependencies.empty(),
                "structured candidate originality should discover local dependencies from typed links and claims");
    }

    {
        CandidateFeature local_generic;
        local_generic.id = "candidate.test_generic_prose_local_links";
        local_generic.type = CandidateFeatureType::Artifact;
        local_generic.description = "generic administrative fragment";
        local_generic.proposed_links = {"entity:office.drowned_chancellor", "entity:site.reservoir_gate", "mystery:mystery.third_lock_authority"};
        CandidateArtifactMetadata metadata;
        metadata.true_creation_year = 620;
        metadata.claimed_creation_year = 620;
        metadata.discovery_year = 813;
        metadata.location_created = "site.reservoir_gate";
        metadata.location_found = "site.salt_cellar_archive";
        metadata.language_id = "language.lattice_dialect";
        metadata.dialect_id = "dialect.upper_lattice";
        metadata.script_id = "script.green_seal";
        metadata.referenced_entity_ids = {"office.drowned_chancellor", "site.reservoir_gate", "script.green_seal"};
        local_generic.structured_artifact_metadata = metadata;
        local_generic.structured_claims.push_back(CandidateClaimMetadata{
            ClaimType::FactualClaim,
            PredicateType::Received,
            "Drowned Chancellor",
            "received",
            "lower lock authority",
            "Drowned Chancellor received lower lock authority at Reservoir Gate.",
            std::optional<std::string>{"office.drowned_chancellor"},
            std::optional<std::string>{"site.reservoir_gate"},
            std::optional<int>{620},
            0.55,
            AccessLevel::Public,
        });
        const OriginalitySignal local_signal = score_originality_feature(originality_feature_for_candidate(local_generic));
        const OriginalitySignal generic_signal = score_originality_feature(OriginalityFeature{
            "candidate.test_generic_prose_no_links",
            OriginalityFeatureKind::Artifact,
            "generic administrative fragment",
        });
        require(local_signal.civilization_specificity_score > generic_signal.civilization_specificity_score,
                "structured candidate with generic prose but local typed links should not be treated as equally low-specificity");
        require(generic_signal.civilization_specificity_score < 0.30,
                "generic prose with no local typed links should remain low-specificity");
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "silt_levy", 42};
        const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, 0U);
        require(maybe_candidate.has_value() && !maybe_candidate->structured_claims.empty(),
                "v16 generated corroborating candidates should carry structured claim metadata");
        if (maybe_candidate.has_value()) {
            const CandidateEvaluation evaluation = evaluate_candidate_feature(state, *maybe_candidate, AccessLevel::Curator);
            require(evaluation.decision == CandidateDecision::Accept,
                    "v16 generated structured corroborating candidate should still evaluate through the normal gate");
        }
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "silt_levy", 42};
        const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, 0U);
        require(maybe_candidate.has_value(), "v16 materialization test should regenerate a candidate");
        if (maybe_candidate.has_value()) {
            const CandidateEvaluation evaluation = evaluate_candidate_feature(state, *maybe_candidate, AccessLevel::Curator);
            const MaterializationResult result = materialize_candidate_feature(state, *maybe_candidate, evaluation, AccessLevel::Curator);
            require(result.mutated && !result.inserted_claim_ids.empty(),
                    "v16 generated structured candidate should materialize through curator path");
            if (!result.inserted_claim_ids.empty()) {
                const Claim* inserted_claim = state.public_archive.find_claim(result.inserted_claim_ids.front());
                require(inserted_claim != nullptr &&
                        contains_substr(inserted_claim->literal_content, "Generated structured levy fragment"),
                        "v16 generated materialization should use structured claim metadata instead of fallback claim prose");
            }
        }
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::AddRitualVariant, 700, "three_keepers", 42};
        const std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, 0U);
        require(maybe_candidate.has_value() && !maybe_candidate->structured_claims.empty() &&
                maybe_candidate->structured_claims.front().claim_type == ClaimType::MythicCompression,
                "v16 ritual variant generation should produce typed mythic-compression claim metadata");
    }


    {
        ArchiveEngineState state = initialize_archive_engine(42);
        CandidateGenerationRequest request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 42};
        std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, 0U);
        require(maybe_candidate.has_value(), "v16.1 answer/theory/mystery test should regenerate a candidate");
        if (maybe_candidate.has_value()) {
            CandidateFeature candidate = *maybe_candidate;
            require(!candidate.structured_claims.empty(), "v16.1 generated candidate should carry structured claims before materialization");
            if (!candidate.structured_claims.empty()) {
                candidate.structured_claims.front().confidence = 0.99;
            }
            const CandidateEvaluation evaluation = evaluate_candidate_feature(state, candidate, AccessLevel::Curator);
            const MaterializationResult result = materialize_candidate_feature(state, candidate, evaluation, AccessLevel::Curator);
            require(result.mutated && !result.inserted_claim_ids.empty(),
                    "v16.1 generated candidate should materialize for archive-year propagation tests");
            if (!result.inserted_claim_ids.empty()) {
                const std::string claim_id = result.inserted_claim_ids.front();
                const Claim* inserted_claim = state.public_archive.find_claim(claim_id);
                const Artifact* inserted_artifact = inserted_claim == nullptr ? nullptr : state.public_archive.find_artifact(inserted_claim->source_artifact_id);
                require(inserted_artifact != nullptr, "v16.1 propagation test should find the inserted artifact");
                const int visible_year = inserted_artifact == nullptr ? 813 : inserted_artifact->discovery_year;
                const int hidden_year = visible_year - 1;

                const std::string answer_before = answer_what_happened(state, AccessLevel::Public, hidden_year);
                const std::string answer_after = answer_what_happened(state, AccessLevel::Public, visible_year);
                require(!contains_substr(answer_before, claim_id) && contains_substr(answer_after, claim_id),
                        "v16.1 generated structured claims should affect public answers only after discovery year");

                const std::string theories_before = format_theories(state, AccessLevel::Scholar, hidden_year);
                const std::string theories_after = format_theories(state, AccessLevel::Scholar, visible_year);
                require(!contains_substr(theories_before, claim_id) && contains_substr(theories_after, claim_id),
                        "v16.1 generated structured claims should affect interpreter theories only after discovery year");

                const std::string mysteries_before = format_mysteries(state, AccessLevel::Public, hidden_year);
                const std::string mysteries_after = format_mysteries(state, AccessLevel::Public, visible_year);
                require(!contains_substr(mysteries_before, claim_id) && contains_substr(mysteries_after, claim_id),
                        "v16.1 generated structured claims should affect linked mystery assessments only after discovery year");
            }
        }
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        CandidateGenerationRequest request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 42};
        std::optional<CandidateFeature> maybe_candidate = generated_candidate_at(state, request, 0U);
        require(maybe_candidate.has_value(), "v16.1 contradiction test should regenerate a candidate");
        if (maybe_candidate.has_value()) {
            const CandidateEvaluation evaluation = evaluate_candidate_feature(state, *maybe_candidate, AccessLevel::Curator);
            const MaterializationResult result = materialize_candidate_feature(state, *maybe_candidate, evaluation, AccessLevel::Curator);
            const bool has_mythic_generated_contradiction = std::any_of(
                result.inserted_contradiction_ids.begin(),
                result.inserted_contradiction_ids.end(),
                [](const std::string& id) {
                    return contains_substr(id, "mythic_identity_compression") &&
                           contains_substr(id, "claim_materialized_candidate_generated");
                });
            require(has_mythic_generated_contradiction,
                    "v16.1 generated claim semantics should drive automatic contradiction detection after materialization");
        }
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const GeneratedCandidateBatch batch = generate_candidate_batch(state, request);
        require(batch.candidates.size() == 3,
                "v17 target dossier generation should produce a small mixed deterministic batch");
        const bool has_corrob = std::any_of(batch.candidates.begin(), batch.candidates.end(), [](const CandidateFeature& candidate) {
            return contains_substr(candidate.id, "corroborating_fragment");
        });
        const bool has_ritual = std::any_of(batch.candidates.begin(), batch.candidates.end(), [](const CandidateFeature& candidate) {
            return contains_substr(candidate.id, "ritual_variant");
        });
        const bool has_forgery = std::any_of(batch.candidates.begin(), batch.candidates.end(), [](const CandidateFeature& candidate) {
            return contains_substr(candidate.id, "misleading_forgery");
        });
        require(has_corrob && has_ritual && has_forgery,
                "v17 lock-authority dossier should mix corroborating, ritual, and misleading candidates");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "silt_levy", 42};
        const GeneratedCandidateBatch batch = generate_candidate_batch(state, request);
        require(!batch.candidates.empty(), "v17 silt-levy dossier should generate candidates for a resolved non-mystery topic");
        const bool any_lock_mystery_link = std::any_of(batch.candidates.begin(), batch.candidates.end(), [](const CandidateFeature& candidate) {
            return std::find(candidate.proposed_links.begin(), candidate.proposed_links.end(), "mystery:mystery.third_lock_authority") != candidate.proposed_links.end();
        });
        require(!any_lock_mystery_link,
                "v17 silt-levy dossier generation should not silently link to the Third Lock Authority mystery");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "silt_levy", 42};
        const GeneratedCandidateBatch batch = generate_candidate_batch(state, request);
        const auto multi_it = std::find_if(batch.candidates.begin(), batch.candidates.end(), [](const CandidateFeature& candidate) {
            return candidate.structured_claims.size() > 1U;
        });
        require(multi_it != batch.candidates.end(),
                "v17 target dossier should explicitly include a multi-claim structured candidate");
        if (multi_it != batch.candidates.end()) {
            const CandidateEvaluation evaluation = evaluate_candidate_feature(state, *multi_it, AccessLevel::Curator);
            const MaterializationResult result = materialize_candidate_feature(state, *multi_it, evaluation, AccessLevel::Curator);
            require(result.mutated && result.inserted_claim_ids.size() == multi_it->structured_claims.size(),
                    "v17 multi-claim generated candidate should materialize all structured claims under curator authority");
            const Artifact* inserted_artifact = result.inserted_artifact_ids.empty() ? nullptr : state.public_archive.find_artifact(result.inserted_artifact_ids.front());
            require(inserted_artifact != nullptr && inserted_artifact->mystery_links.empty(),
                    "v17 materialized silt-levy dossier candidate should not become lock-authority mystery evidence by default");
        }
    }


    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const DossierEvaluation evaluation = evaluate_generated_dossier(state, request, AccessLevel::Curator);
        const std::string after = serialize_for_replay_test(state);
        require(before == after,
                "v18 dossier-level evaluation should be non-mutating");
        require(evaluation.candidate_evaluations.size() == 3U &&
                evaluation.corroboration_pressure > 0.0 &&
                evaluation.ambiguity_pressure > 0.0 &&
                evaluation.forgery_pressure > 0.0,
                "v18 lock-authority dossier should report mixed corroboration, ambiguity, and forgery pressure");
        require(evaluation.mystery_resolution_pressure > 0.0 &&
                contains_substr(evaluation.assessment, "Balanced dossier") &&
                contains_substr(evaluation.assessment, "protected mystery"),
                "v18 lock-authority dossier should report protected mystery resolution pressure while remaining balanced");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::AddCorroboratingFragment, 620, "lock_authority", 42};
        const DossierEvaluation evaluation = evaluate_generated_dossier(state, request, AccessLevel::Curator);
        require(evaluation.corroboration_pressure >= 0.99 &&
                evaluation.ambiguity_pressure == 0.0 &&
                evaluation.forgery_pressure == 0.0 &&
                contains_substr(evaluation.assessment, "Over-confirmation pressure"),
                "v18 dossier with only corroborating fragments should warn about over-confirmation pressure");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "silt_levy", 42};
        const DossierEvaluation evaluation = evaluate_generated_dossier(state, request, AccessLevel::Curator);
        const std::string formatted = format_dossier_evaluation(state, AccessLevel::Curator, request);
        require(evaluation.resolved_target.has_value() && !evaluation.resolved_target->mystery_id.has_value() &&
                evaluation.mystery_resolution_pressure == 0.0 &&
                !contains_substr(formatted, "mystery.third_lock_authority"),
                "v18 silt-levy dossier should not report Third Lock Authority mystery pressure");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string public_dossier = format_dossier_evaluation(state, AccessLevel::Public, request);
        const std::string curator_dossier = format_dossier_evaluation(state, AccessLevel::Curator, request);
        require(!contains_substr(public_dossier, "Pressure scores") &&
                !contains_substr(public_dossier, "corroboration_pressure") &&
                !contains_substr(public_dossier, "Balanced dossier") &&
                contains_substr(public_dossier, "dossier meta-audit internals are restricted"),
                "v18 public dossier evaluation should not expose meta-audit pressure internals");
        require(contains_substr(curator_dossier, "Pressure scores") &&
                contains_substr(curator_dossier, "corroboration_pressure=") &&
                contains_substr(curator_dossier, "mystery_resolution_pressure=") &&
                contains_substr(curator_dossier, "Candidate decisions"),
                "v18 curator dossier evaluation should expose pressure scores and candidate decision rationale");
    }


    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string public_output = format_dossier_materialization_query(state, AccessLevel::Public, request, 1U);
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(public_output, "decision: Reject") &&
                contains_substr(public_output, "Materialization requires curator/canon/debug access") &&
                !contains_substr(public_output, "Dossier pressure summary before materialization"),
                "v19 public users should not materialize dossier candidates or see dossier pressure internals");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::size_t artifact_count_before = state.public_archive.artifacts().size();
        const std::string output = format_dossier_materialization_query(state, AccessLevel::Curator, request, 1U);
        const std::size_t artifact_count_after = state.public_archive.artifacts().size();
        require(artifact_count_after == artifact_count_before + 1U &&
                contains_substr(output, "Dossier pressure summary before materialization") &&
                contains_substr(output, "selected_candidate_role: ritual_variant") &&
                contains_substr(output, "projected_ambiguity_pressure=1.00") &&
                contains_substr(output, "mutated: true"),
                "v19 curator dossier workflow should show pressure context and materialize only the selected dossier candidate");
        const std::size_t materialized_count = static_cast<std::size_t>(std::count_if(
            state.public_archive.artifacts().begin(),
            state.public_archive.artifacts().end(),
            [](const auto& item) {
                return contains_substr(item.first, "artifact.materialized_candidate_generated_");
            }));
        require(materialized_count == 1U,
                "v19 dossier materialization should not auto-insert unselected dossier candidates");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "silt_levy", 42};
        const std::string output = format_dossier_materialization_query(state, AccessLevel::Curator, request, 0U);
        require(contains_substr(output, "Over-confirmation pressure") &&
                contains_substr(output, "selected_candidate_role: corroborating_fragment") &&
                contains_substr(output, "Warning: selected corroborating candidate comes from an over-confirming dossier"),
                "v19 materializing a corroborating candidate from an all-corroborating dossier should warn about over-confirmation");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        (void)format_dossier_materialization_query(state, AccessLevel::Curator, request, 1U);
        const std::string before_duplicate = serialize_for_replay_test(state);
        const std::string duplicate_output = format_dossier_materialization_query(state, AccessLevel::Curator, request, 1U);
        const std::string after_duplicate = serialize_for_replay_test(state);
        require(before_duplicate == after_duplicate &&
                contains_substr(duplicate_output, "Materialization rolled back"),
                "v19 failed duplicate dossier materialization should roll back without changing archive state");
    }


    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::optional<std::size_t> ritual_index = generated_candidate_index_for_role(state, request, GeneratedCandidateRole::RitualVariant);
        require(ritual_index.has_value(),
                "v19.1 role resolver should find a ritual variant in the lock-authority dossier");
        if (ritual_index.has_value()) {
            const std::string by_index = format_dossier_materialization_query(state, AccessLevel::Curator, request, *ritual_index);
            ArchiveEngineState role_state = initialize_archive_engine(42);
            const std::string by_role = format_dossier_materialization_query_by_role(role_state, AccessLevel::Curator, request, GeneratedCandidateRole::RitualVariant);
            require(contains_substr(by_role, "candidate_role: ritual_variant") &&
                    contains_substr(by_role, "resolved_candidate_index: " + std::to_string(*ritual_index)) &&
                    contains_substr(by_role, "selected_candidate_role: ritual_variant") &&
                    contains_substr(by_role, "mutated: true") &&
                    contains_substr(by_index, "mutated: true"),
                    "v19.1 role-based dossier materialization should resolve to the same selected role as index materialization");
        }
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string output = format_dossier_materialization_query_by_role(state, AccessLevel::Public, request, GeneratedCandidateRole::RitualVariant);
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(output, "candidate_role: ritual_variant") &&
                contains_substr(output, "decision: Reject") &&
                contains_substr(output, "Materialization requires curator/canon/debug access"),
                "v19.1 public role-based dossier materialization should reject without mutation");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "silt_levy", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string output = format_dossier_materialization_query_by_role(state, AccessLevel::Curator, request, GeneratedCandidateRole::RitualVariant);
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(output, "decision: Reject") &&
                contains_substr(output, "No generated dossier candidate matched the requested role"),
                "v19.1 unknown or absent dossier role should reject cleanly without mutation");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string output = format_dossier_selection_plan(state, AccessLevel::Curator, request, {0U, 1U});
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(output, "Dossier selection plan visible to curator") &&
                contains_substr(output, "Selected candidates:") &&
                contains_substr(output, "projected_corroboration_pressure=") &&
                contains_substr(output, "projected_ambiguity_pressure=") &&
                contains_substr(output, "recommendation:"),
                "v20 dossier selection planning should be non-mutating and expose curator pressure context");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::size_t artifact_count_before = state.public_archive.artifacts().size();
        const std::string output = format_dossier_selection_materialization_query(state, AccessLevel::Curator, request, {0U, 1U});
        const std::size_t artifact_count_after = state.public_archive.artifacts().size();
        require(artifact_count_after == artifact_count_before + 2U &&
                contains_substr(output, "Dossier selection materialization visible to curator") &&
                contains_substr(output, "decision: InsertArtifact") &&
                contains_substr(output, "mutated: true") &&
                contains_substr(output, "Inserted artifacts:") &&
                contains_substr(output, "Inserted claims:"),
                "v20 curator should materialize multiple selected dossier candidates as an explicit batch");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string output = format_dossier_selection_materialization_query(state, AccessLevel::Public, request, {0U, 1U});
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(output, "decision: Reject") &&
                contains_substr(output, "Materialization requires curator/canon/debug access") &&
                !contains_substr(output, "Dossier pressure summary before batch materialization"),
                "v20 public users should not materialize dossier selections or see batch pressure internals");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        (void)format_dossier_selection_materialization_query(state, AccessLevel::Curator, request, {0U, 1U});
        const std::string before_duplicate = serialize_for_replay_test(state);
        const std::string duplicate_output = format_dossier_selection_materialization_query(state, AccessLevel::Curator, request, {0U, 1U});
        const std::string after_duplicate = serialize_for_replay_test(state);
        require(before_duplicate == after_duplicate &&
                contains_substr(duplicate_output, "rollback: true"),
                "v20 failed duplicate dossier batch materialization should roll back the entire selected batch");
    }



    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string plan_output = format_dossier_selection_plan(state, AccessLevel::Curator, request, {0U, 0U});
        require(contains_substr(plan_output, "requested_indices: 0,0") &&
                contains_substr(plan_output, "selected_indices: 0") &&
                contains_substr(plan_output, "duplicate selected indices were ignored"),
                "v20.1 duplicate dossier selections should be reported transparently");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string output = format_dossier_selection_materialization_query(state, AccessLevel::Curator, request, {0U, 1U});
        require(contains_substr(output, "archive validation passed after batch insertion"),
                "v20.1 successful batch materialization should report final post-batch validation");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string output = format_dossier_selection_materialization_query(state, AccessLevel::Curator, request, {0U, static_cast<std::size_t>(kOpenEndedYear)});
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(output, "rollback: true") &&
                contains_substr(output, "entire selected batch was rolled back"),
                "v20.1 failed later batch candidate should roll back already-inserted selected candidates");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string output = format_hidden_proposals(state, AccessLevel::Curator, request);
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(output, "Hidden proposal gate visible to curator") &&
                contains_substr(output, "decision: AcceptableProposal") &&
                contains_substr(output, "archive_mutated: false"),
                "v21 hidden proposal generation/evaluation should be non-mutating and curator-visible");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string output = format_hidden_proposals(state, AccessLevel::Public, request);
        require(contains_substr(output, "hidden proposal internals are restricted") &&
                !contains_substr(output, "Lower Lock Authority Hearing") &&
                !contains_substr(output, "event.proposed"),
                "v21 public hidden proposal output should hide hidden-truth proposal internals");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "unknown_topic", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string output = format_hidden_proposals(state, AccessLevel::Curator, request);
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(output, "generated_count: 0") &&
                contains_substr(output, "no hidden proposals generated"),
                "v21 unresolved target topics should not silently generate hidden proposals");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        HiddenProposal invalid;
        invalid.id = "hidden_proposal.invalid.future_cause";
        invalid.type = HiddenProposalType::Event;
        invalid.target_topic = "test";
        invalid.description = "Invalid event whose cause has not ended yet.";
        invalid.proposed_event = Event{
            "event.proposed.invalid.future_cause",
            "invalid",
            "Invalid Future Cause Event",
            610,
            610,
            {"person.ivara"},
            {"event.salt_moon_schism"},
            {"site.reservoir_gate"},
            "This should fail because the Salt-Moon Schism ends after 610.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        };
        const HiddenProposalEvaluation evaluation = evaluate_hidden_proposal(state, invalid);
        require(evaluation.decision == HiddenProposalDecision::Reject &&
                std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                    return contains_substr(error, "occurs before cause event.salt_moon_schism ends");
                }),
                "v21 hidden event proposals should reject invalid hidden chronology before any mutation path exists");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::vector<HiddenProposal> first = generate_hidden_proposals(state, request);
        const std::vector<HiddenProposal> second = generate_hidden_proposals(state, request);
        require(first.size() == second.size() && !first.empty(),
                "v21.1 deterministic hidden proposal setup should generate a stable non-empty batch");
        bool same_ids = first.size() == second.size();
        for (std::size_t i = 0; i < first.size() && i < second.size(); ++i) {
            same_ids = same_ids && first[i].id == second[i].id;
        }
        require(same_ids,
                "v21.1 same hidden proposal request should produce identical wider proposal IDs");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        std::set<std::string> ids;
        bool collision = false;
        for (std::uint64_t seed = 1; seed <= 160; ++seed) {
            const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", seed};
            for (const HiddenProposal& proposal : generate_hidden_proposals(state, request)) {
                if (!ids.insert(proposal.id).second) {
                    collision = true;
                }
            }
        }
        require(!collision,
                "v21.1 wider deterministic hidden proposal IDs should not collide across a moderate seed sweep");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        HiddenProposal invalid;
        invalid.id = "hidden_proposal.invalid.future_created_by";
        invalid.type = HiddenProposalType::Entity;
        invalid.target_topic = "test";
        invalid.requested_target_year = 610;
        invalid.description = "Invalid entity created by an event that occurs after the entity starts.";
        invalid.proposed_entity = Entity{
            "office.proposed_future_created_by",
            EntityType::Office,
            "Future-Created Office",
            {},
            Interval{610, 760},
            "event.green_seal_standardized",
            "",
            AccessLevel::Canon,
        };
        const HiddenProposalEvaluation evaluation = evaluate_hidden_proposal(state, invalid);
        require(evaluation.decision == HiddenProposalDecision::Reject &&
                std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                    return contains_substr(error, "created_by_event_id event.green_seal_standardized occurs after entity start year 610");
                }),
                "v21.1 proposed entity provenance chronology should reject created_by events after entity start");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        HiddenProposal invalid;
        invalid.id = "hidden_proposal.invalid.self_cycle";
        invalid.type = HiddenProposalType::Event;
        invalid.target_topic = "test";
        invalid.requested_target_year = 620;
        invalid.description = "Invalid event that causes itself.";
        invalid.proposed_event = Event{
            "event.proposed.invalid.self_cycle",
            "invalid",
            "Self-Causing Proposed Event",
            620,
            620,
            {"person.ivara"},
            {"event.proposed.invalid.self_cycle"},
            {"site.reservoir_gate"},
            "This should fail only after simulated graph insertion can see the self-cycle.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        };
        const std::string before = serialize_for_replay_test(state);
        const HiddenProposalEvaluation evaluation = evaluate_hidden_proposal(state, invalid);
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                evaluation.decision == HiddenProposalDecision::Reject &&
                std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                    return contains_substr(error, "simulated hidden graph validation") &&
                           contains_substr(error, "causal cycle detected");
                }),
                "v21.1 simulated hidden graph validation should catch proposed causal cycles without mutating live state");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string public_output = format_hidden_proposals(state, AccessLevel::Public, request);
        require(contains_substr(public_output, "generated_count: restricted") &&
                !contains_substr(public_output, "generated_count: 2") &&
                !contains_substr(public_output, "hidden_proposal.") &&
                !contains_substr(public_output, "event.proposed"),
                "v21.1 public hidden proposal output should redact counts and hidden IDs");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string curator_output = format_hidden_proposals(state, AccessLevel::Curator, request);
        require(contains_substr(curator_output, "requested_target_year: 620") &&
                contains_substr(curator_output, "proposed_year: 619"),
                "v21.1 curator hidden proposal output should distinguish requested target year from proposed event/entity year");
    }



    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::vector<CandidateFeature> first = generate_candidate_features(state, request);
        const std::vector<CandidateFeature> second = generate_candidate_features(state, request);
        require(first.size() == second.size() && !first.empty() && first.front().id == second.front().id,
                "v21.2 same generated candidate request should produce stable wider candidate IDs");
        const CandidateGenerationRequest other_seed{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 43};
        const std::vector<CandidateFeature> different_seed = generate_candidate_features(state, other_seed);
        require(!different_seed.empty() && first.front().id != different_seed.front().id,
                "v21.2 different seed should change generated candidate IDs");
        require(contains_substr(first.front().id, "candidate.generated.") &&
                contains_substr(first.front().id, "lock_authority_620_") &&
                first.front().id.size() > std::string("candidate.generated.corroborating_fragment.574").size(),
                "v21.2 generated artifact candidate IDs should use wider deterministic digest format");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        std::set<std::string> ids;
        bool collision = false;
        for (std::uint64_t seed = 1; seed <= 200; ++seed) {
            const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", seed};
            for (const CandidateFeature& candidate : generate_candidate_features(state, request)) {
                if (!ids.insert(candidate.id).second) {
                    collision = true;
                }
            }
        }
        require(!collision, "v21.2 widened generated candidate IDs should not collide across a moderate seed sweep");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        CandidateFeature structured = sample_candidate_feature("structured_lock_fragment");
        structured.description = "Structured valid candidate prose says missing but typed fields are complete.";
        const CandidateEvaluation evaluation = evaluate_candidate_feature(state, structured, AccessLevel::Curator);
        require(evaluation.decision == CandidateDecision::Accept,
                "v21.2 structured candidates should not be rejected by legacy prose keyword triggers");

        CandidateFeature legacy = sample_candidate_feature("missing_metadata");
        const CandidateEvaluation legacy_evaluation = evaluate_candidate_feature(state, legacy, AccessLevel::Curator);
        require(legacy_evaluation.decision == CandidateDecision::Reject,
                "v21.2 legacy unstructured candidates should still use prose missing-metadata heuristics");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        CandidateFeature structured = sample_candidate_feature("structured_lock_fragment");
        structured.description = "Clean prose with no suspicious words.";
        if (structured.structured_claims.empty()) {
            structured.structured_claims.push_back(CandidateClaimMetadata{});
        }
        structured.structured_claims.front().object_entity_id = std::optional<std::string>{"entity.missing"};
        const CandidateEvaluation evaluation = evaluate_candidate_feature(state, structured, AccessLevel::Curator);
        require(evaluation.decision == CandidateDecision::Reject,
                "v21.2 structured invalid candidates should still be rejected from typed fields even when prose is clean");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        CandidateFeature structured;
        structured.id = "candidate.test.structured_prose_forgery_no_typed_mediation";
        structured.type = CandidateFeatureType::Artifact;
        structured.description = "Prose says forgery, but typed mediation is absent.";
        structured.proposed_links = {"entity:office.drowned_chancellor"};
        CandidateArtifactMetadata metadata;
        metadata.true_creation_year = 660;
        metadata.claimed_creation_year = 553;
        metadata.discovery_year = 820;
        metadata.location_created = "site.salt_cellar_archive";
        metadata.location_found = "site.salt_cellar_archive";
        metadata.language_id = "language.lattice_dialect";
        metadata.dialect_id = "dialect.late_lock_hand";
        metadata.script_id = "script.green_seal";
        metadata.referenced_entity_ids = {"office.drowned_chancellor"};
        structured.structured_artifact_metadata = metadata;
        structured.structured_claims.push_back(CandidateClaimMetadata{
            ClaimType::LegalFiction,
            PredicateType::CreatedOffice,
            "King Aru",
            "appointed",
            "Drowned Chancellor",
            "Typed claim references Drowned Chancellor too early without typed mediation.",
            std::optional<std::string>{"person.aru"},
            std::optional<std::string>{"office.drowned_chancellor"},
            std::optional<int>{553},
            0.20,
            AccessLevel::Public,
        });
        const CandidateEvaluation evaluation = evaluate_candidate_feature(state, structured, AccessLevel::Curator);
        require(evaluation.decision == CandidateDecision::Reject,
                "v21.2 structured candidates must not receive forgery mediation from prose keywords alone");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        HiddenProposal malformed;
        malformed.id = "hidden_proposal.invalid_type";
        malformed.type = static_cast<HiddenProposalType>(99);
        malformed.target_topic = "lock_authority";
        malformed.requested_target_year = 620;
        malformed.description = "Malformed proposal type from future ingestion.";
        const HiddenProposalEvaluation evaluation = evaluate_hidden_proposal(state, malformed);
        require(evaluation.decision == HiddenProposalDecision::Reject &&
                std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                    return contains_substr(error, "unknown hidden proposal type");
                }),
                "v21.2 invalid HiddenProposalType should reject cleanly");
        require(hidden_proposal_proposed_year_text(malformed) == "unavailable",
                "v21.2 malformed hidden proposals without payload should format proposed_year as unavailable");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        state.hidden_truth.add_event(Event{
            "event.baseline.invalid_order",
            "invalid_fixture",
            "Baseline Invalid Event",
            700,
            690,
            {},
            {},
            {},
            "Pre-existing baseline error for simulation diff test.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        });
        HiddenProposal valid;
        valid.id = "hidden_proposal.valid_on_invalid_baseline";
        valid.type = HiddenProposalType::Event;
        valid.target_topic = "lock_authority";
        valid.requested_target_year = 620;
        valid.description = "Valid proposal should not be blamed for pre-existing baseline graph errors.";
        valid.proposed_event = Event{
            "event.proposed.valid_on_invalid_baseline",
            "audit",
            "Valid Proposal Event",
            620,
            620,
            {"person.ivara"},
            {"event.green_seal_standardized"},
            {"site.reservoir_gate"},
            "Valid proposed event for baseline diff test.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        };
        const HiddenProposalEvaluation evaluation = evaluate_hidden_proposal(state, valid);
        require(evaluation.decision == HiddenProposalDecision::AcceptableProposal,
                "v21.2 simulated graph validation should diff baseline errors and not blame a valid proposal for them");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const CandidateGenerationRequest request{CandidateGenerationStrategy::BuildTargetDossier, 620, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string public_plan = format_hidden_proposal_migration_plan(state, AccessLevel::Public, request, 0U);
        const std::string curator_plan = format_hidden_proposal_migration_plan(state, AccessLevel::Curator, request, 0U);
        const std::string after = serialize_for_replay_test(state);
        require(before == after &&
                contains_substr(public_plan, "hidden proposal migration internals are restricted") &&
                contains_substr(public_plan, "would_mutate_hidden_truth: false") &&
                !contains_substr(public_plan, "proposal_id:") &&
                contains_substr(curator_plan, "Hidden proposal migration plan visible to curator") &&
                contains_substr(curator_plan, "proposal_decision: AcceptableProposal") &&
                contains_substr(curator_plan, "Projected events:") &&
                contains_substr(curator_plan, "Affected mysteries:") &&
                contains_substr(curator_plan, "Predicted public archive effects:"),
                "v22 hidden proposal migration planning should be non-mutating, redacted publicly, and detailed for curator access");
    }



    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster first = generate_hidden_timeline_cluster(state, request);
        const GeneratedHiddenTimelineCluster second = generate_hidden_timeline_cluster(state, request);
        require(!first.proposed_events.empty() && !first.proposed_entities.empty(),
                "v23 lock-authority hidden cluster should generate proposed entities and events");
        const bool has_typed_links = !first.causal_links.empty() &&
            std::all_of(first.causal_links.begin(), first.causal_links.end(), [](const ProposedCausalLink& link) {
                return !link.cause_event_id.empty() && !link.effect_event_id.empty();
            });
        require(has_typed_links,
                "v24.2 generated hidden clusters should populate typed causal links with non-empty cause and effect IDs");
        require(first.proposed_events.size() == second.proposed_events.size() &&
                first.proposed_entities.size() == second.proposed_entities.size() &&
                first.proposed_events.front().id == second.proposed_events.front().id &&
                first.proposed_entities.front().id == second.proposed_entities.front().id,
                "v23 same hidden cluster request should generate deterministic proposed IDs");

        const HiddenTimelineClusterRequest different_seed{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 43};
        const GeneratedHiddenTimelineCluster third = generate_hidden_timeline_cluster(state, different_seed);
        require(!third.proposed_events.empty() && first.proposed_events.front().id != third.proposed_events.front().id,
                "v23 different hidden cluster seed should change proposed IDs or details");
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest unknown{HiddenClusterScope::InstitutionOrigin, "unknown_topic", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, unknown);
        const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
        require(cluster.proposed_events.empty() && evaluation.decision == HiddenClusterDecision::Reject,
                "v23 unknown hidden cluster target should fail cleanly without fallback");
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const std::string before = serialize_for_replay_test(state);
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
        const std::string after = serialize_for_replay_test(state);
        require(before == after && evaluation.simulated_on_copy && !evaluation.would_mutate_hidden_truth,
                "v23 hidden cluster generation/evaluation should simulate on a copy and not mutate live state");
        require(evaluation.decision == HiddenClusterDecision::NeedsCuratorReview &&
                std::find(evaluation.affected_mystery_ids.begin(), evaluation.affected_mystery_ids.end(), "mystery.third_lock_authority") != evaluation.affected_mystery_ids.end(),
                "v23 lock-authority cluster should be structurally valid but require curator review due to protected mystery pressure");
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const std::string public_output = format_hidden_timeline_cluster(state, AccessLevel::Public, request);
        const std::string curator_output = format_hidden_timeline_cluster(state, AccessLevel::Curator, request);
        require(contains_substr(public_output, "hidden cluster internals are restricted") &&
                !contains_substr(public_output, "event.generated") &&
                !contains_substr(public_output, "faction.generated") &&
                contains_substr(curator_output, "Proposed entities:") &&
                contains_substr(curator_output, "Proposed events:") &&
                contains_substr(curator_output, "Causal links:") &&
                contains_substr(curator_output, " -> ") &&
                contains_substr(curator_output, "would_mutate_hidden_truth: false"),
                "v23/v24.2 hidden cluster formatting should redact public internals and expose readable curator causal links");
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        require(!cluster.proposed_events.empty(), "v24.2 malformed typed causal-link test setup should have proposed events");
        if (!cluster.proposed_events.empty()) {
            cluster.causal_links.push_back(ProposedCausalLink{"event.generated.missing_cause", cluster.proposed_events.front().id, "missing cause regression"});
            const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
            const bool saw_missing_cause = std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                return contains_substr(error, "causal link references missing cause event.generated.missing_cause");
            });
            require(evaluation.decision == HiddenClusterDecision::Reject && saw_missing_cause,
                    "v24.2 hidden cluster evaluation should reject typed causal links with missing causes");
        }
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        require(!cluster.proposed_events.empty(), "v24.2 missing-effect typed causal-link test setup should have proposed events");
        if (!cluster.proposed_events.empty()) {
            cluster.causal_links.push_back(ProposedCausalLink{cluster.proposed_events.front().id, "event.generated.missing_effect", "missing effect regression"});
            const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
            const bool saw_missing_effect = std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                return contains_substr(error, "causal link references missing effect event.generated.missing_effect");
            });
            require(evaluation.decision == HiddenClusterDecision::Reject && saw_missing_effect,
                    "v24.2 hidden cluster evaluation should reject typed causal links with missing effects");
        }
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        require(cluster.proposed_events.size() >= 3U, "v24.2 unmirrored typed causal-link test setup should have three events");
        if (cluster.proposed_events.size() >= 3U) {
            cluster.causal_links.push_back(ProposedCausalLink{cluster.proposed_events.front().id, cluster.proposed_events[2].id, "unmirrored link regression"});
            const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
            const bool saw_unmirrored_link = std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                return contains_substr(error, "is not mirrored by effect cause_event_ids");
            });
            require(evaluation.decision == HiddenClusterDecision::Reject && saw_unmirrored_link,
                    "v24.2 hidden cluster evaluation should reject display causal links not mirrored by Event::cause_event_ids");
        }
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        require(cluster.proposed_events.size() >= 3U, "v24.2 typed causal-link temporal test setup should have three events");
        if (cluster.proposed_events.size() >= 3U) {
            cluster.proposed_events[1].cause_event_ids.push_back(cluster.proposed_events[2].id);
            cluster.causal_links.push_back(ProposedCausalLink{cluster.proposed_events[2].id, cluster.proposed_events[1].id, "reverse chronology regression"});
            const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
            const bool saw_typed_temporal_error = std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                return contains_substr(error, "causal link cause") && contains_substr(error, "ends after effect");
            });
            require(evaluation.decision == HiddenClusterDecision::Reject && saw_typed_temporal_error,
                    "v24.2 hidden cluster evaluation should reject typed causal links whose cause ends after the effect starts");
        }
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        require(cluster.proposed_events.size() >= 3U, "v23 cluster chronology test setup should have three proposed events");
        if (cluster.proposed_events.size() >= 3U) {
            cluster.proposed_events[1].cause_event_ids.push_back(cluster.proposed_events[2].id);
            const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
            const bool saw_cause_order = std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                return contains_substr(error, "occurs before cause");
            });
            require(evaluation.decision == HiddenClusterDecision::Reject && saw_cause_order,
                    "v23 hidden cluster should reject cause-after-effect chronology errors");
        }
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        require(!cluster.proposed_events.empty(), "v23 participant availability test setup should have proposed events");
        if (!cluster.proposed_events.empty()) {
            cluster.proposed_events.front().participant_entity_ids.push_back("person.aru");
            const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
            const bool saw_unavailable_participant = std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
                return contains_substr(error, "participant person.aru does not exist during proposed event");
            });
            require(evaluation.decision == HiddenClusterDecision::Reject && saw_unavailable_participant,
                    "v23 hidden cluster should reject participants unavailable at event year");
        }
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        GeneratedHiddenTimelineCluster cluster;
        cluster.request = HiddenTimelineClusterRequest{HiddenClusterScope::InstitutionOrigin, "lock_authority", 620, 620, 42};
        cluster.resolved_target = resolve_generation_target(state, "lock_authority");
        cluster.rationale = "cycle regression fixture";
        cluster.proposed_events.push_back(Event{
            "event.generated.cycle_a",
            "cycle_test",
            "Generated Cycle A",
            620,
            620,
            {"person.ivara"},
            {"event.generated.cycle_b"},
            {"site.reservoir_gate"},
            "Cycle A.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        });
        cluster.proposed_events.push_back(Event{
            "event.generated.cycle_b",
            "cycle_test",
            "Generated Cycle B",
            620,
            620,
            {"person.ivara"},
            {"event.generated.cycle_a"},
            {"site.reservoir_gate"},
            "Cycle B.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        });
        const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
        const bool saw_cycle = std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
            return contains_substr(error, "causal cycle detected");
        });
        require(evaluation.decision == HiddenClusterDecision::Reject && saw_cycle,
                "v23 combined proposed causal graph should reject same-year cycles during copied-graph simulation");
    }

    {
        const ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
        const bool missing_same_cluster_dependency = std::any_of(evaluation.validation_errors.begin(), evaluation.validation_errors.end(), [](const std::string& error) {
            return contains_substr(error, "missing participant faction.generated_lower_lock_keepers") ||
                   contains_substr(error, "missing required entity office.generated_mouth_counters");
        });
        require(!missing_same_cluster_dependency && evaluation.decision != HiddenClusterDecision::Reject,
                "v23 hidden cluster should allow proposed entities to be used by proposed events in the same cluster");
    }



    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        const std::string before = serialize_for_replay_test(state);
        const HiddenClusterMaterializationResult public_result = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Public);
        const HiddenClusterMaterializationResult scholar_result = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Scholar);
        const std::string after = serialize_for_replay_test(state);
        require(!public_result.mutated && !scholar_result.mutated && before == after &&
                contains_substr(public_result.explanation, "requires curator/canon/debug access"),
                "v24 public/scholar users should not materialize hidden clusters or mutate hidden truth");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        const HiddenClusterMaterializationResult result = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        require(result.mutated && result.source_decision == HiddenClusterDecision::NeedsCuratorReview &&
                result.inserted_entity_ids.size() == cluster.proposed_entities.size() &&
                result.inserted_event_ids.size() == cluster.proposed_events.size() &&
                state.hidden_truth.find_entity(result.inserted_entity_ids.front()) != nullptr &&
                state.hidden_truth.find_event(result.inserted_event_ids.front()) != nullptr &&
                state.hidden_truth_mutations.size() == 1U &&
                state.hidden_truth_mutations.front().id == result.mutation_record_id &&
                state.hidden_truth_mutations.front().source_decision == HiddenClusterDecision::NeedsCuratorReview &&
                state.hidden_truth_mutations.front().inserted_entity_ids == result.inserted_entity_ids &&
                state.hidden_truth_mutations.front().inserted_event_ids == result.inserted_event_ids &&
                state.hidden_truth_mutations.front().target_topic == "lock_authority" &&
                state.hidden_truth_mutations.front().cluster_scope == "institution_origin" &&
                state.hidden_truth_mutations.front().start_year == 590 &&
                state.hidden_truth_mutations.front().end_year == 625 &&
                state.hidden_truth_mutations.front().seed == 42U &&
                state.hidden_truth_mutations.front().authorized_access_level == "curator" &&
                state.hidden_truth_mutations.front().algorithm_version == "v24.1.hidden_mutation_audit" &&
                contains_substr(serialize_for_replay_test(state), "M|" + result.mutation_record_id) &&
                validate_full_state(state).empty(),
                "v24.1 curator materialization should create one complete hidden mutation audit record and preserve full validation");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "unknown_topic", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        const std::string before = serialize_for_replay_test(state);
        const HiddenClusterMaterializationResult result = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const std::string after = serialize_for_replay_test(state);
        require(!result.mutated && result.source_decision == HiddenClusterDecision::Reject && before == after &&
                state.hidden_truth_mutations.empty(),
                "v24.1 rejected hidden clusters should not materialize, mutate hidden truth, or create false audit records");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        const HiddenClusterMaterializationResult first = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const std::string after_first = serialize_for_replay_test(state);
        const HiddenClusterMaterializationResult second = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const std::string after_second = serialize_for_replay_test(state);
        require(first.mutated && !second.mutated && after_first == after_second &&
                !second.validation_errors.empty() &&
                state.hidden_truth_mutations.size() == 1U &&
                state.hidden_truth_mutations.front().id == first.mutation_record_id,
                "v24.1 duplicate deterministic hidden cluster materialization should fail, roll back, and create no second audit record");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
        const HiddenClusterMaterializationResult result = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const std::string public_audit = format_hidden_truth_mutations(state, AccessLevel::Public);
        const std::string curator_audit = format_hidden_truth_mutations(state, AccessLevel::Curator);
        require(result.mutated &&
                contains_substr(public_audit, "hidden mutation audit internals are restricted") &&
                !contains_substr(public_audit, result.mutation_record_id) &&
                !contains_substr(public_audit, "event.generated") &&
                contains_substr(curator_audit, result.mutation_record_id) &&
                contains_substr(curator_audit, "source_cluster_id:") &&
                contains_substr(curator_audit, "Inserted entities:") &&
                contains_substr(curator_audit, "Inserted events:"),
                "v24.1 mutation audit formatting should redact public internals and expose curator-reviewable details");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const std::string public_output = format_hidden_cluster_materialization_query(state, AccessLevel::Public, request);
        require(contains_substr(public_output, "mutated: false") &&
                contains_substr(public_output, "hidden cluster materialization internals are restricted") &&
                !contains_substr(public_output, "Inserted entities:") &&
                !contains_substr(public_output, "event.generated") &&
                !contains_substr(public_output, "faction.generated"),
                "v24 public hidden-cluster materialization output should remain access-redacted");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
        const HiddenClusterMaterializationResult materialization = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const std::string before_generation = serialize_for_replay_test(state);
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, state.hidden_truth_mutations.front(), candidate_request);
        const std::string after_generation = serialize_for_replay_test(state);
        const bool all_have_source = std::all_of(batch.candidates.begin(), batch.candidates.end(), [&](const CandidateFeature& candidate) {
            return candidate.hidden_mutation_source.has_value() &&
                   candidate.hidden_mutation_source->mutation_record_id == materialization.mutation_record_id &&
                   state.public_archive.find_artifact(candidate.id) == nullptr;
        });
        const bool all_evaluate = std::all_of(batch.candidates.begin(), batch.candidates.end(), [&](const CandidateFeature& candidate) {
            const CandidateEvaluation evaluation = evaluate_candidate_feature(state, candidate, AccessLevel::Curator);
            return evaluation.decision != CandidateDecision::Reject;
        });
        require(materialization.mutated && batch.candidates.size() == 3U && before_generation == after_generation &&
                all_have_source && all_evaluate,
                "v25.1 hidden-mutation artifact generation should create three non-mutating candidates tied to mutation provenance and reuse evaluation gates");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
        const HiddenClusterMaterializationResult materialization = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const std::string public_output = format_candidates_from_hidden_mutation(state, AccessLevel::Public, state.hidden_truth_mutations.front(), candidate_request);
        const std::string curator_output = format_candidates_from_hidden_mutation(state, AccessLevel::Curator, state.hidden_truth_mutations.front(), candidate_request);
        require(materialization.mutated &&
                contains_substr(public_output, "Hidden-mutation artifact candidates visible to public") &&
                contains_substr(public_output, "hidden mutation provenance is restricted") &&
                contains_substr(public_output, "generated_count: 3") &&
                contains_substr(public_output, "Public source summary:") &&
                contains_substr(public_output, "administrative docket") &&
                contains_substr(public_output, "ritual notice") &&
                contains_substr(public_output, "later scholar fragment") &&
                !contains_substr(public_output, materialization.mutation_record_id) &&
                !contains_substr(public_output, "mutation.hidden_truth") &&
                !contains_substr(public_output, "hidden_cluster.") &&
                !contains_substr(public_output, "event.generated") &&
                !contains_substr(public_output, "faction.generated") &&
                !contains_substr(public_output, "office.generated") &&
                !contains_substr(public_output, "source_events:") &&
                !contains_substr(public_output, "source_entities:") &&
                contains_substr(curator_output, "Hidden mutation source trace:") &&
                contains_substr(curator_output, materialization.mutation_record_id) &&
                contains_substr(curator_output, "source_cluster_id:") &&
                contains_substr(curator_output, "source_entities:") &&
                contains_substr(curator_output, "source_events:"),
                "v25.1 hidden-mutation artifact candidate formatting should show public summaries without hidden IDs and show curator provenance trace");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
        const HiddenClusterMaterializationResult materialization = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        HiddenTruthMutationRecord bad_record = state.hidden_truth_mutations.front();
        bad_record.inserted_event_ids.push_back("event.generated.missing_source");
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const std::vector<std::string> source_errors = validate_hidden_mutation_artifact_source(state, bad_record);
        const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, bad_record, candidate_request);
        require(materialization.mutated && !source_errors.empty() && batch.candidates.empty(),
                "v25.1 hidden-mutation artifact source validation should reject dangling inserted event provenance and generate no candidates");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const std::string before = serialize_for_replay_test(state);
        const std::string public_query = format_hidden_mutation_artifact_generation_query(state, AccessLevel::Public, cluster_request, candidate_request);
        const std::string after = serialize_for_replay_test(state);
        require(before == after && contains_substr(public_query, "hidden_materialization_mutated: false") &&
                contains_substr(public_query, "generated_count: 0") &&
                contains_substr(public_query, "hidden mutation internals are restricted") &&
                state.hidden_truth_mutations.empty(),
                "v25.1 public CLI-style hidden-mutation artifact generation should not bypass hidden materialization access gates");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const std::string output = format_hidden_mutation_artifact_generation_query(state, AccessLevel::Curator, cluster_request, candidate_request);
        require(contains_substr(output, "hidden_materialization_mutated: true") &&
                contains_substr(output, "archive_artifacts_inserted: false") &&
                contains_substr(output, "Candidate evaluation visible to curator") &&
                contains_substr(output, "Hidden mutation source trace:") &&
                state.hidden_truth_mutations.size() == 1U &&
                state.public_archive.find_artifact("candidate.hidden_mutation_artifact.lock_authority_625") == nullptr,
                "v25.1 curator CLI-style workflow should materialize hidden truth in-memory, emit evaluated candidates, and not insert public artifacts");
    }

    {
        ArchiveEngineState state_a = initialize_archive_engine(42);
        ArchiveEngineState state_b = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const GeneratedHiddenTimelineCluster cluster_a = generate_hidden_timeline_cluster(state_a, cluster_request);
        const GeneratedHiddenTimelineCluster cluster_b = generate_hidden_timeline_cluster(state_b, cluster_request);
        const HiddenClusterMaterializationResult materialization_a = materialize_hidden_timeline_cluster(state_a, cluster_a, AccessLevel::Curator);
        const HiddenClusterMaterializationResult materialization_b = materialize_hidden_timeline_cluster(state_b, cluster_b, AccessLevel::Curator);
        const GeneratedCandidateBatch batch_a = generate_candidates_from_hidden_mutation(state_a, state_a.hidden_truth_mutations.front(), candidate_request);
        const GeneratedCandidateBatch batch_b = generate_candidates_from_hidden_mutation(state_b, state_b.hidden_truth_mutations.front(), candidate_request);
        const bool deterministic_ids = batch_a.candidates.size() == batch_b.candidates.size() &&
            std::equal(batch_a.candidates.begin(), batch_a.candidates.end(), batch_b.candidates.begin(), [](const CandidateFeature& lhs, const CandidateFeature& rhs) {
                return lhs.id == rhs.id;
            });
        const bool shape_ids_distinct = batch_a.candidates.size() == 3U &&
            batch_a.candidates[0].id != batch_a.candidates[1].id &&
            batch_a.candidates[1].id != batch_a.candidates[2].id &&
            batch_a.candidates[0].id != batch_a.candidates[2].id &&
            contains_substr(batch_a.candidates[0].id, "admin_docket") &&
            contains_substr(batch_a.candidates[1].id, "ritual_notice") &&
            contains_substr(batch_a.candidates[2].id, "scholar_fragment");
        require(materialization_a.mutated && materialization_b.mutated && deterministic_ids && shape_ids_distinct,
                "v25.1 hidden-mutation artifact candidate IDs should be deterministic and shape-distinct for the same mutation/request seed");
    }


    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
        const HiddenClusterMaterializationResult hidden = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, state.hidden_truth_mutations.front(), candidate_request);
        std::vector<MaterializationResult> results;
        for (const CandidateFeature& candidate : batch.candidates) {
            results.push_back(materialize_hidden_mutation_artifact_candidate(state, candidate, AccessLevel::Curator));
        }
        const bool all_mutated = results.size() == 3U && std::all_of(results.begin(), results.end(), [](const MaterializationResult& result) {
            return result.mutated && result.inserted_artifact_ids.size() == 1U && !result.inserted_claim_ids.empty();
        });
        const bool all_have_provenance = std::all_of(results.begin(), results.end(), [&](const MaterializationResult& result) {
            const Artifact* artifact = state.public_archive.find_artifact(result.inserted_artifact_ids.front());
            return artifact != nullptr && artifact->hidden_mutation_artifact_provenance.has_value() &&
                   artifact->hidden_mutation_artifact_provenance->mutation_record_id == hidden.mutation_record_id &&
                   validate_materialized_hidden_mutation_artifact_provenance(state, *artifact).empty();
        });
        require(hidden.mutated && batch.candidates.size() == 3U && all_mutated && all_have_provenance && validate_full_state(state).empty(),
                "v26 curator should explicitly materialize each hidden-mutation candidate shape as one public artifact with valid hidden provenance");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
        const HiddenClusterMaterializationResult hidden = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, state.hidden_truth_mutations.front(), candidate_request);
        const std::string before = serialize_for_replay_test(state);
        const MaterializationResult public_result = materialize_hidden_mutation_artifact_candidate(state, batch.candidates.front(), AccessLevel::Public);
        const MaterializationResult scholar_result = materialize_hidden_mutation_artifact_candidate(state, batch.candidates.front(), AccessLevel::Scholar);
        const std::string after = serialize_for_replay_test(state);
        require(hidden.mutated && !public_result.mutated && !scholar_result.mutated && before == after &&
                contains_substr(public_result.explanation, "requires curator/canon/debug access"),
                "v26 public/scholar users should not materialize hidden-mutation artifact candidates or mutate public archive");
    }

    {
        const std::array<AccessLevel, 3U> allowed_accesses = {AccessLevel::Curator, AccessLevel::Canon, AccessLevel::Debug};
        for (AccessLevel access : allowed_accesses) {
            ArchiveEngineState state = initialize_archive_engine(42);
            const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
            const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
            const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
            const HiddenClusterMaterializationResult hidden = materialize_hidden_timeline_cluster(state, cluster, access);
            const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, state.hidden_truth_mutations.front(), candidate_request);
            const MaterializationResult result = materialize_hidden_mutation_artifact_candidate(state, batch.candidates.front(), access);
            require(hidden.mutated && result.mutated && validate_full_state(state).empty(),
                    "v26.5 curator/canon/debug access should all pass materialization gates when quality and validation pass");
        }
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
        const HiddenClusterMaterializationResult hidden = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, state.hidden_truth_mutations.front(), candidate_request);
        CandidateFeature missing_source = batch.candidates.front();
        missing_source.hidden_mutation_source = std::nullopt;
        CandidateFeature invalid_source = batch.candidates.front();
        invalid_source.hidden_mutation_source->source_event_ids.push_back("event.generated.missing_public_materialization_source");
        CandidateFeature rejected_candidate = batch.candidates.front();
        rejected_candidate.structured_claims.clear();
        const std::string before = serialize_for_replay_test(state);
        const MaterializationResult missing_result = materialize_hidden_mutation_artifact_candidate(state, missing_source, AccessLevel::Curator);
        const MaterializationResult invalid_result = materialize_hidden_mutation_artifact_candidate(state, invalid_source, AccessLevel::Curator);
        const MaterializationResult rejected_result = materialize_hidden_mutation_artifact_candidate(state, rejected_candidate, AccessLevel::Curator);
        const std::string after = serialize_for_replay_test(state);
        require(hidden.mutated && before == after && !missing_result.mutated && !invalid_result.mutated && !rejected_result.mutated &&
                contains_substr(missing_result.explanation, "not linked") &&
                contains_substr(invalid_result.explanation, "source validation failed") &&
                contains_substr(rejected_result.explanation, "lacks hidden-mutation materialization payload"),
                "v26 hidden-mutation candidate materialization should reject missing provenance, dangling source references, and missing payloads without mutation");
    }

    {
        ArchiveEngineState state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, cluster_request);
        const HiddenClusterMaterializationResult hidden = materialize_hidden_timeline_cluster(state, cluster, AccessLevel::Curator);
        const GeneratedCandidateBatch batch = generate_candidates_from_hidden_mutation(state, state.hidden_truth_mutations.front(), candidate_request);
        const MaterializationResult first = materialize_hidden_mutation_artifact_candidate(state, batch.candidates.front(), AccessLevel::Curator);
        const std::string after_first = serialize_for_replay_test(state);
        const MaterializationResult second = materialize_hidden_mutation_artifact_candidate(state, batch.candidates.front(), AccessLevel::Curator);
        const std::string after_second = serialize_for_replay_test(state);
        const std::string public_output = format_hidden_mutation_artifact_materialization_result(first, batch.candidates.front(), AccessLevel::Public);
        const std::string curator_output = format_hidden_mutation_artifact_materialization_result(first, batch.candidates.front(), AccessLevel::Curator);
        require(hidden.mutated && first.mutated && !second.mutated && after_first == after_second &&
                contains_substr(second.explanation, "already materialized") &&
                contains_substr(public_output, "hidden provenance: restricted") &&
                !contains_substr(public_output, hidden.mutation_record_id) &&
                !contains_substr(public_output, "mutation.hidden_truth") &&
                !contains_substr(public_output, "hidden_cluster.") &&
                !contains_substr(public_output, "event.generated") &&
                !contains_substr(public_output, "faction.generated") &&
                !contains_substr(public_output, "office.generated") &&
                contains_substr(curator_output, "Hidden mutation artifact provenance:") &&
                contains_substr(curator_output, hidden.mutation_record_id) &&
                contains_substr(curator_output, "source_entities:") &&
                contains_substr(curator_output, "source_events:"),
                "v26 duplicate materialization should be rejected and public formatting should redact hidden provenance while curator formatting shows the trace");
    }

    {
        ArchiveEngineState public_state = initialize_archive_engine(42);
        const HiddenTimelineClusterRequest cluster_request{HiddenClusterScope::InstitutionOrigin, "lock_authority", 590, 625, 42};
        const CandidateGenerationRequest candidate_request{CandidateGenerationStrategy::AddCorroboratingFragment, 625, "lock_authority", 42};
        const std::string public_before = serialize_for_replay_test(public_state);
        const std::string public_output = format_hidden_mutation_artifact_candidate_materialization_query(
            public_state,
            AccessLevel::Public,
            cluster_request,
            candidate_request,
            std::optional<HiddenMutationArtifactCandidateShape>{HiddenMutationArtifactCandidateShape::RitualNotice},
            std::nullopt
        );
        const std::string public_after = serialize_for_replay_test(public_state);

        ArchiveEngineState curator_state = initialize_archive_engine(42);
        const std::string curator_output = format_hidden_mutation_artifact_candidate_materialization_query(
            curator_state,
            AccessLevel::Curator,
            cluster_request,
            candidate_request,
            std::optional<HiddenMutationArtifactCandidateShape>{HiddenMutationArtifactCandidateShape::RitualNotice},
            std::nullopt
        );
        require(public_before == public_after &&
                contains_substr(public_output, "hidden_materialization_mutated: false") &&
                contains_substr(public_output, "candidate_materialization_mutated: false") &&
                !contains_substr(public_output, "mutation.hidden_truth") &&
                contains_substr(curator_output, "hidden_materialization_mutated: true") &&
                contains_substr(curator_output, "candidate_materialization_mutated: true") &&
                contains_substr(curator_output, "selected_shape: ritual notice") &&
                contains_substr(curator_output, "Hidden mutation artifact provenance:"),
                "v26 CLI-style workflow should preserve public gates and allow curator shape-selected materialization in memory");
    }


    {
        auto valid_spec_body = [](const std::string& id, const std::string& display_name, std::uint64_t seed) {
            std::ostringstream out;
            out << "{"
                << "\"id\":\"" << id << "\","
                << "\"display_name\":\"" << display_name << "\","
                << "\"description\":\"A compact valid test civilization recipe for intake validation.\","
                << "\"earliest_year\":100,\"latest_year\":700,"
                << "\"geographic_features\":[\"river_delta\",\"salt_marsh\",\"raised_mounds\",\"canal_grid\"],"
                << "\"environmental_pressures\":[\"seasonal_flooding\",\"canal_silting\",\"drought_cycles\"],"
                << "\"major_sites\":[\"central_citadel\",\"lower_gate\",\"reed_shrine\",\"grain_quay\"],"
                << "\"economic_pressures\":[\"grain_tax\",\"canal_control\",\"dock_tolls\"],"
                << "\"trade_goods\":[\"grain\",\"salt\",\"reed_matting\",\"dried_fish\"],"
                << "\"institution_archetypes\":[\"water_office\",\"ritual_court\",\"merchant_house\",\"grain_tax_house\"],"
                << "\"social_actor_archetypes\":[\"marsh_clans\",\"dock_families\"],"
                << "\"authority_conflicts\":[\"water_office_vs_ritual_court\",\"merchant_house_vs_grain_tax_house\"],"
                << "\"religious_or_mythic_archetypes\":[\"flood_ancestor_cult\",\"reed_oracle\",\"drowned_founder_myth\"],"
                << "\"ritual_pressures\":[\"annual_gate_opening\",\"flood_appeasement\"],"
                << "\"writing_system_archetypes\":[\"reed_notches\",\"clay_seal_marks\"],"
                << "\"recordkeeping_styles\":[\"grain_ledger\",\"labor_roster\",\"boundary_stone\",\"ritual_song\"],"
                << "\"artifact_media\":[\"clay_tablet\",\"reed_ledger\",\"boundary_stone\",\"oral_song\"],"
                << "\"evidence_distortion_modes\":[\"flood_damage\",\"ritual_compression\",\"political_forgery\",\"calendar_drift\"],"
                << "\"mystery_archetypes\":[\"disputed_founder_identity\",\"missing_office_origin\",\"contradictory_flood_date\",\"forged_tax_decree\"],"
                << "\"target_hidden_entity_count\":18,"
                << "\"target_hidden_event_count\":24,"
                << "\"target_public_artifact_count\":16,"
                << "\"target_mystery_count\":5,"
                << "\"seed\":" << seed
                << "}";
            return out.str();
        };
        auto catalog_with = [](const std::string& specs) {
            return std::string("{\"schema_version\":\"1.1\",\"catalog_id\":\"test_catalog\",\"civilizations\":[") + specs + "]}";
        };

        const std::string one_spec = catalog_with(valid_spec_body("marsh_test", "Marsh Test", 7001));
        const CivilizationSpecLoadResult one_load = load_civilization_specs_from_json_text(one_spec);
        const CivilizationSpecValidationResult one_validation = validate_civilization_catalog(one_load.catalog);
        require(one_load.ok() && one_validation.valid && one_load.catalog.civilizations.size() == 1U,
                "v26.1 CivilizationSpec intake should accept a one-spec catalog and must not hard-code a 40-spec size");
        require(contains_substr(format_civilization_catalog_validation(one_load.catalog, one_validation), "- loaded: 1"),
                "v26.1 CivilizationSpec validation formatting should print the dynamic loaded count");
        require(find_civilization_spec(one_load.catalog, "marsh_test") != nullptr &&
                find_civilization_spec(one_load.catalog, "missing_test") == nullptr,
                "v26.1 CivilizationSpec lookup should return a pointer for found specs and nullptr for missing specs");

        const std::string two_spec = catalog_with(valid_spec_body("marsh_test", "Marsh Test", 7001) + "," +
                                                  valid_spec_body("ash_test", "Ash Test", 7002));
        const CivilizationSpecLoadResult two_load = load_civilization_specs_from_json_text(two_spec);
        const CivilizationSpecValidationResult two_validation = validate_civilization_catalog(two_load.catalog);
        require(two_load.ok() && two_validation.valid && two_load.catalog.civilizations.size() == 2U,
                "v26.1 CivilizationSpec intake should accept variable catalog cardinality above one");

        const CivilizationSpecLoadResult empty_load = load_civilization_specs_from_json_text("{\"civilizations\":[]}");
        const CivilizationSpecValidationResult empty_validation = validate_civilization_catalog(empty_load.catalog);
        require(empty_load.ok() && !empty_validation.valid &&
                contains_substr(format_civilization_catalog_validation(empty_load.catalog, empty_validation), "at least one CivilizationSpec"),
                "v26.1 CivilizationSpec validation should reject empty catalogs");

        const CivilizationSpecLoadResult duplicate_load = load_civilization_specs_from_json_text(
            catalog_with(valid_spec_body("marsh_test", "Marsh Test", 7001) + "," +
                         valid_spec_body("marsh_test", "Marsh Test Duplicate", 7002)));
        const CivilizationSpecValidationResult duplicate_validation = validate_civilization_catalog(duplicate_load.catalog);
        require(duplicate_load.ok() && !duplicate_validation.valid &&
                std::any_of(duplicate_validation.errors.begin(), duplicate_validation.errors.end(), [](const std::string& error) {
                    return contains_substr(error, "duplicate civilization id");
                }),
                "v26.1 CivilizationSpec validation should reject duplicate civilization IDs");

        CivilizationSpec bad_id = one_load.catalog.civilizations.front();
        bad_id.id = "Marsh-Citadel";
        require(!validate_civilization_spec(bad_id).valid,
                "v26.1 CivilizationSpec validation should reject IDs that are not lowercase snake_case");

        CivilizationSpec missing_social = one_load.catalog.civilizations.front();
        missing_social.social_actor_archetypes.clear();
        require(!validate_civilization_spec(missing_social).valid,
                "v26.1 CivilizationSpec validation should enforce social_actor_archetypes as a v1.1 required vector");

        CivilizationSpec malformed_conflict = one_load.catalog.civilizations.front();
        malformed_conflict.authority_conflicts = {"water_office_ritual_court", "merchant_house_vs_grain_tax_house"};
        require(!validate_civilization_spec(malformed_conflict).valid,
                "v26.1 CivilizationSpec validation should require exactly one _vs_ in authority conflicts");

        CivilizationSpec unknown_actor = one_load.catalog.civilizations.front();
        unknown_actor.authority_conflicts = {"water_office_vs_unknown_clan", "merchant_house_vs_grain_tax_house"};
        require(!validate_civilization_spec(unknown_actor).valid,
                "v26.1 CivilizationSpec validation should reject authority conflicts whose participants do not resolve to known actors");

        CivilizationSpec duplicate_conflict = one_load.catalog.civilizations.front();
        duplicate_conflict.authority_conflicts = {"water_office_vs_ritual_court", "water_office_vs_ritual_court"};
        require(!validate_civilization_spec(duplicate_conflict).valid,
                "v26.1 CivilizationSpec validation should reject duplicate authority conflicts");

        const std::string unknown_field_spec = valid_spec_body("marsh_test", "Marsh Test", 7001);
        const std::string with_unknown_field = catalog_with(unknown_field_spec.substr(0U, unknown_field_spec.size() - 1U) + ",\"unexpected\":\"schema drift\"}");
        const CivilizationSpecLoadResult unknown_field_load = load_civilization_specs_from_json_text(with_unknown_field);
        require(!unknown_field_load.ok() &&
                std::any_of(unknown_field_load.errors.begin(), unknown_field_load.errors.end(), [](const std::string& error) {
                    return contains_substr(error, "unknown field");
                }),
                "v26.1 CivilizationSpec loader should reject unknown spec fields");

        const CivilizationSpecLoadResult trailing_comma_load = load_civilization_specs_from_json_text("{\"civilizations\":[,]}");
        require(!trailing_comma_load.ok(),
                "v26.1 narrow JSON parser should reject malformed arrays instead of accepting loose JSON");

        const ArchiveEngineState before_spec_query = initialize_archive_engine(42);
        const std::string before_serialized = serialize_for_replay_test(before_spec_query);
        (void)format_civilization_spec_summary(one_load.catalog.civilizations.front());
        const std::string after_serialized = serialize_for_replay_test(before_spec_query);
        require(before_serialized == after_serialized,
                "v26.1 CivilizationSpec inspection helpers should not initialize or mutate archive runtime state");

        const CivilizationBootstrapResult marsh_bootstrap = bootstrap_archive_state_from_civilization_spec(
            one_load.catalog.civilizations.front(),
            one_load.catalog.catalog_id,
            one_load.catalog.schema_version
        );
        require(marsh_bootstrap.ok && validate_full_state(marsh_bootstrap.state).empty() &&
                marsh_bootstrap.state.civilization_source.has_value() &&
                marsh_bootstrap.state.civilization_source->civilization_id == "marsh_test",
                "v26.2 bootstrap should build one valid ArchiveEngineState with CivilizationRuntimeSource");
        require(!marsh_bootstrap.state.hidden_truth.entities().empty() &&
                !marsh_bootstrap.state.hidden_truth.events().empty() &&
                marsh_bootstrap.state.public_archive.artifacts().empty(),
                "v26.2 bootstrap should create minimal hidden state without generating a full public artifact pool");

        auto make_spec_quality_gate_candidate = [] {
            CandidateFeature candidate;
            candidate.id = "candidate.spec_quality_gate_generic_direct_copy";
            candidate.type = CandidateFeatureType::Artifact;
            candidate.description = "A generic moon cult artifact where a divine king chosen prophecy restores a lost empire bureaucracy after hubris at a silver temple.";
            CandidateArtifactMetadata metadata;
            metadata.true_creation_year = 390;
            metadata.claimed_creation_year = 390;
            metadata.discovery_year = 720;
            metadata.location_created = "site.marsh_test.central_citadel";
            metadata.location_found = "site.marsh_test.central_citadel";
            metadata.language_id = "language.marsh_test.bootstrap_language";
            metadata.dialect_id = "dialect.marsh_test.bootstrap_dialect";
            metadata.script_id = "script.marsh_test.reed_notches";
            metadata.referenced_entity_ids = {"institution.marsh_test.water_office", "site.marsh_test.central_citadel"};
            candidate.structured_artifact_metadata = metadata;
            CandidateClaimMetadata claim;
            claim.claim_type = ClaimType::FactualClaim;
            claim.predicate_type = PredicateType::LocatedAt;
            claim.subject = "Water Office";
            claim.predicate = "recorded_at";
            claim.object = "Central Citadel";
            claim.literal_content = "Generic candidate claims a public administrative record at the central citadel.";
            claim.subject_entity_id = "institution.marsh_test.water_office";
            claim.object_entity_id = "site.marsh_test.central_citadel";
            claim.claimed_year = 390;
            claim.confidence = 0.51;
            candidate.structured_claims = {claim};
            return candidate;
        };

        {
            ArchiveEngineState quality_state = marsh_bootstrap.state;
            CandidateFeature direct_copy_candidate = make_spec_quality_gate_candidate();
            const CandidateEvaluation direct_copy_eval = evaluate_candidate_feature(quality_state, direct_copy_candidate, AccessLevel::Curator);
            const std::string before = serialize_for_replay_test(quality_state);
            const MaterializationResult direct_copy_result = materialize_candidate_feature(quality_state, direct_copy_candidate, direct_copy_eval, AccessLevel::Curator);
            const std::string after = serialize_for_replay_test(quality_state);
            require(!direct_copy_result.mutated && before == after &&
                    direct_copy_eval.originality.direct_copy_risk_score >= 0.35 &&
                    contains_substr(direct_copy_result.explanation, "direct-copy risk above threshold"),
                    "v26.5 high direct-copy risk should block materialization before public archive mutation");
        }

        {
            ArchiveEngineState quality_state = initialize_archive_engine(42);
            CandidateFeature trope_candidate = sample_candidate_feature("structured_lock_fragment");
            trope_candidate.id = "candidate.structured_lock_fragment_roman_restoration_risk";
            trope_candidate.description += " Restored road and gate order formula for Reservoir Gate authority.";
            const CandidateEvaluation trope_eval = evaluate_candidate_feature(quality_state, trope_candidate, AccessLevel::Curator);
            const std::string before = serialize_for_replay_test(quality_state);
            const MaterializationResult trope_result = materialize_candidate_feature(quality_state, trope_candidate, trope_eval, AccessLevel::Curator);
            const std::string after = serialize_for_replay_test(quality_state);
            require(!trope_result.mutated && before == after &&
                    !trope_eval.originality.trope_flags.empty() &&
                    trope_eval.originality.civilization_specificity_score >= 0.30 &&
                    contains_substr(trope_result.explanation, "originality trope flags present"),
                    "v26.5 originality trope flags should block materialization even when metadata is structurally valid");
        }

        {
            const CivilizationSpecLoadResult missing_metadata_load = load_civilization_specs_from_json_text(std::string("{\"civilizations\":[") + valid_spec_body("metadata_missing", "Metadata Missing", 7003) + "]}");
            const CivilizationBootstrapResult missing_metadata_bootstrap = bootstrap_archive_state_from_civilization_spec(
                missing_metadata_load.catalog.civilizations.front(),
                missing_metadata_load.catalog.catalog_id,
                missing_metadata_load.catalog.schema_version
            );
            const std::string missing_metadata_summary = format_civilization_bootstrap_summary(missing_metadata_bootstrap.state, AccessLevel::Curator);
            require(contains_substr(missing_metadata_summary, "catalog_id: unspecified") &&
                    contains_substr(missing_metadata_summary, "schema_version: unspecified"),
                    "v26.5 missing optional catalog metadata should format as unspecified instead of blank values");
        }

        const CivilizationBootstrapResult marsh_bootstrap_again = bootstrap_archive_state_from_civilization_spec(
            one_load.catalog.civilizations.front(),
            one_load.catalog.catalog_id,
            one_load.catalog.schema_version
        );
        require(marsh_bootstrap_again.ok &&
                serialize_for_replay_test(marsh_bootstrap.state) == serialize_for_replay_test(marsh_bootstrap_again.state),
                "v26.2 bootstrap should be deterministic for the same selected spec and seed");

        const CivilizationBootstrapResult ash_bootstrap = bootstrap_archive_state_from_civilization_spec(
            two_load.catalog.civilizations.at(1U),
            two_load.catalog.catalog_id,
            two_load.catalog.schema_version
        );
        require(ash_bootstrap.ok &&
                serialize_for_replay_test(marsh_bootstrap.state) != serialize_for_replay_test(ash_bootstrap.state) &&
                contains_substr(serialize_for_replay_test(ash_bootstrap.state), "civilization.ash_test"),
                "v26.2 bootstrap should produce distinct deterministic entity/event IDs for different specs");

        const std::string public_bootstrap_summary = format_civilization_bootstrap_summary(marsh_bootstrap.state, AccessLevel::Public);
        require(contains_substr(public_bootstrap_summary, "runtime: spec-selected") &&
                !contains_substr(public_bootstrap_summary, "generated hidden entity IDs") &&
                !contains_substr(public_bootstrap_summary, "event.marsh_test") &&
                !contains_substr(public_bootstrap_summary, "institution.marsh_test") &&
                !contains_substr(public_bootstrap_summary, "social_actor.marsh_test"),
                "v26.2 public bootstrap formatting should show counts and public labels without hidden IDs");

        const std::string curator_bootstrap_summary = format_civilization_bootstrap_summary(marsh_bootstrap.state, AccessLevel::Curator);
        require(contains_substr(curator_bootstrap_summary, "Bootstrap trace:") &&
                contains_substr(curator_bootstrap_summary, "generated hidden entity IDs") &&
                contains_substr(curator_bootstrap_summary, "event.marsh_test.foundation"),
                "v26.2 curator bootstrap formatting should expose deterministic trace details");

        const ArchiveEngineState fixed_fixture_default = initialize_archive_engine(42);
        require(!fixed_fixture_default.civilization_source.has_value(),
                "v26.2 fixed fixture initializer should remain available for regression workflows");

        require(contains_substr(format_generation_targets_for_state(fixed_fixture_default, AccessLevel::Public), "lock_authority") &&
                resolve_generation_target(fixed_fixture_default, "lock_authority").has_value(),
                "v26.3 fixed-fixture target listing and resolution should remain available without spec flags");

        const std::string spec_target_list = format_generation_targets_for_state(marsh_bootstrap.state, AccessLevel::Public);
        require(contains_substr(spec_target_list, "authority_conflict_0") &&
                contains_substr(spec_target_list, "institution_water_office") &&
                !contains_substr(spec_target_list, "institution.marsh_test"),
                "v26.3 public spec target listing should expose spec-derived target names without hidden IDs");

        const std::optional<GenerationTarget> spec_target = resolve_generation_target(marsh_bootstrap.state, "authority_conflict_0");
        require(spec_target.has_value() && spec_target->spec_source.has_value() &&
                spec_target->spec_source->civilization_id == "marsh_test" &&
                !spec_target->entity_ids.empty(),
                "v26.3 spec-bootstrapped states should resolve authority_conflict_N targets with provenance");

        const CandidateGenerationRequest spec_request{
            CandidateGenerationStrategy::AddCorroboratingFragment,
            625,
            "authority_conflict_0",
            42,
        };
        const std::string before_spec_generation = serialize_for_replay_test(marsh_bootstrap.state);
        const GeneratedCandidateBatch spec_batch = generate_candidate_batch(marsh_bootstrap.state, spec_request);
        const std::string after_spec_generation = serialize_for_replay_test(marsh_bootstrap.state);
        require(before_spec_generation == after_spec_generation &&
                spec_batch.candidates.size() == 3U &&
                spec_batch.resolved_target.has_value() &&
                spec_batch.resolved_target->spec_source.has_value(),
                "v26.3 spec candidate generation should be non-mutating and produce a resolved batch");
        require(std::all_of(spec_batch.candidates.begin(), spec_batch.candidates.end(), [&](const CandidateFeature& candidate) {
                    return evaluate_candidate_feature(marsh_bootstrap.state, candidate, AccessLevel::Curator).validation_errors.empty();
                }),
                "v26.3 spec-generated candidates should pass existing candidate evaluation structure checks");

        const HiddenTimelineClusterRequest spec_cluster_request{
            HiddenClusterScope::InstitutionOrigin,
            "authority_conflict_0",
            250,
            380,
            42,
        };
        const GeneratedHiddenTimelineCluster spec_cluster = generate_hidden_timeline_cluster(marsh_bootstrap.state, spec_cluster_request);
        const HiddenTimelineClusterEvaluation spec_cluster_evaluation = evaluate_hidden_timeline_cluster(marsh_bootstrap.state, spec_cluster);
        require(!spec_cluster.proposed_events.empty() &&
                spec_cluster_evaluation.validation_errors.empty() &&
                contains_substr(spec_cluster.proposed_events.front().id, "event.generated.marsh_test.authority_conflict_0") &&
                !contains_substr(spec_cluster.proposed_events.front().id, "reservoir_gate"),
                "v26.3 spec hidden-cluster generation should produce valid spec-scoped IDs without fixed-fixture leakage");

        const std::string public_spec_cluster = format_hidden_timeline_cluster(marsh_bootstrap.state, AccessLevel::Public, spec_cluster_request);
        require(!contains_substr(public_spec_cluster, "event.generated.marsh_test") &&
                !contains_substr(public_spec_cluster, "institution.marsh_test") &&
                contains_substr(public_spec_cluster, "hidden cluster internals are restricted"),
                "v26.3 public spec hidden-cluster formatting should redact hidden IDs");
        const std::string curator_spec_cluster = format_hidden_timeline_cluster(marsh_bootstrap.state, AccessLevel::Curator, spec_cluster_request);
        require(contains_substr(curator_spec_cluster, "Spec target source:") &&
                contains_substr(curator_spec_cluster, "source_entities:") &&
                contains_substr(curator_spec_cluster, "event.generated.marsh_test.authority_conflict_0"),
                "v26.3 curator spec hidden-cluster formatting should include target trace and generated IDs");


        ArchiveEngineState spec_mutation_state = marsh_bootstrap.state;
        const GeneratedHiddenTimelineCluster spec_materialization_cluster = generate_hidden_timeline_cluster(spec_mutation_state, spec_cluster_request);
        const HiddenClusterMaterializationResult spec_public_gate = materialize_hidden_timeline_cluster(spec_mutation_state, spec_materialization_cluster, AccessLevel::Public);
        require(!spec_public_gate.mutated && spec_mutation_state.hidden_truth_mutations.empty(),
                "v26.4 public access should not materialize spec-derived hidden clusters or create audit records");

        const HiddenClusterMaterializationResult spec_curator_materialization = materialize_hidden_timeline_cluster(
            spec_mutation_state,
            spec_materialization_cluster,
            AccessLevel::Curator
        );
        require(spec_curator_materialization.mutated &&
                spec_mutation_state.hidden_truth_mutations.size() == 1U &&
                !spec_curator_materialization.mutation_record_id.empty() &&
                contains_substr(spec_curator_materialization.inserted_event_ids.front(), "event.generated.marsh_test.authority_conflict_0"),
                "v26.4 curator access should materialize a spec-derived hidden cluster and create one audit record");

        const HiddenClusterMaterializationResult spec_duplicate_materialization = materialize_hidden_timeline_cluster(
            spec_mutation_state,
            spec_materialization_cluster,
            AccessLevel::Curator
        );
        require(!spec_duplicate_materialization.mutated && spec_mutation_state.hidden_truth_mutations.size() == 1U,
                "v26.4 duplicate spec hidden-cluster materialization should not create a false audit record");

        const HiddenTruthMutationRecord& spec_record = spec_mutation_state.hidden_truth_mutations.front();
        const CandidateGenerationRequest spec_mutation_candidate_request{
            CandidateGenerationStrategy::AddCorroboratingFragment,
            390,
            "authority_conflict_0",
            42,
        };
        const GeneratedCandidateBatch spec_mutation_candidates = generate_candidates_from_hidden_mutation(
            spec_mutation_state,
            spec_record,
            spec_mutation_candidate_request
        );
        require(spec_mutation_candidates.candidates.size() == 3U &&
                std::all_of(spec_mutation_candidates.candidates.begin(), spec_mutation_candidates.candidates.end(), [&](const CandidateFeature& candidate) {
                    const CandidateEvaluation evaluation = evaluate_candidate_feature(spec_mutation_state, candidate, AccessLevel::Curator);
                    return evaluation.validation_errors.empty() && candidate.hidden_mutation_source.has_value();
                }),
                "v26.4 spec-derived hidden mutations should generate three valid hidden-mutation artifact candidates");

        const auto ritual_it = std::find_if(spec_mutation_candidates.candidates.begin(), spec_mutation_candidates.candidates.end(), [](const CandidateFeature& candidate) {
            return contains_substr(candidate.id, "ritual_notice");
        });
        require(ritual_it != spec_mutation_candidates.candidates.end(),
                "v26.4 spec mutation candidate batch should include ritual_notice shape");
        if (ritual_it != spec_mutation_candidates.candidates.end()) {
            const MaterializationResult public_artifact_gate = materialize_hidden_mutation_artifact_candidate(
                spec_mutation_state,
                *ritual_it,
                AccessLevel::Public
            );
            require(!public_artifact_gate.mutated,
                    "v26.4 public access should not materialize spec-derived hidden-mutation artifact candidates");

            const CandidateEvaluation ritual_evaluation = evaluate_candidate_feature(spec_mutation_state, *ritual_it, AccessLevel::Curator);
            const std::string before_quality_gate = serialize_for_replay_test(spec_mutation_state);
            const std::size_t artifact_count_before = spec_mutation_state.public_archive.artifacts().size();
            const std::size_t discovery_count_before = spec_mutation_state.discovery_log.size();
            const MaterializationResult spec_artifact_materialization = materialize_hidden_mutation_artifact_candidate(
                spec_mutation_state,
                *ritual_it,
                AccessLevel::Curator
            );
            const std::string after_quality_gate = serialize_for_replay_test(spec_mutation_state);
            require(ritual_evaluation.originality.civilization_specificity_score < 0.30 &&
                    !spec_artifact_materialization.mutated &&
                    spec_artifact_materialization.inserted_artifact_ids.empty() &&
                    spec_artifact_materialization.inserted_claim_ids.empty() &&
                    spec_mutation_state.public_archive.artifacts().size() == artifact_count_before &&
                    spec_mutation_state.discovery_log.size() == discovery_count_before &&
                    before_quality_gate == after_quality_gate &&
                    contains_substr(spec_artifact_materialization.explanation, "civilization specificity below threshold"),
                    "v26.5 low-specificity spec-derived hidden-mutation candidates should remain reviewable but non-materializable");
        }

        ArchiveEngineState public_format_state = marsh_bootstrap.state;
        const std::string public_spec_materialization = format_hidden_cluster_materialization_query(
            public_format_state,
            AccessLevel::Public,
            spec_cluster_request
        );
        require(!contains_substr(public_spec_materialization, "event.generated.marsh_test") &&
                !contains_substr(public_spec_materialization, "mutation.hidden_truth") &&
                contains_substr(public_spec_materialization, "internals are restricted"),
                "v26.4 public spec materialization formatting should redact hidden mutation and event IDs");

        ArchiveEngineState curator_format_state = marsh_bootstrap.state;
        const std::string curator_spec_materialization = format_hidden_cluster_materialization_query(
            curator_format_state,
            AccessLevel::Curator,
            spec_cluster_request
        );
        require(contains_substr(curator_spec_materialization, "Spec target source:") &&
                contains_substr(curator_spec_materialization, "mutation_record_id:") &&
                contains_substr(curator_spec_materialization, "event.generated.marsh_test.authority_conflict_0"),
                "v26.4 curator spec materialization formatting should expose target, cluster, and mutation trace");
    }

    if (failures == 0) {
        std::cout << "All self-tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << failures << " self-test(s) failed.\n";
    return EXIT_FAILURE;
}

} // namespace archive
