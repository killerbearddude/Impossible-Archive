#pragma once
#include "archive_common.h"

namespace archive {

struct ArchiveSnapshot {
    std::string snapshot_id;
    std::string source_fixture_id;
    std::uint64_t fixture_seed = 0;
    std::uint64_t state_seed = 0;
    int fixture_archive_year = 0;
    int effective_archive_year = 0;

    std::size_t hidden_entity_count = 0;
    std::size_t hidden_event_count = 0;
    std::size_t public_artifact_count = 0;
    std::size_t public_claim_count = 0;
    std::size_t contradiction_count = 0;
    std::size_t anachronism_report_count = 0;
    std::size_t mystery_count = 0;
    std::size_t theory_count = 0;
    std::size_t discovery_count = 0;
    std::size_t hidden_mutation_record_count = 0;
    std::size_t evidence_potential_count = 0;
    std::size_t knowledge_horizon_finding_count = 0;
    std::size_t knowledge_horizon_error_count = 0;
    std::size_t contradiction_budget_bucket_count = 0;
    std::size_t contradiction_budget_over_budget_count = 0;
    std::size_t contradiction_budget_watch_count = 0;
    std::size_t contradiction_budget_too_clean_count = 0;
    std::size_t contradiction_budget_productive_ambiguity_count = 0;
    std::size_t contradiction_budget_generation_bug_count = 0;
    std::size_t candidate_artifact_plan_count = 0;
    std::size_t candidate_artifact_plan_blocked_count = 0;
    std::size_t candidate_artifact_plan_curator_review_count = 0;
    std::size_t candidate_artifact_plan_evaluation_count = 0;
    std::size_t candidate_artifact_plan_evaluation_pass_count = 0;
    std::size_t candidate_artifact_plan_evaluation_blocked_count = 0;
    std::size_t candidate_artifact_plan_evaluation_review_count = 0;
    std::size_t candidate_artifact_proposal_count = 0;
    std::size_t candidate_artifact_proposal_draftable_count = 0;
    std::size_t candidate_artifact_proposal_blocked_count = 0;
    std::size_t candidate_artifact_proposal_review_count = 0;
    std::size_t candidate_artifact_proposal_audit_count = 0;
    std::size_t candidate_artifact_proposal_audit_pass_count = 0;
    std::size_t candidate_artifact_proposal_audit_blocked_count = 0;
    std::size_t candidate_artifact_proposal_audit_review_count = 0;
    std::size_t candidate_artifact_proposal_audit_revision_count = 0;
    std::size_t candidate_artifact_draft_count = 0;
    std::size_t candidate_artifact_draft_ready_count = 0;
    std::size_t candidate_artifact_draft_blocked_count = 0;
    std::size_t candidate_artifact_draft_review_count = 0;
    std::size_t candidate_artifact_draft_revision_count = 0;
    std::size_t candidate_artifact_draft_mutation_enabled_count = 0;
    std::size_t candidate_artifact_draft_review_record_count = 0;
    std::size_t candidate_artifact_draft_review_pass_count = 0;
    std::size_t candidate_artifact_draft_review_blocked_count = 0;
    std::size_t candidate_artifact_draft_review_curator_review_count = 0;
    std::size_t candidate_artifact_draft_review_revision_count = 0;
    std::size_t candidate_artifact_draft_review_mutation_enabled_count = 0;
    std::size_t control_layer_audit_entry_count = 0;
    std::size_t control_layer_audit_mutation_capable_count = 0;
    std::size_t control_layer_audit_report_only_count = 0;
    std::size_t control_layer_audit_access_gated_count = 0;
    std::size_t control_layer_audit_known_gap_count = 0;
    std::size_t civilization_spec_count = 0;
    std::size_t civilization_fragment_count = 0;

    std::string summary_digest;
    std::vector<std::string> validation_errors;
};

struct ArchiveSnapshotComparison {
    bool same = false;
    ArchiveSnapshot before;
    ArchiveSnapshot after;
};

} // namespace archive
