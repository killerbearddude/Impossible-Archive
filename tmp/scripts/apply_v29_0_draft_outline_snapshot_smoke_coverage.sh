#!/usr/bin/env bash
set -euo pipefail

# Temporary local patch helper for v29.0 CandidateArtifactDraft snapshot/smoke coverage.
# Run from repository root on branch v29-0-draft-outline-snapshot-smoke-coverage.
# This script intentionally changes only snapshot and smoke-test coverage.

python3 - <<'PY'
from pathlib import Path


def read(path: str) -> str:
    return Path(path).read_text()


def write(path: str, data: str) -> None:
    Path(path).write_text(data)


def replace_once(path: str, old: str, new: str) -> None:
    data = read(path)
    if new in data:
        return
    if old not in data:
        raise SystemExit(f"Expected text not found in {path}: {old[:220]!r}")
    write(path, data.replace(old, new, 1))


def insert_after(path: str, anchor: str, block: str) -> None:
    data = read(path)
    if block in data:
        return
    if anchor not in data:
        raise SystemExit(f"Anchor not found in {path}: {anchor[:220]!r}")
    write(path, data.replace(anchor, anchor + block, 1))


replace_once(
    "src/archive_snapshot_model.h",
    "    std::size_t candidate_artifact_proposal_audit_revision_count = 0;\n"
    "    std::size_t control_layer_audit_entry_count = 0;\n",
    "    std::size_t candidate_artifact_proposal_audit_revision_count = 0;\n"
    "    std::size_t candidate_artifact_draft_count = 0;\n"
    "    std::size_t candidate_artifact_draft_ready_count = 0;\n"
    "    std::size_t candidate_artifact_draft_blocked_count = 0;\n"
    "    std::size_t candidate_artifact_draft_review_count = 0;\n"
    "    std::size_t candidate_artifact_draft_revision_count = 0;\n"
    "    std::size_t candidate_artifact_draft_mutation_enabled_count = 0;\n"
    "    std::size_t control_layer_audit_entry_count = 0;\n",
)

replace_once(
    "src/archive_snapshot.cpp",
    '#include "candidate_artifact_proposal_audit_api.h"\n#include "contradiction_budget_api.h"\n',
    '#include "candidate_artifact_proposal_audit_api.h"\n#include "candidate_artifact_draft_api.h"\n#include "contradiction_budget_api.h"\n',
)

replace_once(
    "src/archive_snapshot.cpp",
    '    out << "candidate_artifact_proposal_audit_revision_count=" << audit_revision << "\\n";\n'
    '    const ControlLayerAuditReport control_report = build_control_layer_audit_report();\n',
    '    out << "candidate_artifact_proposal_audit_revision_count=" << audit_revision << "\\n";\n'
    '    const CandidateArtifactDraftReport draft_report = derive_candidate_artifact_drafts(state, AccessLevel::Curator);\n'
    '    std::size_t draft_ready = 0;\n'
    '    std::size_t draft_blocked = 0;\n'
    '    std::size_t draft_review = 0;\n'
    '    std::size_t draft_revision = 0;\n'
    '    std::size_t draft_mutation_enabled = 0;\n'
    '    for (const CandidateArtifactDraft& draft : draft_report.drafts) {\n'
    '        if (draft.status == CandidateArtifactDraftStatus::ReadyForOutline) { ++draft_ready; }\n'
    '        if (draft.status == CandidateArtifactDraftStatus::Blocked || draft.status == CandidateArtifactDraftStatus::Invalid) { ++draft_blocked; }\n'
    '        if (draft.status == CandidateArtifactDraftStatus::NeedsCuratorReview) { ++draft_review; }\n'
    '        if (draft.status == CandidateArtifactDraftStatus::NeedsRevision) { ++draft_revision; }\n'
    '        if (draft.current_artifact_insertion_enabled || draft.current_public_claim_insertion_enabled ||\n'
    '            draft.current_discovery_scheduling_enabled || draft.hidden_truth_mutation_enabled ||\n'
    '            draft.public_archive_mutation_enabled || draft.persistence_enabled) { ++draft_mutation_enabled; }\n'
    '    }\n'
    '    out << "candidate_artifact_draft_count=" << draft_report.drafts.size() << "\\n";\n'
    '    out << "candidate_artifact_draft_ready_count=" << draft_ready << "\\n";\n'
    '    out << "candidate_artifact_draft_blocked_count=" << draft_blocked << "\\n";\n'
    '    out << "candidate_artifact_draft_review_count=" << draft_review << "\\n";\n'
    '    out << "candidate_artifact_draft_revision_count=" << draft_revision << "\\n";\n'
    '    out << "candidate_artifact_draft_mutation_enabled_count=" << draft_mutation_enabled << "\\n";\n'
    '    const ControlLayerAuditReport control_report = build_control_layer_audit_report();\n',
)

replace_once(
    "src/archive_snapshot.cpp",
    '    for (const CandidateArtifactProposalAudit& audit : audit_report.audits) {\n'
    '        out << "U|" << audit.id\n',
    '    for (const CandidateArtifactDraft& draft : draft_report.drafts) {\n'
    '        out << "D|" << draft.id\n'
    '            << "|" << draft.proposal_id\n'
    '            << "|" << draft.audit_id\n'
    '            << "|" << to_string(draft.status)\n'
    '            << "|" << to_string(draft.visibility_class)\n'
    '            << "|outline_lines=" << draft.claim_outline_lines.size()\n'
    '            << "|validation_gates=" << draft.required_validation_gates.size()\n'
    '            << "|artifact_insertion_enabled=" << (draft.current_artifact_insertion_enabled ? "true" : "false")\n'
    '            << "|public_claim_insertion_enabled=" << (draft.current_public_claim_insertion_enabled ? "true" : "false")\n'
    '            << "|discovery_scheduling_enabled=" << (draft.current_discovery_scheduling_enabled ? "true" : "false")\n'
    '            << "|hidden_truth_mutation_enabled=" << (draft.hidden_truth_mutation_enabled ? "true" : "false")\n'
    '            << "|public_archive_mutation_enabled=" << (draft.public_archive_mutation_enabled ? "true" : "false")\n'
    '            << "|persistence_enabled=" << (draft.persistence_enabled ? "true" : "false")\n'
    '            << "\\n";\n'
    '    }\n'
    '    for (const CandidateArtifactProposalAudit& audit : audit_report.audits) {\n'
    '        out << "U|" << audit.id\n',
)

replace_once(
    "src/archive_snapshot.cpp",
    '    snapshot.candidate_artifact_proposal_audit_revision_count = static_cast<std::size_t>(std::count_if(\n'
    '        audit_report.audits.begin(), audit_report.audits.end(),\n'
    '        [](const CandidateArtifactProposalAudit& audit) { return audit.decision == CandidateArtifactProposalAuditDecision::NeedsRevision; }\n'
    '    ));\n'
    '    const ControlLayerAuditReport control_report = build_control_layer_audit_report();\n',
    '    snapshot.candidate_artifact_proposal_audit_revision_count = static_cast<std::size_t>(std::count_if(\n'
    '        audit_report.audits.begin(), audit_report.audits.end(),\n'
    '        [](const CandidateArtifactProposalAudit& audit) { return audit.decision == CandidateArtifactProposalAuditDecision::NeedsRevision; }\n'
    '    ));\n'
    '    const CandidateArtifactDraftReport draft_report = derive_candidate_artifact_drafts(state, AccessLevel::Curator);\n'
    '    snapshot.candidate_artifact_draft_count = draft_report.drafts.size();\n'
    '    snapshot.candidate_artifact_draft_ready_count = static_cast<std::size_t>(std::count_if(\n'
    '        draft_report.drafts.begin(), draft_report.drafts.end(),\n'
    '        [](const CandidateArtifactDraft& draft) { return draft.status == CandidateArtifactDraftStatus::ReadyForOutline; }\n'
    '    ));\n'
    '    snapshot.candidate_artifact_draft_blocked_count = static_cast<std::size_t>(std::count_if(\n'
    '        draft_report.drafts.begin(), draft_report.drafts.end(),\n'
    '        [](const CandidateArtifactDraft& draft) { return draft.status == CandidateArtifactDraftStatus::Blocked || draft.status == CandidateArtifactDraftStatus::Invalid; }\n'
    '    ));\n'
    '    snapshot.candidate_artifact_draft_review_count = static_cast<std::size_t>(std::count_if(\n'
    '        draft_report.drafts.begin(), draft_report.drafts.end(),\n'
    '        [](const CandidateArtifactDraft& draft) { return draft.status == CandidateArtifactDraftStatus::NeedsCuratorReview; }\n'
    '    ));\n'
    '    snapshot.candidate_artifact_draft_revision_count = static_cast<std::size_t>(std::count_if(\n'
    '        draft_report.drafts.begin(), draft_report.drafts.end(),\n'
    '        [](const CandidateArtifactDraft& draft) { return draft.status == CandidateArtifactDraftStatus::NeedsRevision; }\n'
    '    ));\n'
    '    snapshot.candidate_artifact_draft_mutation_enabled_count = static_cast<std::size_t>(std::count_if(\n'
    '        draft_report.drafts.begin(), draft_report.drafts.end(),\n'
    '        [](const CandidateArtifactDraft& draft) {\n'
    '            return draft.current_artifact_insertion_enabled || draft.current_public_claim_insertion_enabled ||\n'
    '                   draft.current_discovery_scheduling_enabled || draft.hidden_truth_mutation_enabled ||\n'
    '                   draft.public_archive_mutation_enabled || draft.persistence_enabled;\n'
    '        }\n'
    '    ));\n'
    '    const ControlLayerAuditReport control_report = build_control_layer_audit_report();\n',
)

replace_once(
    "src/archive_snapshot.cpp",
    '    out << "- candidate_artifact_proposal_audit_revision_count: " << snapshot.candidate_artifact_proposal_audit_revision_count << "\\n";\n'
    '    out << "- control_layer_audit_entry_count: " << snapshot.control_layer_audit_entry_count << "\\n";\n',
    '    out << "- candidate_artifact_proposal_audit_revision_count: " << snapshot.candidate_artifact_proposal_audit_revision_count << "\\n";\n'
    '    out << "- candidate_artifact_draft_count: " << snapshot.candidate_artifact_draft_count << "\\n";\n'
    '    out << "- candidate_artifact_draft_ready_count: " << snapshot.candidate_artifact_draft_ready_count << "\\n";\n'
    '    out << "- candidate_artifact_draft_blocked_count: " << snapshot.candidate_artifact_draft_blocked_count << "\\n";\n'
    '    out << "- candidate_artifact_draft_review_count: " << snapshot.candidate_artifact_draft_review_count << "\\n";\n'
    '    out << "- candidate_artifact_draft_revision_count: " << snapshot.candidate_artifact_draft_revision_count << "\\n";\n'
    '    out << "- candidate_artifact_draft_mutation_enabled_count: " << snapshot.candidate_artifact_draft_mutation_enabled_count << "\\n";\n'
    '    out << "- control_layer_audit_entry_count: " << snapshot.control_layer_audit_entry_count << "\\n";\n',
)

replace_once(
    "src/archive_snapshot.cpp",
    '                       before.candidate_artifact_proposal_audit_revision_count == after.candidate_artifact_proposal_audit_revision_count &&\n'
    '                       before.control_layer_audit_entry_count == after.control_layer_audit_entry_count &&\n',
    '                       before.candidate_artifact_proposal_audit_revision_count == after.candidate_artifact_proposal_audit_revision_count &&\n'
    '                       before.candidate_artifact_draft_count == after.candidate_artifact_draft_count &&\n'
    '                       before.candidate_artifact_draft_ready_count == after.candidate_artifact_draft_ready_count &&\n'
    '                       before.candidate_artifact_draft_blocked_count == after.candidate_artifact_draft_blocked_count &&\n'
    '                       before.candidate_artifact_draft_review_count == after.candidate_artifact_draft_review_count &&\n'
    '                       before.candidate_artifact_draft_revision_count == after.candidate_artifact_draft_revision_count &&\n'
    '                       before.candidate_artifact_draft_mutation_enabled_count == after.candidate_artifact_draft_mutation_enabled_count &&\n'
    '                       before.control_layer_audit_entry_count == after.control_layer_audit_entry_count &&\n',
)

replace_once(
    "src/archive_snapshot.cpp",
    '    format_count_delta(out, "candidate_artifact_proposal_audit_revision_count", before.candidate_artifact_proposal_audit_revision_count, after.candidate_artifact_proposal_audit_revision_count);\n'
    '    format_count_delta(out, "control_layer_audit_entry_count", before.control_layer_audit_entry_count, after.control_layer_audit_entry_count);\n',
    '    format_count_delta(out, "candidate_artifact_proposal_audit_revision_count", before.candidate_artifact_proposal_audit_revision_count, after.candidate_artifact_proposal_audit_revision_count);\n'
    '    format_count_delta(out, "candidate_artifact_draft_count", before.candidate_artifact_draft_count, after.candidate_artifact_draft_count);\n'
    '    format_count_delta(out, "candidate_artifact_draft_ready_count", before.candidate_artifact_draft_ready_count, after.candidate_artifact_draft_ready_count);\n'
    '    format_count_delta(out, "candidate_artifact_draft_blocked_count", before.candidate_artifact_draft_blocked_count, after.candidate_artifact_draft_blocked_count);\n'
    '    format_count_delta(out, "candidate_artifact_draft_review_count", before.candidate_artifact_draft_review_count, after.candidate_artifact_draft_review_count);\n'
    '    format_count_delta(out, "candidate_artifact_draft_revision_count", before.candidate_artifact_draft_revision_count, after.candidate_artifact_draft_revision_count);\n'
    '    format_count_delta(out, "candidate_artifact_draft_mutation_enabled_count", before.candidate_artifact_draft_mutation_enabled_count, after.candidate_artifact_draft_mutation_enabled_count);\n'
    '    format_count_delta(out, "control_layer_audit_entry_count", before.control_layer_audit_entry_count, after.control_layer_audit_entry_count);\n',
)

insert_after(
    "scripts/smoke_test_readme_workflows.sh",
    'run_and_grep candidate_artifact_proposal_audit_snapshot "candidate_artifact_proposal_audit_count: [1-9]" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive\n',
    '\n'
    'run_and_grep candidate_artifact_draft_summary "CandidateArtifactDraft summary|total_drafts: [1-9]|current_artifact_insertion_enabled: 0" "$BIN" --query candidate-artifact-draft-summary\n'
    'run_and_grep candidate_artifact_draft_validation "CandidateArtifactDraft validation|result: passed" "$BIN" --query validate-candidate-artifact-drafts\n'
    'run_and_grep candidate_artifact_draft_list "CandidateArtifactDrafts visible to curator|candidate_artifact_draft\\." "$BIN" --runtime fixed-fixture --access curator --query list-candidate-artifact-drafts\n'
    'run_and_grep candidate_artifact_draft_detail "CandidateArtifactDraft detail:|- found: true|proposal_id:|audit_id:|Claim outline lines:|Required validation gates:" "$BIN" --runtime fixed-fixture --access curator --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket\n'
    'run_and_reject_grep candidate_artifact_draft_public_detail_blocked "CandidateArtifactDraft detail:|- found: false" "proposal_id:|audit_id:|source_evidence_potential_id:|Source chain IDs|Claim outline lines:|Required validation gates:" "$BIN" --runtime fixed-fixture --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket\n'
    'run_and_grep candidate_artifact_draft_snapshot "candidate_artifact_draft_count: [1-9]|candidate_artifact_draft_mutation_enabled_count: 0" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive\n'
    'run_and_grep candidate_artifact_draft_compare_snapshots "ArchiveSnapshot comparison|result: same|candidate_artifact_draft_count" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive\n'
)

print("Applied v29.0 CandidateArtifactDraft snapshot/smoke coverage.")
PY

echo
echo "Run validation:"
echo "  make test"
echo "  make CXXSTD=c++17 test"
echo "  make strict"
echo "  make smoke"
echo
echo "If validation passes:"
echo "  git add src/archive_snapshot_model.h src/archive_snapshot.cpp scripts/smoke_test_readme_workflows.sh"
echo "  git commit -m \"Add CandidateArtifactDraft snapshot and smoke coverage\""
echo "  git push -u origin v29-0-draft-outline-snapshot-smoke-coverage"
