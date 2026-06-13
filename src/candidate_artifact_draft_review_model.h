#pragma once
#include "candidate_artifact_draft_model.h"

namespace archive {

enum class CandidateArtifactDraftReviewDecision {
    Pass,
    NeedsRevision,
    NeedsCuratorReview,
    Blocked,
    Invalid,
};

struct CandidateArtifactDraftReview {
    std::string id;
    std::string draft_id;
    std::string proposal_id;
    std::string audit_id;
    CandidateArtifactDraftReviewDecision decision = CandidateArtifactDraftReviewDecision::NeedsCuratorReview;
    double outline_completeness_score = 0.0;
    double traceability_score = 0.0;
    double safety_score = 0.0;
    double specificity_score = 0.0;
    double revision_pressure_score = 0.0;
    std::vector<std::string> reason_codes;
    std::vector<std::string> required_revisions;
    std::vector<std::string> public_safe_summary_lines;
    std::vector<std::string> curator_notes;
    bool current_artifact_insertion_enabled = false;
    bool current_public_claim_insertion_enabled = false;
    bool current_discovery_scheduling_enabled = false;
    bool hidden_truth_mutation_enabled = false;
    bool public_archive_mutation_enabled = false;
    bool persistence_enabled = false;
    bool final_artifact_prose_generation_enabled = false;
};

struct CandidateArtifactDraftReviewReport {
    std::vector<CandidateArtifactDraftReview> reviews;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(CandidateArtifactDraftReviewDecision decision);

} // namespace archive
