#!/usr/bin/env python3
"""Apply behavior-preserving candidate artifact diagnostic access migration."""
from pathlib import Path

REPLACEMENTS = {
    "src/candidate_artifact_plan.cpp": [
        ('#include "candidate_artifact_plan_api.h"\n#include "contradiction_budget_api.h"', '#include "candidate_artifact_plan_api.h"\n#include "contradiction_budget_api.h"\n#include "diagnostic_access_policy.h"'),
        ('if (can_view(access, AccessLevel::Curator)) {\n        return true;\n    }\n    return plan.public_safe', 'if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlan)) {\n        return true;\n    }\n    return plan.public_safe'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        out << "- details: aggregate-only at this access level; hidden source IDs, diagnostic IDs, rationale, warnings, and protected mystery details are restricted.\\n";\n    }', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlan)) {\n        out << "- details: aggregate-only at this access level; hidden source IDs, diagnostic IDs, rationale, warnings, and protected mystery details are restricted.\\n";\n    }'),
        ('if (can_view(access, AccessLevel::Curator)) {\n            out << "Validation errors:', 'if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) {\n            out << "Validation errors:'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        std::size_t visible = 0;', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlan)) {\n        std::size_t visible = 0;'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        out << "- planned_shape:', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlan)) {\n        out << "- planned_shape:'),
    ],
    "src/candidate_artifact_plan_evaluation.cpp": [
        ('#include "candidate_artifact_plan_api.h"\n#include "contradiction_budget_api.h"', '#include "candidate_artifact_plan_api.h"\n#include "contradiction_budget_api.h"\n#include "diagnostic_access_policy.h"'),
        ('if (can_view(access, AccessLevel::Curator)) {\n        return true;\n    }\n    const CandidateArtifactPlan* plan', 'if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlanEvaluation)) {\n        return true;\n    }\n    const CandidateArtifactPlan* plan'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        out << "- details: aggregate-only at this access level; hidden plan IDs, KnowledgeHorizon IDs, ContradictionBudget IDs, protected mystery details, hidden rationale, and curator-only findings are restricted.\\n";\n    }', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlanEvaluation)) {\n        out << "- details: aggregate-only at this access level; hidden plan IDs, KnowledgeHorizon IDs, ContradictionBudget IDs, protected mystery details, hidden rationale, and curator-only findings are restricted.\\n";\n    }'),
        ('if (can_view(access, AccessLevel::Curator)) {\n            out << "Validation errors:', 'if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) {\n            out << "Validation errors:'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        std::size_t visible = 0;', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlanEvaluation)) {\n        std::size_t visible = 0;'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        out << "- decision:', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactPlanEvaluation)) {\n        out << "- decision:'),
    ],
    "src/candidate_artifact_proposal.cpp": [
        ('#include "candidate_artifact_plan_evaluation_api.h"\n', '#include "candidate_artifact_plan_evaluation_api.h"\n#include "diagnostic_access_policy.h"\n'),
        ('[[nodiscard]] bool candidate_artifact_proposal_visible_to(const CandidateArtifactProposal& proposal, AccessLevel access) {\n    if (can_view(access, AccessLevel::Curator)) {\n        return true;\n    }', '[[nodiscard]] bool candidate_artifact_proposal_visible_to(const CandidateArtifactProposal& proposal, AccessLevel access) {\n    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposal)) {\n        return true;\n    }'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        out << "- details: aggregate-only at this access level; hidden source IDs, diagnostic IDs, protected mystery details, hidden rationale, blocking internals, and curator-only notes are restricted.\\n";\n    }', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposal)) {\n        out << "- details: aggregate-only at this access level; hidden source IDs, diagnostic IDs, protected mystery details, hidden rationale, blocking internals, and curator-only notes are restricted.\\n";\n    }'),
        ('if (can_view(access, AccessLevel::Curator)) {\n            out << "Validation errors:', 'if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) {\n            out << "Validation errors:'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        std::size_t visible = 0;', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposal)) {\n        std::size_t visible = 0;'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        out << "- decision:', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposal)) {\n        out << "- decision:'),
    ],
    "src/candidate_artifact_proposal_audit.cpp": [
        ('#include "candidate_artifact_proposal_api.h"\n', '#include "candidate_artifact_proposal_api.h"\n#include "diagnostic_access_policy.h"\n'),
        ('[[nodiscard]] bool audit_visible_to(const ArchiveEngineState& state, const CandidateArtifactProposalAudit& audit, AccessLevel access) {\n    if (can_view(access, AccessLevel::Curator)) {\n        return true;\n    }', '[[nodiscard]] bool audit_visible_to(const ArchiveEngineState& state, const CandidateArtifactProposalAudit& audit, AccessLevel access) {\n    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposalAudit)) {\n        return true;\n    }'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        out << "- details: aggregate-only at this access level; hidden proposal IDs, source IDs, KnowledgeHorizon diagnostics, ContradictionBudget diagnostics, protected mystery details, hidden rationale, curator-only findings, and privileged required revisions are restricted.\\n";\n    }', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposalAudit)) {\n        out << "- details: aggregate-only at this access level; hidden proposal IDs, source IDs, KnowledgeHorizon diagnostics, ContradictionBudget diagnostics, protected mystery details, hidden rationale, curator-only findings, and privileged required revisions are restricted.\\n";\n    }'),
        ('if (can_view(access, AccessLevel::Curator)) {\n            out << "Validation errors:', 'if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) {\n            out << "Validation errors:'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        std::size_t visible = 0;', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposalAudit)) {\n        std::size_t visible = 0;'),
        ('if (!can_view(access, AccessLevel::Curator)) {\n        out << "- decision:', 'if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactProposalAudit)) {\n        out << "- decision:'),
    ],
}

for path_text, replacements in REPLACEMENTS.items():
    path = Path(path_text)
    text = path.read_text()
    for old, new in replacements:
        if old not in text:
            raise SystemExit(f"expected text not found in {path}: {old[:80]!r}")
        text = text.replace(old, new, 1)
    path.write_text(text)
    print(f"updated {path}")
