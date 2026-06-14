/*
 * Command-line parsing and query dispatch. CLI code depends on engine modules; engine modules should not depend on CLI details.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] std::uint64_t parse_u64(const std::string& text) {
    if (text.empty()) {
        throw std::invalid_argument("invalid unsigned integer: " + text);
    }
    if (text.front() == '-') {
        throw std::invalid_argument("seed must be unsigned: " + text);
    }
    std::size_t consumed = 0;
    const unsigned long long value = std::stoull(text, &consumed, 10);
    if (consumed != text.size()) {
        throw std::invalid_argument("invalid unsigned integer: " + text);
    }
    return static_cast<std::uint64_t>(value);
}

[[nodiscard]] int parse_int(const std::string& text) {
    std::size_t consumed = 0;
    const long value = std::stol(text, &consumed, 10);
    if (consumed != text.size()) {
        throw std::invalid_argument("invalid integer: " + text);
    }
    if (value < 0 || value > kOpenEndedYear) {
        throw std::invalid_argument("archive year out of range: " + text);
    }
    return static_cast<int>(value);
}

[[nodiscard]] std::vector<std::string> split_csv(std::string_view text) {
    std::vector<std::string> parts;
    std::size_t start = 0U;
    while (start <= text.size()) {
        const std::size_t comma = text.find(',', start);
        const std::size_t end = comma == std::string_view::npos ? text.size() : comma;
        std::string part{text.substr(start, end - start)};
        if (!part.empty()) {
            parts.push_back(std::move(part));
        }
        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1U;
    }
    return parts;
}

[[nodiscard]] std::vector<std::size_t> parse_index_list(std::string_view text) {
    std::vector<std::size_t> result;
    for (const std::string& part : split_csv(text)) {
        result.push_back(static_cast<std::size_t>(parse_int(part)));
    }
    return result;
}

[[nodiscard]] std::vector<GeneratedCandidateRole> parse_role_list(std::string_view text) {
    std::vector<GeneratedCandidateRole> result;
    for (const std::string& part : split_csv(text)) {
        result.push_back(parse_generated_candidate_role(part));
    }
    return result;
}

[[nodiscard]] HiddenMutationArtifactCandidateShape parse_hidden_mutation_candidate_shape(std::string_view text) {
    if (text == "admin_docket" || text == "administrative_docket") {
        return HiddenMutationArtifactCandidateShape::AdministrativeDocket;
    }
    if (text == "ritual_notice") {
        return HiddenMutationArtifactCandidateShape::RitualNotice;
    }
    if (text == "scholar_fragment" || text == "later_scholar_fragment") {
        return HiddenMutationArtifactCandidateShape::ScholarFragment;
    }
    throw std::invalid_argument("unknown hidden-mutation candidate shape: " + std::string(text));
}

[[nodiscard]] DefaultRuntimeConfig default_runtime_config() {
    return DefaultRuntimeConfig{};
}

[[nodiscard]] ArchiveRuntimeMode parse_archive_runtime_mode(std::string_view text) {
    if (text == "fixed-fixture") {
        return ArchiveRuntimeMode::FixedFixture;
    }
    if (text == "spec-selected") {
        return ArchiveRuntimeMode::SpecSelected;
    }
    throw std::invalid_argument("unknown runtime mode: " + std::string(text));
}

struct FormattedCommandResult {
    std::string text;
    int exit_code = EXIT_SUCCESS;
};

[[nodiscard]] std::vector<std::size_t> resolve_selection_indices(const ArchiveEngineState& state,
                                                                  const CandidateGenerationRequest& request,
                                                                  const CliOptions& options) {
    if (!options.candidate_indices.empty()) {
        return options.candidate_indices;
    }
    std::vector<std::size_t> resolved;
    if (!options.candidate_roles.empty()) {
        for (GeneratedCandidateRole role : options.candidate_roles) {
            const std::optional<std::size_t> index = generated_candidate_index_for_role(state, request, role);
            if (index.has_value()) {
                resolved.push_back(*index);
            } else {
                resolved.push_back(static_cast<std::size_t>(kOpenEndedYear));
            }
        }
        return resolved;
    }
    if (options.candidate_index_supplied) {
        return {static_cast<std::size_t>(options.candidate_index)};
    }
    if (options.candidate_role.has_value()) {
        const std::optional<std::size_t> index = generated_candidate_index_for_role(state, request, *options.candidate_role);
        if (index.has_value()) {
            return {*index};
        }
        return {static_cast<std::size_t>(kOpenEndedYear)};
    }
    return {};
}

[[nodiscard]] bool is_civilization_spec_query(std::string_view query) {
    return query == "validate-civilization-specs" ||
           query == "list-civilization-specs" ||
           query == "show-civilization-spec" ||
           query == "list-civilization-tags" ||
           query == "list-civilizations-by-tag" ||
           query == "validate-civilization-tags" ||
           query == "list-civilization-fragments" ||
           query == "show-civilization-fragment" ||
           query == "validate-civilization-fragments";
}

[[nodiscard]] bool is_golden_fixture_query(std::string_view query) {
    return query == "list-golden-fixtures" ||
           query == "show-golden-fixture" ||
           query == "archive-snapshot" ||
           query == "compare-archive-snapshots";
}

[[nodiscard]] bool is_fixture_backed_query(std::string_view query) {
    return query == "show-golden-fixture" ||
           query == "archive-snapshot" ||
           query == "compare-archive-snapshots";
}

[[nodiscard]] bool has_incompatible_fixture_override(const CliOptions& options) {
    return options.spec_file_supplied ||
           options.civilization_id_supplied ||
           options.seed_supplied ||
           options.archive_year_supplied;
}

[[nodiscard]] std::string incompatible_fixture_override_error() {
    return "error: --fixture-id uses a fixed golden fixture definition; remove --spec-file, --civilization-id, --seed, and --archive-year.\n";
}

[[nodiscard]] bool is_spec_runtime_compatible_query(std::string_view query) {
    return query == "report" ||
           query == "truth" ||
           query == "artifacts" ||
           query == "claims" ||
           query == "contradictions" ||
           query == "anachronisms" ||
           query == "timeline" ||
           query == "validation" ||
           query == "theories" ||
           query == "discoveries" ||
           query == "mysteries" ||
           query == "originality" ||
           query == "hidden-mutations" ||
           query == "list-evidence-potentials" ||
           query == "show-evidence-potential" ||
           query == "validate-evidence-potentials" ||
           query == "evidence-potential-summary" ||
           query == "validate-knowledge-horizon" ||
           query == "knowledge-horizon-summary" ||
           query == "list-knowledge-horizon-findings" ||
           query == "show-knowledge-horizon-finding" ||
           query == "contradiction-budget-summary" ||
           query == "list-contradiction-budget-buckets" ||
           query == "show-contradiction-budget-bucket" ||
           query == "validate-contradiction-budget" ||
           query == "candidate-artifact-plan-summary" ||
           query == "list-candidate-artifact-plans" ||
           query == "show-candidate-artifact-plan" ||
           query == "validate-candidate-artifact-plans" ||
           query == "candidate-artifact-plan-evaluation-summary" ||
           query == "list-candidate-artifact-plan-evaluations" ||
           query == "show-candidate-artifact-plan-evaluation" ||
           query == "validate-candidate-artifact-plan-evaluations" ||
           query == "candidate-artifact-proposal-summary" ||
           query == "list-candidate-artifact-proposals" ||
           query == "show-candidate-artifact-proposal" ||
           query == "validate-candidate-artifact-proposals" ||
           query == "candidate-artifact-proposal-audit-summary" ||
           query == "list-candidate-artifact-proposal-audits" ||
           query == "show-candidate-artifact-proposal-audit" ||
           query == "validate-candidate-artifact-proposal-audits" ||
            query == "candidate-artifact-draft-summary" ||
            query == "list-candidate-artifact-drafts" ||
            query == "show-candidate-artifact-draft" ||
            query == "validate-candidate-artifact-drafts" ||
             query == "candidate-artifact-draft-review-summary" ||
             query == "list-candidate-artifact-draft-reviews" ||
             query == "show-candidate-artifact-draft-review" ||
             query == "validate-candidate-artifact-draft-reviews" ||
           query == "control-layer-audit-summary" ||
           query == "list-control-layer-audit-entries" ||
           query == "show-control-layer-audit-entry" ||
           query == "validate-control-layer-audit" ||
           query == "bootstrap-civilization" ||
           query == "list-generation-targets" ||
           query == "generate-candidates" ||
           query == "hidden-cluster" ||
           query == "materialize-hidden-cluster" ||
           query == "generate-artifacts-from-hidden-mutation" ||
           query == "materialize-hidden-mutation-artifact-candidate";
}

[[nodiscard]] bool has_any_spec_runtime_flag(const CliOptions& options) {
    return !options.spec_file.empty() || !options.civilization_id.empty();
}

[[nodiscard]] RuntimeStateSelectionResult build_runtime_state_for_query(const CliOptions& options) {
    RuntimeStateSelectionResult result;
    const DefaultRuntimeConfig defaults = default_runtime_config();
    result.selection = options.runtime_selection;

    const bool has_spec_file = !options.spec_file.empty();
    const bool has_civilization_id = !options.civilization_id.empty();

    if (result.selection.explicit_runtime_mode && result.selection.mode == ArchiveRuntimeMode::FixedFixture) {
        if (has_spec_file || has_civilization_id) {
            result.usage_error = true;
            result.errors.push_back("--runtime fixed-fixture cannot be combined with --spec-file or --civilization-id");
            return result;
        }
        result.selection.mode = ArchiveRuntimeMode::FixedFixture;
        result.state = initialize_archive_engine(options.seed);
        derive_evidence_potentials_into_state(result.state);
        derive_candidate_artifact_plans_into_state(result.state, AccessLevel::Curator);
        evaluate_candidate_artifact_plans_into_state(result.state, AccessLevel::Curator);
        draft_candidate_artifact_proposals_into_state(result.state, AccessLevel::Curator);
        audit_candidate_artifact_proposals_into_state(result.state, AccessLevel::Curator);
        derive_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);
        review_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);
        build_control_layer_audit_into_state(result.state);
        result.ok = true;
        return result;
    }

    result.selection.mode = ArchiveRuntimeMode::SpecSelected;

    if (has_spec_file && !has_civilization_id) {
        result.usage_error = true;
        result.errors.push_back("--spec-file requires --civilization-id in spec-selected runtime");
        return result;
    }

    result.selection.spec_file = has_spec_file ? options.spec_file : defaults.default_spec_file;
    result.selection.civilization_id = has_civilization_id ? options.civilization_id : defaults.default_civilization_id;

    if (!is_spec_runtime_compatible_query(options.query)) {
        result.usage_error = true;
        result.errors.push_back("spec-selected runtime is not supported for this query yet: " + options.query + "; use --runtime fixed-fixture for fixed-regression workflows");
        return result;
    }

    const CivilizationSpecLoadResult load = load_civilization_specs_from_json_file(result.selection.spec_file);
    if (!load.ok()) {
        result.errors = load.errors;
        result.warnings = load.warnings;
        return result;
    }
    const CivilizationSpecValidationResult validation = validate_civilization_catalog(load.catalog);
    if (!validation.valid) {
        result.errors = validation.errors;
        result.warnings = validation.warnings;
        return result;
    }
    const CivilizationSpec* spec = find_civilization_spec(load.catalog, result.selection.civilization_id);
    if (spec == nullptr) {
        result.errors.push_back("CivilizationSpec not found: " + result.selection.civilization_id);
        return result;
    }
    const CivilizationBootstrapResult bootstrap = bootstrap_archive_state_from_civilization_spec(
        *spec,
        load.catalog.catalog_id,
        load.catalog.schema_version
    );
    if (!bootstrap.ok) {
        result.errors = bootstrap.errors;
        result.warnings = bootstrap.warnings;
        return result;
    }

    result.state = bootstrap.state;
    derive_evidence_potentials_into_state(result.state);
    derive_candidate_artifact_plans_into_state(result.state, AccessLevel::Curator);
    evaluate_candidate_artifact_plans_into_state(result.state, AccessLevel::Curator);
    draft_candidate_artifact_proposals_into_state(result.state, AccessLevel::Curator);
    audit_candidate_artifact_proposals_into_state(result.state, AccessLevel::Curator);
    derive_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);
    review_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);
        build_control_layer_audit_into_state(result.state);
    result.state.civilization_spec_count = load.catalog.civilizations.size();
    result.state.civilization_fragment_count = load.catalog.fragments.size();
    result.ok = true;
    return result;
}

[[nodiscard]] std::string format_runtime_selection_errors(const CliOptions& options,
                                                               const RuntimeStateSelectionResult& selection) {
    std::ostringstream out;
    out << "Runtime state selection failed:\n";
    out << "- query: " << options.query << "\n";
    out << "- runtime: "
        << (selection.selection.mode == ArchiveRuntimeMode::SpecSelected ? "spec-selected" : "fixed-fixture") << "\n";
    if (!selection.selection.spec_file.empty()) {
        out << "- spec_file: " << selection.selection.spec_file << "\n";
    }
    if (!selection.selection.civilization_id.empty()) {
        out << "- civilization_id: " << selection.selection.civilization_id << "\n";
    }
    out << "- errors: " << selection.errors.size() << "\n";
    for (const std::string& error : selection.errors) {
        out << "  error: " << error << "\n";
    }
    for (const std::string& warning : selection.warnings) {
        out << "  warning: " << warning << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_load_errors(const CivilizationSpecLoadResult& load) {
    std::ostringstream out;
    out << "CivilizationSpec load failed:\n";
    out << "- errors: " << load.errors.size() << "\n";
    for (const std::string& error : load.errors) {
        out << "  error: " << error << "\n";
    }
    for (const std::string& warning : load.warnings) {
        out << "  warning: " << warning << "\n";
    }
    return out.str();
}

[[nodiscard]] FormattedCommandResult format_civilization_spec_query_result(const CliOptions& options) {
    const DefaultRuntimeConfig defaults = default_runtime_config();
    const std::string spec_file = options.spec_file.empty() ? defaults.default_spec_file : options.spec_file;
    const std::string civilization_id = options.civilization_id.empty() ? defaults.default_civilization_id : options.civilization_id;

    const CivilizationSpecLoadResult load = load_civilization_specs_from_json_file(spec_file);
    if (!load.ok()) {
        return {format_load_errors(load), EXIT_FAILURE};
    }
    const CivilizationSpecValidationResult validation = validate_civilization_catalog(load.catalog);
    if (options.query == "validate-civilization-specs") {
        return {format_civilization_catalog_validation(load.catalog, validation), validation.valid ? EXIT_SUCCESS : EXIT_FAILURE};
    }
    if (options.query == "validate-civilization-tags") {
        return {format_civilization_catalog_tag_validation(load.catalog, validation), validation.valid ? EXIT_SUCCESS : EXIT_FAILURE};
    }
    if (options.query == "validate-civilization-fragments") {
        const CivilizationSpecValidationResult fragment_validation = validate_civilization_fragments(load.catalog);
        return {format_civilization_fragment_validation(load.catalog, fragment_validation), fragment_validation.valid ? EXIT_SUCCESS : EXIT_FAILURE};
    }
    if (!validation.valid) {
        return {format_civilization_catalog_validation(load.catalog, validation), EXIT_FAILURE};
    }
    if (options.query == "list-civilization-specs") {
        return {format_civilization_catalog_list(load.catalog, validation), EXIT_SUCCESS};
    }
    if (options.query == "list-civilization-tags") {
        return {format_civilization_catalog_tags(load.catalog), EXIT_SUCCESS};
    }
    if (options.query == "list-civilizations-by-tag") {
        if (options.tag.empty()) {
            return {"list-civilizations-by-tag requires --tag TAG\n", EXIT_FAILURE};
        }
        return {format_civilization_specs_by_tag(load.catalog, options.tag), EXIT_SUCCESS};
    }
    if (options.query == "list-civilization-fragments") {
        return {format_civilization_fragment_list(load.catalog), EXIT_SUCCESS};
    }
    if (options.query == "show-civilization-fragment") {
        if (options.fragment_id.empty()) {
            return {"show-civilization-fragment requires --fragment-id ID\n", EXIT_FAILURE};
        }
        const CivilizationSpecFragment* fragment = find_civilization_fragment(load.catalog, options.fragment_id);
        if (fragment == nullptr) {
            std::ostringstream out;
            out << "CivilizationSpec fragment not found:\n";
            out << "- fragment_id: " << options.fragment_id << "\n";
            out << "- loaded: " << load.catalog.fragments.size() << "\n";
            return {out.str(), EXIT_FAILURE};
        }
        return {format_civilization_spec_fragment(*fragment), EXIT_SUCCESS};
    }
    if (options.query == "show-civilization-spec") {
        if (!options.spec_file.empty() && options.civilization_id.empty()) {
            throw std::invalid_argument("--spec-file requires --civilization-id for show-civilization-spec");
        }
        const CivilizationSpec* spec = find_civilization_spec(load.catalog, civilization_id);
        if (spec == nullptr) {
            std::ostringstream out;
            out << "CivilizationSpec not found:\n";
            out << "- civilization_id: " << civilization_id << "\n";
            out << "- loaded: " << load.catalog.civilizations.size() << "\n";
            return {out.str(), EXIT_FAILURE};
        }
        return {format_civilization_spec_summary(*spec), EXIT_SUCCESS};
    }
    if (options.query == "bootstrap-civilization") {
        const CivilizationSpec* spec = find_civilization_spec(load.catalog, civilization_id);
        if (spec == nullptr) {
            std::ostringstream out;
            out << "CivilizationSpec not found:\n";
            out << "- civilization_id: " << civilization_id << "\n";
            out << "- loaded: " << load.catalog.civilizations.size() << "\n";
            return {out.str(), EXIT_FAILURE};
        }
        const CivilizationBootstrapResult bootstrap = bootstrap_archive_state_from_civilization_spec(
            *spec,
            load.catalog.catalog_id,
            load.catalog.schema_version
        );
        if (!bootstrap.ok) {
            std::ostringstream out;
            out << "Civilization bootstrap failed:\n";
            out << "- civilization_id: " << civilization_id << "\n";
            out << "- errors: " << bootstrap.errors.size() << "\n";
            for (const std::string& error : bootstrap.errors) {
                out << "  error: " << error << "\n";
            }
            for (const std::string& warning : bootstrap.warnings) {
                out << "  warning: " << warning << "\n";
            }
            return {out.str(), EXIT_FAILURE};
        }
        return {format_civilization_bootstrap_summary(bootstrap.state, options.access), EXIT_SUCCESS};
    }
    throw std::invalid_argument("unknown CivilizationSpec query: " + options.query);
}

[[nodiscard]] std::string format_civilization_spec_query(const CliOptions& options) {
    return format_civilization_spec_query_result(options).text;
}


[[nodiscard]] FormattedCommandResult format_golden_fixture_query_result(const CliOptions& options) {
    if (options.query == "list-golden-fixtures") {
        return {format_golden_fixture_worlds(), EXIT_SUCCESS};
    }

    if (is_fixture_backed_query(options.query) && has_incompatible_fixture_override(options)) {
        return {incompatible_fixture_override_error(), EXIT_FAILURE};
    }

    const GoldenFixtureWorldDefinition* definition = find_golden_fixture_world(options.fixture_id);
    if (definition == nullptr) {
        std::ostringstream out;
        out << "Golden fixture not found:\n";
        out << "- fixture_id: " << options.fixture_id << "\n";
        return {out.str(), EXIT_FAILURE};
    }

    if (options.query == "show-golden-fixture") {
        return {format_golden_fixture_world(*definition), EXIT_SUCCESS};
    }

    const GoldenFixtureBuildResult built = build_golden_fixture_world(*definition);
    if (!built.ok) {
        std::ostringstream out;
        out << "Golden fixture build failed:\n";
        out << "- fixture_id: " << options.fixture_id << "\n";
        out << "- errors: " << built.errors.size() << "\n";
        for (const std::string& error : built.errors) {
            out << "  error: " << error << "\n";
        }
        for (const std::string& warning : built.warnings) {
            out << "  warning: " << warning << "\n";
        }
        return {out.str(), EXIT_FAILURE};
    }

    if (options.query == "archive-snapshot") {
        const ArchiveSnapshot snapshot = build_archive_snapshot(
            built.state,
            definition->id,
            definition->seed,
            definition->archive_year,
            definition->archive_year
        );
        return {format_archive_snapshot(snapshot), EXIT_SUCCESS};
    }

    if (options.query == "compare-archive-snapshots") {
        const GoldenFixtureBuildResult rebuilt = build_golden_fixture_world(*definition);
        if (!rebuilt.ok) {
            std::ostringstream out;
            out << "Golden fixture rebuild failed:\n";
            out << "- fixture_id: " << options.fixture_id << "\n";
            for (const std::string& error : rebuilt.errors) {
                out << "  error: " << error << "\n";
            }
            return {out.str(), EXIT_FAILURE};
        }
        const ArchiveSnapshot before = build_archive_snapshot(
            built.state,
            definition->id,
            definition->seed,
            definition->archive_year,
            definition->archive_year
        );
        const ArchiveSnapshot after = build_archive_snapshot(
            rebuilt.state,
            definition->id,
            definition->seed,
            definition->archive_year,
            definition->archive_year
        );
        const ArchiveSnapshotComparison comparison = compare_archive_snapshots(before, after);
        return {format_archive_snapshot_comparison(before, after), comparison.same ? EXIT_SUCCESS : EXIT_FAILURE};
    }

    throw std::invalid_argument("unknown golden fixture query: " + options.query);
}

[[nodiscard]] CliArgs make_cli_args(int argc, char** argv) {
    CliArgs args;
    if (argc <= 0 || argv == nullptr) {
        return args;
    }
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i] == nullptr ? std::string_view{} : std::string_view{argv[i]});
    }
    return args;
}

[[nodiscard]] CliOptions parse_cli(const CliArgs& args) {
    CliOptions options;

    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string arg(args[i]);
        auto next = [&](std::string_view option_name) -> std::string {
            if (i + 1 >= args.size()) {
                throw std::invalid_argument("missing value after " + std::string(option_name));
            }
            ++i;
            return std::string(args[i]);
        };

        if (arg == "--help" || arg == "-h") {
            options.query = "help";
        } else if (arg == "--self-test") {
            options.self_test = true;
        } else if (arg == "--session" || arg == "--runtime-session") {
            options.runtime_session = true;
        } else if (arg == "--seed") {
            options.seed = parse_u64(next("--seed"));
            options.seed_supplied = true;
        } else if (arg == "--access") {
            options.access = parse_access(next("--access"));
        } else if (arg == "--archive-year") {
            options.archive_year = parse_int(next("--archive-year"));
            options.archive_year_supplied = true;
        } else if (arg == "--query") {
            options.query = next("--query");
        } else if (arg == "--candidate") {
            options.candidate_id = next("--candidate");
        } else if (arg == "--strategy") {
            options.generation_strategy = parse_candidate_generation_strategy(next("--strategy"));
        } else if (arg == "--target-topic") {
            options.target_topic = next("--target-topic");
        } else if (arg == "--target-year") {
            options.target_year = parse_int(next("--target-year"));
        } else if (arg == "--candidate-index") {
            options.candidate_index = parse_int(next("--candidate-index"));
            options.candidate_index_supplied = true;
        } else if (arg == "--candidate-role") {
            options.candidate_role = parse_generated_candidate_role(next("--candidate-role"));
        } else if (arg == "--candidate-shape") {
            options.candidate_shape = parse_hidden_mutation_candidate_shape(next("--candidate-shape"));
        } else if (arg == "--candidate-indices") {
            options.candidate_indices = parse_index_list(next("--candidate-indices"));
        } else if (arg == "--candidate-roles") {
            options.candidate_roles = parse_role_list(next("--candidate-roles"));
        } else if (arg == "--cluster-scope") {
            options.cluster_scope = parse_hidden_cluster_scope(next("--cluster-scope"));
        } else if (arg == "--start-year") {
            options.start_year = parse_int(next("--start-year"));
        } else if (arg == "--end-year") {
            options.end_year = parse_int(next("--end-year"));
        } else if (arg == "--spec-file") {
            options.spec_file = next("--spec-file");
            options.spec_file_supplied = true;
        } else if (arg == "--civilization-id") {
            options.civilization_id = next("--civilization-id");
            options.civilization_id_supplied = true;
        } else if (arg == "--tag") {
            options.tag = next("--tag");
        } else if (arg == "--fragment-id") {
            options.fragment_id = next("--fragment-id");
        } else if (arg == "--fixture-id") {
            options.fixture_id = next("--fixture-id");
        } else if (arg == "--evidence-potential-id") {
            options.evidence_potential_id = next("--evidence-potential-id");
        } else if (arg == "--knowledge-horizon-finding-id") {
            options.knowledge_horizon_finding_id = next("--knowledge-horizon-finding-id");
        } else if (arg == "--contradiction-budget-bucket-id") {
            options.contradiction_budget_bucket_id = next("--contradiction-budget-bucket-id");
        } else if (arg == "--candidate-artifact-plan-id") {
            options.candidate_artifact_plan_id = next("--candidate-artifact-plan-id");
        } else if (arg == "--candidate-artifact-plan-evaluation-id") {
            options.candidate_artifact_plan_evaluation_id = next("--candidate-artifact-plan-evaluation-id");
        } else if (arg == "--candidate-artifact-proposal-id") {
            options.candidate_artifact_proposal_id = next("--candidate-artifact-proposal-id");
        } else if (arg == "--candidate-artifact-proposal-audit-id") {
            options.candidate_artifact_proposal_audit_id = next("--candidate-artifact-proposal-audit-id");
        } else if (arg == "--candidate-artifact-draft-id") {
            options.candidate_artifact_draft_id = next("--candidate-artifact-draft-id");
        } else if (arg == "--candidate-artifact-draft-review-id") {
            options.candidate_artifact_draft_review_id = next("--candidate-artifact-draft-review-id");
        } else if (arg == "--control-layer-audit-entry-id") {
            options.control_layer_audit_entry_id = next("--control-layer-audit-entry-id");
        } else if (arg == "--runtime") {
            options.runtime_selection.mode = parse_archive_runtime_mode(next("--runtime"));
            options.runtime_selection.explicit_runtime_mode = true;
        } else {
            throw std::invalid_argument("unknown argument: " + arg);
        }
    }

    options.runtime_selection.spec_file = options.spec_file;
    options.runtime_selection.civilization_id = options.civilization_id;
    if (!options.runtime_selection.explicit_runtime_mode &&
        !options.spec_file.empty() && !options.civilization_id.empty()) {
        options.runtime_selection.mode = ArchiveRuntimeMode::SpecSelected;
    }

    return options;
}

[[nodiscard]] CliOptions parse_cli(int argc, char** argv) {
    return parse_cli(make_cli_args(argc, argv));
}

[[nodiscard]] std::string seed_behavior_note() {
    return "Seed behavior: the selected runtime seed is deterministic; spec-selected runtime derives its base seed from the chosen CivilizationSpec, while fixed-fixture regression mode preserves the legacy seed behavior.";
}

[[nodiscard]] std::string usage(std::string_view exe) {
    std::ostringstream out;
    out << "Usage: " << exe << " [--session] [--runtime fixed-fixture|spec-selected] [--seed N] [--access public|scholar|curator|canon|debug] [--archive-year YEAR] [--query report|truth|artifacts|claims|contradictions|anachronisms|timeline|validation|theories|discoveries|mysteries|originality|candidate|materialize|generate-candidates|evaluate-dossier|materialize-generated|materialize-dossier-candidate|plan-dossier-selection|materialize-dossier-selection|hidden-proposals|evaluate-hidden-proposal|plan-hidden-proposal|hidden-cluster|materialize-hidden-cluster|hidden-mutations|generate-artifacts-from-hidden-mutation|materialize-hidden-mutation-artifact-candidate|list-generation-targets|validate-civilization-specs|list-civilization-specs|show-civilization-spec|list-civilization-tags|list-civilizations-by-tag|validate-civilization-tags|list-civilization-fragments|show-civilization-fragment|validate-civilization-fragments|bootstrap-civilization|list-golden-fixtures|show-golden-fixture|archive-snapshot|compare-archive-snapshots|list-evidence-potentials|show-evidence-potential|validate-evidence-potentials|evidence-potential-summary|validate-knowledge-horizon|knowledge-horizon-summary|list-knowledge-horizon-findings|show-knowledge-horizon-finding|contradiction-budget-summary|list-contradiction-budget-buckets|show-contradiction-budget-bucket|validate-contradiction-budget|candidate-artifact-plan-summary|list-candidate-artifact-plans|show-candidate-artifact-plan|validate-candidate-artifact-plans|candidate-artifact-plan-evaluation-summary|list-candidate-artifact-plan-evaluations|show-candidate-artifact-plan-evaluation|validate-candidate-artifact-plan-evaluations|candidate-artifact-proposal-summary|list-candidate-artifact-proposals|show-candidate-artifact-proposal|validate-candidate-artifact-proposals|candidate-artifact-proposal-audit-summary|list-candidate-artifact-proposal-audits|show-candidate-artifact-proposal-audit|validate-candidate-artifact-proposal-audits|control-layer-audit-summary|list-control-layer-audit-entries|show-control-layer-audit-entry|validate-control-layer-audit] [--candidate ID] [--strategy corroborating_fragment|misleading_forgery|ritual_variant|target_dossier] [--target-topic TOPIC] [--target-year YEAR] [--candidate-index N] [--candidate-role ROLE] [--candidate-shape admin_docket|ritual_notice|scholar_fragment] [--candidate-indices A,B] [--candidate-roles ROLE,ROLE] [--spec-file PATH] [--civilization-id ID] [--tag TAG] [--fragment-id ID] [--fixture-id ID] [--evidence-potential-id ID] [--knowledge-horizon-finding-id ID] [--contradiction-budget-bucket-id ID] [--candidate-artifact-plan-id ID] [--candidate-artifact-plan-evaluation-id ID] [--candidate-artifact-proposal-id ID] [--candidate-artifact-proposal-audit-id ID] [--control-layer-audit-entry-id ID] [--cluster-scope institution_origin|schism_precursor|ecological_pressure|political_realignment|ritual_codification] [--start-year YEAR] [--end-year YEAR] [--self-test]\n";
    out << "\n";
    out << "Examples:\n";
    out << "  " << exe << " --self-test\n";
    out << "  printf '%s\\n' '--query candidate-artifact-draft-review-summary' '--query control-layer-audit-summary' 'end-session' | " << exe << " --session\n";
    out << "  " << exe << " --access public --query truth\n";
    out << "  " << exe << " --access scholar --archive-year 807 --query theories\n";
    out << "  " << exe << " --access canon --query timeline\n";
    out << "  " << exe << " --access debug --query report\n";
    out << "  " << exe << " --query list-generation-targets\n";
    out << "  " << exe << " --access scholar --query generate-candidates --target-topic authority_conflict_0 --target-year 620\n";
    out << "  " << exe << " --runtime fixed-fixture --access curator --query generate-candidates --strategy corroborating_fragment --target-topic lock_authority --target-year 620\n";
    out << "  " << exe << " --access curator --query evaluate-dossier --strategy target_dossier --target-topic lock_authority --target-year 620\n";
    out << "  " << exe << " --access curator --query materialize-generated --strategy corroborating_fragment --target-topic lock_authority --target-year 620 --candidate-index 0\n";
    out << "  " << exe << " --access curator --query materialize-dossier-candidate --strategy target_dossier --target-topic lock_authority --target-year 620 --candidate-role ritual_variant\n";
    out << "  " << exe << " --access curator --query plan-dossier-selection --strategy target_dossier --target-topic lock_authority --target-year 620 --candidate-roles corroborating_fragment,ritual_variant\n";
    out << "  " << exe << " --access curator --query materialize-dossier-selection --strategy target_dossier --target-topic lock_authority --target-year 620 --candidate-indices 0,1\n";
    out << "  " << exe << " --access curator --query hidden-proposals --target-topic lock_authority --target-year 620\n";
    out << "  " << exe << " --access curator --query plan-hidden-proposal --target-topic lock_authority --target-year 620 --candidate-index 0\n";
    out << "  " << exe << " --access curator --query hidden-cluster --cluster-scope institution_origin --target-topic lock_authority --start-year 590 --end-year 625 --seed 42\n";
    out << "  " << exe << " --access curator --query materialize-hidden-cluster --cluster-scope institution_origin --target-topic lock_authority --start-year 590 --end-year 625 --seed 42\n";
    out << "  " << exe << " --access curator --query generate-artifacts-from-hidden-mutation --cluster-scope institution_origin --target-topic lock_authority --start-year 590 --end-year 625 --target-year 625 --seed 42\n";
    out << "  " << exe << " --access curator --query materialize-hidden-mutation-artifact-candidate --candidate-shape ritual_notice --cluster-scope institution_origin --target-topic lock_authority --start-year 590 --end-year 625 --target-year 625 --seed 42\n";
    out << "  " << exe << " --access curator --query hidden-mutations\n";
    out << "  " << exe << " --query list-generation-targets\n";
    out << "  " << exe << " --query validate-civilization-specs --spec-file path/to/civilization_specs.json\n";
    out << "  " << exe << " --query list-civilization-specs --spec-file path/to/civilization_specs.json\n";
    out << "  " << exe << " --query show-civilization-spec --spec-file path/to/civilization_specs.json --civilization-id marsh_citadel\n";
    out << "  " << exe << " --query list-civilization-tags --spec-file path/to/civilization_specs.json\n";
    out << "  " << exe << " --query list-civilizations-by-tag --spec-file path/to/civilization_specs.json --tag river_delta\n";
    out << "  " << exe << " --query validate-civilization-tags --spec-file path/to/civilization_specs.json\n";
    out << "  " << exe << " --query bootstrap-civilization --spec-file path/to/civilization_specs.json --civilization-id marsh_citadel\n";
    out << "  " << exe << " --query list-generation-targets\n";
    out << "  " << exe << " --runtime fixed-fixture --query list-generation-targets\n";
    out << "  " << exe << " --query list-generation-targets --civilization-id ash_steppe\n";
    out << "  " << exe << " --query list-generation-targets --spec-file path/to/civilization_specs.json --civilization-id custom_id\n";
    out << "  " << exe << " --access curator --query hidden-cluster --spec-file path/to/civilization_specs.json --civilization-id ash_steppe --target-topic authority_conflict_0 --start-year 250 --end-year 380 --seed 42\n";
    out << "  " << exe << " --query list-golden-fixtures\n";
    out << "  " << exe << " --query show-golden-fixture --fixture-id fixture.default_archive\n";
    out << "  " << exe << " --query archive-snapshot --fixture-id fixture.default_archive\n";
    out << "  " << exe << " --query compare-archive-snapshots --fixture-id fixture.default_archive\n";
    out << "  " << exe << " --access curator --query list-evidence-potentials\n";
    out << "  " << exe << " --query evidence-potential-summary\n";
    out << "  " << exe << " --query knowledge-horizon-summary\n";
    out << "  " << exe << " --access curator --query list-knowledge-horizon-findings\n";
    out << "  " << exe << " --query contradiction-budget-summary\n";
    out << "  " << exe << " --access curator --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive\n";
    out << "  " << exe << " --query candidate-artifact-plan-summary\n";
    out << "  " << exe << " --access curator --query list-candidate-artifact-plans\n";
    out << "  " << exe << " --query candidate-artifact-plan-evaluation-summary\n";
    out << "  " << exe << " --access curator --query list-candidate-artifact-plan-evaluations\n";
    out << "  " << exe << " --query candidate-artifact-proposal-summary\n";
    out << "  " << exe << " --access curator --query list-candidate-artifact-proposals\n";
    out << "\n" << seed_behavior_note() << "\n";
    return out.str();
}

[[nodiscard]] std::string format_report(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::ostringstream out;
    out << "Impossible Archive Engine MVP\n";
    out << "seed=" << state.seed << "; access=" << to_string(access) << "; archive_year=" << archive_year_text(archive_year) << "\n";
    if (state.civilization_source.has_value()) {
        const CivilizationRuntimeSource& source = *state.civilization_source;
        out << "runtime: spec-selected\n";
        out << "civilization_id: " << source.civilization_id << "\n";
        out << "civilization: " << source.display_name << "\n";
        out << "catalog_id: " << (source.catalog_id.empty() ? std::string{"unspecified"} : source.catalog_id) << "\n";
        out << "schema_version: " << (source.schema_version.empty() ? std::string{"unspecified"} : source.schema_version) << "\n";
    } else {
        out << "runtime: fixed fixture regression mode\n";
    }
    out << seed_behavior_note() << "\n\n";
    out << answer_what_happened(state, access, archive_year) << "\n";
    if (can_view(access, AccessLevel::Scholar)) {
        out << format_theories(state, access, archive_year) << "\n";
    }
    out << format_mysteries(state, access, archive_year) << "\n";
    if (can_view(access, AccessLevel::Curator)) {
        out << format_originality(state, access) << "\n";
        out << format_hidden_truth_mutations(state, access) << "\n";
        out << format_evidence_potential_summary(state, access) << "\n";
        out << format_knowledge_horizon_summary(state, access) << "\n";
        out << format_contradiction_budget_summary(state, access) << "\n";
        out << format_candidate_artifact_plan_summary(state, access) << "\n";
        out << format_candidate_artifact_plan_evaluation_summary(state, access) << "\n";
        out << format_candidate_artifact_proposal_summary(state, access) << "\n";
        out << format_candidate_artifact_proposal_audit_summary(state, access) << "\n";
        out << format_candidate_artifact_draft_summary(state, access) << "\n";
        out << format_candidate_artifact_draft_review_summary(state, access) << "\n";
    }
    out << format_discoveries(state, access, archive_year) << "\n";
    out << format_artifacts(state, access, archive_year) << "\n";
    out << format_claims(state, access, archive_year) << "\n";
    out << format_contradictions(state, access, archive_year) << "\n";
    if (can_view(access, AccessLevel::Scholar)) {
        out << format_anachronisms(state, access, archive_year) << "\n";
    }
    if (can_view(access, AccessLevel::Canon)) {
        out << format_hidden_timeline(state, access) << "\n";
    }
    out << format_validation(state, access);
    return out.str();
}


[[nodiscard]] std::vector<std::string> split_session_command_line(std::string_view line) {
    std::istringstream input{std::string(line)};
    std::vector<std::string> tokens;
    std::string token;
    while (input >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

[[nodiscard]] CliArgs make_session_cli_args(const std::vector<std::string>& tokens) {
    CliArgs args;
    args.reserve(tokens.size() + 1U);
    args.push_back("session-query");
    for (const std::string& token : tokens) {
        args.push_back(token);
    }
    return args;
}

[[nodiscard]] CliOptions runtime_session_initialization_options(CliOptions options) {
    options.runtime_session = false;
    options.query = "report";
    return options;
}

[[nodiscard]] std::string runtime_session_source_label(const RuntimeStateSelectionResult& selection) {
    std::ostringstream out;
    out << (selection.selection.mode == ArchiveRuntimeMode::SpecSelected ? "spec-selected" : "fixed-fixture");
    if (!selection.selection.spec_file.empty()) {
        out << ":" << selection.selection.spec_file;
    }
    if (!selection.selection.civilization_id.empty()) {
        out << ":" << selection.selection.civilization_id;
    }
    return out.str();
}

[[nodiscard]] bool is_runtime_session_end_command(std::string_view query) {
    return query == "end-session" || query == "quit" || query == "exit";
}

[[nodiscard]] bool is_shared_read_only_state_query(std::string_view query) {
    return query == "report" ||
           query == "candidate-artifact-draft-summary" ||
           query == "validate-candidate-artifact-drafts" ||
           query == "list-candidate-artifact-drafts" ||
           query == "show-candidate-artifact-draft" ||
           query == "candidate-artifact-draft-review-summary" ||
           query == "validate-candidate-artifact-draft-reviews" ||
           query == "list-candidate-artifact-draft-reviews" ||
           query == "show-candidate-artifact-draft-review" ||
           query == "control-layer-audit-summary" ||
           query == "validate-control-layer-audit" ||
           query == "list-control-layer-audit-entries" ||
           query == "show-control-layer-audit-entry" ||
           query == "evidence-potential-summary" ||
           query == "validate-evidence-potentials" ||
           query == "contradiction-budget-summary" ||
           query == "validate-contradiction-budget";
}

[[nodiscard]] std::optional<FormattedCommandResult> format_shared_read_only_state_query_result(const ArchiveEngineState& state,
                                                                                               const CliOptions& options) {
    if (options.query == "report") {
        return FormattedCommandResult{format_report(state, options.access, options.archive_year), EXIT_SUCCESS};
    }
    if (options.query == "candidate-artifact-draft-summary") {
        return FormattedCommandResult{format_candidate_artifact_draft_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-candidate-artifact-drafts") {
        return FormattedCommandResult{format_candidate_artifact_draft_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-candidate-artifact-drafts") {
        return FormattedCommandResult{format_candidate_artifact_draft_list(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-candidate-artifact-draft") {
        return FormattedCommandResult{format_candidate_artifact_draft_detail(state, options.access, options.candidate_artifact_draft_id), EXIT_SUCCESS};
    }
    if (options.query == "candidate-artifact-draft-review-summary") {
        return FormattedCommandResult{format_candidate_artifact_draft_review_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-candidate-artifact-draft-reviews") {
        return FormattedCommandResult{format_candidate_artifact_draft_review_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-candidate-artifact-draft-reviews") {
        return FormattedCommandResult{format_candidate_artifact_draft_review_list(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-candidate-artifact-draft-review") {
        return FormattedCommandResult{format_candidate_artifact_draft_review_detail(state, options.access, options.candidate_artifact_draft_review_id), EXIT_SUCCESS};
    }
    if (options.query == "control-layer-audit-summary") {
        return FormattedCommandResult{format_control_layer_audit_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-control-layer-audit") {
        return FormattedCommandResult{format_control_layer_audit_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-control-layer-audit-entries") {
        return FormattedCommandResult{format_control_layer_audit_entries(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-control-layer-audit-entry") {
        return FormattedCommandResult{format_control_layer_audit_entry_detail(state, options.access, options.control_layer_audit_entry_id), EXIT_SUCCESS};
    }
    if (options.query == "evidence-potential-summary") {
        return FormattedCommandResult{format_evidence_potential_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-evidence-potentials") {
        return FormattedCommandResult{format_evidence_potential_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "contradiction-budget-summary") {
        return FormattedCommandResult{format_contradiction_budget_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-contradiction-budget") {
        return FormattedCommandResult{format_contradiction_budget_validation(state, options.access), EXIT_SUCCESS};
    }
    return std::nullopt;
}

[[nodiscard]] bool is_runtime_session_cli_supported_query(std::string_view query) {
    return is_shared_read_only_state_query(query);
}

[[nodiscard]] FormattedCommandResult format_runtime_session_state_query(const RuntimeSession& session, const CliOptions& options) {
    const std::optional<FormattedCommandResult> shared = format_shared_read_only_state_query_result(session.state, options);
    if (shared.has_value()) {
        return *shared;
    }
    return {"RuntimeSession unsupported query:\n- query: " + options.query + "\n", EXIT_FAILURE};
}

void record_runtime_session_unsupported(RuntimeSession& session, std::string_view query) {
    ++session.rejected_query_count;
    session.command_history_summary.push_back("unsupported:" + std::string(query));
    constexpr std::size_t max_history = 32U;
    if (session.command_history_summary.size() > max_history) {
        session.command_history_summary.erase(session.command_history_summary.begin());
    }
}

int run_runtime_session_cli(const CliOptions& options) {
    const CliOptions init_options = runtime_session_initialization_options(options);
    const RuntimeStateSelectionResult runtime_state = build_runtime_state_for_query(init_options);
    if (!runtime_state.ok) {
        std::cout << format_runtime_selection_errors(init_options, runtime_state);
        return EXIT_FAILURE;
    }

    RuntimeSession session = initialize_runtime_session(runtime_state.state, runtime_session_source_label(runtime_state));
    std::cout << "RuntimeSession initialized:\n";
    std::cout << format_runtime_session_summary(session);

    std::string line;
    while (std::getline(std::cin, line)) {
        const std::vector<std::string> tokens = split_session_command_line(line);
        if (tokens.empty()) {
            continue;
        }
        if (tokens.size() == 1U && is_runtime_session_end_command(tokens.front())) {
            end_runtime_session(session, "explicit_end_command");
            std::cout << "RuntimeSession ended:\n";
            std::cout << format_runtime_session_summary(session);
            return EXIT_SUCCESS;
        }

        CliOptions query_options;
        try {
            query_options = parse_cli(make_session_cli_args(tokens));
        } catch (const std::exception& ex) {
            ++session.rejected_query_count;
            std::cout << "RuntimeSession query rejected:\n";
            std::cout << "- query: <parse-error>\n";
            std::cout << "- policy: denied_unknown\n";
            std::cout << "- error: " << ex.what() << "\n";
            std::cout << "- session_active: " << (session.active() ? "true" : "false") << "\n";
            continue;
        }

        if (is_runtime_session_end_command(query_options.query)) {
            end_runtime_session(session, "explicit_end_query");
            std::cout << "RuntimeSession ended:\n";
            std::cout << format_runtime_session_summary(session);
            return EXIT_SUCCESS;
        }

        const RuntimeSessionQueryPolicy policy = classify_runtime_session_query(query_options.query);
        if (policy != RuntimeSessionQueryPolicy::AllowedReadOnly) {
            const RuntimeSessionCommandResult rejected = record_runtime_session_query(session, query_options.query);
            std::cout << "RuntimeSession query rejected:\n";
            std::cout << "- query: " << rejected.query_name << "\n";
            std::cout << "- policy: " << to_string(rejected.policy) << "\n";
            std::cout << "- message: " << rejected.message << "\n";
            std::cout << "- session_active: " << (session.active() ? "true" : "false") << "\n";
            continue;
        }

        if (!is_runtime_session_cli_supported_query(query_options.query)) {
            record_runtime_session_unsupported(session, query_options.query);
            std::cout << "RuntimeSession query rejected:\n";
            std::cout << "- query: " << query_options.query << "\n";
            std::cout << "- policy: allowed_read_only_but_not_in_cli_loop_subset\n";
            std::cout << "- message: read-only query is not implemented in the v29.2 session CLI loop subset yet\n";
            std::cout << "- session_active: " << (session.active() ? "true" : "false") << "\n";
            continue;
        }

        const RuntimeSessionCommandResult accepted = record_runtime_session_query(session, query_options.query);
        std::cout << "RuntimeSession query accepted:\n";
        std::cout << "- query: " << accepted.query_name << "\n";
        std::cout << "- policy: " << to_string(accepted.policy) << "\n";
        const FormattedCommandResult result = format_runtime_session_state_query(session, query_options);
        std::cout << result.text;
    }

    end_runtime_session(session, "input_eof");
    std::cout << "RuntimeSession ended:\n";
    std::cout << format_runtime_session_summary(session);
    return EXIT_SUCCESS;
}

int run_cli(const CliArgs& args) {
    const CliOptions options = parse_cli(args);

    if (options.self_test) {
        return run_self_tests();
    }

    if (options.query == "help") {
        const std::string_view exe = args.empty() ? std::string_view{"impossible_archive_mvp"} : args.front();
        std::cout << usage(exe);
        return EXIT_SUCCESS;
    }

    if (options.runtime_session || options.query == "runtime-session") {
        return run_runtime_session_cli(options);
    }

    if (is_civilization_spec_query(options.query)) {
        const FormattedCommandResult result = format_civilization_spec_query_result(options);
        std::cout << result.text;
        return result.exit_code;
    }

    if (is_golden_fixture_query(options.query)) {
        const FormattedCommandResult result = format_golden_fixture_query_result(options);
        if (result.exit_code == EXIT_SUCCESS) {
            std::cout << result.text;
        } else {
            std::cerr << result.text;
        }
        return result.exit_code;
    }

    const RuntimeStateSelectionResult runtime_state = build_runtime_state_for_query(options);
    if (!runtime_state.ok) {
        std::cout << format_runtime_selection_errors(options, runtime_state);
        return EXIT_FAILURE;
    }
    ArchiveEngineState state = runtime_state.state;

    if (const std::optional<FormattedCommandResult> shared = format_shared_read_only_state_query_result(state, options)) {
        std::cout << shared->text;
        return shared->exit_code;
    }

    if (options.query == "report") {
        std::cout << format_report(state, options.access, options.archive_year);
    } else if (options.query == "bootstrap-civilization") {
        std::cout << format_civilization_bootstrap_summary(state, options.access);
    } else if (options.query == "truth") {
        std::cout << answer_what_happened(state, options.access, options.archive_year);
    } else if (options.query == "artifacts") {
        std::cout << format_artifacts(state, options.access, options.archive_year);
    } else if (options.query == "claims") {
        std::cout << format_claims(state, options.access, options.archive_year);
    } else if (options.query == "contradictions") {
        std::cout << format_contradictions(state, options.access, options.archive_year);
    } else if (options.query == "anachronisms") {
        std::cout << format_anachronisms(state, options.access, options.archive_year);
    } else if (options.query == "discoveries") {
        std::cout << format_discoveries(state, options.access, options.archive_year);
    } else if (options.query == "mysteries") {
        std::cout << format_mysteries(state, options.access, options.archive_year);
    } else if (options.query == "originality") {
        std::cout << format_originality(state, options.access);
    } else if (options.query == "candidate") {
        std::cout << format_candidate_query(state, options.access, options.candidate_id);
    } else if (options.query == "materialize") {
        std::cout << format_materialization_query(state, options.access, options.candidate_id);
    } else if (options.query == "list-generation-targets") {
        std::cout << format_generation_targets_for_state(state, options.access);
    } else if (options.query == "generate-candidates") {
        std::cout << format_generated_candidates(state, options.access, CandidateGenerationRequest{options.generation_strategy, options.target_year, options.target_topic, options.seed});
    } else if (options.query == "evaluate-dossier") {
        std::cout << format_dossier_evaluation(state, options.access, CandidateGenerationRequest{options.generation_strategy, options.target_year, options.target_topic, options.seed});
    } else if (options.query == "materialize-generated") {
        std::cout << format_generated_materialization_query(state, options.access, CandidateGenerationRequest{options.generation_strategy, options.target_year, options.target_topic, options.seed}, static_cast<std::size_t>(options.candidate_index));
    } else if (options.query == "materialize-dossier-candidate") {
        const CandidateGenerationRequest request{options.generation_strategy, options.target_year, options.target_topic, options.seed};
        if (options.candidate_index_supplied) {
            std::cout << format_dossier_materialization_query(state, options.access, request, static_cast<std::size_t>(options.candidate_index));
        } else if (options.candidate_role.has_value()) {
            std::cout << format_dossier_materialization_query_by_role(state, options.access, request, *options.candidate_role);
        } else {
            std::cout << "Dossier candidate materialization visible to " << to_string(options.access) << ":\n";
            std::cout << "- decision: Reject\n- mutated: false\n- explanation: provide --candidate-index or --candidate-role; archive state was not mutated.\n";
        }
    } else if (options.query == "plan-dossier-selection") {
        const CandidateGenerationRequest request{options.generation_strategy, options.target_year, options.target_topic, options.seed};
        std::cout << format_dossier_selection_plan(state, options.access, request, resolve_selection_indices(state, request, options));
    } else if (options.query == "materialize-dossier-selection") {
        const CandidateGenerationRequest request{options.generation_strategy, options.target_year, options.target_topic, options.seed};
        std::cout << format_dossier_selection_materialization_query(state, options.access, request, resolve_selection_indices(state, request, options));
    } else if (options.query == "hidden-proposals") {
        std::cout << format_hidden_proposals(state, options.access, CandidateGenerationRequest{options.generation_strategy, options.target_year, options.target_topic, options.seed});
    } else if (options.query == "evaluate-hidden-proposal") {
        std::cout << format_hidden_proposal_evaluation(state, options.access, CandidateGenerationRequest{options.generation_strategy, options.target_year, options.target_topic, options.seed}, static_cast<std::size_t>(options.candidate_index));
    } else if (options.query == "plan-hidden-proposal") {
        std::cout << format_hidden_proposal_migration_plan(state, options.access, CandidateGenerationRequest{options.generation_strategy, options.target_year, options.target_topic, options.seed}, static_cast<std::size_t>(options.candidate_index));
    } else if (options.query == "hidden-cluster") {
        std::cout << format_hidden_timeline_cluster(state, options.access, HiddenTimelineClusterRequest{options.cluster_scope, options.target_topic, options.start_year, options.end_year, options.seed});
    } else if (options.query == "materialize-hidden-cluster") {
        std::cout << format_hidden_cluster_materialization_query(state, options.access, HiddenTimelineClusterRequest{options.cluster_scope, options.target_topic, options.start_year, options.end_year, options.seed});
    } else if (options.query == "hidden-mutations") {
        std::cout << format_hidden_truth_mutations(state, options.access);
    } else if (options.query == "list-evidence-potentials") {
        std::cout << format_evidence_potential_list(state, options.access);
    } else if (options.query == "show-evidence-potential") {
        std::cout << format_evidence_potential_detail(state, options.access, options.evidence_potential_id);
    } else if (options.query == "validate-evidence-potentials") {
        std::cout << format_evidence_potential_validation(state, options.access);
    } else if (options.query == "evidence-potential-summary") {
        std::cout << format_evidence_potential_summary(state, options.access);
    } else if (options.query == "validate-knowledge-horizon") {
        std::cout << format_knowledge_horizon_validation(state, options.access);
    } else if (options.query == "knowledge-horizon-summary") {
        std::cout << format_knowledge_horizon_summary(state, options.access);
    } else if (options.query == "list-knowledge-horizon-findings") {
        std::cout << format_knowledge_horizon_findings(state, options.access);
    } else if (options.query == "show-knowledge-horizon-finding") {
        std::cout << format_knowledge_horizon_finding_detail(state, options.access, options.knowledge_horizon_finding_id);
    } else if (options.query == "contradiction-budget-summary") {
        std::cout << format_contradiction_budget_summary(state, options.access);
    } else if (options.query == "list-contradiction-budget-buckets") {
        std::cout << format_contradiction_budget_buckets(state, options.access);
    } else if (options.query == "show-contradiction-budget-bucket") {
        std::cout << format_contradiction_budget_bucket_detail(state, options.access, options.contradiction_budget_bucket_id);
    } else if (options.query == "validate-contradiction-budget") {
        std::cout << format_contradiction_budget_validation(state, options.access);
    } else if (options.query == "candidate-artifact-plan-summary") {
        std::cout << format_candidate_artifact_plan_summary(state, options.access);
    } else if (options.query == "list-candidate-artifact-plans") {
        std::cout << format_candidate_artifact_plan_list(state, options.access);
    } else if (options.query == "show-candidate-artifact-plan") {
        std::cout << format_candidate_artifact_plan_detail(state, options.access, options.candidate_artifact_plan_id);
    } else if (options.query == "validate-candidate-artifact-plans") {
        std::cout << format_candidate_artifact_plan_validation(state, options.access);
    } else if (options.query == "candidate-artifact-plan-evaluation-summary") {
        std::cout << format_candidate_artifact_plan_evaluation_summary(state, options.access);
    } else if (options.query == "list-candidate-artifact-plan-evaluations") {
        std::cout << format_candidate_artifact_plan_evaluation_list(state, options.access);
    } else if (options.query == "show-candidate-artifact-plan-evaluation") {
        std::cout << format_candidate_artifact_plan_evaluation_detail(state, options.access, options.candidate_artifact_plan_evaluation_id);
    } else if (options.query == "validate-candidate-artifact-plan-evaluations") {
        std::cout << format_candidate_artifact_plan_evaluation_validation(state, options.access);
    } else if (options.query == "candidate-artifact-proposal-summary") {
        std::cout << format_candidate_artifact_proposal_summary(state, options.access);
    } else if (options.query == "list-candidate-artifact-proposals") {
        std::cout << format_candidate_artifact_proposal_list(state, options.access);
    } else if (options.query == "show-candidate-artifact-proposal") {
        std::cout << format_candidate_artifact_proposal_detail(state, options.access, options.candidate_artifact_proposal_id);
    } else if (options.query == "validate-candidate-artifact-proposals") {
        std::cout << format_candidate_artifact_proposal_validation(state, options.access);
    } else if (options.query == "candidate-artifact-proposal-audit-summary") {
        std::cout << format_candidate_artifact_proposal_audit_summary(state, options.access);
    } else if (options.query == "list-candidate-artifact-proposal-audits") {
        std::cout << format_candidate_artifact_proposal_audit_list(state, options.access);
    } else if (options.query == "show-candidate-artifact-proposal-audit") {
        std::cout << format_candidate_artifact_proposal_audit_detail(state, options.access, options.candidate_artifact_proposal_audit_id);
    } else if (options.query == "validate-candidate-artifact-proposal-audits") {
        std::cout << format_candidate_artifact_proposal_audit_validation(state, options.access);
    } else if (options.query == "candidate-artifact-draft-summary") {
        std::cout << format_candidate_artifact_draft_summary(state, options.access);
    } else if (options.query == "list-candidate-artifact-drafts") {
        std::cout << format_candidate_artifact_draft_list(state, options.access);
    } else if (options.query == "show-candidate-artifact-draft") {
        std::cout << format_candidate_artifact_draft_detail(state, options.access, options.candidate_artifact_draft_id);
    } else if (options.query == "validate-candidate-artifact-drafts") {
        std::cout << format_candidate_artifact_draft_validation(state, options.access);
    } else if (options.query == "candidate-artifact-draft-review-summary") {
        std::cout << format_candidate_artifact_draft_review_summary(state, options.access);
    } else if (options.query == "list-candidate-artifact-draft-reviews") {
        std::cout << format_candidate_artifact_draft_review_list(state, options.access);
    } else if (options.query == "show-candidate-artifact-draft-review") {
        std::cout << format_candidate_artifact_draft_review_detail(state, options.access, options.candidate_artifact_draft_review_id);
    } else if (options.query == "validate-candidate-artifact-draft-reviews") {
        std::cout << format_candidate_artifact_draft_review_validation(state, options.access);
    } else if (options.query == "control-layer-audit-summary") {
        std::cout << format_control_layer_audit_summary(state, options.access);
    } else if (options.query == "list-control-layer-audit-entries") {
        std::cout << format_control_layer_audit_entries(state, options.access);
    } else if (options.query == "show-control-layer-audit-entry") {
        std::cout << format_control_layer_audit_entry_detail(state, options.access, options.control_layer_audit_entry_id);
    } else if (options.query == "validate-control-layer-audit") {
        std::cout << format_control_layer_audit_validation(state, options.access);
    } else if (options.query == "generate-artifacts-from-hidden-mutation") {
        std::cout << format_hidden_mutation_artifact_generation_query(
            state,
            options.access,
            HiddenTimelineClusterRequest{options.cluster_scope, options.target_topic, options.start_year, options.end_year, options.seed},
            CandidateGenerationRequest{options.generation_strategy, options.target_year, options.target_topic, options.seed}
        );
    } else if (options.query == "materialize-hidden-mutation-artifact-candidate") {
        std::optional<std::size_t> selected_index;
        if (options.candidate_index_supplied) {
            selected_index = static_cast<std::size_t>(options.candidate_index);
        }
        std::cout << format_hidden_mutation_artifact_candidate_materialization_query(
            state,
            options.access,
            HiddenTimelineClusterRequest{options.cluster_scope, options.target_topic, options.start_year, options.end_year, options.seed},
            CandidateGenerationRequest{options.generation_strategy, options.target_year, options.target_topic, options.seed},
            options.candidate_shape,
            selected_index
        );
    } else if (options.query == "timeline") {
        std::cout << format_hidden_timeline(state, options.access);
    } else if (options.query == "validation") {
        std::cout << format_validation(state, options.access);
    } else if (options.query == "theories") {
        std::cout << format_theories(state, options.access, options.archive_year);
    } else {
        throw std::invalid_argument("unknown query: " + options.query);
    }

    return EXIT_SUCCESS;
}

int run_cli(int argc, char** argv) {
    return run_cli(make_cli_args(argc, argv));
}

} // namespace archive
