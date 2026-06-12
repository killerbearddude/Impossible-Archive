#pragma once
#include "evidence_potential_model.h"

namespace archive {

enum class CandidateArtifactPlanSourceType {
    EvidencePotential,
    HiddenMutation,
    ManualCuratorSeed,
};

enum class CandidateArtifactPlanShape {
    AdministrativeDocket,
    RitualNotice,
    LedgerEntry,
    BoundaryInscription,
    ShrineCopy,
    ScholarFragment,
    OralTraditionFragment,
    MaterialTrace,
    AbsenceRecord,
};

enum class CandidateArtifactPlanStatus {
    Plausible,
    NeedsCuratorReview,
    BlockedByKnowledgeHorizon,
    BlockedByContradictionPressure,
    BlockedByProtectedMystery,
    BlockedByPublicSafety,
    Invalid,
};

enum class CandidateArtifactPlanRiskLevel {
    Low,
    Moderate,
    High,
    Critical,
};

struct CandidateArtifactPlan {
    std::string id;

    CandidateArtifactPlanSourceType source_type = CandidateArtifactPlanSourceType::EvidencePotential;
    std::string source_id;

    CandidateArtifactPlanShape planned_shape = CandidateArtifactPlanShape::AdministrativeDocket;
    ArtifactType planned_artifact_type = ArtifactType::Inscription;

    std::string target_topic;
    int target_year = 0;

    std::string evidence_role;
    std::string rationale;

    std::vector<std::string> expected_claim_types;
    std::vector<std::string> expected_distortion_modes;
    std::vector<std::string> required_validation_steps;

    bool public_safe = false;
    bool requires_curator_review = true;
    bool materializable_in_future = false;
    bool current_materialization_enabled = false;

    CandidateArtifactPlanStatus status = CandidateArtifactPlanStatus::NeedsCuratorReview;
    CandidateArtifactPlanRiskLevel risk_level = CandidateArtifactPlanRiskLevel::Moderate;

    std::vector<std::string> knowledge_horizon_finding_ids;
    std::vector<std::string> contradiction_budget_bucket_ids;
    std::vector<std::string> warnings;
};

struct CandidateArtifactPlanReport {
    std::vector<CandidateArtifactPlan> plans;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(CandidateArtifactPlanSourceType type);
[[nodiscard]] std::string to_string(CandidateArtifactPlanShape shape);
[[nodiscard]] std::string to_string(CandidateArtifactPlanStatus status);
[[nodiscard]] std::string to_string(CandidateArtifactPlanRiskLevel risk);

} // namespace archive
