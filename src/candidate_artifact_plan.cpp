#include "candidate_artifact_plan_api.h"
#include "contradiction_budget_api.h"
#include "diagnostic_access_policy.h"
#include "knowledge_horizon_api.h"

#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {

[[nodiscard]] bool is_hidden_source(const ArchiveEngineState& state, const EvidencePotential& potential) {
    switch (potential.source_type) {
        case EvidencePotentialSourceType::HiddenEvent: {
            const Event* event = state.hidden_truth.find_event(potential.source_id);
            return event != nullptr && !can_view(AccessLevel::Public, event->min_access);
        }
        case EvidencePotentialSourceType::HiddenEntity:
        case EvidencePotentialSourceType::Site:
        case EvidencePotentialSourceType::Office: {
            const Entity* entity = state.hidden_truth.find_entity(potential.source_id);
            return entity != nullptr && !can_view(AccessLevel::Public, entity->min_access);
        }
        case EvidencePotentialSourceType::Mystery: {
            const auto it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
                return mystery.id == potential.source_id;
            });
            return it != state.mysteries.end() && !can_view(AccessLevel::Public, it->min_access);
        }
    }
    return true;
}

[[nodiscard]] bool source_potential_exists(const ArchiveEngineState& state, const std::string& source_id) {
    return std::any_of(state.evidence_potentials.begin(), state.evidence_potentials.end(), [&](const EvidencePotential& potential) {
        return potential.id == source_id;
    });
}

[[nodiscard]] const EvidencePotential* find_potential(const ArchiveEngineState& state, const std::string& source_id) {
    const auto it = std::find_if(state.evidence_potentials.begin(), state.evidence_potentials.end(), [&](const EvidencePotential& potential) {
        return potential.id == source_id;
    });
    if (it == state.evidence_potentials.end()) {
        return nullptr;
    }
    return &*it;
}

[[nodiscard]] bool source_touches_protected_mystery(const ArchiveEngineState& state, const EvidencePotential& potential) {
    if (potential.source_type == EvidencePotentialSourceType::Mystery) {
        const auto it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
            return mystery.id == potential.source_id;
        });
        if (it == state.mysteries.end()) {
            return false;
        }
        return it->reveal_mode == RevealMode::NeverFullyResolvable ||
               it->reveal_mode == RevealMode::AccessLocked ||
               it->reveal_mode == RevealMode::ContradictoryByDesign ||
               !can_view(AccessLevel::Public, it->min_access);
    }
    (void)state;
    return false;
}

[[nodiscard]] CandidateArtifactPlanShape shape_for_trace(EvidencePotentialTraceType trace_type, const EvidencePotential& potential) {
    switch (trace_type) {
        case EvidencePotentialTraceType::InscriptionTrace:
            return potential.source_type == EvidencePotentialSourceType::Mystery ? CandidateArtifactPlanShape::RitualNotice : CandidateArtifactPlanShape::BoundaryInscription;
        case EvidencePotentialTraceType::LedgerTrace:
            return CandidateArtifactPlanShape::LedgerEntry;
        case EvidencePotentialTraceType::LegalRecordTrace:
            return CandidateArtifactPlanShape::AdministrativeDocket;
        case EvidencePotentialTraceType::RitualTrace:
            return CandidateArtifactPlanShape::RitualNotice;
        case EvidencePotentialTraceType::OralTraditionTrace:
            return CandidateArtifactPlanShape::OralTraditionFragment;
        case EvidencePotentialTraceType::MaterialDepositTrace:
            return CandidateArtifactPlanShape::MaterialTrace;
        case EvidencePotentialTraceType::CopyTraditionTrace:
            return potential.public_safe ? CandidateArtifactPlanShape::ScholarFragment : CandidateArtifactPlanShape::ShrineCopy;
        case EvidencePotentialTraceType::AbsenceTrace:
            return CandidateArtifactPlanShape::AbsenceRecord;
    }
    return CandidateArtifactPlanShape::AdministrativeDocket;
}

[[nodiscard]] ArtifactType artifact_type_for_shape(CandidateArtifactPlanShape shape, ArtifactType fallback) {
    switch (shape) {
        case CandidateArtifactPlanShape::AdministrativeDocket: return ArtifactType::ForgedDecree;
        case CandidateArtifactPlanShape::RitualNotice: return ArtifactType::Inscription;
        case CandidateArtifactPlanShape::LedgerEntry: return ArtifactType::TradeLedger;
        case CandidateArtifactPlanShape::BoundaryInscription: return ArtifactType::Inscription;
        case CandidateArtifactPlanShape::ShrineCopy: return ArtifactType::DamagedManuscript;
        case CandidateArtifactPlanShape::ScholarFragment: return ArtifactType::DamagedManuscript;
        case CandidateArtifactPlanShape::OralTraditionFragment: return ArtifactType::OralHistory;
        case CandidateArtifactPlanShape::MaterialTrace: return fallback;
        case CandidateArtifactPlanShape::AbsenceRecord: return ArtifactType::DamagedManuscript;
    }
    return fallback;
}

[[nodiscard]] std::string evidence_role_for_shape(CandidateArtifactPlanShape shape) {
    switch (shape) {
        case CandidateArtifactPlanShape::AdministrativeDocket: return "legal-administrative corroboration or dispute pressure";
        case CandidateArtifactPlanShape::RitualNotice: return "ritualized notice of an event or office claim";
        case CandidateArtifactPlanShape::LedgerEntry: return "administrative ledger trace for material obligations";
        case CandidateArtifactPlanShape::BoundaryInscription: return "place-bound inscriptional trace";
        case CandidateArtifactPlanShape::ShrineCopy: return "later copied shrine or archive witness";
        case CandidateArtifactPlanShape::ScholarFragment: return "later scholarly fragment or copied excerpt";
        case CandidateArtifactPlanShape::OralTraditionFragment: return "oral-memory fragment with compression risk";
        case CandidateArtifactPlanShape::MaterialTrace: return "material trace rather than authored testimony";
        case CandidateArtifactPlanShape::AbsenceRecord: return "negative evidence or meaningful archival silence";
    }
    return "candidate evidence planning trace";
}

void add_distortion_labels(CandidateArtifactPlan& plan, const EvidencePotential& potential) {
    for (EvidenceModifier modifier : potential.likely_distortions) {
        add_unique_string(plan.expected_distortion_modes, to_string(modifier));
    }
    switch (potential.trace_type) {
        case EvidencePotentialTraceType::CopyTraditionTrace:
            add_unique_string(plan.expected_distortion_modes, "later_copy");
            break;
        case EvidencePotentialTraceType::OralTraditionTrace:
            add_unique_string(plan.expected_distortion_modes, "oral_compression");
            break;
        case EvidencePotentialTraceType::AbsenceTrace:
            add_unique_string(plan.expected_distortion_modes, "archive_silence");
            break;
        case EvidencePotentialTraceType::MaterialDepositTrace:
            add_unique_string(plan.expected_distortion_modes, "context_loss");
            break;
        default:
            break;
    }
}

void add_expected_claim_types(CandidateArtifactPlan& plan) {
    switch (plan.planned_shape) {
        case CandidateArtifactPlanShape::AdministrativeDocket:
            add_unique_string(plan.expected_claim_types, "legal_fiction");
            add_unique_string(plan.expected_claim_types, "factual_claim");
            break;
        case CandidateArtifactPlanShape::RitualNotice:
            add_unique_string(plan.expected_claim_types, "ritual_claim");
            add_unique_string(plan.expected_claim_types, "symbolic_claim");
            break;
        case CandidateArtifactPlanShape::LedgerEntry:
            add_unique_string(plan.expected_claim_types, "factual_claim");
            add_unique_string(plan.expected_claim_types, "administrative_quantity");
            break;
        case CandidateArtifactPlanShape::OralTraditionFragment:
            add_unique_string(plan.expected_claim_types, "mythic_compression");
            add_unique_string(plan.expected_claim_types, "translation_guess");
            break;
        case CandidateArtifactPlanShape::AbsenceRecord:
            add_unique_string(plan.expected_claim_types, "negative_evidence");
            break;
        default:
            add_unique_string(plan.expected_claim_types, "factual_claim");
            break;
    }
}

void add_default_validation_steps(CandidateArtifactPlan& plan) {
    add_unique_string(plan.required_validation_steps, "validate source EvidencePotential still exists");
    add_unique_string(plan.required_validation_steps, "validate KnowledgeHorizon before candidate generation");
    add_unique_string(plan.required_validation_steps, "validate ContradictionBudget pressure before materialization");
    add_unique_string(plan.required_validation_steps, "validate protected mystery confidence caps");
    add_unique_string(plan.required_validation_steps, "validate public safety and access redaction");
    add_unique_string(plan.required_validation_steps, "evaluate candidate artifact without mutating archive state");
}

[[nodiscard]] bool has_high_or_critical_bucket(const ContradictionBudgetReport& report, const CandidateArtifactPlan& plan) {
    for (const std::string& bucket_id : plan.contradiction_budget_bucket_ids) {
        const auto it = std::find_if(report.buckets.begin(), report.buckets.end(), [&](const ContradictionBudgetBucket& bucket) {
            return bucket.id == bucket_id;
        });
        if (it != report.buckets.end() &&
            (it->severity == ContradictionBudgetSeverity::High || it->severity == ContradictionBudgetSeverity::Critical)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_moderate_bucket(const ContradictionBudgetReport& report, const CandidateArtifactPlan& plan) {
    for (const std::string& bucket_id : plan.contradiction_budget_bucket_ids) {
        const auto it = std::find_if(report.buckets.begin(), report.buckets.end(), [&](const ContradictionBudgetBucket& bucket) {
            return bucket.id == bucket_id;
        });
        if (it != report.buckets.end() && it->severity == ContradictionBudgetSeverity::Moderate) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool has_knowledge_horizon_error(const KnowledgeHorizonReport& report, const CandidateArtifactPlan& plan) {
    for (const std::string& finding_id : plan.knowledge_horizon_finding_ids) {
        const auto it = std::find_if(report.findings.begin(), report.findings.end(), [&](const KnowledgeHorizonFinding& finding) {
            return finding.id == finding_id;
        });
        if (it != report.findings.end() && is_invalid(it->status)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::vector<std::string> collect_relevant_knowledge_findings(const ArchiveEngineState& state,
                                                                            const EvidencePotential& potential) {
    const KnowledgeHorizonReport report = validate_knowledge_horizon(state, AccessLevel::Curator);
    std::vector<std::string> ids;
    for (const KnowledgeHorizonFinding& finding : report.findings) {
        if (finding.context_id == potential.id ||
            finding.subject_id == potential.id ||
            finding.subject_id == potential.source_id) {
            add_unique_string(ids, finding.id);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

[[nodiscard]] std::vector<std::string> collect_relevant_budget_buckets(const ArchiveEngineState& state,
                                                                        const EvidencePotential& potential) {
    const ContradictionBudgetReport report = compute_contradiction_budget(state, AccessLevel::Curator);
    std::vector<std::string> ids;
    for (const ContradictionBudgetBucket& bucket : report.buckets) {
        if (bucket.id == "contradiction_budget.archive" ||
            bucket.scope_id == potential.source_id ||
            bucket.scope_id == potential.id) {
            add_unique_string(ids, bucket.id);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

[[nodiscard]] bool plan_visible_to(const CandidateArtifactPlan& plan, AccessLevel access) {
    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlan)) {
        return true;
    }
    return plan.public_safe && plan.status == CandidateArtifactPlanStatus::Plausible;
}

[[nodiscard]] std::vector<CandidateArtifactPlan> plans_for_formatting(const ArchiveEngineState& state, AccessLevel access) {
    if (!state.candidate_artifact_plans.empty()) {
        return state.candidate_artifact_plans;
    }
    return derive_candidate_artifact_plans(state, access).plans;
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, const std::string& id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

[[nodiscard]] std::vector<std::string> validate_plan_vector(const ArchiveEngineState& state, const std::vector<CandidateArtifactPlan>& plans) {
    std::vector<std::string> errors;
    std::set<std::string> seen_ids;
    const KnowledgeHorizonReport horizon_report = validate_knowledge_horizon(state, AccessLevel::Curator);
    std::vector<std::string> horizon_ids;
    for (const KnowledgeHorizonFinding& finding : horizon_report.findings) {
        horizon_ids.push_back(finding.id);
    }
    const ContradictionBudgetReport budget_report = compute_contradiction_budget(state, AccessLevel::Curator);
    std::vector<std::string> bucket_ids;
    for (const ContradictionBudgetBucket& bucket : budget_report.buckets) {
        bucket_ids.push_back(bucket.id);
    }

    for (const CandidateArtifactPlan& plan : plans) {
        if (plan.id.empty()) {
            errors.push_back("CandidateArtifactPlan has empty id");
        } else if (!seen_ids.insert(plan.id).second) {
            errors.push_back("CandidateArtifactPlan has duplicate id: " + plan.id);
        }
        if (plan.source_id.empty()) {
            errors.push_back("CandidateArtifactPlan has empty source id: " + plan.id);
        } else if (plan.source_type == CandidateArtifactPlanSourceType::EvidencePotential && !source_potential_exists(state, plan.source_id)) {
            errors.push_back("CandidateArtifactPlan references missing EvidencePotential: " + plan.id + " -> " + plan.source_id);
        }
        if (plan.target_year <= 0 || plan.target_year > kOpenEndedYear) {
            errors.push_back("CandidateArtifactPlan has invalid target year: " + plan.id);
        }
        if (plan.rationale.empty()) {
            errors.push_back("CandidateArtifactPlan has empty rationale: " + plan.id);
        }
        if (plan.evidence_role.empty()) {
            errors.push_back("CandidateArtifactPlan has empty evidence role: " + plan.id);
        }
        if (plan.current_materialization_enabled) {
            errors.push_back("CandidateArtifactPlan claims current materialization is enabled in v28.5: " + plan.id);
        }
        if (plan.public_safe && plan.source_type == CandidateArtifactPlanSourceType::EvidencePotential) {
            const EvidencePotential* potential = find_potential(state, plan.source_id);
            if (potential != nullptr && is_hidden_source(state, *potential)) {
                errors.push_back("CandidateArtifactPlan public_safe plan references hidden-only source: " + plan.id);
            }
        }
        for (const std::string& finding_id : plan.knowledge_horizon_finding_ids) {
            if (!contains_id(horizon_ids, finding_id)) {
                errors.push_back("CandidateArtifactPlan references unknown KnowledgeHorizon finding id: " + plan.id + " -> " + finding_id);
            }
        }
        for (const std::string& bucket_id : plan.contradiction_budget_bucket_ids) {
            if (!contains_id(bucket_ids, bucket_id)) {
                errors.push_back("CandidateArtifactPlan references unknown ContradictionBudget bucket id: " + plan.id + " -> " + bucket_id);
            }
        }
    }
    return errors;
}

void append_counts(std::ostringstream& out, const std::map<std::string, std::size_t>& counts) {
    for (const auto& [label, count] : counts) {
        out << "- " << label << ": " << count << "\n";
    }
}

[[nodiscard]] CandidateArtifactPlanRiskLevel risk_for_plan(const ArchiveEngineState& state,
                                                            const CandidateArtifactPlan& plan,
                                                            AccessLevel access) {
    if (plan.status == CandidateArtifactPlanStatus::Invalid) {
        return CandidateArtifactPlanRiskLevel::Critical;
    }
    if (plan.status == CandidateArtifactPlanStatus::BlockedByKnowledgeHorizon ||
        plan.status == CandidateArtifactPlanStatus::BlockedByContradictionPressure ||
        plan.status == CandidateArtifactPlanStatus::BlockedByProtectedMystery ||
        plan.status == CandidateArtifactPlanStatus::BlockedByPublicSafety) {
        return CandidateArtifactPlanRiskLevel::High;
    }
    const ContradictionBudgetReport budget_report = compute_contradiction_budget(state, access);
    if (has_moderate_bucket(budget_report, plan) || plan.requires_curator_review) {
        return CandidateArtifactPlanRiskLevel::Moderate;
    }
    return CandidateArtifactPlanRiskLevel::Low;
}

} // namespace

[[nodiscard]] std::string to_string(CandidateArtifactPlanSourceType type) {
    switch (type) {
        case CandidateArtifactPlanSourceType::EvidencePotential: return "evidence_potential";
        case CandidateArtifactPlanSourceType::HiddenMutation: return "hidden_mutation";
        case CandidateArtifactPlanSourceType::ManualCuratorSeed: return "manual_curator_seed";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactPlanShape shape) {
    switch (shape) {
        case CandidateArtifactPlanShape::AdministrativeDocket: return "administrative_docket";
        case CandidateArtifactPlanShape::RitualNotice: return "ritual_notice";
        case CandidateArtifactPlanShape::LedgerEntry: return "ledger_entry";
        case CandidateArtifactPlanShape::BoundaryInscription: return "boundary_inscription";
        case CandidateArtifactPlanShape::ShrineCopy: return "shrine_copy";
        case CandidateArtifactPlanShape::ScholarFragment: return "scholar_fragment";
        case CandidateArtifactPlanShape::OralTraditionFragment: return "oral_tradition_fragment";
        case CandidateArtifactPlanShape::MaterialTrace: return "material_trace";
        case CandidateArtifactPlanShape::AbsenceRecord: return "absence_record";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactPlanStatus status) {
    switch (status) {
        case CandidateArtifactPlanStatus::Plausible: return "plausible";
        case CandidateArtifactPlanStatus::NeedsCuratorReview: return "needs_curator_review";
        case CandidateArtifactPlanStatus::BlockedByKnowledgeHorizon: return "blocked_by_knowledge_horizon";
        case CandidateArtifactPlanStatus::BlockedByContradictionPressure: return "blocked_by_contradiction_pressure";
        case CandidateArtifactPlanStatus::BlockedByProtectedMystery: return "blocked_by_protected_mystery";
        case CandidateArtifactPlanStatus::BlockedByPublicSafety: return "blocked_by_public_safety";
        case CandidateArtifactPlanStatus::Invalid: return "invalid";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactPlanRiskLevel risk) {
    switch (risk) {
        case CandidateArtifactPlanRiskLevel::Low: return "low";
        case CandidateArtifactPlanRiskLevel::Moderate: return "moderate";
        case CandidateArtifactPlanRiskLevel::High: return "high";
        case CandidateArtifactPlanRiskLevel::Critical: return "critical";
    }
    return "unknown";
}

[[nodiscard]] CandidateArtifactPlanStatus classify_candidate_artifact_plan_status(const ArchiveEngineState& state,
                                                                                  const CandidateArtifactPlan& plan,
                                                                                  AccessLevel access) {
    const EvidencePotential* potential = find_potential(state, plan.source_id);
    if (plan.id.empty() || plan.source_id.empty() || plan.rationale.empty() || plan.evidence_role.empty() || potential == nullptr) {
        return CandidateArtifactPlanStatus::Invalid;
    }
    if (plan.public_safe && is_hidden_source(state, *potential)) {
        return CandidateArtifactPlanStatus::Invalid;
    }
    if (plan.current_materialization_enabled) {
        return CandidateArtifactPlanStatus::Invalid;
    }
    const KnowledgeHorizonReport horizon_report = validate_knowledge_horizon(state, AccessLevel::Curator);
    if (has_knowledge_horizon_error(horizon_report, plan)) {
        return CandidateArtifactPlanStatus::BlockedByKnowledgeHorizon;
    }
    const ContradictionBudgetReport budget_report = compute_contradiction_budget(state, AccessLevel::Curator);
    if (has_high_or_critical_bucket(budget_report, plan)) {
        return CandidateArtifactPlanStatus::BlockedByContradictionPressure;
    }
    if (source_touches_protected_mystery(state, *potential)) {
        return CandidateArtifactPlanStatus::BlockedByProtectedMystery;
    }
    if (!can_view(access, AccessLevel::Curator) && (!plan.public_safe || is_hidden_source(state, *potential))) {
        return CandidateArtifactPlanStatus::BlockedByPublicSafety;
    }
    if (!potential->discoverable || plan.requires_curator_review || has_moderate_bucket(budget_report, plan)) {
        return CandidateArtifactPlanStatus::NeedsCuratorReview;
    }
    return CandidateArtifactPlanStatus::Plausible;
}

[[nodiscard]] CandidateArtifactPlan make_plan_from_evidence_potential(const ArchiveEngineState& state,
                                                                       const EvidencePotential& potential,
                                                                       AccessLevel access) {
    CandidateArtifactPlan plan;
    plan.source_type = CandidateArtifactPlanSourceType::EvidencePotential;
    plan.source_id = potential.id;
    plan.planned_shape = shape_for_trace(potential.trace_type, potential);
    plan.planned_artifact_type = artifact_type_for_shape(plan.planned_shape, potential.likely_artifact_type);
    plan.id = "candidate_artifact_plan." + potential.id + "." + to_string(plan.planned_shape);
    plan.target_topic = potential.subject.empty() ? potential.source_id : potential.subject;
    plan.target_year = std::max(1, potential.earliest_possible_year);
    plan.evidence_role = evidence_role_for_shape(plan.planned_shape);
    plan.rationale = "Planning-only bridge from " + potential.id + " (" + to_string(potential.trace_type) + ") to a plausible future " + to_string(plan.planned_shape) + "; no artifact candidate is generated in v28.5.";
    plan.public_safe = potential.public_safe && !is_hidden_source(state, potential);
    plan.requires_curator_review = !plan.public_safe || is_hidden_source(state, potential) || potential.min_access > AccessLevel::Scholar;
    plan.materializable_in_future = false;
    plan.current_materialization_enabled = false;
    add_expected_claim_types(plan);
    add_distortion_labels(plan, potential);
    add_default_validation_steps(plan);
    plan.knowledge_horizon_finding_ids = collect_relevant_knowledge_findings(state, potential);
    plan.contradiction_budget_bucket_ids = collect_relevant_budget_buckets(state, potential);
    if (!potential.discoverable) {
        plan.warnings.push_back("source EvidencePotential is not currently discoverable");
    }
    if (source_touches_protected_mystery(state, potential)) {
        plan.warnings.push_back("source touches a protected or restricted mystery boundary");
    }
    plan.status = classify_candidate_artifact_plan_status(state, plan, access);
    plan.risk_level = risk_for_plan(state, plan, access);
    return plan;
}

[[nodiscard]] CandidateArtifactPlanReport derive_candidate_artifact_plans(const ArchiveEngineState& state, AccessLevel access) {
    CandidateArtifactPlanReport report;
    for (const EvidencePotential& potential : state.evidence_potentials) {
        report.plans.push_back(make_plan_from_evidence_potential(state, potential, access));
    }
    std::sort(report.plans.begin(), report.plans.end(), [](const CandidateArtifactPlan& lhs, const CandidateArtifactPlan& rhs) {
        return lhs.id < rhs.id;
    });
    report.errors = validate_plan_vector(state, report.plans);
    return report;
}

void derive_candidate_artifact_plans_into_state(ArchiveEngineState& state, AccessLevel access) {
    state.candidate_artifact_plans = derive_candidate_artifact_plans(state, access).plans;
}

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_plans(const ArchiveEngineState& state) {
    return validate_plan_vector(state, state.candidate_artifact_plans);
}

[[nodiscard]] std::string format_candidate_artifact_plan_summary(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactPlanReport report = derive_candidate_artifact_plans(state, access);
    std::map<std::string, std::size_t> by_status;
    std::map<std::string, std::size_t> by_risk;
    std::size_t public_safe_count = 0;
    std::size_t review_count = 0;
    std::size_t blocked_count = 0;
    std::size_t current_materialization_count = 0;
    for (const CandidateArtifactPlan& plan : report.plans) {
        ++by_status[to_string(plan.status)];
        ++by_risk[to_string(plan.risk_level)];
        if (plan.public_safe) {
            ++public_safe_count;
        }
        if (plan.requires_curator_review) {
            ++review_count;
        }
        if (plan.status == CandidateArtifactPlanStatus::BlockedByKnowledgeHorizon ||
            plan.status == CandidateArtifactPlanStatus::BlockedByContradictionPressure ||
            plan.status == CandidateArtifactPlanStatus::BlockedByProtectedMystery ||
            plan.status == CandidateArtifactPlanStatus::BlockedByPublicSafety ||
            plan.status == CandidateArtifactPlanStatus::Invalid) {
            ++blocked_count;
        }
        if (plan.current_materialization_enabled) {
            ++current_materialization_count;
        }
    }
    std::ostringstream out;
    out << "CandidateArtifactPlan summary:\n";
    out << "- behavior: planning only; no artifact candidates, artifacts, discoveries, materialization, public archive mutation, hidden truth mutation, fragment activation, resolver/composition, persistence, or session state are introduced in v28.5.\n";
    out << "- total_plans: " << report.plans.size() << "\n";
    out << "- public_safe_plans: " << public_safe_count << "\n";
    out << "- curator_review_plans: " << review_count << "\n";
    out << "- blocked_or_invalid_plans: " << blocked_count << "\n";
    out << "- current_materialization_enabled: " << current_materialization_count << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    out << "Status counts:\n";
    append_counts(out, by_status);
    out << "Risk counts:\n";
    append_counts(out, by_risk);
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlan)) {
        out << "- details: aggregate-only at this access level; hidden source IDs, diagnostic IDs, rationale, warnings, and protected mystery details are restricted.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_plan_validation(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactPlanReport report = derive_candidate_artifact_plans(state, access);
    std::ostringstream out;
    out << "CandidateArtifactPlan validation:\n";
    out << "- result: " << (report.errors.empty() ? "passed" : "failed") << "\n";
    out << "- plans: " << report.plans.size() << "\n";
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

[[nodiscard]] std::string format_candidate_artifact_plan_list(const ArchiveEngineState& state, AccessLevel access) {
    const std::vector<CandidateArtifactPlan> plans = plans_for_formatting(state, access);
    std::ostringstream out;
    out << "CandidateArtifactPlans visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlan)) {
        std::size_t visible = 0;
        for (const CandidateArtifactPlan& plan : plans) {
            if (plan_visible_to(plan, access)) {
                ++visible;
            }
        }
        out << "- public_safe_visible_plans: " << visible << "\n";
        out << "- details: public/scholar access receives aggregate counts and public-safe summaries only; diagnostic IDs and hidden rationale are restricted.\n";
        for (const CandidateArtifactPlan& plan : plans) {
            if (!plan_visible_to(plan, access)) {
                continue;
            }
            out << "- " << to_string(plan.planned_shape)
                << ": artifact_type=" << to_string(plan.planned_artifact_type)
                << " status=" << to_string(plan.status)
                << " risk=" << to_string(plan.risk_level)
                << " role=public evidence planning summary\n";
        }
        return out.str();
    }
    if (plans.empty()) {
        out << "- none\n";
        return out.str();
    }
    for (const CandidateArtifactPlan& plan : plans) {
        out << "- " << plan.id
            << ": source=" << plan.source_id
            << " shape=" << to_string(plan.planned_shape)
            << " artifact_type=" << to_string(plan.planned_artifact_type)
            << " status=" << to_string(plan.status)
            << " risk=" << to_string(plan.risk_level)
            << " current_materialization_enabled=" << (plan.current_materialization_enabled ? "true" : "false") << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_plan_detail(const ArchiveEngineState& state,
                                                                AccessLevel access,
                                                                const std::string& plan_id) {
    const std::vector<CandidateArtifactPlan> plans = plans_for_formatting(state, access);
    const auto it = std::find_if(plans.begin(), plans.end(), [&](const CandidateArtifactPlan& plan) {
        return plan.id == plan_id;
    });
    std::ostringstream out;
    out << "CandidateArtifactPlan:\n";
    if (it == plans.end() || !plan_visible_to(*it, access)) {
        out << "- found: false\n";
        return out.str();
    }
    out << "- found: true\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlan)) {
        out << "- planned_shape: " << to_string(it->planned_shape) << "\n";
        out << "- planned_artifact_type: " << to_string(it->planned_artifact_type) << "\n";
        out << "- evidence_role: public evidence planning summary\n";
        out << "- status: " << to_string(it->status) << "\n";
        out << "- risk_level: " << to_string(it->risk_level) << "\n";
        out << "- public_safe: " << (it->public_safe ? "true" : "false") << "\n";
        out << "- current_materialization_enabled: false\n";
        out << "- details: restricted\n";
        return out.str();
    }
    out << "- id: " << it->id << "\n";
    out << "- source_type: " << to_string(it->source_type) << "\n";
    out << "- source_id: " << it->source_id << "\n";
    out << "- planned_shape: " << to_string(it->planned_shape) << "\n";
    out << "- planned_artifact_type: " << to_string(it->planned_artifact_type) << "\n";
    out << "- target_topic: " << it->target_topic << "\n";
    out << "- target_year: " << year_text(it->target_year) << "\n";
    out << "- evidence_role: " << it->evidence_role << "\n";
    out << "- rationale: " << it->rationale << "\n";
    out << "- public_safe: " << (it->public_safe ? "true" : "false") << "\n";
    out << "- requires_curator_review: " << (it->requires_curator_review ? "true" : "false") << "\n";
    out << "- materializable_in_future: " << (it->materializable_in_future ? "true" : "false") << "\n";
    out << "- current_materialization_enabled: " << (it->current_materialization_enabled ? "true" : "false") << "\n";
    out << "- status: " << to_string(it->status) << "\n";
    out << "- risk_level: " << to_string(it->risk_level) << "\n";
    if (!it->expected_claim_types.empty()) {
        out << "- expected_claim_types:";
        for (const std::string& id : it->expected_claim_types) {
            out << " " << id;
        }
        out << "\n";
    }
    if (!it->expected_distortion_modes.empty()) {
        out << "- expected_distortion_modes:";
        for (const std::string& id : it->expected_distortion_modes) {
            out << " " << id;
        }
        out << "\n";
    }
    if (!it->required_validation_steps.empty()) {
        out << "Required validation steps:\n";
        for (const std::string& step : it->required_validation_steps) {
            out << "- " << step << "\n";
        }
    }
    if (!it->knowledge_horizon_finding_ids.empty()) {
        out << "- knowledge_horizon_finding_ids:";
        for (const std::string& id : it->knowledge_horizon_finding_ids) {
            out << " " << id;
        }
        out << "\n";
    }
    if (!it->contradiction_budget_bucket_ids.empty()) {
        out << "- contradiction_budget_bucket_ids:";
        for (const std::string& id : it->contradiction_budget_bucket_ids) {
            out << " " << id;
        }
        out << "\n";
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
