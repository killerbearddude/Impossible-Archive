#pragma once
#include "candidate_artifact_proposal_model.h"

namespace archive {

enum class CandidateArtifactProposalAuditDecision {
    Pass,
    NeedsRevision,
    NeedsCuratorReview,
    Blocked,
    Invalid,
};

enum class CandidateArtifactProposalAuditGate {
    Structure,
    SourceContinuity,
    ProposalSpecificity,
    VoiceReadiness,
    ClaimSkeletonSafety,
    DistortionPlausibility,
    DamagePlausibility,
    KnowledgeHorizon,
    ContradictionBudget,
    ProtectedMystery,
    AccessSafety,
    NoGeneration,
    NoMutation,
};

enum class CandidateArtifactProposalAuditSeverity {
    Info,
    Warning,
    Error,
};

enum class CandidateArtifactProposalAuditReasonCode {
    None,
    MissingProposal,
    MissingEvaluation,
    MissingPlan,
    MissingEvidencePotential,
    MissingClaimSkeleton,
    MissingDistortionMode,
    MissingDamageMode,
    WeakSpecificity,
    WeakVoiceReadiness,
    KnowledgeHorizonBlocked,
    ContradictionBudgetHighPressure,
    ProtectedMysteryRisk,
    PublicAccessLeak,
    GenerationEnabled,
    MaterializationEnabled,
    ArchiveMutationEnabled,
    ScoreOutOfRange,
};

struct CandidateArtifactProposalAuditPolicy {
    double min_quality_score_for_pass = 0.80;
    double min_specificity_score_for_pass = 0.70;
    double min_safety_score_for_pass = 0.90;
    double max_revision_pressure_for_pass = 0.10;

    double min_quality_score_for_revision = 0.50;
    double min_specificity_score_for_revision = 0.40;
    double max_revision_pressure_before_block = 0.60;

    bool block_on_missing_source_chain = true;
    bool block_on_generation_enabled = true;
    bool block_on_materialization_enabled = true;
    bool block_on_archive_mutation_enabled = true;
    bool block_on_public_access_leak = true;
    bool require_claim_skeletons = true;
    bool require_distortion_modes = true;
    bool require_damage_modes = true;
    bool require_voice_readiness = true;
};

struct CandidateArtifactProposalAuditFinding {
    std::string id;
    CandidateArtifactProposalAuditGate gate = CandidateArtifactProposalAuditGate::Structure;
    CandidateArtifactProposalAuditSeverity severity = CandidateArtifactProposalAuditSeverity::Info;
    CandidateArtifactProposalAuditReasonCode reason_code = CandidateArtifactProposalAuditReasonCode::None;
    std::string message;
    std::string related_id;
};

struct CandidateArtifactProposalAudit {
    std::string id;
    std::string proposal_id;

    CandidateArtifactProposalAuditDecision decision = CandidateArtifactProposalAuditDecision::NeedsCuratorReview;

    bool structure_valid = false;
    bool source_continuity_valid = false;
    bool proposal_specificity_clear = false;
    bool voice_readiness_clear = false;
    bool claim_skeleton_safe = false;
    bool distortion_plausible = false;
    bool damage_plausible = false;
    bool knowledge_horizon_clear = false;
    bool contradiction_budget_clear = false;
    bool protected_mystery_clear = false;
    bool access_safe = false;

    double proposal_quality_score = 0.0;
    double specificity_score = 0.0;
    double safety_score = 0.0;
    double revision_pressure_score = 0.0;

    std::vector<CandidateArtifactProposalAuditFinding> findings;
    std::vector<std::string> required_revisions;

    bool current_generation_enabled = false;
    bool current_materialization_enabled = false;
    bool archive_mutation_enabled = false;
};

struct CandidateArtifactProposalAuditReport {
    std::vector<CandidateArtifactProposalAudit> audits;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(CandidateArtifactProposalAuditDecision decision);
[[nodiscard]] std::string to_string(CandidateArtifactProposalAuditGate gate);
[[nodiscard]] std::string to_string(CandidateArtifactProposalAuditSeverity severity);
[[nodiscard]] std::string to_string(CandidateArtifactProposalAuditReasonCode reason_code);
[[nodiscard]] CandidateArtifactProposalAuditPolicy default_candidate_artifact_proposal_audit_policy();

} // namespace archive
