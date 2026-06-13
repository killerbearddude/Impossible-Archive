#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise SystemExit(f"expected text not found in {path}: {old[:160]!r}")
    p.write_text(text.replace(old, new, 1))


def replace_all(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise SystemExit(f"expected text not found in {path}: {old[:160]!r}")
    p.write_text(text.replace(old, new))

# FUTURE_VERSIONS.md
replace_once(
    "FUTURE_VERSIONS.md",
    "```text\nv28.11 — ContradictionBudget Validator-Backed Status Tightening\n```\n\nThe current engine is in v28 control-layer consolidation. It has a deterministic, access-aware, read-only/advisory chain from hidden truth through EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, and ControlLayerAudit.\n\nThe current baseline still does not include artifact text generation, discovery expansion, proposal materialization, EvidencePotential-to-artifact conversion, persistence, resolver/composition behavior, or interactive runtime behavior.\n",
    "```text\nv29.0 — CandidateArtifactDraft / Text Outline, No Artifact Insertion\n```\n\nThe current engine has a deterministic, access-aware, read-only/advisory chain from hidden truth through EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, CandidateArtifactDraft, and ControlLayerAudit.\n\nThe current baseline includes draft-outline records derived from proposal/audit chains, with CLI inspection, validation, snapshot counters, summary-digest material, snapshot comparison fields, and smoke coverage. It still does not include artifact text generation, Artifact insertion, PublicClaim insertion, discovery scheduling/expansion, proposal materialization, EvidencePotential-to-artifact conversion, persistence, resolver/composition behavior, or interactive runtime behavior.\n",
)
replace_once(
    "FUTURE_VERSIONS.md",
    "### v28.11 — ContradictionBudget Validator-Backed Status Tightening, No Mutation\n\nImplemented explicit ContradictionBudgetPolicy thresholds, deterministic reason codes, too-clean archive detection, productive ambiguity classification, generation-bug pressure classification, and stricter validation while keeping the layer advisory-only and non-mutating.\n\n## Recommended next slices\n",
    "### v28.11 — ContradictionBudget Validator-Backed Status Tightening, No Mutation\n\nImplemented explicit ContradictionBudgetPolicy thresholds, deterministic reason codes, too-clean archive detection, productive ambiguity classification, generation-bug pressure classification, and stricter validation while keeping the layer advisory-only and non-mutating.\n\n### v28.12–v28.14 — Roadmap Reconciliation, CI/Self-Test Hardening, and Diagnostic Access Centralization\n\nImplemented repository hardening needed before richer draft surfaces: roadmap reconciliation, CI/smoke coverage hardening, self-test include-boundary cleanup, and centralized diagnostic detail access helpers for KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, and ControlLayerAudit.\n\n### v29.0 — CandidateArtifactDraft / Text Outline, No Artifact Insertion\n\nImplemented a non-mutating CandidateArtifactDraft outline layer derived from CandidateArtifactProposal and CandidateArtifactProposalAudit chains. Drafts store outline title, intended artifact type/register, claim-outline lines, required validation gates, public-safe summaries, and curator diagnostics. The layer participates in ArchiveEngineState, CLI inspection, full-state validation, ArchiveSnapshot counters, summary digest material, snapshot comparison fields, and README smoke coverage.\n\nThe implementation keeps all insertion and mutation flags false: no Artifact insertion, no PublicClaim insertion, no discovery scheduling, no hidden truth mutation, no PublicArchive mutation, no persistence, no resolver/composition behavior, and no final artifact prose generation.\n\n## Recommended next slices\n",
)
replace_once(
    "FUTURE_VERSIONS.md",
    "### v28.12 — Documentation / Roadmap Reconciliation and Release-Gate Parity Prep\n\nPurpose: reconcile stale roadmap/audit documentation with the v28.11 baseline and prepare the repository for the next feature-shaped slice.\n\nAllowed work:\n\n```text\nUpdate docs that still describe v28.9/v28.10 as current or future.\nClarify that v28.11 is current.\nClarify that v29.0 is the next feature-shaped target.\nRecord CI parity and self-test structure as follow-up hardening work.\nDo not change runtime behavior.\n```\n\n### v28.13 — CI Parity / Test-Structure Hardening, No Runtime Behavior Change\n\nPurpose: bring GitHub Actions closer to the local release gate and make the monolithic self-test suite easier to maintain.\n\nAllowed work:\n\n```text\nAdd CI coverage for smoke and sanitizer paths where practical.\nSplit self-tests by subsystem without removing assertions.\nKeep CLI/runtime behavior unchanged.\n```\n\n### v28.14 — Centralized Diagnostic Detail Access Gates, No Output Expansion\n\nPurpose: reduce hidden/diagnostic leak risk before v29 adds richer draft/detail surfaces.\n\nAllowed work:\n\n```text\nIntroduce small access-policy helpers for diagnostic detail surfaces.\nMigrate KnowledgeHorizon, ContradictionBudget, CandidateArtifactProposalAudit, and ControlLayerAudit detail gates incrementally.\nKeep public/scholar output at least as restrictive as today.\n```\n\n### v29.0 — CandidateArtifactDraft / Text Outline, No Artifact Insertion\n\nPurpose: introduce a draft-outline layer derived from audited CandidateArtifactProposal records.\n\nAllowed work:\n\n```text\nCreate CandidateArtifactDraft records from valid proposal/audit chains.\nStore outline titles, intended artifact type/register, claim-outline lines, and required validation gates.\nExpose public-safe summaries and curator/debug diagnostics.\nAdd validation, snapshot counts, smoke coverage, and self-tests.\n```\n\nNon-goals:\n\n```text\nNo Artifact insertion.\nNo PublicClaim insertion.\nNo discovery scheduling.\nNo hidden truth mutation.\nNo public archive mutation.\nNo persistence.\nNo resolver/composition behavior.\nNo final artifact prose generation.\n```\n",
    "### v29.1 — CandidateArtifactDraft Review/Audit Policy, No Artifact Insertion\n\nPurpose: add an audit/review layer over CandidateArtifactDraft records without converting drafts into generated artifacts or public archive mutations.\n\nAllowed work:\n\n```text\nCreate deterministic draft review/audit records from CandidateArtifactDraft records.\nScore outline completeness, proposal/audit traceability, safety, specificity, and remaining revision pressure.\nRequire actionable revisions for non-pass draft reviews.\nAdd validation, snapshot counters, digest material, smoke coverage, and public-detail blocking checks.\nKeep draft records and review records advisory-only.\n```\n\nNon-goals:\n\n```text\nNo Artifact insertion.\nNo PublicClaim insertion.\nNo discovery scheduling.\nNo hidden truth mutation.\nNo public archive mutation.\nNo persistence.\nNo resolver/composition behavior.\nNo final artifact prose generation.\n```\n\n### v29.2 — Persistent Runtime Session Planning, No File/Database Persistence Yet\n\nPurpose: design the smallest persistent-runtime interface before introducing storage. The first useful target is an in-memory session that can initialize once, run multiple queries against the same state, and end explicitly.\n\nAllowed work:\n\n```text\nDocument Initialize -> Run Query -> Run Query -> End Session flow.\nIdentify the minimal session state wrapper around ArchiveEngineState.\nKeep storage in memory only.\nKeep CLI single-shot behavior unchanged unless an explicit session mode is selected.\n```\n\nNon-goals:\n\n```text\nNo JSON/database persistence.\nNo GUI/API layer.\nNo background worker.\nNo multi-user server.\nNo behavior change for existing CLI queries.\n```\n",
)

# CURRENT_STATE_AUDIT.md
replace_once("docs/CURRENT_STATE_AUDIT.md", "# Current State Audit — v28.11 Control-Layer Consolidation\n", "# Current State Audit — v29.0 CandidateArtifactDraft Outline Baseline\n")
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "The engine is not release-ready. It is in v28 control-layer consolidation with a v28.11 baseline. Artifact generation, artifact text generation, discovery expansion, proposal materialization, EvidencePotential-to-artifact conversion, resolver/composition behavior, persistence, and interactive runtime behavior remain deferred.\n\nv28.11 extends the v28 control stack with validator-backed `ContradictionBudget` policy/status tightening. The current control layers are intended to make the engine inspectable, deterministic, access-aware, and safe to extend without accidentally turning advisory records into archive mutation.\n",
    "The engine is not release-ready. It is at the v29.0 CandidateArtifactDraft outline baseline. Artifact generation, artifact text generation, Artifact insertion, PublicClaim insertion, discovery expansion/scheduling, proposal materialization, EvidencePotential-to-artifact conversion, resolver/composition behavior, persistence, and interactive runtime behavior remain deferred.\n\nv29.0 extends the v28 control stack with non-mutating `CandidateArtifactDraft` outline records derived from proposal/audit chains. The current control layers are intended to make the engine inspectable, deterministic, access-aware, and safe to extend without accidentally turning advisory records into archive mutation.\n",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "v28.11    ContradictionBudget validator-backed status tightening\n```",
    "v28.11    ContradictionBudget validator-backed status tightening\nv28.12    Documentation / roadmap reconciliation and release-gate parity prep\nv28.13    CI parity and self-test structure hardening\nv28.14    Centralized diagnostic detail access gates\nv29.0     CandidateArtifactDraft outline layer, snapshot coverage, and smoke coverage\n```",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "CandidateArtifactProposalAudit\nCandidateGeneration",
    "CandidateArtifactProposalAudit\nCandidateArtifactDraft\nCandidateGeneration",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "| CandidateArtifactProposalAudit | Audit-only quality gate with v28.10 policy thresholds and reason-coded findings. |\n| ControlLayerAudit | Audit-only inventory of the control stack. |",
    "| CandidateArtifactProposalAudit | Audit-only quality gate with v28.10 policy thresholds and reason-coded findings. |\n| CandidateArtifactDraft | Draft-outline-only records derived from proposal/audit chains. No artifact text generation, Artifact insertion, PublicClaim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, or resolver/composition behavior. |\n| ControlLayerAudit | Audit-only inventory of the control stack. |",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "CandidateArtifactProposalAudit\nControlLayerAudit",
    "CandidateArtifactProposalAudit\nCandidateArtifactDraft\nControlLayerAudit",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "## 8. Snapshot/digest coverage\n\n`ArchiveSnapshot` includes summary counts for v28 control layers through `CandidateArtifactProposalAudit`, `ControlLayerAudit`, and v28.11 `ContradictionBudget` reason/status counts. The `summary_digest` is a deterministic regression tripwire, not a persistence format and not a full semantic state digest.\n",
    "## 8. Snapshot/digest coverage\n\n`ArchiveSnapshot` includes summary counts for control layers through `CandidateArtifactDraft`, `ControlLayerAudit`, and v28.11 `ContradictionBudget` reason/status counts. CandidateArtifactDraft contributes counters, mutation-enabled count checks, deterministic digest material, replay lines, comparison fields, and count deltas. The `summary_digest` is a deterministic regression tripwire, not a persistence format and not a full semantic state digest.\n",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "The smoke workflow covers representative runtime selection, specs, fragments, fixtures, snapshots, EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, and ControlLayerAudit query surfaces, including public-detail blocking checks and expected failures.\n",
    "The smoke workflow covers representative runtime selection, specs, fragments, fixtures, snapshots, EvidencePotential, KnowledgeHorizon, ContradictionBudget, CandidateArtifactPlan, CandidateArtifactPlanEvaluation, CandidateArtifactProposal, CandidateArtifactProposalAudit, CandidateArtifactDraft, and ControlLayerAudit query surfaces, including public-detail blocking checks and expected failures.\n",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "No artifact text generation from proposal/audit records.\nNo discovery expansion from the v28 control chain.",
    "No artifact text generation from proposal/audit/draft records.\nNo discovery expansion from the control chain.",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "CandidateArtifactProposalAudit policy/revision outputs\n```\n\nThe safest immediate direction is still non-mutating hardening: documentation reconciliation, CI parity, self-test structure cleanup, and centralized diagnostic access helper work.\n",
    "CandidateArtifactProposalAudit policy/revision outputs\nCandidateArtifactDraft outline records and non-mutation invariants\n```\n\nThe safest immediate direction is still non-mutating hardening: documentation reconciliation, CandidateArtifactDraft review/audit policy, and persistent runtime session planning without storage.\n",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "Do not build artifact generation, discovery expansion, or proposal materialization directly on `CandidateArtifactProposal` or `CandidateArtifactProposalAudit` yet. The next artifact-facing slice should remain a draft/outline layer only, with explicit no-insertion and no-discovery invariants.\n",
    "Do not build artifact generation, discovery expansion, or proposal materialization directly on `CandidateArtifactProposal`, `CandidateArtifactProposalAudit`, or `CandidateArtifactDraft` yet. The next artifact-facing slice should remain an audit/review layer over drafts, with explicit no-insertion and no-discovery invariants.\n",
)
replace_once(
    "docs/CURRENT_STATE_AUDIT.md",
    "Recommended immediate maintenance slice:\n\n```text\nv28.12 — Documentation / roadmap reconciliation, CI parity planning, and test-structure hardening prep\n```\n\nRecommended next feature-shaped slice after maintenance:\n\n```text\nv29.0 — CandidateArtifactDraft / Text Outline, No Artifact Insertion\n```",
    "Recommended immediate maintenance slice:\n\n```text\nv29.0 documentation reconciliation for the landed CandidateArtifactDraft outline baseline\n```\n\nRecommended next feature-shaped slice:\n\n```text\nv29.1 — CandidateArtifactDraft Review/Audit Policy, No Artifact Insertion\n```\n\nRecommended runtime-planning slice after draft-review policy:\n\n```text\nv29.2 — Persistent Runtime Session Planning, In-Memory First\n```",
)

# ARCHITECTURE_MAP.md
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "This is a practical map of the current v28.11 runtime and control-layer code paths. It describes the implemented state of the repository after ContradictionBudget validator-backed status tightening. It does not describe a composition resolver, persistent runtime, GUI/API layer, or discovery expansion because those do not exist yet.\n",
    "This is a practical map of the current v29.0 runtime and control-layer code paths. It describes the implemented repository state after the CandidateArtifactDraft outline layer and its snapshot/smoke coverage. It does not describe a composition resolver, persistent runtime, GUI/API layer, artifact text generation, Artifact insertion, PublicClaim insertion, or discovery expansion because those do not exist yet.\n",
)
replace_all("docs/ARCHITECTURE_MAP.md", "derive v28 control-layer records", "derive current control-layer and draft-outline records")
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "candidate_artifact_proposal_audits\ncontrol_layer_audit_entries",
    "candidate_artifact_proposal_audits\ncandidate_artifact_drafts\ncontrol_layer_audit_entries",
)
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "Known architectural gap: some access behavior remains distributed across formatting/query code rather than centralized in a policy engine. This should be tightened before v29 detail surfaces expand.\n",
    "Diagnostic detail access for the tracked v28/v29 outline surfaces is routed through centralized helper surfaces where migrated. Public/scholar output for hidden or diagnostic details remains restricted or `found: false`; curator/canon/debug can inspect internal traces where the workflow allows it.\n",
)
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "## 23. ControlLayerAudit\n",
    "## 23. CandidateArtifactDraft outline seam\n\nPrimary files:\n\n- `src/candidate_artifact_draft_model.h`\n- `src/candidate_artifact_draft_api.h`\n- `src/candidate_artifact_draft.cpp`\n- `src/archive_snapshot.cpp`\n- `src/cli.cpp`\n\nFlow:\n\n```text\nCandidateArtifactProposal\n+ CandidateArtifactProposalAudit\n-> CandidateArtifactDraft\n-> validate / inspect / snapshot only\n```\n\nCandidateArtifactDraft is a non-mutating outline layer. It stores outline title, intended artifact type/register, claim-outline lines, required validation gates, public-safe summaries, source-chain diagnostics, and curator notes. All mutation flags remain false: no Artifact insertion, no PublicClaim insertion, no discovery scheduling, no hidden truth mutation, no PublicArchive mutation, no persistence, no resolver/composition behavior, and no final artifact prose generation.\n\nPublic/scholar access receives aggregate or public-safe summaries only. Curator/debug access can inspect proposal/audit IDs, source chains, validation gates, and diagnostic notes.\n\n## 24. ControlLayerAudit\n",
)
replace_once("docs/ARCHITECTURE_MAP.md", "## 24. Test and release surfaces\n", "## 25. Test and release surfaces\n")
replace_once("docs/ARCHITECTURE_MAP.md", "## 25. Explicit non-goals in the current architecture\n", "## 26. Explicit non-goals in the current architecture\n")
replace_once(
    "docs/ARCHITECTURE_MAP.md",
    "No artifact text generation from proposal/audit records.\nNo EvidencePotential-to-artifact conversion.",
    "No artifact text generation from proposal/audit/draft records.\nNo Artifact insertion or PublicClaim insertion from CandidateArtifactDraft records.\nNo discovery scheduling from CandidateArtifactDraft records.\nNo EvidencePotential-to-artifact conversion.",
)

print("Reconciled v29.0 CandidateArtifactDraft documentation baseline.")
