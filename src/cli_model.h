#pragma once
#include "archive_engine_state.h"
#include "candidate_model.h"
#include "hidden_truth_model.h"
#include <string_view>
#include <vector>

namespace archive {

enum class ArchiveRuntimeMode {
    FixedFixture,
    SpecSelected
};

struct DefaultRuntimeConfig {
    std::string default_spec_file = "examples/40_civilization_specs_v1_1.json";
    std::string default_civilization_id = "marsh_citadel";
};

struct ArchiveRuntimeSelection {
    ArchiveRuntimeMode mode = ArchiveRuntimeMode::FixedFixture;
    std::string spec_file;
    bool spec_file_supplied = false;
    std::string civilization_id;
    bool civilization_id_supplied = false;
    bool explicit_runtime_mode = false;
};

struct RuntimeStateSelectionResult {
    ArchiveEngineState state;
    ArchiveRuntimeSelection selection;
    bool ok = false;
    bool usage_error = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] bool spec_selected() const {
        return selection.mode == ArchiveRuntimeMode::SpecSelected;
    }
};

using CliArgs = std::vector<std::string_view>;

struct CliOptions {
    std::uint64_t seed = 42;
    bool seed_supplied = false;
    AccessLevel access = AccessLevel::Public;
    int archive_year = kOpenEndedYear;
    bool archive_year_supplied = false;
    std::string query = "report";
    std::string candidate_id = "generic_moon_cult";
    CandidateGenerationStrategy generation_strategy = CandidateGenerationStrategy::AddCorroboratingFragment;
    std::string target_topic = "lock_authority";
    int target_year = 620;
    int candidate_index = 0;
    bool candidate_index_supplied = false;
    std::optional<GeneratedCandidateRole> candidate_role;
    std::optional<HiddenMutationArtifactCandidateShape> candidate_shape;
    std::vector<std::size_t> candidate_indices;
    std::vector<GeneratedCandidateRole> candidate_roles;
    HiddenClusterScope cluster_scope = HiddenClusterScope::InstitutionOrigin;
    int start_year = 590;
    int end_year = 625;
    bool self_test = false;
    std::string spec_file;
    bool spec_file_supplied = false;
    std::string civilization_id;
    bool civilization_id_supplied = false;
    std::string tag;
    std::string fragment_id;
    std::string fixture_id = "fixture.default_archive";
    std::string evidence_potential_id;
    std::string knowledge_horizon_finding_id;
    std::string contradiction_budget_bucket_id = "contradiction_budget.archive";
    std::string candidate_artifact_plan_id;
    std::string candidate_artifact_plan_evaluation_id;
    std::string candidate_artifact_proposal_id;
    std::string candidate_artifact_proposal_audit_id;
    std::string candidate_artifact_draft_id;
    std::string control_layer_audit_entry_id;
    ArchiveRuntimeSelection runtime_selection;
};

int run_self_tests();
[[nodiscard]] DefaultRuntimeConfig default_runtime_config();
[[nodiscard]] ArchiveRuntimeMode parse_archive_runtime_mode(std::string_view text);
[[nodiscard]] CliArgs make_cli_args(int argc, char** argv);
[[nodiscard]] CliOptions parse_cli(const CliArgs& args);
[[nodiscard]] CliOptions parse_cli(int argc, char** argv);
[[nodiscard]] RuntimeStateSelectionResult build_runtime_state_for_query(const CliOptions& options);
[[nodiscard]] std::string seed_behavior_note();
[[nodiscard]] std::string usage(std::string_view exe);
int run_cli(const CliArgs& args);
int run_cli(int argc, char** argv);

} // namespace archive
