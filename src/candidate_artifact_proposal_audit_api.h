#pragma once
#include "archive_engine_state.h"
#include "candidate_artifact_proposal_audit_model.h"

namespace archive {

[[nodiscard]] CandidateArtifactProposalAudit audit_candidate_artifact_proposal(
    const ArchiveEngineState& state,
    const CandidateArtifactProposal& proposal,
    AccessLevel access
);

[[nodiscard]] CandidateArtifactProposalAuditDecision classify_candidate_artifact_proposal_audit(
    const CandidateArtifactProposalAudit& audit
);

[[nodiscard]] CandidateArtifactProposalAuditReport audit_candidate_artifact_proposals(
    const ArchiveEngineState& state,
    AccessLevel access
);

void audit_candidate_artifact_proposals_into_state(ArchiveEngineState& state, AccessLevel access);

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_proposal_audits(const ArchiveEngineState& state);

[[nodiscard]] std::string format_candidate_artifact_proposal_audit_summary(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_proposal_audit_validation(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_proposal_audit_list(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_proposal_audit_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& audit_id
);

} // namespace archive
