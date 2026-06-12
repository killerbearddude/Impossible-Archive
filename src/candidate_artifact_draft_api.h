#pragma once
#include "archive_engine_state.h"
#include "candidate_artifact_draft_model.h"

namespace archive {

[[nodiscard]] CandidateArtifactDraft derive_candidate_artifact_draft(
    const ArchiveEngineState& state,
    const CandidateArtifactProposal& proposal,
    const CandidateArtifactProposalAudit* audit,
    AccessLevel access
);

[[nodiscard]] CandidateArtifactDraftReport derive_candidate_artifact_drafts(
    const ArchiveEngineState& state,
    AccessLevel access
);

void derive_candidate_artifact_drafts_into_state(ArchiveEngineState& state, AccessLevel access);

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_drafts(const ArchiveEngineState& state);

[[nodiscard]] bool candidate_artifact_draft_visible_to(const CandidateArtifactDraft& draft, AccessLevel access);

[[nodiscard]] std::string format_candidate_artifact_draft_summary(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_validation(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_list(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& draft_id
);

} // namespace archive
