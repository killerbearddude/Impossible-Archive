#!/usr/bin/env bash
set -euo pipefail

cat > src/candidate_artifact_draft_review_model.h <<'EOF'
#pragma once
#include "candidate_artifact_draft_model.h"

namespace archive {

enum class CandidateArtifactDraftReviewDecision {
    Pass,
    NeedsRevision,
    NeedsCuratorReview,
    Blocked,
    Invalid,
};

struct CandidateArtifactDraftReview {
    std::string id;
    std::string draft_id;
    std::string proposal_id;
    std::string audit_id;
    CandidateArtifactDraftReviewDecision decision = CandidateArtifactDraftReviewDecision::NeedsCuratorReview;
    double outline_completeness_score = 0.0;
    double traceability_score = 0.0;
    double safety_score = 0.0;
    double specificity_score = 0.0;
    double revision_pressure_score = 0.0;
    std::vector<std::string> reason_codes;
    std::vector<std::string> required_revisions;
    std::vector<std::string> public_safe_summary_lines;
    std::vector<std::string> curator_notes;
    bool current_artifact_insertion_enabled = false;
    bool current_public_claim_insertion_enabled = false;
    bool current_discovery_scheduling_enabled = false;
    bool hidden_truth_mutation_enabled = false;
    bool public_archive_mutation_enabled = false;
    bool persistence_enabled = false;
    bool final_artifact_prose_generation_enabled = false;
};

struct CandidateArtifactDraftReviewReport {
    std::vector<CandidateArtifactDraftReview> reviews;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(CandidateArtifactDraftReviewDecision decision);

} // namespace archive
EOF

cat > src/candidate_artifact_draft_review_api.h <<'EOF'
#pragma once
#include "archive_engine_state.h"
#include "candidate_artifact_draft_review_model.h"

namespace archive {

[[nodiscard]] CandidateArtifactDraftReview review_candidate_artifact_draft(const ArchiveEngineState& state, const CandidateArtifactDraft& draft, AccessLevel access);
[[nodiscard]] CandidateArtifactDraftReviewReport review_candidate_artifact_drafts(const ArchiveEngineState& state, AccessLevel access);
void review_candidate_artifact_drafts_into_state(ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::vector<std::string> validate_candidate_artifact_draft_reviews(const ArchiveEngineState& state);
[[nodiscard]] std::string format_candidate_artifact_draft_review_summary(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_review_validation(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_review_list(const ArchiveEngineState& state, AccessLevel access);
[[nodiscard]] std::string format_candidate_artifact_draft_review_detail(const ArchiveEngineState& state, AccessLevel access, const std::string& review_id);

} // namespace archive
EOF

cat > src/candidate_artifact_draft_review.cpp <<'EOF'
#include "candidate_artifact_draft_review_api.h"
#include "candidate_artifact_draft_api.h"
#include "diagnostic_access_policy.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {

[[nodiscard]] double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }

[[nodiscard]] bool has_mutation_flag(const CandidateArtifactDraft& d) {
    return d.current_artifact_insertion_enabled || d.current_public_claim_insertion_enabled || d.current_discovery_scheduling_enabled || d.hidden_truth_mutation_enabled || d.public_archive_mutation_enabled || d.persistence_enabled;
}

[[nodiscard]] bool has_mutation_flag(const CandidateArtifactDraftReview& r) {
    return r.current_artifact_insertion_enabled || r.current_public_claim_insertion_enabled || r.current_discovery_scheduling_enabled || r.hidden_truth_mutation_enabled || r.public_archive_mutation_enabled || r.persistence_enabled || r.final_artifact_prose_generation_enabled;
}

void add_reason(CandidateArtifactDraftReview& r, const std::string& code, const std::string& revision) {
    add_unique_string(r.reason_codes, code);
    if (!revision.empty()) { add_unique_string(r.required_revisions, revision); }
}

[[nodiscard]] std::vector<CandidateArtifactDraft> drafts_for_review(const ArchiveEngineState& state) {
    if (!state.candidate_artifact_drafts.empty()) { return state.candidate_artifact_drafts; }
    return derive_candidate_artifact_drafts(state, AccessLevel::Curator).drafts;
}

void append_counts(std::ostringstream& out, const std::map<std::string, std::size_t>& counts) {
    for (const auto& [label, count] : counts) { out << "- " << label << ": " << count << "\n"; }
}

} // namespace

[[nodiscard]] std::string to_string(CandidateArtifactDraftReviewDecision decision) {
    switch (decision) {
        case CandidateArtifactDraftReviewDecision::Pass: return "pass";
        case CandidateArtifactDraftReviewDecision::NeedsRevision: return "needs_revision";
        case CandidateArtifactDraftReviewDecision::NeedsCuratorReview: return "needs_curator_review";
        case CandidateArtifactDraftReviewDecision::Blocked: return "blocked";
        case CandidateArtifactDraftReviewDecision::Invalid: return "invalid";
    }
    return "unknown";
}

[[nodiscard]] CandidateArtifactDraftReview review_candidate_artifact_draft(const ArchiveEngineState& state, const CandidateArtifactDraft& draft, AccessLevel access) {
    (void)state;
    (void)access;
    CandidateArtifactDraftReview review;
    review.id = "candidate_artifact_draft_review." + draft.id;
    review.draft_id = draft.id;
    review.proposal_id = draft.proposal_id;
    review.audit_id = draft.audit_id;
    review.outline_completeness_score = clamp01((draft.outline_title.empty() ? 0.0 : 0.30) + (draft.claim_outline_lines.empty() ? 0.0 : 0.35) + (draft.required_validation_gates.empty() ? 0.0 : 0.35));
    review.traceability_score = clamp01((draft.proposal_id.empty() ? 0.0 : 0.25) + (draft.audit_id.empty() ? 0.0 : 0.25) + (draft.source_chain_ids.size() >= 2U ? 0.30 : 0.0) + (draft.source_evidence_potential_id.empty() ? 0.0 : 0.20));
    review.specificity_score = clamp01((draft.target_topic.empty() ? 0.0 : 0.25) + (draft.intended_creation_year == 0 ? 0.0 : 0.25) + (draft.intended_discovery_year == 0 ? 0.0 : 0.20) + (draft.outline_title.empty() ? 0.0 : 0.30));
    review.safety_score = has_mutation_flag(draft) ? 0.0 : (draft.touches_protected_mystery ? 0.65 : 1.0);
    review.revision_pressure_score = clamp01((1.0 - review.outline_completeness_score) * 0.35 + (1.0 - review.traceability_score) * 0.25 + (1.0 - review.specificity_score) * 0.20 + (1.0 - review.safety_score) * 0.20);
    if (draft.outline_title.empty()) { add_reason(review, "missing_outline_title", "add an outline title"); }
    if (draft.claim_outline_lines.empty()) { add_reason(review, "missing_claim_outline_lines", "add claim outline lines"); }
    if (draft.required_validation_gates.empty()) { add_reason(review, "missing_validation_gates", "add required validation gates"); }
    if (draft.source_chain_ids.size() < 2U) { add_reason(review, "low_traceability", "complete proposal/audit source chain"); }
    if (review.specificity_score < 0.70) { add_reason(review, "low_specificity", "improve topic/year/title specificity"); }
    if (draft.touches_protected_mystery) { add_reason(review, "protected_mystery", "curator must review protected mystery handling"); }
    if (has_mutation_flag(draft)) { add_reason(review, "mutation_flag_enabled", "disable all mutation/insertion/persistence flags"); }
    if (review.revision_pressure_score >= 0.25) { add_reason(review, "revision_pressure", "reduce revision pressure below review threshold"); }
    if (draft.status != CandidateArtifactDraftStatus::ReadyForOutline) { add_reason(review, "draft_not_ready", "resolve source draft status before review can pass"); }
    if (has_mutation_flag(draft)) { review.decision = CandidateArtifactDraftReviewDecision::Invalid; }
    else if (draft.status == CandidateArtifactDraftStatus::Blocked || draft.status == CandidateArtifactDraftStatus::Invalid) { review.decision = CandidateArtifactDraftReviewDecision::Blocked; }
    else if (draft.status == CandidateArtifactDraftStatus::NeedsRevision || review.revision_pressure_score >= 0.25) { review.decision = CandidateArtifactDraftReviewDecision::NeedsRevision; }
    else if (draft.status == CandidateArtifactDraftStatus::NeedsCuratorReview || draft.touches_protected_mystery) { review.decision = CandidateArtifactDraftReviewDecision::NeedsCuratorReview; }
    else { review.decision = CandidateArtifactDraftReviewDecision::Pass; add_unique_string(review.reason_codes, "outline_review_passed"); }
    add_unique_string(review.public_safe_summary_lines, "Draft review only: " + draft.outline_title);
    add_unique_string(review.curator_notes, "advisory draft review only; no artifact generation or insertion is enabled");
    return review;
}

[[nodiscard]] CandidateArtifactDraftReviewReport review_candidate_artifact_drafts(const ArchiveEngineState& state, AccessLevel access) {
    CandidateArtifactDraftReviewReport report;
    const std::vector<CandidateArtifactDraft> drafts = drafts_for_review(state);
    for (const CandidateArtifactDraft& draft : drafts) { report.reviews.push_back(review_candidate_artifact_draft(state, draft, access)); }
    std::sort(report.reviews.begin(), report.reviews.end(), [](const CandidateArtifactDraftReview& a, const CandidateArtifactDraftReview& b) { return a.id < b.id; });
    ArchiveEngineState validation_state = state;
    validation_state.candidate_artifact_drafts = drafts;
    validation_state.candidate_artifact_draft_reviews = report.reviews;
    report.errors = validate_candidate_artifact_draft_reviews(validation_state);
    return report;
}

void review_candidate_artifact_drafts_into_state(ArchiveEngineState& state, AccessLevel access) {
    (void)access;
    state.candidate_artifact_draft_reviews = review_candidate_artifact_drafts(state, AccessLevel::Curator).reviews;
}

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_draft_reviews(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> seen;
    std::vector<std::string> draft_ids;
    for (const CandidateArtifactDraft& draft : state.candidate_artifact_drafts) { draft_ids.push_back(draft.id); }
    for (const CandidateArtifactDraftReview& review : state.candidate_artifact_draft_reviews) {
        const std::string label = review.id.empty() ? std::string{"<empty review id>"} : review.id;
        if (review.id.empty()) { errors.push_back("CandidateArtifactDraftReview has empty id"); }
        else if (!seen.insert(review.id).second) { errors.push_back("CandidateArtifactDraftReview has duplicate id: " + review.id); }
        if (review.draft_id.empty() || std::find(draft_ids.begin(), draft_ids.end(), review.draft_id) == draft_ids.end()) { errors.push_back("CandidateArtifactDraftReview references missing draft: " + label); }
        if (review.proposal_id.empty()) { errors.push_back("CandidateArtifactDraftReview has empty proposal id: " + label); }
        if (review.audit_id.empty()) { errors.push_back("CandidateArtifactDraftReview has empty audit id: " + label); }
        for (double score : {review.outline_completeness_score, review.traceability_score, review.safety_score, review.specificity_score, review.revision_pressure_score}) { if (score < 0.0 || score > 1.0) { errors.push_back("CandidateArtifactDraftReview score out of range: " + label); } }
        if (has_mutation_flag(review)) { errors.push_back("CandidateArtifactDraftReview enables mutation/generation/persistence: " + label); }
        if (review.decision != CandidateArtifactDraftReviewDecision::Pass && review.required_revisions.empty()) { errors.push_back("CandidateArtifactDraftReview non-pass decision lacks required revisions: " + label); }
        if (review.public_safe_summary_lines.empty()) { errors.push_back("CandidateArtifactDraftReview lacks public-safe summary: " + label); }
    }
    return errors;
}

[[nodiscard]] std::string format_candidate_artifact_draft_review_summary(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReviewReport report = review_candidate_artifact_drafts(state, access);
    std::map<std::string, std::size_t> counts;
    std::size_t enabled = 0;
    for (const CandidateArtifactDraftReview& review : report.reviews) { ++counts[to_string(review.decision)]; if (has_mutation_flag(review)) { ++enabled; } }
    std::ostringstream out;
    out << "CandidateArtifactDraftReview summary:\n";
    out << "- behavior: advisory review only; no artifact text, insertion, discovery, mutation, persistence, resolver, or composition behavior is introduced in v29.1.\n";
    out << "- total_reviews: " << report.reviews.size() << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    out << "- mutation_or_generation_enabled: " << enabled << "\n";
    out << "Decision counts:\n";
    append_counts(out, counts);
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraftReview)) { out << "- details: aggregate-only at this access level; review IDs, scores, reason codes, and required revisions are restricted.\n"; }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_draft_review_validation(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReviewReport report = review_candidate_artifact_drafts(state, access);
    std::ostringstream out;
    out << "CandidateArtifactDraftReview validation:\n- result: " << (report.errors.empty() ? "passed" : "failed") << "\n- reviews: " << report.reviews.size() << "\n- errors: " << report.errors.size() << "\n";
    if (!report.errors.empty() && can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) { for (const std::string& error : report.errors) { out << "- " << error << "\n"; } }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_draft_review_list(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReviewReport report = review_candidate_artifact_drafts(state, access);
    std::ostringstream out;
    out << "CandidateArtifactDraftReviews visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraftReview)) { out << "- total_reviews: " << report.reviews.size() << "\n- details: restricted\n"; return out.str(); }
    for (const CandidateArtifactDraftReview& review : report.reviews) { out << "- " << review.id << ": draft_id=" << review.draft_id << " decision=" << to_string(review.decision) << " revision_pressure=" << review.revision_pressure_score << " mutation_enabled=false\n"; }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_draft_review_detail(const ArchiveEngineState& state, AccessLevel access, const std::string& review_id) {
    const CandidateArtifactDraftReviewReport report = review_candidate_artifact_drafts(state, access);
    const auto it = std::find_if(report.reviews.begin(), report.reviews.end(), [&](const CandidateArtifactDraftReview& review) { return review.id == review_id; });
    std::ostringstream out;
    out << "CandidateArtifactDraftReview detail:\n";
    if (it == report.reviews.end() || !can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraftReview)) { out << "- found: false\n"; return out.str(); }
    out << "- found: true\n- id: " << it->id << "\n- draft_id: " << it->draft_id << "\n- proposal_id: " << it->proposal_id << "\n- audit_id: " << it->audit_id << "\n- decision: " << to_string(it->decision) << "\n";
    out << "- outline_completeness_score: " << it->outline_completeness_score << "\n- traceability_score: " << it->traceability_score << "\n- safety_score: " << it->safety_score << "\n- specificity_score: " << it->specificity_score << "\n- revision_pressure_score: " << it->revision_pressure_score << "\n";
    out << "- current_artifact_insertion_enabled: false\n- current_public_claim_insertion_enabled: false\n- current_discovery_scheduling_enabled: false\n- hidden_truth_mutation_enabled: false\n- public_archive_mutation_enabled: false\n- persistence_enabled: false\n- final_artifact_prose_generation_enabled: false\n";
    for (const std::string& code : it->reason_codes) { out << "- reason_code: " << code << "\n"; }
    for (const std::string& revision : it->required_revisions) { out << "- required_revision: " << revision << "\n"; }
    return out.str();
}

} // namespace archive
EOF

python3 - <<'PY'
from pathlib import Path

def p(path): return Path(path)
def text(path): return p(path).read_text()
def write(path, s): p(path).write_text(s)
def before(path, marker, block):
    s=text(path)
    if block in s: return
    if marker not in s: raise SystemExit(f'marker not found {path}: {marker!r}')
    write(path, s.replace(marker, block+marker, 1))
def after(path, marker, block):
    s=text(path)
    if block in s: return
    if marker not in s: raise SystemExit(f'marker not found {path}: {marker!r}')
    write(path, s.replace(marker, marker+block, 1))

before('src/archive_engine_state.h', '#include "control_layer_audit_model.h"', '#include "candidate_artifact_draft_review_model.h"\n')
before('src/archive_engine_state.h', '    std::vector<ControlLayerAuditEntry> control_layer_audit_entries;', '    std::vector<CandidateArtifactDraftReview> candidate_artifact_draft_reviews;\n')
after('src/impossible_archive.h', '#include "candidate_artifact_draft_api.h"\n', '#include "candidate_artifact_draft_review_model.h"\n#include "candidate_artifact_draft_review_api.h"\n')
before('src/diagnostic_access_policy.h', '    ControlLayerAudit,', '    CandidateArtifactDraftReview,\n')
before('src/cli_model.h', '    std::string control_layer_audit_entry_id;', '    std::string candidate_artifact_draft_review_id;\n')
before('src/cli.cpp', '           query == "control-layer-audit-summary" ||', '             query == "candidate-artifact-draft-review-summary" ||\n             query == "list-candidate-artifact-draft-reviews" ||\n             query == "show-candidate-artifact-draft-review" ||\n             query == "validate-candidate-artifact-draft-reviews" ||\n')
s=text('src/cli.cpp')
s=s.replace('        derive_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);\n        build_control_layer_audit_into_state(result.state);','        derive_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);\n        review_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);\n        build_control_layer_audit_into_state(result.state);')
s=s.replace('    derive_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);\n        build_control_layer_audit_into_state(result.state);','    derive_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);\n    review_candidate_artifact_drafts_into_state(result.state, AccessLevel::Curator);\n        build_control_layer_audit_into_state(result.state);')
write('src/cli.cpp', s)
before('src/cli.cpp', '        } else if (arg == "--control-layer-audit-entry-id") {', '        } else if (arg == "--candidate-artifact-draft-review-id") {\n            options.candidate_artifact_draft_review_id = next("--candidate-artifact-draft-review-id");\n')
after('src/cli.cpp', '        out << format_candidate_artifact_draft_summary(state, access) << "\\n";\n', '        out << format_candidate_artifact_draft_review_summary(state, access) << "\\n";\n')
before('src/cli.cpp', '    } else if (options.query == "control-layer-audit-summary") {', '    } else if (options.query == "candidate-artifact-draft-review-summary") {\n        std::cout << format_candidate_artifact_draft_review_summary(state, options.access);\n    } else if (options.query == "list-candidate-artifact-draft-reviews") {\n        std::cout << format_candidate_artifact_draft_review_list(state, options.access);\n    } else if (options.query == "show-candidate-artifact-draft-review") {\n        std::cout << format_candidate_artifact_draft_review_detail(state, options.access, options.candidate_artifact_draft_review_id);\n    } else if (options.query == "validate-candidate-artifact-draft-reviews") {\n        std::cout << format_candidate_artifact_draft_review_validation(state, options.access);\n')
after('src/validation.cpp', '    std::vector<std::string> candidate_draft_errors = validate_candidate_artifact_drafts(state);\n    errors.insert(errors.end(), candidate_draft_errors.begin(), candidate_draft_errors.end());\n', '\n    std::vector<std::string> candidate_draft_review_errors = validate_candidate_artifact_draft_reviews(state);\n    errors.insert(errors.end(), candidate_draft_review_errors.begin(), candidate_draft_review_errors.end());\n')
print('Applied v29.1 CandidateArtifactDraftReview core layer.')
PY

echo "Run: make test && make CXXSTD=c++17 test && make strict && make smoke"
