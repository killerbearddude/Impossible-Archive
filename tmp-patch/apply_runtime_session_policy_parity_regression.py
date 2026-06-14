#!/usr/bin/env python3
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)

path = Path("src/self_tests_driver.inc")
text = path.read_text()

helper = r'''void run_v29_3_runtime_session_policy_parity_tests(int& failures) {
    auto require = [&](bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << "\n";
        }
    };

    const std::vector<std::string> cli_runtime_read_only_queries{
        "report",
        "candidate-artifact-plan-summary",
        "validate-candidate-artifact-plans",
        "list-candidate-artifact-plans",
        "show-candidate-artifact-plan",
        "candidate-artifact-plan-evaluation-summary",
        "validate-candidate-artifact-plan-evaluations",
        "list-candidate-artifact-plan-evaluations",
        "show-candidate-artifact-plan-evaluation",
        "candidate-artifact-proposal-summary",
        "validate-candidate-artifact-proposals",
        "list-candidate-artifact-proposals",
        "show-candidate-artifact-proposal",
        "candidate-artifact-proposal-audit-summary",
        "validate-candidate-artifact-proposal-audits",
        "list-candidate-artifact-proposal-audits",
        "show-candidate-artifact-proposal-audit",
        "candidate-artifact-draft-summary",
        "validate-candidate-artifact-drafts",
        "list-candidate-artifact-drafts",
        "show-candidate-artifact-draft",
        "candidate-artifact-draft-review-summary",
        "validate-candidate-artifact-draft-reviews",
        "list-candidate-artifact-draft-reviews",
        "show-candidate-artifact-draft-review",
        "control-layer-audit-summary",
        "validate-control-layer-audit",
        "list-control-layer-audit-entries",
        "show-control-layer-audit-entry",
        "knowledge-horizon-summary",
        "validate-knowledge-horizon",
        "list-knowledge-horizon-findings",
        "show-knowledge-horizon-finding",
        "evidence-potential-summary",
        "validate-evidence-potentials",
        "list-evidence-potentials",
        "show-evidence-potential",
        "contradiction-budget-summary",
        "validate-contradiction-budget",
        "list-contradiction-budget-buckets",
        "show-contradiction-budget-bucket",
    };

    for (const std::string& query : cli_runtime_read_only_queries) {
        require(classify_runtime_session_query(query) == RuntimeSessionQueryPolicy::AllowedReadOnly,
                "v29.3 RuntimeSession core policy should allow CLI read-only query: " + query);
    }

    require(classify_runtime_session_query("materialize-hidden-cluster") == RuntimeSessionQueryPolicy::DeniedMutating,
            "v29.3 RuntimeSession policy parity test should preserve mutating query rejection");
    require(classify_runtime_session_query("definitely-not-a-query") == RuntimeSessionQueryPolicy::DeniedUnknown,
            "v29.3 RuntimeSession policy parity test should preserve unknown query rejection");
}

'''

text = replace_once(
    text,
    'void run_v27_runtime_default_selection_tests(int& failures) {',
    helper + 'void run_v27_runtime_default_selection_tests(int& failures) {',
    'helper insertion point',
)

text = replace_once(
    text,
    '    if (failures == 0) {',
    '    run_v29_3_runtime_session_policy_parity_tests(failures);\n\n    if (failures == 0) {',
    'test call insertion point',
)

path.write_text(text)
