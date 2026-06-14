#!/usr/bin/env python3
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)

cli_path = Path("src/cli.cpp")
cli = cli_path.read_text()

allowlist_old = '''           query == "evidence-potential-summary" ||
           query == "validate-evidence-potentials" ||
           query == "contradiction-budget-summary" ||
           query == "validate-contradiction-budget";'''
allowlist_new = '''           query == "evidence-potential-summary" ||
           query == "validate-evidence-potentials" ||
           query == "list-evidence-potentials" ||
           query == "show-evidence-potential" ||
           query == "contradiction-budget-summary" ||
           query == "validate-contradiction-budget" ||
           query == "list-contradiction-budget-buckets" ||
           query == "show-contradiction-budget-bucket";'''
cli = replace_once(cli, allowlist_old, allowlist_new, "RuntimeSession read-only allowlist")

formatter_old = '''    if (options.query == "evidence-potential-summary") {
        return FormattedCommandResult{format_evidence_potential_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-evidence-potentials") {
        return FormattedCommandResult{format_evidence_potential_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "contradiction-budget-summary") {
        return FormattedCommandResult{format_contradiction_budget_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-contradiction-budget") {
        return FormattedCommandResult{format_contradiction_budget_validation(state, options.access), EXIT_SUCCESS};
    }'''
formatter_new = '''    if (options.query == "evidence-potential-summary") {
        return FormattedCommandResult{format_evidence_potential_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-evidence-potentials") {
        return FormattedCommandResult{format_evidence_potential_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-evidence-potentials") {
        return FormattedCommandResult{format_evidence_potential_list(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-evidence-potential") {
        return FormattedCommandResult{format_evidence_potential_detail(state, options.access, options.evidence_potential_id), EXIT_SUCCESS};
    }
    if (options.query == "contradiction-budget-summary") {
        return FormattedCommandResult{format_contradiction_budget_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-contradiction-budget") {
        return FormattedCommandResult{format_contradiction_budget_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-contradiction-budget-buckets") {
        return FormattedCommandResult{format_contradiction_budget_buckets(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-contradiction-budget-bucket") {
        return FormattedCommandResult{format_contradiction_budget_bucket_detail(state, options.access, options.contradiction_budget_bucket_id), EXIT_SUCCESS};
    }'''
cli = replace_once(cli, formatter_old, formatter_new, "RuntimeSession read-only formatter")
cli_path.write_text(cli)

smoke_path = Path("scripts/smoke_test_cli_workflows.sh")
smoke = smoke_path.read_text()
smoke_insert = '''run_session_and_grep runtime_session_evidence_potential_list_and_detail "RuntimeSession initialized|EvidencePotentials visible to curator|EvidencePotential:|- found: true|RuntimeSession ended" "--access curator --query list-evidence-potentials
--access curator --query show-evidence-potential --evidence-potential-id evidence_potential.0000
end-session
" "$BIN" --session --runtime fixed-fixture
run_session_and_grep runtime_session_contradiction_budget_list_and_detail "RuntimeSession initialized|ContradictionBudget buckets visible to curator|ContradictionBudget bucket:|- found: true|RuntimeSession ended" "--access curator --query list-contradiction-budget-buckets
--access curator --query show-contradiction-budget-bucket --contradiction-budget-bucket-id contradiction_budget.archive
end-session
" "$BIN" --session --runtime fixed-fixture
'''
smoke_anchor = '''run_session_and_grep runtime_session_candidate_plan_evaluation_summary_and_validation "RuntimeSession initialized|CandidateArtifactPlanEvaluation summary|CandidateArtifactPlanEvaluation validation|RuntimeSession ended" "--query candidate-artifact-plan-evaluation-summary
'''
if smoke_insert in smoke:
    raise SystemExit("coverage-7 smoke tests already present")
smoke = replace_once(smoke, smoke_anchor, smoke_insert + smoke_anchor, "RuntimeSession smoke anchor")
smoke_path.write_text(smoke)
