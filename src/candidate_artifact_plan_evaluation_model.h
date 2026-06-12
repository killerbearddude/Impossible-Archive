#pragma once
#include "candidate_artifact_plan_model.h"

namespace archive {

enum class CandidateArtifactPlanEvaluationDecision {
    Pass,
    NeedsCuratorReview,
    Blocked,
    Invalid,
};

enum class CandidateArtifactPlanEvaluationGate {
    SourceEvidencePotential,
    KnowledgeHorizon,
    ContradictionBudget,
    ProtectedMystery,
    PublicSafety,
    CivilizationSpecificity,
    RequiredValidationSteps,
    CurrentMaterializationDisabled,
};

enum class CandidateArtifactPlanEvaluationSeverity {
    Info,
    Warning,
    Error,
};

struct CandidateArtifactPlanEvaluationFinding {
    std::string id;
    CandidateArtifactPlanEvaluationGate gate = CandidateArtifactPlanEvaluationGate::SourceEvidencePotential;
    CandidateArtifactPlanEvaluationSeverity severity = CandidateArtifactPlanEvaluationSeverity::Info;
    std::string message;
    std::string related_id;
};

struct CandidateArtifactPlanEvaluation {
    std::string id;
    std::string plan_id;
    CandidateArtifactPlanEvaluationDecision decision = CandidateArtifactPlanEvaluationDecision::NeedsCuratorReview;
    bool source_valid = false;
    bool knowledge_horizon_clear = false;
    bool contradiction_budget_clear = false;
    bool protected_mystery_clear = false;
    bool public_safe = false;
    bool civilization_specificity_clear = false;
    double readiness_score = 0.0;
    double risk_score = 0.0;
    double civilization_specificity_score = 0.0;
    std::vector<CandidateArtifactPlanEvaluationFinding> findings;
    std::vector<std::string> required_next_checks;
    bool current_generation_enabled = false;
    bool current_materialization_enabled = false;
};

struct CandidateArtifactPlanEvaluationReport {
    std::vector<CandidateArtifactPlanEvaluation> evaluations;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(CandidateArtifactPlanEvaluationDecision decision);
[[nodiscard]] std::string to_string(CandidateArtifactPlanEvaluationGate gate);
[[nodiscard]] std::string to_string(CandidateArtifactPlanEvaluationSeverity severity);

} // namespace archive
