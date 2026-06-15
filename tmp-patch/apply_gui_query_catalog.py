#!/usr/bin/env python3
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)

cli_path = Path("src/cli.cpp")
cli = cli_path.read_text()

catalog_code = r'''struct GuiQueryCatalogEntry {
    std::string_view query;
    std::string_view label;
    std::string_view category;
    std::string_view default_access;
    std::string_view required_flag;
    std::string_view example_value;
};

[[nodiscard]] std::vector<GuiQueryCatalogEntry> gui_query_catalog_entries() {
    return {
        {"report", "Archive report", "Overview", "public", "", ""},
        {"candidate-artifact-plan-summary", "CandidateArtifactPlan summary", "Candidate artifact plan", "public", "", ""},
        {"validate-candidate-artifact-plans", "Validate CandidateArtifactPlans", "Candidate artifact plan", "public", "", ""},
        {"list-candidate-artifact-plans", "List CandidateArtifactPlans", "Candidate artifact plan", "curator", "", ""},
        {"show-candidate-artifact-plan", "Show CandidateArtifactPlan", "Candidate artifact plan", "curator", "--candidate-artifact-plan-id", "candidate_artifact_plan.0000"},
        {"candidate-artifact-plan-evaluation-summary", "CandidateArtifactPlanEvaluation summary", "Candidate artifact plan evaluation", "public", "", ""},
        {"validate-candidate-artifact-plan-evaluations", "Validate CandidateArtifactPlanEvaluations", "Candidate artifact plan evaluation", "public", "", ""},
        {"list-candidate-artifact-plan-evaluations", "List CandidateArtifactPlanEvaluations", "Candidate artifact plan evaluation", "curator", "", ""},
        {"show-candidate-artifact-plan-evaluation", "Show CandidateArtifactPlanEvaluation", "Candidate artifact plan evaluation", "curator", "--candidate-artifact-plan-evaluation-id", "candidate_artifact_plan_evaluation.0000"},
        {"candidate-artifact-proposal-summary", "CandidateArtifactProposal summary", "Candidate artifact proposal", "public", "", ""},
        {"validate-candidate-artifact-proposals", "Validate CandidateArtifactProposals", "Candidate artifact proposal", "public", "", ""},
        {"list-candidate-artifact-proposals", "List CandidateArtifactProposals", "Candidate artifact proposal", "curator", "", ""},
        {"show-candidate-artifact-proposal", "Show CandidateArtifactProposal", "Candidate artifact proposal", "curator", "--candidate-artifact-proposal-id", "candidate_artifact_proposal.0000"},
        {"candidate-artifact-proposal-audit-summary", "CandidateArtifactProposalAudit summary", "Candidate artifact proposal audit", "public", "", ""},
        {"validate-candidate-artifact-proposal-audits", "Validate CandidateArtifactProposalAudits", "Candidate artifact proposal audit", "public", "", ""},
        {"list-candidate-artifact-proposal-audits", "List CandidateArtifactProposalAudits", "Candidate artifact proposal audit", "curator", "", ""},
        {"show-candidate-artifact-proposal-audit", "Show CandidateArtifactProposalAudit", "Candidate artifact proposal audit", "curator", "--candidate-artifact-proposal-audit-id", "candidate_artifact_proposal_audit.0000"},
        {"candidate-artifact-draft-summary", "CandidateArtifactDraft summary", "Candidate artifact draft", "public", "", ""},
        {"validate-candidate-artifact-drafts", "Validate CandidateArtifactDrafts", "Candidate artifact draft", "public", "", ""},
        {"list-candidate-artifact-drafts", "List CandidateArtifactDrafts", "Candidate artifact draft", "curator", "", ""},
        {"show-candidate-artifact-draft", "Show CandidateArtifactDraft", "Candidate artifact draft", "curator", "--candidate-artifact-draft-id", "candidate_artifact_draft.0000"},
        {"candidate-artifact-draft-review-summary", "CandidateArtifactDraftReview summary", "Candidate artifact draft review", "public", "", ""},
        {"validate-candidate-artifact-draft-reviews", "Validate CandidateArtifactDraftReviews", "Candidate artifact draft review", "public", "", ""},
        {"list-candidate-artifact-draft-reviews", "List CandidateArtifactDraftReviews", "Candidate artifact draft review", "curator", "", ""},
        {"show-candidate-artifact-draft-review", "Show CandidateArtifactDraftReview", "Candidate artifact draft review", "curator", "--candidate-artifact-draft-review-id", "candidate_artifact_draft_review.0000"},
        {"control-layer-audit-summary", "ControlLayerAudit summary", "Control layer audit", "public", "", ""},
        {"validate-control-layer-audit", "Validate ControlLayerAudit", "Control layer audit", "public", "", ""},
        {"list-control-layer-audit-entries", "List ControlLayerAudit entries", "Control layer audit", "curator", "", ""},
        {"show-control-layer-audit-entry", "Show ControlLayerAudit entry", "Control layer audit", "curator", "--control-layer-audit-entry-id", "control_layer.cli"},
        {"knowledge-horizon-summary", "KnowledgeHorizon summary", "Knowledge horizon", "public", "", ""},
        {"validate-knowledge-horizon", "Validate KnowledgeHorizon", "Knowledge horizon", "public", "", ""},
        {"list-knowledge-horizon-findings", "List KnowledgeHorizon findings", "Knowledge horizon", "curator", "", ""},
        {"show-knowledge-horizon-finding", "Show KnowledgeHorizon finding", "Knowledge horizon", "curator", "--knowledge-horizon-finding-id", "knowledge_horizon.0039"},
        {"evidence-potential-summary", "EvidencePotential summary", "Evidence potential", "public", "", ""},
        {"validate-evidence-potentials", "Validate EvidencePotentials", "Evidence potential", "public", "", ""},
        {"list-evidence-potentials", "List EvidencePotentials", "Evidence potential", "curator", "", ""},
        {"show-evidence-potential", "Show EvidencePotential", "Evidence potential", "curator", "--evidence-potential-id", "evidence_potential.0000"},
        {"contradiction-budget-summary", "ContradictionBudget summary", "Contradiction budget", "public", "", ""},
        {"validate-contradiction-budget", "Validate ContradictionBudget", "Contradiction budget", "public", "", ""},
        {"list-contradiction-budget-buckets", "List ContradictionBudget buckets", "Contradiction budget", "curator", "", ""},
        {"show-contradiction-budget-bucket", "Show ContradictionBudget bucket", "Contradiction budget", "curator", "--contradiction-budget-bucket-id", "contradiction_budget.archive"},
    };
}

[[nodiscard]] std::string format_gui_query_catalog() {
    std::ostringstream out;
    const std::vector<GuiQueryCatalogEntry> entries = gui_query_catalog_entries();
    out << "GUI query catalog:\n";
    out << "- mode: wrapper-safe read-only CLI invocations\n";
    out << "- default_runtime: fixed-fixture\n";
    out << "- entries: " << entries.size() << "\n";
    for (const GuiQueryCatalogEntry& entry : entries) {
        out << "\n";
        out << "query: " << entry.query << "\n";
        out << "label: " << entry.label << "\n";
        out << "category: " << entry.category << "\n";
        out << "default_access: " << entry.default_access << "\n";
        out << "runtime_modes: fixed-fixture,spec-selected\n";
        if (entry.required_flag.empty()) {
            out << "required_options: none\n";
        } else {
            out << "required_options: " << entry.required_flag << "\n";
            out << "example_value: " << entry.example_value << "\n";
        }
        out << "argv: --runtime fixed-fixture --access " << entry.default_access << " --query " << entry.query;
        if (!entry.required_flag.empty()) {
            out << " " << entry.required_flag << " " << entry.example_value;
        }
        out << "\n";
    }
    return out.str();
}

'''

cli = replace_once(
    cli,
    'struct FormattedCommandResult {\n    std::string text;\n    int exit_code = EXIT_SUCCESS;\n};\n\n',
    'struct FormattedCommandResult {\n    std::string text;\n    int exit_code = EXIT_SUCCESS;\n};\n\n' + catalog_code,
    'catalog helper insertion',
)

cli = replace_once(
    cli,
    '[--query report|truth|artifacts|claims|contradictions|anachronisms|timeline|validation|theories|discoveries|mysteries|originality|candidate|materialize|generate-candidates|evaluate-dossier|materialize-generated|materialize-dossier-candidate|plan-dossier-selection|materialize-dossier-selection|hidden-proposals|evaluate-hidden-proposal|plan-hidden-proposal|hidden-cluster|materialize-hidden-cluster|hidden-mutations|generate-artifacts-from-hidden-mutation|materialize-hidden-mutation-artifact-candidate|list-generation-targets|validate-civilization-specs|list-civilization-specs|show-civilization-spec|list-civilization-tags|list-civilizations-by-tag|validate-civilization-tags|list-civilization-fragments|show-civilization-fragment|validate-civilization-fragments|bootstrap-civilization|list-golden-fixtures|show-golden-fixture|archive-snapshot|compare-archive-snapshots|list-evidence-potentials|show-evidence-potential|validate-evidence-potentials|evidence-potential-summary|validate-knowledge-horizon|knowledge-horizon-summary|list-knowledge-horizon-findings|show-knowledge-horizon-finding|contradiction-budget-summary|list-contradiction-budget-buckets|show-contradiction-budget-bucket|validate-contradiction-budget|candidate-artifact-plan-summary|list-candidate-artifact-plans|show-candidate-artifact-plan|validate-candidate-artifact-plans|candidate-artifact-plan-evaluation-summary|list-candidate-artifact-plan-evaluations|show-candidate-artifact-plan-evaluation|validate-candidate-artifact-plan-evaluations|candidate-artifact-proposal-summary|list-candidate-artifact-proposals|show-candidate-artifact-proposal|validate-candidate-artifact-proposals|candidate-artifact-proposal-audit-summary|list-candidate-artifact-proposal-audits|show-candidate-artifact-proposal-audit|validate-candidate-artifact-proposal-audits|control-layer-audit-summary|list-control-layer-audit-entries|show-control-layer-audit-entry|validate-control-layer-audit]',
    '[--query gui-query-catalog|report|truth|artifacts|claims|contradictions|anachronisms|timeline|validation|theories|discoveries|mysteries|originality|candidate|materialize|generate-candidates|evaluate-dossier|materialize-generated|materialize-dossier-candidate|plan-dossier-selection|materialize-dossier-selection|hidden-proposals|evaluate-hidden-proposal|plan-hidden-proposal|hidden-cluster|materialize-hidden-cluster|hidden-mutations|generate-artifacts-from-hidden-mutation|materialize-hidden-mutation-artifact-candidate|list-generation-targets|validate-civilization-specs|list-civilization-specs|show-civilization-spec|list-civilization-tags|list-civilizations-by-tag|validate-civilization-tags|list-civilization-fragments|show-civilization-fragment|validate-civilization-fragments|bootstrap-civilization|list-golden-fixtures|show-golden-fixture|archive-snapshot|compare-archive-snapshots|list-evidence-potentials|show-evidence-potential|validate-evidence-potentials|evidence-potential-summary|validate-knowledge-horizon|knowledge-horizon-summary|list-knowledge-horizon-findings|show-knowledge-horizon-finding|contradiction-budget-summary|list-contradiction-budget-buckets|show-contradiction-budget-bucket|validate-contradiction-budget|candidate-artifact-plan-summary|list-candidate-artifact-plans|show-candidate-artifact-plan|validate-candidate-artifact-plans|candidate-artifact-plan-evaluation-summary|list-candidate-artifact-plan-evaluations|show-candidate-artifact-plan-evaluation|validate-candidate-artifact-plan-evaluations|candidate-artifact-proposal-summary|list-candidate-artifact-proposals|show-candidate-artifact-proposal|validate-candidate-artifact-proposals|candidate-artifact-proposal-audit-summary|list-candidate-artifact-proposal-audits|show-candidate-artifact-proposal-audit|validate-candidate-artifact-proposal-audits|control-layer-audit-summary|list-control-layer-audit-entries|show-control-layer-audit-entry|validate-control-layer-audit]',
    'usage query list',
)

cli = replace_once(
    cli,
    '    if (options.query == "help") {\n        const std::string_view exe = args.empty() ? std::string_view{"impossible_archive_mvp"} : args.front();\n        std::cout << usage(exe);\n        return EXIT_SUCCESS;\n    }\n\n',
    '    if (options.query == "help") {\n        const std::string_view exe = args.empty() ? std::string_view{"impossible_archive_mvp"} : args.front();\n        std::cout << usage(exe);\n        return EXIT_SUCCESS;\n    }\n\n    if (options.query == "gui-query-catalog") {\n        std::cout << format_gui_query_catalog();\n        return EXIT_SUCCESS;\n    }\n\n',
    'gui-query-catalog dispatch',
)

cli_path.write_text(cli)

smoke_path = Path("scripts/smoke_test_cli_workflows.sh")
smoke = smoke_path.read_text()
smoke = replace_once(
    smoke,
    'run_and_grep self_test "All self-tests passed" "$BIN" --self-test\n',
    'run_and_grep self_test "All self-tests passed" "$BIN" --self-test\nrun_and_grep gui_query_catalog "GUI query catalog:|query: knowledge-horizon-summary|argv: --runtime fixed-fixture --access curator --query show-evidence-potential --evidence-potential-id evidence_potential.0000" "$BIN" --query gui-query-catalog\n',
    'smoke insertion',
)
smoke_path.write_text(smoke)
