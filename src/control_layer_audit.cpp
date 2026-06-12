#include "control_layer_audit_api.h"
#include "diagnostic_access_policy.h"

#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {

void add(std::vector<std::string>& values, std::initializer_list<const char*> entries) {
    for (const char* entry : entries) {
        values.emplace_back(entry);
    }
}

[[nodiscard]] ControlLayerAuditEntry make_entry(
    std::string id,
    std::string name,
    ControlLayerKind kind,
    ControlLayerPersistence persistence,
    ControlLayerBehavior behavior,
    ControlLayerRisk risk,
    bool access_gated,
    bool public_detail_gated,
    bool snapshot_covered,
    bool summary_digest_covered,
    bool smoke_covered,
    bool self_test_covered,
    bool full_state_validation_covered,
    bool can_mutate_state,
    bool should_remain_inert
) {
    ControlLayerAuditEntry entry;
    entry.id = std::move(id);
    entry.name = std::move(name);
    entry.kind = kind;
    entry.persistence = persistence;
    entry.behavior = behavior;
    entry.risk = risk;
    entry.access_gated = access_gated;
    entry.public_detail_gated = public_detail_gated;
    entry.snapshot_covered = snapshot_covered;
    entry.summary_digest_covered = summary_digest_covered;
    entry.smoke_covered = smoke_covered;
    entry.self_test_covered = self_test_covered;
    entry.full_state_validation_covered = full_state_validation_covered;
    entry.can_mutate_state = can_mutate_state;
    entry.should_remain_inert = should_remain_inert;
    return entry;
}

[[nodiscard]] const ControlLayerAuditEntry* find_entry(const std::vector<ControlLayerAuditEntry>& entries, const std::string& id) {
    const auto it = std::find_if(entries.begin(), entries.end(), [&](const ControlLayerAuditEntry& entry) {
        return entry.id == id;
    });
    return it == entries.end() ? nullptr : &*it;
}

[[nodiscard]] std::vector<ControlLayerAuditEntry> entries_for_formatting(const ArchiveEngineState& state) {
    if (!state.control_layer_audit_entries.empty()) {
        return state.control_layer_audit_entries;
    }
    return build_control_layer_audit_report().entries;
}

[[nodiscard]] bool entry_has_access_note(const ControlLayerAuditEntry& entry) {
    return std::any_of(entry.notes.begin(), entry.notes.end(), [](const std::string& note) {
        return contains_substr(note, "access") || contains_substr(note, "Public") || contains_substr(note, "public") || contains_substr(note, "curator");
    });
}

void append_counts(std::ostringstream& out, const std::map<std::string, std::size_t>& counts) {
    for (const auto& [label, count] : counts) {
        out << "- " << label << ": " << count << "\n";
    }
}

} // namespace

[[nodiscard]] std::string to_string(ControlLayerKind kind) {
    switch (kind) {
        case ControlLayerKind::CoreState: return "core_state";
        case ControlLayerKind::Fixture: return "fixture";
        case ControlLayerKind::Snapshot: return "snapshot";
        case ControlLayerKind::Validation: return "validation";
        case ControlLayerKind::Telemetry: return "telemetry";
        case ControlLayerKind::Planning: return "planning";
        case ControlLayerKind::Evaluation: return "evaluation";
        case ControlLayerKind::Proposal: return "proposal";
        case ControlLayerKind::Audit: return "audit";
        case ControlLayerKind::MutationWorkflow: return "mutation_workflow";
        case ControlLayerKind::Formatting: return "formatting";
        case ControlLayerKind::CLI: return "cli";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ControlLayerPersistence persistence) {
    switch (persistence) {
        case ControlLayerPersistence::PersistentState: return "persistent_state";
        case ControlLayerPersistence::DerivedCachedState: return "derived_cached_state";
        case ControlLayerPersistence::DerivedViewOnly: return "derived_view_only";
        case ControlLayerPersistence::ReportOnly: return "report_only";
        case ControlLayerPersistence::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ControlLayerBehavior behavior) {
    switch (behavior) {
        case ControlLayerBehavior::RuntimeEnforced: return "runtime_enforced";
        case ControlLayerBehavior::ValidationOnly: return "validation_only";
        case ControlLayerBehavior::TelemetryOnly: return "telemetry_only";
        case ControlLayerBehavior::PlanningOnly: return "planning_only";
        case ControlLayerBehavior::EvaluationOnly: return "evaluation_only";
        case ControlLayerBehavior::ProposalOnly: return "proposal_only";
        case ControlLayerBehavior::AuditOnly: return "audit_only";
        case ControlLayerBehavior::FormattingOnly: return "formatting_only";
        case ControlLayerBehavior::MutationCapable: return "mutation_capable";
        case ControlLayerBehavior::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ControlLayerRisk risk) {
    switch (risk) {
        case ControlLayerRisk::Low: return "low";
        case ControlLayerRisk::Moderate: return "moderate";
        case ControlLayerRisk::High: return "high";
        case ControlLayerRisk::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] ControlLayerAuditReport build_control_layer_audit_report() {
    ControlLayerAuditReport report;
    auto push = [&](ControlLayerAuditEntry entry) {
        report.entries.push_back(std::move(entry));
    };

    {
        auto e = make_entry("control_layer.hidden_truth", "HiddenTruthGraph", ControlLayerKind::CoreState,
                            ControlLayerPersistence::PersistentState, ControlLayerBehavior::RuntimeEnforced,
                            ControlLayerRisk::High, true, true, true, true, false, true, true, false, false);
        add(e.primary_files, {"src/hidden_truth_model.h", "src/engine_fixture.cpp", "src/validation.cpp", "src/hidden_clusters.cpp"});
        add(e.validation_functions, {"validate_hidden_graph", "validate_full_state"});
        add(e.snapshot_fields, {"hidden_entity_count", "hidden_event_count", "hidden_mutation_record_count"});
        add(e.known_gaps, {"Hidden truth is still in-memory only; no persistence layer exists.", "Only explicit mutation workflows can alter hidden truth."});
        add(e.notes, {"Access to hidden truth is gated through canon/debug/curator formatting surfaces.", "Mutation-capable workflows are classified separately."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.public_archive", "PublicArchive", ControlLayerKind::CoreState,
                            ControlLayerPersistence::PersistentState, ControlLayerBehavior::MutationCapable,
                            ControlLayerRisk::High, true, true, true, true, false, true, true, true, false);
        add(e.primary_files, {"src/public_archive_model.h", "src/artifact_voice_and_views.cpp", "src/candidates_materialization.cpp"});
        add(e.validation_functions, {"PublicArchive::validate_metadata", "validate_cross_references", "validate_full_state"});
        add(e.snapshot_fields, {"public_artifact_count", "public_claim_count", "contradiction_count"});
        add(e.known_gaps, {"PublicArchive can mutate through explicit materialization paths; no persistence layer exists."});
        add(e.notes, {"PublicArchive::add_claim_to_artifact owns ordinary claim/artifact relationship invariants.", "PublicArchive::add_claim is a low-level import/bootstrap path."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.golden_fixtures", "GoldenFixtureWorlds", ControlLayerKind::Fixture,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::RuntimeEnforced,
                            ControlLayerRisk::Low, false, false, true, true, true, true, false, false, false);
        add(e.primary_files, {"src/golden_fixtures.cpp", "src/golden_fixture_model.h"});
        add(e.cli_queries, {"list-golden-fixtures", "show-golden-fixture", "archive-snapshot", "compare-archive-snapshots"});
        add(e.snapshot_fields, {"fixture_seed", "state_seed", "fixture_archive_year", "effective_archive_year"});
        add(e.notes, {"Fixture-backed queries reject incompatible runtime overrides before state construction."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.archive_snapshot", "ArchiveSnapshot", ControlLayerKind::Snapshot,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::RuntimeEnforced,
                            ControlLayerRisk::Low, false, false, true, true, true, true, false, false, false);
        add(e.primary_files, {"src/archive_snapshot_model.h", "src/archive_snapshot.cpp"});
        add(e.cli_queries, {"archive-snapshot", "compare-archive-snapshots"});
        add(e.snapshot_fields, {"summary_digest", "validation_errors"});
        add(e.notes, {"Snapshot comparison is a same-fixture deterministic rebuild check, not arbitrary state diffing."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.fragments", "CivilizationSpecFragments", ControlLayerKind::Validation,
                            ControlLayerPersistence::ReportOnly, ControlLayerBehavior::ValidationOnly,
                            ControlLayerRisk::Moderate, false, false, true, true, true, true, false, false, true);
        add(e.primary_files, {"src/civilization_fragment_model.h", "src/civilization_fragments.cpp"});
        add(e.cli_queries, {"list-civilization-fragments", "show-civilization-fragment", "validate-civilization-fragments"});
        add(e.validation_functions, {"validate_civilization_fragments"});
        add(e.snapshot_fields, {"civilization_fragment_count"});
        add(e.known_gaps, {"Fragments remain inert catalog data; resolver/composition behavior is intentionally absent."});
        add(e.notes, {"Normal spec-selected runtime bootstrap does not fail solely because inert fragments are invalid."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.evidence_potential", "EvidencePotential", ControlLayerKind::Validation,
                            ControlLayerPersistence::DerivedCachedState, ControlLayerBehavior::ValidationOnly,
                            ControlLayerRisk::Moderate, true, true, true, true, true, true, true, false, true);
        add(e.primary_files, {"src/evidence_potential_model.h", "src/evidence_potential.cpp"});
        add(e.cli_queries, {"evidence-potential-summary", "list-evidence-potentials", "show-evidence-potential", "validate-evidence-potentials"});
        add(e.validation_functions, {"validate_evidence_potentials", "validate_full_state"});
        add(e.snapshot_fields, {"evidence_potential_count"});
        add(e.known_gaps, {"EvidencePotential describes possible evidence but does not create evidence."});
        add(e.notes, {"Curator/debug detail can inspect source IDs; public access is restricted."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.knowledge_horizon", "KnowledgeHorizon", ControlLayerKind::Validation,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::ValidationOnly,
                            ControlLayerRisk::Moderate, true, true, true, true, true, true, false, false, true);
        add(e.primary_files, {"src/knowledge_horizon_model.h", "src/knowledge_horizon.cpp"});
        add(e.cli_queries, {"knowledge-horizon-summary", "list-knowledge-horizon-findings", "show-knowledge-horizon-finding", "validate-knowledge-horizon"});
        add(e.validation_functions, {"validate_knowledge_horizon"});
        add(e.snapshot_fields, {"knowledge_horizon_finding_count", "knowledge_horizon_error_count"});
        add(e.notes, {"Public detail lookup returns found: false for hidden/inaccessible findings."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.contradiction_budget", "ContradictionBudget", ControlLayerKind::Telemetry,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::TelemetryOnly,
                            ControlLayerRisk::Low, true, true, true, true, true, true, false, false, true);
        add(e.primary_files, {"src/contradiction_budget_model.h", "src/contradiction_budget.cpp"});
        add(e.cli_queries, {"contradiction-budget-summary", "list-contradiction-budget-buckets", "show-contradiction-budget-bucket", "validate-contradiction-budget"});
        add(e.validation_functions, {"validate_contradiction_budget_report"});
        add(e.snapshot_fields, {"contradiction_budget_bucket_count", "contradiction_budget_over_budget_count", "contradiction_budget_generation_bug_count"});
        add(e.known_gaps, {"ContradictionBudget is telemetry only and does not enforce hard budgets."});
        add(e.notes, {"Public output redacts representative diagnostic IDs."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.candidate_artifact_plan", "CandidateArtifactPlan", ControlLayerKind::Planning,
                            ControlLayerPersistence::DerivedCachedState, ControlLayerBehavior::PlanningOnly,
                            ControlLayerRisk::Moderate, true, true, true, true, true, true, true, false, true);
        add(e.primary_files, {"src/candidate_artifact_plan_model.h", "src/candidate_artifact_plan.cpp"});
        add(e.cli_queries, {"candidate-artifact-plan-summary", "list-candidate-artifact-plans", "show-candidate-artifact-plan", "validate-candidate-artifact-plans"});
        add(e.validation_functions, {"validate_candidate_artifact_plans", "validate_full_state"});
        add(e.snapshot_fields, {"candidate_artifact_plan_count", "candidate_artifact_plan_blocked_count", "candidate_artifact_plan_curator_review_count"});
        add(e.notes, {"Plans are derived from EvidencePotential and never enable current materialization."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.candidate_artifact_plan_evaluation", "CandidateArtifactPlanEvaluation", ControlLayerKind::Evaluation,
                            ControlLayerPersistence::DerivedCachedState, ControlLayerBehavior::EvaluationOnly,
                            ControlLayerRisk::Moderate, true, true, true, true, true, true, true, false, true);
        add(e.primary_files, {"src/candidate_artifact_plan_evaluation_model.h", "src/candidate_artifact_plan_evaluation.cpp"});
        add(e.cli_queries, {"candidate-artifact-plan-evaluation-summary", "list-candidate-artifact-plan-evaluations", "show-candidate-artifact-plan-evaluation", "validate-candidate-artifact-plan-evaluations"});
        add(e.validation_functions, {"validate_candidate_artifact_plan_evaluations", "validate_full_state"});
        add(e.snapshot_fields, {"candidate_artifact_plan_evaluation_count", "candidate_artifact_plan_evaluation_pass_count", "candidate_artifact_plan_evaluation_blocked_count", "candidate_artifact_plan_evaluation_review_count"});
        add(e.notes, {"Evaluation Pass means evaluation-clean only; generation and materialization stay disabled."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.candidate_artifact_proposal", "CandidateArtifactProposal", ControlLayerKind::Proposal,
                            ControlLayerPersistence::DerivedCachedState, ControlLayerBehavior::ProposalOnly,
                            ControlLayerRisk::Moderate, true, true, true, true, true, true, true, false, true);
        add(e.primary_files, {"src/candidate_artifact_proposal_model.h", "src/candidate_artifact_proposal.cpp"});
        add(e.cli_queries, {"candidate-artifact-proposal-summary", "list-candidate-artifact-proposals", "show-candidate-artifact-proposal", "validate-candidate-artifact-proposals"});
        add(e.validation_functions, {"validate_candidate_artifact_proposals", "validate_full_state"});
        add(e.snapshot_fields, {"candidate_artifact_proposal_count", "candidate_artifact_proposal_draftable_count", "candidate_artifact_proposal_blocked_count", "candidate_artifact_proposal_review_count"});
        add(e.notes, {"Stored proposal state is access-neutral; safety is computed at formatting/query time."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.candidate_artifact_proposal_audit", "CandidateArtifactProposalAudit", ControlLayerKind::Audit,
                            ControlLayerPersistence::DerivedCachedState, ControlLayerBehavior::AuditOnly,
                            ControlLayerRisk::Moderate, true, true, true, true, true, true, true, false, true);
        add(e.primary_files, {"src/candidate_artifact_proposal_audit_model.h", "src/candidate_artifact_proposal_audit.cpp"});
        add(e.cli_queries, {"candidate-artifact-proposal-audit-summary", "list-candidate-artifact-proposal-audits", "show-candidate-artifact-proposal-audit", "validate-candidate-artifact-proposal-audits"});
        add(e.validation_functions, {"validate_candidate_artifact_proposal_audits", "validate_full_state"});
        add(e.snapshot_fields, {"candidate_artifact_proposal_audit_count", "candidate_artifact_proposal_audit_pass_count", "candidate_artifact_proposal_audit_blocked_count", "candidate_artifact_proposal_audit_review_count", "candidate_artifact_proposal_audit_revision_count"});
        add(e.notes, {"Audit is quality-gate inspection only and does not enable artifact text generation."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.candidate_generation", "CandidateGeneration", ControlLayerKind::Planning,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::PlanningOnly,
                            ControlLayerRisk::Moderate, true, true, false, false, true, true, false, false, false);
        add(e.primary_files, {"src/candidate_generation.cpp", "src/candidate_model.h"});
        add(e.cli_queries, {"generate-candidates", "evaluate-dossier", "list-generation-targets"});
        add(e.known_gaps, {"CandidateGeneration creates candidate objects but does not itself insert artifacts into PublicArchive."});
        add(e.notes, {"Generation output is still non-mutating until explicit materialization paths are invoked."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.candidate_materialization", "CandidateMaterialization", ControlLayerKind::MutationWorkflow,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::MutationCapable,
                            ControlLayerRisk::High, true, true, false, false, true, true, true, true, false);
        add(e.primary_files, {"src/candidates_materialization.cpp", "src/materialization_api.h"});
        add(e.cli_queries, {"materialize", "materialize-generated", "materialize-dossier-candidate", "materialize-dossier-selection"});
        add(e.validation_functions, {"validate_full_state"});
        add(e.known_gaps, {"Mutation is in-memory only and gated to curator/debug materialization workflows."});
        add(e.notes, {"This is a legacy mutation-capable surface and must remain gated."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.hidden_cluster_preview", "HiddenClusterPreview", ControlLayerKind::Planning,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::PlanningOnly,
                            ControlLayerRisk::Moderate, true, true, false, false, true, true, false, false, true);
        add(e.primary_files, {"src/hidden_clusters.cpp", "src/hidden_workflows_api.h"});
        add(e.cli_queries, {"hidden-cluster"});
        add(e.notes, {"Preview evaluates proposed hidden clusters without mutating live hidden truth."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.hidden_cluster_materialization", "HiddenClusterMaterialization", ControlLayerKind::MutationWorkflow,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::MutationCapable,
                            ControlLayerRisk::High, true, true, false, false, true, true, true, true, false);
        add(e.primary_files, {"src/hidden_clusters.cpp", "src/hidden_workflows_api.h"});
        add(e.cli_queries, {"materialize-hidden-cluster"});
        add(e.validation_functions, {"validate_hidden_graph", "validate_full_state"});
        add(e.known_gaps, {"Hidden-cluster mutation remains in-memory and must be curator/debug gated."});
        add(e.notes, {"Successful hidden-truth mutation creates hidden mutation records."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.hidden_mutation_artifact_candidate", "HiddenMutationArtifactCandidate", ControlLayerKind::Planning,
                            ControlLayerPersistence::DerivedViewOnly, ControlLayerBehavior::PlanningOnly,
                            ControlLayerRisk::Moderate, true, true, false, false, true, true, false, false, true);
        add(e.primary_files, {"src/candidate_generation.cpp", "src/candidates_materialization.cpp"});
        add(e.cli_queries, {"generate-artifacts-from-hidden-mutation", "materialize-hidden-mutation-artifact-candidate"});
        add(e.known_gaps, {"Generated hidden-mutation artifact candidates remain quality-gated; low-specificity candidates reject materialization without mutation."});
        add(e.notes, {"This is not the v28 EvidencePotential-to-artifact pipeline."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.access_control", "AccessControl", ControlLayerKind::Formatting,
                            ControlLayerPersistence::ReportOnly, ControlLayerBehavior::FormattingOnly,
                            ControlLayerRisk::High, true, true, false, false, true, true, false, false, false);
        add(e.primary_files, {"src/archive_common.h", "src/artifact_voice_and_views.cpp", "src/*_proposal*.cpp", "src/knowledge_horizon.cpp"});
        add(e.validation_functions, {"can_view", "artifact_visible_to", "claim_visible_to"});
        add(e.known_gaps, {"Access control is mostly enforced through formatting/query projection, not a separate policy engine."});
        add(e.notes, {"Public detail endpoints for hidden KnowledgeHorizon/proposal/audit diagnostics return found: false."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.cli", "CLI", ControlLayerKind::CLI,
                            ControlLayerPersistence::ReportOnly, ControlLayerBehavior::FormattingOnly,
                            ControlLayerRisk::Moderate, false, false, false, false, true, true, false, false, false);
        add(e.primary_files, {"src/cli_model.h", "src/cli.cpp", "src/main.cpp"});
        add(e.cli_queries, {"control-layer-audit-summary", "list-control-layer-audit-entries", "show-control-layer-audit-entry", "validate-control-layer-audit"});
        add(e.notes, {"Internal CLI parsing uses CliArgs string_view argument views; raw argc/argv is limited to main()."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.smoke_workflow", "SmokeWorkflow", ControlLayerKind::CLI,
                            ControlLayerPersistence::ReportOnly, ControlLayerBehavior::ValidationOnly,
                            ControlLayerRisk::Low, false, false, false, false, true, true, false, false, false);
        add(e.primary_files, {"scripts/smoke_test_readme_workflows.sh"});
        add(e.known_gaps, {"Smoke is CLI coverage, not exhaustive semantic proof."});
        add(e.notes, {"Smoke workflow covers representative public/curator access gates and snapshot determinism checks."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.self_tests", "SelfTests", ControlLayerKind::Validation,
                            ControlLayerPersistence::ReportOnly, ControlLayerBehavior::ValidationOnly,
                            ControlLayerRisk::Moderate, false, false, false, false, true, true, false, false, false);
        add(e.primary_files, {"src/self_tests.cpp"});
        add(e.known_gaps, {"Self-tests remain monolithic; future split is desirable but deferred."});
        add(e.notes, {"Self-tests provide primary regression coverage for v28 control-layer invariants."});
        push(std::move(e));
    }
    {
        auto e = make_entry("control_layer.full_state_validation", "FullStateValidation", ControlLayerKind::Validation,
                            ControlLayerPersistence::ReportOnly, ControlLayerBehavior::ValidationOnly,
                            ControlLayerRisk::High, false, false, false, false, false, true, true, false, false);
        add(e.primary_files, {"src/validation.cpp", "src/validation_api.h"});
        add(e.validation_functions, {"validate_full_state", "validate_cross_references", "validate_candidate_artifact_proposals", "validate_candidate_artifact_proposal_audits"});
        add(e.known_gaps, {"Some report-only layers are validated through their own commands rather than through full-state validation."});
        add(e.notes, {"Full-state validation aggregates core archive, evidence, plan, evaluation, proposal, and proposal-audit checks."});
        push(std::move(e));
    }

    std::sort(report.entries.begin(), report.entries.end(), [](const ControlLayerAuditEntry& lhs, const ControlLayerAuditEntry& rhs) {
        return lhs.id < rhs.id;
    });
    report.errors = validate_control_layer_audit_report(report);
    for (const ControlLayerAuditEntry& entry : report.entries) {
        if (!entry.cli_queries.empty() && !entry.smoke_covered && !entry.self_test_covered) {
            report.warnings.push_back(entry.id + " has CLI queries but no coverage flag");
        }
        if (entry.snapshot_covered && entry.snapshot_fields.empty()) {
            report.warnings.push_back(entry.id + " is snapshot-covered but has no snapshot field notes");
        }
        if (entry.access_gated && !entry_has_access_note(entry)) {
            report.warnings.push_back(entry.id + " is access-gated but lacks an access note");
        }
        if (entry.full_state_validation_covered && entry.validation_functions.empty()) {
            report.warnings.push_back(entry.id + " is full-state-validation-covered but lacks validation function notes");
        }
    }
    return report;
}

void build_control_layer_audit_into_state(ArchiveEngineState& state) {
    state.control_layer_audit_entries = build_control_layer_audit_report().entries;
}

[[nodiscard]] std::vector<std::string> validate_control_layer_audit_report(const ControlLayerAuditReport& report) {
    std::vector<std::string> errors;
    std::set<std::string> ids;
    bool has_mutation_capable = false;
    for (const ControlLayerAuditEntry& entry : report.entries) {
        const std::string label = entry.id.empty() ? "<empty>" : entry.id;
        if (entry.id.empty()) {
            errors.push_back("ControlLayerAuditEntry has empty id");
        }
        if (!entry.id.empty() && !ids.insert(entry.id).second) {
            errors.push_back("duplicate ControlLayerAuditEntry id: " + entry.id);
        }
        if (entry.name.empty()) {
            errors.push_back("ControlLayerAuditEntry " + label + " has empty name");
        }
        if (entry.behavior == ControlLayerBehavior::Unknown) {
            errors.push_back("ControlLayerAuditEntry " + label + " has unknown behavior classification");
        }
        if (entry.can_mutate_state) {
            has_mutation_capable = true;
            if (entry.behavior != ControlLayerBehavior::MutationCapable) {
                errors.push_back("mutation-capable ControlLayerAuditEntry " + label + " is not explicitly marked mutation_capable");
            }
        }
        if (entry.should_remain_inert && entry.can_mutate_state) {
            errors.push_back("inert ControlLayerAuditEntry " + label + " is marked mutation-capable");
        }
    }
    if (!has_mutation_capable) {
        errors.push_back("control layer audit must explicitly identify mutation-capable layers");
    }
    return errors;
}

[[nodiscard]] std::string format_control_layer_audit_summary(const ArchiveEngineState& state, AccessLevel access) {
    const std::vector<ControlLayerAuditEntry> entries = entries_for_formatting(state);
    const ControlLayerAuditReport report = build_control_layer_audit_report();
    std::map<std::string, std::size_t> by_behavior;
    std::map<std::string, std::size_t> by_kind;
    std::size_t snapshot_covered = 0;
    std::size_t smoke_covered = 0;
    std::size_t self_test_covered = 0;
    std::size_t access_gated = 0;
    std::size_t mutation_capable = 0;
    std::size_t inert = 0;
    std::size_t known_gaps = 0;
    for (const ControlLayerAuditEntry& entry : entries) {
        ++by_behavior[to_string(entry.behavior)];
        ++by_kind[to_string(entry.kind)];
        if (entry.snapshot_covered) { ++snapshot_covered; }
        if (entry.smoke_covered) { ++smoke_covered; }
        if (entry.self_test_covered) { ++self_test_covered; }
        if (entry.access_gated) { ++access_gated; }
        if (entry.can_mutate_state) { ++mutation_capable; }
        if (entry.should_remain_inert) { ++inert; }
        known_gaps += entry.known_gaps.size();
    }
    std::ostringstream out;
    out << "ControlLayerAudit summary:\n";
    out << "- status: v28 control-layer consolidation; audit/inspection only\n";
    out << "- engine_release_ready: false\n";
    out << "- artifact_generation_deferred: true\n";
    out << "- discovery_expansion_deferred: true\n";
    out << "- total_entries: " << entries.size() << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    out << "- validation_warnings: " << report.warnings.size() << "\n";
    out << "- snapshot_covered_entries: " << snapshot_covered << "\n";
    out << "- smoke_covered_entries: " << smoke_covered << "\n";
    out << "- self_test_covered_entries: " << self_test_covered << "\n";
    out << "- access_gated_entries: " << access_gated << "\n";
    out << "- mutation_capable_entries: " << mutation_capable << "\n";
    out << "- inert_or_advisory_entries: " << inert << "\n";
    out << "- known_gap_count: " << known_gaps << "\n";
    out << "Behavior counts:\n";
    append_counts(out, by_behavior);
    out << "Kind counts:\n";
    append_counts(out, by_kind);
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::ControlLayerAudit)) {
        out << "- details: public access receives aggregate control-layer status only; primary files, internal source chains, mutation notes, and curator diagnostics are restricted.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_control_layer_audit_validation(const ArchiveEngineState& state, AccessLevel access) {
    (void)state;
    const ControlLayerAuditReport report = build_control_layer_audit_report();
    std::ostringstream out;
    out << "ControlLayerAudit validation:\n";
    out << "- result: " << (report.errors.empty() ? "passed" : "failed") << "\n";
    out << "- entries: " << report.entries.size() << "\n";
    out << "- errors: " << report.errors.size() << "\n";
    out << "- warnings: " << report.warnings.size() << "\n";
    if (!report.errors.empty()) {
        if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) {
            out << "Validation errors:\n";
            for (const std::string& error : report.errors) {
                out << "- " << error << "\n";
            }
        } else {
            out << "- details: restricted\n";
        }
    }
    if (!report.warnings.empty() && can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) {
        out << "Validation warnings:\n";
        for (const std::string& warning : report.warnings) {
            out << "- " << warning << "\n";
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_control_layer_audit_entries(const ArchiveEngineState& state, AccessLevel access) {
    const std::vector<ControlLayerAuditEntry> entries = entries_for_formatting(state);
    std::ostringstream out;
    out << "ControlLayerAudit entries visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::ControlLayerAudit)) {
        out << "- total_entries: " << entries.size() << "\n";
        out << "- details: public/scholar access receives aggregate classification only; internal file lists, source chains, mutation details, and diagnostics are restricted.\n";
        for (const ControlLayerAuditEntry& entry : entries) {
            out << "- " << entry.name
                << ": behavior=" << to_string(entry.behavior)
                << " risk=" << to_string(entry.risk)
                << " access_gated=" << (entry.access_gated ? "true" : "false")
                << " can_mutate_state=" << (entry.can_mutate_state ? "true" : "false") << "\n";
        }
        return out.str();
    }
    for (const ControlLayerAuditEntry& entry : entries) {
        out << "- " << entry.id
            << ": name=" << entry.name
            << " kind=" << to_string(entry.kind)
            << " persistence=" << to_string(entry.persistence)
            << " behavior=" << to_string(entry.behavior)
            << " risk=" << to_string(entry.risk)
            << " access_gated=" << (entry.access_gated ? "true" : "false")
            << " snapshot_covered=" << (entry.snapshot_covered ? "true" : "false")
            << " smoke_covered=" << (entry.smoke_covered ? "true" : "false")
            << " self_test_covered=" << (entry.self_test_covered ? "true" : "false")
            << " full_state_validation_covered=" << (entry.full_state_validation_covered ? "true" : "false")
            << " can_mutate_state=" << (entry.can_mutate_state ? "true" : "false")
            << " should_remain_inert=" << (entry.should_remain_inert ? "true" : "false") << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_control_layer_audit_entry_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& entry_id
) {
    const std::vector<ControlLayerAuditEntry> entries = entries_for_formatting(state);
    const ControlLayerAuditEntry* entry = find_entry(entries, entry_id);
    std::ostringstream out;
    out << "ControlLayerAudit entry:\n";
    if (entry == nullptr) {
        out << "- found: false\n";
        return out.str();
    }
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::ControlLayerAudit)) {
        out << "- found: false\n";
        return out.str();
    }
    out << "- found: true\n";
    out << "- id: " << entry->id << "\n";
    out << "- name: " << entry->name << "\n";
    out << "- kind: " << to_string(entry->kind) << "\n";
    out << "- persistence: " << to_string(entry->persistence) << "\n";
    out << "- behavior: " << to_string(entry->behavior) << "\n";
    out << "- risk: " << to_string(entry->risk) << "\n";
    out << "- access_gated: " << (entry->access_gated ? "true" : "false") << "\n";
    out << "- public_detail_gated: " << (entry->public_detail_gated ? "true" : "false") << "\n";
    out << "- snapshot_covered: " << (entry->snapshot_covered ? "true" : "false") << "\n";
    out << "- summary_digest_covered: " << (entry->summary_digest_covered ? "true" : "false") << "\n";
    out << "- smoke_covered: " << (entry->smoke_covered ? "true" : "false") << "\n";
    out << "- self_test_covered: " << (entry->self_test_covered ? "true" : "false") << "\n";
    out << "- full_state_validation_covered: " << (entry->full_state_validation_covered ? "true" : "false") << "\n";
    out << "- can_mutate_state: " << (entry->can_mutate_state ? "true" : "false") << "\n";
    out << "- should_remain_inert: " << (entry->should_remain_inert ? "true" : "false") << "\n";
    auto dump = [&](std::string_view label, const std::vector<std::string>& values) {
        if (values.empty()) { return; }
        out << label << ":\n";
        for (const std::string& value : values) {
            out << "- " << value << "\n";
        }
    };
    dump("Primary files", entry->primary_files);
    dump("CLI queries", entry->cli_queries);
    dump("Validation functions", entry->validation_functions);
    dump("Snapshot fields", entry->snapshot_fields);
    dump("Known gaps", entry->known_gaps);
    dump("Notes", entry->notes);
    return out.str();
}

} // namespace archive
