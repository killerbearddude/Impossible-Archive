#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise SystemExit(f"expected text not found in {path}: {old[:120]!r}")
    p.write_text(text.replace(old, new, 1))


# FUTURE_VERSIONS.md
replace_once(
    "FUTURE_VERSIONS.md",
    "This file records version history and recommended future slices after the current v28.11 baseline. It is a roadmap, not active runtime behavior.",
    "This file records version history and recommended future slices after the current v29.1 baseline. It is a roadmap, not active runtime behavior.",
)
replace_once(
    "FUTURE_VERSIONS.md",
    "v29.0 — CandidateArtifactDraft / Text Outline, No Artifact Insertion",
    "v29.1 — CandidateArtifactDraftReview / Review Policy, No Artifact Insertion",
)
replace_once(
    "FUTURE_VERSIONS.md",
    "CandidateArtifactProposalAudit, CandidateArtifactDraft, and ControlLayerAudit.",
    "CandidateArtifactProposalAudit, CandidateArtifactDraft, CandidateArtifactDraftReview, and ControlLayerAudit.",
)
replace_once(
    "FUTURE_VERSIONS.md",
    "The current baseline includes draft-outline records derived from proposal/audit chains, with CLI inspection, validation, snapshot counters, summary-digest material, snapshot comparison fields, and smoke coverage. It still does not include artifact text generation, Artifact insertion, PublicClaim insertion, discovery scheduling/expansion, proposal materialization, EvidencePotential-to-artifact conversion, persistence, resolver/composition behavior, or interactive runtime behavior.",
    "The current baseline includes draft-outline records and draft-review records derived from proposal/audit/draft chains, with CLI inspection, validation, snapshot counters, summary-digest material, snapshot comparison fields, and smoke coverage. It still does not include artifact text generation, Artifact insertion, PublicClaim insertion, discovery scheduling/expansion, proposal materialization, EvidencePotential-to-artifact conversion, persistence, resolver/composition behavior, or interactive runtime behavior.",
)
replace_once(
    "FUTURE_VERSIONS.md",
    "## Recommended next slices\n\n### v29.1 — CandidateArtifactDraft Review/Audit Policy, No Artifact Insertion\n\nPurpose: add an audit/review layer over CandidateArtifactDraft records without converting drafts into generated artifacts or public archive mutations.",
    "### v29.1 — CandidateArtifactDraft Review/Audit Policy, No Artifact Insertion\n\nImplemented a non-mutating CandidateArtifactDraftReview policy layer over CandidateArtifactDraft records. Reviews score outline completeness, proposal/audit traceability, safety, specificity, and revision pressure, require actionable revisions for non-pass reviews, and participate in ArchiveEngineState, CLI inspection, full-state validation, ArchiveSnapshot counters, summary digest material, snapshot comparison fields, and README smoke coverage.\n\nThe implementation keeps the layer advisory-only: review pass does not authorize artifact generation, artifact insertion, public claim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, resolver/composition behavior, or final artifact prose generation.\n\n## Recommended next slices\n\n### v29.2 — Persistent Runtime Session Planning, No File/Database Persistence Yet\n\nPurpose: design the smallest persistent-runtime interface before introducing storage. The first useful target is an in-memory session that can initialize once, run multiple queries against the same state, and end explicitly.",
)
replace_once(
    "FUTURE_VERSIONS.md",
    "### v29.2 — Persistent Runtime Session Planning, No File/Database Persistence Yet\n\nPurpose: design the smallest persistent-runtime interface before introducing storage. The first useful target is an in-memory session that can initialize once, run multiple queries against the same state, and end explicitly.\n\nAllowed work:",
    "Allowed work:",
)

# docs/CURRENT_STATE_AUDIT.md
replace_once("docs/CURRENT_STATE_AUDIT.md", "# Current State Audit — v29.0 CandidateArtifactDraft Outline Baseline", "# Current State Audit — v29.1 CandidateArtifactDraftReview Baseline")
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "The engine is not release-ready. It is at the v29.0 CandidateArtifactDraft outline baseline.",
    "The engine is not release-ready. It is at the v29.1 CandidateArtifactDraftReview baseline.",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "v29.0 extends the v28 control stack with non-mutating `CandidateArtifactDraft` outline records derived from proposal/audit chains.",
    "v29.1 extends the v29.0 draft-outline stack with non-mutating `CandidateArtifactDraftReview` records derived from draft records.",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "v29.0     CandidateArtifactDraft outline layer, snapshot coverage, and smoke coverage",
    "v29.0     CandidateArtifactDraft outline layer, snapshot coverage, and smoke coverage\nv29.1     CandidateArtifactDraftReview policy layer, snapshot coverage, and smoke coverage",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "CandidateArtifactDraft\nCandidateGeneration",
    "CandidateArtifactDraft\nCandidateArtifactDraftReview\nCandidateGeneration",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "| CandidateArtifactDraft | Draft-outline-only records derived from proposal/audit chains. No artifact text generation, Artifact insertion, PublicClaim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, or resolver/composition behavior. |\n| ControlLayerAudit | Audit-only inventory of the control stack. |",
    "| CandidateArtifactDraft | Draft-outline-only records derived from proposal/audit chains. |\n| CandidateArtifactDraftReview | Review-policy-only records derived from CandidateArtifactDraft records. Scores outline completeness, traceability, safety, specificity, and revision pressure. No artifact text generation, Artifact insertion, PublicClaim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, or resolver/composition behavior. |\n| ControlLayerAudit | Audit-only inventory of the control stack. |",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "CandidateArtifactDraft\nControlLayerAudit",
    "CandidateArtifactDraft\nCandidateArtifactDraftReview\nControlLayerAudit",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "`ArchiveSnapshot` includes summary counts for control layers through `CandidateArtifactDraft`, `ControlLayerAudit`, and v28.11 `ContradictionBudget` reason/status counts. CandidateArtifactDraft contributes counters, mutation-enabled count checks, deterministic digest material, replay lines, comparison fields, and count deltas.",
    "`ArchiveSnapshot` includes summary counts for control layers through `CandidateArtifactDraftReview`, `ControlLayerAudit`, and v28.11 `ContradictionBudget` reason/status counts. CandidateArtifactDraft and CandidateArtifactDraftReview contribute counters, mutation/generation-enabled count checks, deterministic digest material, replay lines, comparison fields, and count deltas.",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "CandidateArtifactProposalAudit, CandidateArtifactDraft, and ControlLayerAudit query surfaces",
    "CandidateArtifactProposalAudit, CandidateArtifactDraft, CandidateArtifactDraftReview, and ControlLayerAudit query surfaces",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "CandidateArtifactDraft outline records and non-mutation invariants",
    "CandidateArtifactDraft outline records and non-mutation invariants\nCandidateArtifactDraftReview policy records and non-generation invariants",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "Do not build artifact generation, discovery expansion, or proposal materialization directly on `CandidateArtifactProposal`, `CandidateArtifactProposalAudit`, or `CandidateArtifactDraft` yet. The next artifact-facing slice should remain an audit/review layer over drafts, with explicit no-insertion and no-discovery invariants.",
    "Do not build artifact generation, discovery expansion, or proposal materialization directly on `CandidateArtifactProposal`, `CandidateArtifactProposalAudit`, `CandidateArtifactDraft`, or `CandidateArtifactDraftReview` yet. The next slice should be persistent runtime session planning, in-memory first, with no file/database persistence and no behavior change for existing single-shot CLI queries.",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "v29.0 documentation reconciliation for the landed CandidateArtifactDraft outline baseline",
    "v29.1 documentation reconciliation for the landed CandidateArtifactDraftReview baseline",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "Recommended next feature-shaped slice:\n\n```text\nv29.1 — CandidateArtifactDraft Review/Audit Policy, No Artifact Insertion\n```\n\nRecommended runtime-planning slice after draft-review policy:",
    "Recommended next feature-shaped slice:",
)

# docs/ARCHITECTURE_MAP.md
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "current v29.0 runtime and control-layer code paths. It describes the implemented repository state after the CandidateArtifactDraft outline layer and its snapshot/smoke coverage.",
    "current v29.1 runtime and control-layer code paths. It describes the implemented repository state after the CandidateArtifactDraftReview policy layer and its snapshot/smoke coverage.",
)
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "derive current control-layer and draft-outline records",
    "derive current control-layer, draft-outline, and draft-review records",
)
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "derive current control-layer and draft-outline records",
    "derive current control-layer, draft-outline, and draft-review records",
)
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "candidate_artifact_drafts\ncontrol_layer_audit_entries",
    "candidate_artifact_drafts\ncandidate_artifact_draft_reviews\ncontrol_layer_audit_entries",
)
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "## 24. ControlLayerAudit",
    "## 24. CandidateArtifactDraftReview policy seam\n\nPrimary files:\n\n- `src/candidate_artifact_draft_review_model.h`\n- `src/candidate_artifact_draft_review_api.h`\n- `src/candidate_artifact_draft_review.cpp`\n- `src/archive_snapshot.cpp`\n- `src/cli.cpp`\n\nFlow:\n\n```text\nCandidateArtifactDraft\n-> CandidateArtifactDraftReview\n-> validate / inspect / snapshot only\n```\n\nCandidateArtifactDraftReview is a non-mutating review-policy layer. It stores deterministic review decisions, outline completeness, traceability, safety, specificity, revision-pressure scores, reason codes, required revisions, public-safe summaries, and curator notes. A review pass means advisory review-clean only; it does not enable artifact generation, artifact insertion, public claim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, resolver/composition behavior, or final artifact prose generation.\n\nPublic/scholar access receives aggregate or public-safe summaries only. Curator/debug access can inspect draft/proposal/audit IDs, scores, reason codes, and required revisions.\n\n## 25. ControlLayerAudit",
)
replace_once("docs/ARCHITECTURE_MAP.md", "## 25. Test and release surfaces", "## 26. Test and release surfaces")
replace_once("docs/ARCHITECTURE_MAP.md", "## 26. Explicit non-goals in the current architecture", "## 27. Explicit non-goals in the current architecture")
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "No artifact text generation from proposal/audit/draft records.\nNo Artifact insertion or PublicClaim insertion from CandidateArtifactDraft records.\nNo discovery scheduling from CandidateArtifactDraft records.",
    "No artifact text generation from proposal/audit/draft/review records.\nNo Artifact insertion or PublicClaim insertion from CandidateArtifactDraft or CandidateArtifactDraftReview records.\nNo discovery scheduling from CandidateArtifactDraft or CandidateArtifactDraftReview records.",
)

print("Reconciled docs for v29.1 CandidateArtifactDraftReview baseline.")
