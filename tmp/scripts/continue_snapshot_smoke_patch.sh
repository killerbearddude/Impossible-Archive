#!/usr/bin/env bash
set -euo pipefail
python3 - <<'PY'
from pathlib import Path

def add_before(path, marker, block, sentinel):
    p = Path(path)
    s = p.read_text()
    if sentinel in s:
        return
    lines = s.splitlines(True)
    for i, line in enumerate(lines):
        if marker in line:
            lines[i:i] = [x + '\n' for x in block.split('\n') if x]
            p.write_text(''.join(lines))
            return
    raise SystemExit(f'marker not found: {path}: {marker}')

def add_after(path, marker, block, sentinel):
    p = Path(path)
    s = p.read_text()
    if sentinel in s:
        return
    lines = s.splitlines(True)
    for i, line in enumerate(lines):
        if marker in line:
            lines[i+1:i+1] = [x + '\n' for x in block.split('\n') if x]
            p.write_text(''.join(lines))
            return
    raise SystemExit(f'marker not found: {path}: {marker}')

add_before('src/archive_snapshot.cpp',
    'before.control_layer_audit_entry_count == after.control_layer_audit_entry_count',
    '''                       before.candidate_artifact_draft_count == after.candidate_artifact_draft_count &&
                       before.candidate_artifact_draft_ready_count == after.candidate_artifact_draft_ready_count &&
                       before.candidate_artifact_draft_blocked_count == after.candidate_artifact_draft_blocked_count &&
                       before.candidate_artifact_draft_review_count == after.candidate_artifact_draft_review_count &&
                       before.candidate_artifact_draft_revision_count == after.candidate_artifact_draft_revision_count &&
                       before.candidate_artifact_draft_mutation_enabled_count == after.candidate_artifact_draft_mutation_enabled_count &&''',
    'before.candidate_artifact_draft_count == after.candidate_artifact_draft_count')

add_before('src/archive_snapshot.cpp',
    'format_count_delta(out, "control_layer_audit_entry_count"',
    '''    format_count_delta(out, "candidate_artifact_draft_count", before.candidate_artifact_draft_count, after.candidate_artifact_draft_count);
    format_count_delta(out, "candidate_artifact_draft_ready_count", before.candidate_artifact_draft_ready_count, after.candidate_artifact_draft_ready_count);
    format_count_delta(out, "candidate_artifact_draft_blocked_count", before.candidate_artifact_draft_blocked_count, after.candidate_artifact_draft_blocked_count);
    format_count_delta(out, "candidate_artifact_draft_review_count", before.candidate_artifact_draft_review_count, after.candidate_artifact_draft_review_count);
    format_count_delta(out, "candidate_artifact_draft_revision_count", before.candidate_artifact_draft_revision_count, after.candidate_artifact_draft_revision_count);
    format_count_delta(out, "candidate_artifact_draft_mutation_enabled_count", before.candidate_artifact_draft_mutation_enabled_count, after.candidate_artifact_draft_mutation_enabled_count);''',
    'format_count_delta(out, "candidate_artifact_draft_count"')

add_after('scripts/smoke_test_readme_workflows.sh',
    'candidate_artifact_proposal_audit_snapshot',
    '''
run_and_grep candidate_artifact_draft_summary "CandidateArtifactDraft summary|total_drafts: [1-9]|current_artifact_insertion_enabled: 0" "$BIN" --query candidate-artifact-draft-summary
run_and_grep candidate_artifact_draft_validation "CandidateArtifactDraft validation|result: passed" "$BIN" --query validate-candidate-artifact-drafts
run_and_grep candidate_artifact_draft_list "CandidateArtifactDrafts visible to curator|candidate_artifact_draft\\." "$BIN" --runtime fixed-fixture --access curator --query list-candidate-artifact-drafts
run_and_grep candidate_artifact_draft_detail "CandidateArtifactDraft detail:|- found: true|proposal_id:|audit_id:|Claim outline lines:|Required validation gates:" "$BIN" --runtime fixed-fixture --access curator --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_reject_grep candidate_artifact_draft_public_detail_blocked "CandidateArtifactDraft detail:|- found: false" "proposal_id:|audit_id:|source_evidence_potential_id:|Source chain IDs|Claim outline lines:|Required validation gates:" "$BIN" --runtime fixed-fixture --query show-candidate-artifact-draft --candidate-artifact-draft-id candidate_artifact_draft.candidate_artifact_proposal.candidate_artifact_plan_evaluation.candidate_artifact_plan.evidence_potential.0000.administrative_docket
run_and_grep candidate_artifact_draft_snapshot "candidate_artifact_draft_count: [1-9]|candidate_artifact_draft_mutation_enabled_count: 0" "$BIN" --query archive-snapshot --fixture-id fixture.default_archive
run_and_grep candidate_artifact_draft_compare_snapshots "ArchiveSnapshot comparison|result: same|candidate_artifact_draft_count" "$BIN" --query compare-archive-snapshots --fixture-id fixture.default_archive''',
    'candidate_artifact_draft_summary')
print('completed continuation patch')
PY
