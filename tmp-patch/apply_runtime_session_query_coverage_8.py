#!/usr/bin/env python3
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)

cli_path = Path("src/cli.cpp")
cli = cli_path.read_text()

allowlist_old = '''           query == "control-layer-audit-summary" ||
           query == "validate-control-layer-audit" ||
           query == "list-control-layer-audit-entries" ||
           query == "show-control-layer-audit-entry" ||
           query == "evidence-potential-summary" ||'''
allowlist_new = '''           query == "control-layer-audit-summary" ||
           query == "validate-control-layer-audit" ||
           query == "list-control-layer-audit-entries" ||
           query == "show-control-layer-audit-entry" ||
           query == "knowledge-horizon-summary" ||
           query == "validate-knowledge-horizon" ||
           query == "list-knowledge-horizon-findings" ||
           query == "show-knowledge-horizon-finding" ||
           query == "evidence-potential-summary" ||'''
cli = replace_once(cli, allowlist_old, allowlist_new, "RuntimeSession read-only allowlist")

formatter_old = '''    if (options.query == "show-control-layer-audit-entry") {
        return FormattedCommandResult{format_control_layer_audit_entry_detail(state, options.access, options.control_layer_audit_entry_id), EXIT_SUCCESS};
    }
    if (options.query == "evidence-potential-summary") {'''
formatter_new = '''    if (options.query == "show-control-layer-audit-entry") {
        return FormattedCommandResult{format_control_layer_audit_entry_detail(state, options.access, options.control_layer_audit_entry_id), EXIT_SUCCESS};
    }
    if (options.query == "knowledge-horizon-summary") {
        return FormattedCommandResult{format_knowledge_horizon_summary(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "validate-knowledge-horizon") {
        return FormattedCommandResult{format_knowledge_horizon_validation(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "list-knowledge-horizon-findings") {
        return FormattedCommandResult{format_knowledge_horizon_findings(state, options.access), EXIT_SUCCESS};
    }
    if (options.query == "show-knowledge-horizon-finding") {
        return FormattedCommandResult{format_knowledge_horizon_finding_detail(state, options.access, options.knowledge_horizon_finding_id), EXIT_SUCCESS};
    }
    if (options.query == "evidence-potential-summary") {'''
cli = replace_once(cli, formatter_old, formatter_new, "RuntimeSession read-only formatter")
cli_path.write_text(cli)

smoke_path = Path("scripts/smoke_test_cli_workflows.sh")
smoke = smoke_path.read_text()
smoke_insert = '''run_session_and_grep runtime_session_knowledge_horizon_summary_and_validation "RuntimeSession initialized|KnowledgeHorizon summary|KnowledgeHorizon validation|RuntimeSession ended" "--query knowledge-horizon-summary
--query validate-knowledge-horizon
end-session
" "$BIN" --session
run_session_and_grep runtime_session_knowledge_horizon_list_and_detail "RuntimeSession initialized|KnowledgeHorizon findings visible to curator|KnowledgeHorizon finding:|- found: true|RuntimeSession ended" "--access curator --query list-knowledge-horizon-findings
--access curator --query show-knowledge-horizon-finding --knowledge-horizon-finding-id knowledge_horizon.0039
end-session
" "$BIN" --session --runtime fixed-fixture
'''
smoke_anchor = '''run_session_and_grep runtime_session_evidence_potential_list_and_detail "RuntimeSession initialized|EvidencePotentials visible to curator|EvidencePotential:|- found: true|RuntimeSession ended" "--access curator --query list-evidence-potentials
'''
if smoke_insert in smoke:
    raise SystemExit("coverage-8 smoke tests already present")
smoke = replace_once(smoke, smoke_anchor, smoke_insert + smoke_anchor, "RuntimeSession smoke anchor")
smoke_path.write_text(smoke)
