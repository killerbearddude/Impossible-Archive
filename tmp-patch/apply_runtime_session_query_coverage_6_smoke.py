#!/usr/bin/env python3
from pathlib import Path

path = Path("scripts/smoke_test_cli_workflows.sh")
text = path.read_text()
insert = '''run_session_and_grep runtime_session_candidate_proposal_summary_and_validation "RuntimeSession initialized|CandidateArtifactProposal summary|CandidateArtifactProposal validation|RuntimeSession ended" "--query candidate-artifact-proposal-summary
--query validate-candidate-artifact-proposals
end-session
" "$BIN" --session
run_session_and_grep runtime_session_candidate_proposal_audit_summary_and_validation "RuntimeSession initialized|CandidateArtifactProposalAudit summary|CandidateArtifactProposalAudit validation|RuntimeSession ended" "--query candidate-artifact-proposal-audit-summary
--query validate-candidate-artifact-proposal-audits
end-session
" "$BIN" --session
'''
anchor = '''run_session_and_grep runtime_session_candidate_plan_summary_and_validation "RuntimeSession initialized|CandidateArtifactPlan summary|CandidateArtifactPlan validation|RuntimeSession ended" "--query candidate-artifact-plan-summary
'''
if insert in text:
    raise SystemExit("smoke coverage already present")
if anchor not in text:
    raise SystemExit("anchor not found")
path.write_text(text.replace(anchor, insert + anchor, 1))
