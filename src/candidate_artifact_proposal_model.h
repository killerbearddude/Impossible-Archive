#pragma once
#include "candidate_artifact_plan_evaluation_model.h"

namespace archive {

enum class CandidateArtifactProposalDecision {
    Draftable,
    NeedsCuratorReview,
    Blocked,
    Invalid,
};

enum class CandidateArtifactProposalCompleteness {
    Skeleton,
    Partial,
    Detailed,
};

enum class CandidateArtifactProposalSafety {
    PublicSafe,
    ScholarSafe,
    CuratorOnly,
    DebugOnly,
};

enum class CandidateArtifactProposalVisibilityClass {
    PublicEligible,
    ScholarEligible,
    CuratorOnly,
    DebugOnly,
};

enum class CandidateArtifactProposalTextStatus {
    NoTextGenerated,
    OutlineOnly,
    PlaceholderOnly,
};

struct CandidateArtifactProposal {
    std::string id;

    std::string plan_id;
    std::string evaluation_id;
    std::string source_evidence_potential_id;

    CandidateArtifactProposalDecision decision = CandidateArtifactProposalDecision::NeedsCuratorReview;
    CandidateArtifactProposalCompleteness completeness = CandidateArtifactProposalCompleteness::Skeleton;
    CandidateArtifactProposalTextStatus text_status = CandidateArtifactProposalTextStatus::NoTextGenerated;
    CandidateArtifactProposalVisibilityClass visibility_class = CandidateArtifactProposalVisibilityClass::CuratorOnly;

    bool contains_hidden_source_reference = true;
    bool contains_curator_diagnostics = true;
    bool touches_protected_mystery = false;
    bool requires_curator_review = true;

    CandidateArtifactPlanShape proposed_shape = CandidateArtifactPlanShape::AdministrativeDocket;
    ArtifactType proposed_artifact_type = ArtifactType::Inscription;
    ArtifactVoiceRegister proposed_voice_register = ArtifactVoiceRegister::RoyalInscription;

    std::string proposed_title;
    std::string target_topic;
    int proposed_creation_year = 0;
    int proposed_discovery_year = 0;

    std::string evidence_role;
    std::string proposal_rationale;

    std::vector<std::string> proposed_claim_skeletons;
    std::vector<std::string> proposed_distortion_modes;
    std::vector<std::string> proposed_damage_modes;
    std::vector<std::string> proposed_validation_gates;
    std::vector<std::string> required_access_notes;

    std::vector<std::string> blocking_evaluation_finding_ids;
    std::vector<std::string> warnings;

    bool current_generation_enabled = false;
    bool current_materialization_enabled = false;
    bool archive_mutation_enabled = false;
};

struct CandidateArtifactProposalReport {
    std::vector<CandidateArtifactProposal> proposals;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(CandidateArtifactProposalDecision decision);
[[nodiscard]] std::string to_string(CandidateArtifactProposalCompleteness completeness);
[[nodiscard]] std::string to_string(CandidateArtifactProposalSafety safety);
[[nodiscard]] std::string to_string(CandidateArtifactProposalVisibilityClass visibility_class);
[[nodiscard]] std::string to_string(CandidateArtifactProposalTextStatus status);

} // namespace archive
