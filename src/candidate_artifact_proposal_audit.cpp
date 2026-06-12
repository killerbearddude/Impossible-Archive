#include "candidate_artifact_proposal_audit_api.h"
#include "candidate_artifact_proposal_api.h"
#include "diagnostic_access_policy.h"

#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {

[[nodiscard]] const CandidateArtifactProposal* find_proposal(const ArchiveEngineState& state, const std::string& id) {
    const auto it = std::find_if(state.candidate_artifact_proposals.begin(), state.candidate_artifact_proposals.end(), [&](const CandidateArtifactProposal& proposal) {
        return proposal.id == id;
    });
    return it == state.candidate_artifact_proposals.end() ? nullptr : &*it;
}

[[nodiscard]] const CandidateArtifactPlanEvaluation* find_evaluation(const ArchiveEngineState& state, const std::string& id) {
    const auto it = std::find_if(state.candidate_artifact_plan_evaluations.begin(), state.candidate_artifact_plan_evaluations.end(), [&](const CandidateArtifactPlanEvaluation& evaluation) {
        return evaluation.id == id;
    });
    return it == state.candidate_artifact_plan_evaluations.end() ? nullptr : &*it;
}

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

void add_audit_finding(CandidateArtifactProposalAudit& audit,
                       CandidateArtifactProposalAuditGate gate,
                       CandidateArtifactProposalAuditSeverity severity,
                       CandidateArtifactProposalAuditReasonCode reason_code,
                       const std::string& message,
                       const std::string& related_id) {
    CandidateArtifactProposalAuditFinding finding;
    finding.id = audit.id + ".finding." + std::to_string(audit.findings.size());
    finding.gate = gate;
    finding.severity = severity;
    finding.reason_code = reason_code;
    finding.message = message;
    finding.related_id = related_id;
    audit.findings.push_back(std::move(finding));
    if (severity == CandidateArtifactProposalAuditSeverity::Warning || severity == CandidateArtifactProposalAuditSeverity::Error) {
        add_unique_string(audit.required_revisions, message);
    }
}

[[nodiscard]] std::size_t count_severity(const CandidateArtifactProposalAudit& audit, CandidateArtifactProposalAuditSeverity severity) {
    return static_cast<std::size_t>(std::count_if(audit.findings.begin(), audit.findings.end(), [&](const CandidateArtifactProposalAuditFinding& finding) {
        return finding.severity == severity;
    }));
}

[[nodiscard]] bool has_error_findings(const CandidateArtifactProposalAudit& audit) {
    return count_severity(audit, CandidateArtifactProposalAuditSeverity::Error) > 0U;
}

[[nodiscard]] bool has_error_reason(const CandidateArtifactProposalAudit& audit, CandidateArtifactProposalAuditReasonCode reason_code) {
    return std::any_of(audit.findings.begin(), audit.findings.end(), [&](const CandidateArtifactProposalAuditFinding& finding) {
        return finding.reason_code == reason_code && finding.severity == CandidateArtifactProposalAuditSeverity::Error;
    });
}

[[nodiscard]] bool has_reason(const CandidateArtifactProposalAudit& audit, CandidateArtifactProposalAuditReasonCode reason_code) {
    return std::any_of(audit.findings.begin(), audit.findings.end(), [&](const CandidateArtifactProposalAuditFinding& finding) {
        return finding.reason_code == reason_code;
    });
}

[[nodiscard]] bool is_hard_blocker_reason(CandidateArtifactProposalAuditReasonCode reason_code) {
    switch (reason_code) {
        case CandidateArtifactProposalAuditReasonCode::MissingProposal:
        case CandidateArtifactProposalAuditReasonCode::MissingEvaluation:
        case CandidateArtifactProposalAuditReasonCode::MissingPlan:
        case CandidateArtifactProposalAuditReasonCode::MissingEvidencePotential:
        case CandidateArtifactProposalAuditReasonCode::KnowledgeHorizonBlocked:
        case CandidateArtifactProposalAuditReasonCode::ContradictionBudgetHighPressure:
        case CandidateArtifactProposalAuditReasonCode::PublicAccessLeak:
        case CandidateArtifactProposalAuditReasonCode::GenerationEnabled:
        case CandidateArtifactProposalAuditReasonCode::MaterializationEnabled:
        case CandidateArtifactProposalAuditReasonCode::ArchiveMutationEnabled:
        case CandidateArtifactProposalAuditReasonCode::ScoreOutOfRange:
            return true;
        case CandidateArtifactProposalAuditReasonCode::None:
        case CandidateArtifactProposalAuditReasonCode::MissingClaimSkeleton:
        case CandidateArtifactProposalAuditReasonCode::MissingDistortionMode:
        case CandidateArtifactProposalAuditReasonCode::MissingDamageMode:
        case CandidateArtifactProposalAuditReasonCode::WeakSpecificity:
        case CandidateArtifactProposalAuditReasonCode::WeakVoiceReadiness:
        case CandidateArtifactProposalAuditReasonCode::ProtectedMysteryRisk:
            return false;
    }
    return false;
}

[[nodiscard]] bool has_error_hard_blocker(const CandidateArtifactProposalAudit& audit) {
    return std::any_of(audit.findings.begin(), audit.findings.end(), [](const CandidateArtifactProposalAuditFinding& finding) {
        return finding.severity == CandidateArtifactProposalAuditSeverity::Error && is_hard_blocker_reason(finding.reason_code);
    });
}

[[nodiscard]] bool score_out_of_range(double score) {
    return score < 0.0 || score > 1.0;
}

[[nodiscard]] bool voice_register_matches_shape(CandidateArtifactPlanShape shape, ArtifactVoiceRegister voice) {
    switch (shape) {
        case CandidateArtifactPlanShape::AdministrativeDocket:
            return voice == ArtifactVoiceRegister::TradeLedger || voice == ArtifactVoiceRegister::DisputedDecree;
        case CandidateArtifactPlanShape::RitualNotice:
            return voice == ArtifactVoiceRegister::RoyalInscription || voice == ArtifactVoiceRegister::OralSong;
        case CandidateArtifactPlanShape::LedgerEntry:
            return voice == ArtifactVoiceRegister::TradeLedger;
        case CandidateArtifactPlanShape::BoundaryInscription:
            return voice == ArtifactVoiceRegister::RoyalInscription;
        case CandidateArtifactPlanShape::ShrineCopy:
        case CandidateArtifactPlanShape::ScholarFragment:
        case CandidateArtifactPlanShape::MaterialTrace:
        case CandidateArtifactPlanShape::AbsenceRecord:
            return voice == ArtifactVoiceRegister::DamagedChronicle;
        case CandidateArtifactPlanShape::OralTraditionFragment:
            return voice == ArtifactVoiceRegister::OralSong;
    }
    return false;
}

[[nodiscard]] bool claim_skeletons_are_safe(const CandidateArtifactProposal& proposal) {
    if (proposal.proposed_claim_skeletons.empty()) {
        return false;
    }
    return std::all_of(proposal.proposed_claim_skeletons.begin(), proposal.proposed_claim_skeletons.end(), [](const std::string& skeleton) {
        return contains_substr(skeleton, "skeleton") && contains_substr(skeleton, "not a PublicClaim");
    });
}

[[nodiscard]] bool contains_gate_error(const CandidateArtifactProposal& proposal, std::string_view gate_name) {
    const std::string needle = std::string(gate_name) + ": error";
    return std::any_of(proposal.proposed_validation_gates.begin(), proposal.proposed_validation_gates.end(), [&](const std::string& gate) {
        return contains_substr(gate, needle);
    });
}

[[nodiscard]] bool has_specific_local_content(const CandidateArtifactProposal& proposal) {
    return !proposal.target_topic.empty() &&
           !proposal.evidence_role.empty() &&
           !proposal.source_evidence_potential_id.empty() &&
           (contains_substr(proposal.proposal_rationale, "evidence_potential") ||
            contains_substr(proposal.proposal_rationale, "Read-only proposal") ||
            contains_substr(proposal.proposal_rationale, proposal.target_topic));
}

[[nodiscard]] double compute_specificity_score(const CandidateArtifactProposal& proposal) {
    double score = 0.0;
    if (!proposal.target_topic.empty()) { score += 0.20; }
    if (!proposal.evidence_role.empty()) { score += 0.20; }
    if (!proposal.source_evidence_potential_id.empty()) { score += 0.20; }
    if (!proposal.proposal_rationale.empty()) { score += 0.20; }
    if (!proposal.proposed_claim_skeletons.empty()) { score += 0.10; }
    if (!proposal.proposed_distortion_modes.empty()) { score += 0.05; }
    if (!proposal.proposed_damage_modes.empty()) { score += 0.05; }
    return clamp01(score);
}

[[nodiscard]] double compute_quality_score(const CandidateArtifactProposalAudit& audit) {
    double score = 0.0;
    if (audit.structure_valid) { score += 0.12; }
    if (audit.source_continuity_valid) { score += 0.12; }
    if (audit.proposal_specificity_clear) { score += 0.10; }
    if (audit.voice_readiness_clear) { score += 0.10; }
    if (audit.claim_skeleton_safe) { score += 0.10; }
    if (audit.distortion_plausible) { score += 0.08; }
    if (audit.damage_plausible) { score += 0.08; }
    if (audit.knowledge_horizon_clear) { score += 0.08; }
    if (audit.contradiction_budget_clear) { score += 0.08; }
    if (audit.protected_mystery_clear) { score += 0.06; }
    if (audit.access_safe) { score += 0.08; }
    return clamp01(score);
}

[[nodiscard]] double compute_safety_score(const CandidateArtifactProposalAudit& audit) {
    double score = 0.0;
    if (audit.claim_skeleton_safe) { score += 0.20; }
    if (audit.knowledge_horizon_clear) { score += 0.15; }
    if (audit.contradiction_budget_clear) { score += 0.15; }
    if (audit.protected_mystery_clear) { score += 0.15; }
    if (audit.access_safe) { score += 0.15; }
    if (!audit.current_generation_enabled) { score += 0.10; }
    if (!audit.current_materialization_enabled && !audit.archive_mutation_enabled) { score += 0.10; }
    return clamp01(score);
}

[[nodiscard]] double compute_revision_pressure_score(const CandidateArtifactProposalAudit& audit) {
    const double warning_pressure = static_cast<double>(count_severity(audit, CandidateArtifactProposalAuditSeverity::Warning)) * 0.10;
    const double error_pressure = static_cast<double>(count_severity(audit, CandidateArtifactProposalAuditSeverity::Error)) * 0.30;
    double structural_pressure = 0.0;
    if (!audit.proposal_specificity_clear) { structural_pressure += 0.10; }
    if (!audit.voice_readiness_clear) { structural_pressure += 0.10; }
    if (!audit.distortion_plausible) { structural_pressure += 0.10; }
    if (!audit.damage_plausible) { structural_pressure += 0.10; }
    return clamp01(warning_pressure + error_pressure + structural_pressure);
}

[[nodiscard]] std::vector<CandidateArtifactProposalAudit> audits_for_formatting(const ArchiveEngineState& state, AccessLevel access) {
    if (!state.candidate_artifact_proposal_audits.empty()) {
        return state.candidate_artifact_proposal_audits;
    }
    return audit_candidate_artifact_proposals(state, access).audits;
}

[[nodiscard]] bool audit_visible_to(const ArchiveEngineState& state, const CandidateArtifactProposalAudit& audit, AccessLevel access) {
    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposalAudit)) {
        return true;
    }
    const CandidateArtifactProposal* proposal = find_proposal(state, audit.proposal_id);
    return proposal != nullptr && candidate_artifact_proposal_visible_to(*proposal, access);
}

[[nodiscard]] bool has_hidden_diagnostic_related_id(const CandidateArtifactProposalAudit& audit) {
    for (const CandidateArtifactProposalAuditFinding& finding : audit.findings) {
        if (has_prefix(finding.related_id, "knowledge_horizon.") ||
            has_prefix(finding.related_id, "contradiction_budget.") ||
            has_prefix(finding.related_id, "event.") ||
            has_prefix(finding.related_id, "entity.") ||
            has_prefix(finding.related_id, "mystery.") ||
            has_prefix(finding.related_id, "evidence_potential.")) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool id_in(const std::vector<std::string>& values, const std::string& id) {
    return std::find(values.begin(), values.end(), id) != values.end();
}

void append_counts(std::ostringstream& out, const std::map<std::string, std::size_t>& counts) {
    for (const auto& entry : counts) {
        out << "- " << entry.first << ": " << entry.second << "\n";
    }
}

} // namespace

[[nodiscard]] std::string to_string(CandidateArtifactProposalAuditDecision decision) {
    switch (decision) {
        case CandidateArtifactProposalAuditDecision::Pass: return "pass";
        case CandidateArtifactProposalAuditDecision::NeedsRevision: return "needs_revision";
        case CandidateArtifactProposalAuditDecision::NeedsCuratorReview: return "needs_curator_review";
        case CandidateArtifactProposalAuditDecision::Blocked: return "blocked";
        case CandidateArtifactProposalAuditDecision::Invalid: return "invalid";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactProposalAuditGate gate) {
    switch (gate) {
        case CandidateArtifactProposalAuditGate::Structure: return "structure";
        case CandidateArtifactProposalAuditGate::SourceContinuity: return "source_continuity";
        case CandidateArtifactProposalAuditGate::ProposalSpecificity: return "proposal_specificity";
        case CandidateArtifactProposalAuditGate::VoiceReadiness: return "voice_readiness";
        case CandidateArtifactProposalAuditGate::ClaimSkeletonSafety: return "claim_skeleton_safety";
        case CandidateArtifactProposalAuditGate::DistortionPlausibility: return "distortion_plausibility";
        case CandidateArtifactProposalAuditGate::DamagePlausibility: return "damage_plausibility";
        case CandidateArtifactProposalAuditGate::KnowledgeHorizon: return "knowledge_horizon";
        case CandidateArtifactProposalAuditGate::ContradictionBudget: return "contradiction_budget";
        case CandidateArtifactProposalAuditGate::ProtectedMystery: return "protected_mystery";
        case CandidateArtifactProposalAuditGate::AccessSafety: return "access_safety";
        case CandidateArtifactProposalAuditGate::NoGeneration: return "no_generation";
        case CandidateArtifactProposalAuditGate::NoMutation: return "no_mutation";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactProposalAuditSeverity severity) {
    switch (severity) {
        case CandidateArtifactProposalAuditSeverity::Info: return "info";
        case CandidateArtifactProposalAuditSeverity::Warning: return "warning";
        case CandidateArtifactProposalAuditSeverity::Error: return "error";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactProposalAuditReasonCode reason_code) {
    switch (reason_code) {
        case CandidateArtifactProposalAuditReasonCode::None: return "none";
        case CandidateArtifactProposalAuditReasonCode::MissingProposal: return "missing_proposal";
        case CandidateArtifactProposalAuditReasonCode::MissingEvaluation: return "missing_evaluation";
        case CandidateArtifactProposalAuditReasonCode::MissingPlan: return "missing_plan";
        case CandidateArtifactProposalAuditReasonCode::MissingEvidencePotential: return "missing_evidence_potential";
        case CandidateArtifactProposalAuditReasonCode::MissingClaimSkeleton: return "missing_claim_skeleton";
        case CandidateArtifactProposalAuditReasonCode::MissingDistortionMode: return "missing_distortion_mode";
        case CandidateArtifactProposalAuditReasonCode::MissingDamageMode: return "missing_damage_mode";
        case CandidateArtifactProposalAuditReasonCode::WeakSpecificity: return "weak_specificity";
        case CandidateArtifactProposalAuditReasonCode::WeakVoiceReadiness: return "weak_voice_readiness";
        case CandidateArtifactProposalAuditReasonCode::KnowledgeHorizonBlocked: return "knowledge_horizon_blocked";
        case CandidateArtifactProposalAuditReasonCode::ContradictionBudgetHighPressure: return "contradiction_budget_high_pressure";
        case CandidateArtifactProposalAuditReasonCode::ProtectedMysteryRisk: return "protected_mystery_risk";
        case CandidateArtifactProposalAuditReasonCode::PublicAccessLeak: return "public_access_leak";
        case CandidateArtifactProposalAuditReasonCode::GenerationEnabled: return "generation_enabled";
        case CandidateArtifactProposalAuditReasonCode::MaterializationEnabled: return "materialization_enabled";
        case CandidateArtifactProposalAuditReasonCode::ArchiveMutationEnabled: return "archive_mutation_enabled";
        case CandidateArtifactProposalAuditReasonCode::ScoreOutOfRange: return "score_out_of_range";
    }
    return "unknown";
}

[[nodiscard]] CandidateArtifactProposalAuditPolicy default_candidate_artifact_proposal_audit_policy() {
    return CandidateArtifactProposalAuditPolicy{};
}

[[nodiscard]] CandidateArtifactProposalAuditDecision classify_candidate_artifact_proposal_audit(
    const CandidateArtifactProposalAudit& audit
) {
    const CandidateArtifactProposalAuditPolicy policy = default_candidate_artifact_proposal_audit_policy();
    if (audit.id.empty() || audit.proposal_id.empty() ||
        score_out_of_range(audit.proposal_quality_score) ||
        score_out_of_range(audit.specificity_score) ||
        score_out_of_range(audit.safety_score) ||
        score_out_of_range(audit.revision_pressure_score) ||
        (policy.block_on_generation_enabled && audit.current_generation_enabled) ||
        (policy.block_on_materialization_enabled && audit.current_materialization_enabled) ||
        (policy.block_on_archive_mutation_enabled && audit.archive_mutation_enabled) ||
        has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::GenerationEnabled) ||
        has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::MaterializationEnabled) ||
        has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::ArchiveMutationEnabled)) {
        return CandidateArtifactProposalAuditDecision::Invalid;
    }

    const bool source_chain_missing = has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::MissingProposal) ||
                                      has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::MissingEvaluation) ||
                                      has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::MissingPlan) ||
                                      has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::MissingEvidencePotential);
    if ((policy.block_on_missing_source_chain && source_chain_missing) ||
        has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::KnowledgeHorizonBlocked) ||
        has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::ContradictionBudgetHighPressure) ||
        (policy.block_on_public_access_leak && has_error_reason(audit, CandidateArtifactProposalAuditReasonCode::PublicAccessLeak)) ||
        audit.revision_pressure_score > policy.max_revision_pressure_before_block) {
        return CandidateArtifactProposalAuditDecision::Blocked;
    }

    const bool required_gates_clear = audit.structure_valid && audit.source_continuity_valid &&
                                      audit.proposal_specificity_clear && audit.voice_readiness_clear &&
                                      audit.claim_skeleton_safe && audit.distortion_plausible &&
                                      audit.damage_plausible && audit.knowledge_horizon_clear &&
                                      audit.contradiction_budget_clear && audit.protected_mystery_clear &&
                                      audit.access_safe;
    if (required_gates_clear && !has_error_findings(audit) &&
        audit.proposal_quality_score >= policy.min_quality_score_for_pass &&
        audit.specificity_score >= policy.min_specificity_score_for_pass &&
        audit.safety_score >= policy.min_safety_score_for_pass &&
        audit.revision_pressure_score <= policy.max_revision_pressure_for_pass) {
        return CandidateArtifactProposalAuditDecision::Pass;
    }

    if (!audit.protected_mystery_clear || has_reason(audit, CandidateArtifactProposalAuditReasonCode::ProtectedMysteryRisk)) {
        return CandidateArtifactProposalAuditDecision::NeedsCuratorReview;
    }

    if (!audit.proposal_specificity_clear || !audit.voice_readiness_clear ||
        (policy.require_claim_skeletons && !audit.claim_skeleton_safe) ||
        (policy.require_distortion_modes && !audit.distortion_plausible) ||
        (policy.require_damage_modes && !audit.damage_plausible) ||
        audit.proposal_quality_score < policy.min_quality_score_for_pass ||
        audit.specificity_score < policy.min_specificity_score_for_pass ||
        audit.safety_score < policy.min_safety_score_for_pass ||
        audit.revision_pressure_score > policy.max_revision_pressure_for_pass) {
        return CandidateArtifactProposalAuditDecision::NeedsRevision;
    }

    if (count_severity(audit, CandidateArtifactProposalAuditSeverity::Warning) > 0U) {
        return CandidateArtifactProposalAuditDecision::NeedsCuratorReview;
    }
    return CandidateArtifactProposalAuditDecision::NeedsRevision;
}

[[nodiscard]] CandidateArtifactProposalAudit audit_candidate_artifact_proposal(
    const ArchiveEngineState& state,
    const CandidateArtifactProposal& proposal,
    AccessLevel access
) {
    (void)access;
    CandidateArtifactProposalAudit audit;
    audit.id = "candidate_artifact_proposal_audit." + proposal.id;
    audit.proposal_id = proposal.id;
    audit.current_generation_enabled = false;
    audit.current_materialization_enabled = false;
    audit.archive_mutation_enabled = false;

    const CandidateArtifactPlanEvaluation* evaluation = find_evaluation(state, proposal.evaluation_id);
    const CandidateArtifactPlan* plan = find_plan(state, proposal.plan_id);
    const EvidencePotential* potential = find_potential(state, proposal.source_evidence_potential_id);

    audit.structure_valid = !proposal.id.empty() && !proposal.evaluation_id.empty() && !proposal.plan_id.empty() &&
                            !proposal.evidence_role.empty() && !proposal.proposal_rationale.empty() &&
                            proposal.text_status == CandidateArtifactProposalTextStatus::NoTextGenerated;
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::Structure,
                      audit.structure_valid ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Error,
                      audit.structure_valid ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::MissingProposal,
                      audit.structure_valid ? "Proposal has required structural fields and no generated text." : "Proposal is malformed or contains generated/non-placeholder text state.",
                      proposal.id);

    audit.source_continuity_valid = evaluation != nullptr && plan != nullptr && potential != nullptr &&
                                    evaluation->id == proposal.evaluation_id && plan->id == proposal.plan_id &&
                                    plan->source_id == proposal.source_evidence_potential_id;
    CandidateArtifactProposalAuditReasonCode source_reason = CandidateArtifactProposalAuditReasonCode::None;
    if (!audit.source_continuity_valid) {
        if (evaluation == nullptr) { source_reason = CandidateArtifactProposalAuditReasonCode::MissingEvaluation; }
        else if (plan == nullptr) { source_reason = CandidateArtifactProposalAuditReasonCode::MissingPlan; }
        else if (potential == nullptr) { source_reason = CandidateArtifactProposalAuditReasonCode::MissingEvidencePotential; }
        else { source_reason = CandidateArtifactProposalAuditReasonCode::MissingEvidencePotential; }
    }
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::SourceContinuity,
                      audit.source_continuity_valid ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Error,
                      source_reason,
                      audit.source_continuity_valid ? "Proposal preserves evaluation, plan, and EvidencePotential source continuity." : "Proposal source chain is broken or inconsistent.",
                      proposal.source_evidence_potential_id.empty() ? proposal.plan_id : proposal.source_evidence_potential_id);

    audit.specificity_score = compute_specificity_score(proposal);
    audit.proposal_specificity_clear = audit.specificity_score >= 0.55 && has_specific_local_content(proposal);
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::ProposalSpecificity,
                      audit.proposal_specificity_clear ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Warning,
                      audit.proposal_specificity_clear ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::WeakSpecificity,
                      audit.proposal_specificity_clear ? "Proposal carries source-specific topic, role, rationale, and local dependency detail." : "Proposal specificity is weak for future candidate drafting. Add civilization-specific claim skeleton or local source detail.",
                      proposal.id);

    audit.voice_readiness_clear = voice_register_matches_shape(proposal.proposed_shape, proposal.proposed_voice_register);
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::VoiceReadiness,
                      audit.voice_readiness_clear ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Warning,
                      audit.voice_readiness_clear ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::WeakVoiceReadiness,
                      audit.voice_readiness_clear ? "Proposed voice register fits the planned shape." : "Proposed voice register is weak or mismatched for the planned shape. Clarify proposed voice register for artifact shape.",
                      proposal.id);

    audit.claim_skeleton_safe = claim_skeletons_are_safe(proposal);
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::ClaimSkeletonSafety,
                      audit.claim_skeleton_safe ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Warning,
                      audit.claim_skeleton_safe ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::MissingClaimSkeleton,
                      audit.claim_skeleton_safe ? "Proposed claim entries remain explicit skeleton strings, not PublicClaim records." : "Proposal claim skeletons are missing or no longer explicitly marked as skeletons. Add civilization-specific claim skeleton.",
                      proposal.id);

    audit.distortion_plausible = !proposal.proposed_distortion_modes.empty();
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::DistortionPlausibility,
                      audit.distortion_plausible ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Warning,
                      audit.distortion_plausible ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::MissingDistortionMode,
                      audit.distortion_plausible ? "Proposal preserves distortion constraints for future drafting." : "Proposal lacks distortion constraints for future drafting. Add at least one plausible distortion mode.",
                      proposal.id);

    audit.damage_plausible = !proposal.proposed_damage_modes.empty();
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::DamagePlausibility,
                      audit.damage_plausible ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Warning,
                      audit.damage_plausible ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::MissingDamageMode,
                      audit.damage_plausible ? "Proposal includes plausible damage constraints for the artifact type." : "Proposal lacks damage constraints for future drafting. Add at least one plausible damage mode.",
                      proposal.id);

    audit.knowledge_horizon_clear = !contains_gate_error(proposal, "knowledge_horizon");
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::KnowledgeHorizon,
                      audit.knowledge_horizon_clear ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Error,
                      audit.knowledge_horizon_clear ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::KnowledgeHorizonBlocked,
                      audit.knowledge_horizon_clear ? "No proposal validation gate reports a KnowledgeHorizon blocker." : "KnowledgeHorizon gate blocks this proposal. Resolve knowledge-availability failure before future generation.",
                      proposal.id);

    audit.contradiction_budget_clear = !contains_gate_error(proposal, "contradiction_budget");
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::ContradictionBudget,
                      audit.contradiction_budget_clear ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Error,
                      audit.contradiction_budget_clear ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::ContradictionBudgetHighPressure,
                      audit.contradiction_budget_clear ? "No proposal validation gate reports a ContradictionBudget blocker." : "ContradictionBudget gate blocks this proposal. Reduce contradiction pressure before future generation.",
                      proposal.id);

    audit.protected_mystery_clear = !proposal.touches_protected_mystery;
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::ProtectedMystery,
                      audit.protected_mystery_clear ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Warning,
                      audit.protected_mystery_clear ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::ProtectedMysteryRisk,
                      audit.protected_mystery_clear ? "Proposal does not appear to over-resolve a protected mystery." : "Proposal touches protected mystery pressure and needs privileged review. Reduce protected mystery over-resolution risk.",
                      proposal.id);

    audit.access_safe = !proposal.contains_hidden_source_reference || proposal.visibility_class == CandidateArtifactProposalVisibilityClass::CuratorOnly || proposal.visibility_class == CandidateArtifactProposalVisibilityClass::DebugOnly;
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::AccessSafety,
                      audit.access_safe ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Error,
                      audit.access_safe ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::PublicAccessLeak,
                      audit.access_safe ? "Proposal access class is compatible with hidden source and diagnostic fields." : "Proposal access class would expose hidden source or diagnostic state. Lower public exposure or mark curator-only.",
                      proposal.id);

    const bool no_generation = !proposal.current_generation_enabled && !proposal.current_materialization_enabled;
    audit.current_generation_enabled = proposal.current_generation_enabled;
    audit.current_materialization_enabled = proposal.current_materialization_enabled;
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::NoGeneration,
                      no_generation ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Error,
                      no_generation ? CandidateArtifactProposalAuditReasonCode::None : (proposal.current_generation_enabled ? CandidateArtifactProposalAuditReasonCode::GenerationEnabled : CandidateArtifactProposalAuditReasonCode::MaterializationEnabled),
                      no_generation ? "Proposal does not enable artifact generation or materialization." : "Proposal claims generation or materialization is enabled; v28.10 is policy-only.",
                      proposal.id);

    audit.archive_mutation_enabled = proposal.archive_mutation_enabled;
    add_audit_finding(audit,
                      CandidateArtifactProposalAuditGate::NoMutation,
                      !proposal.archive_mutation_enabled ? CandidateArtifactProposalAuditSeverity::Info : CandidateArtifactProposalAuditSeverity::Error,
                      !proposal.archive_mutation_enabled ? CandidateArtifactProposalAuditReasonCode::None : CandidateArtifactProposalAuditReasonCode::ArchiveMutationEnabled,
                      !proposal.archive_mutation_enabled ? "Proposal does not enable archive mutation." : "Proposal claims archive mutation is enabled; v28.10 is policy-only.",
                      proposal.id);

    audit.proposal_quality_score = compute_quality_score(audit);
    audit.safety_score = compute_safety_score(audit);
    audit.revision_pressure_score = compute_revision_pressure_score(audit);
    audit.decision = classify_candidate_artifact_proposal_audit(audit);
    return audit;
}

[[nodiscard]] CandidateArtifactProposalAuditReport audit_candidate_artifact_proposals(
    const ArchiveEngineState& state,
    AccessLevel access
) {
    CandidateArtifactProposalAuditReport report;
    std::vector<CandidateArtifactProposal> proposals = state.candidate_artifact_proposals;
    if (proposals.empty()) {
        proposals = draft_candidate_artifact_proposals(state, AccessLevel::Curator).proposals;
    }
    for (const CandidateArtifactProposal& proposal : proposals) {
        report.audits.push_back(audit_candidate_artifact_proposal(state, proposal, access));
    }
    std::sort(report.audits.begin(), report.audits.end(), [](const CandidateArtifactProposalAudit& lhs, const CandidateArtifactProposalAudit& rhs) {
        return lhs.id < rhs.id;
    });
    ArchiveEngineState validation_state = state;
    validation_state.candidate_artifact_proposal_audits = report.audits;
    report.errors = validate_candidate_artifact_proposal_audits(validation_state);
    return report;
}

void audit_candidate_artifact_proposals_into_state(ArchiveEngineState& state, AccessLevel access) {
    state.candidate_artifact_proposal_audits = audit_candidate_artifact_proposals(state, access).audits;
}

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_proposal_audits(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> seen_ids;
    std::vector<std::string> proposal_ids;
    for (const CandidateArtifactProposal& proposal : state.candidate_artifact_proposals) {
        proposal_ids.push_back(proposal.id);
    }
    for (const CandidateArtifactProposalAudit& audit : state.candidate_artifact_proposal_audits) {
        const std::string label = audit.id.empty() ? std::string("<empty audit id>") : audit.id;
        if (audit.id.empty()) {
            errors.push_back("CandidateArtifactProposalAudit has empty id");
        } else if (!seen_ids.insert(audit.id).second) {
            errors.push_back("duplicate CandidateArtifactProposalAudit id: " + audit.id);
        }
        if (audit.proposal_id.empty()) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " has empty proposal_id");
        } else if (!id_in(proposal_ids, audit.proposal_id)) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " references missing proposal " + audit.proposal_id);
        }
        if (score_out_of_range(audit.proposal_quality_score)) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " has proposal_quality_score outside [0,1]");
        }
        if (score_out_of_range(audit.specificity_score)) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " has specificity_score outside [0,1]");
        }
        if (score_out_of_range(audit.safety_score)) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " has safety_score outside [0,1]");
        }
        if (score_out_of_range(audit.revision_pressure_score)) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " has revision_pressure_score outside [0,1]");
        }
        if (audit.current_generation_enabled) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " enables current generation");
        }
        if (audit.current_materialization_enabled) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " enables current materialization");
        }
        if (audit.archive_mutation_enabled) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " enables archive mutation");
        }
        if (audit.decision == CandidateArtifactProposalAuditDecision::Pass && has_error_findings(audit)) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " is pass but has error findings");
        }
        const CandidateArtifactProposalAuditPolicy policy = default_candidate_artifact_proposal_audit_policy();
        if (audit.decision != CandidateArtifactProposalAuditDecision::Pass &&
            audit.decision != CandidateArtifactProposalAuditDecision::Invalid &&
            audit.required_revisions.empty()) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " is non-pass but has no required revisions");
        }
        for (const CandidateArtifactProposalAuditFinding& finding : audit.findings) {
            if (finding.severity == CandidateArtifactProposalAuditSeverity::Error &&
                finding.reason_code == CandidateArtifactProposalAuditReasonCode::None) {
                errors.push_back("CandidateArtifactProposalAudit " + label + " has error finding without reason code");
            }
        }
        if (audit.decision == CandidateArtifactProposalAuditDecision::Pass) {
            if (audit.proposal_quality_score < policy.min_quality_score_for_pass) {
                errors.push_back("CandidateArtifactProposalAudit " + label + " is pass below policy quality threshold");
            }
            if (audit.specificity_score < policy.min_specificity_score_for_pass) {
                errors.push_back("CandidateArtifactProposalAudit " + label + " is pass below policy specificity threshold");
            }
            if (audit.safety_score < policy.min_safety_score_for_pass) {
                errors.push_back("CandidateArtifactProposalAudit " + label + " is pass below policy safety threshold");
            }
            if (audit.revision_pressure_score > policy.max_revision_pressure_for_pass) {
                errors.push_back("CandidateArtifactProposalAudit " + label + " is pass above policy revision-pressure threshold");
            }
            if (has_hidden_diagnostic_related_id(audit)) {
                errors.push_back("CandidateArtifactProposalAudit " + label + " is public/pass-safe but carries hidden diagnostic related IDs");
            }
        }
        if (audit.decision == CandidateArtifactProposalAuditDecision::NeedsRevision && has_error_hard_blocker(audit)) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " is needs_revision but has a hard blocker reason code");
        }
        if (audit.decision == CandidateArtifactProposalAuditDecision::Blocked && !has_error_findings(audit)) {
            errors.push_back("CandidateArtifactProposalAudit " + label + " is blocked without an error finding");
        }
    }
    return errors;
}

[[nodiscard]] std::string format_candidate_artifact_proposal_audit_summary(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactProposalAuditReport report = audit_candidate_artifact_proposals(state, access);
    std::map<std::string, std::size_t> by_decision;
    std::map<std::string, std::size_t> quality_buckets;
    std::size_t generation_enabled = 0;
    std::size_t materialization_enabled = 0;
    std::size_t mutation_enabled = 0;
    for (const CandidateArtifactProposalAudit& audit : report.audits) {
        ++by_decision[to_string(audit.decision)];
        if (audit.proposal_quality_score >= 0.85) {
            ++quality_buckets["high"];
        } else if (audit.proposal_quality_score >= 0.65) {
            ++quality_buckets["moderate"];
        } else {
            ++quality_buckets["low"];
        }
        if (audit.current_generation_enabled) { ++generation_enabled; }
        if (audit.current_materialization_enabled) { ++materialization_enabled; }
        if (audit.archive_mutation_enabled) { ++mutation_enabled; }
    }
    std::ostringstream out;
    out << "CandidateArtifactProposalAudit summary:\n";
    out << "- behavior: audit-only; no artifact generation, artifact text generation, candidate generation, proposal materialization, discovery scheduling, public archive mutation, hidden truth mutation, fragment activation, resolver/composition, persistence, or session state are introduced in v28.10.\n";
    out << "- total_audits: " << report.audits.size() << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    out << "- current_generation_enabled: " << generation_enabled << "\n";
    out << "- current_materialization_enabled: " << materialization_enabled << "\n";
    out << "- archive_mutation_enabled: " << mutation_enabled << "\n";
    out << "Decision counts:\n";
    append_counts(out, by_decision);
    out << "Quality buckets:\n";
    append_counts(out, quality_buckets);
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposalAudit)) {
        out << "- details: aggregate-only at this access level; hidden proposal IDs, source IDs, KnowledgeHorizon diagnostics, ContradictionBudget diagnostics, protected mystery details, hidden rationale, curator-only findings, and privileged required revisions are restricted.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_proposal_audit_validation(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactProposalAuditReport report = audit_candidate_artifact_proposals(state, access);
    std::ostringstream out;
    out << "CandidateArtifactProposalAudit validation:\n";
    out << "- result: " << (report.errors.empty() ? "passed" : "failed") << "\n";
    out << "- audits: " << report.audits.size() << "\n";
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

[[nodiscard]] std::string format_candidate_artifact_proposal_audit_list(const ArchiveEngineState& state, AccessLevel access) {
    const std::vector<CandidateArtifactProposalAudit> audits = audits_for_formatting(state, access);
    std::ostringstream out;
    out << "CandidateArtifactProposalAudits visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposalAudit)) {
        std::size_t visible = 0;
        for (const CandidateArtifactProposalAudit& audit : audits) {
            if (audit_visible_to(state, audit, access)) { ++visible; }
        }
        out << "- public_safe_visible_audits: " << visible << "\n";
        out << "- details: public/scholar access receives aggregate counts and public-safe summaries only; hidden proposal IDs, diagnostics, and required revisions are restricted.\n";
        for (const CandidateArtifactProposalAudit& audit : audits) {
            if (!audit_visible_to(state, audit, access)) { continue; }
            out << "- decision=" << to_string(audit.decision)
                << " quality_score=" << audit.proposal_quality_score
                << " safety_score=" << audit.safety_score
                << " current_generation_enabled=false current_materialization_enabled=false archive_mutation_enabled=false\n";
        }
        return out.str();
    }
    if (audits.empty()) {
        out << "- none\n";
        return out.str();
    }
    for (const CandidateArtifactProposalAudit& audit : audits) {
        out << "- " << audit.id
            << ": proposal=" << audit.proposal_id
            << " decision=" << to_string(audit.decision)
            << " quality_score=" << audit.proposal_quality_score
            << " specificity_score=" << audit.specificity_score
            << " safety_score=" << audit.safety_score
            << " revision_pressure_score=" << audit.revision_pressure_score
            << " current_generation_enabled=" << (audit.current_generation_enabled ? "true" : "false")
            << " current_materialization_enabled=" << (audit.current_materialization_enabled ? "true" : "false")
            << " archive_mutation_enabled=" << (audit.archive_mutation_enabled ? "true" : "false") << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_proposal_audit_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& audit_id
) {
    const std::vector<CandidateArtifactProposalAudit> audits = audits_for_formatting(state, access);
    const auto it = std::find_if(audits.begin(), audits.end(), [&](const CandidateArtifactProposalAudit& audit) {
        return audit.id == audit_id;
    });
    std::ostringstream out;
    out << "CandidateArtifactProposalAudit:\n";
    if (it == audits.end() || !audit_visible_to(state, *it, access)) {
        out << "- found: false\n";
        return out.str();
    }
    out << "- found: true\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposalAudit)) {
        out << "- decision: " << to_string(it->decision) << "\n";
        out << "- quality_score: " << it->proposal_quality_score << "\n";
        out << "- safety_score: " << it->safety_score << "\n";
        out << "- current_generation_enabled: false\n";
        out << "- current_materialization_enabled: false\n";
        out << "- archive_mutation_enabled: false\n";
        out << "- details: restricted\n";
        return out.str();
    }
    out << "- id: " << it->id << "\n";
    out << "- proposal_id: " << it->proposal_id << "\n";
    out << "- decision: " << to_string(it->decision) << "\n";
    out << "- structure_valid: " << (it->structure_valid ? "true" : "false") << "\n";
    out << "- source_continuity_valid: " << (it->source_continuity_valid ? "true" : "false") << "\n";
    out << "- proposal_specificity_clear: " << (it->proposal_specificity_clear ? "true" : "false") << "\n";
    out << "- voice_readiness_clear: " << (it->voice_readiness_clear ? "true" : "false") << "\n";
    out << "- claim_skeleton_safe: " << (it->claim_skeleton_safe ? "true" : "false") << "\n";
    out << "- distortion_plausible: " << (it->distortion_plausible ? "true" : "false") << "\n";
    out << "- damage_plausible: " << (it->damage_plausible ? "true" : "false") << "\n";
    out << "- knowledge_horizon_clear: " << (it->knowledge_horizon_clear ? "true" : "false") << "\n";
    out << "- contradiction_budget_clear: " << (it->contradiction_budget_clear ? "true" : "false") << "\n";
    out << "- protected_mystery_clear: " << (it->protected_mystery_clear ? "true" : "false") << "\n";
    out << "- access_safe: " << (it->access_safe ? "true" : "false") << "\n";
    out << "- proposal_quality_score: " << it->proposal_quality_score << "\n";
    out << "- specificity_score: " << it->specificity_score << "\n";
    out << "- safety_score: " << it->safety_score << "\n";
    out << "- revision_pressure_score: " << it->revision_pressure_score << "\n";
    const CandidateArtifactProposalAuditPolicy policy = default_candidate_artifact_proposal_audit_policy();
    out << "Policy thresholds:\n";
    out << "- min_quality_score_for_pass: " << policy.min_quality_score_for_pass << "\n";
    out << "- min_specificity_score_for_pass: " << policy.min_specificity_score_for_pass << "\n";
    out << "- min_safety_score_for_pass: " << policy.min_safety_score_for_pass << "\n";
    out << "- max_revision_pressure_for_pass: " << policy.max_revision_pressure_for_pass << "\n";
    out << "- max_revision_pressure_before_block: " << policy.max_revision_pressure_before_block << "\n";
    out << "Policy comparison:\n";
    out << "- quality_pass_threshold_met: " << (it->proposal_quality_score >= policy.min_quality_score_for_pass ? "true" : "false") << "\n";
    out << "- specificity_pass_threshold_met: " << (it->specificity_score >= policy.min_specificity_score_for_pass ? "true" : "false") << "\n";
    out << "- safety_pass_threshold_met: " << (it->safety_score >= policy.min_safety_score_for_pass ? "true" : "false") << "\n";
    out << "- revision_pressure_pass_threshold_met: " << (it->revision_pressure_score <= policy.max_revision_pressure_for_pass ? "true" : "false") << "\n";
    out << "- current_generation_enabled: " << (it->current_generation_enabled ? "true" : "false") << "\n";
    out << "- current_materialization_enabled: " << (it->current_materialization_enabled ? "true" : "false") << "\n";
    out << "- archive_mutation_enabled: " << (it->archive_mutation_enabled ? "true" : "false") << "\n";
    if (!it->findings.empty()) {
        out << "Findings:\n";
        for (const CandidateArtifactProposalAuditFinding& finding : it->findings) {
            out << "- " << finding.id
                << " gate=" << to_string(finding.gate)
                << " severity=" << to_string(finding.severity)
                << " reason_code=" << to_string(finding.reason_code)
                << " related_id=" << finding.related_id
                << " message=" << finding.message << "\n";
        }
    }
    if (!it->required_revisions.empty()) {
        out << "Required revisions:\n";
        for (const std::string& revision : it->required_revisions) {
            out << "- " << revision << "\n";
        }
    }
    return out.str();
}

} // namespace archive
