#pragma once
#include "archive_engine_state.h"
#include "candidate_artifact_draft_review_model.h"

namespace archive {

[[nodiscard]] CandidateArtifactDraftReview review_candidate_artifact_draft(
    const ArchiveEngineState& state,
    const CandidateArtifactDraft& draft,
    AccessLevel access
);

[[nodiscard]] CandidateArtifactDraftReviewReport review_candidate_artifact_drafts(
    const ArchiveEngineState& state,
    AccessLevel access
);

void review_candidate_artifact_drafts_into_state(ArchiveEngineState& state, AccessLevel access);

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_draft_reviews(const ArchiveEngineState& state);

[[nodiscard]] std::string format_candidate_artifact_draft_review_summary(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_review_validation(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_review_list(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_review_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& review_id
);

} // namespace archive
