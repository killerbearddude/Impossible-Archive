#pragma once

#include "archive_common.h"

#include <string_view>

namespace archive {

// Centralized policy helpers for diagnostic/detail formatting surfaces.
// These helpers preserve the current v28 behavior while moving access
// decisions out of individual formatters before richer v29 detail surfaces exist.
enum class DiagnosticDetailSurface {
    KnowledgeHorizonFinding,
    ContradictionBudgetBucket,
    ContradictionBudgetPolicy,
    CandidateArtifactPlan,
    CandidateArtifactPlanEvaluation,
    CandidateArtifactProposal,
    CandidateArtifactProposalAudit,
    ValidationErrors,
};

[[nodiscard]] inline bool can_view_diagnostic_detail(AccessLevel access, DiagnosticDetailSurface surface) {
    (void)surface;
    return can_view(access, AccessLevel::Curator);
}

[[nodiscard]] inline bool can_view_contradiction_budget_public_bucket_summary(AccessLevel access,
                                                                              std::string_view bucket_id) {
    return !can_view_diagnostic_detail(access, DiagnosticDetailSurface::ContradictionBudgetBucket) &&
           bucket_id == "contradiction_budget.archive";
}

} // namespace archive
