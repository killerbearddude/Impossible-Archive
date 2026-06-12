#include "candidate_artifact_proposal_api.h"
#include "candidate_artifact_plan_api.h"
#include "candidate_artifact_plan_evaluation_api.h"
#include "diagnostic_access_policy.h"

#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {

[[nodiscard]] const CandidateArtifactPlan* find_plan(const ArchiveEngineState& state, const std::string& id) {
    const auto it = std::find_if(state.candidate_artifact_plans.begin(), state.candidate_artifact_plans.end(), [&](const CandidateArtifactPlan& plan) {
        return plan.id == id;
    });
    return it == state.candidate_artifact_plans.end() ? nullptr : &*it;
}


[[nodiscard]] const EvidencePotential* find_potential(const ArchiveEngineState& state, const std::string& id) {
    const auto it = std::find_if(state.evidence_potentials.begin(), state.evidence_potentials.end(), [&](const EvidencePotential& potential) {
        return potential.id == id;
    });
    return it == state.evidence_potentials.end() ? nullptr : &*it;
}

[[nodiscard]] ArtifactVoiceRegister voice_register_for_shape(CandidateArtifactPlanShape shape) {
    switch (shape) {
        case CandidateArtifactPlanShape::AdministrativeDocket: return ArtifactVoiceRegister::DisputedDecree;
        case CandidateArtifactPlanShape::RitualNotice: return ArtifactVoiceRegister::RoyalInscription;
        case CandidateArtifactPlanShape::LedgerEntry: return ArtifactVoiceRegister::TradeLedger;
        case CandidateArtifactPlanShape::BoundaryInscription: return ArtifactVoiceRegister::RoyalInscription;
        case CandidateArtifactPlanShape::ShrineCopy: return ArtifactVoiceRegister::DamagedChronicle;
        case CandidateArtifactPlanShape::ScholarFragment: return ArtifactVoiceRegister::DamagedChronicle;
        case CandidateArtifactPlanShape::OralTraditionFragment: return ArtifactVoiceRegister::OralSong;
        case CandidateArtifactPlanShape::MaterialTrace: return ArtifactVoiceRegister::DamagedChronicle;
        case CandidateArtifactPlanShape::AbsenceRecord: return ArtifactVoiceRegister::DamagedChronicle;
    }
    return ArtifactVoiceRegister::RoyalInscription;
}

[[nodiscard]] std::string readable_shape_title(CandidateArtifactPlanShape shape) {
    std::string text = to_string(shape);
    for (char& ch : text) {
        if (ch == '_') { ch = ' '; }
    }
    return text;
}

[[nodiscard]] bool is_error_finding(const CandidateArtifactPlanEvaluationFinding& finding) {
    return finding.severity == CandidateArtifactPlanEvaluationSeverity::Error;
}

[[nodiscard]] std::vector<std::string> collect_blocking_finding_ids(const CandidateArtifactPlanEvaluation& evaluation) {
    std::vector<std::string> ids;
    for (const CandidateArtifactPlanEvaluationFinding& finding : evaluation.findings) {
        if (is_error_finding(finding)) {
            add_unique_string(ids, finding.id);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

[[nodiscard]] bool has_error_findings(const CandidateArtifactPlanEvaluation& evaluation) {
    return std::any_of(evaluation.findings.begin(), evaluation.findings.end(), is_error_finding);
}

[[nodiscard]] CandidateArtifactProposalCompleteness completeness_for(const CandidateArtifactPlan& plan,
                                                                      const CandidateArtifactPlanEvaluation& evaluation) {
    if (plan.expected_claim_types.empty() || plan.expected_distortion_modes.empty() || evaluation.required_next_checks.empty()) {
        return CandidateArtifactProposalCompleteness::Skeleton;
    }
    if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Pass) {
        return CandidateArtifactProposalCompleteness::Detailed;
    }
    return CandidateArtifactProposalCompleteness::Partial;
}

[[nodiscard]] CandidateArtifactProposalVisibilityClass visibility_class_for(const CandidateArtifactPlan& plan,
                                                                                const CandidateArtifactPlanEvaluation& evaluation) {
    if (!plan.public_safe || !evaluation.public_safe || !plan.knowledge_horizon_finding_ids.empty() || !plan.contradiction_budget_bucket_ids.empty()) {
        return CandidateArtifactProposalVisibilityClass::CuratorOnly;
    }
    if (plan.requires_curator_review || evaluation.decision == CandidateArtifactPlanEvaluationDecision::NeedsCuratorReview) {
        return CandidateArtifactProposalVisibilityClass::ScholarEligible;
    }
    return CandidateArtifactProposalVisibilityClass::PublicEligible;
}

[[nodiscard]] bool touches_protected_mystery(const CandidateArtifactPlanEvaluation& evaluation) {
    return std::any_of(evaluation.findings.begin(), evaluation.findings.end(), [](const CandidateArtifactPlanEvaluationFinding& finding) {
        return finding.gate == CandidateArtifactPlanEvaluationGate::ProtectedMystery;
    });
}

void add_claim_skeletons(CandidateArtifactProposal& proposal, const CandidateArtifactPlan& plan) {
    for (const std::string& claim_type : plan.expected_claim_types) {
        add_unique_string(proposal.proposed_claim_skeletons,
                          claim_type + " skeleton for " + proposal.evidence_role + " (not a PublicClaim)");
    }
    if (proposal.proposed_claim_skeletons.empty()) {
        add_unique_string(proposal.proposed_claim_skeletons,
                          "factual_claim skeleton for " + proposal.evidence_role + " (not a PublicClaim)");
    }
}

void add_damage_modes(CandidateArtifactProposal& proposal) {
    switch (proposal.proposed_artifact_type) {
        case ArtifactType::Inscription:
            add_unique_string(proposal.proposed_damage_modes, "surface_erosion");
            add_unique_string(proposal.proposed_damage_modes, "partial_line_loss");
            break;
        case ArtifactType::DamagedManuscript:
            add_unique_string(proposal.proposed_damage_modes, "lacunae");
            add_unique_string(proposal.proposed_damage_modes, "copyist_gap");
            break;
        case ArtifactType::ForgedDecree:
            add_unique_string(proposal.proposed_damage_modes, "seal_uncertainty");
            add_unique_string(proposal.proposed_damage_modes, "formulaic_interpolation");
            break;
        case ArtifactType::OralHistory:
            add_unique_string(proposal.proposed_damage_modes, "oral_formula_substitution");
            break;
        case ArtifactType::TradeLedger:
            add_unique_string(proposal.proposed_damage_modes, "numeric_lacuna");
            add_unique_string(proposal.proposed_damage_modes, "ledger_edge_loss");
            break;
    }
}

void add_validation_gates(CandidateArtifactProposal& proposal, const CandidateArtifactPlanEvaluation& evaluation) {
    for (const CandidateArtifactPlanEvaluationFinding& finding : evaluation.findings) {
        add_unique_string(proposal.proposed_validation_gates,
                          to_string(finding.gate) + ": " + to_string(finding.severity));
    }
    for (const std::string& check : evaluation.required_next_checks) {
        add_unique_string(proposal.proposed_validation_gates, check);
    }
    add_unique_string(proposal.proposed_validation_gates, "candidate proposal remains read-only in v28.7");
    add_unique_string(proposal.proposed_validation_gates, "do not generate artifact text from proposal in v28.7");
}

void add_access_notes(CandidateArtifactProposal& proposal) {
    switch (proposal.visibility_class) {
        case CandidateArtifactProposalVisibilityClass::PublicEligible:
            add_unique_string(proposal.required_access_notes, "public-eligible summary only; still no artifact generation");
            break;
        case CandidateArtifactProposalVisibilityClass::ScholarEligible:
            add_unique_string(proposal.required_access_notes, "scholar-eligible summary only; diagnostic IDs remain restricted");
            break;
        case CandidateArtifactProposalVisibilityClass::CuratorOnly:
            add_unique_string(proposal.required_access_notes, "curator/debug diagnostics required for source IDs and validation gates");
            break;
        case CandidateArtifactProposalVisibilityClass::DebugOnly:
            add_unique_string(proposal.required_access_notes, "debug-only proposal diagnostics");
            break;
    }
}

[[nodiscard]] std::vector<CandidateArtifactProposal> proposals_for_formatting(const ArchiveEngineState& state, AccessLevel access) {
    if (!state.candidate_artifact_proposals.empty()) {
        return state.candidate_artifact_proposals;
    }
    return draft_candidate_artifact_proposals(state, access).proposals;
}

[[nodiscard]] bool id_in(const std::vector<std::string>& values, const std::string& id) {
    return std::find(values.begin(), values.end(), id) != values.end();
}

[[nodiscard]] bool public_safe_exposes_hidden_diagnostic(const CandidateArtifactProposal& proposal) {
    if (proposal.visibility_class != CandidateArtifactProposalVisibilityClass::PublicEligible &&
        proposal.visibility_class != CandidateArtifactProposalVisibilityClass::ScholarEligible) {
        return false;
    }
    if (!proposal.source_evidence_potential_id.empty()) {
        return true;
    }
    for (const std::string& id : proposal.blocking_evaluation_finding_ids) {
        if (!id.empty()) { return true; }
    }
    for (const std::string& gate : proposal.proposed_validation_gates) {
        if (gate.find("knowledge_horizon") != std::string::npos ||
            gate.find("contradiction_budget") != std::string::npos ||
            gate.find("protected_mystery") != std::string::npos) {
            return true;
        }
    }
    return false;
}

void append_counts(std::ostringstream& out, const std::map<std::string, std::size_t>& counts) {
    for (const auto& [label, count] : counts) {
        out << "- " << label << ": " << count << "\n";
    }
}

} // namespace

[[nodiscard]] std::string to_string(CandidateArtifactProposalDecision decision) {
    switch (decision) {
        case CandidateArtifactProposalDecision::Draftable: return "draftable";
        case CandidateArtifactProposalDecision::NeedsCuratorReview: return "needs_curator_review";
        case CandidateArtifactProposalDecision::Blocked: return "blocked";
        case CandidateArtifactProposalDecision::Invalid: return "invalid";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactProposalCompleteness completeness) {
    switch (completeness) {
        case CandidateArtifactProposalCompleteness::Skeleton: return "skeleton";
        case CandidateArtifactProposalCompleteness::Partial: return "partial";
        case CandidateArtifactProposalCompleteness::Detailed: return "detailed";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactProposalSafety safety) {
    switch (safety) {
        case CandidateArtifactProposalSafety::PublicSafe: return "public_safe";
        case CandidateArtifactProposalSafety::ScholarSafe: return "scholar_safe";
        case CandidateArtifactProposalSafety::CuratorOnly: return "curator_only";
        case CandidateArtifactProposalSafety::DebugOnly: return "debug_only";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactProposalVisibilityClass visibility_class) {
    switch (visibility_class) {
        case CandidateArtifactProposalVisibilityClass::PublicEligible: return "public_eligible";
        case CandidateArtifactProposalVisibilityClass::ScholarEligible: return "scholar_eligible";
        case CandidateArtifactProposalVisibilityClass::CuratorOnly: return "curator_only";
        case CandidateArtifactProposalVisibilityClass::DebugOnly: return "debug_only";
    }
    return "unknown";
}

[[nodiscard]] CandidateArtifactProposalSafety proposal_safety_for_access(const CandidateArtifactProposal& proposal, AccessLevel access) {
    if (can_view(access, AccessLevel::Debug) && proposal.visibility_class == CandidateArtifactProposalVisibilityClass::DebugOnly) {
        return CandidateArtifactProposalSafety::DebugOnly;
    }
    if (can_view(access, AccessLevel::Curator)) {
        return proposal.visibility_class == CandidateArtifactProposalVisibilityClass::DebugOnly
            ? CandidateArtifactProposalSafety::DebugOnly
            : CandidateArtifactProposalSafety::CuratorOnly;
    }
    if (proposal.visibility_class == CandidateArtifactProposalVisibilityClass::PublicEligible) {
        return CandidateArtifactProposalSafety::PublicSafe;
    }
    if (proposal.visibility_class == CandidateArtifactProposalVisibilityClass::ScholarEligible && can_view(access, AccessLevel::Scholar)) {
        return CandidateArtifactProposalSafety::ScholarSafe;
    }
    return CandidateArtifactProposalSafety::CuratorOnly;
}

[[nodiscard]] bool candidate_artifact_proposal_visible_to(const CandidateArtifactProposal& proposal, AccessLevel access) {
    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposal)) {
        return true;
    }
    if (proposal.visibility_class == CandidateArtifactProposalVisibilityClass::PublicEligible) {
        return true;
    }
    return proposal.visibility_class == CandidateArtifactProposalVisibilityClass::ScholarEligible && can_view(access, AccessLevel::Scholar);
}

[[nodiscard]] std::string to_string(CandidateArtifactProposalTextStatus status) {
    switch (status) {
        case CandidateArtifactProposalTextStatus::NoTextGenerated: return "no_text_generated";
        case CandidateArtifactProposalTextStatus::OutlineOnly: return "outline_only";
        case CandidateArtifactProposalTextStatus::PlaceholderOnly: return "placeholder_only";
    }
    return "unknown";
}

[[nodiscard]] CandidateArtifactProposalDecision classify_candidate_artifact_proposal(
    const CandidateArtifactPlanEvaluation& evaluation
) {
    if (evaluation.id.empty() || evaluation.plan_id.empty() || evaluation.current_generation_enabled || evaluation.current_materialization_enabled) {
        return CandidateArtifactProposalDecision::Invalid;
    }
    if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Invalid) {
        return CandidateArtifactProposalDecision::Invalid;
    }
    if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Blocked || has_error_findings(evaluation)) {
        return CandidateArtifactProposalDecision::Blocked;
    }
    if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Pass) {
        return CandidateArtifactProposalDecision::Draftable;
    }
    return CandidateArtifactProposalDecision::NeedsCuratorReview;
}

[[nodiscard]] CandidateArtifactProposal draft_candidate_artifact_proposal(const ArchiveEngineState& state,
                                                                           const CandidateArtifactPlanEvaluation& evaluation,
                                                                           AccessLevel access) {
    (void)access;
    CandidateArtifactProposal proposal;
    proposal.id = "candidate_artifact_proposal." + evaluation.id;
    proposal.evaluation_id = evaluation.id;
    proposal.plan_id = evaluation.plan_id;
    proposal.text_status = CandidateArtifactProposalTextStatus::NoTextGenerated;
    proposal.current_generation_enabled = false;
    proposal.current_materialization_enabled = false;
    proposal.archive_mutation_enabled = false;

    const CandidateArtifactPlan* plan = find_plan(state, evaluation.plan_id);
    if (plan == nullptr) {
        proposal.decision = CandidateArtifactProposalDecision::Invalid;
        proposal.visibility_class = CandidateArtifactProposalVisibilityClass::CuratorOnly;
        proposal.proposed_title = "Invalid candidate artifact proposal";
        proposal.evidence_role = "missing plan reference";
        proposal.proposal_rationale = "CandidateArtifactPlanEvaluation references a missing CandidateArtifactPlan.";
        proposal.warnings.push_back("missing CandidateArtifactPlan: " + evaluation.plan_id);
        return proposal;
    }

    proposal.source_evidence_potential_id = plan->source_id;
    proposal.proposed_shape = plan->planned_shape;
    proposal.proposed_artifact_type = plan->planned_artifact_type;
    proposal.proposed_voice_register = voice_register_for_shape(plan->planned_shape);
    proposal.target_topic = plan->target_topic;
    proposal.proposed_creation_year = plan->target_year;
    proposal.proposed_discovery_year = plan->target_year <= 0 ? 0 : plan->target_year + 80;
    proposal.evidence_role = plan->evidence_role;
    proposal.proposal_rationale = "Read-only proposal drafted from " + evaluation.id + "; " + plan->rationale;
    proposal.proposed_title = "Proposed " + readable_shape_title(plan->planned_shape) + " for " + (plan->target_topic.empty() ? plan->source_id : plan->target_topic);
    proposal.completeness = completeness_for(*plan, evaluation);
    proposal.visibility_class = visibility_class_for(*plan, evaluation);
    proposal.contains_hidden_source_reference = !proposal.source_evidence_potential_id.empty();
    proposal.contains_curator_diagnostics = !plan->knowledge_horizon_finding_ids.empty() ||
                                           !plan->contradiction_budget_bucket_ids.empty() ||
                                           !evaluation.findings.empty();
    proposal.touches_protected_mystery = touches_protected_mystery(evaluation);
    proposal.requires_curator_review = plan->requires_curator_review ||
                                       evaluation.decision == CandidateArtifactPlanEvaluationDecision::NeedsCuratorReview;
    proposal.decision = classify_candidate_artifact_proposal(evaluation);
    proposal.blocking_evaluation_finding_ids = collect_blocking_finding_ids(evaluation);
    proposal.proposed_distortion_modes = plan->expected_distortion_modes;
    add_claim_skeletons(proposal, *plan);
    add_damage_modes(proposal);
    add_validation_gates(proposal, evaluation);
    add_access_notes(proposal);

    if (find_potential(state, plan->source_id) == nullptr) {
        proposal.decision = CandidateArtifactProposalDecision::Invalid;
        proposal.warnings.push_back("missing source EvidencePotential: " + plan->source_id);
    }
    if (plan->requires_curator_review && proposal.decision == CandidateArtifactProposalDecision::Draftable) {
        proposal.decision = CandidateArtifactProposalDecision::NeedsCuratorReview;
    }
    if (proposal.visibility_class == CandidateArtifactProposalVisibilityClass::CuratorOnly && proposal.decision == CandidateArtifactProposalDecision::Draftable) {
        proposal.decision = CandidateArtifactProposalDecision::NeedsCuratorReview;
    }
    if (!proposal.blocking_evaluation_finding_ids.empty() && proposal.decision == CandidateArtifactProposalDecision::Draftable) {
        proposal.decision = CandidateArtifactProposalDecision::Blocked;
    }
    return proposal;
}

[[nodiscard]] CandidateArtifactProposalReport draft_candidate_artifact_proposals(const ArchiveEngineState& state, AccessLevel access) {
    (void)access;
    CandidateArtifactProposalReport report;
    std::vector<CandidateArtifactPlanEvaluation> evaluations = state.candidate_artifact_plan_evaluations;
    if (evaluations.empty()) {
        evaluations = evaluate_candidate_artifact_plans(state, AccessLevel::Curator).evaluations;
    }
    for (const CandidateArtifactPlanEvaluation& evaluation : evaluations) {
        report.proposals.push_back(draft_candidate_artifact_proposal(state, evaluation, access));
    }
    std::sort(report.proposals.begin(), report.proposals.end(), [](const CandidateArtifactProposal& lhs, const CandidateArtifactProposal& rhs) {
        return lhs.id < rhs.id;
    });
    ArchiveEngineState validation_state = state;
    validation_state.candidate_artifact_proposals = report.proposals;
    report.errors = validate_candidate_artifact_proposals(validation_state);
    return report;
}

void draft_candidate_artifact_proposals_into_state(ArchiveEngineState& state, AccessLevel access) {
    (void)access;
    state.candidate_artifact_proposals = draft_candidate_artifact_proposals(state, AccessLevel::Curator).proposals;
}

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_proposals(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> seen_ids;
    std::vector<std::string> plan_ids;
    std::vector<std::string> evaluation_ids;
    std::vector<std::string> potential_ids;
    for (const CandidateArtifactPlan& plan : state.candidate_artifact_plans) {
        plan_ids.push_back(plan.id);
    }
    for (const CandidateArtifactPlanEvaluation& evaluation : state.candidate_artifact_plan_evaluations) {
        evaluation_ids.push_back(evaluation.id);
    }
    for (const EvidencePotential& potential : state.evidence_potentials) {
        potential_ids.push_back(potential.id);
    }
    for (const CandidateArtifactProposal& proposal : state.candidate_artifact_proposals) {
        if (proposal.id.empty()) {
            errors.push_back("CandidateArtifactProposal has empty id");
        } else if (!seen_ids.insert(proposal.id).second) {
            errors.push_back("CandidateArtifactProposal has duplicate id: " + proposal.id);
        }
        if (proposal.plan_id.empty()) {
            errors.push_back("CandidateArtifactProposal has empty plan id: " + proposal.id);
        } else if (!id_in(plan_ids, proposal.plan_id)) {
            errors.push_back("CandidateArtifactProposal references missing plan: " + proposal.id + " -> " + proposal.plan_id);
        }
        if (proposal.evaluation_id.empty()) {
            errors.push_back("CandidateArtifactProposal has empty evaluation id: " + proposal.id);
        } else if (!id_in(evaluation_ids, proposal.evaluation_id)) {
            errors.push_back("CandidateArtifactProposal references missing evaluation: " + proposal.id + " -> " + proposal.evaluation_id);
        }
        if (proposal.source_evidence_potential_id.empty()) {
            errors.push_back("CandidateArtifactProposal has empty source EvidencePotential id: " + proposal.id);
        } else if (!id_in(potential_ids, proposal.source_evidence_potential_id)) {
            errors.push_back("CandidateArtifactProposal references missing EvidencePotential: " + proposal.id + " -> " + proposal.source_evidence_potential_id);
        }
        if (proposal.proposed_creation_year < 0) {
            errors.push_back("CandidateArtifactProposal has invalid proposed creation year: " + proposal.id);
        }
        if (proposal.evidence_role.empty()) {
            errors.push_back("CandidateArtifactProposal has empty evidence role: " + proposal.id);
        }
        if (proposal.proposal_rationale.empty()) {
            errors.push_back("CandidateArtifactProposal has empty proposal rationale: " + proposal.id);
        }
        if (proposal.text_status != CandidateArtifactProposalTextStatus::NoTextGenerated) {
            errors.push_back("CandidateArtifactProposal generated text in v28.7: " + proposal.id);
        }
        if (proposal.current_generation_enabled) {
            errors.push_back("CandidateArtifactProposal enables current generation in v28.7: " + proposal.id);
        }
        if (proposal.current_materialization_enabled) {
            errors.push_back("CandidateArtifactProposal enables current materialization in v28.7: " + proposal.id);
        }
        if (proposal.archive_mutation_enabled) {
            errors.push_back("CandidateArtifactProposal enables archive mutation in v28.7: " + proposal.id);
        }
        if (proposal.decision == CandidateArtifactProposalDecision::Draftable && !proposal.blocking_evaluation_finding_ids.empty()) {
            errors.push_back("CandidateArtifactProposal is Draftable with blocking evaluation findings: " + proposal.id);
        }
        if (public_safe_exposes_hidden_diagnostic(proposal)) {
            errors.push_back("CandidateArtifactProposal public/scholar-safe output carries hidden diagnostic IDs: " + proposal.id);
        }
    }
    return errors;
}

[[nodiscard]] std::string format_candidate_artifact_proposal_summary(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactProposalReport report = draft_candidate_artifact_proposals(state, access);
    std::map<std::string, std::size_t> by_decision;
    std::map<std::string, std::size_t> by_visibility;
    std::map<std::string, std::size_t> by_safety;
    std::map<std::string, std::size_t> by_text_status;
    std::size_t generation_enabled = 0;
    std::size_t materialization_enabled = 0;
    std::size_t mutation_enabled = 0;
    for (const CandidateArtifactProposal& proposal : report.proposals) {
        ++by_decision[to_string(proposal.decision)];
        ++by_visibility[to_string(proposal.visibility_class)];
        ++by_safety[to_string(proposal_safety_for_access(proposal, access))];
        ++by_text_status[to_string(proposal.text_status)];
        if (proposal.current_generation_enabled) { ++generation_enabled; }
        if (proposal.current_materialization_enabled) { ++materialization_enabled; }
        if (proposal.archive_mutation_enabled) { ++mutation_enabled; }
    }
    std::ostringstream out;
    out << "CandidateArtifactProposal summary:\n";
    out << "- behavior: proposal drafting only; no Artifact records, final artifact prose, PublicArchive insertion, discovery scheduling, PublicClaim insertion, hidden truth mutation, fragment activation, resolver/composition, persistence, or session state are introduced in v28.7.\n";
    out << "- total_proposals: " << report.proposals.size() << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    out << "- current_generation_enabled: " << generation_enabled << "\n";
    out << "- current_materialization_enabled: " << materialization_enabled << "\n";
    out << "- archive_mutation_enabled: " << mutation_enabled << "\n";
    out << "Decision counts:\n";
    append_counts(out, by_decision);
    out << "Visibility class counts:\n";
    append_counts(out, by_visibility);
    out << "View safety counts:\n";
    append_counts(out, by_safety);
    out << "Text status counts:\n";
    append_counts(out, by_text_status);
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposal)) {
        out << "- details: aggregate-only at this access level; hidden source IDs, diagnostic IDs, protected mystery details, hidden rationale, blocking internals, and curator-only notes are restricted.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_proposal_validation(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactProposalReport report = draft_candidate_artifact_proposals(state, access);
    std::ostringstream out;
    out << "CandidateArtifactProposal validation:\n";
    out << "- result: " << (report.errors.empty() ? "passed" : "failed") << "\n";
    out << "- proposals: " << report.proposals.size() << "\n";
    out << "- errors: " << report.errors.size() << "\n";
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
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_proposal_list(const ArchiveEngineState& state, AccessLevel access) {
    const std::vector<CandidateArtifactProposal> proposals = proposals_for_formatting(state, access);
    std::ostringstream out;
    out << "CandidateArtifactProposals visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposal)) {
        std::size_t visible = 0;
        for (const CandidateArtifactProposal& proposal : proposals) {
            if (candidate_artifact_proposal_visible_to(proposal, access)) { ++visible; }
        }
        out << "- public_safe_visible_proposals: " << visible << "\n";
        out << "- details: public/scholar access receives aggregate counts and public-safe summaries only; hidden IDs, diagnostic gates, and rationale are restricted.\n";
        for (const CandidateArtifactProposal& proposal : proposals) {
            if (!candidate_artifact_proposal_visible_to(proposal, access)) { continue; }
            out << "- decision=" << to_string(proposal.decision)
                << " safety=" << to_string(proposal_safety_for_access(proposal, access))
                << " text_status=" << to_string(proposal.text_status)
                << " current_generation_enabled=false current_materialization_enabled=false archive_mutation_enabled=false\n";
        }
        return out.str();
    }
    if (proposals.empty()) {
        out << "- none\n";
        return out.str();
    }
    for (const CandidateArtifactProposal& proposal : proposals) {
        out << "- " << proposal.id
            << ": evaluation=" << proposal.evaluation_id
            << " plan=" << proposal.plan_id
            << " decision=" << to_string(proposal.decision)
            << " visibility_class=" << to_string(proposal.visibility_class)
            << " safety=" << to_string(proposal_safety_for_access(proposal, access))
            << " text_status=" << to_string(proposal.text_status)
            << " current_generation_enabled=" << (proposal.current_generation_enabled ? "true" : "false")
            << " current_materialization_enabled=" << (proposal.current_materialization_enabled ? "true" : "false")
            << " archive_mutation_enabled=" << (proposal.archive_mutation_enabled ? "true" : "false") << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_proposal_detail(const ArchiveEngineState& state,
                                                                     AccessLevel access,
                                                                     const std::string& proposal_id) {
    const std::vector<CandidateArtifactProposal> proposals = proposals_for_formatting(state, access);
    const auto it = std::find_if(proposals.begin(), proposals.end(), [&](const CandidateArtifactProposal& proposal) {
        return proposal.id == proposal_id;
    });
    std::ostringstream out;
    out << "CandidateArtifactProposal:\n";
    if (it == proposals.end() || !candidate_artifact_proposal_visible_to(*it, access)) {
        out << "- found: false\n";
        return out.str();
    }
    out << "- found: true\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposal)) {
        out << "- decision: " << to_string(it->decision) << "\n";
        out << "- safety: " << to_string(proposal_safety_for_access(*it, access)) << "\n";
        out << "- text_status: " << to_string(it->text_status) << "\n";
        out << "- current_generation_enabled: false\n";
        out << "- current_materialization_enabled: false\n";
        out << "- archive_mutation_enabled: false\n";
        out << "- details: restricted\n";
        return out.str();
    }
    out << "- id: " << it->id << "\n";
    out << "- evaluation_id: " << it->evaluation_id << "\n";
    out << "- plan_id: " << it->plan_id << "\n";
    out << "- source_evidence_potential_id: " << it->source_evidence_potential_id << "\n";
    out << "- decision: " << to_string(it->decision) << "\n";
    out << "- completeness: " << to_string(it->completeness) << "\n";
    out << "- visibility_class: " << to_string(it->visibility_class) << "\n";
    out << "- safety: " << to_string(proposal_safety_for_access(*it, access)) << "\n";
    out << "- contains_hidden_source_reference: " << (it->contains_hidden_source_reference ? "true" : "false") << "\n";
    out << "- contains_curator_diagnostics: " << (it->contains_curator_diagnostics ? "true" : "false") << "\n";
    out << "- touches_protected_mystery: " << (it->touches_protected_mystery ? "true" : "false") << "\n";
    out << "- requires_curator_review: " << (it->requires_curator_review ? "true" : "false") << "\n";
    out << "- text_status: " << to_string(it->text_status) << "\n";
    out << "- proposed_shape: " << to_string(it->proposed_shape) << "\n";
    out << "- proposed_artifact_type: " << to_string(it->proposed_artifact_type) << "\n";
    out << "- proposed_voice_register: " << to_string(it->proposed_voice_register) << "\n";
    out << "- proposed_title: " << it->proposed_title << "\n";
    out << "- target_topic: " << it->target_topic << "\n";
    out << "- proposed_creation_year: " << it->proposed_creation_year << "\n";
    out << "- proposed_discovery_year: " << it->proposed_discovery_year << "\n";
    out << "- evidence_role: " << it->evidence_role << "\n";
    out << "- proposal_rationale: " << it->proposal_rationale << "\n";
    out << "- current_generation_enabled: " << (it->current_generation_enabled ? "true" : "false") << "\n";
    out << "- current_materialization_enabled: " << (it->current_materialization_enabled ? "true" : "false") << "\n";
    out << "- archive_mutation_enabled: " << (it->archive_mutation_enabled ? "true" : "false") << "\n";
    if (!it->proposed_claim_skeletons.empty()) {
        out << "Proposed claim skeletons:\n";
        for (const std::string& skeleton : it->proposed_claim_skeletons) {
            out << "- " << skeleton << "\n";
        }
    }
    if (!it->proposed_distortion_modes.empty()) {
        out << "Proposed distortion modes:\n";
        for (const std::string& mode : it->proposed_distortion_modes) {
            out << "- " << mode << "\n";
        }
    }
    if (!it->proposed_damage_modes.empty()) {
        out << "Proposed damage modes:\n";
        for (const std::string& mode : it->proposed_damage_modes) {
            out << "- " << mode << "\n";
        }
    }
    if (!it->proposed_validation_gates.empty()) {
        out << "Proposed validation gates:\n";
        for (const std::string& gate : it->proposed_validation_gates) {
            out << "- " << gate << "\n";
        }
    }
    if (!it->required_access_notes.empty()) {
        out << "Required access notes:\n";
        for (const std::string& note : it->required_access_notes) {
            out << "- " << note << "\n";
        }
    }
    if (!it->blocking_evaluation_finding_ids.empty()) {
        out << "Blocking evaluation findings:\n";
        for (const std::string& id : it->blocking_evaluation_finding_ids) {
            out << "- " << id << "\n";
        }
    }
    if (!it->warnings.empty()) {
        out << "Warnings:\n";
        for (const std::string& warning : it->warnings) {
            out << "- " << warning << "\n";
        }
    }
    return out.str();
}

} // namespace archive
