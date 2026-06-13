#include "archive_snapshot_api.h"
#include "archive_views_api.h"
#include "candidate_artifact_plan_api.h"
#include "candidate_artifact_plan_evaluation_api.h"
#include "candidate_artifact_proposal_api.h"
#include "candidate_artifact_proposal_audit_api.h"
#include "candidate_artifact_draft_api.h"
#include "candidate_artifact_draft_review_api.h"
#include "contradiction_budget_api.h"
#include "control_layer_audit_api.h"
#include "evidence_potential_api.h"
#include "knowledge_horizon_api.h"
#include "validation_api.h"

#include <iomanip>
#include <sstream>

namespace archive {

namespace {

[[nodiscard]] std::uint64_t fnv1a64(std::string_view text) {
    std::uint64_t value = 1469598103934665603ULL;
    for (char ch : text) {
        const auto byte = static_cast<unsigned char>(ch);
        value ^= static_cast<std::uint64_t>(byte);
        value *= 1099511628211ULL;
    }
    return value;
}

[[nodiscard]] std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}

[[nodiscard]] std::string snapshot_digest_material(const ArchiveEngineState& state,
                                                    const std::string& source_fixture_id,
                                                    std::uint64_t fixture_seed,
                                                    int fixture_archive_year,
                                                    int effective_archive_year) {
    std::ostringstream out;
    out << "fixture=" << source_fixture_id << "\n";
    out << "fixture_seed=" << fixture_seed << "\n";
    out << "state_seed=" << state.seed << "\n";
    out << "fixture_archive_year=" << fixture_archive_year << "\n";
    out << "effective_archive_year=" << effective_archive_year << "\n";
    out << "spec_count=" << state.civilization_spec_count << "\n";
    out << "fragment_count=" << state.civilization_fragment_count << "\n";
    out << "evidence_potential_count=" << state.evidence_potentials.size() << "\n";
    const KnowledgeHorizonReport horizon_report = validate_knowledge_horizon(state, AccessLevel::Curator);
    out << "knowledge_horizon_finding_count=" << horizon_report.findings.size() << "\n";
    out << "knowledge_horizon_error_count=" << horizon_report.errors.size() << "\n";
    const ContradictionBudgetReport budget_report = compute_contradiction_budget(state, AccessLevel::Curator);
    std::size_t budget_generation_bugs = 0;
    std::size_t budget_over_budget = 0;
    std::size_t budget_watch = 0;
    std::size_t budget_too_clean = 0;
    std::size_t budget_productive_ambiguity = 0;
    for (const ContradictionBudgetBucket& bucket : budget_report.buckets) {
        if (bucket.status == ContradictionBudgetStatus::OverBudget) {
            ++budget_over_budget;
        }
        if (bucket.status == ContradictionBudgetStatus::Watch) {
            ++budget_watch;
        }
        if (std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::TooCleanArchive) != bucket.reason_codes.end()) {
            ++budget_too_clean;
        }
        if (std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::ProductiveAmbiguity) != bucket.reason_codes.end() ||
            std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::ValidRitualContradiction) != bucket.reason_codes.end() ||
            std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::ValidLegalFiction) != bucket.reason_codes.end()) {
            ++budget_productive_ambiguity;
        }
        budget_generation_bugs += bucket.generation_bug_count;
    }
    out << "contradiction_budget_bucket_count=" << budget_report.buckets.size() << "\n";
    out << "contradiction_budget_over_budget_count=" << budget_over_budget << "\n";
    out << "contradiction_budget_watch_count=" << budget_watch << "\n";
    out << "contradiction_budget_too_clean_count=" << budget_too_clean << "\n";
    out << "contradiction_budget_productive_ambiguity_count=" << budget_productive_ambiguity << "\n";
    out << "contradiction_budget_generation_bug_count=" << budget_generation_bugs << "\n";
    for (const ContradictionBudgetBucket& bucket : budget_report.buckets) {
        out << "contradiction_budget_bucket=" << bucket.id
            << "|status=" << to_string(bucket.status)
            << "|severity=" << to_string(bucket.severity)
            << "|density=" << std::fixed << std::setprecision(3) << bucket.contradiction_density
            << "|unresolved=" << bucket.unresolved_ratio
            << "|generation_bug=" << bucket.generation_bug_ratio
            << "|reasons=";
        if (bucket.reason_codes.empty()) {
            out << "none";
        } else {
            for (std::size_t index = 0; index < bucket.reason_codes.size(); ++index) {
                if (index > 0U) {
                    out << ",";
                }
                out << to_string(bucket.reason_codes[index]);
            }
        }
        out << "\n";
    }
    const CandidateArtifactPlanReport plan_report = derive_candidate_artifact_plans(state, AccessLevel::Curator);
    std::size_t plan_blocked = 0;
    std::size_t plan_review = 0;
    for (const CandidateArtifactPlan& plan : plan_report.plans) {
        if (plan.status == CandidateArtifactPlanStatus::BlockedByKnowledgeHorizon ||
            plan.status == CandidateArtifactPlanStatus::BlockedByContradictionPressure ||
            plan.status == CandidateArtifactPlanStatus::BlockedByProtectedMystery ||
            plan.status == CandidateArtifactPlanStatus::BlockedByPublicSafety ||
            plan.status == CandidateArtifactPlanStatus::Invalid) {
            ++plan_blocked;
        }
        if (plan.requires_curator_review) {
            ++plan_review;
        }
    }
    out << "candidate_artifact_plan_count=" << plan_report.plans.size() << "\n";
    out << "candidate_artifact_plan_blocked_count=" << plan_blocked << "\n";
    out << "candidate_artifact_plan_curator_review_count=" << plan_review << "\n";
    const CandidateArtifactPlanEvaluationReport evaluation_report = evaluate_candidate_artifact_plans(state, AccessLevel::Curator);
    std::size_t evaluation_pass = 0;
    std::size_t evaluation_blocked = 0;
    std::size_t evaluation_review = 0;
    for (const CandidateArtifactPlanEvaluation& evaluation : evaluation_report.evaluations) {
        if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Pass) { ++evaluation_pass; }
        if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Blocked || evaluation.decision == CandidateArtifactPlanEvaluationDecision::Invalid) { ++evaluation_blocked; }
        if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::NeedsCuratorReview) { ++evaluation_review; }
    }
    out << "candidate_artifact_plan_evaluation_count=" << evaluation_report.evaluations.size() << "\n";
    out << "candidate_artifact_plan_evaluation_pass_count=" << evaluation_pass << "\n";
    out << "candidate_artifact_plan_evaluation_blocked_count=" << evaluation_blocked << "\n";
    out << "candidate_artifact_plan_evaluation_review_count=" << evaluation_review << "\n";
    const CandidateArtifactProposalReport proposal_report = draft_candidate_artifact_proposals(state, AccessLevel::Curator);
    std::size_t proposal_draftable = 0;
    std::size_t proposal_blocked = 0;
    std::size_t proposal_review = 0;
    for (const CandidateArtifactProposal& proposal : proposal_report.proposals) {
        if (proposal.decision == CandidateArtifactProposalDecision::Draftable) { ++proposal_draftable; }
        if (proposal.decision == CandidateArtifactProposalDecision::Blocked || proposal.decision == CandidateArtifactProposalDecision::Invalid) { ++proposal_blocked; }
        if (proposal.decision == CandidateArtifactProposalDecision::NeedsCuratorReview) { ++proposal_review; }
    }
    out << "candidate_artifact_proposal_count=" << proposal_report.proposals.size() << "\n";
    out << "candidate_artifact_proposal_draftable_count=" << proposal_draftable << "\n";
    out << "candidate_artifact_proposal_blocked_count=" << proposal_blocked << "\n";
    out << "candidate_artifact_proposal_review_count=" << proposal_review << "\n";
    const CandidateArtifactProposalAuditReport audit_report = audit_candidate_artifact_proposals(state, AccessLevel::Curator);
    std::size_t audit_pass = 0;
    std::size_t audit_blocked = 0;
    std::size_t audit_review = 0;
    std::size_t audit_revision = 0;
    for (const CandidateArtifactProposalAudit& audit : audit_report.audits) {
        if (audit.decision == CandidateArtifactProposalAuditDecision::Pass) { ++audit_pass; }
        if (audit.decision == CandidateArtifactProposalAuditDecision::Blocked || audit.decision == CandidateArtifactProposalAuditDecision::Invalid) { ++audit_blocked; }
        if (audit.decision == CandidateArtifactProposalAuditDecision::NeedsCuratorReview) { ++audit_review; }
        if (audit.decision == CandidateArtifactProposalAuditDecision::NeedsRevision) { ++audit_revision; }
    }
    out << "candidate_artifact_proposal_audit_count=" << audit_report.audits.size() << "\n";
    out << "candidate_artifact_proposal_audit_pass_count=" << audit_pass << "\n";
    out << "candidate_artifact_proposal_audit_blocked_count=" << audit_blocked << "\n";
    out << "candidate_artifact_proposal_audit_review_count=" << audit_review << "\n";
    out << "candidate_artifact_proposal_audit_revision_count=" << audit_revision << "\n";
    const CandidateArtifactDraftReport draft_report = derive_candidate_artifact_drafts(state, AccessLevel::Curator);
    std::size_t draft_ready = 0;
    std::size_t draft_blocked = 0;
    std::size_t draft_review = 0;
    std::size_t draft_revision = 0;
    std::size_t draft_mutation_enabled = 0;
    for (const CandidateArtifactDraft& draft : draft_report.drafts) {
        if (draft.status == CandidateArtifactDraftStatus::ReadyForOutline) { ++draft_ready; }
        if (draft.status == CandidateArtifactDraftStatus::Blocked || draft.status == CandidateArtifactDraftStatus::Invalid) { ++draft_blocked; }
        if (draft.status == CandidateArtifactDraftStatus::NeedsCuratorReview) { ++draft_review; }
        if (draft.status == CandidateArtifactDraftStatus::NeedsRevision) { ++draft_revision; }
        if (draft.current_artifact_insertion_enabled || draft.current_public_claim_insertion_enabled ||
            draft.current_discovery_scheduling_enabled || draft.hidden_truth_mutation_enabled ||
            draft.public_archive_mutation_enabled || draft.persistence_enabled) { ++draft_mutation_enabled; }
    }
    out << "candidate_artifact_draft_count=" << draft_report.drafts.size() << "\n";
    out << "candidate_artifact_draft_ready_count=" << draft_ready << "\n";
    out << "candidate_artifact_draft_blocked_count=" << draft_blocked << "\n";
    out << "candidate_artifact_draft_review_count=" << draft_review << "\n";
    out << "candidate_artifact_draft_revision_count=" << draft_revision << "\n";
    out << "candidate_artifact_draft_mutation_enabled_count=" << draft_mutation_enabled << "\n";
    const CandidateArtifactDraftReviewReport draft_review_report = review_candidate_artifact_drafts(state, AccessLevel::Curator);
    std::size_t draft_review_pass = 0;
    std::size_t draft_review_blocked = 0;
    std::size_t draft_review_curator_review = 0;
    std::size_t draft_review_revision = 0;
    std::size_t draft_review_mutation_enabled = 0;
    for (const CandidateArtifactDraftReview& review : draft_review_report.reviews) {
        if (review.decision == CandidateArtifactDraftReviewDecision::Pass) { ++draft_review_pass; }
        if (review.decision == CandidateArtifactDraftReviewDecision::Blocked || review.decision == CandidateArtifactDraftReviewDecision::Invalid) { ++draft_review_blocked; }
        if (review.decision == CandidateArtifactDraftReviewDecision::NeedsCuratorReview) { ++draft_review_curator_review; }
        if (review.decision == CandidateArtifactDraftReviewDecision::NeedsRevision) { ++draft_review_revision; }
        if (review.current_artifact_insertion_enabled || review.current_public_claim_insertion_enabled ||
            review.current_discovery_scheduling_enabled || review.hidden_truth_mutation_enabled ||
            review.public_archive_mutation_enabled || review.persistence_enabled ||
            review.final_artifact_prose_generation_enabled) { ++draft_review_mutation_enabled; }
    }
    out << "candidate_artifact_draft_review_record_count=" << draft_review_report.reviews.size() << "\n";
    out << "candidate_artifact_draft_review_pass_count=" << draft_review_pass << "\n";
    out << "candidate_artifact_draft_review_blocked_count=" << draft_review_blocked << "\n";
    out << "candidate_artifact_draft_review_curator_review_count=" << draft_review_curator_review << "\n";
    out << "candidate_artifact_draft_review_revision_count=" << draft_review_revision << "\n";
    out << "candidate_artifact_draft_review_mutation_enabled_count=" << draft_review_mutation_enabled << "\n";
    const ControlLayerAuditReport control_report = build_control_layer_audit_report();
    std::size_t control_mutation_capable = 0;
    std::size_t control_report_only = 0;
    std::size_t control_access_gated = 0;
    std::size_t control_known_gaps = 0;
    for (const ControlLayerAuditEntry& entry : control_report.entries) {
        if (entry.can_mutate_state) { ++control_mutation_capable; }
        if (entry.persistence == ControlLayerPersistence::ReportOnly) { ++control_report_only; }
        if (entry.access_gated) { ++control_access_gated; }
        control_known_gaps += entry.known_gaps.size();
    }
    out << "control_layer_audit_entry_count=" << control_report.entries.size() << "\n";
    out << "control_layer_audit_mutation_capable_count=" << control_mutation_capable << "\n";
    out << "control_layer_audit_report_only_count=" << control_report_only << "\n";
    out << "control_layer_audit_access_gated_count=" << control_access_gated << "\n";
    out << "control_layer_audit_known_gap_count=" << control_known_gaps << "\n";
    for (const ControlLayerAuditEntry& entry : control_report.entries) {
        out << "C|" << entry.id
            << "|" << to_string(entry.kind)
            << "|" << to_string(entry.persistence)
            << "|" << to_string(entry.behavior)
            << "|" << to_string(entry.risk)
            << "|access_gated=" << (entry.access_gated ? "true" : "false")
            << "|snapshot_covered=" << (entry.snapshot_covered ? "true" : "false")
            << "|self_test_covered=" << (entry.self_test_covered ? "true" : "false")
            << "|smoke_covered=" << (entry.smoke_covered ? "true" : "false")
            << "|can_mutate_state=" << (entry.can_mutate_state ? "true" : "false")
            << "|should_remain_inert=" << (entry.should_remain_inert ? "true" : "false")
            << "|known_gap_count=" << entry.known_gaps.size()
            << "\n";
    }
    for (const CandidateArtifactDraft& draft : draft_report.drafts) {
        out << "D|" << draft.id
            << "|" << draft.proposal_id
            << "|" << draft.audit_id
            << "|" << to_string(draft.status)
            << "|" << to_string(draft.visibility_class)
            << "|outline_lines=" << draft.claim_outline_lines.size()
            << "|validation_gates=" << draft.required_validation_gates.size()
            << "|artifact_insertion_enabled=" << (draft.current_artifact_insertion_enabled ? "true" : "false")
            << "|public_claim_insertion_enabled=" << (draft.current_public_claim_insertion_enabled ? "true" : "false")
            << "|discovery_scheduling_enabled=" << (draft.current_discovery_scheduling_enabled ? "true" : "false")
            << "|hidden_truth_mutation_enabled=" << (draft.hidden_truth_mutation_enabled ? "true" : "false")
            << "|public_archive_mutation_enabled=" << (draft.public_archive_mutation_enabled ? "true" : "false")
            << "|persistence_enabled=" << (draft.persistence_enabled ? "true" : "false")
            << "\n";
    }
    for (const CandidateArtifactDraftReview& review : draft_review_report.reviews) {
        out << "R|" << review.id
            << "|" << review.draft_id
            << "|" << review.proposal_id
            << "|" << review.audit_id
            << "|" << to_string(review.decision)
            << "|outline=" << review.outline_completeness_score
            << "|traceability=" << review.traceability_score
            << "|safety=" << review.safety_score
            << "|specificity=" << review.specificity_score
            << "|revision_pressure=" << review.revision_pressure_score
            << "|reason_codes=" << review.reason_codes.size()
            << "|required_revisions=" << review.required_revisions.size()
            << "|mutation_or_generation_enabled=" << ((review.current_artifact_insertion_enabled || review.current_public_claim_insertion_enabled || review.current_discovery_scheduling_enabled || review.hidden_truth_mutation_enabled || review.public_archive_mutation_enabled || review.persistence_enabled || review.final_artifact_prose_generation_enabled) ? "true" : "false")
            << "\n";
    }
    for (const CandidateArtifactProposalAudit& audit : audit_report.audits) {
        out << "U|" << audit.id
            << "|" << audit.proposal_id
            << "|" << to_string(audit.decision)
            << "|quality=" << audit.proposal_quality_score
            << "|specificity=" << audit.specificity_score
            << "|safety=" << audit.safety_score
            << "|revision_pressure=" << audit.revision_pressure_score
            << "|required_revision_count=" << audit.required_revisions.size()
            << "|current_generation_enabled=" << (audit.current_generation_enabled ? "true" : "false")
            << "|current_materialization_enabled=" << (audit.current_materialization_enabled ? "true" : "false")
            << "|archive_mutation_enabled=" << (audit.archive_mutation_enabled ? "true" : "false");
        for (const CandidateArtifactProposalAuditFinding& finding : audit.findings) {
            out << "|reason=" << to_string(finding.reason_code);
        }
        out << "\n";
    }
    for (const CandidateArtifactProposal& proposal : proposal_report.proposals) {
        out << "R|" << proposal.id
            << "|" << proposal.plan_id
            << "|" << proposal.evaluation_id
            << "|" << to_string(proposal.decision)
            << "|" << to_string(proposal.visibility_class)
            << "|hidden_source_ref=" << (proposal.contains_hidden_source_reference ? "true" : "false")
            << "|curator_diagnostics=" << (proposal.contains_curator_diagnostics ? "true" : "false")
            << "|protected_mystery=" << (proposal.touches_protected_mystery ? "true" : "false")
            << "|requires_curator_review=" << (proposal.requires_curator_review ? "true" : "false")
            << "|" << to_string(proposal.text_status)
            << "|current_generation_enabled=" << (proposal.current_generation_enabled ? "true" : "false")
            << "|current_materialization_enabled=" << (proposal.current_materialization_enabled ? "true" : "false")
            << "|archive_mutation_enabled=" << (proposal.archive_mutation_enabled ? "true" : "false")
            << "\n";
    }
    for (const CandidateArtifactPlanEvaluation& evaluation : evaluation_report.evaluations) {
        out << "E|" << evaluation.id
            << "|" << evaluation.plan_id
            << "|" << to_string(evaluation.decision)
            << "|readiness=" << evaluation.readiness_score
            << "|risk=" << evaluation.risk_score
            << "|current_generation_enabled=" << (evaluation.current_generation_enabled ? "true" : "false")
            << "|current_materialization_enabled=" << (evaluation.current_materialization_enabled ? "true" : "false")
            << "\n";
    }
    for (const CandidateArtifactPlan& plan : plan_report.plans) {
        out << "A|" << plan.id
            << "|" << plan.source_id
            << "|" << to_string(plan.planned_shape)
            << "|" << to_string(plan.planned_artifact_type)
            << "|" << to_string(plan.status)
            << "|" << to_string(plan.risk_level)
            << "|public_safe=" << (plan.public_safe ? "true" : "false")
            << "|requires_curator_review=" << (plan.requires_curator_review ? "true" : "false")
            << "|current_materialization_enabled=" << (plan.current_materialization_enabled ? "true" : "false")
            << "\n";
    }
    for (const ContradictionBudgetBucket& bucket : budget_report.buckets) {
        out << "B|" << bucket.id
            << "|" << to_string(bucket.scope)
            << "|" << to_string(bucket.status)
            << "|" << to_string(bucket.severity)
            << "|" << bucket.contradiction_count
            << "|" << bucket.unresolved_contradiction_count
            << "|" << bucket.generation_bug_count
            << "\n";
    }
    for (const KnowledgeHorizonFinding& finding : horizon_report.findings) {
        out << "K|" << finding.id
            << "|" << to_string(finding.status)
            << "|" << to_string(finding.context_type)
            << "|" << to_string(finding.subject_type)
            << "\n";
    }
    for (const EvidencePotential& potential : state.evidence_potentials) {
        out << "P|" << potential.id
            << "|" << to_string(potential.source_type)
            << "|" << potential.source_id
            << "|" << to_string(potential.trace_type)
            << "|" << to_string(potential.likely_artifact_type)
            << "|" << potential.earliest_possible_year
            << "|" << potential.latest_possible_year
            << "|" << to_string(potential.strength)
            << "|public_safe=" << (potential.public_safe ? "true" : "false")
            << "|discoverable=" << (potential.discoverable ? "true" : "false")
            << "\n";
    }
    out << serialize_for_replay_test(state);
    return out.str();
}

void format_count_delta(std::ostringstream& out,
                        std::string_view label,
                        std::size_t before,
                        std::size_t after) {
    const long long delta = static_cast<long long>(after) - static_cast<long long>(before);
    out << "- " << label << ": " << before << " -> " << after << " (delta ";
    if (delta >= 0) {
        out << "+";
    }
    out << delta << ")\n";
}

} // namespace

[[nodiscard]] ArchiveSnapshot build_archive_snapshot(
    const ArchiveEngineState& state,
    const std::string& source_fixture_id,
    std::uint64_t fixture_seed,
    int fixture_archive_year,
    int effective_archive_year
) {
    ArchiveSnapshot snapshot;
    snapshot.source_fixture_id = source_fixture_id;
    snapshot.fixture_seed = fixture_seed;
    snapshot.state_seed = state.seed;
    snapshot.fixture_archive_year = fixture_archive_year;
    snapshot.effective_archive_year = effective_archive_year;
    snapshot.hidden_entity_count = state.hidden_truth.entities().size();
    snapshot.hidden_event_count = state.hidden_truth.events().size();
    snapshot.public_artifact_count = state.public_archive.artifacts().size();
    snapshot.public_claim_count = state.public_archive.claims().size();
    snapshot.contradiction_count = state.public_archive.contradictions().size();
    snapshot.anachronism_report_count = detect_anachronisms(state).size();
    snapshot.mystery_count = state.mysteries.size();
    snapshot.theory_count = build_theories(state, effective_archive_year).size();
    snapshot.discovery_count = state.discovery_log.size();
    snapshot.hidden_mutation_record_count = state.hidden_truth_mutations.size();
    snapshot.evidence_potential_count = state.evidence_potentials.size();
    const KnowledgeHorizonReport horizon_report = validate_knowledge_horizon(state, AccessLevel::Curator);
    snapshot.knowledge_horizon_finding_count = horizon_report.findings.size();
    snapshot.knowledge_horizon_error_count = horizon_report.errors.size();
    const ContradictionBudgetReport budget_report = compute_contradiction_budget(state, AccessLevel::Curator);
    snapshot.contradiction_budget_bucket_count = budget_report.buckets.size();
    snapshot.contradiction_budget_over_budget_count = static_cast<std::size_t>(std::count_if(
        budget_report.buckets.begin(),
        budget_report.buckets.end(),
        [](const ContradictionBudgetBucket& bucket) { return bucket.status == ContradictionBudgetStatus::OverBudget; }
    ));
    snapshot.contradiction_budget_watch_count = static_cast<std::size_t>(std::count_if(
        budget_report.buckets.begin(),
        budget_report.buckets.end(),
        [](const ContradictionBudgetBucket& bucket) { return bucket.status == ContradictionBudgetStatus::Watch; }
    ));
    snapshot.contradiction_budget_too_clean_count = static_cast<std::size_t>(std::count_if(
        budget_report.buckets.begin(),
        budget_report.buckets.end(),
        [](const ContradictionBudgetBucket& bucket) {
            return std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::TooCleanArchive) != bucket.reason_codes.end();
        }
    ));
    snapshot.contradiction_budget_productive_ambiguity_count = static_cast<std::size_t>(std::count_if(
        budget_report.buckets.begin(),
        budget_report.buckets.end(),
        [](const ContradictionBudgetBucket& bucket) {
            return std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::ProductiveAmbiguity) != bucket.reason_codes.end() ||
                   std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::ValidRitualContradiction) != bucket.reason_codes.end() ||
                   std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::ValidLegalFiction) != bucket.reason_codes.end();
        }
    ));
    snapshot.contradiction_budget_generation_bug_count = 0;
    for (const ContradictionBudgetBucket& bucket : budget_report.buckets) {
        snapshot.contradiction_budget_generation_bug_count += bucket.generation_bug_count;
    }
    const CandidateArtifactPlanReport plan_report = derive_candidate_artifact_plans(state, AccessLevel::Curator);
    snapshot.candidate_artifact_plan_count = plan_report.plans.size();
    snapshot.candidate_artifact_plan_blocked_count = static_cast<std::size_t>(std::count_if(
        plan_report.plans.begin(),
        plan_report.plans.end(),
        [](const CandidateArtifactPlan& plan) {
            return plan.status == CandidateArtifactPlanStatus::BlockedByKnowledgeHorizon ||
                   plan.status == CandidateArtifactPlanStatus::BlockedByContradictionPressure ||
                   plan.status == CandidateArtifactPlanStatus::BlockedByProtectedMystery ||
                   plan.status == CandidateArtifactPlanStatus::BlockedByPublicSafety ||
                   plan.status == CandidateArtifactPlanStatus::Invalid;
        }
    ));
    snapshot.candidate_artifact_plan_curator_review_count = static_cast<std::size_t>(std::count_if(
        plan_report.plans.begin(),
        plan_report.plans.end(),
        [](const CandidateArtifactPlan& plan) { return plan.requires_curator_review; }
    ));
    const CandidateArtifactPlanEvaluationReport evaluation_report = evaluate_candidate_artifact_plans(state, AccessLevel::Curator);
    snapshot.candidate_artifact_plan_evaluation_count = evaluation_report.evaluations.size();
    snapshot.candidate_artifact_plan_evaluation_pass_count = static_cast<std::size_t>(std::count_if(
        evaluation_report.evaluations.begin(), evaluation_report.evaluations.end(),
        [](const CandidateArtifactPlanEvaluation& evaluation) { return evaluation.decision == CandidateArtifactPlanEvaluationDecision::Pass; }
    ));
    snapshot.candidate_artifact_plan_evaluation_blocked_count = static_cast<std::size_t>(std::count_if(
        evaluation_report.evaluations.begin(), evaluation_report.evaluations.end(),
        [](const CandidateArtifactPlanEvaluation& evaluation) { return evaluation.decision == CandidateArtifactPlanEvaluationDecision::Blocked || evaluation.decision == CandidateArtifactPlanEvaluationDecision::Invalid; }
    ));
    snapshot.candidate_artifact_plan_evaluation_review_count = static_cast<std::size_t>(std::count_if(
        evaluation_report.evaluations.begin(), evaluation_report.evaluations.end(),
        [](const CandidateArtifactPlanEvaluation& evaluation) { return evaluation.decision == CandidateArtifactPlanEvaluationDecision::NeedsCuratorReview; }
    ));
    const CandidateArtifactProposalReport proposal_report = draft_candidate_artifact_proposals(state, AccessLevel::Curator);
    snapshot.candidate_artifact_proposal_count = proposal_report.proposals.size();
    snapshot.candidate_artifact_proposal_draftable_count = static_cast<std::size_t>(std::count_if(
        proposal_report.proposals.begin(), proposal_report.proposals.end(),
        [](const CandidateArtifactProposal& proposal) { return proposal.decision == CandidateArtifactProposalDecision::Draftable; }
    ));
    snapshot.candidate_artifact_proposal_blocked_count = static_cast<std::size_t>(std::count_if(
        proposal_report.proposals.begin(), proposal_report.proposals.end(),
        [](const CandidateArtifactProposal& proposal) { return proposal.decision == CandidateArtifactProposalDecision::Blocked || proposal.decision == CandidateArtifactProposalDecision::Invalid; }
    ));
    snapshot.candidate_artifact_proposal_review_count = static_cast<std::size_t>(std::count_if(
        proposal_report.proposals.begin(), proposal_report.proposals.end(),
        [](const CandidateArtifactProposal& proposal) { return proposal.decision == CandidateArtifactProposalDecision::NeedsCuratorReview; }
    ));
    const CandidateArtifactProposalAuditReport audit_report = audit_candidate_artifact_proposals(state, AccessLevel::Curator);
    snapshot.candidate_artifact_proposal_audit_count = audit_report.audits.size();
    snapshot.candidate_artifact_proposal_audit_pass_count = static_cast<std::size_t>(std::count_if(
        audit_report.audits.begin(), audit_report.audits.end(),
        [](const CandidateArtifactProposalAudit& audit) { return audit.decision == CandidateArtifactProposalAuditDecision::Pass; }
    ));
    snapshot.candidate_artifact_proposal_audit_blocked_count = static_cast<std::size_t>(std::count_if(
        audit_report.audits.begin(), audit_report.audits.end(),
        [](const CandidateArtifactProposalAudit& audit) { return audit.decision == CandidateArtifactProposalAuditDecision::Blocked || audit.decision == CandidateArtifactProposalAuditDecision::Invalid; }
    ));
    snapshot.candidate_artifact_proposal_audit_review_count = static_cast<std::size_t>(std::count_if(
        audit_report.audits.begin(), audit_report.audits.end(),
        [](const CandidateArtifactProposalAudit& audit) { return audit.decision == CandidateArtifactProposalAuditDecision::NeedsCuratorReview; }
    ));
    snapshot.candidate_artifact_proposal_audit_revision_count = static_cast<std::size_t>(std::count_if(
        audit_report.audits.begin(), audit_report.audits.end(),
        [](const CandidateArtifactProposalAudit& audit) { return audit.decision == CandidateArtifactProposalAuditDecision::NeedsRevision; }
    ));
    const CandidateArtifactDraftReport draft_report = derive_candidate_artifact_drafts(state, AccessLevel::Curator);
    snapshot.candidate_artifact_draft_count = draft_report.drafts.size();
    snapshot.candidate_artifact_draft_ready_count = static_cast<std::size_t>(std::count_if(
        draft_report.drafts.begin(), draft_report.drafts.end(),
        [](const CandidateArtifactDraft& draft) { return draft.status == CandidateArtifactDraftStatus::ReadyForOutline; }
    ));
    snapshot.candidate_artifact_draft_blocked_count = static_cast<std::size_t>(std::count_if(
        draft_report.drafts.begin(), draft_report.drafts.end(),
        [](const CandidateArtifactDraft& draft) { return draft.status == CandidateArtifactDraftStatus::Blocked || draft.status == CandidateArtifactDraftStatus::Invalid; }
    ));
    snapshot.candidate_artifact_draft_review_count = static_cast<std::size_t>(std::count_if(
        draft_report.drafts.begin(), draft_report.drafts.end(),
        [](const CandidateArtifactDraft& draft) { return draft.status == CandidateArtifactDraftStatus::NeedsCuratorReview; }
    ));
    snapshot.candidate_artifact_draft_revision_count = static_cast<std::size_t>(std::count_if(
        draft_report.drafts.begin(), draft_report.drafts.end(),
        [](const CandidateArtifactDraft& draft) { return draft.status == CandidateArtifactDraftStatus::NeedsRevision; }
    ));
    snapshot.candidate_artifact_draft_mutation_enabled_count = static_cast<std::size_t>(std::count_if(
        draft_report.drafts.begin(), draft_report.drafts.end(),
        [](const CandidateArtifactDraft& draft) {
            return draft.current_artifact_insertion_enabled || draft.current_public_claim_insertion_enabled ||
                   draft.current_discovery_scheduling_enabled || draft.hidden_truth_mutation_enabled ||
                   draft.public_archive_mutation_enabled || draft.persistence_enabled;
        }
    ));
    const CandidateArtifactDraftReviewReport draft_review_report = review_candidate_artifact_drafts(state, AccessLevel::Curator);
    snapshot.candidate_artifact_draft_review_record_count = draft_review_report.reviews.size();
    snapshot.candidate_artifact_draft_review_pass_count = static_cast<std::size_t>(std::count_if(
        draft_review_report.reviews.begin(), draft_review_report.reviews.end(),
        [](const CandidateArtifactDraftReview& review) { return review.decision == CandidateArtifactDraftReviewDecision::Pass; }
    ));
    snapshot.candidate_artifact_draft_review_blocked_count = static_cast<std::size_t>(std::count_if(
        draft_review_report.reviews.begin(), draft_review_report.reviews.end(),
        [](const CandidateArtifactDraftReview& review) { return review.decision == CandidateArtifactDraftReviewDecision::Blocked || review.decision == CandidateArtifactDraftReviewDecision::Invalid; }
    ));
    snapshot.candidate_artifact_draft_review_curator_review_count = static_cast<std::size_t>(std::count_if(
        draft_review_report.reviews.begin(), draft_review_report.reviews.end(),
        [](const CandidateArtifactDraftReview& review) { return review.decision == CandidateArtifactDraftReviewDecision::NeedsCuratorReview; }
    ));
    snapshot.candidate_artifact_draft_review_revision_count = static_cast<std::size_t>(std::count_if(
        draft_review_report.reviews.begin(), draft_review_report.reviews.end(),
        [](const CandidateArtifactDraftReview& review) { return review.decision == CandidateArtifactDraftReviewDecision::NeedsRevision; }
    ));
    snapshot.candidate_artifact_draft_review_mutation_enabled_count = static_cast<std::size_t>(std::count_if(
        draft_review_report.reviews.begin(), draft_review_report.reviews.end(),
        [](const CandidateArtifactDraftReview& review) {
            return review.current_artifact_insertion_enabled ||
                   review.current_public_claim_insertion_enabled ||
                   review.current_discovery_scheduling_enabled ||
                   review.hidden_truth_mutation_enabled ||
                   review.public_archive_mutation_enabled ||
                   review.persistence_enabled ||
                   review.final_artifact_prose_generation_enabled;
        }
    ));
    const ControlLayerAuditReport control_report = build_control_layer_audit_report();
    snapshot.control_layer_audit_entry_count = control_report.entries.size();
    snapshot.control_layer_audit_mutation_capable_count = static_cast<std::size_t>(std::count_if(
        control_report.entries.begin(), control_report.entries.end(),
        [](const ControlLayerAuditEntry& entry) { return entry.can_mutate_state; }
    ));
    snapshot.control_layer_audit_report_only_count = static_cast<std::size_t>(std::count_if(
        control_report.entries.begin(), control_report.entries.end(),
        [](const ControlLayerAuditEntry& entry) { return entry.persistence == ControlLayerPersistence::ReportOnly; }
    ));
    snapshot.control_layer_audit_access_gated_count = static_cast<std::size_t>(std::count_if(
        control_report.entries.begin(), control_report.entries.end(),
        [](const ControlLayerAuditEntry& entry) { return entry.access_gated; }
    ));
    for (const ControlLayerAuditEntry& entry : control_report.entries) {
        snapshot.control_layer_audit_known_gap_count += entry.known_gaps.size();
    }
    snapshot.civilization_spec_count = state.civilization_spec_count;
    snapshot.civilization_fragment_count = state.civilization_fragment_count;
    snapshot.validation_errors = validate_full_state(state);

    const std::string material = snapshot_digest_material(state, source_fixture_id, fixture_seed, fixture_archive_year, effective_archive_year);
    snapshot.summary_digest = hex64(fnv1a64(material));
    snapshot.snapshot_id = "snapshot." + source_fixture_id + "." + snapshot.summary_digest;
    return snapshot;
}

[[nodiscard]] std::string format_archive_snapshot(const ArchiveSnapshot& snapshot) {
    std::ostringstream out;
    out << "ArchiveSnapshot:\n";
    out << "- snapshot_id: " << snapshot.snapshot_id << "\n";
    out << "- source_fixture_id: " << snapshot.source_fixture_id << "\n";
    out << "- fixture_seed: " << snapshot.fixture_seed << "\n";
    out << "- state_seed: " << snapshot.state_seed << "\n";
    out << "- fixture_archive_year: " << archive_year_text(snapshot.fixture_archive_year) << "\n";
    out << "- effective_archive_year: " << archive_year_text(snapshot.effective_archive_year) << "\n";
    out << "- hidden_entity_count: " << snapshot.hidden_entity_count << "\n";
    out << "- hidden_event_count: " << snapshot.hidden_event_count << "\n";
    out << "- public_artifact_count: " << snapshot.public_artifact_count << "\n";
    out << "- public_claim_count: " << snapshot.public_claim_count << "\n";
    out << "- contradiction_count: " << snapshot.contradiction_count << "\n";
    out << "- anachronism_report_count: " << snapshot.anachronism_report_count << "\n";
    out << "- mystery_count: " << snapshot.mystery_count << "\n";
    out << "- theory_count: " << snapshot.theory_count << "\n";
    out << "- discovery_count: " << snapshot.discovery_count << "\n";
    out << "- hidden_mutation_record_count: " << snapshot.hidden_mutation_record_count << "\n";
    out << "- evidence_potential_count: " << snapshot.evidence_potential_count << "\n";
    out << "- knowledge_horizon_finding_count: " << snapshot.knowledge_horizon_finding_count << "\n";
    out << "- knowledge_horizon_error_count: " << snapshot.knowledge_horizon_error_count << "\n";
    out << "- contradiction_budget_bucket_count: " << snapshot.contradiction_budget_bucket_count << "\n";
    out << "- contradiction_budget_over_budget_count: " << snapshot.contradiction_budget_over_budget_count << "\n";
    out << "- contradiction_budget_watch_count: " << snapshot.contradiction_budget_watch_count << "\n";
    out << "- contradiction_budget_too_clean_count: " << snapshot.contradiction_budget_too_clean_count << "\n";
    out << "- contradiction_budget_productive_ambiguity_count: " << snapshot.contradiction_budget_productive_ambiguity_count << "\n";
    out << "- contradiction_budget_generation_bug_count: " << snapshot.contradiction_budget_generation_bug_count << "\n";
    out << "- candidate_artifact_plan_count: " << snapshot.candidate_artifact_plan_count << "\n";
    out << "- candidate_artifact_plan_blocked_count: " << snapshot.candidate_artifact_plan_blocked_count << "\n";
    out << "- candidate_artifact_plan_curator_review_count: " << snapshot.candidate_artifact_plan_curator_review_count << "\n";
    out << "- candidate_artifact_plan_evaluation_count: " << snapshot.candidate_artifact_plan_evaluation_count << "\n";
    out << "- candidate_artifact_plan_evaluation_pass_count: " << snapshot.candidate_artifact_plan_evaluation_pass_count << "\n";
    out << "- candidate_artifact_plan_evaluation_blocked_count: " << snapshot.candidate_artifact_plan_evaluation_blocked_count << "\n";
    out << "- candidate_artifact_plan_evaluation_review_count: " << snapshot.candidate_artifact_plan_evaluation_review_count << "\n";
    out << "- candidate_artifact_proposal_count: " << snapshot.candidate_artifact_proposal_count << "\n";
    out << "- candidate_artifact_proposal_draftable_count: " << snapshot.candidate_artifact_proposal_draftable_count << "\n";
    out << "- candidate_artifact_proposal_blocked_count: " << snapshot.candidate_artifact_proposal_blocked_count << "\n";
    out << "- candidate_artifact_proposal_review_count: " << snapshot.candidate_artifact_proposal_review_count << "\n";
    out << "- candidate_artifact_proposal_audit_count: " << snapshot.candidate_artifact_proposal_audit_count << "\n";
    out << "- candidate_artifact_proposal_audit_pass_count: " << snapshot.candidate_artifact_proposal_audit_pass_count << "\n";
    out << "- candidate_artifact_proposal_audit_blocked_count: " << snapshot.candidate_artifact_proposal_audit_blocked_count << "\n";
    out << "- candidate_artifact_proposal_audit_review_count: " << snapshot.candidate_artifact_proposal_audit_review_count << "\n";
    out << "- candidate_artifact_proposal_audit_revision_count: " << snapshot.candidate_artifact_proposal_audit_revision_count << "\n";
    out << "- candidate_artifact_draft_count: " << snapshot.candidate_artifact_draft_count << "\n";
    out << "- candidate_artifact_draft_ready_count: " << snapshot.candidate_artifact_draft_ready_count << "\n";
    out << "- candidate_artifact_draft_blocked_count: " << snapshot.candidate_artifact_draft_blocked_count << "\n";
    out << "- candidate_artifact_draft_review_count: " << snapshot.candidate_artifact_draft_review_count << "\n";
    out << "- candidate_artifact_draft_revision_count: " << snapshot.candidate_artifact_draft_revision_count << "\n";
    out << "- candidate_artifact_draft_mutation_enabled_count: " << snapshot.candidate_artifact_draft_mutation_enabled_count << "\n";
    out << "- candidate_artifact_draft_review_record_count: " << snapshot.candidate_artifact_draft_review_record_count << "\n";
    out << "- candidate_artifact_draft_review_pass_count: " << snapshot.candidate_artifact_draft_review_pass_count << "\n";
    out << "- candidate_artifact_draft_review_blocked_count: " << snapshot.candidate_artifact_draft_review_blocked_count << "\n";
    out << "- candidate_artifact_draft_review_curator_review_count: " << snapshot.candidate_artifact_draft_review_curator_review_count << "\n";
    out << "- candidate_artifact_draft_review_revision_count: " << snapshot.candidate_artifact_draft_review_revision_count << "\n";
    out << "- candidate_artifact_draft_review_mutation_enabled_count: " << snapshot.candidate_artifact_draft_review_mutation_enabled_count << "\n";
    out << "- control_layer_audit_entry_count: " << snapshot.control_layer_audit_entry_count << "\n";
    out << "- control_layer_audit_mutation_capable_count: " << snapshot.control_layer_audit_mutation_capable_count << "\n";
    out << "- control_layer_audit_report_only_count: " << snapshot.control_layer_audit_report_only_count << "\n";
    out << "- control_layer_audit_access_gated_count: " << snapshot.control_layer_audit_access_gated_count << "\n";
    out << "- control_layer_audit_known_gap_count: " << snapshot.control_layer_audit_known_gap_count << "\n";
    out << "- civilization_spec_count: " << snapshot.civilization_spec_count << "\n";
    out << "- civilization_fragment_count: " << snapshot.civilization_fragment_count << "\n";
    out << "- summary_digest: " << snapshot.summary_digest << "\n";
    out << "- validation: " << (snapshot.validation_errors.empty() ? "passed" : "failed") << "\n";
    if (!snapshot.validation_errors.empty()) {
        out << "- validation_errors:\n";
        for (const std::string& error : snapshot.validation_errors) {
            out << "  - " << error << "\n";
        }
    }
    return out.str();
}

[[nodiscard]] ArchiveSnapshotComparison compare_archive_snapshots(
    const ArchiveSnapshot& before,
    const ArchiveSnapshot& after
) {
    ArchiveSnapshotComparison comparison;
    comparison.before = before;
    comparison.after = after;
    comparison.same = before.summary_digest == after.summary_digest &&
                      before.hidden_entity_count == after.hidden_entity_count &&
                      before.hidden_event_count == after.hidden_event_count &&
                      before.public_artifact_count == after.public_artifact_count &&
                      before.public_claim_count == after.public_claim_count &&
                      before.contradiction_count == after.contradiction_count &&
                      before.anachronism_report_count == after.anachronism_report_count &&
                      before.mystery_count == after.mystery_count &&
                      before.theory_count == after.theory_count &&
                      before.discovery_count == after.discovery_count &&
                      before.hidden_mutation_record_count == after.hidden_mutation_record_count &&
                      before.evidence_potential_count == after.evidence_potential_count &&
                      before.knowledge_horizon_finding_count == after.knowledge_horizon_finding_count &&
                      before.knowledge_horizon_error_count == after.knowledge_horizon_error_count &&
                      before.contradiction_budget_bucket_count == after.contradiction_budget_bucket_count &&
                      before.contradiction_budget_over_budget_count == after.contradiction_budget_over_budget_count &&
                      before.contradiction_budget_watch_count == after.contradiction_budget_watch_count &&
                      before.contradiction_budget_too_clean_count == after.contradiction_budget_too_clean_count &&
                      before.contradiction_budget_productive_ambiguity_count == after.contradiction_budget_productive_ambiguity_count &&
                      before.contradiction_budget_generation_bug_count == after.contradiction_budget_generation_bug_count &&
                      before.candidate_artifact_plan_count == after.candidate_artifact_plan_count &&
                      before.candidate_artifact_plan_blocked_count == after.candidate_artifact_plan_blocked_count &&
                      before.candidate_artifact_plan_curator_review_count == after.candidate_artifact_plan_curator_review_count &&
                      before.candidate_artifact_plan_evaluation_count == after.candidate_artifact_plan_evaluation_count &&
                      before.candidate_artifact_plan_evaluation_pass_count == after.candidate_artifact_plan_evaluation_pass_count &&
                      before.candidate_artifact_plan_evaluation_blocked_count == after.candidate_artifact_plan_evaluation_blocked_count &&
                      before.candidate_artifact_plan_evaluation_review_count == after.candidate_artifact_plan_evaluation_review_count &&
                      before.candidate_artifact_proposal_count == after.candidate_artifact_proposal_count &&
                      before.candidate_artifact_proposal_draftable_count == after.candidate_artifact_proposal_draftable_count &&
                      before.candidate_artifact_proposal_blocked_count == after.candidate_artifact_proposal_blocked_count &&
                      before.candidate_artifact_proposal_review_count == after.candidate_artifact_proposal_review_count &&
                      before.candidate_artifact_proposal_audit_count == after.candidate_artifact_proposal_audit_count &&
                      before.candidate_artifact_proposal_audit_pass_count == after.candidate_artifact_proposal_audit_pass_count &&
                      before.candidate_artifact_proposal_audit_blocked_count == after.candidate_artifact_proposal_audit_blocked_count &&
                      before.candidate_artifact_proposal_audit_review_count == after.candidate_artifact_proposal_audit_review_count &&
                      before.candidate_artifact_proposal_audit_revision_count == after.candidate_artifact_proposal_audit_revision_count &&
                       before.candidate_artifact_draft_count == after.candidate_artifact_draft_count &&
                       before.candidate_artifact_draft_ready_count == after.candidate_artifact_draft_ready_count &&
                       before.candidate_artifact_draft_blocked_count == after.candidate_artifact_draft_blocked_count &&
                       before.candidate_artifact_draft_review_count == after.candidate_artifact_draft_review_count &&
                       before.candidate_artifact_draft_revision_count == after.candidate_artifact_draft_revision_count &&
                       before.candidate_artifact_draft_mutation_enabled_count == after.candidate_artifact_draft_mutation_enabled_count &&
                       before.candidate_artifact_draft_review_record_count == after.candidate_artifact_draft_review_record_count &&
                       before.candidate_artifact_draft_review_pass_count == after.candidate_artifact_draft_review_pass_count &&
                       before.candidate_artifact_draft_review_blocked_count == after.candidate_artifact_draft_review_blocked_count &&
                       before.candidate_artifact_draft_review_curator_review_count == after.candidate_artifact_draft_review_curator_review_count &&
                       before.candidate_artifact_draft_review_revision_count == after.candidate_artifact_draft_review_revision_count &&
                       before.candidate_artifact_draft_review_mutation_enabled_count == after.candidate_artifact_draft_review_mutation_enabled_count &&
                      before.control_layer_audit_entry_count == after.control_layer_audit_entry_count &&
                      before.control_layer_audit_mutation_capable_count == after.control_layer_audit_mutation_capable_count &&
                      before.control_layer_audit_report_only_count == after.control_layer_audit_report_only_count &&
                      before.control_layer_audit_access_gated_count == after.control_layer_audit_access_gated_count &&
                      before.control_layer_audit_known_gap_count == after.control_layer_audit_known_gap_count &&
                      before.civilization_spec_count == after.civilization_spec_count &&
                      before.civilization_fragment_count == after.civilization_fragment_count &&
                      before.validation_errors == after.validation_errors;
    return comparison;
}

[[nodiscard]] std::string format_archive_snapshot_comparison(
    const ArchiveSnapshot& before,
    const ArchiveSnapshot& after
) {
    const ArchiveSnapshotComparison comparison = compare_archive_snapshots(before, after);
    std::ostringstream out;
    out << "ArchiveSnapshot comparison:\n";
    out << "- result: " << (comparison.same ? "same" : "different") << "\n";
    out << "- before_summary_digest: " << before.summary_digest << "\n";
    out << "- after_summary_digest: " << after.summary_digest << "\n";
    out << "Count deltas:\n";
    format_count_delta(out, "hidden_entity_count", before.hidden_entity_count, after.hidden_entity_count);
    format_count_delta(out, "hidden_event_count", before.hidden_event_count, after.hidden_event_count);
    format_count_delta(out, "public_artifact_count", before.public_artifact_count, after.public_artifact_count);
    format_count_delta(out, "public_claim_count", before.public_claim_count, after.public_claim_count);
    format_count_delta(out, "contradiction_count", before.contradiction_count, after.contradiction_count);
    format_count_delta(out, "anachronism_report_count", before.anachronism_report_count, after.anachronism_report_count);
    format_count_delta(out, "mystery_count", before.mystery_count, after.mystery_count);
    format_count_delta(out, "theory_count", before.theory_count, after.theory_count);
    format_count_delta(out, "discovery_count", before.discovery_count, after.discovery_count);
    format_count_delta(out, "hidden_mutation_record_count", before.hidden_mutation_record_count, after.hidden_mutation_record_count);
    format_count_delta(out, "evidence_potential_count", before.evidence_potential_count, after.evidence_potential_count);
    format_count_delta(out, "knowledge_horizon_finding_count", before.knowledge_horizon_finding_count, after.knowledge_horizon_finding_count);
    format_count_delta(out, "knowledge_horizon_error_count", before.knowledge_horizon_error_count, after.knowledge_horizon_error_count);
    format_count_delta(out, "contradiction_budget_bucket_count", before.contradiction_budget_bucket_count, after.contradiction_budget_bucket_count);
    format_count_delta(out, "contradiction_budget_over_budget_count", before.contradiction_budget_over_budget_count, after.contradiction_budget_over_budget_count);
    format_count_delta(out, "contradiction_budget_watch_count", before.contradiction_budget_watch_count, after.contradiction_budget_watch_count);
    format_count_delta(out, "contradiction_budget_too_clean_count", before.contradiction_budget_too_clean_count, after.contradiction_budget_too_clean_count);
    format_count_delta(out, "contradiction_budget_productive_ambiguity_count", before.contradiction_budget_productive_ambiguity_count, after.contradiction_budget_productive_ambiguity_count);
    format_count_delta(out, "contradiction_budget_generation_bug_count", before.contradiction_budget_generation_bug_count, after.contradiction_budget_generation_bug_count);
    format_count_delta(out, "candidate_artifact_plan_count", before.candidate_artifact_plan_count, after.candidate_artifact_plan_count);
    format_count_delta(out, "candidate_artifact_plan_blocked_count", before.candidate_artifact_plan_blocked_count, after.candidate_artifact_plan_blocked_count);
    format_count_delta(out, "candidate_artifact_plan_curator_review_count", before.candidate_artifact_plan_curator_review_count, after.candidate_artifact_plan_curator_review_count);
    format_count_delta(out, "candidate_artifact_plan_evaluation_count", before.candidate_artifact_plan_evaluation_count, after.candidate_artifact_plan_evaluation_count);
    format_count_delta(out, "candidate_artifact_plan_evaluation_pass_count", before.candidate_artifact_plan_evaluation_pass_count, after.candidate_artifact_plan_evaluation_pass_count);
    format_count_delta(out, "candidate_artifact_plan_evaluation_blocked_count", before.candidate_artifact_plan_evaluation_blocked_count, after.candidate_artifact_plan_evaluation_blocked_count);
    format_count_delta(out, "candidate_artifact_plan_evaluation_review_count", before.candidate_artifact_plan_evaluation_review_count, after.candidate_artifact_plan_evaluation_review_count);
    format_count_delta(out, "candidate_artifact_proposal_count", before.candidate_artifact_proposal_count, after.candidate_artifact_proposal_count);
    format_count_delta(out, "candidate_artifact_proposal_draftable_count", before.candidate_artifact_proposal_draftable_count, after.candidate_artifact_proposal_draftable_count);
    format_count_delta(out, "candidate_artifact_proposal_blocked_count", before.candidate_artifact_proposal_blocked_count, after.candidate_artifact_proposal_blocked_count);
    format_count_delta(out, "candidate_artifact_proposal_review_count", before.candidate_artifact_proposal_review_count, after.candidate_artifact_proposal_review_count);
    format_count_delta(out, "candidate_artifact_draft_count", before.candidate_artifact_draft_count, after.candidate_artifact_draft_count);
    format_count_delta(out, "candidate_artifact_draft_ready_count", before.candidate_artifact_draft_ready_count, after.candidate_artifact_draft_ready_count);
    format_count_delta(out, "candidate_artifact_draft_blocked_count", before.candidate_artifact_draft_blocked_count, after.candidate_artifact_draft_blocked_count);
    format_count_delta(out, "candidate_artifact_draft_review_count", before.candidate_artifact_draft_review_count, after.candidate_artifact_draft_review_count);
    format_count_delta(out, "candidate_artifact_draft_revision_count", before.candidate_artifact_draft_revision_count, after.candidate_artifact_draft_revision_count);
    format_count_delta(out, "candidate_artifact_draft_mutation_enabled_count", before.candidate_artifact_draft_mutation_enabled_count, after.candidate_artifact_draft_mutation_enabled_count);
    format_count_delta(out, "candidate_artifact_draft_review_record_count", before.candidate_artifact_draft_review_record_count, after.candidate_artifact_draft_review_record_count);
    format_count_delta(out, "candidate_artifact_draft_review_pass_count", before.candidate_artifact_draft_review_pass_count, after.candidate_artifact_draft_review_pass_count);
    format_count_delta(out, "candidate_artifact_draft_review_blocked_count", before.candidate_artifact_draft_review_blocked_count, after.candidate_artifact_draft_review_blocked_count);
    format_count_delta(out, "candidate_artifact_draft_review_curator_review_count", before.candidate_artifact_draft_review_curator_review_count, after.candidate_artifact_draft_review_curator_review_count);
    format_count_delta(out, "candidate_artifact_draft_review_revision_count", before.candidate_artifact_draft_review_revision_count, after.candidate_artifact_draft_review_revision_count);
    format_count_delta(out, "candidate_artifact_draft_review_mutation_enabled_count", before.candidate_artifact_draft_review_mutation_enabled_count, after.candidate_artifact_draft_review_mutation_enabled_count);
    format_count_delta(out, "control_layer_audit_entry_count", before.control_layer_audit_entry_count, after.control_layer_audit_entry_count);
    format_count_delta(out, "control_layer_audit_mutation_capable_count", before.control_layer_audit_mutation_capable_count, after.control_layer_audit_mutation_capable_count);
    format_count_delta(out, "control_layer_audit_report_only_count", before.control_layer_audit_report_only_count, after.control_layer_audit_report_only_count);
    format_count_delta(out, "control_layer_audit_access_gated_count", before.control_layer_audit_access_gated_count, after.control_layer_audit_access_gated_count);
    format_count_delta(out, "control_layer_audit_known_gap_count", before.control_layer_audit_known_gap_count, after.control_layer_audit_known_gap_count);
    format_count_delta(out, "civilization_spec_count", before.civilization_spec_count, after.civilization_spec_count);
    format_count_delta(out, "civilization_fragment_count", before.civilization_fragment_count, after.civilization_fragment_count);
    out << "- validation_errors_before: " << before.validation_errors.size() << "\n";
    out << "- validation_errors_after: " << after.validation_errors.size() << "\n";
    if (!before.validation_errors.empty()) {
        out << "- before_validation_errors:\n";
        for (const std::string& error : before.validation_errors) {
            out << "  - " << error << "\n";
        }
    }
    if (!after.validation_errors.empty()) {
        out << "- after_validation_errors:\n";
        for (const std::string& error : after.validation_errors) {
            out << "  - " << error << "\n";
        }
    }
    return out.str();
}

} // namespace archive
