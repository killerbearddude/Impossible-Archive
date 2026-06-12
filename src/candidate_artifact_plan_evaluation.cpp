#include "candidate_artifact_plan_evaluation_api.h"
#include "candidate_artifact_plan_api.h"
#include "contradiction_budget_api.h"
#include "diagnostic_access_policy.h"
#include "knowledge_horizon_api.h"

#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {

[[nodiscard]] const EvidencePotential* find_potential(const ArchiveEngineState& state, const std::string& id) {
    const auto it = std::find_if(state.evidence_potentials.begin(), state.evidence_potentials.end(), [&](const EvidencePotential& potential) {
        return potential.id == id;
    });
    return it == state.evidence_potentials.end() ? nullptr : &*it;
}

[[nodiscard]] const CandidateArtifactPlan* find_plan(const ArchiveEngineState& state, const std::string& id) {
    const auto it = std::find_if(state.candidate_artifact_plans.begin(), state.candidate_artifact_plans.end(), [&](const CandidateArtifactPlan& plan) {
        return plan.id == id;
    });
    return it == state.candidate_artifact_plans.end() ? nullptr : &*it;
}

[[nodiscard]] const KnowledgeHorizonFinding* find_horizon_finding(const KnowledgeHorizonReport& report, const std::string& id) {
    const auto it = std::find_if(report.findings.begin(), report.findings.end(), [&](const KnowledgeHorizonFinding& finding) {
        return finding.id == id;
    });
    return it == report.findings.end() ? nullptr : &*it;
}

[[nodiscard]] const ContradictionBudgetBucket* find_budget_bucket(const ContradictionBudgetReport& report, const std::string& id) {
    const auto it = std::find_if(report.buckets.begin(), report.buckets.end(), [&](const ContradictionBudgetBucket& bucket) {
        return bucket.id == id;
    });
    return it == report.buckets.end() ? nullptr : &*it;
}

[[nodiscard]] bool source_touches_protected_mystery(const ArchiveEngineState& state, const CandidateArtifactPlan& plan) {
    const EvidencePotential* potential = find_potential(state, plan.source_id);
    if (potential == nullptr || potential->source_type != EvidencePotentialSourceType::Mystery) {
        return false;
    }
    const auto it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
        return mystery.id == potential->source_id;
    });
    if (it == state.mysteries.end()) {
        return false;
    }
    return it->reveal_mode == RevealMode::NeverFullyResolvable ||
           it->reveal_mode == RevealMode::AccessLocked ||
           it->reveal_mode == RevealMode::ContradictoryByDesign ||
           !can_view(AccessLevel::Public, it->min_access);
}

[[nodiscard]] bool has_specificity_word(std::string_view text) {
    return text.find("evidence_potential.") != std::string_view::npos ||
           text.find("marsh") != std::string_view::npos ||
           text.find("citadel") != std::string_view::npos ||
           text.find("ritual") != std::string_view::npos ||
           text.find("ledger") != std::string_view::npos ||
           text.find("shrine") != std::string_view::npos ||
           text.find("boundary") != std::string_view::npos ||
           text.find("material") != std::string_view::npos ||
           text.find("absence") != std::string_view::npos ||
           text.find("copy") != std::string_view::npos;
}

[[nodiscard]] double specificity_score_for_plan(const CandidateArtifactPlan& plan) {
    double score = 0.0;
    if (!plan.source_id.empty()) { score += 0.20; }
    if (!plan.target_topic.empty()) { score += 0.20; }
    if (!plan.evidence_role.empty() && has_specificity_word(plan.evidence_role)) { score += 0.20; }
    if (!plan.rationale.empty() && has_specificity_word(plan.rationale)) { score += 0.25; }
    if (!plan.expected_claim_types.empty() || !plan.expected_distortion_modes.empty()) { score += 0.15; }
    return clamp01(score);
}

void add_finding(CandidateArtifactPlanEvaluation& evaluation,
                 CandidateArtifactPlanEvaluationGate gate,
                 CandidateArtifactPlanEvaluationSeverity severity,
                 const std::string& message,
                 const std::string& related_id) {
    CandidateArtifactPlanEvaluationFinding finding;
    finding.id = evaluation.id + ".finding." + std::to_string(evaluation.findings.size());
    finding.gate = gate;
    finding.severity = severity;
    finding.message = message;
    finding.related_id = related_id;
    evaluation.findings.push_back(std::move(finding));
}

[[nodiscard]] std::size_t count_severity(const CandidateArtifactPlanEvaluation& evaluation,
                                          CandidateArtifactPlanEvaluationSeverity severity) {
    return static_cast<std::size_t>(std::count_if(evaluation.findings.begin(), evaluation.findings.end(), [&](const CandidateArtifactPlanEvaluationFinding& finding) {
        return finding.severity == severity;
    }));
}

[[nodiscard]] bool has_error_gate(const CandidateArtifactPlanEvaluation& evaluation, CandidateArtifactPlanEvaluationGate gate) {
    return std::any_of(evaluation.findings.begin(), evaluation.findings.end(), [&](const CandidateArtifactPlanEvaluationFinding& finding) {
        return finding.gate == gate && finding.severity == CandidateArtifactPlanEvaluationSeverity::Error;
    });
}

[[nodiscard]] bool evaluation_visible_to(const CandidateArtifactPlanEvaluation& evaluation,
                                          const ArchiveEngineState& state,
                                          AccessLevel access) {
    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlanEvaluation)) {
        return true;
    }
    const CandidateArtifactPlan* plan = find_plan(state, evaluation.plan_id);
    return plan != nullptr && plan->public_safe && evaluation.public_safe &&
           (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Pass ||
            evaluation.decision == CandidateArtifactPlanEvaluationDecision::NeedsCuratorReview);
}

[[nodiscard]] std::vector<CandidateArtifactPlanEvaluation> evaluations_for_formatting(const ArchiveEngineState& state, AccessLevel access) {
    if (!state.candidate_artifact_plan_evaluations.empty()) {
        return state.candidate_artifact_plan_evaluations;
    }
    return evaluate_candidate_artifact_plans(state, access).evaluations;
}

void append_counts(std::ostringstream& out, const std::map<std::string, std::size_t>& counts) {
    for (const auto& [label, count] : counts) {
        out << "- " << label << ": " << count << "\n";
    }
}

[[nodiscard]] bool score_out_of_range(double score) {
    return score < 0.0 || score > 1.0;
}

[[nodiscard]] bool has_hidden_diagnostic_related_id(const CandidateArtifactPlanEvaluation& evaluation) {
    for (const CandidateArtifactPlanEvaluationFinding& finding : evaluation.findings) {
        if (has_prefix(finding.related_id, "knowledge_horizon.") ||
            has_prefix(finding.related_id, "contradiction_budget.") ||
            has_prefix(finding.related_id, "event.") ||
            has_prefix(finding.related_id, "entity.") ||
            has_prefix(finding.related_id, "mystery.")) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool id_in(const std::vector<std::string>& values, const std::string& id) {
    return std::find(values.begin(), values.end(), id) != values.end();
}

} // namespace

[[nodiscard]] std::string to_string(CandidateArtifactPlanEvaluationDecision decision) {
    switch (decision) {
        case CandidateArtifactPlanEvaluationDecision::Pass: return "pass";
        case CandidateArtifactPlanEvaluationDecision::NeedsCuratorReview: return "needs_curator_review";
        case CandidateArtifactPlanEvaluationDecision::Blocked: return "blocked";
        case CandidateArtifactPlanEvaluationDecision::Invalid: return "invalid";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactPlanEvaluationGate gate) {
    switch (gate) {
        case CandidateArtifactPlanEvaluationGate::SourceEvidencePotential: return "source_evidence_potential";
        case CandidateArtifactPlanEvaluationGate::KnowledgeHorizon: return "knowledge_horizon";
        case CandidateArtifactPlanEvaluationGate::ContradictionBudget: return "contradiction_budget";
        case CandidateArtifactPlanEvaluationGate::ProtectedMystery: return "protected_mystery";
        case CandidateArtifactPlanEvaluationGate::PublicSafety: return "public_safety";
        case CandidateArtifactPlanEvaluationGate::CivilizationSpecificity: return "civilization_specificity";
        case CandidateArtifactPlanEvaluationGate::RequiredValidationSteps: return "required_validation_steps";
        case CandidateArtifactPlanEvaluationGate::CurrentMaterializationDisabled: return "current_materialization_disabled";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactPlanEvaluationSeverity severity) {
    switch (severity) {
        case CandidateArtifactPlanEvaluationSeverity::Info: return "info";
        case CandidateArtifactPlanEvaluationSeverity::Warning: return "warning";
        case CandidateArtifactPlanEvaluationSeverity::Error: return "error";
    }
    return "unknown";
}

[[nodiscard]] CandidateArtifactPlanEvaluationDecision classify_candidate_artifact_plan_evaluation(const CandidateArtifactPlanEvaluation& evaluation) {
    if (evaluation.id.empty() || evaluation.plan_id.empty() || evaluation.current_generation_enabled || evaluation.current_materialization_enabled ||
        has_error_gate(evaluation, CandidateArtifactPlanEvaluationGate::SourceEvidencePotential)) {
        return CandidateArtifactPlanEvaluationDecision::Invalid;
    }
    if (count_severity(evaluation, CandidateArtifactPlanEvaluationSeverity::Error) > 0U) {
        return CandidateArtifactPlanEvaluationDecision::Blocked;
    }
    if (count_severity(evaluation, CandidateArtifactPlanEvaluationSeverity::Warning) > 0U ||
        !evaluation.protected_mystery_clear || !evaluation.civilization_specificity_clear) {
        return CandidateArtifactPlanEvaluationDecision::NeedsCuratorReview;
    }
    return CandidateArtifactPlanEvaluationDecision::Pass;
}

[[nodiscard]] CandidateArtifactPlanEvaluation evaluate_candidate_artifact_plan(const ArchiveEngineState& state,
                                                                               const CandidateArtifactPlan& plan,
                                                                               AccessLevel access) {
    CandidateArtifactPlanEvaluation evaluation;
    evaluation.id = "candidate_artifact_plan_evaluation." + plan.id;
    evaluation.plan_id = plan.id;
    evaluation.current_generation_enabled = false;
    evaluation.current_materialization_enabled = false;

    const EvidencePotential* potential = find_potential(state, plan.source_id);
    evaluation.source_valid = potential != nullptr && !plan.id.empty() && !plan.source_id.empty();
    add_finding(evaluation,
                CandidateArtifactPlanEvaluationGate::SourceEvidencePotential,
                evaluation.source_valid ? CandidateArtifactPlanEvaluationSeverity::Info : CandidateArtifactPlanEvaluationSeverity::Error,
                evaluation.source_valid ? "Source EvidencePotential exists and is available for read-only evaluation." : "Plan references a missing or malformed EvidencePotential source.",
                plan.source_id);

    add_finding(evaluation,
                CandidateArtifactPlanEvaluationGate::CurrentMaterializationDisabled,
                plan.current_materialization_enabled ? CandidateArtifactPlanEvaluationSeverity::Error : CandidateArtifactPlanEvaluationSeverity::Info,
                plan.current_materialization_enabled ? "Plan claims current materialization is enabled; v28.6 evaluation must remain read-only." : "Current generation and materialization remain disabled; evaluation is advisory only.",
                plan.id);

    add_finding(evaluation,
                CandidateArtifactPlanEvaluationGate::RequiredValidationSteps,
                plan.required_validation_steps.empty() ? CandidateArtifactPlanEvaluationSeverity::Warning : CandidateArtifactPlanEvaluationSeverity::Info,
                plan.required_validation_steps.empty() ? "Plan has no required validation steps for a future candidate-generation gate." : "Plan carries required validation steps for future candidate work.",
                plan.id);

    const KnowledgeHorizonReport horizon_report = validate_knowledge_horizon(state, AccessLevel::Curator);
    evaluation.knowledge_horizon_clear = true;
    for (const std::string& finding_id : plan.knowledge_horizon_finding_ids) {
        const KnowledgeHorizonFinding* finding = find_horizon_finding(horizon_report, finding_id);
        if (finding == nullptr) {
            evaluation.knowledge_horizon_clear = false;
            add_finding(evaluation, CandidateArtifactPlanEvaluationGate::KnowledgeHorizon, CandidateArtifactPlanEvaluationSeverity::Error, "Plan references an unknown KnowledgeHorizon finding.", finding_id);
        } else if (is_invalid(finding->status)) {
            evaluation.knowledge_horizon_clear = false;
            add_finding(evaluation, CandidateArtifactPlanEvaluationGate::KnowledgeHorizon, CandidateArtifactPlanEvaluationSeverity::Error, "KnowledgeHorizon blocks this plan for future candidate generation.", finding_id);
        }
    }
    if (evaluation.knowledge_horizon_clear) {
        add_finding(evaluation, CandidateArtifactPlanEvaluationGate::KnowledgeHorizon, CandidateArtifactPlanEvaluationSeverity::Info, "No referenced KnowledgeHorizon finding blocks this plan.", plan.id);
    }

    const ContradictionBudgetReport budget_report = compute_contradiction_budget(state, AccessLevel::Curator);
    evaluation.contradiction_budget_clear = true;
    for (const std::string& bucket_id : plan.contradiction_budget_bucket_ids) {
        const ContradictionBudgetBucket* bucket = find_budget_bucket(budget_report, bucket_id);
        if (bucket == nullptr) {
            evaluation.contradiction_budget_clear = false;
            add_finding(evaluation, CandidateArtifactPlanEvaluationGate::ContradictionBudget, CandidateArtifactPlanEvaluationSeverity::Error, "Plan references an unknown ContradictionBudget bucket.", bucket_id);
        } else if (bucket->severity == ContradictionBudgetSeverity::High || bucket->severity == ContradictionBudgetSeverity::Critical) {
            evaluation.contradiction_budget_clear = false;
            add_finding(evaluation, CandidateArtifactPlanEvaluationGate::ContradictionBudget, CandidateArtifactPlanEvaluationSeverity::Error, "High or critical ContradictionBudget pressure blocks this plan.", bucket_id);
        } else if (bucket->severity == ContradictionBudgetSeverity::Moderate) {
            add_finding(evaluation, CandidateArtifactPlanEvaluationGate::ContradictionBudget, CandidateArtifactPlanEvaluationSeverity::Warning, "Moderate ContradictionBudget pressure requires curator review.", bucket_id);
        }
    }
    if (evaluation.contradiction_budget_clear) {
        add_finding(evaluation, CandidateArtifactPlanEvaluationGate::ContradictionBudget, CandidateArtifactPlanEvaluationSeverity::Info, "No referenced ContradictionBudget bucket blocks this plan.", plan.id);
    }

    evaluation.protected_mystery_clear = !source_touches_protected_mystery(state, plan) &&
                                         plan.status != CandidateArtifactPlanStatus::BlockedByProtectedMystery;
    if (!evaluation.protected_mystery_clear) {
        add_finding(evaluation, CandidateArtifactPlanEvaluationGate::ProtectedMystery, CandidateArtifactPlanEvaluationSeverity::Error, "Plan touches protected mystery pressure and must not advance toward generation without a later gate.", plan.id);
    } else if (plan.requires_curator_review) {
        add_finding(evaluation, CandidateArtifactPlanEvaluationGate::ProtectedMystery, CandidateArtifactPlanEvaluationSeverity::Warning, "Plan requires curator review before any future candidate-generation phase.", plan.id);
    } else {
        add_finding(evaluation, CandidateArtifactPlanEvaluationGate::ProtectedMystery, CandidateArtifactPlanEvaluationSeverity::Info, "No protected mystery pressure blocks this plan.", plan.id);
    }

    evaluation.public_safe = plan.public_safe;
    if (!plan.public_safe && !can_view(access, AccessLevel::Curator)) {
        add_finding(evaluation, CandidateArtifactPlanEvaluationGate::PublicSafety, CandidateArtifactPlanEvaluationSeverity::Error, "Plan is not public-safe at the requested access level.", plan.id);
    } else if (!plan.public_safe) {
        add_finding(evaluation, CandidateArtifactPlanEvaluationGate::PublicSafety, CandidateArtifactPlanEvaluationSeverity::Warning, "Plan is available only as curator/debug diagnostic detail.", plan.id);
    } else {
        add_finding(evaluation, CandidateArtifactPlanEvaluationGate::PublicSafety, CandidateArtifactPlanEvaluationSeverity::Info, "Plan is public-safe for restricted summary use.", plan.id);
    }

    evaluation.civilization_specificity_score = specificity_score_for_plan(plan);
    evaluation.civilization_specificity_clear = evaluation.civilization_specificity_score >= 0.50;
    add_finding(evaluation,
                CandidateArtifactPlanEvaluationGate::CivilizationSpecificity,
                evaluation.civilization_specificity_clear ? CandidateArtifactPlanEvaluationSeverity::Info : CandidateArtifactPlanEvaluationSeverity::Error,
                evaluation.civilization_specificity_clear ? "Plan has enough local dependency and source-specific grounding for read-only evaluation." : "Plan is too generic for future candidate generation without more civilization-specific grounding.",
                plan.id);

    if (evaluation.public_safe && has_hidden_diagnostic_related_id(evaluation)) {
        evaluation.public_safe = false;
    }

    evaluation.required_next_checks = plan.required_validation_steps;
    add_unique_string(evaluation.required_next_checks, "draft future CandidateArtifactProposal without mutating archive state");
    add_unique_string(evaluation.required_next_checks, "keep current_generation_enabled=false until a later explicit slice");
    add_unique_string(evaluation.required_next_checks, "keep current_materialization_enabled=false until a later explicit slice");

    evaluation.readiness_score = clamp01(
        (evaluation.source_valid ? 0.20 : 0.0) +
        (evaluation.knowledge_horizon_clear ? 0.20 : 0.0) +
        (evaluation.contradiction_budget_clear ? 0.20 : 0.0) +
        (evaluation.protected_mystery_clear ? 0.15 : 0.0) +
        ((evaluation.public_safe || can_view(access, AccessLevel::Curator)) ? 0.15 : 0.0) +
        (evaluation.civilization_specificity_clear ? 0.10 : 0.0));

    const std::size_t warning_count = count_severity(evaluation, CandidateArtifactPlanEvaluationSeverity::Warning);
    const std::size_t error_count = count_severity(evaluation, CandidateArtifactPlanEvaluationSeverity::Error);
    evaluation.risk_score = clamp01(static_cast<double>(warning_count) * 0.10 +
                                    static_cast<double>(error_count) * 0.30 +
                                    (!evaluation.contradiction_budget_clear ? 0.20 : 0.0) +
                                    (!evaluation.protected_mystery_clear ? 0.20 : 0.0) +
                                    ((!evaluation.public_safe && !can_view(access, AccessLevel::Curator)) ? 0.20 : 0.0));
    evaluation.decision = classify_candidate_artifact_plan_evaluation(evaluation);
    return evaluation;
}

[[nodiscard]] CandidateArtifactPlanEvaluationReport evaluate_candidate_artifact_plans(const ArchiveEngineState& state, AccessLevel access) {
    CandidateArtifactPlanEvaluationReport report;
    std::vector<CandidateArtifactPlan> plans = state.candidate_artifact_plans;
    if (plans.empty()) {
        plans = derive_candidate_artifact_plans(state, AccessLevel::Curator).plans;
    }
    for (const CandidateArtifactPlan& plan : plans) {
        report.evaluations.push_back(evaluate_candidate_artifact_plan(state, plan, access));
    }
    std::sort(report.evaluations.begin(), report.evaluations.end(), [](const CandidateArtifactPlanEvaluation& lhs, const CandidateArtifactPlanEvaluation& rhs) {
        return lhs.id < rhs.id;
    });
    ArchiveEngineState validation_state = state;
    validation_state.candidate_artifact_plan_evaluations = report.evaluations;
    report.errors = validate_candidate_artifact_plan_evaluations(validation_state);
    return report;
}

void evaluate_candidate_artifact_plans_into_state(ArchiveEngineState& state, AccessLevel access) {
    state.candidate_artifact_plan_evaluations = evaluate_candidate_artifact_plans(state, access).evaluations;
}

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_plan_evaluations(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> seen_ids;
    std::vector<std::string> plan_ids;
    for (const CandidateArtifactPlan& plan : state.candidate_artifact_plans) {
        plan_ids.push_back(plan.id);
    }
    for (const CandidateArtifactPlanEvaluation& evaluation : state.candidate_artifact_plan_evaluations) {
        if (evaluation.id.empty()) {
            errors.push_back("CandidateArtifactPlanEvaluation has empty id");
        } else if (!seen_ids.insert(evaluation.id).second) {
            errors.push_back("CandidateArtifactPlanEvaluation has duplicate id: " + evaluation.id);
        }
        if (evaluation.plan_id.empty()) {
            errors.push_back("CandidateArtifactPlanEvaluation has empty plan id: " + evaluation.id);
        } else if (!id_in(plan_ids, evaluation.plan_id)) {
            errors.push_back("CandidateArtifactPlanEvaluation references missing plan: " + evaluation.id + " -> " + evaluation.plan_id);
        }
        if (score_out_of_range(evaluation.readiness_score)) {
            errors.push_back("CandidateArtifactPlanEvaluation has readiness score outside [0,1]: " + evaluation.id);
        }
        if (score_out_of_range(evaluation.risk_score)) {
            errors.push_back("CandidateArtifactPlanEvaluation has risk score outside [0,1]: " + evaluation.id);
        }
        if (evaluation.current_generation_enabled) {
            errors.push_back("CandidateArtifactPlanEvaluation enables current generation in v28.6: " + evaluation.id);
        }
        if (evaluation.current_materialization_enabled) {
            errors.push_back("CandidateArtifactPlanEvaluation enables current materialization in v28.6: " + evaluation.id);
        }
        const bool has_error = count_severity(evaluation, CandidateArtifactPlanEvaluationSeverity::Error) > 0U;
        if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Pass && has_error) {
            errors.push_back("CandidateArtifactPlanEvaluation has Pass decision with error findings: " + evaluation.id);
        }
        if (evaluation.decision == CandidateArtifactPlanEvaluationDecision::Pass &&
            (evaluation.current_generation_enabled || evaluation.current_materialization_enabled)) {
            errors.push_back("CandidateArtifactPlanEvaluation has Pass decision while generation/materialization is enabled: " + evaluation.id);
        }
        if (evaluation.public_safe && has_hidden_diagnostic_related_id(evaluation)) {
            errors.push_back("CandidateArtifactPlanEvaluation public-safe output carries hidden diagnostic related IDs: " + evaluation.id);
        }
    }
    return errors;
}

[[nodiscard]] std::string format_candidate_artifact_plan_evaluation_summary(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactPlanEvaluationReport report = evaluate_candidate_artifact_plans(state, access);
    std::map<std::string, std::size_t> by_decision;
    std::map<std::string, std::size_t> readiness_buckets;
    std::map<std::string, std::size_t> risk_buckets;
    std::size_t generation_enabled = 0;
    std::size_t materialization_enabled = 0;
    for (const CandidateArtifactPlanEvaluation& evaluation : report.evaluations) {
        ++by_decision[to_string(evaluation.decision)];
        ++readiness_buckets[evaluation.readiness_score >= 0.80 ? "high" : (evaluation.readiness_score >= 0.50 ? "moderate" : "low")];
        ++risk_buckets[evaluation.risk_score >= 0.70 ? "high" : (evaluation.risk_score >= 0.30 ? "moderate" : "low")];
        if (evaluation.current_generation_enabled) { ++generation_enabled; }
        if (evaluation.current_materialization_enabled) { ++materialization_enabled; }
    }
    std::ostringstream out;
    out << "CandidateArtifactPlanEvaluation summary:\n";
    out << "- behavior: evaluation only; no candidate artifacts, artifact text, discoveries, materialization, public archive mutation, hidden truth mutation, fragment activation, resolver/composition, persistence, or session state are introduced in v28.6.\n";
    out << "- total_evaluations: " << report.evaluations.size() << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    out << "- current_generation_enabled: " << generation_enabled << "\n";
    out << "- current_materialization_enabled: " << materialization_enabled << "\n";
    out << "Decision counts:\n";
    append_counts(out, by_decision);
    out << "Readiness buckets:\n";
    append_counts(out, readiness_buckets);
    out << "Risk buckets:\n";
    append_counts(out, risk_buckets);
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlanEvaluation)) {
        out << "- details: aggregate-only at this access level; hidden plan IDs, KnowledgeHorizon IDs, ContradictionBudget IDs, protected mystery details, hidden rationale, and curator-only findings are restricted.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_plan_evaluation_validation(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactPlanEvaluationReport report = evaluate_candidate_artifact_plans(state, access);
    std::ostringstream out;
    out << "CandidateArtifactPlanEvaluation validation:\n";
    out << "- result: " << (report.errors.empty() ? "passed" : "failed") << "\n";
    out << "- evaluations: " << report.evaluations.size() << "\n";
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

[[nodiscard]] std::string format_candidate_artifact_plan_evaluation_list(const ArchiveEngineState& state, AccessLevel access) {
    const std::vector<CandidateArtifactPlanEvaluation> evaluations = evaluations_for_formatting(state, access);
    std::ostringstream out;
    out << "CandidateArtifactPlanEvaluations visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlanEvaluation)) {
        std::size_t visible = 0;
        for (const CandidateArtifactPlanEvaluation& evaluation : evaluations) {
            if (evaluation_visible_to(evaluation, state, access)) { ++visible; }
        }
        out << "- public_safe_visible_evaluations: " << visible << "\n";
        out << "- details: public/scholar access receives aggregate counts and public-safe summaries only; diagnostic IDs and hidden rationale are restricted.\n";
        for (const CandidateArtifactPlanEvaluation& evaluation : evaluations) {
            if (!evaluation_visible_to(evaluation, state, access)) { continue; }
            out << "- decision=" << to_string(evaluation.decision)
                << " readiness=" << evaluation.readiness_score
                << " risk=" << evaluation.risk_score
                << " current_generation_enabled=false current_materialization_enabled=false\n";
        }
        return out.str();
    }
    if (evaluations.empty()) {
        out << "- none\n";
        return out.str();
    }
    for (const CandidateArtifactPlanEvaluation& evaluation : evaluations) {
        out << "- " << evaluation.id
            << ": plan=" << evaluation.plan_id
            << " decision=" << to_string(evaluation.decision)
            << " readiness=" << evaluation.readiness_score
            << " risk=" << evaluation.risk_score
            << " current_generation_enabled=" << (evaluation.current_generation_enabled ? "true" : "false")
            << " current_materialization_enabled=" << (evaluation.current_materialization_enabled ? "true" : "false") << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_plan_evaluation_detail(const ArchiveEngineState& state,
                                                                            AccessLevel access,
                                                                            const std::string& evaluation_id) {
    const std::vector<CandidateArtifactPlanEvaluation> evaluations = evaluations_for_formatting(state, access);
    const auto it = std::find_if(evaluations.begin(), evaluations.end(), [&](const CandidateArtifactPlanEvaluation& evaluation) {
        return evaluation.id == evaluation_id;
    });
    std::ostringstream out;
    out << "CandidateArtifactPlanEvaluation:\n";
    if (it == evaluations.end() || !evaluation_visible_to(*it, state, access)) {
        out << "- found: false\n";
        return out.str();
    }
    out << "- found: true\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlanEvaluation)) {
        out << "- decision: " << to_string(it->decision) << "\n";
        out << "- readiness_score: " << it->readiness_score << "\n";
        out << "- risk_score: " << it->risk_score << "\n";
        out << "- current_generation_enabled: false\n";
        out << "- current_materialization_enabled: false\n";
        out << "- details: restricted\n";
        return out.str();
    }
    out << "- id: " << it->id << "\n";
    out << "- plan_id: " << it->plan_id << "\n";
    out << "- decision: " << to_string(it->decision) << "\n";
    out << "- source_valid: " << (it->source_valid ? "true" : "false") << "\n";
    out << "- knowledge_horizon_clear: " << (it->knowledge_horizon_clear ? "true" : "false") << "\n";
    out << "- contradiction_budget_clear: " << (it->contradiction_budget_clear ? "true" : "false") << "\n";
    out << "- protected_mystery_clear: " << (it->protected_mystery_clear ? "true" : "false") << "\n";
    out << "- public_safe: " << (it->public_safe ? "true" : "false") << "\n";
    out << "- civilization_specificity_clear: " << (it->civilization_specificity_clear ? "true" : "false") << "\n";
    out << "- readiness_score: " << it->readiness_score << "\n";
    out << "- risk_score: " << it->risk_score << "\n";
    out << "- civilization_specificity_score: " << it->civilization_specificity_score << "\n";
    out << "- current_generation_enabled: " << (it->current_generation_enabled ? "true" : "false") << "\n";
    out << "- current_materialization_enabled: " << (it->current_materialization_enabled ? "true" : "false") << "\n";
    if (!it->findings.empty()) {
        out << "Findings:\n";
        for (const CandidateArtifactPlanEvaluationFinding& finding : it->findings) {
            out << "- " << finding.id
                << ": gate=" << to_string(finding.gate)
                << " severity=" << to_string(finding.severity)
                << " related_id=" << finding.related_id
                << " message=" << finding.message << "\n";
        }
    }
    if (!it->required_next_checks.empty()) {
        out << "Required next checks:\n";
        for (const std::string& check : it->required_next_checks) {
            out << "- " << check << "\n";
        }
    }
    return out.str();
}

} // namespace archive
