#pragma once
#include "candidate_artifact_proposal_audit_model.h"

namespace archive {

enum class CandidateArtifactDraftStatus {
    ReadyForOutline,
    NeedsRevision,
    NeedsCuratorReview,
    Blocked,
    Invalid,
};

enum class CandidateArtifactDraftVisibilityClass {
    PublicSummary,
    ScholarSummary,
    CuratorOnly,
    DebugOnly,
};

struct CandidateArtifactDraft {
    std::string id;

    std::string proposal_id;
    std::string audit_id;
    std::string plan_id;
    std::string evaluation_id;
    std::string source_evidence_potential_id;

    CandidateArtifactDraftStatus status = CandidateArtifactDraftStatus::NeedsCuratorReview;
    CandidateArtifactDraftVisibilityClass visibility_class = CandidateArtifactDraftVisibilityClass::CuratorOnly;

    ArtifactType intended_artifact_type = ArtifactType::Inscription;
    ArtifactVoiceRegister intended_voice_register = ArtifactVoiceRegister::RoyalInscription;

    std::string outline_title;
    std::string target_topic;
    int intended_creation_year = 0;
    int intended_discovery_year = 0;

    std::vector<std::string> source_chain_ids;
    std::vector<std::string> claim_outline_lines;
    std::vector<std::string> required_validation_gates;
    std::vector<std::string> public_safe_summary_lines;
    std::vector<std::string> curator_notes;
    std::vector<std::string> warnings;

    bool contains_hidden_source_reference = true;
    bool contains_curator_diagnostics = true;
    bool touches_protected_mystery = false;

    bool current_artifact_insertion_enabled = false;
    bool current_public_claim_insertion_enabled = false;
    bool current_discovery_scheduling_enabled = false;
    bool hidden_truth_mutation_enabled = false;
    bool public_archive_mutation_enabled = false;
    bool persistence_enabled = false;
};

struct CandidateArtifactDraftReport {
    std::vector<CandidateArtifactDraft> drafts;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(CandidateArtifactDraftStatus status);
[[nodiscard]] std::string to_string(CandidateArtifactDraftVisibilityClass visibility_class);

} // namespace archive
